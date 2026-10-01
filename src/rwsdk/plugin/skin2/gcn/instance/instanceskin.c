#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/os.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"
#include "rwsdk/plugin/skin2/skin.h"

#define rwGCNMAXVTXDATA 16

/* Bones referenced by a single mesh of a split skin */
#define rwGCNMAXMESHBONES 10

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
    RpSkin* skin;

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

    skin = RpSkinGeometryGetSkin(geometry);

    /* Matrix indices */
    if (skin->vertexMaps.maxWeights > 1)
    {
        vtxData[numEntries].data = skin->platformData.indices;

        switch (skin->vertexMaps.maxWeights)
        {
        case 1:
            vtxData[numEntries].type = rwDATATYPE_INT8;
            break;
        case 2:
            vtxData[numEntries].type = rwDATATYPE_INT16;
            break;
        case 3:
            vtxData[numEntries].type = rwDATATYPE_INT24;
            break;
        case 4:
            vtxData[numEntries].type = rwDATATYPE_INT32;
            break;
        }
    }
    else
    {
        vtxData[numEntries].data = skin->vertexMaps.matrixIndices;
        vtxData[numEntries].type = rwDATATYPE_INT32;
    }
    vtxData[numEntries].dep[0] = -1;
    numEntries++;

    /* Matrix weights */
    if (skin->vertexMaps.maxWeights > 1)
    {
        vtxData[numEntries].data = skin->platformData.weights;
        vtxData[numEntries].type = vtxData[numEntries - 1].type;
        vtxData[numEntries].dep[0] = -1;
        numEntries++;
    }
    else
    {
        vtxData[numEntries].data = skin->vertexMaps.matrixWeights;
        vtxData[numEntries].type = rwDATATYPE_V4D;
        vtxData[numEntries].dep[0] = -1;
        numEntries++;
    }

    /* Positions, normals, matrix indices and weights must be remapped together */
    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS)
    {
        vtxData[0].dep[0] = 1;
        vtxData[0].dep[1] = numEntries - 2;
        vtxData[0].dep[2] = numEntries - 1;
        vtxData[0].dep[3] = -1;

        vtxData[1].dep[0] = 0;
        vtxData[1].dep[1] = numEntries - 2;
        vtxData[1].dep[2] = numEntries - 1;
        vtxData[1].dep[3] = -1;

        vtxData[numEntries - 2].dep[0] = 0;
        vtxData[numEntries - 2].dep[1] = 1;
        vtxData[numEntries - 2].dep[2] = numEntries - 1;
        vtxData[numEntries - 2].dep[3] = -1;

        vtxData[numEntries - 1].dep[0] = 0;
        vtxData[numEntries - 1].dep[1] = 1;
        vtxData[numEntries - 1].dep[2] = numEntries - 2;
        vtxData[numEntries - 1].dep[3] = -1;
    }
    else
    {
        vtxData[0].dep[0] = numEntries - 2;
        vtxData[0].dep[1] = numEntries - 1;
        vtxData[0].dep[2] = -1;

        vtxData[numEntries - 2].dep[0] = 0;
        vtxData[numEntries - 2].dep[1] = numEntries - 1;
        vtxData[numEntries - 2].dep[2] = -1;

        vtxData[numEntries - 1].dep[0] = 0;
        vtxData[numEntries - 1].dep[1] = numEntries - 2;
        vtxData[numEntries - 1].dep[2] = -1;
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

    /* The skinning data has no index streams of its own */
    indexData =
        (rwGCNIndexData*)RwMalloc((numEntries - 2) * (numMeshes * sizeof(rwGCNIndexData)));
    if (indexData == NULL)
    {
        RwFree(*vertexDataRemapped);
        RwFree(vertexDataMaps);
        return FALSE;
    }

    memset(indexData, 0, (numEntries - 2) * (numMeshes * sizeof(rwGCNIndexData)));

    for (i = 0; i < numMeshes; i++)
    {
        RpMesh* mesh;
        RwInt32 j;

        mesh = (RpMesh*)(geometry->mesh + 1) + i;

        for (j = 0; j < numEntries - 2; j++)
        {
            indexData[i * (numEntries - 2) + j].indices = mesh->indices;
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

        (*indexDataRemapped)[i] =
            IndexDataCreateRemapped(vertexDataMaps, &indexData[i * (numEntries - 2)],
                                    numEntries - 2, mesh->numIndices);
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
    RpSkin* skin;
    RwUInt32 j;

    skin = RpSkinGeometryGetSkin(geometry);

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    _rwVertexDescriptorInit(&VtxDesc);
    _rwGCNVertexDescSetVAT(&VtxDesc, 0);

    /* Single weight skins select their matrix per vertex */
    if (skin->vertexMaps.maxWeights == 1)
    {
        _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_PNMTXIDX, rwGCNAT_DIRECT);
    }

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
                                    rwGCNIndexDataRemapped* indexDataRemapped, RpGeometry* geometry,
                                    RwUInt16* matrixIndices)
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

    if (matrixIndices != NULL)
    {
        indexBufferData->data[rwGCNVA_PNMTXIDX] = matrixIndices;
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

static RwUInt16* CreateMatrixIndexListOptimized(RpSkin* skin, RwUInt16* indices,
                                                RwUInt32* matrixIndices, RwInt32 numIndices,
                                                RwUInt32 meshIndex)
{
    RwInt32 j;
    RwUInt32 k;
    RwUInt16* indexList;

    if (skin->skinSplitData.numMeshes)
    {
        RwUInt8* bone;
        RwUInt8 boneMap[rwGCNMAXMESHBONES];
        RwUInt8 index;
        RwUInt8 count;

        /* Expand the run length encoded bone list of this mesh */
        bone = boneMap;

        index = skin->skinSplitData.meshRLECount[meshIndex * 2] * 2;
        count = skin->skinSplitData.meshRLECount[meshIndex * 2 + 1];

        for (k = 0; k < count; k++)
        {
            RwUInt8 start;
            RwUInt8 run;
            RwUInt8 b;

            start = skin->skinSplitData.meshRLE[index + k * 2];
            run = skin->skinSplitData.meshRLE[index + k * 2 + 1];

            for (b = 0; b < run; b++)
            {
                *bone++ = start + b;
            }
        }

        indexList = (RwUInt16*)RwMalloc(numIndices * sizeof(RwUInt16));

        for (k = 0; k < (RwUInt32)numIndices; k++)
        {
            for (j = 0; j < rwGCNMAXMESHBONES; j++)
            {
                if (boneMap[j] == (RwUInt8)matrixIndices[indices[k]])
                {
                    indexList[k] = (RwUInt16)(j * 3);
                    break;
                }
            }
        }
    }
    else
    {
        indexList = (RwUInt16*)RwMalloc(numIndices * sizeof(RwUInt16));

        for (k = 0; k < (RwUInt32)numIndices; k++)
        {
            for (j = 0; j < skin->boneData.numUsedBones; j++)
            {
                if (skin->boneData.usedBoneList[j] == (RwUInt8)matrixIndices[indices[k]])
                {
                    indexList[k] = (RwUInt16)(j * 3);
                    break;
                }
            }
        }
    }

    return indexList;
}

RwResEntry* _rwDlGeometrySkinInstanceOptimized(RpGeometry* geometry, void* owner,
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
    RpSkin* skin;
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

            _rwGCNTriStripGetStats(indexDataRemapped[i][0].indices,
                                   GEOMETRYGETMESH(geometry, i)->numIndices, &numStrips,
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

    skin = RpSkinGeometryGetSkin(geometry);

    for (i = 0; i < geometry->mesh->numMeshes; i++)
    {
        RwUInt32 numStrips;
        RwUInt32 numIndices;
        RwUInt32 displayListStride;
        RwUInt32 meshDisplayListSize;
        RwUInt16* matrixIndices;

        matrixIndices = NULL;

        if (RpGeometryGetFlags(geometry) & rpGEOMETRYTRISTRIP)
        {
            _rwGCNTriStripGetStats(indexDataRemapped[i][0].indices,
                                   GEOMETRYGETMESH(geometry, i)->numIndices, &numStrips,
                                   &numIndices, TRUE);
        }
        else
        {
            numIndices = GEOMETRYGETMESH(geometry, i)->numIndices;
            numStrips = 1;
        }

        meshDisplayListSize = _rwGCNDisplayListGetSize(vtxDesc, numStrips, numIndices);
        _rwGCNDisplayListInitialize(&displayLists[i], i, meshDisplayListSize, memory);

        if (skin->vertexMaps.maxWeights > 1)
        {
            IndexDataSetupOptimized(&displayListData, indexDataRemapped[i], geometry,
                                    matrixIndices);
        }
        else
        {
            RwInt32 numElements;
            RwInt32 numTexCoords;
            RwUInt32 j;

            /* Find the remapped matrix indices */
            numElements = 1;

            if (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS)
            {
                numElements++;
            }

            if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
            {
                numElements++;
            }

            if (RpGeometryGetFlags(geometry) & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
            {
                numTexCoords = geometry->numTexCoordSets;
                for (j = 0; j < (RwUInt32)numTexCoords; j++)
                {
                    numElements++;
                }
            }

            matrixIndices = CreateMatrixIndexListOptimized(
                skin, indexDataRemapped[i][0].indices,
                (RwUInt32*)vertexDataRemapped[numElements].data,
                GEOMETRYGETMESH(geometry, i)->numIndices, i);

            IndexDataSetupOptimized(&displayListData, indexDataRemapped[i], geometry,
                                    matrixIndices);
        }

        displayListStride = _rwGCNDisplayListGetStride(vtxDesc);

        _rwGCNDisplayListFill(vtxDesc, &displayLists[i], &displayListData,
                              GEOMETRYGETMESH(geometry, i)->numIndices, numStrips,
                              displayListStride, TRUE, primTypeVAT);

        if (matrixIndices != NULL)
        {
            RwFree(matrixIndices);
        }

        memory = (RwUInt8*)memory + meshDisplayListSize;
    }

    _rwGCNVertexBufferInitialize(vtxDesc, vbHeader, &vtxBufData, (RwUInt8*)memory);
    _rwGCNVertexBufferFill(vtxDesc, vbHeader, &vtxBufData, FALSE);

    /* Hand the remapped skinning data back to the skin */
    if (skin->vertexMaps.maxWeights > 1)
    {
        RwInt32 numElements;
        RwInt32 numTexCoords;
        RwUInt32 j;

        numElements = 1;

        if (RpGeometryGetFlags(geometry) & rpGEOMETRYNORMALS)
        {
            numElements++;
        }

        if (RpGeometryGetFlags(geometry) & rpGEOMETRYPRELIT)
        {
            numElements++;
        }

        if (RpGeometryGetFlags(geometry) & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
        {
            numTexCoords = geometry->numTexCoordSets;
            for (j = 0; j < (RwUInt32)numTexCoords; j++)
            {
                numElements++;
            }
        }

        memcpy(skin->platformData.indices, vertexDataRemapped[numElements].data,
               skin->vertexMaps.maxWeights * vertexDataRemapped[numElements].num);
        memcpy(skin->platformData.weights, vertexDataRemapped[numElements + 1].data,
               skin->vertexMaps.maxWeights * vertexDataRemapped[numElements + 1].num);
    }

    geometry->numVertices = vertexDataRemapped[0].num;

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
    _rwGCNVertexDescSetElementDesc(&VtxDesc, rwGCNVA_PNMTXIDX, rwGCNAT_DIRECT);

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
                               RwUInt16* matrixIndices, RpGeometry* geometry)
{
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    indexBufferData->data[rwGCNVA_PNMTXIDX] = matrixIndices;
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

static RwUInt16* CreateMatrixIndexList(RpSkin* skin, RpMesh* mesh, RwUInt32 meshIndex)
{
    RwUInt32 j;
    RwUInt32 k;
    RwUInt16* indexList;

    if (skin->skinSplitData.numMeshes)
    {
        RwUInt8* bone;
        RwUInt8 boneMap[rwGCNMAXMESHBONES];
        RwUInt8 index;
        RwUInt8 count;

        /* Expand the run length encoded bone list of this mesh */
        bone = boneMap;

        index = skin->skinSplitData.meshRLECount[meshIndex * 2] * 2;
        count = skin->skinSplitData.meshRLECount[meshIndex * 2 + 1];

        for (k = 0; k < count; k++)
        {
            RwUInt8 start;
            RwUInt8 run;
            RwUInt8 b;

            start = skin->skinSplitData.meshRLE[index + k * 2];
            run = skin->skinSplitData.meshRLE[index + k * 2 + 1];

            for (b = 0; b < run; b++)
            {
                *bone++ = start + b;
            }
        }

        indexList = (RwUInt16*)RwMalloc(mesh->numIndices * sizeof(RwUInt16));

        for (k = 0; k < mesh->numIndices; k++)
        {
            for (j = 0; j < rwGCNMAXMESHBONES; j++)
            {
                if (boneMap[j] == (RwUInt8)skin->vertexMaps.matrixIndices[mesh->indices[k]])
                {
                    indexList[k] = (RwUInt16)(j * 3);
                    break;
                }
            }
        }
    }
    else
    {
        indexList = (RwUInt16*)RwMalloc(mesh->numIndices * sizeof(RwUInt16));

        for (k = 0; k < mesh->numIndices; k++)
        {
            for (j = 0; j < skin->boneData.numUsedBones; j++)
            {
                if (skin->boneData.usedBoneList[j] ==
                    (RwUInt8)skin->vertexMaps.matrixIndices[mesh->indices[k]])
                {
                    indexList[k] = (RwUInt16)(j * 3);
                    break;
                }
            }
        }
    }

    return indexList;
}

RwResEntry* _rwDlGeometrySkinInstanceFast(RpGeometry* geometry, void* owner,
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
    RpSkin* skin;
    RpGameCubeVtxFmt* vtxFmt;
    rwGCNDisplayListData displayListData;

    skin = RpSkinGeometryGetSkin(geometry);

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
        RwUInt16* matrixIndices;
        RpMesh* mesh;

        mesh = GEOMETRYGETMESH(geometry, i);

        meshDisplayListSize = _rwGCNDisplayListGetSize(vtxDesc, 1, mesh->numIndices);
        _rwGCNDisplayListInitialize(&displayLists[i], i, meshDisplayListSize, memory);

        matrixIndices = CreateMatrixIndexList(skin, mesh, i);

        IndexDataSetupFast(&displayListData, mesh->indices, matrixIndices, geometry);

        _rwGCNDisplayListFill(vtxDesc, &displayLists[i], &displayListData,
                              GEOMETRYGETMESH(geometry, i)->numIndices, 1,
                              _rwGCNDisplayListGetStride(vtxDesc), TRUE, primTypeVAT);

        RwFree(matrixIndices);

        memory = (RwUInt8*)memory + meshDisplayListSize;
    }

    _rwGCNVertexBufferInitialize(vtxDesc, vbHeader, &vtxBufData, (RwUInt8*)memory);
    _rwGCNVertexBufferFill(vtxDesc, vbHeader, &vtxBufData, FALSE);

    DCFlushRange(vbHeader, vBufferSize);
    GXInvalidateVtxCache();

    return resEntry;
}
