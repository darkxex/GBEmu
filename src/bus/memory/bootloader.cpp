/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "bootloader.h"
#include <string.h>

static const uint8_t BOOTROM[GB_BOOTLOADER_ROM_WIDTH] = {
#embed "bootloader/bootrom.bin"
};

static union {
    uint8_t raw;
    struct {
        uint8_t disabled : 1;
    };
} bootloader = {};

bool gb_bootloader_enabled(void) {
    return !bootloader.disabled;
}

uint8_t gb_bootloader_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_BOOTLOADER_DISABLE:
        result = bootloader.raw;
        break;
    case GB_BOOTLOADER_ROM_BEGIN ... GB_BOOTLOADER_ROM_END:
        result = BOOTROM[address - GB_BOOTLOADER_ROM_BEGIN];
        break;
    default:
        break;
    }
    return result;
}

void gb_bootloader_reset(void) {
    memset(&bootloader, 0, sizeof(bootloader));
    bootloader.raw = 0xFE;
}

void gb_bootloader_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_BOOTLOADER_DISABLE:
        if (!bootloader.disabled) {
            bootloader.raw = data | 0xFE;
        }
        break;
    default:
        break;
    }
}
