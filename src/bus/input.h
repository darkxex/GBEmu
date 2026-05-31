/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_INPUT_H_
#define GB_INPUT_H_

#include "bus.h"

#define GB_INPUT_STATE 0xFF00

typedef enum : uint8_t {
    GB_BUTTON_A = 0,
    GB_BUTTON_B,
    GB_BUTTON_SELECT,
    GB_BUTTON_START,
    GB_BUTTON_RIGHT,
    GB_BUTTON_LEFT,
    GB_BUTTON_UP,
    GB_BUTTON_DOWN,
    GB_BUTTON_MAX
} gb_button_e;

bool (*gb_input_button(void))[GB_BUTTON_MAX];
uint8_t gb_input_read(uint16_t address);
void gb_input_reset(void);
void gb_input_step(void);
void gb_input_write(uint16_t address, uint8_t data);

#endif /* GB_INPUT_H_ */
