/**
 * @file font.h
 * @brief Embedded 8x16 bitmap font table for bootloader UI rendering
 */

#ifndef _BOOTLOADER_FONT_H_
#define _BOOTLOADER_FONT_H_

#include <stdint.h>

/* Standard 8x16 bitmap font data covering ASCII 32 (' ') to 126 ('~') */
extern const uint8_t font_8x16[95][16];

#endif /* _BOOTLOADER_FONT_H_ */
