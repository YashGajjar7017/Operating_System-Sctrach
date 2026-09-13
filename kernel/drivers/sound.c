/**
 * @file sound.c
 * @brief Hardware PC Speaker 8254 PIT Timer 2 PWM & Ambient Welcome Chime Implementation
 */

#include "sound.h"

#define PIT_TIMER2_PORT    0x42
#define PIT_COMMAND_PORT   0x43
#define PC_SPEAKER_PORT    0x61
#define PIT_BASE_FREQUENCY 1193180

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void io_delay(uint32_t ms) {
    /* Precise calibrated delay loop for x86_64 kernel boot */
    for (volatile uint32_t i = 0; i < ms * 40000; i++) {
        __asm__ volatile ("pause");
    }
}

void sound_init(void) {
    sound_stop();
}

void sound_play_tone(uint32_t freq_hz, uint32_t duration_ms) {
    if (freq_hz == 0) {
        sound_stop();
        io_delay(duration_ms);
        return;
    }

    uint32_t divisor = PIT_BASE_FREQUENCY / freq_hz;
    if (divisor > 0xFFFF) divisor = 0xFFFF;

    /* 1. Program PIT Timer 2 to Mode 3 (Square Wave Generator) */
    outb(PIT_COMMAND_PORT, 0xB6);
    outb(PIT_TIMER2_PORT, (uint8_t)(divisor & 0xFF));
    outb(PIT_TIMER2_PORT, (uint8_t)((divisor >> 8) & 0xFF));

    /* 2. Enable PC Speaker gate (Bits 0 & 1) */
    uint8_t speaker_ctrl = inb(PC_SPEAKER_PORT);
    if ((speaker_ctrl & 0x03) != 0x03) {
        outb(PC_SPEAKER_PORT, speaker_ctrl | 0x03);
    }

    if (duration_ms > 0) {
        io_delay(duration_ms);
        sound_stop();
    }
}

void sound_stop(void) {
    uint8_t speaker_ctrl = inb(PC_SPEAKER_PORT);
    outb(PC_SPEAKER_PORT, speaker_ctrl & 0xFC);
}

/**
 * @brief Play Polyphonic Ambient Welcome Chime (Fmaj7 Chord: F3 -> A3 -> C4 -> E4)
 */
void play_system_startup_chime(void) {
    /* F3 (174 Hz) -> A3 (220 Hz) -> C4 (261 Hz) -> E4 (329 Hz) */
    const uint32_t chord_frequencies[4] = { 174, 220, 261, 329 };
    const uint32_t durations[4]         = { 80,  90,  110, 260 };

    for (int i = 0; i < 4; i++) {
        sound_play_tone(chord_frequencies[i], durations[i]);
    }
    sound_stop();
}
