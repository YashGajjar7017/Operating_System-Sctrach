/**
 * @file vbe.c — Xenithra OS — VESA VBE Framebuffer Driver Implementation
 */

#include "vbe.h"
#include "../../kstring.h"
#include "../../../shared/font.h"
#include <stdarg.h>

static VbeContext g_vbe_ctx;
static uint8_t    g_vbe_ready = 0;
static uint32_t   g_back_storage[1920 * 1080];

int vbe_init(uint64_t phys_base, uint32_t width, uint32_t height,
             uint32_t pitch, uint8_t bpp)
{
    if (!phys_base || width == 0 || height == 0) {
        return -1;
    }

    g_vbe_ctx.fb_phys   = phys_base;
    g_vbe_ctx.fb_virt   = (uint8_t *)phys_base;
    g_vbe_ctx.width     = width;
    g_vbe_ctx.height    = height;
    g_vbe_ctx.pitch     = pitch ? pitch : (width * 4);
    g_vbe_ctx.bpp       = bpp ? bpp : 32;
    g_vbe_ctx.format    = VBE_FORMAT_ARGB8888;
    g_vbe_ctx.fb_size   = g_vbe_ctx.pitch * g_vbe_ctx.height;

    if (width * height <= 1920 * 1080) {
        g_vbe_ctx.back_buf = (uint8_t *)g_back_storage;
    } else {
        g_vbe_ctx.back_buf = g_vbe_ctx.fb_virt;
    }

    g_vbe_ready = 1;
    vbe_clear(VBE_XENITHRA_DARK);
    vbe_present();
    return 0;
}

const VbeContext *vbe_get_context(void) {
    return &g_vbe_ctx;
}

uint8_t vbe_is_ready(void) {
    return g_vbe_ready;
}

void vbe_present(void) {
    if (!g_vbe_ready || !g_vbe_ctx.fb_virt || !g_vbe_ctx.back_buf) return;
    if (g_vbe_ctx.fb_virt != g_vbe_ctx.back_buf) {
        memcpy(g_vbe_ctx.fb_virt, g_vbe_ctx.back_buf, g_vbe_ctx.fb_size);
    }
}

void vbe_clear(uint32_t color) {
    if (!g_vbe_ready || !g_vbe_ctx.back_buf) return;
    uint32_t total_pixels = (g_vbe_ctx.pitch / 4) * g_vbe_ctx.height;
    uint32_t *p = (uint32_t *)g_vbe_ctx.back_buf;
    for (uint32_t i = 0; i < total_pixels; i++) {
        p[i] = color;
    }
}

void vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!g_vbe_ready || !g_vbe_ctx.back_buf) return;
    if (x >= g_vbe_ctx.width || y >= g_vbe_ctx.height) return;
    if (x + w > g_vbe_ctx.width)  w = g_vbe_ctx.width - x;
    if (y + h > g_vbe_ctx.height) h = g_vbe_ctx.height - y;

    for (uint32_t row = 0; row < h; row++) {
        uint32_t *dst = (uint32_t *)(g_vbe_ctx.back_buf + (y + row) * g_vbe_ctx.pitch) + x;
        for (uint32_t col = 0; col < w; col++) {
            dst[col] = color;
        }
    }
}

void vbe_fill_gradient(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                       uint32_t color_left, uint32_t color_right)
{
    if (!g_vbe_ready || !g_vbe_ctx.back_buf || w == 0) return;
    if (x >= g_vbe_ctx.width || y >= g_vbe_ctx.height) return;
    if (x + w > g_vbe_ctx.width)  w = g_vbe_ctx.width - x;
    if (y + h > g_vbe_ctx.height) h = g_vbe_ctx.height - y;

    int r1 = (color_left >> 16) & 0xFF, g1 = (color_left >> 8) & 0xFF, b1 = color_left & 0xFF;
    int r2 = (color_right >> 16) & 0xFF, g2 = (color_right >> 8) & 0xFF, b2 = color_right & 0xFF;

    for (uint32_t col = 0; col < w; col++) {
        int r = r1 + ((r2 - r1) * (int)col) / (int)w;
        int g = g1 + ((g2 - g1) * (int)col) / (int)w;
        int b = b1 + ((b2 - b1) * (int)col) / (int)w;
        uint32_t c = VBE_RGB(r, g, b);
        for (uint32_t row = 0; row < h; row++) {
            uint32_t *dst = (uint32_t *)(g_vbe_ctx.back_buf + (y + row) * g_vbe_ctx.pitch) + (x + col);
            *dst = c;
        }
    }
}

void vbe_draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                           uint32_t color, uint32_t thickness)
{
    vbe_fill_rect(x, y, w, thickness, color);
    vbe_fill_rect(x, y + h - thickness, w, thickness, color);
    vbe_fill_rect(x, y, thickness, h, color);
    vbe_fill_rect(x + w - thickness, y, thickness, h, color);
}

void vbe_draw_glyph(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg) {
    if (!g_vbe_ready || !g_vbe_ctx.back_buf) return;
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = font_8x16[(int)c - 32];

    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                vbe_put_pixel(&g_vbe_ctx, x + col, y + row, fg);
            } else if (bg != 0) {
                vbe_put_pixel(&g_vbe_ctx, x + col, y + row, bg);
            }
        }
    }
}

void vbe_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg, uint32_t bg) {
    if (!str) return;
    uint32_t cur_x = x;
    while (*str) {
        if (*str == '\n') {
            cur_x = x;
            y += 16;
        } else {
            vbe_draw_glyph(cur_x, y, *str, fg, bg);
            cur_x += 8;
        }
        str++;
    }
}

void vbe_printf(uint32_t x, uint32_t y, uint32_t fg, uint32_t bg, const char *fmt, ...) {
    char buf[256];
    strncpy(buf, fmt, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    vbe_draw_string(x, y, buf, fg, bg);
}

void vbe_draw_boot_splash(const char *status_line, uint32_t progress_pct) {
    if (!g_vbe_ready) return;
    if (progress_pct > 100) progress_pct = 100;

    vbe_clear(VBE_XENITHRA_DARK);

    uint32_t cx = g_vbe_ctx.width / 2;
    uint32_t cy = g_vbe_ctx.height / 2;

    const char *title = "X E N I T H R A   O S   v 3 . 0";
    uint32_t title_len = (uint32_t)strlen(title);
    vbe_draw_string(cx - (title_len * 8) / 2, cy - 60, title, VBE_ACCENT_BLUE, 0);

    const char *sub = "Kernel + Render Engine Subsystem";
    vbe_draw_string(cx - ((uint32_t)strlen(sub) * 8) / 2, cy - 36, sub, VBE_ACCENT_PURPLE, 0);

    /* Progress bar */
    uint32_t bar_w = 400;
    uint32_t bar_h = 10;
    uint32_t bar_x = cx - bar_w / 2;
    uint32_t bar_y = cy + 10;

    vbe_draw_rect_outline(bar_x - 2, bar_y - 2, bar_w + 4, bar_h + 4, VBE_RGB(40, 50, 70), 1);
    vbe_fill_rect(bar_x, bar_y, bar_w, bar_h, VBE_RGB(15, 20, 30));

    uint32_t filled_w = (bar_w * progress_pct) / 100;
    if (filled_w > 0) {
        vbe_fill_gradient(bar_x, bar_y, filled_w, bar_h, VBE_ACCENT_BLUE, VBE_ACCENT_PURPLE);
    }

    if (status_line) {
        uint32_t slen = (uint32_t)strlen(status_line);
        vbe_draw_string(cx - (slen * 8) / 2, bar_y + 24, status_line, VBE_WHITE, 0);
    }

    vbe_present();
}

void vbe_draw_panic(const char *message, const char *file, uint32_t line,
                    uint64_t rip, uint64_t rsp, uint64_t cr3)
{
    if (!g_vbe_ready) return;
    vbe_clear(VBE_RGB(120, 10, 10));

    vbe_draw_string(40, 40, "=== KERNEL PANIC ===", VBE_WHITE, 0);
    if (message) vbe_draw_string(40, 70, message, VBE_WHITE, 0);
    if (file)    vbe_draw_string(40, 100, file, VBE_WHITE, 0);

    char num_buf[32];
    uint_to_str(line, num_buf);
    vbe_draw_string(40, 120, "Line: ", VBE_WHITE, 0);
    vbe_draw_string(90, 120, num_buf, VBE_WHITE, 0);

    vbe_present();
}
