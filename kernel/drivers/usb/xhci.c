/**
 * @file xhci.c — Xenithra OS — xHCI USB 3.0 Host Controller Driver Implementation
 */

#include "xhci.h"
#include "../../kstring.h"
#include "../../../services/drvmgr/drvmgr.h"

static XhciController g_xhci;
static uint8_t        g_xhci_found = 0;

int xhci_init(void) {
    /* Find xHCI controller by class 0x0C, subclass 0x03, prog_if 0x30 */
    PciDevice *pci = NULL;
    PciDevice *devs[DRVMGR_MAX_DEVICES];
    int count = drvmgr_get_devices(devs, DRVMGR_MAX_DEVICES);
    for (int i = 0; i < count; i++) {
        if (devs[i]->class_code == PCI_CLASS_SERIAL &&
            devs[i]->subclass == PCI_SUBCLASS_USB_XHCI &&
            devs[i]->prog_if == XHCI_PCI_PROGIF)
        {
            pci = devs[i];
            break;
        }
    }

    if (!pci) return -1;

    memset(&g_xhci, 0, sizeof(g_xhci));
    pci_enable_busmaster(pci->bus, pci->slot, pci->func);

    g_xhci.mmio_base = (volatile uint8_t *)pci_map_bar(pci->bus, pci->slot, pci->func, 0);
    g_xhci.irq       = pci->irq_line;

    if (g_xhci.mmio_base) {
        g_xhci.cap_len = g_xhci.mmio_base[XHCI_CAP_CAPLENGTH];
        g_xhci.op_regs = g_xhci.mmio_base + g_xhci.cap_len;
    }

    g_xhci_found = 1;
    return 0;
}

void xhci_poll_events(void) {
}

void xhci_irq_handler(void) {
}

void xhci_enumerate_ports(void) {
}

int xhci_control_transfer(uint8_t slot, uint8_t request_type, uint8_t request,
                          uint16_t value, uint16_t index,
                          void *data, uint16_t len)
{
    (void)slot; (void)request_type; (void)request;
    (void)value; (void)index; (void)data; (void)len;
    return -1;
}

XhciController *xhci_get_controller(void) {
    return g_xhci_found ? &g_xhci : NULL;
}
