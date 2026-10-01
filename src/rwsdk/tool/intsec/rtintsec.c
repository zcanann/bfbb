#include <rwsdk/rwcore.h>
#include <rwsdk/rtintsec.h>

#define rwInvSqrtMacro(_result, _num) ((*(_result)) = _rwInvSqrt(_num))

#define RtIntsecFloatAsInt(_f) (*(const RwInt32*)&(_f))

#define RtIntsecSphereAxisTest(_axis)                                                              \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwInt32 n = (RtIntsecFloatAsInt(vc0._axis) >> 31) +                                        \
                    (RtIntsecFloatAsInt(vc1._axis) >> 31) +                                        \
                    (RtIntsecFloatAsInt(vc2._axis) >> 31);                                         \
                                                                                                   \
        if (n == 0)                                                                                \
        {                                                                                          \
            RwReal r1 = vc0._axis - sphere->radius;                                                \
            RwReal r2 = vc1._axis - sphere->radius;                                                \
            RwReal r3 = vc2._axis - sphere->radius;                                                \
                                                                                                   \
            if ((RtIntsecFloatAsInt(r1) | RtIntsecFloatAsInt(r2) | RtIntsecFloatAsInt(r3)) >= 0)  \
            {                                                                                      \
                return FALSE;                                                                      \
            }                                                                                      \
        }                                                                                          \
        else if (n == -3)                                                                          \
        {                                                                                          \
            RwReal r1 = vc0._axis + sphere->radius;                                                \
            RwReal r2 = vc1._axis + sphere->radius;                                                \
            RwReal r3 = vc2._axis + sphere->radius;                                                \
                                                                                                   \
            if ((RtIntsecFloatAsInt(r1) & RtIntsecFloatAsInt(r2) & RtIntsecFloatAsInt(r3)) < 0)   \
            {                                                                                      \
                return FALSE;                                                                      \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

RwBool RtIntersectionSphereTriangle(RwSphere* sphere, RwV3d* v0, RwV3d* v1, RwV3d* v2,
                                    RwV3d* normal, RwReal* distance)
{
    RwV3d vc0;
    RwV3d vc1;
    RwV3d vc2;
    RwV3d vAtoB;
    RwV3d vN;
    RwReal nDotN;
    RwReal distToPlane;
    RwReal sphereRadiusSquared;
    RwV3d vTmp;
    RwV3d vTmp2;
    RwReal length2;
    RwReal factor;

    /* Triangle vertices relative to the sphere centre */
    RwV3dSubMacro(&vc0, v0, &sphere->center);
    RwV3dSubMacro(&vc1, v1, &sphere->center);
    RwV3dSubMacro(&vc2, v2, &sphere->center);

    /* Trivially reject if all vertices lie beyond the sphere on one side of an axis */
    RtIntsecSphereAxisTest(x);
    RtIntsecSphereAxisTest(y);
    RtIntsecSphereAxisTest(z);

    /* Triangle plane */
    RwV3dSubMacro(&vAtoB, v1, v0);
    RwV3dSubMacro(&vN, v2, v0);
    RwV3dCrossProductMacro(normal, &vAtoB, &vN);

    nDotN = RwV3dDotProductMacro(normal, normal);
    if (nDotN <= (RwReal)0)
    {
        return FALSE;
    }

    rwInvSqrtMacro(&factor, nDotN);
    RwV3dScaleMacro(normal, normal, factor);

    distToPlane = RwV3dDotProductMacro(&vc0, normal);
    if (distToPlane < -sphere->radius || distToPlane > sphere->radius)
    {
        return FALSE;
    }

    *distance = -distToPlane;

    /* Is any vertex inside the sphere? */
    sphereRadiusSquared = sphere->radius * sphere->radius;

    vTmp.x = RwV3dDotProductMacro(&vc0, &vc0);
    if (vTmp.x <= sphereRadiusSquared)
    {
        return TRUE;
    }

    vTmp.y = RwV3dDotProductMacro(&vc1, &vc1);
    if (vTmp.y <= sphereRadiusSquared)
    {
        return TRUE;
    }

    vTmp.z = RwV3dDotProductMacro(&vc2, &vc2);
    if (vTmp.z <= sphereRadiusSquared)
    {
        return TRUE;
    }

    /* Reject if the sphere lies beyond the closest vertex */
    if (vTmp.x < vTmp.y)
    {
        if (vTmp.z < vTmp.x)
        {
            if (RwV3dDotProductMacro(&vc2, &vc0) > vTmp.z &&
                RwV3dDotProductMacro(&vc2, &vc1) > vTmp.z)
            {
                return FALSE;
            }
        }
        else
        {
            if (RwV3dDotProductMacro(&vc0, &vc1) > vTmp.x &&
                RwV3dDotProductMacro(&vc0, &vc2) > vTmp.x)
            {
                return FALSE;
            }
        }
    }
    else
    {
        if (vTmp.z < vTmp.y)
        {
            if (RwV3dDotProductMacro(&vc2, &vc0) > vTmp.z &&
                RwV3dDotProductMacro(&vc2, &vc1) > vTmp.z)
            {
                return FALSE;
            }
        }
        else
        {
            if (RwV3dDotProductMacro(&vc1, &vc0) > vTmp.y &&
                RwV3dDotProductMacro(&vc1, &vc2) > vTmp.y)
            {
                return FALSE;
            }
        }
    }

    /* Reject if the sphere lies beyond any edge */
    RwV3dSubMacro(&vTmp2, &vc1, &vc0);
    factor = RwV3dDotProductMacro(&vTmp2, &vc0) / RwV3dDotProductMacro(&vTmp2, &vTmp2);
    RwV3dScaleMacro(&vTmp, &vTmp2, factor);
    RwV3dSubMacro(&vTmp, &vc0, &vTmp);
    length2 = RwV3dDotProductMacro(&vTmp, &vTmp);
    if (length2 > sphereRadiusSquared && length2 < RwV3dDotProductMacro(&vTmp, &vc2))
    {
        return FALSE;
    }

    RwV3dSubMacro(&vTmp2, &vc2, &vc1);
    factor = RwV3dDotProductMacro(&vTmp2, &vc1) / RwV3dDotProductMacro(&vTmp2, &vTmp2);
    RwV3dScaleMacro(&vTmp, &vTmp2, factor);
    RwV3dSubMacro(&vTmp, &vc1, &vTmp);
    length2 = RwV3dDotProductMacro(&vTmp, &vTmp);
    if (length2 > sphereRadiusSquared && length2 < RwV3dDotProductMacro(&vTmp, &vc0))
    {
        return FALSE;
    }

    RwV3dSubMacro(&vTmp2, &vc0, &vc2);
    factor = RwV3dDotProductMacro(&vTmp2, &vc2) / RwV3dDotProductMacro(&vTmp2, &vTmp2);
    RwV3dScaleMacro(&vTmp, &vTmp2, factor);
    RwV3dSubMacro(&vTmp, &vc2, &vTmp);
    length2 = RwV3dDotProductMacro(&vTmp, &vTmp);
    if (length2 > sphereRadiusSquared && length2 < RwV3dDotProductMacro(&vTmp, &vc1))
    {
        return FALSE;
    }

    return TRUE;
}

#define RtIntsecBBoxOutcode(_v)                                                                    \
    ((((_v)->x > bbox->sup.x) ? 1 : (((_v)->x < bbox->inf.x) ? 2 : 0)) |                          \
     (((_v)->y > bbox->sup.y) ? 4 : (((_v)->y < bbox->inf.y) ? 8 : 0)) |                          \
     (((_v)->z > bbox->sup.z) ? 16 : (((_v)->z < bbox->inf.z) ? 32 : 0)))

#define RtIntsecBBoxFaceTest(_mask, _face, _a, _b, _c)                                             \
    MACRO_START                                                                                    \
    {                                                                                              \
        if ((_mask))                                                                               \
        {                                                                                          \
            RwReal tmp = (_face)._a * del._b;                                                      \
                                                                                                   \
            if (del._a < (RwReal)0)                                                                \
            {                                                                                      \
                if (tmp < hi._b * del._a && tmp > lo._b * del._a)                                  \
                {                                                                                  \
                    tmp = (_face)._a * del._c;                                                     \
                    if (tmp < hi._c * del._a && tmp > lo._c * del._a)                              \
                    {                                                                              \
                        return TRUE;                                                               \
                    }                                                                              \
                }                                                                                  \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                if (tmp > hi._b * del._a && tmp < lo._b * del._a)                                  \
                {                                                                                  \
                    tmp = (_face)._a * del._c;                                                     \
                    if (tmp > hi._c * del._a && tmp < lo._c * del._a)                              \
                    {                                                                              \
                        return TRUE;                                                               \
                    }                                                                              \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

#define RtIntsecBBoxEdgeTest(_vA, _vB, _testA, _testB)                                             \
    MACRO_START                                                                                    \
    {                                                                                              \
        if (!((_testA) & (_testB)))                                                                \
        {                                                                                          \
            RwV3d del;                                                                             \
            RwV3d hi;                                                                              \
            RwV3d lo;                                                                              \
                                                                                                   \
            RwV3dSubMacro(&del, (_vB), (_vA));                                                     \
            RwV3dSubMacro(&lo, (_vA), &bbox->inf);                                                 \
            RwV3dSubMacro(&hi, (_vA), &bbox->sup);                                                 \
                                                                                                   \
            RtIntsecBBoxFaceTest(((_testA) ^ (_testB)) & 1, hi, x, y, z);                          \
            RtIntsecBBoxFaceTest(((_testA) ^ (_testB)) & 2, lo, x, y, z);                          \
            RtIntsecBBoxFaceTest(((_testA) ^ (_testB)) & 4, hi, y, z, x);                          \
            RtIntsecBBoxFaceTest(((_testA) ^ (_testB)) & 8, lo, y, z, x);                          \
            RtIntsecBBoxFaceTest(((_testA) ^ (_testB)) & 16, hi, z, x, y);                         \
            RtIntsecBBoxFaceTest(((_testA) ^ (_testB)) & 32, lo, z, x, y);                         \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

RwBool RtIntersectionBBoxTriangle(RwBBox* bbox, RwV3d* v0, RwV3d* v1, RwV3d* v2)
{
    RwInt32 v0Test;
    RwInt32 v1Test;
    RwInt32 v2Test;

    /* Trivially accept if any vertex lies inside the box */
    v0Test = RtIntsecBBoxOutcode(v0);
    if (!v0Test)
    {
        return TRUE;
    }

    v1Test = RtIntsecBBoxOutcode(v1);
    if (!v1Test)
    {
        return TRUE;
    }

    v2Test = RtIntsecBBoxOutcode(v2);
    if (!v2Test)
    {
        return TRUE;
    }

    /* Trivially reject if all vertices lie outside the same box face */
    if (v0Test & v1Test & v2Test)
    {
        return FALSE;
    }

    /* Does any triangle edge pass through a box face? */
    RtIntsecBBoxEdgeTest(v0, v1, v0Test, v1Test);
    RtIntsecBBoxEdgeTest(v0, v2, v0Test, v2Test);
    RtIntsecBBoxEdgeTest(v1, v2, v1Test, v2Test);

    /* Does the box diagonal best aligned with the triangle normal pierce the triangle? */
    {
        RwV3d v01;
        RwV3d v02;
        RwV3d norm;
        RwV3d diag;
        RwV3d tVec;
        RwReal det;
        RwReal dist;

        RwV3dSubMacro(&v01, v1, v0);
        RwV3dSubMacro(&v02, v2, v0);
        RwV3dCrossProductMacro(&norm, &v01, &v02);

        diag.x = bbox->sup.x - bbox->inf.x;
        tVec.x = bbox->inf.x - v0->x;

        if ((RtIntsecFloatAsInt(norm.x) ^ RtIntsecFloatAsInt(norm.y)) < 0)
        {
            diag.y = bbox->inf.y - bbox->sup.y;
            tVec.y = bbox->sup.y - v0->y;
        }
        else
        {
            diag.y = bbox->sup.y - bbox->inf.y;
            tVec.y = bbox->inf.y - v0->y;
        }

        if ((RtIntsecFloatAsInt(norm.x) ^ RtIntsecFloatAsInt(norm.z)) < 0)
        {
            diag.z = bbox->inf.z - bbox->sup.z;
            tVec.z = bbox->sup.z - v0->z;
        }
        else
        {
            diag.z = bbox->sup.z - bbox->inf.z;
            tVec.z = bbox->inf.z - v0->z;
        }

        det = -RwV3dDotProductMacro(&diag, &norm);
        dist = RwV3dDotProductMacro(&tVec, &norm);

        if (det < (RwReal)0)
        {
            if ((RwReal)0 > dist && dist > det)
            {
                RwReal u;
                RwReal v;
                RwV3d wVec;

                RwV3dCrossProductMacro(&wVec, &tVec, &diag);

                u = RwV3dDotProductMacro(&v02, &wVec);
                if ((RwReal)0 > u && u > det)
                {
                    v = -RwV3dDotProductMacro(&v01, &wVec);
                    if ((RwReal)0 > v && u + v > det)
                    {
                        return TRUE;
                    }
                }
            }
        }
        else
        {
            if ((RwReal)0 < dist && dist < det)
            {
                RwReal u;
                RwReal v;
                RwV3d wVec;

                RwV3dCrossProductMacro(&wVec, &tVec, &diag);

                u = RwV3dDotProductMacro(&v02, &wVec);
                if ((RwReal)0 < u && u < det)
                {
                    v = -RwV3dDotProductMacro(&v01, &wVec);
                    if ((RwReal)0 < v && u + v < det)
                    {
                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;
}
