#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/os.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

#define rwGCNMAXVTXDATA 16

/* GXBegin primitive type | vertex format */
#define rwGCNPRIMTRIANGLES 0x90
#define rwGCNPRIMTRIANGLESTRIP 0x98

#define SECTORGETMESH(_sector, _i) ((RpMesh*)((_sector)->mesh + 1) + (_i))

static RwBool ReconditionVertexIndexData(RpWorld* world, RpWorldSector* sector,
                                         rwGCNVtxDataRemapped** vertexDataRemapped,
                                         rwGCNIndexDataRemapped*** indexDataRemapped)
{
    rwGCNVtxData vtxData[rwGCNMAXVTXDATA];
    rwGCNVtxDataMap* vertexDataMaps;
    rwGCNIndexData* indexData;
    RwUInt32 numEntries;
    RwUInt32 numMeshes;
    RwUInt32 i;
    RpGameCubeVtxFmt* vtxFmt;

    vtxFmt = WORLDVTXFMT(world);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    numEntries = 0;

    vtxData[numEntries].data = sector->vertices;
    vtxData[numEntries].type = rwDATATYPE_V3D;
    vtxData[numEntries].dep[0] = -1;
    numEntries++;

    if (RpWorldGetFlags(world) & rpWORLDNORMALS)
    {
        RwInt32 j;

        /* Compressed normals - clear the padding so that they compare reliably */
        for (j = 0; j < sector->numVertices; j++)
        {
            sector->normals[j].pad = 0;
        }

        vtxData[numEntries].data = sector->normals;
        vtxData[numEntries].type = rwDATATYPE_INT32;
        vtxData[numEntries].dep[0] = -1;
        numEntries++;
    }

    if (RpWorldGetFlags(world) & rpWORLDPRELIT)
    {
        vtxData[numEntries].data = sector->preLitLum;
        vtxData[numEntries].type = rwDATATYPE_INT32;
        vtxData[numEntries].dep[0] = -1;
        numEntries++;
    }

    if (RpWorldGetFlags(world) & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        for (i = 0; i < (RwUInt32)world->numTexCoordSets; i++)
        {
            vtxData[numEntries].data = sector->texCoords[i];
            vtxData[numEntries].type = rwDATATYPE_V2D;
            vtxData[numEntries].dep[0] = -1;
            numEntries++;
        }
    }

    vertexDataMaps = VertexDataCreateMaps(vtxData, numEntries, sector->numVertices);
    if (vertexDataMaps == NULL)
    {
        return FALSE;
    }

    *vertexDataRemapped =
        VertexDataCreateRemapped(vertexDataMaps, vtxData, numEntries, sector->numVertices);
    if (*vertexDataRemapped == NULL)
    {
        RwFree(vertexDataMaps);
        return FALSE;
    }

    numMeshes = sector->mesh->numMeshes;

    indexData = (rwGCNIndexData*)RwMalloc(numEntries * (numMeshes * sizeof(rwGCNIndexData)));
    if (indexData == NULL)
    {
        RwFree(*vertexDataRemapped);
        RwFree(vertexDataMaps);
        return FALSE;
    }

    memset(indexData, 0, numEntries * (numMeshes * sizeof(rwGCNIndexData)));

    for (i = 0; i < numMeshes; i++)
    {
        RpMesh* mesh;
        RwInt32 j;

        mesh = (RpMesh*)(sector->mesh + 1) + i;

        for (j = 0; j < numEntries; j++)
        {
            indexData[i * numEntries + j].indices = mesh->indices;
        }
    }

    *indexDataRemapped =
        (rwGCNIndexDataRemapped**)RwMalloc(numMeshes * sizeof(rwGCNIndexDataRemapped*));
    if (*indexDataRemapped == NULL)
    {
        RwFree(indexData);
        RwFree(*vertexDataRemapped);
        RwFree(vertexDataMaps);
        return FALSE;
    }

    for (i = 0; i < numMeshes; i++)
    {
        RpMesh* mesh;

        mesh = (RpMesh*)(sector->mesh + 1) + i;

        (*indexDataRemapped)[i] = IndexDataCreateRemapped(vertexDataMaps, &indexData[i * numEntries],
                                                          numEntries, mesh->numIndices);
        if ((*indexDataRemapped)[i] == NULL)
        {
            while (--i)
            {
                RwFree((*indexDataRemapped)[i]);
            }

            return FALSE;
        }
    }

    RwFree(indexData);
    RwFree(vertexDataMaps);

    return TRUE;
}

static rwVertexDescriptor VtxDesc;

static rwVertexDescriptor* VtxDescInitOptimized(RpWorld* world,
                                                rwGCNVtxDataRemapped* vertexDataRemapped)
{
    RwUInt32 numAttr;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 j;

    vtxFmt = WORLDVTXFMT(world);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);

    numAttr = 0;

    _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_POS, rwGCNCC_POS_XYZ,
                                   (rwGCNCompType)vtxFmt->pos, vtxFmt->posFrac);
    _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_POS,
                                   (vertexDataRemapped[numAttr].num >= 0xFF) ? rwGCNAT_INDEX16
                                                                            : rwGCNAT_INDEX8);
    numAttr++;

    if (RpWorldGetFlags(world) & rpWORLDNORMALS)
    {
        if (vtxFmt->nbt)
        {
            _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_NBT, rwGCNCC_NRM_NBT,
                                           (rwGCNCompType)vtxFmt->norm, 0);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, rwGCNVA_NBT,
                (vertexDataRemapped[numAttr].num >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
        }
        else
        {
            _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_NRM, rwGCNCC_NRM_XYZ,
                                           (rwGCNCompType)vtxFmt->norm, 0);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, rwGCNVA_NRM,
                (vertexDataRemapped[numAttr].num >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
        }
        numAttr++;
    }

    if (RpWorldGetFlags(world) & rpWORLDPRELIT)
    {
        _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_CLR0,
                                       (vtxFmt->preLight > rpRGBX8) ? rwGCNCC_CLR_RGBA
                                                                    : rwGCNCC_CLR_RGB,
                                       (rwGCNCompType)vtxFmt->preLight, 0);
        _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_CLR0,
                                       (vertexDataRemapped[numAttr].num >= 0xFF) ? rwGCNAT_INDEX16
                                                                                : rwGCNAT_INDEX8);
        numAttr++;
    }

    if (RpWorldGetFlags(world) & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        for (j = 0; j < (RwUInt32)world->numTexCoordSets; j++)
        {
            _rwGCNVertexDescSetElementAttr(&VtxDesc, (rwGCNVertexAttribute)(rwGCNVA_TEX0 + j),
                                           rwGCNCC_TEX_ST, (rwGCNCompType)vtxFmt->texCoord[j],
                                           vtxFmt->texCoordFrac[j]);
            _rwGCNVertexDescSetElementDesc(
                &VtxDesc, (rwGCNVertexAttribute)(rwGCNVA_TEX0 + j),
                (vertexDataRemapped[numAttr].num >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
            numAttr++;
        }
    }

    _rwGCNVertexDescSetNumIndexedAttr(&VtxDesc, (RwUInt8)numAttr);

    return &VtxDesc;
}

static void VertexDataSetupOptimized(rwGCNVertexBufferData* vtxBufferData,
                                     rwGCNVtxDataRemapped* vtxDataRemapped, RpWorld* world)
{
    RwUInt32 numElements;
    RwUInt32 flags;
    RwUInt8 numTexCoords;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    numTexCoords = (RwUInt8)world->numTexCoordSets;
    flags = RpWorldGetFlags(world);

    vtxFmt = WORLDVTXFMT(world);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    numElements = 0;

    vtxBufferData->num[rwGCNVA_POS] = vtxDataRemapped[numElements].num;
    vtxBufferData->data[rwGCNVA_POS] = vtxDataRemapped[numElements].data;
    numElements++;

    if (flags & rpWORLDNORMALS)
    {
        vtxBufferData->num[rwGCNVA_NRM] = vtxDataRemapped[numElements].num;
        vtxBufferData->data[rwGCNVA_NRM] = vtxDataRemapped[numElements].data;
        numElements++;
    }

    if (flags & rpWORLDPRELIT)
    {
        vtxBufferData->num[rwGCNVA_CLR0] = vtxDataRemapped[numElements].num;
        vtxBufferData->data[rwGCNVA_CLR0] = vtxDataRemapped[numElements].data;
        numElements++;
    }

    if (flags & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        for (i = 0; i < numTexCoords; i++)
        {
            vtxBufferData->num[rwGCNVA_TEX0 + i] = vtxDataRemapped[numElements].num;
            vtxBufferData->data[rwGCNVA_TEX0 + i] = vtxDataRemapped[numElements].data;
            numElements++;
        }
    }
}

static void IndexDataSetupOptimized(rwGCNDisplayListData* indexBufferData,
                                    rwGCNIndexDataRemapped* indexDataRemapped, RpWorld* world)
{
    RwUInt32 numElements;
    RwUInt32 flags;
    RwUInt8 numTexCoords;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    numTexCoords = (RwUInt8)world->numTexCoordSets;
    flags = RpWorldGetFlags(world);

    vtxFmt = WORLDVTXFMT(world);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    numElements = 0;

    indexBufferData->data[rwGCNVA_POS] = indexDataRemapped[numElements].indices;
    numElements++;

    if (flags & rpWORLDNORMALS)
    {
        indexBufferData->data[rwGCNVA_NRM] = indexDataRemapped[numElements].indices;
        numElements++;
    }

    if (flags & rpWORLDPRELIT)
    {
        indexBufferData->data[rwGCNVA_CLR0] = indexDataRemapped[numElements].indices;
        numElements++;
    }

    if (flags & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        for (i = 0; i < numTexCoords; i++)
        {
            indexBufferData->data[rwGCNVA_TEX0 + i] = indexDataRemapped[numElements].indices;
            numElements++;
        }
    }
}

RwResEntry* _rwDlWorldSectorInstanceOptimized(RpWorld* world, RpWorldSector* sector)
{
    RwUInt32 i;
    RwUInt32 size;
    RwUInt32 vBufferHeaderSize;
    RwUInt32 displayListSize;
    RwUInt32 vBufferSize;
    RwResEntry* resEntry;
    void* memory;
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* displayLists;
    rwGCNVtxDataRemapped* vertexDataRemapped;
    rwGCNIndexDataRemapped** indexDataRemapped;
    rwVertexDescriptor* vtxDesc;
    rwGCNVertexBufferData vtxBufData;
    RwUInt8 primTypeVAT;
    RpGameCubeVtxFmt* vtxFmt;
    rwGCNDisplayListData displayListData;

    ReconditionVertexIndexData(world, sector, &vertexDataRemapped, &indexDataRemapped);

    vtxDesc = VtxDescInitOptimized(world, vertexDataRemapped);
    VertexDataSetupOptimized(&vtxBufData, vertexDataRemapped, world);

    /* Work out how much memory we need */
    vBufferHeaderSize = _rwGCNVertexBufferHeaderGetSize(vtxDesc);
    displayListSize = sector->mesh->numMeshes * sizeof(RxGameCubeDisplayList);
    size = vBufferHeaderSize + displayListSize;
    size += 31;

    if (RpWorldGetFlags(world) & rpWORLDTRISTRIP)
    {
        primTypeVAT = rwGCNPRIMTRIANGLESTRIP;
    }
    else
    {
        primTypeVAT = rwGCNPRIMTRIANGLES;
    }

    for (i = 0; i < sector->mesh->numMeshes; i++)
    {
        if (RpWorldGetFlags(world) & rpWORLDTRISTRIP)
        {
            RwUInt32 numStrips;
            RwUInt32 numIndices;

            _rwGCNTriStripGetStats(indexDataRemapped[i][0].indices, SECTORGETMESH(sector, i)->numIndices, &numStrips,
                                   &numIndices, TRUE);
            size += _rwGCNDisplayListGetSize(vtxDesc, numStrips, numIndices);
        }
        else
        {
            size += _rwGCNDisplayListGetSize(vtxDesc, 1, SECTORGETMESH(sector, i)->numIndices);
        }
    }

    size += _rwGCNVertexBufferGetSize(vtxDesc, &vtxBufData);

    /* Pre-instanced data lives outside of the resource arena */
    resEntry = (RwResEntry*)RwMalloc(sizeof(RwResEntry) + size);
    resEntry->link.next = NULL;
    resEntry->link.prev = NULL;
    resEntry->owner = sector;
    resEntry->size = size;
    resEntry->ownerRef = &sector->repEntry;
    resEntry->destroyNotify = _rxGCResEntryWaitDone;
    sector->repEntry = resEntry;

    vBufferSize = size;

    memory = resEntry + 1;
    memset(memory, 0, vBufferSize);

    vbHeader = (RxGameCubeVertexBuffer*)memory;
    vbHeader->token = _RwDlTokenLastSeen;
    vbHeader->serialNumber = sector->mesh->serialNum;
    vbHeader->flags = 0;

    if (RpWorldGetFlags(world) & rpWORLDPRELIT)
    {
        vtxFmt = WORLDVTXFMT(world);
        if (vtxFmt == NULL)
        {
            RwInt32 j;

            vbHeader->flags &= ~1;

            for (j = 0; j < RpWorldSectorGetNumVertices(sector); j++)
            {
                if (sector->preLitLum[j].alpha < 0xFF)
                {
                    vbHeader->flags |= 1;
                    break;
                }
            }
        }
        else if (vtxFmt->preLight > rpRGBX8)
        {
            vbHeader->flags |= 1;
        }
        else
        {
            vbHeader->flags &= ~1;
        }
    }

    displayLists = (RxGameCubeDisplayList*)((RwUInt8*)memory + vBufferHeaderSize);
    memory = (RwUInt8*)displayLists + displayListSize;
    memory = (RwUInt8*)memory + 31;
    memory = (void*)((RwUInt32)memory & ~31);

    for (i = 0; i < sector->mesh->numMeshes; i++)
    {
        RwUInt32 numStrips;
        RwUInt32 numIndices;
        RwUInt32 meshDisplayListSize;

        if (RpWorldGetFlags(world) & rpWORLDTRISTRIP)
        {
            _rwGCNTriStripGetStats(indexDataRemapped[i][0].indices, SECTORGETMESH(sector, i)->numIndices, &numStrips,
                                   &numIndices, TRUE);
        }
        else
        {
            numIndices = SECTORGETMESH(sector, i)->numIndices;
            numStrips = 1;
        }

        meshDisplayListSize = _rwGCNDisplayListGetSize(vtxDesc, numStrips, numIndices);
        _rwGCNDisplayListInitialize(&displayLists[i], i, meshDisplayListSize, memory);

        IndexDataSetupOptimized(&displayListData, indexDataRemapped[i], world);

        _rwGCNDisplayListFill(vtxDesc, &displayLists[i], &displayListData, SECTORGETMESH(sector, i)->numIndices,
                              numStrips, _rwGCNDisplayListGetStride(vtxDesc), TRUE, primTypeVAT);

        memory = (RwUInt8*)memory + meshDisplayListSize;
    }

    _rwGCNVertexBufferInitialize(vtxDesc, vbHeader, &vtxBufData, (RwUInt8*)memory);
    _rwGCNVertexBufferFill(vtxDesc, vbHeader, &vtxBufData, TRUE);

    DCFlushRange(vbHeader, vBufferSize);
    GXInvalidateVtxCache();

    for (i = 0; i < sector->mesh->numMeshes; i++)
    {
        RwFree(indexDataRemapped[i]);
    }
    RwFree(indexDataRemapped);
    RwFree(vertexDataRemapped);

    return resEntry;
}

static rwVertexDescriptor* VtxDescInitFast(RpWorld* world, RpWorldSector* sector)
{
    RwUInt32 numAttr;
    RwUInt32 numVerts;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 j;

    numVerts = sector->numVertices;

    vtxFmt = WORLDVTXFMT(world);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);

    numAttr = 0;

    _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_POS, rwGCNCC_POS_XYZ,
                                   (rwGCNCompType)vtxFmt->pos, vtxFmt->posFrac);
    _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_POS,
                                   (numVerts >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
    numAttr++;

    if (RpWorldGetFlags(world) & rpWORLDNORMALS)
    {
        if (vtxFmt->nbt)
        {
            _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_NBT, rwGCNCC_NRM_NBT,
                                           (rwGCNCompType)vtxFmt->norm, 0);
            _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_NBT,
                                           (numVerts >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
        }
        else
        {
            _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_NRM, rwGCNCC_NRM_XYZ,
                                           (rwGCNCompType)vtxFmt->norm, 0);
            _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_NRM,
                                           (numVerts >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
        }
        numAttr++;
    }

    if (RpWorldGetFlags(world) & rpWORLDPRELIT)
    {
        _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_CLR0,
                                       (vtxFmt->preLight > rpRGBX8) ? rwGCNCC_CLR_RGBA
                                                                    : rwGCNCC_CLR_RGB,
                                       (rwGCNCompType)vtxFmt->preLight, 0);
        _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_CLR0,
                                       (numVerts >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
        numAttr++;
    }

    if (RpWorldGetFlags(world) & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        for (j = 0; j < (RwUInt32)world->numTexCoordSets; j++)
        {
            _rwGCNVertexDescSetElementAttr(&VtxDesc, (rwGCNVertexAttribute)(rwGCNVA_TEX0 + j),
                                           rwGCNCC_TEX_ST, (rwGCNCompType)vtxFmt->texCoord[j],
                                           vtxFmt->texCoordFrac[j]);
            _rwGCNVertexDescSetElementDesc(&VtxDesc, (rwGCNVertexAttribute)(rwGCNVA_TEX0 + j),
                                           (numVerts >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
            numAttr++;
        }
    }

    _rwGCNVertexDescSetNumIndexedAttr(&VtxDesc, (RwUInt8)numAttr);

    return &VtxDesc;
}

static void VertexDataFastSetup(rwGCNVertexBufferData* vtxBufferData, RpWorld* world,
                                RpWorldSector* sector)
{
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    vtxFmt = WORLDVTXFMT(world);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    vtxBufferData->num[rwGCNVA_POS] = sector->numVertices;
    vtxBufferData->data[rwGCNVA_POS] = sector->vertices;

    if (RpWorldGetFlags(world) & rpWORLDNORMALS)
    {
        vtxBufferData->num[rwGCNVA_NRM] = sector->numVertices;
        vtxBufferData->data[rwGCNVA_NRM] = sector->normals;
    }

    if (RpWorldGetFlags(world) & rpWORLDPRELIT)
    {
        vtxBufferData->num[rwGCNVA_CLR0] = sector->numVertices;
        vtxBufferData->data[rwGCNVA_CLR0] = sector->preLitLum;
    }

    if (RpWorldGetFlags(world) & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        for (i = 0; i < (RwUInt32)world->numTexCoordSets; i++)
        {
            vtxBufferData->num[rwGCNVA_TEX0 + i] = sector->numVertices;
            vtxBufferData->data[rwGCNVA_TEX0 + i] = sector->texCoords[i];
        }
    }
}

static void IndexDataSetupFast(rwGCNDisplayListData* indexBufferData, RwUInt16* indices,
                               RpWorld* world)
{
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    vtxFmt = WORLDVTXFMT(world);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    indexBufferData->data[rwGCNVA_POS] = indices;

    if (RpWorldGetFlags(world) & rpWORLDNORMALS)
    {
        indexBufferData->data[rwGCNVA_NRM] = indices;
    }

    if (RpWorldGetFlags(world) & rpWORLDPRELIT)
    {
        indexBufferData->data[rwGCNVA_CLR0] = indices;
    }

    if (RpWorldGetFlags(world) & (rpWORLDTEXTURED | rpWORLDTEXTURED2))
    {
        for (i = 0; i < (RwUInt32)world->numTexCoordSets; i++)
        {
            indexBufferData->data[rwGCNVA_TEX0 + i] = indices;
        }
    }
}

RwResEntry* _rwDlWorldSectorInstanceFast(RpWorld* world, RpWorldSector* sector, void* owner,
                                         RwResEntry** resEntryOwner)
{
    RwUInt32 i;
    RwUInt32 size;
    RwUInt32 vBufferHeaderSize;
    RwUInt32 displayListSize;
    RwUInt32 vBufferSize;
    RwResEntry* resEntry;
    void* memory;
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* displayLists;
    rwVertexDescriptor* vtxDesc;
    rwGCNVertexBufferData vtxBufData;
    RwUInt8 primTypeVAT;
    RpGameCubeVtxFmt* vtxFmt;
    rwGCNDisplayListData displayListData;

    vtxDesc = VtxDescInitFast(world, sector);
    VertexDataFastSetup(&vtxBufData, world, sector);

    /* Work out how much memory we need */
    vBufferHeaderSize = _rwGCNVertexBufferHeaderGetSize(vtxDesc);
    displayListSize = sector->mesh->numMeshes * sizeof(RxGameCubeDisplayList);
    size = vBufferHeaderSize + displayListSize;
    size += 31;

    if (RpWorldGetFlags(world) & rpWORLDTRISTRIP)
    {
        primTypeVAT = rwGCNPRIMTRIANGLESTRIP;
    }
    else
    {
        primTypeVAT = rwGCNPRIMTRIANGLES;
    }

    for (i = 0; i < sector->mesh->numMeshes; i++)
    {
        size += _rwGCNDisplayListGetSize(vtxDesc, 1, SECTORGETMESH(sector, i)->numIndices);
    }

    size += _rwGCNVertexBufferGetSize(vtxDesc, &vtxBufData);

    if (RpWorldGetFlags(world) & rpWORLDNATIVEINSTANCE)
    {
        /* Pre-instanced data lives outside of the resource arena */
        resEntry = (RwResEntry*)RwMalloc(sizeof(RwResEntry) + size);
        resEntry->link.next = NULL;
        resEntry->link.prev = NULL;
        resEntry->owner = sector;
        resEntry->size = size;
        resEntry->ownerRef = &sector->repEntry;
        resEntry->destroyNotify = _rxGCResEntryWaitDone;
        sector->repEntry = resEntry;
    }
    else
    {
        resEntry = RwResourcesAllocateResEntry(owner, resEntryOwner, size, _rxGCResEntryWaitDone);
    }

    vBufferSize = size;

    memory = resEntry + 1;
    memset(memory, 0, vBufferSize);

    vbHeader = (RxGameCubeVertexBuffer*)memory;
    vbHeader->token = _RwDlTokenLastSeen;
    vbHeader->serialNumber = sector->mesh->serialNum;
    vbHeader->flags = 0;

    if (RpWorldGetFlags(world) & rpWORLDPRELIT)
    {
        vtxFmt = WORLDVTXFMT(world);
        if (vtxFmt == NULL)
        {
            RwInt32 j;

            vbHeader->flags &= ~1;

            for (j = 0; j < RpWorldSectorGetNumVertices(sector); j++)
            {
                if (sector->preLitLum[j].alpha < 0xFF)
                {
                    vbHeader->flags |= 1;
                    break;
                }
            }
        }
        else if (vtxFmt->preLight > rpRGBX8)
        {
            vbHeader->flags |= 1;
        }
        else
        {
            vbHeader->flags &= ~1;
        }
    }

    displayLists = (RxGameCubeDisplayList*)((RwUInt8*)memory + vBufferHeaderSize);
    memory = (RwUInt8*)displayLists + displayListSize;
    memory = (RwUInt8*)memory + 31;
    memory = (void*)((RwUInt32)memory & ~31);

    for (i = 0; i < sector->mesh->numMeshes; i++)
    {
        RwUInt32 meshDisplayListSize;

        meshDisplayListSize = _rwGCNDisplayListGetSize(vtxDesc, 1, SECTORGETMESH(sector, i)->numIndices);
        _rwGCNDisplayListInitialize(&displayLists[i], i, meshDisplayListSize, memory);

        IndexDataSetupFast(&displayListData, SECTORGETMESH(sector, i)->indices, world);

        _rwGCNDisplayListFill(vtxDesc, &displayLists[i], &displayListData, SECTORGETMESH(sector, i)->numIndices, 1,
                              _rwGCNDisplayListGetStride(vtxDesc), TRUE, primTypeVAT);

        memory = (RwUInt8*)memory + meshDisplayListSize;
    }

    _rwGCNVertexBufferInitialize(vtxDesc, vbHeader, &vtxBufData, (RwUInt8*)memory);
    _rwGCNVertexBufferFill(vtxDesc, vbHeader, &vtxBufData, TRUE);

    DCFlushRange(vbHeader, vBufferSize);
    GXInvalidateVtxCache();

    return resEntry;
}
