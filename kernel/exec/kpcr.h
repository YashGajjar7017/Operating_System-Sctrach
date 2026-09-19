/**
 * @file kpcr.h
 * @brief Kernel Processor Control Region (KPCR) & KPRCB Per-CPU Structures
 *
 * The KPCR is the fundamental per-CPU data structure in Xenithra OS, accessed
 * via the GS segment base register (IA32_GS_BASE MSR, 0xC0000101).
 *
 * The GS base is set per-CPU by kpcr_init() via WRMSR. The self-pointer at
 * GS:0x0 enables the kernel to recover the KPCR* from any context with a
 * single GS-relative load (analogous to NT's KiPcr in Windows).
 *
 * KPCR GS-relative offsets (must match kpcr_get* macros and asm code):
 *   GS:0x0000 = KPCR* self
 *   GS:0x0008 = KPRCB* prcb
 *   GS:0x0010 = uint32_t initial_apic_id
 *   GS:0x0018 = uint32_t current_irql
 *
 * IRQL (Interrupt ReQuest Level) — Windows-style interrupt priority:
 *   0x00 = PASSIVE_LEVEL    (normal thread execution, interrupts enabled)
 *   0x01 = APC_LEVEL        (APC delivery, software interrupts)
 *   0x02 = DISPATCH_LEVEL   (scheduler, DPC queue)
 *   0x0D = CLOCK_LEVEL      (timer interrupt)
 *   0x0E = IPI_LEVEL        (inter-processor interrupt)
 *   0x0F = HIGH_LEVEL       (highest, all interrupts masked, Phase 0 boot)
 */

#ifndef _KERNEL_EXEC_KPCR_H_
#define _KERNEL_EXEC_KPCR_H_

#include <stdint.h>
#include "../arch/x86_64/gdt_idt.h"

/* ---------------------------------------------------------------------------
 * IRQL Definitions (maps to APIC Task Priority Register levels)
 * --------------------------------------------------------------------------- */
#define IRQL_PASSIVE_LEVEL   0x00
#define IRQL_APC_LEVEL       0x01
#define IRQL_DISPATCH_LEVEL  0x02
#define IRQL_CLOCK_LEVEL     0x0D
#define IRQL_IPI_LEVEL       0x0E
#define IRQL_HIGH_LEVEL      0x0F

/* Maximum number of supported logical processors */
#define MAX_CPUS             32

/* ---------------------------------------------------------------------------
 * Deferred Procedure Call (DPC) Node
 * DPCs are kernel callbacks queued during interrupt context and executed at
 * DISPATCH_LEVEL after the IRQ handler returns — critical for scheduler ticks.
 * --------------------------------------------------------------------------- */
typedef void (*DPC_CALLBACK)(void *context);

typedef struct _DPC_NODE {
    DPC_CALLBACK     callback;
    void            *context;
    struct _DPC_NODE *next;
} DPC_NODE;

/* ---------------------------------------------------------------------------
 * Kernel Processor Control Block (KPRCB)
 * Contains hot-path per-CPU scheduler and telemetry data.
 * --------------------------------------------------------------------------- */
typedef struct _KPRCB {
    uint32_t    processor_id;       /* Logical CPU index (0 = BSP, 1+ = APs) */
    uint32_t    apic_id;            /* Hardware Local APIC ID */
    uint32_t    current_irql;       /* Current Interrupt Request Level */
    uint32_t    pad0;

    void       *current_thread;     /* Pointer to currently executing Thread TCB */
    void       *idle_thread;        /* Per-CPU idle thread TCB */
    void       *next_thread;        /* Next thread to run (set by scheduler) */

    uint64_t    scheduler_ticks;    /* Total timer ticks processed on this CPU */
    uint64_t    context_switches;   /* Total context switches performed */
    uint64_t    idle_ticks;         /* Ticks spent in idle loop */

    /* DPC Queue — processed at DISPATCH_LEVEL after IRQ handler */
    DPC_NODE   *dpc_queue_head;
    DPC_NODE   *dpc_queue_tail;
    uint32_t    dpc_queue_count;
    uint32_t    pad1;

    /* Per-CPU ISR statistics */
    uint64_t    irq_count[256];     /* Count of each interrupt vector fired */
} KPRCB;

/* ---------------------------------------------------------------------------
 * Kernel Processor Control Region (KPCR)
 * The ENTIRE KPCR is pointed to by IA32_GS_BASE MSR.
 * GS-relative offsets must be ABI-stable — do not reorder fields.
 * --------------------------------------------------------------------------- */
typedef struct __attribute__((aligned(64))) _KPCR {
    /* [GS:0x0000] */  struct _KPCR       *self;          /* Self-pointer (GS:0 fast access) */
    /* [GS:0x0008] */  KPRCB              *prcb;          /* Points to embedded_prcb below */
    /* [GS:0x0010] */  uint32_t            initial_apic_id;
    /* [GS:0x0014] */  uint32_t            current_irql;  /* Mirror of prcb->current_irql */

    /* Hardware table pointers for this CPU */
    TASK_STATE_SEGMENT *tss;
    IDT_ENTRY          *idt;
    GDT_ENTRY          *gdt;

    /* Stack pointers */
    uint64_t            kernel_stack_top;    /* RSP0 for Ring 3→0 transitions */
    uint64_t            user_stack_mirror;   /* Saved user RSP on syscall entry */

    /* Embedded KPRCB — the KPCR owns its own KPRCB to avoid separate allocation */
    KPRCB               embedded_prcb;
} KPCR;

/* ---------------------------------------------------------------------------
 * Global KPCR Array — one per CPU (up to MAX_CPUS)
 * --------------------------------------------------------------------------- */
extern KPCR g_kpcr_table[MAX_CPUS];
extern uint32_t g_online_cpu_count;

/* ---------------------------------------------------------------------------
 * GS-relative KPCR accessors — inline assembly for hot-path performance
 * The GS register base points to the KPCR for the current CPU.
 * --------------------------------------------------------------------------- */

/** @brief Get the KPCR* for the currently executing CPU via GS:0 */
static inline KPCR *kpcr_get(void) {
    KPCR *kpcr;
    __asm__ volatile (
        "mov %%gs:0x0, %0"
        : "=r"(kpcr)
        :: "memory"
    );
    return kpcr;
}

/** @brief Get the current IRQL for this CPU */
static inline uint32_t kpcr_get_irql(void) {
    uint32_t irql;
    __asm__ volatile ("mov %%gs:0x14, %0" : "=r"(irql));
    return irql;
}

/** @brief Set the current IRQL for this CPU */
static inline void kpcr_set_irql(uint32_t irql) {
    __asm__ volatile ("mov %0, %%gs:0x14" :: "r"(irql) : "memory");
    kpcr_get()->embedded_prcb.current_irql = irql;
}

/** @brief Get the currently executing thread from KPRCB */
static inline void *kpcr_get_current_thread(void) {
    KPCR *kpcr = kpcr_get();
    return kpcr ? kpcr->embedded_prcb.current_thread : NULL;
}

/** @brief Set the current thread in KPRCB */
static inline void kpcr_set_current_thread(void *thread) {
    KPCR *kpcr = kpcr_get();
    if (kpcr) kpcr->embedded_prcb.current_thread = thread;
}

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/**
 * @brief Initialize the KPCR for a specific CPU.
 *        Writes the KPCR virtual address to IA32_GS_BASE MSR.
 *        Sets up all KPRCB fields and links to the hardware tables.
 * @param cpu_id  Logical CPU index (0 = BSP).
 * @param apic_id Hardware APIC ID for this CPU.
 * @param tss     Pointer to this CPU's TSS.
 * @param idt     Pointer to the IDT (shared by all CPUs in this implementation).
 * @param gdt     Pointer to this CPU's GDT.
 */
void kpcr_init(uint32_t cpu_id, uint32_t apic_id,
               TASK_STATE_SEGMENT *tss, IDT_ENTRY *idt, GDT_ENTRY *gdt);

/**
 * @brief Queue a DPC for execution at DISPATCH_LEVEL on the current CPU.
 * @param callback  Function to invoke at DISPATCH_LEVEL.
 * @param context   Opaque argument passed to the callback.
 */
void kpcr_queue_dpc(DPC_CALLBACK callback, void *context);

/**
 * @brief Drain and execute all pending DPCs for the current CPU.
 *        Must be called with IRQL raised to DISPATCH_LEVEL.
 */
void kpcr_flush_dpc_queue(void);

/**
 * @brief AP entry point called from the ap_trampoline after entering 64-bit mode.
 *        Initializes the AP's KPCR, joins the global CPU count, and enters idle.
 */
void ap_kernel_entry_c(void);

#endif /* _KERNEL_EXEC_KPCR_H_ */
