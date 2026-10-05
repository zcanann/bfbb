#ifndef PS2_RPWORLD_H
#define PS2_RPWORLD_H

#include <rwcore.h>

struct RpMaterial;
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

extern "C" {
RpLight* RpLightCreate(RwInt32 type);
RwBool RpLightDestroy(RpLight* light);
RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color);
RpLight* RpLightSetRadius(RpLight* light, RwReal radius);
RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle);
RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light);
RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light);
}

#endif
