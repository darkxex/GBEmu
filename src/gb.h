/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_H_
#define GB_H_

#include <stdbool.h>
#include <stdint.h>

typedef enum : int8_t {
    GB_FAILURE = -1,
    GB_SUCCESS = 0,
    GB_COMPLETE
} gb_error_e;

typedef enum : uint8_t {
    GB_PALETTE_NONE = 0,
    GB_PALETTE_DMG,
    GB_PALETTE_POCKET,
    GB_PALETTE_LIGHT,
    GB_PALETTE_MAX
} gb_palette_e;

typedef struct {
    bool fullscreen;
    uint8_t scale;
    gb_palette_e palette;
} gb_option_t;

typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
} gb_version_t;

gb_error_e gb_entry(const char *const path, const gb_option_t *const option);
const char *gb_error(void);
const gb_version_t *gb_version(void);

#endif /* GB_H_ */
