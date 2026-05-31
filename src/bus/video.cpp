/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "video.h"
#include "processor.h"
#include <string.h>

static struct {
    bool interrupt;
    uint8_t ram[GB_VIDEO_RAM_WIDTH];
    gb_color_e color[GB_VIDEO_HEIGHT][GB_VIDEO_WIDTH];
    struct {
        gb_color_e index[GB_VIDEO_WIDTH];
        gb_palette_t palette;
    } background;
    union {
        uint8_t raw;
        struct {
            uint8_t background_enabled : 1;
            uint8_t object_enabled : 1;
            uint8_t object_size : 1;
            uint8_t background_map : 1;
            uint8_t background_data : 1;
            uint8_t window_enabled : 1;
            uint8_t window_map : 1;
            uint8_t enabled : 1;
        };
    } control;
    struct {
        uint8_t coincidence;
        uint16_t x;
        uint8_t y;
    } line;
    struct {
        gb_palette_t palette[2];
        gb_object_t ram[GB_VIDEO_RAM_OBJECT_WIDTH];
        struct {
            uint8_t count;
            gb_object_entry_t entry[10];
        } shown;
    } object;
    struct {
        uint8_t x;
        uint8_t y;
    } scroll;
    union {
        uint8_t raw;
        struct {
            uint8_t mode : 2;
            uint8_t coincidence : 1;
            uint8_t interrupt_hblank : 1;
            uint8_t interrupt_vblank : 1;
            uint8_t interrupt_search : 1;
            uint8_t interrupt_coincidence : 1;
        };
    } status;
    struct {
        uint8_t address;
        uint8_t delay;
        uint16_t destination;
        uint16_t source;
    } transfer;
    struct {
        uint8_t counter;
        uint8_t x;
        uint8_t y;
    } window;
} video = {};

static inline uint8_t gb_video_line_y(void) {
    return ((video.line.y == 153) && (video.line.x >= 4)) ? 0 : video.line.y;
}

static inline gb_color_e gb_video_palette_color(const gb_palette_t *const palette, gb_color_e color) {
    return static_cast<gb_color_e>((palette->raw >> (color * 2)) & 3);
}

static inline void gb_video_status_update(void) {
    bool interrupt = ((video.status.mode == GB_MODE_HBLANK) && video.status.interrupt_hblank) ||
                     ((video.status.mode == GB_MODE_VBLANK) && video.status.interrupt_vblank) ||
                     ((video.status.mode == GB_MODE_SEARCH) && video.status.interrupt_search) ||
                     (video.status.coincidence && video.status.interrupt_coincidence);
    if (interrupt && !video.interrupt) {
        gb_processor_interrupt(GB_INTERRUPT_SCREEN);
    }
    video.interrupt = interrupt;
}

static inline bool gb_video_window_active(void) {
    return video.control.window_enabled && (video.window.x < 167) && (video.window.y < 144) && (video.window.y <= video.line.y);
}

static gb_color_e gb_video_background_color(uint8_t map, uint8_t x, uint8_t y) {
    uint16_t address = (map ? 0x1C00 : 0x1800) + (32 * ((y >> 3) & 31)) + ((x >> 3) & 31);
    if (video.control.background_data) {
        address = (16 * video.ram[address]) + (2 * (y & 7));
    } else {
        address = (16 * (int8_t)video.ram[address]) + (2 * (y & 7)) + 0x1000;
    }
    x = 1 << (7 - (x & 7));
    return static_cast<gb_color_e>(((video.ram[address + 1] & x) ? 2 : 0) + ((video.ram[address] & x) ? 1 : 0));
}

static void gb_video_background_render(void) {
    bool window = gb_video_window_active();
    for (uint8_t index = 0; index < GB_VIDEO_WIDTH; ++index) {
        gb_color_e color = GB_COLOR_WHITE;
        uint8_t map = 0, x = index, y = video.line.y;
        if (window && ((video.window.x - 7) <= x)) {
            map = video.control.window_map;
            x -= (video.window.x - 7);
            y = video.window.counter;
        } else {
            map = video.control.background_map;
            x += video.scroll.x;
            y += video.scroll.y;
        }
        color = gb_video_background_color(map, x, y);
        video.background.index[index] = color;
        video.color[video.line.y][index] = gb_video_palette_color(&video.background.palette, color);
    }
    if (window) {
        ++video.window.counter;
    }
}

static gb_color_e gb_video_object_color(const gb_object_t *object, uint8_t x, uint8_t y) {
    uint16_t address = 0;
    uint8_t index = object->index;
    if (video.control.object_size) {
        if (object->attribute.flip_y) {
            if ((y - (object->y - 16)) < 8) {
                index |= 1;
            } else {
                index &= 0xFE;
            }
        } else if ((y - (object->y - 16)) < 8) {
            index &= 0xFE;
        } else {
            index |= 1;
        }
    }
    y = (y - object->y) & 7;
    if (object->attribute.flip_x) {
        x = 7 - x;
    }
    if (object->attribute.flip_y) {
        y = 7 - y;
    }
    address = (16 * index) + (2 * y);
    x = 1 << (7 - x);
    return static_cast<gb_color_e>(((video.ram[address + 1] & x) ? 2 : 0) + ((video.ram[address] & x) ? 1 : 0));
}

static void gb_video_object_render(void) {
    uint8_t y = video.line.y;
    for (uint32_t index = 0; index < video.object.shown.count; ++index) {
        const gb_object_t *object = video.object.shown.entry[index].object;
        for (uint8_t x = 0; x < 8; ++x) {
            int16_t screen_x = (int16_t)object->x - 8 + x;
            if ((screen_x >= 0) && (screen_x < GB_VIDEO_WIDTH)) {
                gb_color_e color = gb_video_object_color(object, x, y);
                if (color && (!object->attribute.priority || !video.background.index[screen_x])) {
                    video.color[y][screen_x] = gb_video_palette_color(&video.object.palette[object->attribute.palette], color);
                }
            }
        }
    }
}

static void gb_video_object_sort(void) {
    uint8_t size = video.control.object_size ? 16 : 8;
    int16_t y = video.line.y;
    video.object.shown.count = 0;
    for (uint8_t index = 0; index < GB_VIDEO_RAM_OBJECT_WIDTH; ++index) {
        const gb_object_t *object = &video.object.ram[index];
        int16_t top = (int16_t)object->y - 16, bottom = top + size;
        if ((y >= top) && (y < bottom)) {
            gb_object_entry_t *entry = &video.object.shown.entry[video.object.shown.count++];
            entry->object = object;
            entry->index = index;
        }
        if (video.object.shown.count >= 10) {
            break;
        }
    }
    for (int8_t index = 1; index < video.object.shown.count; ++index) {
        gb_object_entry_t entry = video.object.shown.entry[index];
        int8_t index_previous = index - 1;
        while (index_previous >= 0) {
            const gb_object_entry_t *entry_previous = &video.object.shown.entry[index_previous];
            if (((entry_previous->object->x == entry.object->x) ? ((int8_t)entry.index - (int8_t)entry_previous->index)
                                                                : ((int8_t)entry.object->x - (int8_t)entry_previous->object->x)) <= 0) {
                break;
            }
            video.object.shown.entry[index_previous + 1] = video.object.shown.entry[index_previous];
            --index_previous;
        }
        video.object.shown.entry[index_previous + 1] = entry;
    }
}

static void gb_video_coincidence(void) {
    video.status.coincidence = (video.line.coincidence == gb_video_line_y());
    gb_video_status_update();
}

static void gb_video_mode_hblank(void) {
    video.status.mode = GB_MODE_HBLANK;
    gb_video_status_update();
}

static void gb_video_mode_search(void) {
    video.status.mode = GB_MODE_SEARCH;
    gb_video_status_update();
    if (video.control.object_enabled) {
        gb_video_object_sort();
    }
}

static void gb_video_mode_transfer(void) {
    video.status.mode = GB_MODE_TRANSFER;
    memset(video.background.index, 0, sizeof(video.background.index));
    memset(video.color[video.line.y], 0, sizeof(video.color[video.line.y]));
    gb_video_status_update();
    if (video.control.background_enabled) {
        gb_video_background_render();
    }
    if (video.control.object_enabled) {
        gb_video_object_render();
    }
}

static void gb_video_mode_vblank(void) {
    video.status.mode = GB_MODE_VBLANK;
    gb_video_status_update();
    gb_processor_interrupt(GB_INTERRUPT_VBLANK);
}

static void gb_video_transfer(void) {
    if (video.transfer.destination) {
        if (!video.transfer.delay) {
            ((uint8_t *)video.object.ram)[video.transfer.destination++ - GB_VIDEO_RAM_OBJECT_BEGIN] = gb_bus_read(video.transfer.source++);
            if (video.transfer.destination > GB_VIDEO_RAM_OBJECT_END) {
                video.transfer.destination = 0;
                video.transfer.source = 0;
                return;
            }
            video.transfer.delay = 4;
        }
        --video.transfer.delay;
    }
}

const gb_color_e (*gb_video_color(void)) [GB_VIDEO_HEIGHT][GB_VIDEO_WIDTH] { return &video.color; }

uint8_t gb_video_read(uint16_t address) {
    uint8_t result = 0xFF;
    switch (address) {
    case GB_VIDEO_CONTROL:
        result = video.control.raw;
        break;
    case GB_VIDEO_LINE_Y:
        result = video.control.enabled ? gb_video_line_y() : 0;
        break;
    case GB_VIDEO_LINE_Y_COINCIDENCE:
        result = video.line.coincidence;
        break;
    case GB_VIDEO_PALETTE_BACKGROUND:
        result = video.background.palette.raw;
        break;
    case GB_VIDEO_PALETTE_OBJECT_0:
        result = video.object.palette[0].raw;
        break;
    case GB_VIDEO_PALETTE_OBJECT_1:
        result = video.object.palette[1].raw;
        break;
    case GB_VIDEO_RAM_BEGIN ... GB_VIDEO_RAM_END:
        if (!video.control.enabled || (video.status.mode < GB_MODE_TRANSFER)) {
            result = video.ram[address - GB_VIDEO_RAM_BEGIN];
        }
        break;
    case GB_VIDEO_RAM_OBJECT_BEGIN ... GB_VIDEO_RAM_OBJECT_END:
        if (!video.control.enabled || (video.status.mode < GB_MODE_SEARCH)) {
            result = ((uint8_t *)video.object.ram)[address - GB_VIDEO_RAM_OBJECT_BEGIN];
        }
        break;
    case GB_VIDEO_SCROLL_X:
        result = video.scroll.x;
        break;
    case GB_VIDEO_SCROLL_Y:
        result = video.scroll.y;
        break;
    case GB_VIDEO_STATUS:
        result = video.control.enabled ? video.status.raw : ((video.status.raw & ~0x07) | 0x80);
        break;
    case GB_VIDEO_TRANSFER:
        result = video.transfer.address;
        break;
    case GB_VIDEO_WINDOW_X:
        result = video.window.x;
        break;
    case GB_VIDEO_WINDOW_Y:
        result = video.window.y;
        break;
    default:
        break;
    }
    return result;
}

void gb_video_reset(void) {
    memset(&video, 0, sizeof(video));
    video.status.raw = 0x82;
}

gb_error_e gb_video_step(void) {
    gb_error_e result = GB_SUCCESS;
    gb_video_transfer();
    if (video.control.enabled) {
        if ((video.line.y == 153) && (video.line.x == 4)) {
            gb_video_coincidence();
        }
        if (video.line.y < 144) {
            if (video.line.x == 0) {
                gb_video_mode_search();
            } else if (video.line.x == 80) {
                gb_video_mode_transfer();
            } else if (video.line.x == 252) {
                gb_video_mode_hblank();
            }
        } else if ((video.line.x == 0) && (video.line.y == 144)) {
            gb_video_mode_vblank();
        }
    }
    if (++video.line.x == 456) {
        video.line.x = 0;
        if (++video.line.y == 154) {
            video.line.y = 0;
            video.window.counter = 0;
            result = GB_COMPLETE;
        }
        if (video.control.enabled) {
            gb_video_coincidence();
        }
    }
    return result;
}

void gb_video_write(uint16_t address, uint8_t data) {
    bool enabled = false;
    switch (address) {
    case GB_VIDEO_CONTROL:
        enabled = video.control.enabled;
        video.control.raw = data;
        if (enabled && !video.control.enabled) {
            memset(video.background.index, 0, sizeof(video.background.index));
            memset(video.color, 0, sizeof(video.color));
            video.status.coincidence = 0;
            video.window.counter = 0;
        } else if (!enabled && video.control.enabled) {
            gb_video_coincidence();
            video.window.counter = 0;
        }
        break;
    case GB_VIDEO_LINE_Y_COINCIDENCE:
        video.line.coincidence = data;
        if (video.control.enabled) {
            gb_video_coincidence();
        }
        break;
    case GB_VIDEO_PALETTE_BACKGROUND:
        video.background.palette.raw = data;
        break;
    case GB_VIDEO_PALETTE_OBJECT_0:
        video.object.palette[0].raw = data;
        break;
    case GB_VIDEO_PALETTE_OBJECT_1:
        video.object.palette[1].raw = data;
        break;
    case GB_VIDEO_RAM_BEGIN ... GB_VIDEO_RAM_END:
        if (!video.control.enabled || (video.status.mode < GB_MODE_TRANSFER)) {
            video.ram[address - GB_VIDEO_RAM_BEGIN] = data;
        }
        break;
    case GB_VIDEO_RAM_OBJECT_BEGIN ... GB_VIDEO_RAM_OBJECT_END:
        if (!video.control.enabled || (video.status.mode < GB_MODE_SEARCH)) {
            ((uint8_t *)video.object.ram)[address - GB_VIDEO_RAM_OBJECT_BEGIN] = data;
        }
        break;
    case GB_VIDEO_SCROLL_X:
        video.scroll.x = data;
        break;
    case GB_VIDEO_SCROLL_Y:
        video.scroll.y = data;
        break;
    case GB_VIDEO_STATUS:
        video.status.raw = (data & 0x78) | (video.status.raw & 0x07) | 0x80;
        if (video.control.enabled) {
            gb_video_status_update();
        }
        break;
    case GB_VIDEO_TRANSFER:
        video.transfer.address = data;
        video.transfer.delay = 3;
        video.transfer.destination = GB_VIDEO_RAM_OBJECT_BEGIN;
        video.transfer.source = video.transfer.address << 8;
        break;
    case GB_VIDEO_WINDOW_X:
        video.window.x = data;
        break;
    case GB_VIDEO_WINDOW_Y:
        video.window.y = data;
        break;
    default:
        break;
    }
}
