/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_CARTRIDGE_H_
#define GB_CARTRIDGE_H_

#include "memory.h"

#define GB_CARTRIDGE_CHECKSUM 0x014D
#define GB_CARTRIDGE_CHECKSUM_BEGIN 0x0134
#define GB_CARTRIDGE_CHECKSUM_END 0x014C
#define GB_CARTRIDGE_MAPPER 0x0147
#define GB_CARTRIDGE_RAM 0x0149
#define GB_CARTRIDGE_RAM_BEGIN 0xA000
#define GB_CARTRIDGE_RAM_END 0xBFFF
#define GB_CARTRIDGE_RAM_MAGIC "GB"
#define GB_CARTRIDGE_ROM 0x0148
#define GB_CARTRIDGE_ROM_0_BEGIN 0x0000
#define GB_CARTRIDGE_ROM_0_END 0x3FFF
#define GB_CARTRIDGE_ROM_1_BEGIN 0x4000
#define GB_CARTRIDGE_ROM_1_END 0x7FFF
#define GB_CARTRIDGE_TITLE_BEGIN 0x0134
#define GB_CARTRIDGE_TITLE_END 0x013E

#define GB_CARTRIDGE_RAM_WIDTH GB_WIDTH(GB_CARTRIDGE_RAM_BEGIN, GB_CARTRIDGE_RAM_END)
#define GB_CARTRIDGE_ROM_WIDTH GB_WIDTH(GB_CARTRIDGE_ROM_0_BEGIN, GB_CARTRIDGE_ROM_0_END)
#define GB_CARTRIDGE_TITLE_WIDTH GB_WIDTH(GB_CARTRIDGE_TITLE_BEGIN, GB_CARTRIDGE_TITLE_END)

typedef enum : uint8_t {
    GB_CLOCK_NONE = 0,
    GB_CLOCK_SECOND = 1,
    GB_CLOCK_MINUTE,
    GB_CLOCK_HOUR,
    GB_CLOCK_DAY_LOW,
    GB_CLOCK_DAY_HIGH,
    GB_CLOCK_MAX
} gb_clock_e;

typedef enum : uint8_t {
    GB_MAPPER_MBC0 = 0,
    GB_MAPPER_MBC1,
    GB_MAPPER_MBC2,
    GB_MAPPER_MBC3,
    GB_MAPPER_MBC5,
    GB_MAPPER_MAX
} gb_mapper_e;

typedef struct {
    union {
        uint8_t raw;
        uint8_t counter : 6;
    } second;
    union {
        uint8_t raw;
        uint8_t counter : 6;
    } minute;
    union {
        uint8_t raw;
        uint8_t counter : 5;
    } hour;
    union {
        uint16_t raw;
        struct {
            uint8_t low;
            uint8_t high;
        };
        struct {
            uint16_t counter : 9;
            uint16_t : 5;
            uint16_t halt : 1;
            uint16_t carry : 1;
        };
    } day;
} __attribute__((packed)) gb_clock_t;

typedef union {
    uint8_t raw[16];
    struct {
        char magic[4];
        struct {
            uint8_t major;
            uint8_t minor;
        } version;
        gb_clock_t clock;
    };
} __attribute__((packed)) gb_header_t;

typedef struct {
    uint8_t id;
    gb_mapper_e type;
    uint8_t (*read)(uint16_t);
    void (*reset)(void);
    void (*step)(void);
    void (*write)(uint16_t, uint8_t);
} gb_mapper_t;

void gb_cartridge_clock_latch(void);
uint8_t gb_cartridge_clock_read(gb_clock_e address);
void gb_cartridge_clock_step(void);
void gb_cartridge_clock_write(gb_clock_e address, uint8_t data);
uint16_t gb_cartridge_ram_count(void);
uint8_t gb_cartridge_ram_read(uint16_t bank, uint16_t address);
void gb_cartridge_ram_write(uint16_t bank, uint16_t address, uint8_t data);
uint8_t gb_cartridge_read(uint16_t address);
gb_error_e gb_cartridge_reset(const gb_bank_t *const rom, gb_bank_t *const ram);
uint16_t gb_cartridge_rom_count(void);
uint8_t gb_cartridge_rom_read(uint16_t bank, uint16_t address);
void gb_cartridge_step(void);
const char *gb_cartridge_title(void);
void gb_cartridge_write(uint16_t address, uint8_t data);

#endif /* GB_CARTRIDGE_H_ */
