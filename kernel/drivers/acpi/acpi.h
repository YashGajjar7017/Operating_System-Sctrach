/**
 * @file acpi.h — Xenithra OS — ACPI Power Management Subsystem
 *
 * Parses ACPI tables (RSDP, RSDT/XSDT, FADT, MADT, DSDT) and
 * provides power management: S0-S5 sleep states, CPU C-states,
 * GPE handlers, and battery status.
 */

#ifndef _DRIVER_ACPI_H_
#define _DRIVER_ACPI_H_

#include <stdint.h>
#include <stddef.h>

/* ── ACPI Table Signatures ───────────────────────────────────────────── */
#define ACPI_SIG_RSDP  "RSD PTR "  /* Root System Description Pointer */
#define ACPI_SIG_RSDT  "RSDT"       /* Root System Description Table */
#define ACPI_SIG_XSDT  "XSDT"       /* Extended System Desc Table */
#define ACPI_SIG_FADT  "FACP"       /* Fixed ACPI Description Table */
#define ACPI_SIG_MADT  "APIC"       /* Multiple APIC Desc Table */
#define ACPI_SIG_HPET  "HPET"       /* High Precision Event Timer */
#define ACPI_SIG_MCFG  "MCFG"       /* PCI Express Memory Mapped Config */
#define ACPI_SIG_DSDT  "DSDT"       /* Diff System Desc Table (AML) */
#define ACPI_SIG_SSDT  "SSDT"       /* Secondary System Desc Table */

/* ── ACPI Common Table Header (8 bytes) ──────────────────────────────── */
typedef struct __attribute__((packed)) {
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} AcpiTableHeader;

/* ── RSDP v1 (20 bytes) ──────────────────────────────────────────────── */
typedef struct __attribute__((packed)) {
    char     signature[8];   /* "RSD PTR " */
    uint8_t  checksum;
    char     oem_id[6];
    uint8_t  revision;       /* 0 = v1, 2 = v2 */
    uint32_t rsdt_address;   /* 32-bit physical address of RSDT */
    /* v2 extension */
    uint32_t length;
    uint64_t xsdt_address;   /* 64-bit physical address of XSDT */
    uint8_t  ext_checksum;
    uint8_t  reserved[3];
} AcpiRsdp;

/* ── FADT (Fixed ACPI Description Table) — key fields ───────────────── */
typedef struct __attribute__((packed)) {
    AcpiTableHeader header;
    uint32_t firmware_ctrl;
    uint32_t dsdt;
    uint8_t  reserved0;
    uint8_t  preferred_pm_profile;
    uint16_t sci_int;          /* SCI Interrupt (IRQ for ACPI events) */
    uint32_t smi_cmd;          /* SMI Command Port */
    uint8_t  acpi_enable;      /* Write to SMI_CMD to enable ACPI mode */
    uint8_t  acpi_disable;
    uint8_t  s4bios_req;
    uint8_t  pstate_cnt;
    uint32_t pm1a_evt_blk;     /* PM1a Event Register Block */
    uint32_t pm1b_evt_blk;
    uint32_t pm1a_cnt_blk;     /* PM1a Control Register Block */
    uint32_t pm1b_cnt_blk;
    uint32_t pm2_cnt_blk;
    uint32_t pm_tmr_blk;       /* Power Management Timer */
    uint32_t gpe0_blk;         /* General Purpose Event 0 */
    uint32_t gpe1_blk;
    uint8_t  pm1_evt_len;
    uint8_t  pm1_cnt_len;
    uint8_t  pm2_cnt_len;
    uint8_t  pm_tmr_len;
    uint8_t  gpe0_blk_len;
    uint8_t  gpe1_blk_len;
    uint8_t  gpe1_base;
    uint8_t  cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t  duty_offset;
    uint8_t  duty_width;
    uint8_t  day_alrm;
    uint8_t  mon_alrm;
    uint8_t  century;
    uint16_t iapc_boot_arch;
    uint8_t  reserved1;
    uint32_t flags;
    /* ... truncated for brevity — full FADT is 244 bytes */
} AcpiFadt;

/* ── ACPI Sleep State Values (SLP_TYP) ───────────────────────────────── */
#define ACPI_SLEEP_S0   0   /* Working */
#define ACPI_SLEEP_S1   1   /* CPU off, RAM on */
#define ACPI_SLEEP_S3   3   /* Suspend to RAM */
#define ACPI_SLEEP_S4   4   /* Suspend to Disk (hibernate) */
#define ACPI_SLEEP_S5   5   /* Soft Power Off */

/* ── PM1 Control Register bits ───────────────────────────────────────── */
#define ACPI_PM1_SCI_EN  (1U << 0)  /* SCI Enable */
#define ACPI_PM1_BM_RLD  (1U << 1)  /* Bus Master Reload */
#define ACPI_PM1_SLP_EN  (1U << 13) /* Sleep Enable */
#define ACPI_PM1_SLP_TYP_SHIFT 10   /* Sleep Type shift */

/* ── Power state ─────────────────────────────────────────────────────── */
typedef enum {
    ACPI_POWER_STATE_S0 = 0,   /* Running */
    ACPI_POWER_STATE_S3,       /* Sleep */
    ACPI_POWER_STATE_S4,       /* Hibernate */
    ACPI_POWER_STATE_S5,       /* Shutdown */
} AcpiPowerState;

/* ── Battery status ──────────────────────────────────────────────────── */
typedef struct {
    uint8_t  present;
    uint8_t  charging;
    uint32_t capacity_pct;      /* 0–100% */
    uint32_t remaining_mwh;
    uint32_t full_charge_mwh;
    uint32_t discharge_rate_mw;
    uint32_t voltage_mv;
    int32_t  temperature_c;
} AcpiBatteryStatus;

/* ── Public API ──────────────────────────────────────────────────────── */

/**
 * @brief Initialize ACPI subsystem.
 * @param rsdp_phys Physical address of RSDP (from bootloader).
 * @return 0 on success, -1 if ACPI tables not found.
 */
int  acpi_init(uint64_t rsdp_phys);

/** Enable ACPI mode (write ACPI_ENABLE to SMI_CMD). */
void acpi_enable(void);

/** Perform a power state transition. */
void acpi_sleep(AcpiPowerState state);

/** System shutdown via ACPI S5 state. Does not return on success. */
void acpi_shutdown(void);

/** System reboot via ACPI reset register. Does not return on success. */
void acpi_reboot(void);

/** Get current battery status. Returns 0 if battery present. */
int  acpi_get_battery(AcpiBatteryStatus *out);

/** Handle ACPI SCI interrupt (from IRQ dispatcher). */
void acpi_sci_handler(void);

/** Get PM1A Event Block I/O port. */
uint32_t acpi_get_pm1a_evt(void);

/** Get PM1A Control Block I/O port. */
uint32_t acpi_get_pm1a_cnt(void);

/** Read the ACPI PM timer (24 or 32-bit). */
uint32_t acpi_pm_timer_read(void);

/** Wait approx N microseconds using PM timer. */
void acpi_pm_timer_wait_us(uint32_t us);

/** Lookup and return pointer to an ACPI table by 4-char signature. */
AcpiTableHeader *acpi_find_table(const char *sig);

#endif /* _DRIVER_ACPI_H_ */
