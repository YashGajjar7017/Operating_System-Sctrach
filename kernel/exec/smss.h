/**
 * @file smss.h
 * @brief Session Manager Subsystem (SMSS) & Ring-0 → Ring-3 Transition for Xenithra OS
 *
 * The Session Manager is the first user-mode process created by the kernel.
 * In Xenithra OS, it is modeled directly after Windows smss.exe behavior:
 *
 *   1. Master smss role:
 *      - Processes BootExecute programs (disk check, chkdsk analog)
 *      - Initializes known subsystems (Win32 subsystem equivalent = compositor)
 *      - Creates Session 0 (non-interactive service session)
 *      - Creates Session 1 (interactive user desktop session)
 *
 *   2. Session 0 (Isolation Boundary):
 *      - wininit equivalent: starts services.exe (SCM) and lsass.exe equivalents
 *      - Runs background kernel threads: network, storage, security daemons
 *      - Session 0 processes cannot render windows to the interactive desktop
 *
 *   3. Session 1 (Interactive Desktop):
 *      - Loads the GUI compositor (win32k.sys equivalent)
 *      - Starts csrss equivalent: compositor process bridge
 *      - Starts winlogon equivalent: credential validation, desktop creation
 *      - Starts dwm equivalent: hardware-accelerated window compositor
 *      - After logon: userinit runs policies → spawns explorer (desktop shell)
 *
 *   4. Ring 0 → Ring 3 Transition:
 *      - The kernel builds an IRETQ stack frame to switch from CPL=0 to CPL=3
 *      - The first Ring-3 thread is the "master smss" thread
 */

#ifndef _KERNEL_EXEC_SMSS_H_
#define _KERNEL_EXEC_SMSS_H_

#include <stdint.h>

/* ---------------------------------------------------------------------------
 * Session ID Assignments (mirrors Windows session architecture)
 * --------------------------------------------------------------------------- */
#define SESSION_0_ID    0   /* Non-interactive system services (isolation boundary) */
#define SESSION_1_ID    1   /* First interactive user session */

/* Process/Thread ID constants for well-known system processes */
#define PID_SMSS        4    /* Session Manager (first user-mode process) */
#define PID_CSRSS_S0    8    /* Client/Server Runtime Session 0 */
#define PID_WININIT     12   /* Windows Initialization Process (Session 0) */
#define PID_SERVICES    16   /* Service Control Manager (services.exe) */
#define PID_LSASS       20   /* Local Security Authority Subsystem */
#define PID_CSRSS_S1    24   /* Client/Server Runtime Session 1 (interactive) */
#define PID_WINLOGON    28   /* Windows Logon Manager */
#define PID_DWM         32   /* Desktop Window Manager */
#define PID_LOGONUI     36   /* LogonUI (credential input) */
#define PID_USERINIT    40   /* User Initialization (group policy, shell launch) */
#define PID_EXPLORER    44   /* Shell: explorer.exe (desktop, taskbar, start menu) */

/* ---------------------------------------------------------------------------
 * Logon State Machine
 * --------------------------------------------------------------------------- */
typedef enum _LOGON_STATE {
    LOGON_IDLE          = 0,   /* Waiting for user interaction */
    LOGON_CREDENTIAL_UI = 1,   /* LogonUI active, awaiting credentials */
    LOGON_VALIDATING    = 2,   /* LSASS validating credentials */
    LOGON_FAILED        = 3,   /* Authentication failure */
    LOGON_SUCCESS       = 4,   /* Authenticated — building desktop */
    LOGON_DESKTOP_READY = 5,   /* Explorer shell running */
} LOGON_STATE;

/* ---------------------------------------------------------------------------
 * Session Descriptor
 * --------------------------------------------------------------------------- */
typedef struct _SESSION_DESC {
    uint32_t    session_id;         /* SESSION_0_ID or SESSION_1_ID */
    uint32_t    is_interactive;     /* 1 if this is the user-facing desktop session */
    LOGON_STATE logon_state;
    uint32_t    desktop_thread_count;
    char        session_name[32];
} SESSION_DESC;

/* ---------------------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------------------- */

/**
 * @brief Master SMSS initialization entry point.
 *        Called by the kernel (Phase 1, after Phase 1 exec init) to create
 *        both sessions and launch all subsystem threads.
 *        This function runs in Ring-0 but orchestrates the Ring-3 transition.
 */
void smss_master_init(void);

/**
 * @brief Initialize Session 0 — non-interactive system service session.
 *        Creates kernel threads for: wininit, services (SCM), lsass (security).
 *        These threads run at IRQL_PASSIVE with kernel-mode stacks.
 */
void session_0_init(void);

/**
 * @brief Initialize Session 1 — interactive user desktop session.
 *        Loads win32k subsystem, creates compositor, winlogon, DWM, explorer threads.
 */
void session_1_init(void);

/**
 * @brief Performs the final Ring-0 to Ring-3 privilege transition.
 *        Builds the IRETQ stack frame and atomically switches the CPU to CPL=3.
 *        The target function runs at Ring-3 privilege with a user-mode stack.
 *
 * @param user_rip   Entry point virtual address in user space.
 * @param user_rsp   Stack pointer virtual address in user space.
 *                   (Must be a valid mapped user-mode address)
 *
 * @note  This function does NOT return — the CPU switches execution context.
 */
__attribute__((noreturn))
void ring3_transition(uint64_t user_rip, uint64_t user_rsp);

/**
 * @brief Returns the current logon state for Session 1.
 */
LOGON_STATE smss_get_logon_state(void);

/**
 * @brief Simulates credential validation by LSASS and advances logon state machine.
 *        In a real OS: validates against SAM/Active Directory Kerberos.
 * @return 1 on success (creates access token, proceeds to desktop), 0 on failure.
 */
int smss_validate_credentials(const char *username, const char *password);

#endif /* _KERNEL_EXEC_SMSS_H_ */
