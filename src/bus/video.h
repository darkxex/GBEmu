/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#ifndef GB_VIDEO_H_
#define GB_VIDEO_H_

#include "bus.h"

#define GB_VIDEO_CONTROL 0xFF40
#define GB_VIDEO_HEIGHT 144
#define GB_VIDEO_LINE_Y 0xFF44
#define GB_VIDEO_LINE_Y_COINCIDENCE 0xFF45
#define GB_VIDEO_PALETTE_BACKGROUND 0xFF47
#define GB_VIDEO_PALETTE_OBJECT_0 0xFF48
#define GB_VIDEO_PALETTE_OBJECT_1 0xFF49
#define GB_VIDEO_RAM_BEGIN 0x8000
#define GB_VIDEO_RAM_END 0x9FFF
#define GB_VIDEO_RAM_OBJECT_BEGIN 0xFE00
#define GB_VIDEO_RAM_OBJECT_END 0xFE9F
#define GB_VIDEO_SCROLL_X 0xFF43
#define GB_VIDEO_SCROLL_Y 0xFF42
#define GB_VIDEO_STATUS 0xFF41
#define GB_VIDEO_TRANSFER 0xFF46
#define GB_VIDEO_WIDTH 160
#define GB_VIDEO_WINDOW_X 0xFF4B
#define GB_VIDEO_WINDOW_Y 0xFF4A

#define GB_VIDEO_RAM_OBJECT_WIDTH (GB_WIDTH(GB_VIDEO_RAM_OBJECT_BEGIN, GB_VIDEO_RAM_OBJECT_END) / sizeof(gb_object_t))
#define GB_VIDEO_RAM_WIDTH GB_WIDTH(GB_VIDEO_RAM_BEGIN, GB_VIDEO_RAM_END)

typedef enum : uint8_t {
    GB_COLOR_WHITE = 0,
    GB_COLOR_GREY_LIGHT,
    GB_COLOR_GREY_DARK,
    GB_COLOR_BLACK,
    GB_COLOR_MAX
} gb_color_e;

typedef enum : uint8_t {
    GB_MODE_HBLANK = 0,
    GB_MODE_VBLANK,
    GB_MODE_SEARCH,
    GB_MODE_TRANSFER,
    GB_MODE_MAX,
} gb_mode_e;

typedef struct {
    uint8_t y;
    uint8_t x;
    uint8_t index;
    struct {
        uint8_t : 4;
        uint8_t palette : 1;
        uint8_t flip_x : 1;
        uint8_t flip_y : 1;
        uint8_t priority : 1;
    } attribute;
} gb_object_t;

typedef struct {
    uint8_t index;
    const gb_object_t *object;
} gb_object_entry_t;

typedef union {
    struct {
        uint8_t white : 2;
        uint8_t grey_light : 2;
        uint8_t grey_dark : 2;
        uint8_t black : 2;
    };
    uint8_t raw;
} gb_palette_t;

const gb_color_e (*gb_video_color(void))[GB_VIDEO_HEIGHT][GB_VIDEO_WIDTH];
uint8_t gb_video_read(uint16_t address);
void gb_video_reset(void);
gb_error_e gb_video_step(void);
void gb_video_write(uint16_t address, uint8_t data);

#endif /* GB_VIDEO_H_ */
