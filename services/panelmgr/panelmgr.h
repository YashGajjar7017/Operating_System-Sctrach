/**
 * @file panelmgr.h — Xenithra OS v3.0 — Panel Management Daemon
 *
 * Session 0 service that manages the state of UI panels.
 * Acts as a relay between kernel subsystems and the Render Engine:
 *   - Collects performance telemetry (CPU, RAM, GPU, Disk, Net)
 *   - Collects system event log entries
 *   - Pushes updates over GUI IPC at configurable intervals
 *   - Handles panel-specific queries from Electron
 */

#ifndef _SERVICE_PANELMGR_H_
#define _SERVICE_PANELMGR_H_

#include <stdint.h>

/* ── Telemetry snapshot ───────────────────────────────────────────────── */
typedef struct {
    /* CPU */
    uint32_t cpu_percent;       /* 0–100 */
    uint32_t cpu_freq_mhz;
    uint32_t cpu_temperature_c;
    uint32_t cpu_core_count;

    /* Memory */
    uint64_t ram_used_bytes;
    uint64_t ram_total_bytes;
    uint64_t page_fault_count;

    /* GPU */
    uint32_t gpu_percent;
    uint32_t gpu_vram_used_mb;
    uint32_t gpu_vram_total_mb;

    /* Disk */
    uint64_t disk_read_bytes_per_sec;
    uint64_t disk_write_bytes_per_sec;
    uint32_t disk_active_pct;

    /* Network */
    uint64_t net_rx_bytes_per_sec;
    uint64_t net_tx_bytes_per_sec;

    /* Process/Thread counts */
    uint32_t process_count;
    uint32_t thread_count;
    uint32_t handle_count;

    /* Timestamp */
    uint64_t timestamp_ms;
} PanelTelemetry;

/* ── Event log level ─────────────────────────────────────────────────── */
typedef enum {
    PANEL_LOG_INFO     = 0,
    PANEL_LOG_WARNING  = 1,
    PANEL_LOG_ERROR    = 2,
    PANEL_LOG_CRITICAL = 3,
} PanelLogLevel;

/* ── Event log entry ─────────────────────────────────────────────────── */
typedef struct {
    uint32_t      id;
    uint64_t      timestamp_ms;
    PanelLogLevel level;
    char          source[32];
    uint32_t      event_id;
    char          message[256];
} PanelLogEntry;

/* ── Telemetry publish interval ──────────────────────────────────────── */
#define PANELMGR_TELEMETRY_INTERVAL_MS  1000   /* Push telemetry every 1s */
#define PANELMGR_LOG_RING_SIZE          256    /* Circular event log */

/* ── Public API ──────────────────────────────────────────────────────── */

/**
 * @brief Panel Manager daemon thread entry.
 *        Runs as a kernel thread in Session 0.
 *        Collects and publishes telemetry to the Render Engine.
 */
void panelmgr_thread(void);

/** Initialize the panel manager (call before spawning thread). */
void panelmgr_init(void);

/**
 * @brief Log an event to the panel event ring.
 *        Thread-safe — can be called from any kernel context.
 */
void panelmgr_log(PanelLogLevel level, const char *source,
                  uint32_t event_id, const char *message);

/** Force-publish telemetry snapshot now via GUI IPC. */
void panelmgr_push_telemetry(void);

/** Get latest telemetry snapshot (read-only). */
const PanelTelemetry *panelmgr_get_telemetry(void);

/** Get recent log entries. Returns count copied. */
int  panelmgr_get_log(PanelLogEntry *out, int max_entries);

#endif /* _SERVICE_PANELMGR_H_ */
