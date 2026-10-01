#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"
#include "rwsdk/plugin/skin2/skin.h"

#define rwID_STRUCT 0x1
#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define rpSKINGCNMINVERSION 0x34002

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwID_SKINPLUGIN;                                                   \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_BADVERSION 0x80000004

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))

#define SKINALIGN4(_ptr) ((RwUInt8*)(((RwUInt32)(_ptr) + 3) & ~3))

/*
 * Hand the instanced vertex buffers back to the resource entry so the
 * original (unskinned) vertices are restored before streaming.
 */
#define SkinRestoreVertexBuffers(_geometry, _skin)                                                 \
    MACRO_START                                                                                    \
    {                                                                                              \
        if ((_skin)->platformData.vertices)                                                        \
        {                                                                                          \
            RxGameCubeVertexBuffer* _vbHeader =                                                    \
                (RxGameCubeVertexBuffer*)((_geometry)->repEntry + 1);                              \
                                                                                                   \
            (((RwResEntry**)_vbHeader->attr[0].array)[-1])->ownerRef = (RwResEntry**)NULL;         \
            _vbHeader->attr[0].array = (_skin)->platformData.vertices;                             \
                                                                                                   \
            if ((_geometry)->flags & rpGEOMETRYNORMALS)                                            \
            {                                                                                      \
                _vbHeader->attr[1].array = (_skin)->platformData.normals;                          \
            }                                                                                      \
                                                                                                   \
            (_geometry)->repEntry->destroyNotify = _rxGCResEntryWaitDone;                          \
                                                                                                   \
            (_skin)->platformData.vertices = NULL;                                                 \
            (_skin)->platformData.normals = NULL;                                                  \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

RwInt32 _rpSkinGeometryNativeSize(const RpGeometry* geometry)
{
    RwInt32 size;
    RpSkin* skin = *RPSKINGEOMETRYGETDATA(geometry);

    size = rwCHUNKHEADERSIZE + sizeof(RwPlatformID) + sizeof(RwUInt32) +
           skin->boneData.numUsedBones;

    if (skin->vertexMaps.maxWeights > 1)
    {
        size += geometry->numVertices * (2 * skin->vertexMaps.maxWeights);
    }

    size += skin->boneData.numBones * sizeof(RwMatrix);

    SkinRestoreVertexBuffers(geometry, skin);

    size += _rpSkinSplitDataStreamGetSize(skin);

    return size;
}

RwStream* _rpSkinGeometryNativeWrite(RwStream* stream, const RpGeometry* geometry)
{
    RwPlatformID id = rwID_GAMECUBE;
    RwUInt32 skinInfo;
    RwInt32 numVertices;
    RwInt32 size;
    RwInt32 maxWeights;
    const RpSkin* skin;

    size = _rpSkinGeometryNativeSize(geometry);

    if (!_rwStreamWriteVersionedChunkHeader(stream, rwID_STRUCT, size - rwCHUNKHEADERSIZE,
                                            rwLIBRARYCURRENTVERSION, 0xFFFF))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWriteInt32(stream, (RwInt32*)&id, sizeof(RwPlatformID)))
    {
        return (RwStream*)NULL;
    }

    numVertices = geometry->numVertices;
    skin = *RPSKINGEOMETRYGETDATA(geometry);
    maxWeights = skin->vertexMaps.maxWeights;

    skinInfo = ((maxWeights & 0xFF) << 16) | ((skin->boneData.numUsedBones & 0xFF) << 8) |
               (skin->boneData.numBones & 0xFF);

    if (!RwStreamWriteInt32(stream, (RwInt32*)&skinInfo, sizeof(RwUInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWrite(stream, skin->boneData.usedBoneList, skin->boneData.numUsedBones))
    {
        return (RwStream*)NULL;
    }

    if (skin->vertexMaps.maxWeights > 1)
    {
        size = maxWeights * numVertices;

        if (!RwStreamWrite(stream, skin->platformData.indices, size))
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamWrite(stream, skin->platformData.weights, size))
        {
            return (RwStream*)NULL;
        }
    }

    if (!RwStreamWrite(stream, skin->boneData.invBoneToSkinMat,
                       skin->boneData.numBones * sizeof(RwMatrix)))
    {
        return (RwStream*)NULL;
    }

    if (!_rpSkinSplitDataStreamWrite(stream, skin))
    {
        return (RwStream*)NULL;
    }

    return stream;
}

RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry)
{
    RwUInt32 version;
    RwUInt32 size;
    RwUInt32 skinInfo;
    RwInt32 numVertices;
    RwPlatformID id;
    RpSkin* skin;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return (RwStream*)NULL;
    }

    if (version < rwLIBRARYBASEVERSION || version > rwLIBRARYCURRENTVERSION)
    {
        RWERROR((E_RW_BADVERSION));
        return (RwStream*)NULL;
    }

    if (version < rpSKINGCNMINVERSION)
    {
        RWERROR((E_RW_BADVERSION));
        return (RwStream*)NULL;
    }

    if (!RwStreamReadInt32(stream, (RwInt32*)&id, sizeof(RwPlatformID)))
    {
        return (RwStream*)NULL;
    }

    if (id != rwID_GAMECUBE)
    {
        return (RwStream*)NULL;
    }

    skin = (RpSkin*)RwFreeListAlloc(_rpSkinGlobals.freeList);
    memset(skin, 0, sizeof(RpSkin));

    if (!RwStreamReadInt32(stream, (RwInt32*)&skinInfo, sizeof(RwUInt32)))
    {
        return (RwStream*)NULL;
    }

    skin->boneData.numBones = skinInfo & 0xFF;
    skin->boneData.numUsedBones = (skinInfo >> 8) & 0xFF;
    skin->vertexMaps.maxWeights = (skinInfo >> 16) & 0xFF;

    numVertices = geometry->numVertices;

    size -= sizeof(RwPlatformID) + sizeof(RwUInt32);

    if (skin->vertexMaps.maxWeights > 1)
    {
        skin->platformData.weights = (RwUInt8*)RwMalloc(size + 5);

        skin->platformData.indices =
            skin->platformData.weights + skin->vertexMaps.maxWeights * numVertices;
        skin->platformData.indices = SKINALIGN4(skin->platformData.indices);

        skin->boneData.invBoneToSkinMat =
            (RwMatrix*)(skin->platformData.indices + skin->vertexMaps.maxWeights * numVertices);
        skin->boneData.invBoneToSkinMat = (RwMatrix*)SKINALIGN4(skin->boneData.invBoneToSkinMat);

        skin->boneData.usedBoneList =
            (RwUInt8*)(skin->boneData.invBoneToSkinMat + skin->boneData.numBones);

        size = skin->boneData.numUsedBones;
        if (RwStreamRead(stream, skin->boneData.usedBoneList, size) != size)
        {
            return (RwStream*)NULL;
        }

        size = skin->vertexMaps.maxWeights * numVertices;
        if (RwStreamRead(stream, skin->platformData.indices, size) != size)
        {
            return (RwStream*)NULL;
        }

        size = skin->vertexMaps.maxWeights * numVertices;
        if (RwStreamRead(stream, skin->platformData.weights, size) != size)
        {
            return (RwStream*)NULL;
        }

        size = skin->boneData.numBones * sizeof(RwMatrix);
        if (RwStreamRead(stream, skin->boneData.invBoneToSkinMat, size) != size)
        {
            return (RwStream*)NULL;
        }
    }
    else
    {
        skin->platformData.weights = (RwUInt8*)RwMalloc(size + 3);

        skin->boneData.invBoneToSkinMat = (RwMatrix*)skin->platformData.weights;
        skin->boneData.invBoneToSkinMat = (RwMatrix*)SKINALIGN4(skin->boneData.invBoneToSkinMat);

        skin->boneData.usedBoneList =
            (RwUInt8*)(skin->boneData.invBoneToSkinMat + skin->boneData.numBones);

        size = skin->boneData.numUsedBones;
        if (RwStreamRead(stream, skin->boneData.usedBoneList, size) != size)
        {
            return (RwStream*)NULL;
        }

        size = skin->boneData.numBones * sizeof(RwMatrix);
        if (RwStreamRead(stream, skin->boneData.invBoneToSkinMat, size) != size)
        {
            return (RwStream*)NULL;
        }
    }

    if (!_rpSkinSplitDataStreamRead(stream, skin))
    {
        return (RwStream*)NULL;
    }

    RpSkinGeometrySetSkin(geometry, skin);

    return stream;
}
