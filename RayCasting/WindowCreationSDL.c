#include "WindowCreationSDL.h"

#include <stdint.h>
#include <stdbool.h>

#include <SDL3/SDL.h>

static const char program_log_tag[] = "[WindowCreationSDL.c]";

SDL_Window *WindowCreationSDL_CreateWindowCentered(const char *title, uint32_t window_width, uint32_t window_height, SDL_WindowFlags flags)
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("%s Error: Failed to initialize video subsystem: %s", program_log_tag, SDL_GetError());
        return NULL;
    }

    bool window_center_available = false;
    int window_x = 0;
    int window_y = 0;

    int num_of_displays;
    SDL_DisplayID *displays = SDL_GetDisplays(&num_of_displays);

    if (displays != NULL)
    {
        SDL_Log("%s Number of display(s): %d", program_log_tag, num_of_displays);

        if (num_of_displays > 0)
        {
            const int display_index = 0;

            const SDL_DisplayMode *display_mode = SDL_GetCurrentDisplayMode(displays[(int)display_index]);
            if (display_mode != NULL)
            {
                window_x = (display_mode->w - (int)window_width) / 2;
                window_y = (display_mode->h - (int)window_height) / 2;

                window_center_available = true;

                // SDL_free((void *)display_mode);
            }
            else
                SDL_Log("%s Error: Failed to get display mode of display %d: %s", program_log_tag, (int)display_index, SDL_GetError());
        }
        else
            SDL_Log("%s Error: No available display found", program_log_tag);

        SDL_free((void *)displays);
    }
    else
        SDL_Log("%s Error: Failed to get displays: %s", program_log_tag, SDL_GetError());

    SDL_Window *window = SDL_CreateWindow(title, (int)window_width, (int)window_height, flags);
    if (window == NULL)
    {
        SDL_Log("%s Error: Failed to create window: %s", program_log_tag, SDL_GetError());
        return NULL;
    }

    if (window_center_available)
        SDL_SetWindowPosition(window, window_x, window_y);

    return window;
}
