/**
 * @file diskclone_app.h
 * @brief Raw Sector-by-Sector Disk Cloner & Drive Imager Application
 */

#ifndef _KERNEL_APPS_DISKCLONE_APP_H_
#define _KERNEL_APPS_DISKCLONE_APP_H_

#include "../gui/compositor.h"

Window* diskclone_app_launch(void);
void diskclone_app_tick(void);

#endif /* _KERNEL_APPS_DISKCLONE_APP_H_ */
