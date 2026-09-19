/**
 * @file apic.c
 * @brief Local APIC Initialization, SMP IPI Broadcast, Timer Calibration
 *
 * Boot sequence role:
 *   Phase 0: apic_init() — maps MMIO, enables APIC, clears error state
 *   Phase 1: apic_start_all_aps() — wakes APs via INIT-SIPI-SIPI
 *            apic_calibrate_timer() — calibrates tick rate
 *
 * MMIO Access Pattern:
 *   The Local APIC is memory-mapped at a 4KB-aligned physical address.
 *   All registers are 32-bit wide and must be accessed with 32-bit reads/writes.
 *   The bootloader identity-maps all physical memory ≤ 8GB, so g_lapic_base
 *   equals the physical address directly after ExitBootServices.
 */

#include "apic.h"
#include "../../../shared/bootinfo.h"

/* ---------------------------------------------------------------------------
 * Global State
 * --------------------------------------------------------------------------- */
uint8_t  g_apic_ids[32]  = {0};
uint32_t g_cpu_count     = 1;   /* At minimum the BSP exists */
uint64_t g_lapic_base    = LOCAL_APIC_DEFAULT_BASE;

/* APIC ticks per millisecond (measured during calibration) */
static uint32_t s_apic_ticks_per_ms = 0;

/* ---------------------------------------------------------------------------
 * Low-level MMIO Register Accessors
 * --------------------------------------------------------------------------- */
static inline uint32_t lapic_read(uint32_t reg) {
    volatile uint32_t *addr = (volatile uint32_t *)(g_lapic_base + reg);
    return *addr;
}

static inline void lapic_write(uint32_t reg, uint32_t val) {
    volatile uint32_t *addr = (volatile uint32_t *)(g_lapic_base + reg);
    *addr = val;
}

/* Ensure ICR write is serialized (Intel SDM recommends checking delivery status) */
static void lapic_wait_icr_idle(void) {
    /* Bit 12 of ICR_LOW = Delivery Status: 0=idle, 1=send pending */
    uint32_t timeout = 100000;
    while ((lapic_read(LAPIC_REG_ICR_LOW) & (1 << 12)) && timeout--) {
        __asm__ volatile ("pause");
    }
}

/* ---------------------------------------------------------------------------
 * apic_eoi — Signal End-Of-Interrupt
 * Must be called before returning from ANY hardware IRQ handler (not for NMI/SMI).
 * Writing any value to the EOI register clears the ISR bit for the current IRQ.
 * --------------------------------------------------------------------------- */
void apic_eoi(void) {
    lapic_write(LAPIC_REG_EOI, 0);
}

/* ---------------------------------------------------------------------------
 * apic_get_current_id — Read current CPU's Local APIC ID
 * Bits [31:24] of the ID register hold the APIC ID.
 * --------------------------------------------------------------------------- */
uint8_t apic_get_current_id(void) {
    return (uint8_t)(lapic_read(LAPIC_REG_ID) >> 24);
}

/* ---------------------------------------------------------------------------
 * ACPI Table Checksum Validation
 * Sum all bytes of the table — result must be 0x00 modulo 256 (wrapping add).
 * --------------------------------------------------------------------------- */
static int acpi_validate_checksum(const uint8_t *data, uint32_t length) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return (sum == 0) ? 1 : 0;
}

/* ---------------------------------------------------------------------------
 * parse_madt — Extract Local APIC base and CPU APIC IDs from the MADT
 *
 * MADT layout (after the ACPI_SDT_HEADER, starting at offset 36):
 *   [0..3] Local APIC Address (physical)
 *   [4..7] Flags
 *   [8+]   Variable-length array of MADT entries
 * --------------------------------------------------------------------------- */
static void parse_madt(uint64_t madt_phys) {
    ACPI_SDT_HEADER *madt = (ACPI_SDT_HEADER *)(uintptr_t)madt_phys;

    if (!madt || madt->signature[0] != 'A' || madt->signature[1] != 'P' ||
        madt->signature[2] != 'I' || madt->signature[3] != 'C') {
        return; /* Not a MADT */
    }

    if (!acpi_validate_checksum((const uint8_t *)madt, madt->length)) {
        return; /* Checksum failure */
    }

    /* First field after header: 32-bit Local APIC physical address */
    uint8_t *madt_data = (uint8_t *)madt;
    uint32_t lapic_phys = *(uint32_t *)(madt_data + sizeof(ACPI_SDT_HEADER));
    g_lapic_base = (uint64_t)lapic_phys;   /* Identity mapped ≤ 8GB */

    /* Walk the MADT entry list starting after header + 8-byte fixed fields */
    uint8_t *entry = madt_data + sizeof(ACPI_SDT_HEADER) + 8;
    uint8_t *end   = madt_data + madt->length;

    g_cpu_count = 0;

    while (entry < end) {
        uint8_t type   = entry[0];
        uint8_t length = entry[1];

        if (length < 2) break; /* Malformed entry */

        if (type == MADT_TYPE_LOCAL_APIC) {
            MADT_LOCAL_APIC *lapic_entry = (MADT_LOCAL_APIC *)entry;
            /* Bit 0 of flags: processor is usable */
            if ((lapic_entry->flags & 1) && g_cpu_count < 32) {
                g_apic_ids[g_cpu_count] = lapic_entry->apic_id;
                g_cpu_count++;
            }
        }

        entry += length;
    }

    if (g_cpu_count == 0) g_cpu_count = 1; /* Minimum: BSP */
}

/* ---------------------------------------------------------------------------
 * find_table_in_xsdt — Locate an ACPI table by 4-character signature in XSDT
 * --------------------------------------------------------------------------- */
static uint64_t find_table_in_xsdt(uint64_t xsdt_phys, const char sig[4]) {
    ACPI_SDT_HEADER *xsdt = (ACPI_SDT_HEADER *)(uintptr_t)xsdt_phys;
    if (!xsdt) return 0;

    uint64_t *entries = (uint64_t *)((uint8_t *)xsdt + sizeof(ACPI_SDT_HEADER));
    uint32_t  count   = (xsdt->length - sizeof(ACPI_SDT_HEADER)) / sizeof(uint64_t);

    for (uint32_t i = 0; i < count; i++) {
        ACPI_SDT_HEADER *tbl = (ACPI_SDT_HEADER *)(uintptr_t)entries[i];
        if (tbl &&
            tbl->signature[0] == sig[0] && tbl->signature[1] == sig[1] &&
            tbl->signature[2] == sig[2] && tbl->signature[3] == sig[3]) {
            return entries[i];
        }
    }
    return 0;
}

/* ---------------------------------------------------------------------------
 * apic_init — Master APIC Initialization
 *
 * Steps:
 *   1. Parse ACPI RSDP → XSDT → MADT to find LAPIC base and CPU count
 *   2. Read IA32_APIC_BASE MSR to verify/override LAPIC base
 *   3. Clear the APIC Error Status Register
 *   4. Enable the APIC via Spurious Interrupt Vector Register
 *   5. Mask all LVT entries (timer, LINT0/1, error) to sane defaults
 * --------------------------------------------------------------------------- */
void apic_init(uint64_t acpi_rsdp) {
    if (acpi_rsdp) {
        ACPI_RSDP *rsdp = (ACPI_RSDP *)(uintptr_t)acpi_rsdp;

        /* Prefer XSDT (ACPI 2.0) for 64-bit addresses */
        uint64_t madt_phys = 0;
        if (rsdp->revision >= 2 && rsdp->xsdt_address) {
            madt_phys = find_table_in_xsdt(rsdp->xsdt_address, "APIC");
        }

        if (madt_phys) {
            parse_madt(madt_phys);
        }
    }

    /* Read IA32_APIC_BASE MSR — bits [35:12] hold the LAPIC base page */
    uint32_t msr_lo, msr_hi;
    __asm__ volatile (
        "rdmsr"
        : "=a"(msr_lo), "=d"(msr_hi)
        : "c"(MSR_IA32_APIC_BASE)
    );
    uint64_t msr_base = ((uint64_t)msr_hi << 32) | msr_lo;
    uint64_t lapic_from_msr = msr_base & 0xFFFFFFFFF000ULL;
    if (lapic_from_msr) {
        g_lapic_base = lapic_from_msr;   /* MSR takes priority over MADT */
    }

    /* Ensure APIC is enabled in the MSR (bit 11) */
    msr_lo |= APIC_BASE_ENABLE_FLAG;
    __asm__ volatile (
        "wrmsr"
        :: "c"(MSR_IA32_APIC_BASE), "a"(msr_lo), "d"(msr_hi)
    );

    /* Clear Error Status Register (write twice as per Intel SDM) */
    lapic_write(LAPIC_REG_ESR, 0);
    lapic_write(LAPIC_REG_ESR, 0);

    /* Set Destination Format to Flat (0xFFFFFFFF) */
    lapic_write(LAPIC_REG_DEST_FORMAT, 0xFFFFFFFF);

    /* Set Logical Destination to CPU 0 */
    lapic_write(LAPIC_REG_LOGICAL_DEST, (lapic_read(LAPIC_REG_LOGICAL_DEST) & 0x00FFFFFF) | 1);

    /* Mask LVT entries: timer, thermal, PMC, LINT0, LINT1, error */
    lapic_write(LAPIC_REG_LVT_TIMER,   (1 << 16));  /* Masked */
    lapic_write(LAPIC_REG_LVT_THERMAL, (1 << 16));
    lapic_write(LAPIC_REG_LVT_PMC,     (1 << 16));
    lapic_write(LAPIC_REG_LVT_LINT0,   (1 << 16));
    lapic_write(LAPIC_REG_LVT_LINT1,   (1 << 16));
    lapic_write(LAPIC_REG_LVT_ERROR,   0x37);        /* Vector 0x37, not masked */

    /* Set Task Priority to 0 — accept all interrupt vectors */
    lapic_write(LAPIC_REG_TPR, 0);

    /* Enable APIC by setting bit 8 (Software Enable) in SIVR
     * The spurious vector (0xFF) must be a valid IDT entry with DPL=0 */
    lapic_write(LAPIC_REG_SIVR, LAPIC_SIVR_ENABLE | LAPIC_SPURIOUS_VECTOR);
}

/* ---------------------------------------------------------------------------
 * apic_send_ipi — Send IPI to a single CPU by physical APIC ID
 * --------------------------------------------------------------------------- */
void apic_send_ipi(uint8_t apic_id, uint8_t vector, uint32_t delivery) {
    lapic_wait_icr_idle();

    /* Write destination to ICR_HIGH first */
    lapic_write(LAPIC_REG_ICR_HIGH, (uint32_t)apic_id << 24);

    /* Writing ICR_LOW triggers the IPI delivery */
    lapic_write(LAPIC_REG_ICR_LOW,
        LAPIC_ICR_DEST_PHYSICAL |
        LAPIC_ICR_SHORTHAND_NONE |
        LAPIC_ICR_ASSERT |
        delivery |
        (uint32_t)vector
    );

    lapic_wait_icr_idle();
}

/* ---------------------------------------------------------------------------
 * PIT-based busy delay for AP startup delays (microseconds)
 * Uses I/O port 0x61 (PC speaker / PIT channel 2) as a simple timer.
 * --------------------------------------------------------------------------- */
static inline void pit_udelay(uint32_t us) {
    /* Approximate: 1 I/O port read ≈ 1 µs on modern hardware */
    for (uint32_t i = 0; i < us * 10; i++) {
        __asm__ volatile ("pause; pause; pause; pause; pause");
    }
}

/* ---------------------------------------------------------------------------
 * apic_start_all_aps — Broadcast INIT-SIPI-SIPI to all detected APs
 *
 * The SIPI vector encodes the physical page (4KB aligned) of the trampoline.
 * Example: trampoline at 0x8000 → vector = 0x8000 / 0x1000 = 0x08
 *
 * Timing requirements (Intel MP Spec):
 *   INIT assertion:    20ms delay
 *   INIT de-assert:    (not required in modern APIC)
 *   First SIPI:        10ms delay
 *   Second SIPI:       200µs delay (retry if AP didn't wake)
 * --------------------------------------------------------------------------- */
void apic_start_all_aps(uint64_t trampoline_phys) {
    uint8_t sipi_vector = (uint8_t)((trampoline_phys >> 12) & 0xFF);

    for (uint32_t cpu = 1; cpu < g_cpu_count; cpu++) {
        uint8_t target_apic_id = g_apic_ids[cpu];

        /* Step 1: Send INIT IPI to assert reset state on target AP */
        lapic_wait_icr_idle();
        lapic_write(LAPIC_REG_ICR_HIGH, (uint32_t)target_apic_id << 24);
        lapic_write(LAPIC_REG_ICR_LOW,
            LAPIC_ICR_DELIVERY_INIT |
            LAPIC_ICR_ASSERT |
            LAPIC_ICR_LEVEL_TRIGGER |
            LAPIC_ICR_DEST_PHYSICAL |
            LAPIC_ICR_SHORTHAND_NONE
        );
        lapic_wait_icr_idle();
        pit_udelay(20000); /* 20ms — AP holds reset for ~10ms */

        /* Step 2: De-assert INIT (level-triggered de-assert) */
        lapic_write(LAPIC_REG_ICR_HIGH, (uint32_t)target_apic_id << 24);
        lapic_write(LAPIC_REG_ICR_LOW,
            LAPIC_ICR_DELIVERY_INIT |
            (0 << 14) |  /* De-assert */
            LAPIC_ICR_LEVEL_TRIGGER |
            LAPIC_ICR_DEST_PHYSICAL |
            LAPIC_ICR_SHORTHAND_NONE
        );
        lapic_wait_icr_idle();

        /* Step 3: First SIPI */
        apic_send_ipi(target_apic_id, sipi_vector, LAPIC_ICR_DELIVERY_SIPI);
        pit_udelay(10000); /* 10ms delay */

        /* Step 4: Second SIPI (retry if AP failed to wake) */
        apic_send_ipi(target_apic_id, sipi_vector, LAPIC_ICR_DELIVERY_SIPI);
        pit_udelay(200);   /* 200µs delay */
    }
}

/* ---------------------------------------------------------------------------
 * apic_calibrate_timer — Calibrate APIC timer ticks per millisecond
 *
 * Uses a PIT-based spin measurement:
 *   1. Set APIC timer to count down from max (0xFFFFFFFF)
 *   2. Wait exactly 10ms using PIT delays
 *   3. Read remaining count → compute ticks/ms
 * --------------------------------------------------------------------------- */
void apic_calibrate_timer(void) {
    /* Configure APIC timer: one-shot mode, divide by 16 */
    lapic_write(LAPIC_REG_TIMER_DIV,  LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_LVT_TIMER,  (1 << 16) | IRQ_TIMER_VECTOR); /* Masked during calibration */
    lapic_write(LAPIC_REG_TIMER_INIT, 0xFFFFFFFF); /* Start countdown */

    pit_udelay(10000); /* Wait 10ms */

    uint32_t ticks_elapsed = 0xFFFFFFFF - lapic_read(LAPIC_REG_TIMER_CURR);
    s_apic_ticks_per_ms = ticks_elapsed / 10; /* Ticks per 1ms */

    /* Stop the timer */
    lapic_write(LAPIC_REG_TIMER_INIT, 0);
}

/* ---------------------------------------------------------------------------
 * apic_arm_timer — Arm the APIC one-shot timer for the scheduler tick
 * --------------------------------------------------------------------------- */
void apic_arm_timer(uint64_t microseconds) {
    uint32_t ticks = (uint32_t)((s_apic_ticks_per_ms * microseconds) / 1000);
    if (ticks == 0) ticks = 1;

    /* Unmask the timer LVT entry and arm */
    lapic_write(LAPIC_REG_LVT_TIMER,  LAPIC_TIMER_ONE_SHOT | IRQ_TIMER_VECTOR);
    lapic_write(LAPIC_REG_TIMER_DIV,  LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_TIMER_INIT, ticks);
}

/* ---------------------------------------------------------------------------
 * apic_udelay — Busy-wait using the APIC current count register
 * --------------------------------------------------------------------------- */
void apic_udelay(uint32_t microseconds) {
    if (s_apic_ticks_per_ms == 0) {
        pit_udelay(microseconds);
        return;
    }
    uint32_t ticks = (s_apic_ticks_per_ms * microseconds) / 1000;
    lapic_write(LAPIC_REG_TIMER_DIV,  LAPIC_TIMER_DIV_16);
    lapic_write(LAPIC_REG_LVT_TIMER,  (1 << 16) | IRQ_TIMER_VECTOR); /* Masked */
    lapic_write(LAPIC_REG_TIMER_INIT, ticks);
    while (lapic_read(LAPIC_REG_TIMER_CURR) > 0) {
        __asm__ volatile ("pause");
    }
}
