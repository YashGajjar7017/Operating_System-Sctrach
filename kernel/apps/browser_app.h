/**
 * @file browser_app.h
 * @brief Microsoft Edge / React-based Web Browser App for Xenithra OS Kernel
 */

#ifndef _KERNEL_APPS_BROWSER_APP_H_
#define _KERNEL_APPS_BROWSER_APP_H_

#include "../gui/compositor.h"

void browser_app_launch(void);
void browser_app_navigate(const char *url);
void browser_app_search(const char *query);

#endif /* _KERNEL_APPS_BROWSER_APP_H_ */
