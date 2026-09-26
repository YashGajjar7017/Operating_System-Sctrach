/**
 * @file sched.h
 * @brief Xenithra OS — RTOS Priority Scheduler + DPC Queue
 *
 * Upgraded from 3-level MLFQ to a 6-level hard real-time priority scheduler:
 *
 *   RTOS_PRIO_IDLE      (0) — Idle thread; runs only when all queues empty
 *   RTOS_PRIO_LOW       (1) — WMI, background prefetch, SysMain housekeeping
 *   RTOS_PRIO_NORMAL    (2) — User apps, SysMain predictor, default kernel work
 *   RTOS_PRIO_HIGH      (3) — DWM proxy, Electron shell bridge, AudioSrv, smss
 *   RTOS_PRIO_REALTIME  (4) — MMCSS audio/video, kshell, ISR bottom-halves
 *   RTOS_PRIO_ISR       (5) — Hardware ISR dispatch only (not schedulable)
 *
 * New capabilities vs old MLFQ:
 *   - Priority Inheritance  — prevents inversion on kernel mutexes
 *   - CPU Affinity Masks    — pin REALTIME threads to dedicated cores
 *   - Deadline Scheduling   — EDF option for MMCSS threads
 *   - DPC Queue             — Deferred Procedure Calls (ISR bottom-halves)
 *   - Watchdog Timer        — detects missed REALTIME deadlines → log + recover
 *   - sched_boost()         — I/O wakeup gives immediate RTOS_PRIO_HIGH slot
 */

#ifndef _KERNEL_SCHED_H_
#define _KERNEL_SCHED_H_

#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------------ */
/* Scheduler Limits                                                    */
/* ------------------------------------------------------------------ */

#define MAX_THREADS          128
#define MAX_PROCESSES         32
#define RTOS_PRIORITY_LEVELS   6
#define THREAD_STACK_SIZE    (64 * 1024)    /* 64 KB kernel stack */
#define FPU_STATE_SIZE        512           /* fxsave64 alignment */
#define MAX_CPU_CORES          16           /* max SMP cores */
#define DPC_QUEUE_DEPTH        64           /* max deferred calls */

/* ------------------------------------------------------------------ */
/* RTOS Priority Levels                                                */
/* ------------------------------------------------------------------ */

typedef enum {
    RTOS_PRIO_IDLE        = 0,  /* Idle thread only */
    RTOS_PRIO_LOW         = 1,  /* WMI, background work */
    RTOS_PRIO_NORMAL      = 2,  /* User apps, SysMain */
    RTOS_PRIO_HIGH        = 3,  /* DWM, AudioSrv, SMSS */
    RTOS_PRIO_REALTIME    = 4,  /* MMCSS, kshell, IPC flush */
    RTOS_PRIO_ISR         = 5,  /* Hardware ISRs (not enqueued) */
} RTOSPriority;

/* Time-slice budgets in scheduler ticks (1 tick ≈ 1ms APIC timer) */
#define RTOS_QUANTUM_IDLE        200   /* 200ms — only runs if nothing else */
#define RTOS_QUANTUM_LOW          20   /* 20ms  */
#define RTOS_QUANTUM_NORMAL       10   /* 10ms  */
#define RTOS_QUANTUM_HIGH          4   /* 4ms   */
#define RTOS_QUANTUM_REALTIME      1   /* 1ms   — preemptive */

static const uint32_t RTOS_QUANTA[RTOS_PRIORITY_LEVELS] = {
    RTOS_QUANTUM_IDLE,
    RTOS_QUANTUM_LOW,
    RTOS_QUANTUM_NORMAL,
    RTOS_QUANTUM_HIGH,
    RTOS_QUANTUM_REALTIME,
    0,   /* ISR — not scheduled */
};

/* ------------------------------------------------------------------ */
/* Thread States                                                       */
/* ------------------------------------------------------------------ */

typedef enum {
    THREAD_EMBRYO      = 0,  /* Being created */
    THREAD_READY       = 1,  /* In a priority run queue */
    THREAD_RUNNING     = 2,  /* Currently executing */
    THREAD_BLOCKED     = 3,  /* Waiting on mutex/semaphore */
    THREAD_SLEEPING    = 4,  /* Timed sleep */
    THREAD_DPC_PENDING = 5,  /* Running a DPC */
    THREAD_TERMINATED  = 6,  /* Dead — slot reusable */
} ThreadState;

/* ------------------------------------------------------------------ */
/* Register Frame (hardware interrupt / context switch)               */
/* ------------------------------------------------------------------ */

typedef struct __attribute__((packed)) {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
} InterruptFrame;

/* ------------------------------------------------------------------ */
/* Thread Control Block (TCB)                                         */
/* ------------------------------------------------------------------ */

typedef struct Thread {
    /* Identity */
    uint32_t           tid;
    uint32_t           pid;
    char               name[32];

    /* Scheduling state */
    ThreadState        state;
    RTOSPriority       priority;          /* Current effective priority */
    RTOSPriority       base_priority;     /* Original priority (before inheritance) */
    uint32_t           time_slice_left;   /* Remaining ticks in current quantum */
    uint32_t           quantum;           /* Full quantum for this priority level */
    uint64_t           sleep_until;       /* Absolute tick count for wakeup */

    /* Deadline scheduling (for MMCSS / RTOS_PRIO_REALTIME) */
    uint64_t           deadline_tick;     /* 0 = not using EDF */
    uint8_t            deadline_missed;   /* Set by watchdog if deadline overrun */

    /* CPU affinity (bit mask: bit 0 = CPU 0, bit 1 = CPU 1, etc.) */
    uint16_t           cpu_affinity;      /* 0 = any CPU */
    uint8_t            last_cpu;          /* CPU that last ran this thread */

    /* Priority inheritance */
    struct Thread     *blocking_thread;   /* Thread we're waiting on */
    uint8_t            inherited_count;   /* Depth of inheritance chain */

    /* Telemetry (reported over GUI IPC) */
    uint64_t           total_cpu_ticks;   /* Cumulative ticks executed */
    uint64_t           last_run_tick;     /* When this thread last ran */
    uint32_t           cpu_us_last;       /* CPU µs in last reporting window */

    /* Hardware context */
    uint64_t           rsp;              /* Preserved kernel stack pointer */
    uint64_t           cr3;             /* PML4 page directory (0 = kernel) */
    uint8_t            fpu_state[FPU_STATE_SIZE] __attribute__((aligned(16)));

    /* Kernel stack */
    uint8_t            stack[THREAD_STACK_SIZE] __attribute__((aligned(16)));

    /* Queue linkage */
    struct Thread     *next;
    struct Thread     *prev;
} Thread;

/* ------------------------------------------------------------------ */
/* Process Control Block (PCB)                                        */
/* ------------------------------------------------------------------ */

typedef struct Process {
    uint32_t           pid;
    char               name[32];
    uint64_t           cr3;             /* PML4 physical address */
    uint32_t           thread_count;
    uint32_t           memory_used_kb;
    uint8_t            is_active;
    RTOSPriority       default_priority;
} Process;

/* ------------------------------------------------------------------ */
/* DPC (Deferred Procedure Call) — ISR bottom-half mechanism          */
/* ------------------------------------------------------------------ */

typedef void (*DpcRoutine)(void *context);

typedef struct {
    DpcRoutine  routine;
    void       *context;
    uint8_t     urgent;   /* 1 = run before next scheduler tick */
} DpcEntry;

/* ------------------------------------------------------------------ */
/* Kernel Mutex (with priority inheritance)                           */
/* ------------------------------------------------------------------ */

typedef struct {
    volatile uint32_t locked;   /* 0 = free, TID of owner if locked */
    Thread           *owner;
    Thread           *waiters;  /* singly-linked wait list */
} KMutex;

/* ------------------------------------------------------------------ */
/* Scheduler Core API                                                 */
/* ------------------------------------------------------------------ */

void      sched_init(void);
Thread   *sched_create_kthread(void (*entry_point)(void), RTOSPriority priority);
Thread   *sched_create_named_kthread(void (*entry_point)(void),
                                      RTOSPriority priority,
                                      const char *name);
void      sched_tick(void);
void      sched_yield(void);
void      sched_sleep(uint64_t milliseconds);
void      sched_exit(void);
Thread   *sched_get_current(void);
uint64_t  sched_get_ticks(void);

/* CPU affinity */
void      rtos_set_affinity(Thread *t, uint16_t cpu_mask);

/* Priority boost (used by I/O wakeup, DPC completion) */
void      rtos_boost(Thread *t, RTOSPriority new_prio);

/* Deadline scheduling */
void      rtos_set_deadline(Thread *t, uint64_t deadline_ms);

/* DPC Queue */
void      dpc_queue_init(void);
void      dpc_queue(DpcRoutine routine, void *context, uint8_t urgent);
void      dpc_drain(void);

/* Mutex (priority-inheriting) */
void      kmutex_init(KMutex *m);
void      kmutex_lock(KMutex *m);
void      kmutex_unlock(KMutex *m);

/* Watchdog */
void      rtos_watchdog_init(uint32_t max_realtime_miss_ticks);
void      rtos_watchdog_tick(void);

/* Telemetry snapshot (used by GUI IPC RTOS panel) */
uint32_t  sched_get_thread_count(void);
Thread   *sched_get_thread_by_index(uint32_t index);

#endif /* _KERNEL_SCHED_H_ */
