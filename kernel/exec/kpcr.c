/**
 * @file kpcr.c
 * @brief KPCR/KPRCB Initialization, DPC Queue, and AP Kernel Entry
 */

#include "kpcr.h"
#include "../arch/x86_64/apic.h"
#include "../sched/sched.h"
#include "../mm/heap.h"

/* ---------------------------------------------------------------------------
 * MSR addresses for GS base manipulation
 * --------------------------------------------------------------------------- */
#define MSR_IA32_GS_BASE         0xC0000101
#define MSR_IA32_KERNEL_GS_BASE  0xC0000102

/* ---------------------------------------------------------------------------
 * Global KPCR Table (statically allocated — no heap needed for bootstrap)
 * --------------------------------------------------------------------------- */
KPCR g_kpcr_table[MAX_CPUS] __attribute__((aligned(64)));
uint32_t g_online_cpu_count = 0;

/* ---------------------------------------------------------------------------
 * wrmsr_inline — Write a 64-bit value to a Model-Specific Register
 * --------------------------------------------------------------------------- */
static inline void wrmsr(uint32_t msr, uint64_t value) {
    uint32_t lo = (uint32_t)(value & 0xFFFFFFFF);
    uint32_t hi = (uint32_t)(value >> 32);
    __asm__ volatile ("wrmsr" :: "c"(msr), "a"(lo), "d"(hi) : "memory");
}

static inline uint64_t rdmsr(uint32_t msr) {
    uint32_t lo, hi;
    __asm__ volatile ("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}

/* ---------------------------------------------------------------------------
 * kpcr_init — Initialize KPCR for one CPU and write GS_BASE MSR
 * --------------------------------------------------------------------------- */
void kpcr_init(uint32_t cpu_id, uint32_t apic_id,
               TASK_STATE_SEGMENT *tss, IDT_ENTRY *idt, GDT_ENTRY *gdt)
{
    if (cpu_id >= MAX_CPUS) return;

    KPCR *kpcr = &g_kpcr_table[cpu_id];

    /* Zero-initialize the entire KPCR */
    __builtin_memset(kpcr, 0, sizeof(KPCR));

    /* Self-pointer at GS:0x0 — allows recovering KPCR* via "mov rax, gs:[0]" */
    kpcr->self               = kpcr;
    /* KPRCB pointer at GS:0x8 — points to the embedded KPRCB within this KPCR */
    kpcr->prcb               = &kpcr->embedded_prcb;

    kpcr->initial_apic_id    = apic_id;
    kpcr->current_irql       = IRQL_HIGH_LEVEL;  /* Phase 0: all interrupts masked */

    kpcr->tss = tss;
    kpcr->idt = idt;
    kpcr->gdt = (GDT_ENTRY *)gdt;

    /* Populate the embedded KPRCB */
    KPRCB *prcb = &kpcr->embedded_prcb;
    prcb->processor_id   = cpu_id;
    prcb->apic_id        = apic_id;
    prcb->current_irql   = IRQL_HIGH_LEVEL;
    prcb->current_thread = NULL;
    prcb->idle_thread    = NULL;
    prcb->next_thread    = NULL;
    prcb->scheduler_ticks    = 0;
    prcb->context_switches   = 0;
    prcb->idle_ticks         = 0;
    prcb->dpc_queue_head = NULL;
    prcb->dpc_queue_tail = NULL;
    prcb->dpc_queue_count= 0;

    /* Write IA32_GS_BASE MSR to point to this CPU's KPCR */
    wrmsr(MSR_IA32_GS_BASE, (uint64_t)kpcr);

    /* Also write IA32_KERNEL_GS_BASE so SWAPGS restores the kernel KPCR
     * after returning from user mode (SYSCALL/SYSRET path) */
    wrmsr(MSR_IA32_KERNEL_GS_BASE, (uint64_t)kpcr);

    /* Register this CPU as online */
    if (cpu_id >= g_online_cpu_count) {
        g_online_cpu_count = cpu_id + 1;
    }
}

/* ---------------------------------------------------------------------------
 * kpcr_queue_dpc — Queue a Deferred Procedure Call for the current CPU
 *
 * DPCs are used by drivers and subsystems to defer work from interrupt context
 * (IRQL=CLOCK_LEVEL) to a lower IRQL (DISPATCH_LEVEL) where more operations
 * are safe (e.g., acquiring spin locks, accessing paged memory).
 * --------------------------------------------------------------------------- */
void kpcr_queue_dpc(DPC_CALLBACK callback, void *context) {
    KPCR *kpcr = kpcr_get();
    if (!kpcr || !callback) return;

    KPRCB *prcb = kpcr->prcb;

    /* Allocate a DPC node from the kernel heap */
    DPC_NODE *node = (DPC_NODE *)kmalloc(sizeof(DPC_NODE));
    if (!node) return;

    node->callback = callback;
    node->context  = context;
    node->next     = NULL;

    /* Append to tail of DPC queue */
    if (!prcb->dpc_queue_head) {
        prcb->dpc_queue_head = node;
        prcb->dpc_queue_tail = node;
    } else {
        prcb->dpc_queue_tail->next = node;
        prcb->dpc_queue_tail = node;
    }
    prcb->dpc_queue_count++;
}

/* ---------------------------------------------------------------------------
 * kpcr_flush_dpc_queue — Drain and execute all pending DPCs
 *
 * Must be called with interrupts enabled but at DISPATCH_LEVEL (IRQL=2).
 * Called by the scheduler after an APIC timer interrupt.
 * --------------------------------------------------------------------------- */
void kpcr_flush_dpc_queue(void) {
    KPCR *kpcr = kpcr_get();
    if (!kpcr) return;

    KPRCB *prcb = kpcr->prcb;

    while (prcb->dpc_queue_head) {
        DPC_NODE *node = prcb->dpc_queue_head;
        prcb->dpc_queue_head = node->next;
        if (!prcb->dpc_queue_head) prcb->dpc_queue_tail = NULL;
        prcb->dpc_queue_count--;

        /* Execute the DPC callback */
        if (node->callback) {
            node->callback(node->context);
        }

        kfree(node);
    }
}

/* ---------------------------------------------------------------------------
 * isr_common_handler — Central C-level interrupt dispatch
 *
 * Called from isr_common_entry in gdt_idt.asm with a pointer to the
 * full InterruptFrame on the stack. Dispatches to appropriate handlers.
 * --------------------------------------------------------------------------- */
void isr_common_handler(void *frame_ptr) {
    /* Cast to InterruptFrame structure */
    typedef struct {
        uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
        uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
        uint64_t vector;     /* pushed by ISR stub */
        uint64_t error_code; /* pushed by ISR stub or CPU */
        uint64_t rip, cs, rflags, rsp, ss;  /* CPU-pushed on interrupt */
    } InterruptFrame;

    InterruptFrame *frame = (InterruptFrame *)frame_ptr;
    KPCR *kpcr = kpcr_get();
    if (kpcr) kpcr->prcb->irq_count[frame->vector & 0xFF]++;

    uint64_t vector = frame->vector;

    switch (vector) {
        case 0:  /* #DE Divide Error — log and kill thread */
        case 6:  /* #UD Invalid Opcode */
        case 13: /* #GP General Protection Fault */
        case 14: /* #PF Page Fault */
            /* In a full OS: kill the faulting process and signal SIGSEGV/SIGILL */
            /* For now: halt the CPU */
            __asm__ volatile ("cli; hlt");
            break;

        case 8:  /* #DF Double Fault — unrecoverable */
            __asm__ volatile ("cli; hlt");
            break;

        case 0x20: /* APIC Timer → Scheduler tick */
            if (kpcr) {
                kpcr->prcb->scheduler_ticks++;
                kpcr_set_irql(IRQL_DISPATCH_LEVEL);
                sched_tick();
                kpcr_flush_dpc_queue();
                kpcr_set_irql(IRQL_PASSIVE_LEVEL);
            }
            /* Signal EOI to Local APIC — must be done before returning */
            extern void apic_eoi(void);
            apic_eoi();
            break;

        case 0xFF: /* Spurious interrupt — no EOI for spurious */
            break;

        default:
            /* Hardware IRQs (0x21-0xFE): signal EOI */
            if (vector >= 0x21 && vector <= 0xFE) {
                extern void apic_eoi(void);
                apic_eoi();
            }
            break;
    }
}

/* ---------------------------------------------------------------------------
 * ap_kernel_entry_c — Application Processor entry into C land
 *
 * Called by each AP after the 16→32→64-bit trampoline transition.
 * Each AP must:
 *   1. Initialize its own KPCR (sets GS_BASE MSR for this CPU)
 *   2. Load its own GDT, IDT, TSS (or share the BSP's IDT)
 *   3. Signal the BSP that it's alive (atomic counter increment)
 *   4. Enter the idle scheduler loop
 * --------------------------------------------------------------------------- */
void ap_kernel_entry_c(void) {
    /* Determine which CPU we are from the APIC ID */
    extern uint8_t  g_apic_ids[];
    extern uint64_t g_lapic_base;

    /* Read current CPU's Local APIC ID from MMIO */
    volatile uint32_t *lapic_id_reg = (volatile uint32_t *)(g_lapic_base + 0x020);
    uint8_t my_apic_id = (uint8_t)(*lapic_id_reg >> 24);

    /* Find logical CPU index from APIC ID table */
    uint32_t cpu_id = 0;
    extern uint32_t g_cpu_count;
    for (uint32_t i = 1; i < g_cpu_count && i < MAX_CPUS; i++) {
        if (g_apic_ids[i] == my_apic_id) {
            cpu_id = i;
            break;
        }
    }

    /* Initialize this AP's KPCR using the BSP's hardware tables
     * (In a full SMP kernel each AP would have its own GDT/TSS) */
    extern GDT_ENTRY  g_gdt[];
    extern IDT_ENTRY  g_idt[];
    extern TASK_STATE_SEGMENT g_tss;
    kpcr_init(cpu_id, my_apic_id, &g_tss, g_idt, g_gdt);

    /* Lower IRQL to PASSIVE and enable interrupts */
    kpcr_set_irql(IRQL_PASSIVE_LEVEL);
    __asm__ volatile ("sti");

    /* Enter the idle loop — the scheduler will preempt this when work arrives */
    while (1) {
        KPCR *kpcr = kpcr_get();
        if (kpcr) kpcr->prcb->idle_ticks++;
        __asm__ volatile ("hlt");  /* HLT until next interrupt */
    }
}
