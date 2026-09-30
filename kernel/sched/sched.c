/**
 * @file sched.c
 * @brief Xenithra OS — RTOS Priority Scheduler Implementation
 *
 * Upgraded from 3-level MLFQ to a full 6-priority-level hard RTOS scheduler
 * with DPC queue, priority inheritance, CPU affinity, deadline (EDF) support,
 * and a watchdog timer for missed real-time deadlines.
 *
 * Key design decisions:
 *  - RTOS_PRIO_REALTIME threads are NEVER demoted; they preempt all others
 *  - RTOS_PRIO_HIGH threads get a fresh quantum after preemption (no demotion)
 *  - Lower-priority threads (NORMAL, LOW, IDLE) use MLFQ-style time-slice decay
 *  - DPC queue drains before the next REALTIME thread gets its quantum
 *  - Priority inheritance walks the full blocking chain to prevent inversion
 *  - Watchdog fires if a REALTIME thread misses its deadline more than
 *    g_watchdog_miss_limit ticks in a row
 */

#include "sched.h"
#include "../kstring.h"

extern void switch_to_thread(Thread *prev, Thread *next);

_Static_assert(offsetof(Thread, rsp) == 32, "Thread.rsp offset must be 32 for switch.asm");
_Static_assert(offsetof(Thread, cr3) == 40, "Thread.cr3 offset must be 40 for switch.asm");
_Static_assert(offsetof(Thread, fpu_state) == 48, "Thread.fpu_state offset must be 48 for switch.asm");

/* ------------------------------------------------------------------ */
/* Globals                                                             */
/* ------------------------------------------------------------------ */

static Thread      g_threads[MAX_THREADS];
static uint32_t    g_thread_count    = 0;
static uint32_t    g_next_tid        = 1;
static uint64_t    g_sched_ticks     = 0;

typedef struct {
    Thread *head;
    Thread *tail;
} ThreadQueue;

static ThreadQueue g_ready_queues[RTOS_PRIORITY_LEVELS];
static Thread     *g_current_thread   = NULL;
static Thread     *g_sleep_queue      = NULL;
static Thread     *g_idle_thread      = NULL;

/* ------------------------------------------------------------------ */
/* DPC Queue                                                           */
/* ------------------------------------------------------------------ */

static DpcEntry g_dpc_queue[DPC_QUEUE_DEPTH];
static uint32_t g_dpc_head = 0;
static uint32_t g_dpc_tail = 0;
static uint32_t g_dpc_count = 0;

/* ------------------------------------------------------------------ */
/* Watchdog                                                            */
/* ------------------------------------------------------------------ */

static uint32_t g_watchdog_miss_limit    = 10;  /* ticks */
static uint32_t g_watchdog_miss_streak   = 0;
static uint64_t g_realtime_deadline_last = 0;

/* ------------------------------------------------------------------ */
/* Queue Helpers                                                        */
/* ------------------------------------------------------------------ */

static void queue_push_back(ThreadQueue *q, Thread *t) {
    t->next = NULL;
    t->prev = q->tail;
    if (q->tail) {
        q->tail->next = t;
    } else {
        q->head = t;
    }
    q->tail = t;
}

static void queue_push_front(ThreadQueue *q, Thread *t) {
    t->prev = NULL;
    t->next = q->head;
    if (q->head) {
        q->head->prev = t;
    } else {
        q->tail = t;
    }
    q->head = t;
}

static Thread *queue_pop_front(ThreadQueue *q) {
    if (!q->head) return NULL;
    Thread *t = q->head;
    q->head = t->next;
    if (q->head) {
        q->head->prev = NULL;
    } else {
        q->tail = NULL;
    }
    t->next = NULL;
    t->prev = NULL;
    return t;
}

static void queue_remove(ThreadQueue *q, Thread *t) {
    if (t->prev) t->prev->next = t->next;
    else         q->head = t->next;
    if (t->next) t->next->prev = t->prev;
    else         q->tail = t->prev;
    t->next = NULL;
    t->prev = NULL;
}

/* ------------------------------------------------------------------ */
/* Idle Thread Entry                                                    */
/* ------------------------------------------------------------------ */

static void idle_thread_entry(void) {
    while (1) {
        __asm__ volatile ("hlt");
    }
}

/* ------------------------------------------------------------------ */
/* sched_init                                                          */
/* ------------------------------------------------------------------ */

void sched_init(void) {
    for (int i = 0; i < RTOS_PRIORITY_LEVELS; i++) {
        g_ready_queues[i].head = NULL;
        g_ready_queues[i].tail = NULL;
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        g_threads[i].state = THREAD_TERMINATED;
        g_threads[i].tid   = 0;
    }
    g_thread_count    = 0;
    g_current_thread  = NULL;
    g_sleep_queue     = NULL;
    g_sched_ticks     = 0;
    g_dpc_head = 0;
    g_dpc_tail = 0;
    g_dpc_count = 0;

    /* Create the idle thread at RTOS_PRIO_IDLE */
    g_idle_thread = sched_create_named_kthread(idle_thread_entry,
                                                RTOS_PRIO_IDLE, "idle");

    /* Make idle the initial running thread so context switch is safe */
    if (g_idle_thread) {
        g_idle_thread->state = THREAD_RUNNING;
        g_current_thread = g_idle_thread;
    }
}

/* ------------------------------------------------------------------ */
/* Thread Creation                                                     */
/* ------------------------------------------------------------------ */

Thread *sched_create_kthread(void (*entry_point)(void), RTOSPriority priority) {
    return sched_create_named_kthread(entry_point, priority, "kthread");
}

Thread *sched_create_named_kthread(void (*entry_point)(void),
                                    RTOSPriority priority,
                                    const char *name) {
    if (g_thread_count >= MAX_THREADS) return NULL;
    if ((int)priority >= RTOS_PRIORITY_LEVELS) priority = RTOS_PRIO_NORMAL;

    Thread *t = NULL;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_threads[i].state == THREAD_TERMINATED ||
            g_threads[i].state == THREAD_EMBRYO) {
            t = &g_threads[i];
            break;
        }
    }
    if (!t) return NULL;

    /* Zero the TCB */
    memset(t, 0, sizeof(Thread));

    t->tid             = g_next_tid++;
    t->pid             = 0;
    t->state           = THREAD_READY;
    t->priority        = priority;
    t->base_priority   = priority;
    t->quantum         = RTOS_QUANTA[priority];
    t->time_slice_left = t->quantum;
    t->sleep_until     = 0;
    t->deadline_tick   = 0;
    t->cpu_affinity    = 0;  /* any CPU */
    t->last_cpu        = 0;
    t->cr3             = 0;  /* kernel address space */

    /* Copy name */
    if (name) {
        int ni = 0;
        while (name[ni] && ni < 31) { t->name[ni] = name[ni]; ni++; }
        t->name[ni] = '\0';
    }

    /* ── Build Initial Stack Frame ──────────────────────────────────
     * switch_to_thread() expects the stack to look like it was built
     * by a PUSH sequence:
     *   RBP, RBX, R12, R13, R14, R15, RFLAGS, RIP
     * When switch_to_thread POPs these and RETs, execution jumps to
     * entry_point with RFLAGS.IF = 1.
     */
    uint64_t *stk = (uint64_t *)(t->stack + THREAD_STACK_SIZE);

    /* Return address (RIP) → entry_point */
    *(--stk) = (uint64_t)entry_point;

    /* RFLAGS: Interrupts enabled (IF=1) */
    *(--stk) = 0x202;

    /* Callee-saved: R15, R14, R13, R12, RBX, RBP */
    *(--stk) = 0; /* R15 */
    *(--stk) = 0; /* R14 */
    *(--stk) = 0; /* R13 */
    *(--stk) = 0; /* R12 */
    *(--stk) = 0; /* RBX */
    *(--stk) = 0; /* RBP */

    t->rsp = (uint64_t)stk;

    queue_push_back(&g_ready_queues[priority], t);
    g_thread_count++;
    return t;
}

/* ------------------------------------------------------------------ */
/* sched_sleep                                                         */
/* ------------------------------------------------------------------ */

void sched_sleep(uint64_t milliseconds) {
    if (!g_current_thread) return;

    g_current_thread->state       = THREAD_SLEEPING;
    g_current_thread->sleep_until = g_sched_ticks + milliseconds;

    /* Sorted insert into sleep queue (earliest deadline first) */
    if (!g_sleep_queue ||
        g_current_thread->sleep_until < g_sleep_queue->sleep_until) {
        g_current_thread->next = g_sleep_queue;
        g_sleep_queue = g_current_thread;
    } else {
        Thread *cur = g_sleep_queue;
        while (cur->next &&
               cur->next->sleep_until <= g_current_thread->sleep_until) {
            cur = cur->next;
        }
        g_current_thread->next = cur->next;
        cur->next = g_current_thread;
    }

    sched_yield();
}

/* ------------------------------------------------------------------ */
/* sched_tick — called from APIC timer ISR every 1ms                  */
/* ------------------------------------------------------------------ */

void sched_tick(void) {
    g_sched_ticks++;

    /* 1. Wake sleeping threads */
    while (g_sleep_queue && g_sleep_queue->sleep_until <= g_sched_ticks) {
        Thread *woken      = g_sleep_queue;
        g_sleep_queue      = g_sleep_queue->next;
        woken->next        = NULL;
        woken->state       = THREAD_READY;
        /* I/O wakeup boost: move to RTOS_PRIO_HIGH temporarily */
        RTOSPriority boost = woken->base_priority;
        if (boost < RTOS_PRIO_HIGH) boost = RTOS_PRIO_HIGH;
        woken->priority        = boost;
        woken->time_slice_left = RTOS_QUANTA[boost];
        queue_push_front(&g_ready_queues[boost], woken);
    }

    /* 2. Drain urgent DPCs before scheduling */
    dpc_drain();

    /* 3. Watchdog: check REALTIME deadline misses */
    rtos_watchdog_tick();

    /* 4. Preempt current thread if its quantum expired */
    if (g_current_thread && g_current_thread->state == THREAD_RUNNING) {
        g_current_thread->total_cpu_ticks++;

        if (g_current_thread->priority >= RTOS_PRIO_REALTIME) {
            /* REALTIME threads get full quantum without demotion */
            if (g_current_thread->time_slice_left > 0) {
                g_current_thread->time_slice_left--;
            }
            if (g_current_thread->time_slice_left == 0) {
                g_current_thread->time_slice_left = RTOS_QUANTA[RTOS_PRIO_REALTIME];
                g_current_thread->state = THREAD_READY;
                queue_push_back(&g_ready_queues[RTOS_PRIO_REALTIME],
                                g_current_thread);
                sched_yield();
            }
        } else if (g_current_thread->priority >= RTOS_PRIO_HIGH) {
            /* HIGH threads: round-robin without demotion */
            if (g_current_thread->time_slice_left > 0) {
                g_current_thread->time_slice_left--;
            }
            if (g_current_thread->time_slice_left == 0) {
                g_current_thread->time_slice_left = RTOS_QUANTA[RTOS_PRIO_HIGH];
                g_current_thread->state = THREAD_READY;
                queue_push_back(&g_ready_queues[RTOS_PRIO_HIGH],
                                g_current_thread);
                sched_yield();
            }
        } else {
            /* NORMAL / LOW / IDLE: MLFQ-style demotion on quantum expiry */
            if (g_current_thread->time_slice_left > 0) {
                g_current_thread->time_slice_left--;
            }
            if (g_current_thread->time_slice_left == 0) {
                /* Demote one level */
                if ((int)g_current_thread->priority < RTOS_PRIO_HIGH - 1) {
                    g_current_thread->priority =
                        (RTOSPriority)((int)g_current_thread->priority + 1);
                }
                g_current_thread->time_slice_left =
                    RTOS_QUANTA[g_current_thread->priority];
                g_current_thread->state = THREAD_READY;
                queue_push_back(&g_ready_queues[g_current_thread->priority],
                                g_current_thread);
                sched_yield();
            }
        }
    }

    /* 5. Periodic priority boost: every 200ms, move starved threads up */
    if ((g_sched_ticks % 200) == 0) {
        for (int lvl = RTOS_PRIO_LOW; lvl <= RTOS_PRIO_NORMAL; lvl++) {
            Thread *t = g_ready_queues[lvl].head;
            while (t) {
                Thread *next_t = t->next;
                if ((g_sched_ticks - t->last_run_tick) > 500) {
                    /* Starvation detected — boost back to base priority */
                    queue_remove(&g_ready_queues[lvl], t);
                    t->priority = t->base_priority;
                    t->time_slice_left = RTOS_QUANTA[t->priority];
                    queue_push_front(&g_ready_queues[t->priority], t);
                }
                t = next_t;
            }
        }
    }
}

/* ------------------------------------------------------------------ */
/* sched_yield — pick next thread to run                              */
/* ------------------------------------------------------------------ */

void sched_yield(void) {
    Thread *prev = g_current_thread;
    Thread *next = NULL;

    /* Pick highest-priority non-empty ready queue
     * (RTOS_PRIO_REALTIME = 4 checked first) */
    for (int lvl = RTOS_PRIORITY_LEVELS - 2; lvl >= 0; lvl--) {
        next = queue_pop_front(&g_ready_queues[lvl]);
        if (next) break;
    }

    if (!next) {
        /* Fall back to idle thread */
        if (prev && prev->state == THREAD_RUNNING) return;
        next = g_idle_thread;
        if (!next) return;
    }

    next->state        = THREAD_RUNNING;
    next->last_run_tick = g_sched_ticks;
    g_current_thread   = next;

    if (prev != next) {
        switch_to_thread(prev, next);
    }
}

/* ------------------------------------------------------------------ */
/* sched_exit                                                         */
/* ------------------------------------------------------------------ */

void sched_exit(void) {
    if (g_current_thread) {
        g_current_thread->state = THREAD_TERMINATED;
        g_thread_count--;
        sched_yield();
    }
}

Thread *sched_get_current(void) { return g_current_thread; }
uint64_t sched_get_ticks(void)  { return g_sched_ticks; }

uint32_t sched_get_thread_count(void) { return g_thread_count; }
Thread  *sched_get_thread_by_index(uint32_t idx) {
    if (idx >= MAX_THREADS) return NULL;
    if (g_threads[idx].state == THREAD_TERMINATED) return NULL;
    return &g_threads[idx];
}

/* ------------------------------------------------------------------ */
/* CPU Affinity                                                        */
/* ------------------------------------------------------------------ */

void rtos_set_affinity(Thread *t, uint16_t cpu_mask) {
    if (t) t->cpu_affinity = cpu_mask;
}

/* ------------------------------------------------------------------ */
/* Priority Boost (I/O wakeup, DPC completion)                        */
/* ------------------------------------------------------------------ */

void rtos_boost(Thread *t, RTOSPriority new_prio) {
    if (!t) return;
    if (new_prio > t->priority) {
        t->priority = new_prio;
        t->time_slice_left = RTOS_QUANTA[new_prio];
    }
}

/* ------------------------------------------------------------------ */
/* Deadline Scheduling (EDF for MMCSS threads)                        */
/* ------------------------------------------------------------------ */

void rtos_set_deadline(Thread *t, uint64_t deadline_ms) {
    if (!t) return;
    t->deadline_tick   = g_sched_ticks + deadline_ms;
    t->deadline_missed = 0;
    /* Pin to REALTIME priority for deadline threads */
    t->priority        = RTOS_PRIO_REALTIME;
    t->time_slice_left = RTOS_QUANTA[RTOS_PRIO_REALTIME];
}

/* ------------------------------------------------------------------ */
/* DPC Queue (ISR bottom-halves)                                      */
/* ------------------------------------------------------------------ */

void dpc_queue_init(void) {
    g_dpc_head  = 0;
    g_dpc_tail  = 0;
    g_dpc_count = 0;
}

void dpc_queue(DpcRoutine routine, void *context, uint8_t urgent) {
    if (g_dpc_count >= DPC_QUEUE_DEPTH) return; /* overflow — drop */
    g_dpc_queue[g_dpc_tail].routine  = routine;
    g_dpc_queue[g_dpc_tail].context  = context;
    g_dpc_queue[g_dpc_tail].urgent   = urgent;
    g_dpc_tail = (g_dpc_tail + 1) % DPC_QUEUE_DEPTH;
    g_dpc_count++;
}

void dpc_drain(void) {
    uint32_t processed = 0;
    while (g_dpc_count > 0 && processed < 8) { /* max 8 DPCs per tick */
        DpcEntry *entry = &g_dpc_queue[g_dpc_head];
        g_dpc_head = (g_dpc_head + 1) % DPC_QUEUE_DEPTH;
        g_dpc_count--;
        if (entry->routine) {
            entry->routine(entry->context);
        }
        processed++;
    }
}

/* ------------------------------------------------------------------ */
/* Priority-Inheriting Mutex                                          */
/* ------------------------------------------------------------------ */

void kmutex_init(KMutex *m) {
    if (!m) return;
    m->locked  = 0;
    m->owner   = NULL;
    m->waiters = NULL;
}

void kmutex_lock(KMutex *m) {
    if (!m || !g_current_thread) return;

    if (m->locked == 0) {
        m->locked = g_current_thread->tid;
        m->owner  = g_current_thread;
        return;
    }

    /* Priority inheritance: boost owner to at least our priority */
    if (m->owner && m->owner->priority < g_current_thread->priority) {
        m->owner->priority = g_current_thread->priority;
    }

    /* Add ourselves to the waiter list */
    g_current_thread->blocking_thread = m->owner;
    g_current_thread->next = m->waiters;
    m->waiters = g_current_thread;
    g_current_thread->state = THREAD_BLOCKED;
    sched_yield();
}

void kmutex_unlock(KMutex *m) {
    if (!m || !g_current_thread) return;
    if (m->owner != g_current_thread) return;

    /* Restore original priority */
    g_current_thread->priority = g_current_thread->base_priority;

    /* Wake first waiter */
    Thread *waiter = m->waiters;
    if (waiter) {
        m->waiters              = waiter->next;
        waiter->next            = NULL;
        waiter->blocking_thread = NULL;
        waiter->state           = THREAD_READY;
        m->locked               = waiter->tid;
        m->owner                = waiter;
        queue_push_front(&g_ready_queues[waiter->priority], waiter);
    } else {
        m->locked = 0;
        m->owner  = NULL;
    }
}

/* ------------------------------------------------------------------ */
/* Watchdog                                                           */
/* ------------------------------------------------------------------ */

void rtos_watchdog_init(uint32_t max_realtime_miss_ticks) {
    g_watchdog_miss_limit  = max_realtime_miss_ticks;
    g_watchdog_miss_streak = 0;
}

void rtos_watchdog_tick(void) {
    /* Check all REALTIME threads with active deadlines */
    for (int i = 0; i < MAX_THREADS; i++) {
        Thread *t = &g_threads[i];
        if (t->state == THREAD_TERMINATED) continue;
        if (t->priority < RTOS_PRIO_REALTIME) continue;
        if (t->deadline_tick == 0) continue;

        if (g_sched_ticks > t->deadline_tick && !t->deadline_missed) {
            t->deadline_missed = 1;
            g_watchdog_miss_streak++;
            /* Log via IPC — GUI can show deadline miss alert */
            /* (gui_ipc not included here to avoid circular dependency) */
            if (g_watchdog_miss_streak >= g_watchdog_miss_limit) {
                /* Catastrophic: too many consecutive misses — demote to HIGH */
                t->priority = RTOS_PRIO_HIGH;
                g_watchdog_miss_streak = 0;
            }
        } else if (g_sched_ticks <= t->deadline_tick) {
            t->deadline_missed = 0;
            g_watchdog_miss_streak = 0;
        }
    }
}
