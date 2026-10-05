#ifndef XMODEL_TYPES_H
#define XMODEL_TYPES_H

#include <types.h>
#include <rwcore.h>
#include "xVec3.h"

struct RpAtomic;
struct xAnimPlay;
struct xSurface;
struct xLightKit;
struct xModelBucket;
struct xModelInstance;

struct xModelPool
{
    xModelPool* Next;
    U32 NumMatrices;
    xModelInstance* List;
};

struct xModelInstance
{
    xModelInstance* Next;
    xModelInstance* Parent;
    xModelPool* Pool;
    xAnimPlay* Anim; // 0xC

    // Offset: 0x10
    RpAtomic* Data;
    U32 PipeFlags;
    F32 RedMultiplier;
    F32 GreenMultiplier;

    // Offset: 0x20
    F32 BlueMultiplier;
    F32 Alpha;
    F32 FadeStart;
    F32 FadeEnd;

    // Offset: 0x30
    xSurface* Surf;
    xModelBucket** Bucket;
    xModelInstance* BucketNext;
    xLightKit* LightKit;

    // Offset: 0x40
    void* Object;
    U16 Flags; // 0x44
    U8 BoneCount; // 0x46
    U8 BoneIndex; // 0x47
    U8* BoneRemap; // 0x48
    RwMatrix* Mat; // 0x4C

    // Offset: 0x50
    xVec3 Scale;
    U32 modelID;
    U32 shadowID;
    RpAtomic* shadowmapAtomic;
    struct
    {
        xVec3* verts;
    } anim_coll;
};

#endif
