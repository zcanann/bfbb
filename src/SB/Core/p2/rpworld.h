#ifndef PS2_RPWORLD_H
#define PS2_RPWORLD_H

#include <rwcore.h>

struct RpAtomic;
typedef RpAtomic* (*RpAtomicCallBackRender)(RpAtomic* atomic);

struct RpMaterial
{
    RwTexture* texture;
    RwRGBA color;
    RxPipeline* pipeline;
    RwSurfaceProperties surfaceProps;
    RwInt16 refCount;
    RwInt16 pad;
};
struct RpSector;
struct RpWorldSector;
struct RpLight
{
    RwObjectHasFrame object;
    RwReal radius;
    RwRGBAReal color;
    RwReal minusCosAngle;
    RwLinkList WorldSectorsInLight;
    RwLLLink inWorld;
    RwUInt16 lightFrame;
    RwUInt16 pad;
};

struct RpMaterialList
{
    RpMaterial** materials;
    RwInt32 numMaterials;
    RwInt32 space;
};

struct RpClump;
struct RpGeometry;
struct RpTriangle;
struct RpMeshHeader;

struct RpInterpolator
{
    signed int flags;
    signed short startMorphTarget;
    signed short endMorphTarget;
    float time;
    float recipTime;
    float position;
};

struct RpMorphTarget
{
    RpGeometry* parentGeom;
    RwSphere boundingSphere;
    RwV3d* verts;
    RwV3d* normals;
};

struct RpGeometry
{
    RwObject object;
    unsigned int flags;
    unsigned short lockedSinceLastInst;
    signed short refCount;
    signed int numTriangles;
    signed int numVertices;
    signed int numMorphTargets;
    signed int numTexCoordSets;
    RpMaterialList matList;
    RpTriangle* triangles;
    RwRGBA* preLitLum;
    RwTexCoords* texCoords[8];
    RpMeshHeader* mesh;
    RwResEntry* repEntry;
    RpMorphTarget* morphTarget;
};

struct RpAtomic
{
    RwObjectHasFrame object;
    RwResEntry* repEntry;
    RpGeometry* geometry;
    RwSphere boundingSphere;
    RwSphere worldBoundingSphere;
    RpClump* clump;
    RwLLLink inClumpLink;
    RpAtomic* (*renderCallBack)(RpAtomic*);
    RpInterpolator interpolator;
    unsigned short renderFrame;
    unsigned short pad;
    RwLinkList llWorldSectorsInAtomic;
    RxPipeline* pipeline;
};

struct RpVertexNormal;

struct RpTriangle
{
    RwUInt16 vertIndex[3];
    RwInt16 matIndex;
};

struct RpPolygon
{
    RwUInt16 matIndex;
    RwUInt16 vertIndex[3];
};

struct RpWorldSector
{
    RwInt32 type;
    RpPolygon* polygons;
    RwV3d* vertices;
    RpVertexNormal* normals;
    RwTexCoords* texCoords[8];
    RwRGBA* preLitLum;
    RwResEntry* repEntry;
    RwLinkList collAtomicsInWorldSector;
    RwLinkList noCollAtomicsInWorldSector;
    RwLinkList lightsInWorldSector;
    RwBBox boundingBox;
    RwBBox tightBoundingBox;
    RpMeshHeader* mesh;
    RxPipeline* pipeline;
    RwUInt16 matListWindowBase;
    RwUInt16 numVertices;
    RwUInt16 numPolygons;
    RwUInt16 pad;
};

enum RpWorldRenderOrder
{
    rpWORLDRENDERNARENDERORDER = 0,
    rpWORLDRENDERFRONT2BACK = 1,
    rpWORLDRENDERBACK2FRONT = 2,
    rpWORLDRENDERORDERFORCEENUMSIZEINT = 0x7fffffff
};

struct RpWorld
{
    RwObject object;
    RwUInt32 flags;
    RpWorldRenderOrder renderOrder;
    RpMaterialList matList;
    RpSector* rootSector;
    RwInt32 numTexCoordSets;
    RwInt32 numClumpsInWorld;
    RwLLLink* currentClumpLink;
    RwLinkList clumpList;
    RwLinkList lightList;
    RwLinkList directionalLightList;
    RwV3d worldOrigin;
    RwBBox boundingBox;
    RpWorldSector* (*renderCallBack)(RpWorldSector*);
    RxPipeline* pipeline;
};

#define rpLIGHTPOSITIONINGSTART 0x80

enum RpLightType
{
    rpNALIGHTTYPE = 0,
    rpLIGHTDIRECTIONAL,
    rpLIGHTAMBIENT,
    rpLIGHTPOINT = rpLIGHTPOSITIONINGSTART,
    rpLIGHTSPOT,
    rpLIGHTSPOTSOFT,
    rpLIGHTTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpLightType RpLightType;

#define RpAtomicSetRenderCallBackMacro(_atomic, _callback)                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_atomic)->renderCallBack = (_callback);                                                   \
        if (!(_atomic)->renderCallBack)                                                            \
        {                                                                                          \
            (_atomic)->renderCallBack = AtomicDefaultRenderCallBack;                               \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

#define RpAtomicGetRenderCallBackMacro(_atomic) ((_atomic)->renderCallBack)

#define RpAtomicSetRenderCallBack(_atomic, _callback) RpAtomicSetRenderCallBackMacro(_atomic, _callback)
#define RpAtomicGetRenderCallBack(_atomic) RpAtomicGetRenderCallBackMacro(_atomic)
#define RpMaterialGetTextureMacro(_material) ((_material)->texture)
#define RpMaterialGetTexture(_material) RpMaterialGetTextureMacro(_material)
#define RpWorldGetNumMaterialsMacro(_world) ((_world)->matList.numMaterials)
#define RpWorldGetMaterialMacro(_world, _num) (((_world)->matList.materials)[(_num)])
#define RpWorldGetNumMaterials(_world) RpWorldGetNumMaterialsMacro(_world)
#define RpWorldGetMaterial(_world, _num) RpWorldGetMaterialMacro(_world, _num)
#define RpLightSetFrameMacro(_light, _frame) (rwObjectHasFrameSetFrame((_light), (_frame)), (_light))
#define RpLightGetFrameMacro(_light) ((RwFrame*)rwObjectGetParent((_light)))
#define RpLightSetFrame(_light, _frame) RpLightSetFrameMacro(_light, _frame)
#define RpLightGetFrame(_light) RpLightGetFrameMacro(_light)

typedef RpAtomic* (*RpAtomicCallBack)(RpAtomic* atomic, void* data);
typedef RpMaterial* (*RpMaterialCallBack)(RpMaterial* material, void* data);

extern "C" {
RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic);
RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* data);
RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack callback, void* data);
RpGeometry* RpGeometryLock(RpGeometry* geometry, RwInt32 lockMode);
RpGeometry* RpGeometryUnlock(RpGeometry* geometry);
}

#define RpAtomicGetFrame(_atomic) ((RwFrame*)((_atomic)->object.object.parent))
#define RpAtomicGetGeometry(_atomic) ((_atomic)->geometry)

extern "C" {
RpLight* RpLightCreate(RwInt32 type);
RwBool RpLightDestroy(RpLight* light);
RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color);
RpLight* RpLightSetRadius(RpLight* light, RwReal radius);
RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle);
RpWorld* RpWorldCreate(RwBBox* boundingBox);
RwBool RpWorldDestroy(RpWorld* world);
RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera);
RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light);
RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light);
RpWorld* RwCameraGetWorld(const RwCamera* camera);
}

extern "C" {
RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame);
RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture);
RwInt32 RpClumpGetNumAtomics(RpClump* clump);
RwBool RpAtomicDestroy(RpAtomic* atomic);
RpAtomic* RpAtomicStreamRead(struct RwStream* stream);
const RpAtomic* RpAtomicStreamWrite(const RpAtomic* atomic, struct RwStream* stream);
}

#define RpAtomicRenderMacro(_atomic) ((_atomic)->renderCallBack(_atomic))
#define RpAtomicRender(_atomic) RpAtomicRenderMacro(_atomic)

#define RpAtomicGetGeometryMacro(_atomic) ((_atomic)->geometry)

#define RpAtomicGetClumpMacro(_atomic) ((_atomic)->clump)
#define RpAtomicGetClump(_atomic) RpAtomicGetClumpMacro(_atomic)

// RenderWare SDK bounding-sphere accessor.
enum RpInterpolatorFlag
{
    rpINTERPOLATORDIRTYINSTANCE = 0x01,
    rpINTERPOLATORDIRTYSPHERE = 0x02,
    rpINTERPOLATORNOFRAMEDIRTY = 0x04,
    rpINTERPOLATORFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

extern "C" void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic);

#define RpAtomicGetBoundingSphereMacro(_atomic)                                                        ((((_atomic)->interpolator.flags & rpINTERPOLATORDIRTYSPHERE) ?                                      _rpAtomicResyncInterpolatedSphere(_atomic),                                                        0 : 0),                                                                                           &((_atomic)->boundingSphere))
#define RpAtomicGetBoundingSphere(_atomic) RpAtomicGetBoundingSphereMacro(_atomic)

// RenderWare SDK world/clump stream and clump membership API.
extern "C" {
RpWorld* RpWorldStreamRead(RwStream* stream);
}

#endif
