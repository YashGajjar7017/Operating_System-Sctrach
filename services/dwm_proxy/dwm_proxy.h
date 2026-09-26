/**
 * @file dwm_proxy.h
 */
#ifndef _SERVICES_DWM_PROXY_H_
#define _SERVICES_DWM_PROXY_H_
#include <stdint.h>
#include "../../kernel/gui/gui_ipc.h"
uint32_t dwm_open_window(const char *title, const char *app_tag, int x, int y, int w, int h);
void     dwm_close_window(uint32_t win_id);
void     dwm_minimize_window(uint32_t win_id);
void     dwm_maximize_window(uint32_t win_id);
void     dwm_focus_window(uint32_t win_id);
void     dwm_handle_launch_command(const GuiIpcCommand *cmd);
void     dwm_handle_close_command(const GuiIpcCommand *cmd);
void     dwm_proxy_service_thread(void);
#endif
