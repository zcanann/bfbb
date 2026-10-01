#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/os.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

#define rwGCNMAXVTXDATA 16

/* GXBegin primitive type | vertex format */
#define rwGCNPRIMTRIANGLES 0x90
#define rwGCNPRIMTRIANGLESTRIP 0x98

#define GEOMETRYGETMESH(_geometry, _i) ((RpMesh*)((_geometry)->mesh + 1) + (_i))

static RwBool ReconditionVertexIndexData(RpGeometry* geometry,
                                         rwGCNVtxDataRemapped** vertexDataRemapped,
                                         rwGCNIndexDataRemapped*** indexDataRemapped)
{
    rwGCNVtxData vtxData[rwGCNMAXVTXDATA];
    rwGCNVtxDataMap* vertexDataMaps;
    rwGCNIndexData* indexData;
    RpMorphTarget* morphTarget;
    RwUInt32 numEntries;
    RwUInt32 numMeshes;
    RwUInt32 i;
    RpGameCubeVtxFmt* vtxFmt;

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    morphTarget = geometry->morphTarget;

    numEntries = 0;

    vtxData[numEntries].data = morphTarget->verts;
    vtxData[numEntries].type = rwDATATYPE_V3D;
    vtxData[numEntries].dep[0] = -1;
    numEntries++;

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS)
    {
        vtxData[numEntries].data = morphTarget->normals;
        vtxData[numEntries].type = rwDATATYPE_V3D;
        vtxData[numEntries].dep[0] = -1;
        numEntries++;
    }

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
    {
        vtxData[numEntries].data = geometry->preLitLum;
        vtxData[numEntries].type = rwDATATYPE_INT32;
        vtxData[numEntries].dep[0] = -1;
        numEntries++;
    }

    if (RpGeometryGetFlags(geometry) & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        for (i = 0; i < (RwUInt32)geometry->numTexCoordSets; i++)
        {
            vtxData[numEntries].data = geometry->texCoords[i];
            vtxData[numEntries].type = rwDATATYPE_V2D;
            vtxData[numEntries].dep[0] = -1;
            numEntries++;
        }
    }

    vertexDataMaps = VertexDataCreateMaps(vtxData, numEntries, geometry->numVertices);
    if (vertexDataMaps == NULL)
    {
        return FALSE;
    }

    *vertexDataRemapped =
        VertexDataCreateRemapped(vertexDataMaps, vtxData, numEntries, geometry->numVertices);
    if (*vertexDataRemapped == NULL)
    {
        RwFree(vertexDataMaps);
        return FALSE;
    }

    numMeshes = geometry->mesh->numMeshes;

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

        mesh = (RpMesh*)(geometry->mesh + 1) + i;

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

        mesh = (RpMesh*)(geometry->mesh + 1) + i;

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

static rwVertexDescriptor* VtxDescInitOptimized(RpGeometry* geometry,
                                                rwGCNVtxDataRemapped* vertexDataRemapped)
{
    RwUInt32 numAttr;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 j;

    vtxFmt = GEOMVTXFMT(geometry);
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

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS)
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

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
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

    if (RpGeometryGetFlags(geometry) & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        for (j = 0; j < (RwUInt32)geometry->numTexCoordSets; j++)
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
                                     rwGCNVtxDataRemapped* vtxDataRemapped, RpGeometry* geometry)
{
    RwUInt32 numElements;
    RwUInt32 flags;
    RwUInt8 numTexCoords;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    numTexCoords = (RwUInt8)geometry->numTexCoordSets;
    flags = RpGeometryGetFlags(geometry);

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    numElements = 0;

    vtxBufferData->num[rwGCNVA_POS] = vtxDataRemapped[numElements].num;
    vtxBufferData->data[rwGCNVA_POS] = vtxDataRemapped[numElements].data;
    numElements++;

    if (flags & rpGEOMETRYNORMALS)
    {
        vtxBufferData->num[rwGCNVA_NRM] = vtxDataRemapped[numElements].num;
        vtxBufferData->data[rwGCNVA_NRM] = vtxDataRemapped[numElements].data;
        numElements++;
    }

    if (flags & rpGEOMETRYPRELIT)
    {
        vtxBufferData->num[rwGCNVA_CLR0] = vtxDataRemapped[numElements].num;
        vtxBufferData->data[rwGCNVA_CLR0] = vtxDataRemapped[numElements].data;
        numElements++;
    }

    if (flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
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
                                    rwGCNIndexDataRemapped* indexDataRemapped, RpGeometry* geometry)
{
    RwUInt32 numElements;
    RwUInt32 flags;
    RwUInt8 numTexCoords;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    numTexCoords = (RwUInt8)geometry->numTexCoordSets;
    flags = RpGeometryGetFlags(geometry);

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    numElements = 0;

    indexBufferData->data[rwGCNVA_POS] = indexDataRemapped[numElements].indices;
    numElements++;

    if (flags & rpGEOMETRYNORMALS)
    {
        indexBufferData->data[rwGCNVA_NRM] = indexDataRemapped[numElements].indices;
        numElements++;
    }

    if (flags & rpGEOMETRYPRELIT)
    {
        indexBufferData->data[rwGCNVA_CLR0] = indexDataRemapped[numElements].indices;
        numElements++;
    }

    if (flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        for (i = 0; i < numTexCoords; i++)
        {
            indexBufferData->data[rwGCNVA_TEX0 + i] = indexDataRemapped[numElements].indices;
            numElements++;
        }
    }
}

RwResEntry* _rwDlGeometryInstanceOptimized(RpGeometry* geometry, void* owner,
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
    rwGCNVtxDataRemapped* vertexDataRemapped;
    rwGCNIndexDataRemapped** indexDataRemapped;
    rwVertexDescriptor* vtxDesc;
    rwGCNVertexBufferData vtxBufData;
    RwUInt8 primTypeVAT;
    RpGameCubeVtxFmt* vtxFmt;
    rwGCNDisplayListData displayListData;

    ReconditionVertexIndexData(geometry, &vertexDataRemapped, &indexDataRemapped);

    vtxDesc = VtxDescInitOptimized(geometry, vertexDataRemapped);
    VertexDataSetupOptimized(&vtxBufData, vertexDataRemapped, geometry);

    /* Work out how much memory we need */
    vBufferHeaderSize = _rwGCNVertexBufferHeaderGetSize(vtxDesc);
    displayListSize = geometry->mesh->numMeshes * sizeof(RxGameCubeDisplayList);
    size = vBufferHeaderSize + displayListSize;
    size += 31;

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYTRISTRIP)
    {
        primTypeVAT = rwGCNPRIMTRIANGLESTRIP;
    }
    else
    {
        primTypeVAT = rwGCNPRIMTRIANGLES;
    }

    for (i = 0; i < geometry->mesh->numMeshes; i++)
    {
        if (RpGeometryGetFlags(geometry) & rpGEOMETRYTRISTRIP)
        {
            RwUInt32 numStrips;
            RwUInt32 numIndices;

            _rwGCNTriStripGetStats(indexDataRemapped[i][0].indices, GEOMETRYGETMESH(geometry, i)->numIndices, &numStrips,
                                   &numIndices, TRUE);
            size += _rwGCNDisplayListGetSize(vtxDesc, numStrips, numIndices);
        }
        else
        {
            size += _rwGCNDisplayListGetSize(vtxDesc, 1, GEOMETRYGETMESH(geometry, i)->numIndices);
        }
    }

    size += _rwGCNVertexBufferGetSize(vtxDesc, &vtxBufData);

    /* Pre-instanced data lives outside of the resource arena */
    resEntry = (RwResEntry*)RwMalloc(sizeof(RwResEntry) + size);
    resEntry->link.next = NULL;
    resEntry->link.prev = NULL;
    resEntry->owner = owner;
    resEntry->size = size;
    resEntry->ownerRef = resEntryOwner;
    resEntry->destroyNotify = _rxGCResEntryWaitDone;
    *resEntryOwner = resEntry;

    vBufferSize = size;

    memory = resEntry + 1;
    memset(memory, 0, vBufferSize);

    vbHeader = (RxGameCubeVertexBuffer*)memory;
    vbHeader->token = _RwDlTokenLastSeen;
    vbHeader->serialNumber = geometry->mesh->serialNum;
    vbHeader->flags = 0;

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
    {
        vtxFmt = GEOMVTXFMT(geometry);
        if (vtxFmt == NULL)
        {
            RwInt32 j;

            vbHeader->flags &= ~1;

            for (j = 0; j < geometry->numVertices; j++)
            {
                if (geometry->preLitLum[j].alpha < 0xFF)
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

    memory = (RwUInt8*)memory + vBufferHeaderSize;
    displayLists = (RxGameCubeDisplayList*)memory;
    memory = (RwUInt8*)memory + displayListSize;
    memory = (RwUInt8*)memory + 31;
    memory = (void*)((RwUInt32)memory & ~31);

    for (i = 0; i < geometry->mesh->numMeshes; i++)
    {
        RwUInt32 numStrips;
        RwUInt32 numIndices;
        RwUInt32 displayListstride;
        RwUInt32 meshDisplayListSize;

        if (RpGeometryGetFlags(geometry) & rpGEOMETRYTRISTRIP)
        {
            _rwGCNTriStripGetStats(indexDataRemapped[i][0].indices, GEOMETRYGETMESH(geometry, i)->numIndices, &numStrips,
                                   &numIndices, TRUE);
        }
        else
        {
            numIndices = GEOMETRYGETMESH(geometry, i)->numIndices;
            numStrips = 1;
        }

        meshDisplayListSize = _rwGCNDisplayListGetSize(vtxDesc, numStrips, numIndices);
        _rwGCNDisplayListInitialize(&displayLists[i], i, meshDisplayListSize, memory);

        IndexDataSetupOptimized(&displayListData, indexDataRemapped[i], geometry);

        displayListstride = _rwGCNDisplayListGetStride(vtxDesc);
        _rwGCNDisplayListFill(vtxDesc, &displayLists[i], &displayListData, GEOMETRYGETMESH(geometry, i)->numIndices,
                              numStrips, displayListstride, TRUE, primTypeVAT);

        memory = (RwUInt8*)memory + meshDisplayListSize;
    }

    _rwGCNVertexBufferInitialize(vtxDesc, vbHeader, &vtxBufData, (RwUInt8*)memory);
    _rwGCNVertexBufferFill(vtxDesc, vbHeader, &vtxBufData, FALSE);

    DCFlushRange(vbHeader, vBufferSize);
    GXInvalidateVtxCache();

    for (i = 0; i < geometry->mesh->numMeshes; i++)
    {
        RwFree(indexDataRemapped[i]);
    }
    RwFree(indexDataRemapped);
    RwFree(vertexDataRemapped);

    return resEntry;
}

static rwVertexDescriptor* VtxDescInitFast(RpGeometry* geometry)
{
    RwUInt32 numAttr;
    RwUInt32 numVerts;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 j;

    numVerts = geometry->numVertices;

    vtxFmt = GEOMVTXFMT(geometry);
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

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS)
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

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
    {
        _rwGCNVertexDescSetElementAttr(&VtxDesc, rwGCNVA_CLR0,
                                       (vtxFmt->preLight > rpRGBX8) ? rwGCNCC_CLR_RGBA
                                                                    : rwGCNCC_CLR_RGB,
                                       (rwGCNCompType)vtxFmt->preLight, 0);
        _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_CLR0,
                                       (numVerts >= 0xFF) ? rwGCNAT_INDEX16 : rwGCNAT_INDEX8);
        numAttr++;
    }

    if (RpGeometryGetFlags(geometry) & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        for (j = 0; j < (RwUInt32)geometry->numTexCoordSets; j++)
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

static void VertexDataFastSetup(rwGCNVertexBufferData* vtxBufferData, RpGeometry* geometry)
{
    RpMorphTarget* morphTarget;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    morphTarget = geometry->morphTarget;

    vtxBufferData->num[rwGCNVA_POS] = geometry->numVertices;
    vtxBufferData->data[rwGCNVA_POS] = morphTarget->verts;

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS)
    {
        vtxBufferData->num[rwGCNVA_NRM] = geometry->numVertices;
        vtxBufferData->data[rwGCNVA_NRM] = morphTarget->normals;
    }

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
    {
        vtxBufferData->num[rwGCNVA_CLR0] = geometry->numVertices;
        vtxBufferData->data[rwGCNVA_CLR0] = geometry->preLitLum;
    }

    if (RpGeometryGetFlags(geometry) & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        for (i = 0; i < (RwUInt32)geometry->numTexCoordSets; i++)
        {
            vtxBufferData->num[rwGCNVA_TEX0 + i] = geometry->numVertices;
            vtxBufferData->data[rwGCNVA_TEX0 + i] = geometry->texCoords[i];
        }
    }
}

static void IndexDataSetupFast(rwGCNDisplayListData* indexBufferData, RwUInt16* indices,
                               RpGeometry* geometry)
{
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    indexBufferData->data[rwGCNVA_POS] = indices;

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS)
    {
        indexBufferData->data[rwGCNVA_NRM] = indices;
    }

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
    {
        indexBufferData->data[rwGCNVA_CLR0] = indices;
    }

    if (RpGeometryGetFlags(geometry) & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        for (i = 0; i < (RwUInt32)geometry->numTexCoordSets; i++)
        {
            indexBufferData->data[rwGCNVA_TEX0 + i] = indices;
        }
    }
}

RwResEntry* _rwDlGeometryInstanceFast(RpGeometry* geometry, void* owner,
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

    vtxDesc = VtxDescInitFast(geometry);
    VertexDataFastSetup(&vtxBufData, geometry);

    /* Work out how much memory we need */
    vBufferHeaderSize = _rwGCNVertexBufferHeaderGetSize(vtxDesc);
    displayListSize = geometry->mesh->numMeshes * sizeof(RxGameCubeDisplayList);
    size = vBufferHeaderSize + displayListSize;
    size += 31;

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYTRISTRIP)
    {
        primTypeVAT = rwGCNPRIMTRIANGLESTRIP;
    }
    else
    {
        primTypeVAT = rwGCNPRIMTRIANGLES;
    }

    for (i = 0; i < geometry->mesh->numMeshes; i++)
    {
        size += _rwGCNDisplayListGetSize(vtxDesc, 1, GEOMETRYGETMESH(geometry, i)->numIndices);
    }

    size += _rwGCNVertexBufferGetSize(vtxDesc, &vtxBufData);

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNATIVEINSTANCE)
    {
        /* Pre-instanced data lives outside of the resource arena */
        resEntry = (RwResEntry*)RwMalloc(sizeof(RwResEntry) + size);
        resEntry->link.next = NULL;
        resEntry->link.prev = NULL;
        resEntry->owner = owner;
        resEntry->size = size;
        resEntry->ownerRef = resEntryOwner;
        resEntry->destroyNotify = _rxGCResEntryWaitDone;
        *resEntryOwner = resEntry;
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
    vbHeader->serialNumber = geometry->mesh->serialNum;
    vbHeader->flags = 0;

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
    {
        vtxFmt = GEOMVTXFMT(geometry);
        if (vtxFmt == NULL)
        {
            RwInt32 j;

            vbHeader->flags &= ~1;

            for (j = 0; j < geometry->numVertices; j++)
            {
                if (geometry->preLitLum[j].alpha < 0xFF)
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

    memory = (RwUInt8*)memory + vBufferHeaderSize;
    displayLists = (RxGameCubeDisplayList*)memory;
    memory = (RwUInt8*)memory + displayListSize;
    memory = (RwUInt8*)memory + 31;
    memory = (void*)((RwUInt32)memory & ~31);

    for (i = 0; i < geometry->mesh->numMeshes; i++)
    {
        RwUInt32 meshDisplayListSize;

        meshDisplayListSize = _rwGCNDisplayListGetSize(vtxDesc, 1, GEOMETRYGETMESH(geometry, i)->numIndices);
        _rwGCNDisplayListInitialize(&displayLists[i], i, meshDisplayListSize, memory);

        IndexDataSetupFast(&displayListData, GEOMETRYGETMESH(geometry, i)->indices, geometry);

        _rwGCNDisplayListFill(vtxDesc, &displayLists[i], &displayListData, GEOMETRYGETMESH(geometry, i)->numIndices, 1,
                              _rwGCNDisplayListGetStride(vtxDesc), TRUE, primTypeVAT);

        memory = (RwUInt8*)memory + meshDisplayListSize;
    }

    _rwGCNVertexBufferInitialize(vtxDesc, vbHeader, &vtxBufData, (RwUInt8*)memory);
    _rwGCNVertexBufferFill(vtxDesc, vbHeader, &vtxBufData, FALSE);

    DCFlushRange(vbHeader, vBufferSize);
    GXInvalidateVtxCache();

    return resEntry;
}
