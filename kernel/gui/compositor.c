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
#include "../apps/vlc_app.h"
#include "../apps/installer_app.h"

static XenithraFrameBuffer g_fb;
static uint32_t *g_back_buffer = NULL;
static uint32_t g_back_buffer_storage[1920 * 1080]; /* Static fallback buffer up to 1080p */

static Window g_windows[MAX_WINDOWS] = {0};
static int g_window_count = 0;
static uint32_t g_next_win_id = 1;

static CompositorMouseState g_mouse = {640, 360, 0, 0, 0, 0, 0};
static uint8_t g_start_menu_open = 0;
static uint8_t g_calendar_open = 0;
static uint8_t g_volume_open = 0;
static int g_volume_level = 80;

/* Window Dragging State */
static Window *g_drag_window = NULL;
static int g_drag_offset_x = 0;
static int g_drag_offset_y = 0;

/* Desktop Icon Selection */
static int g_selected_desktop_icon = -1;
static uint64_t g_system_ticks = 0;

/* Desktop Icons Actions */
static void on_launch_this_pc(void)    { explorer_app_launch(); }
static void on_launch_explorer(void)   { explorer_app_launch(); }
static void on_launch_vlc(void)        { vlc_app_launch(); }
static void on_launch_installer(void)  { installer_app_launch(); }
static void on_launch_taskmgr(void)    { taskmgr_app_launch(); }
static void on_launch_firewall(void)   { firewall_app_launch(); }
static void on_launch_terminal(void)   { terminal_app_launch(); }
static void on_launch_recycle(void)    { explorer_app_launch(); }

static DesktopIcon g_desktop_icons[MAX_DESKTOP_ICONS] = {
    {"This PC",         "PC",  GUI_ACCENT_BLUE,   on_launch_this_pc},
    {"File Explorer",   "EX",  0x00D97706,        on_launch_explorer},
    {"VLC Media Player","VLC", GUI_ACCENT_ORANGE, on_launch_vlc},
    {"App Installer",   "APP", 0x000078D4,        on_launch_installer},
    {"Task Manager",    "TM",  GUI_ACCENT_CYAN,   on_launch_taskmgr},
    {"Security Center", "SC",  GUI_ACCENT_GREEN,  on_launch_firewall},
    {"Terminal Shell",  "CL",  GUI_ACCENT_PURPLE, on_launch_terminal},
    {"Recycle Bin",     "RB",  0x0064748B,        on_launch_recycle}
};

/* Modern Windows 11 Mouse Cursor Bitmap (12x18) */
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
    if (den <= 0) return c1;
    if (num <= 0) return c1;
    if (num >= den) return c2;
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
    gui_fill_rounded_rect(x, y, 22, 22, 5, bg_color);
    gui_draw_string(x + 5, y + 3, symbol, fg_color, 1);
}

/* ========================================================================= */
/* Windows 11 Fluent Vector Icon Renderers (24x24 / 32x32)                  */
/* ========================================================================= */

void gui_draw_fluent_icon_this_pc(int x, int y) {
    /* Monitor Display */
    gui_fill_rounded_rect(x + 2, y + 2, 28, 20, 4, 0x000284C7);
    gui_fill_rounded_rect(x + 4, y + 4, 24, 15, 2, 0x0038BDF8);
    gui_fill_rect(x + 6, y + 6, 20, 5, 0x007DD3FC); /* Gloss */
    /* Stand */
    gui_fill_rect(x + 13, y + 22, 6, 5, 0x0094A3B8);
    gui_fill_rounded_rect(x + 8, y + 26, 16, 4, 2, 0x00CBD5E1);
}

void gui_draw_fluent_icon_explorer(int x, int y) {
    /* Back folder tab */
    gui_fill_rounded_rect(x + 2, y + 4, 12, 8, 2, 0x000284C7);
    /* Front folder body */
    gui_fill_rounded_rect(x + 2, y + 8, 28, 20, 4, 0x00F59E0B);
    gui_fill_rect(x + 4, y + 10, 24, 6, 0x00FBBF24); /* Folder highlight */
    /* Inner document paper */
    gui_fill_rounded_rect(x + 8, y + 6, 16, 8, 2, 0x00FFFFFF);
    gui_fill_rect(x + 10, y + 8, 12, 2, 0x0094A3B8);
}

void gui_draw_fluent_icon_vlc(int x, int y) {
    /* Base ring */
    gui_fill_rounded_rect(x + 2, y + 25, 28, 6, 3, 0x00EA580C);
    gui_fill_rounded_rect(x + 4, y + 25, 24, 4, 2, 0x00F97316);
    /* Cone tier 1 */
    gui_fill_rounded_rect(x + 5, y + 19, 22, 6, 2, 0x00EA580C);
    gui_fill_rect(x + 7, y + 19, 18, 3, 0x00FFFFFF); /* White reflective ring */
    /* Cone tier 2 */
    gui_fill_rounded_rect(x + 8, y + 12, 16, 7, 2, 0x00F97316);
    gui_fill_rect(x + 10, y + 12, 12, 3, 0x00FFFFFF); /* White reflective ring */
    /* Cone tip */
    gui_fill_rounded_rect(x + 12, y + 4, 8, 8, 3, 0x00FB923C);
}

void gui_draw_fluent_icon_installer(int x, int y) {
    /* Package Box */
    gui_fill_rounded_rect(x + 3, y + 3, 26, 26, 5, 0x000078D4);
    gui_fill_rounded_rect(x + 5, y + 5, 22, 22, 3, 0x000284C7);
    /* Arrow down / Install motif */
    gui_fill_rect(x + 13, y + 8, 6, 10, 0x00FFFFFF);
    gui_fill_rect(x + 10, y + 16, 12, 3, 0x00FFFFFF);
    gui_fill_rect(x + 12, y + 19, 8, 3, 0x00FFFFFF);
    gui_fill_rect(x + 14, y + 22, 4, 3, 0x00FFFFFF);
}

void gui_draw_fluent_icon_taskmgr(int x, int y) {
    /* Dark card */
    gui_fill_rounded_rect(x + 2, y + 2, 28, 28, 5, 0x000F172A);
    gui_draw_rect(x + 2, y + 2, 28, 28, 0x0006B6D4);
    /* Grid lines */
    gui_fill_rect(x + 6, y + 10, 20, 1, 0x001E293B);
    gui_fill_rect(x + 6, y + 16, 20, 1, 0x001E293B);
    gui_fill_rect(x + 6, y + 22, 20, 1, 0x001E293B);
    /* Pulse ECG line */
    gui_fill_rect(x + 5, y + 16, 4, 2, 0x0022D3EE);
    gui_fill_rect(x + 9, y + 8, 3, 10, 0x0022D3EE);
    gui_fill_rect(x + 12, y + 14, 3, 10, 0x0022D3EE);
    gui_fill_rect(x + 15, y + 10, 4, 6, 0x0022D3EE);
    gui_fill_rect(x + 19, y + 16, 7, 2, 0x0022D3EE);
}

void gui_draw_fluent_icon_security(int x, int y) {
    /* Shield Body */
    gui_fill_rounded_rect(x + 3, y + 3, 26, 20, 4, 0x0010B981);
    gui_fill_rounded_rect(x + 7, y + 18, 18, 10, 5, 0x00059669);
    /* Checkmark motif */
    gui_fill_rect(x + 9, y + 13, 3, 6, 0x00FFFFFF);
    gui_fill_rect(x + 12, y + 17, 3, 5, 0x00FFFFFF);
    gui_fill_rect(x + 15, y + 12, 3, 7, 0x00FFFFFF);
    gui_fill_rect(x + 18, y + 8, 3, 6, 0x00FFFFFF);
}

void gui_draw_fluent_icon_terminal(int x, int y) {
    /* Shell Frame */
    gui_fill_rounded_rect(x + 2, y + 3, 28, 26, 4, 0x000F172A);
    gui_fill_rounded_rect(x + 2, y + 3, 28, 7, 3, 0x001E293B);
    gui_draw_rect(x + 2, y + 3, 28, 26, 0x008B5CF6);
    /* Prompt >_ */
    gui_draw_string(x + 6, y + 11, ">_", 0x0038BDF8, 1);
}

void gui_draw_fluent_icon_recycle(int x, int y) {
    /* Canister */
    gui_fill_rounded_rect(x + 5, y + 4, 22, 6, 2, 0x0064748B);
    gui_fill_rounded_rect(x + 7, y + 10, 18, 18, 3, 0x000284C7);
    gui_fill_rect(x + 9, y + 12, 3, 12, 0x00FFFFFF);
    gui_fill_rect(x + 15, y + 12, 3, 12, 0x00FFFFFF);
    gui_fill_rect(x + 20, y + 12, 3, 12, 0x00FFFFFF);
}

void gui_draw_fluent_icon_by_tag(int x, int y, const char *tag) {
    if (!tag) return;
    if (strcmp(tag, "PC") == 0)        gui_draw_fluent_icon_this_pc(x, y);
    else if (strcmp(tag, "EX") == 0)   gui_draw_fluent_icon_explorer(x, y);
    else if (strcmp(tag, "VLC") == 0)  gui_draw_fluent_icon_vlc(x, y);
    else if (strcmp(tag, "APP") == 0)  gui_draw_fluent_icon_installer(x, y);
    else if (strcmp(tag, "TM") == 0)   gui_draw_fluent_icon_taskmgr(x, y);
    else if (strcmp(tag, "SC") == 0)   gui_draw_fluent_icon_security(x, y);
    else if (strcmp(tag, "CL") == 0)   gui_draw_fluent_icon_terminal(x, y);
    else if (strcmp(tag, "RB") == 0)   gui_draw_fluent_icon_recycle(x, y);
    else gui_draw_fluent_icon_this_pc(x, y);
}

/* ========================================================================= */
/* Window Management Implementation                                         */
/* ========================================================================= */

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
        win->height = g_fb.height - TASKBAR_HEIGHT - 6;
        win->is_maximized = 1;
    } else {
        win->x = win->saved_x;
        win->y = win->saved_y;
        win->width = win->saved_w;
        win->height = win->saved_h;
        win->is_maximized = 0;
    }
    window_focus(win);
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
    vlc_app_tick();
    installer_app_tick();
}

/* ========================================================================= */
/* Authentic Windows 11 3D Bloom Wallpaper Renderer                         */
/* ========================================================================= */

static void render_wallpaper(void) {
    int h = g_fb.height - TASKBAR_HEIGHT;
    int w = g_fb.width;
    int mid = h / 2;

    /* 1. Deep Midnight Mica Flow Gradient */
    gui_draw_gradient_v(0, 0, w, mid, GUI_BG_WALLPAPER_TOP, GUI_BG_WALLPAPER_MID);
    gui_draw_gradient_v(0, mid, w, h - mid, GUI_BG_WALLPAPER_MID, GUI_BG_WALLPAPER_BOT);

    /* 2. Soft Ambient Radial Backlight Glow */
    int cx = w / 2;
    int cy = h / 2 - 10;

    for (int r = 240; r > 0; r -= 18) {
        uint32_t glow = blend(0x000E2448, 0x0008101E, r, 240);
        gui_fill_rounded_rect(cx - r * 2, cy - r, r * 4, r * 2, r, glow);
    }

    /* 3. Windows 11 Bloom Silk Ribbon Petals (Procedural 3D layered folds) */

    /* Layer A: Deep Royal Sapphire Background Petals */
    for (int p = 0; p < 8; p++) {
        int px = cx - 180 + p * 45;
        int py = cy - 80 + (p % 3) * 30;
        gui_fill_rounded_rect(px, py, 110, 160, 48, GUI_BLOOM_BLUE_DEEP);
    }

    /* Layer B: Radiant Cobalt Silk Petals */
    for (int p = 0; p < 6; p++) {
        int px = cx - 140 + p * 48;
        int py = cy - 60 + ((p + 1) % 4) * 20;
        gui_fill_rounded_rect(px, py, 95, 145, 42, GUI_BLOOM_BLUE_MID);
    }

    /* Layer C: Electric Violet & Orchid Swirls */
    for (int p = 0; p < 4; p++) {
        int px = cx - 90 + p * 50;
        int py = cy - 40 + (p % 2) * 25;
        gui_fill_rounded_rect(px, py, 75, 120, 36, (p % 2 == 0) ? GUI_BLOOM_PURPLE : GUI_BLOOM_INDIGO);
    }

    /* Layer D: Soft Sky Blue & Cyan Front Light Petals */
    for (int p = 0; p < 5; p++) {
        int px = cx - 110 + p * 44;
        int py = cy - 20 + ((p * 3) % 4) * 15;
        gui_fill_rounded_rect(px, py, 60, 95, 28, (p % 2 == 0) ? GUI_BLOOM_CYAN : GUI_BLOOM_BLUE_LIGHT);
    }

    /* Layer E: Inner Bloom Center Highlight */
    gui_fill_rounded_rect(cx - 40, cy - 10, 80, 70, 24, 0x0093C5FD);
    gui_fill_rounded_rect(cx - 20, cy + 5, 40, 40, 18, 0x00E0F2FE);

    /* Subtle Modern Typography */
    gui_draw_string_shadow(cx - 130, h - 80, "Xenithra OS", 0x00475569, 0x000F172A, 2);
    gui_draw_string(cx - 145, h - 55, "64-bit High-Security Fluent Workstation", 0x00334155, 1);
}

/* ========================================================================= */
/* Desktop Fluent Shortcuts                                                 */
/* ========================================================================= */

static void render_desktop_icons(void) {
    int start_x = 20;
    int start_y = 20;
    int icon_h = 74;
    int icon_w = 88;

    for (int i = 0; i < MAX_DESKTOP_ICONS; i++) {
        int ix = start_x;
        int iy = start_y + i * icon_h;

        /* Selection / Hover Highlight Card */
        if (g_selected_desktop_icon == i) {
            gui_fill_rounded_rect(ix - 4, iy - 4, icon_w, icon_h - 2, 6, 0x001E293B);
            gui_draw_rect(ix - 4, iy - 4, icon_w, icon_h - 2, GUI_ACCENT_BLUE);
        }

        /* Fluent Icon */
        gui_draw_fluent_icon_by_tag(ix + 28, iy + 4, g_desktop_icons[i].icon_tag);

        /* Icon Title */
        gui_draw_string_shadow(ix + 2, iy + 44, g_desktop_icons[i].title, GUI_TEXT_PRIMARY, 0x00000000, 1);
    }
}

/* ========================================================================= */
/* Window Frame & Titlebar Rendering                                         */
/* ========================================================================= */

static void render_window_frame(Window *win) {
    if (!win || win->id == 0 || win->is_minimized || !win->is_visible) return;

    int wx = win->x;
    int wy = win->y;
    int ww = win->width;
    int wh = win->height;

    /* 1. Window Drop Shadow */
    gui_fill_rounded_rect(wx + 4, wy + 4, ww + 2, wh + 2, 8, 0x0004070E);

    /* 2. Window Main Mica Body */
    gui_fill_rounded_rect(wx, wy, ww, wh, 8, GUI_BG_WINDOW);
    gui_draw_rect(wx, wy, ww, wh, win->is_focused ? GUI_BORDER_FOCUS : GUI_BORDER_COLOR);

    /* 3. Titlebar Header */
    uint32_t tb_color = win->is_focused ? GUI_BG_TITLEBAR_ACT : GUI_BG_TITLEBAR_INACT;
    gui_fill_rounded_rect(wx, wy, ww, TITLEBAR_HEIGHT, 8, tb_color);
    gui_fill_rect(wx, wy + TITLEBAR_HEIGHT - 6, ww, 6, tb_color);
    gui_draw_rect(wx, wy, ww, TITLEBAR_HEIGHT, GUI_BORDER_COLOR);

    /* App Fluent Icon & Title */
    gui_draw_fluent_icon_by_tag(wx + 8, wy + 6, win->app_tag);
    gui_draw_string(wx + 44, wy + 10, win->title, win->is_focused ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1);

    /* Window Control Buttons: [-] [□] [✕] */
    int btn_w = 42;
    int close_x = wx + ww - btn_w;
    int max_x   = close_x - btn_w;
    int min_x   = max_x - btn_w;

    /* Check hover over controls for dynamic feedback */
    int mx = g_mouse.x;
    int my = g_mouse.y;

    /* Minimize button */
    if (mx >= min_x && mx < max_x && my >= wy && my < wy + TITLEBAR_HEIGHT) {
        gui_fill_rounded_rect(min_x + 2, wy + 4, btn_w - 4, TITLEBAR_HEIGHT - 8, 4, GUI_BG_CARD_HOVER);
    }
    gui_draw_string(min_x + 18, wy + 10, "-", GUI_TEXT_PRIMARY, 1);

    /* Maximize button */
    if (mx >= max_x && mx < close_x && my >= wy && my < wy + TITLEBAR_HEIGHT) {
        gui_fill_rounded_rect(max_x + 2, wy + 4, btn_w - 4, TITLEBAR_HEIGHT - 8, 4, GUI_BG_CARD_HOVER);
    }
    if (win->is_maximized) {
        gui_draw_string(max_x + 16, wy + 10, "r", GUI_TEXT_PRIMARY, 1);
    } else {
        gui_draw_string(max_x + 16, wy + 10, "o", GUI_TEXT_PRIMARY, 1);
    }

    /* Close button */
    if (mx >= close_x && mx < wx + ww && my >= wy && my < wy + TITLEBAR_HEIGHT) {
        gui_fill_rounded_rect(close_x + 2, wy + 4, btn_w - 4, TITLEBAR_HEIGHT - 8, 4, GUI_ACCENT_RED);
        gui_draw_string(close_x + 18, wy + 10, "X", 0x00FFFFFF, 1);
    } else {
        gui_draw_string(close_x + 18, wy + 10, "X", GUI_TEXT_PRIMARY, 1);
    }

    /* 4. Client Content Area */
    int content_x = wx + 1;
    int content_y = wy + TITLEBAR_HEIGHT;
    int content_w = ww - 2;
    int content_h = wh - TITLEBAR_HEIGHT - 1;

    if (win->on_paint) {
        win->on_paint(win, content_x, content_y, content_w, content_h);
    }
}

/* ========================================================================= */
/* Centered Windows 11 Floating Acrylic Taskbar Dock                         */
/* ========================================================================= */

static void render_taskbar(void) {
    int ty = g_fb.height - TASKBAR_HEIGHT;
    int tw = g_fb.width;

    /* 1. Acrylic Taskbar Surface */
    gui_fill_rect(0, ty, tw, TASKBAR_HEIGHT, GUI_BG_TASKBAR);
    gui_draw_rect(0, ty, tw, 1, GUI_BG_TASKBAR_BORDER);

    /* Count active windows to center the dock icons */
    int active_win_count = 0;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (g_windows[i].id != 0 && g_windows[i].is_visible) {
            active_win_count++;
        }
    }

    /* Center Dock Calculations */
    int start_btn_w = 44;
    int search_pill_w = 170;
    int app_btn_w = 46;
    int total_dock_w = start_btn_w + 8 + search_pill_w + 12 + active_win_count * (app_btn_w + 6);
    int dock_x = (tw - total_dock_w) / 2;
    if (dock_x < 12) dock_x = 12;

    /* 1.1 Windows 11 Start Button (4 Cyan Tiles) */
    int start_y = ty + 6;
    gui_fill_rounded_rect(dock_x, start_y, start_btn_w, 40, 6, g_start_menu_open ? GUI_BG_CARD_HOVER : GUI_BG_TASKBAR);
    if (g_start_menu_open) {
        gui_draw_rect(dock_x, start_y, start_btn_w, 40, GUI_ACCENT_BLUE);
    }

    gui_fill_rect(dock_x + 12, start_y + 11, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(dock_x + 23, start_y + 11, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(dock_x + 12, start_y + 21, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(dock_x + 23, start_y + 21, 8, 7, GUI_ACCENT_CYAN);

    /* 1.2 Search Pill */
    int search_x = dock_x + start_btn_w + 8;
    gui_fill_rounded_rect(search_x, start_y, search_pill_w, 40, 20, GUI_BG_INPUT);
    gui_draw_rect(search_x, start_y, search_pill_w, 40, GUI_BORDER_COLOR);
    gui_draw_string(search_x + 14, start_y + 12, "Search", GUI_TEXT_MUTED, 1);

    /* 1.3 Running Application Dock Icons */
    int cur_app_x = search_x + search_pill_w + 12;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        Window *w = &g_windows[i];
        if (w->id != 0 && w->is_visible) {
            uint32_t bg = (w->is_focused && !w->is_minimized) ? GUI_BG_CARD_HOVER : GUI_BG_TASKBAR;
            gui_fill_rounded_rect(cur_app_x, start_y, app_btn_w, 40, 6, bg);
            if (w->is_focused && !w->is_minimized) {
                gui_draw_rect(cur_app_x, start_y, app_btn_w, 40, GUI_BORDER_COLOR);
            }

            /* Draw App Fluent Icon */
            gui_draw_fluent_icon_by_tag(cur_app_x + 7, start_y + 5, w->app_tag);

            /* Active Glowing Blue Indicator Pill Underneath */
            if (!w->is_minimized) {
                int pill_w = w->is_focused ? 20 : 8;
                gui_fill_rounded_rect(cur_app_x + (app_btn_w - pill_w) / 2, ty + TASKBAR_HEIGHT - 4, pill_w, 3, 1, GUI_ACCENT_BLUE);
            }

            cur_app_x += app_btn_w + 6;
        }
    }

    /* 1.4 System Tray (Right Edge) */
    int tray_right = tw - 12;

    /* Desktop Show Bar (Rightmost line) */
    gui_fill_rect(tray_right - 4, ty + 12, 2, 28, GUI_TEXT_MUTED);

    /* Clock & Date Widget */
    int clock_w = 90;
    int clock_x = tray_right - 8 - clock_w;
    gui_fill_rounded_rect(clock_x, ty + 6, clock_w, 40, 6, g_calendar_open ? GUI_BG_CARD_HOVER : GUI_BG_TASKBAR);
    gui_draw_string(clock_x + 12, ty + 10, "11:30 AM", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(clock_x + 12, ty + 24, "9/13/2026", GUI_TEXT_MUTED, 1);

    /* Security Guard Badge */
    int sec_x = clock_x - 34;
    gui_fill_rounded_rect(sec_x, ty + 10, 28, 32, 4, 0x0010B981);
    gui_draw_string(sec_x + 9, ty + 18, "S", 0x00FFFFFF, 1);

    /* Battery Widget */
    int bat_x = sec_x - 36;
    gui_fill_rounded_rect(bat_x, ty + 14, 26, 22, 3, GUI_BG_CARD);
    gui_draw_rect(bat_x, ty + 14, 26, 22, GUI_BORDER_COLOR);
    gui_fill_rect(bat_x + 2, ty + 16, 20, 18, GUI_ACCENT_GREEN);

    /* Master Volume Widget */
    int vol_x = bat_x - 34;
    gui_fill_rounded_rect(vol_x, ty + 10, 28, 32, 4, g_volume_open ? GUI_BG_CARD_HOVER : GUI_BG_TASKBAR);
    gui_draw_string(vol_x + 8, ty + 18, "V", GUI_TEXT_PRIMARY, 1);

    /* WiFi Network Widget */
    int net_x = vol_x - 34;
    gui_fill_rounded_rect(net_x, ty + 10, 28, 32, 4, GUI_BG_TASKBAR);
    gui_draw_string(net_x + 8, ty + 18, "N", GUI_ACCENT_CYAN, 1);
}

/* ========================================================================= */
/* Windows 11 Centered Start Menu Flyout                                     */
/* ========================================================================= */

static void render_start_menu(void) {
    if (!g_start_menu_open) return;

    int sm_w = 480;
    int sm_h = 540;
    int sm_x = (g_fb.width - sm_w) / 2;
    int sm_y = g_fb.height - TASKBAR_HEIGHT - sm_h - 12;

    /* Start Menu Acrylic Card Body */
    gui_fill_rounded_rect(sm_x, sm_y, sm_w, sm_h, 12, GUI_BG_STARTMENU);
    gui_draw_rect(sm_x, sm_y, sm_w, sm_h, GUI_ACCENT_BLUE);

    /* Top Search Box */
    gui_fill_rounded_rect(sm_x + 20, sm_y + 18, sm_w - 40, 38, 19, GUI_BG_INPUT);
    gui_draw_rect(sm_x + 20, sm_y + 18, sm_w - 40, 38, GUI_BORDER_COLOR);
    gui_draw_string(sm_x + 38, sm_y + 30, "Type here to search...", GUI_TEXT_MUTED, 1);

    /* Pinned Section */
    gui_draw_string(sm_x + 24, sm_y + 70, "Pinned", GUI_TEXT_PRIMARY, 1);

    const char *pinned_tags[] = {"EX", "VLC", "APP", "TM", "SC", "CL", "PC", "RB"};
    const char *pinned_names[] = {
        "File Explorer", "VLC Player",   "App Installer", "Task Manager",
        "Security Guard","Terminal",     "This PC",       "Recycle Bin"
    };

    for (int i = 0; i < 8; i++) {
        int col = i % 4;
        int row = i / 4;
        int px = sm_x + 20 + col * 110;
        int py = sm_y + 96 + row * 78;

        gui_fill_rounded_rect(px, py, 102, 70, 8, GUI_BG_CARD);
        gui_draw_rect(px, py, 102, 70, GUI_BORDER_COLOR);

        /* Fluent Icon */
        gui_draw_fluent_icon_by_tag(px + 36, py + 8, pinned_tags[i]);
        gui_draw_string(px + 6, py + 48, pinned_names[i], GUI_TEXT_SECONDARY, 1);
    }

    /* Recommended / Recent Section */
    gui_draw_string(sm_x + 24, sm_y + 276, "Recommended", GUI_TEXT_PRIMARY, 1);
    const char *recent[] = {
        "matrix_intro_64.mp4 - VLC Player",
        "vlc_setup_x64.exe   - 64-bit App Installer",
        "firewall.rules      - Security Center"
    };

    for (int i = 0; i < 3; i++) {
        int ry = sm_y + 300 + i * 40;
        gui_fill_rounded_rect(sm_x + 20, ry, sm_w - 40, 34, 6, GUI_BG_CARD);
        gui_draw_string(sm_x + 36, ry + 10, recent[i], GUI_TEXT_SECONDARY, 1);
    }

    /* User Profile Footer */
    int ubar_y = sm_y + sm_h - 56;
    gui_fill_rounded_rect(sm_x, ubar_y, sm_w, 56, 12, 0x000B101E);
    gui_draw_rect(sm_x, ubar_y, sm_w, 1, GUI_BORDER_COLOR);

    /* Avatar */
    gui_fill_rounded_rect(sm_x + 20, ubar_y + 11, 34, 34, 17, GUI_ACCENT_BLUE);
    gui_draw_string(sm_x + 30, ubar_y + 19, "A", 0x00FFFFFF, 1);
    gui_draw_string(sm_x + 64, ubar_y + 14, "Administrator", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(sm_x + 64, ubar_y + 30, "Ring 0 Full Privilege", GUI_ACCENT_GREEN, 1);

    /* Power Button */
    gui_fill_rounded_rect(sm_x + sm_w - 48, ubar_y + 11, 34, 34, 6, GUI_BG_CARD);
    gui_draw_string(sm_x + sm_w - 36, ubar_y + 19, "P", GUI_ACCENT_RED, 1);
}

/* ========================================================================= */
/* Popups (Calendar & Volume)                                               */
/* ========================================================================= */

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

    const char *days[] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};
    for (int d = 0; d < 7; d++) {
        gui_draw_string(cal_x + 20 + d * 36, cal_y + 76, days[d], GUI_TEXT_MUTED, 1);
    }

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

    int vol_w = 270;
    int vol_h = 96;
    int vol_x = g_fb.width - vol_w - 80;
    int vol_y = g_fb.height - TASKBAR_HEIGHT - vol_h - 12;

    gui_fill_rounded_rect(vol_x, vol_y, vol_w, vol_h, 12, GUI_BG_STARTMENU);
    gui_draw_rect(vol_x, vol_y, vol_w, vol_h, GUI_ACCENT_BLUE);

    gui_draw_string(vol_x + 18, vol_y + 16, "Master Audio Output", GUI_TEXT_PRIMARY, 1);

    /* Volume Slider Track */
    gui_fill_rounded_rect(vol_x + 18, vol_y + 46, 180, 8, 4, GUI_BG_INPUT);
    gui_fill_rounded_rect(vol_x + 18, vol_y + 46, (180 * g_volume_level) / 100, 8, 4, GUI_ACCENT_BLUE);
    gui_fill_rounded_rect(vol_x + 14 + (180 * g_volume_level) / 100, vol_y + 40, 16, 20, 8, 0x00FFFFFF);

    char num[8];
    uint_to_str(g_volume_level, num);
    strcat(num, "%");
    gui_draw_string(vol_x + 210, vol_y + 42, num, GUI_TEXT_PRIMARY, 1);
}

static void render_mouse_cursor(void) {
    int mx = g_mouse.x;
    int my = g_mouse.y;

    /* Drop shadow */
    for (int row = 0; row < 18; row++) {
        uint16_t bits = cursor_shadow[row];
        for (int col = 0; col < 12; col++) {
            if ((bits >> (15 - col)) & 1) {
                gui_put_pixel(mx + col + 1, my + row + 1, 0x00060A12);
            }
        }
    }

    /* Main cursor body */
    for (int row = 0; row < 18; row++) {
        uint16_t bits = cursor_bitmap[row];
        for (int col = 0; col < 12; col++) {
            if ((bits >> (15 - col)) & 1) {
                gui_put_pixel(mx + col, my + row, 0x00FFFFFF);
            }
        }
    }
}

/* ========================================================================= */
/* Master Render Pipeline                                                    */
/* ========================================================================= */

void compositor_render(void) {
    if (!g_fb.base_address || !g_back_buffer) return;

    /* 1. Desktop 3D Bloom Wallpaper */
    render_wallpaper();

    /* 2. Desktop Shortcuts */
    render_desktop_icons();

    /* 3. Render Windows in Z-Order (from lowest z to highest z) */
    for (int z = 0; z < g_window_count + 16; z++) {
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (g_windows[i].id != 0 && g_windows[i].z_order == z) {
                render_window_frame(&g_windows[i]);
            }
        }
    }

    /* 4. Windows 11 Centered Taskbar Dock */
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

/* ========================================================================= */
/* Mouse and Keyboard Event Handlers                                         */
/* ========================================================================= */

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
            int tw = g_fb.width;
            int active_win_count = 0;
            for (int i = 0; i < MAX_WINDOWS; i++) {
                if (g_windows[i].id != 0 && g_windows[i].is_visible) active_win_count++;
            }

            int start_btn_w = 44;
            int search_pill_w = 170;
            int app_btn_w = 46;
            int total_dock_w = start_btn_w + 8 + search_pill_w + 12 + active_win_count * (app_btn_w + 6);
            int dock_x = (tw - total_dock_w) / 2;
            if (dock_x < 12) dock_x = 12;

            /* Start button click */
            if (x >= dock_x && x <= dock_x + start_btn_w) {
                compositor_toggle_start_menu();
                g_mouse.prev_left = left_btn;
                g_mouse.prev_right = right_btn;
                compositor_render();
                return;
            }

            /* Search pill click */
            if (x >= dock_x + start_btn_w + 8 && x <= dock_x + start_btn_w + 8 + search_pill_w) {
                compositor_toggle_start_menu();
                g_mouse.prev_left = left_btn;
                g_mouse.prev_right = right_btn;
                compositor_render();
                return;
            }

            /* Running App Dock Icons click */
            int cur_app_x = dock_x + start_btn_w + 8 + search_pill_w + 12;
            for (int i = 0; i < MAX_WINDOWS; i++) {
                Window *w = &g_windows[i];
                if (w->id != 0 && w->is_visible) {
                    if (x >= cur_app_x && x <= cur_app_x + app_btn_w) {
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
                    cur_app_x += app_btn_w + 6;
                }
            }

            /* System tray clicks */
            if (x >= tw - 110 && x <= tw - 12) {
                compositor_toggle_calendar();
                g_mouse.prev_left = left_btn;
                g_mouse.prev_right = right_btn;
                compositor_render();
                return;
            }

            if (x >= tw - 200 && x <= tw - 160) {
                compositor_toggle_volume();
                g_mouse.prev_left = left_btn;
                g_mouse.prev_right = right_btn;
                compositor_render();
                return;
            }
        }

        /* 2. Check Start Menu Items Clicks */
        if (g_start_menu_open) {
            int sm_w = 480;
            int sm_h = 540;
            int sm_x = (g_fb.width - sm_w) / 2;
            int sm_y = g_fb.height - TASKBAR_HEIGHT - sm_h - 12;

            if (x >= sm_x && x <= sm_x + sm_w && y >= sm_y && y <= sm_y + sm_h) {
                /* Check pinned items clicks */
                for (int i = 0; i < 8; i++) {
                    int col = i % 4;
                    int row = i / 4;
                    int px = sm_x + 20 + col * 110;
                    int py = sm_y + 96 + row * 78;

                    if (x >= px && x <= px + 102 && y >= py && y <= py + 70) {
                        if (i == 0) explorer_app_launch();
                        else if (i == 1) vlc_app_launch();
                        else if (i == 2) installer_app_launch();
                        else if (i == 3) taskmgr_app_launch();
                        else if (i == 4) firewall_app_launch();
                        else if (i == 5) terminal_app_launch();
                        else if (i == 6) explorer_app_launch();
                        else if (i == 7) explorer_app_launch();

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

                int btn_w = 42;
                int close_x = wx + ww - btn_w;
                int max_x   = close_x - btn_w;
                int min_x   = max_x - btn_w;

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
            int start_x = 20;
            int start_y = 20;
            int icon_h = 74;
            int icon_w = 88;

            for (int i = 0; i < MAX_DESKTOP_ICONS; i++) {
                int ix = start_x;
                int iy = start_y + i * icon_h;
                if (x >= ix - 4 && x <= ix + icon_w && y >= iy - 4 && y <= iy + icon_h) {
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
