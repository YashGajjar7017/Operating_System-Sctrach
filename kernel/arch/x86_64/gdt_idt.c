/**
 * @file gdt_idt.c
 * @brief x86_64 GDT, IDT, and TSS Runtime Initialization for Xenithra OS Kernel
 *
 * Phase 0 hardware table setup — executes with interrupts disabled (IRQL=HIGH_LEVEL),
 * single core (BSP only), before any user-mode or SMP code runs.
 *
 * Initialization order mandated by the CPU architecture:
 *   1. Build GDT entries in memory         (gdt_init)
 *   2. Build TSS, write its base into GDT  (tss_init)
 *   3. Build IDT entries from ISR stubs    (idt_init)
 *   4. LGDT → far-jump to reload CS        (gdt_load)
 *   5. LIDT                                (idt_load)
 *   6. LTR                                 (tss_load)
 */

#include "gdt_idt.h"

/* ---------------------------------------------------------------------------
 * Static Kernel Stacks — all 16-byte aligned for FXSAVE/FXRSTOR compatibility
 * --------------------------------------------------------------------------- */
uint8_t g_ist1_stack[IST_STACK_SIZE] __attribute__((aligned(16)));
uint8_t g_ist2_stack[IST_STACK_SIZE] __attribute__((aligned(16)));
uint8_t g_ist3_stack[IST_STACK_SIZE] __attribute__((aligned(16)));
uint8_t g_ist4_stack[IST_STACK_SIZE] __attribute__((aligned(16)));
uint8_t g_rsp0_stack[RSP0_STACK_SIZE] __attribute__((aligned(16)));

/* ---------------------------------------------------------------------------
 * Global Descriptor Table — 7 entries (48 bytes + 16-byte TSS = 56 bytes)
 * --------------------------------------------------------------------------- */
GDT_ENTRY g_gdt[GDT_ENTRY_COUNT] __attribute__((aligned(16)));

/* ---------------------------------------------------------------------------
 * Interrupt Descriptor Table — 256 × 16-byte gates = 4096 bytes
 * --------------------------------------------------------------------------- */
IDT_ENTRY g_idt[IDT_VECTOR_COUNT] __attribute__((aligned(16)));

/* ---------------------------------------------------------------------------
 * Task State Segment (BSP)
 * --------------------------------------------------------------------------- */
TASK_STATE_SEGMENT g_tss __attribute__((aligned(16)));

/* ---------------------------------------------------------------------------
 * Helper: Build a standard 8-byte code/data GDT entry
 * --------------------------------------------------------------------------- */
static void gdt_set_entry(int index, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t granularity)
{
    GDT_ENTRY *e = &g_gdt[index];
    e->limit_low  = (uint16_t)(limit & 0xFFFF);
    e->base_low   = (uint16_t)(base  & 0xFFFF);
    e->base_mid   = (uint8_t)((base  >> 16) & 0xFF);
    e->access     = access;
    e->granularity = (granularity & 0xF0) | ((limit >> 16) & 0x0F);
    e->base_high  = (uint8_t)((base  >> 24) & 0xFF);
}

/* ---------------------------------------------------------------------------
 * gdt_init — Build all GDT segment descriptors
 *
 * In 64-bit Long Mode most segment attributes are ignored (base/limit not used
 * for code/data), but the Present, DPL, and L bits are still enforced by the CPU.
 * --------------------------------------------------------------------------- */
void gdt_init(void)
{
    /* 0x00: Null descriptor — required by the ABI */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* 0x08: Kernel 64-bit Code Segment (Ring 0)
     *   Access:      Present | DPL=0 | S=1 | Executable | Read
     *   Granularity: G=1 | L=1 (Long Mode) | D=0           */
    gdt_set_entry(1, 0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL_RING0 | GDT_ACCESS_CODE_DATA |
        GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW,
        GDT_GRAN_4K | GDT_GRAN_LONG_MODE);

    /* 0x10: Kernel Data/Stack Segment (Ring 0)
     *   Access:      Present | DPL=0 | S=1 | Read/Write
     *   Granularity: G=1 | D=1 (32-bit for data, no L bit needed) */
    gdt_set_entry(2, 0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL_RING0 | GDT_ACCESS_CODE_DATA |
        GDT_ACCESS_RW,
        GDT_GRAN_4K | GDT_GRAN_32BIT);

    /* 0x18: User Data Segment (Ring 3)
     *   SYSRET sets SS from Star[47:32]+8, so User DS must be at Kernel CS + 0x10
     *   Access:      Present | DPL=3 | S=1 | Read/Write */
    gdt_set_entry(3, 0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL_RING3 | GDT_ACCESS_CODE_DATA |
        GDT_ACCESS_RW,
        GDT_GRAN_4K | GDT_GRAN_32BIT);

    /* 0x20: User 64-bit Code Segment (Ring 3)
     *   SYSRET sets CS from Star[47:32]+16
     *   Access:      Present | DPL=3 | S=1 | Executable | Read
     *   Granularity: G=1 | L=1 (Long Mode) | D=0 */
    gdt_set_entry(4, 0, 0xFFFFF,
        GDT_ACCESS_PRESENT | GDT_ACCESS_DPL_RING3 | GDT_ACCESS_CODE_DATA |
        GDT_ACCESS_EXECUTABLE | GDT_ACCESS_RW,
        GDT_GRAN_4K | GDT_GRAN_LONG_MODE);

    /*
     * 0x28–0x30: TSS System Descriptor (16-byte in 64-bit mode)
     * The TSS descriptor occupies two consecutive GDT slots.
     * We treat g_gdt[5] and g_gdt[6] as a GDT_SYSTEM_ENTRY by casting.
     * The base address of the TSS is encoded across the two entries.
     */
    GDT_SYSTEM_ENTRY *tss_entry = (GDT_SYSTEM_ENTRY *)&g_gdt[5];
    uint64_t tss_base = (uint64_t)&g_tss;
    uint32_t tss_limit = sizeof(TASK_STATE_SEGMENT) - 1;

    tss_entry->low.limit_low   = (uint16_t)(tss_limit & 0xFFFF);
    tss_entry->low.base_low    = (uint16_t)(tss_base & 0xFFFF);
    tss_entry->low.base_mid    = (uint8_t)((tss_base >> 16) & 0xFF);
    /* Access: Present=1, DPL=0, Type=0x9 (64-bit TSS Available) */
    tss_entry->low.access      = GDT_ACCESS_PRESENT | 0x09;
    tss_entry->low.granularity = (uint8_t)((tss_limit >> 16) & 0x0F);
    tss_entry->low.base_high   = (uint8_t)((tss_base >> 24) & 0xFF);
    tss_entry->base_upper      = (uint32_t)(tss_base >> 32);
    tss_entry->reserved        = 0;
}

/* ---------------------------------------------------------------------------
 * tss_init — Populate the Task State Segment stacks
 *
 * RSP0 is loaded when hardware transitions from Ring-3 to Ring-0 on interrupt/syscall.
 * IST1..IST4 are used for specific critical exception vectors (see IDT setup below).
 * The IOPB offset is set to the TSS size, meaning all I/O port access triggers a #GP.
 * --------------------------------------------------------------------------- */
void tss_init(void)
{
    /* Clear the entire TSS to zero first */
    __builtin_memset(&g_tss, 0, sizeof(TASK_STATE_SEGMENT));

    /* RSP0: Ring-0 kernel exception stack — stack grows down, point to top */
    g_tss.rsp[0] = (uint64_t)(g_rsp0_stack + RSP0_STACK_SIZE);

    /* IST1 → #DF (Double Fault): Guaranteed fresh stack, no recursive fault */
    g_tss.ist[IST_DOUBLE_FAULT - 1]  = (uint64_t)(g_ist1_stack + IST_STACK_SIZE);
    /* IST2 → NMI (Non-Maskable Interrupt): Cannot share stack with other handlers */
    g_tss.ist[IST_NMI - 1]           = (uint64_t)(g_ist2_stack + IST_STACK_SIZE);
    /* IST3 → #MC (Machine Check): Dedicated stack for catastrophic hardware faults */
    g_tss.ist[IST_MACHINE_CHECK - 1] = (uint64_t)(g_ist3_stack + IST_STACK_SIZE);
    /* IST4 → #DB (Debug Trap): Separate stack prevents kernel stack disclosure via GDB */
    g_tss.ist[IST_DEBUG - 1]         = (uint64_t)(g_ist4_stack + IST_STACK_SIZE);

    /* IOPB beyond TSS limit — all I/O port access triggers #GP from userland */
    g_tss.iopb_offset = sizeof(TASK_STATE_SEGMENT);
}

/* ---------------------------------------------------------------------------
 * idt_set_gate — Install a single IDT gate
 * --------------------------------------------------------------------------- */
static void idt_set_gate(uint8_t vector, void *handler, uint8_t ist,
                         uint8_t gate_type, uint8_t dpl)
{
    IDT_ENTRY *e  = &g_idt[vector];
    uint64_t addr = (uint64_t)handler;

    e->offset_low  = (uint16_t)(addr & 0xFFFF);
    e->selector    = SEL_KERNEL_CODE;
    e->ist         = ist & 0x7;
    e->reserved0   = 0;
    e->gate_type   = gate_type & 0xF;
    e->zero        = 0;
    e->dpl         = dpl & 0x3;
    e->present     = 1;
    e->offset_mid  = (uint16_t)((addr >> 16) & 0xFFFF);
    e->offset_high = (uint32_t)(addr >> 32);
    e->reserved1   = 0;
}

/* ---------------------------------------------------------------------------
 * idt_init — Install ISR stubs for all 256 vectors
 *
 * Interrupt Gates (gate_type=0xE) are used for hardware IRQs and exceptions
 * that must not be re-entered — they atomically clear RFLAGS.IF on entry.
 *
 * Trap Gates (gate_type=0xF) preserve RFLAGS.IF — used for software traps
 * and debug breakpoints.
 * --------------------------------------------------------------------------- */
void idt_init(void)
{
    __builtin_memset(g_idt, 0, sizeof(g_idt));

    for (int i = 0; i < IDT_VECTOR_COUNT; i++) {
        idt_set_gate((uint8_t)i, isr_stub_table[i],
                     0,                   /* IST=0: use RSP0 from TSS by default */
                     IDT_GATE_INTERRUPT,  /* Interrupt Gate (clears IF) */
                     0);                  /* DPL=0: only kernel can trigger directly */
    }

    /* -----------------------------------------------------------------------
     * Override specific critical vectors with dedicated IST stacks.
     * This guarantees a clean stack even if the original kernel stack is
     * corrupted or exhausted — preventing triple faults.
     * ----------------------------------------------------------------------- */

    /* Vector 1: #DB Debug Exception — IST4, Trap Gate (preserves IF for single-step) */
    idt_set_gate(1,  isr_stub_table[1],  IST_DEBUG,        IDT_GATE_TRAP,      0);
    /* Vector 2: NMI — IST2, Interrupt Gate (NMI not maskable, needs clean stack) */
    idt_set_gate(2,  isr_stub_table[2],  IST_NMI,          IDT_GATE_INTERRUPT, 0);
    /* Vector 8: #DF Double Fault — IST1, Interrupt Gate (last resort recovery) */
    idt_set_gate(8,  isr_stub_table[8],  IST_DOUBLE_FAULT, IDT_GATE_INTERRUPT, 0);
    /* Vector 18: #MC Machine Check — IST3, Interrupt Gate */
    idt_set_gate(18, isr_stub_table[18], IST_MACHINE_CHECK,IDT_GATE_INTERRUPT, 0);

    /* Vector 3: #BP Breakpoint — DPL=3 so userland debuggers can trigger INT3 */
    idt_set_gate(3,  isr_stub_table[3],  0,                IDT_GATE_TRAP,      3);
    /* Vector 128 (0x80): Legacy POSIX syscall — DPL=3, Trap Gate */
    idt_set_gate(0x80, isr_stub_table[0x80], 0,            IDT_GATE_TRAP,      3);
}

/* ---------------------------------------------------------------------------
 * gdt_load / idt_load / tss_load — Assembly wrapper bodies
 * These delegate to the .asm implementations in gdt_idt.asm
 * --------------------------------------------------------------------------- */
extern void _gdt_load_asm(DT_REGISTER *gdtr);
extern void _idt_load_asm(DT_REGISTER *idtr);
extern void _tss_load_asm(uint16_t selector);

void gdt_load(void)
{
    DT_REGISTER gdtr;
    gdtr.limit = sizeof(g_gdt) - 1;
    gdtr.base  = (uint64_t)&g_gdt;
    _gdt_load_asm(&gdtr);
}

void idt_load(void)
{
    DT_REGISTER idtr;
    idtr.limit = sizeof(g_idt) - 1;
    idtr.base  = (uint64_t)&g_idt;
    _idt_load_asm(&idtr);
}

void tss_load(void)
{
    _tss_load_asm(SEL_TSS);
}

/* ---------------------------------------------------------------------------
 * hardware_tables_init — Master Phase 0 initialization sequence
 * Called from kmain() before any interrupts are unmasked.
 * --------------------------------------------------------------------------- */
void hardware_tables_init(void)
{
    gdt_init();   /* 1. Build GDT entries (including TSS descriptor) */
    tss_init();   /* 2. Populate TSS stacks (bases must be in GDT first) */
    idt_init();   /* 3. Install all 256 IDT gates from ISR stub table   */
    gdt_load();   /* 4. LGDT + far-jump to flush CS pipeline            */
    idt_load();   /* 5. LIDT                                            */
    tss_load();   /* 6. LTR with TSS selector                           */
}
