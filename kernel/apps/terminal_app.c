/**
 * @file terminal_app.c
 * @brief Secure Terminal & Diagnostic Console Application Implementation
 */

#include "terminal_app.h"

static void on_terminal_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* Terminal Console Background */
    gui_fill_rounded_rect(cx, cy, cw, ch, 6, 0x000B0E14);
    gui_draw_rect(cx, cy, cw, ch, 0x001F242C);

    const char *lines[] = {
        "[xenithra@kernel ~]# uname -a",
        "XenithraOS 64-bit Microkernel (x86_64) SMP Hardened #1 Mon",
        "",
        "[+] [SECURITY] Hardware SMEP & SMAP activated (CR4.bit20/21)",
        "[+] [SECURITY] 128-bit Anti-Hijack Session Token Engine initialized",
        "[+] [FIREWALL] Private Stateful Packet Filter active (6 rules loaded)",
        "[+] [MEMORY] 4-Level Paging established. Higher-half base: 0xFFFFFFFF80000000",
        "[+] [COMPOSITOR] 32-bit Double-Buffered Window Server online",
        "",
        "[xenithra@kernel ~]# firewall-cmd --status",
        "State: Running (Stealth Mode: Enabled, Dropped Probes: 0)",
        "",
        "[xenithra@kernel ~]# _"
    };

    int line_y = cy + 12;
    for (int i = 0; i < 13; i++) {
        uint32_t color = GUI_TEXT_WHITE;
        if (lines[i][0] == '[') {
            if (lines[i][4] == 'S') color = 0x0010B981; /* Green security */
            else if (lines[i][4] == 'F') color = 0x0058A6FF; /* Blue firewall */
            else if (lines[i][4] == 'M') color = 0x00D2A8FF; /* Purple memory */
            else if (lines[i][4] == 'C') color = 0x00FFA657; /* Amber compositor */
            else color = GUI_ACCENT_CYAN;
        }

        gui_draw_string(cx + 12, line_y, lines[i], color, 1);
        line_y += 20;
    }
}

Window* terminal_app_launch(void) {
    return window_create(
        "Xenithra Diagnostic Console",
        180,
        140,
        640,
        340,
        on_terminal_paint,
        NULL
    );
}
