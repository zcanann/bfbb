#include "iMorph.h"
#include "iModel.h"

#include "rpusrdat.h"
#include <string.h>
#include <types.h>

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
    RwV3d* nold;
    S32 lockMode;
    U8 useNormals;

    s_geom = model->geometry;
    nold = NULL;
    s_tgt = s_geom->morphTarget;
    s_numV = s_geom->numVertices;
    s_alloc = NULL;
    s_nTemp = NULL;

    useNormals = normals && s_geom->object.flags & 0x10;

    RwV3d* vold = s_tgt->verts;
    lockMode = (useNormals ? 4 : 0) | 2;

    if (useNormals)
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

        if (useNormals)
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
            s_vTemp = NULL;
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

        if (useNormals && s_geom->object.flags & 0x10)
        {
            if (s_numV == 0)
            {
                s_nTemp = NULL;
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
            va[i] = (S16*)((char*)va[i] + (((s_numV * 3 + 7) * 2) & 0xFFFFFFF0));
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
        if (useNormals)
        {
            s_tgt->normals = nold;
        }
    }
    else if (dorender)
    {
        RpGeometryLock(s_geom, lockMode);
        vold = s_tgt->verts;
        s_tgt->verts = (RwV3d*)s_vTemp;
        if (useNormals)
        {
            nold = s_tgt->normals;
            s_tgt->normals = (RwV3d*)s_nTemp;
        }
        RpGeometryUnlock(s_geom);
        iModelRender(model, mat);
        s_tgt->verts = vold;
        if (useNormals)
        {
            s_tgt->normals = nold;
        }
    }
}

void iMorphOptimize(RpAtomic* model, S32 normals)
{
    S32 hasNormals = (normals != 0);
    RpGeometry* geom = model->geometry;

    if (RpGeometryGetUserDataArrayCount(geom) == 0)
    {
        S32 numElements = ((hasNormals + 1) * geom->numVertices) * sizeof(RwV3d) + 56;

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

void FastS16unpack(F32* dest, S16* v, S32 count, F32 scale)
{
    for (S32 i = 0; i < count; i++)
    {
        dest[i] = v[i] * scale;
    }
}

void FastS16weight2(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale)
{
    S32 i;
    S16* a = v_array[0];
    S16* b = v_array[1];
    F32 s0 = scale * weight[0];
    F32 s1 = scale * weight[1];

    for (i = 0; i < count; i++)
    {
        dest[i] = a[i] * s0 + b[i] * s1;
    }
}

void FastS16weight4(F32* dest, S16** v_array, S16* weight, S32 count, F32 scale)
{
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
}
