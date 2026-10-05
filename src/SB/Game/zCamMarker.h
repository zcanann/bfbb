#ifndef ZCAMMARKER_H
#define ZCAMMARKER_H

#include <types.h>
#if defined(PS2)
#include "zCameraControl.h"
#include "xCamAsset.h"
#else
#include "zCamera.h"
#endif
#include "xBase.h"
#if !defined(PS2)
#include "xCamera.h"
#endif
#include "xEvent.h"

struct zCamMarker : xBase
{
    xCamAsset* asset;
};

void zCamMarkerInit(xBase* b, xCamAsset* asset);
void zCamMarkerSave(zCamMarker* m, xSerial* s);
void zCamMarkerLoad(zCamMarker* m, xSerial* s);
int zCamMarkerEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* b3);

#endif
