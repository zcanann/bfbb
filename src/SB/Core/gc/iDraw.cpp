#include "iDraw.h"

#include <dolphin.h>
#include <stddef.h>

#if defined(VERSION_GQPP78) || defined(VERSION_GU4Y78)
#include "xMath.h"

extern GXRenderModeObj* _RwDlRenderMode;
#endif

void iDrawSetFBMSK(U32 abgr)
{
    size_t tmp, hi;

    hi = abgr >> 24;

    if (hi == 0)
    {
        GXSetAlphaUpdate(GX_TRUE);
    }
    else if (hi == 255)
    {
        GXSetAlphaUpdate(GX_FALSE);
    }

    tmp = abgr & 0x00FFFFFF;

    if (tmp == 0)
    {
        GXSetColorUpdate(GX_TRUE);
    }
    else
    {
        GXSetColorUpdate(GX_FALSE);
    }
}

#if defined(VERSION_GQPP78) || defined(VERSION_GU4Y78)
void iDrawSetDisplayOffset(F32 offsetx, F32 offsety)
{
    F32 xpos = 40.0f + 640.0f * offsetx;
    F32 ypos = 23.0f + 528.0f * offsety;

    xpos = MAX(0.0f, MIN(xpos, 80.0f));
    ypos = MAX(0.0f, MIN(ypos, 551.0f));

    _RwDlRenderMode->viXOrigin = (U16)xpos;
    _RwDlRenderMode->viYOrigin = (U16)ypos;
    VIConfigure(_RwDlRenderMode);
}
#endif

void iDrawBegin()
{
    return;
}

void iDrawEnd()
{
    return;
}
