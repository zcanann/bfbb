#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

static RwUInt32 TypeGetSize(rwDataType type)
{
    switch (type)
    {
    case rwDATATYPE_INT8:
        return 1;
    case rwDATATYPE_INT16:
        return 2;
    case rwDATATYPE_INT24:
        return 3;
    case rwDATATYPE_INT32:
        return 4;
    case rwDATATYPE_REAL:
        return sizeof(RwReal);
    case rwDATATYPE_V2D:
        return sizeof(RwV2d);
    case rwDATATYPE_V3D:
        return sizeof(RwV3d);
    case rwDATATYPE_V4D:
        return 4 * sizeof(RwReal);
    case rwDATATYPE_RGBA:
        return sizeof(RwRGBA);
    }

    return 0;
}

static RwBool TypeCheckEqual(rwDataType type, void* a, void* b)
{
    switch (type)
    {
    case rwDATATYPE_INT8:
        return *(RwUInt8*)a == *(RwUInt8*)b;
    case rwDATATYPE_INT16:
        return *(RwUInt16*)a == *(RwUInt16*)b;
    case rwDATATYPE_INT24:
        return ((RwUInt8*)a)[0] == ((RwUInt8*)b)[0] && ((RwUInt8*)a)[1] == ((RwUInt8*)b)[1] &&
               ((RwUInt8*)a)[2] == ((RwUInt8*)b)[2];
    case rwDATATYPE_INT32:
        return *(RwUInt32*)a == *(RwUInt32*)b;
    case rwDATATYPE_REAL:
        return *(RwUInt32*)a == *(RwUInt32*)b;
    case rwDATATYPE_V2D:
        return ((RwUInt32*)a)[0] == ((RwUInt32*)b)[0] && ((RwUInt32*)a)[1] == ((RwUInt32*)b)[1];
    case rwDATATYPE_V3D:
        return ((RwUInt32*)a)[0] == ((RwUInt32*)b)[0] && ((RwUInt32*)a)[1] == ((RwUInt32*)b)[1] &&
               ((RwUInt32*)a)[2] == ((RwUInt32*)b)[2];
    case rwDATATYPE_V4D:
        return ((RwUInt32*)a)[0] == ((RwUInt32*)b)[0] && ((RwUInt32*)a)[1] == ((RwUInt32*)b)[1] &&
               ((RwUInt32*)a)[2] == ((RwUInt32*)b)[2] && ((RwUInt32*)a)[3] == ((RwUInt32*)b)[3];
    case rwDATATYPE_RGBA:
        return *(RwUInt32*)a == *(RwUInt32*)b;
    }

    return FALSE;
}

static void IndicesRemap(RwUInt16* dstIndexList, RwUInt16* srcIndexList, RwInt32* map,
                         RwUInt32 numIndices)
{
    RwUInt32 i;
    RwInt32 numHoles;
    RwUInt16 j;
    RwUInt16 index;

    for (i = 0; i < numIndices; i++)
    {
        index = srcIndexList[i];
        numHoles = 0;

        /* Redirect duplicates to the vertex they were merged with */
        if (map[index] != -1)
        {
            index = (RwUInt16)map[index];
        }

        /* Then close up the holes left by the removed vertices */
        for (j = 0; j < index; j++)
        {
            if (map[j] != -1)
            {
                numHoles++;
            }
        }

        dstIndexList[i] = index - numHoles;
    }
}

rwGCNIndexDataRemapped* IndexDataCreateRemapped(rwGCNVtxDataMap* vtxDataMap,
                                                rwGCNIndexData* indexData, RwUInt32 numEntries,
                                                RwUInt32 numIndices)
{
    RwUInt32 size;
    rwGCNIndexDataRemapped* indexDataRemapped;
    RwUInt32 offset;
    RwUInt32 i;

    size = numIndices * sizeof(RwUInt16);

    indexDataRemapped = (rwGCNIndexDataRemapped*)RwMalloc(
        numEntries * sizeof(rwGCNIndexDataRemapped) + numEntries * size);
    if (indexDataRemapped == NULL)
    {
        return NULL;
    }

    memset(indexDataRemapped, 0, numEntries * sizeof(rwGCNIndexDataRemapped));

    offset = numEntries * sizeof(rwGCNIndexDataRemapped);
    for (i = 0; i < numEntries; i++)
    {
        indexDataRemapped[i].indices = (RwUInt16*)((RwUInt8*)indexDataRemapped + offset);
        offset += size;
    }

    for (i = 0; i < numEntries; i++)
    {
        IndicesRemap(indexDataRemapped[i].indices, indexData[i].indices, vtxDataMap[i].map,
                     numIndices);
    }

    return indexDataRemapped;
}

rwGCNVtxDataRemapped* VertexDataCreateRemapped(rwGCNVtxDataMap* vtxDataMap, rwGCNVtxData* vtxData,
                                               RwUInt32 numEntries, RwUInt32 numVerts)
{
    RwUInt32 i;
    RwUInt32 size;
    RwUInt32 offset;
    rwGCNVtxDataRemapped* vtxDataRemapped;
    RwUInt32 j;
    RwUInt8* srcData;
    RwUInt8* dstData;

    size = numEntries * sizeof(rwGCNVtxDataRemapped);
    for (i = 0; i < numEntries; i++)
    {
        size += vtxDataMap[i].num * TypeGetSize(vtxData[i].type);
    }

    vtxDataRemapped = (rwGCNVtxDataRemapped*)RwMalloc(size);
    if (vtxDataRemapped == NULL)
    {
        return NULL;
    }

    memset(vtxDataRemapped, 0, numEntries * sizeof(rwGCNVtxDataRemapped));

    offset = numEntries * sizeof(rwGCNVtxDataRemapped);
    for (i = 0; i < numEntries; i++)
    {
        vtxDataRemapped[i].data = (RwUInt8*)vtxDataRemapped + offset;
        offset += vtxDataMap[i].num * TypeGetSize(vtxData[i].type);
    }

    for (i = 0; i < numEntries; i++)
    {
        size = TypeGetSize(vtxData[i].type);
        srcData = (RwUInt8*)vtxData[i].data;
        dstData = (RwUInt8*)vtxDataRemapped[i].data;

        /* Keep only the vertices that were not merged into another */
        for (j = 0; j < numVerts; j++)
        {
            if (vtxDataMap[i].map[j] == -1)
            {
                memcpy(dstData, srcData, size);
                dstData += size;
            }
            srcData += size;
        }

        vtxDataRemapped[i].num = vtxDataMap[i].num;
    }

    return vtxDataRemapped;
}

rwGCNVtxDataMap* VertexDataCreateMaps(rwGCNVtxData* vtxData, RwUInt32 numEntries,
                                      RwUInt32 numVerts)
{
    RwUInt32 offset;
    RwUInt32 count;
    RwUInt32 size;
    RwUInt32 i;
    rwGCNVtxDataMap* vtxDataMap;
    RwUInt32 j;
    RwUInt32 k;
    RwUInt32 depSize;
    RwBool equal;
    RwInt8 dep;
    RwInt8 depIndex;

    count = numEntries * numVerts;

    vtxDataMap = (rwGCNVtxDataMap*)RwMalloc(numEntries * sizeof(rwGCNVtxDataMap) +
                                            count * sizeof(RwInt32));
    if (vtxDataMap == NULL)
    {
        return NULL;
    }

    memset(vtxDataMap, 0, numEntries * sizeof(rwGCNVtxDataMap));

    offset = numEntries * sizeof(rwGCNVtxDataMap);
    for (i = 0; i < count; i++)
    {
        ((RwInt32*)(vtxDataMap + numEntries))[i] = -1;
    }

    for (i = 0; i < numEntries; i++)
    {
        vtxDataMap[i].map = (RwInt32*)((RwUInt8*)vtxDataMap + offset);
        vtxDataMap[i].num = numVerts;
        offset += numVerts * sizeof(RwInt32);
    }

    for (i = 0; i < numEntries; i++)
    {
        size = TypeGetSize(vtxData[i].type);

        for (j = 0; j < numVerts; j++)
        {
            if (vtxDataMap[i].map[j] != -1)
            {
                continue;
            }

            /* Look for later vertices which are identical to this one */
            for (k = j; k < numVerts; k++)
            {
                if (j == k)
                {
                    continue;
                }

                if (!TypeCheckEqual(vtxData[i].type, (RwUInt8*)vtxData[i].data + j * size,
                                    (RwUInt8*)vtxData[i].data + k * size))
                {
                    continue;
                }

                /* All the dependent data must match too */
                equal = TRUE;

                depIndex = 0;
                dep = vtxData[i].dep[depIndex];
                depSize = TypeGetSize(vtxData[dep].type);
                while (dep >= 0)
                {
                    if (!TypeCheckEqual(vtxData[dep].type,
                                        (RwUInt8*)vtxData[dep].data + depSize * j,
                                        (RwUInt8*)vtxData[dep].data + depSize * k))
                    {
                        equal = FALSE;
                        break;
                    }

                    depIndex++;
                    dep = vtxData[i].dep[depIndex];
                    depSize = TypeGetSize(vtxData[dep].type);
                }

                if (equal)
                {
                    vtxDataMap[i].map[k] = j;
                    vtxDataMap[i].num--;
                }
            }
        }
    }

    return vtxDataMap;
}
