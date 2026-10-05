#ifndef ZVOLUME_H
#define ZVOLUME_H

#include "xVolume.h"

struct zVolume : xVolume
{
};

#if defined(PS2)
extern S32 gOccludeCount;
#else
extern volatile S32 gOccludeCount;
#endif

void zVolumeInit();
void zVolumeSetup();
zVolume* zVolumeGetVolume(U16 n);
void zVolume_OccludePrecalc(xVec3* camPos);
S32 zVolumeEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*);

#endif
