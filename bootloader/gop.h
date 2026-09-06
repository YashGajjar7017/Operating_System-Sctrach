/**
 * @file gop.h
 * @brief High-performance double-buffered GOP rendering engine for UEFI Bootloader
 */

#ifndef _BOOTLOADER_GOP_H_
#define _BOOTLOADER_GOP_H_

#include "efi.h"
#include "../shared/bootinfo.h"

/* Color Macros (32-bit ARGB/XRGB 8888) */
#define COLOR_RGB(r, g, b)       (((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))
#define COLOR_ARGB(a, r, g, b)   (((uint32_t)(a) << 24) | ((uint32_t)(r) << 16) | ((uint32_t)(g) << 8) | (uint32_t)(b))

/* Modern Sleek Windows Color Palette */
#define COLOR_BG_DARK            0x000D1117   /* Deep Charcoal Mica */
#define COLOR_BG_CARD            0x00161B22   /* Acrylic Card Background */
#define COLOR_BG_CARD_HOVER      0x0021262D   /* Card Hover state */
#define COLOR_ACCENT_BLUE        0x000078D4   /* Fluent Design Blue */
#define COLOR_ACCENT_HOVER       0x001084E3   /* Fluent Light Accent */
#define COLOR_ACCENT_BORDER      0x002D333B   /* Acrylic Subtle Border */
#define COLOR_TEXT_PRIMARY       0x00F0F6FC   /* Pure White-Silver Text */
#define COLOR_TEXT_SECONDARY     0x008B949E   /* Muted Slate Gray Text */
#define COLOR_TEXT_ACCENT        0x0058A6FF   /* Cyan / Fluent Highlight */
#define COLOR_SUCCESS_GREEN      0x002EA043   /* Modern Forest Green */
#define COLOR_WARNING_ORANGE     0x00D29922   /* Warm Amber */

/* Framebuffer Context */
typedef struct {
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    uint32_t *front_buffer;
    uint32_t *back_buffer;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint64_t buffer_size;
    XenithraPixelFormat pixel_format;
} GopContext;

extern GopContext g_gop_ctx;

/* GOP Lifecycle */
EFI_STATUS gop_init(EFI_SYSTEM_TABLE *SystemTable);
XenithraFrameBuffer gop_get_framebuffer_info(void);

/* Rendering Primitives */
void gop_clear(uint32_t color);
void gop_put_pixel(int x, int y, uint32_t color);
void gop_fill_rect(int x, int y, int w, int h, uint32_t color);
void gop_draw_rect(int x, int y, int w, int h, uint32_t color);
void gop_fill_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
void gop_draw_gradient_v(int x, int y, int w, int h, uint32_t color_top, uint32_t color_bottom);

/* Typography */
void gop_draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color, int transparent_bg, int scale);
void gop_draw_string(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color, int transparent_bg, int scale);
void gop_draw_string_centered(int y, const char *str, uint32_t fg_color, int scale);
int  gop_get_string_width(const char *str, int scale);

/* Buffer Presentation */
void gop_swap_buffers(void);

#endif /* _BOOTLOADER_GOP_H_ */
