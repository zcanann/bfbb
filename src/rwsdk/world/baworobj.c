#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rpATOMICPRIVATEWORLDBOUNDDIRTY 0x01

#define rwSTANDARDHINTRENDERF2B 22

#define rwPLUGIN_ID 2

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_NOMEM 0x80000013
#define E_RW_FRAMESYNCFAILED 0x80000016

/* Plane sector types are byte offsets of the split axis within an RwV3d */
#define GETCOORD(vect, axis) (*(const RwReal*)(((const RwUInt8*)(&(vect))) + (axis)))

typedef struct RpPlaneSector RpPlaneSector;
struct RpPlaneSector
{
    RwInt32 type;
    RwReal value;
    RpSector* leftSubTree;
    RpSector* rightSubTree;
    RwReal leftValue;
    RwReal rightValue;
};

#define rpWORLDMAXBSPDEPTH 64

#define rpWORLDFRUSTUMSECTORSGRANULARITY 50

#define MAKECHUNKID(vendorID, chunkID) (((vendorID & 0xFFFFFF) << 8) | (chunkID & 0xFF))

#define rwID_WORLDOBJMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x09)
#define rwID_BINMESHPLUGIN MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x0E)
#define rwID_NATIVEDATAPLUGIN MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x10)
#define rwID_RIGHTTORENDER 0x1F

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct RpTie RpTie;
struct RpTie
{
    RwLLLink lWorldSector;
    RpAtomic* apAtom;
    RwLLLink lAtomic;
    RpWorldSector* worldSector;
};

typedef struct RpLightTie RpLightTie;
struct RpLightTie
{
    RwLLLink lWorldSector;
    RpLight* light;
    RwLLLink lLight;
    RpWorldSector* sect;
};

typedef struct rpWorldObjGlobals rpWorldObjGlobals;
struct rpWorldObjGlobals
{
    RwFreeList* tieFreeList;
    RwFreeList* lightTieFreeList;
    void* worldSectorPool;
};

typedef struct rpWorldAtomicExt rpWorldAtomicExt;
struct rpWorldAtomicExt
{
    RpWorld* world;
    RwObjectHasFrameSyncFunction originalSync;
};

typedef struct rpWorldClumpExt rpWorldClumpExt;
struct rpWorldClumpExt
{
    RpWorld* world;
    void* worldSectorPool;
};

typedef struct rpWorldLightExt rpWorldLightExt;
struct rpWorldLightExt
{
    RpWorld* world;
    RwObjectHasFrameSyncFunction originalSync;
};

typedef struct rpWorldCameraExt rpWorldCameraExt;
struct rpWorldCameraExt
{
    RpWorldSector** frustumSectors;
    RwInt32 spaceInFrustumSectors;
    RwInt32 numSectorsInFrustum;
    RpWorld* world;
    RwCameraBeginUpdateFunc originalBeginUpdate;
    RwCameraEndUpdateFunc originalEndUpdate;
    RwObjectHasFrameSyncFunction originalSync;
};

extern RwInt32 _rpGeometryNativeSize(const RpGeometry* geometry);
extern RwInt32 _rpWorldSectorNativeSize(const RpWorldSector* sector);
extern RwStream* _rpGeometryNativeWrite(RwStream* stream, const RpGeometry* geometry);
extern RwStream* _rpWorldSectorNativeWrite(RwStream* stream, const RpWorldSector* sector);
extern RpGeometry* _rpGeometryNativeRead(RwStream* stream, RpGeometry* geometry);
extern RpWorldSector* _rpWorldSectorNativeRead(RwStream* stream, RpWorldSector* sector);

extern RwObjectHasFrame* WorldAtomicSync(RwObjectHasFrame* object);

static RpWorld* WorldSyncCamera(RpWorld* world, RwCamera* camera);

extern RwStream* _rpReadAtomicRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off,
                                     RwInt32 size);
extern RwStream* _rpWriteAtomicRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off,
                                      RwInt32 size);
extern RwInt32 _rpSizeAtomicRights(const void* obj, RwInt32 off, RwInt32 size);
extern RwStream* _rpReadWorldRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off, RwInt32 size);
extern RwStream* _rpWriteWorldRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off,
                                     RwInt32 size);
extern RwInt32 _rpSizeWorldRights(const void* obj, RwInt32 off, RwInt32 size);
extern RwStream* _rpReadSectRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off, RwInt32 size);
extern RwStream* _rpWriteSectRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off,
                                    RwInt32 size);
extern RwInt32 _rpSizeSectRights(const void* obj, RwInt32 off, RwInt32 size);
extern RwStream* _rpReadMaterialRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off,
                                       RwInt32 size);
extern RwStream* _rpWriteMaterialRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off,
                                        RwInt32 size);
extern RwInt32 _rpSizeMaterialRights(const void* obj, RwInt32 off, RwInt32 size);
extern RwBool _rpWorldPipeAttach(void);

static RwModuleInfo worldObjModule;

RwInt32 atomicExtOffset = 0;
RwInt32 clumpExtOffset = 0;
RwInt32 lightExtOffset = 0;
RwInt32 cameraExtOffset = 0;

RwInt32 _rpTieFreeListBlockSize = 256;
RwInt32 _rpTieFreeListPreallocBlocks = 1;
RwInt32 _rpLightTieFreeListBlockSize = 32;
RwInt32 _rpLightTieFreeListPreallocBlocks = 1;

static RwFreeList _rpTieFreeList;
static RwFreeList _rpLightTieFreeList;

#define ATOMICEXTFROMATOMIC(_atomic) (RWPLUGINOFFSET(rpWorldAtomicExt, (_atomic), atomicExtOffset))
#define CLUMPEXTFROMCLUMP(_clump) (RWPLUGINOFFSET(rpWorldClumpExt, (_clump), clumpExtOffset))
#define LIGHTEXTFROMLIGHT(_light) (RWPLUGINOFFSET(rpWorldLightExt, (_light), lightExtOffset))
#define CAMERAEXTFROMCAMERA(_camera) (RWPLUGINOFFSET(rpWorldCameraExt, (_camera), cameraExtOffset))

#define RWWORLDOBJGLOBAL(var)                                                                      \
    (RWPLUGINOFFSET(rpWorldObjGlobals, RwEngineInstance, worldObjModule.globalsOffset)->var)

void* WorldObjectOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    worldObjModule.globalsOffset = offset;

    RWWORLDOBJGLOBAL(tieFreeList) = RwFreeListCreateAndPreallocateSpace(
        sizeof(RpTie), _rpTieFreeListBlockSize, 4, _rpTieFreeListPreallocBlocks, &_rpTieFreeList);
    if (!RWWORLDOBJGLOBAL(tieFreeList))
    {
        return NULL;
    }

    RWWORLDOBJGLOBAL(lightTieFreeList) =
        RwFreeListCreateAndPreallocateSpace(sizeof(RpLightTie), _rpLightTieFreeListBlockSize, 4,
                                            _rpLightTieFreeListPreallocBlocks,
                                            &_rpLightTieFreeList);
    if (!RWWORLDOBJGLOBAL(lightTieFreeList))
    {
        RwFreeListDestroy(RWWORLDOBJGLOBAL(tieFreeList));
        RWWORLDOBJGLOBAL(tieFreeList) = (RwFreeList*)NULL;

        return NULL;
    }

    RWSRCGLOBAL(renderFrame) = 1;
    RWWORLDOBJGLOBAL(worldSectorPool) = NULL;

    worldObjModule.numInstances++;

    return instance;
}

void* WorldObjectClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWWORLDOBJGLOBAL(lightTieFreeList))
    {
        RwFreeListDestroy(RWWORLDOBJGLOBAL(lightTieFreeList));
        RWWORLDOBJGLOBAL(lightTieFreeList) = (RwFreeList*)NULL;
    }

    if (RWWORLDOBJGLOBAL(tieFreeList))
    {
        RwFreeListDestroy(RWWORLDOBJGLOBAL(tieFreeList));
        RWWORLDOBJGLOBAL(tieFreeList) = (RwFreeList*)NULL;
    }

    worldObjModule.numInstances--;

    return instance;
}

static RwBool SectorsInFrustumAddSpace(rpWorldCameraExt* cameraExt, RwInt32 nNum)
{
    RpWorldSector** newFrustumSectors;
    RwInt32 memSize = (cameraExt->spaceInFrustumSectors + nNum) * sizeof(RpWorldSector*);

    if (cameraExt->frustumSectors)
    {
        newFrustumSectors = (RpWorldSector**)RwRealloc(cameraExt->frustumSectors, memSize);
    }
    else
    {
        newFrustumSectors = (RpWorldSector**)RwMalloc(memSize);
    }

    if (newFrustumSectors)
    {
        cameraExt->frustumSectors = newFrustumSectors;
        cameraExt->spaceInFrustumSectors += nNum;

        return TRUE;
    }

    RWERROR((E_RW_NOMEM, memSize));
    return FALSE;
}

static RwCamera* WorldCameraBeginUpdate(RwCamera* camera)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    RWSRCGLOBAL(curWorld) = cameraExt->world;
    RWSRCGLOBAL(renderFrame)++;

    return cameraExt->originalBeginUpdate(camera);
}

static RwCamera* WorldCameraEndUpdate(RwCamera* camera)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    RWSRCGLOBAL(curWorld) = NULL;

    return cameraExt->originalEndUpdate(camera);
}

static RwObjectHasFrame* WorldCameraSync(RwObjectHasFrame* object)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(object);

    if (cameraExt->originalSync(object))
    {
        RpWorld* world = cameraExt->world;

        if (world)
        {
            RwStandardFunc HintRenderF2BFunc = RWSRCGLOBAL(stdFunc)[rwSTANDARDHINTRENDERF2B];

            WorldSyncCamera(world, (RwCamera*)object);

            HintRenderF2BFunc(NULL, NULL, world->renderOrder == rpWORLDRENDERFRONT2BACK);
        }

        return object;
    }

    return (RwObjectHasFrame*)NULL;
}

static void* WorldInitCameraExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RwCamera* camera = (RwCamera*)object;
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    cameraExt->frustumSectors = (RpWorldSector**)NULL;
    cameraExt->spaceInFrustumSectors = 0;
    cameraExt->numSectorsInFrustum = 0;

    cameraExt->originalBeginUpdate = camera->beginUpdate;
    cameraExt->originalEndUpdate = camera->endUpdate;
    cameraExt->originalSync = camera->object.sync;

    camera->object.sync = WorldCameraSync;
    camera->beginUpdate = WorldCameraBeginUpdate;
    camera->endUpdate = WorldCameraEndUpdate;

    cameraExt->world = (RpWorld*)NULL;

    return object;
}

static void* WorldCopyCameraExt(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                                RwInt32 sizeInObject)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(dstObject);
    const rpWorldCameraExt* srcCameraExt = CAMERAEXTFROMCAMERA(srcObject);

    cameraExt->frustumSectors = (RpWorldSector**)NULL;
    cameraExt->spaceInFrustumSectors = 0;
    cameraExt->numSectorsInFrustum = 0;

    if (srcCameraExt->world)
    {
        RpWorldAddCamera(srcCameraExt->world, (RwCamera*)dstObject);
    }

    return dstObject;
}

static void* WorldDeInitCameraExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RwCamera* camera = (RwCamera*)object;
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    if (cameraExt->frustumSectors)
    {
        RwFree(cameraExt->frustumSectors);
    }

    cameraExt->frustumSectors = (RpWorldSector**)NULL;
    cameraExt->spaceInFrustumSectors = 0;
    cameraExt->numSectorsInFrustum = 0;

    camera->beginUpdate = cameraExt->originalBeginUpdate;
    camera->endUpdate = cameraExt->originalEndUpdate;
    camera->object.sync = cameraExt->originalSync;

    return object;
}

RwBool _rpLightTieDestroy(RpLightTie* tie)
{
    rwLinkListRemoveLLLink(&tie->lLight);
    rwLinkListRemoveLLLink(&tie->lWorldSector);

    RWSRCGLOBAL(memoryFree)(RWWORLDOBJGLOBAL(lightTieFreeList), tie);

    return TRUE;
}

RwBool _rpTieDestroy(RpTie* tie)
{
    if (tie->apAtom && tie->worldSector)
    {
        rwLinkListRemoveLLLink(&tie->lAtomic);
        rwLinkListRemoveLLLink(&tie->lWorldSector);

        RWSRCGLOBAL(memoryFree)(RWWORLDOBJGLOBAL(tieFreeList), tie);
    }

    return TRUE;
}

static void AtomicDestroyTies(RpAtomic* atomic)
{
    RwLLLink* cur;
    RwLLLink* end;
    RpTie* tie;

    cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
    end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
    while (cur != end)
    {
        tie = rwLLLinkGetData(cur, RpTie, lAtomic);
        cur = rwLLLinkGetNext(cur);
        _rpTieDestroy(tie);
    }
}

static void WorldAttachAtomicSphere(RpWorld* world, RpAtomic* atomic)
{
    RwInt32 nStack = 0;
    RpSector* spSect;
    RpSector* spaStack[rpWORLDMAXBSPDEPTH];
    RwV3d inf;
    RwV3d sup;
    const RwSphere* worldSphere;

    worldSphere = RpAtomicGetWorldBoundingSphere(atomic);

    inf = worldSphere->center;
    sup = worldSphere->center;

    inf.x -= worldSphere->radius;
    inf.y -= worldSphere->radius;
    inf.z -= worldSphere->radius;

    sup.x += worldSphere->radius;
    sup.y += worldSphere->radius;
    sup.z += worldSphere->radius;

    spSect = world->rootSector;

    do
    {
        if (spSect->type < 0)
        {
            RpTie* tie = (RpTie*)RWSRCGLOBAL(memoryAlloc)(RWWORLDOBJGLOBAL(tieFreeList));

            tie->worldSector = (RpWorldSector*)spSect;
            tie->apAtom = atomic;

            if (rwObjectTestFlags(atomic, rpATOMICCOLLISIONTEST))
            {
                rwLinkListAddLLLink(&((RpWorldSector*)spSect)->collAtomicsInWorldSector,
                                    &tie->lWorldSector);
            }
            else
            {
                rwLinkListAddLLLink(&((RpWorldSector*)spSect)->noCollAtomicsInWorldSector,
                                    &tie->lWorldSector);
            }

            rwLinkListAddLLLink(&atomic->llWorldSectorsInAtomic, &tie->lAtomic);

            spSect = spaStack[nStack--];
        }
        else
        {
            RpPlaneSector* pspPlane = (RpPlaneSector*)spSect;

            if (GETCOORD(inf, pspPlane->type) < pspPlane->leftValue)
            {
                spSect = pspPlane->leftSubTree;

                if (pspPlane->rightValue < GETCOORD(sup, pspPlane->type))
                {
                    spaStack[++nStack] = pspPlane->rightSubTree;
                }
            }
            else
            {
                if (pspPlane->rightValue < GETCOORD(sup, pspPlane->type))
                {
                    spSect = pspPlane->rightSubTree;
                }
                else
                {
                    spSect = spaStack[nStack--];
                }
            }
        }
    } while (nStack >= 0);
}

RwObjectHasFrame* WorldAtomicSync(RwObjectHasFrame* object)
{
    RpAtomic* atomic = (RpAtomic*)object;
    rpWorldAtomicExt* atomicExt = ATOMICEXTFROMATOMIC(atomic);

    if (atomicExt->originalSync(object))
    {
        RpWorld* world = atomicExt->world;

        if (world)
        {
            AtomicDestroyTies(atomic);
            WorldAttachAtomicSphere(world, atomic);
        }

        return object;
    }

    return (RwObjectHasFrame*)NULL;
}

static void* WorldInitAtomicExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpAtomic* atomic = (RpAtomic*)object;
    rpWorldAtomicExt* atomicExt = ATOMICEXTFROMATOMIC(atomic);

    atomicExt->world = (RpWorld*)NULL;
    atomic->renderFrame = RWSRCGLOBAL(renderFrame) - 1;
    atomicExt->originalSync = atomic->object.sync;
    atomic->object.sync = (RwObjectHasFrameSyncFunction)WorldAtomicSync;

    return object;
}

static void* WorldCopyAtomicExt(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                                RwInt32 sizeInObject)
{
    return dstObject;
}

static void* WorldDeInitAtomicExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpAtomic* atomic = (RpAtomic*)object;
    rpWorldAtomicExt* atomicExt = ATOMICEXTFROMATOMIC(atomic);

    AtomicDestroyTies(atomic);

    atomic->object.sync = atomicExt->originalSync;

    return object;
}

static void* WorldInitClumpExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    rpWorldClumpExt* clumpExt = CLUMPEXTFROMCLUMP(object);

    clumpExt->world = (RpWorld*)NULL;
    clumpExt->worldSectorPool = RWWORLDOBJGLOBAL(worldSectorPool);

    return object;
}

static void* WorldCopyClumpExt(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                               RwInt32 sizeInObject)
{
    if (CLUMPEXTFROMCLUMP(srcObject)->world)
    {
        RpWorldAddClump(CLUMPEXTFROMCLUMP(srcObject)->world, (RpClump*)dstObject);
    }

    return dstObject;
}

static void* WorldDeInitClumpExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

static void LightDestroyTies(RpLight* light)
{
    RwLLLink* cur;
    RwLLLink* end;
    RpLightTie* tie;

    cur = rwLinkListGetFirstLLLink(&light->WorldSectorsInLight);
    end = rwLinkListGetTerminator(&light->WorldSectorsInLight);
    while (cur != end)
    {
        tie = rwLLLinkGetData(cur, RpLightTie, lLight);
        cur = rwLLLinkGetNext(cur);
        _rpLightTieDestroy(tie);
    }
}

static RwObjectHasFrame* WorldLightSync(RwObjectHasFrame* object)
{
    RpLight* light = (RpLight*)object;
    rpWorldLightExt* lightExt = LIGHTEXTFROMLIGHT(light);

    if (lightExt->originalSync(object))
    {
        RpWorld* world;
        RwFrame* lightFrame;

        if (RpLightGetType(light) < rpLIGHTPOSITIONINGSTART)
        {
            return object;
        }

        world = lightExt->world;
        lightFrame = RpLightGetFrame(light);

        if (world && lightFrame)
        {
            RwInt32 nStack = 0;
            RpSector* sect;
            RpSector* spaStack[rpWORLDMAXBSPDEPTH];
            RwV3d inf;
            RwV3d sup;
            RwReal radius = light->radius;

            /* Remove the light from all the sectors it was in */
            LightDestroyTies(light);

            inf = RwFrameGetLTM(lightFrame)->pos;
            sup = inf;

            sup.x += radius;
            sup.y += radius;
            sup.z += radius;

            inf.x -= radius;
            inf.y -= radius;
            inf.z -= radius;

            sect = world->rootSector;

            do
            {
                if (sect->type < 0)
                {
                    RpLightTie* tie =
                        (RpLightTie*)RWSRCGLOBAL(memoryAlloc)(RWWORLDOBJGLOBAL(lightTieFreeList));

                    tie->sect = (RpWorldSector*)sect;
                    tie->light = light;

                    rwLinkListAddLLLink(&((RpWorldSector*)sect)->lightsInWorldSector,
                                        &tie->lWorldSector);
                    rwLinkListAddLLLink(&light->WorldSectorsInLight, &tie->lLight);

                    sect = spaStack[nStack--];
                }
                else
                {
                    RpPlaneSector* pspPlane = (RpPlaneSector*)sect;

                    if (GETCOORD(inf, pspPlane->type) < pspPlane->leftValue)
                    {
                        sect = pspPlane->leftSubTree;

                        if (pspPlane->rightValue < GETCOORD(sup, pspPlane->type))
                        {
                            spaStack[++nStack] = pspPlane->rightSubTree;
                        }
                    }
                    else
                    {
                        if (pspPlane->rightValue < GETCOORD(sup, pspPlane->type))
                        {
                            sect = pspPlane->rightSubTree;
                        }
                        else
                        {
                            sect = spaStack[nStack--];
                        }
                    }
                }
            } while (nStack >= 0);
        }
    }
    else
    {
        RWERROR((E_RW_FRAMESYNCFAILED));
    }

    return object;
}

static void* WorldInitLightExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpLight* light = (RpLight*)object;
    rpWorldLightExt* lightExt = LIGHTEXTFROMLIGHT(light);

    lightExt->world = (RpWorld*)NULL;
    lightExt->originalSync = light->object.sync;
    light->object.sync = WorldLightSync;

    return object;
}

static void* WorldCopyLightExt(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                               RwInt32 sizeInObject)
{
    if (LIGHTEXTFROMLIGHT(srcObject)->world)
    {
        RpWorldAddLight(LIGHTEXTFROMLIGHT(srcObject)->world, (RpLight*)dstObject);
    }

    return dstObject;
}

static void* WorldDeInitLightExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpLight* light = (RpLight*)object;

    LightDestroyTies(light);

    return object;
}

static RpWorld* WorldSyncCamera(RpWorld* world, RwCamera* camera)
{
    RpSector* spaStack[rpWORLDMAXBSPDEPTH];
    RpSector* spSect;
    RwV3d vViewPoint;
    RwV3d inf;
    RwV3d sup;
    const RwFrustumPlane* plpPlanes;
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);
    RwBool goBackToFront;
    RwInt32 nStack = 0;
    RwInt32 position = 0;

    vViewPoint = RwFrameGetLTM(RwCameraGetFrame(camera))->pos;

    goBackToFront = (world->renderOrder == rpWORLDRENDERBACK2FRONT);

    plpPlanes = camera->frustumPlanes;

    inf = camera->frustumBoundBox.inf;
    sup = camera->frustumBoundBox.sup;

    spSect = cameraExt->world->rootSector;

    do
    {
        if (spSect->type < 0)
        {
            RpWorldSector* worldSector = (RpWorldSector*)spSect;
            RwBool outside;
            RwInt32 i;
            const RwV3d* const base = (const RwV3d*)&worldSector->boundingBox;

            for (i = 0; i < 6; i++)
            {
                const RwFrustumPlane* frustumPlane = &plpPlanes[i];
                RwV3d vCorner;
                RwSplitBits sbSide;

                vCorner.x = base[frustumPlane->closestX].x;
                vCorner.y = base[frustumPlane->closestY].y;
                vCorner.z = base[frustumPlane->closestZ].z;

                sbSide.nReal = RwV3dDotProductMacro(&vCorner, &frustumPlane->plane.normal) -
                               frustumPlane->plane.distance;

                outside = (sbSide.nInt > 0);
                if (outside)
                {
                    break;
                }
            }

            if (!outside)
            {
                if (position >= cameraExt->spaceInFrustumSectors)
                {
                    if (!SectorsInFrustumAddSpace(cameraExt, rpWORLDFRUSTUMSECTORSGRANULARITY))
                    {
                        cameraExt->numSectorsInFrustum = position;
                        return world;
                    }
                }

                cameraExt->frustumSectors[position++] = worldSector;
            }

            spSect = spaStack[nStack--];
        }
        else
        {
            RpPlaneSector* pspPlane = (RpPlaneSector*)spSect;
            RwSplitBits sbLeft;
            RwSplitBits sbRight;

            sbLeft.nReal = GETCOORD(inf, pspPlane->type) - pspPlane->leftValue;
            sbRight.nReal = pspPlane->rightValue - GETCOORD(sup, pspPlane->type);

            if ((sbLeft.nInt < 0) && (sbRight.nInt < 0))
            {
                RwBool viewPointIsHigher = (GETCOORD(vViewPoint, pspPlane->type) > pspPlane->value);

                if ((goBackToFront && viewPointIsHigher) || (!goBackToFront && !viewPointIsHigher))
                {
                    spaStack[++nStack] = pspPlane->rightSubTree;
                    spSect = pspPlane->leftSubTree;
                }
                else
                {
                    spaStack[++nStack] = pspPlane->leftSubTree;
                    spSect = pspPlane->rightSubTree;
                }
            }
            else
            {
                spSect = (sbLeft.nInt < 0) ? pspPlane->leftSubTree : pspPlane->rightSubTree;
            }
        }
    } while (nStack >= 0);

    cameraExt->numSectorsInFrustum = position;

    return world;
}

static RpAtomic* WorldAddClumpAtomic(RpAtomic* atomic, void* pData)
{
    RpWorldAddAtomic((RpWorld*)pData, atomic);

    return atomic;
}

static RpLight* WorldAddClumpLight(RpLight* light, void* pData)
{
    RpWorldAddLight((RpWorld*)pData, light);

    return light;
}

static RwCamera* WorldAddClumpCamera(RwCamera* camera, void* pData)
{
    RpWorldAddCamera((RpWorld*)pData, camera);

    return camera;
}

static RwStream* writeGeometryMesh(RwStream* stream, RwInt32 binaryLength, const void* object,
                                   RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpGeometry* geometry = (const RpGeometry*)object;

    return _rpMeshWrite(geometry->mesh, geometry, stream, &geometry->matList);
}

static RwStream* readGeometryMesh(RwStream* stream, RwInt32 binaryLength, void* object,
                                  RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpGeometry* geometry = (RpGeometry*)object;

    geometry->mesh = _rpMeshRead(stream, geometry, &geometry->matList);

    if (!geometry->mesh)
    {
        return (RwStream*)NULL;
    }

    return stream;
}

static RwInt32 sizeGeometryMesh(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpGeometry* geometry = (const RpGeometry*)object;

    return _rpMeshSize(geometry->mesh, geometry);
}

static RwStream* writeGeometryNative(RwStream* stream, RwInt32 binaryLength, const void* object,
                                     RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return _rpGeometryNativeWrite(stream, (const RpGeometry*)object);
}

static RwStream* readGeometryNative(RwStream* stream, RwInt32 binaryLength, void* object,
                                    RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    if (!_rpGeometryNativeRead(stream, (RpGeometry*)object))
    {
        return (RwStream*)NULL;
    }

    return stream;
}

static RwInt32 sizeGeometryNative(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return _rpGeometryNativeSize((const RpGeometry*)object);
}

static RwStream* writeWorldSectorNative(RwStream* stream, RwInt32 binaryLength, const void* object,
                                        RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return _rpWorldSectorNativeWrite(stream, (const RpWorldSector*)object);
}

static RwStream* readWorldSectorNative(RwStream* stream, RwInt32 binaryLength, void* object,
                                       RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    if (!_rpWorldSectorNativeRead(stream, (RpWorldSector*)object))
    {
        return (RwStream*)NULL;
    }

    return stream;
}

static RwInt32 sizeWorldSectorNative(const void* object, RwInt32 offsetInObject,
                                     RwInt32 sizeInObject)
{
    return _rpWorldSectorNativeSize((const RpWorldSector*)object);
}

static RwStream* writeSectorMesh(RwStream* stream, RwInt32 binaryLength, const void* object,
                                 RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpWorldSector* sector = (const RpWorldSector*)object;
    const RpWorld* world = RpWorldSectorGetWorld(sector);

    return _rpMeshWrite(sector->mesh, world, stream, &world->matList);
}

static RwStream* readSectorMesh(RwStream* stream, RwInt32 binaryLength, void* object,
                                RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpWorldSector* sector = (RpWorldSector*)object;
    RpWorld* world = RpWorldSectorGetWorld(sector);

    sector->mesh = _rpMeshRead(stream, world, &world->matList);

    if (!sector->mesh)
    {
        return (RwStream*)NULL;
    }

    return stream;
}

static RwInt32 sizeSectorMesh(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpWorldSector* sector = (const RpWorldSector*)object;

    return _rpMeshSize(sector->mesh, RpWorldSectorGetWorld(sector));
}

RwBool _rpWorldObjRegisterExtensions(void)
{
    RwInt32 status;

    status = RwEngineRegisterPlugin(sizeof(rpWorldObjGlobals), rwID_WORLDOBJMODULE, WorldObjectOpen,
                                    WorldObjectClose);

    cameraExtOffset =
        RwCameraRegisterPlugin(sizeof(rpWorldCameraExt), rwID_WORLDOBJMODULE, WorldInitCameraExt,
                               WorldDeInitCameraExt, WorldCopyCameraExt);
    status |= cameraExtOffset;

    atomicExtOffset =
        RpAtomicRegisterPlugin(sizeof(rpWorldAtomicExt), rwID_WORLDOBJMODULE, WorldInitAtomicExt,
                               WorldDeInitAtomicExt, WorldCopyAtomicExt);
    status |= atomicExtOffset;

    clumpExtOffset =
        RpClumpRegisterPlugin(sizeof(rpWorldClumpExt), rwID_WORLDOBJMODULE, WorldInitClumpExt,
                              WorldDeInitClumpExt, WorldCopyClumpExt);
    status |= clumpExtOffset;

    lightExtOffset =
        RpLightRegisterPlugin(sizeof(rpWorldLightExt), rwID_WORLDOBJMODULE, WorldInitLightExt,
                              WorldDeInitLightExt, WorldCopyLightExt);
    status |= lightExtOffset;

    /* Mesh data */
    status |= RpGeometryRegisterPlugin(0, rwID_BINMESHPLUGIN, (RwPluginObjectConstructor)NULL,
                                       (RwPluginObjectDestructor)NULL, (RwPluginObjectCopy)NULL);
    status |= RpWorldSectorRegisterPlugin(0, rwID_BINMESHPLUGIN, (RwPluginObjectConstructor)NULL,
                                          (RwPluginObjectDestructor)NULL, (RwPluginObjectCopy)NULL);
    status |= RpGeometryRegisterPluginStream(rwID_BINMESHPLUGIN, readGeometryMesh,
                                             writeGeometryMesh, sizeGeometryMesh);
    status |= RpWorldSectorRegisterPluginStream(rwID_BINMESHPLUGIN, readSectorMesh, writeSectorMesh,
                                                sizeSectorMesh);

    /* Native data */
    status |= RpGeometryRegisterPlugin(0, rwID_NATIVEDATAPLUGIN, (RwPluginObjectConstructor)NULL,
                                       (RwPluginObjectDestructor)NULL, (RwPluginObjectCopy)NULL);
    status |= RpWorldSectorRegisterPlugin(0, rwID_NATIVEDATAPLUGIN, (RwPluginObjectConstructor)NULL,
                                          (RwPluginObjectDestructor)NULL, (RwPluginObjectCopy)NULL);
    status |= RpGeometryRegisterPluginStream(rwID_NATIVEDATAPLUGIN, readGeometryNative,
                                             writeGeometryNative, sizeGeometryNative);
    status |= RpWorldSectorRegisterPluginStream(rwID_NATIVEDATAPLUGIN, readWorldSectorNative,
                                                writeWorldSectorNative, sizeWorldSectorNative);

    /* Right to render */
    status |= RpAtomicRegisterPlugin(0, rwID_RIGHTTORENDER, (RwPluginObjectConstructor)NULL,
                                     (RwPluginObjectDestructor)NULL, (RwPluginObjectCopy)NULL);
    status |= RpAtomicRegisterPluginStream(rwID_RIGHTTORENDER, _rpReadAtomicRights,
                                           _rpWriteAtomicRights, _rpSizeAtomicRights);
    status |= RpWorldRegisterPlugin(0, rwID_RIGHTTORENDER, (RwPluginObjectConstructor)NULL,
                                    (RwPluginObjectDestructor)NULL, (RwPluginObjectCopy)NULL);
    status |= RpWorldRegisterPluginStream(rwID_RIGHTTORENDER, _rpReadWorldRights,
                                          _rpWriteWorldRights, _rpSizeWorldRights);
    status |= RpWorldSectorRegisterPlugin(0, rwID_RIGHTTORENDER, (RwPluginObjectConstructor)NULL,
                                          (RwPluginObjectDestructor)NULL, (RwPluginObjectCopy)NULL);
    status |= RpWorldSectorRegisterPluginStream(rwID_RIGHTTORENDER, _rpReadSectRights,
                                                _rpWriteSectRights, _rpSizeSectRights);
    status |= RpMaterialRegisterPlugin(0, rwID_RIGHTTORENDER, (RwPluginObjectConstructor)NULL,
                                       (RwPluginObjectDestructor)NULL, (RwPluginObjectCopy)NULL);
    status |= RpMaterialRegisterPluginStream(rwID_RIGHTTORENDER, _rpReadMaterialRights,
                                             _rpWriteMaterialRights, _rpSizeMaterialRights);

    if (status < 0)
    {
        return FALSE;
    }

    return _rpWorldPipeAttach() ? TRUE : FALSE;
}

RpWorld* RpWorldAddCamera(RpWorld* world, RwCamera* camera)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    if (RwCameraGetFrame(camera))
    {
        RwFrameUpdateObjects(RwCameraGetFrame(camera));
    }

    cameraExt->world = world;

    return world;
}

RpWorld* RpWorldRemoveCamera(RpWorld* world, RwCamera* camera)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    if (cameraExt->world)
    {
        if (cameraExt->frustumSectors)
        {
            RwFree(cameraExt->frustumSectors);
        }

        cameraExt->frustumSectors = (RpWorldSector**)NULL;
        cameraExt->spaceInFrustumSectors = 0;
        cameraExt->numSectorsInFrustum = 0;
        cameraExt->world = (RpWorld*)NULL;

        return world;
    }

    return (RpWorld*)NULL;
}

RpWorld* RwCameraGetWorld(const RwCamera* camera)
{
    return CAMERAEXTFROMCAMERA(camera)->world;
}

RpWorld* RpWorldAddAtomic(RpWorld* world, RpAtomic* atomic)
{
    rpWorldAtomicExt* atomicExt = ATOMICEXTFROMATOMIC(atomic);

    if (RpAtomicGetFrame(atomic))
    {
        RwFrameUpdateObjects(RpAtomicGetFrame(atomic));
    }

    atomicExt->world = world;

    return world;
}

RpWorld* RpAtomicGetWorld(const RpAtomic* atomic)
{
    return ATOMICEXTFROMATOMIC(atomic)->world;
}

RpAtomic* RpAtomicForAllWorldSectors(RpAtomic* atomic, RpWorldSectorCallBack callback, void* pData)
{
    RwLLLink* cur;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
    end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
    while (cur != end)
    {
        RpTie* tie = rwLLLinkGetData(cur, RpTie, lAtomic);
        RwLLLink* next = rwLLLinkGetNext(cur);

        if (!callback(tie->worldSector, pData))
        {
            return atomic;
        }

        cur = next;
    }

    return atomic;
}

RpWorld* RpWorldAddClump(RpWorld* world, RpClump* clump)
{
    rpWorldClumpExt* clumpExt = CLUMPEXTFROMCLUMP(clump);
    RwFrame* frame = RpClumpGetFrame(clump);

    rwLinkListAddLLLink(&world->clumpList, &clump->inWorldLink);
    world->numClumpsInWorld++;
    clumpExt->world = world;

    RpClumpForAllAtomics(clump, WorldAddClumpAtomic, world);
    RpClumpForAllLights(clump, WorldAddClumpLight, world);
    RpClumpForAllCameras(clump, WorldAddClumpCamera, world);

    if (frame)
    {
        RwMatrixOptimize(RwFrameGetMatrix(frame), (const RwMatrixTolerance*)NULL);
        RwFrameUpdateObjects(frame);
    }

    clumpExt->worldSectorPool = RWWORLDOBJGLOBAL(worldSectorPool);

    return world;
}

RwCamera* RwCameraForAllSectorsInFrustum(RwCamera* camera, RpWorldSectorCallBack callBack,
                                         void* pData)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);
    RpWorldSector** spSect = cameraExt->frustumSectors;
    RwInt32 numSect = cameraExt->numSectorsInFrustum;

    while (numSect)
    {
        if (!callBack(*spSect, pData))
        {
            return camera;
        }

        spSect++;
        numSect--;
    }

    return camera;
}

RpWorld* RpWorldAddLight(RpWorld* world, RpLight* light)
{
    LIGHTEXTFROMLIGHT(light)->world = world;

    if (rwObjectGetSubType(light) < rpLIGHTPOSITIONINGSTART)
    {
        rwLinkListAddLLLink(&world->directionalLightList, &light->inWorld);
    }
    else
    {
        if (RpLightGetFrame(light))
        {
            RwFrameUpdateObjects(RpLightGetFrame(light));
        }

        rwLinkListAddLLLink(&world->lightList, &light->inWorld);
    }

    return world;
}

RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light)
{
    LIGHTEXTFROMLIGHT(light)->world = (RpWorld*)NULL;

    LightDestroyTies(light);

    rwLinkListRemoveLLLink(&light->inWorld);

    return world;
}
