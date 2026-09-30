/**
 * @file main.c
 * @brief Xenithra OS 64-bit Kernel Main Entry & Modern Desktop Environment Boot
 *
 * Boot Phase Summary:
 *   Phase 0 (interrupts OFF, BSP only, IRQL=HIGH_LEVEL):
 *     hardware_tables_init() → GDT / IDT / TSS
 *     kpcr_init()            → GS_BASE MSR → per-CPU KPCR
 *     vmm_init()             → PFN Database → Virtual Memory Manager
 *     kheap_init()           → Kernel Non-Paged Pool (64 MB)
 *     apic_init()            → APIC MMIO enable (ACPI MADT parse)
 *     syscall_init_msrs()    → IA32_LSTAR / IA32_STAR / IA32_FMASK
 *
 *   Phase 1 (interrupts ON, SMP, IRQL=PASSIVE_LEVEL):
 *     apic_calibrate_timer() → TSC/APIC tick calibration
 *     apic_start_all_aps()   → INIT-SIPI-SIPI → APs enter Long Mode
 *     sched_init()           → RTOS priority scheduler (6 levels)
 *     smss_master_init()     → Session 0 / Session 1 subsystem threads
 *
 *   Phase 2 (GUI Chain — Node.js → Vite/Electron):
 *     gui_ipc_init()         → Named Pipe server for Electron IPC
 *     kshell_init()          → GDB-capable kernel debug shell thread
 *     smss registers:        → SysMain, DWM proxy, MMCSS, AudioSrv, WMI
 *
 * GUI Architecture (replaces legacy C compositor + Django):
 *   kernel → gui_ipc Named Pipe → Node.js host → Electron → React TSX UI
 *
 * REMOVED (legacy):
 *   - sys_launch_django_kiosk()  [Django backend — fully removed]
 *   - compositor_render()        [C framebuffer drawing — replaced by Electron]
 *   - v8_engine_init/tick()      [Fake V8 stub — replaced by real Node.js]
 *   - py_runtime_init/tick()     [Python inline runtime stub]
 *   - All kernel/gui draw primitives (gui_fill_rect, gui_draw_string, etc.)
 */

#include <stdint.h>
#include <stddef.h>
#include "../shared/bootinfo.h"

/* Phase 0: Hardware tables, VMM, KPCR */
#include "arch/x86_64/gdt_idt.h"
#include "arch/x86_64/apic.h"
#include "mm/vmm.h"
#include "mm/heap.h"
#include "exec/kpcr.h"
#include "exec/smss.h"

/* Phase 1: Core Kernel Subsystems */
#include "security/session.h"
#include "security/firewall.h"
#include "drivers/ps2.h"
#include "drivers/sound.h"
#include "sched/sched.h"
#include "gui/anim.h"
#include "gui/compositor.h"
#include "gui/v8_engine.h"
#include "python/py_runtime.h"

/* Phase 2: New GUI IPC Layer */
#include "gui/gui_ipc.h"

/* Kernel Shell (GDB-compatible debug shell) */
#include "exec/kshell.h"

/* Kernel-level syscall providers (draw-stripped, IPC-backed) */
#include "apps/browser_app.h"
#include "apps/explorer_app.h"
#include "apps/taskmgr_app.h"
#include "apps/firewall_app.h"
#include "apps/terminal_app.h"
#include "apps/vlc_app.h"
#include "apps/installer_app.h"
#include "apps/diskclone_app.h"

/* Kernel Non-Paged Pool size: 64 MB allocated from VMM */
#define KERNEL_HEAP_SIZE  (64ULL * 1024 * 1024)

static XenithraBootInfo g_kernel_boot_info;

/* Declared in gdt_idt.asm */
extern void syscall_init_msrs(void);

/**
 * @brief Kernel Entry Point called from entry.asm
 *
 * Execution begins here with:
 *   - Interrupts DISABLED (CLI by entry.asm)
 *   - 64-bit Long Mode active (bootloader set this up)
 *   - Higher-half PML4 page tables active (CR3 from bootloader)
 *   - Stack = 64 KB BSS region in entry.asm
 *   - RDI / RCX = pointer to XenithraBootInfo (UEFI → kernel ABI)
 */
void kmain(XenithraBootInfo *boot_info) {
    /* =========================================================
     * PHASE 0: Single-Core, Interrupts Masked (IRQL=HIGH_LEVEL)
     * ========================================================= */

    /* 1. Copy Boot Information from bootloader ABI contract */
    if (boot_info && boot_info->magic == XENITHRA_BOOT_MAGIC) {
        g_kernel_boot_info = *boot_info;
    }

    /* 2. Initialize GDT, IDT, TSS — hardware descriptor tables
     *    Sets up all 7 GDT selectors (Null/KernCS/KernDS/UserDS/UserCS/TSS),
     *    256 IDT interrupt gates from ISR stubs, and TSS IST stacks.
     *    Calls: LGDT → far-jump → LIDT → LTR */
    hardware_tables_init();

    /* 3. Initialize per-CPU KPCR for the Bootstrap Processor (CPU 0).
     *    Writes IA32_GS_BASE MSR so GS-relative KPCR access works.
     *    APIC ID 0 for BSP, uses the hardware tables we just installed. */
    extern GDT_ENTRY g_gdt[];
    extern IDT_ENTRY g_idt[];
    extern TASK_STATE_SEGMENT g_tss;
    kpcr_init(0, 0, &g_tss, g_idt, g_gdt);

    /* 4. Initialize Physical Frame Allocator (PFN Database) and VMM.
     *    Parses the UEFI memory map, builds the free-list PFN database,
     *    and establishes the kernel virtual pool region. */
    vmm_init(&g_kernel_boot_info);

    /* 5. Initialize Kernel Non-Paged Pool (64 MB boundary-tag heap).
     *    Allocates 16384 × 4KB pages from the VMM, then bootstraps
     *    the boundary-tag free-list allocator on top of it. */
    uint64_t heap_va = vmm_alloc_pages(KERNEL_HEAP_SIZE / 4096, PTE_KERNEL_RW);
    if (heap_va) {
        kheap_init(heap_va, KERNEL_HEAP_SIZE);
    }

    /* 6. Initialize Local APIC — parse ACPI RSDP/MADT, map MMIO, enable SIVR.
     *    Also discovers all CPU APIC IDs for the SMP boot sequence. */
    apic_init(g_kernel_boot_info.acpi_rsdp);

    /* 7. Configure SYSCALL/SYSRET MSRs for fast system calls:
     *    IA32_LSTAR → syscall_entry, IA32_STAR → segment selectors,
     *    IA32_FMASK → clear IF+DF on SYSCALL, IA32_EFER.SCE=1 */
    syscall_init_msrs();

    /* =========================================================
     * PHASE 1: Interrupts Enabled, SMP, (IRQL=PASSIVE_LEVEL)
     * ========================================================= */

    /* 8. Enable interrupts — Phase 0 complete, safe to handle IRQs now */
    kpcr_set_irql(IRQL_PASSIVE_LEVEL);
    __asm__ volatile ("sti");

    /* 9. Calibrate APIC Timer against PIT (10ms measurement).
     *    Must be done with interrupts enabled for the PIT delay to work.
     *    Result: s_apic_ticks_per_ms calibrated for the scheduler tick. */
    apic_calibrate_timer();

    /* 10. Arm the APIC periodic scheduler tick (1ms = 1000µs interval).
     *     IRQ vector 0x20 → isr_common_handler → sched_tick() via DPC. */
    apic_arm_timer(1000);

    /* 11. Wake Application Processors via INIT-SIPI-SIPI sequence.
     *     APs boot trampoline at 0x8000 → Long Mode → ap_kernel_entry_c().
     *     Each AP initializes its own KPCR and enters the idle loop. */
    if (g_cpu_count > 1) {
        apic_start_all_aps(0x8000);
    }

    /* =========================================================
     * PHASE 1B: CORE SUBSYSTEM INITIALIZATION
     * ========================================================= */

    /* 12. Initialize Hardware Sound Driver & Synthesize Ambient Startup Chime */
    sound_init();
    play_system_startup_chime();

    /* 13. Initialize RTOS Priority Scheduler (6 priority levels + DPC queue) */
    sched_init();

    /* 14. Session Manager Subsystem (SMSS) — Session 0 / Session 1 lifecycle.
     *     Creates kernel threads for: wininit, services, lsass (Session 0)
     *     and csrss, winlogon, dwm_proxy, userinit (Session 1).
     *     smss_master_init() runs as a kernel thread at RTOS_PRIO_HIGH. */
    sched_create_kthread(smss_master_init, RTOS_PRIO_HIGH);

    /* 15. Initialize Hardware PS/2 Mouse & Keyboard Drivers */
    uint32_t screen_w = g_kernel_boot_info.framebuffer.width ? g_kernel_boot_info.framebuffer.width : 1280;
    uint32_t screen_h = g_kernel_boot_info.framebuffer.height ? g_kernel_boot_info.framebuffer.height : 720;
    ps2_init(screen_w, screen_h);

    /* 16. Initialize Kernel Security & Anti-Hijack Guard */
    security_init();
    security_enable_smep_smap();

    /* 17. Initialize Kernel Private Firewall */
    firewall_init();

    /* =========================================================
     * PHASE 2: GRAPHICAL COMPOSITOR & IPC SUBSYSTEMS
     * ========================================================= */

    /* 18. Initialize Windows 11 Fluent Compositor & Framebuffer */
    compositor_init(g_kernel_boot_info.framebuffer);
    v8_engine_init();
    py_runtime_init();

    /* 19. Launch Desktop Kiosk / Main Browser Shell */
    sys_launch_django_kiosk();

    /* 20. Render Initial Desktop State onto GOP Framebuffer */
    compositor_render();

    /* 21. Open kernel-side Named Pipe server for IPC */
    gui_ipc_init();

    /* 22. Spawn kernel debug shell (GDB-compatible, REALTIME priority) */
    sched_create_kthread(kshell_init, RTOS_PRIO_REALTIME);

    /* =========================================================
     * PHASE 3: HIGH-PERFORMANCE EVENT PUMP & COMPOSITING LOOP
     * ========================================================= */

    PS2MouseState mouse_state;
    PS2KeyEvent   key_event;
    uint64_t      loop_counter = 0;

    while (1) {
        loop_counter++;

        /* 3.1 Poll PS/2 Mouse Hardware → route to Compositor & IPC */
        if (ps2_poll_mouse(&mouse_state)) {
            compositor_update_mouse(
                mouse_state.x,
                mouse_state.y,
                mouse_state.left_button,
                mouse_state.right_button,
                mouse_state.middle_button
            );
            gui_ipc_send_mouse_event(
                mouse_state.x,
                mouse_state.y,
                mouse_state.left_button,
                mouse_state.right_button,
                mouse_state.middle_button
            );
        }

        /* 3.2 Poll PS/2 Keyboard Hardware → route to Compositor & IPC */
        if (ps2_poll_keyboard(&key_event)) {
            if (key_event.is_pressed && (key_event.ascii || key_event.scancode)) {
                compositor_dispatch_key(key_event.ascii, key_event.scancode, key_event.is_pressed);
                gui_ipc_send_key_event(key_event.ascii, key_event.scancode, key_event.is_pressed);
            }
        }

        /* 3.3 Periodic RTOS tick, engine ticks, and Compositor Frame Render */
        if ((loop_counter & 0x3FFF) == 0) {
            sched_tick();
            py_runtime_tick();
            v8_engine_tick();
            session_guard_audit();
            compositor_tick();
            compositor_render();
            gui_ipc_tick();
        }
    }
}
