#include "xFog.h"
#include "xEvent.h"

#include "iCamera.h"

#include <types.h>

void xFogClearFog()
{
    iCameraSetFogParams(NULL, 0.0f);
}

void xFogInit(void* b, void* tasset)
{
    xFogInit((xBase*)b, (xFogAsset*)tasset);
}

#if defined(PS2)
inline
#endif
void xFogInit(xBase* ent, xFogAsset* tasset)
{
    xBaseInit(ent, (xBaseAsset*)tasset);
    ent->eventFunc = xFogEventCB;
    _xFog* fog = (_xFog*)ent;
    fog->tasset = tasset;
    if (ent->linkCount != 0)
    {
        ent->link = (xLinkAsset*)(fog->tasset + 1);
    }
    else
    {
        ent->link = NULL;
    }
}

#if defined(PS2)
inline
#endif
void xFogReset(_xFog* ent)
{
    xBaseReset((xBase*)ent, (xBaseAsset*)ent->tasset);
}

void xFogSave(_xFog* ent, xSerial* s)
{
    xBaseSave((xBase*)ent, s);
}

void xFogLoad(_xFog* ent, xSerial* s)
{
    xBaseLoad((xBase*)ent, s);
}

S32 xFogEventCB(xBase* to, xBase* from, U32 toEvent, const F32* toParam, xBase* b3)
{
    switch (toEvent)
    {
    case eEventOn:
    {
        iFogParams fog;
        fog.type = rwFOGTYPELINEAR;
        _xFog* t = (_xFog*)from;
        fog.start = t->tasset->fogStart;
        fog.stop = t->tasset->fogStop;
        fog.density = t->tasset->fogDensity;
        fog.fogcolor.red = t->tasset->fogColor[0];
        fog.fogcolor.green = t->tasset->fogColor[1];
        fog.fogcolor.blue = t->tasset->fogColor[2];
        fog.fogcolor.alpha = t->tasset->fogColor[3];
        fog.bgcolor.red = t->tasset->bkgndColor[0];
        fog.bgcolor.green = t->tasset->bkgndColor[1];
        fog.bgcolor.blue = t->tasset->bkgndColor[2];
        fog.bgcolor.alpha = t->tasset->bkgndColor[3];
        fog.table = NULL;
        iCameraSetFogParams(&fog, t->tasset->transitionTime);
        break;
    }
    case eEventOff:
        iCameraSetFogParams(NULL, 0.0f);
        break;
    case eEventReset:
        xFogReset((_xFog*)from);
        break;
    }
    return eEventEnable;
}

void xFogUpdate(xBase* ent, xScene* sc, F32 dt)
{
}
