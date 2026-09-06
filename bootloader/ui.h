/**
 * @file ui.h
 * @brief Modern Windows-style Graphical Boot Selector UI
 */

#ifndef _BOOTLOADER_UI_H_
#define _BOOTLOADER_UI_H_

#include "efi.h"
#include "gop.h"

typedef struct {
    const char *title;
    const char *description;
    uint32_t boot_mode;
} BootEntry;

#define MAX_BOOT_ENTRIES 4

typedef enum {
    BOOT_ACTION_START_OS = 0,
    BOOT_ACTION_FIRMWARE_SETUP,
    BOOT_ACTION_REBOOT,
    BOOT_ACTION_SHUTDOWN
} BootAction;

typedef struct {
    BootAction action;
    uint32_t selected_mode;
} BootSelectionResult;

/* UI Lifecycle & Interaction */
BootSelectionResult ui_run_boot_menu(EFI_SYSTEM_TABLE *SystemTable, uint32_t timeout_seconds);

#endif /* _BOOTLOADER_UI_H_ */
