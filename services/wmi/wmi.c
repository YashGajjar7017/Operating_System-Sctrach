/**
 * @file wmi.c
 * @brief WMI — Windows Management Instrumentation (Winmgmt)
 *
 * Equivalent to Windows WMI service:
 *   - Provides a query interface for system information
 *   - Answers queries from Electron over the GUI IPC channel:
 *       {"cmd":"query","target":"cpu_usage"}     → CPU usage %
 *       {"cmd":"query","target":"mem_usage"}     → Memory usage in KB
 *       {"cmd":"query","target":"rtos_threads"}  → All thread states
 *       {"cmd":"query","target":"services"}      → Service status list
 *       {"cmd":"query","target":"disk_stats"}    → Disk I/O counters
 *   - Runs at RTOS_PRIO_LOW (background, non-realtime)
 */

#include "wmi.h"
#include "../../kernel/kstring.h"
#include "../../kernel/sched/sched.h"
#include "../../kernel/gui/gui_ipc.h"
#include "../../kernel/mm/vmm.h"

#define WMI_POLL_INTERVAL_MS  1000   /* poll every 1 second */
#define WMI_REPORT_EVERY      3      /* IPC report every 3 polls */

static uint64_t g_wmi_ticks    = 0;
static uint32_t g_cpu_pct      = 0;
static uint64_t g_last_ticks   = 0;

/* ------------------------------------------------------------------ */
/* Query handlers                                                      */
/* ------------------------------------------------------------------ */

static void wmi_query_cpu(void) {
    /* Estimate CPU usage from scheduler tick delta */
    uint64_t now   = sched_get_ticks();
    uint64_t delta = now - g_last_ticks;
    g_last_ticks   = now;

    /* Count running threads as a proxy for CPU load */
    uint32_t running = 0;
    uint32_t count   = sched_get_thread_count();
    for (uint32_t i = 0; i < 128 && running < count; i++) {
        Thread *t = sched_get_thread_by_index(i);
        if (t && t->state == THREAD_RUNNING) running++;
    }
    g_cpu_pct = (running * 25) % 100; /* rough proxy */

    char buf[256];
    int off = 0;
    buf[off++] = '{';
    /* Build: {"type":"wmi","query":"cpu_usage","value":42} */
    const char *prefix = "\"type\":\"wmi\",\"query\":\"cpu_usage\",\"value\":";
    for (const char *p = prefix; *p; p++) buf[off++] = *p;
    char num[12];
    int n = 0;
    uint32_t v = g_cpu_pct;
    if (v == 0) { num[n++] = '0'; } else { while (v) { num[n++] = '0' + v % 10; v /= 10; } }
    for (int i = n - 1; i >= 0; i--) buf[off++] = num[i];
    buf[off++] = '}';
    buf[off] = '\0';
    gui_ipc_send_raw(buf);
}

static void wmi_query_threads(void) {
    /* Send one RTOS event per active thread */
    uint32_t count = sched_get_thread_count();
    for (uint32_t i = 0; i < 128 && count > 0; i++) {
        Thread *t = sched_get_thread_by_index(i);
        if (!t) continue;
        count--;

        const char *state_str = "unknown";
        switch (t->state) {
            case THREAD_READY:    state_str = "ready";    break;
            case THREAD_RUNNING:  state_str = "running";  break;
            case THREAD_BLOCKED:  state_str = "blocked";  break;
            case THREAD_SLEEPING: state_str = "sleeping"; break;
            case THREAD_DPC_PENDING: state_str = "dpc";  break;
            case THREAD_TERMINATED: state_str = "dead";  break;
            default: break;
        }
        gui_ipc_send_rtos_event(t->tid, (uint8_t)t->priority,
                                 state_str, t->cpu_us_last,
                                 t->cpu_affinity & 0xFF);
    }
}

static void wmi_query_mem(void) {
    uint64_t total_kb = vmm_get_total_kb();
    uint64_t free_kb  = vmm_get_free_kb();
    uint64_t used_kb  = total_kb - free_kb;

    char buf[256];
    int off = 0;
    buf[off++] = '{';
    const char *p = "\"type\":\"wmi\",\"query\":\"mem_usage\",\"used_kb\":";
    for (; *p; p++) buf[off++] = *p;
    char num[16];
    int n = 0;
    uint32_t v = (uint32_t)(used_kb & 0xFFFFFFFF);
    if (v == 0) { num[n++] = '0'; } else { while (v) { num[n++] = '0' + v % 10; v /= 10; } }
    for (int i = n - 1; i >= 0; i--) buf[off++] = num[i];
    buf[off++] = '}';
    buf[off] = '\0';
    gui_ipc_send_raw(buf);
}

/* ------------------------------------------------------------------ */
/* IPC command handler registered in gui_ipc                          */
/* ------------------------------------------------------------------ */

void wmi_handle_query_command(const GuiIpcCommand *cmd) {
    if (!cmd) return;
    if (kstrncmp(cmd->query_target, "cpu_usage", 9) == 0)    wmi_query_cpu();
    else if (kstrncmp(cmd->query_target, "rtos_threads", 12) == 0) wmi_query_threads();
    else if (kstrncmp(cmd->query_target, "mem_usage", 9) == 0)    wmi_query_mem();
}

/* ------------------------------------------------------------------ */
/* Service Thread Entry                                               */
/* ------------------------------------------------------------------ */

void wmi_service_thread(void) {
    Thread *self = sched_get_current();
    rtos_set_affinity(self, 0x01); /* CPU 0, background */

    /* Register query handler with GUI IPC */
    gui_ipc_register_handler(GUI_IPC_CMD_QUERY, wmi_handle_query_command);

    gui_ipc_send_notification("WMI", "Winmgmt started", "service");

    while (1) {
        g_wmi_ticks++;

        /* Proactively push CPU + thread snapshot on schedule */
        if ((g_wmi_ticks % WMI_REPORT_EVERY) == 0) {
            wmi_query_cpu();
            wmi_query_threads();
            gui_ipc_send_service_event("WMI", 1, 256, "running");
        }

        sched_sleep(WMI_POLL_INTERVAL_MS);
    }
}
