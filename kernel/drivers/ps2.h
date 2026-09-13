/**
 * @file ps2.h
 * @brief PS/2 Mouse and Keyboard Hardware Driver for Xenithra OS
 */

#ifndef _KERNEL_DRIVERS_PS2_H_
#define _KERNEL_DRIVERS_PS2_H_

#include <stdint.h>

typedef struct {
    int x;
    int y;
    int dx;
    int dy;
    uint8_t left_button;
    uint8_t right_button;
    uint8_t middle_button;
    uint8_t updated;
} PS2MouseState;

typedef struct {
    char ascii;
    uint8_t scancode;
    uint8_t is_pressed;
    uint8_t is_shift;
    uint8_t is_ctrl;
    uint8_t is_alt;
} PS2KeyEvent;

void ps2_init(uint32_t screen_w, uint32_t screen_h);
void ps2_set_screen_bounds(uint32_t screen_w, uint32_t screen_h);
int  ps2_poll_mouse(PS2MouseState *state);
int  ps2_poll_keyboard(PS2KeyEvent *event);

#endif /* _KERNEL_DRIVERS_PS2_H_ */
