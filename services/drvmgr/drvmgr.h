/**
 * @file drvmgr.h — Xenithra OS — Driver Manager Service
 *
 * Plug-and-Play device manager running as a Session 0 kernel thread.
 * Responsibilities:
 *   - PCI bus enumeration (config space scanning)
 *   - Device ↔ driver matching (vendor/device ID table)
 *   - Driver loading / initialization sequencing
 *   - Device status tracking and IPC event publishing
 *   - Hot-plug support (USB, PCIe hot-plug via ACPI GPE)
 */

#ifndef _SERVICE_DRVMGR_H_
#define _SERVICE_DRVMGR_H_

#include <stdint.h>
#include <stddef.h>

/* ── PCI Config Space I/O Ports ─────────────────────────────────────── */
#define PCI_CONFIG_ADDR  0xCF8
#define PCI_CONFIG_DATA  0xCFC

/* ── PCI Class Codes ─────────────────────────────────────────────────── */
#define PCI_CLASS_STORAGE        0x01
#define PCI_CLASS_NETWORK        0x02
#define PCI_CLASS_DISPLAY        0x03
#define PCI_CLASS_MULTIMEDIA     0x04
#define PCI_CLASS_BRIDGE         0x06
#define PCI_CLASS_SERIAL         0x0C   /* USB, FireWire, etc. */
#define PCI_CLASS_INPUT          0x09
#define PCI_CLASS_PROCESSOR      0x0B

#define PCI_SUBCLASS_USB_XHCI    0x03
#define PCI_SUBCLASS_USB_EHCI    0x03
#define PCI_SUBCLASS_VGA         0x00
#define PCI_SUBCLASS_ETHERNET    0x00

/* ── PCI Config Register Offsets ─────────────────────────────────────── */
#define PCI_CFG_VENDOR_ID     0x00
#define PCI_CFG_DEVICE_ID     0x02
#define PCI_CFG_COMMAND       0x04
#define PCI_CFG_STATUS        0x06
#define PCI_CFG_REVISION      0x08
#define PCI_CFG_PROG_IF       0x09
#define PCI_CFG_SUBCLASS      0x0A
#define PCI_CFG_CLASS         0x0B
#define PCI_CFG_HEADER_TYPE   0x0E
#define PCI_CFG_BAR0          0x10
#define PCI_CFG_BAR1          0x14
#define PCI_CFG_BAR2          0x18
#define PCI_CFG_BAR3          0x1C
#define PCI_CFG_BAR4          0x20
#define PCI_CFG_BAR5          0x24
#define PCI_CFG_IRQ_LINE      0x3C
#define PCI_CFG_IRQ_PIN       0x3D

/* ── PCI Command bits ────────────────────────────────────────────────── */
#define PCI_CMD_IO_ENABLE     (1U << 0)
#define PCI_CMD_MEM_ENABLE    (1U << 1)
#define PCI_CMD_BUSMASTER     (1U << 2)
#define PCI_CMD_INT_DISABLE   (1U << 10)

/* ── Device status ───────────────────────────────────────────────────── */
typedef enum {
    DRVMGR_STATUS_UNKNOWN   = 0,
    DRVMGR_STATUS_OK        = 1,   /* Driver loaded, device working */
    DRVMGR_STATUS_ERROR     = 2,   /* Driver failed to init */
    DRVMGR_STATUS_DISABLED  = 3,   /* Manually disabled */
    DRVMGR_STATUS_NO_DRIVER = 4,   /* No driver available */
    DRVMGR_STATUS_LOADING   = 5,   /* Currently loading */
} DrvmgrStatus;

/* ── PCI device descriptor ───────────────────────────────────────────── */
typedef struct {
    uint8_t  bus;
    uint8_t  slot;
    uint8_t  func;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    uint8_t  revision;
    uint8_t  header_type;
    uint32_t bar[6];           /* Base Address Registers (mapped) */
    uint8_t  irq_line;
    uint8_t  irq_pin;

    /* Driver binding */
    char         driver_name[32];  /* e.g. "e1000", "xhci_hcd", "vbe" */
    char         driver_version[16];
    DrvmgrStatus status;
    char         device_name[64];  /* Human-readable name */
    char         manufacturer[48];
    uint8_t      driver_signed;
} PciDevice;

/* ── Driver descriptor ───────────────────────────────────────────────── */
typedef struct {
    const char *name;           /* Short driver name */
    const char *description;    /* Human-readable description */
    const char *version;
    uint16_t    vendor_id;      /* 0 = match any */
    uint16_t    device_id;      /* 0 = match by class */
    uint8_t     class_code;     /* 0 = match by vendor/device only */
    uint8_t     subclass;
    uint8_t     prog_if;        /* 0xFF = match any prog_if */
    int        (*probe)(PciDevice *dev);   /* Returns 0 if driver claims device */
    int        (*init)(PciDevice *dev);    /* Initialize device. Returns 0. */
    void       (*remove)(PciDevice *dev);  /* Clean up on hot-unplug. */
    uint8_t     signed_driver;  /* 1 if driver is kernel-signed */
} DriverDescriptor;

/* ── Known driver table ──────────────────────────────────────────────── */
/*
 * Drivers are registered at build time via drvmgr_register_driver().
 * The table below is populated during kernel Phase 1B init.
 * Order matters — first matching driver wins.
 */
#define DRVMGR_MAX_DEVICES 64
#define DRVMGR_MAX_DRIVERS 32

/* ── Public API ──────────────────────────────────────────────────────── */

/**
 * @brief Initialize the Driver Manager.
 *        Enumerates all PCI buses (0-255), slots (0-31), functions (0-7).
 *        For each device found, attempts to match and load a driver.
 *        Called from kernel Phase 1B (after heap + APIC init).
 * @return Number of devices successfully initialized.
 */
int  drvmgr_init(void);

/**
 * @brief Driver Manager daemon main loop (runs as kernel thread).
 *        Handles hot-plug events, re-probe, and IPC queries.
 *        Never returns.
 */
void drvmgr_thread(void);

/**
 * @brief Register a driver with the manager.
 *        Must be called before drvmgr_init().
 */
void drvmgr_register_driver(const DriverDescriptor *drv);

/**
 * @brief Force re-enumeration of a PCI bus.
 * @param bus  PCI bus number (0-255).
 */
void drvmgr_scan_bus(uint8_t bus);

/** Get all enumerated devices. Returns count. */
int  drvmgr_get_devices(PciDevice **out_array, int max_count);

/** Get a device by vendor+device ID. Returns NULL if not found. */
PciDevice *drvmgr_find_device(uint16_t vendor, uint16_t device);

/** Get a device by PCI location. */
PciDevice *drvmgr_find_by_location(uint8_t bus, uint8_t slot, uint8_t func);

/** Enable/disable a device. */
int  drvmgr_set_device_enabled(PciDevice *dev, uint8_t enabled);

/** Publish all device statuses via GUI IPC (for ManagementPanel). */
void drvmgr_publish_all(void);

/**
 * @brief Read a PCI config space register.
 * @param bus, slot, func  PCI address.
 * @param offset  Register offset (must be 4-byte aligned).
 * @return 32-bit register value.
 */
uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
uint8_t  pci_config_read8 (uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);

/** Write a PCI config space register. */
void pci_config_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val);
void pci_config_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val);

/** Enable bus mastering DMA for a PCI device. */
void pci_enable_busmaster(uint8_t bus, uint8_t slot, uint8_t func);

/** Map a PCI BAR to virtual address. Returns mapped VA or 0. */
uint64_t pci_map_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_idx);

#endif /* _SERVICE_DRVMGR_H_ */
