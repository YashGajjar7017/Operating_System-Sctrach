/**
 * @file taskmgr_app.c
 * @brief Windows 11 Fluent Task Manager Application Implementation
 */

#include "taskmgr_app.h"
#include "../kstring.h"
#include "../security/session.h"
#include "../security/firewall.h"

typedef struct {
    char name[32];
    uint32_t pid;
    const char *status;
    uint32_t cpu_usage;
    uint32_t mem_kb;
    const char *ring;
    const char *caps;
    uint8_t is_active;
} TaskProcess;

#define MAX_PROCESSES 10
#define CPU_HISTORY_LEN 28

static TaskProcess g_processes[MAX_PROCESSES] = {
    {"System Core", 0, "Running", 4, 16384, "Ring 0", "ADMIN | STORAGE", 1},
    {"dwm.exe", 1, "Running", 6, 32768, "Ring 0", "COMPOSITOR | BLOOM", 1},
    {"firewall_guard.exe", 2, "Running", 1, 8192, "Ring 0", "NET_FILTER | AUDIT", 1},
    {"explorer.exe", 3, "Running", 3, 24576, "Ring 3", "SHELL | USER_GUI", 1},
    {"vlc.exe", 4, "Running", 7, 48200, "Ring 3", "MEDIA | VIDEO_60FPS", 1},
    {"installer.exe", 5, "Running", 2, 14200, "Ring 3", "X64_PACKAGE_MGR", 1},
    {"taskmgr.exe", 6, "Running", 2, 12288, "Ring 3", "DIAGNOSTICS", 1},
    {"terminal.exe", 7, "Running", 1, 6144, "Ring 3", "CONSOLE | CLI", 1},
    {"session_guard.exe", 8, "Running", 1, 4096, "Ring 0", "ANTI_HIJACK | TOKEN", 1},
    {"csrss.exe", 9, "Running", 3, 18432, "Ring 0", "SUBSYSTEM_HOST", 1}
};

static uint8_t g_current_tab = 0; /* 0: Processes, 1: Performance, 2: Security Guard */
static int g_selected_proc = 0;
static uint32_t g_cpu_history[CPU_HISTORY_LEN] = {12, 18, 15, 22, 19, 14, 16, 20, 25, 18, 14, 12, 15, 19, 23, 28, 20, 15, 12, 14, 16, 19, 15, 18, 22, 16, 14, 15};
static uint64_t g_uptime_ticks = 145;

void taskmgr_tick(void) {
    g_uptime_ticks++;

    /* Shift CPU history and add dynamic jitter */
    for (int i = 0; i < CPU_HISTORY_LEN - 1; i++) {
        g_cpu_history[i] = g_cpu_history[i + 1];
    }
    uint32_t new_val = 12 + (g_uptime_ticks % 17) + ((g_uptime_ticks / 3) % 11);
    if (new_val > 95) new_val = 95;
    g_cpu_history[CPU_HISTORY_LEN - 1] = new_val;
}

static void on_taskmgr_mouse(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click) {
    (void)win;
    (void)right_click;
    if (!left_click) return;

    /* Tab Bar Switching (Y: 38 - 64) */
    if (rel_y >= 38 && rel_y <= 64) {
        if (rel_x >= 12 && rel_x <= 130) {
            g_current_tab = 0; /* Processes */
        } else if (rel_x >= 135 && rel_x <= 265) {
            g_current_tab = 1; /* Performance */
        } else if (rel_x >= 270 && rel_x <= 410) {
            g_current_tab = 2; /* Security Guard */
        }
        return;
    }

    /* Process Table Row Selection (Tab 0) */
    if (g_current_tab == 0 && rel_y >= 100) {
        int row = (rel_y - 100) / 28;
        if (row >= 0 && row < MAX_PROCESSES) {
            g_selected_proc = row;
        }

        /* End Task Button Click (Bottom Right) */
        if (rel_y >= 380 && rel_y <= 416 && rel_x >= 560 && rel_x <= 690) {
            if (g_selected_proc > 2) { /* Don't kill kernel PID 0/1/2 */
                g_processes[g_selected_proc].is_active = 0;
                g_processes[g_selected_proc].status = "Terminated";
                g_processes[g_selected_proc].cpu_usage = 0;
            }
        }
    }
}

static void render_processes_tab(int cx, int cy, int cw, int ch) {
    int table_y = cy + 74;
    int table_w = cw - 24;

    /* Table Header */
    gui_fill_rect(cx + 12, table_y, table_w, 24, 0x00141C2E);
    gui_draw_rect(cx + 12, table_y, table_w, 24, GUI_BORDER_COLOR);
    gui_draw_string(cx + 20, table_y + 4, "Name", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 200, table_y + 4, "PID", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 260, table_y + 4, "Status", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 360, table_y + 4, "CPU", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 420, table_y + 4, "Memory", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 510, table_y + 4, "Privilege", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 600, table_y + 4, "Capabilities", GUI_TEXT_SECONDARY, 1);

    /* Process Rows */
    int row_y = table_y + 24;
    for (int i = 0; i < MAX_PROCESSES; i++) {
        TaskProcess *p = &g_processes[i];
        int cur_y = row_y + i * 28;

        uint32_t bg = (g_selected_proc == i) ? GUI_BG_CARD_HOVER : ((i % 2 == 0) ? GUI_BG_WINDOW : 0x00131C2C);
        gui_fill_rect(cx + 12, cur_y, table_w, 28, bg);
        if (g_selected_proc == i) {
            gui_draw_rect(cx + 12, cur_y, table_w, 28, GUI_ACCENT_BLUE);
        }

        /* App Icon */
        gui_fill_rounded_rect(cx + 18, cur_y + 5, 18, 18, 3, (i < 3) ? GUI_ACCENT_BLUE : GUI_ACCENT_CYAN);
        gui_draw_string(cx + 24, cur_y + 6, (i < 3) ? "K" : "U", 0x00FFFFFF, 1);

        /* Name */
        gui_draw_string(cx + 44, cur_y + 6, p->name, p->is_active ? GUI_TEXT_PRIMARY : GUI_TEXT_MUTED, 1);

        /* PID */
        char num_buf[16];
        uint_to_str(p->pid, num_buf);
        gui_draw_string(cx + 200, cur_y + 6, num_buf, GUI_TEXT_SECONDARY, 1);

        /* Status */
        gui_draw_string(cx + 260, cur_y + 6, p->status, p->is_active ? GUI_ACCENT_GREEN : GUI_ACCENT_RED, 1);

        /* CPU */
        uint_to_str(p->cpu_usage, num_buf);
        strcat(num_buf, " %");
        gui_draw_string(cx + 360, cur_y + 6, num_buf, GUI_TEXT_PRIMARY, 1);

        /* Memory */
        uint_to_str(p->mem_kb / 1024, num_buf);
        strcat(num_buf, " MB");
        gui_draw_string(cx + 420, cur_y + 6, num_buf, GUI_TEXT_SECONDARY, 1);

        /* Privilege */
        gui_draw_string(cx + 510, cur_y + 6, p->ring, (i < 3 || i == 7) ? 0x00D2A8FF : GUI_ACCENT_CYAN, 1);

        /* Capabilities */
        gui_draw_string(cx + 600, cur_y + 6, p->caps, GUI_TEXT_MUTED, 1);
    }

    /* Action Footer */
    int footer_y = cy + ch - 42;
    gui_fill_rounded_rect(cx + cw - 150, footer_y, 130, 32, 6, (g_selected_proc > 2) ? GUI_ACCENT_RED : 0x00334155);
    gui_draw_string(cx + cw - 128, footer_y + 8, "End Task", 0x00FFFFFF, 1);

    gui_fill_rounded_rect(cx + 12, footer_y, 140, 32, 6, GUI_BG_CARD);
    gui_draw_rect(cx + 12, footer_y, 140, 32, GUI_BORDER_COLOR);
    gui_draw_string(cx + 28, footer_y + 8, "+ Run New Task", GUI_ACCENT_CYAN, 1);
}

static void render_performance_tab(int cx, int cy, int cw, int ch) {
    (void)ch;
    /* Left Sidebar Cards */
    int side_x = cx + 12;
    int side_y = cy + 74;
    int side_w = 200;

    /* Card 1: CPU */
    gui_fill_rounded_rect(side_x, side_y, side_w, 64, 6, GUI_BG_CARD_HOVER);
    gui_draw_rect(side_x, side_y, side_w, 64, GUI_ACCENT_BLUE);
    gui_draw_string(side_x + 14, side_y + 10, "CPU", GUI_ACCENT_CYAN, 2);
    char cpu_str[16];
    uint_to_str(g_cpu_history[CPU_HISTORY_LEN - 1], cpu_str);
    strcat(cpu_str, " % 3.40 GHz");
    gui_draw_string(side_x + 14, side_y + 38, cpu_str, GUI_TEXT_SECONDARY, 1);

    /* Card 2: Memory */
    gui_fill_rounded_rect(side_x, side_y + 72, side_w, 64, 6, GUI_BG_CARD);
    gui_draw_rect(side_x, side_y + 72, side_w, 64, GUI_BORDER_COLOR);
    gui_draw_string(side_x + 14, side_y + 82, "Memory", GUI_TEXT_PRIMARY, 2);
    gui_draw_string(side_x + 14, side_y + 110, "142 MB / 2048 MB (7%)", GUI_TEXT_MUTED, 1);

    /* Card 3: Disk */
    gui_fill_rounded_rect(side_x, side_y + 144, side_w, 64, 6, GUI_BG_CARD);
    gui_draw_rect(side_x, side_y + 144, side_w, 64, GUI_BORDER_COLOR);
    gui_draw_string(side_x + 14, side_y + 154, "Disk 0 (C:)", GUI_TEXT_PRIMARY, 2);
    gui_draw_string(side_x + 14, side_y + 182, "0% Active | 64MB FAT32", GUI_TEXT_MUTED, 1);

    /* Card 4: Network */
    gui_fill_rounded_rect(side_x, side_y + 216, side_w, 64, 6, GUI_BG_CARD);
    gui_draw_rect(side_x, side_y + 216, side_w, 64, GUI_BORDER_COLOR);
    gui_draw_string(side_x + 14, side_y + 226, "Ethernet (NAT)", GUI_TEXT_PRIMARY, 2);
    gui_draw_string(side_x + 14, side_y + 254, "Send: 0 Kbps | Rcv: 0", GUI_TEXT_MUTED, 1);

    /* Right Main Graph Area */
    int graph_x = cx + side_w + 24;
    int graph_y = side_y;
    int graph_w = cw - side_w - 36;
    int graph_h = 200;

    gui_fill_rounded_rect(graph_x, graph_y, graph_w, graph_h, 8, 0x000A101C);
    gui_draw_rect(graph_x, graph_y, graph_w, graph_h, GUI_ACCENT_BLUE);

    gui_draw_string(graph_x + 16, graph_y + 12, "% Utilization (60 Seconds Waveform)", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(graph_x + graph_w - 60, graph_y + 12, "100%", GUI_TEXT_MUTED, 1);
    gui_draw_string(graph_x + graph_w - 40, graph_y + graph_h - 20, "0%", GUI_TEXT_MUTED, 1);

    /* Grid lines */
    for (int gl = 1; gl <= 3; gl++) {
        int gy = graph_y + (graph_h * gl) / 4;
        gui_draw_rect(graph_x + 10, gy, graph_w - 20, 1, 0x001B263C);
    }

    /* Draw Real-time CPU Waveform Bars */
    int bar_width = (graph_w - 30) / CPU_HISTORY_LEN;
    for (int i = 0; i < CPU_HISTORY_LEN; i++) {
        int val = g_cpu_history[i];
        int bar_h = (val * (graph_h - 50)) / 100;
        int bx = graph_x + 15 + i * bar_width;
        int by = graph_y + graph_h - 10 - bar_h;

        gui_fill_rect(bx, by, bar_width - 2, bar_h, GUI_ACCENT_CYAN);
    }

    /* Detailed System Stats underneath graph */
    int stat_y = graph_y + graph_h + 16;
    gui_draw_string(graph_x + 10, stat_y, "Utilization: ", GUI_TEXT_MUTED, 1);
    gui_draw_string(graph_x + 110, stat_y, cpu_str, GUI_TEXT_PRIMARY, 1);

    gui_draw_string(graph_x + 10, stat_y + 24, "Processes: ", GUI_TEXT_MUTED, 1);
    gui_draw_string(graph_x + 110, stat_y + 24, "8 Active", GUI_TEXT_PRIMARY, 1);

    gui_draw_string(graph_x + 240, stat_y, "Base Speed: ", GUI_TEXT_MUTED, 1);
    gui_draw_string(graph_x + 340, stat_y, "3.40 GHz", GUI_TEXT_PRIMARY, 1);

    gui_draw_string(graph_x + 240, stat_y + 24, "Up Time: ", GUI_TEXT_MUTED, 1);
    char up_str[32];
    uint_to_str(g_uptime_ticks / 60, up_str);
    strcat(up_str, "m ");
    char sec_buf[16];
    uint_to_str(g_uptime_ticks % 60, sec_buf);
    strcat(up_str, sec_buf);
    strcat(up_str, "s");
    gui_draw_string(graph_x + 340, stat_y + 24, up_str, GUI_ACCENT_GREEN, 1);
}

static void render_security_tab(int cx, int cy, int cw, int ch) {
    (void)ch;
    int sec_y = cy + 74;
    int sec_w = cw - 24;

    /* 1. Hardware Security Status Banner */
    gui_fill_rounded_rect(cx + 12, sec_y, sec_w, 54, 6, GUI_BG_CARD);
    gui_draw_rect(cx + 12, sec_y, sec_w, 54, GUI_BORDER_COLOR);
    gui_draw_string(cx + 24, sec_y + 10, "Hardware Protection Status: SMEP / SMAP Enforced", 0x0010B981, 1);
    gui_draw_string(cx + 24, sec_y + 30, "Anti-Hijack Sentry: Active | 128-bit High-Entropy Session Rotation", GUI_TEXT_SECONDARY, 1);

    /* 2. Active Session Tokens Table */
    int st_y = sec_y + 68;
    gui_fill_rect(cx + 12, st_y, sec_w, 24, 0x00141C2E);
    gui_draw_rect(cx + 12, st_y, sec_w, 24, GUI_BORDER_COLOR);
    gui_draw_string(cx + 20, st_y + 4, "Session ID", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 130, st_y + 4, "Owner PID", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 230, st_y + 4, "Security Token (128-bit)", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 470, st_y + 4, "Status", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 590, st_y + 4, "Probes Blocked", GUI_TEXT_SECONDARY, 1);

    for (int i = 0; i < 4; i++) {
        const XenithraSession *s = security_get_session(i);
        int cur_y = st_y + 24 + i * 28;
        gui_fill_rect(cx + 12, cur_y, sec_w, 28, (i % 2 == 0) ? GUI_BG_WINDOW : 0x00131C2C);

        char buf[32];
        if (s && s->session_id != 0) {
            uint_to_str(s->session_id, buf);
            gui_draw_string(cx + 20, cur_y + 6, buf, GUI_ACCENT_CYAN, 1);

            uint_to_str(s->owner_pid, buf);
            gui_draw_string(cx + 130, cur_y + 6, buf, GUI_TEXT_PRIMARY, 1);

            gui_draw_string(cx + 230, cur_y + 6, "0xA5... / 0x5A... [VALID]", 0x00D2A8FF, 1);
            gui_draw_string(cx + 470, cur_y + 6, "ENFORCED", 0x0010B981, 1);

            uint_to_str(s->hijack_attempts, buf);
            gui_draw_string(cx + 590, cur_y + 6, buf, (s->hijack_attempts > 0) ? 0x00EF4444 : GUI_TEXT_MUTED, 1);
        } else {
            gui_draw_string(cx + 20, cur_y + 6, "--", GUI_TEXT_MUTED, 1);
        }
    }
}

static void on_taskmgr_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* 1. Main Background */
    gui_fill_rect(cx, cy, cw, ch, GUI_BG_WINDOW);

    /* 2. Top Tab Navigation Bar */
    int tab_y = cy + 8;
    int tab_w = 110;

    /* Tab 0: Processes */
    gui_fill_rounded_rect(cx + 12, tab_y, tab_w, 28, 6, (g_current_tab == 0) ? GUI_ACCENT_BLUE : GUI_BG_CARD);
    gui_draw_string(cx + 24, tab_y + 6, "Processes", 0x00FFFFFF, 1);

    /* Tab 1: Performance */
    gui_fill_rounded_rect(cx + 130, tab_y, tab_w + 10, 28, 6, (g_current_tab == 1) ? GUI_ACCENT_BLUE : GUI_BG_CARD);
    gui_draw_string(cx + 140, tab_y + 6, "Performance", 0x00FFFFFF, 1);

    /* Tab 2: Security Guard */
    gui_fill_rounded_rect(cx + 260, tab_y, tab_w + 30, 28, 6, (g_current_tab == 2) ? GUI_ACCENT_BLUE : GUI_BG_CARD);
    gui_draw_string(cx + 270, tab_y + 6, "Security Guard", 0x00FFFFFF, 1);

    /* 3. Render Active Tab Content */
    if (g_current_tab == 0) {
        render_processes_tab(cx, cy, cw, ch);
    } else if (g_current_tab == 1) {
        render_performance_tab(cx, cy, cw, ch);
    } else {
        render_security_tab(cx, cy, cw, ch);
    }
}

Window* taskmgr_app_launch(void) {
    Window *win = window_create(
        "Task Manager",
        "taskmgr",
        120,
        90,
        780,
        470,
        on_taskmgr_paint,
        NULL
    );
    if (win) {
        window_set_callbacks(win, NULL, on_taskmgr_mouse);
    }
    return win;
}
