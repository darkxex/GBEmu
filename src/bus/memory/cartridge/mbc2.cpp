/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "mbc2.h"
#include <string.h>

static struct {
    struct {
        bool enabled;
    } ram;
    struct {
        uint16_t bank;
    } rom;
} mbc2 = {};

static void gb_mbc2_update(void) {
    if (!mbc2.rom.bank) {
        ++mbc2.rom.bank;
    }
    mbc2.rom.bank &= gb_cartridge_rom_count() - 1;
}

uint8_t gb_mbc2_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        if (mbc2.ram.enabled) {
            result = gb_cartridge_ram_read(0, (address - GB_CARTRIDGE_RAM_BEGIN) & 511) | 0xF0;
        }
        break;
    case GB_CARTRIDGE_ROM_0_BEGIN ... GB_CARTRIDGE_ROM_0_END:
        result = gb_cartridge_rom_read(0, address - GB_CARTRIDGE_ROM_0_BEGIN);
        break;
    case GB_CARTRIDGE_ROM_1_BEGIN ... GB_CARTRIDGE_ROM_1_END:
        result = gb_cartridge_rom_read(mbc2.rom.bank, address - GB_CARTRIDGE_ROM_1_BEGIN);
        break;
    default:
        break;
    }
    return result;
}

void gb_mbc2_reset(void) {
    memset(&mbc2, 0, sizeof(mbc2));
    gb_mbc2_update();
}

void gb_mbc2_step(void) {
    return;
}

void gb_mbc2_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        if (mbc2.ram.enabled) {
            gb_cartridge_ram_write(0, (address - GB_CARTRIDGE_RAM_BEGIN) & 511, data | 0xF0);
        }
        break;
    case GB_MBC2_BANK_SELECT_BEGIN ... GB_MBC2_BANK_SELECT_END:
        if (address & 0x0100) {
            mbc2.rom.bank = data & 15;
            gb_mbc2_update();
        } else {
            mbc2.ram.enabled = ((data & 0x0F) == 0x0A);
        }
        break;
    default:
        break;
    }
}
