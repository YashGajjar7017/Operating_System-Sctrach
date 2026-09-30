/**
 * @file acpi.c — Xenithra OS — ACPI Power Management Subsystem Implementation
 */

#include "acpi.h"
#include "../../kstring.h"

static AcpiRsdp        *g_rsdp = NULL;
static AcpiTableHeader *g_rsdt = NULL;
static uint32_t         g_pm1a_cnt = 0x604; /* Default QEMU PM1a */
static uint32_t         g_pm1a_evt = 0x600;
static uint8_t          g_acpi_enabled = 0;

static inline void outw_acpi(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}
static inline void outb_acpi(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint16_t inw_acpi(uint16_t port) {
    uint16_t ret;
    __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline uint32_t inl_acpi(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

int acpi_init(uint64_t rsdp_phys) {
    if (!rsdp_phys) return -1;

    g_rsdp = (AcpiRsdp *)rsdp_phys;
    if (strncmp(g_rsdp->signature, ACPI_SIG_RSDP, 8) != 0) {
        return -1;
    }

    if (g_rsdp->rsdt_address) {
        g_rsdt = (AcpiTableHeader *)(uint64_t)g_rsdp->rsdt_address;
    }

    return 0;
}

void acpi_enable(void) {
    g_acpi_enabled = 1;
}

void acpi_sleep(AcpiPowerState state) {
    if (state == ACPI_POWER_STATE_S5) {
        acpi_shutdown();
    }
}

void acpi_shutdown(void) {
    /* QEMU shutdown port */
    outw_acpi(0x604, 0x2000);
    /* Bochs / older QEMU port */
    outw_acpi(0xB004, 0x2000);
    /* VirtualBox port */
    outw_acpi(0x4004, 0x3400);

    /* Fallback halt loop */
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

void acpi_reboot(void) {
    /* Standard 8042 keyboard controller reset */
    uint8_t good = 0x02;
    while (good & 0x02) {
        __asm__ volatile ("inb %1, %0" : "=a"(good) : "Nd"((uint16_t)0x64));
    }
    outb_acpi(0x64, 0xFE);

    /* Fallback triple fault */
    uint64_t null_idt[2] = {0, 0};
    __asm__ volatile ("lidt (%0); int $3" : : "r"(null_idt));
    while (1) {
        __asm__ volatile ("cli; hlt");
    }
}

int acpi_get_battery(AcpiBatteryStatus *out) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    out->present = 1;
    out->charging = 1;
    out->capacity_pct = 100;
    out->voltage_mv = 12000;
    out->temperature_c = 35;
    return 0;
}

void acpi_sci_handler(void) {
}

uint32_t acpi_get_pm1a_evt(void) {
    return g_pm1a_evt;
}

uint32_t acpi_get_pm1a_cnt(void) {
    return g_pm1a_cnt;
}

uint32_t acpi_pm_timer_read(void) {
    return inl_acpi(0x608);
}

void acpi_pm_timer_wait_us(uint32_t us) {
    uint32_t ticks = (us * 358) / 100; /* ~3.579545 MHz */
    uint32_t start = acpi_pm_timer_read();
    while ((acpi_pm_timer_read() - start) < ticks) {
        __asm__ volatile ("pause");
    }
}

AcpiTableHeader *acpi_find_table(const char *sig) {
    if (!g_rsdt || !sig) return NULL;

    uint32_t entries = (g_rsdt->length - sizeof(AcpiTableHeader)) / 4;
    uint32_t *table_ptrs = (uint32_t *)((uint8_t *)g_rsdt + sizeof(AcpiTableHeader));

    for (uint32_t i = 0; i < entries; i++) {
        AcpiTableHeader *hdr = (AcpiTableHeader *)(uint64_t)table_ptrs[i];
        if (hdr && strncmp(hdr->signature, sig, 4) == 0) {
            return hdr;
        }
    }
    return NULL;
}
