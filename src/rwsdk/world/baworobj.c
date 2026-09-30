#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rpATOMICPRIVATEWORLDBOUNDDIRTY 0x01

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

extern RwCamera* WorldCameraSync(RwCamera* camera);
extern RpLight* WorldLightSync(RpLight* light);
extern void WorldAttachAtomicSphere(RpWorld* world, RpAtomic* atomic);
extern RwObjectHasFrame* WorldAtomicSync(RwObjectHasFrame* object);

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

static void* WorldCopyAtomicExt(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                                RwInt32 sizeInObject)
{
    return dstObject;
}

static void* WorldDeInitClumpExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

RpWorld* RpAtomicGetWorld(const RpAtomic* atomic)
{
    return ATOMICEXTFROMATOMIC(atomic)->world;
}

RpWorld* RwCameraGetWorld(const RwCamera* camera)
{
    return CAMERAEXTFROMCAMERA(camera)->world;
}

static RwInt32 sizeGeometryNative(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return _rpGeometryNativeSize((const RpGeometry*)object);
}

static RwInt32 sizeWorldSectorNative(const void* object, RwInt32 offsetInObject,
                                     RwInt32 sizeInObject)
{
    return _rpWorldSectorNativeSize((const RpWorldSector*)object);
}

static RwStream* writeGeometryNative(RwStream* stream, RwInt32 binaryLength, const void* object,
                                     RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return _rpGeometryNativeWrite(stream, (const RpGeometry*)object);
}

static RwStream* writeWorldSectorNative(RwStream* stream, RwInt32 binaryLength, const void* object,
                                        RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return _rpWorldSectorNativeWrite(stream, (const RpWorldSector*)object);
}

static void* WorldInitClumpExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    rpWorldClumpExt* clumpExt = CLUMPEXTFROMCLUMP(object);

    clumpExt->world = (RpWorld*)NULL;
    clumpExt->worldSectorPool = RWWORLDOBJGLOBAL(worldSectorPool);

    return object;
}

static void* WorldInitLightExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpLight* light = (RpLight*)object;
    rpWorldLightExt* lightExt = LIGHTEXTFROMLIGHT(light);

    lightExt->world = (RpWorld*)NULL;
    lightExt->originalSync = light->object.sync;
    light->object.sync = (RwObjectHasFrameSyncFunction)WorldLightSync;

    return object;
}

static RwInt32 sizeGeometryMesh(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpGeometry* geometry = (const RpGeometry*)object;

    return _rpMeshSize(geometry->mesh, geometry);
}

static RwStream* writeGeometryMesh(RwStream* stream, RwInt32 binaryLength, const void* object,
                                   RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpGeometry* geometry = (const RpGeometry*)object;

    return _rpMeshWrite(geometry->mesh, geometry, stream, &geometry->matList);
}

static RpAtomic* WorldAddClumpAtomic(RpAtomic* atomic, void* pData)
{
    RpWorldAddAtomic((RpWorld*)pData, atomic);

    return atomic;
}

static RwCamera* WorldAddClumpCamera(RwCamera* camera, void* pData)
{
    RpWorldAddCamera((RpWorld*)pData, camera);

    return camera;
}

static RpLight* WorldAddClumpLight(RpLight* light, void* pData)
{
    RpWorldAddLight((RpWorld*)pData, light);

    return light;
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

static RwInt32 sizeSectorMesh(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpWorldSector* sector = (const RpWorldSector*)object;
    const RpWorld* world = RpWorldSectorGetWorld(sector);

    return _rpMeshSize(sector->mesh, world);
}

static RwCamera* WorldCameraEndUpdate(RwCamera* camera)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    RWSRCGLOBAL(curWorld) = NULL;

    return cameraExt->originalEndUpdate(camera);
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

static void* WorldCopyLightExt(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                               RwInt32 sizeInObject)
{
    if (LIGHTEXTFROMLIGHT(srcObject)->world)
    {
        RpWorldAddLight(LIGHTEXTFROMLIGHT(srcObject)->world, (RpLight*)dstObject);
    }

    return dstObject;
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

static RwStream* readWorldSectorNative(RwStream* stream, RwInt32 binaryLength, void* object,
                                       RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    if (!_rpWorldSectorNativeRead(stream, (RpWorldSector*)object))
    {
        return (RwStream*)NULL;
    }

    return stream;
}

static RwCamera* WorldCameraBeginUpdate(RwCamera* camera)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    RWSRCGLOBAL(curWorld) = cameraExt->world;
    RWSRCGLOBAL(renderFrame)++;

    return cameraExt->originalBeginUpdate(camera);
}

static RwStream* writeSectorMesh(RwStream* stream, RwInt32 binaryLength, const void* object,
                                 RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpWorldSector* sector = (const RpWorldSector*)object;
    const RpWorld* world = RpWorldSectorGetWorld(sector);

    return _rpMeshWrite(sector->mesh, world, stream, &world->matList);
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

static void* WorldCopyCameraExt(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                                RwInt32 sizeInObject)
{
    rpWorldCameraExt* cameraExt = CAMERAEXTFROMCAMERA(dstObject);
    RpWorld* world = CAMERAEXTFROMCAMERA(srcObject)->world;

    cameraExt->frustumSectors = (RpWorldSector**)NULL;
    cameraExt->spaceInFrustumSectors = 0;
    cameraExt->numSectorsInFrustum = 0;

    if (world)
    {
        RpWorldAddCamera(world, (RwCamera*)dstObject);
    }

    return dstObject;
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

    camera->object.sync = (RwObjectHasFrameSyncFunction)WorldCameraSync;
    camera->beginUpdate = WorldCameraBeginUpdate;
    camera->endUpdate = WorldCameraEndUpdate;

    cameraExt->world = (RpWorld*)NULL;

    return object;
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

RwBool _rpLightTieDestroy(RpLightTie* tie)
{
    rwLinkListRemoveLLLink(&tie->lLight);
    rwLinkListRemoveLLLink(&tie->lWorldSector);

    RWSRCGLOBAL(memoryFree)(RWWORLDOBJGLOBAL(lightTieFreeList), tie);

    return TRUE;
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

RwObjectHasFrame* WorldAtomicSync(RwObjectHasFrame* object)
{
    RpAtomic* atomic = (RpAtomic*)object;
    rpWorldAtomicExt* atomicExt = ATOMICEXTFROMATOMIC(atomic);

    if (!atomicExt->originalSync(object))
    {
        return (RwObjectHasFrame*)NULL;
    }

    if (atomicExt->world)
    {
        RwLLLink* cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
        RwLLLink* end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);

        while (cur != end)
        {
            RpTie* tie = rwLLLinkGetData(cur, RpTie, lAtomic);

            cur = rwLLLinkGetNext(cur);
            _rpTieDestroy(tie);
        }

        WorldAttachAtomicSphere(atomicExt->world, atomic);
    }

    return object;
}

static void* WorldDeInitAtomicExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpAtomic* atomic = (RpAtomic*)object;
    rpWorldAtomicExt* atomicExt = ATOMICEXTFROMATOMIC(atomic);
    RwLLLink* cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
    RwLLLink* end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);

    while (cur != end)
    {
        RpTie* tie = rwLLLinkGetData(cur, RpTie, lAtomic);

        cur = rwLLLinkGetNext(cur);
        _rpTieDestroy(tie);
    }

    atomic->object.sync = atomicExt->originalSync;

    return object;
}

static void* WorldDeInitLightExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RpLight* light = (RpLight*)object;
    RwLLLink* cur = rwLinkListGetFirstLLLink(&light->WorldSectorsInLight);
    RwLLLink* end = rwLinkListGetTerminator(&light->WorldSectorsInLight);

    while (cur != end)
    {
        RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, lLight);
        RwLLLink* next = rwLLLinkGetNext(cur);

        _rpLightTieDestroy(tie);
        cur = next;
    }

    return object;
}

RpWorld* RpWorldRemoveLight(RpWorld* world, RpLight* light)
{
    RwLLLink* cur;
    RwLLLink* end;

    LIGHTEXTFROMLIGHT(light)->world = (RpWorld*)NULL;

    cur = rwLinkListGetFirstLLLink(&light->WorldSectorsInLight);
    end = rwLinkListGetTerminator(&light->WorldSectorsInLight);
    while (cur != end)
    {
        RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, lLight);
        RwLLLink* next = rwLLLinkGetNext(cur);

        _rpLightTieDestroy(tie);
        cur = next;
    }

    rwLinkListRemoveLLLink(&light->inWorld);

    return world;
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
