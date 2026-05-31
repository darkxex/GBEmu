/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "mbc5.h"
#include <string.h>

static struct {
    struct {
        bool enabled;
        uint16_t bank;
    } ram;
    struct {
        union {
            uint16_t bank;
            struct {
                uint16_t low : 8;
                uint16_t high : 1;
            };
        };
    } rom;
} mbc5 = {};

static void gb_mbc5_update(void) {
    mbc5.ram.bank &= gb_cartridge_ram_count() - 1;
    mbc5.rom.bank &= gb_cartridge_rom_count() - 1;
}

uint8_t gb_mbc5_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        if (mbc5.ram.enabled) {
            result = gb_cartridge_ram_read(mbc5.ram.bank, address - GB_CARTRIDGE_RAM_BEGIN);
        }
        break;
    case GB_CARTRIDGE_ROM_0_BEGIN ... GB_CARTRIDGE_ROM_0_END:
        result = gb_cartridge_rom_read(0, address - GB_CARTRIDGE_ROM_0_BEGIN);
        break;
    case GB_CARTRIDGE_ROM_1_BEGIN ... GB_CARTRIDGE_ROM_1_END:
        result = gb_cartridge_rom_read(mbc5.rom.bank, address - GB_CARTRIDGE_ROM_1_BEGIN);
        break;
    default:
        break;
    }
    return result;
}

void gb_mbc5_reset(void) {
    memset(&mbc5, 0, sizeof(mbc5));
    mbc5.rom.bank = 1;
    gb_mbc5_update();
}

void gb_mbc5_step(void) {
    return;
}

void gb_mbc5_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        if (mbc5.ram.enabled) {
            gb_cartridge_ram_write(mbc5.ram.bank, address - GB_CARTRIDGE_RAM_BEGIN, data);
        }
        break;
    case GB_MBC5_RAM_BANK_BEGIN ... GB_MBC5_RAM_BANK_END:
        mbc5.ram.bank = data & 15;
        gb_mbc5_update();
        break;
    case GB_MBC5_RAM_ENABLE_BEGIN ... GB_MBC5_RAM_ENABLE_END:
        mbc5.ram.enabled = ((data & 0x0F) == 0x0A);
        break;
    case GB_MBC5_ROM_BANK_HIGH_BEGIN ... GB_MBC5_ROM_BANK_HIGH_END:
        mbc5.rom.high = data & 1;
        gb_mbc5_update();
        break;
    case GB_MBC5_ROM_BANK_LOW_BEGIN ... GB_MBC5_ROM_BANK_LOW_END:
        mbc5.rom.low = data;
        gb_mbc5_update();
        break;
    default:
        break;
    }
}
