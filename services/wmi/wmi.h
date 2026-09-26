/**
 * @file wmi.h
 */
#ifndef _SERVICES_WMI_H_
#define _SERVICES_WMI_H_
#include <stdint.h>
#include "../../kernel/gui/gui_ipc.h"
void wmi_handle_query_command(const GuiIpcCommand *cmd);
void wmi_service_thread(void);
#endif
