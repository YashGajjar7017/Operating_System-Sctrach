/**
 * @file vbe.h — Xenithra OS — VESA VBE Framebuffer Driver
 *
 * Provides a kernel-mode interface to the VESA BIOS Extensions (VBE)
 * framebuffer for VirtualBox / bare-metal display output.
 *
 * In the v3.0 architecture this driver is ONLY used for:
 *   1. Initial boot splash (before Electron starts)
 *   2. Fallback text console (kernel panic, debug shell)
 *   3. DXGKRNL proxy surface sharing
 *
 * The actual desktop rendering is done by the Electron Render Engine.
 */

#ifndef _DRIVER_VBE_H_
#define _DRIVER_VBE_H_

#include <stdint.h>
#include <stddef.h>

/* ── VBE Mode Info Block (512 bytes at 0x8000 from BIOS INT 10h) ───── */
typedef struct __attribute__((packed)) {
    /* Mandatory info for all VBE revisions */
    uint16_t mode_attributes;
    uint8_t  win_a_attributes;
    uint8_t  win_b_attributes;
    uint16_t win_granularity;
    uint16_t win_size;
    uint16_t win_a_segment;
    uint16_t win_b_segment;
    uint32_t win_func_ptr;
    uint16_t bytes_per_scan_line;

    /* Mandatory info for VBE 1.2+ */
    uint16_t x_resolution;
    uint16_t y_resolution;
    uint8_t  x_char_size;
    uint8_t  y_char_size;
    uint8_t  number_of_planes;
    uint8_t  bits_per_pixel;
    uint8_t  number_of_banks;
    uint8_t  memory_model;
    uint8_t  bank_size;
    uint8_t  number_of_image_pages;
    uint8_t  reserved0;

    /* Direct Color info */
    uint8_t  red_mask_size;
    uint8_t  red_field_position;
    uint8_t  green_mask_size;
    uint8_t  green_field_position;
    uint8_t  blue_mask_size;
    uint8_t  blue_field_position;
    uint8_t  rsv_mask_size;
    uint8_t  rsv_field_position;
    uint8_t  direct_color_mode_info;

    /* Mandatory info for VBE 2.0+ */
    uint32_t phys_base_ptr;    /* Physical base address of framebuffer */
    uint32_t reserved1;
    uint16_t reserved2;

    /* Mandatory info for VBE 3.0+ */
    uint16_t lin_bytes_per_scan_line;
    uint8_t  bnk_number_of_image_pages;
    uint8_t  lin_number_of_image_pages;
    uint8_t  lin_red_mask_size;
    uint8_t  lin_red_field_position;
    uint8_t  lin_green_mask_size;
    uint8_t  lin_green_field_position;
    uint8_t  lin_blue_mask_size;
    uint8_t  lin_blue_field_position;
    uint8_t  lin_rsv_mask_size;
    uint8_t  lin_rsv_field_position;
    uint32_t max_pixel_clock;

    uint8_t  reserved3[189];
} VbeModeInfo;

/* ── Framebuffer pixel format ─────────────────────────────────────────── */
typedef enum {
    VBE_FORMAT_RGB888  = 0,   /* 24-bit RGB (3 bytes per pixel) */
    VBE_FORMAT_ARGB8888 = 1,  /* 32-bit ARGB (4 bytes per pixel) */
    VBE_FORMAT_RGB565  = 2,   /* 16-bit RGB565 */
    VBE_FORMAT_BGR888  = 3,
} VbePixelFormat;

/* ── VBE framebuffer context ──────────────────────────────────────────── */
typedef struct {
    uint8_t        *fb_virt;       /* Virtual mapped framebuffer */
    uint8_t        *back_buf;      /* Double-buffer (kernel heap) */
    uint64_t        fb_phys;       /* Physical base address */
    uint32_t        width;
    uint32_t        height;
    uint32_t        pitch;         /* Bytes per scan line */
    uint8_t         bpp;           /* Bits per pixel */
    VbePixelFormat  format;
    uint32_t        fb_size;       /* Total framebuffer size in bytes */
} VbeContext;

/* ── Color helpers (32-bit ARGB) ─────────────────────────────────────── */
#define VBE_RGB(r,g,b)     (0xFF000000u | ((uint32_t)(r)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(b))
#define VBE_RGBA(r,g,b,a)  (((uint32_t)(a)<<24) | ((uint32_t)(r)<<16) | ((uint32_t)(g)<<8) | (uint32_t)(b))
#define VBE_BLACK          VBE_RGB(0,0,0)
#define VBE_WHITE          VBE_RGB(255,255,255)
#define VBE_XENITHRA_DARK  VBE_RGB(3,5,10)       /* --bg-base */
#define VBE_ACCENT_BLUE    VBE_RGB(96,165,250)   /* --clr-accent-primary */
#define VBE_ACCENT_PURPLE  VBE_RGB(167,139,250)  /* --clr-accent-secondary */

/* ── Public API ──────────────────────────────────────────────────────── */

/**
 * @brief Initialize VBE driver using framebuffer info from bootloader.
 * @param phys_base  Physical address of framebuffer (from XenithraBootInfo).
 * @param width, height  Display resolution.
 * @param pitch  Bytes per scan line.
 * @param bpp    Bits per pixel (typically 32).
 * @return 0 on success.
 */
int  vbe_init(uint64_t phys_base, uint32_t width, uint32_t height,
              uint32_t pitch, uint8_t bpp);

/** Fill a rectangle with a solid color. */
void vbe_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

/** Draw a horizontal gradient rectangle. */
void vbe_fill_gradient(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                        uint32_t color_left, uint32_t color_right);

/** Put a single pixel. */
static inline void vbe_put_pixel(VbeContext *ctx, uint32_t x, uint32_t y, uint32_t color) {
    if (x >= ctx->width || y >= ctx->height) return;
    uint32_t *row = (uint32_t *)(ctx->back_buf + y * ctx->pitch);
    row[x] = color;
}

/** Draw a bitmap glyph (8×16 from shared/font.h). */
void vbe_draw_glyph(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);

/** Draw a string using the bitmap font. */
void vbe_draw_string(uint32_t x, uint32_t y, const char *str, uint32_t fg, uint32_t bg);

/** Draw a formatted string (like printf). */
void vbe_printf(uint32_t x, uint32_t y, uint32_t fg, uint32_t bg, const char *fmt, ...);

/** Draw a rounded rectangle outline. */
void vbe_draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                            uint32_t color, uint32_t thickness);

/** Present the back-buffer to the physical framebuffer (double-buffered flip). */
void vbe_present(void);

/** Clear the entire framebuffer to a color. */
void vbe_clear(uint32_t color);

/**
 * @brief Render a minimal kernel boot splash screen.
 *        Called before Electron render engine connects.
 *        Shows Xenithra OS logo, version, and boot progress bar.
 */
void vbe_draw_boot_splash(const char *status_line, uint32_t progress_pct);

/**
 * @brief Render a kernel panic screen.
 *        Called from panic() handler in kernel/exec/kshell.c.
 */
void vbe_draw_panic(const char *message, const char *file, uint32_t line,
                    uint64_t rip, uint64_t rsp, uint64_t cr3);

/** Get the VBE context pointer (read-only access for other subsystems). */
const VbeContext *vbe_get_context(void);

/** Check if VBE is initialized. */
uint8_t vbe_is_ready(void);

#endif /* _DRIVER_VBE_H_ */
