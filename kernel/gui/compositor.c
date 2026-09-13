/**
 * @file compositor.c
 * @brief Windows 11 Fluent 32-bit Window Compositor & Desktop Environment
 */

#include "compositor.h"
#include "../kstring.h"
#include "../../shared/font.h"
#include "../apps/explorer_app.h"
#include "../apps/taskmgr_app.h"
#include "../apps/firewall_app.h"
#include "../apps/terminal_app.h"

static XenithraFrameBuffer g_fb;
static uint32_t *g_back_buffer = NULL;
static uint32_t g_back_buffer_storage[1920 * 1080]; /* Static allocation fallback up to 1080p */

static Window g_windows[MAX_WINDOWS] = {0};
static int g_window_count = 0;
static uint32_t g_next_win_id = 1;

static CompositorMouseState g_mouse = {640, 360, 0, 0, 0, 0, 0};
static uint8_t g_start_menu_open = 0;
static uint8_t g_calendar_open = 0;
static uint8_t g_volume_open = 0;
static int g_volume_level = 75;

/* Window Dragging State */
static Window *g_drag_window = NULL;
static int g_drag_offset_x = 0;
static int g_drag_offset_y = 0;

/* Desktop Icon Selection */
static int g_selected_desktop_icon = -1;
static uint64_t g_last_icon_click_tick = 0;
static uint64_t g_system_ticks = 0;

/* Desktop Icons Definition */
static void on_launch_this_pc(void)    { explorer_app_launch(); }
static void on_launch_explorer(void)   { explorer_app_launch(); }
static void on_launch_taskmgr(void)    { taskmgr_app_launch(); }
static void on_launch_firewall(void)   { firewall_app_launch(); }
static void on_launch_terminal(void)   { terminal_app_launch(); }
static void on_launch_recycle(void)    { explorer_app_launch(); }

static DesktopIcon g_desktop_icons[MAX_DESKTOP_ICONS] = {
    {"This PC",         "PC", GUI_ACCENT_BLUE,  on_launch_this_pc},
    {"File Explorer",   "EX", 0x00D97706,       on_launch_explorer},
    {"Task Manager",    "TM", GUI_ACCENT_CYAN,  on_launch_taskmgr},
    {"Security Center", "SC", GUI_ACCENT_GREEN, on_launch_firewall},
    {"Terminal Shell",  "CL", 0x00A855F7,       on_launch_terminal},
    {"Recycle Bin",     "RB", 0x0064748B,       on_launch_recycle}
};

/* Modern Sleek Mouse Cursor Bitmap (12x18) */
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

static const uint16_t cursor_shadow[18] = {
    0b1100000000000000,
    0b1110000000000000,
    0b1111000000000000,
    0b1111100000000000,
    0b1111110000000000,
    0b1111111000000000,
    0b1111111100000000,
    0b1111111110000000,
    0b1111111111000000,
    0b1111111100000000,
    0b1111111110000000,
    0b1100111110000000,
    0b1000011111000000,
    0b0000011111000000,
    0b0000001111100000,
    0b0000001111100000,
    0b0000000111000000,
    0b0000000011000000
};

void compositor_init(XenithraFrameBuffer fb) {
    g_fb = fb;
    g_back_buffer = g_back_buffer_storage;
    g_window_count = 0;
    g_start_menu_open = 0;
    g_calendar_open = 0;
    g_volume_open = 0;
    g_mouse.x = fb.width / 2;
    g_mouse.y = fb.height / 2;
    g_mouse.left_button = 0;
    g_mouse.right_button = 0;
    g_mouse.middle_button = 0;
    g_mouse.prev_left = 0;
    g_mouse.prev_right = 0;

    for (int i = 0; i < MAX_WINDOWS; i++) {
        g_windows[i].id = 0;
    }
}

void gui_put_pixel(int x, int y, uint32_t color) {
    if (x < 0 || (uint32_t)x >= g_fb.width || y < 0 || (uint32_t)y >= g_fb.height) {
        return;
    }
    g_back_buffer[y * g_fb.pixels_per_scanline + x] = color;
}

void gui_fill_rect(int x, int y, int w, int h, uint32_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > (int)g_fb.width)  w = (int)g_fb.width - x;
    if (y + h > (int)g_fb.height) h = (int)g_fb.height - y;
    if (w <= 0 || h <= 0) return;

    uint32_t stride = g_fb.pixels_per_scanline;
    for (int r = y; r < y + h; r++) {
        uint32_t *line = &g_back_buffer[r * stride + x];
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
    uint32_t r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    uint32_t r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    uint32_t r = r1 + ((r2 - r1) * num) / den;
    uint32_t g = g1 + ((g2 - g1) * num) / den;
    uint32_t b = b1 + ((b2 - b1) * num) / den;
    return (r << 16) | (g << 8) | b;
}

void gui_draw_gradient_v(int x, int y, int w, int h, uint32_t top_color, uint32_t bot_color) {
    if (h <= 0 || w <= 0) return;
    for (int dy = 0; dy < h; dy++) {
        uint32_t c = blend(top_color, bot_color, dy, h);
        gui_fill_rect(x, y + dy, w, 1, c);
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

void gui_draw_string_shadow(int x, int y, const char *str, uint32_t color, uint32_t shadow_color, int scale) {
    gui_draw_string(x + 1, y + 1, str, shadow_color, scale);
    gui_draw_string(x, y, str, color, scale);
}

void gui_draw_icon_badge(int x, int y, const char *symbol, uint32_t bg_color, uint32_t fg_color) {
    gui_fill_rounded_rect(x, y, 20, 20, 4, bg_color);
    gui_draw_string(x + 4, y + 2, symbol, fg_color, 1);
}

/* Window Management Implementation */
Window* window_create(const char *title, const char *app_tag, int x, int y, int w, int h, WindowPaintCallback on_paint, void *user_data) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id == 0) {
            Window *win = &g_windows[i];
            win->id = g_next_win_id++;
            strncpy(win->title, title, sizeof(win->title) - 1);
            strncpy(win->app_tag, app_tag ? app_tag : "app", sizeof(win->app_tag) - 1);
            win->x = x;
            win->y = y;
            win->width = w;
            win->height = h;
            win->is_minimized = 0;
            win->is_maximized = 0;
            win->is_focused = 1;
            win->is_visible = 1;
            win->saved_x = x;
            win->saved_y = y;
            win->saved_w = w;
            win->saved_h = h;
            win->z_order = g_window_count++;
            win->on_paint = on_paint;
            win->on_key = NULL;
            win->on_mouse = NULL;
            win->user_data = user_data;
            window_focus(win);
            return win;
        }
    }
    return NULL;
}

void window_set_callbacks(Window *win, WindowKeyCallback on_key, WindowMouseCallback on_mouse) {
    if (win) {
        win->on_key = on_key;
        win->on_mouse = on_mouse;
    }
}

void window_destroy(uint32_t win_id) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id == win_id) {
            g_windows[i].id = 0;
            g_windows[i].is_visible = 0;
            if (g_window_count > 0) g_window_count--;
            return;
        }
    }
}

void window_focus(Window *win) {
    if (!win) return;
    int max_z = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id != 0) {
            if (g_windows[i].z_order > max_z) max_z = g_windows[i].z_order;
            g_windows[i].is_focused = 0;
        }
    }
    win->is_focused = 1;
    win->is_minimized = 0;
    win->z_order = max_z + 1;
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
    if (win) {
        win->is_minimized = 1;
        win->is_focused = 0;
    }
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

Window* window_get_by_tag(const char *app_tag) {
    if (!app_tag) return NULL;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id != 0 && strcmp(g_windows[i].app_tag, app_tag) == 0) {
            return &g_windows[i];
        }
    }
    return NULL;
}

Window* window_get_focused(void) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id != 0 && g_windows[i].is_focused && !g_windows[i].is_minimized) {
            return &g_windows[i];
        }
    }
    return NULL;
}

void compositor_toggle_start_menu(void) {
    g_start_menu_open = !g_start_menu_open;
    if (g_start_menu_open) {
        g_calendar_open = 0;
        g_volume_open = 0;
    }
}

void compositor_toggle_calendar(void) {
    g_calendar_open = !g_calendar_open;
    if (g_calendar_open) {
        g_start_menu_open = 0;
        g_volume_open = 0;
    }
}

void compositor_toggle_volume(void) {
    g_volume_open = !g_volume_open;
    if (g_volume_open) {
        g_start_menu_open = 0;
        g_calendar_open = 0;
    }
}

void compositor_tick(void) {
    g_system_ticks++;
    taskmgr_tick();
}

/* Rendering Functions */
static void render_wallpaper(void) {
    int h = g_fb.height - TASKBAR_HEIGHT;
    int mid = h / 2;

    /* Layered Flow Gradient */
    gui_draw_gradient_v(0, 0, g_fb.width, mid, GUI_BG_WALLPAPER_TOP, GUI_BG_WALLPAPER_MID);
    gui_draw_gradient_v(0, mid, g_fb.width, h - mid, GUI_BG_WALLPAPER_MID, GUI_BG_WALLPAPER_BOT);

    /* Subtle Modern Bloom Glow */
    int glow_cx = g_fb.width / 2;
    int glow_cy = h / 2;
    for (int r = 180; r > 0; r -= 20) {
        uint32_t glow_color = blend(0x001D3557, 0x0014223D, r, 180);
        gui_fill_rounded_rect(glow_cx - r * 2, glow_cy - r, r * 4, r * 2, r, glow_color);
    }

    /* Windows 11 Fluent Center Watermark */
    gui_draw_string_shadow(g_fb.width / 2 - 140, h / 2 - 20, "Xenithra OS", 0x003A4E72, 0x000F172A, 3);
    gui_draw_string(g_fb.width / 2 - 160, h / 2 + 25, "64-bit Microkernel Protected Workstation", 0x002A3C5A, 1);
}

static void render_desktop_icons(void) {
    int start_x = 24;
    int start_y = 28;
    int icon_h = 76;
    int icon_w = 84;

    for (int i = 0; i < MAX_DESKTOP_ICONS; i++) {
        int ix = start_x;
        int iy = start_y + i * icon_h;

        /* Highlight box on select */
        if (g_selected_desktop_icon == i) {
            gui_fill_rounded_rect(ix - 6, iy - 6, icon_w, icon_h - 4, 6, 0x002D3B55);
            gui_draw_rect(ix - 6, iy - 6, icon_w, icon_h - 4, GUI_ACCENT_BLUE);
        }

        /* Icon Badge */
        gui_fill_rounded_rect(ix + 16, iy, 40, 40, 8, g_desktop_icons[i].icon_color);
        gui_draw_rect(ix + 16, iy, 40, 40, 0x00FFFFFF);
        gui_draw_string(ix + 26, iy + 12, g_desktop_icons[i].icon_tag, 0x00FFFFFF, 1);

        /* Icon Title */
        gui_draw_string_shadow(ix, iy + 46, g_desktop_icons[i].title, GUI_TEXT_PRIMARY, 0x00000000, 1);
    }
}

static void render_window_frame(Window *win) {
    if (!win || win->id == 0 || win->is_minimized || !win->is_visible) return;

    int wx = win->x;
    int wy = win->y;
    int ww = win->width;
    int wh = win->height;

    /* 1. Window Drop Shadow */
    gui_fill_rounded_rect(wx + 3, wy + 3, ww + 2, wh + 2, 8, 0x00050810);

    /* 2. Window Main Body */
    gui_fill_rounded_rect(wx, wy, ww, wh, 8, GUI_BG_WINDOW);
    gui_draw_rect(wx, wy, ww, wh, win->is_focused ? GUI_BORDER_FOCUS : GUI_BORDER_COLOR);

    /* 3. Titlebar Header */
    uint32_t tb_color = win->is_focused ? GUI_BG_TITLEBAR_ACT : GUI_BG_TITLEBAR_INACT;
    gui_fill_rounded_rect(wx, wy, ww, TITLEBAR_HEIGHT, 8, tb_color);
    gui_fill_rect(wx, wy + TITLEBAR_HEIGHT - 6, ww, 6, tb_color);
    gui_draw_rect(wx, wy, ww, TITLEBAR_HEIGHT, GUI_BORDER_COLOR);

    /* App Icon & Title */
    gui_fill_rounded_rect(wx + 10, wy + 8, 18, 18, 4, win->is_focused ? GUI_ACCENT_BLUE : GUI_BG_CARD);
    gui_draw_string(wx + 14, wy + 10, "*", 0x00FFFFFF, 1);
    gui_draw_string(wx + 36, wy + 9, win->title, win->is_focused ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1);

    /* Window Control Buttons: [-] [□] [✕] */
    int btn_w = 36;
    int btn_h = TITLEBAR_HEIGHT;
    int close_x = wx + ww - 44;
    int max_x   = close_x - btn_w;
    int min_x   = max_x - btn_w;

    /* Minimize button */
    gui_draw_string(min_x + 14, wy + 10, "-", GUI_TEXT_SECONDARY, 1);

    /* Maximize button */
    gui_draw_string(max_x + 12, wy + 9, win->is_maximized ? "r" : "o", GUI_TEXT_SECONDARY, 1);

    /* Close button */
    gui_draw_string(close_x + 16, wy + 9, "X", GUI_TEXT_PRIMARY, 1);

    /* 4. Client Content Area */
    int content_x = wx + 1;
    int content_y = wy + TITLEBAR_HEIGHT;
    int content_w = ww - 2;
    int content_h = wh - TITLEBAR_HEIGHT - 1;

    if (win->on_paint) {
        win->on_paint(win, content_x, content_y, content_w, content_h);
    }
}

static void render_taskbar(void) {
    int ty = g_fb.height - TASKBAR_HEIGHT;
    int tw = g_fb.width;

    /* Taskbar Background & Top Glass Border */
    gui_fill_rect(0, ty, tw, TASKBAR_HEIGHT, GUI_BG_TASKBAR);
    gui_draw_rect(0, ty, tw, 1, GUI_BORDER_COLOR);

    /* 1. Xenithra / Windows Start Button */
    int start_btn_w = 42;
    int start_btn_h = 36;
    int start_btn_x = 12;
    int start_btn_y = ty + 6;

    gui_fill_rounded_rect(start_btn_x, start_btn_y, start_btn_w, start_btn_h, 6, g_start_menu_open ? GUI_BG_CARD_HOVER : GUI_BG_TASKBAR);
    if (g_start_menu_open) {
        gui_draw_rect(start_btn_x, start_btn_y, start_btn_w, start_btn_h, GUI_ACCENT_BLUE);
    }

    /* Windows 4-Square Logo */
    gui_fill_rect(start_btn_x + 12, start_btn_y + 10, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(start_btn_x + 22, start_btn_y + 10, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(start_btn_x + 12, start_btn_y + 19, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(start_btn_x + 22, start_btn_y + 19, 8, 7, GUI_ACCENT_CYAN);

    /* 2. Search Pill */
    int search_x = start_btn_x + start_btn_w + 8;
    int search_w = 200;
    gui_fill_rounded_rect(search_x, ty + 6, search_w, 36, 18, GUI_BG_INPUT);
    gui_draw_rect(search_x, ty + 6, search_w, 36, GUI_BORDER_COLOR);
    gui_draw_string(search_x + 12, ty + 16, "Search apps, files...", GUI_TEXT_MUTED, 1);

    /* 3. Docked Running Application Icons */
    int dock_x = search_x + search_w + 16;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *w = &g_windows[i];
        if (w->id != 0 && w->is_visible) {
            int icon_w = 160;
            int bx = dock_x;
            uint32_t bg = w->is_focused ? GUI_BG_CARD_HOVER : GUI_BG_TASKBAR;

            gui_fill_rounded_rect(bx, ty + 6, icon_w, 36, 6, bg);
            gui_draw_rect(bx, ty + 6, icon_w, 36, w->is_focused ? GUI_ACCENT_BLUE : GUI_BORDER_COLOR);

            /* App Badge */
            gui_fill_rounded_rect(bx + 8, ty + 12, 20, 20, 4, GUI_ACCENT_BLUE);
            gui_draw_string(bx + 12, ty + 14, "*", 0x00FFFFFF, 1);

            /* App Title */
            gui_draw_string(bx + 34, ty + 16, w->title, w->is_focused ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1);

            /* Active Underline Indicator */
            if (!w->is_minimized) {
                int bar_w = w->is_focused ? 40 : 16;
                gui_fill_rounded_rect(bx + (icon_w - bar_w) / 2, ty + TASKBAR_HEIGHT - 3, bar_w, 3, 1, GUI_ACCENT_BLUE);
            }

            dock_x += icon_w + 6;
        }
    }

    /* 4. System Tray (Right Edge) */
    int tray_right = tw - 12;

    /* Desktop Peek bar */
    gui_draw_rect(tray_right - 4, ty + 10, 2, 28, GUI_TEXT_MUTED);

    /* Digital Clock & Date */
    int clock_w = 96;
    int clock_x = tray_right - 8 - clock_w;
    gui_fill_rounded_rect(clock_x, ty + 4, clock_w, 40, 6, g_calendar_open ? GUI_BG_CARD_HOVER : GUI_BG_TASKBAR);
    gui_draw_string(clock_x + 12, ty + 10, "11:04 AM", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(clock_x + 12, ty + 24, "9/13/2026", GUI_TEXT_MUTED, 1);

    /* Security Shield Icon */
    int sec_x = clock_x - 34;
    gui_fill_rounded_rect(sec_x, ty + 8, 28, 30, 4, 0x0010B981);
    gui_draw_string(sec_x + 8, ty + 14, "S", 0x00FFFFFF, 1);

    /* Battery Icon */
    int bat_x = sec_x - 34;
    gui_fill_rounded_rect(bat_x, ty + 12, 24, 22, 3, GUI_BG_CARD);
    gui_draw_rect(bat_x, ty + 12, 24, 22, GUI_BORDER_COLOR);
    gui_fill_rect(bat_x + 2, ty + 14, 18, 18, GUI_ACCENT_GREEN);

    /* Volume Icon */
    int vol_x = bat_x - 34;
    gui_fill_rounded_rect(vol_x, ty + 8, 28, 30, 4, g_volume_open ? GUI_BG_CARD_HOVER : GUI_BG_TASKBAR);
    gui_draw_string(vol_x + 8, ty + 14, "V", GUI_TEXT_PRIMARY, 1);

    /* Network WiFi Icon */
    int net_x = vol_x - 34;
    gui_fill_rounded_rect(net_x, ty + 8, 28, 30, 4, GUI_BG_TASKBAR);
    gui_draw_string(net_x + 8, ty + 14, "N", GUI_ACCENT_CYAN, 1);
}

static void render_start_menu(void) {
    if (!g_start_menu_open) return;

    int sm_w = 460;
    int sm_h = 520;
    int sm_x = 12;
    int sm_y = g_fb.height - TASKBAR_HEIGHT - sm_h - 12;

    /* Start Menu Glass Container */
    gui_fill_rounded_rect(sm_x, sm_y, sm_w, sm_h, 12, GUI_BG_STARTMENU);
    gui_draw_rect(sm_x, sm_y, sm_w, sm_h, GUI_ACCENT_BLUE);

    /* Top Search Box */
    gui_fill_rounded_rect(sm_x + 20, sm_y + 20, sm_w - 40, 36, 18, GUI_BG_INPUT);
    gui_draw_rect(sm_x + 20, sm_y + 20, sm_w - 40, 36, GUI_BORDER_COLOR);
    gui_draw_string(sm_x + 36, sm_y + 30, "Type here to search...", GUI_TEXT_MUTED, 1);

    /* Pinned Section */
    gui_draw_string(sm_x + 24, sm_y + 70, "Pinned", GUI_TEXT_PRIMARY, 1);

    const char *pinned_names[] = {
        "File Explorer", "Task Manager", "Security Center",
        "Terminal",      "Settings",     "Memory Diagnostic"
    };
    uint32_t pinned_colors[] = {
        0x00D97706, GUI_ACCENT_CYAN, GUI_ACCENT_GREEN,
        0x00A855F7, GUI_ACCENT_BLUE, 0x00EC4899
    };

    for (int i = 0; i < 6; i++) {
        int col = i % 3;
        int row = i / 3;
        int px = sm_x + 24 + col * 140;
        int py = sm_y + 96 + row * 76;

        gui_fill_rounded_rect(px, py, 130, 68, 8, GUI_BG_CARD);
        gui_draw_rect(px, py, 130, 68, GUI_BORDER_COLOR);

        /* App Icon Badge */
        gui_fill_rounded_rect(px + 45, py + 10, 32, 32, 6, pinned_colors[i]);
        gui_draw_string(px + 55, py + 18, "*", 0x00FFFFFF, 1);
        gui_draw_string(px + 10, py + 48, pinned_names[i], GUI_TEXT_SECONDARY, 1);
    }

    /* Recommended / Recent Section */
    gui_draw_string(sm_x + 24, sm_y + 270, "Recommended", GUI_TEXT_PRIMARY, 1);
    const char *recent[] = {
        "firewall.rules  -  Security Center",
        "session.vault   -  Encrypted Tokens",
        "kernel.sys      -  Microkernel Core"
    };

    for (int i = 0; i < 3; i++) {
        int ry = sm_y + 296 + i * 40;
        gui_fill_rounded_rect(sm_x + 20, ry, sm_w - 40, 34, 6, GUI_BG_CARD);
        gui_draw_string(sm_x + 36, ry + 10, recent[i], GUI_TEXT_SECONDARY, 1);
    }

    /* User Profile & Power Section */
    int ubar_y = sm_y + sm_h - 54;
    gui_fill_rounded_rect(sm_x, ubar_y, sm_w, 54, 12, 0x000E1422);
    gui_draw_rect(sm_x, ubar_y, sm_w, 1, GUI_BORDER_COLOR);

    /* Avatar */
    gui_fill_rounded_rect(sm_x + 20, ubar_y + 10, 34, 34, 17, GUI_ACCENT_BLUE);
    gui_draw_string(sm_x + 30, ubar_y + 18, "A", 0x00FFFFFF, 1);
    gui_draw_string(sm_x + 64, ubar_y + 14, "Administrator", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(sm_x + 64, ubar_y + 30, "Ring 0 Full Privilege", GUI_TEXT_MUTED, 1);

    /* Power Buttons (Lock, Shutdown) */
    gui_fill_rounded_rect(sm_x + sm_w - 46, ubar_y + 10, 34, 34, 6, GUI_BG_CARD);
    gui_draw_string(sm_x + sm_w - 34, ubar_y + 18, "P", GUI_ACCENT_RED, 1);
}

static void render_calendar_popup(void) {
    if (!g_calendar_open) return;

    int cal_w = 280;
    int cal_h = 300;
    int cal_x = g_fb.width - cal_w - 12;
    int cal_y = g_fb.height - TASKBAR_HEIGHT - cal_h - 12;

    gui_fill_rounded_rect(cal_x, cal_y, cal_w, cal_h, 12, GUI_BG_STARTMENU);
    gui_draw_rect(cal_x, cal_y, cal_w, cal_h, GUI_ACCENT_BLUE);

    gui_draw_string(cal_x + 20, cal_y + 18, "September 2026", GUI_TEXT_PRIMARY, 2);
    gui_draw_string(cal_x + 20, cal_y + 46, "Sunday, September 13", GUI_ACCENT_CYAN, 1);

    /* Calendar Grid */
    const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    for (int d = 0; d < 7; d++) {
        gui_draw_string(cal_x + 20 + d * 36, cal_y + 76, days[d], GUI_TEXT_MUTED, 1);
    }

    /* Date Numbers */
    for (int day = 1; day <= 30; day++) {
        int col = (day + 1) % 7;
        int row = (day + 1) / 7;
        int dx = cal_x + 20 + col * 36;
        int dy = cal_y + 104 + row * 28;

        if (day == 13) {
            gui_fill_rounded_rect(dx - 4, dy - 4, 24, 24, 12, GUI_ACCENT_BLUE);
            gui_draw_string(dx, dy, "13", 0x00FFFFFF, 1);
        } else {
            char num[8];
            uint_to_str(day, num);
            gui_draw_string(dx, dy, num, GUI_TEXT_SECONDARY, 1);
        }
    }
}

static void render_volume_popup(void) {
    if (!g_volume_open) return;

    int vol_w = 260;
    int vol_h = 90;
    int vol_x = g_fb.width - vol_w - 80;
    int vol_y = g_fb.height - TASKBAR_HEIGHT - vol_h - 12;

    gui_fill_rounded_rect(vol_x, vol_y, vol_w, vol_h, 12, GUI_BG_STARTMENU);
    gui_draw_rect(vol_x, vol_y, vol_w, vol_h, GUI_ACCENT_BLUE);

    gui_draw_string(vol_x + 18, vol_y + 16, "Master Audio Output", GUI_TEXT_PRIMARY, 1);

    /* Volume Slider Track */
    gui_fill_rounded_rect(vol_x + 18, vol_y + 44, 170, 8, 4, GUI_BG_INPUT);
    gui_fill_rounded_rect(vol_x + 18, vol_y + 44, (170 * g_volume_level) / 100, 8, 4, GUI_ACCENT_BLUE);
    gui_fill_rounded_rect(vol_x + 14 + (170 * g_volume_level) / 100, vol_y + 38, 16, 20, 8, 0x00FFFFFF);

    char num[8];
    uint_to_str(g_volume_level, num);
    strcat(num, "%");
    gui_draw_string(vol_x + 200, vol_y + 40, num, GUI_TEXT_PRIMARY, 1);
}

static void render_mouse_cursor(void) {
    int mx = g_mouse.x;
    int my = g_mouse.y;

    /* Drop shadow pass */
    for (int row = 0; row < 18; row++) {
        uint16_t bits = cursor_shadow[row];
        for (int col = 0; col < 12; col++) {
            if ((bits >> (15 - col)) & 1) {
                gui_put_pixel(mx + col + 1, my + row + 1, 0x000A0F1A);
            }
        }
    }

    /* Main pointer pass */
    for (int row = 0; row < 18; row++) {
        uint16_t bits = cursor_bitmap[row];
        for (int col = 0; col < 12; col++) {
            if ((bits >> (15 - col)) & 1) {
                gui_put_pixel(mx + col, my + row, 0x00FFFFFF);
            }
        }
    }
}

void compositor_render(void) {
    if (!g_fb.base_address || !g_back_buffer) return;

    /* 1. Desktop Wallpaper */
    render_wallpaper();

    /* 2. Desktop Shortcuts */
    render_desktop_icons();

    /* 3. Render Windows in Z-Order (from lowest z to highest z) */
    for (int z = 0; z < g_window_count + 10; z++) {
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (g_windows[i].id != 0 && g_windows[i].z_order == z) {
                render_window_frame(&g_windows[i]);
            }
        }
    }

    /* 4. Windows 11 Taskbar */
    render_taskbar();

    /* 5. Start Menu & Popups */
    render_start_menu();
    render_calendar_popup();
    render_volume_popup();

    /* 6. Mouse Cursor */
    render_mouse_cursor();

    /* 7. Fast 64-bit Blit to GOP Framebuffer */
    uint64_t *src = (uint64_t*)g_back_buffer;
    uint64_t *dst = (uint64_t*)g_fb.base_address;
    uint64_t count = (g_fb.pixels_per_scanline * g_fb.height * sizeof(uint32_t)) / sizeof(uint64_t);

    for (uint64_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

void compositor_update_mouse(int x, int y, uint8_t left_btn, uint8_t right_btn, uint8_t mid_btn) {
    (void)mid_btn;
    g_mouse.x = x;
    g_mouse.y = y;
    g_mouse.left_button = left_btn;
    g_mouse.right_button = right_btn;

    uint8_t left_pressed  = left_btn && !g_mouse.prev_left;
    uint8_t left_released = !left_btn && g_mouse.prev_left;

    /* Window Dragging Update */
    if (g_drag_window) {
        if (left_btn) {
            int nx = x - g_drag_offset_x;
            int ny = y - g_drag_offset_y;
            if (ny < 0) ny = 0;
            if (ny > (int)g_fb.height - TASKBAR_HEIGHT - TITLEBAR_HEIGHT) {
                ny = (int)g_fb.height - TASKBAR_HEIGHT - TITLEBAR_HEIGHT;
            }
            window_move(g_drag_window, nx, ny);
        } else {
            g_drag_window = NULL;
        }
    }

    if (left_pressed) {
        /* 1. Check Taskbar Clicks */
        if (y >= (int)g_fb.height - TASKBAR_HEIGHT) {
            /* Start button click */
            if (x >= 12 && x <= 60) {
                compositor_toggle_start_menu();
                g_mouse.prev_left = left_btn;
                g_mouse.prev_right = right_btn;
                compositor_render();
                return;
            }

            /* System tray clicks */
            int tw = g_fb.width;
            if (x >= tw - 120 && x <= tw - 16) {
                compositor_toggle_calendar();
                g_mouse.prev_left = left_btn;
                g_mouse.prev_right = right_btn;
                compositor_render();
                return;
            }

            if (x >= tw - 220 && x <= tw - 180) {
                compositor_toggle_volume();
                g_mouse.prev_left = left_btn;
                g_mouse.prev_right = right_btn;
                compositor_render();
                return;
            }

            /* Taskbar Dock Apps */
            int search_w = 200;
            int dock_x = 12 + 42 + 8 + search_w + 16;
            for (int i = 0; i < MAX_WINDOWS; i++) {
                Window *w = &g_windows[i];
                if (w->id != 0 && w->is_visible) {
                    if (x >= dock_x && x <= dock_x + 160) {
                        if (w->is_minimized) {
                            window_restore(w);
                        } else if (w->is_focused) {
                            window_minimize(w);
                        } else {
                            window_focus(w);
                        }
                        g_mouse.prev_left = left_btn;
                        g_mouse.prev_right = right_btn;
                        compositor_render();
                        return;
                    }
                    dock_x += 166;
                }
            }
        }

        /* 2. Check Start Menu Items Clicks */
        if (g_start_menu_open) {
            int sm_w = 460;
            int sm_h = 520;
            int sm_x = 12;
            int sm_y = g_fb.height - TASKBAR_HEIGHT - sm_h - 12;

            if (x >= sm_x && x <= sm_x + sm_w && y >= sm_y && y <= sm_y + sm_h) {
                /* Check pinned items clicks */
                for (int i = 0; i < 6; i++) {
                    int col = i % 3;
                    int row = i / 3;
                    int px = sm_x + 24 + col * 140;
                    int py = sm_y + 96 + row * 76;
                    if (x >= px && x <= px + 130 && y >= py && y <= py + 68) {
                        if (i == 0) explorer_app_launch();
                        else if (i == 1) taskmgr_app_launch();
                        else if (i == 2) firewall_app_launch();
                        else if (i == 3) terminal_app_launch();
                        g_start_menu_open = 0;
                        g_mouse.prev_left = left_btn;
                        g_mouse.prev_right = right_btn;
                        compositor_render();
                        return;
                    }
                }
            } else {
                g_start_menu_open = 0;
            }
        }

        /* 3. Check Windows Clicks (Top-most z-order first) */
        Window *clicked_win = NULL;
        int max_z = -1;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            Window *w = &g_windows[i];
            if (w->id != 0 && !w->is_minimized && w->is_visible) {
                if (x >= w->x && x <= w->x + w->width && y >= w->y && y <= w->y + w->height) {
                    if (w->z_order > max_z) {
                        max_z = w->z_order;
                        clicked_win = w;
                    }
                }
            }
        }

        if (clicked_win) {
            window_focus(clicked_win);

            /* Check Titlebar controls */
            if (y >= clicked_win->y && y <= clicked_win->y + TITLEBAR_HEIGHT) {
                int wx = clicked_win->x;
                int wy = clicked_win->y;
                int ww = clicked_win->width;

                int close_x = wx + ww - 44;
                int max_x   = close_x - 36;
                int min_x   = max_x - 36;

                if (x >= close_x && x <= wx + ww) {
                    window_destroy(clicked_win->id);
                } else if (x >= max_x && x < close_x) {
                    window_maximize(clicked_win);
                } else if (x >= min_x && x < max_x) {
                    window_minimize(clicked_win);
                } else {
                    /* Titlebar Drag Initiation */
                    g_drag_window = clicked_win;
                    g_drag_offset_x = x - clicked_win->x;
                    g_drag_offset_y = y - clicked_win->y;
                }
            } else {
                /* Content click: dispatch to window */
                if (clicked_win->on_mouse) {
                    int rel_x = x - clicked_win->x;
                    int rel_y = y - clicked_win->y;
                    clicked_win->on_mouse(clicked_win, rel_x, rel_y, 1, 0);
                }
            }
        } else {
            /* 4. Check Desktop Icon Clicks */
            int start_x = 24;
            int start_y = 28;
            int icon_h = 76;
            int icon_w = 84;

            for (int i = 0; i < MAX_DESKTOP_ICONS; i++) {
                int ix = start_x;
                int iy = start_y + i * icon_h;
                if (x >= ix - 6 && x <= ix + icon_w && y >= iy - 6 && y <= iy + icon_h) {
                    if (g_selected_desktop_icon == i) {
                        /* Double click action */
                        if (g_desktop_icons[i].on_activate) {
                            g_desktop_icons[i].on_activate();
                        }
                    } else {
                        g_selected_desktop_icon = i;
                    }
                    break;
                }
            }
        }
    }

    if (left_released) {
        g_drag_window = NULL;
    }

    g_mouse.prev_left  = left_btn;
    g_mouse.prev_right = right_btn;
    compositor_render();
}

void compositor_dispatch_key(char ascii, uint8_t scancode, uint8_t is_pressed) {
    if (!is_pressed) return;
    Window *win = window_get_focused();
    if (win && win->on_key) {
        win->on_key(win, ascii, scancode);
        compositor_render();
    }
}
