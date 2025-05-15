#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <SDL3/SDL.h>

typedef struct
{
    uint8_t key_states[((int)SDL_SCANCODE_COUNT) / 8];
}
KeyStatesSDL;

#ifdef __cplusplus
extern "C" {
#endif

    extern void KeyStatesSDL_ClearStates(KeyStatesSDL *key_states);

    extern void KeyStatesSDL_UpdateState(KeyStatesSDL *key_states, SDL_Scancode scancode, bool is_down);

    extern bool KeyStatesSDL_IsKeyDown(KeyStatesSDL *key_states, SDL_Scancode scancode);

#ifdef __cplusplus
}
#endif
