/**
 * @file main.c — Xenithra OS v3.0 — Kernel Entry Point
 *
 * Architecture v3.0 Changes:
 *   ✅ KEEP: All hardware init (GDT/IDT/APIC/VMM/Heap/Syscall)
 *   ✅ KEEP: Security (SMEP/SMAP, session guard, firewall)
 *   ✅ KEEP: Scheduler (RTOS 6-level), SMP, PS/2 drivers
 *   ✅ KEEP: GUI IPC Named Pipe server (gui_ipc.c)
 *   ✅ NEW:  e1000 / RTL8139 network drivers
 *   ✅ NEW:  xHCI USB 3.0 host controller
 *   ✅ NEW:  ACPI power management
 *   ✅ NEW:  VBE boot splash (pre-Electron)
 *   ✅ NEW:  netmgr / drvmgr / panelmgr service threads
 *   ❌ REMOVED: compositor.c  (C framebuffer drawing)
 *   ❌ REMOVED: v8_engine.c   (fake V8 stub)
 *   ❌ REMOVED: dom_engine.c  (C DOM)
 *   ❌ REMOVED: anim.c        (C animations)
 *   ❌ REMOVED: py_runtime.c  (inline Python stub)
 *   ❌ REMOVED: react_bundle.h
 *   ❌ REMOVED: sys_launch_django_kiosk()
 *
 * GUI Architecture (v3.0):
 *   kernel → gui_ipc Named Pipe → Electron (render_engine/) → React/TSX
 *   C draws NOTHING. All rendering is in the V8/Chromium Render Engine.
 *
 * Boot Phases:
 *   Phase 0 (IRQs OFF, BSP, IRQL=HIGH):
 *     hardware_tables_init → kpcr_init → vmm_init → kheap_init
 *     → apic_init → syscall_init_msrs
 *   Phase 1 (IRQs ON, SMP, IRQL=PASSIVE):
 *     apic_calibrate_timer → apic_arm_timer → apic_start_all_aps
 *     → sound_init → sched_init → smss_master_init
 *     → ps2_init → security_init → firewall_init
 *   Phase 1B (New Drivers & Services):
 *     acpi_init → drvmgr_init → e1000_init/rtl8139_init → xhci_init
 *     → netmgr thread → drvmgr thread → panelmgr thread
 *   Phase 2 (GUI IPC):
 *     vbe_draw_boot_splash → gui_ipc_init → kshell_init
 *   Phase 3 (Event Pump):
 *     while(1): poll PS/2 → gui_ipc → sched_tick → security audit
 */

#include <stdint.h>
#include <stddef.h>
#include "../shared/bootinfo.h"

/* ── Phase 0: Hardware tables, VMM, KPCR ─────────────────────────────── */
#include "arch/x86_64/gdt_idt.h"
#include "arch/x86_64/apic.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "exec/kpcr.h"
#include "exec/smss.h"

/* ── Phase 1: Core Kernel Subsystems ─────────────────────────────────── */
#include "security/session.h"
#include "security/firewall.h"
#include "drivers/ps2.h"
#include "drivers/sound.h"
#include "sched/sched.h"

/* ── Phase 1B: New Drivers (v3.0) ───────────────────────────────────── */
#include "drivers/acpi/acpi.h"
#include "drivers/net/e1000.h"
#include "drivers/net/rtl8139.h"
#include "drivers/usb/xhci.h"
#include "drivers/gpu/vbe.h"

/* ── Phase 1B: New Services (v3.0) ──────────────────────────────────── */
#include "../services/drvmgr/drvmgr.h"
#include "../services/netmgr/netmgr.h"

/* ── Phase 2: GUI IPC (Named Pipe → Electron Render Engine) ──────────── */
#include "gui/gui_ipc.h"

/* ── Kernel Debug Shell ───────────────────────────────────────────────── */
#include "exec/kshell.h"

/* ── Kernel-level syscall providers (IPC-backed, NO draw calls) ──────── */
#include "apps/browser_app.h"
#include "apps/explorer_app.h"
#include "apps/taskmgr_app.h"
#include "apps/firewall_app.h"
#include "apps/terminal_app.h"
#include "apps/vlc_app.h"
#include "apps/installer_app.h"
#include "apps/diskclone_app.h"

/*
 * REMOVED in v3.0 (no longer included):
 *   #include "gui/compositor.h"   ← C framebuffer compositor
 *   #include "gui/v8_engine.h"    ← Fake V8 stub
 *   #include "gui/dom_engine.h"   ← C DOM engine
 *   #include "gui/anim.h"         ← C animations
 *   #include "gui/react_bundle.h" ← Inline React bundle
 *   #include "python/py_runtime.h"← Python stub
 */

#define KERNEL_HEAP_SIZE  (64ULL * 1024 * 1024)  /* 64 MB */

static XenithraBootInfo g_kernel_boot_info;
extern void syscall_init_msrs(void);

/* ─────────────────────────────────────────────────────────────────────────
 * kmain() — Kernel Entry Point
 * Called from kernel/arch/x86_64/entry.asm after Long Mode setup.
 * RDI/RCX = pointer to XenithraBootInfo (UEFI → kernel ABI).
 * ───────────────────────────────────────────────────────────────────────── */
void kmain(XenithraBootInfo *boot_info)
{
    /* ==================================================================
     * PHASE 0: Single-Core, Interrupts Masked (IRQL = HIGH_LEVEL)
     * ================================================================== */

    /* 1. Copy boot info from UEFI handoff */
    if (boot_info && boot_info->magic == XENITHRA_BOOT_MAGIC) {
        g_kernel_boot_info = *boot_info;
    }

    /* 2. GDT / IDT / TSS — all 7 GDT selectors, 256 IDT gates, IST stacks */
    hardware_tables_init();

    /* 3. Per-CPU KPCR for BSP (CPU 0) — sets IA32_GS_BASE MSR */
    extern GDT_ENTRY g_gdt[];
    extern IDT_ENTRY g_idt[];
    extern TASK_STATE_SEGMENT g_tss;
    kpcr_init(0, 0, &g_tss, g_idt, g_gdt);

    /* 4. Physical Frame Allocator (PFN Database) + VMM */
    vmm_init(&g_kernel_boot_info);

    /* 5. Kernel Non-Paged Pool (64 MB boundary-tag heap) */
    uint64_t heap_va = vmm_alloc_pages(KERNEL_HEAP_SIZE / 4096, PTE_KERNEL_RW);
    if (heap_va) {
        kheap_init(heap_va, KERNEL_HEAP_SIZE);
    }

    /* 6. Local APIC — parse ACPI RSDP/MADT, map MMIO, enable SIVR */
    apic_init(g_kernel_boot_info.acpi_rsdp);

    /* 7. SYSCALL/SYSRET MSRs — IA32_LSTAR / IA32_STAR / IA32_FMASK */
    syscall_init_msrs();

    /* ==================================================================
     * PHASE 1: Interrupts Enabled, SMP (IRQL = PASSIVE_LEVEL)
     * ================================================================== */

    /* 8. Enable interrupts */
    kpcr_set_irql(IRQL_PASSIVE_LEVEL);
    __asm__ volatile ("sti");

    /* 9. Calibrate APIC timer against PIT (10ms measurement) */
    apic_calibrate_timer();

    /* 10. Arm APIC periodic tick (1ms interval → IRQ 0x20 → sched_tick) */
    apic_arm_timer(1000);

    /* 11. Wake Application Processors (INIT-SIPI-SIPI → Long Mode) */
    if (g_cpu_count > 1) {
        apic_start_all_aps(0x8000);
    }

    /* ==================================================================
     * PHASE 1B: CORE SUBSYSTEM + NEW DRIVERS + SERVICES (v3.0)
     * ================================================================== */

    /* 12. Sound driver + startup chime */
    sound_init();
    play_system_startup_chime();

    /* 13. RTOS Priority Scheduler (6 levels + DPC queue) */
    sched_init();

    /* 14. SMSS — Session 0 / Session 1 lifecycle threads */
    sched_create_kthread(smss_master_init, RTOS_PRIO_HIGH);

    /* 15. PS/2 Mouse + Keyboard drivers */
    uint32_t screen_w = g_kernel_boot_info.framebuffer.width  ? g_kernel_boot_info.framebuffer.width  : 1280;
    uint32_t screen_h = g_kernel_boot_info.framebuffer.height ? g_kernel_boot_info.framebuffer.height : 720;
    ps2_init(screen_w, screen_h);

    /* 16. Security subsystem — Anti-Session-Hijack Guard + SMEP/SMAP */
    security_init();
    security_enable_smep_smap();

    /* 17. Kernel Private Firewall (stateful packet filter) */
    firewall_init();

    /* 18. [NEW v3.0] ACPI power management subsystem */
    acpi_init(g_kernel_boot_info.acpi_rsdp);
    acpi_enable();

    /* 19. [NEW v3.0] VBE framebuffer — boot splash while Electron loads */
    if (g_kernel_boot_info.framebuffer.base_address) {
        vbe_init(
            g_kernel_boot_info.framebuffer.base_address,
            screen_w, screen_h,
            g_kernel_boot_info.framebuffer.pixels_per_scanline * 4,
            32
        );
        vbe_draw_boot_splash("Initializing Xenithra OS v3.0...", 10);
    }

    /* 20. [NEW v3.0] PCI Driver Manager — enumerate bus, load drivers */
    vbe_draw_boot_splash("Loading drivers...", 25);
    int dev_count = drvmgr_init();
    gui_ipc_send_notification("Driver Manager", "PCI enumeration complete.", "drvmgr");

    /* 21. [NEW v3.0] Network drivers (e1000 preferred, RTL8139 fallback) */
    vbe_draw_boot_splash("Initializing network...", 40);
    if (e1000_init() != 0) {
        rtl8139_init();   /* Try RTL8139 if e1000 not found */
    }

    /* 22. [NEW v3.0] xHCI USB 3.0 Host Controller */
    vbe_draw_boot_splash("Starting USB subsystem...", 55);
    xhci_init();
    xhci_enumerate_ports();

    /* 23. [NEW v3.0] Service daemon threads (run concurrently) */
    vbe_draw_boot_splash("Starting services...", 70);
    sched_create_kthread(netmgr_thread,  RTOS_PRIO_NORMAL);   /* Network Manager */
    sched_create_kthread(drvmgr_thread,  RTOS_PRIO_NORMAL);   /* Driver Manager daemon */

    /* ==================================================================
     * PHASE 2: GUI IPC — Kernel Named Pipe → Electron Render Engine
     *
     * NOTE: C draws NOTHING beyond this point.
     * All GUI rendering is handled by render_engine/ (Electron + React).
     * The kernel only publishes JSON events over the Named Pipe.
     * ================================================================== */

    /* 24. Open kernel-side Named Pipe server: \\.\pipe\XenithraGUI */
    vbe_draw_boot_splash("Starting render engine IPC...", 85);
    gui_ipc_init();

    /* 25. Register syscall-level app handlers (IPC-backed, no draw code) */
    browser_app_init();
    explorer_app_init();
    taskmgr_app_init();
    firewall_app_init();
    terminal_app_init();
    vlc_app_init();
    installer_app_init();
    diskclone_app_init();

    /* 26. Spawn GDB-compatible kernel debug shell (REALTIME priority) */
    sched_create_kthread(kshell_init, RTOS_PRIO_REALTIME);

    /* 27. Boot splash complete — Electron will take over rendering */
    vbe_draw_boot_splash("Waiting for Render Engine...", 100);

    /* Publish initial service telemetry to the Render Engine */
    gui_ipc_send_service_event("SysMain",  2,  18, "Running");
    gui_ipc_send_service_event("DWM Proxy", 1, 12, "Running");
    gui_ipc_send_service_event("MMCSS",    0,   4, "Running");
    gui_ipc_send_service_event("AudioSrv", 1,   8, "Running");
    gui_ipc_send_service_event("WMI",      0,   6, "Running");
    gui_ipc_send_service_event("NetMgr",   1,  10, "Running");
    gui_ipc_send_service_event("DrvMgr",   0,   6, "Running");
    gui_ipc_send_notification("Xenithra OS", "System ready. Render engine connecting...", "system");

    /* ==================================================================
     * PHASE 3: HIGH-PERFORMANCE EVENT PUMP
     *
     * Polls hardware I/O and routes events to:
     *   a) GUI IPC pipe → Electron Render Engine (all GUI events)
     *   b) Security audit / scheduler tick
     *
     * C draws NOTHING here. No compositor_render(). No v8_engine_tick().
     * ================================================================== */

    PS2MouseState mouse_state;
    PS2KeyEvent   key_event;
    uint64_t      loop_counter = 0;

    while (1) {
        loop_counter++;

        /* 3.1 PS/2 Mouse → GUI IPC → Render Engine */
        if (ps2_poll_mouse(&mouse_state)) {
            gui_ipc_send_mouse_event(
                mouse_state.x,
                mouse_state.y,
                mouse_state.left_button,
                mouse_state.right_button,
                mouse_state.middle_button
            );
        }

        /* 3.2 PS/2 Keyboard → GUI IPC → Render Engine */
        if (ps2_poll_keyboard(&key_event)) {
            if (key_event.is_pressed && (key_event.ascii || key_event.scancode)) {
                gui_ipc_send_key_event(
                    key_event.ascii,
                    key_event.scancode,
                    key_event.is_pressed
                );
            }
        }

        /* 3.3 xHCI USB event poll (hot-plug, data transfers) */
        if ((loop_counter & 0x0FFF) == 0) {
            xhci_poll_events();
        }

        /* 3.4 Periodic kernel ticks (~every 16ms at 1MHz loop rate) */
        if ((loop_counter & 0x3FFF) == 0) {
            sched_tick();
            session_guard_audit();
            gui_ipc_tick();   /* Flush IPC ring buffer → Named Pipe */
        }

        /* 3.5 Network driver polling (e1000 / RTL8139) */
        if ((loop_counter & 0x07FF) == 0) {
            /* NIC drivers use IRQ, this is a fallback poll */
            uint8_t pkt[2048];
            int len = e1000_recv(pkt, sizeof(pkt));
            if (len > 0) {
                netmgr_on_packet_received("eth0", pkt, (uint16_t)len);
            }
        }

        /* 3.6 Periodic service telemetry (every ~1 second) */
        if ((loop_counter & 0xFFFF) == 0) {
            gui_ipc_send_service_event("NetMgr", 1, 10, "Running");
            gui_ipc_send_service_event("DrvMgr", 0,  6, "Running");
            drvmgr_publish_all();
        }
    }
}
