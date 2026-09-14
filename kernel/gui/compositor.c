/**
 * @file compositor.c
 * @brief Windows 11 Fluent 32-bit Window Compositor & Desktop Environment
 */

#include "compositor.h"
#include "v8_engine.h"
#include "../kstring.h"
#include "../../shared/font.h"
#include "../apps/browser_app.h"
#include "../apps/explorer_app.h"
#include "../apps/taskmgr_app.h"
#include "../apps/firewall_app.h"
#include "../apps/terminal_app.h"
#include "../apps/vlc_app.h"
#include "../apps/installer_app.h"
#include "../apps/diskclone_app.h"

/* x86 I/O Port Helpers */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static XenithraFrameBuffer g_fb;
static uint32_t *g_back_buffer = NULL;
static uint32_t g_back_buffer_storage[1920 * 1080];

static Window g_windows[MAX_WINDOWS] = {0};
static int g_window_count = 0;
static uint32_t g_next_win_id = 1;

static CompositorMouseState g_mouse = {640, 360, 0, 0, 0, 0, 0};
static uint8_t g_start_menu_open = 0;
static uint8_t g_calendar_open = 0;
static uint8_t g_volume_open = 0;
static uint8_t g_power_menu_open = 0;
static uint8_t g_is_suspended = 0;
static int g_volume_level = 80;

/* Window Dragging State */
static Window *g_drag_window = NULL;
static int g_drag_offset_x = 0;
static int g_drag_offset_y = 0;

/* Desktop Icon Selection */
static int g_selected_desktop_icon = -1;
static uint64_t g_system_ticks = 0;

/* Hardware Power Control Implementations */
void system_shutdown(void) {
    outw(0x604, 0x2000);   /* QEMU ACPI */
    outw(0x4004, 0x3400);  /* VirtualBox ACPI */
    outw(0x600, 0x34);
    outw(0xB004, 0x2000);  /* Bochs ACPI */
    __asm__ volatile ("cli; hlt");
    while (1) { __asm__ volatile ("hlt"); }
}

void system_reboot(void) {
    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 1) inb(0x60);
    } while (temp & 2);
    outb(0x64, 0xFE);
    outb(0xCF9, 0x06);
    __asm__ volatile ("lidt 0; int3");
    while (1) { __asm__ volatile ("hlt"); }
}

void system_hibernate(void) {
    g_is_suspended = 1;
    compositor_render();
}

void compositor_toggle_power_menu(void) {
    g_power_menu_open = !g_power_menu_open;
}

/* Desktop Icons Launch Actions */
static void on_launch_this_pc(void)    { explorer_app_launch(); }
static void on_launch_explorer(void)   { explorer_app_launch(); }
static void on_launch_browser(void)    { browser_app_launch(); }
static void on_launch_vlc(void)        { vlc_app_launch(); }
static void on_launch_installer(void)  { installer_app_launch(); }
static void on_launch_diskclone(void)  { diskclone_app_launch(); }
static void on_launch_taskmgr(void)    { taskmgr_app_launch(); }
static void on_launch_firewall(void)   { firewall_app_launch(); }
static void on_launch_terminal(void)   { terminal_app_launch(); }

/* Exact Desktop Icons Definition from Reference Image 1 */
static DesktopIcon g_desktop_icons[MAX_DESKTOP_ICONS] = {
    /* Left Column 1 (X: 16) */
    {"This PC",          "PC",   GUI_ACCENT_BLUE,   0, 0, on_launch_this_pc},
    {"Recycle Bin",      "RB",   0x0064748B,        0, 1, on_launch_this_pc},
    {"Personal - Edge",  "EDGE", 0x000078D4,        0, 2, on_launch_browser},
    {"Google Chrome",    "CRM",  0x00EA4335,        0, 3, on_launch_browser},
    {"iTunes",           "ITN",  0x00EC4899,        0, 4, on_launch_vlc},
    {"Antigravity IDE",  "AGY",  0x00A855F7,        0, 5, on_launch_terminal},
    {"VLC player",       "VLC",  GUI_ACCENT_ORANGE, 0, 6, on_launch_vlc},
    {"Arduino IDE",      "ARD",  0x0006B6D4,        0, 7, on_launch_terminal},

    /* Left Column 2 (X: 110) */
    {"Modbus Poll",      "MOD",  0x003B82F6,        1, 0, on_launch_installer},
    {"Postman",          "PST",  0x00F97316,        1, 1, on_launch_installer},
    {"Modbus Slave",     "MOD",  0x0010B981,        1, 2, on_launch_installer},
    {"LocalSend",        "LCL",  0x0006B6D4,        1, 3, on_launch_installer},
    {"Free Downlo...",   "FDM",  0x000284C7,        1, 4, on_launch_installer},
    {"VirtualBox",       "VBX",  0x000078D4,        1, 5, on_launch_installer},
    {"Sector Cloner",    "DCL",  0x000284C7,        1, 6, on_launch_diskclone},

    /* Right Column 1 (X: Screen - 200) */
    {"recovery_ac...",   "DOC",  0x0038BDF8,        2, 0, on_launch_this_pc},
    {"nodejsRe...",      "BAT",  0x0064748B,        2, 1, on_launch_terminal},
    {"Force_Virtu...",   "DOC",  0x0038BDF8,        2, 2, on_launch_this_pc},
    {"content.txt",      "DOC",  0x0038BDF8,        2, 3, on_launch_this_pc},
    {"Verborse.bat",     "BAT",  0x0064748B,        2, 4, on_launch_terminal},

    /* Right Column 2 (X: Screen - 98) */
    {"main.txt",         "DOC",  0x0038BDF8,        3, 0, on_launch_this_pc},
    {"Log.txt",          "DOC",  0x0038BDF8,        3, 1, on_launch_this_pc},
    {"Games_Lis...",     "DOC",  0x0038BDF8,        3, 2, on_launch_this_pc},
    {"link.txt",         "DOC",  0x0038BDF8,        3, 3, on_launch_this_pc},
    {"Restart_Net...",   "RST",  0x00EF4444,        3, 4, on_launch_firewall}
};

/* Modern Windows 11 Mouse Cursor Bitmap (12x18) */
static const uint16_t cursor_bitmap[18] = {
    0b1000000000000000, 0b1100000000000000, 0b1110000000000000, 0b1111000000000000,
    0b1111100000000000, 0b1111110000000000, 0b1111111000000000, 0b1111111100000000,
    0b1111111110000000, 0b1111110000000000, 0b1101111000000000, 0b1000111100000000,
    0b0000011110000000, 0b0000011110000000, 0b0000001111000000, 0b0000001111000000,
    0b0000000110000000, 0b0000000000000000
};

static const uint16_t cursor_shadow[18] = {
    0b1100000000000000, 0b1110000000000000, 0b1111000000000000, 0b1111100000000000,
    0b1111110000000000, 0b1111111000000000, 0b1111111100000000, 0b1111111110000000,
    0b1111111111000000, 0b1111111100000000, 0b1111111110000000, 0b1100111110000000,
    0b1000011111000000, 0b0000011111000000, 0b0000001111100000, 0b0000001111100000,
    0b0000000111000000, 0b0000000011000000
};

void compositor_init(XenithraFrameBuffer fb) {
    g_fb = fb;
    g_back_buffer = g_back_buffer_storage;
    g_window_count = 0;
    g_start_menu_open = 0;
    g_calendar_open = 0;
    g_volume_open = 0;
    g_power_menu_open = 0;
    g_is_suspended = 0;
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
    if (den <= 0 || num <= 0) return c1;
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

/* ========================================================================= */
/* Windows 11 Fluent Vector Icon Renderers                                   */
/* ========================================================================= */

void gui_draw_fluent_icon_this_pc(int x, int y) {
    gui_fill_rounded_rect(x + 2, y + 2, 28, 20, 4, 0x000284C7);
    gui_fill_rounded_rect(x + 4, y + 4, 24, 15, 2, 0x0038BDF8);
    gui_fill_rect(x + 6, y + 6, 20, 4, 0x007DD3FC);
    gui_fill_rect(x + 13, y + 22, 6, 5, 0x0094A3B8);
    gui_fill_rounded_rect(x + 8, y + 26, 16, 4, 2, 0x00CBD5E1);
}

void gui_draw_fluent_icon_explorer(int x, int y) {
    gui_fill_rounded_rect(x + 2, y + 4, 12, 8, 2, 0x000284C7);
    gui_fill_rounded_rect(x + 2, y + 8, 28, 20, 4, 0x00F59E0B);
    gui_fill_rect(x + 4, y + 10, 24, 6, 0x00FBBF24);
    gui_fill_rounded_rect(x + 8, y + 6, 16, 8, 2, 0x00FFFFFF);
    gui_fill_rect(x + 10, y + 8, 12, 2, 0x0094A3B8);
}

void gui_draw_fluent_icon_vlc(int x, int y) {
    gui_fill_rounded_rect(x + 2, y + 25, 28, 6, 3, 0x00EA580C);
    gui_fill_rounded_rect(x + 4, y + 25, 24, 4, 2, 0x00F97316);
    gui_fill_rounded_rect(x + 5, y + 19, 22, 6, 2, 0x00EA580C);
    gui_fill_rect(x + 7, y + 19, 18, 3, 0x00FFFFFF);
    gui_fill_rounded_rect(x + 8, y + 12, 16, 7, 2, 0x00F97316);
    gui_fill_rect(x + 10, y + 12, 12, 3, 0x00FFFFFF);
    gui_fill_rounded_rect(x + 12, y + 4, 8, 8, 3, 0x00FB923C);
}

void gui_draw_fluent_icon_installer(int x, int y) {
    gui_fill_rounded_rect(x + 3, y + 3, 26, 26, 5, 0x000078D4);
    gui_fill_rounded_rect(x + 5, y + 5, 22, 22, 3, 0x000284C7);
    gui_fill_rect(x + 13, y + 8, 6, 10, 0x00FFFFFF);
    gui_fill_rect(x + 10, y + 16, 12, 3, 0x00FFFFFF);
    gui_fill_rect(x + 12, y + 19, 8, 3, 0x00FFFFFF);
}

void gui_draw_fluent_icon_diskclone(int x, int y) {
    gui_fill_rounded_rect(x + 2, y + 3, 28, 26, 4, 0x000284C7);
    gui_fill_rect(x + 6, y + 7, 20, 10, 0x0038BDF8);
    gui_fill_rect(x + 6, y + 20, 6, 5, 0x0010B981);
    gui_fill_rect(x + 14, y + 20, 6, 5, 0x0010B981);
    gui_fill_rect(x + 22, y + 20, 4, 5, 0x00F59E0B);
}

void gui_draw_fluent_icon_taskmgr(int x, int y) {
    gui_fill_rounded_rect(x + 2, y + 2, 28, 28, 5, 0x000F172A);
    gui_draw_rect(x + 2, y + 2, 28, 28, 0x0006B6D4);
    gui_fill_rect(x + 6, y + 10, 20, 1, 0x001E293B);
    gui_fill_rect(x + 6, y + 16, 20, 1, 0x001E293B);
    gui_fill_rect(x + 5, y + 16, 4, 2, 0x0022D3EE);
    gui_fill_rect(x + 9, y + 8, 3, 10, 0x0022D3EE);
    gui_fill_rect(x + 12, y + 14, 3, 10, 0x0022D3EE);
    gui_fill_rect(x + 15, y + 10, 4, 6, 0x0022D3EE);
    gui_fill_rect(x + 19, y + 16, 7, 2, 0x0022D3EE);
}

void gui_draw_fluent_icon_security(int x, int y) {
    gui_fill_rounded_rect(x + 3, y + 3, 26, 20, 4, 0x0010B981);
    gui_fill_rounded_rect(x + 7, y + 18, 18, 10, 5, 0x00059669);
    gui_fill_rect(x + 9, y + 13, 3, 6, 0x00FFFFFF);
    gui_fill_rect(x + 12, y + 17, 3, 5, 0x00FFFFFF);
    gui_fill_rect(x + 15, y + 12, 3, 7, 0x00FFFFFF);
    gui_fill_rect(x + 18, y + 8, 3, 6, 0x00FFFFFF);
}

void gui_draw_fluent_icon_terminal(int x, int y) {
    gui_fill_rounded_rect(x + 2, y + 3, 28, 26, 4, 0x000F172A);
    gui_fill_rounded_rect(x + 2, y + 3, 28, 7, 3, 0x001E293B);
    gui_draw_rect(x + 2, y + 3, 28, 26, 0x008B5CF6);
    gui_draw_string(x + 6, y + 11, ">_", 0x0038BDF8, 1);
}

void gui_draw_fluent_icon_recycle(int x, int y) {
    gui_fill_rounded_rect(x + 5, y + 4, 22, 6, 2, 0x0064748B);
    gui_fill_rounded_rect(x + 7, y + 10, 18, 18, 3, 0x000284C7);
    gui_fill_rect(x + 9, y + 12, 3, 12, 0x00FFFFFF);
    gui_fill_rect(x + 15, y + 12, 3, 12, 0x00FFFFFF);
    gui_fill_rect(x + 20, y + 12, 3, 12, 0x00FFFFFF);
}

void gui_draw_fluent_icon_edge(int x, int y) {
    gui_fill_rounded_rect(x + 3, y + 3, 26, 26, 13, 0x000078D4);
    gui_fill_rounded_rect(x + 6, y + 6, 20, 20, 10, 0x0000A8FF);
    gui_fill_rounded_rect(x + 11, y + 11, 10, 10, 5, 0x0010B981);
}

void gui_draw_fluent_icon_chrome(int x, int y) {
    gui_fill_rounded_rect(x + 3, y + 3, 26, 26, 13, 0x00EA4335);
    gui_fill_rounded_rect(x + 7, y + 7, 18, 18, 9, 0x00FBBC05);
    gui_fill_rounded_rect(x + 11, y + 11, 10, 10, 5, 0x0034A853);
    gui_fill_rounded_rect(x + 13, y + 13, 6, 6, 3, 0x004285F4);
}

void gui_draw_fluent_icon_postman(int x, int y) {
    gui_fill_rounded_rect(x + 3, y + 3, 26, 26, 13, 0x00F97316);
    gui_draw_string(x + 11, y + 8, "P", 0x00FFFFFF, 1);
}

void gui_draw_fluent_icon_antigravity(int x, int y) {
    gui_fill_rounded_rect(x + 3, y + 3, 26, 26, 6, 0x001E1B4B);
    gui_draw_rect(x + 3, y + 3, 26, 26, 0x00818CF8);
    gui_draw_string(x + 9, y + 8, "A", 0x00A5B4FC, 1);
}

void gui_draw_fluent_icon_modbus(int x, int y) {
    gui_fill_rounded_rect(x + 3, y + 3, 26, 26, 4, 0x000F172A);
    gui_draw_rect(x + 3, y + 3, 26, 26, 0x0038BDF8);
    gui_draw_string(x + 6, y + 8, "MB", 0x0038BDF8, 1);
}

void gui_draw_fluent_icon_text_doc(int x, int y) {
    gui_fill_rounded_rect(x + 5, y + 2, 22, 28, 3, 0x00F1F5F9);
    gui_fill_rect(x + 8, y + 7, 14, 2, 0x0064748B);
    gui_fill_rect(x + 8, y + 12, 14, 2, 0x0064748B);
    gui_fill_rect(x + 8, y + 17, 14, 2, 0x0064748B);
}

void gui_draw_fluent_icon_batch_file(int x, int y) {
    gui_fill_rounded_rect(x + 5, y + 2, 22, 28, 3, 0x00334155);
    gui_draw_rect(x + 5, y + 2, 22, 28, 0x0094A3B8);
    gui_draw_string(x + 7, y + 8, "GE", 0x0038BDF8, 1);
}

void gui_draw_fluent_icon_by_tag(int x, int y, const char *tag) {
    if (!tag) return;
    if (strcmp(tag, "PC") == 0)        gui_draw_fluent_icon_this_pc(x, y);
    else if (strcmp(tag, "EX") == 0)   gui_draw_fluent_icon_explorer(x, y);
    else if (strcmp(tag, "VLC") == 0)  gui_draw_fluent_icon_vlc(x, y);
    else if (strcmp(tag, "APP") == 0)  gui_draw_fluent_icon_installer(x, y);
    else if (strcmp(tag, "DCL") == 0)  gui_draw_fluent_icon_diskclone(x, y);
    else if (strcmp(tag, "TM") == 0)   gui_draw_fluent_icon_taskmgr(x, y);
    else if (strcmp(tag, "SC") == 0)   gui_draw_fluent_icon_security(x, y);
    else if (strcmp(tag, "CL") == 0)   gui_draw_fluent_icon_terminal(x, y);
    else if (strcmp(tag, "RB") == 0)   gui_draw_fluent_icon_recycle(x, y);
    else if (strcmp(tag, "EDGE") == 0) gui_draw_fluent_icon_edge(x, y);
    else if (strcmp(tag, "CRM") == 0)  gui_draw_fluent_icon_chrome(x, y);
    else if (strcmp(tag, "PST") == 0)  gui_draw_fluent_icon_postman(x, y);
    else if (strcmp(tag, "AGY") == 0)  gui_draw_fluent_icon_antigravity(x, y);
    else if (strcmp(tag, "MOD") == 0)  gui_draw_fluent_icon_modbus(x, y);
    else if (strcmp(tag, "DOC") == 0)  gui_draw_fluent_icon_text_doc(x, y);
    else if (strcmp(tag, "BAT") == 0)  gui_draw_fluent_icon_batch_file(x, y);
    else if (strcmp(tag, "RST") == 0)  gui_draw_fluent_icon_security(x, y);
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
        win->height = g_fb.height - TASKBAR_HEIGHT;
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
        g_power_menu_open = 0;
    }
}

void compositor_toggle_calendar(void) {
    g_calendar_open = !g_calendar_open;
    if (g_calendar_open) {
        g_start_menu_open = 0;
        g_volume_open = 0;
        g_power_menu_open = 0;
    }
}

void compositor_toggle_volume(void) {
    g_volume_open = !g_volume_open;
    if (g_volume_open) {
        g_start_menu_open = 0;
        g_calendar_open = 0;
        g_power_menu_open = 0;
    }
}

void compositor_tick(void) {
    g_system_ticks++;
    taskmgr_tick();
    vlc_app_tick();
    installer_app_tick();
    diskclone_app_tick();
}

/* ========================================================================= */
/* Authentic Windows 11 Dark Mode Bloom Wallpaper Renderer                   */
/* ========================================================================= */

static void render_wallpaper(void) {
    int h = g_fb.height - TASKBAR_HEIGHT;
    int w = g_fb.width;
    int mid = h / 2;

    /* 1. Deep Midnight Navy Background Gradient */
    gui_draw_gradient_v(0, 0, w, mid, 0x00060B18, 0x000E192E);
    gui_draw_gradient_v(0, mid, w, h - mid, 0x000E192E, 0x00040810);

    /* 2. Soft Ambient Radial Glow (Dark Cobalt) */
    int cx = w / 2 + 10;
    int cy = h / 2 - 10;

    for (int r = 180; r > 0; r -= 15) {
        uint32_t glow = blend(0x0013284C, 0x0008101E, r, 180);
        gui_fill_rounded_rect(cx - r * 2, cy - r, r * 4, r * 2, r, glow);
    }

    /* 3. Windows 11 3D Bloom Petals (Clean Royal Cobalt & Cyan Curves) */
    gui_fill_rounded_rect(cx - 80, cy - 70, 160, 140, 48, 0x001D4ED8);
    gui_fill_rounded_rect(cx - 60, cy - 50, 120, 110, 38, 0x002563EB);
    gui_fill_rounded_rect(cx - 40, cy - 30, 80, 80, 26, 0x0038BDF8);
    gui_fill_rounded_rect(cx - 15, cy - 10, 40, 45, 16, 0x0093C5FD);
    gui_fill_rounded_rect(cx - 5, cy + 2, 20, 20, 10, 0x00E0F2FE);
}

/* ========================================================================= */
/* Desktop Shortcuts (Proper Column Spacing to Prevent Collisions)           */
/* ========================================================================= */

static void render_desktop_icons(void) {
    int start_y = 16;
    int row_h = 72;
    int col_w = 84;

    int left_col0_x = 16;
    int left_col1_x = 110;
    int right_col3_x = g_fb.width - 98;
    int right_col2_x = g_fb.width - 200;

    for (int i = 0; i < MAX_DESKTOP_ICONS; i++) {
        DesktopIcon *ic = &g_desktop_icons[i];
        int ix = left_col0_x;
        int iy = start_y + ic->row * row_h;

        if (ic->col == 0)      ix = left_col0_x;
        else if (ic->col == 1) ix = left_col1_x;
        else if (ic->col == 2) ix = right_col2_x;
        else if (ic->col == 3) ix = right_col3_x;

        /* Selection Card */
        if (g_selected_desktop_icon == i) {
            gui_fill_rounded_rect(ix - 4, iy - 2, col_w, row_h - 4, 6, 0x001E293B);
            gui_draw_rect(ix - 4, iy - 2, col_w, row_h - 4, GUI_ACCENT_BLUE);
        }

        /* Fluent Icon */
        gui_draw_fluent_icon_by_tag(ix + 26, iy + 4, ic->icon_tag);

        /* Icon Title with Shadow */
        gui_draw_string_shadow(ix + 2, iy + 42, ic->title, GUI_TEXT_PRIMARY, 0x00000000, 1);
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

    /* Drop Shadow */
    gui_fill_rounded_rect(wx + 4, wy + 4, ww + 2, wh + 2, 8, 0x0004070E);

    /* Main Mica Body */
    gui_fill_rounded_rect(wx, wy, ww, wh, 8, GUI_BG_WINDOW);
    gui_draw_rect(wx, wy, ww, wh, win->is_focused ? GUI_BORDER_FOCUS : GUI_BORDER_COLOR);

    /* Titlebar Header */
    uint32_t tb_color = win->is_focused ? GUI_BG_TITLEBAR_ACT : GUI_BG_TITLEBAR_INACT;
    gui_fill_rounded_rect(wx, wy, ww, TITLEBAR_HEIGHT, 8, tb_color);
    gui_fill_rect(wx, wy + TITLEBAR_HEIGHT - 6, ww, 6, tb_color);
    gui_draw_rect(wx, wy, ww, TITLEBAR_HEIGHT, GUI_BORDER_COLOR);

    /* App Fluent Icon & Title */
    gui_draw_fluent_icon_by_tag(wx + 8, wy + 6, win->app_tag);
    gui_draw_string(wx + 44, wy + 10, win->title, win->is_focused ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1);

    /* Window Control Buttons */
    int btn_w = 42;
    int close_x = wx + ww - btn_w;
    int max_x   = close_x - btn_w;
    int min_x   = max_x - btn_w;

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
    gui_draw_string(max_x + 16, wy + 10, win->is_maximized ? "r" : "o", GUI_TEXT_PRIMARY, 1);

    /* Close button */
    if (mx >= close_x && mx < wx + ww && my >= wy && my < wy + TITLEBAR_HEIGHT) {
        gui_fill_rounded_rect(close_x + 2, wy + 4, btn_w - 4, TITLEBAR_HEIGHT - 8, 4, GUI_ACCENT_RED);
        gui_draw_string(close_x + 18, wy + 10, "X", 0x00FFFFFF, 1);
    } else {
        gui_draw_string(close_x + 18, wy + 10, "X", GUI_TEXT_PRIMARY, 1);
    }

    /* Client Content Area */
    int content_x = wx + 1;
    int content_y = wy + TITLEBAR_HEIGHT;
    int content_w = ww - 2;
    int content_h = wh - TITLEBAR_HEIGHT - 1;

    if (win->on_paint) {
        win->on_paint(win, content_x, content_y, content_w, content_h);
    }
}

/* ========================================================================= */
/* Windows 11 Taskbar Dock                                                   */
/* ========================================================================= */

static void render_taskbar(void) {
    int ty = g_fb.height - TASKBAR_HEIGHT;
    int tw = g_fb.width;

    gui_fill_rect(0, ty, tw, TASKBAR_HEIGHT, 0x00101624);
    gui_draw_rect(0, ty, tw, 1, 0x001E293B);

    int dock_x = 12;
    int icon_btn_w = 40;
    int icon_btn_h = 38;
    int start_y = ty + 5;

    /* 1. Start Button */
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, g_start_menu_open ? GUI_BG_CARD_HOVER : 0x00101624);
    if (g_start_menu_open) gui_draw_rect(dock_x, start_y, icon_btn_w, icon_btn_h, GUI_ACCENT_BLUE);
    gui_fill_rect(dock_x + 11, start_y + 10, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(dock_x + 21, start_y + 10, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(dock_x + 11, start_y + 19, 8, 7, GUI_ACCENT_CYAN);
    gui_fill_rect(dock_x + 21, start_y + 19, 8, 7, GUI_ACCENT_CYAN);
    dock_x += icon_btn_w + 4;

    /* 2. Search Magnifier */
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, 0x00101624);
    gui_draw_string(dock_x + 14, start_y + 11, "O", GUI_TEXT_PRIMARY, 1);
    dock_x += icon_btn_w + 4;

    /* 3. Edge Browser */
    Window *br_win = window_get_by_tag("browser");
    uint32_t br_bg = (br_win && br_win->is_focused && !br_win->is_minimized) ? GUI_BG_CARD_HOVER : 0x00101624;
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, br_bg);
    gui_draw_fluent_icon_edge(dock_x + 5, start_y + 4);
    if (br_win && br_win->id != 0) {
        gui_fill_rounded_rect(dock_x + 14, ty + TASKBAR_HEIGHT - 3, 12, 2, 1, GUI_ACCENT_CYAN);
    }
    dock_x += icon_btn_w + 4;

    /* 4. App Store / Installer */
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, 0x00101624);
    gui_draw_fluent_icon_installer(dock_x + 5, start_y + 4);
    dock_x += icon_btn_w + 4;

    /* 5. File Explorer */
    Window *exp_win = window_get_by_tag("explorer");
    uint32_t exp_bg = (exp_win && exp_win->is_focused && !exp_win->is_minimized) ? GUI_BG_CARD_HOVER : 0x00101624;
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, exp_bg);
    gui_draw_fluent_icon_explorer(dock_x + 5, start_y + 4);
    if (exp_win && exp_win->id != 0) {
        gui_fill_rounded_rect(dock_x + 14, ty + TASKBAR_HEIGHT - 3, 12, 2, 1, GUI_ACCENT_BLUE);
    }
    dock_x += icon_btn_w + 4;

    /* 6. Chrome */
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, 0x00101624);
    gui_draw_fluent_icon_chrome(dock_x + 5, start_y + 4);
    dock_x += icon_btn_w + 4;

    /* 7. Task Manager */
    Window *tm_win = window_get_by_tag("taskmgr");
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, (tm_win && tm_win->is_focused) ? GUI_BG_CARD_HOVER : 0x00101624);
    gui_draw_fluent_icon_taskmgr(dock_x + 5, start_y + 4);
    if (tm_win && tm_win->id != 0) {
        gui_fill_rounded_rect(dock_x + 14, ty + TASKBAR_HEIGHT - 3, 12, 2, 1, GUI_ACCENT_CYAN);
    }
    dock_x += icon_btn_w + 4;

    /* 8. Antigravity IDE */
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, 0x00101624);
    gui_draw_fluent_icon_antigravity(dock_x + 5, start_y + 4);
    dock_x += icon_btn_w + 4;

    /* 9. Raw Sector Cloner */
    Window *dc_win = window_get_by_tag("diskclone");
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, (dc_win && dc_win->is_focused) ? GUI_BG_CARD_HOVER : 0x00101624);
    gui_draw_fluent_icon_diskclone(dock_x + 5, start_y + 4);
    if (dc_win && dc_win->id != 0) {
        gui_fill_rounded_rect(dock_x + 14, ty + TASKBAR_HEIGHT - 3, 12, 2, 1, GUI_ACCENT_GREEN);
    }
    dock_x += icon_btn_w + 4;

    /* 10. VLC Player */
    Window *vlc_win = window_get_by_tag("vlc");
    gui_fill_rounded_rect(dock_x, start_y, icon_btn_w, icon_btn_h, 6, (vlc_win && vlc_win->is_focused) ? GUI_BG_CARD_HOVER : 0x00101624);
    gui_draw_fluent_icon_vlc(dock_x + 5, start_y + 4);
    if (vlc_win && vlc_win->id != 0) {
        gui_fill_rounded_rect(dock_x + 14, ty + TASKBAR_HEIGHT - 3, 12, 2, 1, GUI_ACCENT_ORANGE);
    }

    /* Right System Tray */
    int tray_right = tw - 10;

    gui_fill_rounded_rect(tray_right - 26, ty + 10, 24, 28, 4, 0x00101624);
    gui_draw_string(tray_right - 20, ty + 16, "[]", GUI_TEXT_SECONDARY, 1);

    int clock_w = 88;
    int clock_x = tray_right - 30 - clock_w;
    gui_fill_rounded_rect(clock_x, ty + 4, clock_w, 40, 6, g_calendar_open ? GUI_BG_CARD_HOVER : 0x00101624);
    gui_draw_string(clock_x + 8, ty + 8, "12:19 PM", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(clock_x + 8, ty + 23, "2026-09-13", GUI_TEXT_MUTED, 1);

    int eng_x = clock_x - 38;
    gui_fill_rounded_rect(eng_x, ty + 10, 34, 28, 4, 0x00101624);
    gui_draw_string(eng_x + 6, ty + 16, "ENG", GUI_TEXT_SECONDARY, 1);

    int net_vol_x = eng_x - 76;
    gui_fill_rounded_rect(net_vol_x, ty + 8, 72, 32, 6, (g_volume_open) ? GUI_BG_CARD_HOVER : 0x00101624);
    gui_draw_string(net_vol_x + 8, ty + 15, "W", GUI_ACCENT_CYAN, 1);
    gui_draw_string(net_vol_x + 28, ty + 15, "V", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(net_vol_x + 48, ty + 15, "B", GUI_ACCENT_GREEN, 1);

    int chev_x = net_vol_x - 24;
    gui_draw_string(chev_x + 6, ty + 16, "^", GUI_TEXT_MUTED, 1);
}

/* ========================================================================= */
/* Windows 11 Start Menu & Power Flyout                                      */
/* ========================================================================= */

static void render_start_menu(void) {
    if (!g_start_menu_open) return;

    int sm_w = 480;
    int sm_h = 540;
    int sm_x = 12;
    int sm_y = g_fb.height - TASKBAR_HEIGHT - sm_h - 10;

    gui_fill_rounded_rect(sm_x, sm_y, sm_w, sm_h, 12, GUI_BG_STARTMENU);
    gui_draw_rect(sm_x, sm_y, sm_w, sm_h, GUI_ACCENT_BLUE);

    gui_fill_rounded_rect(sm_x + 20, sm_y + 18, sm_w - 40, 38, 19, GUI_BG_INPUT);
    gui_draw_rect(sm_x + 20, sm_y + 18, sm_w - 40, 38, GUI_BORDER_COLOR);
    gui_draw_string(sm_x + 38, sm_y + 30, "Type here to search...", GUI_TEXT_MUTED, 1);

    gui_draw_string(sm_x + 24, sm_y + 70, "Pinned", GUI_TEXT_PRIMARY, 1);

    const char *pinned_tags[] = {"EX", "VLC", "APP", "DCL", "TM", "SC", "CL", "EDGE"};
    const char *pinned_names[] = {
        "File Explorer", "VLC Player",   "App Installer", "Sector Cloner",
        "Task Manager",  "Security Center","Terminal",    "Microsoft Edge"
    };

    for (int i = 0; i < 8; i++) {
        int col = i % 4;
        int row = i / 4;
        int px = sm_x + 20 + col * 110;
        int py = sm_y + 96 + row * 78;

        gui_fill_rounded_rect(px, py, 102, 70, 8, GUI_BG_CARD);
        gui_draw_rect(px, py, 102, 70, GUI_BORDER_COLOR);

        gui_draw_fluent_icon_by_tag(px + 36, py + 8, pinned_tags[i]);
        gui_draw_string(px + 6, py + 48, pinned_names[i], GUI_TEXT_SECONDARY, 1);
    }

    gui_draw_string(sm_x + 24, sm_y + 276, "Recommended", GUI_TEXT_PRIMARY, 1);
    const char *recent[] = {
        "main.txt            - Desktop Document",
        "backup_disk.img     - Sector Cloner Target",
        "matrix_intro_64.mp4 - VLC Player"
    };

    for (int i = 0; i < 3; i++) {
        int ry = sm_y + 300 + i * 40;
        gui_fill_rounded_rect(sm_x + 20, ry, sm_w - 40, 34, 6, GUI_BG_CARD);
        gui_draw_string(sm_x + 36, ry + 10, recent[i], GUI_TEXT_SECONDARY, 1);
    }

    int ubar_y = sm_y + sm_h - 56;
    gui_fill_rounded_rect(sm_x, ubar_y, sm_w, 56, 12, 0x000B101E);
    gui_draw_rect(sm_x, ubar_y, sm_w, 1, GUI_BORDER_COLOR);

    gui_fill_rounded_rect(sm_x + 20, ubar_y + 11, 34, 34, 17, GUI_ACCENT_BLUE);
    gui_draw_string(sm_x + 30, ubar_y + 19, "A", 0x00FFFFFF, 1);
    gui_draw_string(sm_x + 64, ubar_y + 14, "Administrator", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(sm_x + 64, ubar_y + 30, "Ring 0 Full Privilege", GUI_ACCENT_GREEN, 1);

    gui_fill_rounded_rect(sm_x + sm_w - 48, ubar_y + 11, 34, 34, 6, g_power_menu_open ? GUI_BG_CARD_HOVER : GUI_BG_CARD);
    gui_draw_string(sm_x + sm_w - 36, ubar_y + 19, "P", GUI_ACCENT_RED, 1);

    if (g_power_menu_open) {
        int pw_w = 160;
        int pw_h = 120;
        int pw_x = sm_x + sm_w - pw_w - 10;
        int pw_y = ubar_y - pw_h - 6;

        gui_fill_rounded_rect(pw_x, pw_y, pw_w, pw_h, 8, 0x00131C2C);
        gui_draw_rect(pw_x, pw_y, pw_w, pw_h, GUI_ACCENT_RED);

        gui_fill_rounded_rect(pw_x + 6, pw_y + 8, pw_w - 12, 32, 4, GUI_BG_CARD);
        gui_draw_string(pw_x + 16, pw_y + 16, "zZ  Hibernate / Sleep", GUI_ACCENT_CYAN, 1);

        gui_fill_rounded_rect(pw_x + 6, pw_y + 44, pw_w - 12, 32, 4, GUI_BG_CARD);
        gui_draw_string(pw_x + 16, pw_y + 52, "(!) Shut down (ACPI)", GUI_ACCENT_RED, 1);

        gui_fill_rounded_rect(pw_x + 6, pw_y + 80, pw_w - 12, 32, 4, GUI_BG_CARD);
        gui_draw_string(pw_x + 16, pw_y + 88, "[R] Restart (Reset)", GUI_ACCENT_AMBER, 1);
    }
}

static void render_calendar_popup(void) {
    if (!g_calendar_open) return;

    int cal_w = 280;
    int cal_h = 300;
    int cal_x = g_fb.width - cal_w - 12;
    int cal_y = g_fb.height - TASKBAR_HEIGHT - cal_h - 10;

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
    int vol_x = g_fb.width - vol_w - 120;
    int vol_y = g_fb.height - TASKBAR_HEIGHT - vol_h - 10;

    gui_fill_rounded_rect(vol_x, vol_y, vol_w, vol_h, 12, GUI_BG_STARTMENU);
    gui_draw_rect(vol_x, vol_y, vol_w, vol_h, GUI_ACCENT_BLUE);

    gui_draw_string(vol_x + 18, vol_y + 16, "Master Audio Output", GUI_TEXT_PRIMARY, 1);

    gui_fill_rounded_rect(vol_x + 18, vol_y + 46, 180, 8, 4, GUI_BG_INPUT);
    gui_fill_rounded_rect(vol_x + 18, vol_y + 46, (180 * g_volume_level) / 100, 8, 4, GUI_ACCENT_BLUE);
    gui_fill_rounded_rect(vol_x + 14 + (180 * g_volume_level) / 100, vol_y + 40, 16, 20, 8, 0x00FFFFFF);

    char num[8];
    uint_to_str(g_volume_level, num);
    strcat(num, "%");
    gui_draw_string(vol_x + 210, vol_y + 42, num, GUI_TEXT_PRIMARY, 1);
}

static void render_suspended_overlay(void) {
    gui_fill_rect(0, 0, g_fb.width, g_fb.height, 0x00020408);
    gui_draw_string(g_fb.width / 2 - 180, g_fb.height / 2 - 20, "Xenithra OS Suspended (Low-Power ACPI S3)", GUI_ACCENT_CYAN, 1);
    gui_draw_string(g_fb.width / 2 - 140, g_fb.height / 2 + 10, "Click mouse or press key to resume...", GUI_TEXT_MUTED, 1);
}

static void render_mouse_cursor(void) {
    int mx = g_mouse.x;
    int my = g_mouse.y;

    for (int row = 0; row < 18; row++) {
        uint16_t bits = cursor_shadow[row];
        for (int col = 0; col < 12; col++) {
            if ((bits >> (15 - col)) & 1) {
                gui_put_pixel(mx + col + 1, my + row + 1, 0x00060A12);
            }
        }
    }

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

    if (g_is_suspended) {
        render_suspended_overlay();
        render_mouse_cursor();
    } else {
        /* 1. Desktop 3D Bloom Wallpaper */
        render_wallpaper();

        /* 2. Desktop Shortcuts */
        render_desktop_icons();

        /* 3. Render Windows in Z-Order */
        for (int z = 0; z < g_window_count + 16; z++) {
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
    }

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

    if (g_is_suspended && left_pressed) {
        g_is_suspended = 0;
        compositor_render();
        g_mouse.prev_left = left_btn;
        return;
    }

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

            /* 1. Start button click */
            if (x >= 12 && x <= 52) {
                compositor_toggle_start_menu();
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 2. Search / Terminal */
            if (x >= 56 && x <= 96) {
                terminal_app_launch();
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 3. Edge / Browser */
            if (x >= 100 && x <= 140) {
                Window *w = window_get_by_tag("browser");
                if (w) {
                    if (w->is_minimized) window_restore(w);
                    else if (w->is_focused) window_minimize(w);
                    else window_focus(w);
                } else {
                    browser_app_launch();
                }
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 4. App Store / Installer */
            if (x >= 144 && x <= 184) {
                Window *w = window_get_by_tag("installer");
                if (w) {
                    if (w->is_minimized) window_restore(w);
                    else if (w->is_focused) window_minimize(w);
                    else window_focus(w);
                } else {
                    installer_app_launch();
                }
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 5. Explorer icon click */
            if (x >= 188 && x <= 228) {
                Window *w = window_get_by_tag("explorer");
                if (w) {
                    if (w->is_minimized) window_restore(w);
                    else if (w->is_focused) window_minimize(w);
                    else window_focus(w);
                } else {
                    explorer_app_launch();
                }
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 6. Chrome */
            if (x >= 232 && x <= 272) {
                terminal_app_launch();
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 7. Task Manager click */
            if (x >= 276 && x <= 316) {
                Window *w = window_get_by_tag("taskmgr");
                if (w) {
                    if (w->is_minimized) window_restore(w);
                    else if (w->is_focused) window_minimize(w);
                    else window_focus(w);
                } else {
                    taskmgr_app_launch();
                }
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 8. Antigravity IDE */
            if (x >= 320 && x <= 360) {
                terminal_app_launch();
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 9. Sector Cloner click */
            if (x >= 364 && x <= 404) {
                Window *w = window_get_by_tag("diskclone");
                if (w) {
                    if (w->is_minimized) window_restore(w);
                    else if (w->is_focused) window_minimize(w);
                    else window_focus(w);
                } else {
                    diskclone_app_launch();
                }
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* 10. VLC click */
            if (x >= 408 && x <= 448) {
                Window *w = window_get_by_tag("vlc");
                if (w) {
                    if (w->is_minimized) window_restore(w);
                    else if (w->is_focused) window_minimize(w);
                    else window_focus(w);
                } else {
                    vlc_app_launch();
                }
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* Clock click */
            if (x >= tw - 120 && x <= tw - 34) {
                compositor_toggle_calendar();
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            /* Volume/WiFi click */
            if (x >= tw - 240 && x <= tw - 160) {
                compositor_toggle_volume();
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }
        }

        /* 2. Check Start Menu & Power Menu Clicks */
        if (g_start_menu_open) {
            int sm_w = 480;
            int sm_h = 540;
            int sm_x = 12;
            int sm_y = g_fb.height - TASKBAR_HEIGHT - sm_h - 10;

            int ubar_y = sm_y + sm_h - 56;
            if (x >= sm_x + sm_w - 48 && x <= sm_x + sm_w - 14 && y >= ubar_y + 11 && y <= ubar_y + 45) {
                compositor_toggle_power_menu();
                g_mouse.prev_left = left_btn;
                compositor_render();
                return;
            }

            if (g_power_menu_open) {
                int pw_w = 160;
                int pw_h = 120;
                int pw_x = sm_x + sm_w - pw_w - 10;
                int pw_y = ubar_y - pw_h - 6;

                if (x >= pw_x && x <= pw_x + pw_w && y >= pw_y && y <= pw_y + pw_h) {
                    if (y >= pw_y + 8 && y <= pw_y + 40) {
                        g_start_menu_open = 0;
                        g_power_menu_open = 0;
                        system_hibernate();
                        return;
                    }
                    if (y >= pw_y + 44 && y <= pw_y + 76) {
                        system_shutdown();
                        return;
                    }
                    if (y >= pw_y + 80 && y <= pw_y + 112) {
                        system_reboot();
                        return;
                    }
                }
            }

            if (x >= sm_x && x <= sm_x + sm_w && y >= sm_y && y <= sm_y + sm_h) {
                for (int i = 0; i < 8; i++) {
                    int col = i % 4;
                    int row = i / 4;
                    int px = sm_x + 20 + col * 110;
                    int py = sm_y + 96 + row * 78;

                    if (x >= px && x <= px + 102 && y >= py && y <= py + 70) {
                        if (i == 0) explorer_app_launch();
                        else if (i == 1) vlc_app_launch();
                        else if (i == 2) installer_app_launch();
                        else if (i == 3) diskclone_app_launch();
                        else if (i == 4) taskmgr_app_launch();
                        else if (i == 5) firewall_app_launch();
                        else if (i == 6) terminal_app_launch();
                        else if (i == 7) browser_app_launch();

                        g_start_menu_open = 0;
                        g_power_menu_open = 0;
                        g_mouse.prev_left = left_btn;
                        compositor_render();
                        return;
                    }
                }
            } else {
                g_start_menu_open = 0;
                g_power_menu_open = 0;
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
                    g_drag_window = clicked_win;
                    g_drag_offset_x = x - clicked_win->x;
                    g_drag_offset_y = y - clicked_win->y;
                }
            } else {
                if (clicked_win->on_mouse) {
                    int rel_x = x - clicked_win->x;
                    int rel_y = y - clicked_win->y;
                    clicked_win->on_mouse(clicked_win, rel_x, rel_y, 1, 0);
                }
            }
        } else {
            /* 4. Check Desktop Icon Clicks */
            int start_y = 16;
            int row_h = 72;
            int col_w = 84;
            int left_col0_x = 16;
            int left_col1_x = 110;
            int right_col3_x = g_fb.width - 98;
            int right_col2_x = g_fb.width - 200;

            for (int i = 0; i < MAX_DESKTOP_ICONS; i++) {
                DesktopIcon *ic = &g_desktop_icons[i];
                int ix = left_col0_x;
                int iy = start_y + ic->row * row_h;
                if (ic->col == 0)      ix = left_col0_x;
                else if (ic->col == 1) ix = left_col1_x;
                else if (ic->col == 2) ix = right_col2_x;
                else if (ic->col == 3) ix = right_col3_x;

                if (x >= ix - 4 && x <= ix + col_w && y >= iy - 2 && y <= iy + row_h - 4) {
                    if (g_selected_desktop_icon == i) {
                        if (ic->on_activate) {
                            ic->on_activate();
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
    if (g_is_suspended) {
        g_is_suspended = 0;
        compositor_render();
        return;
    }
    Window *win = window_get_focused();
    if (win && win->on_key) {
        win->on_key(win, ascii, scancode);
        compositor_render();
    }
}
