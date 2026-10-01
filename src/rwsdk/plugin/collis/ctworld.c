#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rtintsec.h>
#include <rwsdk/rpcollis.h>
#include <rwsdk/rpcollbsptree.h>

#define rwID_COLLISPLUGIN 0x11D

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwID_COLLISPLUGIN;                                                 \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RP_COLLIS_INV_INTERSECTION 0

#define rwInvSqrtMacro(_result, _num) ((*(_result)) = _rwInvSqrt(_num))

#define rpCOLLISWORLDMAXBSPDEPTH 64

#define rpCOLLISLINETRIANGLEEPSILON ((RwReal)1e-8)
#define rpCOLLISLINETRIANGLEEDGEEPSILON ((RwReal)1e-5)

/* Line / triangle intersection, the distance is a parameter along the line */
#define RpCollisLineTriangleIntersectMacro(_result, _lineStart, _lineDelta, _v0, _v1, _v2,        \
                                           _distance)                                              \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwV3d _edge1, _edge2, _tVec, _pVec, _qVec;                                                 \
        RwReal _det;                                                                               \
                                                                                                   \
        RwV3dSubMacro(&_edge1, (_v1), (_v0));                                                      \
        RwV3dSubMacro(&_edge2, (_v2), (_v0));                                                      \
                                                                                                   \
        RwV3dCrossProductMacro(&_pVec, (_lineDelta), &_edge2);                                     \
        _det = RwV3dDotProductMacro(&_edge1, &_pVec);                                              \
                                                                                                   \
        (_result) = (_det > rpCOLLISLINETRIANGLEEPSILON);                                          \
        if (_result)                                                                               \
        {                                                                                          \
            RwReal _lo, _hi, _u, _v;                                                               \
                                                                                                   \
            _lo = -_det * rpCOLLISLINETRIANGLEEDGEEPSILON;                                         \
            _hi = _det - _lo;                                                                      \
                                                                                                   \
            RwV3dSubMacro(&_tVec, (_lineStart), (_v0));                                            \
            _u = RwV3dDotProductMacro(&_tVec, &_pVec);                                             \
                                                                                                   \
            (_result) = (_u >= _lo && _u <= _hi);                                                  \
            if (_result)                                                                           \
            {                                                                                      \
                RwV3dCrossProductMacro(&_qVec, &_tVec, &_edge1);                                   \
                _v = RwV3dDotProductMacro((_lineDelta), &_qVec);                                   \
                                                                                                   \
                (_result) = (_v >= _lo && _u + _v <= _hi);                                         \
                if (_result)                                                                       \
                {                                                                                  \
                    (_distance) = RwV3dDotProductMacro(&_edge2, &_qVec);                           \
                                                                                                   \
                    (_result) = ((_distance) >= _lo && (_distance) <= _hi);                        \
                    if (_result)                                                                   \
                    {                                                                              \
                        (_distance) /= _det;                                                       \
                    }                                                                              \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

/* Unit normal of a triangle */
#define RpCollisTriangleNormalMacro(_normal, _v0, _v1, _v2)                                        \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwV3d _a, _b;                                                                              \
        RwReal _recipLength;                                                                       \
                                                                                                   \
        RwV3dSubMacro(&_a, (_v1), (_v0));                                                          \
        RwV3dSubMacro(&_b, (_v2), (_v0));                                                          \
        RwV3dCrossProductMacro((_normal), &_a, &_b);                                               \
        _rwV3dNormalizeMacro(_recipLength, (_normal), (_normal));                                  \
    }                                                                                              \
    MACRO_STOP

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

typedef RwInt32 (*RpCollBSPLeafCallBack)(RwInt32 numPolygons, RwInt32 firstPolygon, void* data);

typedef struct RpCollisWorldCallBackData RpCollisWorldCallBackData;
struct RpCollisWorldCallBackData
{
    RpIntersection* intersection;
    RpIntersectionCallBackWorldTriangle callBack;
    void* data;
};

typedef struct RpCollisWorldLineData RpCollisWorldLineData;
struct RpCollisWorldLineData
{
    RpV3dGradient grad;
    RwLine line;
    RpWorldSector* sector;
    RwLine* worldLine;
    RwV3d delta;
    RpCollisWorldCallBackData* cbData;
};

typedef struct RpCollisSphereData RpCollisSphereData;
struct RpCollisSphereData
{
    RwSphere* sphere;
    RwReal recipRadius;
};

typedef struct RpCollisWorldBoxData RpCollisWorldBoxData;
struct RpCollisWorldBoxData
{
    RwBBox box;
    RpWorldSector* sector;
    RpCollBSPLeafCallBack leafCallBack;
    void* primitiveData;
    RpCollisWorldCallBackData* cbData;
};

extern RpCollBSPTree* _rpCollBSPTreeForAllLineLeafNodeIntersections(RpCollBSPTree* tree,
                                                                    RwLine* line,
                                                                    RpV3dGradient* grad,
                                                                    RpCollBSPLeafCallBack callBack,
                                                                    void* data);
extern RpCollBSPTree* _rpCollBSPTreeForAllBoxLeafNodeIntersections(RpCollBSPTree* tree, RwBBox* box,
                                                                   RpCollBSPLeafCallBack callBack,
                                                                   void* data);

extern RpWorld* WorldForAllLineWorldSectorIntersections(RpWorld* world, RwLine* line,
                                                        RpV3dGradient* grad,
                                                        RpWorldSectorCallBack callBack, void* data,
                                                        RwLine* sectorLine);

/* Walk the world BSP, calling the leaf callback for the world sectors the box overlaps */
#define RpCollisWorldForAllBoxSectorsMacro(_world, _isData)                                        \
    MACRO_START                                                                                    \
    {                                                                                              \
        RpSector* _stack[rpCOLLISWORLDMAXBSPDEPTH];                                                \
        RpSector* _sector = (_world)->rootSector;                                                  \
        RwInt32 _nStack = 0;                                                                       \
                                                                                                   \
        while (_nStack >= 0)                                                                       \
        {                                                                                          \
            if (_sector->type < 0)                                                                 \
            {                                                                                      \
                RpCollisionData* _collData = *RWPLUGINOFFSET(                                      \
                    RpCollisionData*, _sector, _rpCollisionWorldSectorDataOffset);                 \
                                                                                                   \
                if (_collData)                                                                     \
                {                                                                                  \
                    (_isData)->sector = (RpWorldSector*)_sector;                                   \
                                                                                                   \
                    if (!_rpCollBSPTreeForAllBoxLeafNodeIntersections(                             \
                            _collData->tree, &(_isData)->box, (_isData)->leafCallBack, (_isData))) \
                    {                                                                              \
                        _sector = (RpSector*)NULL;                                                 \
                    }                                                                              \
                }                                                                                  \
                                                                                                   \
                if (!_sector)                                                                      \
                {                                                                                  \
                    break;                                                                         \
                }                                                                                  \
                                                                                                   \
                _sector = _stack[_nStack--];                                                       \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                RpPlaneSector* _plane = (RpPlaneSector*)_sector;                                   \
                                                                                                   \
                if (*(RwReal*)((RwUInt8*)&(_isData)->box.inf + _plane->type) < _plane->leftValue)  \
                {                                                                                  \
                    _sector = _plane->leftSubTree;                                                 \
                                                                                                   \
                    if (*(RwReal*)((RwUInt8*)&(_isData)->box.sup + _plane->type) >=                \
                        _plane->rightValue)                                                        \
                    {                                                                              \
                        _stack[++_nStack] = _plane->rightSubTree;                                  \
                    }                                                                              \
                }                                                                                  \
                else                                                                               \
                {                                                                                  \
                    _sector = _plane->rightSubTree;                                                \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

static RwInt32 LeafNodeForAllLinePolyIntersections(RwInt32 numPolygons, RwInt32 firstPolygon,
                                                   void* data)
{
    RpCollisWorldLineData* isData = (RpCollisWorldLineData*)data;
    RpCollisWorldCallBackData* cbData = isData->cbData;
    RpWorldSector* sector = isData->sector;
    RwV3d* vertices;
    RpPolygon* polygons;
    RwUInt16* triangleMap;

    triangleMap = (*RWPLUGINOFFSET(RpCollisionData*, sector, _rpCollisionWorldSectorDataOffset))
                      ->triangleMap +
                  firstPolygon;
    vertices = sector->vertices;
    polygons = sector->polygons;

    while (numPolygons--)
    {
        RwInt32 index = *triangleMap;
        RpPolygon* polygon = &polygons[index];
        RwV3d* v0 = &vertices[polygon->vertIndex[0]];
        RwV3d* v1 = &vertices[polygon->vertIndex[1]];
        RwV3d* v2 = &vertices[polygon->vertIndex[2]];
        RwBool result;
        RwReal distance;

        RpCollisLineTriangleIntersectMacro(result, &isData->worldLine->start, &isData->delta, v0,
                                           v1, v2, distance);

        if (result)
        {
            RpCollisionTriangle collTriangle;

            collTriangle.point = *v0;
            collTriangle.index = index;

            RpCollisTriangleNormalMacro(&collTriangle.normal, v0, v1, v2);

            collTriangle.vertices[0] = v0;
            collTriangle.vertices[1] = v1;
            collTriangle.vertices[2] = v2;

            if (!cbData->callBack(cbData->intersection, sector, &collTriangle, distance,
                                  cbData->data))
            {
                return FALSE;
            }
        }

        triangleMap++;
    }

    return TRUE;
}

static RwInt32 LeafNodeForAllSpherePolyIntersections(RwInt32 numPolygons, RwInt32 firstPolygon,
                                                     void* data)
{
    RpCollisWorldBoxData* isData = (RpCollisWorldBoxData*)data;
    RpCollisWorldCallBackData* cbData = isData->cbData;
    RpCollisSphereData* sphereData = (RpCollisSphereData*)isData->primitiveData;
    RpWorldSector* sector = isData->sector;
    RwV3d* vertices;
    RpPolygon* polygons;
    RwUInt16* triangleMap;

    triangleMap = (*RWPLUGINOFFSET(RpCollisionData*, sector, _rpCollisionWorldSectorDataOffset))
                      ->triangleMap +
                  firstPolygon;
    vertices = sector->vertices;
    polygons = sector->polygons;

    while (numPolygons--)
    {
        RpPolygon* polygon = &polygons[*triangleMap];
        RwV3d* v0 = &vertices[polygon->vertIndex[0]];
        RwV3d* v1 = &vertices[polygon->vertIndex[1]];
        RwV3d* v2 = &vertices[polygon->vertIndex[2]];
        RpCollisionTriangle collTriangle;
        RwReal distance;

        if (RtIntersectionSphereTriangle(sphereData->sphere, v0, v1, v2, &collTriangle.normal,
                                         &distance))
        {
            collTriangle.point = *v0;
            collTriangle.index = *triangleMap;
            collTriangle.vertices[0] = v0;
            collTriangle.vertices[1] = v1;
            collTriangle.vertices[2] = v2;

            distance *= sphereData->recipRadius;

            if (!cbData->callBack(cbData->intersection, sector, &collTriangle, distance,
                                  cbData->data))
            {
                return FALSE;
            }
        }

        triangleMap++;
    }

    return TRUE;
}

static RwInt32 LeafNodeForAllBoxPolyIntersections(RwInt32 numPolygons, RwInt32 firstPolygon,
                                                  void* data)
{
    RpCollisWorldBoxData* isData = (RpCollisWorldBoxData*)data;
    RpCollisWorldCallBackData* cbData = isData->cbData;
    RpWorldSector* sector = isData->sector;
    RwV3d* vertices;
    RpPolygon* polygons;
    RwUInt16* triangleMap;

    triangleMap = (*RWPLUGINOFFSET(RpCollisionData*, sector, _rpCollisionWorldSectorDataOffset))
                      ->triangleMap +
                  firstPolygon;
    vertices = sector->vertices;
    polygons = sector->polygons;

    while (numPolygons--)
    {
        RpPolygon* polygon = &polygons[*triangleMap];
        RwV3d* v0 = &vertices[polygon->vertIndex[0]];
        RwV3d* v1 = &vertices[polygon->vertIndex[1]];
        RwV3d* v2 = &vertices[polygon->vertIndex[2]];

        if (RtIntersectionBBoxTriangle(&isData->box, v0, v1, v2))
        {
            RpCollisionTriangle collTriangle;

            collTriangle.point = *v0;
            collTriangle.index = *triangleMap;

            RpCollisTriangleNormalMacro(&collTriangle.normal, v0, v1, v2);

            collTriangle.vertices[0] = v0;
            collTriangle.vertices[1] = v1;
            collTriangle.vertices[2] = v2;

            if (!cbData->callBack(cbData->intersection, sector, &collTriangle, (RwReal)0,
                                  cbData->data))
            {
                return FALSE;
            }
        }

        triangleMap++;
    }

    return TRUE;
}

static RpWorldSector* WorldSectorForAllLinePolyIntersections(RpWorldSector* sector, void* data)
{
    RpCollisWorldLineData* isData = (RpCollisWorldLineData*)data;
    RpCollisionData* collData =
        *RWPLUGINOFFSET(RpCollisionData*, sector, _rpCollisionWorldSectorDataOffset);

    if (collData)
    {
        isData->sector = sector;

        if (!_rpCollBSPTreeForAllLineLeafNodeIntersections(collData->tree, &isData->line,
                                                           &isData->grad,
                                                           LeafNodeForAllLinePolyIntersections,
                                                           isData))
        {
            return (RpWorldSector*)NULL;
        }
    }

    return sector;
}

static RpWorldSector* WorldSectorForAllBoxedPrimitivePolyIntersections(RpWorldSector* sector,
                                                                       void* data)
{
    RpCollisWorldBoxData* isData = (RpCollisWorldBoxData*)data;
    RpCollisionData* collData =
        *RWPLUGINOFFSET(RpCollisionData*, sector, _rpCollisionWorldSectorDataOffset);

    if (collData)
    {
        isData->sector = sector;

        if (!_rpCollBSPTreeForAllBoxLeafNodeIntersections(collData->tree, &isData->box,
                                                          isData->leafCallBack, isData))
        {
            return (RpWorldSector*)NULL;
        }
    }

    return sector;
}

RpWorld* RpCollisionWorldForAllIntersections(RpWorld* world, RpIntersection* intersection,
                                             RpIntersectionCallBackWorldTriangle callBack,
                                             void* data)
{
    RpCollisWorldCallBackData cbData;

    cbData.callBack = callBack;
    cbData.intersection = intersection;
    cbData.data = data;

    switch (intersection->type)
    {
    case rpINTERSECTPOINT:
        RWERROR((E_RP_COLLIS_INV_INTERSECTION));
        return (RpWorld*)NULL;

    case rpINTERSECTLINE:
    {
        RpCollisWorldLineData isData;
        RwReal recip;

        isData.worldLine = &intersection->t.line;
        RwV3dSubMacro(&isData.delta, &intersection->t.line.end, &intersection->t.line.start);
        isData.cbData = &cbData;

        recip = (isData.delta.x != (RwReal)0) ? (RwReal)1 / isData.delta.x : (RwReal)0;
        isData.grad.dydx = isData.delta.y * recip;
        isData.grad.dzdx = isData.delta.z * recip;

        recip = (isData.delta.y != (RwReal)0) ? (RwReal)1 / isData.delta.y : (RwReal)0;
        isData.grad.dxdy = isData.delta.x * recip;
        isData.grad.dzdy = isData.delta.z * recip;

        recip = (isData.delta.z != (RwReal)0) ? (RwReal)1 / isData.delta.z : (RwReal)0;
        isData.grad.dxdz = isData.delta.x * recip;
        isData.grad.dydz = isData.delta.y * recip;

        WorldForAllLineWorldSectorIntersections(world, &intersection->t.line, &isData.grad,
                                                WorldSectorForAllLinePolyIntersections, &isData,
                                                &isData.line);
        return world;
    }

    case rpINTERSECTSPHERE:
    {
        RpCollisSphereData sphereData;
        RpCollisWorldBoxData isData;

        sphereData.sphere = &intersection->t.sphere;

        isData.box.sup = intersection->t.sphere.center;
        isData.box.inf = intersection->t.sphere.center;
        isData.box.inf.x -= intersection->t.sphere.radius;
        isData.box.inf.y -= intersection->t.sphere.radius;
        isData.box.inf.z -= intersection->t.sphere.radius;
        isData.box.sup.x += intersection->t.sphere.radius;
        isData.box.sup.y += intersection->t.sphere.radius;
        isData.box.sup.z += intersection->t.sphere.radius;

        sphereData.recipRadius = (RwReal)1 / intersection->t.sphere.radius;

        isData.leafCallBack = LeafNodeForAllSpherePolyIntersections;
        isData.primitiveData = &sphereData;
        isData.cbData = &cbData;

        RpCollisWorldForAllBoxSectorsMacro(world, &isData);

        return world;
    }

    case rpINTERSECTBOX:
    {
        RpCollisWorldBoxData isData;

        isData.box = intersection->t.box;
        isData.leafCallBack = LeafNodeForAllBoxPolyIntersections;
        isData.cbData = &cbData;

        RpCollisWorldForAllBoxSectorsMacro(world, &isData);

        return world;
    }

    case rpINTERSECTATOMIC:
    {
        RpAtomic* atomic = (RpAtomic*)intersection->t.object;
        RwSphere sphere = *RpAtomicGetWorldBoundingSphere(atomic);
        RpCollisSphereData sphereData;
        RpCollisWorldBoxData isData;

        sphereData.sphere = &sphere;
        sphereData.recipRadius = (RwReal)1 / sphere.radius;

        isData.box.sup = sphere.center;
        isData.box.inf = sphere.center;
        isData.box.inf.x -= sphere.radius;
        isData.box.inf.y -= sphere.radius;
        isData.box.inf.z -= sphere.radius;
        isData.box.sup.x += sphere.radius;
        isData.box.sup.y += sphere.radius;
        isData.box.sup.z += sphere.radius;

        isData.leafCallBack = LeafNodeForAllSpherePolyIntersections;
        isData.primitiveData = &sphereData;
        isData.cbData = &cbData;

        RpAtomicForAllWorldSectors(atomic, WorldSectorForAllBoxedPrimitivePolyIntersections,
                                   &isData);

        return world;
    }

    default:
        RWERROR((E_RP_COLLIS_INV_INTERSECTION));
        return (RpWorld*)NULL;
    }
}
