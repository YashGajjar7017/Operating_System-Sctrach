/**
 * @file firewall.h
 * @brief Xenithra OS Kernel Private Firewall & Network Security Filter
 */

#ifndef _KERNEL_SECURITY_FIREWALL_H_
#define _KERNEL_SECURITY_FIREWALL_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_FIREWALL_RULES 64
#define PROTO_ALL   0
#define PROTO_TCP   6
#define PROTO_UDP   17
#define PROTO_ICMP  1

typedef enum {
    FIREWALL_ACTION_ALLOW = 0,
    FIREWALL_ACTION_DROP,
    FIREWALL_ACTION_REJECT,
    FIREWALL_ACTION_LOG
} FirewallAction;

typedef enum {
    DIRECTION_INBOUND = 0,
    DIRECTION_OUTBOUND,
    DIRECTION_BOTH
} RuleDirection;

/**
 * @brief Firewall Rule definition
 */
typedef struct {
    uint32_t rule_id;
    char name[32];
    uint8_t enabled;
    RuleDirection direction;
    uint8_t protocol;
    
    uint32_t src_ip;
    uint32_t src_mask;
    uint16_t src_port;
    
    uint32_t dst_ip;
    uint32_t dst_mask;
    uint16_t dst_port;
    
    FirewallAction action;
    uint64_t matched_packets;
} FirewallRule;

/**
 * @brief Live Firewall Statistics
 */
typedef struct {
    uint64_t total_inspected;
    uint64_t total_allowed;
    uint64_t total_blocked;
    uint32_t active_rules_count;
    uint8_t  stealth_mode_enabled; /* Silent drop port-scanners */
    uint8_t  firewall_active;
} FirewallStats;

/* Firewall Lifecycle & Inspection API */
void firewall_init(void);
void firewall_set_active(uint8_t active);
int  firewall_add_rule(const char *name, RuleDirection dir, uint8_t proto, uint32_t dst_ip, uint16_t dst_port, FirewallAction action);
void firewall_delete_rule(uint32_t rule_id);

/* Packet Inspection Engine */
FirewallAction firewall_filter_packet(RuleDirection dir, uint8_t proto, uint32_t src_ip, uint16_t src_port, uint32_t dst_ip, uint16_t dst_port);

/* Stats & Inspection Queries */
FirewallStats firewall_get_stats(void);
const FirewallRule* firewall_get_rule(int index);

#endif /* _KERNEL_SECURITY_FIREWALL_H_ */
