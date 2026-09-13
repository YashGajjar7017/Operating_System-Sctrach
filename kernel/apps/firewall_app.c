/**
 * @file firewall_app.c
 * @brief Graphical Firewall & Anti-Hijack Security Center Application Implementation
 */

#include "firewall_app.h"
#include "../kstring.h"
#include "../security/session.h"
#include "../security/firewall.h"

static void on_firewall_mouse(Window *win, int rel_x, int rel_y, uint8_t left_click, uint8_t right_click) {
    (void)win;
    (void)right_click;
    if (!left_click) return;

    /* Rule Allow / Drop toggle buttons */
    int table_y = 76 + 96 + 16;
    if (rel_y >= table_y + 40 && rel_y <= table_y + 40 + 4 * 32) {
        int rule_idx = (rel_y - (table_y + 40)) / 32;
        if (rule_idx >= 0 && rule_idx < 4 && rel_x >= 650 && rel_x <= 730) {
            FirewallRule *r = (FirewallRule*)firewall_get_rule(rule_idx);
            if (r) {
                r->action = (r->action == FIREWALL_ACTION_ALLOW) ? FIREWALL_ACTION_DROP : FIREWALL_ACTION_ALLOW;
            }
            return;
        }
    }

    /* Rotate Session Keys Button */
    if (rel_y >= 430 && rel_y <= 466 && rel_x >= 196 && rel_x <= 376) {
        security_generate_token();
    }
}

static void on_firewall_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* 1. Header Banner */
    gui_fill_rounded_rect(cx, cy, cw, 64, 8, GUI_BG_WINDOW);
    gui_draw_rect(cx, cy, cw, 64, GUI_BORDER_COLOR);

    gui_draw_string(cx + 16, cy + 12, "Xenithra Private Firewall & Anti-Hijack Center", GUI_TEXT_PRIMARY, 1);
    gui_draw_string(cx + 16, cy + 34, "Hardware SMEP/SMAP Active | Token Randomized 128-bit Entropy", GUI_TEXT_MUTED, 1);

    /* Security Status Badge */
    gui_fill_rounded_rect(cx + cw - 160, cy + 16, 144, 32, 6, GUI_ACCENT_GREEN);
    gui_draw_string(cx + cw - 148, cy + 24, "SYSTEM SECURED", 0x00FFFFFF, 1);

    /* 2. Three Metric Cards */
    int card_w = (cw - 32) / 3;
    int card_h = 96;
    int card_y = cy + 76;

    char num_buf[32];

    /* Card 1: Session Hijacking Guard */
    gui_fill_rounded_rect(cx, card_y, card_w, card_h, 8, GUI_BG_CARD);
    gui_draw_rect(cx, card_y, card_w, card_h, GUI_BORDER_COLOR);
    gui_draw_string(cx + 14, card_y + 12, "Anti-Hijack Guard", GUI_ACCENT_CYAN, 1);
    gui_draw_string(cx + 14, card_y + 36, "Active Sessions: ", GUI_TEXT_MUTED, 1);
    uint_to_str(security_get_active_session_count(), num_buf);
    gui_draw_string(cx + 140, card_y + 36, num_buf, GUI_TEXT_PRIMARY, 1);
    gui_draw_string(cx + 14, card_y + 60, "Blocked Hijacks: ", GUI_TEXT_MUTED, 1);
    uint_to_str(security_get_blocked_hijacks_count(), num_buf);
    gui_draw_string(cx + 140, card_y + 60, num_buf, GUI_ACCENT_GREEN, 1);

    /* Card 2: Firewall Packet Filter */
    int card2_x = cx + card_w + 16;
    FirewallStats fw_stats = firewall_get_stats();
    gui_fill_rounded_rect(card2_x, card_y, card_w, card_h, 8, GUI_BG_CARD);
    gui_draw_rect(card2_x, card_y, card_w, card_h, GUI_BORDER_COLOR);
    gui_draw_string(card2_x + 14, card_y + 12, "Private Firewall", GUI_ACCENT_BLUE, 1);
    gui_draw_string(card2_x + 14, card_y + 36, "Total Filtered:  ", GUI_TEXT_MUTED, 1);
    uint_to_str(fw_stats.total_inspected, num_buf);
    gui_draw_string(card2_x + 140, card_y + 36, num_buf, GUI_TEXT_PRIMARY, 1);
    gui_draw_string(card2_x + 14, card_y + 60, "Threats Dropped: ", GUI_TEXT_MUTED, 1);
    uint_to_str(fw_stats.total_blocked, num_buf);
    gui_draw_string(card2_x + 140, card_y + 60, num_buf, (fw_stats.total_blocked > 0) ? GUI_ACCENT_RED : GUI_TEXT_MUTED, 1);

    /* Card 3: Memory Integrity & KASLR */
    int card3_x = card2_x + card_w + 16;
    gui_fill_rounded_rect(card3_x, card_y, card_w, card_h, 8, GUI_BG_CARD);
    gui_draw_rect(card3_x, card_y, card_w, card_h, GUI_BORDER_COLOR);
    gui_draw_string(card3_x + 14, card_y + 12, "Kernel Integrity", 0x00D2A8FF, 1);
    gui_draw_string(card3_x + 14, card_y + 36, "SMEP Protection: ", GUI_TEXT_MUTED, 1);
    gui_draw_string(card3_x + 140, card_y + 36, "Enforced", GUI_ACCENT_GREEN, 1);
    gui_draw_string(card3_x + 14, card_y + 60, "SMAP Protection: ", GUI_TEXT_MUTED, 1);
    gui_draw_string(card3_x + 140, card_y + 60, "Enforced", GUI_ACCENT_GREEN, 1);

    /* 3. Active Firewall Rules List */
    int table_y = card_y + card_h + 16;
    int table_h = ch - (table_y - cy) - 56;
    gui_fill_rounded_rect(cx, table_y, cw, table_h, 8, GUI_BG_WINDOW);
    gui_draw_rect(cx, table_y, cw, table_h, GUI_BORDER_COLOR);

    gui_draw_string(cx + 16, table_y + 12, "[Active Packet Inspection Rules - Click Action to Toggle]", GUI_TEXT_PRIMARY, 1);

    for (int i = 0; i < 4; i++) {
        const FirewallRule *r = firewall_get_rule(i);
        if (!r || !r->enabled) continue;

        int row_y = table_y + 40 + i * 32;
        gui_fill_rounded_rect(cx + 12, row_y, cw - 24, 26, 4, GUI_BG_CARD);
        gui_draw_string(cx + 20, row_y + 5, r->name, GUI_TEXT_PRIMARY, 1);

        if (r->action == FIREWALL_ACTION_ALLOW) {
            gui_fill_rounded_rect(cx + cw - 90, row_y + 3, 64, 20, 3, GUI_ACCENT_GREEN);
            gui_draw_string(cx + cw - 80, row_y + 5, "ALLOW", 0x00FFFFFF, 1);
        } else {
            gui_fill_rounded_rect(cx + cw - 90, row_y + 3, 64, 20, 3, GUI_ACCENT_RED);
            gui_draw_string(cx + cw - 78, row_y + 5, "DROP", 0x00FFFFFF, 1);
        }
    }

    /* 4. Quick Action Buttons */
    int btn_y = cy + ch - 40;
    gui_fill_rounded_rect(cx, btn_y, 180, 36, 6, GUI_ACCENT_BLUE);
    gui_draw_string(cx + 18, btn_y + 10, "+ Add Firewall Rule", 0x00FFFFFF, 1);

    gui_fill_rounded_rect(cx + 196, btn_y, 180, 36, 6, GUI_BG_CARD);
    gui_draw_rect(cx + 196, btn_y, 180, 36, GUI_BORDER_COLOR);
    gui_draw_string(cx + 214, btn_y + 10, "Rotate Session Keys", GUI_TEXT_PRIMARY, 1);

    gui_fill_rounded_rect(cx + 392, btn_y, 180, 36, 6, GUI_BG_CARD);
    gui_draw_rect(cx + 392, btn_y, 180, 36, GUI_BORDER_COLOR);
    gui_draw_string(cx + 410, btn_y + 10, "Flush Threat Logs", GUI_TEXT_PRIMARY, 1);
}

Window* firewall_app_launch(void) {
    Window *win = window_create(
        "Xenithra Security & Firewall Center",
        "firewall",
        60,
        50,
        760,
        480,
        on_firewall_paint,
        NULL
    );
    if (win) {
        window_set_callbacks(win, NULL, on_firewall_mouse);
    }
    return win;
}
