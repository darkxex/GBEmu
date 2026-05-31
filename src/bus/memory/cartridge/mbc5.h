/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_MBC5_H_
#define GB_MBC5_H_

#include "cartridge.h"

#define GB_MBC5_RAM_BANK_BEGIN 0x4000
#define GB_MBC5_RAM_BANK_END 0x5FFF
#define GB_MBC5_RAM_ENABLE_BEGIN 0x0000
#define GB_MBC5_RAM_ENABLE_END 0x1FFF
#define GB_MBC5_ROM_BANK_HIGH_BEGIN 0x3000
#define GB_MBC5_ROM_BANK_HIGH_END 0x3FFF
#define GB_MBC5_ROM_BANK_LOW_BEGIN 0x2000
#define GB_MBC5_ROM_BANK_LOW_END 0x2FFF

uint8_t gb_mbc5_read(uint16_t address);
void gb_mbc5_reset(void);
void gb_mbc5_step(void);
void gb_mbc5_write(uint16_t address, uint8_t data);

#endif /* GB_MBC5_H_ */
