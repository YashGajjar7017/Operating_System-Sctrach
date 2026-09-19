/**
 * @file py_runtime.h
 * @brief Python 3.12 LTS Embedded Runtime & In-Memory Django ASGI Server Engine
 * 
 * Embeds Python 3.12 LTS runtime side-by-side in kernel/system memory, providing
 * bytecode compilation, GIL-free / sub-interpreter execution, and native Django ASGI routing.
 */

#ifndef _KERNEL_PYTHON_PY_RUNTIME_H_
#define _KERNEL_PYTHON_PY_RUNTIME_H_

#include <stdint.h>
#include <stddef.h>

#define PY_VERSION_STR       "Python 3.12.5 LTS (x86_64 Xenithra Kernel Embed)"
#define PY_HEAP_SIZE_MB      128
#define PY_MAX_MODULES       64
#define PY_DEFAULT_PORT      8000

typedef struct {
    char version[64];
    uint32_t heap_total_bytes;
    uint32_t heap_used_bytes;
    uint32_t loaded_modules;
    uint32_t executed_bytecodes;
    uint8_t is_initialized;
    uint8_t django_active;
    uint16_t django_port;
} PyRuntimeStats;

typedef struct {
    const char *method;
    const char *path;
    const char *body;
    char *response_out;
    size_t max_resp_len;
} PyHttpRequest;

/* Python 3.12 Core Runtime Lifecycle */
void py_runtime_init(void);
void py_runtime_tick(void);
PyRuntimeStats py_runtime_get_stats(void);

/* In-Memory Django Backend Service */
uint8_t py_runtime_start_django(uint16_t port);
uint8_t py_runtime_stop_django(void);
void py_runtime_dispatch_http(PyHttpRequest *req);

/* Python Bytecode & Script Execution */
uint8_t py_runtime_eval(const char *python_code);
uint8_t py_runtime_exec_file(const char *filepath);

#endif /* _KERNEL_PYTHON_PY_RUNTIME_H_ */
