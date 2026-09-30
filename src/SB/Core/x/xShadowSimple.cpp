#include "rwcore.h"
#include "rwplcore.h"
#include "xString.h"
#include "xVec3.h"
#include "xstransvc.h"
#include <string.h>
#include <types.h>

#include "xShadowSimple.h"

#include "rpcollis.h"
#include "xMath3.h"
#include "xQuickCull.h"
#include "zGlobals.h"
#include "zGrid.h"
#include "zBase.h"
#include "zNPCTypeTiki.h"
#include "iModel.h"
#include "xVec3Inlines.h"

struct ShadowCBParam
{
    xShadowSimpleCache* cache;
    RpIntersection* isx;
};

static xShadowSimpleQueue sCollQueue[2];
static RxRenderStateVector xrsv;
static RxObjSpace3DVertex sShadVert[384];
static RwRaster* sShadRasters[64];
static RwRaster* sShadRaster;
static u32 sShadVertCount;
static RwMatrixTag* sModelMat;

inline void xQuickCullForLine(xQCData* q, const xLine3* ln)
{
    xQuickCullForLine(&xqc_def_ctrl, q, ln);
}

static RpCollisionTriangle* shadowRayCB(RpIntersection*, RpWorldSector*, RpCollisionTriangle* tri,
                                        F32 dist, void* data)
{
    xVec3 xformnorm;
    xVec3* norm = NULL;
    F32* testdist;
    xShadowSimpleCache* cache = (xShadowSimpleCache*)data;

    if (sModelMat)
    {
        testdist = &cache->shadowHeight;
        xMat3x3RMulVec(&xformnorm, (xMat3x3*)sModelMat, (xVec3*)&tri->normal);
        xVec3Normalize(&xformnorm, &xformnorm);
        norm = &xformnorm;
    }
    else
    {
        testdist = &cache->envHeight;
        norm = (xVec3*)&tri->normal;
    }

    if (dist >= *testdist || (norm->y < 0.0871557f))
    {
        return tri;
    }

    *testdist = dist;

    cache->poly.vert[0] = *((xVec3*)tri->vertices[0]);
    cache->poly.vert[1] = *((xVec3*)tri->vertices[1]);
    cache->poly.vert[2] = *((xVec3*)tri->vertices[2]);
    cache->poly.norm = *((xVec3*)&tri->normal);

    return tri;
}

static RpCollisionTriangle* shadowRayModelCB(RpIntersection* isx, RpCollisionTriangle* tri,
                                             F32 dist, void* data)
{
    return shadowRayCB(isx, NULL, tri, dist, data);
}

static S32 shadowRayEntCB(xEnt* ent, void* cbdata)
{
    ShadowCBParam* cbparam = (ShadowCBParam*)cbdata;
    zNPCTiki* tiki;
    F32 endY;
    F32 dist;
    xModelInstance* m;
    F32 oldHeight;

    if (!(ent->baseFlags & 0x10))
    {
        return 1;
    }

    tiki = (zNPCTiki*)ent;

    if ((cbparam->cache->flags & 8) && ent->baseType == eBaseTypeNPC &&
        (tiki->SelfType() & 0xffffff00) == 'NTT\0')
    {
        xVec3 pt;
        xVec3 corner;

        xVec3Init(&pt, cbparam->isx->t.line.start.x, cbparam->isx->t.line.start.y,
                  cbparam->isx->t.line.start.z);

        endY = cbparam->isx->t.line.end.y;
        dist = (ent->bound.box.box.upper.y - pt.y) / (endY - pt.y);

        if (cbparam->cache->shadowHeight > dist && ent->bound.box.box.upper.y > endY &&
            pt.x > ent->bound.box.box.lower.x && pt.x < ent->bound.box.box.upper.x &&
            pt.z > ent->bound.box.box.lower.z && pt.z < ent->bound.box.box.upper.z)
        {
            cbparam->cache->shadowHeight = dist;

            corner.y = ent->bound.box.box.upper.y;
            corner.x = 0.25f + pt.x;
            corner.z = pt.z;
            xVec3SubFrom(&corner, (xVec3*)&ent->model->Mat->pos);
            cbparam->cache->poly.vert[0].x =
                xVec3Dot(&corner, (xVec3*)&ent->model->Mat->right);
            cbparam->cache->poly.vert[0].y = xVec3Dot(&corner, (xVec3*)&ent->model->Mat->up);
            cbparam->cache->poly.vert[0].z = xVec3Dot(&corner, (xVec3*)&ent->model->Mat->at);

            corner.y = ent->bound.box.box.upper.y;
            corner.x = pt.x - 0.25f;
            corner.z = 0.25f + pt.z;
            xVec3SubFrom(&corner, (xVec3*)&ent->model->Mat->pos);
            cbparam->cache->poly.vert[1].x =
                xVec3Dot(&corner, (xVec3*)&ent->model->Mat->right);
            cbparam->cache->poly.vert[1].y = xVec3Dot(&corner, (xVec3*)&ent->model->Mat->up);
            cbparam->cache->poly.vert[1].z = xVec3Dot(&corner, (xVec3*)&ent->model->Mat->at);

            corner.y = ent->bound.box.box.upper.y;
            corner.x = pt.x - 0.25f;
            corner.z = pt.z - 0.25f;
            xVec3SubFrom(&corner, (xVec3*)&ent->model->Mat->pos);
            cbparam->cache->poly.vert[2].x =
                xVec3Dot(&corner, (xVec3*)&ent->model->Mat->right);
            cbparam->cache->poly.vert[2].y = xVec3Dot(&corner, (xVec3*)&ent->model->Mat->up);
            cbparam->cache->poly.vert[2].z = xVec3Dot(&corner, (xVec3*)&ent->model->Mat->at);

            xVec3Init(&cbparam->cache->poly.norm, 0.0f, 1.0f, 0.0f);

            cbparam->cache->castOnEnt = ent;
        }

        return 1;
    }

    m = ent->model;

    if (iModelNumBones(m->Data) > 1)
    {
        return 1;
    }

    oldHeight = cbparam->cache->shadowHeight;

    RwFrameTransform(RpAtomicGetFrame(m->Data), m->Mat, rwCOMBINEREPLACE);

    sModelMat = m->Mat;
    RpAtomicForAllIntersections(m->Data, cbparam->isx, shadowRayModelCB, cbparam->cache);
    sModelMat = NULL;

    if (cbparam->cache->shadowHeight != oldHeight)
    {
        cbparam->cache->castOnEnt = ent;
    }

    return 1;
}

static void xShadowSimple_SceneCollide(xShadowSimpleCache* cache, xVec3* pos, F32 depth)
{
    xEnv* env;
    RpIntersection isx;
    ShadowCBParam cbparam;
    xQCData qcd;

    cache->envHeight = 1e38f;
    cache->shadowHeight = 1e38f;
    cache->castOnEnt = NULL;
    cache->collPriority = 0;

    env = globals.sceneCur->env;

    isx.type = rpINTERSECTLINE;
    isx.t.line.start.x = pos->x;
    isx.t.line.start.y = 0.1f + pos->y;
    isx.t.line.start.z = pos->z;
    isx.t.line.end.x = pos->x;
    isx.t.line.end.y = pos->y - depth;
    isx.t.line.end.z = pos->z;

    if (env->geom->jsp)
    {
        xClumpColl_ForAllIntersections(env->geom->jsp->colltree, &isx, shadowRayCB, cache);
    }
    else
    {
        RpCollisionWorldForAllIntersections(env->geom->world, &isx, shadowRayCB, cache);
    }

    if (1e38f != cache->envHeight)
    {
        cache->envHeight =
            isx.t.line.start.y + cache->envHeight * (isx.t.line.end.y - isx.t.line.start.y);
        isx.t.line.end.y = cache->envHeight;
    }

    xQuickCullForLine(&qcd, (xLine3*)&isx.t.line);

    cbparam.cache = cache;
    cbparam.isx = &isx;

    xGridCheckPosition(&colls_grid, (xVec3*)&isx.t.line.start, &qcd, shadowRayEntCB, &cbparam);
    xGridCheckPosition(&colls_oso_grid, (xVec3*)&isx.t.line.start, &qcd, shadowRayEntCB, &cbparam);

    if (cache->flags & 8)
    {
        xGridCheckPosition(&npcs_grid, (xVec3*)&isx.t.line.start, &qcd, shadowRayEntCB, &cbparam);
    }

    if (1e38f != cache->shadowHeight)
    {
        cache->shadowHeight =
            isx.t.line.start.y + cache->shadowHeight * (isx.t.line.end.y - isx.t.line.start.y);
    }
    else
    {
        cache->shadowHeight = cache->envHeight;
    }

    if (1e38f != cache->shadowHeight && cache->castOnEnt == NULL)
    {
        cache->dydx = -cache->poly.norm.x / cache->poly.norm.y;
        cache->dydz = -cache->poly.norm.z / cache->poly.norm.y;
    }
}

static void xShadowSimple_CalcCorners(xShadowSimpleCache* cache, xEnt* ent, F32 radius, F32 ecc)
{
    RwMatrixTag* mat;
    xVec3 tempnorm;
    F32 ax, az, ay;
    F32 bz, bx, by;

    if (1e38f == cache->shadowHeight)
    {
        return;
    }

    mat = ent->model->Mat;

    if (cache->castOnEnt)
    {
        xMat3x3RMulVec(&tempnorm, (xMat3x3*)cache->castOnEnt->model->Mat, &cache->poly.norm);
        xVec3Normalize(&tempnorm, &tempnorm);

        cache->dydx = -tempnorm.x / tempnorm.y;
        cache->dydz = -tempnorm.z / tempnorm.y;
    }

    if (cache->flags & 1)
    {
        F32 dydz = cache->dydz;
        F32 dydx = cache->dydx;

        ax = mat->right.x * radius * ecc;
        az = mat->right.z * radius * ecc;
        bx = mat->at.x * radius;
        bz = mat->at.z * radius;
        ay = ax * dydx + az * dydz;
        by = bx * dydx + bz * dydz;
    }
    else
    {
        ax = radius * ecc;
        az = 0.0f;
        bx = 0.0f;
        bz = radius;
        ay = ax * cache->dydx;
        by = radius * cache->dydz;
    }

    cache->corner[0].x = mat->pos.x + ax;
    cache->corner[0].y = 0.02f + (cache->shadowHeight + ay);
    cache->corner[0].z = mat->pos.z + az;

    cache->corner[1].x = mat->pos.x + bx;
    cache->corner[1].y = 0.02f + (cache->shadowHeight + by);
    cache->corner[1].z = mat->pos.z + bz;

    cache->corner[2].x = mat->pos.x - bx;
    cache->corner[2].y = 0.02f + (cache->shadowHeight - by);
    cache->corner[2].z = mat->pos.z - bz;

    cache->corner[3].x = mat->pos.x - ax;
    cache->corner[3].y = 0.02f + (cache->shadowHeight - ay);
    cache->corner[3].z = mat->pos.z - az;
}

static void xShadowSimple_AddVerts(xShadowSimpleCache* cache)
{
    F32 y;
    F32 z;
    U8 alpha;

    if (1e38f == cache->shadowHeight || sShadVertCount >= 384)
    {
        return;
    }

    y = cache->corner[0].y;
    z = cache->corner[0].z;
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 0], cache->corner[0].x, y, z);
    y = cache->corner[1].y;
    z = cache->corner[1].z;
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 1], cache->corner[1].x, y, z);
    y = cache->corner[2].y;
    z = cache->corner[2].z;
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 2], cache->corner[2].x, y, z);
    y = cache->corner[1].y;
    z = cache->corner[1].z;
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 3], cache->corner[1].x, y, z);
    y = cache->corner[2].y;
    z = cache->corner[2].z;
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 4], cache->corner[2].x, y, z);
    y = cache->corner[3].y;
    z = cache->corner[3].z;
    RwIm3DVertexSetPos(&sShadVert[sShadVertCount + 5], cache->corner[3].x, y, z);

    alpha = cache->alpha;
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 0], 0, 0, 0, alpha);
    alpha = cache->alpha;
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 1], 0, 0, 0, alpha);
    alpha = cache->alpha;
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 2], 0, 0, 0, alpha);
    alpha = cache->alpha;
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 3], 0, 0, 0, alpha);
    alpha = cache->alpha;
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 4], 0, 0, 0, alpha);
    alpha = cache->alpha;
    RwIm3DVertexSetRGBA(&sShadVert[sShadVertCount + 5], 0, 0, 0, alpha);

    sShadRasters[sShadVertCount / 6] = (RwRaster*)cache->raster;

    sShadVertCount += 6;
}

void xShadowSimple_Init()
{
    RwTexture* tex;
    U32 i;

    memset(sCollQueue, 0, sizeof(sCollQueue));

    tex = (RwTexture*)xSTFindAsset(xStrHash("shadow_round"), NULL);

    sShadRaster = (tex != NULL) ? tex->raster : NULL;

    memset(sShadVert, 0, sizeof(sShadVert));

    for (i = 0; i < 64; i++)
    {
        sShadVert[i * 6 + 1].u = 1.0f;
        sShadVert[i * 6 + 2].v = 1.0f;
        sShadVert[i * 6 + 3].u = 1.0f;
        sShadVert[i * 6 + 4].v = 1.0f;
        sShadVert[i * 6 + 5].u = 1.0f;
        sShadVert[i * 6 + 5].v = 1.0f;
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 0], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 1], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 2], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 3], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 4], 0.0f, 1.0f, 0.0f);
        RwIm3DVertexSetNormal(&sShadVert[i * 6 + 5], 0.0f, 1.0f, 0.0f);
    }
}

void xShadowSimple_CacheInit(xShadowSimpleCache* cache, xEnt* ent, U8 alpha)
{
    S32 i;
    S32 n;
    U32 j;
    zSimpleShadowTableHeader* sst;
    RwRaster* raster;
    U32 flags;
    U32 size;
    RwTexture* tex;

    memset(cache, 0, sizeof(xShadowSimpleCache));

    cache->envHeight = 1e38f;
    cache->shadowHeight = 1e38f;
    cache->flags = 4;
    cache->alpha = alpha;

    if (ent->model == NULL || ent->model->shadowID != 0xDEADBEEF)
    {
        return;
    }

    raster = NULL;
    flags = 0;

    n = xSTAssetCountByType('SHDW');

    for (i = 0; i < n; i++)
    {
        sst = (zSimpleShadowTableHeader*)xSTFindAssetByType('SHDW', i, &size);

        zSimpleShadowTableEntry* entries = (zSimpleShadowTableEntry*)(sst + 1);

        for (j = 0; j < sst->num; j++)
        {
            if (ent->model->modelID == entries[j].modelID)
            {
                tex = (RwTexture*)xSTFindAsset(entries[j].assetID, NULL);

                if (tex != NULL)
                {
                    raster = tex->raster;
                    flags = entries[j].flags;
                }
                else
                {
                    raster = (RwRaster*)0xDEADBEEF;
                }
            }

            if (raster != NULL)
            {
                break;
            }
        }

        if (raster != NULL)
        {
            break;
        }
    }

    if (raster == NULL || raster == (RwRaster*)0xDEADBEEF)
    {
        raster = sShadRaster;
    }

    cache->raster = (U32)raster;
    cache->flags |= (U16)flags;
    ent->model->shadowID = (U32)raster;
}

void xShadowSimple_Add(xShadowSimpleCache* cache, xEnt* ent, F32 radius, F32 ecc)
{
    S32 i;
    U16 shadowWas;
    U32 moved;
    S32 j;
    xVec3* vert;
    xVec3 xformvert[3];
    xVec3 xformnorm;
    xEnt* castOnEnt;
    F32 poo;
    F32 nx;
    F32 nz;
    F32 pdot;
    xVec3* v0;
    xVec3* v1;

    if (cache->flags & 4)
    {
        cache->flags &= (U16)~4;

        shadowWas = ent->baseFlags & 0x10;
        ent->baseFlags &= (U16)~0x10;

        xShadowSimple_SceneCollide(cache, (xVec3*)&ent->model->Mat->pos, 10.0f);

        ent->baseFlags |= shadowWas;

        cache->pos = *(xVec3*)&ent->model->Mat->pos;
        cache->at = *(xVec3*)&ent->model->Mat->at;

        if (1e38f != cache->shadowHeight)
        {
            xShadowSimple_CalcCorners(cache, ent, radius, ecc);
            xShadowSimple_AddVerts(cache);
        }
    }
    else
    {
        moved = (cache->pos.x != ent->model->Mat->pos.x) ||
                (cache->pos.y != ent->model->Mat->pos.y) ||
                (cache->pos.z != ent->model->Mat->pos.z);

        if (1e38f != cache->shadowHeight && (moved || cache->castOnEnt != NULL))
        {
            castOnEnt = cache->castOnEnt;

            if (castOnEnt != NULL)
            {
                xMat4x3Toworld(&xformvert[0], (xMat4x3*)castOnEnt->model->Mat,
                               &cache->poly.vert[0]);
                xMat4x3Toworld(&xformvert[1], (xMat4x3*)castOnEnt->model->Mat,
                               &cache->poly.vert[1]);
                xMat4x3Toworld(&xformvert[2], (xMat4x3*)castOnEnt->model->Mat,
                               &cache->poly.vert[2]);
                xMat3x3RMulVec(&xformnorm, (xMat3x3*)castOnEnt->model->Mat, &cache->poly.norm);
                xVec3Normalize(&xformnorm, &xformnorm);

                // Retail bug, reproduced deliberately: xabs() is applied to the
                // *boolean* (xformnorm.y > 1e-5f), so poo is 0.0f or 1.0f and the
                // magnitude of y is never actually tested.  The target emits the
                // full int->float conversion for the comparison result.
                poo = xabs(xformnorm.y > 1e-5f);

                if (poo)
                {
                    cache->dydx = -xformnorm.x / xformnorm.y;
                    cache->dydz = -xformnorm.z / xformnorm.y;
                }
                else
                {
                    F32 eps = (xformnorm.y > 0.0f) ? 1e-5f : -1e-5f;

                    cache->dydx = -xformnorm.x / eps;
                    cache->dydz = -xformnorm.z / eps;
                }

                vert = xformvert;
            }
            else
            {
                vert = cache->poly.vert;
            }

            for (j = 0; j < 3; j++)
            {
                v0 = &vert[j];
                v1 = &vert[(j == 2) ? 0 : j + 1];

                pdot = (v0->z - v1->z) * (ent->model->Mat->pos.x - v0->x) +
                       (v1->x - v0->x) * (ent->model->Mat->pos.z - v0->z);

                if (pdot > 1e-5f)
                {
                    cache->collPriority += 0x14;
                    break;
                }
            }

            nx = ent->model->Mat->pos.x - vert[0].x;
            nz = ent->model->Mat->pos.z - vert[0].z;

            cache->shadowHeight = vert[0].y + cache->dydx * nx + cache->dydz * nz;

            if (moved)
            {
                cache->envHeight = 1e38f;
            }
        }

        if (moved)
        {
            cache->collPriority += 6;
        }
        else if (cache->castOnEnt != NULL)
        {
            cache->collPriority += 3;
        }
        else
        {
            cache->collPriority += 2;
        }

        for (i = 0; i < 2; i++)
        {
            if (sCollQueue[i].cache == NULL)
            {
                sCollQueue[i].cache = cache;
                sCollQueue[i].priority = sCollQueue[i].cache->collPriority;
                sCollQueue[i].ent = ent;
                sCollQueue[i].radius = radius;
                sCollQueue[i].ecc = ecc;
                return;
            }

            if (cache->collPriority > sCollQueue[i].cache->collPriority)
            {
                if (sCollQueue[1].cache != NULL)
                {
                    xShadowSimple_CalcCorners(sCollQueue[1].cache, sCollQueue[1].ent,
                                              sCollQueue[1].radius, sCollQueue[1].ecc);
                    xShadowSimple_AddVerts(sCollQueue[1].cache);
                }

                for (j = 0; j >= i; j--)
                {
                    sCollQueue[j + 1] = sCollQueue[j];
                }

                sCollQueue[i].cache = cache;
                sCollQueue[i].priority = sCollQueue[i].cache->collPriority;
                sCollQueue[i].ent = ent;
                sCollQueue[i].radius = radius;
                sCollQueue[i].ecc = ecc;
                return;
            }
        }

        xShadowSimple_CalcCorners(cache, ent, radius, ecc);
        xShadowSimple_AddVerts(cache);
    }
}

void xShadowSimple_Render()
{
    S32 qnum;
    xShadowSimpleCache* cache;
    xEnt* ent;
    U16 shadowWas;
    U32 i;
    U32 j;
    RwRaster* raster;

    for (qnum = 0; qnum < 2; qnum++)
    {
        cache = sCollQueue[qnum].cache;

        if (cache != NULL)
        {
            ent = sCollQueue[qnum].ent;

            shadowWas = ent->baseFlags & 0x10;
            ent->baseFlags &= (U16)~0x10;

            xShadowSimple_SceneCollide(cache, (xVec3*)&sCollQueue[qnum].ent->model->Mat->pos,
                                       10.0f);

            ent->baseFlags |= shadowWas;

            cache->pos = *(xVec3*)&sCollQueue[qnum].ent->model->Mat->pos;
            cache->at = *(xVec3*)&sCollQueue[qnum].ent->model->Mat->at;

            xShadowSimple_CalcCorners(cache, sCollQueue[qnum].ent, sCollQueue[qnum].radius,
                                      sCollQueue[qnum].ecc);
            xShadowSimple_AddVerts(cache);
        }

        sCollQueue[qnum].cache = NULL;
    }

    if (sShadVertCount == 0)
    {
        return;
    }

    RxRenderStateVectorLoadDriverState(&xrsv);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCCOLOR);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDDESTCOLOR);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)rwSHADEMODEGOURAUD);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

    i = 0;

    while (i < sShadVertCount)
    {
        raster = sShadRasters[i / 6];

        for (j = i; j < sShadVertCount && sShadRasters[j / 6] == raster; j += 6)
        {
        }

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        RwIm3DTransform(&sShadVert[i], j - i, NULL,
                        rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);
        i = j;

        RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
    }

    RwIm3DEnd();

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)xrsv.SrcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)xrsv.DestBlend);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)xrsv.ShadeMode);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)((xrsv.Flags >> 2) & 1));
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)((xrsv.Flags >> 3) & 1));

    sShadVertCount = 0;
}
