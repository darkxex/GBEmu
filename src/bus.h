/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_BUS_H_
#define GB_BUS_H_

#include "common.h"

typedef struct {
    uint32_t length;
    uint8_t *data;
} gb_bank_t;

uint8_t gb_bus_read(uint16_t address);
gb_error_e gb_bus_reset(const gb_bank_t *const rom, gb_bank_t *const ram);
gb_error_e gb_bus_run(void);
void gb_bus_write(uint16_t address, uint8_t data);

#endif /* GB_BUS_H_ */
