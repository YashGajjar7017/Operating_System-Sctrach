/**
 * @file gop.c
 * @brief Double-buffered GOP software rendering implementation
 */

#include "gop.h"
#include "font.h"

GopContext g_gop_ctx = {0};

/* Initialize GOP and set optimal resolution */
EFI_STATUS gop_init(EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;

    status = SystemTable->BootServices->LocateProtocol(&gop_guid, NULL, (VOID**)&gop);
    if (EFI_ERROR(status) || !gop) {
        return status;
    }

    g_gop_ctx.gop = gop;

    /* Find best video mode (Target: 1920x1080 > 1280x720 > 1024x768 > Highest Mode) */
    UINT32 best_mode = gop->Mode->Mode;
    UINT32 target_w = 1280, target_h = 720;
    UINT32 max_w = 0, max_h = 0;

    for (UINT32 i = 0; i < gop->Mode->MaxMode; i++) {
        UINTN info_size = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = NULL;

        status = gop->QueryMode(gop, i, &info_size, &info);
        if (EFI_ERROR(status) || !info) continue;

        if (info->HorizontalResolution == target_w && info->VerticalResolution == target_h) {
            best_mode = i;
            break;
        }

        if (info->HorizontalResolution >= max_w && info->VerticalResolution >= max_h) {
            max_w = info->HorizontalResolution;
            max_h = info->VerticalResolution;
            best_mode = i;
        }

        if (info) {
            SystemTable->BootServices->FreePool(info);
        }
    }

    /* Set chosen mode */
    if (best_mode != gop->Mode->Mode) {
        gop->SetMode(gop, best_mode);
    }

    g_gop_ctx.front_buffer = (uint32_t*)gop->Mode->FrameBufferBase;
    g_gop_ctx.width = gop->Mode->Info->HorizontalResolution;
    g_gop_ctx.height = gop->Mode->Info->VerticalResolution;
    g_gop_ctx.stride = gop->Mode->Info->PixelsPerScanLine;
    g_gop_ctx.buffer_size = gop->Mode->FrameBufferSize;

    switch (gop->Mode->Info->PixelFormat) {
        case PixelRedGreenBlueReserved8BitPerColor:
            g_gop_ctx.pixel_format = PIXEL_RGBX_8888;
            break;
        case PixelBlueGreenRedReserved8BitPerColor:
            g_gop_ctx.pixel_format = PIXEL_BGRX_8888;
            break;
        case PixelBitMask:
            g_gop_ctx.pixel_format = PIXEL_BITMASK;
            break;
        default:
            g_gop_ctx.pixel_format = PIXEL_BLT_ONLY;
            break;
    }

    /* Allocate backbuffer memory */
    UINTN pages = (g_gop_ctx.buffer_size + 0xFFF) / 0x1000;
    EFI_PHYSICAL_ADDRESS back_buf_addr = 0;
    status = SystemTable->BootServices->AllocatePages(
        AllocateAnyPages,
        EfiLoaderData,
        pages,
        &back_buf_addr
    );

    if (EFI_ERROR(status)) {
        /* Fallback: use front buffer directly if allocation fails */
        g_gop_ctx.back_buffer = g_gop_ctx.front_buffer;
    } else {
        g_gop_ctx.back_buffer = (uint32_t*)back_buf_addr;
        SystemTable->BootServices->SetMem(g_gop_ctx.back_buffer, g_gop_ctx.buffer_size, 0);
    }

    return EFI_SUCCESS;
}

AuraFrameBuffer gop_get_framebuffer_info(void) {
    AuraFrameBuffer fb;
    fb.base_address = (uint64_t)g_gop_ctx.front_buffer;
    fb.buffer_size = g_gop_ctx.buffer_size;
    fb.width = g_gop_ctx.width;
    fb.height = g_gop_ctx.height;
    fb.pixels_per_scanline = g_gop_ctx.stride;
    fb.pixel_format = (uint32_t)g_gop_ctx.pixel_format;
    return fb;
}

void gop_clear(uint32_t color) {
    uint32_t total_pixels = g_gop_ctx.stride * g_gop_ctx.height;
    uint32_t *dst = g_gop_ctx.back_buffer;
    for (uint32_t i = 0; i < total_pixels; i++) {
        dst[i] = color;
    }
}

void gop_put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || (uint32_t)x >= g_gop_ctx.width || y < 0 || (uint32_t)y >= g_gop_ctx.height) {
        return;
    }
    g_gop_ctx.back_buffer[y * g_gop_ctx.stride + x] = color;
}

void gop_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)g_gop_ctx.width)  w = (int)g_gop_ctx.width - x;
    if (y + h > (int)g_gop_ctx.height) h = (int)g_gop_ctx.height - y;
    if (w <= 0 || h <= 0) return;

    for (int row = y; row < y + h; row++) {
        uint32_t *line = &g_gop_ctx.back_buffer[row * g_gop_ctx.stride + x];
        for (int col = 0; col < w; col++) {
            line[col] = color;
        }
    }
}

void gop_draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    gop_fill_rect(x, y, w, 1, color);
    gop_fill_rect(x, y + h - 1, w, 1, color);
    gop_fill_rect(x, y, 1, h, color);
    gop_fill_rect(x + w - 1, y, 1, h, color);
}

void gop_fill_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    if (radius <= 0) {
        gop_fill_rect(x, y, w, h, color);
        return;
    }
    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;

    /* Fill central rectangular blocks */
    gop_fill_rect(x + radius, y, w - 2 * radius, h, color);
    gop_fill_rect(x, y + radius, radius, h - 2 * radius, color);
    gop_fill_rect(x + w - radius, y + radius, radius, h - 2 * radius, color);

    /* Fill corners using circle equation */
    int r2 = radius * radius;
    for (int dy = 0; dy < radius; dy++) {
        for (int dx = 0; dx < radius; dx++) {
            int cx = radius - 1 - dx;
            int cy = radius - 1 - dy;
            if (cx * cx + cy * cy <= r2) {
                /* Top-Left */
                gop_put_pixel(x + dx, y + dy, color);
                /* Top-Right */
                gop_put_pixel(x + w - 1 - dx, y + dy, color);
                /* Bottom-Left */
                gop_put_pixel(x + dx, y + h - 1 - dy, color);
                /* Bottom-Right */
                gop_put_pixel(x + w - 1 - dx, y + h - 1 - dy, color);
            }
        }
    }
}

static inline uint32_t blend_channel(uint32_t c1, uint32_t c2, int ratio, int max_ratio) {
    return c1 + ((c2 - c1) * ratio) / max_ratio;
}

void gop_draw_gradient_v(int x, int y, int w, int h, uint32_t color_top, uint32_t color_bottom) {
    if (h <= 0 || w <= 0) return;

    uint32_t r1 = (color_top >> 16) & 0xFF;
    uint32_t g1 = (color_top >> 8) & 0xFF;
    uint32_t b1 = color_top & 0xFF;

    uint32_t r2 = (color_bottom >> 16) & 0xFF;
    uint32_t g2 = (color_bottom >> 8) & 0xFF;
    uint32_t b2 = color_bottom & 0xFF;

    for (int dy = 0; dy < h; dy++) {
        uint32_t r = blend_channel(r1, r2, dy, h);
        uint32_t g = blend_channel(g1, g2, dy, h);
        uint32_t b = blend_channel(b1, b2, dy, h);
        uint32_t current_color = COLOR_RGB(r, g, b);

        int draw_y = y + dy;
        if (draw_y < 0 || (uint32_t)draw_y >= g_gop_ctx.height) continue;

        int start_x = (x < 0) ? 0 : x;
        int end_x = (x + w > (int)g_gop_ctx.width) ? (int)g_gop_ctx.width : x + w;

        uint32_t *line = &g_gop_ctx.back_buffer[draw_y * g_gop_ctx.stride + start_x];
        for (int dx = 0; dx < (end_x - start_x); dx++) {
            line[dx] = current_color;
        }
    }
}

void gop_draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color, int transparent_bg, int scale) {
    if (c < 32 || c > 126) c = '?';
    if (scale <= 0) scale = 1;

    const uint8_t *glyph = font_8x16[(int)c - 32];

    for (int row = 0; row < 16; row++) {
        uint8_t row_bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            int bit = (row_bits >> (7 - col)) & 1;
            uint32_t px_color = bit ? fg_color : bg_color;

            if (bit || !transparent_bg) {
                if (scale == 1) {
                    gop_put_pixel(x + col, y + row, px_color);
                } else {
                    gop_fill_rect(x + col * scale, y + row * scale, scale, scale, px_color);
                }
            }
        }
    }
}

void gop_draw_string(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color, int transparent_bg, int scale) {
    if (!str) return;
    if (scale <= 0) scale = 1;

    int cur_x = x;
    int cur_y = y;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cur_x = x;
            cur_y += 16 * scale + 4;
            continue;
        }
        gop_draw_char(cur_x, cur_y, str[i], fg_color, bg_color, transparent_bg, scale);
        cur_x += 8 * scale;
    }
}

int gop_get_string_width(const char *str, int scale) {
    if (!str) return 0;
    if (scale <= 0) scale = 1;

    int max_len = 0;
    int cur_len = 0;

    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            if (cur_len > max_len) max_len = cur_len;
            cur_len = 0;
        } else {
            cur_len++;
        }
    }
    if (cur_len > max_len) max_len = cur_len;
    return max_len * 8 * scale;
}

void gop_draw_string_centered(int y, const char *str, uint32_t fg_color, int scale) {
    int str_width = gop_get_string_width(str, scale);
    int start_x = ((int)g_gop_ctx.width - str_width) / 2;
    gop_draw_string(start_x, y, str, fg_color, 0, 1, scale);
}

void gop_swap_buffers(void) {
    if (g_gop_ctx.back_buffer == g_gop_ctx.front_buffer) {
        return;
    }

    uint64_t *src = (uint64_t*)g_gop_ctx.back_buffer;
    uint64_t *dst = (uint64_t*)g_gop_ctx.front_buffer;
    uint64_t count = g_gop_ctx.buffer_size / sizeof(uint64_t);

    for (uint64_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}
