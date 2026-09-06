/**
 * @file session.c
 * @brief Xenithra OS Session Hijacking Prevention & Security Engine Implementation
 */

#include "session.h"

static XenithraSession g_sessions[MAX_SESSIONS] = {0};
static uint32_t g_next_session_id = 1000;
static uint32_t g_total_blocked_hijacks = 0;
static uint64_t g_system_ticks = 100;

/* Hardware Entropy & Pseudo-Random Generation fallback */
static uint64_t get_hardware_entropy(void) {
    uint64_t rand_val = 0;
    unsigned char ok = 0;

    /* Attempt hardware RDRAND instruction (x86_64) */
    __asm__ volatile (
        "rdrand %0\n\t"
        "setc %1"
        : "=r"(rand_val), "=qm"(ok)
        :
        : "cc"
    );

    if (ok) {
        return rand_val;
    }

    /* Fallback: Read Time-Stamp Counter (RDTSC) + Jitter Mixing */
    uint32_t low, high;
    __asm__ volatile ("rdtsc" : "=a"(low), "=d"(high));
    uint64_t tsc = ((uint64_t)high << 32) | low;

    /* 64-bit XorShift mixer */
    tsc ^= tsc >> 12;
    tsc ^= tsc << 25;
    tsc ^= tsc >> 27;
    return tsc * 0x2545F4914F6CDD1DULL;
}

SecureToken128 security_generate_token(void) {
    SecureToken128 tok;
    tok.high = get_hardware_entropy() ^ 0xA5A5A5A5A5A5A5A5ULL;
    tok.low  = get_hardware_entropy() ^ 0x5A5A5A5A5A5A5A5AULL;
    return tok;
}

/* Enable Hardware SMEP and SMAP in CR4 to prevent user-space hijack exploits */
void security_enable_smep_smap(void) {
    uint64_t cr4 = 0;
    __asm__ volatile ("mov %%cr4, %0" : "=r"(cr4));

    /* Bit 20: SMEP (Supervisor Mode Execution Prevention) */
    /* Bit 21: SMAP (Supervisor Mode Access Prevention) */
    /* Enable if supported by CPU, ignore if VM rejects */
    cr4 |= (1 << 20); /* SMEP */
    
    /* Write back CR4 */
    __asm__ volatile ("mov %0, %%cr4" : : "r"(cr4) : "memory");
}

void security_init(void) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        g_sessions[i].status = SESSION_EXPIRED;
        g_sessions[i].session_id = 0;
    }
    g_total_blocked_hijacks = 0;

    /* Create initial Kernel Master Session (PID 0) */
    session_create(0, 0, CAP_SYSTEM_ADMIN | CAP_NETWORK_SOCKET | CAP_FIREWALL_CONTROL | CAP_STORAGE_WRITE | CAP_USER_DESKTOP, 0x7F000001);

    /* Create initial Secure User Session (PID 1) */
    session_create(1, 3, CAP_USER_DESKTOP | CAP_NETWORK_SOCKET, 0x7F000001);
}

XenithraSession* session_create(uint32_t pid, uint32_t ring_level, uint32_t capabilities, uint32_t client_ip) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].status == SESSION_EXPIRED || g_sessions[i].session_id == 0) {
            g_sessions[i].session_id = g_next_session_id++;
            g_sessions[i].owner_pid = pid;
            g_sessions[i].ring_level = ring_level;
            g_sessions[i].capabilities = capabilities;
            g_sessions[i].token = security_generate_token();
            g_sessions[i].creation_tick = ++g_system_ticks;
            g_sessions[i].last_activity_tick = g_system_ticks;
            g_sessions[i].client_ip = client_ip;
            g_sessions[i].status = SESSION_VALID;
            g_sessions[i].hijack_attempts = 0;
            return &g_sessions[i];
        }
    }
    return NULL;
}

int session_validate(uint32_t session_id, const SecureToken128 *token, uint32_t caller_pid, uint32_t required_cap) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].session_id == session_id) {
            XenithraSession *s = &g_sessions[i];

            if (s->status != SESSION_VALID) {
                return 0; /* Invalid session */
            }

            /* 1. Verify 128-bit Secret Token */
            if (!token || s->token.high != token->high || s->token.low != token->low) {
                /* Token mismatch: Potential Brute-force / Interception Attempt */
                s->hijack_attempts++;
                g_total_blocked_hijacks++;
                if (s->hijack_attempts >= 3) {
                    s->status = SESSION_HIJACK_ATTEMPT_DETECTED;
                }
                return 0;
            }

            /* 2. Strict Process ID Binding (Prevents Cross-Process Session Hijacking) */
            if (s->owner_pid != caller_pid && s->owner_pid != 0) {
                /* Another rogue PID is attempting to use this session! */
                s->hijack_attempts++;
                g_total_blocked_hijacks++;
                s->status = SESSION_HIJACK_ATTEMPT_DETECTED;
                return 0;
            }

            /* 3. Capability Permission Check */
            if ((s->capabilities & required_cap) != required_cap) {
                return 0; /* Insufficient capability */
            }

            s->last_activity_tick = ++g_system_ticks;
            return 1; /* Authorized */
        }
    }
    return 0;
}

void session_revoke(uint32_t session_id, SessionStatus reason) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].session_id == session_id) {
            g_sessions[i].status = reason;
            g_sessions[i].token.high = 0;
            g_sessions[i].token.low = 0;
            return;
        }
    }
}

int session_guard_audit(void) {
    int anomalies = 0;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].session_id != 0 && g_sessions[i].status == SESSION_HIJACK_ATTEMPT_DETECTED) {
            anomalies++;
        }
    }
    return anomalies;
}

uint32_t security_get_active_session_count(void) {
    uint32_t count = 0;
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (g_sessions[i].session_id != 0 && g_sessions[i].status == SESSION_VALID) {
            count++;
        }
    }
    return count;
}

uint32_t security_get_blocked_hijacks_count(void) {
    return g_total_blocked_hijacks;
}

const XenithraSession* security_get_session(int index) {
    if (index >= 0 && index < MAX_SESSIONS) {
        return &g_sessions[index];
    }
    return NULL;
}
