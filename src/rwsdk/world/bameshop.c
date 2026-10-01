#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include <stdlib.h>
#include <string.h>

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

#define rpMESHHEADERTRISTRIP 0x0001

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rpMeshGlobals rpMeshGlobals;
struct rpMeshGlobals
{
    RwInt16 nextSerialNum;
    RwFreeList* triStripListEntryFreeList;
};

typedef RpMeshHeader* (*RpMeshTriStripMethod)(RpBuildMesh* buildMesh, void* data);

typedef struct RpMeshopStatic RpMeshopStatic;
struct RpMeshopStatic
{
    RpMeshHeader nullMeshHeader;
    RpMeshTriStripMethod meshTristripMethod;
    void* data;
};

typedef struct TriBinEntry TriBinEntry;
typedef struct Edge Edge;

struct TriBinEntry
{
    RwUInt32 tri;
    Edge* edge[3];
    TriBinEntry* next;
    TriBinEntry* prev;
    RwBool used;
    RwBool used2;
    RwUInt8 adjCount;
};

struct Edge
{
    RwUInt16 v1;
    RwUInt16 v2;
    TriBinEntry* tri1;
    TriBinEntry* tri2;
    Edge* next;
};

typedef struct TriBinList TriBinList;
struct TriBinList
{
    TriBinEntry* head;
};

typedef struct TriStripListEntry TriStripListEntry;
struct TriStripListEntry
{
    RxVertexIndex* strip;
    RwUInt32 stripLen;
    RwUInt32 stripSize;
    TriStripListEntry* next;
};

typedef struct TriStripList TriStripList;
struct TriStripList
{
    TriStripListEntry* head;
};

typedef struct MeshOpFreeLists MeshOpFreeLists;
struct MeshOpFreeLists
{
    RwFreeList* binEntryFreeList;
    RwFreeList* edgeFreeList;
};

/* Unlink a triangle from the bin of triangles with the same number of free neighbours */
#define TriBinListRemove(_binListArray, _entry)                                                    \
    MACRO_START                                                                                    \
    {                                                                                              \
        if ((_binListArray)[(_entry)->adjCount].head == (_entry))                                  \
        {                                                                                          \
            (_binListArray)[(_entry)->adjCount].head =                                             \
                (_binListArray)[(_entry)->adjCount].head->next;                                    \
            if ((_binListArray)[(_entry)->adjCount].head)                                          \
            {                                                                                      \
                (_binListArray)[(_entry)->adjCount].head->prev = (TriBinEntry*)NULL;               \
            }                                                                                      \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            if ((_entry)->next)                                                                    \
            {                                                                                      \
                (_entry)->next->prev = (_entry)->prev;                                             \
            }                                                                                      \
            if ((_entry)->prev)                                                                    \
            {                                                                                      \
                (_entry)->prev->next = (_entry)->next;                                             \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

/* Number of unused triangles on an edge, for the main and trial passes */
#define TriStripEdgeFreeTris(_edge)                                                                \
    (((_edge)->tri1 && !(_edge)->tri1->used) + ((_edge)->tri2 && !(_edge)->tri2->used))
#define TriStripEdgeFreeTris2(_edge)                                                               \
    (((_edge)->tri1 && !(_edge)->tri1->used2) + ((_edge)->tri2 && !(_edge)->tri2->used2))

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

#define RWMESHGLOBAL(var)                                                                          \
    (RWPLUGINOFFSET(rpMeshGlobals, RwEngineInstance, meshModule.globalsOffset)->var)

extern RwModuleInfo meshModule;

static RpMeshopStatic MeshopStatic = { { 0, 0, 0, 0, 0 },
                                       RpBuildMeshGenerateDefaultTriStrip,
                                       NULL };

static int SortPolygons(const void* pA, const void* pB)
{
    static const RwUInt32 transBIT = 16;
    const RpBuildMeshTriangle* const* mtpA = (const RpBuildMeshTriangle* const*)pA;
    const RpBuildMeshTriangle* const* mtpB = (const RpBuildMeshTriangle* const*)pB;
    RpMaterial* materialA = (*mtpA)->material;
    RpMaterial* materialB = (*mtpB)->material;
    RwUInt32 orderA = 0;
    RwUInt32 orderB = 0;

    if (materialA == materialB)
    {
        return 0;
    }

    /* Transparent materials go after opaque ones */
    if (materialA)
    {
        RwTexture* textureA = materialA->texture;

        if (textureA)
        {
            RwRaster* rasterA = textureA->raster;
            RwInt32 format = RwRasterGetFormat(rasterA) & rwRASTERFORMATPIXELFORMATMASK;

            if (format == rwRASTERFORMAT1555 || format == rwRASTERFORMAT4444 ||
                format == rwRASTERFORMAT8888)
            {
                orderA |= transBIT;
            }
        }

        if (materialA->color.alpha != 0xFF)
        {
            orderA |= transBIT;
        }
    }

    if (materialB)
    {
        RwTexture* textureB = materialB->texture;

        if (textureB)
        {
            RwRaster* rasterB = textureB->raster;
            RwInt32 format = RwRasterGetFormat(rasterB) & rwRASTERFORMATPIXELFORMATMASK;

            if (format == rwRASTERFORMAT1555 || format == rwRASTERFORMAT4444 ||
                format == rwRASTERFORMAT8888)
            {
                orderB |= transBIT;
            }
        }

        if (materialB->color.alpha != 0xFF)
        {
            orderB |= transBIT;
        }
    }

    orderA |= ((*mtpA)->rasterIndex > (*mtpB)->rasterIndex) ? 8 : 0;
    orderB |= ((*mtpA)->rasterIndex < (*mtpB)->rasterIndex) ? 8 : 0;

    orderA |= ((*mtpA)->pipelineIndex > (*mtpB)->pipelineIndex) ? 4 : 0;
    orderB |= ((*mtpA)->pipelineIndex < (*mtpB)->pipelineIndex) ? 4 : 0;

    orderA |= ((*mtpA)->textureIndex > (*mtpB)->textureIndex) ? 2 : 0;
    orderB |= ((*mtpA)->textureIndex < (*mtpB)->textureIndex) ? 2 : 0;

    orderA |= ((*mtpA)->matIndex > (*mtpB)->matIndex) ? 1 : 0;
    orderB |= ((*mtpA)->matIndex < (*mtpB)->matIndex) ? 1 : 0;

    return orderA - orderB;
}

static RpMesh* SortPolygonsInTriListMesh(RpMesh* mesh, RpMeshHeader* meshHeader, void* pData)
{
    RwUInt32 arraySize;
    RwUInt32* vertexTagBuffer;
    RwUInt32 numTriangles;
    RxVertexIndex* indices;
    RxVertexIndex* oldIndices;
    RwInt8* cs;
    RwInt8 c;
    RwUInt32 count;
    RxVertexIndex* testInds;
    RwUInt32 maxVertex;
    RwUInt32 i;
    RwUInt32 outIndex;
    RwUInt32 inIndex;

    maxVertex = 0;
    numTriangles = mesh->numIndices / 3;
    indices = mesh->indices;

    for (i = 0; i < mesh->numIndices; i++)
    {
        maxVertex |= indices[i];
    }

    arraySize = (maxVertex + 31) >> 3;

    vertexTagBuffer = (RwUInt32*)RwMalloc(arraySize);
    if (vertexTagBuffer)
    {
        RwUInt32 indicesSize = mesh->numIndices * sizeof(RxVertexIndex);

        oldIndices = (RxVertexIndex*)RwMalloc(indicesSize);
        if (oldIndices)
        {
            memcpy(oldIndices, indices, indicesSize);

            outIndex = 0;
            while (outIndex < numTriangles)
            {
                cs = (RwInt8*)vertexTagBuffer;
                c = 0;
                count = arraySize;

                while (count--)
                {
                    *cs++ = c;
                }

                testInds = oldIndices;

                for (inIndex = 0; inIndex < numTriangles; inIndex++)
                {
                    if ((testInds[0] & testInds[1] & testInds[2]) != 0xFFFF)
                    {
                        if (!(vertexTagBuffer[testInds[0] >> 5] & (1 << (testInds[0] & 31))) &&
                            !(vertexTagBuffer[testInds[1] >> 5] & (1 << (testInds[1] & 31))) &&
                            !(vertexTagBuffer[testInds[2] >> 5] & (1 << (testInds[2] & 31))))
                        {
                            vertexTagBuffer[testInds[0] >> 5] |= 1 << (testInds[0] & 31);
                            outIndex++;
                            vertexTagBuffer[testInds[1] >> 5] |= 1 << (testInds[1] & 31);
                            vertexTagBuffer[testInds[2] >> 5] |= 1 << (testInds[2] & 31);

                            indices[0] = testInds[0];
                            indices[1] = testInds[1];
                            indices[2] = testInds[2];
                            indices += 3;

                            testInds[2] = 0xFFFF;
                            testInds[1] = 0xFFFF;
                            testInds[0] = 0xFFFF;
                        }
                    }

                    testInds += 3;
                }
            }

            RwFree(oldIndices);
        }

        RwFree(vertexTagBuffer);

        return mesh;
    }

    return mesh;
}

static Edge* TriStripAddEdge(RwFreeList* edgeFreeList, Edge** edgelist, RwUInt16 v1, RwUInt16 v2,
                             RwUInt32 tri, TriBinEntry** binEntryArray)
{
    Edge* temp;
    Edge* newEdge;

    /* Is this edge shared with a triangle we have already seen? */
    temp = *edgelist;
    while (temp)
    {
        if (v1 == temp->v2 && v2 == temp->v1 && !temp->tri2)
        {
            temp->tri1->adjCount++;
            binEntryArray[tri]->adjCount++;
            temp->tri2 = binEntryArray[tri];

            return temp;
        }

        temp = temp->next;
    }

    newEdge = (Edge*)RwFreeListAlloc(edgeFreeList);
    newEdge->v1 = v1;
    newEdge->v2 = v2;
    newEdge->tri1 = binEntryArray[tri];
    newEdge->tri2 = (TriBinEntry*)NULL;
    newEdge->next = *edgelist;
    *edgelist = newEdge;

    return newEdge;
}

static TriBinEntry** TriStripBinEntryArrayCreate(RwUInt32 numTris, MeshOpFreeLists* meshOpFreeLists,
                                                 Edge** edgelist, RpBuildMeshTriangle* triList)
{
    TriBinEntry** binEntryArray;
    RwUInt32 i;

    binEntryArray = (TriBinEntry**)RwMalloc(numTris * sizeof(TriBinEntry*));

    meshOpFreeLists->binEntryFreeList = RwFreeListCreate(sizeof(TriBinEntry), numTris, 4);
    meshOpFreeLists->edgeFreeList = RwFreeListCreate(sizeof(Edge), (numTris / 2) + 1, 4);

    for (i = 0; i < numTris; i++)
    {
        binEntryArray[i] = (TriBinEntry*)RwFreeListAlloc(meshOpFreeLists->binEntryFreeList);

        binEntryArray[i]->adjCount = 0;
        binEntryArray[i]->tri = i;
        binEntryArray[i]->prev = (TriBinEntry*)NULL;
        binEntryArray[i]->next = (TriBinEntry*)NULL;
        binEntryArray[i]->used = FALSE;

        binEntryArray[i]->edge[0] =
            TriStripAddEdge(meshOpFreeLists->edgeFreeList, edgelist, triList[i].vertIndex[0],
                            triList[i].vertIndex[1], i, binEntryArray);
        binEntryArray[i]->edge[1] =
            TriStripAddEdge(meshOpFreeLists->edgeFreeList, edgelist, triList[i].vertIndex[1],
                            triList[i].vertIndex[2], i, binEntryArray);
        binEntryArray[i]->edge[2] =
            TriStripAddEdge(meshOpFreeLists->edgeFreeList, edgelist, triList[i].vertIndex[2],
                            triList[i].vertIndex[0], i, binEntryArray);
    }

    return binEntryArray;
}

static TriBinEntry** TriStripBinEntryArrayDestroy(RwUInt32 numTris,
                                                  MeshOpFreeLists* meshOpFreeLists, Edge* edge,
                                                  TriBinEntry** binEntryArray)
{
    RwUInt32 i;

    while (edge)
    {
        Edge* next = edge->next;

        RwFreeListFree(meshOpFreeLists->edgeFreeList, edge);
        edge = next;
    }

    RwFreeListDestroy(meshOpFreeLists->edgeFreeList);
    meshOpFreeLists->edgeFreeList = (RwFreeList*)NULL;

    for (i = 0; i < numTris; i++)
    {
        RwFreeListFree(meshOpFreeLists->binEntryFreeList, binEntryArray[i]);
        binEntryArray[i] = (TriBinEntry*)NULL;
    }

    RwFreeListDestroy(meshOpFreeLists->binEntryFreeList);
    meshOpFreeLists->binEntryFreeList = (RwFreeList*)NULL;

    RwFree(binEntryArray);

    return (TriBinEntry**)NULL;
}

static Edge* TriStripGetTriEdge(TriBinEntry* binEntry, RxVertexIndex v1, RxVertexIndex v2)
{
    if ((binEntry->edge[0]->v1 == v1 && binEntry->edge[0]->v2 == v2) ||
        (binEntry->edge[0]->v1 == v2 && binEntry->edge[0]->v2 == v1))
    {
        return binEntry->edge[0];
    }

    if ((binEntry->edge[1]->v1 == v1 && binEntry->edge[1]->v2 == v2) ||
        (binEntry->edge[1]->v1 == v2 && binEntry->edge[1]->v2 == v1))
    {
        return binEntry->edge[1];
    }

    if ((binEntry->edge[2]->v1 == v1 && binEntry->edge[2]->v2 == v2) ||
        (binEntry->edge[2]->v1 == v2 && binEntry->edge[2]->v2 == v1))
    {
        return binEntry->edge[2];
    }

    return (Edge*)NULL;
}

static void TriStripMarkTriUsed(TriBinEntry* tri, TriBinList* binListArray, RwInt32 currentAttempt)
{
    RwUInt8 i;
    TriBinEntry* newTri;

    if (currentAttempt < 4)
    {
        tri->used2 = TRUE;
        return;
    }

    tri->used = TRUE;

    TriBinListRemove(binListArray, tri);

    /* The neighbours now have one fewer free neighbour each */
    for (i = 0; i < 3; i++)
    {
        Edge* edge = tri->edge[i];

        newTri = (TriBinEntry*)NULL;

        if (edge->tri1 && edge->tri1 != tri && !edge->tri1->used)
        {
            newTri = edge->tri1;
        }
        else if (edge->tri2 && !edge->tri2->used)
        {
            newTri = edge->tri2;
        }

        if (newTri)
        {
            TriBinListRemove(binListArray, newTri);

            newTri->adjCount--;

            newTri->next = binListArray[newTri->adjCount].head;
            if (newTri->next)
            {
                newTri->next->prev = newTri;
            }

            binListArray[newTri->adjCount].head = newTri;
            newTri->prev = (TriBinEntry*)NULL;
        }
    }
}

static RwBool TriStripEdgeIsLast(Edge* edge)
{
    return !edge || !TriStripEdgeFreeTris(edge);
}

static RwBool TriStripEdgeIsLast2(Edge* edge)
{
    return !edge || !TriStripEdgeFreeTris2(edge);
}

static RwBool TriStripEdgeIsAvailable(Edge* edge)
{
    return edge && TriStripEdgeFreeTris(edge);
}

static RwBool TriStripEdgeIsAvailable2(Edge* edge)
{
    return edge && TriStripEdgeFreeTris2(edge);
}

static RwUInt32 TriStripFollow(TriStripListEntry* strip, Edge* nextEdge, TriBinList* binListArray,
                               RpBuildMeshTriangle* triList, RwInt32 currentAttempt)
{
    RxVertexIndex* stripEnd;
    RwUInt32 addedTris;
    Edge* prevEdge;
    Edge* otherEdge;
    RwBool nextIsLast;
    RwBool otherIsAvailable;
    TriBinEntry* bestTri;
    RxVertexIndex v3;
    RwInt32 nextEdgeIndex;

    addedTris = 0;
    otherEdge = (Edge*)NULL;

    while (nextEdge)
    {
        bestTri = (TriBinEntry*)NULL;
        nextEdgeIndex = -1;

        /* Find an unused triangle on the far side of the edge */
        if (currentAttempt < 4)
        {
            if (nextEdge->tri1 && !nextEdge->tri1->used2)
            {
                bestTri = nextEdge->tri1;
            }
            else if (nextEdge->tri2 && !nextEdge->tri2->used2)
            {
                bestTri = nextEdge->tri2;
            }
        }
        else
        {
            if (nextEdge->tri1 && !nextEdge->tri1->used)
            {
                bestTri = nextEdge->tri1;
            }
            else if (nextEdge->tri2 && !nextEdge->tri2->used)
            {
                bestTri = nextEdge->tri2;
            }
        }

        if (!bestTri)
        {
            return addedTris;
        }

        addedTris++;
        TriStripMarkTriUsed(bestTri, binListArray, currentAttempt);

        if (nextEdge == bestTri->edge[0])
        {
            nextEdgeIndex = 0;
        }
        else if (nextEdge == bestTri->edge[1])
        {
            nextEdgeIndex = 1;
        }
        else if (nextEdge == bestTri->edge[2])
        {
            nextEdgeIndex = 2;
        }

        /* The vertex of the new triangle that is not on the shared edge */
        if (bestTri->edge[(nextEdgeIndex + 1) % 3]->tri1 == bestTri)
        {
            v3 = bestTri->edge[(nextEdgeIndex + 1) % 3]->v2;
        }
        else
        {
            v3 = bestTri->edge[(nextEdgeIndex + 1) % 3]->v1;
        }

        prevEdge = nextEdge;

        /* The edge we leave the triangle by joins the last strip vertex to the new one */
        stripEnd = &strip->strip[strip->stripLen];
        nextEdge = TriStripGetTriEdge(bestTri, stripEnd[-1], v3);

        if (currentAttempt < 4)
        {
            nextIsLast = TriStripEdgeIsLast2(nextEdge);
        }
        else
        {
            nextIsLast = TriStripEdgeIsLast(nextEdge);
        }

        if (nextIsLast)
        {
            /* Look at the remaining edge to see whether a swap would extend the strip */
            if (bestTri->edge[0] != prevEdge && bestTri->edge[0] != nextEdge)
            {
                otherEdge = bestTri->edge[0];
            }
            else if (bestTri->edge[1] != prevEdge && bestTri->edge[1] != nextEdge)
            {
                otherEdge = bestTri->edge[1];
            }
            else if (bestTri->edge[2] != prevEdge && bestTri->edge[2] != nextEdge)
            {
                otherEdge = bestTri->edge[2];
            }

            otherIsAvailable = (currentAttempt < 4) ? TriStripEdgeIsAvailable2(otherEdge) :
                                                      TriStripEdgeIsAvailable(otherEdge);

            if (otherIsAvailable)
            {
                if (strip->stripLen & 1)
                {
                    *stripEnd = v3;
                    nextEdge = (Edge*)NULL;
                    strip->stripLen++;
                }
                else
                {
                    /* Insert a swap so the strip can turn onto the other edge */
                    nextEdge = otherEdge;
                    *stripEnd = stripEnd[-2];
                    strip->stripLen++;
                    strip->strip[strip->stripLen] = v3;
                    strip->stripLen++;
                }
            }
            else
            {
                *stripEnd = v3;
                nextEdge = (Edge*)NULL;
                strip->stripLen++;
            }
        }
        else
        {
            *stripEnd = v3;
            strip->stripLen++;
        }
    }

    return addedTris;
}

static RwBool TriStripStripTris(RpBuildMeshTriangle* triList, RwUInt32 numTris,
                                TriStripList* stripList, RwBool preprocess)
{
    Edge* edgelist = (Edge*)NULL;
    TriStripListEntry* newStrip;
    TriStripListEntry* buildStrip;
    TriStripListEntry* revBuildStrip;
    RwUInt32 i;
    RwUInt32 trisUsed = 0;
    TriBinList binListArray[4];
    TriBinEntry** binEntryArray;
    Edge* nextEdge = (Edge*)NULL;
    Edge* firstEdge = (Edge*)NULL;
    MeshOpFreeLists meshOpFreeLists;

    meshOpFreeLists.binEntryFreeList = (RwFreeList*)NULL;
    meshOpFreeLists.edgeFreeList = (RwFreeList*)NULL;

    binListArray[0].head = (TriBinEntry*)NULL;
    binListArray[1].head = (TriBinEntry*)NULL;
    binListArray[2].head = (TriBinEntry*)NULL;
    binListArray[3].head = (TriBinEntry*)NULL;

    binEntryArray = TriStripBinEntryArrayCreate(numTris, &meshOpFreeLists, &edgelist, triList);

    /* Sort the triangles into bins by the number of neighbours they have */
    for (i = 0; i < numTris; i++)
    {
        binEntryArray[i]->next = binListArray[binEntryArray[i]->adjCount].head;
        if (binEntryArray[i]->next)
        {
            binEntryArray[i]->next->prev = binEntryArray[i];
        }

        binListArray[binEntryArray[i]->adjCount].head = binEntryArray[i];
        binEntryArray[i]->prev = (TriBinEntry*)NULL;
    }

    buildStrip = (TriStripListEntry*)RwFreeListAlloc(RWMESHGLOBAL(triStripListEntryFreeList));
    buildStrip->stripSize = (numTris * 2) + 2;
    buildStrip->stripLen = 0;
    buildStrip->strip = (RxVertexIndex*)RwMalloc(buildStrip->stripSize * sizeof(RwUInt32));

    revBuildStrip = (TriStripListEntry*)RwFreeListAlloc(RWMESHGLOBAL(triStripListEntryFreeList));
    revBuildStrip->stripSize = (numTris * 2) + 2;
    revBuildStrip->stripLen = 0;
    revBuildStrip->strip = (RxVertexIndex*)RwMalloc(revBuildStrip->stripSize * sizeof(RwUInt32));

    while (trisUsed < numTris)
    {
        RwUInt32 bestSize = 0;
        RwInt32 currentAttempt = preprocess ? 0 : 3;
        RwUInt32 startEdge;
        RwUInt32 bestOffset;
        RwUInt32 trisUsedTemp;
        RwUInt32 bin;

        if (binListArray[0].head)
        {
            /* An isolated triangle becomes a strip on its own */
            RwUInt32 tri = binListArray[0].head->tri;

            newStrip = (TriStripListEntry*)RwFreeListAlloc(RWMESHGLOBAL(triStripListEntryFreeList));
            newStrip->next = stripList->head;
            stripList->head = newStrip;

            newStrip->stripSize = 3;
            newStrip->stripLen = 3;
            newStrip->strip = (RxVertexIndex*)RwMalloc(3 * sizeof(RwUInt32));

            newStrip->strip[0] = triList[tri].vertIndex[0];
            newStrip->strip[1] = triList[tri].vertIndex[1];
            newStrip->strip[2] = triList[tri].vertIndex[2];

            binListArray[0].head->used = TRUE;
            binListArray[0].head->used2 = TRUE;

            binListArray[0].head = binListArray[0].head->next;
            if (binListArray[0].head)
            {
                binListArray[0].head->prev = (TriBinEntry*)NULL;
            }

            trisUsed++;
            continue;
        }

        /* Start from a triangle with as few neighbours as possible */
        for (bin = 1; !binListArray[bin].head; bin++)
        {
        }

        /* Pick the edge to start the strip from */
        if (TriStripEdgeFreeTris(binListArray[bin].head->edge[2]) >= 2 &&
            TriStripEdgeFreeTris(binListArray[bin].head->edge[1]) >= 2)
        {
            startEdge = 1;
        }
        else if (TriStripEdgeFreeTris(binListArray[bin].head->edge[0]) >= 2 &&
                 TriStripEdgeFreeTris(binListArray[bin].head->edge[2]) >= 2)
        {
            startEdge = 2;
        }
        else if (TriStripEdgeFreeTris(binListArray[bin].head->edge[1]) >= 2 &&
                 TriStripEdgeFreeTris(binListArray[bin].head->edge[0]) >= 2)
        {
            startEdge = 0;
        }
        else if (TriStripEdgeFreeTris(binListArray[bin].head->edge[0]) >
                 TriStripEdgeFreeTris(binListArray[bin].head->edge[1]))
        {
            if (TriStripEdgeFreeTris(binListArray[bin].head->edge[0]) >
                TriStripEdgeFreeTris(binListArray[bin].head->edge[2]))
            {
                startEdge = 2;
            }
            else
            {
                startEdge = 1;
            }
        }
        else
        {
            if (TriStripEdgeFreeTris(binListArray[bin].head->edge[1]) >
                TriStripEdgeFreeTris(binListArray[bin].head->edge[2]))
            {
                startEdge = 0;
            }
            else
            {
                startEdge = 1;
            }
        }

        trisUsedTemp = trisUsed;
        bestOffset = startEdge;

        /* Try each rotation of the first triangle, then build the best one for real */
        do
        {
            RwUInt32 offset;
            RwUInt32 tri;
            RwBool firstEdgeIsAvailable;

            for (i = 0; i < numTris; i++)
            {
                binEntryArray[i]->used2 = binEntryArray[i]->used;
            }

            switch (currentAttempt++)
            {
            case 0:
                offset = startEdge % 3;
                break;
            case 1:
                offset = (startEdge + 1) % 3;
                break;
            case 2:
                offset = (startEdge + 2) % 3;
                break;
            default:
                offset = bestOffset;
                break;
            }

            switch (offset)
            {
            case 0:
                nextEdge = binListArray[bin].head->edge[1];
                firstEdge = binListArray[bin].head->edge[0];
                break;
            case 1:
                nextEdge = binListArray[bin].head->edge[2];
                firstEdge = binListArray[bin].head->edge[1];
                break;
            case 2:
                nextEdge = binListArray[bin].head->edge[0];
                firstEdge = binListArray[bin].head->edge[2];
                break;
            }

            tri = binListArray[bin].head->tri;

            buildStrip->strip[0] = triList[tri].vertIndex[offset % 3];
            buildStrip->strip[1] = triList[tri].vertIndex[(offset + 1) % 3];
            buildStrip->strip[2] = triList[tri].vertIndex[(offset + 2) % 3];
            buildStrip->stripLen = 3;

            TriStripMarkTriUsed(binListArray[bin].head, binListArray, currentAttempt);

            trisUsed = trisUsedTemp + 1;
            trisUsed += TriStripFollow(buildStrip, nextEdge, binListArray, triList, currentAttempt);

            firstEdgeIsAvailable = (currentAttempt < 4) ? (TriStripEdgeFreeTris2(firstEdge) > 0) :
                                                          (TriStripEdgeFreeTris(firstEdge) > 0);

            if (firstEdgeIsAvailable)
            {
                /* Extend the strip backwards from the first triangle as well */
                revBuildStrip->strip[0] = buildStrip->strip[1];
                revBuildStrip->strip[1] = buildStrip->strip[0];
                revBuildStrip->stripLen = 2;

                trisUsed +=
                    TriStripFollow(revBuildStrip, firstEdge, binListArray, triList, currentAttempt);

                if (revBuildStrip->stripLen & 1)
                {
                    revBuildStrip->strip[revBuildStrip->stripLen] =
                        revBuildStrip->strip[revBuildStrip->stripLen - 2];
                    revBuildStrip->stripLen++;
                }

                if (trisUsed > bestSize)
                {
                    bestSize = trisUsed;
                    bestOffset = offset;
                }

                if (currentAttempt >= 4)
                {
                    newStrip = (TriStripListEntry*)RwFreeListAlloc(
                        RWMESHGLOBAL(triStripListEntryFreeList));
                    newStrip->next = stripList->head;
                    stripList->head = newStrip;

                    newStrip->stripSize = (revBuildStrip->stripLen - 2) + buildStrip->stripLen;
                    newStrip->stripLen = 0;
                    newStrip->strip =
                        (RxVertexIndex*)RwMalloc(newStrip->stripSize * sizeof(RwUInt32));

                    while (revBuildStrip->stripLen > 2)
                    {
                        newStrip->strip[newStrip->stripLen] =
                            revBuildStrip->strip[revBuildStrip->stripLen - 1];
                        newStrip->stripLen++;
                        revBuildStrip->stripLen--;
                    }

                    memcpy(&newStrip->strip[newStrip->stripLen], buildStrip->strip,
                           buildStrip->stripLen * sizeof(RwUInt32));

                    newStrip->stripLen = newStrip->stripSize;
                }
            }
            else
            {
                if (trisUsed > bestSize)
                {
                    bestSize = trisUsed;
                    bestOffset = offset;
                }

                if (currentAttempt >= 4)
                {
                    newStrip = (TriStripListEntry*)RwFreeListAlloc(
                        RWMESHGLOBAL(triStripListEntryFreeList));
                    newStrip->next = stripList->head;
                    stripList->head = newStrip;

                    newStrip->stripSize = buildStrip->stripLen;
                    newStrip->stripLen = buildStrip->stripLen;
                    newStrip->strip =
                        (RxVertexIndex*)RwMalloc(buildStrip->stripLen * sizeof(RwUInt32));

                    memcpy(newStrip->strip, buildStrip->strip,
                           buildStrip->stripLen * sizeof(RwUInt32));
                }
            }
        } while (currentAttempt < 4);
    }

    RwFree(revBuildStrip->strip);
    revBuildStrip->strip = (RxVertexIndex*)NULL;
    RwFreeListFree(RWMESHGLOBAL(triStripListEntryFreeList), revBuildStrip);

    RwFree(buildStrip->strip);
    buildStrip->strip = (RxVertexIndex*)NULL;
    RwFreeListFree(RWMESHGLOBAL(triStripListEntryFreeList), buildStrip);

    TriStripBinEntryArrayDestroy(numTris, &meshOpFreeLists, edgelist, binEntryArray);

    return TRUE;
}

static RwBool TriStripJoin(TriStripList* stripList, RwBool maintainWinding)
{
    RwUInt32 i;
    TriStripListEntry* newStrip;
    TriStripListEntry* stripPtr;
    TriStripListEntry* tempStrip;
    TriStripListEntry* tempStrip2;

    if (!stripList->head)
    {
        return FALSE;
    }

    newStrip = (TriStripListEntry*)RwFreeListAlloc(RWMESHGLOBAL(triStripListEntryFreeList));
    newStrip->stripLen = 0;
    newStrip->stripSize = 0;

    /* Leave room for the degenerate triangles that stitch the strips together */
    for (stripPtr = stripList->head; stripPtr; stripPtr = stripPtr->next)
    {
        newStrip->stripSize += stripPtr->stripLen + 6;
    }

    newStrip->strip = (RxVertexIndex*)RwMalloc(newStrip->stripSize * sizeof(RwUInt32));

    stripPtr = stripList->head;
    for (i = 0; i < stripPtr->stripLen; i++)
    {
        newStrip->strip[newStrip->stripLen] = stripPtr->strip[i];
        newStrip->stripLen++;
    }

    RwFree(stripPtr->strip);
    stripPtr->strip = (RxVertexIndex*)NULL;

    tempStrip = stripPtr->next;
    RwFreeListFree(RWMESHGLOBAL(triStripListEntryFreeList), stripPtr);

    while (tempStrip)
    {
        /* Prefer a strip that starts with our last vertex */
        for (stripPtr = tempStrip; stripPtr; stripPtr = stripPtr->next)
        {
            if (stripPtr->strip[0] == newStrip->strip[newStrip->stripLen - 1])
            {
                if ((newStrip->stripLen & 1) && maintainWinding)
                {
                    newStrip->strip[newStrip->stripLen] = stripPtr->strip[0];
                    newStrip->stripLen++;
                }

                break;
            }
        }

        /* Next best is one that starts with our second to last vertex */
        if (!stripPtr)
        {
            for (stripPtr = tempStrip; stripPtr; stripPtr = stripPtr->next)
            {
                if (stripPtr->strip[0] == newStrip->strip[newStrip->stripLen - 2])
                {
                    if (!(newStrip->stripLen & 1) && maintainWinding)
                    {
                        newStrip->strip[newStrip->stripLen] = stripPtr->strip[0];
                        newStrip->strip[newStrip->stripLen + 1] = stripPtr->strip[0];
                        newStrip->stripLen += 2;
                    }
                    else
                    {
                        newStrip->strip[newStrip->stripLen] = stripPtr->strip[0];
                        newStrip->stripLen++;
                    }

                    break;
                }
            }
        }

        /* Otherwise just take the next one */
        if (!stripPtr)
        {
            stripPtr = tempStrip;

            if (!(newStrip->stripLen & 1) || !maintainWinding)
            {
                newStrip->strip[newStrip->stripLen] = newStrip->strip[newStrip->stripLen - 1];
                newStrip->strip[newStrip->stripLen + 1] = stripPtr->strip[0];
                newStrip->stripLen += 2;
            }
            else
            {
                newStrip->strip[newStrip->stripLen] = newStrip->strip[newStrip->stripLen - 1];
                newStrip->strip[newStrip->stripLen + 1] = stripPtr->strip[0];
                newStrip->strip[newStrip->stripLen + 2] = stripPtr->strip[0];
                newStrip->stripLen += 3;
            }
        }

        for (i = 0; i < stripPtr->stripLen; i++)
        {
            newStrip->strip[newStrip->stripLen] = stripPtr->strip[i];
            newStrip->stripLen++;
        }

        RwFree(stripPtr->strip);
        stripPtr->strip = (RxVertexIndex*)NULL;

        if (tempStrip == stripPtr)
        {
            tempStrip = tempStrip->next;
            RwFreeListFree(RWMESHGLOBAL(triStripListEntryFreeList), stripPtr);
        }
        else
        {
            for (tempStrip2 = tempStrip; tempStrip2->next != stripPtr;
                 tempStrip2 = tempStrip2->next)
            {
            }

            tempStrip2->next = stripPtr->next;
            RwFreeListFree(RWMESHGLOBAL(triStripListEntryFreeList), stripPtr);
        }
    }

    stripList->head = newStrip;
    newStrip->next = (TriStripListEntry*)NULL;

    return TRUE;
}

static RpMeshHeader* TriStripMeshGenerate(RpBuildMesh* mesh, RwBool preprocess,
                                          RwBool maintainWinding)
{
    RpMeshHeader* result;
    RpBuildMeshTriangle** triPointers;
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 numMats;
    RwUInt32 numMatMeshes;
    RwUInt32 meshSize;
    RpBuildMeshTriangle** tempTriPtr;
    TriStripList stripList;
    RwUInt16 numOutMeshes;
    RxVertexIndex* stripMeshInds;
    RpMesh** outMeshes;
    RpMesh* matMeshes;
    RpMesh* meshEl;
    RpBuildMeshTriangle* triList;
    RwUInt32 totalIndices;
    TriStripListEntry* stripPtr;

    numOutMeshes = 0;

    triPointers =
        (RpBuildMeshTriangle**)RwMalloc(mesh->numTriangles * sizeof(RpBuildMeshTriangle*));
    if (!triPointers)
    {
        return (RpMeshHeader*)NULL;
    }

    for (i = 0; i < mesh->numTriangles; i++)
    {
        triPointers[i] = &mesh->meshTriangles[i];
    }

    qsort(triPointers, mesh->numTriangles, sizeof(RpBuildMeshTriangle*), SortPolygons);

    numMats = 1;
    if (mesh->numTriangles >= 2)
    {
        RpMaterial* lastMat = triPointers[0]->material;

        for (i = 1; i < mesh->numTriangles; i++)
        {
            if (triPointers[i]->material != lastMat)
            {
                lastMat = triPointers[i]->material;
                numMats++;
            }
        }
    }

    outMeshes = (RpMesh**)RwMalloc(numMats * sizeof(RpMesh*));

    /* Split the sorted triangles into runs of the same material: numIndices holds the
     * first triangle of each run until the run lengths are known */
    matMeshes = (RpMesh*)RwMalloc(numMats * sizeof(RpMesh));

    numMatMeshes = 1;
    matMeshes[0].material = triPointers[0]->material;
    matMeshes[0].numIndices = 0;
    matMeshes[0].indices = (RxVertexIndex*)NULL;

    if (mesh->numTriangles >= 2)
    {
        for (i = 0; i < mesh->numTriangles - 1; i++)
        {
            if (triPointers[i]->material != triPointers[i + 1]->material)
            {
                matMeshes[numMatMeshes].material = triPointers[i + 1]->material;
                matMeshes[numMatMeshes].numIndices = i + 1;
                matMeshes[numMatMeshes].indices = (RxVertexIndex*)NULL;
                matMeshes[numMatMeshes - 1].numIndices =
                    (i + 1) - matMeshes[numMatMeshes - 1].numIndices;
                numMatMeshes++;
            }
        }
    }

    matMeshes[numMatMeshes - 1].numIndices =
        mesh->numTriangles - matMeshes[numMatMeshes - 1].numIndices;

    RWMESHGLOBAL(triStripListEntryFreeList) =
        RwFreeListCreate(sizeof(TriStripListEntry), (mesh->numTriangles / 10) + 5, 4);

    tempTriPtr = triPointers;
    stripList.head = (TriStripListEntry*)NULL;

    for (i = 0; i < numMatMeshes; i++)
    {
        triList =
            (RpBuildMeshTriangle*)RwMalloc(matMeshes[i].numIndices * sizeof(RpBuildMeshTriangle));

        for (j = 0; j < matMeshes[i].numIndices; j++)
        {
            triList[j] = **tempTriPtr;
            tempTriPtr++;
        }

        TriStripStripTris(triList, matMeshes[i].numIndices, &stripList, preprocess);
        TriStripJoin(&stripList, maintainWinding);

        for (stripPtr = stripList.head; stripPtr; stripPtr = stripPtr->next)
        {
            RpMesh* outMeshInfo =
                (RpMesh*)RwMalloc(sizeof(RpMesh) + stripPtr->stripLen * sizeof(RxVertexIndex));

            outMeshInfo->material = matMeshes[i].material;
            outMeshInfo->numIndices = stripPtr->stripLen;
            outMeshInfo->indices = (RxVertexIndex*)(outMeshInfo + 1);

            for (j = 0; j < outMeshInfo->numIndices; j++)
            {
                outMeshInfo->indices[j] = stripPtr->strip[j];
            }

            outMeshes[numOutMeshes++] = outMeshInfo;
        }

        while (stripList.head)
        {
            stripPtr = stripList.head;
            stripList.head = stripPtr->next;

            RwFree(stripPtr->strip);
            stripPtr->strip = (RxVertexIndex*)NULL;

            RwFreeListFree(RWMESHGLOBAL(triStripListEntryFreeList), stripPtr);
        }

        RwFree(triList);
    }

    RwFreeListDestroy(RWMESHGLOBAL(triStripListEntryFreeList));
    RWMESHGLOBAL(triStripListEntryFreeList) = (RwFreeList*)NULL;

    meshSize = sizeof(RpMeshHeader);
    totalIndices = 0;
    for (i = 0; i < numOutMeshes; i++)
    {
        meshSize += sizeof(RpMesh) + outMeshes[i]->numIndices * sizeof(RxVertexIndex);
        totalIndices += outMeshes[i]->numIndices;
    }

    result = _rpMeshHeaderCreate(meshSize);

    result->flags = rpMESHHEADERTRISTRIP;
    result->numMeshes = numOutMeshes;

    meshEl = (RpMesh*)(result + 1);

    result->serialNum = RWMESHGLOBAL(nextSerialNum);
    result->firstMeshOffset = 0;
    result->totalIndicesInMesh = totalIndices;

    stripMeshInds = (RxVertexIndex*)(meshEl + numOutMeshes);

    RWMESHGLOBAL(nextSerialNum)++;

    for (i = 0; i < numOutMeshes; i++)
    {
        meshEl->indices = stripMeshInds;
        meshEl->numIndices = outMeshes[i]->numIndices;
        meshEl->material = outMeshes[i]->material;

        memcpy(stripMeshInds, outMeshes[i]->indices,
               outMeshes[i]->numIndices * sizeof(RxVertexIndex));
        stripMeshInds += meshEl->numIndices;
        meshEl++;

        RwFree(outMeshes[i]);
        outMeshes[i] = (RpMesh*)NULL;
    }

    RwFree(triPointers);
    RwFree(outMeshes);
    RwFree(matMeshes);

    return result;
}

RpMeshHeader* RpBuildMeshGenerateDefaultTriStrip(RpBuildMesh* buildMesh, void* data)
{
    return TriStripMeshGenerate(buildMesh, FALSE, TRUE);
}

RpMeshHeader* _rpTriListMeshGenerate(RpBuildMesh* buildMesh, void* data)
{
    RpBuildMeshTriangle** triPointers;
    RpMeshHeader* result;
    RwUInt32 i;
    RwUInt32 numMats;
    RwUInt32 meshSize;
    RpMesh* meshEl;
    RxVertexIndex* meshTriInds;

    triPointers =
        (RpBuildMeshTriangle**)RwMalloc(buildMesh->numTriangles * sizeof(RpBuildMeshTriangle*));
    if (triPointers)
    {
        for (i = 0; i < buildMesh->numTriangles; i++)
        {
            triPointers[i] = &buildMesh->meshTriangles[i];
        }

        qsort(triPointers, buildMesh->numTriangles, sizeof(RpBuildMeshTriangle*), SortPolygons);

        numMats = 1;
        if (buildMesh->numTriangles >= 2)
        {
            RpMaterial* lastMat = triPointers[0]->material;

            for (i = 1; i < buildMesh->numTriangles; i++)
            {
                if (triPointers[i]->material != lastMat)
                {
                    lastMat = triPointers[i]->material;
                    numMats++;
                }
            }
        }

        meshSize = sizeof(RpMeshHeader) + numMats * sizeof(RpMesh) +
                   buildMesh->numTriangles * (3 * sizeof(RxVertexIndex));

        result = _rpMeshHeaderCreate(meshSize);
        if (!result)
        {
            RwFree(triPointers);
            RWERROR((E_RW_NOMEM, meshSize));
            return (RpMeshHeader*)NULL;
        }

        result->flags = 0;
        result->numMeshes = 1;

        meshEl = (RpMesh*)(result + 1);
        meshTriInds = (RxVertexIndex*)(meshEl + numMats);

        result->serialNum = RWMESHGLOBAL(nextSerialNum);
        result->firstMeshOffset = 0;
        result->totalIndicesInMesh = buildMesh->numTriangles * 3;

        RWMESHGLOBAL(nextSerialNum)++;

        meshEl->numIndices = 0;
        meshEl->material = triPointers[0]->material;
        meshEl->indices = meshTriInds;

        i = 0;
        do
        {
            if (triPointers[i]->material != meshEl->material)
            {
                meshEl++;
                meshEl->numIndices = 0;
                meshEl->material = triPointers[i]->material;
                meshEl->indices = meshTriInds;

                result->numMeshes++;
            }

            *meshTriInds++ = triPointers[i]->vertIndex[0];
            *meshTriInds++ = triPointers[i]->vertIndex[1];
            *meshTriInds++ = triPointers[i]->vertIndex[2];

            meshEl->numIndices += 3;

            i++;
        } while (i < buildMesh->numTriangles);

        _rpMeshHeaderForAllMeshes(result, SortPolygonsInTriListMesh, NULL);

        RwFree(triPointers);

        return result;
    }

    RWERROR((E_RW_NOMEM, buildMesh->numTriangles * sizeof(RpBuildMeshTriangle*)));
    return (RpMeshHeader*)NULL;
}

RpMeshHeader* _rpMeshOptimise(RpBuildMesh* mesh, RwUInt32 flags)
{
    if (mesh)
    {
        RpMeshTriStripMethod func;
        void* data;
        RpMeshHeader* newMesh;

        if (!mesh->numTriangles)
        {
            _rpBuildMeshDestroy(mesh);
            return &MeshopStatic.nullMeshHeader;
        }

        if (flags & rpMESHHEADERTRISTRIP)
        {
            func = MeshopStatic.meshTristripMethod;
            data = MeshopStatic.data;
        }
        else
        {
            func = _rpTriListMeshGenerate;
            data = NULL;
        }

        newMesh = func(mesh, data);
        if (newMesh)
        {
            _rpBuildMeshDestroy(mesh);
            return newMesh;
        }
    }

    return (RpMeshHeader*)NULL;
}
