/**
 * @file main.c
 * @brief Xenithra OS 64-bit Kernel Main Entry & Windows 11 Desktop Environment Boot
 */

#include <stdint.h>
#include <stddef.h>
#include "../shared/bootinfo.h"
#include "security/session.h"
#include "security/firewall.h"
#include "drivers/ps2.h"
#include "drivers/sound.h"
#include "sched/sched.h"
#include "gui/anim.h"
#include "gui/compositor.h"
#include "gui/v8_engine.h"
#include "apps/browser_app.h"
#include "apps/explorer_app.h"
#include "apps/taskmgr_app.h"
#include "apps/firewall_app.h"
#include "apps/terminal_app.h"
#include "apps/vlc_app.h"
#include "apps/installer_app.h"
#include "apps/diskclone_app.h"

static XenithraBootInfo g_kernel_boot_info;

/**
 * @brief Kernel Entry Point called from entry.asm
 */
void kmain(XenithraBootInfo *boot_info) {
    /* 1. Copy Boot Information */
    if (boot_info && boot_info->magic == XENITHRA_BOOT_MAGIC) {
        g_kernel_boot_info = *boot_info;
    }

    /* 2. Initialize Hardware Sound Driver & Synthesize Ambient Startup Chime */
    sound_init();
    play_system_startup_chime();

    /* 3. Initialize Preemptive Multilevel Feedback Queue (MLFQ) Scheduler */
    sched_init();

    /* 4. Initialize Hardware PS/2 Mouse & Keyboard Drivers */
    uint32_t screen_w = g_kernel_boot_info.framebuffer.width ? g_kernel_boot_info.framebuffer.width : 1280;
    uint32_t screen_h = g_kernel_boot_info.framebuffer.height ? g_kernel_boot_info.framebuffer.height : 720;
    ps2_init(screen_w, screen_h);

    /* 5. Initialize Kernel Security & Anti-Hijack Guard */
    security_init();
    security_enable_smep_smap();

    /* 6. Initialize Kernel Private Firewall */
    firewall_init();

    /* 7. Initialize Windows 11 Fluent Window Compositor & V8 JavaScript Bridge */
    compositor_init(g_kernel_boot_info.framebuffer);
    v8_engine_init();

    /* 8. Launch Modern Microsoft Edge / Google React App at startup */
    browser_app_launch();

    /* 9. Render Initial Windows 11 Desktop State */
    compositor_render();

    /* 10. High-Performance Hardware Event Pump & Scheduling Loop */
    PS2MouseState mouse_state;
    PS2KeyEvent key_event;
    uint64_t loop_counter = 0;

    while (1) {
        loop_counter++;

        /* 10.1 Poll PS/2 Mouse Hardware */
        if (ps2_poll_mouse(&mouse_state)) {
            compositor_update_mouse(
                mouse_state.x,
                mouse_state.y,
                mouse_state.left_button,
                mouse_state.right_button,
                mouse_state.middle_button
            );
        }

        /* 10.2 Poll PS/2 Keyboard Hardware */
        if (ps2_poll_keyboard(&key_event)) {
            if (key_event.is_pressed && (key_event.ascii || key_event.scancode)) {
                compositor_dispatch_key(key_event.ascii, key_event.scancode, key_event.is_pressed);
            }
        }

        /* 10.3 Preemptive Sched, V8 Engine & Compositor 60 FPS Animation Ticks */
        if ((loop_counter & 0x3FFF) == 0) {
            sched_tick();
            v8_engine_tick();
            session_guard_audit();
            compositor_tick();
            compositor_render();
        }
    }
}
