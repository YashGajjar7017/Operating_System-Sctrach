/**
 * @file drvmgr.c — Xenithra OS — Driver Manager Service Implementation
 */

#include "drvmgr.h"
#include "../../kernel/kstring.h"
#include "../../kernel/gui/gui_ipc.h"

static PciDevice         g_devices[DRVMGR_MAX_DEVICES];
static int               g_device_count = 0;
static DriverDescriptor  g_drivers[DRVMGR_MAX_DRIVERS];
static int               g_driver_count = 0;

static inline void outl_pci(uint16_t port, uint32_t val) {
    __asm__ volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint32_t inl_pci(uint16_t port) {
    uint32_t ret;
    __asm__ volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

uint32_t pci_config_read32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t addr = (1U << 31)
                  | ((uint32_t)bus << 16)
                  | ((uint32_t)slot << 11)
                  | ((uint32_t)func << 8)
                  | (offset & 0xFC);
    outl_pci(PCI_CONFIG_ADDR, addr);
    return inl_pci(PCI_CONFIG_DATA);
}

uint16_t pci_config_read16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t val = pci_config_read32(bus, slot, func, offset);
    return (uint16_t)((val >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t pci_config_read8(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t val = pci_config_read32(bus, slot, func, offset);
    return (uint8_t)((val >> ((offset & 3) * 8)) & 0xFF);
}

void pci_config_write32(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t val) {
    uint32_t addr = (1U << 31)
                  | ((uint32_t)bus << 16)
                  | ((uint32_t)slot << 11)
                  | ((uint32_t)func << 8)
                  | (offset & 0xFC);
    outl_pci(PCI_CONFIG_ADDR, addr);
    outl_pci(PCI_CONFIG_DATA, val);
}

void pci_config_write16(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t val) {
    uint32_t cur = pci_config_read32(bus, slot, func, offset);
    int shift = (offset & 2) * 8;
    cur = (cur & ~(0xFFFFU << shift)) | ((uint32_t)val << shift);
    pci_config_write32(bus, slot, func, offset, cur);
}

void pci_enable_busmaster(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t cmd = pci_config_read16(bus, slot, func, PCI_CFG_COMMAND);
    cmd |= (PCI_CMD_BUSMASTER | PCI_CMD_MEM_ENABLE | PCI_CMD_IO_ENABLE);
    pci_config_write16(bus, slot, func, PCI_CFG_COMMAND, cmd);
}

uint64_t pci_map_bar(uint8_t bus, uint8_t slot, uint8_t func, uint8_t bar_idx) {
    if (bar_idx >= 6) return 0;
    uint32_t bar = pci_config_read32(bus, slot, func, PCI_CFG_BAR0 + bar_idx * 4);
    if (bar & 1) {
        return (uint64_t)(bar & ~3U); /* I/O bar */
    }
    return (uint64_t)(bar & ~0xFU);   /* Memory bar */
}

void drvmgr_register_driver(const DriverDescriptor *drv) {
    if (!drv || g_driver_count >= DRVMGR_MAX_DRIVERS) return;
    g_drivers[g_driver_count++] = *drv;
}

void drvmgr_scan_bus(uint8_t bus) {
    for (uint8_t slot = 0; slot < 32; slot++) {
        uint16_t vendor = pci_config_read16(bus, slot, 0, PCI_CFG_VENDOR_ID);
        if (vendor == 0xFFFF || vendor == 0x0000) continue;

        uint8_t header_type = pci_config_read8(bus, slot, 0, PCI_CFG_HEADER_TYPE);
        uint8_t max_func = (header_type & 0x80) ? 8 : 1;

        for (uint8_t func = 0; func < max_func; func++) {
            uint16_t dev_vendor = pci_config_read16(bus, slot, func, PCI_CFG_VENDOR_ID);
            if (dev_vendor == 0xFFFF || dev_vendor == 0x0000) continue;

            if (g_device_count >= DRVMGR_MAX_DEVICES) return;

            PciDevice *dev = &g_devices[g_device_count++];
            memset(dev, 0, sizeof(*dev));
            dev->bus         = bus;
            dev->slot        = slot;
            dev->func        = func;
            dev->vendor_id   = dev_vendor;
            dev->device_id   = pci_config_read16(bus, slot, func, PCI_CFG_DEVICE_ID);
            dev->class_code  = pci_config_read8(bus, slot, func, PCI_CFG_CLASS);
            dev->subclass    = pci_config_read8(bus, slot, func, PCI_CFG_SUBCLASS);
            dev->prog_if     = pci_config_read8(bus, slot, func, PCI_CFG_PROG_IF);
            dev->revision    = pci_config_read8(bus, slot, func, PCI_CFG_REVISION);
            dev->header_type = header_type;
            dev->irq_line    = pci_config_read8(bus, slot, func, PCI_CFG_IRQ_LINE);
            dev->irq_pin     = pci_config_read8(bus, slot, func, PCI_CFG_IRQ_PIN);

            for (int b = 0; b < 6; b++) {
                dev->bar[b] = pci_config_read32(bus, slot, func, PCI_CFG_BAR0 + b * 4);
            }

            dev->status = DRVMGR_STATUS_OK;
            dev->driver_signed = 1;

            if (dev->vendor_id == 0x8086 && dev->device_id == 0x100E) {
                strcpy(dev->driver_name, "e1000");
                strcpy(dev->device_name, "Intel PRO/1000 Gigabit Adapter");
                strcpy(dev->manufacturer, "Intel Corporation");
            } else if (dev->vendor_id == 0x10EC && dev->device_id == 0x8139) {
                strcpy(dev->driver_name, "rtl8139");
                strcpy(dev->device_name, "Realtek RTL8139 Fast Ethernet");
                strcpy(dev->manufacturer, "Realtek Semiconductor");
            } else if (dev->class_code == PCI_CLASS_SERIAL && dev->subclass == PCI_SUBCLASS_USB_XHCI) {
                strcpy(dev->driver_name, "xhci_hcd");
                strcpy(dev->device_name, "xHCI USB 3.0 Host Controller");
                strcpy(dev->manufacturer, "Generic USB");
            } else if (dev->class_code == PCI_CLASS_DISPLAY) {
                strcpy(dev->driver_name, "vbe");
                strcpy(dev->device_name, "Standard VGA / VESA Display");
                strcpy(dev->manufacturer, "VESA Compliant");
            } else {
                strcpy(dev->driver_name, "pci_generic");
                strcpy(dev->device_name, "PCI Standard Device");
                strcpy(dev->manufacturer, "Generic");
            }
        }
    }
}

int drvmgr_init(void) {
    g_device_count = 0;
    drvmgr_scan_bus(0);
    return g_device_count;
}

void drvmgr_thread(void) {
    while (1) {
        __asm__ volatile ("pause");
    }
}

int drvmgr_get_devices(PciDevice **out_array, int max_count) {
    if (!out_array) return 0;
    int count = g_device_count < max_count ? g_device_count : max_count;
    for (int i = 0; i < count; i++) {
        out_array[i] = &g_devices[i];
    }
    return count;
}

PciDevice *drvmgr_find_device(uint16_t vendor, uint16_t device) {
    for (int i = 0; i < g_device_count; i++) {
        if (g_devices[i].vendor_id == vendor && g_devices[i].device_id == device) {
            return &g_devices[i];
        }
    }
    return NULL;
}

PciDevice *drvmgr_find_by_location(uint8_t bus, uint8_t slot, uint8_t func) {
    for (int i = 0; i < g_device_count; i++) {
        if (g_devices[i].bus == bus && g_devices[i].slot == slot && g_devices[i].func == func) {
            return &g_devices[i];
        }
    }
    return NULL;
}

int drvmgr_set_device_enabled(PciDevice *dev, uint8_t enabled) {
    if (!dev) return -1;
    dev->status = enabled ? DRVMGR_STATUS_OK : DRVMGR_STATUS_DISABLED;
    return 0;
}

void drvmgr_publish_all(void) {
    for (int i = 0; i < g_device_count; i++) {
        gui_ipc_send_service_event(
            g_devices[i].device_name,
            0,
            0,
            "Working"
        );
    }
}
