/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "gb.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *DESCRIPTION[] = { "Set window fullscreen", "Show help information", "Set window scale",
                                     "Show version information" };

static const struct option FLAG[] = { { "fullscreen", no_argument, nullptr, 'f' },    { "help", no_argument, nullptr, 'h' },
                                      { "scale", required_argument, nullptr, 's' }, { "version", no_argument, nullptr, 'v' },
                                      { nullptr, 0, nullptr, 0 } };

static void usage(void) {
    uint32_t index = 0;
    fprintf(stdout, "Usage: gb [options] [file]\n\n");
    fprintf(stdout, "Options:\n");
    while (FLAG[index].name) {
        fprintf(stdout, "   -%c, --%-12s%s\n", FLAG[index].val, FLAG[index].name, DESCRIPTION[index]);
        ++index;
    }
}

static void version(void) {
    const gb_version_t *const version = gb_version();
    fprintf(stdout, "%u.%u-%x\n", version->major, version->minor, version->patch);
}

int main(int argc, char *argv[]) {
    int index = 0;
    const char *path = nullptr;
    gb_error_e result = GB_SUCCESS;
    gb_option_t option{};
    option.fullscreen = false;
    option.scale = 2;
    while ((index = getopt_long(argc, argv, "fhs:v", FLAG, nullptr)) != -1) {
        switch (index) {
        case 'f':
            option.fullscreen = true;
            break;
        case 'h':
            usage();
            return GB_SUCCESS;
        case 's':
            option.scale = static_cast<uint8_t>(strtoul(optarg, nullptr, 10));
            break;
        case 'v':
            version();
            return GB_SUCCESS;
        case '?':
        default:
            usage();
            return GB_FAILURE;
        }
    }
    for (index = optind; index < argc; ++index) {
        if (path) {
            usage();
            return GB_FAILURE;
        }
        path = argv[index];
    }
    if ((result = gb_entry(path, &option)) != GB_SUCCESS) {
        fprintf(stderr, "%s\n", gb_error());
    }
    return result;
}
