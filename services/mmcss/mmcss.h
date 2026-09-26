/**
 * @file mmcss.h
 * @brief MMCSS — Multimedia Class Scheduler Service API
 */

#ifndef _SERVICES_MMCSS_H_
#define _SERVICES_MMCSS_H_

#include <stdint.h>
#include "../../kernel/sched/sched.h"

typedef enum {
    MMCSS_CLASS_AUDIO    = 0,
    MMCSS_CLASS_VIDEO    = 1,
    MMCSS_CLASS_CAPTURE  = 2,
    MMCSS_CLASS_PLAYBACK = 3,
} MmcssClass;

/** Register a thread as a multimedia client (gives REALTIME priority + EDF). */
void mmcss_register(const char *name, Thread *thread,
                    MmcssClass cls, uint32_t period_us);

/** Unregister a multimedia client thread. */
void mmcss_unregister(Thread *thread);

/** MMCSS kernel service thread entry — run at RTOS_PRIO_REALTIME. */
void mmcss_service_thread(void);

#endif /* _SERVICES_MMCSS_H_ */
