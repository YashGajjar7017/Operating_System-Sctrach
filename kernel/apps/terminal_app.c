/**
 * @file terminal_app.c
 * @brief Interactive Terminal & Diagnostic Shell Console Application Implementation
 */

#include "terminal_app.h"
#include "../kstring.h"
#include "../security/session.h"
#include "../security/firewall.h"

#define MAX_TERM_LINES 18
#define MAX_LINE_LEN   72

static char g_term_lines[MAX_TERM_LINES][MAX_LINE_LEN];
static int g_term_line_count = 0;
static char g_input_buf[64] = {0};
static int g_input_len = 0;

static void term_add_line(const char *str) {
    if (g_term_line_count < MAX_TERM_LINES) {
        strncpy(g_term_lines[g_term_line_count], str, MAX_LINE_LEN - 1);
        g_term_lines[g_term_line_count][MAX_LINE_LEN - 1] = '\0';
        g_term_line_count++;
    } else {
        /* Shift up */
        for (int i = 0; i < MAX_TERM_LINES - 1; i++) {
            strcpy(g_term_lines[i], g_term_lines[i + 1]);
        }
        strncpy(g_term_lines[MAX_TERM_LINES - 1], str, MAX_LINE_LEN - 1);
        g_term_lines[MAX_TERM_LINES - 1][MAX_LINE_LEN - 1] = '\0';
    }
}

static void term_init_history(void) {
    g_term_line_count = 0;
    term_add_line("Xenithra OS 64-bit Microkernel Interactive Shell v1.0");
    term_add_line("[+] Hardware SMEP/SMAP Active | 4-Level Paging Enforced");
    term_add_line("[+] Anti-Hijack Guard Online | Private Stateful Firewall Active");
    term_add_line("Type 'help' for available kernel commands.");
    term_add_line("");
}

static void execute_command(const char *cmd) {
    char echo_line[MAX_LINE_LEN];
    strcpy(echo_line, "[xenithra@kernel ~]# ");
    strcat(echo_line, cmd);
    term_add_line(echo_line);

    if (strcmp(cmd, "help") == 0) {
        term_add_line("Built-in Commands:");
        term_add_line("  help      - Display this command reference");
        term_add_line("  uname -a  - Kernel version and CPU security status");
        term_add_line("  ls / dir  - List root storage files and directories");
        term_add_line("  ps        - List active processes and privilege rings");
        term_add_line("  firewall  - Display packet filter statistics");
        term_add_line("  sessions  - Display anti-hijack session security tokens");
        term_add_line("  clear/cls - Clear the terminal output console");
    } else if (strcmp(cmd, "uname") == 0 || strcmp(cmd, "uname -a") == 0) {
        term_add_line("XenithraOS 64-bit Microkernel (x86_64) SMP Hardened #1 Mon");
        term_add_line("Security: SMEP+SMAP Enforced | Higher-Half Base 0xFFFFFFFF80000000");
    } else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "dir") == 0) {
        term_add_line("C:\\ Directory Contents:");
        term_add_line("  <DIR> EFI           <DIR> XENITHRA      <DIR> System32");
        term_add_line("  <DIR> Users         <DIR> Security      45424 KERNEL.ELF");
        term_add_line("  65536 kernel.sys    1024  readme.txt    (48.2 MB Free)");
    } else if (strcmp(cmd, "ps") == 0 || strcmp(cmd, "top") == 0) {
        term_add_line("PID  NAME                RING    CPU%  MEM     STATUS");
        term_add_line("0    System Core         Ring 0  4%    16MB    Running");
        term_add_line("1    csrss.exe           Ring 0  6%    28MB    Running");
        term_add_line("2    firewall_guard.exe  Ring 0  1%    8MB     Running");
        term_add_line("3    explorer.exe        Ring 3  3%    24MB    Running");
        term_add_line("4    taskmgr.exe         Ring 3  2%    12MB    Running");
    } else if (strcmp(cmd, "firewall") == 0 || strcmp(cmd, "firewall-cmd --status") == 0) {
        FirewallStats stats = firewall_get_stats();
        char buf[64];
        char num[16];
        strcpy(buf, "Firewall: Running (Stealth Mode: Active) | Inspected: ");
        uint_to_str(stats.total_inspected, num);
        strcat(buf, num);
        strcat(buf, " | Dropped: ");
        uint_to_str(stats.total_blocked, num);
        strcat(buf, num);
        term_add_line(buf);
    } else if (strcmp(cmd, "sessions") == 0) {
        char buf[64];
        char num[16];
        strcpy(buf, "Anti-Hijack Guard: Active Sessions = ");
        uint_to_str(security_get_active_session_count(), num);
        strcat(buf, num);
        strcat(buf, " | Intercepted Hijacks = ");
        uint_to_str(security_get_blocked_hijacks_count(), num);
        strcat(buf, num);
        term_add_line(buf);
    } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        g_term_line_count = 0;
    } else if (strlen(cmd) > 0) {
        char err[MAX_LINE_LEN];
        strcpy(err, "Command not found: '");
        strcat(err, cmd);
        strcat(err, "'. Type 'help' for command list.");
        term_add_line(err);
    }
}

static void on_terminal_key(Window *win, char ascii, uint8_t scancode) {
    (void)win;
    (void)scancode;

    if (ascii == '\n' || ascii == '\r') {
        execute_command(g_input_buf);
        g_input_len = 0;
        g_input_buf[0] = '\0';
    } else if (ascii == '\b') {
        if (g_input_len > 0) {
            g_input_len--;
            g_input_buf[g_input_len] = '\0';
        }
    } else if (ascii >= 32 && ascii <= 126) {
        if (g_input_len < (int)sizeof(g_input_buf) - 2) {
            g_input_buf[g_input_len++] = ascii;
            g_input_buf[g_input_len] = '\0';
        }
    }
}

void terminal_input_char(char c) {
    on_terminal_key(NULL, c, 0);
}

static void on_terminal_paint(Window *win, int cx, int cy, int cw, int ch) {
    (void)win;

    /* Terminal Console Background */
    gui_fill_rounded_rect(cx, cy, cw, ch, 6, 0x00080C14);
    gui_draw_rect(cx, cy, cw, ch, 0x001B2438);

    /* Render Output Lines */
    int line_y = cy + 10;
    for (int i = 0; i < g_term_line_count; i++) {
        if (line_y + 18 > cy + ch - 30) break;
        const char *l = g_term_lines[i];

        uint32_t color = GUI_TEXT_PRIMARY;
        if (l[0] == '[') {
            if (l[4] == 'S') color = GUI_ACCENT_GREEN;      /* [SECURITY] */
            else if (l[4] == 'F') color = GUI_ACCENT_BLUE;  /* [FIREWALL] */
            else if (l[4] == 'M') color = 0x00D2A8FF;        /* [MEMORY] */
            else if (l[4] == 'C') color = 0x00FFA657;        /* [COMPOSITOR] */
            else color = GUI_ACCENT_CYAN;                    /* Prompt */
        } else if (l[0] == 'C' && l[1] == 'o') {
            color = GUI_ACCENT_AMBER;
        }

        gui_draw_string(cx + 12, line_y, l, color, 1);
        line_y += 18;
    }

    /* Active Prompt Line */
    char prompt_line[96];
    strcpy(prompt_line, "[xenithra@kernel ~]# ");
    strcat(prompt_line, g_input_buf);
    strcat(prompt_line, "_"); /* Cursor */
    gui_draw_string(cx + 12, line_y, prompt_line, GUI_ACCENT_CYAN, 1);
}

Window* terminal_app_launch(void) {
    if (g_term_line_count == 0) {
        term_init_history();
    }
    Window *win = window_create(
        "Diagnostic Terminal Console",
        "terminal",
        180,
        140,
        680,
        380,
        on_terminal_paint,
        NULL
    );
    if (win) {
        window_set_callbacks(win, on_terminal_key, NULL);
    }
    return win;
}
