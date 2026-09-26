/**
 * @file kshell.h
 * @brief Xenithra OS — Kernel Shell (GDB-Compatible Debug Shell)
 *
 * kshell is a privileged kernel thread that:
 *   1. Provides a GDB-compatible remote stub over COM2 (UART 0x2F8)
 *   2. Parses GDB RSP (Remote Serial Protocol) packets for kernel debugging
 *   3. Boots the Node.js → Vite/Electron GUI chain after kernel stabilizes
 *   4. Provides a fallback interactive debug console if Electron fails to start
 */

#ifndef _KERNEL_EXEC_KSHELL_H_
#define _KERNEL_EXEC_KSHELL_H_

#include <stdint.h>

/** Kernel shell thread entry — runs at RTOS_PRIO_REALTIME. */
void kshell_init(void);

/** Send a string to the kernel shell console (COM2). */
void kshell_print(const char *msg);

/** Query whether the GUI chain (Node.js/Electron) has booted. */
uint8_t kshell_gui_ready(void);

#endif /* _KERNEL_EXEC_KSHELL_H_ */
