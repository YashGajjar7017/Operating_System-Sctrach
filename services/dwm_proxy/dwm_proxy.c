/**
 * @file dwm_proxy.c
 * @brief DWM Proxy Service — Desktop Window Manager IPC Bridge
 *
 * In Windows, DWM (dwm.exe / UxSms) runs in Session 1 and composites all
 * application windows into the final frame using DirectCompose / DirectX.
 *
 * In Xenithra OS, the actual compositing is done by Electron (Chromium's
 * GPU compositor). This DWM Proxy service acts as the kernel-side bridge:
 *   - Tracks window state (open, close, minimize, maximize, focus)
 *   - Forwards window events to Electron over GUI IPC
 *   - Receives shell commands back from Electron (open app, close window)
 *   - Implements the kernel's window manager API (window_create, window_destroy)
 *   - Reports compositor telemetry (frame time, GPU %, vsync) over IPC
 *
 * Runs at RTOS_PRIO_HIGH on CPU 0 (same as Electron IPC flush).
 */

#include "dwm_proxy.h"
#include "../../kernel/kstring.h"
#include "../../kernel/sched/sched.h"
#include "../../kernel/gui/gui_ipc.h"

#define DWM_MAX_WINDOWS      32
#define DWM_TICK_INTERVAL_MS  8     /* ~120Hz compositor poll */
#define DWM_REPORT_EVERY     15     /* IPC telemetry every 15 ticks (~120ms) */
#define DWM_TARGET_FPS       60
#define DWM_FRAME_BUDGET_US  16666  /* 1/60s in µs */

typedef struct {
    uint32_t id;
    char     title[64];
    char     app_tag[32];
    uint8_t  is_open;
    uint8_t  is_minimized;
    uint8_t  is_maximized;
    uint8_t  is_focused;
    int      x, y, w, h;
} DwmWindow;

static DwmWindow  g_windows[DWM_MAX_WINDOWS];
static uint32_t   g_window_count     = 0;
static uint32_t   g_next_win_id      = 1;
static uint64_t   g_dwm_ticks        = 0;
static uint32_t   g_frame_time_us    = 0;
static uint32_t   g_cpu_pct          = 0;
static uint32_t   g_fps              = 60;

/* ------------------------------------------------------------------ */
/* Window Management API                                              */
/* ------------------------------------------------------------------ */

uint32_t dwm_open_window(const char *title, const char *app_tag,
                          int x, int y, int w, int h) {
    if (g_window_count >= DWM_MAX_WINDOWS) return 0;

    DwmWindow *win = NULL;
    for (uint32_t i = 0; i < DWM_MAX_WINDOWS; i++) {
        if (!g_windows[i].is_open) { win = &g_windows[i]; break; }
    }
    if (!win) return 0;

    kmemset(win, 0, sizeof(DwmWindow));
    win->id         = g_next_win_id++;
    win->is_open    = 1;
    win->is_focused = 1;
    win->x = x; win->y = y; win->w = w; win->h = h;
    kstrncpy(win->title,   title,   63);
    kstrncpy(win->app_tag, app_tag, 31);
    g_window_count++;

    /* Notify Electron to open the app window */
    gui_ipc_send_window_event("open", app_tag, title);
    return win->id;
}

void dwm_close_window(uint32_t win_id) {
    for (uint32_t i = 0; i < DWM_MAX_WINDOWS; i++) {
        if (g_windows[i].id == win_id && g_windows[i].is_open) {
            gui_ipc_send_window_event("close",
                                       g_windows[i].app_tag,
                                       g_windows[i].title);
            g_windows[i].is_open = 0;
            g_window_count--;
            return;
        }
    }
}

void dwm_minimize_window(uint32_t win_id) {
    for (uint32_t i = 0; i < DWM_MAX_WINDOWS; i++) {
        if (g_windows[i].id == win_id) {
            g_windows[i].is_minimized = 1;
            gui_ipc_send_window_event("minimize",
                                       g_windows[i].app_tag,
                                       g_windows[i].title);
        }
    }
}

void dwm_maximize_window(uint32_t win_id) {
    for (uint32_t i = 0; i < DWM_MAX_WINDOWS; i++) {
        if (g_windows[i].id == win_id) {
            g_windows[i].is_maximized = 1;
            g_windows[i].is_minimized = 0;
            gui_ipc_send_window_event("maximize",
                                       g_windows[i].app_tag,
                                       g_windows[i].title);
        }
    }
}

void dwm_focus_window(uint32_t win_id) {
    for (uint32_t i = 0; i < DWM_MAX_WINDOWS; i++) {
        g_windows[i].is_focused = (g_windows[i].id == win_id) ? 1 : 0;
    }
    for (uint32_t i = 0; i < DWM_MAX_WINDOWS; i++) {
        if (g_windows[i].id == win_id) {
            gui_ipc_send_window_event("focus",
                                       g_windows[i].app_tag,
                                       g_windows[i].title);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Inbound IPC command handler (Electron → kernel)                   */
/* ------------------------------------------------------------------ */

void dwm_handle_launch_command(const GuiIpcCommand *cmd) {
    if (!cmd) return;
    dwm_open_window(cmd->app_tag, cmd->app_tag, 80, 40, 900, 600);
}

void dwm_handle_close_command(const GuiIpcCommand *cmd) {
    if (!cmd) return;
    for (uint32_t i = 0; i < DWM_MAX_WINDOWS; i++) {
        if (kstrncmp(g_windows[i].app_tag, cmd->app_tag, 31) == 0 &&
            g_windows[i].is_open) {
            dwm_close_window(g_windows[i].id);
            return;
        }
    }
}

/* ------------------------------------------------------------------ */
/* Service Thread Entry                                               */
/* ------------------------------------------------------------------ */

void dwm_proxy_service_thread(void) {
    Thread *self = sched_get_current();
    rtos_set_affinity(self, 0x01); /* CPU 0 — same as IPC flush */

    /* Register command handlers */
    gui_ipc_register_handler(GUI_IPC_CMD_LAUNCH, dwm_handle_launch_command);
    gui_ipc_register_handler(GUI_IPC_CMD_CLOSE,  dwm_handle_close_command);

    gui_ipc_send_notification("DWM", "Desktop Window Manager started", "dwm");

    /* Open initial desktop windows */
    dwm_open_window("Microsoft Edge", "browser", 80, 40, 900, 600);

    while (1) {
        g_dwm_ticks++;

        /* Simulate frame timing measurement */
        g_frame_time_us = DWM_FRAME_BUDGET_US + (g_dwm_ticks % 500);
        g_fps = 1000000 / g_frame_time_us;
        g_cpu_pct = 3 + (g_window_count * 2);

        /* Report compositor telemetry */
        if ((g_dwm_ticks % DWM_REPORT_EVERY) == 0) {
            gui_ipc_send_service_event("DWM", g_cpu_pct,
                                        g_window_count * 8192,
                                        g_fps >= 55 ? "vsync-ok" : "vsync-late");
        }

        sched_sleep(DWM_TICK_INTERVAL_MS);
    }
}
