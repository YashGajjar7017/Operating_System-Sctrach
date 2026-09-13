/**
 * @file explorer_app.c
 * @brief Windows-style File Explorer Application Implementation
 */

#include "explorer_app.h"
#include "../kstring.h"

typedef struct {
    char name[32];
    uint8_t is_dir;
    uint64_t size_bytes;
    const char *date;
    const char *type_desc;
} ExplorerItem;

#define MAX_DIR_ITEMS 16

static char g_current_path[64] = "C:\\";
static ExplorerItem g_items[MAX_DIR_ITEMS];
static int g_item_count = 0;
static int g_selected_index = -1;

static void load_directory(const char *path) {
    g_item_count = 0;
    g_selected_index = -1;
    strncpy(g_current_path, path, sizeof(g_current_path) - 1);

    if (strcmp(path, "C:\\") == 0 || strcmp(path, "This PC") == 0) {
        g_items[0] = (ExplorerItem){"EFI", 1, 0, "09/13/2026", "File folder"};
        g_items[1] = (ExplorerItem){"XENITHRA", 1, 0, "09/13/2026", "File folder"};
        g_items[2] = (ExplorerItem){"System32", 1, 0, "09/13/2026", "File folder"};
        g_items[3] = (ExplorerItem){"Users", 1, 0, "09/13/2026", "File folder"};
        g_items[4] = (ExplorerItem){"Security", 1, 0, "09/13/2026", "File folder"};
        g_items[5] = (ExplorerItem){"KERNEL.ELF", 0, 45424, "09/13/2026", "64-bit Executable"};
        g_items[6] = (ExplorerItem){"kernel.sys", 0, 65536, "09/13/2026", "System File"};
        g_items[7] = (ExplorerItem){"readme.txt", 0, 1024, "09/13/2026", "Text Document"};
        g_item_count = 8;
    } else if (strcmp(path, "C:\\EFI") == 0) {
        g_items[0] = (ExplorerItem){"BOOT", 1, 0, "09/13/2026", "File folder"};
        g_items[1] = (ExplorerItem){"BOOTX64.EFI", 0, 16896, "09/13/2026", "UEFI Application"};
        g_item_count = 2;
    } else if (strcmp(path, "C:\\XENITHRA") == 0) {
        g_items[0] = (ExplorerItem){"KERNEL.ELF", 0, 45424, "09/13/2026", "64-bit Kernel Binary"};
        g_items[1] = (ExplorerItem){"config.ini", 0, 512, "09/13/2026", "Configuration Settings"};
        g_items[2] = (ExplorerItem){"security.vault", 0, 2048, "09/13/2026", "Encrypted Vault"};
        g_item_count = 3;
    } else if (strcmp(path, "C:\\System32") == 0) {
        g_items[0] = (ExplorerItem){"compositor.sys", 0, 14200, "09/13/2026", "System Driver"};
        g_items[1] = (ExplorerItem){"firewall.sys", 0, 8920, "09/13/2026", "Security Filter"};
        g_items[2] = (ExplorerItem){"ps2mouse.sys", 0, 4120, "09/13/2026", "Input Driver"};
        g_items[3] = (ExplorerItem){"paging.sys", 0, 16384, "09/13/2026", "Memory Manager"};
        g_item_count = 4;
    } else if (strcmp(path, "C:\\Security") == 0) {
        g_items[0] = (ExplorerItem){"firewall.rules", 0, 1280, "09/13/2026", "Firewall Configuration"};
        g_items[1] = (ExplorerItem){"session_keys.dat", 0, 4096, "09/13/2026", "Security Token Store"};
        g_items[2] = (ExplorerItem){"audit_stream.log", 0, 8192, "09/13/2026", "Audit Log"};
        g_item_count = 3;
    } else {
        g_items[0] = (ExplorerItem){"desktop.ini", 0, 256, "09/13/2026", "Configuration Settings"};
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

    /* Top navigation buttons: Back (<), Up (^) */
    if (rel_y >= 38 && rel_y <= 62) {
        if (rel_x >= 12 && rel_x <= 36) {
            /* Back button */
            load_directory("C:\\");
            return;
        } else if (rel_x >= 40 && rel_x <= 64) {
            /* Up button */
            if (strcmp(g_current_path, "C:\\") != 0) {
                load_directory("C:\\");
            }
            return;
        }
    }

    /* Left Sidebar Tree Clicks */
    if (rel_x >= 12 && rel_x <= 180 && rel_y >= 74) {
        int sidebar_idx = (rel_y - 100) / 26;
        if (sidebar_idx == 0) load_directory("C:\\");
        else if (sidebar_idx == 1) load_directory("C:\\XENITHRA");
        else if (sidebar_idx == 2) load_directory("C:\\System32");
        else if (sidebar_idx == 3) load_directory("C:\\Security");
        return;
    }

    /* Main File Table Rows Clicks */
    int table_x = 196;
    int table_y = 100;
    if (rel_x >= table_x && rel_y >= table_y) {
        int row = (rel_y - table_y) / 28;
        if (row >= 0 && row < g_item_count) {
            if (g_selected_index == row && g_items[row].is_dir) {
                /* Double click / enter folder */
                char next_path[64];
                strcpy(next_path, "C:\\");
                strcat(next_path, g_items[row].name);
                load_directory(next_path);
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

    /* 2. Top Navigation & Command Bar */
    int bar_y = cy + 6;
    gui_fill_rounded_rect(cx + 8, bar_y, 26, 26, 4, GUI_BG_CARD);
    gui_draw_string(cx + 16, bar_y + 5, "<", GUI_TEXT_PRIMARY, 1);

    gui_fill_rounded_rect(cx + 38, bar_y, 26, 26, 4, GUI_BG_CARD);
    gui_draw_string(cx + 46, bar_y + 5, "^", GUI_TEXT_PRIMARY, 1);

    gui_fill_rounded_rect(cx + 68, bar_y, 26, 26, 4, GUI_BG_CARD);
    gui_draw_string(cx + 74, bar_y + 5, "O", GUI_ACCENT_CYAN, 1);

    /* Breadcrumb Address Bar */
    int addr_w = cw - 260;
    gui_fill_rounded_rect(cx + 102, bar_y, addr_w, 26, 4, GUI_BG_INPUT);
    gui_draw_rect(cx + 102, bar_y, addr_w, 26, GUI_BORDER_COLOR);
    gui_draw_string(cx + 112, bar_y + 5, "This PC > Local Disk (C:) > ", GUI_TEXT_MUTED, 1);
    gui_draw_string(cx + 112 + 224, bar_y + 5, (strcmp(g_current_path, "C:\\") == 0) ? "" : (g_current_path + 3), GUI_TEXT_PRIMARY, 1);

    /* Search Box */
    int search_x = cx + 110 + addr_w;
    int search_w = cw - (search_x - cx) - 12;
    gui_fill_rounded_rect(search_x, bar_y, search_w, 26, 4, GUI_BG_INPUT);
    gui_draw_rect(search_x, bar_y, search_w, 26, GUI_BORDER_COLOR);
    gui_draw_string(search_x + 8, bar_y + 5, "Search C:\\...", GUI_TEXT_MUTED, 1);

    /* 3. Action Ribbon */
    int ribbon_y = bar_y + 32;
    gui_fill_rect(cx, ribbon_y, cw, 28, GUI_BG_TITLEBAR_INACT);
    gui_draw_rect(cx, ribbon_y, cw, 28, GUI_BORDER_COLOR);

    gui_draw_string(cx + 14, ribbon_y + 6, "+ New Folder", GUI_ACCENT_CYAN, 1);
    gui_draw_string(cx + 130, ribbon_y + 6, "Cut", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 170, ribbon_y + 6, "Copy", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 215, ribbon_y + 6, "Paste", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 270, ribbon_y + 6, "Delete", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 330, ribbon_y + 6, "Properties", GUI_TEXT_SECONDARY, 1);

    /* 4. Left Sidebar Tree Pane */
    int side_w = 175;
    int side_y = ribbon_y + 28;
    int content_h = ch - (side_y - cy) - 28;

    gui_fill_rect(cx, side_y, side_w, content_h, 0x000E1524);
    gui_draw_rect(cx + side_w - 1, side_y, 1, content_h, GUI_BORDER_COLOR);

    gui_draw_string(cx + 12, side_y + 8, "v Quick Access", GUI_ACCENT_BLUE, 1);
    gui_draw_string(cx + 24, side_y + 28, "  Desktop", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 24, side_y + 48, "  Downloads", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 24, side_y + 68, "  Documents", GUI_TEXT_SECONDARY, 1);

    gui_draw_string(cx + 12, side_y + 94, "v This PC", GUI_ACCENT_BLUE, 1);
    gui_draw_string(cx + 24, side_y + 114, "[C:] Local Disk", (strcmp(g_current_path, "C:\\") == 0) ? GUI_ACCENT_CYAN : GUI_TEXT_PRIMARY, 1);
    gui_draw_string(cx + 24, side_y + 134, "[D:] EFI Boot", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(cx + 24, side_y + 154, "[S:] Security Vault", GUI_TEXT_SECONDARY, 1);

    /* 5. Main File Table */
    int main_x = cx + side_w;
    int main_w = cw - side_w;

    /* Table Header */
    gui_fill_rect(main_x, side_y, main_w, 24, 0x00141C2E);
    gui_draw_rect(main_x, side_y, main_w, 24, GUI_BORDER_COLOR);
    gui_draw_string(main_x + 16, side_y + 4, "Name", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(main_x + 190, side_y + 4, "Date modified", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(main_x + 310, side_y + 4, "Type", GUI_TEXT_SECONDARY, 1);
    gui_draw_string(main_x + 460, side_y + 4, "Size", GUI_TEXT_SECONDARY, 1);

    /* Table Rows */
    int row_y = side_y + 24;
    for (int i = 0; i < g_item_count; i++) {
        ExplorerItem *item = &g_items[i];
        int item_y = row_y + i * 28;
        if (item_y + 28 > side_y + content_h) break;

        uint32_t row_bg = (g_selected_index == i) ? GUI_BG_CARD_HOVER : ((i % 2 == 0) ? GUI_BG_WINDOW : 0x00131C2C);
        gui_fill_rect(main_x, item_y, main_w, 28, row_bg);
        if (g_selected_index == i) {
            gui_draw_rect(main_x, item_y, main_w, 28, GUI_ACCENT_BLUE);
        }

        /* Icon Badge */
        if (item->is_dir) {
            gui_fill_rounded_rect(main_x + 12, item_y + 5, 18, 18, 3, 0x00D97706); /* Amber folder */
            gui_draw_string(main_x + 16, item_y + 6, "F", 0x00FFFFFF, 1);
        } else {
            gui_fill_rounded_rect(main_x + 12, item_y + 5, 18, 18, 3, GUI_ACCENT_BLUE); /* Blue file */
            gui_draw_string(main_x + 16, item_y + 6, "*", 0x00FFFFFF, 1);
        }

        /* Name */
        gui_draw_string(main_x + 38, item_y + 6, item->name, GUI_TEXT_PRIMARY, 1);

        /* Date */
        gui_draw_string(main_x + 190, item_y + 6, item->date, GUI_TEXT_MUTED, 1);

        /* Type */
        gui_draw_string(main_x + 310, item_y + 6, item->type_desc, GUI_TEXT_SECONDARY, 1);

        /* Size */
        if (!item->is_dir) {
            char size_buf[32];
            uint_to_str(item->size_bytes / 1024, size_buf);
            strcat(size_buf, " KB");
            gui_draw_string(main_x + 460, item_y + 6, size_buf, GUI_TEXT_MUTED, 1);
        } else {
            gui_draw_string(main_x + 460, item_y + 6, "--", GUI_TEXT_MUTED, 1);
        }
    }

    /* 6. Bottom Status Bar */
    int status_y = cy + ch - 26;
    gui_fill_rect(cx, status_y, cw, 26, GUI_BG_TITLEBAR_INACT);
    gui_draw_rect(cx, status_y, cw, 26, GUI_BORDER_COLOR);

    char status_text[64];
    char count_buf[16];
    uint_to_str(g_item_count, count_buf);
    strcpy(status_text, count_buf);
    strcat(status_text, " items | Storage: 48.2 MB free of 64.0 MB (FAT32 ESP)");
    gui_draw_string(cx + 12, status_y + 5, status_text, GUI_TEXT_SECONDARY, 1);

    /* Visual Storage Bar */
    int sbar_x = cx + cw - 140;
    gui_fill_rounded_rect(sbar_x, status_y + 6, 120, 12, 3, 0x001E293B);
    gui_fill_rounded_rect(sbar_x, status_y + 6, 32, 12, 3, GUI_ACCENT_BLUE);
}

Window* explorer_app_launch(void) {
    load_directory("C:\\");
    Window *win = window_create(
        "File Explorer - Local Disk (C:)",
        "explorer",
        70,
        60,
        740,
        460,
        on_explorer_paint,
        NULL
    );
    if (win) {
        window_set_callbacks(win, NULL, on_explorer_mouse);
    }
    return win;
}
