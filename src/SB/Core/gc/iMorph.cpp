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
// The original hand-scheduled kernels consume padded groups of eight shorts.
// Preserve their pipeline delay slots and bounded partial-block stores. Weighted
// variants add signed integer products before converting to float in VU0;
// scalar floating-point blending does not reproduce that arithmetic.
#pragma dont_inline on
#endif

#if defined(PS2)
asm
#endif
void FastS16unpack(F32* dest, S16* v, S32 count, F32 scale)
{
#if defined(PS2)
    lui v1, 0x3780
    mtc1 v1, f0
    nop
    mul.s f0, f0, f12
    addiu t6, a2, -0x1
    srl t6, t6, 3
    sll t6, t6, 5
    addu t6, t6, a0
    mfc1 v1, f0
    b load_block
    qmtc2 v1, vf1
    nop
store_block:
    sqc2 vf2, 0x0(a0)
    sqc2 vf3, 0x10(a0)
    addiu a0, a0, 0x20
    nop
load_block:
    lq a4, 0x0(a1)
    addiu a1, a1, 0x10
    pextlh t4, a4, zero
    pextuh t5, a4, zero
    qmtc2 t4, vf2
    qmtc2 t5, vf3
    vitof0.xyzw vf2, vf2
    vmulx.xyzw vf2, vf2, vf1x
    vitof0.xyzw vf3, vf3
    bne a0, t6, store_block
    vmulx.xyzw vf3, vf3, vf1x
    nop
    andi a5, a2, 0x7
    beqz a5, store_full
    nop
    andi a5, a2, 0x4
    beqz a5, lower_tail
    nop
    nop
    sqc2 vf2, 0x0(a0)
    qmfc2 a4, vf3
    j tail_two
    addiu a0, a0, 0x10
    nop
lower_tail:
    qmfc2 a4, vf2
    nop
tail_two:
    andi a5, a2, 0x2
    beqz a5, tail_one
    nop
    nop
    sd a4, 0x0(a0)
    addiu a0, a0, 0x8
    pcpyud a4, a4, zero
    nop
tail_one:
    andi a5, a2, 0x1
    beqz a5, done
    nop
    nop
    b done
    sw a4, 0x0(a0)
store_full:
    sqc2 vf2, 0x0(a0)
    sqc2 vf3, 0x10(a0)
done:
    jr ra
    nop
#else
    for (S32 i = 0; i < count; i++)
    {
        dest[i] = v[i] * scale;
    }
#endif
}

#if defined(PS2)
asm
#endif
void FastS16weight2(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale)
{
#if defined(PS2)
    lh a4, 0x0(a2)
    lh a5, 0x2(a2)
    pcpyh a4, a4
    pcpyh a5, a5
    pextlh v0, a4, a5
    addiu t6, a3, -0x1
    srl t6, t6, 3
    sll t6, t6, 5
    addu t6, t6, a0
    mfc1 v1, f12
    nop
    qmtc2 v1, vf1
    lwu v1, 0x0(a1)
    b load_block
    lwu a1, 0x4(a1)
    nop
store_block:
    sqc2 vf2, 0x0(a0)
    sqc2 vf3, 0x10(a0)
    addiu a0, a0, 0x20
    nop
load_block:
    lq a4, 0x0(v1)
    lq a5, 0x0(a1)
    addiu v1, v1, 0x10
    pextlh t4, a4, a5
    phmadh t4, t4, v0
    pextuh t5, a4, a5
    phmadh t5, t5, v0
    addiu a1, a1, 0x10
    qmtc2 t4, vf2
    qmtc2 t5, vf3
    vitof0.xyzw vf2, vf2
    vmulx.xyzw vf2, vf2, vf1x
    vitof0.xyzw vf3, vf3
    bne a0, t6, store_block
    vmulx.xyzw vf3, vf3, vf1x
    nop
    andi a5, a3, 0x7
    beqz a5, store_full
    nop
    andi a5, a3, 0x4
    beqz a5, lower_tail
    nop
    nop
    sqc2 vf2, 0x0(a0)
    qmfc2 a4, vf3
    j tail_two
    addiu a0, a0, 0x10
    nop
lower_tail:
    qmfc2 a4, vf2
    nop
tail_two:
    andi a5, a3, 0x2
    beqz a5, tail_one
    nop
    nop
    sd a4, 0x0(a0)
    addiu a0, a0, 0x8
    pcpyud a4, a4, zero
    nop
tail_one:
    andi a5, a3, 0x1
    beqz a5, done
    nop
    nop
    b done
    sw a4, 0x0(a0)
store_full:
    sqc2 vf2, 0x0(a0)
    sqc2 vf3, 0x10(a0)
done:
    jr ra
    nop
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

#if defined(PS2)
asm
#endif
void FastS16weight4(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale)
{
#if defined(PS2)
    lh a4, 0x0(a2)
    lh a5, 0x2(a2)
    lh a6, 0x4(a2)
    lh a7, 0x6(a2)
    pcpyh a4, a4
    pcpyh a5, a5
    pcpyh a6, a6
    pcpyh a7, a7
    pextlh v0, a4, a5
    pextlh v1, a6, a7
    addiu t6, a3, -0x1
    srl t6, t6, 3
    sll t6, t6, 5
    addu t6, t6, a0
    mfc1 a2, f12
    nop
    qmtc2 a2, vf1
    lwu a2, 0x0(a1)
    lwu t7, 0x8(a1)
    lwu t8, 0xc(a1)
    b load_block
    lwu a1, 0x4(a1)
store_block:
    sqc2 vf2, 0x0(a0)
    sqc2 vf3, 0x10(a0)
    addiu a0, a0, 0x20
    nop
load_block:
    lq a4, 0x0(a2)
    lq a5, 0x0(a1)
    lq a6, 0x0(t7)
    lq a7, 0x0(t8)
    addiu a2, a2, 0x10
    pextlh t4, a4, a5
    phmadh t4, t4, v0
    pextuh t5, a4, a5
    phmadh t5, t5, v0
    pextlh a4, a6, a7
    phmadh a4, a4, v1
    pextuh a5, a6, a7
    phmadh a5, a5, v1
    addiu a1, a1, 0x10
    addiu t7, t7, 0x10
    paddw t4, t4, a4
    paddw t5, t5, a5
    qmtc2 t4, vf2
    qmtc2 t5, vf3
    addiu t8, t8, 0x10
    vitof0.xyzw vf2, vf2
    vmulx.xyzw vf2, vf2, vf1x
    vitof0.xyzw vf3, vf3
    bne a0, t6, store_block
    vmulx.xyzw vf3, vf3, vf1x
    nop
    andi a5, a3, 0x7
    beqz a5, store_full
    nop
    andi a5, a3, 0x4
    beqz a5, lower_tail
    nop
    nop
    sqc2 vf2, 0x0(a0)
    qmfc2 a4, vf3
    j tail_two
    addiu a0, a0, 0x10
    nop
lower_tail:
    qmfc2 a4, vf2
    nop
tail_two:
    andi a5, a3, 0x2
    beqz a5, tail_one
    nop
    nop
    sd a4, 0x0(a0)
    addiu a0, a0, 0x8
    pcpyud a4, a4, zero
    nop
tail_one:
    andi a5, a3, 0x1
    beqz a5, done
    nop
    nop
    b done
    sw a4, 0x0(a0)
store_full:
    sqc2 vf2, 0x0(a0)
    sqc2 vf3, 0x10(a0)
done:
    jr ra
    nop
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
