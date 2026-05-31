/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_AUDIO_H_
#define GB_AUDIO_H_

#include "bus.h"

#define GB_AUDIO_CONTROL 0xFF26
#define GB_AUDIO_MIXER 0xFF25
#define GB_AUDIO_NOISE_CONTROL 0xFF23
#define GB_AUDIO_NOISE_ENVELOPE 0xFF21
#define GB_AUDIO_NOISE_FREQUENCY 0xFF22
#define GB_AUDIO_NOISE_LENGTH 0xFF20
#define GB_AUDIO_RAM_BEGIN 0xFF30
#define GB_AUDIO_RAM_END 0xFF3F
#define GB_AUDIO_SAMPLES 560
#define GB_AUDIO_SQUARE_1_ENVELOPE 0xFF12
#define GB_AUDIO_SQUARE_1_FREQUENCY_HIGH 0xFF14
#define GB_AUDIO_SQUARE_1_FREQUENCY_LOW 0xFF13
#define GB_AUDIO_SQUARE_1_LENGTH 0xFF11
#define GB_AUDIO_SQUARE_1_SWEEP 0xFF10
#define GB_AUDIO_SQUARE_2_ENVELOPE 0xFF17
#define GB_AUDIO_SQUARE_2_FREQUENCY_HIGH 0xFF19
#define GB_AUDIO_SQUARE_2_FREQUENCY_LOW 0xFF18
#define GB_AUDIO_SQUARE_2_LENGTH 0xFF16
#define GB_AUDIO_VOLUME 0xFF24
#define GB_AUDIO_WAVE_CONTROL 0xFF1A
#define GB_AUDIO_WAVE_FREQUENCY_HIGH 0xFF1E
#define GB_AUDIO_WAVE_FREQUENCY_LOW 0xFF1D
#define GB_AUDIO_WAVE_LENGTH 0xFF1B
#define GB_AUDIO_WAVE_LEVEL 0xFF1C

#define GB_AUDIO_RAM_WIDTH GB_WIDTH(GB_AUDIO_RAM_BEGIN, GB_AUDIO_RAM_END)

typedef struct {
    uint32_t delay;
    uint16_t sample;
    uint8_t volume;
    union {
        uint8_t raw;
        struct {
            struct {
                uint8_t : 6;
                uint8_t enabled : 1;
                uint8_t trigger : 1;
            };
        };
    } control;
    union {
        uint8_t raw;
        struct {
            uint8_t period : 3;
            uint8_t direction : 1;
            uint8_t volume : 4;
        };
    } envelope;
    union {
        uint8_t raw;
        struct {
            uint8_t divider : 3;
            uint8_t width : 1;
            uint8_t shift : 4;
        };
    } frequency;
    union {
        uint8_t raw;
        struct {
            uint8_t timer : 6;
            uint8_t : 2;
        };
    } length;
    struct {
        uint8_t length;
        struct {
            uint8_t period;
        } envelope;
    } timer;
} gb_noise_t;

typedef struct {
    uint32_t delay;
    uint8_t position;
    uint8_t volume;
    union {
        uint8_t raw;
        struct {
            uint8_t period : 3;
            uint8_t direction : 1;
            uint8_t volume : 4;
        };
    } envelope;
    struct {
        uint8_t low;
        union {
            uint8_t raw;
            struct {
                uint8_t period : 3;
                uint8_t : 3;
                uint8_t enabled : 1;
                uint8_t trigger : 1;
            };
        } high;
    } frequency;
    union {
        uint8_t raw;
        struct {
            uint8_t timer : 6;
            uint8_t duty : 2;
        };
    } length;
    union {
        uint8_t raw;
        struct {
            uint8_t shift : 3;
            uint8_t direction : 1;
            uint8_t period : 3;
        };
    } sweep;
    struct {
        uint8_t length;
        struct {
            uint8_t period;
        } envelope;
        struct {
            bool enabled;
            uint16_t frequency;
            uint8_t period;
        } sweep;
    } timer;
} gb_square_1_t;

typedef struct {
    uint32_t delay;
    uint8_t position;
    uint8_t volume;
    union {
        uint8_t raw;
        struct {
            uint8_t period : 3;
            uint8_t direction : 1;
            uint8_t volume : 4;
        };
    } envelope;
    struct {
        uint8_t low;
        union {
            uint8_t raw;
            struct {
                uint8_t period : 3;
                uint8_t : 3;
                uint8_t enabled : 1;
                uint8_t trigger : 1;
            };
        } high;
    } frequency;
    union {
        uint8_t raw;
        struct {
            uint8_t timer : 6;
            uint8_t duty : 2;
        };
    } length;
    struct {
        uint8_t length;
        struct {
            uint8_t period;
        } envelope;
    } timer;
} gb_square_2_t;

typedef struct {
    uint32_t delay;
    uint8_t length;
    uint8_t position;
    union {
        uint8_t raw;
        struct {
            uint8_t : 7;
            uint8_t enabled : 1;
        };
    } control;
    struct {
        uint8_t low;
        union {
            uint8_t raw;
            struct {
                uint8_t period : 3;
                uint8_t : 3;
                uint8_t enabled : 1;
                uint8_t trigger : 1;
            };
        } high;
    } frequency;
    union {
        uint8_t raw;
        struct {
            uint8_t : 5;
            uint8_t output : 2;
        };
    } level;
    struct {
        uint16_t length;
    } timer;
} gb_wave_t;

void gb_audio_interrupt(void);
uint8_t gb_audio_read(uint16_t address);
void gb_audio_reset(void);
const int16_t (*gb_audio_sample(uint32_t *const length))[GB_AUDIO_SAMPLES];
void gb_audio_step(void);
void gb_audio_write(uint16_t address, uint8_t data);

#endif /* GB_AUDIO_H_ */
