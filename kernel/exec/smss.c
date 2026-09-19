/**
 * @file smss.c
 * @brief Session Manager Subsystem — Session Lifecycle & Ring-0→Ring-3 Transition
 */

#include "smss.h"
#include "kpcr.h"
#include "../sched/sched.h"
#include "../security/session.h"
#include "../gui/compositor.h"
#include "../kstring.h"

/* ---------------------------------------------------------------------------
 * Global Session State
 * --------------------------------------------------------------------------- */
static SESSION_DESC s_sessions[2];
static LOGON_STATE  s_logon_state  = LOGON_IDLE;

/* ---------------------------------------------------------------------------
 * ring3_transition — IRETQ-based Ring-0 to Ring-3 Privilege Drop
 *
 * Constructs the 5-element stack frame that IRETQ expects when switching to
 * a different privilege level (CPL 0 → CPL 3):
 *
 *   [RSP+32] SS    — User stack segment (SEL_USER_DATA | RPL3 = 0x1B)
 *   [RSP+24] RSP   — User stack pointer
 *   [RSP+16] RFLAGS — Interrupts enabled (bit 9 set), reserved bit 1 set
 *   [RSP+ 8] CS    — User code segment (SEL_USER_CODE | RPL3 = 0x23)
 *   [RSP+ 0] RIP   — User entry point
 *
 * After IRETQ the CPU:
 *   1. Atomically loads CS and RIP from the frame
 *   2. Checks CS.DPL — if changing to lower privilege (Ring 3), also loads SS and RSP
 *   3. Loads RFLAGS
 *   4. The CPU privilege level changes from CPL=0 to CPL=3
 *
 * Note: All kernel data is still mapped (global PTE_GLOBAL flag), but Ring-3 code
 * cannot access it due to hardware privilege level enforcement (SMEP/SMAP).
 * --------------------------------------------------------------------------- */
__attribute__((noreturn))
void ring3_transition(uint64_t user_rip, uint64_t user_rsp)
{
    kpcr_set_irql(IRQL_PASSIVE_LEVEL);  /* Lower IRQL before dropping privilege */

    __asm__ volatile (
        /* Set data segment registers to user data selector (0x1B = SEL_USER_DATA | RPL3) */
        "mov $0x1B, %%ax       \n\t"
        "mov %%ax,  %%ds       \n\t"
        "mov %%ax,  %%es       \n\t"
        "mov %%ax,  %%fs       \n\t"
        /* GS must be preserved for KPCR access — do NOT set GS to user selector here */

        /* Build IRETQ stack frame (SS RSP RFLAGS CS RIP) — push in high-to-low order */
        "push $0x1B            \n\t"   /* SS  = User Data Selector (RPL=3) */
        "push %1               \n\t"   /* RSP = User stack top */
        "push $0x202           \n\t"   /* RFLAGS: IF=1, Reserved=1 */
        "push $0x23            \n\t"   /* CS  = User Code Selector (RPL=3) */
        "push %0               \n\t"   /* RIP = User entry point */

        /* SWAPGS restores the user GS (which may be 0 for first user thread) */
        /* For the very first user thread transition, GS is already kernel KPCR */
        /* A real OS would SWAPGS here if ring 3 GS base is different */

        /* Execute the atomic far-return to Ring 3 */
        "iretq                 \n\t"
        :
        : "r"(user_rip), "r"(user_rsp)
        : "rax", "memory"
    );

    /* Unreachable — IRETQ does not return */
    __builtin_unreachable();
}

/* ---------------------------------------------------------------------------
 * Session 0 Thread Functions — simulated as kernel threads (Ring 0)
 * In a real OS these would be Ring-3 user-mode processes.
 * --------------------------------------------------------------------------- */

/** wininit.exe: Orchestrates Session 0 setup */
static void wininit_thread(void) {
    /* wininit monitors services.exe and lsass.exe;
     * if either exits unexpectedly it triggers a BSoD. */
    session_create(PID_WININIT, 0, CAP_SYSTEM_ADMIN, 0);
    while (1) {
        sched_sleep(5000);   /* Monitor subsystems every 5 seconds */
        session_guard_audit();
    }
}

/** services.exe: Service Control Manager */
static void services_scm_thread(void) {
    /* In a real OS: reads HKLM\SYSTEM\CurrentControlSet\Services,
     * builds dependency graph, starts Auto services in dependency order */
    session_create(PID_SERVICES, 0, CAP_SYSTEM_ADMIN | CAP_NETWORK_SOCKET, 0);

    /* Start firewall service */
    session_create(PID_SERVICES + 100, 0, CAP_FIREWALL_CONTROL | CAP_NETWORK_SOCKET, 0);

    while (1) {
        sched_sleep(1000);
    }
}

/** lsass.exe: Local Security Authority Subsystem */
static void lsass_thread(void) {
    /* Owns the SAM database, Kerberos ticket cache, NTLM challenge-response.
     * Creates primary access tokens for authenticated users. */
    session_create(PID_LSASS, 0, CAP_SYSTEM_ADMIN, 0);
    while (1) {
        sched_sleep(2000);
        /* In a real implementation: process authentication requests via ALPC */
    }
}

/* ---------------------------------------------------------------------------
 * session_0_init — Spawn the non-interactive service session
 * --------------------------------------------------------------------------- */
void session_0_init(void) {
    SESSION_DESC *s0 = &s_sessions[0];
    s0->session_id      = SESSION_0_ID;
    s0->is_interactive  = 0;
    s0->logon_state     = LOGON_DESKTOP_READY;  /* Session 0 is always ready */
    strcpy(s0->session_name, "Session-0 (Services)");

    /* Spawn kernel threads representing Session 0 processes */
    sched_create_kthread(wininit_thread,   2);  /* Low priority: wininit monitor */
    sched_create_kthread(services_scm_thread, 1); /* Normal: SCM */
    sched_create_kthread(lsass_thread,     1);  /* Normal: LSASS */
}

/* ---------------------------------------------------------------------------
 * Session 1 Thread Functions
 * --------------------------------------------------------------------------- */

/** csrss.exe: Client/Server Runtime Subsystem (Session 1) */
static void csrss_session1_thread(void) {
    /* CSRSS manages Win32 process creation API (NtCreateUserProcess),
     * console window hosting, and the shutdown sequence. */
    session_create(PID_CSRSS_S1, 1, CAP_USER_DESKTOP, 0);
    while (1) {
        sched_sleep(500);
    }
}

/** winlogon.exe: Logon Manager */
static void winlogon_thread(void) {
    /* Winlogon owns the Winlogon Desktop — the secure desktop where
     * credentials are entered (Ctrl+Alt+Del sequence). It spawns LogonUI
     * and upon successful auth spawns userinit.exe. */
    session_create(PID_WINLOGON, 1, CAP_USER_DESKTOP | CAP_SYSTEM_ADMIN, 0);

    s_logon_state = LOGON_CREDENTIAL_UI;

    /* Simulate user authentication — in a real OS LogonUI collects credentials
     * and passes them to lsass via ALPC RPC. */
    sched_sleep(1000);  /* Wait for credential provider UI */

    s_logon_state = LOGON_VALIDATING;
    sched_sleep(500);   /* LSASS validation time */

    s_logon_state = LOGON_SUCCESS;

    /* After successful logon, spawn the user initialization process */
    /* In a real OS: NtCreateUserProcess("userinit.exe") with the user's token */
    sched_sleep(100);
    s_logon_state = LOGON_DESKTOP_READY;

    while (1) {
        sched_sleep(2000);
        /* Monitor secure desktop and handle SAS (Secure Attention Sequence) */
    }
}

/** dwm.exe: Desktop Window Manager (DirectComposition) */
static void dwm_thread(void) {
    /* DWM uses Direct3D/DirectComposition to composite application windows.
     * Each app renders to private off-screen buffers; DWM merges them with
     * transparency, animations, and blur effects. */
    session_create(PID_DWM, 1, CAP_USER_DESKTOP, 0);

    /* The Xenithra compositor IS the DWM equivalent — already initialized */
    while (1) {
        compositor_tick();
        compositor_render();
        sched_sleep(16);  /* ~60 FPS compositor tick */
    }
}

/** userinit.exe: User Initialization — brief process, spawns explorer then exits */
static void userinit_thread(void) {
    session_create(PID_USERINIT, 1, CAP_USER_DESKTOP | CAP_STORAGE_WRITE, 0);

    /* Apply Group Policies, run logon scripts, configure registry run keys */
    sched_sleep(200);

    /* Hand off to the interactive shell (explorer.exe) */
    /* In the Xenithra OS, the compositor/browser shell IS the explorer equivalent */
    /* The kernel main.c sys_launch_django_kiosk() plays this role */

    /* userinit terminates itself after spawning the shell */
    session_revoke(PID_USERINIT, SESSION_VALID);
    sched_exit();
}

/* ---------------------------------------------------------------------------
 * session_1_init — Spawn the interactive desktop session
 * --------------------------------------------------------------------------- */
void session_1_init(void) {
    SESSION_DESC *s1 = &s_sessions[1];
    s1->session_id      = SESSION_1_ID;
    s1->is_interactive  = 1;
    s1->logon_state     = LOGON_IDLE;
    strcpy(s1->session_name, "Session-1 (Interactive Desktop)");

    /* Session 1 process spawn order matches the exact Windows boot sequence */
    sched_create_kthread(csrss_session1_thread, 0); /* Realtime: CSRSS must be responsive */
    sched_create_kthread(winlogon_thread,       1); /* Normal: Winlogon waits for credentials */
    sched_create_kthread(dwm_thread,            0); /* Realtime: DWM drives frame composition */
    sched_create_kthread(userinit_thread,       1); /* Normal: brief, then exits */
}

/* ---------------------------------------------------------------------------
 * smss_master_init — Top-level Session Manager Entry Point
 *
 * This function is called by the kernel during Phase 1 initialization.
 * It simulates the SMSS.EXE master process lifecycle:
 *   1. BootExecute programs (disk check)
 *   2. Initialize paging file
 *   3. Populate KnownDLLs object directory
 *   4. Fork into Session 0 instance (non-interactive services)
 *   5. Fork into Session 1 instance (interactive desktop)
 * --------------------------------------------------------------------------- */
void smss_master_init(void) {
    /* Step 1: BootExecute — disk integrity check equivalent (autochk.exe) */
    /* In Xenithra: check framebuffer coherency and compositor state */

    /* Step 2: Initialize virtual memory manager paging */
    /* VMM already initialized in main.c before this call */

    /* Step 3: Create KnownDLLs mapping (pre-mapped shared libraries)
     * Xenithra equivalent: pre-initialize compositor, sound, and scheduler objects */
    sched_sleep(10);

    /* Step 4: Initialize the isolated system service session */
    session_0_init();

    /* Step 5: Initialize the interactive user desktop session */
    session_1_init();

    /* The master SMSS kernel thread now monitors the sessions */
    while (1) {
        sched_sleep(10000);  /* Check session health every 10 seconds */
        session_guard_audit();
    }
}

/* ---------------------------------------------------------------------------
 * Credential Validation
 * --------------------------------------------------------------------------- */
int smss_validate_credentials(const char *username, const char *password) {
    (void)username;
    (void)password;
    /* Xenithra OS auto-logs in — production implementation would call lsass */
    s_logon_state = LOGON_SUCCESS;
    return 1;
}

LOGON_STATE smss_get_logon_state(void) {
    return s_logon_state;
}
