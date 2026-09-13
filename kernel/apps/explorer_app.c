/**
 * @file explorer_app.c
 * @brief Exact Windows File Explorer (Image 2 Match)
 */

#include "explorer_app.h"
#include "../kstring.h"
#include "vlc_app.h"
#include "installer_app.h"

typedef struct {
    const char *name;
    const char *subtext;
    const char *motif_tag;
    uint32_t motif_color;
} ExplorerFolderTile;

#define SPECIAL_FOLDERS_COUNT 7
static ExplorerFolderTile g_special_folders[SPECIAL_FOLDERS_COUNT] = {
    {"3D Objects", "", "3D",  0x0006B6D4},
    {"Desktop",    "", "DSK", 0x000284C7},
    {"Documents",  "", "DOC", 0x0064748B},
    {"Downloads",  "", "DWN", 0x000078D4},
    {"Music",      "", "MUS", 0x000284C7},
    {"Pictures",   "", "PIC", 0x00059669},
    {"Videos",     "", "VID", 0x00F97316}
};

static int g_selected_folder = -1;
static int g_selected_drive = 0;
static int g_ribbon_tab = 1; /* 0: File, 1: Computer, 2: View */

static void on_explorer_mouse(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click) {
    (void)win;
    (void)right_click;
    if (!left_click) return;

    /* 1. Ribbon Tabs (Y: 0 - 24) */
    if (rel_y >= 0 && rel_y <= 24) {
        if (rel_x >= 8 && rel_x <= 50)  g_ribbon_tab = 0; /* File */
        if (rel_x >= 54 && rel_x <= 124) g_ribbon_tab = 1; /* Computer */
        if (rel_x >= 128 && rel_x <= 180) g_ribbon_tab = 2; /* View */
        return;
    }

    /* 2. Ribbon Action Toolbar (Y: 26 - 100) */
    if (rel_y >= 26 && rel_y <= 100) {
        /* Open Settings / System Properties / Manage */
        if (rel_x >= 330 && rel_x <= 380) {
            installer_app_launch();
            return;
        }
    }

    /* 3. Navigation Buttons (Y: 104 - 134) */
    if (rel_y >= 104 && rel_y <= 134) {
        if (rel_x >= 8 && rel_x <= 32) return; /* Back */
        if (rel_x >= 36 && rel_x <= 60) return; /* Forward */
        if (rel_x >= 64 && rel_x <= 88) return; /* Up */
    }

    /* 4. Left Sidebar (X: 0 - 160) */
    if (rel_x >= 0 && rel_x <= 160 && rel_y >= 136) {
        int side_idx = (rel_y - 156) / 22;
        if (side_idx >= 0 && side_idx <= 10) {
            g_selected_folder = -1;
            g_selected_drive = 1;
            return;
        }
    }

    /* 5. Main Content Area: Folders Grid (Y: 160 - 290) */
    int main_x = 170;
    if (rel_x >= main_x && rel_y >= 160 && rel_y <= 290) {
        int col = (rel_x - main_x) / 190;
        int row = (rel_y - 160) / 44;
        int idx = row * 3 + col;
        if (idx >= 0 && idx < SPECIAL_FOLDERS_COUNT) {
            if (g_selected_folder == idx) {
                /* Double click action */
                if (idx == 6) vlc_app_launch(); /* Videos */
                else installer_app_launch();
            } else {
                g_selected_folder = idx;
                g_selected_drive = 0;
            }
            return;
        }
    }

    /* 6. Main Content Area: Devices & Drives (Y: 320 - 380) */
    if (rel_x >= main_x && rel_y >= 320 && rel_y <= 380) {
        g_selected_drive = 1;
        g_selected_folder = -1;
    }
}

static void draw_folder_tile(int x, int y, int w, int h, const char *name, const char *motif_tag, uint32_t motif_color, uint8_t is_selected) {
    if (is_selected) {
        gui_fill_rounded_rect(x, y, w, h, 4, 0x001E293B);
        gui_draw_rect(x, y, w, h, GUI_ACCENT_BLUE);
    }

    /* Windows 11 Fluent Folder Icon */
    gui_fill_rounded_rect(x + 8, y + 6, 14, 8, 2, 0x000284C7);
    gui_fill_rounded_rect(x + 8, y + 10, 32, 24, 3, 0x00F59E0B);
    gui_fill_rect(x + 10, y + 12, 28, 6, 0x00FBBF24);

    /* Inner Motif Badge */
    if (strcmp(motif_tag, "3D") == 0) {
        gui_fill_rounded_rect(x + 16, y + 16, 16, 14, 2, motif_color);
        gui_draw_rect(x + 16, y + 16, 16, 14, 0x00FFFFFF);
    } else if (strcmp(motif_tag, "DSK") == 0) {
        gui_fill_rounded_rect(x + 14, y + 15, 20, 14, 2, motif_color);
        gui_fill_rect(x + 21, y + 29, 6, 3, 0x00CBD5E1);
    } else if (strcmp(motif_tag, "DOC") == 0) {
        gui_fill_rounded_rect(x + 16, y + 14, 16, 18, 2, 0x00FFFFFF);
        gui_fill_rect(x + 19, y + 18, 10, 2, 0x000078D4);
        gui_fill_rect(x + 19, y + 22, 10, 2, 0x0064748B);
    } else if (strcmp(motif_tag, "DWN") == 0) {
        gui_fill_rounded_rect(x + 16, y + 14, 16, 18, 2, motif_color);
        gui_draw_string(x + 20, y + 16, "v", 0x00FFFFFF, 1);
    } else if (strcmp(motif_tag, "MUS") == 0) {
        gui_fill_rounded_rect(x + 16, y + 14, 16, 18, 2, motif_color);
        gui_draw_string(x + 20, y + 16, "#", 0x00FFFFFF, 1);
    } else if (strcmp(motif_tag, "PIC") == 0) {
        gui_fill_rounded_rect(x + 14, y + 16, 20, 14, 2, 0x00FFFFFF);
        gui_fill_rect(x + 16, y + 18, 16, 8, motif_color);
    } else if (strcmp(motif_tag, "VID") == 0) {
        gui_fill_rounded_rect(x + 14, y + 15, 20, 16, 2, motif_color);
        gui_draw_string(x + 20, y + 16, ">", 0x00FFFFFF, 1);
    }

    /* Name */
    gui_draw_string(x + 48, y + 16, name, GUI_TEXT_PRIMARY, 1);
}

static void on_explorer_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* 1. Main Background */
    gui_fill_rect(cx, cy, cw, ch, 0x00141414);

    /* 2. Top Menu Ribbon Tabs (File, Computer, View) */
    int menu_h = 24;
    gui_fill_rect(cx, cy, cw, menu_h, 0x00101010);
    gui_draw_rect(cx, cy, cw, menu_h, 0x00222222);

    /* Tab: File */
    gui_fill_rect(cx + 8, cy + 2, 42, 20, (g_ribbon_tab == 0) ? GUI_ACCENT_BLUE : 0x00101010);
    gui_draw_string(cx + 16, cy + 5, "File", (g_ribbon_tab == 0) ? 0x00FFFFFF : GUI_TEXT_SECONDARY, 1);

    /* Tab: Computer (Active Blue in Image 2) */
    gui_fill_rect(cx + 52, cy + 2, 72, 20, (g_ribbon_tab == 1) ? GUI_ACCENT_BLUE : 0x00101010);
    gui_draw_string(cx + 60, cy + 5, "Computer", (g_ribbon_tab == 1) ? 0x00FFFFFF : GUI_TEXT_SECONDARY, 1);

    /* Tab: View */
    gui_fill_rect(cx + 126, cy + 2, 48, 20, (g_ribbon_tab == 2) ? GUI_ACCENT_BLUE : 0x00101010);
    gui_draw_string(cx + 134, cy + 5, "View", (g_ribbon_tab == 2) ? 0x00FFFFFF : GUI_TEXT_SECONDARY, 1);

    /* 3. Action Toolbar Ribbon (Matching Image 2 exactly) */
    int ribbon_y = cy + menu_h;
    int ribbon_h = 76;
    gui_fill_rect(cx, ribbon_y, cw, ribbon_h, 0x001C1C1C);
    gui_draw_rect(cx, ribbon_y, cw, ribbon_h, 0x002A2A2A);

    /* Location Group (Properties, Open, Rename) */
    gui_draw_string(cx + 10, ribbon_y + 8, "Properties", GUI_TEXT_MUTED, 1);
    gui_draw_string(cx + 70, ribbon_y + 8, "Open", GUI_TEXT_MUTED, 1);
    gui_draw_string(cx + 110, ribbon_y + 8, "Rename", GUI_TEXT_MUTED, 1);
    gui_draw_string(cx + 50, ribbon_y + 58, "Location", GUI_TEXT_MUTED, 1);
    gui_draw_rect(cx + 160, ribbon_y + 6, 1, 60, 0x00333333);

    /* Network Group */
    gui_draw_string(cx + 170, ribbon_y + 8, "Access media v", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 170, ribbon_y + 24, "Map network drive v", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 170, ribbon_y + 40, "Add network location", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 220, ribbon_y + 58, "Network", GUI_TEXT_MUTED, 1);
    gui_draw_rect(cx + 320, ribbon_y + 6, 1, 60, 0x00333333);

    /* System Group */
    gui_fill_rounded_rect(cx + 330, ribbon_y + 8, 36, 36, 6, GUI_ACCENT_BLUE);
    gui_draw_string(cx + 342, ribbon_y + 18, "*", 0x00FFFFFF, 2);
    gui_draw_string(cx + 330, ribbon_y + 46, "Open", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 326, ribbon_y + 56, "Settings", GUI_TEXT_SECONDARY, 1);

    gui_draw_string(cx + 376, ribbon_y + 8, "Uninstall or change a program", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 376, ribbon_y + 24, "System properties", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 376, ribbon_y + 40, "Manage", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 420, ribbon_y + 58, "System", GUI_TEXT_MUTED, 1);

    /* 4. Breadcrumb Navigation & Search Bar (Matching Image 2) */
    int nav_y = ribbon_y + ribbon_h + 4;
    int nav_h = 28;

    /* Back [<], Forward [>], Up [^] */
    gui_fill_rounded_rect(cx + 8, nav_y, 24, 24, 3, 0x00242424);
    gui_draw_string(cx + 15, nav_y + 4, "<", GUI_TEXT_MUTED, 1);

    gui_fill_rounded_rect(cx + 34, nav_y, 24, 24, 3, 0x00242424);
    gui_draw_string(cx + 41, nav_y + 4, ">", GUI_TEXT_MUTED, 1);

    gui_fill_rounded_rect(cx + 60, nav_y, 24, 24, 3, 0x00242424);
    gui_draw_string(cx + 67, nav_y + 4, "^", GUI_TEXT_PRIMARY, 1);

    /* Breadcrumb Path Box */
    int addr_x = cx + 90;
    int addr_w = cw - 320;
    gui_fill_rounded_rect(addr_x, nav_y, addr_w, 24, 3, 0x001A1A1A);
    gui_draw_rect(addr_x, nav_y, addr_w, 24, 0x00333333);
    gui_draw_fluent_icon_this_pc(addr_x + 4, nav_y + 2);
    gui_draw_string(addr_x + 28, nav_y + 4, "This PC", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(addr_x + addr_w - 36, nav_y + 4, "v", GUI_TEXT_MUTED, 1);
    gui_draw_string(addr_x + addr_w - 18, nav_y + 4, "O", GUI_ACCENT_CYAN, 1);

    /* Search Box */
    int search_x = addr_x + addr_w + 8;
    int search_w = cw - (search_x - cx) - 8;
    gui_fill_rounded_rect(search_x, nav_y, search_w, 24, 3, 0x001A1A1A);
    gui_draw_rect(search_x, nav_y, search_w, 24, 0x00333333);
    gui_draw_string(search_x + 8, nav_y + 4, "Search This PC", GUI_TEXT_MUTED, 1);
    gui_draw_string(search_x + search_w - 18, nav_y + 4, "o", GUI_TEXT_MUTED, 1);

    /* 5. Left Navigation Tree Sidebar */
    int side_w = 160;
    int side_y = nav_y + nav_h + 4;
    int content_h = ch - (side_y - cy) - 24;

    gui_fill_rect(cx, side_y, side_w, content_h, 0x00181818);
    gui_draw_rect(cx + side_w - 1, side_y, 1, content_h, 0x002A2A2A);

    gui_draw_string(cx + 8, side_y + 8, "* Quick access", GUI_ACCENT_CYAN, 1);
    gui_draw_string(cx + 20, side_y + 26, "Desktop", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 20, side_y + 44, "Downloads", GUI_TEXT_SECONDARY, 1);

    gui_fill_rect(cx, side_y + 68, side_w, 22, 0x00242424);
    gui_draw_string(cx + 8, side_y + 72, "v This PC", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(cx + 20, side_y + 92, "3D Objects", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 20, side_y + 110, "Desktop", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 20, side_y + 128, "Documents", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 20, side_y + 146, "Downloads", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 20, side_y + 164, "Music", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 20, side_y + 182, "Pictures", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 20, side_y + 200, "Videos", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 20, side_y + 218, "Local Disk (C:)", GUI_ACCENT_CYAN, 1);

    gui_draw_string(cx + 8, side_y + 242, "> Network", GUI_TEXT_MUTED, 1);

    /* 6. Main Content Area (Matching Image 2 Tile Grid) */
    int main_x = cx + side_w + 12;
    int main_y = side_y + 8;

    /* Category: v Folders (7) */
    gui_draw_string(main_x, main_y, "v Folders (7)", GUI_TEXT_SECONDARY, 1);

    /* 7 Folders Grid */
    int folder_start_y = main_y + 20;
    int tile_w = 180;
    int tile_h = 40;

    for (int i = 0; i < SPECIAL_FOLDERS_COUNT; i++) {
        int col = i % 3;
        int row = i / 3;
        int tx = main_x + col * 190;
        int ty = folder_start_y + row * 44;

        draw_folder_tile(
            tx, ty, tile_w, tile_h,
            g_special_folders[i].name,
            g_special_folders[i].motif_tag,
            g_special_folders[i].motif_color,
            (g_selected_folder == i)
        );
    }

    /* Category: v Devices and drives (1) */
    int dev_header_y = folder_start_y + 3 * 44 + 8;
    gui_draw_string(main_x, dev_header_y, "v Devices and drives (1)", GUI_TEXT_SECONDARY, 1);

    /* Local Disk (C:) Drive Tile */
    int drive_y = dev_header_y + 20;
    int drive_w = 260;
    int drive_h = 56;

    if (g_selected_drive) {
        gui_fill_rounded_rect(main_x, drive_y, drive_w, drive_h, 4, 0x001E293B);
        gui_draw_rect(main_x, drive_y, drive_w, drive_h, GUI_ACCENT_BLUE);
    }

    /* Hard Disk Drive Icon */
    gui_fill_rounded_rect(main_x + 8, drive_y + 10, 36, 26, 4, 0x000284C7);
    gui_fill_rect(main_x + 12, drive_y + 14, 28, 12, 0x0038BDF8);
    gui_fill_rect(main_x + 14, drive_y + 30, 24, 3, 0x00CBD5E1);

    /* Drive Name & Storage Meter */
    gui_draw_string(main_x + 50, drive_y + 8, "Local Disk (C:)", GUI_TEXT_PRIMARY, 1);

    /* Storage Progress Bar (103 GB free of 222 GB) */
    int bar_w = 170;
    int bar_h = 10;
    gui_fill_rounded_rect(main_x + 50, drive_y + 26, bar_w, bar_h, 2, 0x00CBD5E1); /* Free white/gray */
    gui_fill_rounded_rect(main_x + 50, drive_y + 26, 92, bar_h, 2, 0x000078D4);    /* Used blue */

    gui_draw_string(main_x + 50, drive_y + 40, "103 GB free of 222 GB", GUI_TEXT_MUTED, 1);

    /* 7. Bottom Status Bar */
    int status_y = cy + ch - 22;
    gui_fill_rect(cx, status_y, cw, 22, 0x00181818);
    gui_draw_rect(cx, status_y, cw, 22, 0x002A2A2A);

    gui_draw_string(cx + 10, status_y + 4, "8 items |", GUI_TEXT_MUTED, 1);
    gui_draw_string(cx + cw - 40, status_y + 4, ":: #", GUI_TEXT_MUTED, 1);
}

Window* explorer_app_launch(void) {
    Window *win = window_create(
        "This PC",
        "explorer",
        70,
        40,
        800,
        500,
        on_explorer_paint,
        NULL
    );
    if (win) {
        window_set_callbacks(win, NULL, on_explorer_mouse);
    }
    return win;
}
