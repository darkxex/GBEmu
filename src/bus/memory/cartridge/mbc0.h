/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_MBC0_H_
#define GB_MBC0_H_

#include "cartridge.h"

uint8_t gb_mbc0_read(uint16_t address);
void gb_mbc0_reset(void);
void gb_mbc0_step(void);
void gb_mbc0_write(uint16_t address, uint8_t data);

#endif /* GB_MBC0_H_ */
