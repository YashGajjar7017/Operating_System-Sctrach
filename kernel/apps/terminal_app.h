/**
 * @file terminal_app.h
 * @brief Interactive Terminal & Shell Console Application for Xenithra OS
 */

#ifndef _KERNEL_APPS_TERMINAL_APP_H_
#define _KERNEL_APPS_TERMINAL_APP_H_

#include "../gui/compositor.h"

Window* terminal_app_launch(void);
void    terminal_input_char(char c);

#endif /* _KERNEL_APPS_TERMINAL_APP_H_ */
