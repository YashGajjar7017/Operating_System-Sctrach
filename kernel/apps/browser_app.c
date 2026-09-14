/**
 * @file browser_app.c
 * @brief Full-Screen Immersive React/Browser Command Shell & V8 Engine for Xenithra OS
 */

#include "browser_app.h"
#include "../kstring.h"
#include "../gui/v8_engine.h"
#include "firewall_app.h"
#include "terminal_app.h"
#include "explorer_app.h"
#include "taskmgr_app.h"
#include "vlc_app.h"
#include "diskclone_app.h"

typedef enum {
    TAB_COMMAND_CENTER = 0,
    TAB_GOOGLE         = 1,
    TAB_DOCS           = 2,
    TAB_CUSTOM         = 3
} BrowserTabType;

static Window *g_browser_win = NULL;
static BrowserTabType g_active_tab = TAB_COMMAND_CENTER;
static char g_url_buffer[128] = "https://www.google.com";
static char g_search_query[64] = "What would you like to do?";
static char g_command_output[128] = "Ready. Type a command (e.g. 'help', 'terminal', 'firewall', 'v8', 'clone') or search Google.";
static uint8_t g_focus_input = 1; /* 0: none, 1: search/command, 2: url */
static uint8_t g_is_loading = 0;

/* Command Interpreter */
static void execute_system_command(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;

    if (strcmp(cmd, "help") == 0) {
        strcpy(g_command_output, "Available: terminal, firewall, explorer, taskmgr, vlc, clone, v8, reboot, shutdown");
    } else if (strcmp(cmd, "terminal") == 0 || strcmp(cmd, "cmd") == 0) {
        terminal_app_launch();
        strcpy(g_command_output, "Launched Diagnostic Terminal Console.");
    } else if (strcmp(cmd, "firewall") == 0 || strcmp(cmd, "sec") == 0 || strcmp(cmd, "security") == 0) {
        firewall_app_launch();
        strcpy(g_command_output, "Launched Security & Firewall Center.");
    } else if (strcmp(cmd, "explorer") == 0 || strcmp(cmd, "files") == 0) {
        explorer_app_launch();
        strcpy(g_command_output, "Launched File Explorer.");
    } else if (strcmp(cmd, "taskmgr") == 0 || strcmp(cmd, "top") == 0) {
        taskmgr_app_launch();
        strcpy(g_command_output, "Launched Task Manager.");
    } else if (strcmp(cmd, "vlc") == 0 || strcmp(cmd, "media") == 0) {
        vlc_app_launch();
        strcpy(g_command_output, "Launched VLC Media Player.");
    } else if (strcmp(cmd, "clone") == 0 || strcmp(cmd, "disk") == 0 || strcmp(cmd, "backup") == 0) {
        diskclone_app_launch();
        strcpy(g_command_output, "Launched Raw Sector Cloner.");
    } else if (strcmp(cmd, "v8") == 0) {
        strcpy(g_command_output, "V8 Engine: 64MB Heap | JIT Active | React Reconciler Running.");
    } else if (strcmp(cmd, "reboot") == 0) {
        system_reboot();
    } else if (strcmp(cmd, "shutdown") == 0) {
        system_shutdown();
    } else {
        /* Treat as Google search */
        g_active_tab = TAB_GOOGLE;
        strcpy(g_url_buffer, "https://www.google.com/search?q=");
        strcat(g_url_buffer, cmd);
        strcpy(g_command_output, "Searching Google for query...");
    }
}

static void on_browser_mouse(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click) {
    (void)win;
    (void)right_click;
    if (!left_click) return;

    /* 1. Tab Bar (Y: 0 - 32) */
    if (rel_y >= 0 && rel_y <= 32) {
        if (rel_x >= 8 && rel_x <= 180) {
            g_active_tab = TAB_COMMAND_CENTER;
            strcpy(g_url_buffer, "xenithra://command-center");
            return;
        }
        if (rel_x >= 185 && rel_x <= 330) {
            g_active_tab = TAB_GOOGLE;
            strcpy(g_url_buffer, "https://www.google.com");
            return;
        }
        if (rel_x >= 335 && rel_x <= 490) {
            g_active_tab = TAB_DOCS;
            strcpy(g_url_buffer, "https://docs.xenithra.org/kernel");
            return;
        }
    }

    /* 2. Navigation Bar & Omnibox (Y: 34 - 70) */
    if (rel_y >= 34 && rel_y <= 70) {
        /* Back Button */
        if (rel_x >= 10 && rel_x <= 36) {
            g_active_tab = TAB_COMMAND_CENTER;
            strcpy(g_url_buffer, "xenithra://command-center");
            return;
        }
        /* Forward Button */
        if (rel_x >= 40 && rel_x <= 66) {
            g_active_tab = TAB_GOOGLE;
            strcpy(g_url_buffer, "https://www.google.com");
            return;
        }
        /* Reload Button */
        if (rel_x >= 70 && rel_x <= 96) {
            g_is_loading = 1;
            return;
        }
        /* URL Omnibox (X: 106 - cw-100) */
        if (rel_x >= 106 && rel_x <= win->width - 100) {
            g_focus_input = 2;
            return;
        }
    }

    /* 3. Bookmarks Toolbar (Y: 72 - 96) */
    if (rel_y >= 72 && rel_y <= 96) {
        if (rel_x >= 12 && rel_x <= 120) {
            g_active_tab = TAB_COMMAND_CENTER;
            strcpy(g_url_buffer, "xenithra://command-center");
            return;
        }
        if (rel_x >= 125 && rel_x <= 200) {
            g_active_tab = TAB_GOOGLE;
            strcpy(g_url_buffer, "https://www.google.com");
            return;
        }
        if (rel_x >= 205 && rel_x <= 280) {
            g_active_tab = TAB_CUSTOM;
            strcpy(g_url_buffer, "https://github.com/xenithra");
            return;
        }
        if (rel_x >= 285 && rel_x <= 370) {
            terminal_app_launch();
            return;
        }
        if (rel_x >= 375 && rel_x <= 490) {
            firewall_app_launch();
            return;
        }
    }

    /* 4. Content Area Interactions */
    int cw = win->width;
    int body_y = 96;

    if (g_active_tab == TAB_COMMAND_CENTER || g_active_tab == TAB_GOOGLE) {
        /* Command / Search Input Box */
        int search_w = 540;
        if (search_w > cw - 60) search_w = cw - 60;
        int search_x = (cw - search_w) / 2;
        int search_y = body_y + 110;

        if (rel_y >= search_y && rel_y <= search_y + 44 && rel_x >= search_x && rel_x <= search_x + search_w) {
            g_focus_input = 1;
            return;
        }

        /* Action Buttons (Y: search_y + 54) */
        int btn_y = search_y + 54;
        if (rel_y >= btn_y && rel_y <= btn_y + 36) {
            /* Button 1: Run Command / Search */
            if (rel_x >= search_x + 80 && rel_x <= search_x + 240) {
                execute_system_command(g_search_query);
                return;
            }
            /* Button 2: I'm Feeling Lucky / Diagnostics */
            if (rel_x >= search_x + 260 && rel_x <= search_x + 440) {
                terminal_app_launch();
                return;
            }
        }

        /* 6-Card Fast Command Launch Grid */
        int grid_y = btn_y + 60;
        int card_w = 140;
        int card_h = 75;
        int gap = 16;
        int cols = 3;
        int total_grid_w = cols * card_w + (cols - 1) * gap;
        int start_grid_x = (cw - total_grid_w) / 2;

        for (int i = 0; i < 6; i++) {
            int col = i % 3;
            int row = i / 3;
            int gx = start_grid_x + col * (card_w + gap);
            int gy = grid_y + row * (card_h + gap);

            if (rel_x >= gx && rel_x <= gx + card_w && rel_y >= gy && rel_y <= gy + card_h) {
                if (i == 0) terminal_app_launch();
                else if (i == 1) firewall_app_launch();
                else if (i == 2) explorer_app_launch();
                else if (i == 3) taskmgr_app_launch();
                else if (i == 4) vlc_app_launch();
                else if (i == 5) diskclone_app_launch();
                return;
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
            execute_system_command(g_search_query);
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
    gui_fill_rect(cx, cy, cw, ch, 0x00080E1A);

    /* 2. Modern Tab Strip (Y: 0 - 32) */
    gui_fill_rect(cx, cy, cw, 34, 0x000F172A);
    gui_draw_rect(cx, cy + 33, cw, 1, 0x001E293B);

    /* Tab 1: Command Center */
    uint32_t tab1_bg = (g_active_tab == TAB_COMMAND_CENTER) ? 0x001E293B : 0x000F172A;
    gui_fill_rounded_rect(cx + 8, cy + 4, 170, 28, 6, tab1_bg);
    gui_fill_rounded_rect(cx + 14, cy + 10, 14, 14, 3, 0x000078D4);
    gui_draw_string(cx + 18, cy + 9, ">", 0x00FFFFFF, 1);
    gui_draw_string(cx + 34, cy + 10, "Command Center", (g_active_tab == TAB_COMMAND_CENTER) ? 0x00FFFFFF : 0x0094A3B8, 1);
    gui_draw_string(cx + 160, cy + 10, "x", 0x0064748B, 1);

    /* Tab 2: Google */
    uint32_t tab2_bg = (g_active_tab == TAB_GOOGLE) ? 0x001E293B : 0x000F172A;
    gui_fill_rounded_rect(cx + 182, cy + 4, 140, 28, 6, tab2_bg);
    gui_fill_rounded_rect(cx + 188, cy + 10, 14, 14, 3, 0x002563EB);
    gui_draw_string(cx + 192, cy + 9, "G", 0x00FFFFFF, 1);
    gui_draw_string(cx + 208, cy + 10, "Google", (g_active_tab == TAB_GOOGLE) ? 0x00FFFFFF : 0x0094A3B8, 1);
    gui_draw_string(cx + 308, cy + 10, "x", 0x0064748B, 1);

    /* Tab 3: Xenithra OS Docs */
    uint32_t tab3_bg = (g_active_tab == TAB_DOCS) ? 0x001E293B : 0x000F172A;
    gui_fill_rounded_rect(cx + 326, cy + 4, 160, 28, 6, tab3_bg);
    gui_fill_rounded_rect(cx + 332, cy + 10, 14, 14, 3, 0x00059669);
    gui_draw_string(cx + 336, cy + 9, "D", 0x00FFFFFF, 1);
    gui_draw_string(cx + 352, cy + 10, "Xenithra Docs", (g_active_tab == TAB_DOCS) ? 0x00FFFFFF : 0x0094A3B8, 1);

    /* New Tab (+) Button */
    gui_fill_rounded_rect(cx + 492, cy + 8, 22, 22, 4, 0x001E293B);
    gui_draw_string(cx + 499, cy + 10, "+", 0x0094A3B8, 1);

    /* 3. Navigation Bar & Omnibox (Y: 34 - 72) */
    gui_fill_rect(cx, cy + 34, cw, 38, 0x00111A2E);
    gui_draw_rect(cx, cy + 71, cw, 1, 0x001E293B);

    /* Navigation Controls */
    gui_fill_rounded_rect(cx + 8, cy + 40, 26, 26, 4, 0x001E293B);
    gui_draw_string(cx + 16, cy + 45, "<", 0x0094A3B8, 1);

    gui_fill_rounded_rect(cx + 38, cy + 40, 26, 26, 4, 0x001E293B);
    gui_draw_string(cx + 47, cy + 45, ">", 0x0094A3B8, 1);

    gui_fill_rounded_rect(cx + 68, cy + 40, 26, 26, 4, 0x001E293B);
    gui_draw_string(cx + 76, cy + 45, "R", 0x0038BDF8, 1);

    /* Omnibox / System Command Bar */
    int omni_w = cw - 180;
    if (omni_w < 200) omni_w = 200;
    gui_fill_rounded_rect(cx + 100, cy + 39, omni_w, 28, 6, 0x00090F1C);
    gui_draw_rect(cx + 100, cy + 39, omni_w, 28, (g_focus_input == 2) ? 0x000078D4 : 0x0024344E);

    gui_fill_rounded_rect(cx + 106, cy + 44, 48, 18, 3, 0x00064E3B);
    gui_draw_string(cx + 110, cy + 45, "SEC", 0x0034D399, 1);

    gui_draw_string(cx + 160, cy + 45, g_url_buffer, 0x00E2E8F0, 1);

    /* V8 Engine Badge */
    gui_fill_rounded_rect(cx + cw - 70, cy + 40, 60, 26, 13, 0x001E293B);
    gui_draw_string(cx + cw - 56, cy + 45, "V8.JS", 0x0038BDF8, 1);

    /* 4. Bookmarks Toolbar (Y: 72 - 96) */
    gui_fill_rect(cx, cy + 72, cw, 24, 0x000B1220);
    gui_draw_rect(cx, cy + 95, cw, 1, 0x001B263B);

    gui_draw_string(cx + 14, cy + 76, "[>] System Shell", 0x0038BDF8, 1);
    gui_draw_string(cx + 128, cy + 76, "[G] Google Search", 0x0094A3B8, 1);
    gui_draw_string(cx + 250, cy + 76, "[Git] Repository", 0x0094A3B8, 1);
    gui_draw_string(cx + 360, cy + 76, "[CLI] Terminal", 0x0094A3B8, 1);
    gui_draw_string(cx + 470, cy + 76, "[Sec] Firewall Guard", 0x0034D399, 1);

    /* 5. Main Canvas Body (Y: 96 - ch) */
    int body_y = cy + 96;
    int body_h = ch - 96;

    if (g_active_tab == TAB_COMMAND_CENTER || g_active_tab == TAB_GOOGLE) {
        /* Centered Header / Logo */
        int logo_y = body_y + 35;
        int logo_x = cx + (cw - 240) / 2;

        if (g_active_tab == TAB_GOOGLE) {
            gui_draw_string(logo_x,       logo_y, "G", 0x004285F4, 3);
            gui_draw_string(logo_x + 36,  logo_y, "o", 0x00EA4335, 3);
            gui_draw_string(logo_x + 72,  logo_y, "o", 0x00FBBC05, 3);
            gui_draw_string(logo_x + 108, logo_y, "g", 0x004285F4, 3);
            gui_draw_string(logo_x + 144, logo_y, "l", 0x0034A853, 3);
            gui_draw_string(logo_x + 172, logo_y, "e", 0x00EA4335, 3);
        } else {
            gui_draw_string(cx + (cw - 320) / 2, logo_y, "Xenithra OS Command Center", 0x0038BDF8, 2);
            gui_draw_string(cx + (cw - 260) / 2, logo_y + 30, "What would you like to do today?", 0x0094A3B8, 1);
        }

        /* Command / Search Input Box */
        int search_w = 540;
        if (search_w > cw - 60) search_w = cw - 60;
        int search_x = cx + (cw - search_w) / 2;
        int search_y = body_y + 100;

        gui_fill_rounded_rect(search_x, search_y, search_w, 44, 22, 0x00131E33);
        gui_draw_rect(search_x, search_y, search_w, 44, (g_focus_input == 1) ? 0x0038BDF8 : 0x002B3D5E);

        gui_draw_string(search_x + 18, search_y + 14, ">", 0x0038BDF8, 1);
        gui_draw_string(search_x + 38, search_y + 14, g_search_query, 0x00FFFFFF, 1);

        if (g_focus_input == 1) {
            int q_len = strlen(g_search_query);
            gui_fill_rect(search_x + 38 + q_len * 8 + 2, search_y + 12, 2, 20, 0x0038BDF8);
        }

        /* Action Buttons */
        int btn_y = search_y + 54;
        gui_fill_rounded_rect(search_x + 80, btn_y, 160, 32, 6, 0x001E293B);
        gui_draw_string(search_x + 104, btn_y + 8, "Execute / Search", 0x00CBD5E1, 1);

        gui_fill_rounded_rect(search_x + 260, btn_y, 160, 32, 6, 0x001E293B);
        gui_draw_string(search_x + 280, btn_y + 8, "Open Terminal", 0x00CBD5E1, 1);

        /* Telemetry / Output Status Bar */
        gui_fill_rounded_rect(search_x, btn_y + 44, search_w, 28, 4, 0x000B111F);
        gui_draw_rect(search_x, btn_y + 44, search_w, 28, 0x001E293B);
        gui_draw_string(search_x + 12, btn_y + 50, g_command_output, 0x0034D399, 1);

        /* Fast 6-Card Action Grid */
        int grid_y = btn_y + 82;
        int card_w = 140;
        int card_h = 70;
        int gap = 16;
        int cols = 3;
        int total_grid_w = cols * card_w + (cols - 1) * gap;
        int start_grid_x = cx + (cw - total_grid_w) / 2;

        const char *card_titles[] = {"Terminal Console", "Security Guard", "File Explorer", "Task Manager", "VLC Player", "Sector Cloner"};
        const char *card_subs[]   = {"CLI Diagnostics",  "Private Firewall","Storage Drives", "CPU & Memory", "Media Streamer", "Disk Backup 64MB"};
        uint32_t card_colors[]    = {0x002563EB,         0x00059669,        0x00D97706,      0x007C3AED,     0x00EA580C,     0x000891B2};

        for (int i = 0; i < 6; i++) {
            int col = i % 3;
            int row = i / 3;
            int gx = start_grid_x + col * (card_w + gap);
            int gy = grid_y + row * (card_h + gap);

            gui_fill_rounded_rect(gx, gy, card_w, card_h, 8, 0x00111A2D);
            gui_draw_rect(gx, gy, card_w, card_h, 0x0022304A);

            gui_fill_rounded_rect(gx + 10, gy + 10, 8, card_h - 20, 3, card_colors[i]);
            gui_draw_string(gx + 24, gy + 16, card_titles[i], 0x00FFFFFF, 1);
            gui_draw_string(gx + 24, gy + 38, card_subs[i], 0x0094A3B8, 1);
        }

    } else if (g_active_tab == TAB_DOCS) {
        gui_fill_rect(cx + 16, body_y + 16, cw - 32, body_h - 32, 0x000E172A);
        gui_draw_rect(cx + 16, body_y + 16, cw - 32, body_h - 32, 0x001E293B);

        gui_draw_string(cx + 32, body_y + 32, "Xenithra OS Architecture & V8 Runtime Engine", 0x0038BDF8, 2);
        gui_draw_string(cx + 32, body_y + 68, "------------------------------------------------------------", 0x00334155, 1);
        gui_draw_string(cx + 32, body_y + 90, "1. V8 JIT Engine & Native DOM Reconciler Pipeline active.", 0x00E2E8F0, 1);
        gui_draw_string(cx + 32, body_y + 115, "2. Hardware SMEP / SMAP & 128-bit Anti-Session Hijacking Guard.", 0x00E2E8F0, 1);
        gui_draw_string(cx + 32, body_y + 140, "3. GPU-Accelerated DirectX 12 / DWM Flip Model Compositor.", 0x00E2E8F0, 1);
        gui_draw_string(cx + 32, body_y + 165, "4. Real-time Multi-pass Gaussian Blur & Acrylic Shader Materials.", 0x00E2E8F0, 1);
    } else {
        gui_fill_rect(cx + 16, body_y + 16, cw - 32, body_h - 32, 0x000E172A);
        gui_draw_string(cx + 32, body_y + 32, "Navigated URL:", 0x0094A3B8, 1);
        gui_draw_string(cx + 150, body_y + 32, g_url_buffer, 0x0038BDF8, 1);
        gui_draw_string(cx + 32, body_y + 65, "Rendering via Xenithra V8 Headless Electron Bridge...", 0x0010B981, 1);
    }
}

void browser_app_launch(void) {
    if (g_browser_win) {
        window_restore(g_browser_win);
        window_focus(g_browser_win);
        return;
    }

    v8_mount_react_app("browser");

    /* Full-Screen Immersive Dimensions */
    g_browser_win = window_create(
        "Xenithra OS - Integrated Windows 11 Desktop Shell & Browser",
        "browser",
        0, 0, 1280, 672,
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
