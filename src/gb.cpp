/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "cartridge.h"
#include "client.h"
#include <stdio.h>
#include <string.h>

static struct {
    const gb_option_t *option;
    struct {
        char *path;
        gb_bank_t bank;
    } ram;
    struct {
        const char *path;
        gb_bank_t bank;
    } rom;
} gb = {};

static gb_error_e gb_ram_load(void) {
    gb_error_e result = GB_SUCCESS;
    if (gb.rom.path && ((result = gb_string_allocate(&gb.ram.path, "%s.ram", gb.rom.path)) == GB_SUCCESS)) {
        if (gb_file_exists(gb.ram.path)) {
            result = gb_file_read(gb.ram.path, &gb.ram.bank.data, &gb.ram.bank.length);
        } else {
            gb.ram.bank.length = (GB_CARTRIDGE_RAM_WIDTH * 16) + sizeof(gb_header_t);
            if ((result = gb_buffer_allocate(&gb.ram.bank.data, gb.ram.bank.length)) == GB_SUCCESS) {
                gb_header_t *header = (gb_header_t *)gb.ram.bank.data;
                snprintf(header->magic, sizeof(header->magic), GB_CARTRIDGE_RAM_MAGIC);
                header->version.major = GB_VERSION_MAJOR;
                header->version.minor = GB_VERSION_MINOR;
            }
        }
    }
    return result;
}

static gb_error_e gb_ram_save(void) {
    gb_error_e result = GB_SUCCESS;
    if (gb.ram.path && (gb.ram.bank.length > sizeof(gb_header_t))) {
        result = gb_file_write(gb.ram.path, gb.ram.bank.data, gb.ram.bank.length);
    }
    return result;
}

static void gb_ram_unload(void) {
    gb_buffer_free(gb.ram.bank.data);
    gb_string_free(gb.ram.path);
}

static gb_error_e gb_rom_load(void) {
    gb_error_e result = GB_SUCCESS;
    if (gb.rom.path) {
        result = gb_file_read(gb.rom.path, &gb.rom.bank.data, &gb.rom.bank.length);
    }
    return result;
}

static void gb_rom_unload(void) {
    gb_buffer_free(gb.rom.bank.data);
}

static gb_error_e gb_run(void) {
    gb_error_e result = GB_SUCCESS;
    if ((result = gb_bus_reset(&gb.rom.bank, &gb.ram.bank)) == GB_SUCCESS) {
        if ((result = gb_client_create(gb.option)) == GB_SUCCESS) {
            while (gb_client_poll() && ((result = gb_bus_run()) == GB_SUCCESS) && ((result = gb_client_sync()) == GB_SUCCESS))
                ;
        }
        gb_client_destroy();
    }
    return result;
}

gb_error_e gb_entry(const char *const path, const gb_option_t *const option) {
    gb_error_e result = GB_SUCCESS;
    memset(&gb, 0, sizeof(gb));
    gb.option = option;
    gb.rom.path = path;
    if (((result = gb_rom_load()) == GB_SUCCESS) && ((result = gb_ram_load()) == GB_SUCCESS) && ((result = gb_run()) == GB_SUCCESS)) {
        result = gb_ram_save();
    }
    gb_ram_unload();
    gb_rom_unload();
    return result;
}
