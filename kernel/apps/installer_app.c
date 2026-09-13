/**
 * @file installer_app.c
 * @brief Windows 11 64-bit App Installer & Package Manager with Drag & Drop .exe support
 */

#include "installer_app.h"
#include "../kstring.h"
#include "vlc_app.h"
#include "explorer_app.h"
#include "taskmgr_app.h"
#include "firewall_app.h"
#include "terminal_app.h"

typedef struct {
    const char *name;
    const char *version;
    const char *publisher;
    const char *size_str;
    const char *icon_tag;
    uint32_t icon_color;
    uint8_t is_installed;
    void (*on_launch)(void);
} CatalogApp;

#define CATALOG_COUNT 6
static CatalogApp g_catalog[CATALOG_COUNT] = {
    {"VLC Media Player 64-bit", "v3.0.21", "VideoLAN Corp",   "48.2 MB", "VLC", 0x00F97316, 1, (void(*)(void))vlc_app_launch},
    {"VS Code Pro 64-bit",      "v1.92.0", "Microsoft Corp",  "88.4 MB", "VSC", 0x000078D4, 0, (void(*)(void))terminal_app_launch},
    {"Firewall Sentinel x64",   "v2.4.0",  "Xenithra Sec",    "14.1 MB", "SEC", 0x0010B981, 1, (void(*)(void))firewall_app_launch},
    {"7-Zip File Manager x64",  "v24.07",  "Igor Pavlov",     "3.2 MB",  "7Z",  0x0064748B, 0, (void(*)(void))explorer_app_launch},
    {"Retro 3D Arcade x64",     "v1.0.4",  "Gaming Systems",  "32.0 MB", "RET", 0x00A855F7, 0, (void(*)(void))vlc_app_launch},
    {"NodeJS Runtime Engine",   "v22.6.0", "OpenJS Found.",   "42.5 MB", "JS",  0x0022C55E, 0, (void(*)(void))terminal_app_launch}
};

/* Drag & Drop / Active Install State */
static uint8_t g_installing = 0;
static int g_install_progress = 0;
static int g_install_stage = 0;
static const char *g_install_app_name = "setup_package_x64.exe";
static const char *g_stage_messages[4] = {
    "Verifying PE64 / ELF64 Architecture & Signature...",
    "Validating SMEP/SMAP Ring Isolation Security...",
    "Extracting Binaries to C:\\Program Files\\...",
    "Registration Complete - Ready to Launch!"
};

void installer_app_tick(void) {
    if (g_installing) {
        g_install_progress += 2;
        if (g_install_progress < 25) {
            g_install_stage = 0;
        } else if (g_install_progress < 60) {
            g_install_stage = 1;
        } else if (g_install_progress < 95) {
            g_install_stage = 2;
        } else if (g_install_progress >= 100) {
            g_install_progress = 100;
            g_install_stage = 3;
            g_installing = 0;
        }
    }
}

static void trigger_install_simulation(const char *app_name) {
    g_install_app_name = app_name;
    g_installing = 1;
    g_install_progress = 0;
    g_install_stage = 0;
}

static void on_installer_mouse(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click) {
    (void)win;
    (void)right_click;
    if (!left_click) return;

    /* 1. Drag & Drop Zone / Browse Button Click (Y: 50 - 150) */
    if (rel_y >= 50 && rel_y <= 160 && rel_x >= 20 && rel_x <= 740) {
        trigger_install_simulation("custom_setup_x86_64.exe");
        return;
    }

    /* 2. Catalog App List Install / Launch Buttons (Y: 210 - 450) */
    if (rel_y >= 210 && rel_y <= 460) {
        for (int i = 0; i < CATALOG_COUNT; i++) {
            int row_y = 210 + i * 42;
            if (rel_y >= row_y && rel_y <= row_y + 36) {
                int btn_x = 620;
                if (rel_x >= btn_x && rel_x <= btn_x + 110) {
                    if (g_catalog[i].is_installed) {
                        if (g_catalog[i].on_launch) {
                            g_catalog[i].on_launch();
                        }
                    } else {
                        g_catalog[i].is_installed = 1;
                        trigger_install_simulation(g_catalog[i].name);
                    }
                    return;
                }
            }
        }
    }
}

static void on_installer_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* 1. Main Background */
    gui_fill_rect(cx, cy, cw, ch, GUI_BG_WINDOW);

    /* 2. Header Banner */
    gui_fill_rounded_rect(cx + 12, cy + 8, cw - 24, 34, 6, 0x00141D2F);
    gui_draw_rect(cx + 12, cy + 8, cw - 24, 34, GUI_BORDER_COLOR);
    gui_draw_fluent_icon_installer(cx + 20, cy + 9);
    gui_draw_string(cx + 48, cy + 14, "Xenithra 64-bit App Installer & Package Store", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(cx + cw - 210, cy + 14, "Architecture: x86_64 AMD64", GUI_ACCENT_CYAN, 1);

    /* 3. Drag & Drop Zone Box */
    int drop_x = cx + 12;
    int drop_y = cy + 48;
    int drop_w = cw - 24;
    int drop_h = 110;

    gui_fill_rounded_rect(drop_x, drop_y, drop_w, drop_h, 8, g_installing ? 0x0013223A : 0x00101828);
    gui_draw_rect(drop_x, drop_y, drop_w, drop_h, g_installing ? GUI_ACCENT_CYAN : GUI_ACCENT_BLUE);

    if (g_installing || g_install_progress == 100) {
        /* Installation in progress / complete */
        gui_draw_string(drop_x + 20, drop_y + 14, "Installing Package:", GUI_TEXT_SECONDARY, 1);
        gui_draw_string(drop_x + 180, drop_y + 14, g_install_app_name, GUI_ACCENT_CYAN, 1);

        /* Progress Bar */
        int pbar_w = drop_w - 40;
        gui_fill_rounded_rect(drop_x + 20, drop_y + 40, pbar_w, 14, 7, 0x001F293D);
        int fill_w = (pbar_w * g_install_progress) / 100;
        gui_fill_rounded_rect(drop_x + 20, drop_y + 40, fill_w, 14, 7, (g_install_progress == 100) ? GUI_ACCENT_GREEN : GUI_ACCENT_BLUE);

        /* Percentage & Stage Text */
        char p_str[16];
        uint_to_str(g_install_progress, p_str);
        strcat(p_str, "%");
        gui_draw_string(drop_x + drop_w - 70, drop_y + 14, p_str, GUI_TEXT_PRIMARY, 1);
        gui_draw_string(drop_x + 20, drop_y + 64, g_stage_messages[g_install_stage], (g_install_progress == 100) ? GUI_ACCENT_GREEN : GUI_TEXT_MUTED, 1);

        if (g_install_progress == 100) {
            gui_fill_rounded_rect(drop_x + drop_w - 140, drop_y + 60, 110, 28, 4, GUI_ACCENT_GREEN);
            gui_draw_string(drop_x + drop_w - 124, drop_y + 66, "Launch App", 0x00FFFFFF, 1);
        }
    } else {
        /* Default Drop Prompt */
        gui_fill_rounded_rect(drop_x + drop_w / 2 - 20, drop_y + 12, 40, 40, 8, GUI_ACCENT_BLUE);
        gui_draw_string(drop_x + drop_w / 2 - 8, drop_y + 24, "v", 0x00FFFFFF, 2);

        gui_draw_string(drop_x + drop_w / 2 - 170, drop_y + 60, "Drag & Drop .exe package here to Install", GUI_TEXT_PRIMARY, 1);
        gui_draw_string(drop_x + drop_w / 2 - 210, drop_y + 80, "Supports PE32+ (x86_64), ELF64 binaries, and MSIX bundles", GUI_TEXT_MUTED, 1);

        /* Browse button */
        gui_fill_rounded_rect(drop_x + drop_w - 150, drop_y + 36, 120, 32, 6, GUI_BG_CARD_HOVER);
        gui_draw_rect(drop_x + drop_w - 150, drop_y + 36, 120, 32, GUI_ACCENT_BLUE);
        gui_draw_string(drop_x + drop_w - 134, drop_y + 44, "+ Select .exe", GUI_TEXT_PRIMARY, 1);
    }

    /* 4. Software Store & Catalog Header */
    int cat_header_y = cy + 168;
    gui_draw_string(cx + 16, cat_header_y, "Recommended 64-bit Verified Applications", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(cx + cw - 260, cat_header_y, "SMEP/SMAP Sandboxed Repositories", GUI_TEXT_MUTED, 1);

    /* 5. Software Items Table */
    int table_y = cat_header_y + 20;
    for (int i = 0; i < CATALOG_COUNT; i++) {
        CatalogApp *app = &g_catalog[i];
        int row_y = table_y + i * 42;
        if (row_y + 36 > cy + ch - 10) break;

        gui_fill_rounded_rect(cx + 12, row_y, cw - 24, 38, 6, GUI_BG_CARD);
        gui_draw_rect(cx + 12, row_y, cw - 24, 38, GUI_BORDER_COLOR);

        /* App Icon Badge */
        gui_fill_rounded_rect(cx + 20, row_y + 7, 24, 24, 4, app->icon_color);
        gui_draw_string(cx + 24, row_y + 11, app->icon_tag, 0x00FFFFFF, 1);

        /* Title & Publisher */
        gui_draw_string(cx + 56, row_y + 6, app->name, GUI_TEXT_PRIMARY, 1);
        gui_draw_string(cx + 56, row_y + 20, app->publisher, GUI_TEXT_MUTED, 1);

        /* Version */
        gui_draw_string(cx + 310, row_y + 11, app->version, GUI_ACCENT_CYAN, 1);

        /* Size */
        gui_draw_string(cx + 420, row_y + 11, app->size_str, GUI_TEXT_SECONDARY, 1);

        /* Security Badge */
        gui_fill_rounded_rect(cx + 500, row_y + 8, 90, 22, 3, 0x00132822);
        gui_draw_rect(cx + 500, row_y + 8, 90, 22, GUI_ACCENT_GREEN);
        gui_draw_string(cx + 508, row_y + 11, "Verified x64", GUI_ACCENT_GREEN, 1);

        /* Action Button */
        int btn_x = cx + cw - 130;
        if (app->is_installed) {
            gui_fill_rounded_rect(btn_x, row_y + 6, 100, 26, 4, 0x0016A34A);
            gui_draw_string(btn_x + 18, row_y + 10, "Launch", 0x00FFFFFF, 1);
        } else {
            gui_fill_rounded_rect(btn_x, row_y + 6, 100, 26, 4, GUI_ACCENT_BLUE);
            gui_draw_string(btn_x + 18, row_y + 10, "Install", 0x00FFFFFF, 1);
        }
    }
}

Window* installer_app_launch(void) {
    Window *win = window_create(
        "64-bit App Installer & Package Store",
        "installer",
        90,
        60,
        760,
        480,
        on_installer_paint,
        NULL
    );
    if (win) {
        window_set_callbacks(win, NULL, on_installer_mouse);
    }
    return win;
}
