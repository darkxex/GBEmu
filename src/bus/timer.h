/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_TIMER_H_
#define GB_TIMER_H_

#include "bus.h"

#define GB_TIMER_CONTROL 0xFF07
#define GB_TIMER_COUNTER 0xFF05
#define GB_TIMER_DIVIDER 0xFF04
#define GB_TIMER_MODULO 0xFF06

uint8_t gb_timer_read(uint16_t address);
void gb_timer_reset(void);
void gb_timer_step(void);
void gb_timer_write(uint16_t address, uint8_t data);

#endif /* GB_TIMER_H_ */
