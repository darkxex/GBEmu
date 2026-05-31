/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_CLIENT_H_
#define GB_CLIENT_H_

#include "common.h"

#define GB_CLIENT_FRAME_RATE ((float)GB_CPU_CLOCK / GB_CYCLES_PER_FRAME)
#define GB_CLIENT_SCALE_MAX 8
#define GB_CLIENT_SCALE_MIN 1
#define GB_CLIENT_VSYNC false

typedef union {
    uint16_t raw;
    struct {
        uint16_t red : 5;
        uint16_t green : 5;
        uint16_t blue : 5;
        uint16_t : 1;
    };
} gb_color_t;

gb_error_e gb_client_create(const gb_option_t *const option);
void gb_client_destroy(void);
bool gb_client_poll(void);
gb_error_e gb_client_sync(void);

#endif /* GB_CLIENT_H_ */
