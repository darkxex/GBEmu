/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "common.h"
#include <stdlib.h>

gb_error_e gb_buffer_allocate(uint8_t **const buffer, uint32_t length) {
    if (length == 0) {
        return GB_ERROR("Buffer is empty: %u bytes", length);
    } else if (length > GB_LENGTH_MAX) {
        return GB_ERROR("Buffer is too large: %u bytes", length);
    }
    if (!(*buffer = static_cast<uint8_t*>(calloc(length, sizeof(**buffer))))) {
        return GB_ERROR("Failed to allocate buffer: %u bytes", length);
    }
    return GB_SUCCESS;
}

void gb_buffer_free(uint8_t *const buffer) {
    free(buffer);
}
