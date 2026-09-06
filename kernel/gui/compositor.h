/**
 * @file compositor.h
 * @brief Xenithra OS 32-bit Window Compositor & Desktop Environment
 */

#ifndef _KERNEL_GUI_COMPOSITOR_H_
#define _KERNEL_GUI_COMPOSITOR_H_

#include <stdint.h>
#include <stddef.h>
#include "../../shared/bootinfo.h"

#define MAX_WINDOWS 16
#define TITLEBAR_HEIGHT 32
#define TASKBAR_HEIGHT  48

/* Modern Fluent Dark Palette */
#define GUI_BG_WALLPAPER    0x000F172A /* Slate Deep Blue */
#define GUI_BG_DESKTOP_GRAD 0x001E293B /* Mica Slate */
#define GUI_BG_TASKBAR      0x000F172AE0 /* Acrylic Dark */
#define GUI_BG_WINDOW       0x001E293B /* Window Card */
#define GUI_BG_TITLEBAR     0x000F172A /* Titlebar Dark */
#define GUI_ACCENT_BLUE     0x000078D4 /* Fluent Blue */
#define GUI_ACCENT_CYAN     0x0006B6D4 /* Cyan Highlight */
#define GUI_ACCENT_GREEN    0x0010B981 /* Emerald Green */
#define GUI_ACCENT_RED      0x00EF4444 /* Crimson Close */
#define GUI_TEXT_WHITE      0x00F8FAFC /* Pure White */
#define GUI_TEXT_MUTED      0x0094A3B8 /* Muted Slate */
#define GUI_BORDER_COLOR    0x00334155 /* Subtle Border */

typedef struct Window Window;
typedef void (*WindowPaintCallback)(Window *win, int content_x, int content_y, int content_w, int content_h);

struct Window {
    uint32_t id;
    char title[64];
    int x, y, width, height;
    
    /* State flags */
    uint8_t is_minimized;
    uint8_t is_maximized;
    uint8_t is_focused;
    int saved_x, saved_y, saved_w, saved_h;
    
    int z_order;
    WindowPaintCallback on_paint;
    void *user_data;
};

typedef struct {
    int x;
    int y;
    uint8_t left_button;
    uint8_t right_button;
} MouseState;

/* Compositor API */
void compositor_init(XenithraFrameBuffer fb);
void compositor_render(void);

/* Window Management API */
Window* window_create(const char *title, int x, int y, int w, int h, WindowPaintCallback on_paint, void *user_data);
void window_destroy(uint32_t win_id);
void window_maximize(Window *win);
void window_minimize(Window *win);
void window_restore(Window *win);
void window_focus(Window *win);
void window_move(Window *win, int new_x, int new_y);

/* Input & Interaction */
void compositor_set_mouse(int x, int y, uint8_t left_btn, uint8_t right_btn);
void compositor_toggle_start_menu(void);

/* Drawing Primitives */
void gui_fill_rect(int x, int y, int w, int h, uint32_t color);
void gui_draw_rect(int x, int y, int w, int h, uint32_t color);
void gui_fill_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
void gui_draw_string(int x, int y, const char *str, uint32_t color, int scale);
void gui_draw_gradient_v(int x, int y, int w, int h, uint32_t top_color, uint32_t bot_color);

#endif /* _KERNEL_GUI_COMPOSITOR_H_ */
