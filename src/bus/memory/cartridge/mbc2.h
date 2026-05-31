/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_MBC2_H_
#define GB_MBC2_H_

#include "cartridge.h"

#define GB_MBC2_BANK_SELECT_BEGIN 0x0000
#define GB_MBC2_BANK_SELECT_END 0x3FFF

uint8_t gb_mbc2_read(uint16_t address);
void gb_mbc2_reset(void);
void gb_mbc2_step(void);
void gb_mbc2_write(uint16_t address, uint8_t data);

#endif /* GB_MBC2_H_ */
