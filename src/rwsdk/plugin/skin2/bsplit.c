#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <string.h>

#include "rwsdk/plugin/skin2/skin.h"

RwBool _rpSkinSplitDataDestroy(RpSkin* skin)
{
    SkinSplitData* skinSplitData = &skin->skinSplitData;

    if (skinSplitData->matrixRemapIndices)
    {
        RwFree(skinSplitData->matrixRemapIndices);
    }

    skinSplitData->boneLimit = 0;
    skinSplitData->numMeshes = 0;
    skinSplitData->numRLE = 0;
    skinSplitData->matrixRemapIndices = (RwUInt8*)NULL;
    skinSplitData->meshRLECount = (RwUInt8*)NULL;
    skinSplitData->meshRLE = (RwUInt8*)NULL;

    return TRUE;
}

RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, RwUInt32 boneLimit, RwUInt32 numMatrices,
                               RwUInt32 numMeshes, RwUInt32 numRLE)
{
    SkinSplitData* skinSplitData = &skin->skinSplitData;
    RwUInt32 size;

    _rpSkinSplitDataDestroy(skin);

    size = numMatrices * sizeof(RwUInt8) + numMeshes * sizeof(RwUInt16) + numRLE * sizeof(RwUInt16);

    skinSplitData->matrixRemapIndices = (RwUInt8*)RwMalloc(size);
    if (!skinSplitData->matrixRemapIndices)
    {
        return (RpSkin*)NULL;
    }

    memset(skinSplitData->matrixRemapIndices, 0, size);

    skinSplitData->boneLimit = boneLimit;
    skinSplitData->numMeshes = numMeshes;
    skinSplitData->numRLE = numRLE;
    skinSplitData->meshRLECount = skinSplitData->matrixRemapIndices + numMatrices;
    skinSplitData->meshRLE = skinSplitData->meshRLECount + numMeshes * 2;

    return skin;
}

RwStream* _rpSkinSplitDataStreamWrite(RwStream* stream, const RpSkin* skin)
{
    const SkinSplitData* skinSplitData = &skin->skinSplitData;

    if (!RwStreamWriteInt32(stream, (RwInt32*)&skinSplitData->boneLimit, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWriteInt32(stream, (RwInt32*)&skinSplitData->numMeshes, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWriteInt32(stream, (RwInt32*)&skinSplitData->numRLE, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (skin->skinSplitData.numMeshes)
    {
        if (!RwStreamWrite(stream, skinSplitData->matrixRemapIndices,
                           skin->boneData.numBones + skinSplitData->numMeshes * 2 +
                               skinSplitData->numRLE * 2))
        {
            return (RwStream*)NULL;
        }
    }

    return stream;
}

RwStream* _rpSkinSplitDataStreamRead(RwStream* stream, RpSkin* skin)
{
    RwInt32 numMeshes;
    RwInt32 numRLE;
    RwInt32 boneLimit;
    SkinSplitData* skinSplitData = &skin->skinSplitData;

    if (!RwStreamReadInt32(stream, &boneLimit, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamReadInt32(stream, &numMeshes, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamReadInt32(stream, &numRLE, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (numMeshes > 0)
    {
        if (!_rpSkinSplitDataCreate(skin, boneLimit, skin->boneData.numBones, numMeshes, numRLE))
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamRead(stream, skinSplitData->matrixRemapIndices,
                          skin->boneData.numBones + skinSplitData->numMeshes * 2 +
                              skinSplitData->numRLE * 2))
        {
            RwFree(skinSplitData);
            return (RwStream*)NULL;
        }
    }

    return stream;
}

RwInt32 _rpSkinSplitDataStreamGetSize(const RpSkin* skin)
{
    RwUInt32 streamSize = 3 * sizeof(RwInt32);

    if (skin->skinSplitData.numMeshes)
    {
        streamSize += skin->boneData.numBones + skin->skinSplitData.numMeshes * 2 +
                      skin->skinSplitData.numRLE * 2;
    }

    return streamSize;
}
