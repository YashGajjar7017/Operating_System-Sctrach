/**
 * @file netmgr.h — Xenithra OS — Network Manager Service
 *
 * User-space-style daemon running as a kernel thread in Session 0.
 * Manages: DHCP client, DNS resolver, routing table, link monitoring,
 * and publishes network events to the Render Engine via GUI IPC.
 */

#ifndef _SERVICE_NETMGR_H_
#define _SERVICE_NETMGR_H_

#include <stdint.h>
#include <stddef.h>

/* ── Interface types ─────────────────────────────────────────────────── */
#define NETMGR_IFACE_ETHERNET  0
#define NETMGR_IFACE_WIFI      1
#define NETMGR_IFACE_LOOPBACK  2
#define NETMGR_IFACE_TUNNEL    3

/* ── DHCP states ─────────────────────────────────────────────────────── */
typedef enum {
    DHCP_STATE_INIT       = 0,
    DHCP_STATE_SELECTING  = 1,   /* Sent DISCOVER, awaiting OFFER */
    DHCP_STATE_REQUESTING = 2,   /* Sent REQUEST, awaiting ACK */
    DHCP_STATE_BOUND      = 3,   /* IP assigned */
    DHCP_STATE_RENEWING   = 4,   /* Renewing lease */
    DHCP_STATE_REBINDING  = 5,   /* Rebinding (server unreachable) */
    DHCP_STATE_FAILED     = 6,
} DhcpState;

/* ── DHCP packet structure (simplified) ─────────────────────────────── */
typedef struct __attribute__((packed)) {
    uint8_t  op;           /* 1=BOOTREQUEST, 2=BOOTREPLY */
    uint8_t  htype;        /* 1 = Ethernet */
    uint8_t  hlen;         /* 6 for Ethernet */
    uint8_t  hops;
    uint32_t xid;          /* Transaction ID */
    uint16_t secs;
    uint16_t flags;        /* 0x8000 = broadcast */
    uint32_t ciaddr;       /* Client IP */
    uint32_t yiaddr;       /* Your (offered) IP */
    uint32_t siaddr;       /* Server IP */
    uint32_t giaddr;       /* Relay agent IP */
    uint8_t  chaddr[16];   /* Client hardware address */
    uint8_t  sname[64];    /* Server host name */
    uint8_t  file[128];    /* Boot filename */
    uint8_t  options[312]; /* Options (starts with magic cookie 0x63825363) */
} DhcpPacket;

/* ── DHCP option codes ───────────────────────────────────────────────── */
#define DHCP_OPT_SUBNET_MASK    1
#define DHCP_OPT_ROUTER         3   /* Default gateway */
#define DHCP_OPT_DNS            6
#define DHCP_OPT_HOSTNAME      12
#define DHCP_OPT_DOMAIN_NAME   15
#define DHCP_OPT_MTU           26
#define DHCP_OPT_BROADCAST     28
#define DHCP_OPT_LEASE_TIME    51
#define DHCP_OPT_MSG_TYPE      53   /* 1=DISCOVER,2=OFFER,3=REQUEST,4=DECLINE,5=ACK,6=NAK,7=RELEASE */
#define DHCP_OPT_SERVER_ID     54
#define DHCP_OPT_RENEW_TIME    58
#define DHCP_OPT_REBIND_TIME   59
#define DHCP_OPT_END          255

/* ── Network interface configuration ─────────────────────────────────── */
typedef struct {
    char     name[16];         /* e.g. "eth0" */
    uint8_t  type;             /* NETMGR_IFACE_* */
    uint8_t  mac[6];
    uint32_t ipv4;             /* Network byte order */
    uint32_t netmask;
    uint32_t gateway;
    uint32_t dns[4];           /* Up to 4 DNS servers */
    uint8_t  dns_count;
    uint32_t lease_time;       /* DHCP lease time in seconds */
    uint32_t lease_obtained;   /* Unix timestamp */
    DhcpState dhcp_state;
    uint8_t  link_up;
    uint32_t rx_bytes;
    uint32_t tx_bytes;
    uint32_t rx_packets;
    uint32_t tx_packets;
    uint32_t rx_errors;
    uint32_t tx_errors;
} NetInterface;

/* ── DNS resolver ─────────────────────────────────────────────────────── */
#define NETMGR_MAX_DNS_CACHE 128

typedef struct {
    char     hostname[256];
    uint32_t ipv4;             /* Resolved IP */
    uint32_t ttl;              /* Time-to-live in seconds */
    uint32_t resolved_at;      /* Timestamp */
    uint8_t  valid;
} DnsCacheEntry;

/* ── Routing table ───────────────────────────────────────────────────── */
#define NETMGR_MAX_ROUTES 32

typedef struct {
    uint32_t dest;       /* Destination network */
    uint32_t netmask;
    uint32_t gateway;    /* 0 = on-link */
    char     iface[16];  /* Interface name */
    uint8_t  metric;
} RouteEntry;

/* ── Public API ──────────────────────────────────────────────────────── */

/**
 * @brief Network Manager main thread entry point.
 *        Called from smss_master_init() as a kernel thread.
 *        Never returns (runs as daemon).
 */
void netmgr_thread(void);

/** Trigger DHCP discovery on an interface. */
int  netmgr_dhcp_discover(const char *iface_name);

/** Release DHCP lease on an interface. */
int  netmgr_dhcp_release(const char *iface_name);

/** Resolve a hostname to IPv4. Returns 0 on success. */
int  netmgr_dns_resolve(const char *hostname, uint32_t *out_ip);

/** Add a static route. Returns 0 on success. */
int  netmgr_add_route(uint32_t dest, uint32_t netmask, uint32_t gateway, const char *iface);

/** Remove a route. */
void netmgr_del_route(uint32_t dest, uint32_t netmask);

/** Publish current interface status via GUI IPC. */
void netmgr_publish_status(const char *iface_name);

/** Get interface config by name. Returns NULL if not found. */
NetInterface *netmgr_get_iface(const char *name);

/** Get routing table entry for destination IP. */
const RouteEntry *netmgr_lookup_route(uint32_t dest_ip);

/** Handle link state change from NIC driver. */
void netmgr_on_link_change(const char *iface_name, uint8_t link_up);

/** Handle received packet (called from NIC driver IRQ). */
void netmgr_on_packet_received(const char *iface_name, const void *data, uint16_t len);

#endif /* _SERVICE_NETMGR_H_ */
