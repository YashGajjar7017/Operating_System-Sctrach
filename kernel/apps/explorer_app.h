/**
 * @file explorer_app.h
 * @brief Windows-style File Explorer Application for Xenithra OS
 */

#ifndef _KERNEL_APPS_EXPLORER_APP_H_
#define _KERNEL_APPS_EXPLORER_APP_H_

#include "../gui/compositor.h"

Window* explorer_app_launch(void);
void    explorer_navigate_to(const char *path);

#endif /* _KERNEL_APPS_EXPLORER_APP_H_ */
