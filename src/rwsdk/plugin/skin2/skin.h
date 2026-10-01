#ifndef SKIN_H
#define SKIN_H

/* Private declarations shared by the units of the RpSkin plugin. */

#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rphanim.h>
#include <rwsdk/rpskin.h>

#define rwID_SKINPLUGIN 0x116
#define rwID_MATERIALEFFECTSPLUGIN 0x120
#define rwID_TOONPLUGIN 0x12E

#define rpSKINMAXNUMBEROFMATRICES 256
#define rpSKINMAXWEIGHTS 4

/* Pipeline flags passed to _rpSkinPipelinesCreate */
#define rpSKINPIPELINESKINGENERIC 0x01
#define rpSKINPIPELINESKINMATFX 0x02
#define rpSKINPIPELINESKINTOON 0x04

#define rpSKINNUMPIPELINES 5

#ifndef RWMODULEINFO_DEFINED
#define RWMODULEINFO_DEFINED
typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};
#endif

typedef struct SkinMatrixCache SkinMatrixCache;
struct SkinMatrixCache
{
    RwMatrix* aligned;
    void* unaligned;
};

typedef struct SkinGlobalPlatform SkinGlobalPlatform;
struct SkinGlobalPlatform
{
    RxPipeline* pipelines[rpSKINNUMPIPELINES];
};

typedef struct SkinSplitData SkinSplitData;
struct SkinSplitData
{
    RwUInt32 boneLimit;
    RwUInt32 numMeshes;
    RwUInt32 numRLE;
    RwUInt8* matrixRemapIndices;
    RwUInt8* meshRLECount;
    RwUInt8* meshRLE;
};

typedef struct SkinGlobals SkinGlobals;
struct SkinGlobals
{
    RwInt32 engineOffset;
    RwInt32 atomicOffset;
    RwInt32 geometryOffset;
    SkinMatrixCache matrixCache;
    RwUInt32 numMatrices;
    RwFreeList* freeList;
    RwModuleInfo module;
    SkinGlobalPlatform platform;
    SkinSplitData* skinSplitData;
};

typedef struct SkinBoneData SkinBoneData;
struct SkinBoneData
{
    RwUInt32 numBones;
    RwUInt32 numUsedBones;
    RwUInt8* usedBoneList;
    RwMatrix* invBoneToSkinMat;
};

typedef struct SkinVertexMaps SkinVertexMaps;
struct SkinVertexMaps
{
    RwUInt32 maxWeights;
    RwUInt32* matrixIndices;
    RwMatrixWeights* matrixWeights;
};

typedef struct SkinPlatformData SkinPlatformData;
struct SkinPlatformData
{
    void* vertices;
    void* normals;
    RwUInt8* weights;
    RwUInt8* indices;
};

struct RpSkin
{
    SkinBoneData boneData;
    SkinVertexMaps vertexMaps;
    SkinPlatformData platformData;
    SkinSplitData skinSplitData;
    void* unaligned;
};

#define RPSKINATOMICGETDATA(_atomic) \
    (RWPLUGINOFFSET(RpHAnimHierarchy*, _atomic, _rpSkinGlobals.atomicOffset))

#define RPSKINGEOMETRYGETDATA(_geometry) \
    (RWPLUGINOFFSET(RpSkin*, _geometry, _rpSkinGlobals.geometryOffset))

#ifdef __cplusplus
extern "C" {
#endif

extern SkinGlobals _rpSkinGlobals;

/* Platform specific */
extern RwBool _rpSkinPipelinesCreate(RwUInt32 pipes);
extern RwBool _rpSkinPipelinesDestroy(void);
extern RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RpSkinType type);

extern RpGeometry* _rpSkinInitialize(RpGeometry* geometry);
extern RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry);

extern RwInt32 _rpSkinGeometryNativeSize(const RpGeometry* geometry);
extern RwStream* _rpSkinGeometryNativeWrite(RwStream* stream, const RpGeometry* geometry);
extern RwStream* _rpSkinGeometryNativeRead(RwStream* stream, RpGeometry* geometry);

/* Bone splitting */
extern RpSkin* _rpSkinSplitDataCreate(RpSkin* skin, RwUInt32 boneLimit, RwUInt32 numMatrices,
                                      RwUInt32 numMeshes, RwUInt32 numRLE);
extern RwBool _rpSkinSplitDataDestroy(RpSkin* skin);
extern RwStream* _rpSkinSplitDataStreamWrite(RwStream* stream, const RpSkin* skin);
extern RwStream* _rpSkinSplitDataStreamRead(RwStream* stream, RpSkin* skin);
extern RwInt32 _rpSkinSplitDataStreamGetSize(const RpSkin* skin);

#ifdef __cplusplus
}
#endif

#endif
