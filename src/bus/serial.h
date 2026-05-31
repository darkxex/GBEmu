/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_SERIAL_H_
#define GB_SERIAL_H_

#include "bus.h"

#define GB_SERIAL_CONTROL 0xFF02
#define GB_SERIAL_DATA 0xFF01

uint8_t gb_serial_read(uint16_t address);
void gb_serial_reset(void);
void gb_serial_step(void);
void gb_serial_write(uint16_t address, uint8_t data);

#endif /* GB_SERIAL_H_ */
