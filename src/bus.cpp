/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "audio.h"
#include "cartridge.h"
#include "input.h"
#include "processor.h"
#include "serial.h"
#include "timer.h"
#include "video.h"

uint8_t gb_bus_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_AUDIO_CONTROL:
    case GB_AUDIO_MIXER:
    case GB_AUDIO_NOISE_CONTROL:
    case GB_AUDIO_NOISE_ENVELOPE:
    case GB_AUDIO_NOISE_FREQUENCY:
    case GB_AUDIO_NOISE_LENGTH:
    case GB_AUDIO_RAM_BEGIN ... GB_AUDIO_RAM_END:
    case GB_AUDIO_SQUARE_1_ENVELOPE:
    case GB_AUDIO_SQUARE_1_FREQUENCY_HIGH:
    case GB_AUDIO_SQUARE_1_FREQUENCY_LOW:
    case GB_AUDIO_SQUARE_1_LENGTH:
    case GB_AUDIO_SQUARE_1_SWEEP:
    case GB_AUDIO_SQUARE_2_ENVELOPE:
    case GB_AUDIO_SQUARE_2_FREQUENCY_HIGH:
    case GB_AUDIO_SQUARE_2_FREQUENCY_LOW:
    case GB_AUDIO_SQUARE_2_LENGTH:
    case GB_AUDIO_VOLUME:
    case GB_AUDIO_WAVE_CONTROL:
    case GB_AUDIO_WAVE_FREQUENCY_HIGH:
    case GB_AUDIO_WAVE_FREQUENCY_LOW:
    case GB_AUDIO_WAVE_LENGTH:
    case GB_AUDIO_WAVE_LEVEL:
        result = gb_audio_read(address);
        break;
    case GB_INPUT_STATE:
        result = gb_input_read(address);
        break;
    case GB_PROCESSOR_INTERRUPT_ENABLE:
    case GB_PROCESSOR_INTERRUPT_FLAG:
        result = gb_processor_read(address);
        break;
    case GB_SERIAL_CONTROL:
    case GB_SERIAL_DATA:
        result = gb_serial_read(address);
        break;
    case GB_TIMER_CONTROL:
    case GB_TIMER_COUNTER:
    case GB_TIMER_DIVIDER:
    case GB_TIMER_MODULO:
        result = gb_timer_read(address);
        break;
    case GB_VIDEO_CONTROL:
    case GB_VIDEO_LINE_Y:
    case GB_VIDEO_LINE_Y_COINCIDENCE:
    case GB_VIDEO_PALETTE_BACKGROUND:
    case GB_VIDEO_PALETTE_OBJECT_0:
    case GB_VIDEO_PALETTE_OBJECT_1:
    case GB_VIDEO_RAM_BEGIN ... GB_VIDEO_RAM_END:
    case GB_VIDEO_RAM_OBJECT_BEGIN ... GB_VIDEO_RAM_OBJECT_END:
    case GB_VIDEO_SCROLL_X:
    case GB_VIDEO_SCROLL_Y:
    case GB_VIDEO_STATUS:
    case GB_VIDEO_TRANSFER:
    case GB_VIDEO_WINDOW_X:
    case GB_VIDEO_WINDOW_Y:
        result = gb_video_read(address);
        break;
    default:
        result = gb_memory_read(address);
        break;
    }
    return result;
}

gb_error_e gb_bus_reset(const gb_bank_t *const rom, gb_bank_t *const ram) {
    gb_error_e result = GB_SUCCESS;
    if ((result = gb_memory_reset(rom, ram)) == GB_SUCCESS) {
        gb_audio_reset();
        gb_input_reset();
        gb_processor_reset();
        gb_serial_reset();
        gb_timer_reset();
        gb_video_reset();
    }
    return result;
}

gb_error_e gb_bus_run(void) {
    gb_error_e result = GB_SUCCESS;
    while ((result = gb_processor_step()) == GB_SUCCESS) {
        gb_audio_step();
        gb_cartridge_step();
        gb_input_step();
        gb_serial_step();
        gb_timer_step();
        if ((result = gb_video_step()) != GB_SUCCESS) {
            if (result == GB_COMPLETE) {
                result = GB_SUCCESS;
            }
            break;
        }
    }
    return result;
}

void gb_bus_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_AUDIO_CONTROL:
    case GB_AUDIO_MIXER:
    case GB_AUDIO_NOISE_CONTROL:
    case GB_AUDIO_NOISE_ENVELOPE:
    case GB_AUDIO_NOISE_FREQUENCY:
    case GB_AUDIO_NOISE_LENGTH:
    case GB_AUDIO_RAM_BEGIN ... GB_AUDIO_RAM_END:
    case GB_AUDIO_SQUARE_1_ENVELOPE:
    case GB_AUDIO_SQUARE_1_FREQUENCY_HIGH:
    case GB_AUDIO_SQUARE_1_FREQUENCY_LOW:
    case GB_AUDIO_SQUARE_1_LENGTH:
    case GB_AUDIO_SQUARE_1_SWEEP:
    case GB_AUDIO_SQUARE_2_ENVELOPE:
    case GB_AUDIO_SQUARE_2_FREQUENCY_HIGH:
    case GB_AUDIO_SQUARE_2_FREQUENCY_LOW:
    case GB_AUDIO_SQUARE_2_LENGTH:
    case GB_AUDIO_VOLUME:
    case GB_AUDIO_WAVE_CONTROL:
    case GB_AUDIO_WAVE_FREQUENCY_HIGH:
    case GB_AUDIO_WAVE_FREQUENCY_LOW:
    case GB_AUDIO_WAVE_LENGTH:
    case GB_AUDIO_WAVE_LEVEL:
        gb_audio_write(address, data);
        break;
    case GB_INPUT_STATE:
        gb_input_write(address, data);
        break;
    case GB_PROCESSOR_INTERRUPT_ENABLE:
    case GB_PROCESSOR_INTERRUPT_FLAG:
        gb_processor_write(address, data);
        break;
    case GB_SERIAL_CONTROL:
    case GB_SERIAL_DATA:
        gb_serial_write(address, data);
        break;
    case GB_TIMER_CONTROL:
    case GB_TIMER_COUNTER:
    case GB_TIMER_DIVIDER:
    case GB_TIMER_MODULO:
        gb_timer_write(address, data);
        break;
    case GB_VIDEO_CONTROL:
    case GB_VIDEO_LINE_Y:
    case GB_VIDEO_LINE_Y_COINCIDENCE:
    case GB_VIDEO_PALETTE_BACKGROUND:
    case GB_VIDEO_PALETTE_OBJECT_0:
    case GB_VIDEO_PALETTE_OBJECT_1:
    case GB_VIDEO_RAM_BEGIN ... GB_VIDEO_RAM_END:
    case GB_VIDEO_RAM_OBJECT_BEGIN ... GB_VIDEO_RAM_OBJECT_END:
    case GB_VIDEO_SCROLL_X:
    case GB_VIDEO_SCROLL_Y:
    case GB_VIDEO_STATUS:
    case GB_VIDEO_TRANSFER:
    case GB_VIDEO_WINDOW_X:
    case GB_VIDEO_WINDOW_Y:
        gb_video_write(address, data);
        break;
    default:
        gb_memory_write(address, data);
        break;
    }
}
