/**
 * @file gdt_idt.h
 * @brief x86_64 GDT, IDT, and TSS Definitions for Xenithra OS Kernel
 *
 * Implements the hardware descriptor table layout matching the Intel Software
 * Developer's Manual (SDM) Vol.3A, Chapter 3 (Protected-Mode Memory Management)
 * and Chapter 6 (Interrupt and Exception Handling).
 *
 * Segment Selector Layout (Ring 0 / Ring 3):
 *   0x00 - Null Descriptor
 *   0x08 - Kernel Code Segment  (CS, 64-bit, Ring 0, RPL=0)
 *   0x10 - Kernel Data Segment  (DS/SS, Ring 0, RPL=0)
 *   0x18 - User Data Segment    (DS, Ring 3, RPL=3)  [SYSRET layout]
 *   0x20 - User Code Segment    (CS, 64-bit, Ring 3, RPL=3)
 *   0x28 - TSS Low Descriptor   (16-byte System Segment Descriptor)
 *   0x30 - TSS High Descriptor  (upper 8 bytes of 16-byte TSS descriptor)
 */

#ifndef _ARCH_X86_64_GDT_IDT_H_
#define _ARCH_X86_64_GDT_IDT_H_

#include <stdint.h>
#include <stddef.h>

/* ---------------------------------------------------------------------------
 * Segment Selectors (TI=0 GDT, RPL encoded in low 2 bits)
 * --------------------------------------------------------------------------- */
#define SEL_NULL         0x00   /* Null descriptor */
#define SEL_KERNEL_CODE  0x08   /* Kernel 64-bit Code  (RPL=0) */
#define SEL_KERNEL_DATA  0x10   /* Kernel Data/Stack   (RPL=0) */
#define SEL_USER_DATA    0x18   /* User Data/Stack     (RPL=3, adjusted to 0x1B in practice) */
#define SEL_USER_CODE    0x20   /* User 64-bit Code    (RPL=3, adjusted to 0x23 in practice) */
#define SEL_TSS          0x28   /* TSS Descriptor Low  (16-byte) */

/* RPL=3 (Ring 3) selector values actually loaded into segment registers */
#define SEL_USER_DATA_RPL3  (SEL_USER_DATA | 3)   /* 0x1B */
#define SEL_USER_CODE_RPL3  (SEL_USER_CODE | 3)   /* 0x23 */

/* Number of GDT entries (including both halves of the 16-byte TSS descriptor) */
#define GDT_ENTRY_COUNT  7

/* Number of IDT vectors (full IA-32e range) */
#define IDT_VECTOR_COUNT 256

/* ---------------------------------------------------------------------------
 * Interrupt Stack Table (IST) assignments in the TSS
 * Each IST entry is a separate kernel stack for specific critical fault vectors.
 * --------------------------------------------------------------------------- */
#define IST_DOUBLE_FAULT   1   /* #DF  - Vector 8  */
#define IST_NMI            2   /* NMI  - Vector 2  */
#define IST_MACHINE_CHECK  3   /* #MC  - Vector 18 */
#define IST_DEBUG          4   /* #DB  - Vector 1  */

/* Size of each dedicated IST stack (16 KB) */
#define IST_STACK_SIZE   (16 * 1024)
/* Size of the Ring-0 RSP0 kernel exception stack (64 KB) */
#define RSP0_STACK_SIZE  (64 * 1024)

/* ---------------------------------------------------------------------------
 * GDT Entry — Standard 8-byte Segment Descriptor (SDM Vol.3A §3.4.5)
 * --------------------------------------------------------------------------- */
#pragma pack(push, 1)
typedef struct _GDT_ENTRY {
    uint16_t limit_low;     /* Segment Limit [15:0]                    */
    uint16_t base_low;      /* Segment Base  [15:0]                    */
    uint8_t  base_mid;      /* Segment Base  [23:16]                   */
    uint8_t  access;        /* Access byte:
                             *   [0]   = Accessed (CPU sets this)
                             *   [1]   = Read/Write
                             *   [2]   = Direction/Conforming
                             *   [3]   = Executable
                             *   [4]   = Descriptor Type (1 = code/data)
                             *   [6:5] = DPL (Descriptor Privilege Level)
                             *   [7]   = Present                        */
    uint8_t  granularity;   /* Flags + Limit [19:16]:
                             *   [3:0] = Limit [19:16]
                             *   [4]   = Available for system (AVL)
                             *   [5]   = Long Mode (L) — must be 1 for 64-bit code
                             *   [6]   = Default Op Size (D) — must be 0 in Long Mode
                             *   [7]   = Granularity (G): 0=byte, 1=4KB page          */
    uint8_t  base_high;     /* Segment Base [31:24]                    */
} GDT_ENTRY;

/* 16-byte System Segment Descriptor for TSS/LDT in 64-bit mode (SDM Vol.3A §7.2.3) */
typedef struct _GDT_SYSTEM_ENTRY {
    GDT_ENTRY low;          /* Lower 8 bytes (identical format to normal entry) */
    uint32_t  base_upper;   /* Segment Base [63:32] */
    uint32_t  reserved;     /* Must be zero */
} GDT_SYSTEM_ENTRY;

/* Descriptor Table Register loaded by LGDT/LIDT instructions */
typedef struct _DT_REGISTER {
    uint16_t limit;         /* Table size in bytes minus 1 */
    uint64_t base;          /* Linear base address of the table */
} DT_REGISTER;

/* ---------------------------------------------------------------------------
 * IDT Entry — 64-bit Interrupt Gate Descriptor (SDM Vol.3A §6.14.1)
 * --------------------------------------------------------------------------- */
typedef struct _IDT_ENTRY {
    uint16_t offset_low;    /* ISR handler address [15:0]               */
    uint16_t selector;      /* Code segment selector (SEL_KERNEL_CODE)  */
    uint8_t  ist : 3;       /* Interrupt Stack Table index (0 = use RSP0)*/
    uint8_t  reserved0 : 5; /* Must be zero                              */
    uint8_t  gate_type : 4; /* 0xE = 64-bit Interrupt Gate               */
                             /* 0xF = 64-bit Trap Gate                    */
    uint8_t  zero : 1;      /* Must be zero                              */
    uint8_t  dpl : 2;       /* Descriptor Privilege Level (0=kernel)     */
    uint8_t  present : 1;   /* Gate Present                              */
    uint16_t offset_mid;    /* ISR handler address [31:16]               */
    uint32_t offset_high;   /* ISR handler address [63:32]               */
    uint32_t reserved1;     /* Must be zero                              */
} IDT_ENTRY;

/* ---------------------------------------------------------------------------
 * Task State Segment — 64-bit TSS (SDM Vol.3A §7.7)
 * Used for Ring-0 RSP0 stack pointer and IST stacks on privilege-level changes
 * and critical exceptions.
 * --------------------------------------------------------------------------- */
typedef struct _TASK_STATE_SEGMENT {
    uint32_t reserved0;
    uint64_t rsp[3];        /* RSP0, RSP1, RSP2 — stack pointers for CPL 0,1,2 */
    uint64_t reserved1;
    uint64_t ist[7];        /* IST1 through IST7 — Interrupt Stack Table entries */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;   /* I/O Permission Bitmap offset from TSS base       */
} TASK_STATE_SEGMENT;
#pragma pack(pop)

/* ---------------------------------------------------------------------------
 * Gate type constants for IDT_ENTRY.gate_type
 * --------------------------------------------------------------------------- */
#define IDT_GATE_INTERRUPT  0xE    /* Clears RFLAGS.IF on entry (disables interrupts) */
#define IDT_GATE_TRAP       0xF    /* Preserves RFLAGS.IF on entry                    */

/* Access byte constants for GDT_ENTRY.access */
#define GDT_ACCESS_PRESENT      (1 << 7)
#define GDT_ACCESS_DPL_RING0    (0 << 5)
#define GDT_ACCESS_DPL_RING3    (3 << 5)
#define GDT_ACCESS_CODE_DATA    (1 << 4)  /* S bit: code/data segment */
#define GDT_ACCESS_EXECUTABLE   (1 << 3)
#define GDT_ACCESS_RW           (1 << 1)  /* Read (code) / Write (data) */

/* Granularity byte constants for GDT_ENTRY.granularity */
#define GDT_GRAN_4K         (1 << 7)   /* G bit: 4KB granularity */
#define GDT_GRAN_LONG_MODE  (1 << 5)   /* L bit: 64-bit code segment */
#define GDT_GRAN_32BIT      (1 << 6)   /* D bit: 32-bit default size (must be 0 in Long Mode) */

/* ---------------------------------------------------------------------------
 * Global GDT, IDT, TSS structures (one set per BSP, extended per AP)
 * --------------------------------------------------------------------------- */
extern GDT_ENTRY        g_gdt[GDT_ENTRY_COUNT];
extern IDT_ENTRY        g_idt[IDT_VECTOR_COUNT];
extern TASK_STATE_SEGMENT g_tss;

/* Per-IST dedicated stacks */
extern uint8_t g_ist1_stack[IST_STACK_SIZE];   /* #DF  */
extern uint8_t g_ist2_stack[IST_STACK_SIZE];   /* NMI  */
extern uint8_t g_ist3_stack[IST_STACK_SIZE];   /* #MC  */
extern uint8_t g_ist4_stack[IST_STACK_SIZE];   /* #DB  */
extern uint8_t g_rsp0_stack[RSP0_STACK_SIZE];  /* Ring-0 exception stack */

/* ISR stub table declared in gdt_idt.asm — 256 entries */
extern void* isr_stub_table[IDT_VECTOR_COUNT];

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/**
 * @brief Initializes the Global Descriptor Table with all required segment descriptors.
 *        Installs the 16-byte TSS descriptor at SEL_TSS.
 *        Must be called before idt_init() and tss_init().
 */
void gdt_init(void);

/**
 * @brief Populates all 256 IDT gates from the isr_stub_table[] array.
 *        Critical vectors (#DF, NMI, #MC, #DB) are configured with their IST stacks.
 *        Must be called after gdt_init().
 */
void idt_init(void);

/**
 * @brief Populates the Task State Segment with kernel RSP0 and IST stacks.
 *        Must be called after gdt_init() (requires TSS descriptor base already set).
 */
void tss_init(void);

/**
 * @brief Loads GDT register via LGDT, reloads all segment registers.
 *        Far-jumps to flush instruction pipeline with new CS.
 */
void gdt_load(void);

/**
 * @brief Loads IDT register via LIDT instruction.
 */
void idt_load(void);

/**
 * @brief Loads Task Register with the TSS selector via LTR instruction.
 */
void tss_load(void);

/**
 * @brief Complete Phase 0 hardware table initialization sequence.
 *        Calls gdt_init() -> tss_init() -> idt_init() -> gdt_load() ->
 *              idt_load() -> tss_load() in the correct order.
 */
void hardware_tables_init(void);

#endif /* _ARCH_X86_64_GDT_IDT_H_ */
