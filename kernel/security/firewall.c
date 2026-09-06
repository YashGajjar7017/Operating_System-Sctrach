/**
 * @file firewall.c
 * @brief Xenithra OS Kernel Private Firewall Implementation
 */

#include "firewall.h"

static FirewallRule g_rules[MAX_FIREWALL_RULES] = {0};
static uint32_t g_next_rule_id = 1;
static FirewallStats g_stats = {0};

static void str_copy(char *dest, const char *src, size_t max) {
    size_t i = 0;
    while (i + 1 < max && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void firewall_init(void) {
    for (int i = 0; i < MAX_FIREWALL_RULES; i++) {
        g_rules[i].enabled = 0;
        g_rules[i].rule_id = 0;
    }

    g_stats.total_inspected = 0;
    g_stats.total_allowed = 0;
    g_stats.total_blocked = 0;
    g_stats.active_rules_count = 0;
    g_stats.stealth_mode_enabled = 1; /* Drop suspicious probe packets silently */
    g_stats.firewall_active = 1;

    /* Rule 1: Allow Localhost Loopback */
    firewall_add_rule("Allow Loopback (127.0.0.1)", DIRECTION_BOTH, PROTO_ALL, 0x7F000001, 0, FIREWALL_ACTION_ALLOW);

    /* Rule 2: Allow Outbound DNS (Port 53) */
    firewall_add_rule("Allow Outbound DNS (UDP 53)", DIRECTION_OUTBOUND, PROTO_UDP, 0, 53, FIREWALL_ACTION_ALLOW);

    /* Rule 3: Allow Outbound HTTPS / Web (Ports 80, 443) */
    firewall_add_rule("Allow Outbound Web (TCP 443)", DIRECTION_OUTBOUND, PROTO_TCP, 0, 443, FIREWALL_ACTION_ALLOW);
    firewall_add_rule("Allow Outbound HTTP (TCP 80)", DIRECTION_OUTBOUND, PROTO_TCP, 0, 80, FIREWALL_ACTION_ALLOW);

    /* Rule 4: Block Malicious Inbound SMB/NetBIOS (Ports 445, 137-139) */
    firewall_add_rule("Block SMB Exploits (TCP 445)", DIRECTION_INBOUND, PROTO_TCP, 0, 445, FIREWALL_ACTION_DROP);
    firewall_add_rule("Block NetBIOS Probe (UDP 137)", DIRECTION_INBOUND, PROTO_UDP, 0, 137, FIREWALL_ACTION_DROP);

    /* Rule 5: Block Telnet & Unencrypted Legacy Ports */
    firewall_add_rule("Block Insecure Telnet (TCP 23)", DIRECTION_INBOUND, PROTO_TCP, 0, 23, FIREWALL_ACTION_DROP);
}

void firewall_set_active(uint8_t active) {
    g_stats.firewall_active = active;
}

int firewall_add_rule(const char *name, RuleDirection dir, uint8_t proto, uint32_t dst_ip, uint16_t dst_port, FirewallAction action) {
    for (int i = 0; i < MAX_FIREWALL_RULES; i++) {
        if (!g_rules[i].enabled) {
            g_rules[i].rule_id = g_next_rule_id++;
            str_copy(g_rules[i].name, name, sizeof(g_rules[i].name));
            g_rules[i].enabled = 1;
            g_rules[i].direction = dir;
            g_rules[i].protocol = proto;
            g_rules[i].dst_ip = dst_ip;
            g_rules[i].dst_port = dst_port;
            g_rules[i].action = action;
            g_rules[i].matched_packets = 0;
            g_stats.active_rules_count++;
            return g_rules[i].rule_id;
        }
    }
    return -1;
}

void firewall_delete_rule(uint32_t rule_id) {
    for (int i = 0; i < MAX_FIREWALL_RULES; i++) {
        if (g_rules[i].enabled && g_rules[i].rule_id == rule_id) {
            g_rules[i].enabled = 0;
            g_rules[i].rule_id = 0;
            if (g_stats.active_rules_count > 0) {
                g_stats.active_rules_count--;
            }
            return;
        }
    }
}

FirewallAction firewall_filter_packet(RuleDirection dir, uint8_t proto, uint32_t src_ip, uint16_t src_port, uint32_t dst_ip, uint16_t dst_port) {
    (void)src_ip;
    (void)src_port;

    if (!g_stats.firewall_active) {
        return FIREWALL_ACTION_ALLOW;
    }

    g_stats.total_inspected++;

    /* Match against active rules */
    for (int i = 0; i < MAX_FIREWALL_RULES; i++) {
        if (!g_rules[i].enabled) continue;

        FirewallRule *r = &g_rules[i];

        /* Match Direction */
        if (r->direction != DIRECTION_BOTH && r->direction != dir) continue;

        /* Match Protocol */
        if (r->protocol != PROTO_ALL && r->protocol != proto) continue;

        /* Match Destination Port */
        if (r->dst_port != 0 && r->dst_port != dst_port) continue;

        /* Match Destination IP */
        if (r->dst_ip != 0 && r->dst_ip != dst_ip) continue;

        /* Rule Matched */
        r->matched_packets++;
        if (r->action == FIREWALL_ACTION_ALLOW) {
            g_stats.total_allowed++;
        } else {
            g_stats.total_blocked++;
        }
        return r->action;
    }

    /* Default Policy: Allow outbound, Drop unsolicited inbound */
    if (dir == DIRECTION_OUTBOUND) {
        g_stats.total_allowed++;
        return FIREWALL_ACTION_ALLOW;
    } else {
        g_stats.total_blocked++;
        return FIREWALL_ACTION_DROP;
    }
}

FirewallStats firewall_get_stats(void) {
    return g_stats;
}

const FirewallRule* firewall_get_rule(int index) {
    if (index >= 0 && index < MAX_FIREWALL_RULES) {
        return &g_rules[index];
    }
    return NULL;
}
