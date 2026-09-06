/**
 * @file ui.c
 * @brief Modern Windows-style Graphical Boot Selector UI implementation
 */

#include "ui.h"

static const BootEntry g_entries[MAX_BOOT_ENTRIES] = {
    {
        .title = "AuraOS - Modern Desktop Environment",
        .description = "Start standard 64-bit kernel with full graphical compositor & multitasking",
        .boot_mode = 0
    },
    {
        .title = "AuraOS - Safe Mode (Diagnostic Logging)",
        .description = "Start with verbose kernel memory logging, single-core APIC debug mode",
        .boot_mode = 1
    },
    {
        .title = "UEFI Firmware Diagnostics & Reboot",
        .description = "Perform a warm system restart into UEFI firmware setup",
        .boot_mode = 2
    },
    {
        .title = "Power Off System",
        .description = "Safely flush hardware states and perform complete ACPI shutdown",
        .boot_mode = 3
    }
};

static void draw_modern_logo(int cx, int cy, int size) {
    int half = size / 2;
    int gap = 3;
    int tile = half - gap;

    uint32_t c1 = 0x000078D4; /* Top-Left: Windows Fluent Blue */
    uint32_t c2 = 0x000086F8; /* Top-Right: Sky Accent */
    uint32_t c3 = 0x000067B8; /* Bottom-Left: Deep Azure */
    uint32_t c4 = 0x000078D4; /* Bottom-Right */

    /* 4-pane modern Fluent tile logo */
    gop_fill_rounded_rect(cx - half, cy - half, tile, tile, 2, c1);
    gop_fill_rounded_rect(cx + gap,  cy - half, tile, tile, 2, c2);
    gop_fill_rounded_rect(cx - half, cy + gap,  tile, tile, 2, c3);
    gop_fill_rounded_rect(cx + gap,  cy + gap,  tile, tile, 2, c4);
}

static void render_menu_frame(int selected_index, int remaining_ms, int total_ms, int timer_active) {
    /* 1. Dark Gradient Background */
    gop_draw_gradient_v(0, 0, g_gop_ctx.width, g_gop_ctx.height, 0x000B0E14, 0x00161B22);

    /* 2. Top Header & Modern Logo */
    int center_x = (int)g_gop_ctx.width / 2;
    int logo_y = 60;
    draw_modern_logo(center_x, logo_y, 44);

    gop_draw_string_centered(logo_y + 36, "AuraOS Boot Manager", COLOR_TEXT_PRIMARY, 2);
    gop_draw_string_centered(logo_y + 74, "Modern x86_64 Freestanding UEFI Environment", COLOR_TEXT_SECONDARY, 1);

    /* 3. Central Selection Card */
    int card_w = 640;
    if (card_w > (int)g_gop_ctx.width - 40) {
        card_w = (int)g_gop_ctx.width - 40;
    }
    int card_h = MAX_BOOT_ENTRIES * 68 + 24;
    int card_x = center_x - (card_w / 2);
    int card_y = logo_y + 110;

    /* Card Background & Subtle Acrylic Border */
    gop_fill_rounded_rect(card_x, card_y, card_w, card_h, 8, COLOR_BG_CARD);
    gop_draw_rect(card_x, card_y, card_w, card_h, COLOR_ACCENT_BORDER);

    /* 4. Render Menu Entries */
    for (int i = 0; i < MAX_BOOT_ENTRIES; i++) {
        int item_x = card_x + 12;
        int item_y = card_y + 12 + i * 68;
        int item_w = card_w - 24;
        int item_h = 60;

        int is_sel = (i == selected_index);

        if (is_sel) {
            /* Highlighted Active Item */
            gop_fill_rounded_rect(item_x, item_y, item_w, item_h, 6, COLOR_ACCENT_BLUE);
            gop_draw_rect(item_x, item_y, item_w, item_h, COLOR_ACCENT_HOVER);

            /* Selection Indicator Pill */
            gop_fill_rounded_rect(item_x + 6, item_y + 10, 4, item_h - 20, 2, 0x00FFFFFF);

            /* Text */
            gop_draw_string(item_x + 22, item_y + 12, g_entries[i].title, 0x00FFFFFF, 0, 1, 1);
            gop_draw_string(item_x + 22, item_y + 34, g_entries[i].description, 0x00D0E8FF, 0, 1, 1);
        } else {
            /* Inactive Item */
            gop_fill_rounded_rect(item_x, item_y, item_w, item_h, 6, COLOR_BG_CARD_HOVER);

            /* Text */
            gop_draw_string(item_x + 22, item_y + 12, g_entries[i].title, COLOR_TEXT_PRIMARY, 0, 1, 1);
            gop_draw_string(item_x + 22, item_y + 34, g_entries[i].description, COLOR_TEXT_SECONDARY, 0, 1, 1);
        }
    }

    /* 5. Countdown Progress Bar or Key Help */
    int footer_y = card_y + card_h + 30;

    if (timer_active && remaining_ms > 0) {
        int bar_w = 360;
        int bar_h = 6;
        int bar_x = center_x - (bar_w / 2);
        int bar_y = footer_y + 24;

        /* Progress fill */
        int fill_w = (bar_w * remaining_ms) / total_ms;
        if (fill_w > bar_w) fill_w = bar_w;
        if (fill_w < 0) fill_w = 0;

        char timer_buf[64];
        int sec = (remaining_ms + 999) / 1000;
        timer_buf[0] = 'A'; timer_buf[1] = 'u'; timer_buf[2] = 't'; timer_buf[3] = 'o';
        timer_buf[4] = '-'; timer_buf[5] = 'b'; timer_buf[6] = 'o'; timer_buf[7] = 'o';
        timer_buf[8] = 't'; timer_buf[9] = 'i'; timer_buf[10] = 'n'; timer_buf[11] = 'g';
        timer_buf[12] = ' '; timer_buf[13] = 'i'; timer_buf[14] = 'n'; timer_buf[15] = ' ';
        timer_buf[16] = '0' + (sec % 10);
        timer_buf[17] = 's'; timer_buf[18] = '.'; timer_buf[19] = '.'; timer_buf[20] = '.';
        timer_buf[21] = '\0';

        gop_draw_string_centered(footer_y, timer_buf, COLOR_TEXT_ACCENT, 1);

        /* Background bar */
        gop_fill_rounded_rect(bar_x, bar_y, bar_w, bar_h, 3, COLOR_BG_CARD_HOVER);
        /* Active progress */
        if (fill_w > 0) {
            gop_fill_rounded_rect(bar_x, bar_y, fill_w, bar_h, 3, COLOR_ACCENT_BLUE);
        }
    } else {
        gop_draw_string_centered(footer_y, "Use [Up / Down] arrows to select, [Enter] to boot", COLOR_TEXT_SECONDARY, 1);
    }

    /* 6. Footer Signature */
    gop_draw_string_centered(g_gop_ctx.height - 30, "AuraOS UEFI Boot Protocol v2.0 | Press [ESC] to reboot", 0x00484F58, 1);

    /* 7. Swap to Screen */
    gop_swap_buffers();
}

BootSelectionResult ui_run_boot_menu(EFI_SYSTEM_TABLE *SystemTable, uint32_t timeout_seconds) {
    BootSelectionResult result;
    result.action = BOOT_ACTION_START_OS;
    result.selected_mode = 0;

    int selected_index = 0;
    int total_ms = (int)timeout_seconds * 1000;
    int remaining_ms = total_ms;
    int timer_active = (timeout_seconds > 0) ? 1 : 0;
    const int frame_delay_ms = 40; /* ~25 FPS */

    /* Reset console input */
    if (SystemTable->ConIn) {
        SystemTable->ConIn->Reset(SystemTable->ConIn, FALSE);
    }

    while (1) {
        /* Render current frame */
        render_menu_frame(selected_index, remaining_ms, total_ms, timer_active);

        /* Poll for keystrokes */
        if (SystemTable->ConIn) {
            EFI_INPUT_KEY key;
            EFI_STATUS status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &key);

            if (!EFI_ERROR(status)) {
                /* Key pressed: stop automatic countdown */
                timer_active = 0;

                if (key.ScanCode == SCAN_UP) {
                    selected_index = (selected_index - 1 + MAX_BOOT_ENTRIES) % MAX_BOOT_ENTRIES;
                } else if (key.ScanCode == SCAN_DOWN) {
                    selected_index = (selected_index + 1) % MAX_BOOT_ENTRIES;
                } else if (key.UnicodeChar == '\r' || key.UnicodeChar == '\n') {
                    /* Enter key selected */
                    break;
                } else if (key.ScanCode == SCAN_ESC) {
                    result.action = BOOT_ACTION_REBOOT;
                    return result;
                }
            }
        }

        /* Check countdown timer */
        if (timer_active) {
            remaining_ms -= frame_delay_ms;
            if (remaining_ms <= 0) {
                /* Timer expired: proceed with default selected item */
                break;
            }
        }

        /* Sleep 40ms */
        SystemTable->BootServices->Stall(frame_delay_ms * 1000);
    }

    /* Process selected item */
    if (selected_index == 0) {
        result.action = BOOT_ACTION_START_OS;
        result.selected_mode = 0;
    } else if (selected_index == 1) {
        result.action = BOOT_ACTION_START_OS;
        result.selected_mode = 1;
    } else if (selected_index == 2) {
        result.action = BOOT_ACTION_REBOOT;
        result.selected_mode = 2;
    } else {
        result.action = BOOT_ACTION_SHUTDOWN;
        result.selected_mode = 3;
    }

    return result;
}
