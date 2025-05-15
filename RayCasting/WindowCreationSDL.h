#pragma once

#include <stdint.h>

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

    extern SDL_Window *WindowCreationSDL_CreateWindowCentered(const char *title, uint32_t window_width, uint32_t window_height, SDL_WindowFlags flags);

#ifdef __cplusplus
}
#endif
