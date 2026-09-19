/**
 * @file apic.h
 * @brief Local APIC & SMP Inter-Processor Interrupt (IPI) Interface for Xenithra OS
 *
 * The Advanced Programmable Interrupt Controller (APIC) is the core hardware
 * abstraction for:
 *   - Replacing the legacy 8259A PIC for hardware interrupt routing
 *   - Providing per-CPU timer interrupts (scheduler preemption ticks)
 *   - Broadcasting INIT-SIPI-SIPI sequences to wake Application Processors (APs)
 *   - Sending cross-core IPIs for TLB shootdown and IPI messaging
 *
 * APIC Register Layout (Memory-Mapped I/O at LOCAL_APIC_BASE):
 *   +0x020 : Local APIC ID Register (LAPIC_ID)
 *   +0x030 : APIC Version Register
 *   +0x080 : Task Priority Register (TPR) — IRQL analog
 *   +0x0B0 : End-Of-Interrupt Register (EOI) — write 0 to signal IRQ complete
 *   +0x0D0 : Logical Destination Register
 *   +0x0E0 : Destination Format Register
 *   +0x0F0 : Spurious Interrupt Vector Register (SIVR) — enables APIC
 *   +0x280 : Error Status Register
 *   +0x300 : Interrupt Command Register LOW  (ICR_LOW)  — triggers IPIs
 *   +0x310 : Interrupt Command Register HIGH (ICR_HIGH) — sets target APIC ID
 *   +0x320 : LVT Timer Register
 *   +0x380 : Initial Timer Count Register
 *   +0x390 : Current Timer Count Register
 *   +0x3E0 : Timer Divide Configuration Register
 */

#ifndef _ARCH_X86_64_APIC_H_
#define _ARCH_X86_64_APIC_H_

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Default Local APIC MMIO base (overridden by IA32_APIC_BASE MSR or ACPI MADT)
 * --------------------------------------------------------------------------- */
#define LOCAL_APIC_DEFAULT_BASE  0xFEE00000UL

/* ---------------------------------------------------------------------------
 * Local APIC Register Offsets (byte offsets from MMIO base)
 * --------------------------------------------------------------------------- */
#define LAPIC_REG_ID             0x020  /* Local APIC ID */
#define LAPIC_REG_VERSION        0x030  /* APIC version */
#define LAPIC_REG_TPR            0x080  /* Task Priority Register */
#define LAPIC_REG_EOI            0x0B0  /* End-Of-Interrupt (write 0) */
#define LAPIC_REG_LOGICAL_DEST   0x0D0  /* Logical Destination */
#define LAPIC_REG_DEST_FORMAT    0x0E0  /* Destination Format */
#define LAPIC_REG_SIVR           0x0F0  /* Spurious Interrupt Vector Register */
#define LAPIC_REG_ESR            0x280  /* Error Status Register */
#define LAPIC_REG_ICR_LOW        0x300  /* Interrupt Command Register Low */
#define LAPIC_REG_ICR_HIGH       0x310  /* Interrupt Command Register High */
#define LAPIC_REG_LVT_TIMER      0x320  /* LVT Timer */
#define LAPIC_REG_LVT_THERMAL    0x330  /* LVT Thermal Sensor */
#define LAPIC_REG_LVT_PMC        0x340  /* LVT Performance Counter */
#define LAPIC_REG_LVT_LINT0      0x350  /* LVT Local Interrupt 0 */
#define LAPIC_REG_LVT_LINT1      0x360  /* LVT Local Interrupt 1 */
#define LAPIC_REG_LVT_ERROR      0x370  /* LVT Error */
#define LAPIC_REG_TIMER_INIT     0x380  /* Timer Initial Count */
#define LAPIC_REG_TIMER_CURR     0x390  /* Timer Current Count */
#define LAPIC_REG_TIMER_DIV      0x3E0  /* Timer Divide Configuration */

/* ---------------------------------------------------------------------------
 * SIVR Flags — Spurious Interrupt Vector Register
 * --------------------------------------------------------------------------- */
#define LAPIC_SIVR_ENABLE        (1 << 8)   /* Software-enable the APIC */
#define LAPIC_SPURIOUS_VECTOR    0xFF        /* Spurious vector (must be 0xFx) */

/* ---------------------------------------------------------------------------
 * ICR_LOW Delivery Mode and Shorthand Bits
 * --------------------------------------------------------------------------- */
#define LAPIC_ICR_DELIVERY_FIXED   (0 << 8)   /* Deliver to specific vector */
#define LAPIC_ICR_DELIVERY_SMI     (2 << 8)   /* System Management Interrupt */
#define LAPIC_ICR_DELIVERY_NMI     (4 << 8)   /* Non-Maskable Interrupt */
#define LAPIC_ICR_DELIVERY_INIT    (5 << 8)   /* INIT IPI */
#define LAPIC_ICR_DELIVERY_SIPI    (6 << 8)   /* Startup IPI (SIPI) */

#define LAPIC_ICR_DEST_PHYSICAL    (0 << 11)  /* Physical destination mode */
#define LAPIC_ICR_ASSERT           (1 << 14)  /* Assert (vs. de-assert for INIT) */
#define LAPIC_ICR_LEVEL_TRIGGER    (1 << 15)  /* Level-triggered (vs. edge) */

#define LAPIC_ICR_SHORTHAND_NONE   (0 << 18)  /* Use destination field */
#define LAPIC_ICR_SHORTHAND_SELF   (1 << 18)  /* Send to self only */
#define LAPIC_ICR_SHORTHAND_ALL    (2 << 18)  /* Send to all including self */
#define LAPIC_ICR_SHORTHAND_OTHERS (3 << 18)  /* Send to all excluding self */

/* ---------------------------------------------------------------------------
 * Timer Divide Configuration
 * --------------------------------------------------------------------------- */
#define LAPIC_TIMER_DIV_16         0x03        /* Divide by 16 */

/* LVT Timer mode bits */
#define LAPIC_TIMER_ONE_SHOT       (0 << 17)
#define LAPIC_TIMER_PERIODIC       (1 << 17)
#define LAPIC_TIMER_VECTOR         0x20        /* IRQ vector 0x20 for scheduler tick */

/* ---------------------------------------------------------------------------
 * Interrupt Vector Assignments
 * --------------------------------------------------------------------------- */
#define IRQ_TIMER_VECTOR           0x20   /* APIC timer → scheduler preemption */
#define IRQ_SPURIOUS_VECTOR        0xFF   /* Spurious interrupt (must be 0xFF) */
#define IRQ_IPI_TLB_SHOOTDOWN      0x21   /* Cross-CPU TLB invalidation IPI */
#define IRQ_IPI_RESCHEDULE         0x22   /* Cross-CPU rescheduling IPI */
#define IRQ_IPI_PANIC              0x23   /* Panic IPI — halt remote CPUs */

/* MSR for reading the APIC base and checking APIC enabled state */
#define MSR_IA32_APIC_BASE         0x1B
#define APIC_BASE_BSP_FLAG         (1 << 8)   /* Set if this CPU is the Bootstrap Processor */
#define APIC_BASE_ENABLE_FLAG      (1 << 11)  /* Global APIC enable bit */

/* ---------------------------------------------------------------------------
 * ACPI MADT (Multiple APIC Description Table) parsing structures
 * Used to discover the APIC base address and enumerate CPU APIC IDs from RSDP.
 * --------------------------------------------------------------------------- */
#pragma pack(push, 1)

typedef struct _ACPI_RSDP {
    char     signature[8];     /* "RSD PTR " */
    uint8_t  checksum;
    char     oem_id[6];
    uint8_t  revision;
    uint32_t rsdt_address;     /* Physical address of RSDT (ACPI 1.0) */
    uint32_t length;
    uint64_t xsdt_address;     /* Physical address of XSDT (ACPI 2.0+) */
    uint8_t  extended_checksum;
    uint8_t  reserved[3];
} ACPI_RSDP;

typedef struct _ACPI_SDT_HEADER {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} ACPI_SDT_HEADER;

/* MADT entry types */
#define MADT_TYPE_LOCAL_APIC        0
#define MADT_TYPE_IO_APIC           1
#define MADT_TYPE_INT_OVERRIDE      2
#define MADT_TYPE_LOCAL_APIC_NMI    4

typedef struct _MADT_LOCAL_APIC {
    uint8_t type;       /* 0 */
    uint8_t length;     /* 8 */
    uint8_t proc_uid;   /* ACPI Processor UID */
    uint8_t apic_id;    /* Local APIC ID */
    uint32_t flags;     /* Bit 0: CPU is enabled */
} MADT_LOCAL_APIC;

#pragma pack(pop)

/* ---------------------------------------------------------------------------
 * Global State
 * --------------------------------------------------------------------------- */
/* Physical APIC IDs discovered from MADT, indexed by logical CPU number */
extern uint8_t  g_apic_ids[32];
extern uint32_t g_cpu_count;
extern uint64_t g_lapic_base;    /* Virtual address of LAPIC MMIO */

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/**
 * @brief Parse ACPI RSDP/MADT to find Local APIC base and enumerate CPU APIC IDs.
 *        Maps the Local APIC MMIO region, enables the APIC via SIVR.
 * @param acpi_rsdp Physical address of RSDP from XenithraBootInfo.
 */
void apic_init(uint64_t acpi_rsdp);

/**
 * @brief Signals End-Of-Interrupt to the Local APIC.
 *        Must be called at the END of every IRQ handler before returning.
 */
void apic_eoi(void);

/**
 * @brief Sends an Inter-Processor Interrupt to a specific CPU by APIC ID.
 * @param apic_id   Target processor Local APIC ID.
 * @param vector    Interrupt vector (0x20-0xFE).
 * @param delivery  Delivery mode (LAPIC_ICR_DELIVERY_FIXED, _NMI, etc.)
 */
void apic_send_ipi(uint8_t apic_id, uint8_t vector, uint32_t delivery);

/**
 * @brief Broadcasts INIT→SIPI→SIPI sequence to all Application Processors (APs).
 *        The trampoline physical address must be page-aligned and below 1MB
 *        (SIPI vector encodes it as a 4KB-aligned page number in bits [7:0]).
 * @param trampoline_phys Physical address of the 16-bit AP wakeup trampoline code.
 */
void apic_start_all_aps(uint64_t trampoline_phys);

/**
 * @brief Arms the Local APIC one-shot timer to fire after `microseconds`.
 *        Uses the IRQ_TIMER_VECTOR. Call after apic_calibrate_timer().
 */
void apic_arm_timer(uint64_t microseconds);

/**
 * @brief Calibrate the APIC timer against the PIT (Programmable Interval Timer).
 *        Runs a 10ms PIT measurement loop to determine APIC timer ticks per ms.
 */
void apic_calibrate_timer(void);

/**
 * @brief Busy-wait for `microseconds` using the APIC timer current-count register.
 *        Does NOT use interrupts — safe during Phase 0 initialization.
 */
void apic_udelay(uint32_t microseconds);

/**
 * @brief Returns the Local APIC ID of the calling CPU (reads LAPIC_REG_ID).
 */
uint8_t apic_get_current_id(void);

#endif /* _ARCH_X86_64_APIC_H_ */
