/**
 * @file panelmgr.c — Xenithra OS v3.0 — Panel Management Daemon Implementation
 */

#include "panelmgr.h"
#include "../../kernel/kstring.h"
#include "../../kernel/gui/gui_ipc.h"

static PanelTelemetry g_telemetry;
static PanelLogEntry  g_log_ring[PANELMGR_LOG_RING_SIZE];
static uint32_t       g_log_head = 0;
static uint32_t       g_log_count = 0;
static uint32_t       g_next_log_id = 1;

void panelmgr_init(void) {
    memset(&g_telemetry, 0, sizeof(g_telemetry));
    g_telemetry.cpu_percent       = 12;
    g_telemetry.cpu_freq_mhz      = 3200;
    g_telemetry.cpu_temperature_c = 42;
    g_telemetry.cpu_core_count    = 4;

    g_telemetry.ram_total_bytes   = 2048ULL * 1024 * 1024;
    g_telemetry.ram_used_bytes    = 340ULL * 1024 * 1024;

    g_telemetry.gpu_percent       = 8;
    g_telemetry.gpu_vram_total_mb = 128;
    g_telemetry.gpu_vram_used_mb  = 32;

    g_telemetry.process_count     = 14;
    g_telemetry.thread_count      = 28;
    g_telemetry.handle_count      = 120;

    g_log_head  = 0;
    g_log_count = 0;

    panelmgr_log(PANEL_LOG_INFO, "System", 100, "Xenithra OS v3.0 kernel initialized.");
}

void panelmgr_thread(void) {
    panelmgr_init();
    while (1) {
        __asm__ volatile ("pause");
    }
}

void panelmgr_log(PanelLogLevel level, const char *source,
                  uint32_t event_id, const char *message)
{
    uint32_t idx = (g_log_head + g_log_count) % PANELMGR_LOG_RING_SIZE;
    if (g_log_count < PANELMGR_LOG_RING_SIZE) {
        g_log_count++;
    } else {
        g_log_head = (g_log_head + 1) % PANELMGR_LOG_RING_SIZE;
    }

    PanelLogEntry *e = &g_log_ring[idx];
    e->id = g_next_log_id++;
    e->level = level;
    e->event_id = event_id;
    if (source)  strncpy(e->source, source, sizeof(e->source) - 1);
    if (message) strncpy(e->message, message, sizeof(e->message) - 1);

    gui_ipc_send_notification(source ? source : "System", message ? message : "", "info");
}

void panelmgr_push_telemetry(void) {
    gui_ipc_send_service_event(
        "PanelMgr",
        g_telemetry.cpu_percent,
        (uint32_t)(g_telemetry.ram_used_bytes / 1024),
        "Running"
    );
}

const PanelTelemetry *panelmgr_get_telemetry(void) {
    return &g_telemetry;
}

int panelmgr_get_log(PanelLogEntry *out, int max_entries) {
    if (!out || max_entries <= 0) return 0;
    int count = g_log_count < (uint32_t)max_entries ? (int)g_log_count : max_entries;
    for (int i = 0; i < count; i++) {
        uint32_t idx = (g_log_head + i) % PANELMGR_LOG_RING_SIZE;
        out[i] = g_log_ring[idx];
    }
    return count;
}
