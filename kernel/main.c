/**
 * @file main.c
 * @brief 64-bit Kernel Main Entry & Hardware Verification
 */

#include <stdint.h>
#include <stddef.h>
#include "../shared/bootinfo.h"

/* Forward font definition (8x16 font table) */
extern const uint8_t font_8x16[95][16];

static AuraBootInfo g_kernel_boot_info;

/* Kernel-side direct framebuffer drawing */
static void k_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= g_kernel_boot_info.framebuffer.width || y >= g_kernel_boot_info.framebuffer.height) {
        return;
    }
    uint32_t *fb = (uint32_t*)g_kernel_boot_info.framebuffer.base_address;
    fb[y * g_kernel_boot_info.framebuffer.pixels_per_scanline + x] = color;
}

static void k_fill_rect(int x, int y, int w, int h, uint32_t color) {
    int max_w = (int)g_kernel_boot_info.framebuffer.width;
    int max_h = (int)g_kernel_boot_info.framebuffer.height;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > max_w) w = max_w - x;
    if (y + h > max_h) h = max_h - y;
    if (w <= 0 || h <= 0) return;

    uint32_t *fb = (uint32_t*)g_kernel_boot_info.framebuffer.base_address;
    uint32_t stride = g_kernel_boot_info.framebuffer.pixels_per_scanline;

    for (int r = y; r < y + h; r++) {
        uint32_t *line = &fb[r * stride + x];
        for (int c = 0; c < w; c++) {
            line[c] = color;
        }
    }
}

static void k_draw_char(int x, int y, char c, uint32_t fg, uint32_t bg, int transparent, int scale) {
    if (c < 32 || c > 126) c = '?';
    if (scale <= 0) scale = 1;

    const uint8_t *glyph = font_8x16[(int)c - 32];
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            int bit = (bits >> (7 - col)) & 1;
            uint32_t color = bit ? fg : bg;
            if (bit || !transparent) {
                if (scale == 1) {
                    k_put_pixel(x + col, y + row, color);
                } else {
                    k_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

static void k_print(int x, int y, const char *str, uint32_t fg) {
    int cur_x = x;
    int cur_y = y;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cur_x = x;
            cur_y += 20;
            continue;
        }
        k_draw_char(cur_x, cur_y, str[i], fg, 0, 1, 1);
        cur_x += 8;
    }
}

/* Simple integer to hex/dec string converters */
static void uint64_to_hex(uint64_t val, char *buf) {
    buf[0] = '0'; buf[1] = 'x';
    const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; i--) {
        buf[2 + (15 - i)] = hex_chars[(val >> (i * 4)) & 0xF];
    }
    buf[18] = '\0';
}

static void uint64_to_dec(uint64_t val, char *buf) {
    if (val == 0) {
        buf[0] = '0'; buf[1] = '\0';
        return;
    }
    char temp[24];
    int len = 0;
    while (val > 0) {
        temp[len++] = '0' + (val % 10);
        val /= 10;
    }
    for (int i = 0; i < len; i++) {
        buf[i] = temp[len - 1 - i];
    }
    buf[len] = '\0';
}

/**
 * @brief Kernel Entry Point called from entry.asm
 */
void kmain(AuraBootInfo *boot_info) {
    /* Copy boot information to kernel memory */
    if (boot_info && boot_info->magic == AURA_BOOT_MAGIC) {
        g_kernel_boot_info = *boot_info;
    }

    uint32_t width = g_kernel_boot_info.framebuffer.width;
    uint32_t height = g_kernel_boot_info.framebuffer.height;

    /* Fill background with sleek Slate/Mica dark theme */
    k_fill_rect(0, 0, width, height, 0x000E1117);

    /* Draw Modern Header / Title Banner */
    k_fill_rect(0, 0, width, 60, 0x00161B22);
    k_fill_rect(0, 59, width, 1, 0x0030363D);

    k_print(24, 20, "AuraOS 64-bit Kernel Core (v0.1.0-alpha)", 0x0058A6FF);
    k_print(width - 240, 20, "Architecture: x86_64", 0x008B949E);

    /* System Status Card */
    int card_x = 40;
    int card_y = 80;
    int card_w = 680;
    int card_h = 360;

    k_fill_rect(card_x, card_y, card_w, card_h, 0x001F242C);
    k_fill_rect(card_x, card_y, card_w, 1, 0x0030363D);
    k_fill_rect(card_x, card_y + card_h - 1, card_w, 1, 0x0030363D);
    k_fill_rect(card_x, card_y, 1, card_h, 0x0030363D);
    k_fill_rect(card_x + card_w - 1, card_y, 1, card_h, 0x0030363D);

    k_print(card_x + 20, card_y + 16, "[System Boot Diagnostics & Hardware Inspection]", 0x00F0F6FC);

    /* Print Framebuffer Resolution */
    char num_buf[32];
    k_print(card_x + 20, card_y + 50, "GOP Video Mode: ", 0x008B949E);
    uint64_to_dec(width, num_buf);
    k_print(card_x + 180, card_y + 50, num_buf, 0x007EE787);
    k_print(card_x + 220, card_y + 50, "x", 0x008B949E);
    uint64_to_dec(height, num_buf);
    k_print(card_x + 236, card_y + 50, num_buf, 0x007EE787);
    k_print(card_x + 280, card_y + 50, " (32-bit Linear Framebuffer)", 0x008B949E);

    /* Print Framebuffer Base */
    k_print(card_x + 20, card_y + 80, "VRAM Phys Base: ", 0x008B949E);
    uint64_to_hex(g_kernel_boot_info.framebuffer.base_address, num_buf);
    k_print(card_x + 180, card_y + 80, num_buf, 0x00D2A8FF);

    /* Print Memory Statistics */
    uint64_t total_mb = g_kernel_boot_info.memory_map.total_memory_bytes / (1024 * 1024);
    uint64_t usable_mb = g_kernel_boot_info.memory_map.usable_memory_bytes / (1024 * 1024);

    k_print(card_x + 20, card_y + 110, "Total Physical RAM: ", 0x008B949E);
    uint64_to_dec(total_mb, num_buf);
    k_print(card_x + 180, card_y + 110, num_buf, 0x0079C0FF);
    k_print(card_x + 230, card_y + 110, "MB", 0x008B949E);

    k_print(card_x + 20, card_y + 140, "Usable Free RAM:   ", 0x008B949E);
    uint64_to_dec(usable_mb, num_buf);
    k_print(card_x + 180, card_y + 140, num_buf, 0x007EE787);
    k_print(card_x + 230, card_y + 140, "MB", 0x008B949E);

    /* Print ACPI RSDP Pointer */
    k_print(card_x + 20, card_y + 170, "ACPI RSDP Table:   ", 0x008B949E);
    if (g_kernel_boot_info.acpi_rsdp) {
        uint64_to_hex(g_kernel_boot_info.acpi_rsdp, num_buf);
        k_print(card_x + 180, card_y + 170, num_buf, 0x00FFA657);
    } else {
        k_print(card_x + 180, card_y + 170, "Not Found (Legacy ACPI)", 0x00FF7B72);
    }

    /* Print Boot Mode */
    k_print(card_x + 20, card_y + 200, "Kernel Boot Mode:  ", 0x008B949E);
    if (g_kernel_boot_info.boot_mode == 0) {
        k_print(card_x + 180, card_y + 200, "Desktop Normal", 0x007EE787);
    } else if (g_kernel_boot_info.boot_mode == 1) {
        k_print(card_x + 180, card_y + 200, "Safe Mode / Verbose Logging", 0x00FFA657);
    } else {
        k_print(card_x + 180, card_y + 200, "Diagnostics", 0x0079C0FF);
    }

    /* Transition Status */
    k_print(card_x + 20, card_y + 240, "Status: Kernel Handoff Successful -> Ready for Phase 3.", 0x0058A6FF);
    k_print(card_x + 20, card_y + 270, "Next steps: GDT, IDT Interrupts, 4-Level Paging & Buddy/Slab Allocator.", 0x008B949E);

    /* Bottom Taskbar Placeholder */
    k_fill_rect(0, height - 48, width, 48, 0x00161B22);
    k_fill_rect(0, height - 49, width, 1, 0x0030363D);

    /* Modern Start Button */
    k_fill_rect(12, height - 40, 32, 32, 0x000078D4);
    k_print(56, height - 32, "AuraOS Desktop Engine", COLOR_RGB(240, 246, 252));

    /* Kernel infinite halt loop */
    while (1) {
        __asm__ volatile ("hlt");
    }
}
