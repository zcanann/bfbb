#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <string.h>

#include "rwsdk/plugin/skin2/skin.h"

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

#define rwMatrixCopy(_target, _source) (*(_target) = *(_source))

static RwInt32 _rpSkinFreeListBlockSize = 20;
static RwInt32 _rpSkinFreeListPreallocBlocks = 1;
static RwFreeList _rpSkinFreeList;

/* Work out the maximum number of non zero weights used by any vertex */
static void SkinFindMaxWeights(RpSkin* skin, RwUInt32 numVertices, const RwMatrixWeights* weights)
{
    RwUInt32 i;
    RwUInt32 j;

    skin->vertexMaps.maxWeights = 1;

    for (i = 0; i < numVertices; i++)
    {
        const RwUInt32* vertexWeights = (const RwUInt32*)&weights[i];

        for (j = skin->vertexMaps.maxWeights; j < rpSKINMAXWEIGHTS; j++)
        {
            if (!vertexWeights[j])
            {
                break;
            }

            skin->vertexMaps.maxWeights++;

            if (skin->vertexMaps.maxWeights == rpSKINMAXWEIGHTS)
            {
                return;
            }
        }
    }
}

/* Build the list of bones that actually influence a vertex */
static void SkinFindUsedBones(RwUInt8* usedBoneList, RwUInt32* numUsedBones, const RpSkin* skin,
                              RwUInt32 numVertices, const RwUInt32* indices,
                              const RwMatrixWeights* weights)
{
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 k;

    *numUsedBones = 0;

    for (i = 0; i < numVertices; i++)
    {
        const RwUInt32* vertexWeights = (const RwUInt32*)&weights[i];

        for (j = 0; j < skin->vertexMaps.maxWeights; j++)
        {
            if (vertexWeights[j])
            {
                RwUInt8 bone = (RwUInt8)(indices[i] >> (j * 8));
                RwBool newBone = TRUE;

                for (k = 0; k < *numUsedBones; k++)
                {
                    if (bone == usedBoneList[k])
                    {
                        newBone = FALSE;
                        break;
                    }
                }

                if (newBone)
                {
                    usedBoneList[*numUsedBones] = bone;
                    (*numUsedBones)++;
                }
            }
        }
    }
}

static RwBool SkinAllocate(RpSkin* skin, RwUInt32 numVertices, RwUInt32 numBones,
                           RwUInt32 numUsedBones, const RwUInt8* usedBoneList,
                           const RwMatrixWeights* vertexWeights, const RwUInt32* vertexIndices,
                           const RwMatrix* inverseMatrices)
{
    RwUInt32 size;

    size = sizeof(RwMatrix) * numBones + numUsedBones;
    size += 15; /* alignment padding for the matrices */
    size += (sizeof(RwUInt32) + sizeof(RwMatrixWeights)) * numVertices;

    skin->unaligned = RwMalloc(size);
    if (!skin->unaligned)
    {
        return FALSE;
    }

    memset(skin->unaligned, 0, size);

    skin->boneData.numBones = numBones;
    skin->boneData.numUsedBones = numUsedBones;
    skin->boneData.usedBoneList = (RwUInt8*)skin->unaligned;
    skin->boneData.invBoneToSkinMat =
        (RwMatrix*)(((RwUInt32)skin->boneData.usedBoneList + numUsedBones + 15) & ~15);
    skin->vertexMaps.matrixIndices = (RwUInt32*)(skin->boneData.invBoneToSkinMat + numBones);
    skin->vertexMaps.matrixWeights =
        (RwMatrixWeights*)(skin->vertexMaps.matrixIndices + numVertices);

    if (usedBoneList != NULL && numUsedBones > 0)
    {
        memcpy(skin->boneData.usedBoneList, usedBoneList, numUsedBones);
    }

    if (inverseMatrices)
    {
        RwUInt32 i = skin->boneData.numBones;

        while (i--)
        {
            rwMatrixCopy(&skin->boneData.invBoneToSkinMat[i], &inverseMatrices[i]);
        }
    }

    if (vertexIndices)
    {
        memcpy(skin->vertexMaps.matrixIndices, vertexIndices, numVertices * sizeof(RwUInt32));
    }

    if (vertexWeights)
    {
        memcpy(skin->vertexMaps.matrixWeights, vertexWeights,
               numVertices * sizeof(RwMatrixWeights));
    }

    return TRUE;
}

static RpSkin* SkinCreate(RwUInt32 numVertices, RwUInt32 numBones, RwUInt32 numUsedBones,
                          RwUInt32 maxWeights, const RwMatrixWeights* vertexWeights,
                          const RwUInt32* vertexIndices, const RwMatrix* inverseMatrices)
{
    RpSkin* skin;
    RwUInt8 usedBoneList[rpSKINMAXNUMBEROFMATRICES];

    skin = (RpSkin*)RwFreeListAlloc(_rpSkinGlobals.freeList);
    memset(skin, 0, sizeof(RpSkin));

    if (!maxWeights)
    {
        SkinFindMaxWeights(skin, numVertices, vertexWeights);
    }

    if (!numUsedBones)
    {
        SkinFindUsedBones(usedBoneList, &numUsedBones, skin, numVertices, vertexIndices,
                          vertexWeights);
    }

    if (!SkinAllocate(skin, numVertices, numBones, numUsedBones, usedBoneList, vertexWeights,
                      vertexIndices, inverseMatrices))
    {
        RwFreeListFree(_rpSkinGlobals.freeList, skin);
        return (RpSkin*)NULL;
    }

    return skin;
}

static void* SkinOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (_rpSkinGlobals.module.numInstances == 0)
    {
        RwUInt32 pipes = rpSKINPIPELINESKINGENERIC;

        if (RwEngineGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN) != -1)
        {
            pipes |= rpSKINPIPELINESKINMATFX;
        }

        if (RwEngineGetPluginOffset(rwID_TOONPLUGIN) != -1)
        {
            pipes |= rpSKINPIPELINESKINTOON;
        }

        _rpSkinPipelinesCreate(pipes);

        _rpSkinGlobals.freeList =
            RwFreeListCreateAndPreallocateSpace(sizeof(RpSkin), _rpSkinFreeListBlockSize, 4,
                                                _rpSkinFreeListPreallocBlocks, &_rpSkinFreeList);

        _rpSkinGlobals.matrixCache.unaligned =
            RwMalloc(sizeof(RwMatrix) * rpSKINMAXNUMBEROFMATRICES + 15);
        memset(_rpSkinGlobals.matrixCache.unaligned, 0,
               sizeof(RwMatrix) * rpSKINMAXNUMBEROFMATRICES + 15);
        _rpSkinGlobals.matrixCache.aligned =
            (RwMatrix*)(((RwUInt32)_rpSkinGlobals.matrixCache.unaligned + 15) & ~15);
    }

    _rpSkinGlobals.module.numInstances++;

    return instance;
}

static void* SkinClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (--_rpSkinGlobals.module.numInstances == 0)
    {
        _rpSkinPipelinesDestroy();

        RwFreeListDestroy(_rpSkinGlobals.freeList);
        _rpSkinGlobals.freeList = (RwFreeList*)NULL;

        RwFree(_rpSkinGlobals.matrixCache.unaligned);
        _rpSkinGlobals.matrixCache.unaligned = NULL;
    }

    return instance;
}

static void* SkinGeometryConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    *RPSKINGEOMETRYGETDATA(object) = (RpSkin*)NULL;

    return object;
}

static void* SkinGeometryDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpSkin* skin = *RPSKINGEOMETRYGETDATA(object);

    if (skin)
    {
        _rpSkinDeinitialize((RpGeometry*)object);
        *RPSKINGEOMETRYGETDATA(object) = RpSkinDestroy(skin);
    }

    return object;
}

static void* SkinGeometryCopy(void* dstObject, const void* srcObject, RwInt32 offset,
                              RwInt32 size)
{
    return dstObject;
}

static void* SkinAtomicConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    *RPSKINATOMICGETDATA(object) = (RpHAnimHierarchy*)NULL;

    return object;
}

static void* SkinAtomicDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    if (*RPSKINATOMICGETDATA(object))
    {
        *RPSKINATOMICGETDATA(object) = (RpHAnimHierarchy*)NULL;
    }

    return object;
}

static void* SkinAtomicCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    *RPSKINATOMICGETDATA(dstObject) = *RPSKINATOMICGETDATA(srcObject);

    return dstObject;
}

static RpAtomic* SkinAtomicSetType(RpAtomic* atomic, RpSkinType type)
{
    /* Fall back to the generic pipeline when the required plugin is missing */
    if (RwEngineGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN) == -1 && type == rpSKINTYPEMATFX)
    {
        type = rpSKINTYPEGENERIC;
    }
    else if (RwEngineGetPluginOffset(rwID_TOONPLUGIN) == -1 && type == rpSKINTYPETOON)
    {
        type = rpSKINTYPEGENERIC;
    }

    return _rpSkinPipelinesAttach(atomic, type);
}

static RwBool SkinAtomicAlways(void* object, RwInt32 offset, RwInt32 size)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry;
    RpSkinType type = rpSKINTYPEGENERIC;

    if (RwEngineGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN) != -1)
    {
        if (*RWPLUGINOFFSET(RwUInt8, atomic, RpAtomicGetPluginOffset(rwID_MATERIALEFFECTSPLUGIN)))
        {
            type = rpSKINTYPEMATFX;
        }
    }

    geometry = atomic->geometry;

    if (geometry && RpSkinGeometryGetSkin(geometry))
    {
        SkinAtomicSetType(atomic, type);
    }

    return TRUE;
}

static RwBool SkinAtomicRights(void* object, RwInt32 offset, RwInt32 size, RwUInt32 extraData)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;

    if (geometry && RpSkinGeometryGetSkin(geometry))
    {
        SkinAtomicSetType(atomic, (RpSkinType)extraData);
    }

    return TRUE;
}

static RwInt32 SkinGeometrySize(const void* object, RwInt32 offset, RwInt32 size)
{
    const RpGeometry* geometry = (const RpGeometry*)object;
    RwInt32 streamSize = 0;
    RpSkin* skin = *RPSKINGEOMETRYGETDATA(geometry);

    if (skin)
    {
        if (!(geometry->flags & rpGEOMETRYNATIVE))
        {
            RwUInt32 numVertices = geometry->numVertices;

            streamSize = sizeof(RwInt32);
            streamSize += sizeof(RwUInt8) * skin->boneData.numUsedBones;
            streamSize += sizeof(RwUInt32) * numVertices;
            streamSize += sizeof(RwMatrixWeights) * numVertices;
            streamSize += sizeof(RwMatrix) * skin->boneData.numBones;
            streamSize += _rpSkinSplitDataStreamGetSize(skin);
        }
        else
        {
            streamSize = _rpSkinGeometryNativeSize(geometry);
        }
    }

    return streamSize;
}

static RwStream* SkinGeometryWrite(RwStream* stream, RwInt32 binaryLength, const void* object,
                                   RwInt32 offset, RwInt32 size)
{
    const RpGeometry* geometry = (const RpGeometry*)object;
    RpSkin* skin = *RPSKINGEOMETRYGETDATA(geometry);

    if (skin)
    {
        if (!(geometry->flags & rpGEOMETRYNATIVE))
        {
            RwUInt32 numVertices;
            RwUInt32 info;

            info = ((skin->vertexMaps.maxWeights & 0xFF) << 16) |
                   ((skin->boneData.numUsedBones & 0xFF) << 8) |
                   (skin->boneData.numBones & 0xFF);

            numVertices = geometry->numVertices;

            if (!RwStreamWriteInt32(stream, (RwInt32*)&info, sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

            if (!RwStreamWrite(stream, skin->boneData.usedBoneList, skin->boneData.numUsedBones))
            {
                return (RwStream*)NULL;
            }

            if (!RwStreamWriteInt32(stream, (RwInt32*)skin->vertexMaps.matrixIndices,
                                    numVertices * sizeof(RwUInt32)))
            {
                return (RwStream*)NULL;
            }

            if (!RwStreamWriteReal(stream, (RwReal*)skin->vertexMaps.matrixWeights,
                                   numVertices * sizeof(RwMatrixWeights)))
            {
                return (RwStream*)NULL;
            }

            if (!RwStreamWriteReal(stream, (RwReal*)skin->boneData.invBoneToSkinMat,
                                   skin->boneData.numBones * sizeof(RwMatrix)))
            {
                return (RwStream*)NULL;
            }

            if (!_rpSkinSplitDataStreamWrite(stream, skin))
            {
                return (RwStream*)NULL;
            }
        }
        else
        {
            if (!_rpSkinGeometryNativeWrite(stream, geometry))
            {
                return (RwStream*)NULL;
            }
        }
    }

    return stream;
}

static RwStream* SkinGeometryRead(RwStream* stream, RwInt32 binaryLength, void* object,
                                  RwInt32 offset, RwInt32 size)
{
    RpGeometry* geometry = (RpGeometry*)object;

    if (!(geometry->flags & rpGEOMETRYNATIVE))
    {
        RwUInt32 numVertices;
        RpSkin* skin;
        RwUInt32 info;
        RwUInt32 numBones;
        RwUInt32 maxWeights;
        RwUInt32 numUsedBones;

        if (!RwStreamReadInt32(stream, (RwInt32*)&info, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        numVertices = geometry->numVertices;
        numBones = info & 0xFF;
        numUsedBones = (info >> 8) & 0xFF;
        maxWeights = (info >> 16) & 0xFF;

        if (!maxWeights)
        {
            /* Old format: no used bone list */
            skin = SkinCreate(numVertices, numBones, numBones, rpSKINMAXWEIGHTS,
                              (RwMatrixWeights*)NULL, (RwUInt32*)NULL, (RwMatrix*)NULL);
            if (!skin)
            {
                return (RwStream*)NULL;
            }
        }
        else
        {
            skin = SkinCreate(numVertices, numBones, numUsedBones, maxWeights,
                              (RwMatrixWeights*)NULL, (RwUInt32*)NULL, (RwMatrix*)NULL);
            if (!skin)
            {
                return (RwStream*)NULL;
            }

            if (RwStreamRead(stream, skin->boneData.usedBoneList, numUsedBones) != numUsedBones)
            {
                return (RwStream*)NULL;
            }
        }

        if (!RwStreamReadInt32(stream, (RwInt32*)skin->vertexMaps.matrixIndices,
                               numVertices * sizeof(RwUInt32)))
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamReadReal(stream, (RwReal*)skin->vertexMaps.matrixWeights,
                              numVertices * sizeof(RwMatrixWeights)))
        {
            return (RwStream*)NULL;
        }

        if (!maxWeights)
        {
            RwUInt32 i;

            for (i = 0; i < skin->boneData.numBones; i++)
            {
                /* Skip the old bone tag */
                if (!RwStreamSkip(stream, sizeof(RwInt32)))
                {
                    return (RwStream*)NULL;
                }

                if (!RwStreamReadReal(stream, (RwReal*)&skin->boneData.invBoneToSkinMat[i],
                                      sizeof(RwMatrix)))
                {
                    return (RwStream*)NULL;
                }
            }

            SkinFindMaxWeights(skin, numVertices, skin->vertexMaps.matrixWeights);

            SkinFindUsedBones(skin->boneData.usedBoneList, &skin->boneData.numUsedBones, skin,
                              numVertices, skin->vertexMaps.matrixIndices,
                              skin->vertexMaps.matrixWeights);
        }
        else
        {
            skin->vertexMaps.maxWeights = maxWeights;

            if (!RwStreamReadReal(stream, (RwReal*)skin->boneData.invBoneToSkinMat,
                                  skin->boneData.numBones * sizeof(RwMatrix)))
            {
                return (RwStream*)NULL;
            }

            if (!_rpSkinSplitDataStreamRead(stream, skin))
            {
                return (RwStream*)NULL;
            }
        }

        RpSkinGeometrySetSkin(geometry, skin);
    }
    else
    {
        if (!_rpSkinGeometryNativeRead(stream, geometry))
        {
            return (RwStream*)NULL;
        }
    }

    return stream;
}

static RwStream* SkinAtomicRead(RwStream* stream, RwInt32 binaryLength, void* object,
                                RwInt32 offset, RwInt32 size)
{
    RpSkin* skin;
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;

    if (!RpSkinGeometryGetSkin(geometry))
    {
        /* Old format: the skin was stored with the atomic */
        RwInt32 numBones;
        RwUInt32 numVertices;
        RwUInt32 i;

        if (!RwStreamReadInt32(stream, &numBones, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        numVertices = geometry->numVertices;

        skin = SkinCreate(numVertices, numBones, numBones, rpSKINMAXWEIGHTS,
                          (RwMatrixWeights*)NULL, (RwUInt32*)NULL, (RwMatrix*)NULL);

        if (!RwStreamSkip(stream, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamReadInt32(stream, (RwInt32*)skin->vertexMaps.matrixIndices,
                               numVertices * sizeof(RwUInt32)))
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamReadReal(stream, (RwReal*)skin->vertexMaps.matrixWeights,
                              numVertices * sizeof(RwMatrixWeights)))
        {
            return (RwStream*)NULL;
        }

        for (i = 0; i < skin->boneData.numBones; i++)
        {
            /* Skip the old bone tag, index and flags */
            if (!RwStreamSkip(stream, 3 * sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

            if (!RwStreamReadReal(stream, (RwReal*)&skin->boneData.invBoneToSkinMat[i],
                                  sizeof(RwMatrix)))
            {
                return (RwStream*)NULL;
            }
        }

        SkinFindMaxWeights(skin, numVertices, skin->vertexMaps.matrixWeights);

        SkinFindUsedBones(skin->boneData.usedBoneList, &skin->boneData.numUsedBones, skin,
                          numVertices, skin->vertexMaps.matrixIndices,
                          skin->vertexMaps.matrixWeights);

        RpSkinGeometrySetSkin(geometry, skin);
    }
    else
    {
        if (!RwStreamSkip(stream, size))
        {
            return (RwStream*)NULL;
        }
    }

    return stream;
}

static RwStream* SkinAtomicWrite(RwStream* stream, RwInt32 binaryLength, const void* object,
                                 RwInt32 offset, RwInt32 size)
{
    return stream;
}

static RwInt32 SkinAtomicGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    return 0;
}

RwBool RpSkinPluginAttach(void)
{
    _rpSkinGlobals.engineOffset = RwEngineRegisterPlugin(0, rwID_SKINPLUGIN, SkinOpen, SkinClose);

    _rpSkinGlobals.atomicOffset =
        RpAtomicRegisterPlugin(sizeof(RpHAnimHierarchy*), rwID_SKINPLUGIN, SkinAtomicConstructor,
                               SkinAtomicDestructor, SkinAtomicCopy);

    RpAtomicRegisterPluginStream(rwID_SKINPLUGIN, SkinAtomicRead, SkinAtomicWrite,
                                 SkinAtomicGetSize);
    RpAtomicSetStreamAlwaysCallBack(rwID_SKINPLUGIN, SkinAtomicAlways);
    RpAtomicSetStreamRightsCallBack(rwID_SKINPLUGIN, SkinAtomicRights);

    _rpSkinGlobals.geometryOffset =
        RpGeometryRegisterPlugin(sizeof(RpSkin*), rwID_SKINPLUGIN, SkinGeometryConstructor,
                                 SkinGeometryDestructor, SkinGeometryCopy);

    RpGeometryRegisterPluginStream(rwID_SKINPLUGIN, SkinGeometryRead, SkinGeometryWrite,
                                   SkinGeometrySize);

    return TRUE;
}

RpAtomic* RpSkinAtomicSetHAnimHierarchy(RpAtomic* atomic, RpHAnimHierarchy* hierarchy)
{
    *RPSKINATOMICGETDATA(atomic) = hierarchy;

    return atomic;
}

RpSkin* RpSkinGeometryGetSkin(RpGeometry* geometry)
{
    return *RPSKINGEOMETRYGETDATA(geometry);
}

RpGeometry* RpSkinGeometrySetSkin(RpGeometry* geometry, RpSkin* skin)
{
    RpSkin* oldSkin = *RPSKINGEOMETRYGETDATA(geometry);

    if (skin != oldSkin)
    {
        if (oldSkin)
        {
            _rpSkinDeinitialize(geometry);
        }

        *RPSKINGEOMETRYGETDATA(geometry) = skin;

        if (skin)
        {
            if (!_rpSkinInitialize(geometry))
            {
                return (RpGeometry*)NULL;
            }
        }
    }

    return geometry;
}

RpSkin* RpSkinDestroy(RpSkin* skin)
{
    if (skin->unaligned)
    {
        RwFree(skin->unaligned);
    }

    _rpSkinSplitDataDestroy(skin);

    RwFreeListFree(_rpSkinGlobals.freeList, skin);

    return (RpSkin*)NULL;
}

RwUInt32 RpSkinGetNumBones(RpSkin* skin)
{
    return skin->boneData.numBones;
}

const RwMatrixWeights* RpSkinGetVertexBoneWeights(RpSkin* skin)
{
    return skin->vertexMaps.matrixWeights;
}

const RwUInt32* RpSkinGetVertexBoneIndices(RpSkin* skin)
{
    return skin->vertexMaps.matrixIndices;
}

const RwMatrix* RpSkinGetSkinToBoneMatrices(RpSkin* skin)
{
    return skin->boneData.invBoneToSkinMat;
}

RpAtomic* RpSkinAtomicSetType(RpAtomic* atomic, RpSkinType type)
{
    return SkinAtomicSetType(atomic, type);
}
