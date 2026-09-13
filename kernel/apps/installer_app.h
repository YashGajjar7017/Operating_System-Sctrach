/**
 * @file installer_app.h
 * @brief Windows 11 64-bit App Installer & Package Manager
 */

#ifndef _KERNEL_APPS_INSTALLER_APP_H_
#define _KERNEL_APPS_INSTALLER_APP_H_

#include "../gui/compositor.h"

Window* installer_app_launch(void);
void installer_app_tick(void);

#endif /* _KERNEL_APPS_INSTALLER_APP_H_ */
