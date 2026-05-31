/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "mbc0.h"
#include "mbc1.h"
#include "mbc2.h"
#include "mbc3.h"
#include "mbc5.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static const gb_mapper_t MAPPER[] = { { 0, GB_MAPPER_MBC0, gb_mbc0_read, gb_mbc0_reset, gb_mbc0_step, gb_mbc0_write },
                                      { 1, GB_MAPPER_MBC1, gb_mbc1_read, gb_mbc1_reset, gb_mbc1_step, gb_mbc1_write },
                                      { 2, GB_MAPPER_MBC1, gb_mbc1_read, gb_mbc1_reset, gb_mbc1_step, gb_mbc1_write },
                                      { 3, GB_MAPPER_MBC1, gb_mbc1_read, gb_mbc1_reset, gb_mbc1_step, gb_mbc1_write },
                                      { 5, GB_MAPPER_MBC2, gb_mbc2_read, gb_mbc2_reset, gb_mbc2_step, gb_mbc2_write },
                                      { 6, GB_MAPPER_MBC2, gb_mbc2_read, gb_mbc2_reset, gb_mbc2_step, gb_mbc2_write },
                                      { 8, GB_MAPPER_MBC0, gb_mbc0_read, gb_mbc0_reset, gb_mbc0_step, gb_mbc0_write },
                                      { 9, GB_MAPPER_MBC0, gb_mbc0_read, gb_mbc0_reset, gb_mbc0_step, gb_mbc0_write },
                                      { 15, GB_MAPPER_MBC3, gb_mbc3_read, gb_mbc3_reset, gb_mbc3_step, gb_mbc3_write },
                                      { 16, GB_MAPPER_MBC3, gb_mbc3_read, gb_mbc3_reset, gb_mbc3_step, gb_mbc3_write },
                                      { 17, GB_MAPPER_MBC3, gb_mbc3_read, gb_mbc3_reset, gb_mbc3_step, gb_mbc3_write },
                                      { 18, GB_MAPPER_MBC3, gb_mbc3_read, gb_mbc3_reset, gb_mbc3_step, gb_mbc3_write },
                                      { 19, GB_MAPPER_MBC3, gb_mbc3_read, gb_mbc3_reset, gb_mbc3_step, gb_mbc3_write },
                                      { 25, GB_MAPPER_MBC5, gb_mbc5_read, gb_mbc5_reset, gb_mbc5_step, gb_mbc5_write },
                                      { 26, GB_MAPPER_MBC5, gb_mbc5_read, gb_mbc5_reset, gb_mbc5_step, gb_mbc5_write },
                                      { 27, GB_MAPPER_MBC5, gb_mbc5_read, gb_mbc5_reset, gb_mbc5_step, gb_mbc5_write },
                                      { 28, GB_MAPPER_MBC5, gb_mbc5_read, gb_mbc5_reset, gb_mbc5_step, gb_mbc5_write },
                                      { 29, GB_MAPPER_MBC5, gb_mbc5_read, gb_mbc5_reset, gb_mbc5_step, gb_mbc5_write },
                                      { 30, GB_MAPPER_MBC5, gb_mbc5_read, gb_mbc5_reset, gb_mbc5_step, gb_mbc5_write } };

static const uint16_t RAM[] = { 0, 0, 1, 4, 16, 8 };

static const uint16_t ROM[] = { 2, 4, 8, 16, 32, 64, 128, 256, 512 };

static struct {
    const gb_mapper_t *mapper;
    char title[GB_CARTRIDGE_TITLE_WIDTH + 1];
    struct {
        uint32_t delay;
        gb_clock_t latch;
    } clock;
    struct {
        uint16_t count;
        uint8_t *data;
        gb_clock_t *clock;
    } ram;
    struct {
        uint16_t count;
        const uint8_t *data;
    } rom;
} cartridge = {};

static gb_error_e gb_cartridge_checksum(const gb_bank_t *const rom) {
    uint8_t expected = 0, found = rom->data[GB_CARTRIDGE_CHECKSUM];
    for (uint32_t address = GB_CARTRIDGE_CHECKSUM_BEGIN; address <= GB_CARTRIDGE_CHECKSUM_END; ++address) {
        expected = expected - rom->data[address] - 1;
    }
    if (expected != found) {
        return GB_ERROR("Mismatched checksum: %02X (expecting %02X)", found, expected);
    }
    return GB_SUCCESS;
}

static void gb_cartridge_clock_step_day(void) {
    if ((cartridge.ram.clock->day.carry = (cartridge.ram.clock->day.counter == 511))) {
        cartridge.ram.clock->day.counter = 0;
    } else {
        ++cartridge.ram.clock->day.counter;
    }
}

static void gb_cartridge_clock_step_hour(void) {
    if (++cartridge.ram.clock->hour.counter == 24) {
        cartridge.ram.clock->hour.counter = 0;
        gb_cartridge_clock_step_day();
    }
}

static void gb_cartridge_clock_step_minute(void) {
    if (++cartridge.ram.clock->minute.counter == 60) {
        cartridge.ram.clock->minute.counter = 0;
        gb_cartridge_clock_step_hour();
    }
}

static void gb_cartridge_clock_step_second(void) {
    if (++cartridge.ram.clock->second.counter == 60) {
        cartridge.ram.clock->second.counter = 0;
        gb_cartridge_clock_step_minute();
    }
}

static gb_error_e gb_cartridge_mapper_set(const gb_bank_t *const rom) {
    uint8_t id = rom->data[GB_CARTRIDGE_MAPPER];
    for (uint32_t index = 0; index < GB_LENGTH(MAPPER); ++index) {
        cartridge.mapper = &MAPPER[index];
        if (cartridge.mapper->id == id) {
            cartridge.mapper->reset();
            return GB_SUCCESS;
        }
    }
    return GB_ERROR("Unsupported mapper: %u", id);
}

static void gb_cartridge_title_set(const gb_bank_t *const rom) {
    memset(cartridge.title, 0, sizeof(cartridge.title));
    for (uint32_t offset = 0; offset < GB_CARTRIDGE_TITLE_WIDTH; ++offset) {
        char value = rom->data[GB_CARTRIDGE_TITLE_BEGIN + offset];
        if (value == '\0') {
            break;
        } else if (isprint(value) || isspace(value)) {
            cartridge.title[offset] = value;
        }
    }
    if (!strlen(cartridge.title)) {
        snprintf(cartridge.title, GB_CARTRIDGE_TITLE_WIDTH + 1, "UNTITLED");
    }
}

static gb_error_e gb_cartridge_validate_ram(gb_bank_t *const ram) {
    uint8_t type = cartridge.rom.data[GB_CARTRIDGE_RAM];
    uint32_t length = 0;
    gb_error_e result = GB_SUCCESS;
    gb_header_t *header = (gb_header_t *)ram->data;
    if (type >= GB_LENGTH(RAM)) {
        return GB_ERROR("Unsupported ram type: %u", type);
    }
    if (ram->length < sizeof(*header)) {
        return GB_ERROR("Invalid ram length: %u bytes", ram->length);
    }
    if (strncmp(header->magic, GB_CARTRIDGE_RAM_MAGIC, sizeof(header->magic))) {
        return GB_ERROR("Invalid ram header: %.*s", (int)sizeof(header->magic), header->magic);
    }
    switch (cartridge.mapper->type) {
    case GB_MAPPER_MBC2:
        cartridge.ram.count = 1;
        length = 512;
        break;
    default:
        cartridge.ram.count = RAM[type];
        length = cartridge.ram.count * GB_CARTRIDGE_RAM_WIDTH;
        break;
    }
    length += sizeof(*header);
    if (ram->length < length) {
        return GB_ERROR("Invalid ram length: %u bytes (expecting >= %u bytes)", ram->length, length);
    }
    cartridge.ram.clock = &header->clock;
    ram->length = length;
    if (cartridge.ram.count) {
        cartridge.ram.data = ram->data + sizeof(*header);
    }
    return result;
}

static gb_error_e gb_cartridge_validate_rom(const gb_bank_t *const rom) {
    gb_error_e result = GB_SUCCESS;
    if (rom->length < (GB_CARTRIDGE_ROM_WIDTH * 2)) {
        return GB_ERROR("Invalid rom length: %u bytes", rom->length);
    }
    if (((result = gb_cartridge_checksum(rom)) == GB_SUCCESS) && ((result = gb_cartridge_mapper_set(rom)) == GB_SUCCESS)) {
        uint8_t type = rom->data[GB_CARTRIDGE_ROM];
        uint32_t length = 0;
        if (type >= GB_LENGTH(ROM)) {
            return GB_ERROR("Unsupported rom type: %u", type);
        }
        cartridge.rom.count = ROM[type];
        length = cartridge.rom.count * GB_CARTRIDGE_ROM_WIDTH;
        if (rom->length != length) {
            return GB_ERROR("Invalid rom length: %u bytes (expecting %u bytes)", rom->length, length);
        }
        type = rom->data[GB_CARTRIDGE_RAM];
        if (type >= GB_LENGTH(RAM)) {
            return GB_ERROR("Unsupported ram type: %u", type);
        }
        gb_cartridge_title_set(rom);
        cartridge.rom.data = rom->data;
    }
    return result;
}

void gb_cartridge_clock_latch(void) {
    memcpy(&cartridge.clock.latch, cartridge.ram.clock, sizeof(*cartridge.ram.clock));
}

uint8_t gb_cartridge_clock_read(gb_clock_e address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_CLOCK_SECOND:
        result = cartridge.clock.latch.second.raw;
        break;
    case GB_CLOCK_MINUTE:
        result = cartridge.clock.latch.minute.raw;
        break;
    case GB_CLOCK_HOUR:
        result = cartridge.clock.latch.hour.raw;
        break;
    case GB_CLOCK_DAY_LOW:
        result = cartridge.clock.latch.day.low;
        break;
    case GB_CLOCK_DAY_HIGH:
        result = cartridge.clock.latch.day.high;
        break;
    default:
        break;
    }
    return result;
}

void gb_cartridge_clock_step(void) {
    if (!cartridge.ram.clock->day.halt) {
        if (++cartridge.clock.delay >= GB_CPU_CLOCK) {
            cartridge.clock.delay = 0;
            gb_cartridge_clock_step_second();
        }
    }
}

void gb_cartridge_clock_write(gb_clock_e address, uint8_t data) {
    switch (address) {
    case GB_CLOCK_SECOND:
        cartridge.ram.clock->second.counter = data;
        break;
    case GB_CLOCK_MINUTE:
        cartridge.ram.clock->minute.counter = data;
        break;
    case GB_CLOCK_HOUR:
        cartridge.ram.clock->hour.counter = data;
        break;
    case GB_CLOCK_DAY_LOW:
        cartridge.ram.clock->day.low = data;
        break;
    case GB_CLOCK_DAY_HIGH:
        cartridge.ram.clock->day.high = data & 0xC1;
        break;
    default:
        break;
    }
}

uint16_t gb_cartridge_ram_count(void) {
    return cartridge.ram.count;
}

uint8_t gb_cartridge_ram_read(uint16_t bank, uint16_t address) {
    uint8_t result = 0xFF;
    if (cartridge.ram.data) {
        result = cartridge.ram.data[(bank * GB_CARTRIDGE_RAM_WIDTH) + address];
    }
    return result;
}

void gb_cartridge_ram_write(uint16_t bank, uint16_t address, uint8_t data) {
    if (cartridge.ram.data) {
        cartridge.ram.data[(bank * GB_CARTRIDGE_RAM_WIDTH) + address] = data;
    }
}

uint8_t gb_cartridge_read(uint16_t address) {
    return cartridge.mapper->read(address);
}

gb_error_e gb_cartridge_reset(const gb_bank_t *const rom, gb_bank_t *const ram) {
    gb_error_e result = GB_SUCCESS;
    memset(&cartridge, 0, sizeof(cartridge));
    cartridge.mapper = &MAPPER[0];
    snprintf(cartridge.title, GB_CARTRIDGE_TITLE_WIDTH + 1, "UNDEFINED");
    if (rom->data && ((result = gb_cartridge_validate_rom(rom)) == GB_SUCCESS)) {
        result = gb_cartridge_validate_ram(ram);
    }
    return result;
}

uint16_t gb_cartridge_rom_count(void) {
    return cartridge.rom.count;
}

uint8_t gb_cartridge_rom_read(uint16_t bank, uint16_t address) {
    uint8_t result = 0xFF;
    if (cartridge.rom.data) {
        result = cartridge.rom.data[(bank * GB_CARTRIDGE_ROM_WIDTH) + address];
    }
    return result;
}

void gb_cartridge_step(void) {
    cartridge.mapper->step();
}

const char *gb_cartridge_title(void) {
    return cartridge.title;
}

void gb_cartridge_write(uint16_t address, uint8_t data) {
    cartridge.mapper->write(address, data);
}
