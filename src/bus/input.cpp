/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "input.h"
#include "processor.h"
#include <string.h>

static struct {
    bool overflow;
    uint16_t divider;
    struct {
        bool current[GB_BUTTON_MAX];
        bool next[GB_BUTTON_MAX];
    } button;
    union {
        uint8_t raw;
        struct {
            uint8_t pressed : 4;
            uint8_t direction : 1;
            uint8_t button : 1;
        };
    } state;
} input = {};

bool (*gb_input_button(void)) [GB_BUTTON_MAX] { return &input.button.next; }

uint8_t gb_input_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_INPUT_STATE:
        if (!input.state.button) {
            input.state.pressed = 0xF;
            for (uint8_t index = GB_BUTTON_A; index <= GB_BUTTON_START; ++index) {
                if (input.button.current[index]) {
                    input.state.pressed &= ~(1 << (index - GB_BUTTON_A));
                }
            }
        }
        if (!input.state.direction) {
            for (uint8_t index = GB_BUTTON_RIGHT; index <= GB_BUTTON_DOWN; ++index) {
                if (input.button.current[index]) {
                    input.state.pressed &= ~(1 << (index - GB_BUTTON_RIGHT));
                }
            }
        }
        result = input.state.raw;
        break;
    default:
        break;
    }
    return result;
}

void gb_input_reset(void) {
    memset(&input, 0, sizeof(input));
    input.state.raw = 0xCF;
}

void gb_input_step(void) {
    bool overflow = input.divider & 4096;
    if (overflow && !input.overflow) {
        bool changed = false;
        for (uint8_t index = 0; index < GB_BUTTON_MAX; ++index) {
            if (input.button.current[index] != input.button.next[index]) {
                switch (index) {
                case GB_BUTTON_A ... GB_BUTTON_START:
                    if (!input.state.button && !input.button.current[index]) {
                        changed = true;
                    }
                    break;
                case GB_BUTTON_RIGHT ... GB_BUTTON_DOWN:
                    if (!input.state.direction && !input.button.current[index]) {
                        changed = true;
                    }
                    break;
                default:
                    break;
                }
                input.button.current[index] = input.button.next[index];
            }
        }
        if (changed) {
            gb_processor_interrupt(GB_INTERRUPT_INPUT);
        }
    }
    input.overflow = overflow;
    ++input.divider;
}

void gb_input_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_INPUT_STATE:
        input.state.raw = data | 0xCF;
        break;
    default:
        break;
    }
}
