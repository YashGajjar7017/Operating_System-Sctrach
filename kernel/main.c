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
#include "gui/compositor.h"
#include "apps/explorer_app.h"
#include "apps/taskmgr_app.h"
#include "apps/firewall_app.h"
#include "apps/terminal_app.h"
#include "apps/vlc_app.h"
#include "apps/installer_app.h"

static XenithraBootInfo g_kernel_boot_info;

/**
 * @brief Kernel Entry Point called from entry.asm
 */
void kmain(XenithraBootInfo *boot_info) {
    /* 1. Copy Boot Information */
    if (boot_info && boot_info->magic == XENITHRA_BOOT_MAGIC) {
        g_kernel_boot_info = *boot_info;
    }

    /* 2. Initialize Hardware PS/2 Mouse & Keyboard Drivers */
    uint32_t screen_w = g_kernel_boot_info.framebuffer.width ? g_kernel_boot_info.framebuffer.width : 1280;
    uint32_t screen_h = g_kernel_boot_info.framebuffer.height ? g_kernel_boot_info.framebuffer.height : 720;
    ps2_init(screen_w, screen_h);

    /* 3. Initialize Kernel Security & Anti-Hijack Guard */
    security_init();
    security_enable_smep_smap();

    /* 4. Initialize Kernel Private Firewall */
    firewall_init();

    /* 5. Initialize Windows 11 Fluent Window Compositor */
    compositor_init(g_kernel_boot_info.framebuffer);

    /* 6. Launch Built-in Windows 11 Applications Suite */
    explorer_app_launch();
    vlc_app_launch();
    installer_app_launch();
    taskmgr_app_launch();

    /* 7. Render Initial Windows 11 Desktop State */
    compositor_render();

    /* 8. High-Performance Hardware Event Pump & Scheduling Loop */
    PS2MouseState mouse_state;
    PS2KeyEvent key_event;
    uint64_t loop_counter = 0;

    while (1) {
        loop_counter++;

        /* 8.1 Poll PS/2 Mouse Hardware */
        if (ps2_poll_mouse(&mouse_state)) {
            compositor_update_mouse(
                mouse_state.x,
                mouse_state.y,
                mouse_state.left_button,
                mouse_state.right_button,
                mouse_state.middle_button
            );
        }

        /* 8.2 Poll PS/2 Keyboard Hardware */
        if (ps2_poll_keyboard(&key_event)) {
            if (key_event.is_pressed && (key_event.ascii || key_event.scancode)) {
                compositor_dispatch_key(key_event.ascii, key_event.scancode, key_event.is_pressed);
            }
        }

        /* 8.3 Periodic Background Audits and Animation Ticks (60 FPS feel) */
        if ((loop_counter & 0x3FFF) == 0) {
            session_guard_audit();
            compositor_tick();
            compositor_render();
        }
    }
}
