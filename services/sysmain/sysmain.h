/**
 * @file sysmain.h
 * @brief SysMain (Superfetch) — Memory Prefetcher Service API
 */

#ifndef _SERVICES_SYSMAIN_H_
#define _SERVICES_SYSMAIN_H_

#include <stdint.h>

/** Called by SMSS when an application launches — records access pattern. */
void sysmain_record_launch(const char *app_tag);

/** Kernel thread entry — register with sched_create_named_kthread() at RTOS_PRIO_LOW. */
void sysmain_service_thread(void);

#endif /* _SERVICES_SYSMAIN_H_ */
