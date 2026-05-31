/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "mbc3.h"
#include <string.h>

static struct {
    struct {
        bool latched;
        gb_clock_e bank;
    } clock;
    struct {
        bool enabled;
        uint16_t bank;
    } ram;
    struct {
        uint16_t bank;
    } rom;
} mbc3 = {};

static void gb_mbc3_update(void) {
    mbc3.ram.bank &= gb_cartridge_ram_count() - 1;
    mbc3.rom.bank &= gb_cartridge_rom_count() - 1;
    if (!mbc3.rom.bank) {
        ++mbc3.rom.bank;
    }
}

uint8_t gb_mbc3_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        if (mbc3.ram.enabled) {
            switch (mbc3.clock.bank) {
            case GB_CLOCK_SECOND ... GB_CLOCK_DAY_HIGH:
                result = gb_cartridge_clock_read(mbc3.clock.bank);
                break;
            default:
                result = gb_cartridge_ram_read(mbc3.ram.bank, address - GB_CARTRIDGE_RAM_BEGIN);
                break;
            }
        }
        break;
    case GB_CARTRIDGE_ROM_0_BEGIN ... GB_CARTRIDGE_ROM_0_END:
        result = gb_cartridge_rom_read(0, address - GB_CARTRIDGE_ROM_0_BEGIN);
        break;
    case GB_CARTRIDGE_ROM_1_BEGIN ... GB_CARTRIDGE_ROM_1_END:
        result = gb_cartridge_rom_read(mbc3.rom.bank, address - GB_CARTRIDGE_ROM_1_BEGIN);
        break;
    default:
        break;
    }
    return result;
}

void gb_mbc3_reset(void) {
    memset(&mbc3, 0, sizeof(mbc3));
    gb_mbc3_update();
}

void gb_mbc3_step(void) {
    if (mbc3.ram.enabled) {
        gb_cartridge_clock_step();
    }
}

void gb_mbc3_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_CARTRIDGE_RAM_BEGIN ... GB_CARTRIDGE_RAM_END:
        if (mbc3.ram.enabled) {
            switch (mbc3.clock.bank) {
            case GB_CLOCK_SECOND ... GB_CLOCK_DAY_HIGH:
                gb_cartridge_clock_write(mbc3.clock.bank, data);
                break;
            default:
                gb_cartridge_ram_write(mbc3.ram.bank, address - GB_CARTRIDGE_RAM_BEGIN, data);
                break;
            }
        }
        break;
    case GB_MBC3_CLOCK_LATCH_BEGIN ... GB_MBC3_CLOCK_LATCH_END:
        if (!data && !mbc3.clock.latched) {
            mbc3.clock.latched = true;
        } else if (data && mbc3.clock.latched) {
            mbc3.clock.latched = false;
            gb_cartridge_clock_latch();
        }
        break;
    case GB_MBC3_RAM_BANK_BEGIN ... GB_MBC3_RAM_BANK_END:
        switch (data) {
        case GB_MBC3_CLOCK_BANK_BEGIN ... GB_MBC3_CLOCK_BANK_END:
            mbc3.clock.bank = static_cast<gb_clock_e>((data - GB_MBC3_CLOCK_BANK_BEGIN) + GB_CLOCK_SECOND);
            break;
        default:
            mbc3.clock.bank = GB_CLOCK_NONE;
            mbc3.ram.bank = data & 3;
            gb_mbc3_update();
            break;
        }
        break;
    case GB_MBC3_RAM_ENABLE_BEGIN ... GB_MBC3_RAM_ENABLE_END:
        mbc3.ram.enabled = ((data & 0x0F) == 0x0A);
        break;
    case GB_MBC3_ROM_BANK_BEGIN ... GB_MBC3_ROM_BANK_END:
        mbc3.rom.bank = data & 0x7F;
        gb_mbc3_update();
        break;
    default:
        break;
    }
}
