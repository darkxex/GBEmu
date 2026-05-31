/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "common.h"
#include <stdio.h>

bool gb_file_exists(const char *const path) {
    FILE *file = nullptr;
    if (!(file = fopen(path, "rb"))) {
        return false;
    }
    fclose(file);
    return true;
}

gb_error_e gb_file_read(const char *const path, uint8_t **const buffer, uint32_t *const length) {
    long file_len = 0;
    FILE *file = nullptr;
    gb_error_e result = GB_SUCCESS;
    if (!(file = fopen(path, "rb"))) {
        return GB_ERROR("Failed to open file: \'%s\'", path);
    }
    if (fseek(file, 0, SEEK_END)) {
        fclose(file);
        return GB_ERROR("Failed to seek file: \'%s\'", path);
    }
    if ((file_len = ftell(file)) == -1) {
        fclose(file);
        return GB_ERROR("Failed to read file length: \'%s\'", path);
    }
    *length = file_len;
    if (fseek(file, 0, SEEK_SET)) {
        fclose(file);
        return GB_ERROR("Failed to seek file: \'%s\'", path);
    }
    if ((result = gb_buffer_allocate(buffer, *length)) == GB_SUCCESS) {
        if (fread(*buffer, sizeof(**buffer), *length, file) != *length) {
            result = GB_ERROR("Failed to read file: \'%s\'", path);
            gb_buffer_free(*buffer);
            *buffer = nullptr;
        }
    }
    fclose(file);
    return result;
}

gb_error_e gb_file_write(const char *const path, const uint8_t *const buffer, uint32_t length) {
    FILE *file = nullptr;
    gb_error_e result = GB_SUCCESS;
    if (!(file = fopen(path, "wb"))) {
        return GB_ERROR("Failed to open file: \'%s\'", path);
    }
    if (fwrite(buffer, sizeof(*buffer), length, file) != length) {
        result = GB_ERROR("Failed to write file: \'%s\'", path);
    }
    fclose(file);
    return result;
}
