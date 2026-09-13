/**
 * @file sched.c
 * @brief Preemptive Multilevel Feedback Queue (MLFQ) Process & Thread Scheduler Implementation
 */

#include "sched.h"
#include "../kstring.h"

extern void switch_to_thread(Thread *prev, Thread *next);

static Thread g_threads[MAX_THREADS];
static uint32_t g_thread_count = 0;
static uint32_t g_next_tid = 1;

static const uint32_t MLFQ_QUANTA[MLFQ_LEVELS] = { 5, 15, 40 };

typedef struct {
    Thread *head;
    Thread *tail;
} ThreadQueue;

static ThreadQueue g_ready_queues[MLFQ_LEVELS];
static Thread *g_current_thread = NULL;
static Thread *g_sleep_queue = NULL;
static uint64_t g_sched_ticks = 0;

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

static Thread* queue_pop_front(ThreadQueue *q) {
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

void sched_init(void) {
    for (int i = 0; i < MLFQ_LEVELS; i++) {
        g_ready_queues[i].head = NULL;
        g_ready_queues[i].tail = NULL;
    }
    for (int i = 0; i < MAX_THREADS; i++) {
        g_threads[i].state = THREAD_TERMINATED;
        g_threads[i].tid = 0;
    }
    g_thread_count = 0;
    g_current_thread = NULL;
    g_sleep_queue = NULL;
    g_sched_ticks = 0;
}

Thread* sched_create_kthread(void (*entry_point)(void), uint8_t priority) {
    if (g_thread_count >= MAX_THREADS) return NULL;
    if (priority >= MLFQ_LEVELS) priority = MLFQ_LEVELS - 1;

    Thread *t = NULL;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_threads[i].state == THREAD_TERMINATED || g_threads[i].state == THREAD_EMBRYO) {
            t = &g_threads[i];
            break;
        }
    }
    if (!t) return NULL;

    t->tid = g_next_tid++;
    t->pid = 0;
    t->state = THREAD_READY;
    t->priority = priority;
    t->quantum = MLFQ_QUANTA[priority];
    t->time_slice_left = t->quantum;
    t->sleep_until = 0;
    t->cr3 = 0;

    /* Initialize stack frame so switch_to_thread returns directly to entry_point */
    uint64_t *stack_top = (uint64_t *)(t->stack + THREAD_STACK_SIZE);

    /* 1. Return address (RIP) */
    *(--stack_top) = (uint64_t)entry_point;

    /* 2. RFLAGS (Interrupts Enabled = 0x202) */
    *(--stack_top) = 0x202;

    /* 3. Callee-saved registers (R15, R14, R13, R12, RBX, RBP) */
    *(--stack_top) = 0; /* R15 */
    *(--stack_top) = 0; /* R14 */
    *(--stack_top) = 0; /* R13 */
    *(--stack_top) = 0; /* R12 */
    *(--stack_top) = 0; /* RBX */
    *(--stack_top) = 0; /* RBP */

    t->rsp = (uint64_t)stack_top;

    queue_push_back(&g_ready_queues[priority], t);
    g_thread_count++;
    return t;
}

void sched_sleep(uint64_t milliseconds) {
    if (!g_current_thread) return;

    g_current_thread->state = THREAD_SLEEPING;
    g_current_thread->sleep_until = g_sched_ticks + milliseconds;

    if (!g_sleep_queue || g_current_thread->sleep_until < g_sleep_queue->sleep_until) {
        g_current_thread->next = g_sleep_queue;
        g_sleep_queue = g_current_thread;
    } else {
        Thread *cur = g_sleep_queue;
        while (cur->next && cur->next->sleep_until <= g_current_thread->sleep_until) {
            cur = cur->next;
        }
        g_current_thread->next = cur->next;
        cur->next = g_current_thread;
    }

    sched_yield();
}

void sched_tick(void) {
    g_sched_ticks++;

    /* 1. Wake up sleeping threads whose deadlines have arrived */
    while (g_sleep_queue && g_sleep_queue->sleep_until <= g_sched_ticks) {
        Thread *woken = g_sleep_queue;
        g_sleep_queue = g_sleep_queue->next;
        woken->next = NULL;
        woken->state = THREAD_READY;
        woken->priority = 0; /* Priority boost on I/O / sleep wakeup */
        woken->time_slice_left = MLFQ_QUANTA[0];
        queue_push_back(&g_ready_queues[0], woken);
    }

    /* 2. Check current running thread's time slice */
    if (g_current_thread && g_current_thread->state == THREAD_RUNNING) {
        if (g_current_thread->time_slice_left > 0) {
            g_current_thread->time_slice_left--;
        }

        if (g_current_thread->time_slice_left == 0) {
            if (g_current_thread->priority < MLFQ_LEVELS - 1) {
                g_current_thread->priority++;
            }
            g_current_thread->time_slice_left = MLFQ_QUANTA[g_current_thread->priority];
            g_current_thread->state = THREAD_READY;
            queue_push_back(&g_ready_queues[g_current_thread->priority], g_current_thread);

            sched_yield();
        }
    }
}

void sched_yield(void) {
    Thread *prev = g_current_thread;
    Thread *next = NULL;

    for (int lvl = 0; lvl < MLFQ_LEVELS; lvl++) {
        next = queue_pop_front(&g_ready_queues[lvl]);
        if (next) break;
    }

    if (!next) {
        if (prev && prev->state == THREAD_RUNNING) return;
        return;
    }

    next->state = THREAD_RUNNING;
    g_current_thread = next;

    if (prev != next) {
        switch_to_thread(prev, next);
    }
}

void sched_exit(void) {
    if (g_current_thread) {
        g_current_thread->state = THREAD_TERMINATED;
        g_thread_count--;
        sched_yield();
    }
}

Thread* sched_get_current(void) {
    return g_current_thread;
}
