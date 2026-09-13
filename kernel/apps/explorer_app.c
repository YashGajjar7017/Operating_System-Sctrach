/**
 * @file explorer_app.c
 * @brief Authentic Windows 11 File Explorer Application
 */

#include "explorer_app.h"
#include "../kstring.h"
#include "vlc_app.h"
#include "installer_app.h"

typedef struct {
    char name[32];
    uint8_t is_dir;
    uint64_t size_bytes;
    const char *date;
    const char *type_desc;
    const char *icon_tag;
    uint32_t icon_color;
} ExplorerItem;

#define MAX_DIR_ITEMS 16

static char g_current_path[64] = "C:\\";
static ExplorerItem g_items[MAX_DIR_ITEMS];
static int g_item_count = 0;
static int g_selected_index = -1;
static int g_active_tab = 1; /* 0: Home, 1: Local Disk (C:), 2: Videos */

static void load_directory(const char *path) {
    g_item_count = 0;
    g_selected_index = -1;
    strncpy(g_current_path, path, sizeof(g_current_path) - 1);

    if (strcmp(path, "C:\\") == 0 || strcmp(path, "This PC") == 0) {
        g_items[0] = (ExplorerItem){"EFI", 1, 0, "09/13/2026 11:20 AM", "File folder", "DIR", 0x00F59E0B};
        g_items[1] = (ExplorerItem){"XENITHRA", 1, 0, "09/13/2026 11:20 AM", "File folder", "DIR", 0x00F59E0B};
        g_items[2] = (ExplorerItem){"System32", 1, 0, "09/13/2026 11:20 AM", "File folder", "DIR", 0x00F59E0B};
        g_items[3] = (ExplorerItem){"Program Files", 1, 0, "09/13/2026 11:20 AM", "File folder", "DIR", 0x00F59E0B};
        g_items[4] = (ExplorerItem){"Videos", 1, 0, "09/13/2026 11:20 AM", "Media folder", "DIR", 0x00F97316};
        g_items[5] = (ExplorerItem){"matrix_intro_64.mp4", 0, 48234496, "09/13/2026 10:15 AM", "MP4 Video File", "MP4", 0x00F97316};
        g_items[6] = (ExplorerItem){"vlc_setup_x64.exe", 0, 50593792, "09/13/2026 09:30 AM", "Application", "EXE", 0x000078D4};
        g_items[7] = (ExplorerItem){"KERNEL.ELF", 0, 85376, "09/13/2026 11:00 AM", "64-bit Kernel ELF", "SYS", 0x008B5CF6};
        g_items[8] = (ExplorerItem){"readme.txt", 0, 1420, "09/13/2026 08:45 AM", "Text Document", "TXT", 0x0038BDF8};
        g_item_count = 9;
    } else if (strcmp(path, "C:\\Videos") == 0) {
        g_items[0] = (ExplorerItem){"matrix_intro_64.mp4", 0, 48234496, "09/13/2026 10:15 AM", "MP4 Video File", "MP4", 0x00F97316};
        g_items[1] = (ExplorerItem){"cyber_sentinel_trailer.mp4", 0, 31457280, "09/13/2026 10:30 AM", "MP4 Video File", "MP4", 0x00F97316};
        g_items[2] = (ExplorerItem){"bloom_visualizer_demo.mp4", 0, 24117248, "09/13/2026 10:45 AM", "MP4 Video File", "MP4", 0x00F97316};
        g_items[3] = (ExplorerItem){"xenithra_security_doc.mp4", 0, 52428800, "09/13/2026 11:00 AM", "MP4 Video File", "MP4", 0x00F97316};
        g_item_count = 4;
    } else if (strcmp(path, "C:\\EFI") == 0) {
        g_items[0] = (ExplorerItem){"BOOT", 1, 0, "09/13/2026 11:20 AM", "File folder", "DIR", 0x00F59E0B};
        g_items[1] = (ExplorerItem){"BOOTX64.EFI", 0, 16896, "09/13/2026 11:20 AM", "UEFI Application", "EFI", 0x000078D4};
        g_item_count = 2;
    } else if (strcmp(path, "C:\\XENITHRA") == 0) {
        g_items[0] = (ExplorerItem){"KERNEL.ELF", 0, 85376, "09/13/2026 11:00 AM", "64-bit Kernel Binary", "SYS", 0x008B5CF6};
        g_items[1] = (ExplorerItem){"config.ini", 0, 512, "09/13/2026 11:00 AM", "Configuration Settings", "INI", 0x0064748B};
        g_items[2] = (ExplorerItem){"security.vault", 0, 4096, "09/13/2026 11:00 AM", "Encrypted Key Vault", "SEC", 0x0010B981};
        g_item_count = 3;
    } else if (strcmp(path, "C:\\System32") == 0) {
        g_items[0] = (ExplorerItem){"compositor.sys", 0, 34200, "09/13/2026 11:00 AM", "System Compositor", "SYS", 0x008B5CF6};
        g_items[1] = (ExplorerItem){"firewall.sys", 0, 18920, "09/13/2026 11:00 AM", "Security Driver", "SYS", 0x0010B981};
        g_items[2] = (ExplorerItem){"ps2mouse.sys", 0, 8120, "09/13/2026 11:00 AM", "Input Hardware Driver", "SYS", 0x0064748B};
        g_items[3] = (ExplorerItem){"session_guard.sys", 0, 16384, "09/13/2026 11:00 AM", "Anti-Hijack Filter", "SYS", 0x0010B981};
        g_item_count = 4;
    } else if (strcmp(path, "C:\\Program Files") == 0) {
        g_items[0] = (ExplorerItem){"VLC", 1, 0, "09/13/2026 11:20 AM", "File folder", "DIR", 0x00F97316};
        g_items[1] = (ExplorerItem){"Xenithra Security", 1, 0, "09/13/2026 11:20 AM", "File folder", "DIR", 0x0010B981};
        g_items[2] = (ExplorerItem){"vlc.exe", 0, 48200000, "09/13/2026 11:20 AM", "Application", "EXE", 0x00F97316};
        g_item_count = 3;
    } else {
        g_items[0] = (ExplorerItem){"desktop.ini", 0, 256, "09/13/2026 11:00 AM", "Configuration Settings", "INI", 0x0064748B};
        g_item_count = 1;
    }
}

void explorer_navigate_to(const char *path) {
    load_directory(path);
}

static void on_explorer_mouse(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click) {
    (void)win;
    (void)right_click;
    if (!left_click) return;

    /* 1. Top Mica Tabs (Y: 0 - 30) */
    if (rel_y >= 0 && rel_y <= 32) {
        if (rel_x >= 12 && rel_x <= 110) {
            g_active_tab = 0;
            load_directory("This PC");
            return;
        } else if (rel_x >= 115 && rel_x <= 250) {
            g_active_tab = 1;
            load_directory("C:\\");
            return;
        } else if (rel_x >= 255 && rel_x <= 360) {
            g_active_tab = 2;
            load_directory("C:\\Videos");
            return;
        }
    }

    /* 2. Top Navigation Bar (Y: 34 - 64) */
    if (rel_y >= 34 && rel_y <= 64) {
        /* Back button [<-] */
        if (rel_x >= 10 && rel_x <= 34) {
            load_directory("C:\\");
            return;
        }
        /* Forward button [->] */
        if (rel_x >= 38 && rel_x <= 62) {
            load_directory("C:\\Videos");
            return;
        }
        /* Up button [^] */
        if (rel_x >= 66 && rel_x <= 90) {
            if (strcmp(g_current_path, "C:\\") != 0) {
                load_directory("C:\\");
            }
            return;
        }
        /* Refresh button [R] */
        if (rel_x >= 94 && rel_x <= 118) {
            load_directory(g_current_path);
            return;
        }
    }

    /* 3. Action Ribbon Bar (Y: 66 - 96) */
    if (rel_y >= 66 && rel_y <= 96) {
        /* [+ New] */
        if (rel_x >= 12 && rel_x <= 90) {
            /* Create new folder trigger */
            return;
        }
        /* [Sort / View] */
    }

    /* 4. Left Navigation Tree Clicks (Y: 100 - 400, X: 0 - 180) */
    if (rel_x >= 0 && rel_x <= 180 && rel_y >= 100) {
        int side_idx = (rel_y - 120) / 24;
        if (side_idx == 0) load_directory("C:\\");          /* Desktop */
        else if (side_idx == 1) load_directory("C:\\Program Files"); /* Downloads */
        else if (side_idx == 2) load_directory("C:\\XENITHRA"); /* Documents */
        else if (side_idx == 3) load_directory("C:\\Videos");    /* Videos */
        else if (side_idx == 5) load_directory("C:\\");          /* Local Disk C: */
        else if (side_idx == 6) load_directory("C:\\EFI");       /* EFI System D: */
        else if (side_idx == 7) load_directory("C:\\System32");  /* System32 */
        return;
    }

    /* 5. Main File Table Rows Clicks */
    int table_x = 186;
    int table_y = 126;
    if (rel_x >= table_x && rel_y >= table_y) {
        int row = (rel_y - table_y) / 28;
        if (row >= 0 && row < g_item_count) {
            if (g_selected_index == row) {
                /* Double click action */
                if (g_items[row].is_dir) {
                    char next_path[64];
                    strcpy(next_path, "C:\\");
                    strcat(next_path, g_items[row].name);
                    load_directory(next_path);
                } else if (strcmp(g_items[row].icon_tag, "MP4") == 0) {
                    vlc_app_launch();
                } else if (strcmp(g_items[row].icon_tag, "EXE") == 0) {
                    installer_app_launch();
                }
            } else {
                g_selected_index = row;
            }
        }
    }
}

static void on_explorer_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* 1. Main Background */
    gui_fill_rect(cx, cy, cw, ch, GUI_BG_WINDOW);

    /* 2. Windows 11 Fluent Tab Bar (Top) */
    int tab_h = 30;
    gui_fill_rect(cx, cy, cw, tab_h, 0x000E1422);
    gui_draw_rect(cx, cy, cw, tab_h, GUI_BORDER_COLOR);

    /* Tab 0: Home */
    gui_fill_rounded_rect(cx + 8, cy + 3, 96, 26, 4, (g_active_tab == 0) ? GUI_BG_WINDOW : GUI_BG_CARD);
    if (g_active_tab == 0) gui_draw_rect(cx + 8, cy + 3, 96, 26, GUI_BORDER_COLOR);
    gui_draw_fluent_icon_explorer(cx + 12, cy + 5);
    gui_draw_string(cx + 34, cy + 8, "Home", (g_active_tab == 0) ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1);

    /* Tab 1: Local Disk (C:) */
    gui_fill_rounded_rect(cx + 108, cy + 3, 140, 26, 4, (g_active_tab == 1) ? GUI_BG_WINDOW : GUI_BG_CARD);
    if (g_active_tab == 1) gui_draw_rect(cx + 108, cy + 3, 140, 26, GUI_BORDER_COLOR);
    gui_draw_fluent_icon_this_pc(cx + 112, cy + 5);
    gui_draw_string(cx + 134, cy + 8, "Local Disk (C:)", (g_active_tab == 1) ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1);

    /* Tab 2: Videos */
    gui_fill_rounded_rect(cx + 252, cy + 3, 100, 26, 4, (g_active_tab == 2) ? GUI_BG_WINDOW : GUI_BG_CARD);
    if (g_active_tab == 2) gui_draw_rect(cx + 252, cy + 3, 100, 26, GUI_BORDER_COLOR);
    gui_draw_fluent_icon_vlc(cx + 256, cy + 5);
    gui_draw_string(cx + 278, cy + 8, "Videos", (g_active_tab == 2) ? GUI_TEXT_PRIMARY : GUI_TEXT_SECONDARY, 1);

    /* New Tab [+] */
    gui_fill_rounded_rect(cx + 356, cy + 4, 24, 24, 4, GUI_BG_CARD);
    gui_draw_string(cx + 363, cy + 8, "+", GUI_TEXT_SECONDARY, 1);

    /* 3. Modern Breadcrumb Address & Search Bar */
    int nav_y = cy + tab_h + 4;
    int nav_h = 32;

    /* Back [<], Forward [>], Up [^], Refresh [R] */
    gui_fill_rounded_rect(cx + 8, nav_y, 26, 26, 4, GUI_BG_CARD);
    gui_draw_string(cx + 16, nav_y + 5, "<", GUI_TEXT_PRIMARY, 1);

    gui_fill_rounded_rect(cx + 38, nav_y, 26, 26, 4, GUI_BG_CARD);
    gui_draw_string(cx + 46, nav_y + 5, ">", GUI_TEXT_MUTED, 1);

    gui_fill_rounded_rect(cx + 68, nav_y, 26, 26, 4, GUI_BG_CARD);
    gui_draw_string(cx + 76, nav_y + 5, "^", GUI_TEXT_PRIMARY, 1);

    gui_fill_rounded_rect(cx + 98, nav_y, 26, 26, 4, GUI_BG_CARD);
    gui_draw_string(cx + 104, nav_y + 5, "O", GUI_ACCENT_CYAN, 1);

    /* Breadcrumb Path Pill */
    int addr_x = cx + 130;
    int addr_w = cw - 380;
    gui_fill_rounded_rect(addr_x, nav_y, addr_w, 28, 6, GUI_BG_INPUT);
    gui_draw_rect(addr_x, nav_y, addr_w, 28, GUI_BORDER_COLOR);
    gui_draw_fluent_icon_this_pc(addr_x + 6, nav_y + 4);
    gui_draw_string(addr_x + 28, nav_y + 6, "This PC > Local Disk (C:) > ", GUI_TEXT_MUTED, 1);
    gui_draw_string(addr_x + 28 + 224, nav_y + 6, (strcmp(g_current_path, "C:\\") == 0) ? "" : (g_current_path + 3), GUI_ACCENT_CYAN, 1);

    /* Search Box */
    int search_x = addr_x + addr_w + 10;
    int search_w = cw - (search_x - cx) - 10;
    gui_fill_rounded_rect(search_x, nav_y, search_w, 28, 6, GUI_BG_INPUT);
    gui_draw_rect(search_x, nav_y, search_w, 28, GUI_BORDER_COLOR);
    gui_draw_string(search_x + 10, nav_y + 6, "Search in C:\\...", GUI_TEXT_MUTED, 1);

    /* 4. Windows 11 Action Command Ribbon */
    int ribbon_y = nav_y + nav_h + 4;
    int ribbon_h = 32;
    gui_fill_rect(cx, ribbon_y, cw, ribbon_h, 0x00131B2B);
    gui_draw_rect(cx, ribbon_y, cw, ribbon_h, GUI_BORDER_COLOR);

    gui_fill_rounded_rect(cx + 10, ribbon_y + 3, 80, 26, 4, GUI_ACCENT_BLUE);
    gui_draw_string(cx + 18, ribbon_y + 8, "+ New", 0x00FFFFFF, 1);

    gui_draw_string(cx + 104, ribbon_y + 8, "Cut", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 144, ribbon_y + 8, "Copy", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 190, ribbon_y + 8, "Paste", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 244, ribbon_y + 8, "Rename", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 304, ribbon_y + 8, "Share", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 358, ribbon_y + 8, "Delete", GUI_TEXT_SECONDARY, 1);

    gui_draw_rect(cx + 420, ribbon_y + 6, 1, 20, GUI_BORDER_COLOR);
    gui_draw_string(cx + 434, ribbon_y + 8, "Sort v", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(cx + 494, ribbon_y + 8, "View v", GUI_TEXT_PRIMARY, 1);

    /* 5. Left Navigation Sidebar */
    int side_w = 180;
    int side_y = ribbon_y + ribbon_h;
    int content_h = ch - (side_y - cy) - 28;

    gui_fill_rect(cx, side_y, side_w, content_h, 0x000E1422);
    gui_draw_rect(cx + side_w - 1, side_y, 1, content_h, GUI_BORDER_COLOR);

    gui_draw_string(cx + 12, side_y + 8, "Quick Access", GUI_ACCENT_BLUE, 1);
    gui_draw_string(cx + 22, side_y + 26, "- Desktop", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 22, side_y + 46, "- Downloads", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 22, side_y + 66, "- Documents", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 22, side_y + 86, "- Videos", (strcmp(g_current_path, "C:\\Videos") == 0) ? GUI_ACCENT_ORANGE : GUI_TEXT_SECONDARY, 1);

    gui_draw_string(cx + 12, side_y + 114, "This PC", GUI_ACCENT_BLUE, 1);
    gui_draw_string(cx + 22, side_y + 132, "[C:] Local Disk", (strcmp(g_current_path, "C:\\") == 0) ? GUI_ACCENT_CYAN : GUI_TEXT_PRIMARY, 1);

    /* Small Storage Usage Meter for C: */
    gui_fill_rounded_rect(cx + 22, side_y + 150, 130, 6, 3, 0x001E293B);
    gui_fill_rounded_rect(cx + 22, side_y + 150, 48, 6, 3, GUI_ACCENT_BLUE);
    gui_draw_string(cx + 22, side_y + 158, "48.2 MB free / 64 MB", GUI_TEXT_MUTED, 1);

    gui_draw_string(cx + 22, side_y + 178, "[D:] EFI System", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 22, side_y + 198, "[S:] Security Vault", GUI_TEXT_SECONDARY, 1);

    /* 6. Main File Content Table */
    int main_x = cx + side_w;
    int main_w = cw - side_w;

    /* Table Column Header */
    gui_fill_rect(main_x, side_y, main_w, 24, 0x00141C2E);
    gui_draw_rect(main_x, side_y, main_w, 24, GUI_BORDER_COLOR);
    gui_draw_string(main_x + 16, side_y + 4, "Name", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(main_x + 220, side_y + 4, "Date modified", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(main_x + 360, side_y + 4, "Type", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(main_x + 480, side_y + 4, "Size", GUI_TEXT_SECONDARY, 1);

    /* File Rows */
    int row_y = side_y + 24;
    for (int i = 0; i < g_item_count; i++) {
        ExplorerItem *item = &g_items[i];
        int item_y = row_y + i * 28;
        if (item_y + 28 > side_y + content_h) break;

        uint32_t row_bg = (g_selected_index == i) ? GUI_BG_CARD_HOVER : ((i % 2 == 0) ? GUI_BG_WINDOW : 0x00121A28);
        gui_fill_rect(main_x, item_y, main_w, 28, row_bg);
        if (g_selected_index == i) {
            gui_draw_rect(main_x, item_y, main_w, 28, GUI_ACCENT_BLUE);
        }

        /* Fluent File Icon Badge */
        gui_fill_rounded_rect(main_x + 12, item_y + 4, 20, 20, 4, item->icon_color);
        gui_draw_string(main_x + 15, item_y + 6, (strcmp(item->icon_tag, "DIR") == 0) ? "F" : ((strcmp(item->icon_tag, "MP4") == 0) ? "V" : "*"), 0x00FFFFFF, 1);

        /* Name */
        gui_draw_string(main_x + 40, item_y + 6, item->name, (g_selected_index == i) ? GUI_ACCENT_CYAN : GUI_TEXT_PRIMARY, 1);

        /* Date */
        gui_draw_string(main_x + 220, item_y + 6, item->date, GUI_TEXT_MUTED, 1);

        /* Type */
        gui_draw_string(main_x + 360, item_y + 6, item->type_desc, GUI_TEXT_SECONDARY, 1);

        /* Size */
        if (!item->is_dir) {
            char size_buf[32];
            if (item->size_bytes >= 1048576) {
                uint_to_str(item->size_bytes / 1048576, size_buf);
                strcat(size_buf, " MB");
            } else {
                uint_to_str(item->size_bytes / 1024, size_buf);
                strcat(size_buf, " KB");
            }
            gui_draw_string(main_x + 480, item_y + 6, size_buf, GUI_TEXT_MUTED, 1);
        } else {
            gui_draw_string(main_x + 480, item_y + 6, "--", GUI_TEXT_MUTED, 1);
        }
    }

    /* 7. Bottom Status Bar */
    int status_y = cy + ch - 26;
    gui_fill_rect(cx, status_y, cw, 26, 0x000E1422);
    gui_draw_rect(cx, status_y, cw, 26, GUI_BORDER_COLOR);

    char status_text[80];
    char count_buf[16];
    uint_to_str(g_item_count, count_buf);
    strcpy(status_text, count_buf);
    strcat(status_text, " items | Local Disk (C:) FAT32 ESP | 48.2 MB free of 64.0 MB");
    gui_draw_string(cx + 12, status_y + 5, status_text, GUI_TEXT_SECONDARY, 1);

    /* Storage Indicator Progress Bar */
    int sbar_x = cx + cw - 150;
    gui_fill_rounded_rect(sbar_x, status_y + 7, 130, 12, 4, 0x001E293B);
    gui_fill_rounded_rect(sbar_x, status_y + 7, 48, 12, 4, GUI_ACCENT_BLUE);
}

Window* explorer_app_launch(void) {
    load_directory("C:\\");
    Window *win = window_create(
        "File Explorer",
        "explorer",
        60,
        40,
        780,
        490,
        on_explorer_paint,
        NULL
    );
    if (win) {
        window_set_callbacks(win, NULL, on_explorer_mouse);
    }
    return win;
}
