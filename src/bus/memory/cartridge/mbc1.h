/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_MBC1_H_
#define GB_MBC1_H_

#include "cartridge.h"

#define GB_MBC1_BANK_HIGH_BEGIN 0x4000
#define GB_MBC1_BANK_HIGH_END 0x5FFF
#define GB_MBC1_BANK_LOW_BEGIN 0x2000
#define GB_MBC1_BANK_LOW_END 0x3FFF
#define GB_MBC1_BANK_SELECT_BEGIN 0x6000
#define GB_MBC1_BANK_SELECT_END 0x7FFF
#define GB_MBC1_RAM_ENABLE_BEGIN 0x0000
#define GB_MBC1_RAM_ENABLE_END 0x1FFF

uint8_t gb_mbc1_read(uint16_t address);
void gb_mbc1_reset(void);
void gb_mbc1_step(void);
void gb_mbc1_write(uint16_t address, uint8_t data);

#endif /* GB_MBC1_H_ */
