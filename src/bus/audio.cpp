/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "audio.h"
#include <string.h>

static const uint32_t DIVIDER[] = { 8, 16, 32, 48, 64, 80, 96, 112 };

static const bool MUTE[] = { true, false, false, false };

static const int8_t PULSE[][8] = {
    { -1, -1, -1, -1, -1, -1, -1, 1 }, { -1, -1, -1, -1, -1, -1, 1, 1 }, { -1, -1, -1, -1, 1, 1, 1, 1 }, { 1, 1, 1, 1, 1, 1, -1, -1 }
};

static const uint8_t SHIFT[] = { 0, 0, 1, 2 };

static struct {
    uint32_t cycle;
    uint16_t delay;
    uint32_t index;
    uint8_t ram[GB_AUDIO_RAM_WIDTH];
    int16_t sample[GB_AUDIO_SAMPLES];
    gb_noise_t noise;
    gb_square_1_t square_1;
    gb_square_2_t square_2;
    gb_wave_t wave;
    union {
        uint8_t raw;
        struct {
            uint8_t square_1_enabled : 1;
            uint8_t square_2_enabled : 1;
            uint8_t wave_enabled : 1;
            uint8_t noise_enabled : 1;
            uint8_t : 3;
            uint8_t enabled : 1;
        };
    } control;
    union {
        uint8_t raw;
        struct {
            uint8_t square_1_right : 1;
            uint8_t square_2_right : 1;
            uint8_t wave_right : 1;
            uint8_t noise_right : 1;
            uint8_t square_1_left : 1;
            uint8_t square_2_left : 1;
            uint8_t wave_left : 1;
            uint8_t noise_left : 1;
        };
    } mixer;
    union {
        uint8_t raw;
        struct {
            uint8_t right : 3;
            uint8_t : 1;
            uint8_t left : 3;
            uint8_t : 1;
        };
    } volume;
} audio = {};

static void gb_audio_noise_envelope(void) {
    if (audio.noise.timer.envelope.period && !--audio.noise.timer.envelope.period) {
        audio.noise.timer.envelope.period = audio.noise.envelope.period;
        if (audio.noise.envelope.direction) {
            if (audio.noise.volume < 15) {
                ++audio.noise.volume;
            }
        } else if (audio.noise.volume) {
            --audio.noise.volume;
        }
    }
}

static void gb_audio_noise_length(void) {
    if (audio.noise.control.enabled && audio.noise.timer.length) {
        if (!--audio.noise.timer.length) {
            audio.control.noise_enabled = false;
        }
    }
}

static void gb_audio_noise_sample(int32_t *const left, int32_t *const right) {
    if (audio.control.noise_enabled) {
        int32_t sample = ((audio.noise.sample & 1) ? 1 : -1) * audio.noise.volume;
        if (audio.mixer.noise_left) {
            *left += sample;
        }
        if (audio.mixer.noise_right) {
            *right += sample;
        }
    }
}

static void gb_audio_noise_step(void) {
    if (audio.control.noise_enabled) {
        if (!audio.noise.delay) {
            uint16_t sample = 0;
            audio.noise.delay = DIVIDER[audio.noise.frequency.divider] << audio.noise.frequency.shift;
            sample = !((audio.noise.sample & 1) ^ ((audio.noise.sample & 2) >> 1));
            audio.noise.sample = (audio.noise.sample >> 1) | (sample << 14);
            if (audio.noise.frequency.width) {
                audio.noise.sample &= ~(1 << 6);
                audio.noise.sample |= (sample << 6);
            }
        }
        --audio.noise.delay;
    }
}

static void gb_audio_noise_trigger(void) {
    if (audio.noise.control.trigger) {
        audio.noise.sample = 0;
        audio.noise.timer.envelope.period = audio.noise.envelope.period;
        audio.noise.timer.length = 64 - audio.noise.length.timer;
        audio.noise.volume = audio.noise.envelope.volume;
        audio.control.noise_enabled = true;
    }
}

static void gb_audio_square_1_envelope(void) {
    if (audio.square_1.timer.envelope.period && !--audio.square_1.timer.envelope.period) {
        audio.square_1.timer.envelope.period = audio.square_1.envelope.period;
        if (audio.square_1.envelope.direction) {
            if (audio.square_1.volume < 15) {
                ++audio.square_1.volume;
            }
        } else if (audio.square_1.volume) {
            --audio.square_1.volume;
        }
    }
}

static void gb_audio_square_1_length(void) {
    if (audio.square_1.frequency.high.enabled && audio.square_1.timer.length) {
        if (!--audio.square_1.timer.length) {
            audio.control.square_1_enabled = false;
        }
    }
}

static void gb_audio_square_1_sample(int32_t *const left, int32_t *const right) {
    if (audio.control.square_1_enabled) {
        int32_t sample = PULSE[audio.square_1.length.duty][audio.square_1.position] * audio.square_1.volume;
        if (audio.mixer.square_1_left) {
            *left += sample;
        }
        if (audio.mixer.square_1_right) {
            *right += sample;
        }
    }
}

static void gb_audio_square_1_step(void) {
    if (audio.control.square_1_enabled) {
        if (!audio.square_1.delay) {
            audio.square_1.delay = (2048 - ((audio.square_1.frequency.high.period << 8) | audio.square_1.frequency.low)) * 4;
            audio.square_1.position = (audio.square_1.position + 1) & 7;
        }
        --audio.square_1.delay;
    }
}

static void gb_audio_square_1_sweep(void) {
    if (audio.square_1.timer.sweep.period && !--audio.square_1.timer.sweep.period) {
        audio.square_1.timer.sweep.period = !audio.square_1.sweep.period ? 8 : audio.square_1.sweep.period;
        if (audio.square_1.timer.sweep.enabled && audio.square_1.sweep.period) {
            uint16_t frequency = audio.square_1.timer.sweep.frequency >> audio.square_1.sweep.shift;
            if (audio.square_1.sweep.direction) {
                frequency = audio.square_1.timer.sweep.frequency - frequency;
            } else {
                frequency = audio.square_1.timer.sweep.frequency + frequency;
            }
            if (frequency > 2047) {
                audio.square_1.timer.sweep.enabled = false;
                audio.control.square_1_enabled = false;
            } else if (audio.square_1.sweep.shift) {
                audio.square_1.frequency.high.period = frequency >> 8;
                audio.square_1.frequency.low = frequency;
                audio.square_1.timer.sweep.frequency = frequency;
            }
        }
    }
}

static void gb_audio_square_1_trigger(void) {
    if (audio.square_1.frequency.high.trigger) {
        audio.square_1.timer.envelope.period = audio.square_1.envelope.period;
        audio.square_1.timer.length = 64 - audio.square_1.length.timer;
        audio.square_1.timer.sweep.enabled = (audio.square_1.sweep.period || audio.square_1.sweep.shift);
        audio.square_1.timer.sweep.frequency = (audio.square_1.frequency.high.period << 8) | audio.square_1.frequency.low;
        audio.square_1.timer.sweep.period = !audio.square_1.sweep.period ? 8 : audio.square_1.sweep.period;
        audio.square_1.volume = audio.square_1.envelope.volume;
        audio.control.square_1_enabled = true;
    }
}

static void gb_audio_square_2_envelope(void) {
    if (audio.square_2.timer.envelope.period && !--audio.square_2.timer.envelope.period) {
        audio.square_2.timer.envelope.period = audio.square_2.envelope.period;
        if (audio.square_2.envelope.direction) {
            if (audio.square_2.volume < 15) {
                ++audio.square_2.volume;
            }
        } else if (audio.square_2.volume) {
            --audio.square_2.volume;
        }
    }
}

static void gb_audio_square_2_length(void) {
    if (audio.square_2.frequency.high.enabled && audio.square_2.timer.length) {
        if (!--audio.square_2.timer.length) {
            audio.control.square_2_enabled = false;
        }
    }
}

static void gb_audio_square_2_sample(int32_t *const left, int32_t *const right) {
    if (audio.control.square_2_enabled) {
        int32_t sample = PULSE[audio.square_2.length.duty][audio.square_2.position] * audio.square_2.volume;
        if (audio.mixer.square_2_left) {
            *left += sample;
        }
        if (audio.mixer.square_2_right) {
            *right += sample;
        }
    }
}

static void gb_audio_square_2_step(void) {
    if (audio.control.square_2_enabled) {
        if (!audio.square_2.delay) {
            audio.square_2.delay = (2048 - ((audio.square_2.frequency.high.period << 8) | audio.square_2.frequency.low)) * 4;
            audio.square_2.position = (audio.square_2.position + 1) & 7;
        }
        --audio.square_2.delay;
    }
}

static void gb_audio_square_2_trigger(void) {
    if (audio.square_2.frequency.high.trigger) {
        audio.square_2.timer.envelope.period = audio.square_2.envelope.period;
        audio.square_2.timer.length = 64 - audio.square_2.length.timer;
        audio.square_2.volume = audio.square_2.envelope.volume;
        audio.control.square_2_enabled = true;
    }
}

static void gb_audio_wave_length(void) {
    if (audio.wave.frequency.high.enabled && audio.wave.timer.length) {
        if (!--audio.wave.timer.length) {
            audio.control.wave_enabled = false;
        }
    }
}

static void gb_audio_wave_sample(int32_t *const left, int32_t *const right) {
    if (audio.control.wave_enabled && !MUTE[audio.wave.level.output]) {
        uint8_t data = audio.ram[audio.wave.position / 2];
        if (audio.wave.position & 1) {
            data &= 15;
        } else {
            data >>= 4;
        }
        int32_t sample = ((int32_t)data * 2 - 15) >> SHIFT[audio.wave.level.output];
        if (audio.mixer.wave_left) {
            *left += sample;
        }
        if (audio.mixer.wave_right) {
            *right += sample;
        }
    }
}

static void gb_audio_wave_step(void) {
    if (audio.control.wave_enabled) {
        if (!audio.wave.delay) {
            audio.wave.delay = (2048 - ((audio.wave.frequency.high.period << 8) | audio.wave.frequency.low)) * 2;
            audio.wave.position = (audio.wave.position + 1) & 31;
        }
        --audio.wave.delay;
    }
}

static void gb_audio_wave_trigger(void) {
    if (audio.wave.frequency.high.trigger) {
        audio.wave.position = 0;
        audio.wave.timer.length = 256 - audio.wave.length;
        audio.control.wave_enabled = true;
    }
}

void gb_audio_interrupt(void) {
    gb_audio_noise_length();
    gb_audio_square_1_length();
    gb_audio_square_2_length();
    gb_audio_wave_length();
    if (!(audio.cycle & 1)) {
        gb_audio_square_1_sweep();
    }
    if (!(audio.cycle & 3)) {
        gb_audio_noise_envelope();
        gb_audio_square_1_envelope();
        gb_audio_square_2_envelope();
    }
    if (++audio.cycle >= 4) {
        audio.cycle = 0;
    }
}

uint8_t gb_audio_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_AUDIO_CONTROL:
        result = audio.control.raw;
        break;
    case GB_AUDIO_MIXER:
        result = audio.mixer.raw;
        break;
    case GB_AUDIO_NOISE_CONTROL:
        result = audio.noise.control.raw;
        break;
    case GB_AUDIO_NOISE_ENVELOPE:
        result = audio.noise.envelope.raw;
        break;
    case GB_AUDIO_NOISE_FREQUENCY:
        result = audio.noise.frequency.raw;
        break;
    case GB_AUDIO_NOISE_LENGTH:
        result = audio.noise.length.raw;
        break;
    case GB_AUDIO_RAM_BEGIN ... GB_AUDIO_RAM_END:
        result = audio.ram[address - GB_AUDIO_RAM_BEGIN];
        break;
    case GB_AUDIO_SQUARE_1_ENVELOPE:
        result = audio.square_1.envelope.raw;
        break;
    case GB_AUDIO_SQUARE_1_FREQUENCY_HIGH:
        result = audio.square_1.frequency.high.raw;
        break;
    case GB_AUDIO_SQUARE_1_LENGTH:
        result = audio.square_1.length.raw;
        break;
    case GB_AUDIO_SQUARE_1_SWEEP:
        result = audio.square_1.sweep.raw;
        break;
    case GB_AUDIO_SQUARE_2_ENVELOPE:
        result = audio.square_2.envelope.raw;
        break;
    case GB_AUDIO_SQUARE_2_FREQUENCY_HIGH:
        result = audio.square_2.frequency.high.raw;
        break;
    case GB_AUDIO_SQUARE_2_LENGTH:
        result = audio.square_2.length.raw;
        break;
    case GB_AUDIO_VOLUME:
        result = audio.volume.raw;
        break;
    case GB_AUDIO_WAVE_CONTROL:
        result = audio.wave.control.raw;
        break;
    case GB_AUDIO_WAVE_FREQUENCY_HIGH:
        result = audio.wave.frequency.high.raw;
        break;
    case GB_AUDIO_WAVE_LENGTH:
        result = audio.wave.length;
        break;
    case GB_AUDIO_WAVE_LEVEL:
        result = audio.wave.level.raw;
        break;
    default:
        break;
    }
    return result;
}

void gb_audio_reset(void) {
    memset(&audio, 0, sizeof(audio));
    audio.control.raw = 0x70;
    audio.noise.control.raw = 0x3F;
    audio.noise.length.raw = 0xC0;
    audio.square_1.frequency.high.raw = 0x38;
    audio.square_1.sweep.raw = 0x80;
    audio.square_2.frequency.high.raw = 0x38;
    audio.volume.raw = 0x88;
    audio.wave.control.raw = 0x7F;
    audio.wave.frequency.high.raw = 0x38;
    audio.wave.level.raw = 0x9F;
}

const int16_t (*gb_audio_sample(uint32_t *const length)) [GB_AUDIO_SAMPLES] {
    *length = audio.index;
    audio.index = 0;
    return &audio.sample;
}

void gb_audio_step(void) {
    if (audio.control.enabled) {
        gb_audio_noise_step();
        gb_audio_square_1_step();
        gb_audio_square_2_step();
        gb_audio_wave_step();
    }
    if (!audio.delay) {
        int32_t mixed = 0;
        if (audio.control.enabled) {
            int32_t left = 0, right = 0;
            gb_audio_noise_sample(&left, &right);
            gb_audio_square_1_sample(&left, &right);
            gb_audio_square_2_sample(&left, &right);
            gb_audio_wave_sample(&left, &right);
            left *= (audio.volume.left + 1);
            right *= (audio.volume.right + 1);
            mixed = (left + right) * 32;
            if (mixed > INT16_MAX) {
                mixed = INT16_MAX;
            } else if (mixed < INT16_MIN) {
                mixed = INT16_MIN;
            }
        }
        audio.sample[audio.index++] = (int16_t)mixed;
        if (audio.index >= GB_AUDIO_SAMPLES) {
            audio.index = 0;
        }
        audio.delay = 128;
    }
    --audio.delay;
}

void gb_audio_write(uint16_t address, uint8_t data) {
    bool enabled = false;
    switch (address) {
    case GB_AUDIO_CONTROL:
        enabled = audio.control.enabled;
        audio.control.raw = (audio.control.raw & 0x7F) | (data & 0x80);
        if (enabled && !audio.control.enabled) {
            memset(audio.sample, 0, sizeof(audio.sample));
            memset(&audio.noise, 0, sizeof(audio.noise));
            memset(&audio.square_1, 0, sizeof(audio.square_1));
            memset(&audio.square_2, 0, sizeof(audio.square_2));
            memset(&audio.wave, 0, sizeof(audio.wave));
            audio.control.raw = 0x70;
        }
        break;
    case GB_AUDIO_MIXER:
        if (audio.control.enabled) {
            audio.mixer.raw = data;
        }
        break;
    case GB_AUDIO_NOISE_CONTROL:
        if (audio.control.enabled) {
            audio.noise.control.raw = data | 0x3F;
            gb_audio_noise_trigger();
        }
        break;
    case GB_AUDIO_NOISE_ENVELOPE:
        if (audio.control.enabled) {
            audio.noise.envelope.raw = data;
        }
        break;
    case GB_AUDIO_NOISE_FREQUENCY:
        if (audio.control.enabled) {
            audio.noise.frequency.raw = data;
        }
        break;
    case GB_AUDIO_NOISE_LENGTH:
        if (audio.control.enabled) {
            audio.noise.length.raw = data | 0xC0;
        }
        break;
    case GB_AUDIO_RAM_BEGIN ... GB_AUDIO_RAM_END:
        if (audio.control.enabled) {
            audio.ram[address - GB_AUDIO_RAM_BEGIN] = data;
        }
        break;
    case GB_AUDIO_SQUARE_1_ENVELOPE:
        if (audio.control.enabled) {
            audio.square_1.envelope.raw = data;
        }
        break;
    case GB_AUDIO_SQUARE_1_FREQUENCY_HIGH:
        if (audio.control.enabled) {
            audio.square_1.frequency.high.raw = data | 0x38;
            gb_audio_square_1_trigger();
        }
        break;
    case GB_AUDIO_SQUARE_1_FREQUENCY_LOW:
        if (audio.control.enabled) {
            audio.square_1.frequency.low = data;
        }
        break;
    case GB_AUDIO_SQUARE_1_LENGTH:
        if (audio.control.enabled) {
            audio.square_1.length.raw = data;
        }
        break;
    case GB_AUDIO_SQUARE_1_SWEEP:
        if (audio.control.enabled) {
            audio.square_1.sweep.raw = data | 0x80;
        }
        break;
    case GB_AUDIO_SQUARE_2_ENVELOPE:
        if (audio.control.enabled) {
            audio.square_2.envelope.raw = data;
        }
        break;
    case GB_AUDIO_SQUARE_2_FREQUENCY_HIGH:
        if (audio.control.enabled) {
            audio.square_2.frequency.high.raw = data | 0x38;
            gb_audio_square_2_trigger();
        }
        break;
    case GB_AUDIO_SQUARE_2_FREQUENCY_LOW:
        if (audio.control.enabled) {
            audio.square_2.frequency.low = data;
        }
        break;
    case GB_AUDIO_SQUARE_2_LENGTH:
        if (audio.control.enabled) {
            audio.square_2.length.raw = data;
        }
        break;
    case GB_AUDIO_VOLUME:
        if (audio.control.enabled) {
            audio.volume.raw = data;
        }
        break;
    case GB_AUDIO_WAVE_CONTROL:
        if (audio.control.enabled) {
            audio.wave.control.raw = data | 0x7F;
            if (!audio.wave.control.enabled) {
                audio.control.wave_enabled = false;
            }
        }
        break;
    case GB_AUDIO_WAVE_FREQUENCY_HIGH:
        if (audio.control.enabled) {
            audio.wave.frequency.high.raw = data | 0x38;
            gb_audio_wave_trigger();
        }
        break;
    case GB_AUDIO_WAVE_FREQUENCY_LOW:
        if (audio.control.enabled) {
            audio.wave.frequency.low = data;
        }
        break;
    case GB_AUDIO_WAVE_LENGTH:
        if (audio.control.enabled) {
            audio.wave.length = data;
        }
        break;
    case GB_AUDIO_WAVE_LEVEL:
        if (audio.control.enabled) {
            audio.wave.level.raw = data | 0x9F;
        }
        break;
    default:
        break;
    }
}
