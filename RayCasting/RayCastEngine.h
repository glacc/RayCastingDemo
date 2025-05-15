#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    extern bool RayCast_Initialize(void);
    extern void RayCast_Deinitialize(void);

    extern bool RayCast_Tick(void);

#ifdef __cplusplus
}
#endif
