/**
 * @file gui_ipc.h
 * @brief Xenithra OS — Kernel GUI IPC Server
 *
 * Named Pipe server that replaces the legacy C framebuffer compositor.
 * The kernel publishes all GUI events (mouse, keyboard, window state,
 * service telemetry) over "\\\\.\\pipe\\XenithraGUI" to the Node.js/
 * Electron desktop shell process.
 *
 * Event Protocol (newline-delimited JSON):
 *   → Mouse:    {"type":"mouse","x":640,"y":480,"l":0,"r":0,"m":0}
 *   → Key:      {"type":"key","ascii":65,"scan":30,"pressed":1}
 *   → Window:   {"type":"window","action":"open","tag":"browser","title":"Edge"}
 *   → Service:  {"type":"service","name":"SysMain","cpu":2,"mem":18}
 *   → RTOS:     {"type":"rtos","tid":3,"prio":4,"state":"running","cpu_us":340}
 *   → Power:    {"type":"power","action":"shutdown"}
 *
 * Commands received from Electron (inbound):
 *   ← Launch:  {"cmd":"launch","app":"browser","url":"https://google.com"}
 *   ← Close:   {"cmd":"close","app":"terminal"}
 *   ← Power:   {"cmd":"power","action":"shutdown"}
 *   ← Query:   {"cmd":"query","target":"rtos_threads"}
 */

#ifndef _KERNEL_GUI_IPC_H_
#define _KERNEL_GUI_IPC_H_

#include <stdint.h>
#include <stddef.h>

/* Maximum size of a single IPC JSON message */
#define GUI_IPC_MAX_MSG_LEN   512

/* IPC event types */
typedef enum {
    GUI_IPC_EVT_MOUSE       = 0,
    GUI_IPC_EVT_KEY         = 1,
    GUI_IPC_EVT_WINDOW      = 2,
    GUI_IPC_EVT_SERVICE     = 3,
    GUI_IPC_EVT_RTOS        = 4,
    GUI_IPC_EVT_POWER       = 5,
    GUI_IPC_EVT_AUDIO       = 6,
    GUI_IPC_EVT_NOTIFY      = 7,
} GuiIpcEventType;

/* IPC command types (inbound from Electron) */
typedef enum {
    GUI_IPC_CMD_LAUNCH      = 0,
    GUI_IPC_CMD_CLOSE       = 1,
    GUI_IPC_CMD_POWER       = 2,
    GUI_IPC_CMD_QUERY       = 3,
    GUI_IPC_CMD_UNKNOWN     = 255,
} GuiIpcCmdType;

typedef struct {
    GuiIpcCmdType type;
    char          app_tag[32];
    char          url[256];
    char          action[32];
    char          query_target[64];
} GuiIpcCommand;

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

/**
 * @brief Initialize the GUI IPC Named Pipe server.
 *        Opens "\\\\.\\pipe\\XenithraGUI" (server side).
 *        Must be called once during Phase 2 boot, before kshell_init().
 */
void gui_ipc_init(void);

/**
 * @brief Periodic IPC flush — call from main loop every ~16ms.
 *        Drains the outbound ring buffer → pipe.
 *        Reads any inbound commands from Electron and dispatches them.
 */
void gui_ipc_tick(void);

/**
 * @brief Check if Electron client is currently connected.
 * @return 1 if connected, 0 otherwise.
 */
uint8_t gui_ipc_is_connected(void);

/* ------------------------------------------------------------------ */
/* Outbound Events (kernel → Electron)                                */
/* ------------------------------------------------------------------ */

/** Forward a PS/2 mouse event to the Electron shell. */
void gui_ipc_send_mouse_event(int x, int y,
                               uint8_t left, uint8_t right, uint8_t middle);

/** Forward a PS/2 keyboard event to the Electron shell. */
void gui_ipc_send_key_event(char ascii, uint8_t scancode, uint8_t is_pressed);

/** Notify Electron that a kernel window was opened/closed/focused. */
void gui_ipc_send_window_event(const char *action, const char *app_tag,
                                const char *title);

/** Push a service telemetry update (SysMain, MMCSS, AudioSrv, WMI, DWM). */
void gui_ipc_send_service_event(const char *name, uint32_t cpu_pct,
                                 uint32_t mem_kb, const char *status);

/** Push an RTOS thread telemetry snapshot. */
void gui_ipc_send_rtos_event(uint32_t tid, uint8_t priority,
                              const char *state, uint32_t cpu_us,
                              uint8_t cpu_affinity);

/** Send a desktop notification toast to Electron. */
void gui_ipc_send_notification(const char *title, const char *body,
                                const char *icon_tag);

/** Send raw JSON over the IPC pipe (escape hatch for custom events). */
void gui_ipc_send_raw(const char *json);

/* ------------------------------------------------------------------ */
/* Inbound Command Dispatch (Electron → kernel)                       */
/* ------------------------------------------------------------------ */

typedef void (*GuiIpcCommandHandler)(const GuiIpcCommand *cmd);

/**
 * @brief Register a handler for inbound Electron commands.
 *        Commands are dispatched from gui_ipc_tick().
 */
void gui_ipc_register_handler(GuiIpcCmdType type, GuiIpcCommandHandler handler);

#endif /* _KERNEL_GUI_IPC_H_ */
