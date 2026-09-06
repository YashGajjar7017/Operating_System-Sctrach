/**
 * @file compositor.c
 * @brief 32-bit Window Compositor & Desktop Environment Implementation
 */

#include "compositor.h"
#include "dom_engine.h"
#include "../../shared/font.h"

static XenithraFrameBuffer g_fb;
static Window g_windows[MAX_WINDOWS] = {0};
static int g_window_count = 0;
static uint32_t g_next_win_id = 1;
static MouseState g_mouse = {100, 100, 0, 0};
static uint8_t g_start_menu_open = 0;

/* Mouse Cursor Bitmap (12x18) */
static const uint16_t cursor_bitmap[18] = {
    0b1000000000000000,
    0b1100000000000000,
    0b1110000000000000,
    0b1111000000000000,
    0b1111100000000000,
    0b1111110000000000,
    0b1111111000000000,
    0b1111111100000000,
    0b1111111110000000,
    0b1111110000000000,
    0b1101111000000000,
    0b1000111100000000,
    0b0000011110000000,
    0b0000011110000000,
    0b0000001111000000,
    0b0000001111000000,
    0b0000000110000000,
    0b0000000000000000
};

static void str_copy(char *dest, const char *src, size_t max) {
    size_t i = 0;
    while (i + 1 < max && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void compositor_init(XenithraFrameBuffer fb) {
    g_fb = fb;
    g_window_count = 0;
    g_start_menu_open = 0;
    g_mouse.x = fb.width / 2;
    g_mouse.y = fb.height / 2;
    g_mouse.left_button = 0;
    g_mouse.right_button = 0;

    for (int i = 0; i < MAX_WINDOWS; i++) {
        g_windows[i].id = 0;
    }
}

/* Framebuffer Primitives */
void gui_put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || (uint32_t)x >= g_fb.width || y < 0 || (uint32_t)y >= g_fb.height) {
        return;
    }
    uint32_t *dst = (uint32_t*)g_fb.base_address;
    dst[y * g_fb.pixels_per_scanline + x] = color;
}

void gui_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)g_fb.width)  w = (int)g_fb.width - x;
    if (y + h > (int)g_fb.height) h = (int)g_fb.height - y;
    if (w <= 0 || h <= 0) return;

    uint32_t *dst = (uint32_t*)g_fb.base_address;
    uint32_t stride = g_fb.pixels_per_scanline;

    for (int r = y; r < y + h; r++) {
        uint32_t *line = &dst[r * stride + x];
        for (int c = 0; c < w; c++) {
            line[c] = color;
        }
    }
}

void gui_draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    gui_fill_rect(x, y, w, 1, color);
    gui_fill_rect(x, y + h - 1, w, 1, color);
    gui_fill_rect(x, y, 1, h, color);
    gui_fill_rect(x + w - 1, y, 1, h, color);
}

void gui_fill_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    if (radius <= 0) {
        gui_fill_rect(x, y, w, h, color);
        return;
    }
    if (radius * 2 > w) radius = w / 2;
    if (radius * 2 > h) radius = h / 2;

    gui_fill_rect(x + radius, y, w - 2 * radius, h, color);
    gui_fill_rect(x, y + radius, radius, h - 2 * radius, color);
    gui_fill_rect(x + w - radius, y + radius, radius, h - 2 * radius, color);

    int r2 = radius * radius;
    for (int dy = 0; dy < radius; dy++) {
        for (int dx = 0; dx < radius; dx++) {
            int cx = radius - 1 - dx;
            int cy = radius - 1 - dy;
            if (cx * cx + cy * cy <= r2) {
                gui_put_pixel(x + dx, y + dy, color);
                gui_put_pixel(x + w - 1 - dx, y + dy, color);
                gui_put_pixel(x + dx, y + h - 1 - dy, color);
                gui_put_pixel(x + w - 1 - dx, y + h - 1 - dy, color);
            }
        }
    }
}

static inline uint32_t blend(uint32_t c1, uint32_t c2, int num, int den) {
    return c1 + ((c2 - c1) * num) / den;
}

void gui_draw_gradient_v(int x, int y, int w, int h, uint32_t top_color, uint32_t bot_color) {
    if (h <= 0 || w <= 0) return;
    uint32_t r1 = (top_color >> 16) & 0xFF, g1 = (top_color >> 8) & 0xFF, b1 = top_color & 0xFF;
    uint32_t r2 = (bot_color >> 16) & 0xFF, g2 = (bot_color >> 8) & 0xFF, b2 = bot_color & 0xFF;

    for (int dy = 0; dy < h; dy++) {
        uint32_t r = blend(r1, r2, dy, h);
        uint32_t g = blend(g1, g2, dy, h);
        uint32_t b = blend(b1, b2, dy, h);
        gui_fill_rect(x, y + dy, w, 1, (r << 16) | (g << 8) | b);
    }
}

void gui_draw_char(int x, int y, char c, uint32_t color, int scale) {
    if (c < 32 || c > 126) c = '?';
    if (scale <= 0) scale = 1;

    const uint8_t *glyph = font_8x16[(int)c - 32];
    for (int row = 0; row < 16; row++) {
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if ((bits >> (7 - col)) & 1) {
                if (scale == 1) {
                    gui_put_pixel(x + col, y + row, color);
                } else {
                    gui_fill_rect(x + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
    }
}

void gui_draw_string(int x, int y, const char *str, uint32_t color, int scale) {
    if (!str) return;
    int cur_x = x;
    int cur_y = y;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] == '\n') {
            cur_x = x;
            cur_y += 16 * scale + 4;
            continue;
        }
        gui_draw_char(cur_x, cur_y, str[i], color, scale);
        cur_x += 8 * scale;
    }
}

/* Window Management */
Window* window_create(const char *title, int x, int y, int w, int h, WindowPaintCallback on_paint, void *user_data) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id == 0) {
            Window *win = &g_windows[i];
            win->id = g_next_win_id++;
            str_copy(win->title, title, sizeof(win->title));
            win->x = x;
            win->y = y;
            win->width = w;
            win->height = h;
            win->is_minimized = 0;
            win->is_maximized = 0;
            win->is_focused = 1;
            win->saved_x = x;
            win->saved_y = y;
            win->saved_w = w;
            win->saved_h = h;
            win->z_order = g_window_count++;
            win->on_paint = on_paint;
            win->user_data = user_data;
            window_focus(win);
            return win;
        }
    }
    return NULL;
}

void window_destroy(uint32_t win_id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id == win_id) {
            g_windows[i].id = 0;
            if (g_window_count > 0) g_window_count--;
            return;
        }
    }
}

void window_focus(Window *win) {
    if (!win) return;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id != 0) {
            g_windows[i].is_focused = (g_windows[i].id == win->id);
        }
    }
}

void window_maximize(Window *win) {
    if (!win) return;
    if (!win->is_maximized) {
        win->saved_x = win->x;
        win->saved_y = win->y;
        win->saved_w = win->width;
        win->saved_h = win->height;
        win->x = 0;
        win->y = 0;
        win->width = g_fb.width;
        win->height = g_fb.height - TASKBAR_HEIGHT;
        win->is_maximized = 1;
    } else {
        win->x = win->saved_x;
        win->y = win->saved_y;
        win->width = win->saved_w;
        win->height = win->saved_h;
        win->is_maximized = 0;
    }
}

void window_minimize(Window *win) {
    if (win) win->is_minimized = 1;
}

void window_restore(Window *win) {
    if (win) {
        win->is_minimized = 0;
        window_focus(win);
    }
}

void window_move(Window *win, int new_x, int new_y) {
    if (win && !win->is_maximized) {
        win->x = new_x;
        win->y = new_y;
    }
}

void compositor_set_mouse(int x, int y, uint8_t left_btn, uint8_t right_btn) {
    g_mouse.x = x;
    g_mouse.y = y;
    g_mouse.left_button = left_btn;
    g_mouse.right_button = right_btn;
}

void compositor_toggle_start_menu(void) {
    g_start_menu_open = !g_start_menu_open;
}

/* Rendering Pipeline */
static void render_wallpaper(void) {
    gui_draw_gradient_v(0, 0, g_fb.width, g_fb.height - TASKBAR_HEIGHT, 0x000F172A, 0x001E293B);

    /* Desktop Watermark & Brand */
    gui_draw_string(g_fb.width - 240, g_fb.height - TASKBAR_HEIGHT - 36, "Xenithra OS v1.0", 0x00334155, 2);
    gui_draw_string(g_fb.width - 240, g_fb.height - TASKBAR_HEIGHT - 16, "High-Security Microkernel Core", 0x00334155, 1);
}

static void render_window(Window *win) {
    if (!win || win->id == 0 || win->is_minimized) return;

    int wx = win->x;
    int wy = win->y;
    int ww = win->width;
    int wh = win->height;

    /* 1. Window Body & Border */
    gui_fill_rounded_rect(wx, wy, ww, wh, 8, GUI_BG_WINDOW);
    gui_draw_rect(wx, wy, ww, wh, win->is_focused ? GUI_ACCENT_BLUE : GUI_BORDER_COLOR);

    /* 2. Window Titlebar */
    uint32_t tb_color = win->is_focused ? 0x000F172A : 0x001E293B;
    gui_fill_rounded_rect(wx, wy, ww, TITLEBAR_HEIGHT, 8, tb_color);
    gui_fill_rect(wx, wy + TITLEBAR_HEIGHT - 4, ww, 4, tb_color); /* Square bottom corners of titlebar */
    gui_draw_rect(wx, wy, ww, TITLEBAR_HEIGHT, GUI_BORDER_COLOR);

    /* Window Icon & Title */
    gui_fill_rounded_rect(wx + 10, wy + 8, 16, 16, 3, GUI_ACCENT_BLUE);
    gui_draw_string(wx + 34, wy + 8, win->title, win->is_focused ? GUI_TEXT_WHITE : GUI_TEXT_MUTED, 1);

    /* Window Control Buttons: [-] [+] [X] */
    int btn_size = 18;
    int btn_y = wy + 7;
    int btn_close_x = wx + ww - 26;
    int btn_max_x   = wx + ww - 50;
    int btn_min_x   = wx + ww - 74;

    /* Close Button [X] */
    gui_fill_rounded_rect(btn_close_x, btn_y, btn_size, btn_size, 4, GUI_ACCENT_RED);
    gui_draw_string(btn_close_x + 5, btn_y + 2, "X", 0x00FFFFFF, 1);

    /* Maximize Button [+] */
    gui_fill_rounded_rect(btn_max_x, btn_y, btn_size, btn_size, 4, 0x00334155);
    gui_draw_string(btn_max_x + 5, btn_y + 2, "+", GUI_TEXT_WHITE, 1);

    /* Minimize Button [-] */
    gui_fill_rounded_rect(btn_min_x, btn_y, btn_size, btn_size, 4, 0x00334155);
    gui_draw_string(btn_min_x + 5, btn_y + 2, "-", GUI_TEXT_WHITE, 1);

    /* 3. Window Content Callback */
    if (win->on_paint) {
        win->on_paint(win, wx + 8, wy + TITLEBAR_HEIGHT + 8, ww - 16, wh - TITLEBAR_HEIGHT - 16);
    }
}

static void render_taskbar(void) {
    int ty = g_fb.height - TASKBAR_HEIGHT;
    gui_fill_rect(0, ty, g_fb.width, TASKBAR_HEIGHT, 0x000F172A);
    gui_fill_rect(0, ty, g_fb.width, 1, GUI_BORDER_COLOR);

    /* 1. Start Button (Windows 11 Fluent 4-tile style) */
    int start_x = 16;
    int start_y = ty + 8;
    gui_fill_rounded_rect(start_x, start_y, 36, 32, 6, g_start_menu_open ? GUI_ACCENT_BLUE : 0x001E293B);
    gui_draw_rect(start_x, start_y, 36, 32, GUI_BORDER_COLOR);

    /* 4 mini tiles */
    gui_fill_rect(start_x + 10, start_y + 8, 6, 6, 0x0058A6FF);
    gui_fill_rect(start_x + 19, start_y + 8, 6, 6, 0x0079C0FF);
    gui_fill_rect(start_x + 10, start_y + 17, 6, 6, 0x00388BFD);
    gui_fill_rect(start_x + 19, start_y + 17, 6, 6, 0x0058A6FF);

    /* 2. Open Window Task Pills */
    int pill_x = start_x + 48;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id != 0) {
            Window *w = &g_windows[i];
            int pill_w = 160;
            uint32_t bg = w->is_focused ? GUI_ACCENT_BLUE : 0x001E293B;

            gui_fill_rounded_rect(pill_x, start_y, pill_w, 32, 6, bg);
            gui_draw_rect(pill_x, start_y, pill_w, 32, GUI_BORDER_COLOR);
            gui_draw_string(pill_x + 10, start_y + 8, w->title, GUI_TEXT_WHITE, 1);
            pill_x += pill_w + 8;
        }
    }

    /* 3. System Tray (Clock & Security Shield) */
    int tray_x = g_fb.width - 220;
    /* Security Shield Badge */
    gui_fill_rounded_rect(tray_x, start_y, 90, 32, 6, 0x0010B981);
    gui_draw_string(tray_x + 8, start_y + 8, "[Secured]", 0x00FFFFFF, 1);

    /* System Clock */
    gui_fill_rounded_rect(tray_x + 98, start_y, 106, 32, 6, 0x001E293B);
    gui_draw_rect(tray_x + 98, start_y, 106, 32, GUI_BORDER_COLOR);
    gui_draw_string(tray_x + 112, start_y + 8, "12:00 PM", GUI_TEXT_WHITE, 1);
}

static void render_start_menu(void) {
    if (!g_start_menu_open) return;

    int sm_w = 340;
    int sm_h = 380;
    int sm_x = 16;
    int sm_y = g_fb.height - TASKBAR_HEIGHT - sm_h - 10;

    /* Start Menu Background */
    gui_fill_rounded_rect(sm_x, sm_y, sm_w, sm_h, 12, 0x001E293B);
    gui_draw_rect(sm_x, sm_y, sm_w, sm_h, GUI_ACCENT_BLUE);

    /* Header */
    gui_draw_string(sm_x + 20, sm_y + 20, "Xenithra OS Core", GUI_TEXT_WHITE, 2);
    gui_draw_string(sm_x + 20, sm_y + 54, "Protected Workstation Environment", GUI_TEXT_MUTED, 1);

    /* Menu Items */
    const char *apps[] = {
        "1. Xenithra Private Firewall Monitor",
        "2. Anti-Hijack Security Center",
        "3. System Hardware Diagnostics",
        "4. Secure Terminal / Shell",
        "5. Lock Session & Reboot"
    };

    for (int i = 0; i < 5; i++) {
        int item_y = sm_y + 90 + i * 48;
        gui_fill_rounded_rect(sm_x + 16, item_y, sm_w - 32, 40, 6, 0x000F172A);
        gui_draw_rect(sm_x + 16, item_y, sm_w - 32, 40, GUI_BORDER_COLOR);
        gui_draw_string(sm_x + 28, item_y + 12, apps[i], (i == 0 || i == 1) ? GUI_ACCENT_CYAN : GUI_TEXT_WHITE, 1);
    }
}

static void render_mouse_cursor(void) {
    int mx = g_mouse.x;
    int my = g_mouse.y;

    for (int row = 0; row < 18; row++) {
        uint16_t bits = cursor_bitmap[row];
        for (int col = 0; col < 12; col++) {
            if ((bits >> (15 - col)) & 1) {
                gui_put_pixel(mx + col, my + row, 0x00FFFFFF); /* White pointer */
            }
        }
    }
}

void compositor_render(void) {
    /* 1. Wallpaper */
    render_wallpaper();

    /* 2. All Windows */
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id != 0) {
            render_window(&g_windows[i]);
        }
    }

    /* 3. Taskbar */
    render_taskbar();

    /* 4. Start Menu popup (if active) */
    render_start_menu();

    /* 5. Mouse Cursor */
    render_mouse_cursor();
}
