/**
 * @file browser_app.h
 * @brief C-Based Fullscreen Browser Engine Connected Directly to Python 3.12 Django Backend
 */

#ifndef _KERNEL_APPS_BROWSER_APP_H_
#define _KERNEL_APPS_BROWSER_APP_H_

#include "../gui/compositor.h"
#include <stdint.h>

/* Kernel & System Call Entry Points */
void sys_launch_django_kiosk(void);
void browser_app_launch_django(uint16_t port);
void browser_app_launch(void);
void browser_app_navigate(const char *url);
void browser_app_search(const char *query);
void browser_app_init(void);

#endif /* _KERNEL_APPS_BROWSER_APP_H_ */
