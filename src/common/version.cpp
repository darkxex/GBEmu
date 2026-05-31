/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "common.h"

static const gb_version_t VERSION = { GB_VERSION_MAJOR, GB_VERSION_MINOR, GB_VERSION_PATCH };

const gb_version_t *gb_version(void) {
    return &VERSION;
}
