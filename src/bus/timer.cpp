/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "timer.h"
#include "audio.h"
#include "processor.h"
#include <string.h>

static const uint16_t OVERFLOW[] = { 512, 8, 32, 128 };

static struct {
    uint8_t counter;
    uint16_t divider;
    uint8_t modulo;
    union {
        uint8_t raw;
        struct {
            uint8_t mode : 2;
            uint8_t enabled : 1;
        };
    } control;
    struct {
        bool audio;
        bool processor;
    } overflow;
} timer = {};

static void gb_timer_step_audio(void) {
    bool overflow = timer.divider & 8192;
    if (overflow && !timer.overflow.audio) {
        gb_audio_interrupt();
    }
    timer.overflow.audio = overflow;
}

static void gb_timer_step_processor(void) {
    bool overflow = timer.divider & OVERFLOW[timer.control.mode];
    if (timer.control.enabled && overflow && !timer.overflow.processor && !++timer.counter) {
        timer.counter = timer.modulo;
        gb_processor_interrupt(GB_INTERRUPT_TIMER);
    }
    timer.overflow.processor = overflow;
}

uint8_t gb_timer_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_TIMER_CONTROL:
        result = timer.control.raw;
        break;
    case GB_TIMER_COUNTER:
        result = timer.counter;
        break;
    case GB_TIMER_DIVIDER:
        result = timer.divider >> 8;
        break;
    case GB_TIMER_MODULO:
        result = timer.modulo;
        break;
    default:
        break;
    }
    return result;
}

void gb_timer_reset(void) {
    memset(&timer, 0, sizeof(timer));
    timer.control.raw = 0xF8;
}

void gb_timer_step(void) {
    if (!gb_processor_stopped()) {
        gb_timer_step_processor();
        gb_timer_step_audio();
        ++timer.divider;
    }
}

void gb_timer_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_TIMER_CONTROL:
        timer.control.raw = data | 0xF8;
        break;
    case GB_TIMER_COUNTER:
        timer.counter = data;
        break;
    case GB_TIMER_DIVIDER:
        timer.divider = 0;
        timer.overflow.audio = false;
        timer.overflow.processor = false;
        break;
    case GB_TIMER_MODULO:
        timer.modulo = data;
        break;
    default:
        break;
    }
}
