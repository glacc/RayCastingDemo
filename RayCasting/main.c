#include <stdio.h>
#include <math.h>

#include <SDL3/SDL.h>

#include "RayCastEngine.h"

int main(int argc, char *argv[])
{
    float time_ms = 0.0F;
    const float ms_per_tick = 1000.0F / 60.0F;

    if (RayCast_Initialize())
    {
        Uint64 last_tick = SDL_GetTicks();

        while (true)
        {
            Uint64 current_tick = SDL_GetTicks();

            time_ms += (float)(current_tick - last_tick);

            last_tick = current_tick;

            if (time_ms >= ms_per_tick)
            {
                if (!RayCast_Tick())
                    break;

                time_ms = fmodf(time_ms, ms_per_tick);
            }

            SDL_Delay(1);
        }
    }

    RayCast_Deinitialize();

    SDL_Quit();

    return 0;
}
