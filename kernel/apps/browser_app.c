/**
 * @file browser_app.c
 * @brief Microsoft Edge / React-based Web Browser App with Google UI & V8 Bridge
 */

#include "browser_app.h"
#include "../kstring.h"
#include "../gui/v8_engine.h"
#include "firewall_app.h"
#include "terminal_app.h"

typedef enum {
    TAB_GOOGLE = 0,
    TAB_DOCS   = 1,
    TAB_CUSTOM = 2
} BrowserTabType;

static Window *g_browser_win = NULL;
static BrowserTabType g_active_tab = TAB_GOOGLE;
static char g_url_buffer[128] = "https://www.google.com";
static char g_search_query[64] = "Xenithra OS x86_64 V8 Engine";
static uint8_t g_focus_input = 1; /* 0: none, 1: search, 2: url */
static uint8_t g_is_loading = 0;

static void on_browser_mouse(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click) {
    (void)win;
    (void)right_click;
    if (!left_click) return;

    /* 1. Tab Bar (Y: 0 - 32) */
    if (rel_y >= 0 && rel_y <= 32) {
        if (rel_x >= 8 && rel_x <= 160) {
            g_active_tab = TAB_GOOGLE;
            strcpy(g_url_buffer, "https://www.google.com");
            return;
        }
        if (rel_x >= 165 && rel_x <= 340) {
            g_active_tab = TAB_DOCS;
            strcpy(g_url_buffer, "https://docs.xenithra.org/kernel");
            return;
        }
    }

    /* 2. Navigation Bar & Omnibox (Y: 34 - 70) */
    if (rel_y >= 34 && rel_y <= 70) {
        /* Back Button */
        if (rel_x >= 10 && rel_x <= 36) {
            g_active_tab = TAB_GOOGLE;
            strcpy(g_url_buffer, "https://www.google.com");
            return;
        }
        /* Forward Button */
        if (rel_x >= 40 && rel_x <= 66) {
            g_active_tab = TAB_DOCS;
            strcpy(g_url_buffer, "https://docs.xenithra.org/kernel");
            return;
        }
        /* Reload Button */
        if (rel_x >= 70 && rel_x <= 96) {
            g_is_loading = 1;
            return;
        }
        /* URL Omnibox (X: 106 - 680) */
        if (rel_x >= 106 && rel_x <= 680) {
            g_focus_input = 2;
            return;
        }
    }

    /* 3. Bookmarks Toolbar (Y: 72 - 96) */
    if (rel_y >= 72 && rel_y <= 96) {
        /* Bookmark 1: Google */
        if (rel_x >= 12 && rel_x <= 80) {
            g_active_tab = TAB_GOOGLE;
            strcpy(g_url_buffer, "https://www.google.com");
            return;
        }
        /* Bookmark 2: GitHub */
        if (rel_x >= 85 && rel_x <= 150) {
            g_active_tab = TAB_CUSTOM;
            strcpy(g_url_buffer, "https://github.com/xenithra/kernel");
            return;
        }
        /* Bookmark 3: OSDev */
        if (rel_x >= 155 && rel_x <= 245) {
            g_active_tab = TAB_CUSTOM;
            strcpy(g_url_buffer, "https://wiki.osdev.org");
            return;
        }
        /* Bookmark 4: Kernel Portal */
        if (rel_x >= 250 && rel_x <= 370) {
            firewall_app_launch();
            return;
        }
    }

    /* 4. Google Page Content Interactions */
    if (g_active_tab == TAB_GOOGLE) {
        /* Google Search Input Bar (Y: 200 - 244, X: (win_w-500)/2 .. ) */
        int content_w = win->width;
        int search_box_w = 480;
        int search_box_x = (content_w - search_box_w) / 2;
        if (rel_y >= 200 && rel_y <= 244 && rel_x >= search_box_x && rel_x <= search_box_x + search_box_w) {
            g_focus_input = 1;
            return;
        }

        /* Search Buttons (Y: 254 - 286) */
        if (rel_y >= 254 && rel_y <= 286) {
            /* Google Search Button */
            if (rel_x >= search_box_x + 60 && rel_x <= search_box_x + 210) {
                g_active_tab = TAB_CUSTOM;
                strcpy(g_url_buffer, "https://www.google.com/search?q=xenithra+v8+kernel");
                return;
            }
            /* I'm Feeling Lucky */
            if (rel_x >= search_box_x + 230 && rel_x <= search_box_x + 410) {
                terminal_app_launch();
                return;
            }
        }

        /* Quick Shortcut Cards (Y: 310 - 390) */
        if (rel_y >= 310 && rel_y <= 390) {
            int card_w = 110;
            int start_x = (content_w - (4 * card_w + 3 * 16)) / 2;
            for (int i = 0; i < 4; i++) {
                int cx = start_x + i * (card_w + 16);
                if (rel_x >= cx && rel_x <= cx + card_w) {
                    if (i == 0) { g_active_tab = TAB_CUSTOM; strcpy(g_url_buffer, "https://github.com/xenithra"); }
                    if (i == 1) { g_active_tab = TAB_DOCS; strcpy(g_url_buffer, "https://v8.dev/docs"); }
                    if (i == 2) { g_active_tab = TAB_DOCS; strcpy(g_url_buffer, "https://learn.microsoft.com/dwm"); }
                    if (i == 3) { firewall_app_launch(); }
                    return;
                }
            }
        }
    }
}

static void on_browser_key(Window *win, char ascii, uint8_t scancode) {
    (void)win;
    (void)scancode;

    if (ascii == 0) return;

    char *target_str = (g_focus_input == 2) ? g_url_buffer : g_search_query;
    size_t max_len = (g_focus_input == 2) ? sizeof(g_url_buffer) : sizeof(g_search_query);

    size_t len = strlen(target_str);

    if (ascii == '\b') {
        if (len > 0) {
            target_str[len - 1] = '\0';
        }
    } else if (ascii == '\r' || ascii == '\n') {
        if (g_focus_input == 1) {
            /* Execute Google Search */
            g_active_tab = TAB_CUSTOM;
            strcpy(g_url_buffer, "https://www.google.com/search?q=");
            strcat(g_url_buffer, g_search_query);
        } else {
            g_active_tab = TAB_CUSTOM;
        }
    } else if (ascii >= 32 && ascii <= 126) {
        if (len + 1 < max_len) {
            target_str[len] = ascii;
            target_str[len + 1] = '\0';
        }
    }
}

static void render_browser_window(Window *win, int cx, int cy, int cw, int ch) {
    /* 1. Main Background - Deep Fluent Navy */
    gui_fill_rect(cx, cy, cw, ch, 0x000B111E);

    /* 2. Modern Tab Strip (Y: 0 - 32) */
    gui_fill_rect(cx, cy, cw, 34, 0x000F172A);
    gui_draw_rect(cx, cy + 33, cw, 1, 0x001E293B);

    /* Tab 1: Google (Active if TAB_GOOGLE) */
    uint32_t tab1_bg = (g_active_tab == TAB_GOOGLE) ? 0x001E293B : 0x000F172A;
    gui_fill_rounded_rect(cx + 8, cy + 4, 150, 28, 6, tab1_bg);
    gui_fill_rounded_rect(cx + 14, cy + 10, 14, 14, 3, 0x002563EB); /* Blue Edge/G icon */
    gui_draw_string(cx + 18, cy + 9, "G", 0x00FFFFFF, 1);
    gui_draw_string(cx + 34, cy + 10, "Google", (g_active_tab == TAB_GOOGLE) ? 0x00FFFFFF : 0x0094A3B8, 1);
    gui_draw_string(cx + 142, cy + 10, "x", 0x0064748B, 1);

    /* Tab 2: Xenithra OS Docs */
    uint32_t tab2_bg = (g_active_tab == TAB_DOCS) ? 0x001E293B : 0x000F172A;
    gui_fill_rounded_rect(cx + 162, cy + 4, 170, 28, 6, tab2_bg);
    gui_fill_rounded_rect(cx + 168, cy + 10, 14, 14, 3, 0x00059669);
    gui_draw_string(cx + 172, cy + 9, "D", 0x00FFFFFF, 1);
    gui_draw_string(cx + 188, cy + 10, "Xenithra Docs", (g_active_tab == TAB_DOCS) ? 0x00FFFFFF : 0x0094A3B8, 1);
    gui_draw_string(cx + 318, cy + 10, "x", 0x0064748B, 1);

    /* New Tab (+) Button */
    gui_fill_rounded_rect(cx + 338, cy + 8, 22, 22, 4, 0x001E293B);
    gui_draw_string(cx + 345, cy + 10, "+", 0x0094A3B8, 1);

    /* 3. Navigation Bar & Omnibox (Y: 34 - 72) */
    gui_fill_rect(cx, cy + 34, cw, 38, 0x00131D31);
    gui_draw_rect(cx, cy + 71, cw, 1, 0x001E293B);

    /* Back / Forward / Refresh buttons */
    gui_fill_rounded_rect(cx + 8, cy + 40, 26, 26, 4, 0x001E293B);
    gui_draw_string(cx + 16, cy + 45, "<", 0x0094A3B8, 1);

    gui_fill_rounded_rect(cx + 38, cy + 40, 26, 26, 4, 0x001E293B);
    gui_draw_string(cx + 47, cy + 45, ">", 0x0094A3B8, 1);

    gui_fill_rounded_rect(cx + 68, cy + 40, 26, 26, 4, 0x001E293B);
    gui_draw_string(cx + 76, cy + 45, "R", 0x0038BDF8, 1);

    /* Omnibox / Address Bar */
    int omni_w = cw - 170;
    if (omni_w < 200) omni_w = 200;
    gui_fill_rounded_rect(cx + 100, cy + 39, omni_w, 28, 6, 0x000A0F1D);
    gui_draw_rect(cx + 100, cy + 39, omni_w, 28, (g_focus_input == 2) ? 0x000078D4 : 0x0024344E);

    /* Security Lock Badge */
    gui_fill_rounded_rect(cx + 106, cy + 44, 48, 18, 3, 0x00064E3B);
    gui_draw_string(cx + 110, cy + 45, "SSL", 0x0034D399, 1);

    /* URL Text */
    gui_draw_string(cx + 160, cy + 45, g_url_buffer, 0x00E2E8F0, 1);

    /* Star Bookmark Icon */
    gui_draw_string(cx + 100 + omni_w - 24, cy + 45, "*", 0x00FBBF24, 1);

    /* Extension Pill / Profile */
    gui_fill_rounded_rect(cx + cw - 60, cy + 40, 52, 26, 13, 0x001E293B);
    gui_draw_string(cx + cw - 48, cy + 45, "V8", 0x0038BDF8, 1);

    /* 4. Bookmarks Bar (Y: 72 - 96) */
    gui_fill_rect(cx, cy + 72, cw, 24, 0x000E1726);
    gui_draw_rect(cx, cy + 95, cw, 1, 0x001B263B);

    gui_draw_string(cx + 14, cy + 76, "[G] Google", 0x0094A3B8, 1);
    gui_draw_string(cx + 94, cy + 76, "[Git] GitHub", 0x0094A3B8, 1);
    gui_draw_string(cx + 180, cy + 76, "[OS] OSDev", 0x0094A3B8, 1);
    gui_draw_string(cx + 260, cy + 76, "[W] Wikipedia", 0x0094A3B8, 1);
    gui_draw_string(cx + 360, cy + 76, "[Sec] Firewall Center", 0x0038BDF8, 1);

    /* 5. Webpage Body Area (Y: 96 - ch) */
    int body_y = cy + 96;
    int body_h = ch - 96;

    if (g_active_tab == TAB_GOOGLE) {
        /* Vibrant Google Multi-Color Centered Logo */
        int logo_y = body_y + 40;
        int logo_x = cx + (cw - 240) / 2;

        gui_draw_string(logo_x,       logo_y, "G", 0x004285F4, 3); /* Blue */
        gui_draw_string(logo_x + 36,  logo_y, "o", 0x00EA4335, 3); /* Red */
        gui_draw_string(logo_x + 72,  logo_y, "o", 0x00FBBC05, 3); /* Yellow */
        gui_draw_string(logo_x + 108, logo_y, "g", 0x004285F4, 3); /* Blue */
        gui_draw_string(logo_x + 144, logo_y, "l", 0x0034A853, 3); /* Green */
        gui_draw_string(logo_x + 172, logo_y, "e", 0x00EA4335, 3); /* Red */

        /* Search Input Box */
        int search_w = 480;
        if (search_w > cw - 40) search_w = cw - 40;
        int search_x = cx + (cw - search_w) / 2;
        int search_y = body_y + 110;

        gui_fill_rounded_rect(search_x, search_y, search_w, 42, 21, 0x00172238);
        gui_draw_rect(search_x, search_y, search_w, 42, (g_focus_input == 1) ? 0x0038BDF8 : 0x002B3D5E);

        gui_draw_string(search_x + 18, search_y + 12, "Q", 0x0094A3B8, 1);
        gui_draw_string(search_x + 42, search_y + 12, g_search_query, 0x00FFFFFF, 1);
        if (g_focus_input == 1) {
            int q_len = strlen(g_search_query);
            gui_fill_rect(search_x + 42 + q_len * 8 + 2, search_y + 10, 2, 20, 0x0038BDF8);
        }

        /* Action Buttons */
        int btn_y = search_y + 54;
        gui_fill_rounded_rect(search_x + 70, btn_y, 150, 32, 6, 0x001E293B);
        gui_draw_string(search_x + 92, btn_y + 8, "Google Search", 0x00CBD5E1, 1);

        gui_fill_rounded_rect(search_x + 240, btn_y, 160, 32, 6, 0x001E293B);
        gui_draw_string(search_x + 256, btn_y + 8, "I'm Feeling Lucky", 0x00CBD5E1, 1);

        /* Fast Shortcuts Grid */
        int grid_y = btn_y + 55;
        int card_w = 116;
        int total_grid_w = 4 * card_w + 3 * 16;
        int start_grid_x = cx + (cw - total_grid_w) / 2;

        const char *titles[] = {"GitHub Repo", "V8 JIT Docs", "DirectX/DWM", "Security Guard"};
        const char *icons[]  = {"[Git]", "[V8]", "[DX12]", "[Sec]"};
        uint32_t colors[]    = {0x002563EB, 0x00D97706, 0x007C3AED, 0x00059669};

        for (int i = 0; i < 4; i++) {
            int gx = start_grid_x + i * (card_w + 16);
            gui_fill_rounded_rect(gx, grid_y, card_w, 75, 8, 0x00131D31);
            gui_draw_rect(gx, grid_y, card_w, 75, 0x0024344E);

            gui_fill_rounded_rect(gx + 42, grid_y + 10, 32, 32, 16, colors[i]);
            gui_draw_string(gx + 46, grid_y + 18, icons[i], 0x00FFFFFF, 1);
            gui_draw_string(gx + 12, grid_y + 50, titles[i], 0x0094A3B8, 1);
        }

    } else if (g_active_tab == TAB_DOCS) {
        /* Documentation Reader View */
        gui_fill_rect(cx + 10, body_y + 10, cw - 20, body_h - 20, 0x000F172A);
        gui_draw_rect(cx + 10, body_y + 10, cw - 20, body_h - 20, 0x001E293B);

        gui_draw_string(cx + 25, body_y + 25, "Xenithra OS Architecture & V8 Runtime Engine", 0x0038BDF8, 2);
        gui_draw_string(cx + 25, body_y + 60, "------------------------------------------------------------", 0x00334155, 1);
        gui_draw_string(cx + 25, body_y + 80, "- V8 JavaScript JIT Engine hooked via native DOM reconciler", 0x00E2E8F0, 1);
        gui_draw_string(cx + 25, body_y + 105, "- Hardware Session Hijacking Guard & SMEP/SMAP Active", 0x00E2E8F0, 1);
        gui_draw_string(cx + 25, body_y + 130, "- Modern Desktop Window Manager Compositor & Acrylic Blur", 0x00E2E8F0, 1);
        gui_draw_string(cx + 25, body_y + 155, "- Direct2D / DirectX 12 Flip Model SwapChain Pipeline", 0x00E2E8F0, 1);

    } else {
        /* Custom Web Result / URL View */
        gui_fill_rect(cx + 10, body_y + 10, cw - 20, body_h - 20, 0x000F172A);
        gui_draw_string(cx + 25, body_y + 25, "Navigated to:", 0x0094A3B8, 1);
        gui_draw_string(cx + 140, body_y + 25, g_url_buffer, 0x0038BDF8, 1);
        gui_draw_string(cx + 25, body_y + 55, "Page loaded via Xenithra V8 Native Rendering Pipeline.", 0x0010B981, 1);
    }
}

void browser_app_launch(void) {
    if (g_browser_win) {
        window_restore(g_browser_win);
        window_focus(g_browser_win);
        return;
    }

    v8_mount_react_app("browser");

    g_browser_win = window_create(
        "Microsoft Edge - Google",
        "browser",
        70, 30, 840, 530,
        render_browser_window,
        NULL
    );

    if (g_browser_win) {
        window_set_callbacks(g_browser_win, on_browser_key, on_browser_mouse);
        window_focus(g_browser_win);
    }
}

void browser_app_navigate(const char *url) {
    if (!url) return;
    strcpy(g_url_buffer, url);
    g_active_tab = TAB_CUSTOM;
    browser_app_launch();
}

void browser_app_search(const char *query) {
    if (!query) return;
    strcpy(g_search_query, query);
    g_active_tab = TAB_GOOGLE;
    browser_app_launch();
}
