/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_MBC3_H_
#define GB_MBC3_H_

#include "cartridge.h"

#define GB_MBC3_CLOCK_BANK_BEGIN 0x08
#define GB_MBC3_CLOCK_BANK_END 0x0C
#define GB_MBC3_CLOCK_LATCH_BEGIN 0x6000
#define GB_MBC3_CLOCK_LATCH_END 0x7FFF
#define GB_MBC3_RAM_BANK_BEGIN 0x4000
#define GB_MBC3_RAM_BANK_END 0x5FFF
#define GB_MBC3_RAM_ENABLE_BEGIN 0x0000
#define GB_MBC3_RAM_ENABLE_END 0x1FFF
#define GB_MBC3_ROM_BANK_BEGIN 0x2000
#define GB_MBC3_ROM_BANK_END 0x3FFF

uint8_t gb_mbc3_read(uint16_t address);
void gb_mbc3_reset(void);
void gb_mbc3_step(void);
void gb_mbc3_write(uint16_t address, uint8_t data);

#endif /* GB_MBC3_H_ */
