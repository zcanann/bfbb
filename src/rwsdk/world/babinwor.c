#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include <string.h>

#define rwPLUGIN_ID 2

#define rwSECTORATOMIC (-1)

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_BADVERSION 0x80000004
#define E_RW_NOMEM 0x80000013

/* Streams older than these use the earlier chunk layouts */
#define rpWORLDOLDPOLYGONVERSION 0x30400
#define rpWORLDSURFACEPROPSVERSION 0x34001
#define rpWORLDBOUNDINGBOXVERSION 0x34003

#define rpWORLDNUMTEXCOORDSETS(_flags)                                                             \
    (((_flags) & 0xff0000) ?                                                                       \
         (((_flags) & 0xff0000) >> 16) :                                                           \
         (((_flags) & rpWORLDTEXTURED2) ? 2 : (((_flags) & rpWORLDTEXTURED) ? 1 : 0)))

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

typedef struct RpWorldChunkInfoSector RpWorldChunkInfoSector;
struct RpWorldChunkInfoSector
{
    RwInt32 matListWindowBase;
    RwInt32 numTriangles;
    RwInt32 numVertices;
    RwV3d inf;
    RwV3d sup;
    RwBool collSectorPresent;
    RwBool unused;
};

typedef struct RpPlaneSectorChunkInfo RpPlaneSectorChunkInfo;
struct RpPlaneSectorChunkInfo
{
    RwInt32 type;
    RwReal value;
    RwBool leftIsWorldSector;
    RwBool rightIsWorldSector;
    RwReal leftValue;
    RwReal rightValue;
};

typedef struct RpWorldChunkInfo RpWorldChunkInfo;
struct RpWorldChunkInfo
{
    RwBool rootIsWorldSector;
    RwV3d invWorldOrigin;
    RwInt32 numTriangles;
    RwInt32 numVertices;
    RwInt32 numPlaneSectors;
    RwInt32 numWorldSectors;
    RwInt32 colSectorSize;
    RwInt32 format;
    RwBBox boundingBox;
};

/* World chunk layout used before the surface properties moved to the materials */
typedef struct RpWorldChunkInfo34000 RpWorldChunkInfo34000;
struct RpWorldChunkInfo34000
{
    RwBool rootIsWorldSector;
    RwV3d invWorldOrigin;
    RwSurfaceProperties surfaceProps;
    RwInt32 numTriangles;
    RwInt32 numVertices;
    RwInt32 numPlaneSectors;
    RwInt32 numWorldSectors;
    RwInt32 colSectorSize;
    RwInt32 format;
};

extern RwPluginRegistry worldTKList;
extern RwPluginRegistry sectorTKList;

extern void* _rpBinaryWorldOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpBinaryWorldClose(void* instance, RwInt32 offset, RwInt32 size);
extern RwStream* _rpReadWorldRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off, RwInt32 size);
extern RwStream* _rpWriteWorldRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off,
                                     RwInt32 size);
extern RwInt32 _rpSizeWorldRights(const void* obj, RwInt32 off, RwInt32 size);
extern RwStream* _rpReadSectRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off, RwInt32 size);
extern RwStream* _rpWriteSectRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off,
                                    RwInt32 size);
extern RwInt32 _rpSizeSectRights(const void* obj, RwInt32 off, RwInt32 size);

static RwModuleInfo binWorldModule;

static RwUInt32 lastSeenWorldRightsPluginId;
static RwUInt32 lastSeenWorldExtraData;
static RwUInt32 lastSeenSectRightsPluginId;
static RwUInt32 lastSeenSectExtraData;

static RpPlaneSector* PlaneSectorStreamRead(RwStream* stream, RwUInt8** binaryWorldMallocAddr,
                                            RpWorld* world, RwUInt32 flags);

RwStream* _rpReadWorldRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off, RwInt32 size)
{
    if (!RwStreamReadInt32(s, (RwInt32*)&lastSeenWorldRightsPluginId, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (len == (RwInt32)(sizeof(RwInt32) * 2))
    {
        if (!RwStreamReadInt32(s, (RwInt32*)&lastSeenWorldExtraData, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }
    }

    return s;
}

RwStream* _rpWriteWorldRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off, RwInt32 size)
{
    const RpWorld* wrl = (const RpWorld*)obj;

    if (!RwStreamWriteInt32(s, (const RwInt32*)&wrl->pipeline->pluginId, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWriteInt32(s, (const RwInt32*)&wrl->pipeline->pluginData, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    return s;
}

RwInt32 _rpSizeWorldRights(const void* obj, RwInt32 off, RwInt32 size)
{
    const RpWorld* wrl = (const RpWorld*)obj;

    if (wrl->pipeline && wrl->pipeline->pluginId)
    {
        return sizeof(RwInt32) * 2;
    }

    return 0;
}

RwStream* _rpReadSectRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off, RwInt32 size)
{
    if (!RwStreamReadInt32(s, (RwInt32*)&lastSeenSectRightsPluginId, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (len == (RwInt32)(sizeof(RwInt32) * 2))
    {
        if (!RwStreamReadInt32(s, (RwInt32*)&lastSeenSectExtraData, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }
    }

    return s;
}

RwStream* _rpWriteSectRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off, RwInt32 size)
{
    const RpWorldSector* sect = (const RpWorldSector*)obj;

    if (!RwStreamWriteInt32(s, (const RwInt32*)&sect->pipeline->pluginId, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWriteInt32(s, (const RwInt32*)&sect->pipeline->pluginData, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    return s;
}

RwInt32 _rpSizeSectRights(const void* obj, RwInt32 off, RwInt32 size)
{
    const RpWorldSector* sect = (const RpWorldSector*)obj;

    if (sect->pipeline && sect->pipeline->pluginId)
    {
        return sizeof(RwInt32) * 2;
    }

    return 0;
}

static void* BinaryWorldMalloc(RwUInt8** binaryWorldMallocAddr, RwInt32 size)
{
    void* pMemory = *binaryWorldMallocAddr;

    *binaryWorldMallocAddr += size;

    return pMemory;
}

static RpWorldSector* WorldSectorStreamRead(RwStream* stream, RwUInt8** binaryWorldMallocAddr,
                                            RpWorld* world, RwUInt32 flags)
{
    RpWorldSector* worldSector;
    RpWorldChunkInfoSector as;
    RwUInt32 version;
    RwInt32 i;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, (RwUInt32*)NULL, &version))
    {
        return (RpWorldSector*)NULL;
    }

    if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
    {
        RWERROR((E_RW_BADVERSION));
        return (RpWorldSector*)NULL;
    }

    if (RwStreamRead(stream, &as, sizeof(as)) != sizeof(as))
    {
        return (RpWorldSector*)NULL;
    }

    RwMemNative32(&as, sizeof(as));

    worldSector =
        (RpWorldSector*)BinaryWorldMalloc(binaryWorldMallocAddr, sectorTKList.sizeOfStruct);
    if (!worldSector)
    {
        RWERROR((E_RW_NOMEM, sectorTKList.sizeOfStruct));
        return (RpWorldSector*)NULL;
    }

    worldSector->type = rwSECTORATOMIC;

    worldSector->tightBoundingBox.inf = as.inf;
    worldSector->tightBoundingBox.sup = as.sup;

    worldSector->matListWindowBase = (RwUInt16)as.matListWindowBase;
    worldSector->numPolygons = (RwUInt16)as.numTriangles;
    worldSector->numVertices = (RwUInt16)as.numVertices;

    worldSector->polygons = (RpPolygon*)NULL;
    worldSector->vertices = (RwV3d*)NULL;
    worldSector->preLitLum = (RwRGBA*)NULL;
    worldSector->normals = (RpVertexNormal*)NULL;
    memset(worldSector->texCoords, 0, sizeof(worldSector->texCoords));

    worldSector->pipeline = (RxPipeline*)NULL;
    worldSector->repEntry = (RwResEntry*)NULL;
    worldSector->mesh = (RpMeshHeader*)NULL;

    rwLinkListInitialize(&worldSector->collAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->noCollAtomicsInWorldSector);
    rwLinkListInitialize(&worldSector->lightsInWorldSector);

    if (!(world->flags & rpWORLDNATIVE))
    {
        if (as.numVertices)
        {
            RwInt32 vertexSize = as.numVertices * sizeof(RwV3d);

            worldSector->vertices = (RwV3d*)BinaryWorldMalloc(binaryWorldMallocAddr, vertexSize);
            if (!worldSector->vertices)
            {
                RWERROR((E_RW_NOMEM, vertexSize));
                return (RpWorldSector*)NULL;
            }

            if (!RwStreamReadReal(stream, (RwReal*)worldSector->vertices, vertexSize))
            {
                return (RpWorldSector*)NULL;
            }

            if (flags & rpWORLDNORMALS)
            {
                RwInt32 normalSize = as.numVertices * sizeof(RpVertexNormal);

                worldSector->normals =
                    (RpVertexNormal*)BinaryWorldMalloc(binaryWorldMallocAddr, normalSize);
                if (!worldSector->normals)
                {
                    RWERROR((E_RW_NOMEM, normalSize));
                    return (RpWorldSector*)NULL;
                }

                if (RwStreamRead(stream, worldSector->normals, normalSize) != normalSize)
                {
                    return (RpWorldSector*)NULL;
                }
            }

            if (flags & rpWORLDPRELIT)
            {
                RwInt32 preLitLumSize = as.numVertices * sizeof(RwRGBA);

                worldSector->preLitLum =
                    (RwRGBA*)BinaryWorldMalloc(binaryWorldMallocAddr, preLitLumSize);
                if (!worldSector->preLitLum)
                {
                    RWERROR((E_RW_NOMEM, preLitLumSize));
                    return (RpWorldSector*)NULL;
                }

                if (!RwStreamRead(stream, worldSector->preLitLum, preLitLumSize))
                {
                    return (RpWorldSector*)NULL;
                }
            }

            if (world->numTexCoordSets > 0)
            {
                RwInt32 texCoordSize = worldSector->numVertices * sizeof(RwTexCoords);

                for (i = 0; i < world->numTexCoordSets; i++)
                {
                    worldSector->texCoords[i] =
                        (RwTexCoords*)BinaryWorldMalloc(binaryWorldMallocAddr, texCoordSize);
                    if (!worldSector->texCoords[i])
                    {
                        RWERROR((E_RW_NOMEM, texCoordSize));
                        return (RpWorldSector*)NULL;
                    }

                    if (!RwStreamReadReal(stream, (RwReal*)worldSector->texCoords[i], texCoordSize))
                    {
                        return (RpWorldSector*)NULL;
                    }
                }
            }
        }

        if (as.numTriangles)
        {
            RwInt32 triangleSize = as.numTriangles * sizeof(RpPolygon);
            RwInt32 readSize;

            worldSector->polygons =
                (RpPolygon*)BinaryWorldMalloc(binaryWorldMallocAddr, triangleSize);
            if (!worldSector->polygons)
            {
                RWERROR((E_RW_NOMEM, triangleSize));
                return (RpWorldSector*)NULL;
            }

            /* Old streams store each polygon as four bytes */
            readSize = triangleSize;
            if (version < rpWORLDOLDPOLYGONVERSION)
            {
                readSize = triangleSize / 2;
            }

            if (RwStreamRead(stream, worldSector->polygons, readSize) != readSize)
            {
                return (RpWorldSector*)NULL;
            }

            if (version < rpWORLDOLDPOLYGONVERSION)
            {
                RwInt32 numPolygons = worldSector->numPolygons;
                RwUInt32 bytes = numPolygons * sizeof(RwUInt32);
                RpPolygon* polygons = worldSector->polygons;
                RwUInt8* oldPolygons = (RwUInt8*)RwMalloc(bytes);
                RwUInt8* src = oldPolygons;

                memcpy(oldPolygons, polygons, bytes);

                for (i = 0; i < numPolygons; i++)
                {
                    polygons[i].matIndex = *src++;
                    polygons[i].vertIndex[0] = *src++;
                    polygons[i].vertIndex[1] = *src++;
                    polygons[i].vertIndex[2] = *src++;
                }

                RwFree(oldPolygons);
            }
            else
            {
                RwMemNative16(worldSector->polygons, triangleSize);
            }
        }
    }

    _rwPluginRegistryInitObject(&sectorTKList, worldSector);

    lastSeenSectRightsPluginId = 0;
    lastSeenSectExtraData = 0;

    if (!_rwPluginRegistryReadDataChunks(&sectorTKList, stream, worldSector))
    {
        return (RpWorldSector*)NULL;
    }

    if (lastSeenSectRightsPluginId)
    {
        _rwPluginRegistryInvokeRights(&sectorTKList, lastSeenSectRightsPluginId, worldSector,
                                      lastSeenSectExtraData);
    }

    return worldSector;
}

static RpPlaneSector* PlaneSectorStreamRead(RwStream* stream, RwUInt8** binaryWorldMallocAddr,
                                            RpWorld* world, RwUInt32 flags)
{
    RpPlaneSector* planeSector;
    RpPlaneSectorChunkInfo ps;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return (RpPlaneSector*)NULL;
    }

    if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
    {
        RWERROR((E_RW_BADVERSION));
        return (RpPlaneSector*)NULL;
    }

    memset(&ps, 0, sizeof(ps));

    if (RwStreamRead(stream, &ps, size) != size)
    {
        return (RpPlaneSector*)NULL;
    }

    RwMemNative32(&ps, sizeof(ps));

    planeSector = (RpPlaneSector*)BinaryWorldMalloc(binaryWorldMallocAddr, sizeof(RpPlaneSector));
    if (!planeSector)
    {
        RWERROR((E_RW_NOMEM, sizeof(RpPlaneSector)));
        return (RpPlaneSector*)NULL;
    }

    planeSector->type = ps.type;
    planeSector->value = ps.value;

    if (flags & rpWORLDSECTORSOVERLAP)
    {
        planeSector->leftValue = ps.leftValue;
        planeSector->rightValue = ps.rightValue;
    }
    else
    {
        planeSector->leftValue = ps.value;
        planeSector->rightValue = ps.value;
    }

    if (ps.leftIsWorldSector)
    {
        if (!RwStreamFindChunk(stream, rwID_ATOMICSECT, (RwUInt32*)NULL, &version))
        {
            return (RpPlaneSector*)NULL;
        }

        if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
        {
            RWERROR((E_RW_BADVERSION));
            return (RpPlaneSector*)NULL;
        }

        if (!(planeSector->leftSubTree =
                  (RpSector*)WorldSectorStreamRead(stream, binaryWorldMallocAddr, world, flags)))
        {
            return (RpPlaneSector*)NULL;
        }
    }
    else
    {
        if (!RwStreamFindChunk(stream, rwID_PLANESECT, (RwUInt32*)NULL, &version))
        {
            return (RpPlaneSector*)NULL;
        }

        if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
        {
            RWERROR((E_RW_BADVERSION));
            return (RpPlaneSector*)NULL;
        }

        if (!(planeSector->leftSubTree =
                  (RpSector*)PlaneSectorStreamRead(stream, binaryWorldMallocAddr, world, flags)))
        {
            return (RpPlaneSector*)NULL;
        }
    }

    if (ps.rightIsWorldSector)
    {
        if (!RwStreamFindChunk(stream, rwID_ATOMICSECT, (RwUInt32*)NULL, &version))
        {
            return (RpPlaneSector*)NULL;
        }

        if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
        {
            RWERROR((E_RW_BADVERSION));
            return (RpPlaneSector*)NULL;
        }

        if (!(planeSector->rightSubTree =
                  (RpSector*)WorldSectorStreamRead(stream, binaryWorldMallocAddr, world, flags)))
        {
            return (RpPlaneSector*)NULL;
        }
    }
    else
    {
        if (!RwStreamFindChunk(stream, rwID_PLANESECT, (RwUInt32*)NULL, &version))
        {
            return (RpPlaneSector*)NULL;
        }

        if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
        {
            RWERROR((E_RW_BADVERSION));
            return (RpPlaneSector*)NULL;
        }

        if (!(planeSector->rightSubTree =
                  (RpSector*)PlaneSectorStreamRead(stream, binaryWorldMallocAddr, world, flags)))
        {
            return (RpPlaneSector*)NULL;
        }
    }

    return planeSector;
}

RpWorld* RpWorldStreamRead(RwStream* stream)
{
    RpWorld* world;
    RpWorldChunkInfo w;
    RwInt32 i;
    RwUInt8* binaryWorldMallocAddr;
    RwInt32 worldSize;
    RwUInt32 size;
    RwUInt32 worldVersion;
    RwUInt32 version;
    RwInt32 flags;
    RwUInt32 numTexCoordSets;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &worldVersion))
    {
        return (RpWorld*)NULL;
    }

    if ((worldVersion < rwLIBRARYBASEVERSION) || (worldVersion > rwLIBRARYCURRENTVERSION))
    {
        RWERROR((E_RW_BADVERSION));
        return (RpWorld*)NULL;
    }

    memset(&w, 0, sizeof(w));

    if (worldVersion < rpWORLDSURFACEPROPSVERSION)
    {
        RpWorldChunkInfo34000 w34000;

        memset(&w34000, 0, sizeof(w34000));

        if (RwStreamRead(stream, &w34000, size) != size)
        {
            return (RpWorld*)NULL;
        }

        w.rootIsWorldSector = w34000.rootIsWorldSector;
        w.invWorldOrigin = w34000.invWorldOrigin;
        w.numTriangles = w34000.numTriangles;
        w.numVertices = w34000.numVertices;
        w.numPlaneSectors = w34000.numPlaneSectors;
        w.numWorldSectors = w34000.numWorldSectors;
        w.colSectorSize = w34000.colSectorSize;
        w.format = w34000.format;
    }
    else
    {
        if (RwStreamRead(stream, &w, size) != size)
        {
            return (RpWorld*)NULL;
        }
    }

    /* Collision sectors are no longer supported */
    if (w.colSectorSize > 0)
    {
        RWERROR((E_RW_BADVERSION));
        return (RpWorld*)NULL;
    }

    RwMemNative32(&w, sizeof(w));

    worldSize = worldTKList.sizeOfStruct + w.numPlaneSectors * sizeof(RpPlaneSector) +
                w.numWorldSectors * sectorTKList.sizeOfStruct;

    flags = w.format;
    numTexCoordSets = rpWORLDNUMTEXCOORDSETS(flags);

    if (!(flags & rpWORLDNATIVE))
    {
        worldSize += w.numVertices * sizeof(RwV3d);

        if (flags & rpWORLDNORMALS)
        {
            worldSize += w.numVertices * sizeof(RpVertexNormal);
        }

        if (flags & rpWORLDPRELIT)
        {
            worldSize += w.numVertices * sizeof(RwRGBA);
        }

        if (numTexCoordSets)
        {
            worldSize += numTexCoordSets * w.numVertices * sizeof(RwTexCoords);
        }

        worldSize += w.numTriangles * sizeof(RpPolygon);
    }

    world = (RpWorld*)RwMalloc(worldSize);
    if (!world)
    {
        RWERROR((E_RW_NOMEM, worldSize));
        return (RpWorld*)NULL;
    }

    memset(world, 0, worldSize);

    binaryWorldMallocAddr = (RwUInt8*)world + worldTKList.sizeOfStruct;

    rwObjectInitialize(world, rpWORLD, 0);
    rwObjectSetPrivateFlags(world, rpWORLDSINGLEMALLOC);

    _rpWorldRegisterWorld(world, worldSize);

    world->flags = flags;
    world->renderOrder = rpWORLDRENDERBACK2FRONT;

    RwV3dScaleMacro(&world->worldOrigin, &w.invWorldOrigin, (RwReal)-1.0);

    world->numTexCoordSets = numTexCoordSets;
    world->rootSector = (RpSector*)NULL;
    world->pipeline = (RxPipeline*)NULL;

    if (!RwStreamFindChunk(stream, rwID_MATLIST, (RwUInt32*)NULL, &version))
    {
        _rpWorldUnregisterWorld(world);
        RwFree(world);
        return (RpWorld*)NULL;
    }

    if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
    {
        RWERROR((E_RW_BADVERSION));
        return (RpWorld*)NULL;
    }

    if (!_rpMaterialListStreamRead(stream, &world->matList))
    {
        _rpWorldUnregisterWorld(world);
        RwFree(world);
        return (RpWorld*)NULL;
    }

    if (w.rootIsWorldSector)
    {
        if (!RwStreamFindChunk(stream, rwID_ATOMICSECT, (RwUInt32*)NULL, &version))
        {
            _rpWorldUnregisterWorld(world);
            RwFree(world);
            return (RpWorld*)NULL;
        }

        if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
        {
            RWERROR((E_RW_BADVERSION));
            _rpWorldUnregisterWorld(world);
            RwFree(world);
            return (RpWorld*)NULL;
        }

        if (!(world->rootSector =
                  (RpSector*)WorldSectorStreamRead(stream, &binaryWorldMallocAddr, world, flags)))
        {
            _rpWorldUnregisterWorld(world);
            RwFree(world);
            return (RpWorld*)NULL;
        }
    }
    else
    {
        if (!RwStreamFindChunk(stream, rwID_PLANESECT, (RwUInt32*)NULL, &version))
        {
            _rpWorldUnregisterWorld(world);
            RwFree(world);
            return (RpWorld*)NULL;
        }

        if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
        {
            RWERROR((E_RW_BADVERSION));
            _rpWorldUnregisterWorld(world);
            RwFree(world);
            return (RpWorld*)NULL;
        }

        if (!(world->rootSector =
                  (RpSector*)PlaneSectorStreamRead(stream, &binaryWorldMallocAddr, world, flags)))
        {
            _rpWorldUnregisterWorld(world);
            RwFree(world);
            return (RpWorld*)NULL;
        }
    }

    rwLinkListInitialize(&world->clumpList);
    world->numClumpsInWorld = 0;
    world->currentClumpLink = rwLinkListGetTerminator(&world->clumpList);

    rwLinkListInitialize(&world->lightList);
    rwLinkListInitialize(&world->directionalLightList);

    if (worldVersion < rpWORLDBOUNDINGBOXVERSION)
    {
        _rpWorldFindBBox(world, &world->boundingBox);
    }
    else
    {
        world->boundingBox = w.boundingBox;
    }

    _rpWorldSetupSectorBoundingBoxes(world);

    RpWorldSetSectorRenderCallBack(world, (RpWorldSectorCallBackRender)NULL);

    /* Make sure the material textures are set up properly */
    for (i = 0; i < world->matList.numMaterials; i++)
    {
        RpMaterial* mat = world->matList.materials[i];
        RwTexture* tex = mat->texture;

        if (tex)
        {
            RpMaterialSetTexture(mat, tex);
        }
    }

    _rwPluginRegistryInitObject(&worldTKList, world);

    lastSeenWorldRightsPluginId = 0;
    lastSeenWorldExtraData = 0;

    if (!_rwPluginRegistryReadDataChunks(&worldTKList, stream, world))
    {
        _rpWorldUnregisterWorld(world);
        RwFree(world);
        return (RpWorld*)NULL;
    }

    if (lastSeenWorldRightsPluginId)
    {
        _rwPluginRegistryInvokeRights(&worldTKList, lastSeenWorldRightsPluginId, world,
                                      lastSeenWorldExtraData);
    }

    if (!RpWorldUnlock(world))
    {
        RpWorldDestroy(world);
        return (RpWorld*)NULL;
    }

    return world;
}

void* _rpBinaryWorldClose(void* instance, RwInt32 offset, RwInt32 size)
{
    binWorldModule.numInstances--;

    return instance;
}

void* _rpBinaryWorldOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    binWorldModule.numInstances++;

    return instance;
}
