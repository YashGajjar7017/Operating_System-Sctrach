/**
 * @file gui_ipc.c
 * @brief Xenithra OS — Kernel GUI IPC Named Pipe Server Implementation
 *
 * Replaces the legacy C framebuffer compositor entirely.
 * All GUI events (mouse, keyboard, window state, service telemetry,
 * RTOS stats) are serialized as newline-delimited JSON and streamed
 * over a Named Pipe to the Node.js/Electron desktop shell.
 *
 * Architecture:
 *   kernel/gui_ipc  ←→  "\\\\.\\pipe\\XenithraGUI"  ←→  ipc_bridge.cjs
 *                                                          (Node.js / Electron)
 *
 * REMOVED (legacy paths this file replaces):
 *   - compositor_init() / compositor_render() / compositor_tick()
 *   - gui_fill_rect / gui_draw_string / gui_put_pixel  (all C drawing)
 *   - v8_engine_init / v8_engine_tick                  (fake V8 stub)
 *   - sys_launch_django_kiosk()                        (Django backend)
 */

#include "gui_ipc.h"
#include "../kstring.h"

/* ------------------------------------------------------------------ */
/* Internal Ring Buffer                                                */
/* ------------------------------------------------------------------ */

#define IPC_RING_SIZE      64
#define IPC_PIPE_NAME      "\\\\.\\pipe\\XenithraGUI"
#define IPC_MAX_HANDLERS   16

typedef struct {
    char msg[GUI_IPC_MAX_MSG_LEN];
    uint32_t len;
} IpcMessage;

static IpcMessage  g_ring[IPC_RING_SIZE];
static uint32_t    g_ring_head    = 0;
static uint32_t    g_ring_tail    = 0;
static uint8_t     g_connected    = 0;
static uint64_t    g_msg_seq      = 0;

/* Command dispatch table */
static struct {
    GuiIpcCmdType        type;
    GuiIpcCommandHandler handler;
} g_handlers[IPC_MAX_HANDLERS];
static int g_handler_count = 0;

/* ------------------------------------------------------------------ */
/* Minimal kernel-level pipe handle (stub for bare-metal; real        */
/* Named Pipe is managed by the ipc_bridge.cjs side over the kernel   */
/* ABI). In a hosted/QEMU dev environment this writes to serial port  */
/* 0x3F8 (COM1) for cross-process forwarding by the host tool.        */
/* ------------------------------------------------------------------ */

/* COM1 UART registers (bare-metal dev output) */
#define COM1_DATA   0x3F8
#define COM1_LSR    0x3FD
#define COM1_LSR_THRE 0x20

static inline void outb_ipc(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb_ipc(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/** Write one byte to COM1 (blocks until UART TX FIFO ready). */
static void uart_putc(char c) {
    while (!(inb_ipc(COM1_LSR) & COM1_LSR_THRE)) {
        __asm__ volatile ("pause");
    }
    outb_ipc(COM1_DATA, (uint8_t)c);
}

/** Write a null-terminated string to COM1. */
static void uart_puts(const char *s) {
    while (s && *s) {
        uart_putc(*s++);
    }
}

/* ------------------------------------------------------------------ */
/* Internal Helpers                                                    */
/* ------------------------------------------------------------------ */

/** Push a JSON string into the outbound ring buffer. */
static void ipc_ring_push(const char *json, uint32_t len) {
    uint32_t next_tail = (g_ring_tail + 1) % IPC_RING_SIZE;
    if (next_tail == g_ring_head) {
        /* Ring full — drop oldest message (overflow protection) */
        g_ring_head = (g_ring_head + 1) % IPC_RING_SIZE;
    }
    uint32_t copy_len = len < GUI_IPC_MAX_MSG_LEN - 2 ? len : GUI_IPC_MAX_MSG_LEN - 2;
    kmemcpy(g_ring[g_ring_tail].msg, json, copy_len);
    g_ring[g_ring_tail].msg[copy_len]     = '\n';
    g_ring[g_ring_tail].msg[copy_len + 1] = '\0';
    g_ring[g_ring_tail].len = copy_len + 1;
    g_ring_tail = next_tail;
}

/** Flush ring buffer to UART / Named Pipe. */
static void ipc_ring_flush(void) {
    while (g_ring_head != g_ring_tail) {
        uart_puts(g_ring[g_ring_head].msg);
        g_ring_head = (g_ring_head + 1) % IPC_RING_SIZE;
    }
}

/** Tiny itoa for unsigned 32-bit integers into buf. Returns length. */
static int ipc_u32toa(uint32_t val, char *buf) {
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    char tmp[12];
    int  i = 0;
    while (val) { tmp[i++] = '0' + (val % 10); val /= 10; }
    int len = i;
    for (int j = 0; j < i; j++) buf[j] = tmp[i - 1 - j];
    buf[len] = '\0';
    return len;
}

/** Append quoted JSON string field. */
static int ipc_append_str(char *dst, int off, int max,
                           const char *key, const char *val) {
    /* "key":"val" */
    dst[off++] = '"';
    for (const char *k = key; *k && off < max - 2; k++) dst[off++] = *k;
    dst[off++] = '"'; dst[off++] = ':'; dst[off++] = '"';
    for (const char *v = val; *v && off < max - 2; v++) dst[off++] = *v;
    dst[off++] = '"';
    return off;
}

/** Append integer JSON field. */
static int ipc_append_int(char *dst, int off, int max,
                           const char *key, uint32_t val) {
    /* "key":val */
    dst[off++] = '"';
    for (const char *k = key; *k && off < max - 2; k++) dst[off++] = *k;
    dst[off++] = '"'; dst[off++] = ':';
    char num[12];
    int nlen = ipc_u32toa(val, num);
    for (int i = 0; i < nlen && off < max - 1; i++) dst[off++] = num[i];
    return off;
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

void gui_ipc_init(void) {
    g_ring_head    = 0;
    g_ring_tail    = 0;
    g_connected    = 1;   /* assume Electron will connect over COM1/pipe */
    g_msg_seq      = 0;
    g_handler_count = 0;

    /* Announce kernel IPC server online */
    uart_puts("{\"type\":\"ipc\",\"action\":\"init\","
              "\"version\":\"XenithraOS-3.0\","
              "\"pipe\":\"XenithraGUI\"}\n");
}

void gui_ipc_tick(void) {
    /* Flush pending outbound messages */
    ipc_ring_flush();

    /* TODO: In a full kernel implementation, poll the inbound FIFO
     * from the Named Pipe / virtio-serial for commands from Electron.
     * For now, COM1 is one-directional (kernel → Electron). */
}

uint8_t gui_ipc_is_connected(void) {
    return g_connected;
}

/* ------------------------------------------------------------------ */
/* Outbound Events                                                     */
/* ------------------------------------------------------------------ */

void gui_ipc_send_mouse_event(int x, int y,
                               uint8_t left, uint8_t right, uint8_t middle) {
    char buf[GUI_IPC_MAX_MSG_LEN];
    int  off = 0;
    buf[off++] = '{';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "type", "mouse");
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "x", (uint32_t)x);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "y", (uint32_t)y);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "l", left);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "r", right);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "m", middle);
    buf[off++] = '}';
    buf[off] = '\0';
    ipc_ring_push(buf, off);
}

void gui_ipc_send_key_event(char ascii, uint8_t scancode, uint8_t is_pressed) {
    char buf[GUI_IPC_MAX_MSG_LEN];
    int  off = 0;
    buf[off++] = '{';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "type", "key");
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "ascii", (uint8_t)ascii);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "scan", scancode);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "pressed", is_pressed);
    buf[off++] = '}';
    buf[off] = '\0';
    ipc_ring_push(buf, off);
}

void gui_ipc_send_window_event(const char *action, const char *app_tag,
                                const char *title) {
    char buf[GUI_IPC_MAX_MSG_LEN];
    int  off = 0;
    buf[off++] = '{';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "type", "window");
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "action", action);
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "tag", app_tag);
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "title", title);
    buf[off++] = '}';
    buf[off] = '\0';
    ipc_ring_push(buf, off);
}

void gui_ipc_send_service_event(const char *name, uint32_t cpu_pct,
                                 uint32_t mem_kb, const char *status) {
    char buf[GUI_IPC_MAX_MSG_LEN];
    int  off = 0;
    buf[off++] = '{';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "type", "service");
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "name", name);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "cpu", cpu_pct);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "mem", mem_kb);
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "status", status);
    buf[off++] = '}';
    buf[off] = '\0';
    ipc_ring_push(buf, off);
}

void gui_ipc_send_rtos_event(uint32_t tid, uint8_t priority,
                              const char *state, uint32_t cpu_us,
                              uint8_t cpu_affinity) {
    char buf[GUI_IPC_MAX_MSG_LEN];
    int  off = 0;
    buf[off++] = '{';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "type", "rtos");
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "tid", tid);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "prio", priority);
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "state", state);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "cpu_us", cpu_us);
    buf[off++] = ',';
    off = ipc_append_int(buf, off, GUI_IPC_MAX_MSG_LEN, "affinity", cpu_affinity);
    buf[off++] = '}';
    buf[off] = '\0';
    ipc_ring_push(buf, off);
}

void gui_ipc_send_notification(const char *title, const char *body,
                                const char *icon_tag) {
    char buf[GUI_IPC_MAX_MSG_LEN];
    int  off = 0;
    buf[off++] = '{';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "type", "notify");
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "title", title);
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "body", body);
    buf[off++] = ',';
    off = ipc_append_str(buf, off, GUI_IPC_MAX_MSG_LEN, "icon", icon_tag);
    buf[off++] = '}';
    buf[off] = '\0';
    ipc_ring_push(buf, off);
}

void gui_ipc_send_raw(const char *json) {
    if (!json) return;
    uint32_t len = 0;
    while (json[len] && len < GUI_IPC_MAX_MSG_LEN - 2) len++;
    ipc_ring_push(json, len);
}

/* ------------------------------------------------------------------ */
/* Inbound Command Dispatch                                            */
/* ------------------------------------------------------------------ */

void gui_ipc_register_handler(GuiIpcCmdType type, GuiIpcCommandHandler handler) {
    if (g_handler_count >= IPC_MAX_HANDLERS) return;
    g_handlers[g_handler_count].type    = type;
    g_handlers[g_handler_count].handler = handler;
    g_handler_count++;
}
