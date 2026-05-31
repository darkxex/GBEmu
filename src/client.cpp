/*
 * SPDX-FileCopyrightText: 2026 David Jolly <jolly.a.david@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include "client.h"
#include "audio.h"
#include "cartridge.h"
#include "input.h"
#include "video.h"
#include <SDL3/SDL.h>
#include <string.h>

static const SDL_GamepadButton BUTTON[GB_BUTTON_MAX] = { SDL_GAMEPAD_BUTTON_EAST,       SDL_GAMEPAD_BUTTON_SOUTH,
                                                         SDL_GAMEPAD_BUTTON_BACK,       SDL_GAMEPAD_BUTTON_START,
                                                         SDL_GAMEPAD_BUTTON_DPAD_RIGHT, SDL_GAMEPAD_BUTTON_DPAD_LEFT,
                                                         SDL_GAMEPAD_BUTTON_DPAD_UP,    SDL_GAMEPAD_BUTTON_DPAD_DOWN };

static const SDL_Scancode KEY[GB_BUTTON_MAX] = { SDL_SCANCODE_X,     SDL_SCANCODE_Z,    SDL_SCANCODE_SPACE,  SDL_SCANCODE_RETURN,
                                                 SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT, SDL_SCANCODE_UP, SDL_SCANCODE_DOWN };

static const uint16_t PALETTE[GB_COLOR_MAX] = { 0x6F7B, 0x4E73, 0x318C, 0x1084 };

static struct {
    bool fullscreen;
    SDL_Cursor *cursor;
    SDL_Gamepad *gamepad;
    SDL_Renderer *renderer;
    SDL_AudioStream *stream;
    SDL_Texture *texture;
    SDL_Window *window;
    struct {
        uint64_t begin;
        float remaining;
    } frame;
} client = {};

static gb_error_e gb_client_audio_create(void) {
    SDL_AudioSpec specification{};
    specification.freq = 32768;
    specification.format = SDL_AUDIO_S16LE;
    specification.channels = 1;
    if (!(client.stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &specification, nullptr, nullptr))) {
        return GB_ERROR("SDL_OpenAudioDeviceStream failed: %s", SDL_GetError());
    }
    if (!SDL_ResumeAudioDevice(SDL_GetAudioStreamDevice(client.stream))) {
        return GB_ERROR("SDL_ResumeAudioDevice failed: %s", SDL_GetError());
    }
    return GB_SUCCESS;
}

static void gb_client_audio_destroy(void) {
    if (client.stream) {
        SDL_PauseAudioDevice(SDL_GetAudioStreamDevice(client.stream));
        SDL_CloseAudioDevice(SDL_GetAudioStreamDevice(client.stream));
        SDL_DestroyAudioStream(client.stream);
    }
}

static gb_error_e gb_client_audio_sync(void) {
    uint32_t length = 0;
    const int16_t *samples = (*gb_audio_sample(&length));
    if (!SDL_PutAudioStreamData(client.stream, samples, length * sizeof(*samples))) {
        return GB_ERROR("SDL_PutAudioStreamData failed: %s", SDL_GetError());
    }
    return GB_SUCCESS;
}

static const gb_color_t (*gb_client_color(void)) [GB_VIDEO_HEIGHT][GB_VIDEO_WIDTH] {
    static gb_color_t colors[GB_VIDEO_HEIGHT][GB_VIDEO_WIDTH] = {};
    for (uint8_t y = 0; y < GB_VIDEO_HEIGHT; ++y) {
        for (uint8_t x = 0; x < GB_VIDEO_WIDTH; ++x) {
            colors[y][x].raw = PALETTE[(*gb_video_color())[y][x]];
        }
    }
    return &colors;
}

static void gb_client_frame_begin(void) {
    client.frame.begin = SDL_GetPerformanceCounter();
}

static float gb_client_frame_elapsed(void) {
    return ((SDL_GetPerformanceCounter() - client.frame.begin) / (float)SDL_GetPerformanceFrequency()) * 1000.f;
}

static void gb_client_frame_end(void) {
    while ((gb_client_frame_elapsed() + client.frame.remaining) < (1000.f / GB_CLIENT_FRAME_RATE)) {
        SDL_Delay(1);
    }
    if (gb_client_frame_elapsed() < (1000.f / GB_CLIENT_FRAME_RATE)) {
        client.frame.remaining -= (1000.f / GB_CLIENT_FRAME_RATE) - gb_client_frame_elapsed();
    } else {
        client.frame.remaining += gb_client_frame_elapsed() - (1000.f / GB_CLIENT_FRAME_RATE);
    }
}

static void gb_client_gamepad_create(const SDL_GamepadDeviceEvent *const device) {
    if (!client.gamepad) {
        client.gamepad = SDL_OpenGamepad(device->which);
    }
}

static void gb_client_gamepad_destroy(const SDL_GamepadDeviceEvent *const device) {
    if (client.gamepad && (!device || (device->which == SDL_GetJoystickID(SDL_GetGamepadJoystick(client.gamepad))))) {
        SDL_CloseGamepad(client.gamepad);
        client.gamepad = nullptr;
    }
}

static void gb_client_gamepad_detect(void) {
    if (!client.gamepad) {
        int32_t count = 0;
        SDL_JoystickID *joystick = SDL_GetJoysticks(&count);
        if (joystick) {
            for (int32_t id = 0; id < count; ++id) {
                if (SDL_IsGamepad(joystick[id])) {
                    client.gamepad = SDL_OpenGamepad(joystick[id]);
                    break;
                }
            }
            SDL_free(joystick);
        }
    }
}

static void gb_client_gamepad_sync(const SDL_GamepadButtonEvent *const button) {
    if (client.gamepad && (button->which == SDL_GetJoystickID(SDL_GetGamepadJoystick(client.gamepad)))) {
        for (uint8_t index = 0; index < GB_BUTTON_MAX; ++index) {
            if (button->button == BUTTON[index]) {
                (*gb_input_button())[index] = button->down;
                break;
            }
        }
    }
}

static void gb_client_input_clear(void) {
    for (uint8_t index = 0; index < GB_BUTTON_MAX; ++index) {
        (*gb_input_button())[index] = false;
    }
}

static void gb_client_video_fullscreen_toggle(void) {
    client.fullscreen = !client.fullscreen;
    if (client.fullscreen) {
        SDL_HideCursor();
        SDL_SetWindowFullscreen(client.window, true);
    } else {
        SDL_ShowCursor();
        SDL_SetWindowFullscreen(client.window, false);
    }
}

static void gb_client_keyboard_sync(const SDL_KeyboardEvent *const key) {
    if (!key->repeat) {
        if (key->down && (key->scancode == SDL_SCANCODE_RETURN) && (key->mod & SDL_KMOD_ALT)) {
            gb_client_video_fullscreen_toggle();
            return;
        }
        for (uint8_t index = 0; index < GB_BUTTON_MAX; ++index) {
            if (key->scancode == KEY[index]) {
                (*gb_input_button())[index] = key->down;
                break;
            }
        }
    }
}

static gb_error_e gb_client_video_create(const gb_option_t *const option) {
    if ((option->scale < GB_CLIENT_SCALE_MIN) || (option->scale > GB_CLIENT_SCALE_MAX)) {
        return GB_ERROR("Unsupported window scale: %u", option->scale);
    }
    client.fullscreen = option->fullscreen;
    if (!SDL_CreateWindowAndRenderer(gb_cartridge_title(), GB_VIDEO_WIDTH * option->scale, GB_VIDEO_HEIGHT * option->scale,
                                     SDL_WINDOW_RESIZABLE, &client.window, &client.renderer)) {
        return GB_ERROR("SDL_CreateWindowAndRenderer failed: %s", SDL_GetError());
    }
    if (!SDL_SetRenderLogicalPresentation(client.renderer, GB_VIDEO_WIDTH, GB_VIDEO_HEIGHT, SDL_LOGICAL_PRESENTATION_LETTERBOX)) {
        return GB_ERROR("SDL_SetRenderLogicalPresentation failed: %s", SDL_GetError());
    }
    if (!SDL_SetRenderDrawColor(client.renderer, 0, 0, 0, 0)) {
        return GB_ERROR("SDL_SetRenderDrawColor failed: %s", SDL_GetError());
    }
    if (!SDL_SetRenderVSync(client.renderer, GB_CLIENT_VSYNC ? 1 : 0)) {
        return GB_ERROR("SDL_SetRenderVSync failed: %s", SDL_GetError());
    }
    if (!(client.texture =
              SDL_CreateTexture(client.renderer, SDL_PIXELFORMAT_XBGR1555, SDL_TEXTUREACCESS_STREAMING, GB_VIDEO_WIDTH, GB_VIDEO_HEIGHT))) {
        return GB_ERROR("SDL_CreateTexture failed: %s", SDL_GetError());
    }
    if (!SDL_SetTextureScaleMode(client.texture, SDL_SCALEMODE_NEAREST)) {
        return GB_ERROR("SDL_SetTextureScaleMode failed: %s", SDL_GetError());
    }
    if (!(client.cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR))) {
        return GB_ERROR("SDL_CreateSystemCursor failed: %s", SDL_GetError());
    }
    if (!SDL_SetCursor(client.cursor)) {
        return GB_ERROR("SDL_SetCursor failed: %s", SDL_GetError());
    }
    if (client.fullscreen) {
        if (!SDL_HideCursor()) {
            return GB_ERROR("SDL_HideCursor failed: %s", SDL_GetError());
        }
        if (!SDL_SetWindowFullscreen(client.window, true)) {
            return GB_ERROR("SDL_SetWindowFullscreen failed: %s", SDL_GetError());
        }
    }
    return GB_SUCCESS;
}

static void gb_client_video_destroy(void) {
    if (client.cursor) {
        SDL_DestroyCursor(client.cursor);
    }
    if (client.texture) {
        SDL_DestroyTexture(client.texture);
    }
    if (client.renderer) {
        SDL_DestroyRenderer(client.renderer);
    }
    if (client.window) {
        SDL_DestroyWindow(client.window);
    }
}

static gb_error_e gb_client_video_sync(void) {
    if (!SDL_UpdateTexture(client.texture, nullptr, gb_client_color(), GB_VIDEO_WIDTH * sizeof(uint16_t))) {
        return GB_ERROR("SDL_UpdateTexture failed: %s", SDL_GetError());
    }
    if (!SDL_RenderClear(client.renderer)) {
        return GB_ERROR("SDL_RenderClear failed: %s", SDL_GetError());
    }
    if (!SDL_RenderTexture(client.renderer, client.texture, nullptr, nullptr)) {
        return GB_ERROR("SDL_RenderTexture failed: %s", SDL_GetError());
    }
    if (!SDL_RenderPresent(client.renderer)) {
        return GB_ERROR("SDL_RenderPresent failed: %s", SDL_GetError());
    }
    return GB_SUCCESS;
}

gb_error_e gb_client_create(const gb_option_t *const option) {
    gb_error_e result = GB_SUCCESS;
    memset(&client, 0, sizeof(client));
    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_VIDEO)) {
        return GB_ERROR("SDL_Init failed: %s", SDL_GetError());
    }
    if (((result = gb_client_video_create(option)) == GB_SUCCESS) && ((result = gb_client_audio_create()) == GB_SUCCESS)) {
        gb_client_gamepad_detect();
    }
    return result;
}

void gb_client_destroy(void) {
    gb_client_gamepad_destroy(nullptr);
    gb_client_audio_destroy();
    gb_client_video_destroy();
    SDL_Quit();
}

bool gb_client_poll(void) {
    SDL_Event event = {};
    gb_client_frame_begin();
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            gb_client_gamepad_sync(&event.gbutton);
            break;
        case SDL_EVENT_GAMEPAD_ADDED:
            gb_client_gamepad_create(&event.gdevice);
            break;
        case SDL_EVENT_GAMEPAD_REMOVED:
            gb_client_gamepad_destroy(&event.gdevice);
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            gb_client_keyboard_sync(&event.key);
            break;
        case SDL_EVENT_QUIT:
            return false;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
        case SDL_EVENT_WINDOW_MINIMIZED:
            gb_client_input_clear();
            break;
        default:
            break;
        }
    }
    return true;
}

gb_error_e gb_client_sync(void) {
    gb_error_e result = GB_SUCCESS;
    if (((result = gb_client_video_sync()) == GB_SUCCESS) && ((result = gb_client_audio_sync()) == GB_SUCCESS)) {
        gb_client_frame_end();
    }
    return result;
}
