/**
 * @file main.c
 * @brief Xenithra OS 64-bit Kernel Main Entry & Desktop Environment Boot
 */

#include <stdint.h>
#include <stddef.h>
#include "../shared/bootinfo.h"
#include "security/session.h"
#include "security/firewall.h"
#include "gui/compositor.h"
#include "apps/firewall_app.h"
#include "apps/terminal_app.h"

static XenithraBootInfo g_kernel_boot_info;

/**
 * @brief Kernel Entry Point called from entry.asm
 */
void kmain(XenithraBootInfo *boot_info) {
    /* 1. Copy Boot Information */
    if (boot_info && boot_info->magic == XENITHRA_BOOT_MAGIC) {
        g_kernel_boot_info = *boot_info;
    }

    /* 2. Initialize Kernel Security & Anti-Hijack Guard */
    security_init();
    security_enable_smep_smap();

    /* 3. Initialize Kernel Private Firewall */
    firewall_init();

    /* 4. Initialize Desktop Window Compositor with GOP Framebuffer */
    compositor_init(g_kernel_boot_info.framebuffer);

    /* 5. Launch Built-in Secure GUI Applications */
    firewall_app_launch();
    terminal_app_launch();

    /* 6. Render Initial Desktop & Window Compositor State */
    compositor_render();

    /* 7. Kernel Main Event & Scheduling Loop */
    while (1) {
        /* Run periodic security audit for session hijack attempts */
        session_guard_audit();

        /* Halt CPU until next hardware interrupt */
        __asm__ volatile ("hlt");
    }
}
