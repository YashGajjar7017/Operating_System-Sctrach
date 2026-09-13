/**
 * @file taskmgr_app.h
 * @brief Windows 11 Fluent Task Manager Application for Xenithra OS
 */

#ifndef _KERNEL_APPS_TASKMGR_APP_H_
#define _KERNEL_APPS_TASKMGR_APP_H_

#include "../gui/compositor.h"

Window* taskmgr_app_launch(void);
void    taskmgr_tick(void);

#endif /* _KERNEL_APPS_TASKMGR_APP_H_ */
