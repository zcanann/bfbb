#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwSECTORATOMIC (-1)
#define rwSECTORBUILD (-2)

#define rpWORLDMAXBSPDEPTH 64

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

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

typedef struct rpWorldListEntry rpWorldListEntry;
struct rpWorldListEntry
{
    RpWorld* world;
    RwUInt32 size;
    RwLLLink link;
};

typedef struct rpWorldGlobals rpWorldGlobals;
struct rpWorldGlobals
{
    RwFreeList* worldListFreeList;
    RwLinkList worldList;
};

typedef struct rpFindSectorData rpFindSectorData;
struct rpFindSectorData
{
    const RpWorldSector* sector;
    RwBool found;
};

typedef struct rxPipelineGlobalVars rxPipelineGlobalVars;
struct rxPipelineGlobalVars
{
    RwUInt8 pad[0x40];
    RxPipeline* platformWorldSectorPipeline;
};

extern RwInt32 _rxPipelineGlobalsOffset;

extern RwBool _rpWorldPipelineOpen(void);
extern RwBool _rpWorldPipelineClose(void);
extern RwBool _rpTieDestroy(RpTie* tie);
extern RwBool _rpLightTieDestroy(RpLightTie* tie);
extern RpWorldSector* WorldSectorRender(RpWorldSector* sector, void* pData);
extern RpWorld* WorldBuildMeshAtomicSector(RpWorld* world, RpBuildMesh* buildMesh,
                                           RpWorldSector* sector, RpMaterial** matList);
extern void WorldSectorDestroyRecurse(RpSector* sector);

static RwModuleInfo worldModule;

static RwFreeList _rpWorldListFreeList;

static RwPluginRegistry worldTKList = { sizeof(RpWorld),         sizeof(RpWorld),        0, 0,
                                        (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

extern RwPluginRegistry sectorTKList;

#define RWWORLDGLOBAL(var)                                                                         \
    (RWPLUGINOFFSET(rpWorldGlobals, RwEngineInstance, worldModule.globalsOffset)->var)

#define RXPIPELINEGLOBAL(var)                                                                      \
    (RWPLUGINOFFSET(rxPipelineGlobalVars, RwEngineInstance, _rxPipelineGlobalsOffset)->var)

static RpWorldSector* WorldFindSector(RpWorldSector* sector, void* pData)
{
    rpFindSectorData* findData = (rpFindSectorData*)pData;

    if (findData->sector != sector)
    {
        return sector;
    }

    findData->found = TRUE;

    return (RpWorldSector*)NULL;
}

void* WorldClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWWORLDGLOBAL(worldListFreeList))
    {
        RwFreeListDestroy(RWWORLDGLOBAL(worldListFreeList));
        RWWORLDGLOBAL(worldListFreeList) = (RwFreeList*)NULL;
    }

    _rpWorldPipelineClose();

    worldModule.numInstances--;

    return instance;
}

void* WorldOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    worldModule.globalsOffset = offset;

    if (!_rpWorldPipelineOpen())
    {
        return NULL;
    }

    RWWORLDGLOBAL(worldListFreeList) = RwFreeListCreateAndPreallocateSpace(
        sizeof(rpWorldListEntry), 8, 4, 1, &_rpWorldListFreeList);
    if (!RWWORLDGLOBAL(worldListFreeList))
    {
        return NULL;
    }

    rwLinkListInitialize(&RWWORLDGLOBAL(worldList));

    worldModule.numInstances++;

    return instance;
}

RpWorldSector* _rpSectorDefaultRenderCallBack(RpWorldSector* sector)
{
    RxPipeline* pipeline;

    if (!sector->numPolygons)
    {
        return sector;
    }

    if (sector->pipeline)
    {
        pipeline = sector->pipeline;
    }
    else if (((RpWorld*)RWSRCGLOBAL(curWorld))->pipeline)
    {
        pipeline = ((RpWorld*)RWSRCGLOBAL(curWorld))->pipeline;
    }
    else
    {
        pipeline = RXPIPELINEGLOBAL(platformWorldSectorPipeline);
    }

    if (RxPipelineExecute(pipeline, sector, TRUE))
    {
        return sector;
    }

    return (RpWorldSector*)NULL;
}

RwBool _rpWorldFindBBox(RpWorld* world, RwBBox* boundBox)
{
    RpSector* spaStack[rpWORLDMAXBSPDEPTH];
    RpSector* spSect = world->rootSector;
    RwInt32 numSectors = 0;
    RwBool initialized = FALSE;

    while (1)
    {
        if (spSect->type < 0)
        {
            RpWorldSector* worldSector = (RpWorldSector*)spSect;

            if (!initialized)
            {
                RwBBoxInitialize(boundBox, &worldSector->tightBoundingBox.inf);
                initialized = TRUE;
            }
            else
            {
                RwBBoxAddPoint(boundBox, &worldSector->tightBoundingBox.inf);
            }

            RwBBoxAddPoint(boundBox, &worldSector->tightBoundingBox.sup);

            spSect = spaStack[numSectors];
            numSectors--;
        }
        else
        {
            RpPlaneSector* planeSector = (RpPlaneSector*)spSect;

            numSectors++;
            spSect = planeSector->leftSubTree;
            spaStack[numSectors] = planeSector->rightSubTree;
        }

        if (numSectors < 0)
        {
            break;
        }
    }

    return TRUE;
}

void _rpWorldRegisterWorld(RpWorld* world, RwUInt32 memorySize)
{
    rpWorldListEntry* entry;

    entry = (rpWorldListEntry*)RWSRCGLOBAL(memoryAlloc)(RWWORLDGLOBAL(worldListFreeList));
    if (entry)
    {
        entry->world = world;
        entry->size = memorySize;

        rwLinkListAddLLLink(&RWWORLDGLOBAL(worldList), &entry->link);
    }
}

void _rpWorldUnregisterWorld(RpWorld* world)
{
    RwLLLink* cur;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink(&RWWORLDGLOBAL(worldList));
    end = rwLinkListGetTerminator(&RWWORLDGLOBAL(worldList));
    while (cur != end)
    {
        rpWorldListEntry* entry = rwLLLinkGetData(cur, rpWorldListEntry, link);

        if (entry->world == world)
        {
            rwLinkListRemoveLLLink(&entry->link);

            RWSRCGLOBAL(memoryFree)(RWWORLDGLOBAL(worldListFreeList), entry);

            return;
        }

        cur = rwLLLinkGetNext(cur);
    }
}

RpWorld* RpWorldUnlock(RpWorld* world)
{
    RpSector* spaStack[rpWORLDMAXBSPDEPTH];
    RpSector* spSect = world->rootSector;
    RwInt32 numSectors = 0;

    while (1)
    {
        if (spSect->type < 0)
        {
            RpWorldSector* worldSector = (RpWorldSector*)spSect;
            RpMaterial** matList = world->matList.materials + worldSector->matListWindowBase;

            if (!worldSector->mesh)
            {
                RpBuildMesh* buildMesh = _rpBuildMeshCreate(worldSector->numPolygons);

                if (!buildMesh)
                {
                    return (RpWorld*)NULL;
                }

                world = WorldBuildMeshAtomicSector(world, buildMesh, worldSector, matList);

                if (!world)
                {
                    return (RpWorld*)NULL;
                }
            }

            spSect = spaStack[numSectors];
            numSectors--;
        }
        else
        {
            RpPlaneSector* planeSector = (RpPlaneSector*)spSect;

            numSectors++;
            spSect = planeSector->leftSubTree;
            spaStack[numSectors] = planeSector->rightSubTree;
        }

        if (numSectors < 0)
        {
            return world;
        }
    }
}

static void WorldSectorDeinstanceAll(RpSector* sector)
{
    if (sector->type == rwSECTORATOMIC)
    {
        RpWorldSector* worldSector = (RpWorldSector*)sector;
        RwLLLink* cur;
        RwLLLink* end;

        if (worldSector->repEntry)
        {
            RwResourcesFreeResEntry(worldSector->repEntry);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->collAtomicsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->collAtomicsInWorldSector);
        while (cur != end)
        {
            RpTie* tie = rwLLLinkGetData(cur, RpTie, lWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpTieDestroy(tie);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->noCollAtomicsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->noCollAtomicsInWorldSector);
        while (cur != end)
        {
            RpTie* tie = rwLLLinkGetData(cur, RpTie, lWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpTieDestroy(tie);
        }

        cur = rwLinkListGetFirstLLLink(&worldSector->lightsInWorldSector);
        end = rwLinkListGetTerminator(&worldSector->lightsInWorldSector);
        while (cur != end)
        {
            RpLightTie* tie = rwLLLinkGetData(cur, RpLightTie, lWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpLightTieDestroy(tie);
        }

        _rwPluginRegistryDeInitObject(&sectorTKList, worldSector);
    }
    else if (sector->type != rwSECTORBUILD)
    {
        RpPlaneSector* planeSector = (RpPlaneSector*)sector;

        WorldSectorDeinstanceAll(planeSector->leftSubTree);
        WorldSectorDeinstanceAll(planeSector->rightSubTree);
    }
}

RpWorld* RpWorldRender(RpWorld* world)
{
    RwCameraForAllSectorsInFrustum((RwCamera*)RWSRCGLOBAL(curCamera), WorldSectorRender, world);

    return world;
}

RpWorld* RpWorldSectorGetWorld(const RpWorldSector* sector)
{
    RwLLLink* cur;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink(&RWWORLDGLOBAL(worldList));
    end = rwLinkListGetTerminator(&RWWORLDGLOBAL(worldList));
    while (cur != end)
    {
        rpWorldListEntry* entry = rwLLLinkGetData(cur, rpWorldListEntry, link);
        RpWorld* world = entry->world;

        if (rwObjectTestPrivateFlags(world, rpWORLDSINGLEMALLOC))
        {
            if ((sector >= (const RpWorldSector*)world) &&
                (sector < (const RpWorldSector*)((RwUInt8*)world + entry->size)))
            {
                return world;
            }
        }
        else
        {
            rpFindSectorData findData;

            findData.sector = sector;
            findData.found = FALSE;

            RpWorldForAllWorldSectors(world, WorldFindSector, &findData);

            if (findData.found)
            {
                return entry->world;
            }
        }

        cur = rwLLLinkGetNext(cur);
    }

    return (RpWorld*)NULL;
}

RpWorld* RpWorldForAllWorldSectors(RpWorld* world, RpWorldSectorCallBack fpCallBack, void* pData)
{
    RpSector* spaStack[rpWORLDMAXBSPDEPTH];
    RwInt32 numSectors = 0;
    RpSector* spSect = world->rootSector;

    while (1)
    {
        if (spSect->type < 0)
        {
            if (!fpCallBack((RpWorldSector*)spSect, pData))
            {
                return world;
            }

            spSect = spaStack[numSectors];
            numSectors--;
        }
        else
        {
            RpPlaneSector* planeSector = (RpPlaneSector*)spSect;

            numSectors++;
            spSect = planeSector->leftSubTree;
            spaStack[numSectors] = planeSector->rightSubTree;
        }

        if (numSectors < 0)
        {
            break;
        }
    }

    return world;
}

RwInt32 RpWorldRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&worldTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);
}

RwInt32 RpWorldRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    return _rwPluginRegistryAddPluginStream(&worldTKList, pluginID, readCB, writeCB, getSizeCB);
}

RpWorld* RpWorldSetSectorRenderCallBack(RpWorld* world, RpWorldSectorCallBackRender fpCallBack)
{
    if (!fpCallBack)
    {
        fpCallBack = _rpSectorDefaultRenderCallBack;
    }

    world->renderCallBack = fpCallBack;

    return world;
}
