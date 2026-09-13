/**
 * @file compositor.h
 * @brief Xenithra OS Windows 11 Fluent Window Compositor & Desktop Environment
 */

#ifndef _KERNEL_GUI_COMPOSITOR_H_
#define _KERNEL_GUI_COMPOSITOR_H_

#include <stdint.h>
#include <stddef.h>
#include "../../shared/bootinfo.h"

#define MAX_WINDOWS         16
#define TITLEBAR_HEIGHT     36
#define TASKBAR_HEIGHT      48
#define MAX_DESKTOP_ICONS   24

/* Windows 11 Fluent Dark Mica & Acrylic Palette */
#define GUI_BG_WALLPAPER_TOP    0x00060A14 /* Deep Midnight Navy */
#define GUI_BG_WALLPAPER_MID    0x000E172A /* Slate 900 Ambient */
#define GUI_BG_WALLPAPER_BOT    0x0004070D /* Obsidian Abyss */

/* Windows 11 Bloom Petal Colors */
#define GUI_BLOOM_BLUE_LIGHT    0x0060A5FA /* Soft Sky Blue */
#define GUI_BLOOM_BLUE_MID      0x002563EB /* Radiant Cobalt */
#define GUI_BLOOM_BLUE_DEEP     0x001D4ED8 /* Deep Royal Sapphire */
#define GUI_BLOOM_CYAN          0x0038BDF8 /* Electric Cyan */
#define GUI_BLOOM_INDIGO        0x004F46E5 /* Electric Indigo */
#define GUI_BLOOM_PURPLE        0x007C3AED /* Vivid Violet */
#define GUI_BLOOM_PINK          0x00C084FC /* Soft Orchid */

/* Surface & Material Tints */
#define GUI_BG_TASKBAR          0x00101726 /* Acrylic Translucent Dock */
#define GUI_BG_TASKBAR_BORDER   0x0024344E /* Glowing Dock Border */
#define GUI_BG_STARTMENU        0x00111A2D /* Acrylic Start Menu */
#define GUI_BG_WINDOW           0x000F172A /* Mica Dark Window Body */
#define GUI_BG_TITLEBAR_ACT     0x0018243A /* Active Window Header */
#define GUI_BG_TITLEBAR_INACT   0x00101827 /* Inactive Window Header */
#define GUI_BG_CARD             0x00172238 /* Card Surface */
#define GUI_BG_CARD_HOVER       0x00223250 /* Card Hover State */
#define GUI_BG_INPUT            0x000B111F /* Input Box Surface */

/* Vibrant Windows 11 Accents */
#define GUI_ACCENT_BLUE         0x000078D4 /* Windows Fluent Blue */
#define GUI_ACCENT_CYAN         0x0000A8FF /* Bright Sky Blue */
#define GUI_ACCENT_TEAL         0x0006B6D4 /* Teal Indicator */
#define GUI_ACCENT_GREEN        0x0010B981 /* Emerald Online/Secure */
#define GUI_ACCENT_RED          0x00E81123 /* Close Button Crimson */
#define GUI_ACCENT_ORANGE       0x00F97316 /* VLC Cone Orange */
#define GUI_ACCENT_AMBER        0x00F59E0B /* Warning Amber */
#define GUI_ACCENT_PURPLE       0x008B5CF6 /* Violet Accent */

/* Typography & Borders */
#define GUI_TEXT_PRIMARY        0x00FFFFFF /* Pure White */
#define GUI_TEXT_SECONDARY      0x00CBD5E1 /* Slate 300 */
#define GUI_TEXT_MUTED          0x0064748B /* Slate 500 */
#define GUI_BORDER_COLOR        0x0025354F /* Subtle Surface Border */
#define GUI_BORDER_FOCUS        0x000078D4 /* Focused Border */

typedef struct Window Window;
typedef void (*WindowPaintCallback)(Window *win, int content_x, int content_y, int content_w, int content_h);
typedef void (*WindowKeyCallback)(Window *win, char ascii, uint8_t scancode);
typedef void (*WindowMouseCallback)(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click);

struct Window {
    uint32_t id;
    char title[64];
    char app_tag[16];
    int x, y, width, height;
    
    /* State flags */
    uint8_t is_minimized;
    uint8_t is_maximized;
    uint8_t is_focused;
    uint8_t is_visible;
    int saved_x, saved_y, saved_w, saved_h;
    
    int z_order;
    WindowPaintCallback on_paint;
    WindowKeyCallback   on_key;
    WindowMouseCallback on_mouse;
    void *user_data;
};

typedef struct {
    int x;
    int y;
    uint8_t left_button;
    uint8_t right_button;
    uint8_t middle_button;
    uint8_t prev_left;
    uint8_t prev_right;
} CompositorMouseState;

typedef struct {
    const char *title;
    const char *icon_tag;
    uint32_t icon_color;
    int col; /* 0: Left col 1, 1: Left col 2, 2: Right col 1, 3: Right col 2 */
    int row;
    void (*on_activate)(void);
} DesktopIcon;

/* Compositor Core API */
void compositor_init(XenithraFrameBuffer fb);
void compositor_render(void);
void compositor_update_mouse(int x, int y, uint8_t left_btn, uint8_t right_btn, uint8_t mid_btn);
void compositor_dispatch_key(char ascii, uint8_t scancode, uint8_t is_pressed);
void compositor_tick(void);

/* Window Management API */
Window* window_create(const char *title, const char *app_tag, int x, int y, int w, int h, WindowPaintCallback on_paint, void *user_data);
void window_set_callbacks(Window *win, WindowKeyCallback on_key, WindowMouseCallback on_mouse);
void window_destroy(uint32_t win_id);
void window_maximize(Window *win);
void window_minimize(Window *win);
void window_restore(Window *win);
void window_focus(Window *win);
void window_move(Window *win, int new_x, int new_y);
Window* window_get_by_tag(const char *app_tag);
Window* window_get_focused(void);

/* UI Controls, Popups & Power Management */
void compositor_toggle_start_menu(void);
void compositor_toggle_calendar(void);
void compositor_toggle_volume(void);
void compositor_toggle_power_menu(void);
void system_shutdown(void);
void system_reboot(void);
void system_hibernate(void);

/* Drawing Primitives & Fluent Graphics */
void gui_put_pixel(int x, int y, uint32_t color);
void gui_fill_rect(int x, int y, int w, int h, uint32_t color);
void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
void gui_fill_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
void gui_draw_string(int x, int y, const char *str, uint32_t color, int scale);
void gui_draw_string_shadow(int x, int y, const char *str, uint32_t color, uint32_t shadow_color, int scale);
void gui_draw_gradient_v(int x, int y, int w, int h, uint32_t top_color, uint32_t bot_color);
void gui_draw_icon_badge(int x, int y, const char *symbol, uint32_t bg_color, uint32_t fg_color);

/* Windows 11 Fluent Procedural Vector Icon Renderers */
void gui_draw_fluent_icon_this_pc(int x, int y);
void gui_draw_fluent_icon_explorer(int x, int y);
void gui_draw_fluent_icon_vlc(int x, int y);
void gui_draw_fluent_icon_installer(int x, int y);
void gui_draw_fluent_icon_taskmgr(int x, int y);
void gui_draw_fluent_icon_security(int x, int y);
void gui_draw_fluent_icon_terminal(int x, int y);
void gui_draw_fluent_icon_recycle(int x, int y);
void gui_draw_fluent_icon_edge(int x, int y);
void gui_draw_fluent_icon_chrome(int x, int y);
void gui_draw_fluent_icon_postman(int x, int y);
void gui_draw_fluent_icon_antigravity(int x, int y);
void gui_draw_fluent_icon_modbus(int x, int y);
void gui_draw_fluent_icon_text_doc(int x, int y);
void gui_draw_fluent_icon_batch_file(int x, int y);
void gui_draw_fluent_icon_by_tag(int x, int y, const char *tag);

#endif /* _KERNEL_GUI_COMPOSITOR_H_ */
