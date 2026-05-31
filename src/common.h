/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_COMMON_H_
#define GB_COMMON_H_

#include "gb.h"

#ifndef PATCH
#define PATCH 0
#endif /* PATCH */

#define GB_CPU_CLOCK 4194304
#define GB_CYCLES_PER_FRAME 70224
#define GB_LENGTH_MAX 0x800000

#define GB_VERSION_MAJOR 0
#define GB_VERSION_MINOR 2
#define GB_VERSION_PATCH PATCH

#define GB_ERROR(_FORMAT_, ...) gb_error_set(__FILE__, __LINE__, _FORMAT_, ##__VA_ARGS__)
#define GB_LENGTH(_ARRAY_) (sizeof(_ARRAY_) / sizeof(*(_ARRAY_)))
#define GB_WIDTH(_BEGIN_, _END_) (((_END_) + 1) - (_BEGIN_))

gb_error_e gb_buffer_allocate(uint8_t **const buffer, uint32_t length);
void gb_buffer_free(uint8_t *const buffer);
gb_error_e gb_error_set(const char *const path, uint32_t line, const char *const format, ...);
bool gb_file_exists(const char *const path);
gb_error_e gb_file_read(const char *const path, uint8_t **const buffer, uint32_t *const length);
gb_error_e gb_file_write(const char *const path, const uint8_t *const buffer, uint32_t length);
gb_error_e gb_string_allocate(char **const string, const char *const format, ...);
void gb_string_free(char *const string);

#endif /* GB_COMMON_H_ */
