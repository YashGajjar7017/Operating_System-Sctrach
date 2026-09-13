/**
 * @file vlc_app.h
 * @brief Windows 11 VLC Media Player Application
 */

#ifndef _KERNEL_APPS_VLC_APP_H_
#define _KERNEL_APPS_VLC_APP_H_

#include "../gui/compositor.h"

Window* vlc_app_launch(void);
void vlc_app_tick(void);

#endif /* _KERNEL_APPS_VLC_APP_H_ */
