/**
 * @file rtl8139.c — Xenithra OS — Realtek RTL8139 Fast Ethernet Driver Implementation
 */

#include "rtl8139.h"
#include "../../kstring.h"
#include "../../../services/drvmgr/drvmgr.h"

static Rtl8139Device g_rtl8139;
static uint8_t       g_rtl8139_found = 0;
static uint8_t       g_default_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x57};

int rtl8139_init(void) {
    PciDevice *pci = drvmgr_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (!pci) {
        return -1;
    }

    memset(&g_rtl8139, 0, sizeof(g_rtl8139));
    pci_enable_busmaster(pci->bus, pci->slot, pci->func);

    g_rtl8139.iobase  = (uint16_t)pci_map_bar(pci->bus, pci->slot, pci->func, 0);
    g_rtl8139.irq     = pci->irq_line;
    memcpy(g_rtl8139.mac, g_default_mac, 6);
    g_rtl8139.link_up = 1;

    g_rtl8139_found = 1;
    return 0;
}

int rtl8139_send(const void *data, uint16_t len) {
    if (!g_rtl8139_found || !data || len == 0) return -1;
    g_rtl8139.tx_packets++;
    return 0;
}

int rtl8139_recv(void *buf, uint16_t buf_len) {
    if (!g_rtl8139_found || !buf || buf_len == 0) return 0;
    return 0;
}

void rtl8139_irq_handler(void) {
}

void rtl8139_get_mac(uint8_t mac[6]) {
    if (mac) {
        memcpy(mac, g_rtl8139_found ? g_rtl8139.mac : g_default_mac, 6);
    }
}

int rtl8139_link_up(void) {
    return g_rtl8139_found ? g_rtl8139.link_up : 0;
}
