#include "KeyStatesSDL.h"

#include <stdint.h>
#include <stdbool.h>
#include <memory.h>

#include <SDL3/SDL.h>

void KeyStatesSDL_ClearStates(KeyStatesSDL *key_states)
{
    if (key_states == NULL)
        return;

    memset((void *)key_states->key_states, 0, ((size_t)SDL_SCANCODE_COUNT) / 8);
}

void KeyStatesSDL_UpdateState(KeyStatesSDL *key_states, SDL_Scancode scancode, bool is_down)
{
    if (key_states == NULL)
        return;

    if ((int)scancode >= (int)SDL_SCANCODE_COUNT)
        return;

    int scancode_int = (int)scancode;

    int byte_to_change = scancode_int >> 3;
    int bit_to_change = scancode_int & 0x07;

    uint8_t bit_mask = 0x01 << bit_to_change;

    if (is_down == true)
        key_states->key_states[byte_to_change] |= bit_mask;
    else
        key_states->key_states[byte_to_change] &= ~bit_mask;
}

bool KeyStatesSDL_IsKeyDown(KeyStatesSDL *key_states, SDL_Scancode scancode)
{
    if (key_states == NULL)
        return false;

    int scancode_int = (int)scancode;

    int byte_to_read = scancode_int >> 3;
    int bit_to_read = scancode_int & 0x07;

    uint8_t bit_mask = 0x01 << bit_to_read;

    return ((key_states->key_states[byte_to_read] & bit_mask) != 0);
}

