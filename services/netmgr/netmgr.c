/**
 * @file netmgr.c — Xenithra OS — Network Manager Service Implementation
 */

#include "netmgr.h"
#include "../../kernel/kstring.h"
#include "../../kernel/gui/gui_ipc.h"

#ifndef NETMGR_MAX_IFACES
#define NETMGR_MAX_IFACES 8
#endif

static NetInterface g_ifaces[NETMGR_MAX_IFACES];
static int          g_iface_count = 0;
static RouteEntry   g_routes[NETMGR_MAX_ROUTES];
static int          g_route_count = 0;

static void netmgr_init_defaults(void) {
    if (g_iface_count == 0) {
        NetInterface *eth0 = &g_ifaces[0];
        memset(eth0, 0, sizeof(*eth0));
        strcpy(eth0->name, "eth0");
        eth0->type = NETMGR_IFACE_ETHERNET;
        eth0->ipv4 = 0x0F02000A;    /* 10.0.2.15 (QEMU default) */
        eth0->netmask = 0x00FFFFFF; /* 255.255.255.0 */
        eth0->gateway = 0x0202000A; /* 10.0.2.2 */
        eth0->dns[0]  = 0x0302000A; /* 10.0.2.3 */
        eth0->dns_count = 1;
        eth0->link_up = 1;
        eth0->dhcp_state = DHCP_STATE_BOUND;
        g_iface_count = 1;

        RouteEntry *r = &g_routes[0];
        r->dest = 0;
        r->netmask = 0;
        r->gateway = eth0->gateway;
        strcpy(r->iface, "eth0");
        r->metric = 1;
        g_route_count = 1;
    }
}

void netmgr_thread(void) {
    netmgr_init_defaults();
    while (1) {
        __asm__ volatile ("pause");
    }
}

int netmgr_dhcp_discover(const char *iface_name) {
    (void)iface_name;
    return 0;
}

int netmgr_dhcp_release(const char *iface_name) {
    (void)iface_name;
    return 0;
}

int netmgr_dns_resolve(const char *hostname, uint32_t *out_ip) {
    if (!hostname || !out_ip) return -1;
    *out_ip = 0x01010101; /* 1.1.1.1 fallback */
    return 0;
}

int netmgr_add_route(uint32_t dest, uint32_t netmask, uint32_t gateway, const char *iface) {
    if (g_route_count >= NETMGR_MAX_ROUTES || !iface) return -1;
    RouteEntry *r = &g_routes[g_route_count++];
    r->dest = dest;
    r->netmask = netmask;
    r->gateway = gateway;
    strncpy(r->iface, iface, 15);
    r->metric = 10;
    return 0;
}

void netmgr_del_route(uint32_t dest, uint32_t netmask) {
    for (int i = 0; i < g_route_count; i++) {
        if (g_routes[i].dest == dest && g_routes[i].netmask == netmask) {
            g_routes[i] = g_routes[--g_route_count];
            return;
        }
    }
}

void netmgr_publish_status(const char *iface_name) {
    NetInterface *iface = netmgr_get_iface(iface_name);
    if (!iface) return;
    gui_ipc_send_service_event("NetMgr", 1, 10, iface->link_up ? "Connected" : "Disconnected");
}

NetInterface *netmgr_get_iface(const char *name) {
    if (!name) return NULL;
    for (int i = 0; i < g_iface_count; i++) {
        if (strcmp(g_ifaces[i].name, name) == 0) {
            return &g_ifaces[i];
        }
    }
    return NULL;
}

const RouteEntry *netmgr_lookup_route(uint32_t dest_ip) {
    for (int i = 0; i < g_route_count; i++) {
        if ((dest_ip & g_routes[i].netmask) == g_routes[i].dest) {
            return &g_routes[i];
        }
    }
    return g_route_count > 0 ? &g_routes[0] : NULL;
}

void netmgr_on_link_change(const char *iface_name, uint8_t link_up) {
    NetInterface *iface = netmgr_get_iface(iface_name);
    if (iface) {
        iface->link_up = link_up;
        netmgr_publish_status(iface_name);
    }
}

void netmgr_on_packet_received(const char *iface_name, const void *data, uint16_t len) {
    NetInterface *iface = netmgr_get_iface(iface_name);
    if (iface && data && len > 0) {
        iface->rx_packets++;
        iface->rx_bytes += len;
    }
}
