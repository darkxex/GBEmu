/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "common.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static char error[256] = {};

const char *gb_error(void) {
    return error;
}

gb_error_e gb_error_set(const char *const file, uint32_t line, const char *const format, ...) {
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(error, sizeof(error), format, arguments);
    va_end(arguments);
#ifndef NDEBUG
    snprintf(error + strlen(error), sizeof(error) - strlen(error), " (%s:%u)", file, line);
#endif
    return GB_FAILURE;
}
