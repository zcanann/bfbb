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
#define E_RW_NULLP 0x80000016

#define rwMatrixInitialize(_m, _t) ((_m)->flags = (RwUInt32)(_t))

#define rwSqrtMacro(_result, _num) ((*(_result)) = _rwSqrt(_num))
#define rwInvSqrtMacro(_result, _num) ((*(_result)) = _rwInvSqrt(_num))

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

/* Interpolate a vertex between two morph targets */
#define RpCollisInterpolateVertexMacro(_out, _start, _end, _scale)                                 \
    MACRO_START                                                                                    \
    {                                                                                              \
        const RwV3d* _s = (_start);                                                                \
        const RwV3d* _e = (_end);                                                                  \
                                                                                                   \
        RwV3dSubMacro((_out), _e, _s);                                                             \
        RwV3dScaleMacro((_out), (_out), (_scale));                                                 \
        RwV3dAddMacro((_out), (_out), _s);                                                         \
    }                                                                                              \
    MACRO_STOP

typedef struct RpCollisCallBackData RpCollisCallBackData;
struct RpCollisCallBackData
{
    RpIntersection* intersection;
    RpIntersectionCallBackGeometryTriangle callBack;
    void* data;
};

typedef struct RpCollisLineData RpCollisLineData;
struct RpCollisLineData
{
    RpGeometry* geometry;
    RpCollisCallBackData* cbData;
    RwLine* line;
    RwV3d delta;
};

typedef struct RpCollisSphereData RpCollisSphereData;
struct RpCollisSphereData
{
    RpGeometry* geometry;
    RpCollisCallBackData* cbData;
    RwSphere* sphere;
    RwReal recipRadius;
};

typedef RwInt32 (*RpCollBSPLeafCallBack)(RwInt32 numPolygons, RwInt32 firstPolygon, void* data);

extern RpCollBSPTree* _rpCollBSPTreeForAllLineLeafNodeIntersections(RpCollBSPTree* tree,
                                                                    RwLine* line,
                                                                    RpV3dGradient* grad,
                                                                    RpCollBSPLeafCallBack callBack,
                                                                    void* data);
extern RpCollBSPTree* _rpCollBSPTreeForAllBoxLeafNodeIntersections(RpCollBSPTree* tree, RwBBox* box,
                                                                   RpCollBSPLeafCallBack callBack,
                                                                   void* data);

static RwInt32 GeomLeafNodeForAllLineIntersections(RwInt32 numPolygons, RwInt32 firstPolygon,
                                                   void* data)
{
    RpCollisLineData* isData = (RpCollisLineData*)data;
    RpGeometry* geometry = isData->geometry;
    RwV3d* vertices;
    RpTriangle* triangles;
    RpCollisCallBackData* cbData = isData->cbData;
    RwUInt16* triangleMap;

    vertices = geometry->morphTarget->verts;
    triangles = geometry->triangles;
    triangleMap = (*RWPLUGINOFFSET(RpCollisionData*, geometry, _rpCollisionGeometryDataOffset))
                      ->triangleMap +
                  firstPolygon;

    while (numPolygons--)
    {
        RwInt32 index = *triangleMap;
        RpTriangle* triangle = &triangles[index];
        RwV3d* v0 = &vertices[triangle->vertIndex[0]];
        RwV3d* v1 = &vertices[triangle->vertIndex[1]];
        RwV3d* v2 = &vertices[triangle->vertIndex[2]];
        RwBool result;
        RwReal distance;

        RpCollisLineTriangleIntersectMacro(result, &isData->line->start, &isData->delta, v0, v1,
                                           v2, distance);

        if (result)
        {
            RpCollisionTriangle collTriangle;

            collTriangle.point = *v0;
            collTriangle.index = index;

            RpCollisTriangleNormalMacro(&collTriangle.normal, v0, v1, v2);

            collTriangle.vertices[0] = v0;
            collTriangle.vertices[1] = v1;
            collTriangle.vertices[2] = v2;

            if (!cbData->callBack(cbData->intersection, &collTriangle, distance, cbData->data))
            {
                return FALSE;
            }
        }

        triangleMap++;
    }

    return TRUE;
}

static RwInt32 GeomLeafNodeForAllSphereIntersections(RwInt32 numPolygons, RwInt32 firstPolygon,
                                                     void* data)
{
    RpCollisSphereData* isData = (RpCollisSphereData*)data;
    RpGeometry* geometry = isData->geometry;
    RwV3d* vertices;
    RpTriangle* triangles;
    RpCollisCallBackData* cbData = isData->cbData;
    RwUInt16* triangleMap;

    vertices = geometry->morphTarget->verts;
    triangles = geometry->triangles;
    triangleMap = (*RWPLUGINOFFSET(RpCollisionData*, geometry, _rpCollisionGeometryDataOffset))
                      ->triangleMap +
                  firstPolygon;

    while (numPolygons--)
    {
        RpTriangle* triangle = &triangles[*triangleMap];
        RwV3d* v0 = &vertices[triangle->vertIndex[0]];
        RwV3d* v1 = &vertices[triangle->vertIndex[1]];
        RwV3d* v2 = &vertices[triangle->vertIndex[2]];
        RpCollisionTriangle collTriangle;
        RwReal distance;

        if (RtIntersectionSphereTriangle(isData->sphere, v0, v1, v2, &collTriangle.normal,
                                         &distance))
        {
            collTriangle.point = *v0;
            collTriangle.index = *triangleMap;
            collTriangle.vertices[0] = v0;
            collTriangle.vertices[1] = v1;
            collTriangle.vertices[2] = v2;

            distance *= isData->recipRadius;

            if (!cbData->callBack(cbData->intersection, &collTriangle, distance, cbData->data))
            {
                return FALSE;
            }
        }

        triangleMap++;
    }

    return TRUE;
}

static RpGeometry* GeometryForAllLineIntersections(RpGeometry* geometry, RwLine* line,
                                                   RpCollisCallBackData* cbData)
{
    RpCollisionData* collData =
        *RWPLUGINOFFSET(RpCollisionData*, geometry, _rpCollisionGeometryDataOffset);

    if (collData)
    {
        RpV3dGradient grad;
        RpCollisLineData isData;
        RwReal recip;

        isData.cbData = cbData;
        isData.geometry = geometry;
        isData.line = line;
        RwV3dSubMacro(&isData.delta, &line->end, &line->start);

        recip = (isData.delta.x != (RwReal)0) ? (RwReal)1 / isData.delta.x : (RwReal)0;
        grad.dydx = isData.delta.y * recip;
        grad.dzdx = isData.delta.z * recip;

        recip = (isData.delta.y != (RwReal)0) ? (RwReal)1 / isData.delta.y : (RwReal)0;
        grad.dxdy = isData.delta.x * recip;
        grad.dzdy = isData.delta.z * recip;

        recip = (isData.delta.z != (RwReal)0) ? (RwReal)1 / isData.delta.z : (RwReal)0;
        grad.dxdz = isData.delta.x * recip;
        grad.dydz = isData.delta.y * recip;

        _rpCollBSPTreeForAllLineLeafNodeIntersections(collData->tree, line, &grad,
                                                      GeomLeafNodeForAllLineIntersections, &isData);
    }
    else
    {
        RwV3d delta;
        RwV3d* vertices;
        RpTriangle* triangle;
        RwInt32 i;

        RwV3dSubMacro(&delta, &line->end, &line->start);

        vertices = geometry->morphTarget->verts;
        triangle = geometry->triangles;

        for (i = 0; i < geometry->numTriangles; i++, triangle++)
        {
            RwV3d* v0 = &vertices[triangle->vertIndex[0]];
            RwV3d* v1 = &vertices[triangle->vertIndex[1]];
            RwV3d* v2 = &vertices[triangle->vertIndex[2]];
            RwBool result;
            RwReal distance;

            RpCollisLineTriangleIntersectMacro(result, &line->start, &delta, v0, v1, v2, distance);

            if (result)
            {
                RpCollisionTriangle collTriangle;

                collTriangle.point = *v0;
                collTriangle.index = i;

                RpCollisTriangleNormalMacro(&collTriangle.normal, v0, v1, v2);

                collTriangle.vertices[0] = v0;
                collTriangle.vertices[1] = v1;
                collTriangle.vertices[2] = v2;

                if (!cbData->callBack(cbData->intersection, &collTriangle, distance,
                                      cbData->data))
                {
                    return (RpGeometry*)NULL;
                }
            }
        }
    }

    return geometry;
}

static RpGeometry* GeometryForAllSphereIntersections(RpGeometry* geometry, RwSphere* sphere,
                                                     RpCollisCallBackData* cbData)
{
    RpCollisionData* collData =
        *RWPLUGINOFFSET(RpCollisionData*, geometry, _rpCollisionGeometryDataOffset);

    if (collData)
    {
        RpCollisSphereData isData;
        RwBBox box;

        isData.cbData = cbData;
        isData.geometry = geometry;
        isData.sphere = sphere;
        isData.recipRadius = (RwReal)1 / sphere->radius;

        box.sup = sphere->center;
        box.inf = sphere->center;
        box.inf.x -= sphere->radius;
        box.inf.y -= sphere->radius;
        box.inf.z -= sphere->radius;
        box.sup.x += sphere->radius;
        box.sup.y += sphere->radius;
        box.sup.z += sphere->radius;

        _rpCollBSPTreeForAllBoxLeafNodeIntersections(collData->tree, &box,
                                                     GeomLeafNodeForAllSphereIntersections,
                                                     &isData);
    }
    else
    {
        RwReal recipRadius = (RwReal)1 / sphere->radius;
        RwV3d* vertices = geometry->morphTarget->verts;
        RpTriangle* triangle = geometry->triangles;
        RwInt32 i;

        for (i = 0; i < geometry->numTriangles; i++, triangle++)
        {
            RwV3d* v0 = &vertices[triangle->vertIndex[0]];
            RwV3d* v1 = &vertices[triangle->vertIndex[1]];
            RwV3d* v2 = &vertices[triangle->vertIndex[2]];
            RpCollisionTriangle collTriangle;
            RwReal distance;

            if (RtIntersectionSphereTriangle(sphere, v0, v1, v2, &collTriangle.normal, &distance))
            {
                collTriangle.point = *v0;
                collTriangle.index = i;
                collTriangle.vertices[0] = v0;
                collTriangle.vertices[1] = v1;
                collTriangle.vertices[2] = v2;

                distance *= recipRadius;

                if (!cbData->callBack(cbData->intersection, &collTriangle, distance,
                                      cbData->data))
                {
                    return (RpGeometry*)NULL;
                }
            }
        }
    }

    return geometry;
}

static RpAtomic* AtomicForAllLineIntersections(RpAtomic* atomic, RwLine* line,
                                               RpCollisCallBackData* cbData)
{
    RpGeometry* geometry = atomic->geometry;
    RwMatrix invMatrix;
    RpCollisionTriangle collTriangle;
    RwLine localLine;
    RwV3d interpV0;
    RwV3d interpV1;
    RwV3d interpV2;

    rwMatrixInitialize(&invMatrix, rwMATRIXTYPENORMAL);
    RwMatrixInvert(&invMatrix, RwFrameGetLTM((RwFrame*)rwObjectGetParent(atomic)));
    RwV3dTransformPoints(&localLine.start, &line->start, 2, &invMatrix);

    if (geometry->numMorphTargets == 1)
    {
        if (!GeometryForAllLineIntersections(geometry, &localLine, cbData))
        {
            return (RpAtomic*)NULL;
        }
    }
    else
    {
        RwV3d delta;
        RwV3d* v0 = (RwV3d*)NULL;
        RwV3d* v1 = (RwV3d*)NULL;
        RwV3d* v2 = (RwV3d*)NULL;
        RwV3d* startVerts;
        RwV3d* endVerts;
        RwReal scale;
        RwInt32 i;
        RpTriangle* triangle;
        RwInt32 startMorph;
        RwInt32 endMorph;

        RwV3dSubMacro(&delta, &localLine.end, &localLine.start);

        startMorph = atomic->interpolator.startMorphTarget;
        endMorph = atomic->interpolator.endMorphTarget;
        scale = (RwReal)0;

        if (startMorph == endMorph || startMorph >= geometry->numMorphTargets ||
            endMorph >= geometry->numMorphTargets)
        {
            if (startMorph < geometry->numMorphTargets && endMorph < geometry->numMorphTargets)
            {
                startVerts = geometry->morphTarget[startMorph].verts;
            }
            else
            {
                startVerts = geometry->morphTarget[0].verts;
            }

            endVerts = startVerts;
        }
        else
        {
            startVerts = geometry->morphTarget[startMorph].verts;
            endVerts = geometry->morphTarget[endMorph].verts;
            scale = atomic->interpolator.recipTime * atomic->interpolator.position;

            v0 = &interpV0;
            v1 = &interpV1;
            v2 = &interpV2;
        }

        triangle = geometry->triangles;

        for (i = 0; i < geometry->numTriangles; triangle++, i++)
        {
            RwBool result;
            RwReal distance;

            if (startVerts == endVerts)
            {
                v0 = &startVerts[triangle->vertIndex[0]];
                v1 = &startVerts[triangle->vertIndex[1]];
                v2 = &startVerts[triangle->vertIndex[2]];
            }
            else
            {
                RpCollisInterpolateVertexMacro(&interpV0, &startVerts[triangle->vertIndex[0]],
                                               &endVerts[triangle->vertIndex[0]], scale);
                RpCollisInterpolateVertexMacro(&interpV1, &startVerts[triangle->vertIndex[1]],
                                               &endVerts[triangle->vertIndex[1]], scale);
                RpCollisInterpolateVertexMacro(&interpV2, &startVerts[triangle->vertIndex[2]],
                                               &endVerts[triangle->vertIndex[2]], scale);
            }

            RpCollisLineTriangleIntersectMacro(result, &localLine.start, &delta, v0, v1, v2,
                                               distance);

            if (result)
            {
                RpCollisTriangleNormalMacro(&collTriangle.normal, v0, v1, v2);

                collTriangle.point = *v0;
                collTriangle.index = i;
                collTriangle.vertices[0] = v0;
                collTriangle.vertices[1] = v1;
                collTriangle.vertices[2] = v2;

                if (!cbData->callBack(cbData->intersection, &collTriangle, distance,
                                      cbData->data))
                {
                    return atomic;
                }
            }
        }
    }

    return atomic;
}

static RpAtomic* AtomicForAllSphereIntersections(RpAtomic* atomic, RwSphere* sphere,
                                                 RpCollisCallBackData* cbData)
{
    RpGeometry* geometry = atomic->geometry;
    RwMatrix invMatrix;
    RpCollisionTriangle collTriangle;
    RwSphere localSphere;
    RwV3d interpV0;
    RwV3d interpV1;
    RwV3d interpV2;
    RwReal distance;
    RwReal scale;

    rwMatrixInitialize(&invMatrix, rwMATRIXTYPENORMAL);
    RwMatrixInvert(&invMatrix, RwFrameGetLTM((RwFrame*)rwObjectGetParent(atomic)));
    RwV3dTransformPoints(&localSphere.center, &sphere->center, 1, &invMatrix);

    RwV3dLengthMacro(scale, &invMatrix.at);
    localSphere.radius = scale * sphere->radius;

    if (geometry->numMorphTargets == 1)
    {
        if (!GeometryForAllSphereIntersections(geometry, &localSphere, cbData))
        {
            return (RpAtomic*)NULL;
        }
    }
    else
    {
        RwV3d* v0 = (RwV3d*)NULL;
        RwV3d* v1 = (RwV3d*)NULL;
        RwV3d* v2 = (RwV3d*)NULL;
        RwV3d* startVerts;
        RwV3d* endVerts;
        RwReal interpScale;
        RwReal recipRadius;
        RwInt32 i;
        RpTriangle* triangle;
        RwInt32 startMorph = atomic->interpolator.startMorphTarget;
        RwInt32 endMorph = atomic->interpolator.endMorphTarget;

        recipRadius = (RwReal)1 / localSphere.radius;
        interpScale = (RwReal)0;

        if (startMorph == endMorph || startMorph >= geometry->numMorphTargets ||
            endMorph >= geometry->numMorphTargets)
        {
            if (startMorph < geometry->numMorphTargets && endMorph < geometry->numMorphTargets)
            {
                startVerts = geometry->morphTarget[startMorph].verts;
            }
            else
            {
                startVerts = geometry->morphTarget[0].verts;
            }

            endVerts = startVerts;
        }
        else
        {
            startVerts = geometry->morphTarget[startMorph].verts;
            endVerts = geometry->morphTarget[endMorph].verts;
            interpScale = atomic->interpolator.recipTime * atomic->interpolator.position;

            v0 = &interpV0;
            v1 = &interpV1;
            v2 = &interpV2;
        }

        triangle = geometry->triangles;

        for (i = 0; i < geometry->numTriangles; triangle++, i++)
        {
            if (startVerts == endVerts)
            {
                v0 = &startVerts[triangle->vertIndex[0]];
                v1 = &startVerts[triangle->vertIndex[1]];
                v2 = &startVerts[triangle->vertIndex[2]];
            }
            else
            {
                RpCollisInterpolateVertexMacro(&interpV0, &startVerts[triangle->vertIndex[0]],
                                               &endVerts[triangle->vertIndex[0]], interpScale);
                RpCollisInterpolateVertexMacro(&interpV1, &startVerts[triangle->vertIndex[1]],
                                               &endVerts[triangle->vertIndex[1]], interpScale);
                RpCollisInterpolateVertexMacro(&interpV2, &startVerts[triangle->vertIndex[2]],
                                               &endVerts[triangle->vertIndex[2]], interpScale);
            }

            if (RtIntersectionSphereTriangle(&localSphere, v0, v1, v2, &collTriangle.normal,
                                             &distance))
            {
                distance *= recipRadius;

                collTriangle.point = *v0;
                collTriangle.index = i;
                collTriangle.vertices[0] = v0;
                collTriangle.vertices[1] = v1;
                collTriangle.vertices[2] = v2;

                if (!cbData->callBack(cbData->intersection, &collTriangle, distance,
                                      cbData->data))
                {
                    return atomic;
                }
            }
        }
    }

    return atomic;
}

RpAtomic* RpAtomicForAllIntersections(RpAtomic* atomic, RpIntersection* intersection,
                                      RpIntersectionCallBackGeometryTriangle callBack, void* data)
{
    RpCollisCallBackData cbData;

    cbData.callBack = callBack;
    cbData.data = data;
    cbData.intersection = intersection;

    switch (intersection->type)
    {
    case rpINTERSECTLINE:
        AtomicForAllLineIntersections(atomic, &intersection->t.line, &cbData);
        return atomic;

    case rpINTERSECTSPHERE:
        AtomicForAllSphereIntersections(atomic, &intersection->t.sphere, &cbData);
        return atomic;

    case rpINTERSECTATOMIC:
        if (intersection->t.object)
        {
            RwSphere* sphere =
                (RwSphere*)RpAtomicGetWorldBoundingSphere((RpAtomic*)intersection->t.object);

            AtomicForAllSphereIntersections(atomic, sphere, &cbData);
            return atomic;
        }

        RWERROR((E_RW_NULLP));
        return (RpAtomic*)NULL;

    default:
        RWERROR((E_RP_COLLIS_INV_INTERSECTION));
        return (RpAtomic*)NULL;
    }
}
