#ifndef PS2_RPWORLD_H
#define PS2_RPWORLD_H

#include <rwcore.h>

struct RpMaterial;
struct RpSector;
struct RpWorldSector;
struct RpLight;

struct RpMaterialList
{
    RpMaterial** materials;
    RwInt32 numMaterials;
    RwInt32 space;
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

#endif
