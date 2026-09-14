/**
 * @file v8_engine.h
 * @brief V8 JavaScript Engine & Native Electron/DOM Bridge for Xenithra OS Kernel
 * 
 * Provides bare-metal / kernel-level runtime interfacing for V8 JavaScript execution,
 * WebAssembly runtime hooks, React Reconciler DOM dispatch, and IPC bridge.
 */

#ifndef _KERNEL_GUI_V8_ENGINE_H_
#define _KERNEL_GUI_V8_ENGINE_H_

#include <stdint.h>
#include <stddef.h>
#include "dom_engine.h"

#define V8_MAX_CONTEXTS     8
#define V8_HEAP_SIZE_MB     64
#define V8_MAX_IPC_HANDLERS 32

typedef struct {
    uint32_t heap_total_bytes;
    uint32_t heap_used_bytes;
    uint32_t active_contexts;
    uint32_t total_gc_cycles;
    uint32_t dispatched_events;
    uint8_t  is_initialized;
    uint8_t  jit_compiler_enabled;
} V8EngineStats;

typedef void (*V8IpcCallback)(const char *channel, const char *payload, char *response_out, size_t max_len);

typedef struct {
    char channel[32];
    V8IpcCallback callback;
} V8IpcHandler;

/* V8 Engine Core Lifecycle */
void v8_engine_init(void);
void v8_engine_tick(void);
V8EngineStats v8_engine_get_stats(void);

/* V8 Execution & React Shell Mounting */
uint8_t v8_eval_script(const char *source);
uint8_t v8_mount_react_app(const char *app_tag);

/* V8 IPC Channel (Kernel <-> Electron React Shell) */
void v8_register_ipc_handler(const char *channel, V8IpcCallback cb);
void v8_send_ipc_event(const char *channel, const char *json_payload);

/* Native DOM / UI Reconciler Hooks */
void v8_reconcile_dom(VNode *vnode_root);

#endif /* _KERNEL_GUI_V8_ENGINE_H_ */
