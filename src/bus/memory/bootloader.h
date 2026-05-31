/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_BOOTLOADER_H_
#define GB_BOOTLOADER_H_

#include "memory.h"

#define GB_BOOTLOADER_DISABLE 0xFF50
#define GB_BOOTLOADER_ROM_BEGIN 0x0000
#define GB_BOOTLOADER_ROM_END 0x00FF

#define GB_BOOTLOADER_ROM_WIDTH GB_WIDTH(GB_BOOTLOADER_ROM_BEGIN, GB_BOOTLOADER_ROM_END)

bool gb_bootloader_enabled(void);
uint8_t gb_bootloader_read(uint16_t address);
void gb_bootloader_reset(void);
void gb_bootloader_write(uint16_t address, uint8_t data);

#endif /* GB_BOOTLOADER_H_ */
