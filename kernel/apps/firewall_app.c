/**
 * @file firewall_app.c
 * @brief Graphical Firewall & Anti-Hijack Security Center Application Implementation
 */

#include "firewall_app.h"
#include "../gui/dom_engine.h"
#include "../security/session.h"
#include "../security/firewall.h"

static void uint_to_str(uint64_t val, char *buf) {
    if (val == 0) {
        buf[0] = '0'; buf[1] = '\0';
        return;
    }
    char temp[24];
    int len = 0;
    while (val > 0) {
        temp[len++] = '0' + (val % 10);
        val /= 10;
    }
    for (int i = 0; i < len; i++) {
        buf[i] = temp[len - 1 - i];
    }
    buf[len] = '\0';
}

static void on_firewall_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* 1. Header Banner */
    gui_fill_rounded_rect(cx, cy, cw, 64, 8, 0x000F172A);
    gui_draw_rect(cx, cy, cw, 64, GUI_BORDER_COLOR);

    gui_draw_string(cx + 16, cy + 12, "Xenithra Private Firewall & Anti-Hijack Center", GUI_TEXT_WHITE, 1);
    gui_draw_string(cx + 16, cy + 34, "Hardware SMEP/SMAP Active | Token Randomized 128-bit", GUI_TEXT_MUTED, 1);

    /* Security Status Badge */
    gui_fill_rounded_rect(cx + cw - 160, cy + 16, 144, 32, 6, 0x0010B981);
    gui_draw_string(cx + cw - 148, cy + 24, "SYSTEM SECURED", 0x00FFFFFF, 1);

    /* 2. Three Metric Cards */
    int card_w = (cw - 32) / 3;
    int card_h = 96;
    int card_y = cy + 76;

    char num_buf[32];

    /* Card 1: Session Hijacking Guard */
    gui_fill_rounded_rect(cx, card_y, card_w, card_h, 8, 0x000F172A);
    gui_draw_rect(cx, card_y, card_w, card_h, GUI_BORDER_COLOR);
    gui_draw_string(cx + 14, card_y + 12, "Anti-Hijack Guard", GUI_ACCENT_CYAN, 1);
    gui_draw_string(cx + 14, card_y + 36, "Active Sessions: ", GUI_TEXT_MUTED, 1);
    uint_to_str(security_get_active_session_count(), num_buf);
    gui_draw_string(cx + 140, card_y + 36, num_buf, GUI_TEXT_WHITE, 1);
    gui_draw_string(cx + 14, card_y + 60, "Blocked Hijacks: ", GUI_TEXT_MUTED, 1);
    uint_to_str(security_get_blocked_hijacks_count(), num_buf);
    gui_draw_string(cx + 140, card_y + 60, num_buf, 0x0010B981, 1);

    /* Card 2: Firewall Packet Filter */
    int card2_x = cx + card_w + 16;
    FirewallStats fw_stats = firewall_get_stats();
    gui_fill_rounded_rect(card2_x, card_y, card_w, card_h, 8, 0x000F172A);
    gui_draw_rect(card2_x, card_y, card_w, card_h, GUI_BORDER_COLOR);
    gui_draw_string(card2_x + 14, card_y + 12, "Private Firewall", GUI_ACCENT_BLUE, 1);
    gui_draw_string(card2_x + 14, card_y + 36, "Total Filtered:  ", GUI_TEXT_MUTED, 1);
    uint_to_str(fw_stats.total_inspected, num_buf);
    gui_draw_string(card2_x + 140, card_y + 36, num_buf, GUI_TEXT_WHITE, 1);
    gui_draw_string(card2_x + 14, card_y + 60, "Threats Dropped: ", GUI_TEXT_MUTED, 1);
    uint_to_str(fw_stats.total_blocked, num_buf);
    gui_draw_string(card2_x + 140, card_y + 60, num_buf, 0x00EF4444, 1);

    /* Card 3: Memory Integrity & KASLR */
    int card3_x = card2_x + card_w + 16;
    gui_fill_rounded_rect(card3_x, card_y, card_w, card_h, 8, 0x000F172A);
    gui_draw_rect(card3_x, card_y, card_w, card_h, GUI_BORDER_COLOR);
    gui_draw_string(card3_x + 14, card_y + 12, "Kernel Integrity", 0x00D2A8FF, 1);
    gui_draw_string(card3_x + 14, card_y + 36, "SMEP Protection: ", GUI_TEXT_MUTED, 1);
    gui_draw_string(card3_x + 140, card_y + 36, "Enforced", 0x0010B981, 1);
    gui_draw_string(card3_x + 14, card_y + 60, "SMAP Protection: ", GUI_TEXT_MUTED, 1);
    gui_draw_string(card3_x + 140, card_y + 60, "Enforced", 0x0010B981, 1);

    /* 3. Active Firewall Rules List */
    int table_y = card_y + card_h + 16;
    int table_h = ch - (table_y - cy) - 56;
    gui_fill_rounded_rect(cx, table_y, cw, table_h, 8, 0x000F172A);
    gui_draw_rect(cx, table_y, cw, table_h, GUI_BORDER_COLOR);

    gui_draw_string(cx + 16, table_y + 12, "[Active Packet Inspection Rules]", GUI_TEXT_WHITE, 1);

    for (int i = 0; i < 4; i++) {
        const FirewallRule *r = firewall_get_rule(i);
        if (!r || !r->enabled) continue;

        int row_y = table_y + 40 + i * 32;
        gui_fill_rounded_rect(cx + 12, row_y, cw - 24, 26, 4, 0x001E293B);
        gui_draw_string(cx + 20, row_y + 5, r->name, GUI_TEXT_WHITE, 1);

        if (r->action == FIREWALL_ACTION_ALLOW) {
            gui_fill_rounded_rect(cx + cw - 90, row_y + 3, 60, 20, 3, 0x0010B981);
            gui_draw_string(cx + cw - 80, row_y + 5, "ALLOW", 0x00FFFFFF, 1);
        } else {
            gui_fill_rounded_rect(cx + cw - 90, row_y + 3, 60, 20, 3, 0x00EF4444);
            gui_draw_string(cx + cw - 78, row_y + 5, "DROP", 0x00FFFFFF, 1);
        }
    }

    /* 4. Quick Action Buttons */
    int btn_y = cy + ch - 40;
    gui_fill_rounded_rect(cx, btn_y, 180, 36, 6, GUI_ACCENT_BLUE);
    gui_draw_string(cx + 18, btn_y + 10, "+ Add Firewall Rule", 0x00FFFFFF, 1);

    gui_fill_rounded_rect(cx + 196, btn_y, 180, 36, 6, 0x00334155);
    gui_draw_string(cx + 214, btn_y + 10, "Rotate Session Keys", GUI_TEXT_WHITE, 1);

    gui_fill_rounded_rect(cx + 392, btn_y, 180, 36, 6, 0x00334155);
    gui_draw_string(cx + 410, btn_y + 10, "Flush Threat Logs", GUI_TEXT_WHITE, 1);
}

Window* firewall_app_launch(void) {
    return window_create(
        "Xenithra Security & Firewall Center",
        60,
        50,
        760,
        480,
        on_firewall_paint,
        NULL
    );
}
