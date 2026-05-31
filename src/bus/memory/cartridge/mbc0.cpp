/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "mbc0.h"
#include <string.h>

uint8_t gb_mbc0_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        result = gb_cartridge_ram_read(0, address - GB_CARTRIDGE_RAM_BEGIN);
        break;
    case GB_CARTRIDGE_ROM_0_BEGIN ... GB_CARTRIDGE_ROM_0_END:
        result = gb_cartridge_rom_read(0, address - GB_CARTRIDGE_ROM_0_BEGIN);
        break;
    case GB_CARTRIDGE_ROM_1_BEGIN ... GB_CARTRIDGE_ROM_1_END:
        result = gb_cartridge_rom_read(1, address - GB_CARTRIDGE_ROM_1_BEGIN);
        break;
    default:
        break;
    }
    return result;
}

void gb_mbc0_reset(void) {
    return;
}

void gb_mbc0_step(void) {
    return;
}

void gb_mbc0_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        gb_cartridge_ram_write(0, address - GB_CARTRIDGE_RAM_BEGIN, data);
        break;
    default:
        break;
    }
}
