/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "serial.h"
#include "processor.h"
#ifndef NDEBUG
#include <stdio.h>
#endif /* NDEBUG */
#include <string.h>

static struct {
    bool overflow;
    uint8_t data;
    uint16_t divider;
    union {
        uint8_t raw;
        struct {
            uint8_t mode : 1;
            uint8_t : 6;
            uint8_t enabled : 1;
        };
    } control;
} serial = {};

uint8_t gb_serial_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_SERIAL_CONTROL:
        result = serial.control.raw;
        break;
    case GB_SERIAL_DATA:
        result = serial.data;
        break;
    default:
        break;
    }
    return result;
}

void gb_serial_reset(void) {
    memset(&serial, 0, sizeof(serial));
    serial.control.raw = 0x7E;
    serial.data = 0xFF;
}

void gb_serial_step(void) {
    if (serial.control.enabled && serial.control.mode) {
        bool overflow = serial.divider & 2048;
        if (overflow && !serial.overflow) {
            serial.control.enabled = false;
            serial.data = 0xFF;
            serial.divider = 0;
            serial.overflow = false;
            gb_processor_interrupt(GB_INTERRUPT_SERIAL);
            return;
        }
        serial.overflow = overflow;
        ++serial.divider;
    }
}

void gb_serial_write(uint16_t address, uint8_t data) {
    switch (address) {
    case GB_SERIAL_CONTROL:
        serial.control.raw = data | 0x7E;
        serial.divider = 0;
        serial.overflow = false;
        break;
    case GB_SERIAL_DATA:
#ifndef NDEBUG
        fprintf(stdout, "%c", data);
        fflush(stdout);
#endif /* NDEBUG */
        serial.data = data;
        break;
    default:
        break;
    }
}
