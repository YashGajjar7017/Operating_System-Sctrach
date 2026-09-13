/**
 * @file ps2.c
 * @brief PS/2 Mouse & Keyboard Hardware Driver Implementation
 */

#include "ps2.h"
#include "../arch/x86_64/io.h"

#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_COMMAND_PORT 0x64

#define PS2_STATUS_OUTPUT_BUFFER 0x01
#define PS2_STATUS_INPUT_BUFFER  0x02
#define PS2_STATUS_MOUSE_DATA    0x20

static int g_screen_w = 1280;
static int g_screen_h = 720;

static PS2MouseState g_mouse = {640, 360, 0, 0, 0, 0, 0, 0};
static uint8_t g_mouse_cycle = 0;
static uint8_t g_mouse_packet[3] = {0};

static uint8_t g_shift_state = 0;
static uint8_t g_ctrl_state = 0;
static uint8_t g_alt_state = 0;

static const char scancode_ascii_lower[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static const char scancode_ascii_upper[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,
    '*', 0,   ' ', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static void ps2_wait_write(void) {
    int timeout = 100000;
    while ((inb(PS2_STATUS_PORT) & PS2_STATUS_INPUT_BUFFER) && --timeout > 0) {
        io_wait();
    }
}

static void ps2_wait_read(void) {
    int timeout = 100000;
    while (!(inb(PS2_STATUS_PORT) & PS2_STATUS_OUTPUT_BUFFER) && --timeout > 0) {
        io_wait();
    }
}

static void ps2_mouse_write(uint8_t byte) {
    ps2_wait_write();
    outb(PS2_COMMAND_PORT, 0xD4); /* Signal controller to route next byte to auxiliary device (mouse) */
    ps2_wait_write();
    outb(PS2_DATA_PORT, byte);
}

static uint8_t ps2_mouse_read(void) {
    ps2_wait_read();
    return inb(PS2_DATA_PORT);
}

void ps2_set_screen_bounds(uint32_t screen_w, uint32_t screen_h) {
    if (screen_w > 0) g_screen_w = (int)screen_w;
    if (screen_h > 0) g_screen_h = (int)screen_h;
}

void ps2_init(uint32_t screen_w, uint32_t screen_h) {
    ps2_set_screen_bounds(screen_w, screen_h);
    g_mouse.x = g_screen_w / 2;
    g_mouse.y = g_screen_h / 2;
    g_mouse.dx = 0;
    g_mouse.dy = 0;
    g_mouse.left_button = 0;
    g_mouse.right_button = 0;
    g_mouse.middle_button = 0;
    g_mouse.updated = 0;
    g_mouse_cycle = 0;

    /* 1. Enable Auxiliary Device (Mouse) on PS/2 Controller */
    ps2_wait_write();
    outb(PS2_COMMAND_PORT, 0xA8);

    /* 2. Enable Keyboard & Mouse Interrupts in Controller Configuration Byte */
    ps2_wait_write();
    outb(PS2_COMMAND_PORT, 0x20); /* Read Config Byte */
    ps2_wait_read();
    uint8_t status = inb(PS2_DATA_PORT);
    status |= 0x03;               /* Enable IRQ1 and IRQ12 */
    status &= ~0x20;              /* Enable mouse clock */

    ps2_wait_write();
    outb(PS2_COMMAND_PORT, 0x60); /* Write Config Byte */
    ps2_wait_write();
    outb(PS2_DATA_PORT, status);

    /* 3. Tell Mouse to use default settings */
    ps2_mouse_write(0xF6);
    ps2_mouse_read(); /* Read ACK (0xFA) */

    /* 4. Enable Packet Streaming */
    ps2_mouse_write(0xF4);
    ps2_mouse_read(); /* Read ACK (0xFA) */
}

int ps2_poll_mouse(PS2MouseState *state) {
    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & PS2_STATUS_OUTPUT_BUFFER)) {
        return 0; /* No data ready */
    }

    /* Check if the data came from the mouse */
    if (!(status & PS2_STATUS_MOUSE_DATA)) {
        return 0; /* Keyboard data, handled separately */
    }

    uint8_t b = inb(PS2_DATA_PORT);

    if (g_mouse_cycle == 0) {
        /* Bit 3 of byte 0 must be 1 in standard PS/2 packet */
        if ((b & 0x08) == 0) {
            return 0; /* Discard out-of-sync byte */
        }
        g_mouse_packet[0] = b;
        g_mouse_cycle = 1;
        return 0;
    } else if (g_mouse_cycle == 1) {
        g_mouse_packet[1] = b;
        g_mouse_cycle = 2;
        return 0;
    } else if (g_mouse_cycle == 2) {
        g_mouse_packet[2] = b;
        g_mouse_cycle = 0;

        uint8_t flags = g_mouse_packet[0];
        int dx = (int)g_mouse_packet[1];
        int dy = (int)g_mouse_packet[2];

        /* Sign extension based on flags */
        if (flags & 0x10) {
            dx |= ~0xFF;
        }
        if (flags & 0x20) {
            dy |= ~0xFF;
        }

        /* Update buttons */
        g_mouse.left_button   = (flags & 0x01) ? 1 : 0;
        g_mouse.right_button  = (flags & 0x02) ? 1 : 0;
        g_mouse.middle_button = (flags & 0x04) ? 1 : 0;

        /* Apply delta (PS/2 inverted Y) */
        g_mouse.x += dx;
        g_mouse.y -= dy;

        /* Clamp to screen bounds */
        if (g_mouse.x < 0) g_mouse.x = 0;
        if (g_mouse.x >= g_screen_w) g_mouse.x = g_screen_w - 1;
        if (g_mouse.y < 0) g_mouse.y = 0;
        if (g_mouse.y >= g_screen_h) g_mouse.y = g_screen_h - 1;

        g_mouse.dx = dx;
        g_mouse.dy = dy;
        g_mouse.updated = 1;

        if (state) {
            *state = g_mouse;
        }
        return 1;
    }

    return 0;
}

int ps2_poll_keyboard(PS2KeyEvent *event) {
    uint8_t status = inb(PS2_STATUS_PORT);
    if (!(status & PS2_STATUS_OUTPUT_BUFFER)) {
        return 0;
    }

    /* Check if it is keyboard data */
    if (status & PS2_STATUS_MOUSE_DATA) {
        return 0; /* Mouse data */
    }

    uint8_t scancode = inb(PS2_DATA_PORT);
    uint8_t is_release = (scancode & 0x80) ? 1 : 0;
    uint8_t code = scancode & 0x7F;

    if (code == 0x2A || code == 0x36) {
        g_shift_state = !is_release;
    } else if (code == 0x1D) {
        g_ctrl_state = !is_release;
    } else if (code == 0x38) {
        g_alt_state = !is_release;
    }

    if (event) {
        event->scancode = scancode;
        event->is_pressed = !is_release;
        event->is_shift = g_shift_state;
        event->is_ctrl = g_ctrl_state;
        event->is_alt = g_alt_state;
        if (code < 128) {
            event->ascii = g_shift_state ? scancode_ascii_upper[code] : scancode_ascii_lower[code];
        } else {
            event->ascii = 0;
        }
    }

    return 1;
}
