#include "iMorph.h"
#include "iModel.h"

#include "rpusrdat.h"
#include <string.h>
#include <types.h>

#if defined(PS2)
extern "C" RwInt32 RpGeometryAddUserDataArray(RpGeometry* geometry, RwChar* name,
                                            RpUserDataFormat format, RwInt32 numElements);
#endif

static RpGeometry* s_geom;
static RpMorphTarget* s_tgt;
static F32* s_alloc;
static F32* s_vTemp;
static F32* s_nTemp;
static U32 s_numV;

static void MorphCommon(RpAtomic* model, RwMatrixTag* mat, S16** v_array, S16* weight, U32 normals,
                        F32 scale, S32 dorender)
{
    U32 i;
    U32 a;
    S16 wa[4];
    S16* va[4];
    S32 wsum;
    RwV3d* vold;
    RwV3d* nold;
    S32 lockMode;

    s_geom = model->geometry;
    nold = NULL;
    s_tgt = s_geom->morphTarget;
    s_numV = s_geom->numVertices;
    s_alloc = NULL;
    s_nTemp = NULL;

    normals = normals && s_geom->object.flags & 0x10;

    vold = s_tgt->verts;

    lockMode = (normals ? 4 : 0) | 2;

    if (normals)
    {
        nold = s_tgt->normals;
    }

    a = 0;
    wsum = 0;

    for (i = 0; i < 4; i++)
    {
        if (v_array[i] != NULL && weight[i] != 0)
        {
            va[a] = v_array[i];
            wa[a] = weight[i];
            wsum += weight[i];
            a++;
        }
    }

    RpUserDataArray* usr = RpGeometryGetUserDataArray(s_geom, 0);

    if (usr != NULL)
    {
        DirtyMorph* dm = (DirtyMorph*)usr->data;

        s_vTemp = (F32*)((char*)dm + 32);

        while ((U32)s_vTemp & 0xF)
        {
            s_vTemp++;
        }

        s_tgt->verts = (RwV3d*)s_vTemp;

        if (normals)
        {
            s_nTemp = (F32*)((char*)s_vTemp + s_numV * sizeof(RwV3d));

            while ((U32)s_nTemp & 0xF)
            {
                s_nTemp++;
            }

            s_tgt->normals = (RwV3d*)s_nTemp;
        }

        if (dm->count == a && dm->weight[0] == wa[0] && dm->v_array[0] == va[0] &&
            dm->scale == scale)
        {
            for (i = 1; i < a; ++i)
            {
                if (dm->weight[i] != wa[i] || va[i] != dm->v_array[i])
                {
                    break;
                }
            }

            if (a == i)
            {
                if (dorender)
                    iModelRender(model, mat);

                s_tgt->verts = vold;
                if (nold)
                    s_tgt->normals = nold;

                return;
            }
        }

        dm->count = a;
        dm->scale = scale;

        for (i = 0; i < a; ++i)
        {
            dm->weight[i] = wa[i];
            dm->v_array[i] = v_array[i];
        }

        RpGeometryLock(s_geom, lockMode);
    }

    if (a == 3)
    {
        va[3] = va[2];
        wa[3] = 0;
    }
    if (usr == NULL)
    {
        if (s_numV == 0)
        {
#if defined(PS2)
            s_vTemp = (F32*)0x70000000;
#else
            s_vTemp = NULL;
#endif
        }
        else
        {
            s_vTemp = (F32*)xMemPushTemp(s_numV * 3 * sizeof(F32) + 16);

            s_alloc = s_vTemp;

            while ((U32)s_vTemp & 0xF)
            {
                s_vTemp++;
            }
        }

        if (normals && s_geom->object.flags & 0x10)
        {
            if (s_numV == 0)
            {
#if defined(PS2)
                s_nTemp = (F32*)0x70000000;
#else
                s_nTemp = NULL;
#endif
            }
            else
            {
                s_nTemp = (F32*)xMemPushTemp(s_numV * 3 * sizeof(F32) + 16);

                if (s_alloc == 0)
                {
                    s_alloc = s_nTemp;
                }

                while ((U32)s_nTemp & 0xF)
                {
                    s_nTemp++;
                }
            }
        }
    }

    if (a == 1)
    {
        FastS16unpack(s_vTemp, va[0], s_numV * 3, scale * wsum);
    }
    else if (a == 2)
    {
        FastS16weight2(s_vTemp, va, wa, s_numV * 3, scale);
    }
    else
    {
        FastS16weight4(s_vTemp, va, wa, s_numV * 3, scale);
    }

    if (s_nTemp != NULL)
    {
        scale = 1.0f / (wsum * 16384.0f);

        for (i = 0; i < a; ++i)
        {
#if defined(PS2)
            va[i] += (s_numV * 3 + 7) & ~7;
#else
            va[i] = (S16*)((char*)va[i] + (((s_numV * 3 + 7) * 2) & 0xFFFFFFF0));
#endif
        }

        if (a == 1)
        {
            FastS16unpack(s_nTemp, va[0], s_numV * 3, scale * wsum);
        }
        else if (a == 2)
        {
            FastS16weight2(s_nTemp, va, wa, s_numV * 3, scale);
        }
        else
        {
            FastS16weight4(s_nTemp, va, wa, s_numV * 3, scale);
        }
    }

    if (usr != NULL)
    {
        RpGeometryUnlock(s_geom);
        if (dorender)
        {
            iModelRender(model, mat);
        }
        s_tgt->verts = vold;
        if (normals)
        {
            s_tgt->normals = nold;
        }
    }
    else if (dorender)
    {
        RpGeometryLock(s_geom, lockMode);
        vold = s_tgt->verts;
        s_tgt->verts = (RwV3d*)s_vTemp;
        if (normals)
        {
            nold = s_tgt->normals;
            s_tgt->normals = (RwV3d*)s_nTemp;
        }
        RpGeometryUnlock(s_geom);
        iModelRender(model, mat);
        s_tgt->verts = vold;
        if (normals)
        {
            s_tgt->normals = nold;
        }
    }
}

void iMorphOptimize(RpAtomic* model, S32 normals)
{
#if defined(PS2)
    normals = (normals != 0);
#else
    S32 hasNormals = (normals != 0);
#endif
    RpGeometry* geom = model->geometry;

    if (RpGeometryGetUserDataArrayCount(geom) == 0)
    {
#if defined(PS2)
        S32 numElements = ((normals + 1) * geom->numVertices) * sizeof(RwV3d) + 56;
#else
        S32 numElements = ((hasNormals + 1) * geom->numVertices) * sizeof(RwV3d) + 56;
#endif

        S32 usridx = RpGeometryAddUserDataArray(geom, "MORPHSTATE", rpINTUSERDATA, numElements);
        RpUserDataArray* usr = RpGeometryGetUserDataArray(geom, usridx);

        memset(usr->data, 0, 0x20);
    }
}

void iMorphRender(RpAtomic* model, RwMatrix* mat, S16** v_array, S16* weight, U32 normals,
                  F32 scale)

{
    MorphCommon(model, mat, v_array, weight, normals, scale, 1);
    if (s_alloc != NULL)
    {
        xMemPopTemp(s_alloc);
        s_alloc = NULL;
    }
    return;
}

#if defined(PS2)
// These kernels consume padded groups of eight signed shorts. The weighted
// variants add signed integer products before converting to float in VU0;
// scalar floating-point blending does not reproduce that arithmetic.
#pragma dont_inline on
#endif

void FastS16unpack(F32* dest, S16* v, S32 count, F32 scale)
{
#if defined(PS2)
    F32* last = dest + (((U32)(count - 1) >> 3) * 8);
    scale *= 1.0f / 65536.0f;
    U32 bits;
    asm volatile("mfc1 %0, %1\n"
                 "qmtc2 %0, vf1\n"
                 : "=r"(bits) : "f"(scale));

    for (;;)
    {
        asm volatile("lq a4, 0x0(%0)\n"
                     "pextlh t4, a4, zero\n"
                     "pextuh t5, a4, zero\n"
                     "qmtc2 t4, vf2\n"
                     "qmtc2 t5, vf3\n"
                     "vitof0.xyzw vf2, vf2\n"
                     "vmulx.xyzw vf2, vf2, vf1x\n"
                     "vitof0.xyzw vf3, vf3\n"
                     "vmulx.xyzw vf3, vf3, vf1x\n"
                     : : "r"(v) : "a4", "t4", "t5", "memory");
        v += 8;
        if (dest == last)
            break;
        asm volatile("sqc2 vf2, 0x0(%0)\n"
                     "sqc2 vf3, 0x10(%0)\n" : : "r"(dest) : "memory");
        dest += 8;
    }

    if (count & 7)
    {
        unsigned __int128 tail;
        if (count & 4)
        {
            asm volatile("sqc2 vf2, 0x0(%1)\n"
                         "qmfc2 %0, vf3\n" : "=r"(tail) : "r"(dest) : "memory");
            dest += 4;
        }
        else
        {
            asm volatile("qmfc2 %0, vf2\n" : "=r"(tail));
        }
        if (count & 2)
        {
            asm volatile("sd %0, 0x0(%1)\n"
                         "pcpyud %0, %0, zero\n" : "+r"(tail) : "r"(dest) : "memory");
            dest += 2;
        }
        if (count & 1)
        {
            asm volatile("sw %0, 0x0(%1)\n" : : "r"(tail), "r"(dest) : "memory");
        }
    }
    else
    {
        asm volatile("sqc2 vf2, 0x0(%0)\n"
                     "sqc2 vf3, 0x10(%0)\n" : : "r"(dest) : "memory");
    }
#else
    for (S32 i = 0; i < count; i++)
    {
        dest[i] = v[i] * scale;
    }
#endif
}

void FastS16weight2(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale)
{
#if defined(PS2)
    unsigned __int128 weights01;
    asm volatile("lh a4, 0x0(%1)\n"
                 "lh a5, 0x2(%1)\n"
                 "pcpyh a4, a4\n"
                 "pcpyh a5, a5\n"
                 "pextlh %0, a4, a5\n"
                 : "=r"(weights01) : "r"(weight) : "a4", "a5");
    F32* last = dest + (((U32)(count - 1) >> 3) * 8);
    U32 bits;
    asm volatile("mfc1 %0, %1\n"
                 "qmtc2 %0, vf1\n" : "=r"(bits) : "f"(scale));
    S16* v0 = v_array[0];
    S16* v1 = v_array[1];
    for (;;)
    {
        asm volatile("lq a4, 0x0(%0)\n"
                     "lq a5, 0x0(%1)\n"
                     "pextlh t4, a4, a5\n"
                     "phmadh t4, t4, %2\n"
                     "pextuh t5, a4, a5\n"
                     "phmadh t5, t5, %2\n"
                     "qmtc2 t4, vf2\n"
                     "qmtc2 t5, vf3\n"
                     "vitof0.xyzw vf2, vf2\n"
                     "vmulx.xyzw vf2, vf2, vf1x\n"
                     "vitof0.xyzw vf3, vf3\n"
                     "vmulx.xyzw vf3, vf3, vf1x\n"
                     : : "r"(v0), "r"(v1), "r"(weights01)
                     : "a4", "a5", "t4", "t5", "memory");
        v0 += 8;
        v1 += 8;
        if (dest == last)
            break;
        asm volatile("sqc2 vf2, 0x0(%0)\n"
                     "sqc2 vf3, 0x10(%0)\n" : : "r"(dest) : "memory");
        dest += 8;
    }

    if (count & 7)
    {
        unsigned __int128 tail;
        if (count & 4)
        {
            asm volatile("sqc2 vf2, 0x0(%1)\n"
                         "qmfc2 %0, vf3\n" : "=r"(tail) : "r"(dest) : "memory");
            dest += 4;
        }
        else
        {
            asm volatile("qmfc2 %0, vf2\n" : "=r"(tail));
        }
        if (count & 2)
        {
            asm volatile("sd %0, 0x0(%1)\n"
                         "pcpyud %0, %0, zero\n" : "+r"(tail) : "r"(dest) : "memory");
            dest += 2;
        }
        if (count & 1)
        {
            asm volatile("sw %0, 0x0(%1)\n" : : "r"(tail), "r"(dest) : "memory");
        }
    }
    else
    {
        asm volatile("sqc2 vf2, 0x0(%0)\n"
                     "sqc2 vf3, 0x10(%0)\n" : : "r"(dest) : "memory");
    }
#else
    S32 i;
    S16* a = v_array[0];
    S16* b = v_array[1];
    F32 s0 = scale * weight[0];
    F32 s1 = scale * weight[1];

    for (i = 0; i < count; i++)
    {
        dest[i] = a[i] * s0 + b[i] * s1;
    }
#endif
}

void FastS16weight4(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale)
{
#if defined(PS2)
    unsigned __int128 weights01, weights23;
    asm volatile("lh a4, 0x0(%2)\n"
                 "lh a5, 0x2(%2)\n"
                 "lh a6, 0x4(%2)\n"
                 "lh a7, 0x6(%2)\n"
                 "pcpyh a4, a4\n"
                 "pcpyh a5, a5\n"
                 "pcpyh a6, a6\n"
                 "pcpyh a7, a7\n"
                 "pextlh %0, a4, a5\n"
                 "pextlh %1, a6, a7\n"
                 : "=r"(weights01), "=r"(weights23) : "r"(weight) : "a4", "a5", "a6", "a7");
    F32* last = dest + (((U32)(count - 1) >> 3) * 8);
    U32 bits;
    asm volatile("mfc1 %0, %1\n"
                 "qmtc2 %0, vf1\n" : "=r"(bits) : "f"(scale));
    S16* v0 = v_array[0];
    S16* v1 = v_array[1];
    S16* v2 = v_array[2];
    S16* v3 = v_array[3];
    for (;;)
    {
        asm volatile("lq a4, 0x0(%0)\n"
                     "lq a5, 0x0(%1)\n"
                     "lq a6, 0x0(%2)\n"
                     "lq a7, 0x0(%3)\n"
                     "pextlh t4, a4, a5\n"
                     "phmadh t4, t4, %4\n"
                     "pextuh t5, a4, a5\n"
                     "phmadh t5, t5, %4\n"
                     "pextlh a4, a6, a7\n"
                     "phmadh a4, a4, %5\n"
                     "pextuh a5, a6, a7\n"
                     "phmadh a5, a5, %5\n"
                     "paddw t4, t4, a4\n"
                     "paddw t5, t5, a5\n"
                     "qmtc2 t4, vf2\n"
                     "qmtc2 t5, vf3\n"
                     "vitof0.xyzw vf2, vf2\n"
                     "vmulx.xyzw vf2, vf2, vf1x\n"
                     "vitof0.xyzw vf3, vf3\n"
                     "vmulx.xyzw vf3, vf3, vf1x\n"
                     : : "r"(v0), "r"(v1), "r"(v2), "r"(v3), "r"(weights01), "r"(weights23)
                     : "a4", "a5", "a6", "a7", "t4", "t5", "memory");
        v0 += 8;
        v1 += 8;
        v2 += 8;
        v3 += 8;
        if (dest == last)
            break;
        asm volatile("sqc2 vf2, 0x0(%0)\n"
                     "sqc2 vf3, 0x10(%0)\n" : : "r"(dest) : "memory");
        dest += 8;
    }

    if (count & 7)
    {
        unsigned __int128 tail;
        if (count & 4)
        {
            asm volatile("sqc2 vf2, 0x0(%1)\n"
                         "qmfc2 %0, vf3\n" : "=r"(tail) : "r"(dest) : "memory");
            dest += 4;
        }
        else
        {
            asm volatile("qmfc2 %0, vf2\n" : "=r"(tail));
        }
        if (count & 2)
        {
            asm volatile("sd %0, 0x0(%1)\n"
                         "pcpyud %0, %0, zero\n" : "+r"(tail) : "r"(dest) : "memory");
            dest += 2;
        }
        if (count & 1)
        {
            asm volatile("sw %0, 0x0(%1)\n" : : "r"(tail), "r"(dest) : "memory");
        }
    }
    else
    {
        asm volatile("sqc2 vf2, 0x0(%0)\n"
                     "sqc2 vf3, 0x10(%0)\n" : : "r"(dest) : "memory");
    }
#else
    S32 i;
    S16* a0 = v_array[0];
    S16* a1 = v_array[1];
    S16* a2 = v_array[2];
    S16* a3 = v_array[3];
    F32 s0 = scale * weight[0];
    F32 s1 = scale * weight[1];
    F32 s2 = scale * weight[2];
    F32 s3 = scale * weight[3];

    for (i = 0; i < count; ++i)
    {
        dest[i] = a0[i] * s0 + a1[i] * s1 + a2[i] * s2 + a3[i] * s3;
    }
#endif
}

#if defined(PS2)
#pragma dont_inline reset
#endif
