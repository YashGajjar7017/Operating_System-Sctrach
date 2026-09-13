/**
 * @file sched.h
 * @brief Preemptive Multilevel Feedback Queue (MLFQ) Process & Thread Scheduler
 */

#ifndef _KERNEL_SCHED_H_
#define _KERNEL_SCHED_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_THREADS          64
#define MAX_PROCESSES        16
#define MLFQ_LEVELS          3
#define THREAD_STACK_SIZE    (32 * 1024)   /* 32 KB kernel stack */
#define FPU_STATE_SIZE       512           /* 512-byte aligned fxsave64 state */

typedef enum {
    THREAD_EMBRYO = 0,
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_BLOCKED,
    THREAD_SLEEPING,
    THREAD_TERMINATED
} ThreadState;

/* Register frame preserved during hardware interrupt / context switch */
typedef struct __attribute__((packed)) {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t rip, cs, rflags, rsp, ss;
} InterruptFrame;

/* Thread Control Block (TCB) */
typedef struct Thread {
    uint32_t           tid;
    uint32_t           pid;
    ThreadState        state;
    uint8_t            priority;        /* 0 = Realtime/UI, 1 = Normal, 2 = Low */
    uint32_t           time_slice_left; /* Remaining ticks in current quantum */
    uint32_t           quantum;         /* Max quantum for current queue level */
    uint64_t           sleep_until;     /* Absolute system tick count for wakeup */
    
    uint64_t           rsp;             /* Preserved Kernel Stack Pointer */
    uint64_t           cr3;             /* Virtual Address Space Page Directory pointer */
    uint8_t            fpu_state[FPU_STATE_SIZE] __attribute__((aligned(16)));
    
    uint8_t            stack[THREAD_STACK_SIZE] __attribute__((aligned(16)));
    struct Thread     *next;
    struct Thread     *prev;
} Thread;

/* Process Control Block (PCB) */
typedef struct Process {
    uint32_t           pid;
    char               name[32];
    uint64_t           cr3;             /* PML4 physical address */
    uint32_t           thread_count;
    uint32_t           memory_used_kb;
    uint8_t            is_active;
} Process;

/* Scheduler Core Functions */
void sched_init(void);
Thread* sched_create_kthread(void (*entry_point)(void), uint8_t priority);
void sched_tick(void);
void sched_yield(void);
void sched_sleep(uint64_t milliseconds);
void sched_exit(void);
Thread* sched_get_current(void);

#endif /* _KERNEL_SCHED_H_ */
