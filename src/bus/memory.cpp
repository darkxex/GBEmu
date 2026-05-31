/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "bootloader.h"
#include "cartridge.h"
#include <string.h>

static struct {
    uint8_t high[GB_MEMORY_RAM_HIGH_WIDTH];
    uint8_t work[GB_MEMORY_RAM_WORK_WIDTH];
} memory = {};

uint8_t gb_memory_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_BOOTLOADER_DISABLE:
        result = gb_bootloader_read(address);
        break;
    case GB_BOOTLOADER_ROM_BEGIN ... GB_BOOTLOADER_ROM_END:
        result = gb_bootloader_enabled() ? gb_bootloader_read(address) : gb_cartridge_read(address);
        break;
    case GB_MEMORY_RAM_HIGH_BEGIN ... GB_MEMORY_RAM_HIGH_END:
        result = memory.high[address - GB_MEMORY_RAM_HIGH_BEGIN];
        break;
    case GB_MEMORY_RAM_ECHO_BEGIN ... GB_MEMORY_RAM_ECHO_END:
        result = memory.work[address - GB_MEMORY_RAM_ECHO_BEGIN];
        break;
    case GB_MEMORY_RAM_UNUSED_BEGIN ... GB_MEMORY_RAM_UNUSED_END:
        result = 0;
        break;
    case GB_MEMORY_RAM_WORK_BEGIN ... GB_MEMORY_RAM_WORK_END:
        result = memory.work[address - GB_MEMORY_RAM_WORK_BEGIN];
        break;
    default:
        result = gb_cartridge_read(address);
        break;
    }
    return result;
}

gb_error_e gb_memory_reset(const gb_bank_t *const rom, gb_bank_t *const ram) {
    gb_error_e result = GB_SUCCESS;
    memset(&memory, 0, sizeof(memory));
    if ((result = gb_cartridge_reset(rom, ram)) == GB_SUCCESS) {
        gb_bootloader_reset();
    }
    return result;
}

void gb_memory_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_BOOTLOADER_DISABLE:
        gb_bootloader_write(address, data);
        break;
    case GB_MEMORY_RAM_HIGH_BEGIN ... GB_MEMORY_RAM_HIGH_END:
        memory.high[address - GB_MEMORY_RAM_HIGH_BEGIN] = data;
        break;
    case GB_MEMORY_RAM_ECHO_BEGIN ... GB_MEMORY_RAM_ECHO_END:
        memory.work[address - GB_MEMORY_RAM_ECHO_BEGIN] = data;
        break;
    case GB_MEMORY_RAM_UNUSED_BEGIN ... GB_MEMORY_RAM_UNUSED_END:
        break;
    case GB_MEMORY_RAM_WORK_BEGIN ... GB_MEMORY_RAM_WORK_END:
        memory.work[address - GB_MEMORY_RAM_WORK_BEGIN] = data;
        break;
    default:
        gb_cartridge_write(address, data);
        break;
    }
}
