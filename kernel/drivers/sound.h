/**
 * @file sound.h
 * @brief Bare-Metal Hardware PC Speaker PWM & Audio Chime Driver for Xenithra OS
 */

#ifndef _KERNEL_DRIVERS_SOUND_H_
#define _KERNEL_DRIVERS_SOUND_H_

#include <stdint.h>
#include <stddef.h>

void sound_init(void);
void sound_play_tone(uint32_t freq_hz, uint32_t duration_ms);
void sound_stop(void);
void play_system_startup_chime(void);

#endif /* _KERNEL_DRIVERS_SOUND_H_ */
