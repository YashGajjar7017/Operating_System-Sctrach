/**
 * @file session.h
 * @brief Xenithra OS Kernel Session Hijacking Prevention & Capability Security Subsystem
 */

#ifndef _KERNEL_SECURITY_SESSION_H_
#define _KERNEL_SECURITY_SESSION_H_

#include <stdint.h>
#include <stddef.h>

#define MAX_SESSIONS 32
#define TOKEN_BYTES 16 /* 128-bit cryptographically secure session token */

/* Capability & Privilege Flags */
#define CAP_SYSTEM_ADMIN      (1 << 0)
#define CAP_NETWORK_SOCKET    (1 << 1)
#define CAP_FIREWALL_CONTROL  (1 << 2)
#define CAP_STORAGE_WRITE     (1 << 3)
#define CAP_USER_DESKTOP      (1 << 4)

/* Security State / Anomaly Detection Flags */
typedef enum {
    SESSION_VALID = 0,
    SESSION_EXPIRED,
    SESSION_HIJACK_ATTEMPT_DETECTED,
    SESSION_REVOKED_ANOMALY,
    SESSION_LOCKED
} SessionStatus;

/**
 * @brief 128-bit Secure Token Structure
 */
typedef struct {
    uint64_t high;
    uint64_t low;
} SecureToken128;

/**
 * @brief Kernel Protected Session Entity
 */
typedef struct {
    uint32_t session_id;
    uint32_t owner_pid;
    uint32_t ring_level;         /* Ring 0 (Kernel) vs Ring 3 (User) */
    uint32_t capabilities;       /* Bitmask of allowed operations */
    
    SecureToken128 token;        /* Randomized 128-bit Secret Token */
    uint64_t creation_tick;      /* Timestamp of session creation */
    uint64_t last_activity_tick; /* Last verified activity */
    uint32_t client_ip;          /* Bound IP (anti-spoofing) */
    
    SessionStatus status;
    uint32_t hijack_attempts;    /* Counter for blocked unauthorized access */
} XenithraSession;

/* Security Subsystem API */
void security_init(void);
void security_enable_smep_smap(void);

/* Session Management & Anti-Hijack Guard */
XenithraSession* session_create(uint32_t pid, uint32_t ring_level, uint32_t capabilities, uint32_t client_ip);
int session_validate(uint32_t session_id, const SecureToken128 *token, uint32_t caller_pid, uint32_t required_cap);
void session_revoke(uint32_t session_id, SessionStatus reason);
int session_guard_audit(void);

/* Hardware Entropy & Token Generation */
SecureToken128 security_generate_token(void);

/* Diagnostics & App Inspection API */
uint32_t security_get_active_session_count(void);
uint32_t security_get_blocked_hijacks_count(void);
const XenithraSession* security_get_session(int index);

#endif /* _KERNEL_SECURITY_SESSION_H_ */
