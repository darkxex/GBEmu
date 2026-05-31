/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_MEMORY_H_
#define GB_MEMORY_H_

#include "bus.h"

#define GB_MEMORY_RAM_HIGH_BEGIN 0xFF80
#define GB_MEMORY_RAM_HIGH_END 0xFFFE
#define GB_MEMORY_RAM_ECHO_BEGIN 0xE000
#define GB_MEMORY_RAM_ECHO_END 0xFDFF
#define GB_MEMORY_RAM_UNUSED_BEGIN 0xFEA0
#define GB_MEMORY_RAM_UNUSED_END 0xFEFF
#define GB_MEMORY_RAM_WORK_BEGIN 0xC000
#define GB_MEMORY_RAM_WORK_END 0xDFFF

#define GB_MEMORY_RAM_HIGH_WIDTH GB_WIDTH(GB_MEMORY_RAM_HIGH_BEGIN, GB_MEMORY_RAM_HIGH_END)
#define GB_MEMORY_RAM_WORK_WIDTH GB_WIDTH(GB_MEMORY_RAM_WORK_BEGIN, GB_MEMORY_RAM_WORK_END)

uint8_t gb_memory_read(uint16_t address);
gb_error_e gb_memory_reset(const gb_bank_t *const rom, gb_bank_t *const ram);
void gb_memory_write(uint16_t address, uint8_t data);

#endif /* GB_MEMORY_H_ */
