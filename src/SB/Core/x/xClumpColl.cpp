#include "xClumpColl.h"

#include <types.h>
#if defined(PS2)
#include <rwplcore.h>
#include <rwsdk/rtintsec.h>
#include <math.h>
#include "xMemMgr.h"
#endif

U8 xClumpColl_FilterFlags = 0x04;

union IntersectionCallBack
{
    RpIntersectionCallBackWorldSector sectorCB;
    RpIntersectionCallBackWorldTriangle worldCB;
    RpIntersectionCallBackAtomic atomicCB;
};

struct CallBackParam
{
    RpIntersection* intersection;
    IntersectionCallBack u;
    void* data;
};

struct PolyLineTestParam
{
    RwV3d start;
    RpWorldSector* worldSector;
    RwV3d delta;
    xClumpCollV3dGradient grad;
    RwLine line;
    CallBackParam* cbParam;
};

struct PolyTestParam
{
    RwBBox bbox;
    RpWorldSector* worldSector;
    void* leafTestData;
    CallBackParam* cbParam;
};

struct TestSphere
{
    RwSphere* sphere;
    F32 recipRadius;
};

xClumpCollBSPTree* xClumpColl_StaticBufferInit(void* data, U32)
{
    U32* header = (U32*)data;
    U32 numBranchNodes = header[1];
    U32 numTriangles = header[2];
    xClumpCollBSPTree* tree = (xClumpCollBSPTree*)RwMalloc(sizeof(xClumpCollBSPTree));

    if (numBranchNodes)
    {
        tree->branchNodes = (xClumpCollBSPBranchNode*)(header + 3);
        tree->triangles = (xClumpCollBSPTriangle*)(tree->branchNodes + numBranchNodes);
    }
    else
    {
        tree->branchNodes = NULL;
        tree->triangles = (xClumpCollBSPTriangle*)(header + 3);
    }

    tree->numBranchNodes = numBranchNodes;
    tree->numTriangles = numTriangles;

    return tree;
}

#if defined(PS2)
// PS2 RenderWare resource, mesh and PS2All instance declarations
// (rwcore.h, rpworld.h, bapipew.h, ps2all.h).
struct RpMesh
{
    RxVertexIndex* indices;
    RwUInt32 numIndices;
    RpMaterial* material;
};

struct RpMeshHeader
{
    RwUInt32 flags;
    RwUInt16 numMeshes;
    RwUInt16 serialNum;
    RwUInt32 totalIndicesInMesh;
    RwUInt32 firstMeshOffset;
};

struct RwResEntry
{
    RwLLLink link;
    RwInt32 size;
    void* owner;
    RwResEntry** ownerRef;
    void (*destroyNotify)(RwResEntry* resEntry);
};

struct rwPS2AllResEntryHeader
{
    RwInt32 refCnt;
    RwInt32 clrCnt;
    __int128* data;
};

extern RwInt32 rwPip2GeometryOffset;
extern RwInt32 rwPip2AtomicOffset;

extern "C" RwMeshCache** _rpMeshCacheCreate(RwMeshCache** cacheHandle, RwUInt32 numMeshes);

#define RWPIP2GEOMETRYMESHCACHE(_geometry)                                                           ((RwMeshCache**)(((RwUInt8*)(_geometry)) + rwPip2GeometryOffset))
#define RWPIP2ATOMICMESHCACHE(_atomic)                                                               ((RwMeshCache**)(((RwUInt8*)(_atomic)) + rwPip2AtomicOffset))

#define rpGeometryGetMeshCache(_geometry, _numMeshes)                                                (*(((*RWPIP2GEOMETRYMESHCACHE(_geometry)) &&                                                         ((*RWPIP2GEOMETRYMESHCACHE(_geometry))->lengthOfMeshesArray == (_numMeshes))) ?                     RWPIP2GEOMETRYMESHCACHE(_geometry) :                                                             _rpMeshCacheCreate(RWPIP2GEOMETRYMESHCACHE(_geometry), (_numMeshes))))
#define rpAtomicGetMeshCache(_atomic, _numMeshes)                                                    (*(((*RWPIP2ATOMICMESHCACHE(_atomic)) &&                                                             ((*RWPIP2ATOMICMESHCACHE(_atomic))->lengthOfMeshesArray == (_numMeshes))) ?                         RWPIP2ATOMICMESHCACHE(_atomic) :                                                                 _rpMeshCacheCreate(RWPIP2ATOMICMESHCACHE(_atomic), (_numMeshes))))

static RpAtomic* AddAtomicCB(RpAtomic* atomic, void* data)
{
    TempAtomicList** tmpList = (TempAtomicList**)data;

    (*tmpList)->atomic = atomic;
    (*tmpList)->geom = atomic->geometry;
    (*tmpList)->meshHeader = (*tmpList)->geom->mesh;

    if ((*tmpList)->geom->numMorphTargets != 1)
    {
        (*tmpList)->meshCache = rpAtomicGetMeshCache(atomic, (*tmpList)->meshHeader->numMeshes);
    }
    else
    {
        (*tmpList)->meshCache =
            rpGeometryGetMeshCache((*tmpList)->geom, (*tmpList)->meshHeader->numMeshes);
    }

    (*tmpList)--;

    return atomic;
}
#endif

void xClumpColl_InstancePointers(xClumpCollBSPTree* tree, RpClump* clump)
{
#if defined(PS2)
    S32 i;
    S32 numAtom;
    TempAtomicList* atomicList;
    TempAtomicList* iterList;
    TempAtomicList* alist;
    S32 vertIndex;
    S32 numMeshes;
    S32 meshIndex;
    RpMesh* mesh;

    numAtom = RpClumpGetNumAtomics(clump);
    atomicList = (TempAtomicList*)xMemPushTemp(numAtom * sizeof(TempAtomicList));
    iterList = atomicList + (numAtom - 1);
    RpClumpForAllAtomics(clump, AddAtomicCB, &iterList);

    for (i = 0; i < tree->numTriangles; i++)
    {
        alist = &atomicList[tree->triangles[i].v.i.atomIndex];
        vertIndex = tree->triangles[i].v.i.meshVertIndex;
        numMeshes = alist->meshHeader->numMeshes;
        mesh = (RpMesh*)((RwUInt8*)(alist->meshHeader + 1) + alist->meshHeader->firstMeshOffset);
        meshIndex = 0;

        while (numMeshes--)
        {
            if (vertIndex < mesh->numIndices)
            {
                break;
            }

            vertIndex -= mesh->numIndices;
            meshIndex++;
            mesh++;
        }

        tree->triangles[i].v.p =
            (RwV3d*)((RwUInt8*)&((rwPS2AllResEntryHeader*)(alist->meshCache->meshes[meshIndex] + 1))
                         ->data[(vertIndex / 66) * 120 + 2] +
                     (vertIndex % 66) * sizeof(RwV3d));
    }

    xMemPopTemp(atomicList);
#endif
}

xClumpCollBSPTree*
xClumpColl_ForAllBoxLeafNodeIntersections(xClumpCollBSPTree* tree, RwBBox* box,
                                          xClumpCollIntersectionCallback callBack, void* data)
{
    S32 nStack;
    nodeInfo nodeStack[33];
    nodeInfo node;

    node.type = tree->branchNodes ? 2 : 1;
    node.index = 0;
    nStack = 0;

    while (nStack >= 0)
    {
        if (node.type == 1)
        {
            xClumpCollBSPTriangle* tris = tree->triangles + node.index;
            if (!callBack(tris, data))
                return NULL;

            node = nodeStack[nStack];
            nStack--;
        }
        else
        {
            xClumpCollBSPBranchNode* branch = tree->branchNodes + node.index;

            if (*((RwReal*)((U8*)&box->inf + (branch->leftInfo & 0xC))) < branch->leftValue)
            {
                node.type = branch->leftInfo & 0x3;
                node.index = branch->leftInfo >> 12;

                if (*(RwReal*)((U8*)&box->sup + (branch->leftInfo & 0xC)) >= branch->rightValue)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                }
            }
            else
            {
                node.type = branch->rightInfo & 0x3;
                node.index = branch->rightInfo >> 12;
            }
        }
    }

    return tree;
}

xClumpCollBSPTree*
xClumpColl_ForAllLineLeafNodeIntersections(xClumpCollBSPTree* tree, RwLine* line,
                                           xClumpCollV3dGradient* grad,
                                           xClumpCollIntersectionCallback callBack, void* data)
{
    S32 nStack;
    nodeInfo nodeStack[33];
    nodeInfo node;
    RwLine lineStack[33];
    RwLine currLine;

    node.type = tree->branchNodes ? 2 : 1;
    node.index = 0;
    currLine = *line;
    nStack = 0;

    while (nStack >= 0)
    {
        if (node.type == 1)
        {
            xClumpCollBSPTriangle* tris = tree->triangles + node.index;
            if (!callBack(tris, data))
                return NULL;

            node = nodeStack[nStack];
            currLine = lineStack[nStack];
            nStack--;
        }
        else
        {
            RwSplitBits lStart, lEnd, rStart, rEnd;
            xClumpCollBSPBranchNode* branch = tree->branchNodes + node.index;

            lStart.nReal =
                *(RwReal*)((U8*)&currLine.start + (branch->leftInfo & 0xC)) - branch->leftValue;
            lEnd.nReal =
                *(RwReal*)((U8*)&currLine.end + (branch->leftInfo & 0xC)) - branch->leftValue;
            rStart.nReal =
                *(RwReal*)((U8*)&currLine.start + (branch->leftInfo & 0xC)) - branch->rightValue;
            rEnd.nReal =
                *(RwReal*)((U8*)&currLine.end + (branch->leftInfo & 0xC)) - branch->rightValue;

            if (rStart.nInt < 0 && rEnd.nInt < 0)
            {
                node.type = branch->leftInfo & 0x3;
                node.index = branch->leftInfo >> 12;
            }
            else if (lStart.nInt >= 0 && lEnd.nInt >= 0)
            {
                node.type = branch->rightInfo & 0x3;
                node.index = branch->rightInfo >> 12;
            }
            else if (!((lStart.nInt ^ lEnd.nInt) & 0x80000000) &&
                     !((rStart.nInt ^ rEnd.nInt) & 0x80000000))
            {
                if (rStart.nInt < rEnd.nInt)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->leftInfo & 0x3;
                    node.index = branch->leftInfo >> 12;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = branch->leftInfo & 0x3;
                    nodeStack[nStack].index = branch->leftInfo >> 12;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->rightInfo & 0x3;
                    node.index = branch->rightInfo >> 12;
                }
            }
            else if (((lStart.nInt ^ lEnd.nInt) & 0x80000000) && rStart.nInt >= 0 && rEnd.nInt >= 0)
            {
                RwV3d vTmp;
                F32 delta;
                switch (branch->leftInfo & 0xC)
                {
                case 0:
                    delta = branch->leftValue - currLine.start.x;
                    vTmp.x = branch->leftValue;
                    vTmp.y = currLine.start.y + grad->dydx * delta;
                    vTmp.z = currLine.start.z + grad->dzdx * delta;
                    break;
                case 4:
                    delta = branch->leftValue - currLine.start.y;
                    vTmp.x = currLine.start.x + grad->dxdy * delta;
                    vTmp.y = branch->leftValue;
                    vTmp.z = currLine.start.z + grad->dzdy * delta;
                    break;
                case 8:
                    delta = branch->leftValue - currLine.start.z;
                    vTmp.x = currLine.start.x + grad->dxdz * delta;
                    vTmp.y = currLine.start.y + grad->dydz * delta;
                    vTmp.z = branch->leftValue;
                    break;
                }
                if (lStart.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->leftInfo & 0x3;
                    node.index = branch->leftInfo >> 12;
                    currLine.end = vTmp;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = branch->leftInfo & 0x3;
                    nodeStack[nStack].index = branch->leftInfo >> 12;
                    lineStack[nStack].start = vTmp;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->rightInfo & 0x3;
                    node.index = branch->rightInfo >> 12;
                }
            }
            else if (((rStart.nInt ^ rEnd.nInt) & 0x80000000) && lStart.nInt < 0 && lEnd.nInt < 0)
            {
                RwV3d vTmp;
                F32 delta;
                switch (branch->leftInfo & 0xC)
                {
                case 0:
                    delta = branch->rightValue - currLine.start.x;
                    vTmp.x = branch->rightValue;
                    vTmp.y = currLine.start.y + grad->dydx * delta;
                    vTmp.z = currLine.start.z + grad->dzdx * delta;
                    break;
                case 4:
                    delta = branch->rightValue - currLine.start.y;
                    vTmp.x = currLine.start.x + grad->dxdy * delta;
                    vTmp.y = branch->rightValue;
                    vTmp.z = currLine.start.z + grad->dzdy * delta;
                    break;
                case 8:
                    delta = branch->rightValue - currLine.start.z;
                    vTmp.x = currLine.start.x + grad->dxdz * delta;
                    vTmp.y = currLine.start.y + grad->dydz * delta;
                    vTmp.z = branch->rightValue;
                    break;
                }
                if (rStart.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                    lineStack[nStack].start = vTmp;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->leftInfo & 0x3;
                    node.index = branch->leftInfo >> 12;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = branch->leftInfo & 0x3;
                    nodeStack[nStack].index = branch->leftInfo >> 12;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->rightInfo & 0x3;
                    node.index = branch->rightInfo >> 12;
                    currLine.end = vTmp;
                }
            }
            else
            {
                RwV3d vLeft;
                RwV3d vRight;
                {
                    F32 delta;
                    switch (branch->leftInfo & 0xC)
                    {
                    case 0:
                        delta = branch->leftValue - currLine.start.x;
                        vLeft.x = branch->leftValue;
                        vLeft.y = currLine.start.y + grad->dydx * delta;
                        vLeft.z = currLine.start.z + grad->dzdx * delta;
                        break;
                    case 4:
                        delta = branch->leftValue - currLine.start.y;
                        vLeft.x = currLine.start.x + grad->dxdy * delta;
                        vLeft.y = branch->leftValue;
                        vLeft.z = currLine.start.z + grad->dzdy * delta;
                        break;
                    case 8:
                        delta = branch->leftValue - currLine.start.z;
                        vLeft.x = currLine.start.x + grad->dxdz * delta;
                        vLeft.y = currLine.start.y + grad->dydz * delta;
                        vLeft.z = branch->leftValue;
                        break;
                    }
                }
                {
                    F32 delta;
                    switch (branch->leftInfo & 0xC)
                    {
                    case 0:
                        delta = branch->rightValue - currLine.start.x;
                        vRight.x = branch->rightValue;
                        vRight.y = currLine.start.y + grad->dydx * delta;
                        vRight.z = currLine.start.z + grad->dzdx * delta;
                        break;
                    case 4:
                        delta = branch->rightValue - currLine.start.y;
                        vRight.x = currLine.start.x + grad->dxdy * delta;
                        vRight.y = branch->rightValue;
                        vRight.z = currLine.start.z + grad->dzdy * delta;
                        break;
                    case 8:
                        delta = branch->rightValue - currLine.start.z;
                        vRight.x = currLine.start.x + grad->dxdz * delta;
                        vRight.y = currLine.start.y + grad->dydz * delta;
                        vRight.z = branch->rightValue;
                        break;
                    }
                }
                if (lStart.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                    lineStack[nStack].start = vRight;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->leftInfo & 0x3;
                    node.index = branch->leftInfo >> 12;
                    currLine.end = vLeft;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = branch->leftInfo & 0x3;
                    nodeStack[nStack].index = branch->leftInfo >> 12;
                    lineStack[nStack].start = vLeft;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->rightInfo & 0x3;
                    node.index = branch->rightInfo >> 12;
                    currLine.end = vRight;
                }
            }
        }
    }

    return tree;
}

xClumpCollBSPTree*
xClumpColl_ForAllCapsuleLeafNodeIntersections(xClumpCollBSPTree* tree, RwLine* line, F32 radius,
                                              xClumpCollV3dGradient* grad,
                                              xClumpCollIntersectionCallback callBack, void* data)
{
    S32 nStack;
    nodeInfo nodeStack[33];
    nodeInfo node;
    RwLine lineStack[33];
    RwLine currLine;

    node.type = tree->branchNodes ? 2 : 1;
    node.index = 0;
    currLine = *line;
    nStack = 0;

    while (nStack >= 0)
    {
        if (node.type == 1)
        {
            xClumpCollBSPTriangle* tris = tree->triangles + node.index;
            if (!callBack(tris, data))
                return NULL;

            node = nodeStack[nStack];
            currLine = lineStack[nStack];
            nStack--;
        }
        else
        {
            RwSplitBits lStart, lEnd, rStart, rEnd;
            xClumpCollBSPBranchNode* branch = tree->branchNodes + node.index;

            lStart.nReal = *(RwReal*)((U8*)&currLine.start + (branch->leftInfo & 0xC)) -
                           (branch->leftValue + radius);
            lEnd.nReal = *(RwReal*)((U8*)&currLine.end + (branch->leftInfo & 0xC)) -
                         (branch->leftValue + radius);
            rStart.nReal = *(RwReal*)((U8*)&currLine.start + (branch->leftInfo & 0xC)) -
                           (branch->rightValue - radius);
            rEnd.nReal = *(RwReal*)((U8*)&currLine.end + (branch->leftInfo & 0xC)) -
                         (branch->rightValue - radius);

            if (rStart.nInt < 0 && rEnd.nInt < 0)
            {
                node.type = branch->leftInfo & 0x3;
                node.index = branch->leftInfo >> 12;
            }
            else if (lStart.nInt >= 0 && lEnd.nInt >= 0)
            {
                node.type = branch->rightInfo & 0x3;
                node.index = branch->rightInfo >> 12;
            }
            else if (!((lStart.nInt ^ lEnd.nInt) & 0x80000000) &&
                     !((rStart.nInt ^ rEnd.nInt) & 0x80000000))
            {
                if (rStart.nInt < rEnd.nInt)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->leftInfo & 0x3;
                    node.index = branch->leftInfo >> 12;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = branch->leftInfo & 0x3;
                    nodeStack[nStack].index = branch->leftInfo >> 12;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->rightInfo & 0x3;
                    node.index = branch->rightInfo >> 12;
                }
            }
            else if (((lStart.nInt ^ lEnd.nInt) & 0x80000000) && rStart.nInt >= 0 && rEnd.nInt >= 0)
            {
                RwV3d vTmp;
                F32 delta;
                switch (branch->leftInfo & 0xC)
                {
                case 0:
                    delta = branch->leftValue - currLine.start.x;
                    vTmp.x = branch->leftValue;
                    vTmp.y = currLine.start.y + grad->dydx * delta;
                    vTmp.z = currLine.start.z + grad->dzdx * delta;
                    break;
                case 4:
                    delta = branch->leftValue - currLine.start.y;
                    vTmp.x = currLine.start.x + grad->dxdy * delta;
                    vTmp.y = branch->leftValue;
                    vTmp.z = currLine.start.z + grad->dzdy * delta;
                    break;
                case 8:
                    delta = branch->leftValue - currLine.start.z;
                    vTmp.x = currLine.start.x + grad->dxdz * delta;
                    vTmp.y = currLine.start.y + grad->dydz * delta;
                    vTmp.z = branch->leftValue;
                    break;
                }
                if (lStart.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->leftInfo & 0x3;
                    node.index = branch->leftInfo >> 12;
                    currLine.end = vTmp;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = branch->leftInfo & 0x3;
                    nodeStack[nStack].index = branch->leftInfo >> 12;
                    lineStack[nStack].start = vTmp;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->rightInfo & 0x3;
                    node.index = branch->rightInfo >> 12;
                }
            }
            else if (((rStart.nInt ^ rEnd.nInt) & 0x80000000) && lStart.nInt < 0 && lEnd.nInt < 0)
            {
                RwV3d vTmp;
                F32 delta;
                switch (branch->leftInfo & 0xC)
                {
                case 0:
                    delta = branch->rightValue - currLine.start.x;
                    vTmp.x = branch->rightValue;
                    vTmp.y = currLine.start.y + grad->dydx * delta;
                    vTmp.z = currLine.start.z + grad->dzdx * delta;
                    break;
                case 4:
                    delta = branch->rightValue - currLine.start.y;
                    vTmp.x = currLine.start.x + grad->dxdy * delta;
                    vTmp.y = branch->rightValue;
                    vTmp.z = currLine.start.z + grad->dzdy * delta;
                    break;
                case 8:
                    delta = branch->rightValue - currLine.start.z;
                    vTmp.x = currLine.start.x + grad->dxdz * delta;
                    vTmp.y = currLine.start.y + grad->dydz * delta;
                    vTmp.z = branch->rightValue;
                    break;
                }
                if (rStart.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                    lineStack[nStack].start = vTmp;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->leftInfo & 0x3;
                    node.index = branch->leftInfo >> 12;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = branch->leftInfo & 0x3;
                    nodeStack[nStack].index = branch->leftInfo >> 12;
                    lineStack[nStack].start = currLine.start;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->rightInfo & 0x3;
                    node.index = branch->rightInfo >> 12;
                    currLine.end = vTmp;
                }
            }
            else
            {
                RwV3d vLeft;
                RwV3d vRight;
                {
                    F32 delta;
                    switch (branch->leftInfo & 0xC)
                    {
                    case 0:
                        delta = branch->leftValue - currLine.start.x;
                        vLeft.x = branch->leftValue;
                        vLeft.y = currLine.start.y + grad->dydx * delta;
                        vLeft.z = currLine.start.z + grad->dzdx * delta;
                        break;
                    case 4:
                        delta = branch->leftValue - currLine.start.y;
                        vLeft.x = currLine.start.x + grad->dxdy * delta;
                        vLeft.y = branch->leftValue;
                        vLeft.z = currLine.start.z + grad->dzdy * delta;
                        break;
                    case 8:
                        delta = branch->leftValue - currLine.start.z;
                        vLeft.x = currLine.start.x + grad->dxdz * delta;
                        vLeft.y = currLine.start.y + grad->dydz * delta;
                        vLeft.z = branch->leftValue;
                        break;
                    }
                }
                {
                    F32 delta;
                    switch (branch->leftInfo & 0xC)
                    {
                    case 0:
                        delta = branch->rightValue - currLine.start.x;
                        vRight.x = branch->rightValue;
                        vRight.y = currLine.start.y + grad->dydx * delta;
                        vRight.z = currLine.start.z + grad->dzdx * delta;
                        break;
                    case 4:
                        delta = branch->rightValue - currLine.start.y;
                        vRight.x = currLine.start.x + grad->dxdy * delta;
                        vRight.y = branch->rightValue;
                        vRight.z = currLine.start.z + grad->dzdy * delta;
                        break;
                    case 8:
                        delta = branch->rightValue - currLine.start.z;
                        vRight.x = currLine.start.x + grad->dxdz * delta;
                        vRight.y = currLine.start.y + grad->dydz * delta;
                        vRight.z = branch->rightValue;
                        break;
                    }
                }
                if (lStart.nInt < 0)
                {
                    nStack++;
                    nodeStack[nStack].type = branch->rightInfo & 0x3;
                    nodeStack[nStack].index = branch->rightInfo >> 12;
                    lineStack[nStack].start = vRight;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->leftInfo & 0x3;
                    node.index = branch->leftInfo >> 12;
                    currLine.end = vLeft;
                }
                else
                {
                    nStack++;
                    nodeStack[nStack].type = branch->leftInfo & 0x3;
                    nodeStack[nStack].index = branch->leftInfo >> 12;
                    lineStack[nStack].start = vLeft;
                    lineStack[nStack].end = currLine.end;
                    node.type = branch->rightInfo & 0x3;
                    node.index = branch->rightInfo >> 12;
                    currLine.end = vRight;
                }
            }
        }
    }

    return tree;
}

static S32 LeafNodeLinePolyIntersect(xClumpCollBSPTriangle* triangles, void* data)
{
    PolyLineTestParam* isData = (PolyLineTestParam*)data;
    CallBackParam* cbParam = isData->cbParam;
    do
    {
        if (triangles->flags & xClumpColl_FilterFlags)
        {
            RwV3d *v0, *v1, *v2;
            F32 distance;
            S32 result;
            RwV3d edge1, edge2, tVec, pVec, qVec;
            F32 det;
            v0 = &triangles->v.p[0];
            if (triangles->flags & 0x2)
            {
                v1 = &triangles->v.p[2];
                v2 = &triangles->v.p[1];
            }
            else
            {
                v1 = &triangles->v.p[1];
                v2 = &triangles->v.p[2];
            }
#if defined(PS2)
            {
                // Pack the unaligned 12-byte vertices, then form both triangle edges
                // and the ray cross product in the VU.
                U32 x, y, z, packed;
                F32 det, u, v, t, epsilon, tolerance;
                const RwV3d* start = &isData->start;
                const RwV3d* delta = &isData->delta;
                asm volatile("lw %0, 0(%9)\n"
                             "vaddw.xyz vf11, vf0, vf0w\n"
                             "lw %1, 4(%9)\n"
                             "lw %2, 8(%9)\n"
                             "pextlw %3, %1, %0\n"
                             "lw %0, 0(%11)\n"
                             "pcpyld %3, %2, %3\n"
                             "lw %1, 4(%11)\n"
                             "qmtc2 %3, vf1\n"
                             "lw %2, 8(%11)\n"
                             "pextlw %3, %1, %0\n"
                             "lqc2 vf7, 0(%8)\n"
                             "pcpyld %3, %2, %3\n"
                             "lw %0, 0(%10)\n"
                             "qmtc2 %3, vf3\n"
                             "lw %1, 4(%10)\n"
                             "vsub.xyz vf5, vf3, vf1\n"
                             "lw %2, 8(%10)\n"
                             "pextlw %3, %1, %0\n"
                             "lqc2 vf6, 0(%7)\n"
                             "pcpyld %3, %2, %3\n"
                             "qmtc2 %3, vf2\n"
                             "vopmula.xyz ACC, vf7, vf5\n"
                             "lui %1, 0xb22b\n"
                             "vopmsub.xyz vf8, vf5, vf7\n"
                             "ori %1, %1, 0xcc77\n"
                             "vsub.xyz vf4, vf2, vf1\n"
                             "lui %2, 0xb727\n"
                             "vsub.xyz vf6, vf6, vf1\n"
                             "ori %2, %2, 0xc5ac\n"
                             "vadd.xyz vf12, vf0, vf5\n"
                             "vmul.xyz vf9, vf8, vf4\n"
                             "vadday.x ACC, vf9, vf9y\n"
                             "vmaddz.x vf9, vf11, vf9z\n"
                             "qmfc2 %0, vf9\n"
                             "mtc1 %1, %5\n"
                             "mtc1 %0, %4\n"
                             "mtc1 %2, %6\n"
                             : "=&r"(x), "=&r"(y), "=&r"(z), "=&r"(packed), "=f"(det),
                               "=f"(epsilon), "=f"(tolerance)
                             : "r"(start), "r"(delta), "r"(v0), "r"(v1), "r"(v2)
                             : "memory");
                // Flip the edges for the reverse-facing triangle.
                if (det < epsilon)
                {
                    asm volatile("vopmula.xyz ACC, vf7, vf4\n"
                                 "vopmsub.xyz vf8, vf4, vf7\n"
                                 "vadd.xyz vf5, vf0, vf4\n"
                                 "vadd.xyz vf4, vf0, vf12\n"
                                 "vmul.xyz vf9, vf8, vf12\n"
                                 "vadday.x ACC, vf9, vf9y\n"
                                 "vmaddz.x vf9, vf11, vf9z\n"
                                 "qmfc2 %1, vf9\n"
                                 "mtc1 %1, %0\n"
                                 : "=f"(det), "=&r"(x) : : "memory");
                }
                if (det <= -epsilon)
                    continue;
                asm volatile("vmul.xyz vf9, vf8, vf6\n"
                             "vadday.x ACC, vf9, vf9y\n"
                             "vmaddz.x vf9, vf11, vf9z\n"
                             "qmfc2 %1, vf9\n"
                             "mtc1 %1, %0\n"
                             : "=f"(u), "=&r"(x) : : "memory");
                F32 lo = det * tolerance;
                F32 hi = det - lo;
                if (u < lo)
                    continue;
                if (hi < u)
                    continue;
                asm volatile("vopmula.xyz ACC, vf6, vf4\n"
                             "vopmsub.xyz vf8, vf4, vf6\n"
                             "vmul.xyz vf9, vf8, vf7\n"
                             "vadday.x ACC, vf9, vf9y\n"
                             "vmaddz.x vf9, vf11, vf9z\n"
                             "qmfc2 %1, vf9\n"
                             "mtc1 %1, %0\n"
                             : "=f"(v), "=&r"(x) : : "memory");
                if (v < lo)
                    continue;
                u += v;
                if (hi < u)
                    continue;
                asm volatile("vmul.xyz vf9, vf8, vf5\n"
                             "vadday.x ACC, vf9, vf9y\n"
                             "vmaddz.x vf9, vf11, vf9z\n"
                             "qmfc2 %1, vf9\n"
                             "mtc1 %1, %0\n"
                             : "=f"(t), "=&r"(x) : : "memory");
                if (t < lo)
                    continue;
                if (hi < t)
                    continue;
                F32 hit_distance = t / det;
                asm volatile("swc1 %0, 0(%1)" : : "f"(hit_distance), "r"(&distance) : "memory");
                result = 1;
            }
#else
            RwV3dSubMacro(&edge1, v1, v0);
            RwV3dSubMacro(&edge2, v2, v0);
            RwV3dCrossProductMacro(&pVec, &isData->delta, &edge2);
            det = RwV3dDotProductMacro(&edge1, &pVec);
            if (det < -1e-8f)
            {
                RwV3d edgetmp = edge1;
                edge1 = edge2;
                edge2 = edgetmp;
                RwV3dCrossProductMacro(&pVec, &isData->delta, &edge2);
                det = RwV3dDotProductMacro(&edge1, &pVec);
            }
            result = (det > 1e-8f);
            if (result)
            {
                F32 lo, hi, u, v;
                lo = 0.00001f * -det;
                hi = det - lo;
                RwV3dSubMacro(&tVec, &isData->start, v0);
                u = RwV3dDotProductMacro(&tVec, &pVec);
                result = (u >= lo && u <= hi);
                if (result)
                {
                    RwV3dCrossProductMacro(&qVec, &tVec, &edge1);
                    v = RwV3dDotProductMacro(&isData->delta, &qVec);
                    result = (v >= lo && u + v <= hi);
                    if (result)
                    {
                        distance = RwV3dDotProductMacro(&edge2, &qVec);
                        result = (distance >= lo && distance <= hi);
                        if (result)
                        {
                            distance /= det;
                        }
                    }
                }
            }
#endif
            if (result)
            {
                RpCollisionTriangle collisionTri;
                RwV3d vTmp, vTmp2;
                F32 recipLength, lengthSq;
                collisionTri.point = *v0;
                collisionTri.index = (RwInt32)triangles;
                collisionTri.vertices[0] = v0;
                collisionTri.vertices[1] = v1;
                collisionTri.vertices[2] = v2;
                RwV3dSubMacro(&vTmp, collisionTri.vertices[1], collisionTri.vertices[0]);
                RwV3dSubMacro(&vTmp2, collisionTri.vertices[2], collisionTri.vertices[0]);
                RwV3dCrossProductMacro(&collisionTri.normal, &vTmp, &vTmp2);
                lengthSq = RwV3dDotProductMacro(&collisionTri.normal, &collisionTri.normal);
#if defined(PS2)
                recipLength = sqrtf(lengthSq);
                if (recipLength > 0.0f)
                {
                    recipLength = 1.0f / recipLength;
                }
#else
                recipLength = _rwInvSqrt(lengthSq);
#endif
                RwV3dScaleMacro(&collisionTri.normal, &collisionTri.normal, recipLength);
                if (!cbParam->u.worldCB(cbParam->intersection, NULL, &collisionTri, distance,
                                        cbParam->data))
                {
                    return 0;
                }
            }
        }
    } while ((triangles++)->flags & 0x1);
    return 1;
}

#if defined(PS2)
static RwBool FastIntersectSphereTriangle(RwSphere* sphere, RwV3d* v0, RwV3d* v1, RwV3d* v2,
                                          RwV3d* normal, RwReal* distance, RwV3d* vc);

// The PS2 broad phase uses hardware min/max, not comparison-and-select branches.
static inline void SphereMinMax(F32 a, F32 b, F32 c, F32& lo, F32& hi)
{
    asm volatile("min.s %0, %2, %3\n"
                 "max.s %1, %2, %3\n"
                 "min.s %0, %0, %4\n"
                 "max.s %1, %1, %4\n"
                 : "=&f"(lo), "=&f"(hi) : "f"(a), "f"(b), "f"(c));
}

// Reject the triangle when it lies wholly beyond the sphere along one axis, otherwise
// record its vertices relative to the sphere centre.
#define SphereAxisReject(_axis)                                                    \
    {                                                                              \
        F32 a = v0->_axis, b = v1->_axis, c = v2->_axis;                           \
        F32 centre = sphere->center._axis;                                         \
        F32 lo, hi;                                                                \
        SphereMinMax(a, b, c, lo, hi);                                             \
        if (centre + radius <= lo) continue;                                       \
        if (hi <= centre - radius) continue;                                       \
        vc[0]._axis = a - centre;                                                  \
        vc[1]._axis = b - centre;                                                  \
        vc[2]._axis = c - centre;                                                  \
    }

#endif

static S32 LeafNodeSpherePolyIntersect(xClumpCollBSPTriangle* triangles, void* data)
{
    PolyTestParam* isData = (PolyTestParam*)data;
    CallBackParam* cbParam = isData->cbParam;
    TestSphere* testSphere = (TestSphere*)isData->leafTestData;
    do
    {
        if (triangles->flags & xClumpColl_FilterFlags)
        {
            RwV3d *v0, *v1, *v2;
            F32 distance;
            RpCollisionTriangle collisionTri;
            v0 = &triangles->v.p[0];
            if (triangles->flags & 0x2)
            {
                v1 = &triangles->v.p[2];
                v2 = &triangles->v.p[1];
            }
            else
            {
                v1 = &triangles->v.p[1];
                v2 = &triangles->v.p[2];
            }
#if defined(PS2)
            RwV3d vc[3];
            const RwSphere* sphere = testSphere->sphere;
            F32 radius = sphere->radius;

            SphereAxisReject(x);
            SphereAxisReject(y);
            SphereAxisReject(z);

            if (FastIntersectSphereTriangle(testSphere->sphere, v0, v1, v2, &collisionTri.normal,
                                            &distance, vc))
#else
            if (RtIntersectionSphereTriangle(testSphere->sphere, v0, v1, v2, &collisionTri.normal,
                                             &distance))
#endif
            {
                collisionTri.point = *v0;
                collisionTri.index = (RwInt32)triangles;
                collisionTri.vertices[0] = v0;
                collisionTri.vertices[1] = v1;
                collisionTri.vertices[2] = v2;
                distance *= testSphere->recipRadius;
                if (!cbParam->u.worldCB(cbParam->intersection, NULL, &collisionTri, distance,
                                        cbParam->data))
                {
                    return 0;
                }
            }
        }
    } while ((triangles++)->flags & 0x1);
    return 1;
}

static S32 LeafNodeBoxPolyIntersect(xClumpCollBSPTriangle* triangles, void* data)
{
    PolyTestParam* isData = (PolyTestParam*)data;
    CallBackParam* cbParam = isData->cbParam;
    do
    {
        if (triangles->flags & xClumpColl_FilterFlags)
        {
            RwV3d *v0, *v1, *v2;
            v0 = &triangles->v.p[0];
            if (triangles->flags & 0x2)
            {
                v1 = &triangles->v.p[2];
                v2 = &triangles->v.p[1];
            }
            else
            {
                v1 = &triangles->v.p[1];
                v2 = &triangles->v.p[2];
            }
            if (RtIntersectionBBoxTriangle(&isData->bbox, v0, v1, v2))
            {
                RpCollisionTriangle collisionTri;
                RwV3d vTmp, vTmp2;

                collisionTri.point = *v0;
                collisionTri.index = (RwInt32)triangles;
                RwV3dSubMacro(&vTmp, v1, v0);
                RwV3dSubMacro(&vTmp2, v2, v0);
                RwV3dCrossProductMacro(&collisionTri.normal, &vTmp, &vTmp2);

                F32 lengthSq = RwV3dDotProductMacro(&collisionTri.normal, &collisionTri.normal);
#if defined(PS2)
                F32 recipLength = sqrtf(lengthSq);
                recipLength = (recipLength > 0.0f) ? 1.0f / recipLength : recipLength;
#else
                F32 recipLength = _rwInvSqrt(lengthSq);
#endif

                RwV3dScaleMacro(&collisionTri.normal, &collisionTri.normal, recipLength);

                collisionTri.vertices[0] = v0;
                collisionTri.vertices[1] = v1;
                collisionTri.vertices[2] = v2;

                if (!cbParam->u.worldCB(cbParam->intersection, NULL, &collisionTri, 0.0f,
                                        cbParam->data))
                {
                    return 0;
                }
            }
        }
    } while ((triangles++)->flags & 0x1);
    return 1;
}

xClumpCollBSPTree* xClumpColl_ForAllIntersections(xClumpCollBSPTree* tree,
                                                  RpIntersection* intersection,
                                                  RpIntersectionCallBackWorldTriangle callBack,
                                                  void* data)
{
    CallBackParam cbParam;
    cbParam.intersection = intersection;
    cbParam.u.worldCB = callBack;
    cbParam.data = data;

    switch (intersection->type)
    {
        case rpINTERSECTPOINT:
            return NULL;
        case rpINTERSECTLINE:
        {
            PolyLineTestParam isData;
            RwLine* line = &intersection->t.line;
            F32 recip;

            isData.start = line->start;
            RwV3dSubMacro(&isData.delta, &line->end, &line->start);
            isData.cbParam = &cbParam;
            isData.line = *line;

            recip = (isData.delta.x != 0.0f) ? (1.0f / isData.delta.x) : 0.0f;
            isData.grad.dydx = isData.delta.y * recip;
            isData.grad.dzdx = isData.delta.z * recip;

            recip = (isData.delta.y != 0.0f) ? (1.0f / isData.delta.y) : 0.0f;
            isData.grad.dxdy = isData.delta.x * recip;
            isData.grad.dzdy = isData.delta.z * recip;

            recip = (isData.delta.z != 0.0f) ? (1.0f / isData.delta.z) : 0.0f;
            isData.grad.dxdz = isData.delta.x * recip;
            isData.grad.dydz = isData.delta.y * recip;

            xClumpColl_ForAllLineLeafNodeIntersections(tree, line, &isData.grad,
                                                    LeafNodeLinePolyIntersect, &isData);

            return tree;
        }
        case rpINTERSECTSPHERE:
        {
            PolyTestParam isData;
            TestSphere testSphere;

            isData.bbox.inf = isData.bbox.sup = intersection->t.sphere.center;
            isData.bbox.inf.x -= intersection->t.sphere.radius;
            isData.bbox.inf.y -= intersection->t.sphere.radius;
            isData.bbox.inf.z -= intersection->t.sphere.radius;
            isData.bbox.sup.x += intersection->t.sphere.radius;
            isData.bbox.sup.y += intersection->t.sphere.radius;
            isData.bbox.sup.z += intersection->t.sphere.radius;

            testSphere.sphere = &intersection->t.sphere;
            testSphere.recipRadius = 1.0f / testSphere.sphere->radius;

            isData.leafTestData = &testSphere;
            isData.cbParam = &cbParam;

            xClumpColl_ForAllBoxLeafNodeIntersections(tree, &isData.bbox, LeafNodeSpherePolyIntersect,
                                                    &isData);

            return tree;
        }
        case rpINTERSECTBOX:
        {
            PolyTestParam isData;

            isData.bbox = intersection->t.box;
            isData.cbParam = &cbParam;

            xClumpColl_ForAllBoxLeafNodeIntersections(tree, &isData.bbox, LeafNodeBoxPolyIntersect,
                                                    &isData);

            return tree;
        }
    }

    return NULL;
}

#if defined(PS2)
static RwBool FastIntersectSphereTriangle(RwSphere* sphere, RwV3d* v0, RwV3d* v1, RwV3d* v2,
                                          RwV3d* normal, RwReal* distance, RwV3d* vc)
{
    RwReal nDotN;
    RwReal distToPlane;
    RwReal sphereRadiusSquared;
    RwReal length2;
    RwReal factor;
    RwV3d vAtoB;
    RwV3d vN;
    RwV3d vTmp;
    RwV3d vTmp2;

    // Triangle plane
    RwV3dSubMacro(&vAtoB, v1, v0);
    RwV3dSubMacro(&vN, v2, v0);
    RwV3dCrossProductMacro(normal, &vAtoB, &vN);

    nDotN = RwV3dDotProductMacro(normal, normal);
    if (nDotN <= 0.0f)
    {
        return FALSE;
    }

    factor = sqrtf(nDotN);
    factor = (factor > 0.0f) ? 1.0f / factor : factor;
    RwV3dScaleMacro(normal, normal, factor);

    distToPlane = RwV3dDotProductMacro(&vc[0], normal);
    if (distToPlane < -sphere->radius || distToPlane > sphere->radius)
    {
        return FALSE;
    }

    *distance = -distToPlane;

    // Is any vertex inside the sphere?
    sphereRadiusSquared = sphere->radius * sphere->radius;

    vTmp.x = RwV3dDotProductMacro(&vc[0], &vc[0]);
    if (vTmp.x <= sphereRadiusSquared)
    {
        return TRUE;
    }

    vTmp.y = RwV3dDotProductMacro(&vc[1], &vc[1]);
    if (vTmp.y <= sphereRadiusSquared)
    {
        return TRUE;
    }

    vTmp.z = RwV3dDotProductMacro(&vc[2], &vc[2]);
    if (vTmp.z <= sphereRadiusSquared)
    {
        return TRUE;
    }

    // Reject if the sphere lies beyond the closest vertex
    if (vTmp.x < vTmp.y)
    {
        if (vTmp.z < vTmp.x)
        {
            if (RwV3dDotProductMacro(&vc[2], &vc[0]) > vTmp.z &&
                RwV3dDotProductMacro(&vc[2], &vc[1]) > vTmp.z)
            {
                return FALSE;
            }
        }
        else
        {
            if (RwV3dDotProductMacro(&vc[0], &vc[1]) > vTmp.x &&
                RwV3dDotProductMacro(&vc[0], &vc[2]) > vTmp.x)
            {
                return FALSE;
            }
        }
    }
    else
    {
        if (vTmp.z < vTmp.y)
        {
            if (RwV3dDotProductMacro(&vc[2], &vc[0]) > vTmp.z &&
                RwV3dDotProductMacro(&vc[2], &vc[1]) > vTmp.z)
            {
                return FALSE;
            }
        }
        else
        {
            if (RwV3dDotProductMacro(&vc[1], &vc[0]) > vTmp.y &&
                RwV3dDotProductMacro(&vc[1], &vc[2]) > vTmp.y)
            {
                return FALSE;
            }
        }
    }

    // Reject if the sphere lies beyond any edge
    RwV3dSubMacro(&vTmp2, &vc[1], &vc[0]);
    factor = RwV3dDotProductMacro(&vTmp2, &vc[0]) / RwV3dDotProductMacro(&vTmp2, &vTmp2);
    RwV3dScaleMacro(&vTmp, &vTmp2, factor);
    RwV3dSubMacro(&vTmp, &vc[0], &vTmp);
    length2 = RwV3dDotProductMacro(&vTmp, &vTmp);
    if (length2 > sphereRadiusSquared && length2 < RwV3dDotProductMacro(&vTmp, &vc[2]))
    {
        return FALSE;
    }

    RwV3dSubMacro(&vTmp2, &vc[2], &vc[1]);
    factor = RwV3dDotProductMacro(&vTmp2, &vc[1]) / RwV3dDotProductMacro(&vTmp2, &vTmp2);
    RwV3dScaleMacro(&vTmp, &vTmp2, factor);
    RwV3dSubMacro(&vTmp, &vc[1], &vTmp);
    length2 = RwV3dDotProductMacro(&vTmp, &vTmp);
    if (length2 > sphereRadiusSquared && length2 < RwV3dDotProductMacro(&vTmp, &vc[0]))
    {
        return FALSE;
    }

    RwV3dSubMacro(&vTmp2, &vc[0], &vc[2]);
    factor = RwV3dDotProductMacro(&vTmp2, &vc[2]) / RwV3dDotProductMacro(&vTmp2, &vTmp2);
    RwV3dScaleMacro(&vTmp, &vTmp2, factor);
    RwV3dSubMacro(&vTmp, &vc[2], &vTmp);
    length2 = RwV3dDotProductMacro(&vTmp, &vTmp);
    if (length2 > sphereRadiusSquared && length2 < RwV3dDotProductMacro(&vTmp, &vc[1]))
    {
        return FALSE;
    }

    return TRUE;
}
#endif
