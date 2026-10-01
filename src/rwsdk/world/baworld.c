#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include <string.h>

#define rwSECTORATOMIC (-1)
#define rwSECTORBUILD (-2)

#define rpWORLDMAXBSPDEPTH 64

#define rwMAXTEXTURECOORDS 8

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

/* Plane sector types are byte offsets of the split axis within an RwV3d */
#define SETCOORD(vect, axis, value) (((RwReal*)(((RwUInt8*)(&(vect))) + (axis)))[0] = (value))

#define MAKECHUNKID(vendorID, chunkID) (((vendorID & 0xFFFFFF) << 8) | (chunkID & 0xFF))

#define rwID_MATERIALMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x01)
#define rwID_MESHMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x02)
#define rwID_GEOMETRYMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x03)
#define rwID_CLUMPMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x04)
#define rwID_LIGHTMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x05)
#define rwID_WORLDMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x07)
#define rwID_SECTORMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x0A)
#define rwID_BINWORLDMODULE MAKECHUNKID(rwVENDORID_CRITERIONWORLD, 0x0B)

/* Sizes of the engine globals reserved by the other world modules */
#define rpMATERIALGLOBALSSIZE sizeof(RwFreeList*)
#define rpMESHGLOBALSSIZE 0x30
#define rpGEOMETRYGLOBALSSIZE sizeof(RwInt32)
#define rpCLUMPGLOBALSSIZE (sizeof(RwFreeList*) * 2)
#define rpLIGHTGLOBALSSIZE sizeof(RwFreeList*)

#define rpMESHHEADERTRISTRIP 0x0001

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
extern void* _rpMaterialOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpMaterialClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpGeometryOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpGeometryClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpClumpOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpClumpClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpLightOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpLightClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpSectorOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpSectorClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpBinaryWorldOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpBinaryWorldClose(void* instance, RwInt32 offset, RwInt32 size);
extern RwBool _rpWorldObjRegisterExtensions(void);
extern RwBool _rpClumpRegisterExtensions(void);
extern RwBool _rxWorldDevicePluginAttach(void);

static RwModuleInfo worldModule;

static RwFreeList _rpWorldListFreeList;

RwPluginRegistry worldTKList = { sizeof(RpWorld),         sizeof(RpWorld),        0, 0,
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

static void WorldSectorRenderAtomics(RpWorldSector* worldSector)
{
    RwLLLink* cur;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink(&worldSector->collAtomicsInWorldSector);
    end = rwLinkListGetTerminator(&worldSector->collAtomicsInWorldSector);
    while (cur != end)
    {
        RpTie* tie = rwLLLinkGetData(cur, RpTie, lWorldSector);
        RpAtomic* atomic = tie->apAtom;

        if (rwObjectTestFlags(atomic, rpATOMICRENDER))
        {
            if (atomic->renderFrame != RWSRCGLOBAL(renderFrame))
            {
                const RwSphere* atomicBoundingSphere = RpAtomicGetWorldBoundingSphere(atomic);

                if (RwCameraFrustumTestSphere((RwCamera*)RWSRCGLOBAL(curCamera),
                                              atomicBoundingSphere) != rwSPHEREOUTSIDE)
                {
                    RpAtomicRender(atomic);
                }

                atomic->renderFrame = RWSRCGLOBAL(renderFrame);
            }
        }

        cur = rwLLLinkGetNext(cur);
    }

    cur = rwLinkListGetFirstLLLink(&worldSector->noCollAtomicsInWorldSector);
    end = rwLinkListGetTerminator(&worldSector->noCollAtomicsInWorldSector);
    while (cur != end)
    {
        RpTie* tie = rwLLLinkGetData(cur, RpTie, lWorldSector);
        RpAtomic* atomic = tie->apAtom;

        if (rwObjectTestFlags(atomic, rpATOMICRENDER))
        {
            if (atomic->renderFrame != RWSRCGLOBAL(renderFrame))
            {
                const RwSphere* atomicBoundingSphere = RpAtomicGetWorldBoundingSphere(atomic);

                if (RwCameraFrustumTestSphere((RwCamera*)RWSRCGLOBAL(curCamera),
                                              atomicBoundingSphere) != rwSPHEREOUTSIDE)
                {
                    RpAtomicRender(atomic);
                }

                atomic->renderFrame = RWSRCGLOBAL(renderFrame);
            }
        }

        cur = rwLLLinkGetNext(cur);
    }
}

static RpWorldSector* WorldSectorRender(RpWorldSector* sector, void* data)
{
    if (RpWorldSectorRender(sector))
    {
        WorldSectorRenderAtomics(sector);
    }

    return sector;
}

static RpWorld* WorldBuildMeshAtomicSector(RpWorld* world, RpBuildMesh* buildMesh,
                                           RpWorldSector* worldSector, RpMaterial** matBase)
{
    RpMeshHeader* mesh;
    RwInt32 i;
    RwTexture** textureArray;
    RwRaster** rasterArray;
    RxPipeline** pipelineArray;
    RwUInt16 numTex = 0;
    RwUInt16 numRas = 0;
    RwUInt16 numPip = 0;
    RwUInt32 numMaterials = world->matList.numMaterials;

    textureArray = (RwTexture**)RwMalloc(numMaterials * sizeof(RwTexture*));
    rasterArray = (RwRaster**)RwMalloc(numMaterials * sizeof(RwRaster*));
    pipelineArray = (RxPipeline**)RwMalloc(numMaterials * sizeof(RxPipeline*));

    for (i = 0; i < worldSector->numPolygons; i++)
    {
        RpPolygon* tri = &worldSector->polygons[i];
        RpMaterial* material = matBase[tri->matIndex];
        RwUInt16 texIndex;
        RwUInt16 rasIndex;
        RwUInt16 pipIndex;
        RxPipeline* pipeline;
        RwTexture* texture;
        RwRaster* raster = (RwRaster*)NULL;

        texture = material->texture;
        for (texIndex = 0; texIndex < numTex; texIndex++)
        {
            if (textureArray[texIndex] == texture)
            {
                break;
            }
        }

        if (texIndex == numTex)
        {
            textureArray[texIndex] = texture;
            numTex++;
        }

        if (texture)
        {
            raster = texture->raster;
        }

        for (rasIndex = 0; rasIndex < numRas; rasIndex++)
        {
            if (rasterArray[rasIndex] == raster)
            {
                break;
            }
        }

        if (rasIndex == numRas)
        {
            rasterArray[rasIndex] = raster;
            numRas++;
        }

        pipeline = material->pipeline;
        for (pipIndex = 0; pipIndex < numPip; pipIndex++)
        {
            if (pipelineArray[pipIndex] == pipeline)
            {
                break;
            }
        }

        if (pipIndex == numPip)
        {
            pipelineArray[pipIndex] = pipeline;
            numPip++;
        }

        _rpBuildMeshAddTriangle(buildMesh, material, tri->vertIndex[0], tri->vertIndex[1],
                                tri->vertIndex[2], tri->matIndex, texIndex, rasIndex, pipIndex);
    }

    RwFree(textureArray);
    RwFree(rasterArray);
    RwFree(pipelineArray);

    if (world->flags & rpWORLDTRISTRIP)
    {
        mesh = _rpMeshOptimise(buildMesh, rpMESHHEADERTRISTRIP);
    }
    else
    {
        mesh = _rpMeshOptimise(buildMesh, 0);
    }

    if (mesh)
    {
        worldSector->mesh = mesh;
    }
    else
    {
        _rpBuildMeshDestroy(buildMesh);
        return (RpWorld*)NULL;
    }

    return world;
}

static void WorldSectorDeinstanceAll(RpSector* sector)
{
    switch (sector->type)
    {
    case rwSECTORATOMIC:
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
            RpLightTie* lightTie = rwLLLinkGetData(cur, RpLightTie, lWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpLightTieDestroy(lightTie);
        }

        _rwPluginRegistryDeInitObject(&sectorTKList, worldSector);
        break;
    }
    case rwSECTORBUILD:
        break;
    default:
    {
        RpPlaneSector* planeSector = (RpPlaneSector*)sector;

        WorldSectorDeinstanceAll(planeSector->leftSubTree);
        WorldSectorDeinstanceAll(planeSector->rightSubTree);
        break;
    }
    }
}

static void WorldSectorDestroyRecurse(RpSector* sector)
{
    switch (sector->type)
    {
    case rwSECTORATOMIC:
    {
        RpWorldSector* worldSector = (RpWorldSector*)sector;
        RwLLLink* cur;
        RwLLLink* end;
        RwInt32 i;

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
            RpLightTie* lightTie = rwLLLinkGetData(cur, RpLightTie, lWorldSector);

            cur = rwLLLinkGetNext(cur);
            _rpLightTieDestroy(lightTie);
        }

        _rwPluginRegistryDeInitObject(&sectorTKList, worldSector);

        if (worldSector->vertices)
        {
            RwFree(worldSector->vertices);
            worldSector->vertices = (RwV3d*)NULL;
        }

        if (worldSector->normals)
        {
            RwFree(worldSector->normals);
            worldSector->normals = (RpVertexNormal*)NULL;
        }

        if (worldSector->preLitLum)
        {
            RwFree(worldSector->preLitLum);
            worldSector->preLitLum = (RwRGBA*)NULL;
        }

        if (worldSector->polygons)
        {
            RwFree(worldSector->polygons);
            worldSector->polygons = (RpPolygon*)NULL;
        }

        for (i = 0; i < rwMAXTEXTURECOORDS; i++)
        {
            if (worldSector->texCoords[i])
            {
                RwFree(worldSector->texCoords[i]);
                worldSector->texCoords[i] = (RwTexCoords*)NULL;
            }
        }

        RwFree(worldSector);
        break;
    }
    case rwSECTORBUILD:
        RwFree(sector);
        break;
    default:
    {
        RpPlaneSector* planeSector = (RpPlaneSector*)sector;

        WorldSectorDestroyRecurse(planeSector->leftSubTree);
        planeSector->leftSubTree = (RpSector*)NULL;

        WorldSectorDestroyRecurse(planeSector->rightSubTree);
        planeSector->rightSubTree = (RpSector*)NULL;

        RwFree(planeSector);
        break;
    }
    }
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

RpWorld* _rpWorldSetupSectorBoundingBoxes(RpWorld* world)
{
    RwInt32 nStack = 0;
    RpSector* sector;
    RpSector* sectorStack[rpWORLDMAXBSPDEPTH];
    RwBBox bbox;
    RwBBox bboxStack[rpWORLDMAXBSPDEPTH];

    bbox = world->boundingBox;
    sector = world->rootSector;

    while (nStack >= 0)
    {
        if (sector->type < 0)
        {
            RpWorldSector* worldSector = (RpWorldSector*)sector;

            worldSector->boundingBox = bbox;

            bbox = bboxStack[nStack];
            sector = sectorStack[nStack];
            nStack--;
        }
        else
        {
            RpPlaneSector* plane = (RpPlaneSector*)sector;

            nStack++;
            bboxStack[nStack] = bbox;
            sectorStack[nStack] = plane->rightSubTree;
            SETCOORD(bboxStack[nStack].inf, plane->type, plane->rightValue);

            SETCOORD(bbox.sup, plane->type, plane->leftValue);
            sector = plane->leftSubTree;
        }
    }

    return world;
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

                if (buildMesh)
                {
                    world = WorldBuildMeshAtomicSector(world, buildMesh, worldSector, matList);

                    if (!world)
                    {
                        return (RpWorld*)NULL;
                    }
                }
                else
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

RpWorld* RpWorldRender(RpWorld* world)
{
    RwCameraForAllSectorsInFrustum((RwCamera*)RWSRCGLOBAL(curCamera), WorldSectorRender, world);

    return world;
}

RwBool RpWorldDestroy(RpWorld* world)
{
    RpSector* spaStack[rpWORLDMAXBSPDEPTH];
    RpSector* spSect;
    RwInt32 numSectors;

    _rpWorldUnregisterWorld(world);

    /* Throw away the meshes */
    numSectors = 0;
    spSect = world->rootSector;
    while (1)
    {
        if (spSect->type < 0)
        {
            RpWorldSector* worldSector = (RpWorldSector*)spSect;

            if (worldSector->mesh)
            {
                _rpMeshDestroy(worldSector->mesh);
                worldSector->mesh = (RpMeshHeader*)NULL;
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

    _rpMaterialListDeinitialize(&world->matList);

    if (rwObjectTestPrivateFlags(world, rpWORLDSINGLEMALLOC))
    {
        WorldSectorDeinstanceAll(world->rootSector);
        _rwPluginRegistryDeInitObject(&worldTKList, world);
        RwFree(world);
    }
    else
    {
        WorldSectorDestroyRecurse(world->rootSector);
        _rwPluginRegistryDeInitObject(&worldTKList, world);
        RwFree(world);
    }

    return TRUE;
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

RpWorld* RpWorldCreate(RwBBox* boundingBox)
{
    RpWorld* world;
    RpWorldSector* worldSector;

    world = (RpWorld*)RwMalloc(worldTKList.sizeOfStruct);
    if (!world)
    {
        RWERROR((E_RW_NOMEM, worldTKList.sizeOfStruct));
        return (RpWorld*)NULL;
    }

    rwObjectInitialize(world, rpWORLD, 0);

    _rpMaterialListInitialize(&world->matList);

    world->renderOrder = rpWORLDRENDERBACK2FRONT;
    world->flags = 0;

    worldSector = (RpWorldSector*)RwMalloc(sectorTKList.sizeOfStruct);
    if (!worldSector)
    {
        RWERROR((E_RW_NOMEM, sizeof(worldSector)));
        RwFree(world);
        return (RpWorld*)NULL;
    }

    worldSector->type = rwSECTORATOMIC;
    worldSector->repEntry = (RwResEntry*)NULL;
    worldSector->mesh = (RpMeshHeader*)NULL;

    rwLinkListInitialize(&worldSector->collAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->noCollAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->lightsInWorldSector);

    worldSector->numVertices = 0;
    worldSector->numPolygons = 0;
    worldSector->vertices = (RwV3d*)NULL;
    worldSector->polygons = (RpPolygon*)NULL;
    worldSector->normals = (RpVertexNormal*)NULL;
    memset(worldSector->texCoords, 0, sizeof(worldSector->texCoords));
    worldSector->preLitLum = (RwRGBA*)NULL;

    worldSector->boundingBox.inf = boundingBox->inf;
    worldSector->boundingBox.sup = boundingBox->sup;
    worldSector->tightBoundingBox.inf = boundingBox->inf;
    worldSector->tightBoundingBox.sup = boundingBox->sup;

    worldSector->pipeline = (RxPipeline*)NULL;

    world->rootSector = (RpSector*)worldSector;
    world->numTexCoordSets = 0;

    world->worldOrigin.x = world->worldOrigin.y = world->worldOrigin.z = (RwReal)0.0;

    world->boundingBox.inf = boundingBox->inf;
    world->boundingBox.sup = boundingBox->sup;

    rwLinkListInitialize(&world->clumpList);
    world->numClumpsInWorld = 0;
    world->currentClumpLink = rwLinkListGetTerminator(&world->clumpList);

    rwLinkListInitialize(&world->lightList);
    rwLinkListInitialize(&world->directionalLightList);

    RpWorldSetSectorRenderCallBack(world, (RpWorldSectorCallBackRender)NULL);

    world->pipeline = (RxPipeline*)NULL;

    _rpWorldRegisterWorld(world, worldTKList.sizeOfStruct);

    _rwPluginRegistryInitObject(&worldTKList, world);
    _rwPluginRegistryInitObject(&sectorTKList, worldSector);

    if (!RpWorldUnlock(world))
    {
        RpWorldDestroy(world);
        return (RpWorld*)NULL;
    }

    return world;
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

RwBool RpWorldPluginAttach(void)
{
    RwInt32 status;

    status = RwEngineRegisterPlugin(rpMATERIALGLOBALSSIZE, rwID_MATERIALMODULE, _rpMaterialOpen,
                                    _rpMaterialClose);
    status |= RwEngineRegisterPlugin(rpMESHGLOBALSSIZE, rwID_MESHMODULE, _rpMeshOpen, _rpMeshClose);
    status |= RwEngineRegisterPlugin(rpGEOMETRYGLOBALSSIZE, rwID_GEOMETRYMODULE, _rpGeometryOpen,
                                     _rpGeometryClose);
    status |=
        RwEngineRegisterPlugin(rpCLUMPGLOBALSSIZE, rwID_CLUMPMODULE, _rpClumpOpen, _rpClumpClose);
    status |=
        RwEngineRegisterPlugin(rpLIGHTGLOBALSSIZE, rwID_LIGHTMODULE, _rpLightOpen, _rpLightClose);
    status |= RwEngineRegisterPlugin(0, rwID_SECTORMODULE, _rpSectorOpen, _rpSectorClose);
    status |=
        RwEngineRegisterPlugin(sizeof(rpWorldGlobals), rwID_WORLDMODULE, WorldOpen, WorldClose);
    status |=
        RwEngineRegisterPlugin(0, rwID_BINWORLDMODULE, _rpBinaryWorldOpen, _rpBinaryWorldClose);

    if (status < 0)
    {
        return FALSE;
    }

    if (!_rpWorldObjRegisterExtensions())
    {
        return FALSE;
    }

    if (!_rpClumpRegisterExtensions())
    {
        return FALSE;
    }

    return _rxWorldDevicePluginAttach() ? TRUE : FALSE;
}
