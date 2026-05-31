/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "mbc1.h"
#include <string.h>

static struct {
    uint8_t high;
    uint8_t low;
    uint8_t select;
    struct {
        bool enabled;
        uint16_t bank;
    } ram;
    struct {
        uint16_t bank[2];
    } rom;
} mbc1 = {};

static void gb_mbc1_update(void) {
    uint16_t count = gb_cartridge_rom_count();
    if (count >= 64) {
        mbc1.ram.bank = 0;
        mbc1.rom.bank[0] = mbc1.select ? (mbc1.high << 5) : 0;
        mbc1.rom.bank[1] = (mbc1.high << 5) | mbc1.low;
    } else {
        mbc1.ram.bank = mbc1.select ? mbc1.high : 0;
        mbc1.rom.bank[0] = 0;
        mbc1.rom.bank[1] = mbc1.low;
    }
    switch (mbc1.rom.bank[1]) {
    case 0:
    case 32:
    case 64:
    case 96:
        ++mbc1.rom.bank[1];
        break;
    default:
        break;
    }
    mbc1.rom.bank[0] &= count - 1;
    mbc1.rom.bank[1] &= count - 1;
    count = gb_cartridge_ram_count();
    mbc1.ram.bank &= count - 1;
}

uint8_t gb_mbc1_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        if (mbc1.ram.enabled) {
            result = gb_cartridge_ram_read(mbc1.ram.bank, address - GB_CARTRIDGE_RAM_BEGIN);
        }
        break;
    case GB_CARTRIDGE_ROM_0_BEGIN ... GB_CARTRIDGE_ROM_0_END:
        result = gb_cartridge_rom_read(mbc1.rom.bank[0], address - GB_CARTRIDGE_ROM_0_BEGIN);
        break;
    case GB_CARTRIDGE_ROM_1_BEGIN ... GB_CARTRIDGE_ROM_1_END:
        result = gb_cartridge_rom_read(mbc1.rom.bank[1], address - GB_CARTRIDGE_ROM_1_BEGIN);
        break;
    default:
        break;
    }
    return result;
}

void gb_mbc1_reset(void) {
    memset(&mbc1, 0, sizeof(mbc1));
    gb_mbc1_update();
}

void gb_mbc1_step(void) {
    return;
}

void gb_mbc1_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        if (mbc1.ram.enabled) {
            gb_cartridge_ram_write(mbc1.ram.bank, address - GB_CARTRIDGE_RAM_BEGIN, data);
        }
        break;
    case GB_MBC1_BANK_HIGH_BEGIN ... GB_MBC1_BANK_HIGH_END:
        mbc1.high = data & 3;
        gb_mbc1_update();
        break;
    case GB_MBC1_BANK_LOW_BEGIN ... GB_MBC1_BANK_LOW_END:
        mbc1.low = data & 31;
        gb_mbc1_update();
        break;
    case GB_MBC1_BANK_SELECT_BEGIN ... GB_MBC1_BANK_SELECT_END:
        mbc1.select = data & 1;
        gb_mbc1_update();
        break;
    case GB_MBC1_RAM_ENABLE_BEGIN ... GB_MBC1_RAM_ENABLE_END:
        mbc1.ram.enabled = ((data & 0x0F) == 0x0A);
        break;
    default:
        break;
    }
}
