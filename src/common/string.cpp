/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "common.h"
#include <stdarg.h>
#include <stdio.h>

gb_error_e gb_string_allocate(char **const string, const char *const format, ...) {
    uint32_t length = 0;
    int32_t format_length = 0;
    va_list arguments;
    gb_error_e result = GB_SUCCESS;
    va_start(arguments, format);
    format_length = vsnprintf(nullptr, 0, format, arguments);
    va_end(arguments);
    if (format_length < 0) {
        return GB_ERROR("Malformed string format");
    }
    length = format_length + 1;
    if ((result = gb_buffer_allocate((uint8_t **const)string, length)) == GB_SUCCESS) {
        va_start(arguments, format);
        vsnprintf(*string, length, format, arguments);
        va_end(arguments);
    }
    return result;
}

void gb_string_free(char *const string) {
    gb_buffer_free((uint8_t *const)string);
}
