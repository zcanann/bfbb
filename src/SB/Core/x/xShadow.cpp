#include "xShadow.h"

#include "rpworld.h"
#if defined(PS2)
#include <rwsdk/rpcollbsptree.h>
#else
#include "rpcollbsptree.h"
#endif

#include "xMath.h"
#include "xMathInlines.h"
#include "xDraw.h"
#include "xRay3.h"
#include "xQuickCull.h"
#include "xScene.h"
#include "zGrid.h"
#include "iCollide.h"
#include "iCamera.h"
#include "xNPCBasic.h"
#include "iModel.h"
#include "zBase.h"
#include "zEnt.h"
#include "zGlobals.h"

#include <types.h>
#if defined(PS2)
#include <stdlib.h>
#include <rwplcore.h>
#include <rwim2d.h>
#include <rwim3d.h>
#include "iParMgr.h"
#include "xJSP.h"
#include "xClumpColl.h"
#else
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\stdlib.h>
#endif
#include <string.h>

RwRGBAReal ShadowLightColor = { 1.0f, 1.0f, 1.0f, 1.0f };
RwV3d gCamPos = { 0.0f, 0.0f, 0.0f };

F32 ShadowStrength = 0.3f;
static F32 rscale = 1.0f;

RpLight* volatile ShadowLight;
static F32 SHADOW_BF_DOT;
static F32 SHADOW_BOTH;
static RxObjSpace3DVertex* Im3DBuffer;
static U32 Im3DBufferPos;
RwCamera* ShadowCamera;
#if defined(PS2)
RwRaster* ShadowCameraRaster;
#else
RwRaster* volatile ShadowCameraRaster;
#endif
static RwRaster* ShadowRenderRaster;
U32 gShadowFlags;
F32 gShadowObjectRadius;
static S32 shadow_ent_count;
static S32 sShadowCollJSP;
#if defined(PS2)
static U32 skyOldTest;
static U32 shadvolquad_idx[6][4] = { { 3, 2, 1, 0 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 },
                                     { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };
#else
static RwRaster* gc_saveraster;
#endif
static xEnt* sEntSelf;
static xShadowMgr* sMgrList;
static S32 sMgrCount;
static S32 sMgrTotal;
#if defined(PS2)
static RxPipeline* adlSkinPipe;
static RxPipeline* a4dSkinPipe;
static RxPipeline* adlSkinPipeADC;
static RxPipeline* a4dSkinPipeADC;
#endif
static xShadowCache sCacheList[6];

struct _ProjectionParam
{
    RwV3d at;
    RwMatrixTag invMatrix;
    U8 shadowValue;
    S32 fade;
    U32 numIm3DBatch;
    U32 shadowWord;
};

extern U8 xClumpColl_FilterFlags;

#if defined(PS2)
extern "C" RwBool RpSkyTexCacheRasterLock(RwRaster* raster, RwBool bLocked);
extern "C" RxPipeline* RpPDSGetPipe(RwUInt32 pipeID);
void iDrawSetFBA1(S32 value);
void iDrawSetTEST2(S32 value);
void iDrawSetFBMSK(U32 abgr);

enum RpSkyRenderState
{
    rpSKYRENDERSTATEATEST_1 = 3
};

extern "C" RwBool RpSkyRenderStateGet(RpSkyRenderState nState, void* pParam);
extern "C" RwBool RpSkyRenderStateSet(RpSkyRenderState nState, void* pParam);
S32 ShadowMapCreatePipelines();

// PS2 projects receiver triangles in a separate VU0 inline-asm routine; its body
// is not recovered as C.
static void xShadowReceiveShadowFastPS2(xEnt* ent, F32 shadowFactor, S32 shadowMode,
                                        RwMatrixTag* shadowMat, RwRaster* shadowRast);
#endif

RpCollBSPTree* _rpCollBSPTreeForAllCapsuleLeafNodeIntersections(
    RpCollBSPTree* tree, RwLine* line, RwReal radius, RpV3dGradient* grad,
    RwBool (*callBack)(RwInt32, RwInt32, void*), void* data);

void xShadowInit();
static void ShadowCameraDestroy(RwCamera* shadowCamera);
static S32 SetupShadow();
static RwRaster* ShadowRasterCreate(S32 res);
static RwCamera* ShadowCameraCreatePersp(S32 param);
U32 xShadowCameraCreate();
void xShadowRenderWorld(xVec3* center, F32 radius, F32 max_dist);
void xShadowRender(xVec3* center, F32 radius, F32 max_dist);
void xShadow_ListAdd(xEnt* ent);
void xShadowManager_Add(xEnt* ent);
#if !defined(PS2)
static void GCSaveFrameBuffer();
#endif
static RwCamera* ShadowCameraSetSpherePersp(RwCamera* camera, RwV3d* center, F32 radius);
int Im2DRenderQuad(float x1, float y1, float x2, float y2, float z, float recipCamZ, float uvOffset);

static RwCamera* ShadowCameraUpdate(RwCamera* shadowCamera, void* model, void (*renderCB)(void*),
                                    xVec3* center, F32 radius, S32 shadowMode);
#if !defined(PS2)
static void InvertRaster(RwCamera* shadowCamera);
static void GCRestoreFrameBuffer();
#endif

static void xShadow_PickByRayCast(xShadowMgr* mgr);
static void xShadow_PickEntForNPC(xShadowMgr* mgr);

// Layout-only references retain the weak helpers left by stripped debug code.
void __deadstripped_xShadow_draw(const xVec3* center, F32 radius, U32 flags)
{
    xDrawSetColor(0, 0, 0, 0);
    xDrawSphere(center, radius, flags);
}

void xShadowInit()
{
    xShadowCameraCreate();
#if defined(PS2)
    RpSkyTexCacheRasterLock(ShadowCameraRaster, TRUE);
#else
    gc_saveraster = RwRasterCreate(256, 256, 32, 0x504);
#endif
    shadow_ent_count = 0;
#if defined(PS2)
    ShadowMapCreatePipelines();
#endif
    ShadowLight = RpLightCreate(1);
    RpLightSetColor(ShadowLight, &ShadowLightColor);
    RwFrame* frame = RwFrameCreate();
    _rwObjectHasFrameSetFrame(ShadowLight, frame);
}

void xShadowRender(xVec3* center, F32 radius, F32 max_dist)
{
    xShadowRenderWorld(center, radius, max_dist);
}

#if defined(PS2)
#pragma dont_inline on
#endif
static S32 SetupShadow()
{
    S32 res = 256;

    // Continuously halve res until it is less than or
    // equal to either display width or height.
    // On GCN, this routine normally won't happen,
    // as we're already below both dimensions.
#if defined(VERSION_SLES_51968) || defined(VERSION_SLES_51970)
    for (; (res > 512) || (res > 512); res >>= 1);
#elif defined(PS2)
    for (; (res > 640) || (res > 448); res >>= 1);
#elif defined(VERSION_GQPP78) || defined(VERSION_GU4Y78)
    for (; (res > 640) || (res > 528); res >>= 1);
#else
    for (; (res > 640) || (res > 480); res >>= 1);
#endif

    ShadowCamera = ShadowCameraCreatePersp(res);
    if (ShadowCamera == NULL)
    {
        return 0;
    }
    ShadowCameraRaster = ShadowRasterCreate(res);

    RwRaster* raster = ShadowCameraRaster;
    if (raster == NULL)
    {
        return 0;
    }

    ShadowCamera->frameBuffer = raster;
    return 1;
}

#if defined(PS2)
#pragma dont_inline reset
#endif

void xShadowSetWorld(RpWorld* world)
{
    RpWorldAddCamera(world, ShadowCamera);
    SHADOW_BOTH = 2.0f;
}

void xShadowSetLight(xVec3* target_pos, xVec3* in_vec, F32 dst_cast)
{
    xVec3 zvec;
    xMat4x3 matrix;

    xVec3Normalize(&zvec, in_vec);
    xMat3x3LookVec(&matrix, &zvec);
    matrix.pos = *target_pos;

    RwFrame* camFrame = (RwFrame*)ShadowCamera->object.object.parent;
    RwMatrixTag* camMatrix = &camFrame->modelling;

    xMat4x3Copy((xMat4x3*)camMatrix, &matrix);
    RwFrameOrthoNormalize(camFrame);
    RwMatrixUpdate(camMatrix);
    RwFrameUpdateObjects(camFrame);
}

U32 xShadowCameraCreate()
{
    U32 setup = SetupShadow();
    return ((-setup | setup) >> 0x1f);
}

void xShadowCameraUpdate(void* model, void(*renderCB)(void*), xVec3* center, float radius, int shadowMode)
{
    ShadowCameraSetSpherePersp(ShadowCamera, (RwV3d*)center, radius);
    ShadowCameraUpdate(ShadowCamera, model, renderCB, center, radius, shadowMode);
    ShadowRenderRaster = ShadowCameraRaster;
}

static S32 ShadowRender(RwCamera* shadowCamera, RwRaster* shadowRast, RpIntersection* shadowZone,
                        F32 shadowFactor, F32 fadeDist);

void xShadowRenderWorld(xVec3* center, F32 radius, F32 max_dist)
{
    RwFrame* camFrame = (RwFrame*)ShadowCamera->object.object.parent;
    RwMatrixTag* camMatrix = &camFrame->modelling;
    xVec3* at = (xVec3*)&camMatrix->at;
    xVec3* up = (xVec3*)&camMatrix->up;
    xVec3* rt = (xVec3*)&camMatrix->right;
    xCollis entcoll[1];
    xCollis envcoll[1];
    xRay3 R[1];
    RpIntersection shadowZone;
    const F32 sf[3][2] = { { 0.0f, 0.0f }, { 0.0f, 0.5f }, { 0.0f, -0.5f } };
    xQCData q;
    xSphere zone;
    xVec3 ent_pos;
    xVec3 env_pos;
    U32 hit_env;
    U32 hit_ent;
    F32 ent_dist;
    F32 env_dist;
    S32 i;

    gShadowFlags = 0;
    hit_env = 0;
    hit_ent = 0;
    ent_dist = 100.0f;
    env_dist = 100.0f;

    xVec3Init(&ent_pos, 0.0f, 0.0f, 0.0f);
    xVec3Init(&env_pos, 0.0f, 0.0f, 0.0f);

    for (i = 0; i < 1; i++)
    {
        R[i].dir = *at;
        R[i].origin = *center;
        xVec3AddScaled(&R[i].origin, rt, sf[i][0]);
        xVec3AddScaled(&R[i].origin, up, sf[i][1]);
        R[i].min_t = 0.0f;
        R[i].max_t = max_dist;
        R[i].flags = 0xc00;

        xQuickCullForRay(&q, &R[i]);

        entcoll[i].dist = FLOAT_MAX;
        xRayHitsGrid(&colls_grid, globals.sceneCur, &R[i], xRayHitsEnt, &q, &entcoll[i]);
        xRayHitsGrid(&colls_oso_grid, globals.sceneCur, &R[i], xRayHitsEnt, &q, &entcoll[i]);
        xRayHitsGrid(&npcs_grid, globals.sceneCur, &R[i], xRayHitsEnt, &q, &entcoll[i]);
        if (entcoll[i].dist < FLOAT_MAX)
        {
            entcoll[i].flags |= 0x1;
        }
        else
        {
            entcoll[i].flags &= ~0x1;
        }

        envcoll[i].dist = FLOAT_MAX;
        iRayHitsEnv(&R[i], globals.sceneCur->env, &envcoll[i]);
        if (envcoll[i].dist < FLOAT_MAX)
        {
            envcoll[i].flags |= 0x1;
        }
        else
        {
            envcoll[i].flags &= ~0x1;
        }

        if (entcoll[i].flags & 0x1)
        {
            hit_ent = 1;
            if (entcoll[i].dist < ent_dist)
            {
                ent_dist = entcoll[i].dist;
                ent_pos = R[i].origin;
                xVec3AddScaled(&ent_pos, &R[i].dir, entcoll[i].dist);
            }
        }

        if (envcoll[i].flags & 0x1)
        {
            hit_env = 1;
            if (envcoll[i].dist < env_dist)
            {
                env_dist = envcoll[i].dist;
                env_pos = R[i].origin;
                xVec3AddScaled(&env_pos, &R[i].dir, envcoll[i].dist);
            }
        }
    }

    if (hit_env && hit_ent && (ent_dist > 0.0f) && (env_dist > 0.0f) &&
        (xabs(ent_dist - env_dist) < SHADOW_BOTH))
    {
        gShadowFlags |= 0x3;
    }

    if (hit_env && (env_dist > 0.0f) && (env_dist < ent_dist))
    {
        gShadowFlags |= 0x1;
    }

    if (hit_ent && (ent_dist > 0.0f) && (ent_dist < env_dist))
    {
        gShadowFlags |= 0x2;
    }

    if (gShadowFlags & 0x1)
    {
        zone.center = env_pos;
        zone.r = radius;
    }
    else
    {
        return;
    }

    shadowZone.type = rpINTERSECTSPHERE;
    shadowZone.t.sphere.center = *(RwV3d*)&zone.center;
    shadowZone.t.sphere.radius = zone.r;

    ShadowRender(ShadowCamera, ShadowRenderRaster, &shadowZone, ShadowStrength, 0.0f);
}

static void modelRenderCB(void* model)
{
    xModelRender((xModelInstance*)model);
}

U32 xShadowReceiveShadowSetup(xEnt* ent)
{
    if
    (
    (ent->model != NULL) &&
    (xEntIsVisible(ent)) &&
    (ent->baseFlags & 0x10) &&
    (!iModelCull(ent->model->Data, ent->model->Mat))
    )
    {
        return 1;
    }
    return 0;
}

// Layout-only references reproduce the shared literal order from stripped code.
void __deadstripped_xShadow_fractions(F32* values)
{
    values[0] = 0.5f;
    values[1] = 1.0f;
    values[2] = 1e-5f;
}

void xShadowReceiveShadow(xEnt* ent, F32 shadowFactor, S32 shadowMode, RwMatrixTag* shadowMat,
                          RwRaster* shadowRast)
{
    RwMatrixTag oldroot;
#if !defined(PS2)
    RwMatrixTag invMatrix;
    RwV3d vShadOut[3];
    RwV3d vShad[3];
    RwV3d at;
    RwV3d scl;
    RwV3d tr;
    RwV3d normal;
    S32 fogstate;
#endif

    if (ent->model->Scale.x)
    {
        oldroot = *ent->model->Mat;

        ent->model->Mat->right.x *= ent->model->Scale.x;
        ent->model->Mat->right.y *= ent->model->Scale.x;
        ent->model->Mat->right.z *= ent->model->Scale.x;
        ent->model->Mat->up.x *= ent->model->Scale.y;
        ent->model->Mat->up.y *= ent->model->Scale.y;
        ent->model->Mat->up.z *= ent->model->Scale.y;
        ent->model->Mat->at.x *= ent->model->Scale.z;
        ent->model->Mat->at.y *= ent->model->Scale.z;
        ent->model->Mat->at.z *= ent->model->Scale.z;
    }

#if defined(PS2)
    xShadowReceiveShadowFastPS2(ent, shadowFactor, shadowMode, shadowMat, shadowRast);
#else
    RwCamera* shadowCamera = ShadowCamera;

    if (shadowRast != NULL)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowRast);
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowCamera->frameBuffer);
    }

    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);

    switch (shadowMode)
    {
    case 1:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        break;
    case 0:
    default:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
        break;
    }

    if (shadowFactor < 0.0f)
    {
        shadowFactor = -shadowFactor;

        switch (shadowMode)
        {
        case 1:
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCALPHA);
            break;
        case 0:
        default:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
            break;
        }
    }
    else
    {
        switch (shadowMode)
        {
        case 1:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            break;
        case 0:
        default:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
            break;
        }
    }

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);

    RwMatrixTag* shadowMatrix;
    if (shadowMat != NULL)
    {
        shadowMatrix = shadowMat;
    }
    else
    {
        shadowMatrix = &((RwFrame*)shadowCamera->object.object.parent)->modelling;
    }

    at = shadowMatrix->at;

    F32 radius = gShadowObjectRadius;
    F32 fadeDist = 0.0f;

    RwMatrixInvert(&invMatrix, shadowMatrix);

    scl.x = scl.y = -0.5f / radius;
    scl.z = 1.0f / (fadeDist + radius);
    RwMatrixScale(&invMatrix, &scl, rwCOMBINEPOSTCONCAT);

    tr.x = tr.y = 0.5f;
    tr.z = 0.0f;
    RwMatrixTranslate(&invMatrix, &tr, rwCOMBINEPOSTCONCAT);

    xModelInstance* model;
    U32 i;
    xVec3* xvert;
    RpTriangle* tri;
    RpGeometry* geom;
    U8 val;
    U32 max_verts = 0;

    for (model = ent->model; model != NULL; model = model->Next)
    {
        U32 num_verts = model->Data->geometry->numVertices;

        if (num_verts > max_verts)
        {
            max_verts = num_verts;
        }

        if (ent->pflags & 0x40)
        {
            break;
        }
    }

    xvert = (xVec3*)xMemPushTemp(max_verts * sizeof(xVec3));
    if (xvert != NULL)
    {
        Im3DBuffer = gRenderBuffer.m_vertex;

        for (model = ent->model; model != NULL; model = model->Next)
        {
            geom = model->Data->geometry;

            iModelVertEval(model->Data, 0, geom->numVertices, model->Mat, NULL, xvert);

            val = (U8)(255.0f * shadowFactor);
            tri = geom->triangles;

            for (i = 0; i < geom->numTriangles; i++, tri++)
            {
                if (Im3DBufferPos > 0x1dd)
                {
                    if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                                        rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
                    {
                        RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
                        RwIm3DEnd();
                    }
                    Im3DBufferPos = 0;
                }

                RxObjSpace3DVertex* imv = &Im3DBuffer[Im3DBufferPos];
                xVec3* v0 = &xvert[tri->vertIndex[0]];
                xVec3* v1 = &xvert[tri->vertIndex[1]];
                xVec3* v2 = &xvert[tri->vertIndex[2]];

                vShad[0] = *(RwV3d*)v0;
                vShad[1] = *(RwV3d*)v1;
                vShad[2] = *(RwV3d*)v2;

                RwV3dTransformPoints(vShadOut, vShad, 3, &invMatrix);

                if (((vShadOut[0].z < 0.0f) && (vShadOut[1].z < 0.0f) && (vShadOut[2].z < 0.0f)) ||
                    ((vShadOut[0].x < 0.0f) && (vShadOut[1].x < 0.0f) && (vShadOut[2].x < 0.0f)) ||
                    ((vShadOut[0].x > 1.0f) && (vShadOut[1].x > 1.0f) && (vShadOut[2].x > 1.0f)) ||
                    ((vShadOut[0].y < 0.0f) && (vShadOut[1].y < 0.0f) && (vShadOut[2].y < 0.0f)) ||
                    ((vShadOut[0].y > 1.0f) && (vShadOut[1].y > 1.0f) && (vShadOut[2].y > 1.0f)))
                {
                    continue;
                }

                xVec3 a;
                xVec3 b;

                a.x = v1->x - v0->x;
                a.y = v1->y - v0->y;
                a.z = v1->z - v0->z;
                b.x = v2->x - v0->x;
                b.y = v2->y - v0->y;
                b.z = v2->z - v0->z;

                normal.x = a.y * b.z - a.z * b.y;
                normal.y = a.z * b.x - a.x * b.z;
                normal.z = a.x * b.y - a.y * b.x;

                F32 len = RwV3dLength(&normal);
                if (xabs(len) < 1e-05f)
                {
                    if (len < 0.0f)
                    {
                        len = -1e-05f;
                    }
                    else
                    {
                        len = 1e-05f;
                    }
                }

                F32 scale = 0.008f / len;
                normal.x *= scale;
                normal.y *= scale;
                normal.z *= scale;

                if (normal.x * at.x + normal.y * at.y + normal.z * at.z > -0.00069724565f)
                {
                    continue;
                }

                RwIm3DVertexSetPos(&imv[0], v0->x + normal.x, v0->y + normal.y, v0->z + normal.z);

                RwIm3DVertexSetPos(&imv[1], v1->x + normal.x, v1->y + normal.y, v1->z + normal.z);

                RwIm3DVertexSetPos(&imv[2], v2->x + normal.x, v2->y + normal.y, v2->z + normal.z);

                imv[0].u = vShadOut[0].x;
                imv[1].u = vShadOut[1].x;
                imv[2].u = vShadOut[2].x;
                imv[0].v = vShadOut[0].y;
                imv[1].v = vShadOut[1].y;
                imv[2].v = vShadOut[2].y;

                RwIm3DVertexSetRGBA(&imv[0], val, val, val, val);
                RwIm3DVertexSetRGBA(&imv[1], val, val, val, val);
                RwIm3DVertexSetRGBA(&imv[2], val, val, val, val);

                Im3DBufferPos += 3;
            }

            if (Im3DBufferPos != 0)
            {
                if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                                    rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
                {
                    RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
                    RwIm3DEnd();
                }
                Im3DBufferPos = 0;
            }

            if (ent->pflags & 0x40)
            {
                break;
            }
        }

        xMemPopTemp(xvert);

        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
    }
#endif

    if (ent->model->Scale.x)
    {
        *ent->model->Mat = oldroot;
    }
}

#if defined(PS2)
static void xShadowReceiveShadowFastPS2(xEnt* ent, F32 shadowFactor, S32 shadowMode,
                                      RwMatrixTag* shadowMat, RwRaster* shadowRast)
{
    RwCamera* shadowCamera = ShadowCamera;
    F32 radius;
    F32 fadeDist = 0.0f;
    RwMatrixTag invMatrix;
    RwV3d at __attribute__((aligned(16)));
    RwV3d scl;
    RwV3d tr;

    if (shadowRast != NULL)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowRast);
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowCamera->frameBuffer);
    }

    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

    switch (shadowMode)
    {
    case 1:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        break;
    case 0:
    default:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
        break;
    }

    if (shadowFactor < 0.0f)
    {
        shadowFactor = -shadowFactor;

        switch (shadowMode)
        {
        case 1:
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCALPHA);
            break;
        case 0:
        default:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCALPHA);
            break;
        }
    }
    else
    {
        switch (shadowMode)
        {
        case 1:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            break;
        case 0:
        default:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            break;
        }
    }


    RwMatrixTag* shadowMatrix;
    if (shadowMat != NULL)
    {
        shadowMatrix = shadowMat;
    }
    else
    {
        shadowMatrix = &((RwFrame*)shadowCamera->object.object.parent)->modelling;
    }

    at = shadowMatrix->at;

    radius = gShadowObjectRadius;

    RwMatrixInvert(&invMatrix, shadowMatrix);

    scl.x = scl.y = -0.5f / radius;
    scl.z = 1.0f / (fadeDist + radius);
    RwMatrixScale(&invMatrix, &scl, rwCOMBINEPOSTCONCAT);

    tr.x = tr.y = 0.5f;
    tr.z = 0.0f;
    RwMatrixTranslate(&invMatrix, &tr, rwCOMBINEPOSTCONCAT);

    asm volatile("lqc2 vf28, 0x0(%0)\n"
                 "lqc2 vf29, 0x10(%0)\n"
                 "lqc2 vf30, 0x20(%0)\n"
                 "lqc2 vf31, 0x30(%0)\n"
                 : : "r"(&invMatrix) : "memory");

    U32 i;
    U32 num_verts;
    xVec3* xvert;
    RpTriangle* tri;
    RpGeometry* geom;
    U8 val = (U8)(255.0f * shadowFactor);
    U32 vertex_color = (val << 24) | (val << 16) | (val << 8) | val;
    xModelInstance* model = ent->model;
    U32 max_verts = 0;
    U32 model_num = 0;
    U32 ent_id = ent->id;

    for (; model != NULL; model = model->Next)
    {
        RpAtomic* atomic = model->Data;
        model_num++;
        // Retail keeps this atomic/model diagnostic snapshot in EE registers.
        asm volatile("addiu $10, %0, 0x0\n"
                     "addiu $11, %1, 0x0\n"
                     "lw $12, 0x0(%2)\n"
                     "lw $13, 0x4(%2)\n"
                     "pextlw $12, $13, $12\n"
                     "lw $13, 0x8(%2)\n"
                     "lw $14, 0xc(%2)\n"
                     "pextlw $13, $14, $13\n"
                     "lw $14, 0x10(%2)\n"
                     "lw $15, 0x14(%2)\n"
                     "pextlw $14, $15, $14\n"
                     "lw $15, 0x18(%2)\n"
                     "lw $24, 0x1c(%2)\n"
                     "pextlw $15, $24, $15\n"
                     : : "r"(model_num), "r"(ent_id), "r"(atomic)
                     : "$10", "$11", "$12", "$13", "$14", "$15", "$24");

        geom = atomic->geometry;
        num_verts = geom->numVertices;
        if (num_verts > max_verts)
        {
            max_verts = num_verts;
        }
    }

    xvert = (xVec3*)xMemPushTemp(max_verts * sizeof(xVec3));
    if (xvert != NULL)
    {
        Im3DBuffer = gRenderBuffer.m_vertex;
        asm volatile("lqc2 vf20, 0x0(%0)\n"
                     "vmulx.w vf20, vf0, vf20x\n"
                     "vmuly.w vf21, vf0, vf20y\n"
                     "vmulz.w vf22, vf0, vf20z\n"
                     : : "r"(&at) : "memory");

        for (model = ent->model; model != NULL; model = model->Next)
        {
            geom = model->Data->geometry;
            iModelVertEval(model->Data, 0, geom->numVertices, model->Mat, NULL, xvert);
            tri = geom->triangles;
            for (i = 0; i < geom->numTriangles; i++, tri++)
            {
                if (Im3DBufferPos > 0x1dd)
                {
                    if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                                        rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
                    {
                        RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
                        RwIm3DEnd();
                    }
                    Im3DBufferPos = 0;
                }

                RxObjSpace3DVertex* imv = &Im3DBuffer[Im3DBufferPos];
                xVec3* v0 = &xvert[tri->vertIndex[0]];
                xVec3* v1 = &xvert[tri->vertIndex[1]];
                xVec3* v2 = &xvert[tri->vertIndex[2]];

                U32 clip;
                // Project into shadow UV space and reject a shared outside clip plane.
                asm volatile("lwu $8, 0x0(%1)\n"
                             "lui $9, 0xbf00\n"
                             "qmtc2 $9, vf24\n"
                             "lwu $9, 0x4(%1)\n"
                             "lwu $10, 0x8(%1)\n"
                             "pextlw $8, $9, $8\n"
                             "pcpyld $8, $10, $8\n"
                             "qmtc2 $8, vf1\n"
                             "lwu $8, 0x0(%2)\n"
                             "vaddx.yzw vf24, vf0, vf24x\n"
                             "lwu $9, 0x4(%2)\n"
                             "lwu $10, 0x8(%2)\n"
                             "pextlw $8, $9, $8\n"
                             "pcpyld $8, $10, $8\n"
                             "qmtc2 $8, vf2\n"
                             "lwu $8, 0x0(%3)\n"
                             "lwu $9, 0x4(%3)\n"
                             "lwu $10, 0x8(%3)\n"
                             "pextlw $8, $9, $8\n"
                             "pcpyld $8, $10, $8\n"
                             "qmtc2 $8, vf3\n"
                             "vmulax.xyz ACC, vf28, vf1x\n"
                             "vmadday.xyz ACC, vf29, vf1y\n"
                             "vmaddaz.xyz ACC, vf30, vf1z\n"
                             "vmaddw.xyz vf4, vf31, vf0w\n"
                             "vmulax.xyz ACC, vf28, vf2x\n"
                             "vmadday.xyz ACC, vf29, vf2y\n"
                             "vmaddaz.xyz ACC, vf30, vf2z\n"
                             "vmaddw.xyz vf5, vf31, vf0w\n"
                             "vmulax.xyz ACC, vf28, vf3x\n"
                             "vmadday.xyz ACC, vf29, vf3y\n"
                             "vmaddaz.xyz ACC, vf30, vf3z\n"
                             "vmaddw.xyz vf6, vf31, vf0w\n"
                             "vadd.xyz vf25, vf4, vf24\n"
                             "vadd.xyz vf26, vf5, vf24\n"
                             "vadd.xyz vf27, vf6, vf24\n"
                             "vsub vf7, vf2, vf1\n"
                             "vclipw.xyz vf25, vf24w\n"
                             "vclipw.xyz vf26, vf24w\n"
                             "vclipw.xyz vf27, vf24w\n"
                             "vsub vf8, vf3, vf1\n"
                             "vnop\n"
                             "vnop\n"
                             "vnop\n"
                             "vnop\n"
                             "vnop\n"
                             "cfc2 $8, vi18\n"
                             "srl $9, $8, 6\n"
                             "srl $10, $8, 12\n"
                             "and $8, $8, $9\n"
                             "and $8, $8, $10\n"
                             "andi %0, $8, 0x2f\n"
                             : "=r"(clip) : "r"(v0), "r"(v1), "r"(v2)
                             : "$8", "$9", "$10", "memory");
                if (clip != 0)
                {
                    continue;
                }

                asm volatile("vopmula.xyz ACC, vf7, vf8\n"
                             "vopmsub.xyz vf27, vf8, vf7\n"
                             : : : "memory");

                F32 local_SHADOW_BIAS_AMT = 0.002f;
                F32 local_SHADOW_MINNORMY = 0.00017431141f;
                // Start the reciprocal square root while testing the unnormalized facing dot.
                asm volatile("vmul vf22, vf27, vf27\n"
                             "vmulax.w ACC, vf20, vf27x\n"
                             "vmadday.w ACC, vf21, vf27y\n"
                             "vmaddz.w vf24, vf22, vf27z\n"
                             "vaddy.x vf24, vf22, vf22y\n"
                             "vnop\n"
                             "vnop\n"
                             "vnop\n"
                             "vaddz.x vf24, vf24, vf22z\n"
                             "vnop\n"
                             "vnop\n"
                             "vnop\n"
                             "lw $8, 0x0(%1)\n"
                             "vnop\n"
                             "qmtc2 $8, vf23\n"
                             "vrsqrt Q, vf23x, vf24x\n"
                             "qmfc2 $8, vf24\n"
                             "mtsah $0, 0x6\n"
                             "qfsrv $8, $8, $8\n"
                             "mtc1 $8, %0\n"
                             : "=f"(shadowFactor) : "r"(&local_SHADOW_BIAS_AMT)
                             : "$8", "memory");

                if (!(shadowFactor <= 0.0f))
                {
                    continue;
                }

                asm volatile("vwaitq\n"
                             "vmulq vf27, vf27, Q\n"
                             "vnop\n"
                             "vnop\n"
                             "lw $9, 0x0(%1)\n"
                             "qmfc2 $10, vf27\n"
                             "vadd vf1, vf1, vf27\n"
                             "vadd vf2, vf2, vf27\n"
                             "vadd vf3, vf3, vf27\n"
                             "dsrl32 $10, $10, 0\n"
                             "sltu %0, $10, $9\n"
                             : "=r"(clip) : "r"(&local_SHADOW_MINNORMY)
                             : "$8", "$9", "$10", "memory");
                if (clip != 0)
                {
                    continue;
                }

                // Store biased world positions and the original projected UV coordinates.
                asm volatile("qmfc2 $8, vf1\n"
                             "dsrl32 $9, $8, 0\n"
                             "pcpyud $10, $8, $8\n"
                             "sw $8, 0x0(%0)\n"
                             "sw $9, 0x4(%0)\n"
                             "sw $10, 0x8(%0)\n"
                             "qmfc2 $8, vf2\n"
                             "dsrl32 $9, $8, 0\n"
                             "pcpyud $10, $8, $8\n"
                             "sw $8, 0x24(%0)\n"
                             "sw $9, 0x28(%0)\n"
                             "sw $10, 0x2c(%0)\n"
                             "qmfc2 $8, vf3\n"
                             "dsrl32 $9, $8, 0\n"
                             "pcpyud $10, $8, $8\n"
                             "sw $8, 0x48(%0)\n"
                             "sw $9, 0x4c(%0)\n"
                             "sw $10, 0x50(%0)\n"
                             "qmfc2 $8, vf4\n"
                             "dsrl32 $9, $8, 0\n"
                             "sw $8, 0x1c(%0)\n"
                             "sw $9, 0x20(%0)\n"
                             "qmfc2 $8, vf5\n"
                             "dsrl32 $9, $8, 0\n"
                             "sw $8, 0x40(%0)\n"
                             "sw $9, 0x44(%0)\n"
                             "qmfc2 $8, vf6\n"
                             "dsrl32 $9, $8, 0\n"
                             "sw $8, 0x64(%0)\n"
                             "sw $9, 0x68(%0)\n"
                             : : "r"(imv) : "$8", "$9", "$10", "memory");

                *(U32*)&imv[0].c = vertex_color;
                *(U32*)&imv[1].c = vertex_color;
                *(U32*)&imv[2].c = vertex_color;
                Im3DBufferPos += 3;
            }

            if (Im3DBufferPos != 0)
            {
                if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                                    rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
                {
                    RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
                    RwIm3DEnd();
                }
                Im3DBufferPos = 0;
            }
        }

        xMemPopTemp(xvert);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    }
}
#endif

void xShadowRender(xEnt* ent, F32 max_dist)
{
    xVec3 center;
    F32 radius;

    zEntGetShadowParams(ent, &center, &radius, xEntShadow::RADIUS_RASTER);
    xShadowCameraUpdate(ent->model, modelRenderCB, &center, radius, 0);
    xShadowRender(&center, radius, max_dist);
}

void xShadow_ListAdd(xEnt* ent)
{
    xShadowManager_Add(ent);
}

int Im2DRenderQuad(float x1, float y1, float x2, float y2, float z, float recipCamZ, float uvOffset)
{
    RwIm2DVertex v[4];

    RwIm2DVertexSetScreenX(&v[0], x1);
    RwIm2DVertexSetScreenY(&v[0], y1);
    RwIm2DVertexSetScreenZ(&v[0], z);
    RwIm2DVertexSetRecipCameraZ(&v[0], recipCamZ);
    RwIm2DVertexSetIntRGBA(&v[0], 255, 255, 255, 255);
    RwIm2DVertexSetU(&v[0], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&v[0], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&v[1], x1);
    RwIm2DVertexSetScreenY(&v[1], y2);
    RwIm2DVertexSetScreenZ(&v[1], z);
    RwIm2DVertexSetRecipCameraZ(&v[1], recipCamZ);
    RwIm2DVertexSetIntRGBA(&v[1], 255, 255, 255, 255);
    RwIm2DVertexSetU(&v[1], uvOffset, recipCamZ);
    RwIm2DVertexSetV(&v[1], 1.0f + uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&v[2], x2);
    RwIm2DVertexSetScreenY(&v[2], y1);
    RwIm2DVertexSetScreenZ(&v[2], z);
    RwIm2DVertexSetRecipCameraZ(&v[2], recipCamZ);
    RwIm2DVertexSetIntRGBA(&v[2], 255, 255, 255, 255);
    RwIm2DVertexSetU(&v[2], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&v[2], uvOffset, recipCamZ);

    RwIm2DVertexSetScreenX(&v[3], x2);
    RwIm2DVertexSetScreenY(&v[3], y2);
    RwIm2DVertexSetScreenZ(&v[3], z);
    RwIm2DVertexSetRecipCameraZ(&v[3], recipCamZ);
    RwIm2DVertexSetIntRGBA(&v[3], 255, 255, 255, 255);
    RwIm2DVertexSetU(&v[3], 1.0f + uvOffset, recipCamZ);
    RwIm2DVertexSetV(&v[3], 1.0f + uvOffset, recipCamZ);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, v, 4);
    return 1;
}

void __deadstripped_xShadow_conversion(F32* values, S32 value)
{
    values[0] = 10.0f;
    values[1] = (F32)value;
}

#if !defined(PS2)
static void InvertRaster(RwCamera* shadowCamera)
{
    RwIm2DVertex vx[4];
    RwRaster* raster = shadowCamera->frameBuffer;
    F32 w = raster->width;
    F32 h = raster->height;

    RwIm2DVertexSetScreenX(&vx[0], 0.0f);
    RwIm2DVertexSetScreenY(&vx[0], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[0], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[0], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[0], 0.0f, 1.0f);
    RwIm2DVertexSetV(&vx[0], 0.0f, 1.0f);

    RwIm2DVertexSetScreenX(&vx[1], 0.0f);
    RwIm2DVertexSetScreenY(&vx[1], h);
    RwIm2DVertexSetScreenZ(&vx[1], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[1], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[1], 0.0f, 1.0f);
    RwIm2DVertexSetV(&vx[1], 1.0f, 1.0f);

    RwIm2DVertexSetScreenX(&vx[2], w);
    RwIm2DVertexSetScreenY(&vx[2], 0.0f);
    RwIm2DVertexSetScreenZ(&vx[2], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[2], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[2], 1.0f, 1.0f);
    RwIm2DVertexSetV(&vx[2], 0.0f, 1.0f);

    RwIm2DVertexSetScreenX(&vx[3], w);
    RwIm2DVertexSetScreenY(&vx[3], h);
    RwIm2DVertexSetScreenZ(&vx[3], RwIm2DGetNearScreenZ());
    RwIm2DVertexSetIntRGBA(&vx[3], 255, 255, 255, 255);
    RwIm2DVertexSetU(&vx[3], 1.0f, 1.0f);
    RwIm2DVertexSetV(&vx[3], 1.0f, 1.0f);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)0);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVDESTCOLOR);

    RwIm2DRenderPrimitive(rwPRIMTYPETRISTRIP, vx, 4);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
}
#endif

F32 __deadstripped_xShadow_square(F32 value)
{
    return 0.001f * SQ(value);
}

void ShadowCameraDestroy(RwCamera* shadowCamera)
{
    if (shadowCamera == NULL)
    {
        return;
    }

    _rwFrameSyncDirty();
    RwFrame* parent = (RwFrame*)shadowCamera->object.object.parent;
    if (parent != NULL)
    {
        _rwObjectHasFrameSetFrame(shadowCamera, NULL);
        RwFrameDestroy(parent);
    }

    // Scheduling issue with RwRasterDestroy calls

    RwRaster* zBuffer = shadowCamera->zBuffer;
    if (zBuffer != NULL)
    {
        shadowCamera->zBuffer = NULL;
        RwRasterDestroy(zBuffer);
    }

    RwRaster* frameBuffer = shadowCamera->frameBuffer;
    if (frameBuffer != NULL)
    {
        shadowCamera->frameBuffer = NULL;
        RwRasterDestroy(frameBuffer);
    }

    RwCameraDestroy(shadowCamera);
}

static RwCamera* ShadowCameraUpdate(RwCamera* shadowCamera, void* model, void (*renderCB)(void*),
                                    xVec3* center, F32 radius, S32 shadowMode)
{
    RwRGBA bgColor = { 255, 255, 255, 0 };
    RwCamera* camera = *(RwCamera**)RwEngineInstance;
    S32 fogstate;

    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);

#if defined(PS2)
    if (camera != NULL)
    {
        iDrawSetFBA1(0);
    }
#endif

    if (camera != NULL)
    {
        RwCameraEndUpdate(camera);
    }

#if defined(PS2)
    RwCameraClear(shadowCamera, &bgColor, rwCAMERACLEARIMAGE);
#else
    GCSaveFrameBuffer();

    shadowCamera->frameBuffer->width--;
    shadowCamera->frameBuffer->height--;
    RwCameraClear(shadowCamera, &bgColor, rwCAMERACLEARIMAGE);
    shadowCamera->frameBuffer->width++;
    shadowCamera->frameBuffer->height++;
#endif

    RwFrameOrthoNormalize((RwFrame*)shadowCamera->object.object.parent);

    if (RwCameraBeginUpdate(shadowCamera) != NULL)
    {
        iCameraFrustumPlanes(shadowCamera, globals.camera.frustplane);

        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)0);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

#if defined(PS2)
        iDrawSetFBA1(1);
        iDrawSetTEST2(0);
        renderCB(model);
        iDrawSetFBA1(0);
#else
        renderCB(model);

        if (shadowMode == 0)
        {
            InvertRaster(shadowCamera);
        }
#endif

        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);

        RwCameraEndUpdate(shadowCamera);
#if !defined(PS2)
        RwGameCubeCameraTextureFlush(shadowCamera->frameBuffer, 0);
#endif
    }

    if (camera != NULL)
    {
        RwCameraBeginUpdate(camera);
        iCameraFrustumPlanes(camera, globals.camera.frustplane);
    }

#if !defined(PS2)
    GCRestoreFrameBuffer();
#endif
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

    return shadowCamera;
}

static RwRaster* ShadowRasterCreate(S32 res)
{
#if defined(PS2)
    return RwRasterCreate(res, res, 16, 5);
#else
    return RwRasterCreate(res, res, 0, 5);
#endif
}

static RpCollisionTriangle* ShadowRenderTriangleCB(RpIntersection* isx, RpWorldSector* sector,
                                                   RpCollisionTriangle* collTriangle, F32 distance,
                                                   void* data)
{
    _ProjectionParam* param = (_ProjectionParam*)data;
    RwV3d vShadOut[3];
    RwV3d vShad[3];

    if (collTriangle->normal.x * param->at.x + collTriangle->normal.y * param->at.y +
            collTriangle->normal.z * param->at.z >
        SHADOW_BF_DOT)
    {
        return collTriangle;
    }

#if defined(PS2)
    // Project three unaligned input positions using the matrix in vf28-vf31.
    asm volatile("lwu $8, 0(%0)\n"
                 "lwu $9, 4(%0)\n"
                 "lwu $10, 8(%0)\n"
                 "pextlw $8, $9, $8\n"
                 "pcpyld $8, $10, $8\n"
                 "qmtc2 $8, vf1\n"
                 "lwu $8, 0(%1)\n"
                 "lwu $9, 4(%1)\n"
                 "lwu $10, 8(%1)\n"
                 "pextlw $8, $9, $8\n"
                 "pcpyld $8, $10, $8\n"
                 "qmtc2 $8, vf2\n"
                 "lwu $8, 0(%2)\n"
                 "lwu $9, 4(%2)\n"
                 "lwu $10, 8(%2)\n"
                 "pextlw $8, $9, $8\n"
                 "pcpyld $8, $10, $8\n"
                 "qmtc2 $8, vf3\n"
                 "vmulax.xyz ACC, vf28, vf1x\n"
                 "vmadday.xyz ACC, vf29, vf1y\n"
                 "vmaddaz.xyz ACC, vf30, vf1z\n"
                 "vmaddw.xyz vf1, vf31, vf0w\n"
                 "vmulax.xyz ACC, vf28, vf2x\n"
                 "vmadday.xyz ACC, vf29, vf2y\n"
                 "vmaddaz.xyz ACC, vf30, vf2z\n"
                 "vmaddw.xyz vf2, vf31, vf0w\n"
                 "vmulax.xyz ACC, vf28, vf3x\n"
                 "vmadday.xyz ACC, vf29, vf3y\n"
                 "vmaddaz.xyz ACC, vf30, vf3z\n"
                 "vmaddw.xyz vf3, vf31, vf0w\n"
                 "qmfc2 $8, vf1\n"
                 "dsrl32 $9, $8, 0\n"
                 "pcpyud $10, $8, $8\n"
                 "sw $8, 0(%3)\n"
                 "sw $9, 4(%3)\n"
                 "sw $10, 8(%3)\n"
                 "qmfc2 $8, vf2\n"
                 "dsrl32 $9, $8, 0\n"
                 "pcpyud $10, $8, $8\n"
                 "sw $8, 12(%3)\n"
                 "sw $9, 16(%3)\n"
                 "sw $10, 20(%3)\n"
                 "qmfc2 $8, vf3\n"
                 "dsrl32 $9, $8, 0\n"
                 "pcpyud $10, $8, $8\n"
                 "sw $8, 24(%3)\n"
                 "sw $9, 28(%3)\n"
                 "sw $10, 32(%3)"
                 : : "r"(collTriangle->vertices[0]), "r"(collTriangle->vertices[1]),
                     "r"(collTriangle->vertices[2]), "r"(vShadOut)
                 : "$8", "$9", "$10", "memory");
#else
    vShad[0] = *collTriangle->vertices[0];
    vShad[1] = *collTriangle->vertices[1];
    vShad[2] = *collTriangle->vertices[2];

    RwV3dTransformPoints(vShadOut, vShad, 3, &param->invMatrix);
#endif

    if (((vShadOut[0].z < 0.0f) && (vShadOut[1].z < 0.0f) && (vShadOut[2].z < 0.0f)) ||
        ((vShadOut[0].x < 0.0f) && (vShadOut[1].x < 0.0f) && (vShadOut[2].x < 0.0f)) ||
        ((vShadOut[0].x > 1.0f) && (vShadOut[1].x > 1.0f) && (vShadOut[2].x > 1.0f)) ||
        ((vShadOut[0].y < 0.0f) && (vShadOut[1].y < 0.0f) && (vShadOut[2].y < 0.0f)) ||
        ((vShadOut[0].y > 1.0f) && (vShadOut[1].y > 1.0f) && (vShadOut[2].y > 1.0f)))
    {
        return collTriangle;
    }

    if (Im3DBufferPos > 0x1dd)
    {
        if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                            rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
            RwIm3DEnd();
        }
        param->numIm3DBatch++;
        Im3DBufferPos = 0;
    }

#if defined(PS2)
    RxObjSpace3DVertex* imv = &Im3DBuffer[Im3DBufferPos];
    xVec3 c;

    c.x = 0.002f * collTriangle->normal.x;
    c.y = 0.002f * collTriangle->normal.y;
    c.z = 0.002f * collTriangle->normal.z;

    RwIm3DVertexSetPos(&imv[0], collTriangle->vertices[0]->x + c.x,
                      collTriangle->vertices[0]->y + c.y,
                      collTriangle->vertices[0]->z + c.z);

    RwIm3DVertexSetPos(&imv[1], collTriangle->vertices[1]->x + c.x,
                      collTriangle->vertices[1]->y + c.y,
                      collTriangle->vertices[1]->z + c.z);

    RwIm3DVertexSetPos(&imv[2], collTriangle->vertices[2]->x + c.x,
                      collTriangle->vertices[2]->y + c.y,
                      collTriangle->vertices[2]->z + c.z);
#else
    RwV3d* v = collTriangle->vertices[0];
    RxObjSpace3DVertex* imv = &Im3DBuffer[Im3DBufferPos];
    xVec3 c;

    c.x = 0.008f * collTriangle->normal.x;
    c.y = 0.008f * collTriangle->normal.y;
    c.z = 0.008f * collTriangle->normal.z;

    RwIm3DVertexSetPos(&imv[0], v->x + c.x, v->y + c.y, v->z + c.z);

    v = collTriangle->vertices[1];
    RwIm3DVertexSetPos(&imv[1], v->x + c.x, v->y + c.y, v->z + c.z);

    v = collTriangle->vertices[2];
    RwIm3DVertexSetPos(&imv[2], v->x + c.x, v->y + c.y, v->z + c.z);

#endif

    imv[0].u = vShadOut[0].x;
    imv[1].u = vShadOut[1].x;
    imv[2].u = vShadOut[2].x;
    imv[0].v = vShadOut[0].y;
    imv[1].v = vShadOut[1].y;
    imv[2].v = vShadOut[2].y;

#if defined(PS2)
    U32 sw = param->shadowWord;
    *(U32*)&imv[0].c = sw;
    *(U32*)&imv[1].c = sw;
    *(U32*)&imv[2].c = sw;
#else
    U8 sw = param->shadowValue;

    RwIm3DVertexSetRGBA(&imv[0], sw, sw, sw, sw);
    RwIm3DVertexSetRGBA(&imv[1], sw, sw, sw, sw);
    RwIm3DVertexSetRGBA(&imv[2], sw, sw, sw, sw);
#endif

    Im3DBufferPos += 3;

    return collTriangle;
}

static S32 ShadowRender(RwCamera* shadowCamera, RwRaster* shadowRast, RpIntersection* shadowZone,
                        F32 shadowFactor, F32 fadeDist)
{
    _ProjectionParam param;
    F32 radius;
    RwV3d scl;
    RwV3d tr;
    xVec3 A;
    xVec3 B;
#if !defined(PS2)
    S32 fogstate;
#endif

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowCamera->frameBuffer);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);

    if (shadowFactor < 0.0f)
    {
        shadowFactor = -shadowFactor;
#if defined(PS2)
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCALPHA);
#else
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
#endif
    }
    else
    {
#if defined(PS2)
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
#else
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
#endif
    }

    RwMatrixTag* shadowMatrix = &((RwFrame*)shadowCamera->object.object.parent)->modelling;

    param.at = shadowMatrix->at;

    radius = gShadowObjectRadius;

    RwMatrixInvert(&param.invMatrix, shadowMatrix);

    scl.x = scl.y = -0.5f / radius;
    scl.z = 1.0f / (fadeDist + radius);
    RwMatrixScale(&param.invMatrix, &scl, rwCOMBINEPOSTCONCAT);

    param.fade = (fadeDist > 0.0f) ? 1 : 0;

    param.shadowValue = (U8)(255.0f * shadowFactor);
    param.shadowWord = (param.shadowValue << 24) | (param.shadowValue << 16) |
                       (param.shadowValue << 8) | param.shadowValue;

    param.numIm3DBatch = 0;

    Im3DBuffer = gRenderBuffer.m_vertex;
    Im3DBufferPos = 0;

    xVec3Add(&A, (xVec3*)&shadowMatrix->pos, (xVec3*)&shadowMatrix->at);
    RwV3dTransformPoints((RwV3d*)&B, (RwV3d*)&A, 1, &param.invMatrix);

    tr.x = tr.y = 0.5f;
    tr.z = 0.0f;
    RwMatrixTranslate(&param.invMatrix, &tr, rwCOMBINEPOSTCONCAT);

#if defined(PS2)
    // The original callbacks use this persistent VU0 projection matrix.
    asm volatile("lqc2 vf28, 0(%0)\n"
                 "lqc2 vf29, 16(%0)\n"
                 "lqc2 vf30, 32(%0)\n"
                 "lqc2 vf31, 48(%0)"
                 : : "r"(&param.invMatrix) : "memory");
#else
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
#endif

    if (globals.sceneCur->env->geom->jsp != NULL)
    {
        sShadowCollJSP = 1;
        xClumpColl_ForAllIntersections(globals.sceneCur->env->geom->jsp->colltree, shadowZone,
                                       ShadowRenderTriangleCB, &param);
        sShadowCollJSP = 0;
    }
    else
    {
        RpCollisionWorldForAllIntersections(globals.sceneCur->env->geom->world, shadowZone,
                                           ShadowRenderTriangleCB, &param);
    }

    if (Im3DBufferPos != 0)
    {
        if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                            rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
            RwIm3DEnd();
        }
        Im3DBufferPos = 0;
    }

#if !defined(PS2)
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);
#endif
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);

    return 1;
}

#if !defined(PS2)
void GCSaveFrameBuffer()
{
    RwGameCubeCameraTextureFlush(gc_saveraster, 0);
}

static void GCRestoreFrameBuffer()
{
    RwCamera* cam = *(RwCamera**)RwEngineInstance;
    F32 recipCamZ = (1.0f / cam->farPlane);

    RwRenderStateSet(rwRENDERSTATESRCBLEND,      (void*)2);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,     (void*)1);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,   (void*)0);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE,  (void*)0);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)1);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, gc_saveraster);

    Im2DRenderQuad(0.0f, 0.0f, 256.0f, 256.0f, RwIm2DGetFarScreenZ(), recipCamZ, 0.001953125f);

    RwRenderStateSet(rwRENDERSTATEZTESTENABLE,  (void*)1);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATESRCBLEND,     (void*)5);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND,    (void*)6);
}
#endif

static RwCamera* ShadowCameraCreatePersp(S32 param)
{
    RwCamera* cam = RwCameraCreate();
    if (cam != NULL)
    {
        _rwObjectHasFrameSetFrame(cam, RwFrameCreate());

        RwV2d viewWin;
        viewWin.x = 0.001745331f;
        viewWin.y = 0.001745331f;

        RwCameraSetViewWindow(cam, &viewWin);

        if (cam->object.object.parent != NULL)
        {
            RwRaster* raster = RwRasterCreate(param, param, 0, 1);
            if (raster != NULL)
            {
                cam->zBuffer = raster;
                return cam;
            }
        }
    }
    ShadowCameraDestroy(cam);
    return NULL;
}

static RwCamera* ShadowCameraSetSpherePersp(RwCamera* camera, RwV3d* center, F32 radius)
{
    RwFrame* camFrame = (RwFrame*)camera->object.object.parent;
    RwMatrixTag* camMatrix = &camFrame->modelling;
    RwV3d* camPos = &camMatrix->pos;

    F32 objDepth = 572.95807f * radius;
    F32 nearZ = objDepth - rscale * radius;
    F32 farZ = objDepth + rscale * radius;

    camera->nearPlane = nearZ;
    camera->farPlane = farZ;
    RwCameraSetNearClipPlane(camera, nearZ);
    RwCameraSetFarClipPlane(camera, farZ);

    *camPos = *center;
    RwV3dIncrementScaledMacro(camPos, &camMatrix->at, -objDepth);
    gCamPos = *camPos;

    RwMatrixUpdate(camMatrix);
    RwFrameUpdateObjects(camFrame);

    gShadowObjectRadius = radius;

    return camera;
}

#if defined(PS2)
static U8 ShadowInsideBoxAdjust(xVec3* volume)
{
    xVec3* v0;
    xVec3* v1;
    xVec3* v2;
    xVec3* v3;
    xVec3 normal[6];
    U32 i;
    RwCamera* mainCamera = *(RwCamera**)RwEngineInstance;
    RwMatrixTag* mainMatrix = &((RwFrame*)mainCamera->object.object.parent)->modelling;
    F32 nearclip;

    nearclip = 1.05f * xVec3Dist((xVec3*)&mainMatrix->pos, (xVec3*)&mainCamera->frustumCorners[0]);

    for (i = 0; i < 6; i++)
    {
        v0 = &volume[shadvolquad_idx[i][0]];
        v1 = &volume[shadvolquad_idx[i][1]];
        v2 = &volume[shadvolquad_idx[i][2]];

        F32 dx = mainMatrix->pos.x - v0->x;
        F32 dy = mainMatrix->pos.y - v0->y;
        F32 dz = mainMatrix->pos.z - v0->z;
        F32 ax = v1->x - v0->x;
        F32 ay = v1->y - v0->y;
        F32 az = v1->z - v0->z;
        F32 bx = v2->x - v0->x;
        F32 by = v2->y - v0->y;
        F32 bz = v2->z - v0->z;
        normal[i].x = ay * bz - by * az;
        normal[i].y = az * bx - bz * ax;
        normal[i].z = ax * by - bx * ay;
        xVec3Normalize(&normal[i], &normal[i]);

        if (!(dx * normal[i].x + dy * normal[i].y + dz * normal[i].z >= nearclip))
        {
            continue;
        }

        return 0;
    }

    for (i = 0; i < 6; i++)
    {
        v0 = &volume[shadvolquad_idx[i][0]];
        v1 = &volume[shadvolquad_idx[i][1]];
        v2 = &volume[shadvolquad_idx[i][2]];
        v3 = &volume[shadvolquad_idx[i][3]];

        v0->x += 2.0f * nearclip * normal[i].x;
        v0->y += 2.0f * nearclip * normal[i].y;
        v0->z += 2.0f * nearclip * normal[i].z;
        v1->x += 2.0f * nearclip * normal[i].x;
        v1->y += 2.0f * nearclip * normal[i].y;
        v1->z += 2.0f * nearclip * normal[i].z;
        v2->x += 2.0f * nearclip * normal[i].x;
        v2->y += 2.0f * nearclip * normal[i].y;
        v2->z += 2.0f * nearclip * normal[i].z;
        v3->x += 2.0f * nearclip * normal[i].x;
        v3->y += 2.0f * nearclip * normal[i].y;
        v3->z += 2.0f * nearclip * normal[i].z;
    }

    return 1;
}

static void DrawAlphaBox(xVec3* volume, S32 frontface, U8 alpha)
{
    U32 i;
    U32 numV;
    RxObjSpace3DVertex boxV[36];
    RxObjSpace3DVertex* v3d;
    RwMatrixTag* mainMatrix;
    xVec3 normal;
    xVec3* v0;
    xVec3* v1;
    xVec3* v2;
    xVec3* v3;

    mainMatrix = &((RwFrame*)(*(RwCamera**)RwEngineInstance)->object.object.parent)->modelling;
    numV = 0;
    v3d = boxV;

    for (i = 0; i < 6; i++)
    {
        v0 = &volume[shadvolquad_idx[i][0]];
        v1 = &volume[shadvolquad_idx[i][1]];
        v2 = &volume[shadvolquad_idx[i][2]];
        v3 = &volume[shadvolquad_idx[i][3]];

        F32 dx = mainMatrix->pos.x - v0->x;
        F32 dy = mainMatrix->pos.y - v0->y;
        F32 dz = mainMatrix->pos.z - v0->z;
        F32 ax = v1->x - v0->x;
        F32 ay = v1->y - v0->y;
        F32 az = v1->z - v0->z;
        F32 bx = v2->x - v0->x;
        F32 by = v2->y - v0->y;
        F32 bz = v2->z - v0->z;
        normal.x = ay * bz - by * az;
        normal.y = az * bx - bz * ax;
        normal.z = ax * by - bx * ay;
        xVec3Normalize(&normal, &normal);

        F32 dot = dx * normal.x + dy * normal.y + dz * normal.z;

        if (frontface && dot < 0.0f)
        {
            continue;
        }

        if (!frontface && dot > 0.0f)
        {
            continue;
        }

        RwIm3DVertexSetPos(&v3d[0], v0->x, v0->y, v0->z);
        RwIm3DVertexSetPos(&v3d[1], v1->x, v1->y, v1->z);
        RwIm3DVertexSetPos(&v3d[2], v2->x, v2->y, v2->z);
        RwIm3DVertexSetPos(&v3d[3], v0->x, v0->y, v0->z);
        RwIm3DVertexSetPos(&v3d[4], v2->x, v2->y, v2->z);
        RwIm3DVertexSetPos(&v3d[5], v3->x, v3->y, v3->z);

        RwIm3DVertexSetRGBA(&v3d[0], alpha, 0, 0, alpha);
        RwIm3DVertexSetRGBA(&v3d[1], alpha, 0, 0, alpha);
        RwIm3DVertexSetRGBA(&v3d[2], alpha, 0, 0, alpha);
        RwIm3DVertexSetRGBA(&v3d[3], alpha, 0, 0, alpha);
        RwIm3DVertexSetRGBA(&v3d[4], alpha, 0, 0, alpha);
        RwIm3DVertexSetRGBA(&v3d[5], alpha, 0, 0, alpha);

        numV += 6;
        v3d += 6;
    }

    if (RwIm3DTransform(boxV, numV, NULL, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
    {
        RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
    }
    RwIm3DEnd();
}

static void xShadowSetVolume(RwCamera* shadowCamera, xVec3* pos, F32 depth)
{
    S32 i;
    xVec3 volume[8];
    F32 invNearFar;
    F32 lerp;
    RwMatrixTag* camMatrix = &((RwFrame*)shadowCamera->object.object.parent)->modelling;
    S32 cullstate;
    S32 ztest;
    S32 zwrite;
    S32 srcblend;
    S32 destblend;

    F32 atx = camMatrix->at.x;
    F32 aty = camMatrix->at.y;
    F32 atz = camMatrix->at.z;

    for (i = 0; i < 4; i++)
    {
        xVec3* cnear = (xVec3*)&shadowCamera->frustumCorners[i];
        xVec3* cfar = (xVec3*)&shadowCamera->frustumCorners[i + 4];
        F32 dnear = atx * cnear->x + aty * cnear->y + atz * cnear->z;
        F32 dpos = atx * pos->x + aty * pos->y + atz * pos->z;
        F32 dfar = atx * cfar->x + aty * cfar->y + atz * cfar->z;

        invNearFar = 1.0f / (dfar - dnear);

        lerp = invNearFar * (dpos - dnear);
        volume[i].x = xlerp(cnear->x, cfar->x, lerp);
        volume[i].y = xlerp(cnear->y, cfar->y, lerp);
        volume[i].z = xlerp(cnear->z, cfar->z, lerp);

        lerp = invNearFar * (dpos + depth - dnear);
        volume[i + 4].x = xlerp(cnear->x, cfar->x, lerp);
        volume[i + 4].y = xlerp(cnear->y, cfar->y, lerp);
        volume[i + 4].z = xlerp(cnear->z, cfar->z, lerp);
    }

    RwRenderStateGet(rwRENDERSTATECULLMODE, &cullstate);
    RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &ztest);
    RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);
    RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcblend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, &destblend);

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)0);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

    iDrawSetFBMSK(0xffffff);

    RpSkyRenderStateGet(rpSKYRENDERSTATEATEST_1, &skyOldTest);
    RpSkyRenderStateSet(rpSKYRENDERSTATEATEST_1, (void*)(skyOldTest & ~0x1));

    if (ShadowInsideBoxAdjust(volume))
    {
        DrawAlphaBox(volume, 0, 0);
        RpSkyRenderStateSet(rpSKYRENDERSTATEATEST_1, (void*)((skyOldTest & ~0xC001) | 0xC001));
    }
    else
    {
        DrawAlphaBox(volume, 1, 0);
        DrawAlphaBox(volume, 0, 0xFF);
        RpSkyRenderStateSet(rpSKYRENDERSTATEATEST_1, (void*)((skyOldTest & ~0xC001) | 0x4001));
    }

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)cullstate);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)ztest);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)zwrite);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcblend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)destblend);

    iDrawSetFBMSK(0);
}
#endif

struct ShadowCacheContext
{
    xShadowCache* cache;
    F32 minNormY;
};

struct ShadowCBParam
{
    xShadowCache* cache;
    RpIntersection* isx;
    xVec3 capsuleStart;
    xVec3 capsuleEnd;
    F32 capsuleRadius;
    xEnt* ent;
    RwLine localLine;
    RwV3d localDelta;
    F32 localRadius;
    xMat4x3* modelMat;
    RpGeometry* geom;
    U32 polyFound;
    xEnt* rayCloser[5];
};

static RpCollisionTriangle* shadowCacheEnvCB(RpIntersection* isx, RpWorldSector* sector,
                                             RpCollisionTriangle* collTriangle, F32 distance,
                                             void* data)
{
    ShadowCacheContext* context = (ShadowCacheContext*)data;
    xShadowCache* cache = context->cache;

    if (cache->polyCount >= 256)
    {
        return NULL;
    }

    if (collTriangle->normal.y < context->minNormY)
    {
        return collTriangle;
    }

    if (sShadowCollJSP && !(((xClumpCollBSPTriangle*)collTriangle->index)->flags & 0x8))
    {
        return collTriangle;
    }

    xShadowPoly* poly = &cache->poly[cache->polyCount];
    cache->polyCount++;

    poly->vert[0] = *(xVec3*)collTriangle->vertices[0];
    poly->vert[1] = *(xVec3*)collTriangle->vertices[1];
    poly->vert[2] = *(xVec3*)collTriangle->vertices[2];
    poly->norm = *(xVec3*)&collTriangle->normal;

    F32 dydx = -collTriangle->normal.x / collTriangle->normal.y;
    F32 dydz = -collTriangle->normal.z / collTriangle->normal.y;
    F32 depth0 = poly->vert[0].y + (dydx * (cache->pos.x - poly->vert[0].x) +
                                    dydz * (cache->pos.z - poly->vert[0].z));
    F32 n0x = poly->vert[0].z - poly->vert[1].z;
    F32 n0z = poly->vert[1].x - poly->vert[0].x;
    F32 n0d = n0x * (cache->pos.x - poly->vert[0].x) + n0z * (cache->pos.z - poly->vert[0].z);
    F32 n1x = poly->vert[1].z - poly->vert[2].z;
    F32 n1z = poly->vert[2].x - poly->vert[1].x;
    F32 n1d = n1x * (cache->pos.x - poly->vert[1].x) + n1z * (cache->pos.z - poly->vert[1].z);
    F32 n2x = poly->vert[2].z - poly->vert[0].z;
    F32 n2z = poly->vert[0].x - poly->vert[2].x;
    F32 n2d = n2x * (cache->pos.x - poly->vert[2].x) + n2z * (cache->pos.z - poly->vert[2].z);

    if ((n0d <= 1e-05f) && (n1d <= 1e-05f) && (n2d <= 1e-05f))
    {
        cache->polyRayDepth[0] = MAX(depth0, cache->polyRayDepth[0]);
    }

    if ((0.5f * n0x * cache->radius + n0d <= 1e-05f) &&
        (0.5f * n1x * cache->radius + n1d <= 1e-05f) &&
        (0.5f * n2x * cache->radius + n2d <= 1e-05f))
    {
        cache->polyRayDepth[1] = MAX(0.5f * dydx * cache->radius + depth0, cache->polyRayDepth[1]);
    }

#if defined(PS2)
    if ((n0d - 0.5f * n0x * cache->radius <= 1e-05f) &&
        (n1d - 0.5f * n1x * cache->radius <= 1e-05f) &&
        (n2d - 0.5f * n2x * cache->radius <= 1e-05f))
#else
    if ((-(0.5f * n0x * cache->radius - n0d) <= 1e-05f) &&
        (-(0.5f * n1x * cache->radius - n1d) <= 1e-05f) &&
        (-(0.5f * n2x * cache->radius - n2d) <= 1e-05f))
#endif
    {
#if defined(PS2)
        cache->polyRayDepth[2] =
            MAX(depth0 - 0.5f * dydx * cache->radius, cache->polyRayDepth[2]);
#else
        cache->polyRayDepth[2] =
            MAX(-(0.5f * dydx * cache->radius - depth0), cache->polyRayDepth[2]);
#endif
    }

    if ((0.5f * n0z * cache->radius + n0d <= 1e-05f) &&
        (0.5f * n1z * cache->radius + n1d <= 1e-05f) &&
        (0.5f * n2z * cache->radius + n2d <= 1e-05f))
    {
        cache->polyRayDepth[3] = MAX(0.5f * dydz * cache->radius + depth0, cache->polyRayDepth[3]);
    }

#if defined(PS2)
    if ((n0d - 0.5f * n0z * cache->radius <= 1e-05f) &&
        (n1d - 0.5f * n1z * cache->radius <= 1e-05f) &&
        (n2d - 0.5f * n2z * cache->radius <= 1e-05f))
#else
    if ((-(0.5f * n0z * cache->radius - n0d) <= 1e-05f) &&
        (-(0.5f * n1z * cache->radius - n1d) <= 1e-05f) &&
        (-(0.5f * n2z * cache->radius - n2d) <= 1e-05f))
#endif
    {
#if defined(PS2)
        cache->polyRayDepth[4] =
            MAX(depth0 - 0.5f * dydz * cache->radius, cache->polyRayDepth[4]);
#else
        cache->polyRayDepth[4] =
            MAX(-(0.5f * dydz * cache->radius - depth0), cache->polyRayDepth[4]);
#endif
    }

    return collTriangle;
}

static S32 shadowCacheLeafCB(S32 numTriangles, S32 triOffset, void* data)
{
    xVec3* wv;
    ShadowCBParam* cbparam = (ShadowCBParam*)data;
    xShadowCache* cache = cbparam->cache;
    RpGeometry* geometry = cbparam->geom;
    RwV3d* vertices = geometry->morphTarget->verts;
    RpTriangle* triangles = geometry->triangles;
    S32 triSlot;
    U16* triIndex = RpCollisionGeometryGetData(geometry)->triangleMap + triOffset;

#if defined(PS2)
    S32 i;
#endif

    while (numTriangles--)
    {
        triSlot = *triIndex;
        triIndex++;

        RpTriangle* tri = &triangles[triSlot];
        S32 vertIndex0 = tri->vertIndex[0];
        S32 vertIndex1 = tri->vertIndex[1];
        S32 vertIndex2 = tri->vertIndex[2];
        RwV3d* v0 = &vertices[vertIndex0];
        RwV3d* v1 = &vertices[vertIndex1];
        RwV3d* v2 = &vertices[vertIndex2];
        xVec3 worldV[3];

        xMat4x3Toworld(&worldV[0], cbparam->modelMat, (xVec3*)v0);
        xMat4x3Toworld(&worldV[1], cbparam->modelMat, (xVec3*)v1);
        xMat4x3Toworld(&worldV[2], cbparam->modelMat, (xVec3*)v2);

        F32 startX = cbparam->capsuleStart.x;
        F32 startZ = cbparam->capsuleStart.z;

#if defined(PS2)
        for (i = 0; i < 3; i++)
        {
            xVec3* vert0 = &worldV[i];
            xVec3* vert1 = &worldV[(i == 2) ? 0 : i + 1];
            F32 nz = vert0->z - vert1->z;
            F32 nx = vert1->x - vert0->x;
            F32 nmag2 = nz * nz + nx * nx;
            F32 pdot = nz * (startX - vert0->x) + nx * (startZ - vert0->z);

            if ((pdot > 0.0f) &&
                (pdot * pdot >=
                 nmag2 * (cbparam->capsuleRadius * cbparam->capsuleRadius)))
            {
                goto next_tri;
            }
        }

        for (i = 0; i < 3; i++)
        {
            xVec3* vert0 = &worldV[i];
            xVec3* vert1 = &worldV[(i + 1) % 3];
            xVec3* vert2 = &worldV[(i + 2) % 3];
            F32 dotA = (vert1->z - vert0->z) * (startZ - vert0->z) +
                       (vert1->x - vert0->x) * (startX - vert0->x);
            F32 dotB = (vert2->z - vert0->z) * (startZ - vert0->z) +
                       (vert2->x - vert0->x) * (startX - vert0->x);

            if ((dotA < 0.0f) && (dotB < 0.0f) &&
                ((startZ - vert0->z) * (startZ - vert0->z) +
                     (startX - vert0->x) * (startX - vert0->x) >
                 cbparam->capsuleRadius * cbparam->capsuleRadius))
            {
                goto next_tri;
            }
        }

#else
        wv = worldV;

        U32 j;
        for (j = 0; j < 3; j++)
        {
            U32 k = (j == 2) ? 0 : j + 1;
            F32 posX = startX - wv[j].x;
            F32 posZ = startZ - wv[j].z;
            F32 nz = wv[j].z - worldV[k].z;
            F32 nx = worldV[k].x - wv[j].x;
            F32 nmag2 = nz * nz + nx * nx;
            F32 pdot = nz * posX + nx * posZ;

            if ((pdot > 0.0f) &&
                (pdot * pdot >=
                 nmag2 * (cbparam->capsuleRadius * cbparam->capsuleRadius)))
            {
                goto next_tri;
            }
        }

        for (S32 k = 0; k < 3; k++)
        {
            xVec3* vert0 = &worldV[(k + 1) % 3];
            xVec3* vert1 = &worldV[(k + 2) % 3];
            F32 dotA = (vert0->x - wv[k].x) * (startX - wv[k].x) +
                       (vert0->z - wv[k].z) * (startZ - wv[k].z);
            F32 dotBx = (vert1->x - wv[k].x) * (startX - wv[k].x);
            F32 dotBz = (vert1->z - wv[k].z) * (startZ - wv[k].z);
            F32 dotB = dotBx + dotBz;

            if ((dotA < 0.0f) && (dotB < 0.0f) &&
                ((startX - wv[k].x) * (startX - wv[k].x) +
                     (startZ - wv[k].z) * (startZ - wv[k].z) >
                 cbparam->capsuleRadius * cbparam->capsuleRadius))
            {
                goto next_tri;
            }
        }

#endif
        cbparam->polyFound++;

        {
            xVec3 aa;
            xVec3 bb;
            xVec3 trinorm;
            xVec3Sub(&aa, (xVec3*)v1, (xVec3*)v0);
            xVec3Sub(&bb, (xVec3*)v2, (xVec3*)v0);
            xVec3Cross(&trinorm, &aa, &bb);
            xVec3Normalize(&trinorm, &trinorm);

            F32 depthtest = (xabs(trinorm.y) > 1e-05f) ? trinorm.y : 1e-05f;

            F32 dydx = -trinorm.x / depthtest;
            F32 dydz = -trinorm.z / depthtest;
            F32 depth0 = worldV[0].y + (dydx * (cache->pos.x - worldV[0].x) +
                                        dydz * (cache->pos.z - worldV[0].z));
            F32 n0x = worldV[0].z - worldV[1].z;
            F32 n0z = worldV[1].x - worldV[0].x;
            F32 n0d = n0x * (cache->pos.x - worldV[0].x) + n0z * (cache->pos.z - worldV[0].z);
            F32 n1x = worldV[1].z - worldV[2].z;
            F32 n1z = worldV[2].x - worldV[1].x;
            F32 n1d = n1x * (cache->pos.x - worldV[1].x) + n1z * (cache->pos.z - worldV[1].z);
            F32 n2x = worldV[2].z - worldV[0].z;
            F32 n2z = worldV[0].x - worldV[2].x;
            F32 n2d = n2x * (cache->pos.x - worldV[2].x) + n2z * (cache->pos.z - worldV[2].z);

            F32 denom;

            if ((n0d <= 1e-05f) && (n1d <= 1e-05f) && (n2d <= 1e-05f) &&
                (depth0 > cache->polyRayDepth[0]))
            {
                cbparam->rayCloser[0] = cbparam->ent;
                cache->polyRayDepth[0] = depth0;
            }

            if ((0.5f * n0x * cache->radius + n0d <= 1e-05f) &&
                (0.5f * n1x * cache->radius + n1d <= 1e-05f) &&
                (0.5f * n2x * cache->radius + n2d <= 1e-05f))
            {
                denom = 0.5f * dydx * cache->radius + depth0;

                if (denom > cache->polyRayDepth[1])
                {
                    cbparam->rayCloser[1] = cbparam->ent;
                    cache->polyRayDepth[1] = denom;
                }
            }

#if defined(PS2)
            if ((n0d - 0.5f * n0x * cache->radius <= 1e-05f) &&
                (n1d - 0.5f * n1x * cache->radius <= 1e-05f) &&
                (n2d - 0.5f * n2x * cache->radius <= 1e-05f))
#else
            if ((-(0.5f * n0x * cache->radius - n0d) <= 1e-05f) &&
                (-(0.5f * n1x * cache->radius - n1d) <= 1e-05f) &&
                (-(0.5f * n2x * cache->radius - n2d) <= 1e-05f))
#endif
            {
#if defined(PS2)
                denom = depth0 - 0.5f * dydx * cache->radius;
#else
                denom = -(0.5f * dydx * cache->radius - depth0);
#endif

                if (denom > cache->polyRayDepth[2])
                {
                    cbparam->rayCloser[2] = cbparam->ent;
                    cache->polyRayDepth[2] = denom;
                }
            }

            if ((0.5f * n0z * cache->radius + n0d <= 1e-05f) &&
                (0.5f * n1z * cache->radius + n1d <= 1e-05f) &&
                (0.5f * n2z * cache->radius + n2d <= 1e-05f))
            {
                denom = 0.5f * dydz * cache->radius + depth0;

                if (denom > cache->polyRayDepth[3])
                {
                    cbparam->rayCloser[3] = cbparam->ent;
                    cache->polyRayDepth[3] = denom;
                }
            }

#if defined(PS2)
            if ((n0d - 0.5f * n0z * cache->radius <= 1e-05f) &&
                (n1d - 0.5f * n1z * cache->radius <= 1e-05f) &&
                (n2d - 0.5f * n2z * cache->radius <= 1e-05f))
#else
            if ((-(0.5f * n0z * cache->radius - n0d) <= 1e-05f) &&
                (-(0.5f * n1z * cache->radius - n1d) <= 1e-05f) &&
                (-(0.5f * n2z * cache->radius - n2d) <= 1e-05f))
#endif
            {
#if defined(PS2)
                denom = depth0 - 0.5f * dydz * cache->radius;
#else
                denom = -(0.5f * dydz * cache->radius - depth0);
#endif

                if (denom > cache->polyRayDepth[4])
                {
                    cbparam->rayCloser[4] = cbparam->ent;
                    cache->polyRayDepth[4] = denom;
                }
            }
        }

    next_tri:;
    }

    return 1;
}

static S32 shadowCacheEntityCB(xEnt* ent, void* cbdata)
{
    ShadowCBParam* cbparam = (ShadowCBParam*)cbdata;
    xCollis coll;
    RwMatrixTag inverseLTM;
    RpV3dGradient grad;
    F32 recip;

    if (!(ent->baseFlags & 0x10))
    {
        return 1;
    }

    if (cbparam->cache->entCount >= 16)
    {
        return 0;
    }

    if (ent == sEntSelf)
    {
        return 1;
    }

    coll.flags = 0;

    if (ent->bound.type == XBOUND_TYPE_SPHERE)
    {
        xBoxHitsSphere((xBox*)cbparam->isx, &ent->bound.sph, &coll);
    }
    else if (ent->bound.type == XBOUND_TYPE_OBB)
    {
        xBoxHitsObb((xBox*)cbparam->isx, &ent->bound.box.box, ent->bound.mat, &coll);
    }
    else if ((ent->bound.type == XBOUND_TYPE_BOX) &&
             (((xBox*)cbparam->isx)->upper.x > ent->bound.box.box.lower.x) &&
             (((xBox*)cbparam->isx)->upper.y > ent->bound.box.box.lower.y) &&
             (((xBox*)cbparam->isx)->upper.z > ent->bound.box.box.lower.z) &&
             (((xBox*)cbparam->isx)->lower.x < ent->bound.box.box.upper.x) &&
             (((xBox*)cbparam->isx)->lower.y < ent->bound.box.box.upper.y) &&
             (((xBox*)cbparam->isx)->lower.z < ent->bound.box.box.upper.z))
    {
        coll.flags |= 0x1;
    }

    if (coll.flags & 0x1)
    {
        xModelInstance* model = (ent->collModel != NULL) ? ent->collModel : ent->model;

        RpCollisionData* colldata = RpCollisionGeometryGetData(model->Data->geometry);

        if ((model->Data->boundingSphere.radius > 2.0f) && (colldata != NULL) &&
            (colldata->tree != NULL))
        {
            RwMatrixInvert(&inverseLTM, model->Mat);
            RwV3dTransformPoints((RwV3d*)&cbparam->localLine, (RwV3d*)&cbparam->capsuleStart, 2,
                                 &inverseLTM);

            cbparam->localDelta.x = cbparam->localLine.end.x - cbparam->localLine.start.x;
            cbparam->localDelta.y = cbparam->localLine.end.y - cbparam->localLine.start.y;
            cbparam->localDelta.z = cbparam->localLine.end.z - cbparam->localLine.start.z;
            cbparam->localRadius = cbparam->capsuleRadius * xVec3Length((xVec3*)&inverseLTM);
            cbparam->geom = model->Data->geometry;
            cbparam->modelMat = (xMat4x3*)model->Mat;
            cbparam->polyFound = 0;
            cbparam->ent = ent;

            memset(cbparam->rayCloser, 0, sizeof(cbparam->rayCloser));

            recip = 0.0f;
            if (cbparam->localDelta.x != 0.0f)
            {
                recip = 1.0f / cbparam->localDelta.x;
            }
            grad.dydx = cbparam->localDelta.y * recip;
            grad.dzdx = cbparam->localDelta.z * recip;

            recip = 0.0f;
            if (cbparam->localDelta.y != 0.0f)
            {
                recip = 1.0f / cbparam->localDelta.y;
            }
            grad.dxdy = cbparam->localDelta.x * recip;
            grad.dzdy = cbparam->localDelta.z * recip;

            recip = 0.0f;
            if (cbparam->localDelta.z != 0.0f)
            {
                recip = 1.0f / cbparam->localDelta.z;
            }
            grad.dxdz = cbparam->localDelta.x * recip;
            grad.dydz = cbparam->localDelta.y * recip;

            _rpCollBSPTreeForAllCapsuleLeafNodeIntersections(colldata->tree, &cbparam->localLine,
                                                             cbparam->localRadius, &grad,
                                                             shadowCacheLeafCB, cbparam);

            if (cbparam->polyFound != 0)
            {
                cbparam->cache->ent[cbparam->cache->entCount++] = ent;
            }
        }
        else
        {
            cbparam->cache->ent[cbparam->cache->entCount++] = ent;
        }
    }

    return 1;
}

void xShadowVertical_FillCache(xShadowCache* cache, xVec3* pos, F32 r, F32 depth, F32 minNormY)
{
    ShadowCBParam cbparam;
    RpIntersection isx;
    F32 sortRayDepth[5];
    xQCData qcd;
    ShadowCacheContext context;

    cache->pos = *pos;
    cache->radius = r;
    cache->entCount = 0;
    cache->polyCount = 0;
    cache->polyRayDepth[0] = -1e38f;
    cache->polyRayDepth[1] = -1e38f;
    cache->polyRayDepth[2] = -1e38f;
    cache->polyRayDepth[3] = -1e38f;
    cache->polyRayDepth[4] = -1e38f;

    xEnv* env = globals.sceneCur->env;

    isx.type = rpINTERSECTBOX;
    isx.t.box.sup.x = pos->x + r;
    isx.t.box.sup.y = pos->y + r;
    isx.t.box.sup.z = pos->z + r;
    isx.t.box.inf.x = pos->x - r;
    isx.t.box.inf.y = (pos->y - r) - depth;
    isx.t.box.inf.z = pos->z - r;

    context.cache = cache;
    context.minNormY = minNormY;

    if (env->geom->jsp != NULL)
    {
        sShadowCollJSP = 1;
        xClumpColl_ForAllIntersections(env->geom->jsp->colltree, &isx, shadowCacheEnvCB, &context);
        sShadowCollJSP = 0;
    }
    else
    {
        RpCollisionWorldForAllIntersections(env->geom->world, &isx, shadowCacheEnvCB, &context);
    }

    memcpy(sortRayDepth, cache->polyRayDepth, sizeof(sortRayDepth));

    for (S32 i = 0; i < 5; i++)
    {
        for (S32 j = 0; j < 4; j++)
        {
            if (sortRayDepth[j] > sortRayDepth[j + 1])
            {
                F32 t = sortRayDepth[j];
                sortRayDepth[j] = sortRayDepth[j + 1];
                sortRayDepth[j + 1] = t;
            }
        }
    }

    F32 endY = sortRayDepth[2];
    if (endY == -1e38f)
    {
        endY = pos->y - depth;
    }
    if (endY > pos->y - 0.001f)
    {
        endY = pos->y - 0.001f;
    }
    if (endY - 1.0f > isx.t.box.inf.y)
    {
        isx.t.box.inf.y = endY - 1.0f;
    }

    cbparam.cache = cache;
    cbparam.isx = &isx;
    cbparam.capsuleStart.x = pos->x;
    cbparam.capsuleStart.y = pos->y;
    cbparam.capsuleStart.z = pos->z;
    cbparam.capsuleEnd.x = cbparam.capsuleStart.x;
    cbparam.capsuleEnd.y = endY;
    cbparam.capsuleEnd.z = cbparam.capsuleStart.z;
    cbparam.capsuleRadius = r;

    xQuickCullForBox(&qcd, (xBox*)cbparam.isx);

    xGridCheckPosition(&colls_grid, (xVec3*)&isx, &qcd, shadowCacheEntityCB, &cbparam);
    xGridCheckPosition(&colls_oso_grid, (xVec3*)&isx, &qcd, shadowCacheEntityCB, &cbparam);
    xGridCheckPosition(&npcs_grid, (xVec3*)&isx, &qcd, shadowCacheEntityCB, &cbparam);

    cache->castOnEnt = cache->entCount != 0;
    cache->castOnPoly = cache->polyCount != 0;
}

void xShadowVertical_DrawCache(xShadowCache* cache, F32 shadowFactor, F32 fadeDist, S32 shadowMode,
                               RwMatrixTag* shadowMat, RwRaster* shadowRast)
{
    _ProjectionParam param;
    RpCollisionTriangle tri;
    RwV3d scl;
    RwV3d tr;
    xVec3 A;
    xVec3 B;
#if !defined(PS2)
    S32 fogstate;
#endif

    if (shadowRast != NULL)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, shadowRast);
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, ShadowCamera->frameBuffer);
    }

    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

    switch (shadowMode)
    {
    case 1:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
        break;
    case 0:
    default:
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDZERO);
        break;
    }

    if (shadowFactor < 0.0f)
    {
        shadowFactor = -shadowFactor;

        switch (shadowMode)
        {
        case 1:
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDINVSRCALPHA);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCALPHA);
            break;
        case 0:
        default:
#if defined(PS2)
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCALPHA);
#else
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDSRCCOLOR);
#endif
            break;
        }
    }
    else
    {
        switch (shadowMode)
        {
        case 1:
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
            break;
        case 0:
        default:
#if defined(PS2)
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
#else
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCCOLOR);
#endif
            break;
        }
    }

    RwMatrixTag* shadowMatrix;
    if (shadowMat != NULL)
    {
        shadowMatrix = shadowMat;
    }
    else
    {
        shadowMatrix = &((RwFrame*)ShadowCamera->object.object.parent)->modelling;
    }

    param.at = shadowMatrix->at;

    F32 radius = gShadowObjectRadius;

    RwMatrixInvert(&param.invMatrix, shadowMatrix);

    scl.x = scl.y = -0.5f / radius;
    scl.z = 1.0f / (fadeDist + radius);
    RwMatrixScale(&param.invMatrix, &scl, rwCOMBINEPOSTCONCAT);

    param.fade = (fadeDist > 0.0f) ? 1 : 0;

    param.shadowValue = (U8)(255.0f * shadowFactor);
    param.shadowWord = (param.shadowValue << 24) | (param.shadowValue << 16) |
                       (param.shadowValue << 8) | param.shadowValue;

    param.numIm3DBatch = 0;

    Im3DBuffer = gRenderBuffer.m_vertex;
    Im3DBufferPos = 0;

    xVec3Add(&A, (xVec3*)&shadowMatrix->pos, (xVec3*)&shadowMatrix->at);
    RwV3dTransformPoints((RwV3d*)&B, (RwV3d*)&A, 1, &param.invMatrix);

    tr.x = tr.y = 0.5f;
    tr.z = 0.0f;
    RwMatrixTranslate(&param.invMatrix, &tr, rwCOMBINEPOSTCONCAT);

#if defined(PS2)
    // The original callbacks use this persistent VU0 projection matrix.
    asm volatile("lqc2 vf28, 0(%0)\n"
                 "lqc2 vf29, 16(%0)\n"
                 "lqc2 vf30, 32(%0)\n"
                 "lqc2 vf31, 48(%0)"
                 : : "r"(&param.invMatrix) : "memory");
#else
    RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0);
#endif

    for (U32 i = 0; i < cache->polyCount; i++)
    {
        tri.normal = *(RwV3d*)&cache->poly[i].norm;
        tri.vertices[0] = (RwV3d*)&cache->poly[i].vert[0];
        tri.vertices[1] = (RwV3d*)&cache->poly[i].vert[1];
        tri.vertices[2] = (RwV3d*)&cache->poly[i].vert[2];

        ShadowRenderTriangleCB(NULL, NULL, &tri, 0.0f, &param);
    }

    if (Im3DBufferPos != 0)
    {
        if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                            rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
            RwIm3DEnd();
        }
        Im3DBufferPos = 0;
    }

#if !defined(PS2)
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);
#endif
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
}

void xShadowManager_Init(S32 numEnts)
{
    sMgrList = (xShadowMgr*)xMemAlloc(gActiveHeap, numEnts << 4, 0);
    sMgrTotal = numEnts;
    sMgrCount = 0; // Scheduling off
#if defined(PS2)
    a4dSkinPipe = RpPDSGetPipe(0x5000e);
    adlSkinPipe = RpPDSGetPipe(0x5000d);
    a4dSkinPipeADC = RpPDSGetPipe(0x5003e);
    adlSkinPipeADC = RpPDSGetPipe(0x5003d);
#endif
}

void xShadowManager_Reset()
{
    sMgrCount = 0;
}

#if defined(PS2)
#pragma dont_inline on
#endif
void xShadowManager_Add(xEnt* ent)
{
    for (int i = 0; i < sMgrCount; i++)
    {
        if (sMgrList[i].ent == ent)
        {
            return;
        }
    }

    if (sMgrCount < sMgrTotal)
    {
        sMgrList[sMgrCount].ent = ent;
        sMgrList[sMgrCount].cache = 0;
        sMgrList[sMgrCount].priority = 1000;
        sMgrList[sMgrCount].cacheReady = 0;
        sMgrCount++;
    }
}

#if defined(PS2)
#pragma dont_inline reset
#endif

void xShadowManager_Remove(xEnt* ent)
{
    int a = 0;
    for (int i = 6; i < sMgrCount; i++)
    {
        sMgrList[i].cache = NULL;
        a++;
    }

    a = 0;
    int i = 0;
    while (a < sMgrCount)
    {
        if (ent == sMgrList[i].ent)
        {
            sMgrList[i] = sMgrList[sMgrCount - 1];
            sMgrCount--;
        }
        else
        {
            i++;
            a++;
        }
    }
}

static S32 CmpShadowMgr(const void* a, const void* b)
{
    xEnt* entA = ((const xShadowMgr*)a)->ent;
    xEnt* entB = ((const xShadowMgr*)b)->ent;

    S32 isPlayerA =
        (entA->baseType == eBaseTypePlayer) || (entA->baseType == eBaseTypeBoulder);
    S32 isPlayerB =
        (entB->baseType == eBaseTypePlayer) || (entB->baseType == eBaseTypeBoulder);

    if (isPlayerA && !isPlayerB)
    {
        return -1;
    }
    if (isPlayerB && !isPlayerA)
    {
        return 1;
    }

    xVec3* campos = &globals.camera.mat.pos;

    F32 dxa = campos->x - entA->model->Mat->pos.x;
    F32 dya = campos->y - entA->model->Mat->pos.y;
    F32 dza = campos->z - entA->model->Mat->pos.z;
    F32 distA = dxa * dxa + dya * dya + dza * dza;

    F32 dxb = campos->x - entB->model->Mat->pos.x;
    F32 dyb = campos->y - entB->model->Mat->pos.y;
    F32 dzb = campos->z - entB->model->Mat->pos.z;
    F32 distB = dxb * dxb + dyb * dyb + dzb * dzb;

    if (distA < distB)
    {
        return -1;
    }
    return distA > distB;
}

void xShadowManager_Render()
{
    S32 i;
    S32 cacheUsed[6];
    U32 j;
    U8 old_xClumpColl_FilterFlags;
    S32 bestIndex;
    S32 foundPriority;

    old_xClumpColl_FilterFlags = xClumpColl_FilterFlags;
    xClumpColl_FilterFlags |= 0x20;

    for (i = 6; i < sMgrCount; i++)
    {
        sMgrList[i].cache = NULL;
    }

    i = 0;
    while (i < sMgrCount)
    {
        if (!xEntIsVisible(sMgrList[i].ent) || (sMgrList[i].ent->model->Flags & 0x400))
        {
            sMgrList[i] = sMgrList[sMgrCount - 1];
            sMgrCount--;
        }
        else
        {
            i++;
        }
    }

    qsort(sMgrList, sMgrCount, sizeof(xShadowMgr), CmpShadowMgr);

    memset(cacheUsed, 0, sizeof(cacheUsed));

    for (i = 0; (i < 6) && (i < sMgrCount); i++)
    {
        if (sMgrList[i].cache != NULL)
        {
            cacheUsed[sMgrList[i].cache - sCacheList] = 1;
        }
    }

    for (i = 0; (i < 6) && (i < sMgrCount); i++)
    {
        if (sMgrList[i].cache == NULL)
        {
            for (j = 0; j < 6; j++)
            {
                if (cacheUsed[j] == 0)
                {
                    sMgrList[i].cache = &sCacheList[j];
                    sMgrList[i].cacheReady = 0;
                    sMgrList[i].priority = 1000;
                    sCacheList[j].entCount = 0;
                    sCacheList[j].polyCount = 0;
                    cacheUsed[j] = 1;
                    break;
                }
            }
        }
    }

    for (i = 6; i < sMgrCount; i++)
    {
        sMgrList[i].cache = NULL;
    }

    bestIndex = -1;
    foundPriority = -1;

    for (i = 0; (i < 6) && (i < sMgrCount); i++)
    {
        sMgrList[i].priority++;
        if (sMgrList[i].priority > foundPriority)
        {
            bestIndex = i;
            foundPriority = sMgrList[i].priority;
        }
    }

    if (bestIndex != -1)
    {
        xVec3 center;
        F32 radius;
        F32 dst_depth;

        zEntGetShadowParams(sMgrList[bestIndex].ent, &center, &radius, xEntShadow::RADIUS_CACHE);

        xShadowMgr* mgr_best = &sMgrList[bestIndex];
        xEnt* ep = mgr_best->ent;

        dst_depth = 10.0f;
        if (ep->entShadow->dst_cast > 0.0f)
        {
            dst_depth = ep->entShadow->dst_cast;
        }

        sEntSelf = ep;

        xShadowVertical_FillCache(mgr_best->cache, &center, radius, dst_depth, 0.0871557f);

        sEntSelf = NULL;
        sMgrList[bestIndex].priority = 0;
        sMgrList[bestIndex].cacheReady = 1;

        xShadow_PickEntForNPC(mgr_best);
    }

    for (i = 0; i < sMgrCount; i++)
    {
        xEnt* ent = sMgrList[i].ent;
        S32 shadowOutside;
        xVec3 shadVec;

        shadVec.x = ent->model->Mat->pos.x;
        shadVec.y = ent->model->Mat->pos.y - 10.0f;
        shadVec.z = ent->model->Mat->pos.z;

        iModelCullPlusShadow(ent->model->Data, ent->model->Mat, &shadVec, &shadowOutside);

        if (shadowOutside != 0)
        {
            continue;
        }

        if ((sMgrList[i].cache != NULL) && (sMgrList[i].cacheReady != 0))
        {
            xVec3 center;
            F32 radius;

            ent->entShadow->pos.x = ent->model->Mat->pos.x;
            ent->entShadow->pos.y = 1.0f + ent->model->Mat->pos.y;
            ent->entShadow->pos.z = ent->model->Mat->pos.z;
            ent->entShadow->vec.x = 0.0f;
            ent->entShadow->vec.y = -1.0f;
            ent->entShadow->vec.z = 0.0f;

            xShadowSetLight(&ent->entShadow->pos, &ent->entShadow->vec, 1.0f);

            zEntGetShadowParams(ent, &center, &radius, xEntShadow::RADIUS_RASTER);

            RpAtomic* old_model = NULL;
            xModelInstance* old_mnext = ent->model->Next;

            if (ent->entShadow->shadowModel != NULL)
            {
                old_model = ent->model->Data;
                ent->model->Data = ent->entShadow->shadowModel;
                ent->model->Next = NULL;
            }

#if defined(PS2)
            {
                S32 material;
                RpMaterialList* list = &ent->model->Data->geometry->matList;
                for (material = 0; material < list->numMaterials; material++)
                {
                    if (list->materials[material]->pipeline == a4dSkinPipe)
                    {
                        list->materials[material]->pipeline = adlSkinPipe;
                    }
                    if (list->materials[material]->pipeline == a4dSkinPipeADC)
                    {
                        list->materials[material]->pipeline = adlSkinPipeADC;
                    }
                }
            }
#endif

            xShadowCameraUpdate(ent->model, (void (*)(void*))xModelRender, &center, radius, 0);

#if defined(PS2)
            {
                S32 material;
                RpMaterialList* list = &ent->model->Data->geometry->matList;
                for (material = 0; material < list->numMaterials; material++)
                {
                    if (list->materials[material]->pipeline == adlSkinPipe)
                    {
                        list->materials[material]->pipeline = a4dSkinPipe;
                    }
                    if (list->materials[material]->pipeline == adlSkinPipeADC)
                    {
                        list->materials[material]->pipeline = a4dSkinPipeADC;
                    }
                }
            }
#endif

            if (old_model != NULL)
            {
                ent->model->Data = old_model;
                ent->model->Next = old_mnext;
            }

#if defined(PS2)
            if (i == 0)
            {
                xShadowSetVolume(ShadowCamera, &center, 10.0f);
            }
#endif

            xShadowVertical_DrawCache(sMgrList[i].cache, ShadowStrength, 0.0f, 0, NULL, NULL);

            if ((i == 0) || (sMgrList[i].ent->baseType == eBaseTypeBoulder))
            {
                for (j = 0; j < sMgrList[i].cache->entCount; j++)
                {
                    xEnt* ep = sMgrList[i].cache->ent[j];
                    if (xShadowReceiveShadowSetup(ep))
                    {
                        xShadowReceiveShadow(ep, 0.3f, 0, NULL, NULL);
                    }
                }
            }
            else
            {
                xShadowMgr* mgr = &sMgrList[i];
                xNPCBasic* npc_base;

                if ((mgr->ent->baseType == eBaseTypeNPC) && (mgr->cache->entCount != 0) &&
                    (npc_base = (xNPCBasic*)mgr->ent, npc_base->flags1.flg_basenpc & 0x18))
                {
                    S32 num = 1;
                    if (npc_base->flags1.flg_basenpc & 0x10)
                    {
                        num = mgr->cache->entCount;
                    }

                    for (S32 a = 0; a < num; a++)
                    {
                        xEnt* ep = mgr->cache->ent[a];
                        if (xShadowReceiveShadowSetup(ep))
                        {
                            xShadowReceiveShadow(ep, 0.3f, 0, NULL, NULL);
                        }
                    }
                }
            }
#if defined(PS2)
            if (i == 0)
            {
                RpSkyRenderStateSet(rpSKYRENDERSTATEATEST_1, (void*)skyOldTest);
            }
#endif
        }
        else
        {
            F32 rad;

            if (ent->baseType == eBaseTypeBoulder)
            {
                rad = ent->model->Data->boundingSphere.radius;
                if (rad > 0.75f)
                {
                    rad = 0.75f;
                }
                rad *= 2.0f;
            }
            else if (ent->bound.type == XBOUND_TYPE_SPHERE)
            {
                rad = ent->bound.sph.r;
            }
            else
            {
                rad = 0.167f *
                      (ent->bound.box.box.upper.x + ent->bound.box.box.upper.y +
                       ent->bound.box.box.upper.z - ent->bound.box.box.lower.x -
                       ent->bound.box.box.lower.y - ent->bound.box.box.lower.z);
            }

            xShadowSimple_Add(ent->simpShadow, ent, rad, 1.0f);
        }
    }

    xClumpColl_FilterFlags = old_xClumpColl_FilterFlags;
}

static void xShadow_PickByRayCast(xShadowMgr* mgr)
{
    xEnt* ent_best = NULL;
    S32 idx_best = -1;
    xCollis colrec;
    xRay3 ray;

    memset(&colrec, 0, sizeof(colrec));

    ray.dir = g_NY3;
    ray.min_t = 0.0f;
    ray.max_t = 10.5f;
    ray.flags = 0xc00;

    S32 num = mgr->cache->entCount;
    for (S32 i = 0; i < num; i++)
    {
        xEnt* ep = mgr->cache->ent[i];

        colrec.flags = 0;
        colrec.flags |= 0x1f00;

        ray.origin.x = ep->model->Mat->pos.x;
        ray.origin.y = ep->model->Mat->pos.y;
        ray.origin.z = ep->model->Mat->pos.z;

        iRayHitsModel(&ray, ep->model, &colrec);

        if (!(colrec.flags & 1))
        {
            continue;
        }
        if (colrec.dist > 21.7f)
        {
            continue;
        }

        ent_best = ep;
        idx_best = i;
    }

    if (idx_best > 0)
    {
        mgr->cache->ent[idx_best] = mgr->cache->ent[0];
        mgr->cache->ent[0] = ent_best;
    }
}

#if defined(PS2)
#pragma dont_inline on
#endif
static void xShadow_PickEntForNPC(xShadowMgr* mgr)
{
    if (mgr->cache->entCount >= 2)
    {
        if ((mgr->ent->baseType == eBaseTypeNPC) &&
            (((xNPCBasic*)mgr->ent)->flags1.flg_basenpc & 0x8))
        {
            xShadow_PickByRayCast(mgr);
        }
    }
}

#if defined(PS2)
#pragma dont_inline reset
#endif
