#include "xFX.h"

#include "iFX.h"
#include "iParMgr.h"
#include "iMath.h"

#include "xDebug.h"
#include "xMathInlines.h"
#include "xstransvc.h"
#include "xScrFx.h"

#include "zEntPickup.h"
#include "zParEmitter.h"
#include "zSurface.h"
#include "zFX.h"
#include "zGlobals.h"
#include "zRumble.h"

#include <string.h>
#include <rpmatfx.h>
#include <rwplcore.h>
#include <rpskin.h>
#include <PowerPC_EABI_Support/MSL_C/MSL_Common/stdlib.h>

// no clue why this file is so out of order

/* boot.HIP texture IDs */
#define ID_gloss_edge 0xB8C2351E
#define ID_rainbowfilm_smooth32 0x741B0566

/* Global variables from .comm segment */
tier_queue_allocator xFXRibbon::joint_alloc;
xFXRing ringlist[RING_COUNT];
xFXStreak sStreakList[10];
xFXShine sShineList[2];

RpAtomicCallBackRender gAtomicRenderCallBack = NULL;
RpLight* MainLight = NULL;
F32 EnvMapShininess = 1.0f;

/* End global variables */

static U32 num_fx_atomics = 0;
static U32 xfx_initted = 0;

static void LightResetFrame(RpLight* light);

void xFXInit()
{
    if (!xfx_initted)
    {
        xfx_initted = 1;

        RpLight* light = RpLightCreate(rpLIGHTDIRECTIONAL);

        if (light)
        {
            RwFrame* frame = RwFrameCreate();

            if (frame)
            {
                RpLightSetFrame(light, frame);
                LightResetFrame(light);

                MainLight = light;
            }
            else
            {
                RpLightDestroy(light);
            }
        }

        xFXanimUVCreate();
        xFXAuraInit();
    }
}

static U32 Im3DBufferPos = 0;
static RwTexture* g_txtr_drawRing = NULL;

static xFXBubbleParams defaultBFX = {
    // pass1, pass2, pass3, padding
    1, 1, 1, 0,
    // pass1_alpha, pass2_alpha, pass3_alpha, pass1_fbsk
    0, 0, 192, 0xFFFFFFFF,
    // fresnel_map, fresnel_map_coeff
    ID_gloss_edge, 0.75f,
    // env_map, env_map_coeff
    ID_rainbowfilm_smooth32, 0.5f
};

static U32 bfx_curr = 0;
static xFXBubbleParams* BFX = &defaultBFX;

static U32 sFresnelMap = 0;
static U32 sEnvMap = 0;
static S32 sTweaked = 0;

static RxPipeline* xFXanimUVPipeline = NULL;
F32 xFXanimUVRotMat0[2] = { 1.0f, 0.0f };
F32 xFXanimUVRotMat1[2] = { 0.0f, 1.0f };
F32 xFXanimUVTrans[2] = { 0.0f, 0.0f };
F32 xFXanimUVScale[2] = { 1.0f, 1.0f };
F32 xFXanimUV2PRotMat0[2] = { 1.0f, 0.0f };
F32 xFXanimUV2PRotMat1[2] = { 0.0f, 1.0f };
F32 xFXanimUV2PTrans[2] = { 0.0f, 0.0f };
F32 xFXanimUV2PScale[2] = { 1.0f, 1.0f };
RwTexture* xFXanimUV2PTexture = NULL;

static void DrawRingSetup()
{
    g_txtr_drawRing = (RwTexture*)xSTFindAsset(ID_rainbowfilm_smooth32, NULL);
}

static void DrawRingSceneExit()
{
    g_txtr_drawRing = NULL;
}

static void DrawRing(xFXRing* m)
{
    RwCullMode cmode;
    xVec3 lprev;
    xVec3 lcur;
    xVec3 uprev;
    xVec3 ucur;
    S32 i;
    RwRaster* raster;
    U8 red;
    U8 green;
    U8 blue;
    U8 alpha;
    F32 u;
    F32 radius;
    F32 oradius;
    F32 height;
    F32 tilt;
    F32 dt;
    xVec3* center;
    RxObjSpace3DVertex* Im3DBuffer;
    RxObjSpace3DVertex* imv;
    F32 oour;

    if (m->time <= 0.0f)
    {
        return;
    }

    dt = m->time / m->lifetime;

    radius = m->ring_radius + m->ring_radius_delta * dt;
    height = m->ring_height + m->ring_height_delta * dt;
    tilt = m->ring_tilt + m->ring_tilt_delta * dt;

    oradius = radius + height * icos(tilt);
    height = height * isin(tilt);

    center = &m->pos;

    Im3DBuffer = gRenderBuffer.m_vertex;

    lprev.x = center->x;
    lprev.y = center->y;
    lprev.z = center->z + radius;
    uprev.x = center->x;
    uprev.y = center->y + height;
    uprev.z = center->z + oradius;

    if (g_txtr_drawRing)
    {
        raster = g_txtr_drawRing->raster;
    }
    else
    {
        raster = NULL;
    }

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)raster);
    RwRenderStateGet(rwRENDERSTATECULLMODE, &cmode);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);

    oour = (F32)m->u_repeat / m->ring_segs;

    for (i = 1; i <= m->ring_segs; i++)
    {
        if (Im3DBufferPos > 474)
        {
            if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                                rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
            {
                RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
                RwIm3DEnd();
            }

            Im3DBufferPos = 0;
        }

        imv = Im3DBuffer + Im3DBufferPos;

        if (i == m->ring_segs)
        {
            tilt = 0.0f;
        }
        else
        {
            tilt = 6.2831855f * i / m->ring_segs;
        }

        F32 si = isin(tilt);
        F32 co = icos(tilt);

        lcur.x = center->x + radius * si;
        lcur.y = center->y;
        lcur.z = center->z + radius * co;
        ucur.x = center->x + oradius * si;
        ucur.y = center->y + height;
        ucur.z = center->z + oradius * co;

        F32 fr = m->ring_color.r - m->ring_color.r * dt;
        F32 fg = m->ring_color.g - m->ring_color.g * dt;
        F32 fb = m->ring_color.b - m->ring_color.b * dt;
        F32 fa = m->ring_color.a - m->ring_color.a * dt;

        if (fr < 0.0f)
        {
            fr = 0.0f;
        }

        if (fg < 0.0f)
        {
            fg = 0.0f;
        }

        if (fb < 0.0f)
        {
            fb = 0.0f;
        }

        if (fa < 0.0f)
        {
            fa = 0.0f;
        }

        red = fr;
        green = fg;
        blue = fb;
        alpha = fa * (1.0f - dt);

        u = i * oour;

        F32 v0x = lprev.x;
        F32 v0y = lprev.y;
        F32 v0z = lprev.z;
        RwIm3DVertexSetPos(&imv[0], v0x, v0y, v0z);
        F32 v1x = uprev.x;
        F32 v1y = uprev.y;
        F32 v1z = uprev.z;
        RwIm3DVertexSetPos(&imv[1], v1x, v1y, v1z);
        F32 v2x = lcur.x;
        F32 v2y = lcur.y;
        F32 v2z = lcur.z;
        RwIm3DVertexSetPos(&imv[2], v2x, v2y, v2z);
        F32 v3x = uprev.x;
        F32 v3y = uprev.y;
        F32 v3z = uprev.z;
        RwIm3DVertexSetPos(&imv[3], v3x, v3y, v3z);
        F32 v4x = lcur.x;
        F32 v4y = lcur.y;
        F32 v4z = lcur.z;
        RwIm3DVertexSetPos(&imv[4], v4x, v4y, v4z);
        F32 v5x = ucur.x;
        F32 v5y = ucur.y;
        F32 v5z = ucur.z;
        RwIm3DVertexSetPos(&imv[5], v5x, v5y, v5z);

        imv[0].u = u;
        imv[1].u = u;
        imv[2].u = u + oour;
        imv[3].u = u;
        imv[4].u = u + oour;
        imv[5].u = u + oour;

        imv[0].v = 0.0f;
        imv[1].v = 0.999f;
        imv[2].v = 0.0f;
        imv[3].v = 0.999f;
        imv[4].v = 0.0f;
        imv[5].v = 0.999f;

        RwIm3DVertexSetRGBA(&imv[0], red, green, blue, alpha);
        RwIm3DVertexSetRGBA(&imv[1], red, green, blue, alpha);
        RwIm3DVertexSetRGBA(&imv[2], red, green, blue, alpha);
        RwIm3DVertexSetRGBA(&imv[3], red, green, blue, alpha);
        RwIm3DVertexSetRGBA(&imv[4], red, green, blue, alpha);
        RwIm3DVertexSetRGBA(&imv[5], red, green, blue, alpha);

        Im3DBufferPos += 6;

        lprev = lcur;
        uprev = ucur;
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

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)cmode);
}

xFXRing* xFXRingCreate(const xVec3* pos, const xFXRing* params)
{
    xFXRing* ring = &ringlist[0];

    if (!pos || !params)
    {
        return NULL;
    }

    for (S32 i = 0; i < RING_COUNT; i++, ring++)
    {
        if (ring->time <= 0.0f)
        {
            // non-matching: our compiler hoists the `lfs 0.0f` of the `ring->time <=
            // 0.0f` test out of the loop; retail reloads it every iteration. The three
            // `1.0f / ring->lifetime` divides are not CSE'd in either object.

            memcpy(ring, params, sizeof(xFXRing));

            ring->time = 0.001f;
            ring->pos = *pos;
            ring->ring_radius_delta *= 1.0f / ring->lifetime;
            ring->ring_height_delta *= 1.0f / ring->lifetime;
            ring->ring_tilt_delta *= 1.0f / ring->lifetime;

            return ring;
        }
    }

    return NULL;
}

static void xFXRingUpdate(F32 dt)
{
    xFXRing* ring = &ringlist[0];

    if ((F32)iabs(dt) < 0.001f)
    {
        return;
    }

    for (S32 i = 0; i < RING_COUNT; i++, ring++)
    {
        if (ring->time <= 0.0f)
        {
            continue;
        }

        F32 lifetime = ring->lifetime;

        if (lifetime < dt)
        {
            lifetime = dt;
        }

        ring->time += dt;

        F32 t = ring->time / lifetime;

        // non-matching: float scheduling

        if (t > 1.0f)
        {
            ring->time = 0.0f;

            if (ring->parent)
            {
                *ring->parent = NULL;
            }

            ring->parent = NULL;
        }
    }
}

void xFXRingRender()
{
    S32 i;
    xFXRing* ring = &ringlist[0];

    for (i = 0; i < RING_COUNT; i++, ring++)
    {
        if (ring->time > 0.0f)
        {
            DrawRing(ring);
        }
    }
}

static RpMaterial* MaterialSetEnvMap(RpMaterial* material, void* data);
static RpMaterial* MaterialSetBumpMap(RpMaterial* material, void* data);
static RpMaterial* MaterialSetBumpEnvMap(RpMaterial* material, RwTexture* env, F32 shininess,
                                         RwTexture* bump, F32 bumpiness);

void xFX_SceneEnter(RpWorld* world)
{
    S32 i;
    S32 num = RpWorldGetNumMaterials(world);

    for (i = 0; i < num; i++)
    {
        xSurface* sp = zSurfaceGetSurface(i);
        zSurfaceProps* pp = (zSurfaceProps*)sp->moprops;

        if (pp && pp->asset)
        {
            zSurfMatFX* fxp = &pp->asset->matfx;

            if (fxp->flags)
            {
                if (fxp->flags == 0x10)
                {
                    fxp->flags |= 0x1;
                }

                RpMaterial* mp = RpWorldGetMaterial(world, i);

                if (RpMaterialGetTexture(mp))
                {
                    gFXSurfaceFlags = fxp->flags;

                    if (fxp->flags & 0x1)
                    {
                        RwTexture* env = (RwTexture*)xSTFindAsset(fxp->envmapID, NULL);

                        if (!env)
                        {
                            continue;
                        }

                        MaterialSetEnvMap(mp, env);
                        RpMatFXMaterialSetEnvMapCoefficient(mp, 0.5f * fxp->shininess);
                    }

                    if (fxp->flags & 0x2)
                    {
                        RwTexture* bump = (RwTexture*)xSTFindAsset(fxp->bumpmapID, NULL);

                        if (!bump)
                        {
                            continue;
                        }

                        MaterialSetBumpMap(mp, bump);
                        RpMatFXMaterialSetBumpMapCoefficient(mp, fxp->bumpiness);
                    }

                    if (fxp->flags & 0x4)
                    {
                        RwTexture* env = (RwTexture*)xSTFindAsset(fxp->envmapID, NULL);
                        RwTexture* bump = (RwTexture*)xSTFindAsset(fxp->bumpmapID, NULL);

                        if (!env || !bump)
                        {
                            continue;
                        }

                        MaterialSetBumpEnvMap(mp, env, fxp->shininess, bump, fxp->bumpiness);
                    }
                }
            }
        }
    }

    zScene* sc = globals.sceneCur;

    for (i = 0; i < sc->num_act_ents; i++)
    {
        xEnt* ent = sc->act_ents[i];

        if (!gAtomicRenderCallBack && ent->model)
        {
            RpAtomicCallBackRender tmp = RpAtomicGetRenderCallBack(ent->model->Data);

            RpAtomicSetRenderCallBack(ent->model->Data, NULL);

            gAtomicRenderCallBack = RpAtomicGetRenderCallBack(ent->model->Data);

            RpAtomicSetRenderCallBack(ent->model->Data, tmp);
        }

        if (ent->model)
        {
            U32 bubble = 0;

            bubble |= (ent->id == xStrHash("bubble buddy"));
            bubble |= (ent->id == xStrHash("bubble missile"));
            bubble |= (ent->id == xStrHash("bubble helmet"));
            bubble |= (ent->id == xStrHash("bubble bowling ball"));
            bubble |= (ent->id == xStrHash("bubble shoeL"));
            bubble |= (ent->id == xStrHash("bubble shoeR"));

            if (bubble)
            {
                xSTAssetName(ent->id);

                RpAtomicSetRenderCallBack(ent->model->Data, xFXBubbleRender);
            }
        }
    }

    num_fx_atomics = 0;
}

void xFX_SceneExit(RpWorld*)
{
}

void xFXUpdate(F32 dt)
{
    xFXRingUpdate(dt);
    xFXRibbonUpdate(dt);
    xFXAuraUpdate(dt);
}

static void LightResetFrame(RpLight* light)
{
    RwV3d v1 = { 1, 0, 0 };
    RwV3d v2 = { 0, 1, 0 };

    RwFrame* frame = RpLightGetFrame(light);

    RwFrameRotate(frame, &v1, 45.0f, rwCOMBINEREPLACE);
    RwFrameRotate(frame, &v2, 45.0f, rwCOMBINEPOSTCONCAT);
}

static RpMaterial* MaterialDisableMatFX(RpMaterial* material, void*)
{
    RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
    return material;
}

RpAtomic* AtomicDisableMatFX(RpAtomic* atomic)
{
    RpMatFXAtomicEnableEffects(atomic);

    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    if (geometry)
    {
        RpGeometryForAllMaterials(geometry, MaterialDisableMatFX, NULL);
    }

    return atomic;
}

static RpAtomic* PreAllocMatFX_cb(RpAtomic* atomic, void*)
{
    AtomicDisableMatFX(atomic);
    return atomic;
}

void xFXPreAllocMatFX(RpClump* clump)
{
    RpClumpForAllAtomics(clump, PreAllocMatFX_cb, NULL);
}

RpMaterial* MaterialSetShininess(RpMaterial* material, void*)
{
    RpMatFXMaterialFlags flags = RpMatFXMaterialGetEffects(material);

    if (flags == rpMATFXEFFECTENVMAP || flags == rpMATFXEFFECTBUMPENVMAP)
    {
        RpMatFXMaterialSetEnvMapCoefficient(material, EnvMapShininess);
    }

    return material;
}

static RpAtomic* AtomicSetShininess(RpAtomic* atomic, void* data)
{
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    if (geometry)
    {
        RpGeometryForAllMaterials(geometry, MaterialSetShininess, data);
    }

    return atomic;
}

RpMaterial* MaterialSetEnvMap(RpMaterial* material, void* data)
{
    // Matching but with mr. instead of cmplwi
    if (data == NULL)
    {
        return NULL;
    }
    if (material->texture)
    {
        RwTexture* texture = (RwTexture*)data;

        if (texture)
        {
            RwFrame* frame = NULL;
            if ((gFXSurfaceFlags & 0x10) != 0)
            {
                if (globals.camera.lo_cam)
                {
                    frame = (RwFrame*)globals.camera.lo_cam->object.object.parent;
                }
                else
                {
                    frame = (RwFrame*)MainLight->object.object.parent;
                }
            }
            else
            {
                frame = (RwFrame*)MainLight->object.object.parent;
            }
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTENVMAP);
            RpMatFXMaterialSetupEnvMap(material, texture, frame, FALSE, 1.0f);
        }
        else
        {
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
        }
    }
    return material;
}

RpMaterial* MaterialSetEnvMap2(RpMaterial* material, void* data)
{
    if (material->texture != NULL)
    {
        RwTexture* texture = (RwTexture*)data;
        RwFrame* frame;
        if (RwEngineInstance->stringFuncs.vecStrcmp(texture->name, "spec3") == 0)
        {
            frame = (RwFrame*)globals.camera.lo_cam->object.object.parent;
        }
        else
        {
            frame = (RwFrame*)MainLight->object.object.parent;
        }
        RpMatFXMaterialSetEffects(material, rpMATFXEFFECTENVMAP);
        RpMatFXMaterialSetupEnvMap(material, texture, frame, FALSE, EnvMapShininess);
    }
    return material;
}

RpAtomic* xFXBubbleRender(RpAtomic* atomic)
{
    RwCullMode cmode;
    xFXBubbleParams* bp;

    bp = BFX + bfx_curr * 1; // Why multiply by 1? Probably needs rewritten differently

    RwRenderStateGet(rwRENDERSTATECULLMODE, &cmode);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
    iDrawSetFBMSK(bp->pass1_fbmsk);
    iModelSetMaterialAlpha(atomic, bp->pass1_alpha);

    if (((char)bp->pass1))
    {
        AtomicDisableMatFX(atomic);
        (*gAtomicRenderCallBack)(atomic);
    }

    iDrawSetFBMSK(0);
    iModelSetMaterialAlpha(atomic, bp->pass2_alpha);

    if (((char)bp->pass2) != 0)
    {
        gFXSurfaceFlags = 0x10;
        xFXAtomicEnvMapSetup(atomic, bp->fresnel_map, bp->fresnel_map_coeff);
        gFXSurfaceFlags = 0;
        (*gAtomicRenderCallBack)(atomic);
    }

    iModelSetMaterialAlpha(atomic, bp->pass3_alpha);

    if (((char)bp->pass3) != 0)
    {
        AtomicDisableMatFX(atomic);
        gFXSurfaceFlags = 0x10;
        xFXAtomicEnvMapSetup(atomic, bp->env_map, bp->env_map_coeff);
        gFXSurfaceFlags = 0;
        (*gAtomicRenderCallBack)(atomic);
    }

    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)cmode);

    return atomic;
}

RpAtomic* xFXShinyRender(RpAtomic* atomic)
{
    RwCullMode cmode;

    if (sTweaked == 0)
    {
        sEnvMap = xStrHash("default_env_map.RW3");
        sFresnelMap = xStrHash("gloss_edge");
        sTweaked = 1;
    }

    RwRenderStateGet(rwRENDERSTATECULLMODE, &cmode);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLBACK);
    iDrawSetFBMSK(-1);
    iModelSetMaterialAlpha(atomic, 255);
    AtomicDisableMatFX(atomic);
    (*gAtomicRenderCallBack)(atomic);
    iDrawSetFBMSK(0);
    iModelSetMaterialAlpha(atomic, 0);
    gFXSurfaceFlags = 0x10;
    xFXAtomicEnvMapSetup(atomic, sFresnelMap, 0.0f);
    gFXSurfaceFlags = 0;
    (*gAtomicRenderCallBack)(atomic);
    iModelSetMaterialAlpha(atomic, 0xff);
    AtomicDisableMatFX(atomic);
    gFXSurfaceFlags = 0x10;
    xFXAtomicEnvMapSetup(atomic, sEnvMap, 1.0f);
    gFXSurfaceFlags = 0;
    (*gAtomicRenderCallBack)(atomic);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)cmode);

    return atomic;
}

static RpAtomic* AtomicSetEnvMap(RpAtomic* atomic, void* data)
{
    RpMatFXAtomicEnableEffects(atomic);
    if (atomic->geometry != 0)
    {
        RpGeometryForAllMaterials(atomic->geometry, MaterialSetEnvMap2, data);
    }
    return atomic;
}

RpAtomic* xFXAtomicEnvMapSetup(RpAtomic* atomic, U32 envmapID, F32 shininess)
{
    void* env = xSTFindAsset(envmapID, NULL);
    if (env)
    {
        AtomicSetEnvMap(atomic, env);
        F32 tmp = EnvMapShininess;
        EnvMapShininess = shininess;
        AtomicSetShininess(atomic, NULL);
        EnvMapShininess = tmp;
        RpSkin* skin = RpSkinGeometryGetSkin(atomic->geometry);
        if (skin)
        {
            RpSkinAtomicSetType(atomic, rpSKINTYPEMATFX);
        }
        return atomic;
    }
    return NULL;
}

RpMaterial* MaterialSetBumpMap(RpMaterial* material, void* data)
{
    if (data == NULL)
    {
        return NULL;
    }
    else if (material->texture)
    {
        RwTexture* texture = (RwTexture*)data;

        if (texture)
        {
            RwFrame* frame = (RwFrame*)MainLight->object.object.parent;
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTBUMPMAP);
            RpMatFXMaterialSetupBumpMap(material, texture, frame, 1.0f);
        }
        else
        {
            RpMatFXMaterialSetEffects(material, rpMATFXEFFECTNULL);
        }
    }
    return material;
}

RpMaterial* MaterialSetBumpEnvMap(RpMaterial* material, RwTexture* envMap, F32 envCooef,
                                  RwTexture* bumpMap, F32 bumpCooef)
{
    if (envMap == NULL || bumpMap == NULL)
    {
        return NULL;
    }
    else
    {
        RwFrame* frame;
        RpMatFXMaterialSetEffects(material, rpMATFXEFFECTBUMPENVMAP);
        if ((gFXSurfaceFlags & 0x10) != 0)
        {
            frame = (RwFrame*)globals.camera.lo_cam->object.object.parent;
        }
        else
        {
            frame = (RwFrame*)MainLight->object.object.parent;
        }
        RpMatFXMaterialSetupEnvMap(material, envMap, frame, TRUE, envCooef);
        RpMatFXMaterialSetupBumpMap(material, bumpMap, (RwFrame*)MainLight->object.object.parent,
                                    bumpCooef);
    }
    return material;
}

void xFXanimUVSetTranslation(const xVec3* trans)
{
    xFXanimUVTrans[0] = trans->x;
    xFXanimUVTrans[1] = trans->y;
}

void xFXanimUVSetScale(const xVec3* scale)
{
    xFXanimUVScale[0] = scale->x;
    xFXanimUVScale[1] = scale->y;
}

void xFXanimUVSetAngle(F32 angle)
{
    F32 sin = isin(angle);
    F32 cos = icos(angle);
    xFXanimUVRotMat0[0] = cos;
    xFXanimUVRotMat0[1] = -sin;
    xFXanimUVRotMat1[0] = sin;
    xFXanimUVRotMat1[1] = cos;
}

void xFXanimUV2PSetTranslation(const xVec3* trans)
{
    xFXanimUV2PTrans[0] = trans->x;
    xFXanimUV2PTrans[1] = trans->y;
}

void xFXanimUV2PSetScale(const xVec3* scale)
{
    xFXanimUV2PScale[0] = scale->x;
    xFXanimUV2PScale[1] = scale->y;
}

void xFXanimUV2PSetAngle(F32 angle)
{
    F32 sin = isin(angle);
    F32 cos = icos(angle);
    xFXanimUV2PRotMat0[0] = cos;
    xFXanimUV2PRotMat0[1] = -sin;
    xFXanimUV2PRotMat1[0] = sin;
    xFXanimUV2PRotMat1[1] = cos;
}

void xFXanimUV2PSetTexture(RwTexture* texture)
{
    xFXanimUV2PTexture = texture;
}

RpAtomic* xFXanimUVAtomicSetup(RpAtomic* atomic)
{
    if (atomic == 0)
    {
        return atomic;
    }
    if (xFXanimUVPipeline == 0)
    {
        return atomic;
    }
    atomic->pipeline = xFXanimUVPipeline;
    return atomic;
}

U32 xFXanimUVCreate()
{
    if (xFXanimUVPipeline == NULL)
    {
        xFXanimUVPipeline = iFXanimUVCreatePipe();
    }
    return (-(U32)xFXanimUVPipeline | (U32)xFXanimUVPipeline) >> 0x1f;
}

namespace
{
    struct vert_data
    {
        xVec3 loc;
        xVec3 norm;
        RwRGBA color;
        RwTexCoords uv;
        F32 depth;
    };

    struct tri_data
    {
        vert_data vert[3];

        void init(const xVec3* loc, const xVec3* norm, const RwTexCoords* uv, const F32* depth,
                  const U16* index);
    };

    // Every overload returns void: each one is byte-identical to retail as
    // written, and a non-void return would have to materialise a result here
    // because no caller uses one. Only the vert_data overload survives in the
    // dwarf; the parameter names of the rest follow it.
    void lerp(vert_data& v, F32 frac, const vert_data& v0, const vert_data& v1);
    void lerp(RwTexCoords& v, F32 frac, const RwTexCoords& v0, const RwTexCoords& v1);
    void lerp(F32& v, F32 frac, F32 v0, F32 v1);
    void lerp(RwRGBA& v, F32 frac, RwRGBA v0, RwRGBA v1);
    void lerp(U8& v, F32 frac, U8 v0, U8 v1);
    void lerp(xVec3& v, F32 frac, const xVec3& v0, const xVec3& v1);

    void set_vert(RxObjSpace3DVertex& vert, const vert_data& vd);
    void set_vert(RxObjSpace3DVertex& vert, const xVec3& loc, const xVec3& norm,
                  const RwTexCoords& uv, U8 alpha);
    void push_triangle(RxObjSpace3DVertex*& vert, const tri_data& tri);
    S32 clip_triangle(tri_data* out, const tri_data& in, F32 depth);
    void refresh_vert_buffer(RxObjSpace3DVertex*& vert, bool flush);
    U32 count_alpha_triangles(const RpTriangle* tri, const F32* depth, u32 size);
    void depth_sort(U16* index, const tri_data* tri, u32 size);

#define ALPHA_COUNT 300

    U8 alpha_count0[ALPHA_COUNT];
    U8 alpha_count1[ALPHA_COUNT];

    void depth_sort(U16* index, const tri_data* tri, u32 size)
    {
        for (U32 i = 0; i < size; i++)
        {
            U16& e0 = index[i];
            F32 d0 = tri[e0].vert[0].depth;

            for (U32 j = i + 1; j < size; j++)
            {
                if (tri[index[j]].vert[0].depth > d0)
                {
                    U16 temp = e0;

                    e0 = index[j];
                    index[j] = temp;
                }
            }
        }
    }
} // namespace

void xFXRenderProximityFade(const xModelInstance& model, F32 near_dist, F32 far_dist)
{
    RpGeometry* geom;
    RwRaster* raster;
    RpTriangle* tri;
    RwFrame* frame;
    RwTexCoords* uv;
    S32 vert_total;
    xVec3* vert;
    xVec3* normal;
    U8* alpha;
    F32* depth;
    xMat4x3* cm;
    xVec3 ov;
    F32 zfrac;
    S32 i;
    F32 a;
    RxObjSpace3DVertex* out_vert;
    S32 tri_total;
    U16* alpha_tri_index;
    tri_data* alpha_tri;
    U32 alpha_tri_total;
    U32 alpha_tri_used;
    tri_data tri_buffer[2][3];
    tri_data cur_tri;
    RpTriangle* end;
    S32 tri_index;

    memset(alpha_count0, 0, sizeof(alpha_count0));
    memset(alpha_count1, 0, sizeof(alpha_count1));

    geom = model.Data->geometry;
    raster = geom->matList.materials[0]->texture->raster;
    tri = geom->triangles;
    frame = RwCameraGetFrame(RwCameraGetCurrentCamera());
    uv = geom->texCoords[0];
    vert_total = geom->numVertices;

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)raster);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

    vert = (xVec3*)xMemPushTemp(vert_total * sizeof(xVec3));
    normal = (xVec3*)xMemPushTemp(vert_total * sizeof(xVec3));

    iModelVertEval(model.Data, 0, vert_total, model.Mat, NULL, vert);
    iModelNormalEval(normal, *model.Data, model.Mat, 0, -1, NULL);

    alpha = (U8*)xMemPushTemp(vert_total * sizeof(U8));
    depth = (F32*)xMemPushTemp(vert_total * sizeof(F32));

    cm = (xMat4x3*)RwFrameGetLTM(frame);

    if (near_dist >= far_dist)
    {
        zfrac = 1.0f;
    }
    else
    {
        zfrac = 1.0f / (far_dist - near_dist);
    }

    for (i = 0; i < vert_total; i++)
    {
        xMat4x3Tolocal(&ov, cm, &vert[i]);

        depth[i] = zfrac * (ov.z - near_dist);

        a = 255.0f * depth[i];

        if (a < 0.0f)
        {
            alpha[i] = 0;
        }
        else if (a >= 255.0f)
        {
            alpha[i] = 255;
        }
        else
        {
            alpha[i] = 0.5f + a;
        }
    }

    out_vert = gRenderBuffer.m_vertex;

    alpha_tri_index = NULL;
    alpha_tri = NULL;
    alpha_tri_used = 0;

    tri_total = geom->numTriangles;

    alpha_tri_total = count_alpha_triangles(tri, depth, tri_total);

    if (alpha_tri_total == 0)
    {
        // Retail as-is: vert_total is never negative, so this early-out is dead code.
        if (vert_total < 0 && depth[0] < 0.0f)
        {
            goto cleanup;
        }
    }
    else
    {
        alpha_tri_index = (U16*)xMemPushTemp(alpha_tri_total * sizeof(U16));
        alpha_tri = (tri_data*)xMemPushTemp(alpha_tri_total * sizeof(tri_data));
    }

    end = tri + tri_total;
    tri_index = 0;

    while (tri != end)
    {
        U16 vi[3];
        F32 d0;
        F32 d1;
        F32 d2;
        U32 flags;
        S32 j;
        S32 size1;
        S32 k;
        S32 size2;

        refresh_vert_buffer(out_vert, false);

        vi[0] = tri->vertIndex[0];
        vi[1] = tri->vertIndex[1];
        vi[2] = tri->vertIndex[2];

        flags = 0;

        d0 = depth[vi[0]];
        d1 = depth[vi[1]];
        d2 = depth[vi[2]];

        if (d0 < 0.0f)
        {
            flags |= 0x1;
        }
        else if (d0 > 1.0f)
        {
            flags |= 0x40;
        }
        else
        {
            flags |= 0x8;
        }

        if (d1 < 0.0f)
        {
            flags |= 0x2;
        }
        else if (d1 > 1.0f)
        {
            flags |= 0x80;
        }
        else
        {
            flags |= 0x10;
        }

        if (d2 < 0.0f)
        {
            flags |= 0x4;
        }
        else if (d2 > 1.0f)
        {
            flags |= 0x100;
        }
        else
        {
            flags |= 0x20;
        }

        if ((flags & 0x38) == 0x38 || (flags & 0x1c0) == 0x1c0)
        {
            if (alpha[vi[0]])
            {
                set_vert(*out_vert++, vert[vi[0]], normal[vi[0]], uv[vi[0]], alpha[vi[0]]);
                set_vert(*out_vert++, vert[vi[1]], normal[vi[1]], uv[vi[1]], alpha[vi[1]]);
                set_vert(*out_vert++, vert[vi[2]], normal[vi[2]], uv[vi[2]], alpha[vi[2]]);
            }
        }
        else if ((flags & 0x7) != 0x7)
        {
            cur_tri.init(vert, normal, uv, depth, vi);

            size1 = clip_triangle(tri_buffer[0], cur_tri, 0.0f);

            for (j = 0; j < size1; j++)
            {
                if (tri_buffer[0][j].vert[0].depth <= 0.0f &&
                    tri_buffer[0][j].vert[1].depth <= 0.0f &&
                    tri_buffer[0][j].vert[2].depth <= 0.0f)
                {
                    continue;
                }

                size2 = clip_triangle(tri_buffer[1], tri_buffer[0][j], 1.0f);

                for (k = 0; k < size2; k++)
                {
                    if (tri_buffer[1][k].vert[0].depth >= 1.0f &&
                        tri_buffer[1][k].vert[1].depth >= 1.0f &&
                        tri_buffer[1][k].vert[2].depth >= 1.0f)
                    {
                        push_triangle(out_vert, tri_buffer[1][k]);
                    }
                    else
                    {
                        alpha_count1[tri_index]++;

                        alpha_tri_index[alpha_tri_used] = alpha_tri_used;
                        alpha_tri[alpha_tri_used] = tri_buffer[1][k];
                        alpha_tri_used++;
                    }
                }
            }
        }

        tri++;
        tri_index++;
    }

    depth_sort(alpha_tri_index, alpha_tri, alpha_tri_used);

    {
        U32 n;

        for (n = 0; n < alpha_tri_used; n++)
        {
            refresh_vert_buffer(out_vert, false);
            push_triangle(out_vert, alpha_tri[alpha_tri_index[n]]);
        }
    }

    refresh_vert_buffer(out_vert, true);

    if (alpha_tri_total)
    {
        xMemPopTemp(alpha_tri);
        xMemPopTemp(alpha_tri_index);
    }

cleanup:
    xMemPopTemp(depth);
    xMemPopTemp(alpha);
    xMemPopTemp(normal);
    xMemPopTemp(vert);
}

namespace
{
    void push_triangle(RxObjSpace3DVertex*& vert, const tri_data& tri)
    {
        for (S32 i = 0; i < 3; i++)
        {
            set_vert(*vert, tri.vert[i]);
            vert++;
        }
    }

    void set_vert(RxObjSpace3DVertex& vert, const vert_data& vd)
    {
        U8 alpha;
        F32 a;

        a = 255.0f * vd.depth;

        if (a < 0.0f)
        {
            alpha = 0;
        }
        else if (a >= 255.0f)
        {
            alpha = 255;
        }
        else
        {
            alpha = 0.5f + a;
        }

        F32 lx = vd.loc.x;
        F32 ly = vd.loc.y;
        F32 lz = vd.loc.z;
        RwIm3DVertexSetPos(&vert, lx, ly, lz);
        RwIm3DVertexSetNormal(&vert, vd.norm.x, vd.norm.y, vd.norm.z);
        RwIm3DVertexSetRGBA(&vert, 255, 255, 255, alpha);
        RwIm3DVertexSetUV(&vert, vd.uv.u, vd.uv.v);
    }

    S32 clip_triangle(tri_data* out, const tri_data& in, F32 depth)
    {
        U16 flags;
        U8 i0;
        U8 i1;
        U8 i2;

        flags = 0;

        flags |= (in.vert[0].depth < depth) ? 0x1 : 0x8;
        flags |= (in.vert[1].depth < depth) ? 0x2 : 0x10;
        flags |= (in.vert[2].depth < depth) ? 0x4 : 0x20;

        if ((in.vert[0].depth == depth && (flags & 0x6)) ||
            (in.vert[1].depth == depth && (flags & 0x5)) ||
            (in.vert[2].depth == depth && (flags & 0x3)))
        {
            out[0] = in;
            return 1;
        }

        switch (flags)
        {
        case 0x0e:
        case 0x31:
            i0 = 0;
            i1 = 1;
            i2 = 2;
            break;
        case 0x15:
        case 0x2a:
            i0 = 1;
            i1 = 2;
            i2 = 0;
            break;
        case 0x1c:
        case 0x23:
            i0 = 2;
            i1 = 0;
            i2 = 1;
            break;
        default:
            out[0] = in;
            return 1;
        }

        F32 d0 = in.vert[i0].depth;
        F32 d1 = in.vert[i1].depth;
        F32 d2 = in.vert[i2].depth;

        out[0].vert[0] = in.vert[i0];

        lerp(out[0].vert[1], (depth - d0) / (d1 - d0), in.vert[i0], in.vert[i1]);
        lerp(out[0].vert[2], (depth - d0) / (d2 - d0), in.vert[i0], in.vert[i2]);

        out[0].vert[2].depth = depth;
        out[0].vert[1].depth = depth;

        out[1].vert[0] = out[0].vert[1];
        out[1].vert[1] = in.vert[i1];
        out[1].vert[2] = in.vert[i2];

        out[2].vert[0] = in.vert[i2];
        out[2].vert[1] = out[0].vert[2];
        out[2].vert[2] = out[0].vert[1];

        return 3;
    }

    void lerp(vert_data& v, F32 frac, const vert_data& v0, const vert_data& v1)
    // Yes, These are the actual parameter names for this function from the dwarf
    {
        lerp(v.loc, frac, v0.loc, v1.loc);
        lerp(v.norm, frac, v0.norm, v1.norm);
        lerp(v.color, frac, v0.color, v1.color);
        lerp(v.uv, frac, v0.uv, v1.uv);
    }

    void lerp(RwTexCoords& v, F32 frac, const RwTexCoords& v0, const RwTexCoords& v1)
    {
        lerp(v.u, frac, v0.u, v1.u);
        lerp(v.v, frac, v0.v, v1.v);
    }

    void lerp(F32& v, F32 frac, F32 v0, F32 v1)
    {
        v = v0 + (v1 - v0) * frac;
    }

    void lerp(RwRGBA& v, F32 frac, RwRGBA v0, RwRGBA v1)
    {
        lerp(v.red, frac, v0.red, v1.red);
        lerp(v.green, frac, v0.green, v1.green);
        lerp(v.blue, frac, v0.blue, v1.blue);
        lerp(v.alpha, frac, v0.alpha, v1.alpha);
    }

    void lerp(U8& v, F32 frac, U8 v0, U8 v1)
    {
        v = 0.5f + ((F32)v0 + ((F32)v1 - (F32)v0) * frac);
    }

    void lerp(xVec3& v, F32 frac, const xVec3& v0, const xVec3& v1)
    {
        lerp(v.x, frac, v0.x, v1.x);
        lerp(v.y, frac, v0.y, v1.y);
        lerp(v.z, frac, v0.z, v1.z);
    }

    void tri_data::init(const xVec3* loc, const xVec3* norm, const RwTexCoords* uv,
                        const F32* depth, const U16* index)
    {
        for (S32 i = 0; i < 3; i++)
        {
            vert_data& v = vert[i];
            U16 vi = index[i];

            v.loc = loc[vi];
            v.norm = norm[vi];
            v.color.red = v.color.green = v.color.blue = v.color.alpha = 255;
            v.uv = uv[vi];
            v.depth = depth[vi];
        }
    }

    void set_vert(RxObjSpace3DVertex& vert, const xVec3& loc, const xVec3& norm,
                  const RwTexCoords& uv, U8 alpha)
    {
        F32 lx = loc.x;
        F32 ly = loc.y;
        F32 lz = loc.z;
        RwIm3DVertexSetPos(&vert, lx, ly, lz);
        RwIm3DVertexSetNormal(&vert, norm.x, norm.y, norm.z);
        RwIm3DVertexSetRGBA(&vert, 255, 255, 255, alpha);
        RwIm3DVertexSetUV(&vert, uv.u, uv.v);
    }

    void refresh_vert_buffer(RxObjSpace3DVertex*& vert, bool flush)
    {
        S32 count = vert - gRenderBuffer.m_vertex;

        if (!flush && count < 492)
        {
            return;
        }

        if (count == 0)
        {
            return;
        }

        vert = gRenderBuffer.m_vertex;

        if (RwIm3DTransform(vert, count, NULL,
                            rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
            RwIm3DEnd();
        }
    }

    U32 count_alpha_triangles(const RpTriangle* tri, const F32* depth, u32 size)
    {
        static const U8 segments[43] = { 0, 1, 3, 0, 1, 2, 4, 0, 3, 4, 3, 0, 0, 0, 0,
                                         0, 1, 2, 4, 0, 2, 1, 2, 0, 4, 2, 1, 0, 0, 0,
                                         0, 0, 3, 4, 3, 0, 4, 2, 1, 0, 3, 1, 0 };

        u32 i = 0;
        U32 total = 0;
        const RpTriangle* end = tri + size;

        while (tri != end)
        {
            U32 flags = 0;
            F32 d0 = depth[tri->vertIndex[0]];
            F32 d1 = depth[tri->vertIndex[1]];
            F32 d2 = depth[tri->vertIndex[2]];
            U8 n;

            if (d0 < 0.0f)
            {
            }
            else if (d0 > 1.0f)
            {
                flags |= 0x2;
            }
            else
            {
                flags |= 0x1;
            }

            if (d1 < 0.0f)
            {
            }
            else if (d1 > 1.0f)
            {
                flags |= 0x8;
            }
            else
            {
                flags |= 0x4;
            }

            if (d2 < 0.0f)
            {
            }
            else if (d2 > 1.0f)
            {
                flags |= 0x20;
            }
            else
            {
                flags |= 0x10;
            }

            n = segments[flags];

            alpha_count0[i] = n;
            total += n;

            i++;
            tri++;
        }

        return total;
    }
} // namespace

struct _tagFirework
{
    S32 state;
    F32 timer;
    xVec3 vel;
    xVec3 pos;
    F32 fuel;
};

#define FIREWORK_COUNT 10

static _tagFirework sFirework[FIREWORK_COUNT];
static zParEmitter* sFireworkTrailEmit = NULL;
static zParEmitter* sFirework1Emit = NULL;
static zParEmitter* sFirework2Emit = NULL;
static U32 sFireworkSoundID = 0;
static U32 sFireworkLaunchSoundID = 0;

static RwIm3DVertex sStripVert_2188[4];

static RwIm3DVertex blah_2485[4];

namespace
{
#define RIBBON_COUNT 64

    xFXRibbon* active_ribbons[RIBBON_COUNT];
    U32 active_ribbons_size = 0;
    bool ribbons_dirty = false;
} // namespace

struct _xFXAuraAngle
{
    F32 angle;
    F32 cc;
    F32 ss;
};

struct _xFXAura
{
    xVec3 pos;
    iColor_tag color;
    F32 size;
    void* parent;
    U32 frame;
    F32 dangle[2];
};

#define AURA_COUNT 32

static F32 sAuraPulse[2];
static F32 sAuraPulseAng[2];
static _xFXAuraAngle sAuraAngle[2];
static _xFXAura sAura[AURA_COUNT];
static RwTexture* gAuraTex = NULL;

void xFXFireworksInit(const char* fireworksTrailEmitter, const char* fireworksEmitter1,
                      const char* fireworksEmitter2, const char* fireworksSound,
                      const char* fireworksLaunchSound)
{
    sFireworkTrailEmit = zParEmitterFind(fireworksTrailEmitter);
    sFirework1Emit = zParEmitterFind(fireworksEmitter1);
    sFirework2Emit = zParEmitterFind(fireworksEmitter2);
    sFireworkSoundID = xStrHash(fireworksSound);
    sFireworkLaunchSoundID = xStrHash(fireworksLaunchSound);
    memset(sFirework, 0, sizeof(sFirework));
    for (U32 i = 0; i < FIREWORK_COUNT; ++i)
    {
        sFirework[i].state = 0;
    }
}

void xFXFireworksLaunch(F32 countdownTime, const xVec3* pos, F32 fuelTime)
{
    for (S32 i = 0; i < FIREWORK_COUNT; i++)
    {
        _tagFirework& f = sFirework[i];

        if (f.state == 0)
        {
            f.state = 1;
            f.timer = countdownTime;
            f.pos = *pos;
            f.fuel = fuelTime;
            return;
        }
    }
}

void xFXFireworksUpdate(F32 dt)
{
    for (S32 i = 0; i < FIREWORK_COUNT; ++i)
    {
        if (sFirework[i].state == 0)
        {
            continue;
        }
        if (sFirework[i].state == 1)
        {
            sFirework[i].timer -= dt;
            if (sFirework[i].timer <= 0.0f)
            {
                // The addend really is a negative literal, not a subtraction:
                // the target's pool holds -6.5f and the code is an fmadds.
                sFirework[i].vel.x = 13.0f * xurand() + -6.5f;
                sFirework[i].vel.y = 0.0f;
                sFirework[i].vel.z = 13.0f * xurand() + -6.5f;
                sFirework[i].state = 2;

                if (sFireworkLaunchSoundID != 0)
                {
                    xSndPlay3D(sFireworkLaunchSoundID,
                               0.308f, // Volume
                               0.0f, // Pitch
                               0x80, // Priority
                               0, // Flags
                               &sFirework[i].pos,
                               20.0f, // Radius
                               5.0f, SND_CAT_GAME,
                               0.0f); // Delay
                }
            }
        }
        else
        {
            sFirework[i].fuel -= dt;
            if (sFirework[i].fuel > 0.0f)
            {
                sFirework[i].vel.y += 15.0f * dt;
            }
            xParEmitterCustomSettings trail_info;
            trail_info.custom_flags = eParEmitterCustomPos;
            F32 vx = sFirework[i].vel.x;
            sFirework[i].pos.x += vx * dt;
            F32 vy = sFirework[i].vel.y;
            sFirework[i].pos.y += vy * dt;
            F32 vz = sFirework[i].vel.z;
            sFirework[i].pos.z += vz * dt;
            trail_info.pos = sFirework[i].pos;
            xParEmitterEmitCustom(sFireworkTrailEmit, dt, &trail_info);

            if (sFirework[i].fuel <= 0.0f)
            {
                sFirework[i].state = 0;
                sFirework[i].timer = 0.0f;

                zParEmitter* femit = sFirework1Emit;
                if (xurand() < 0.75f)
                {
                    femit = sFirework2Emit;
                }

                xParEmitterCustomSettings xplo_info;
                xplo_info.custom_flags =
                    eParEmitterCustomColorDeath | eParEmitterCustomColorBirth |
                    eParEmitterCustomPos;
                xplo_info.pos = sFirework[i].pos;

                if (femit != NULL)
                {
                    xplo_info.color_birth[0].set(127.0f * xurand() + 128.0f, 75.0f, 0.0f, 0);
                    xplo_info.color_birth[1].set(127.0f * xurand() + 128.0f, 75.0f, 0.0f, 0);
                    xplo_info.color_birth[2].set(127.0f * xurand() + 128.0f, 75.0f, 0.0f, 0);
                    xplo_info.color_birth[3].set(255.0f, 0.0f, 1.0f, 0);
                    memcpy(xplo_info.color_death, &xplo_info.color_birth,
                           sizeof(xplo_info.color_birth));
                    xplo_info.color_death[3].set(0.0f, 0.0f, 1.0f, 0);
                    xParEmitterEmitCustom(femit, dt, &xplo_info);
                }

                xScrFXGlareAdd(&sFirework[i].pos, 0.5f * xurand() + 0.5f, 0.3f * xurand() + 0.1f,
                               5.0f * xurand() + 2.0f, 0.5f * xurand() + 0.5f,
                               0.5f * xurand() + 0.5f, 0.5f * xurand() + 0.5f,
                               0.4f * xurand() + 0.1f, NULL);

                xVec3 diff;
                xVec3Sub(&diff, xEntGetPos(&globals.player.ent), &sFirework[i].pos);
                zRumbleStartDistance(globals.currentActivePad, diff.x * diff.x + diff.z * diff.z,
                                     48.0f, eRumble_Medium, 0.35f);
                sFirework[i].pos.y = xEntGetPos(&globals.player.ent)->y;
                if (sFireworkSoundID != 0)
                {
                    xSndPlay3D(sFireworkSoundID,
                               0.77f, // Volume
                               0.0f, // Pitch
                               0x80, // Priority
                               0, // Flags
                               &sFirework[i].pos,
                               20.0f, // Radius
                               5.0f, SND_CAT_GAME,
                               0.0f); // Delay
                }
            }
        }
    }
}

// Carrier, not recovered code. This string literal is present in the target's
// @stringBase0 at this position and is referenced by nothing in the target
// object, so it belongs to a function the retail link deadstripped. We do not
// know what that function was; only the literal and its position are evidence.
// Emitting it here keeps every later @stringBase0 offset in step with the
// target. A string only survives into the pool if it is returned, so this needs
// a body -- an unused local drops it.
const char* __deadstripped_xFX_1()
{
    return "update frame\n";
}

void xFXStreakInit()
{
    for (S32 i = 0; i < 10; i++)
    {
        memset(&sStreakList[i], 0, sizeof(xFXStreak));
        sStreakList[i].flags = 0;
        sStreakList[i].head = 0;
    }
}

void xFXStreakUpdate(F32 dt)
{
    S32 i;

    for (i = 0; i < 10; i++)
    {
        xFXStreak* s = &sStreakList[i];

        if (s->flags == 0)
        {
            continue;
        }

        s->elapsed += dt;
        s->lifetime += dt;

        S32 j;
        xFXStreakElem* e;

        for (j = 0; j < 50; j++)
        {
            e = &s->elem[j];

            if (s->flags & 0x4)
            {
                xVec3 diff = e->p[0] - e->p[1];

                diff *= 0.5f * (s->alphaFadeRate * dt);

                e->p[0] -= diff;
                e->p[1] += diff;
            }

            e->a -= s->alphaFadeRate * dt;

            if (e->a < 0.01f)
            {
                e->a = 0.0f;
                e->flag = 0;
            }
        }

        if (s->flags & 0x2)
        {
            S32 done = 1;
            S32 k;

            for (k = 0; k < 50; k++)
            {
                if (s->elem[k].flag == 1)
                {
                    done = 0;
                    break;
                }
            }

            if (done)
            {
                s->flags = 0;
            }
        }
    }
}

void xFXStreakRender()
{
    xFXStreakElem* e1;
    S32 streak;
    xFXStreak* s;
    S32 count;
    S32 j;
    xFXStreakElem* e;

    for (streak = 0; streak < 10; streak++)
    {
        s = &sStreakList[streak];

        if (s->flags == 0)
        {
            continue;
        }

        count = s->head;

        j = 49;

        while (j > 0)
        {
            e = &s->elem[count];

            if (count - 1 > 0)
            {
                e1 = &s->elem[count - 1];
            }
            else
            {
                e1 = &s->elem[49];
            }

            if (e->flag == 0)
            {
                break;
            }

            if (e1->flag == 0)
            {
                break;
            }

            RwIm3DVertexSetPos(&sStripVert_2188[0], e->p[0].x, e->p[0].y, e->p[0].z);
            RwIm3DVertexSetUV(&sStripVert_2188[0], 0.0f, 0.0f);
            U8 c0r = s->color_a.r;
            U8 c0g = s->color_a.g;
            U8 c0b = s->color_a.b;
            F32 c0af = 255.0f * e->a;
            F32 c2af = 255.0f * e1->a;
            RwIm3DVertexSetRGBA(&sStripVert_2188[0], c0r, c0g, c0b, (U8)c0af);

            RwIm3DVertexSetPos(&sStripVert_2188[1], e->p[1].x, e->p[1].y, e->p[1].z);
            RwIm3DVertexSetUV(&sStripVert_2188[1], 0.0f, 1.0f);
            U8 c1r = s->color_b.r;
            U8 c1g = s->color_b.g;
            U8 c1b = s->color_b.b;
            RwIm3DVertexSetRGBA(&sStripVert_2188[1], c1r, c1g, c1b, (U8)c0af);

            RwIm3DVertexSetPos(&sStripVert_2188[2], e1->p[0].x, e1->p[0].y, e1->p[0].z);
            RwIm3DVertexSetUV(&sStripVert_2188[2], 1.0f, 0.0f);
            U8 c2r = s->color_a.r;
            U8 c2g = s->color_a.g;
            U8 c2b = s->color_a.b;
            RwIm3DVertexSetRGBA(&sStripVert_2188[2], c2r, c2g, c2b, (U8)c2af);

            RwIm3DVertexSetPos(&sStripVert_2188[3], e1->p[1].x, e1->p[1].y, e1->p[1].z);
            RwIm3DVertexSetUV(&sStripVert_2188[3], 1.0f, 1.0f);
            U8 c3r = s->color_b.r;
            U8 c3g = s->color_b.g;
            U8 c3b = s->color_b.b;
            RwIm3DVertexSetRGBA(&sStripVert_2188[3], c3r, c3g, c3b, (U8)c2af);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)s->textureRasterPtr);

            if (RwIm3DTransform(sStripVert_2188, 4, NULL,
                                rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
            {
                RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
                RwIm3DEnd();
            }

            count--;
            j--;

            if (count < 0)
            {
                count = 49;
            }
        }
    }
}

U32 xFXStreakStart(F32 frequency, F32 alphaFadeRate, F32 alphaStart, U32 textureID,
                   const iColor_tag* edge_a, const iColor_tag* edge_b, S32 taper)
{
    xFXStreak* s;
    U32 i;

    for (i = 0; i < 10; i++)
    {
        s = &sStreakList[i];

        if (s->flags == 0x0)
        {
            s->flags = 0x1;

            if (taper != 0)
            {
                s->flags |= 0x4;
            }

            s->frequency = frequency;
            s->alphaFadeRate = alphaFadeRate;
            s->alphaStart = alphaStart;
            s->head = 0;
            s->elapsed = 0.0f;
            s->lifetime = 0.0f;
            s->textureRasterPtr = NULL;
            s->texturePtr = NULL;

            for (S32 j = 0; j < 50; j++)
            {
                s->elem[j].flag = 0x0;
            }

            if (edge_a != NULL)
            {
                s->color_a = *edge_a;
            }
            else
            {
                s->color_a.a = 255;
                s->color_a.b = 255;
                s->color_a.g = 255;
                s->color_a.r = 255;
            }

            if (edge_b != NULL)
            {
                s->color_b = *edge_b;
            }
            else
            {
                s->color_b.a = 255;
                s->color_b.b = 255;
                s->color_b.g = 255;
                s->color_b.r = 255;
            }

            if (textureID == 0)
            {
                textureID = xStrHash("fx_streak1");
            }

            s->texturePtr = (RwTexture*)xSTFindAsset(textureID, NULL);

            if (s->texturePtr != NULL)
            {
                s->textureRasterPtr = RwTextureGetRaster(s->texturePtr);
            }

            return i;
        }
    }

    return 10;
}

void xFXStreakStop(U32 id)
{
    if (id == 10)
    {
        return;
    }

    xFXStreak* s = &sStreakList[id];

    // The `& 1` is redundant -- `(x == 0)` is already 0 or 1 -- but the target
    // narrows the comparison to a single bit (`extrwi. 1,26` off the `cntlzw`),
    // which only this spelling reproduces. `!s->flags` gives `cmplwi/beqlr`.
    if ((s->flags == 0) & 1)
    {
        return;
    }

    if (s->flags & 0x1)
    {
        s->flags = s->flags ^ 0x1;
    }

    s->flags |= 0x2;
}

void xFXStreakUpdate(U32 id, const xVec3* a, const xVec3* b)
{
    xFXStreak* s;

    if (id == 10)
    {
        return;
    }

    s = &sStreakList[id];

    if (s->elapsed > s->frequency)
    {
        s->elapsed -= s->frequency;
        s->head = s->head + 1;

        if (s->head >= 50)
        {
            s->head = 0;
        }
    }

    s->elem[s->head].p[0] = *a;
    s->elem[s->head].p[1] = *b;
    s->elem[s->head].flag = 1;
    s->elem[s->head].a = s->alphaStart;
}

void xFXShineInit()
{
    for (S32 i = 0; i < 2; i++)
    {
        memset(&sShineList[i], 0, sizeof(xFXShine));
        sShineList[i].flags = 0;
    }
}

void xFXShineUpdate(F32 dt)
{
    xFXShine* s;
    S32 i;
    S32 j;
    xFXShineElem* e;

    for (i = 0; i < 2; i++)
    {
        s = &sShineList[i];

        if (s->flags == 0)
        {
            continue;
        }

        s->elapsed += dt;
        s->lifetime += dt;
        s->rotateZ += s->rotateSpeed * dt;

        if (s->lifetimeMax && !(s->flags & 0x2) && s->lifetime > s->lifetimeMax)
        {
            s->flags |= 0x2;
        }

        if (s->ppos != NULL)
        {
            s->pos = *s->ppos;
        }

        for (j = 0; j < 100; j++)
        {
            e = &s->elem[j];

            if (e->flag == 0 && s->elapsed >= s->frequency && !(s->flags & 0x2))
            {
                e->vel.x = s->spd * (2.0f * xurand() - 1.0f);
                e->vel.y = s->spd * (2.0f * xurand() - 1.0f);
                e->vel.z = s->spd * (2.0f * xurand() - 1.0f);

                e->p = e->vel;

                e->cola.r = s->color_a.r;
                e->cola.g = s->color_a.g;
                e->cola.b = s->color_a.b;
                e->colb.r = s->color_b.r;
                e->colb.g = s->color_b.g;
                e->colb.b = s->color_b.b;

                e->a = 1.0f;
                e->lifetime = 0.0f;
                e->flag = 1;

                s->elapsed -= s->frequency;
            }

            if (e->flag != 0)
            {
                e->p += e->vel * dt;
                e->lifetime += dt;

                if (e->lifetime < s->lifetimeElemMax)
                {
                    e->a = isin(3.1415927f * (e->lifetime / s->lifetimeElemMax));
                }
                else
                {
                    e->a = 0.0f;
                    e->flag = 0;
                }
            }
        }

        if (s->flags & 0x2)
        {
            S32 done = 1;
            S32 k;

            for (k = 0; k < 100; k++)
            {
                if (s->elem[k].flag & 1)
                {
                    done = 0;
                    break;
                }
            }

            if (done)
            {
                s->flags = 0;
            }
        }
    }
}

void xFXShineRender()
{
    xFXShineElem* e;
    S32 shine;
    xFXShine* s;
    S32 j;
    RxObjSpace3DVertex* vert;
    RwFrame* frame;
    xVec3 v;
    xVec3 w;
    xVec3 w2;
    F32 uoff;
    xMat3x3 mat;

    for (shine = 0; shine < 2; shine++)
    {
        s = &sShineList[shine];

        if (s->flags == 0)
        {
            continue;
        }

        for (j = 0; j < 100; j++)
        {
            e = &s->elem[j];

            if (e->flag == 0)
            {
                continue;
            }

            vert = blah_2485;

            if (globals.camera.lo_cam)
            {
                frame = RwCameraGetFrame(globals.camera.lo_cam);
            }

            v.x = e->p.x;
            v.y = e->p.y;
            v.z = e->p.z;

            w.x = 0.5f * (frame->ltm.right.x * s->width);
            w.y = 0.5f * (frame->ltm.right.y * s->width);
            w.z = 0.5f * (frame->ltm.right.z * s->width);

            w2 = w * 1.2f;

            uoff = e->lifetime * s->spd;

            xMat3x3Transpose(&mat, (xMat3x3*)frame);
            xMat3x3LMulVec(&v, &mat, &v);

            F32 a0x = s->pos.x;
            F32 a0y = s->pos.y;
            F32 a0z = s->pos.z;
            RwIm3DVertexSetPos(&vert[0], a0x, a0y, a0z);
            RwIm3DVertexSetUV(&vert[0], uoff, 0.0f);
            U8 cola0_r = e->cola.r;
            U8 cola0_g = e->cola.g;
            U8 cola0_b = e->cola.b;
            RwIm3DVertexSetRGBA(&vert[0], cola0_r, cola0_g, cola0_b, 0);

            F32 a1x = s->pos.x;
            F32 a1y = s->pos.y;
            F32 a1z = s->pos.z;
            RwIm3DVertexSetPos(&vert[1], a1x, a1y, a1z);
            RwIm3DVertexSetUV(&vert[1], uoff, 1.0f);
            U8 cola1_r = e->cola.r;
            U8 cola1_g = e->cola.g;
            U8 cola1_b = e->cola.b;
            RwIm3DVertexSetRGBA(&vert[1], cola1_r, cola1_g, cola1_b, 0);

            F32 a2x = s->pos.x + v.x - w.x;
            F32 a2y = s->pos.y + v.y - w.y;
            F32 a2z = s->pos.z + v.z - w.z;
            RwIm3DVertexSetPos(&vert[2], a2x, a2y, a2z);
            RwIm3DVertexSetUV(&vert[2], 1.0f + uoff, 0.0f);
            U8 cola2_r = e->cola.r;
            U8 cola2_g = e->cola.g;
            U8 cola2_b = e->cola.b;
            F32 cola2_af = 255.0f * e->a;
            RwIm3DVertexSetRGBA(&vert[2], cola2_r, cola2_g, cola2_b, (U8)cola2_af);

            F32 a3x = w.x + (s->pos.x + v.x);
            F32 a3y = w.y + (s->pos.y + v.y);
            F32 a3z = w.z + (s->pos.z + v.z);
            RwIm3DVertexSetPos(&vert[3], a3x, a3y, a3z);
            RwIm3DVertexSetUV(&vert[3], 1.0f + uoff, 1.0f);
            U8 cola3_r = e->cola.r;
            U8 cola3_g = e->cola.g;
            U8 cola3_b = e->cola.b;
            RwIm3DVertexSetRGBA(&vert[3], cola3_r, cola3_g, cola3_b, (U8)cola2_af);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)s->textureRasterPtr);

            if (RwIm3DTransform(blah_2485, 4, NULL,
                                rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
            {
                RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
                RwIm3DEnd();
            }

            vert = blah_2485;

            F32 b0x = s->pos.x;
            F32 b0y = s->pos.y;
            F32 b0z = s->pos.z;
            RwIm3DVertexSetPos(&vert[0], b0x, b0y, b0z);
            RwIm3DVertexSetUV(&vert[0], uoff, 0.0f);
            U8 colb0_r = e->colb.r;
            U8 colb0_g = e->colb.g;
            U8 colb0_b = e->colb.b;
            RwIm3DVertexSetRGBA(&vert[0], colb0_r, colb0_g, colb0_b, 0);

            F32 b1x = s->pos.x;
            F32 b1y = s->pos.y;
            F32 b1z = s->pos.z;
            RwIm3DVertexSetPos(&vert[1], b1x, b1y, b1z);
            RwIm3DVertexSetUV(&vert[1], 0.0f, 1.0f);
            U8 colb1_r = e->colb.r;
            U8 colb1_g = e->colb.g;
            U8 colb1_b = e->colb.b;
            RwIm3DVertexSetRGBA(&vert[1], colb1_r, colb1_g, colb1_b, 0);

            F32 b2x = s->pos.x + v.x - w2.x;
            F32 b2y = s->pos.y + v.y - w2.y;
            F32 b2z = s->pos.z + v.z - w2.z;
            RwIm3DVertexSetPos(&vert[2], b2x, b2y, b2z);
            RwIm3DVertexSetUV(&vert[2], 1.0f, 0.0f);
            U8 colb2_r = e->colb.r;
            U8 colb2_g = e->colb.g;
            U8 colb2_b = e->colb.b;
            F32 colb2_af = 255.0f * e->a;
            RwIm3DVertexSetRGBA(&vert[2], colb2_r, colb2_g, colb2_b, (U8)colb2_af);

            F32 b3x = w2.x + (s->pos.x + v.x);
            F32 b3y = w2.y + (s->pos.y + v.y);
            F32 b3z = w2.z + (s->pos.z + v.z);
            RwIm3DVertexSetPos(&vert[3], b3x, b3y, b3z);
            RwIm3DVertexSetUV(&vert[3], 1.0f, 1.0f);
            U8 colb3_r = e->colb.r;
            U8 colb3_g = e->colb.g;
            U8 colb3_b = e->colb.b;
            RwIm3DVertexSetRGBA(&vert[3], colb3_r, colb3_g, colb3_b, (U8)colb2_af);

            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)s->textureRasterPtr);

            if (RwIm3DTransform(blah_2485, 4, NULL,
                                rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
            {
                RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
                RwIm3DEnd();
            }
        }
    }
}

U32 xFXShineStart(const xVec3*, F32, F32, F32, F32, U32, const iColor_tag*, const iColor_tag*, F32,
                  S32)
{
    return 2;
}

namespace
{
    S32 compare_ribbons(const void* e1, const void* e2)
    {
        if (e1 == e2)
        {
            return 0;
        }

        if (e1 == NULL)
        {
            return 1;
        }

        if (e2 == NULL)
        {
            return -1;
        }

        return (*(const xFXRibbon* const*)e1)->render_compare(**(const xFXRibbon* const*)e2);
    }

    void sort_ribbons()
    {
        if (!ribbons_dirty || !active_ribbons_size)
        {
            return;
        }

        qsort(active_ribbons, active_ribbons_size, sizeof(xFXRibbon*), compare_ribbons);
        ribbons_dirty = false;
    }

    void activate_ribbon(xFXRibbon* ribbon)
    {
        if (active_ribbons_size >= RIBBON_COUNT)
        {
            return;
        }

        active_ribbons[active_ribbons_size] = ribbon;
        active_ribbons_size = active_ribbons_size + 1;
        ribbons_dirty = true;
    }

    void deactivate_ribbon(xFXRibbon* ribbon)
    {
        xFXRibbon** it = active_ribbons;
        xFXRibbon** end = active_ribbons + active_ribbons_size;

        while (it != end)
        {
            if (*it == ribbon)
            {
                active_ribbons_size--;
                memmove(it, it + 1, (end - it - 1) * sizeof(xFXRibbon*));
                return;
            }

            it++;
        }
    }
} // namespace

U8 tier_queue_allocator::alloc_block()
{
    U8 block = head;
    block_data& data = blocks[block];

    head = data.next;
    blocks[data.prev].next = data.next;
    blocks[data.next].prev = data.prev;

    if (data.data == NULL)
    {
        data.data = alloc_block_data();
    }

    return block;
}

void tier_queue_allocator::free_block(U8 block)
{
    block_data& data = blocks[block];

    data.next = head;
    data.prev = blocks[head].prev;
    blocks[data.prev].next = block;
    blocks[data.next].prev = block;

    head = block;
}

void xFXRibbon::init(const char* group, const char* name)
{
    activated = false;
    mtime = 0;

    joints.init(joint_alloc);

    if (cfg.life_time <= 0.0f)
    {
        set_default_config();
    }

    debug_init(group, name);
}

template <> void tier_queue<xFXRibbon::joint_data>::clear()
{
    u32 block = get_block(first);
    u32 last = wrap_block(block + get_block(_size + alloc->block_size() - 1));

    while (block != last)
    {
        alloc->free_block(blocks[block]);
        block = wrap_block(block + 1);
    }

    _size = 0;
    first = 0;
}

void xFXRibbon::set_default_config()
{
    cfg.life_time = 1.0f;
    cfg.blend_src = 5;
    cfg.blend_dst = 6;
    cfg.pivot = 0.5f;
    refresh_config();
}

void xFXRibbon::refresh_config()
{
    ilife = 1.0f / cfg.life_time;
    mlife = 1000.0f * cfg.life_time;
    if (activated)
    {
        ribbons_dirty = true;
    }
}

void xFXRibbon::set_curve(const curve_node* curve, size_t size)
{
    curve_size = size;
    (this->curve) = ((curve_node*)curve);
    xFXRibbon::debug_update_curve();
    xFXRibbon::update_curve_tweaks();
}

void xFXRibbon::insert(const xVec3& loc, const xVec3& norm, F32 scale, F32 alpha, U32 flags)
{
    while (joints.front_full() && !joints.empty())
    {
        joints.pop_back();
    }

    if (!joints.front_full())
    {
        joints.push_front();

        joint_data& joint = joints.front();

        joint.flags = flags & 1;
        joint.loc = loc;
        joint.norm = norm;
        joint.born = mtime;
        joint.scale = scale;
        joint.alpha = alpha;

        activate();
    }
}

void xFXRibbon::insert(const xVec3& loc, F32 orient, F32 scale, F32 alpha, U32 flags)
{
    while (joints.front_full() && !joints.empty())
    {
        joints.pop_back();
    }

    if (!joints.front_full())
    {
        joints.push_front();

        joint_data& joint = joints.front();

        joint.flags = (flags & 1) | 0x30000;
        joint.loc = loc;
        joint.orient = orient;
        joint.born = mtime;
        joint.scale = scale;
        joint.alpha = alpha;

        activate();
    }
}

void xFXRibbon::activate()
{
    if (activated == 0)
    {
        activate_ribbon(this);
        activated = 1;
    }
}

void xFXRibbon::deactivate()
{
    if (activated != 0)
    {
        deactivate_ribbon(this);
        activated = 0;
    }
}

void xFXRibbon::update(F32 dt)
{
    debug_update(dt);

    mtime += (U32)(1000.0f * dt);

    while (!joints.empty())
    {
        joint_data& oldest = joints.back();

        if (mtime - oldest.born < mlife)
        {
            break;
        }

        oldest.born = mtime - mlife;

        if (joints.size() == 1)
        {
            joints.clear();
            break;
        }

        if (mtime - (*(joints.end() - 2)).born < mlife)
        {
            break;
        }

        joints.pop_back();
    }

    if (!need_update())
    {
        deactivate();
    }
}

void xFXRibbon::start_render()
{
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)cfg.blend_src);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)cfg.blend_dst);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)raster);
}

void xFXRibbon::render()
{
    RxObjSpace3DVertex* verts = gRenderBuffer.m_vertex;

    curve_index = curve_size - 2;

    S32 it = joints.size();

    while (it > 1)
    {
        S32 subsize = (it > 240) ? 240 : it;
        S32 next_it = it - subsize + 1;
        S32 break_it = it - 1;

        while (break_it >= next_it)
        {
            break_it--;

            if (joints[break_it].flags & 1)
            {
                next_it = break_it + 1;
                subsize = it - next_it;
                break;
            }
        }

        if (subsize > 1)
        {
            render_strip(verts, joints.begin() + it, subsize);
        }

        it = next_it;
    }
}

void xFXRibbon::set_raster(RwRaster* rast)
{
    this->raster = rast;
    if (activated <= 0)
    {
        return;
    }
    ribbons_dirty = true;
}

void xFXRibbon::set_texture(RwTexture* texture)
{
    set_raster((texture == NULL) ? NULL : texture->raster);
}

void xFXRibbon::set_texture(U32 id)
{
    set_texture((id == 0) ? NULL : (RwTexture*)xSTFindAsset(id, NULL));
}

void xFXRibbon::set_texture(const char* name)
{
    set_texture(xStrHash(name));
}

void xFXRibbon::get_normal(xVec3& norm, const xVec3& dir, F32 orient)
{
    F32 a = isin(orient);
    F32 b = icos(orient);

    // Declared z,y,x but assigned x,y,z, and that split is load-bearing: the target
    // loads the components in address order (0x0, 0x4, 0x8) while colouring them
    // f9, f8, f7 -- descending. CodeWarrior colours the FP class in DECLARATION
    // order, lowest free colour first, so only a bare declaration in z,y,x order
    // with the assignments left in x,y,z reproduces both the load order and the
    // register order. The locals themselves are real: the target keeps all three
    // live across the xabs compares and reuses them in the first arm below.
    F32 dz, dy, dx;

    dx = dir.x;
    dy = dir.y;
    dz = dir.z;

    F32 ax = xabs(dx);
    F32 ay = xabs(dy);
    F32 az = xabs(dz);

    if (ax < ay && ax < az)
    {
        // Retail bug, reproduced: each arm builds cos * cross(dir, axis) +
        // sin * cross(dir, cross(dir, axis)) for the axis dir is least aligned
        // with, so norm.y here should be `dir.y * (a * dir.x)`. The x-axis arm
        // repeats the z-axis arm's `dir.z * (a * dir.y)` instead, which is what
        // the target object computes.

        // Numerical fidelity, load-bearing -- do NOT fold these back into one
        // `dir.y * dir.y + dir.z * dir.z` expression. Retail rounds each square to
        // single precision and only then adds:
        //     fmuls f2, f8, f8 / fmuls f0, f7, f7 / fadds f5, f2, f0
        // Written as a single expression CodeWarrior contracts the pair into one
        // `fmadds`, which keeps the first product at full internal precision and
        // gives a different result. Naming the two products is what forces the
        // rounded form. Only this arm binds the components; the two arms below
        // re-read `dir` because the target reloads them there.
        F32 dy2 = dy * dy;
        F32 dz2 = dz * dz;

        norm.x = -a * (dy2 + dz2);
        norm.y = dz * (a * dy) + b * dz;
        norm.z = dz * (a * dx) - b * dy;
        norm *= 1.0f / xsqrt(dy2 + dz2);
    }
    else if (ay < az)
    {
        norm.x = dir.y * (a * dir.x) - b * dir.z;
        norm.y = -a * (dir.x * dir.x + dir.z * dir.z);
        norm.z = dir.z * (a * dir.y) + b * dir.x;
        norm *= 1.0f / xsqrt(dir.x * dir.x + dir.z * dir.z);
    }
    else
    {
        norm.x = dir.z * (a * dir.x) + b * dir.y;
        norm.y = dir.z * (a * dir.y) - b * dir.x;
        norm.z = -a * (dir.x * dir.x + dir.y * dir.y);
        norm *= 1.0f / xsqrt(dir.x * dir.x + dir.y * dir.y);
    }
}

void xFXRibbon::refresh_joint(joint_data& joint, const tier_queue<joint_data>::iterator& it)
{
    if (joint.flags & 0x20000)
    {
        joint.flags &= ~0x20000;

        const xVec3& prev = (it == joints.begin()) ? joint.loc : (it - 1)->loc;
        const xVec3& next = (it + 1 == joints.end()) ? joint.loc : (it + 1)->loc;
        xVec3 dir = prev - next;
        F32 len2 = dir.length2();

        if (xfeq0(len2))
        {
            joint.norm.x = icos(joint.orient);
            joint.norm.y = isin(joint.orient);
            joint.norm.z = 0.0f;
        }
        else
        {
            const xVec3 offset = dir * (1.0f / xsqrt(len2));

            get_normal(joint.norm, offset, joint.orient);
        }
    }
}

void xFXRibbon::eval_joint(const joint_data& joint, iColor_tag& color, F32& width)
{
    F32 frac = get_age(joint) * ilife;

    if (frac > 1.0f)
    {
        frac = 1.0f;
    }

    while (curve_index != 0)
    {
        if (frac >= curve[curve_index].time && frac <= curve[curve_index + 1].time)
        {
            break;
        }

        curve_index--;
    }

    curve_node& node0 = curve[curve_index];
    curve_node& node1 = curve[curve_index + 1];
    F32 subfrac = (1.0f / (node1.time - node0.time)) * (frac - node0.time);

    lerp(color.r, subfrac, node0.color.r, node1.color.r);
    lerp(color.g, subfrac, node0.color.g, node1.color.g);
    lerp(color.b, subfrac, node0.color.b, node1.color.b);

    F32 alpha = 0.0f;

    lerp(alpha, subfrac, node0.color.a, node1.color.a);

    color.a = (U8)(alpha * joint.alpha + 0.5f);

    lerp(width, subfrac, node0.scale, node1.scale);

    width *= joint.scale;
}

namespace
{
    void set_vert(RxObjSpace3DVertex& vert, const xVec3& loc, F32 u, F32 v, iColor_tag color);
}

void xFXRibbon::render_strip(RxObjSpace3DVertex* verts, tier_queue<joint_data>::iterator first,
                             u32 size)
{
    RxObjSpace3DVertex* v = verts;
    S32 back = first.global_index() & 1;
    F32 ulookup[2] = { 0.0f, 1.0f };
    tier_queue<joint_data>::iterator last = first - size;

    while (first != last)
    {
        --first;

        joint_data& joint = *first;

        refresh_joint(joint, first);

        iColor_tag color;
        F32 width;

        color.r = 0;
        color.g = 0;
        color.b = 0;
        color.a = 0;
        width = 0.0f;

        eval_joint(joint, color, width);

        F32 offset1 = cfg.pivot * width;
        F32 offset2 = (cfg.pivot - 1.0f) * width;
        F32 u = ulookup[back];

        const xVec3 loc1 = joint.loc + joint.norm * offset1;

        set_vert(*v, loc1, u, 0.0f, color);

        const xVec3 loc2 = joint.loc + joint.norm * offset2;

        set_vert(v[1], loc2, u, 1.0f, color);

        back ^= 1;
        v += 2;
    }

    RwIm3DTransform(verts, v - verts, NULL,
                    rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);
    RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
    RwIm3DEnd();
}

namespace
{
    void set_vert(RxObjSpace3DVertex& vert, const xVec3& loc, F32 u, F32 v, iColor_tag color)
    {
        RwIm3DVertexSetPos(&vert, loc.x, loc.y, loc.z);
        RwIm3DVertexSetUV(&vert, u, v);
        U8 r = color.r;
        U8 g = color.g;
        U8 b = color.b;
        U8 a = color.a;

        RwIm3DVertexSetRGBA(&vert, r, g, b, a);
    }
} // namespace

S32 xFXRibbon::render_compare(const xFXRibbon& c) const
{
    if (raster < c.raster)
    {
        return -1;
    }

    if (raster > c.raster)
    {
        return 1;
    }

    if (cfg.blend_src < c.cfg.blend_src)
    {
        return -1;
    }

    if (cfg.blend_src > c.cfg.blend_src)
    {
        return 1;
    }

    if (cfg.blend_dst < c.cfg.blend_dst)
    {
        return -1;
    }

    if (cfg.blend_dst > c.cfg.blend_dst)
    {
        return 1;
    }

    return 0;
}

// Carrier, not recovered code. These eleven RwBlendFunction names are present
// in the target's @stringBase0 right here, between "fx_streak1" and
// "FX|Ribbon", and nothing in the target object references them -- they belong
// to a function the retail link deadstripped, plausibly the "FX|Ribbon" tweak
// registration, since xFXRibbonSceneEnter only ever removes that tweak. That
// attribution is a guess; the literals, their order and their position are the
// only evidence. Emitting them keeps "FX|Ribbon" at its target offset of 0x13a.
// The if-chain is the one shape that leaks nothing of its own: a string table
// adds a pointer array to .rodata and a switch adds a jump table to .data.
const char* __deadstripped_xFX_2(S32 i)
{
    if (i == 0)
    {
        return "BLENDZERO";
    }
    if (i == 1)
    {
        return "BLENDONE";
    }
    if (i == 2)
    {
        return "BLENDSRCCOLOR";
    }
    if (i == 3)
    {
        return "BLENDINVSRCCOLOR";
    }
    if (i == 4)
    {
        return "BLENDSRCALPHA";
    }
    if (i == 5)
    {
        return "BLENDINVSRCALPHA";
    }
    if (i == 6)
    {
        return "BLENDDESTALPHA";
    }
    if (i == 7)
    {
        return "BLENDINVDESTALPHA";
    }
    if (i == 8)
    {
        return "BLENDDESTCOLOR";
    }
    if (i == 9)
    {
        return "BLENDINVDESTCOLOR";
    }

    return "BLENDSRCALPHASAT";
}

void xFXRibbonSceneEnter()
{
    xDebugRemoveTweak("FX|Ribbon");

    xFXRibbon::joint_alloc.init(sizeof(xFXRibbon::joint_data), 32, 128);

    active_ribbons_size = 0;
}

void xFXRibbonUpdate(F32 dt)
{
    for (U32 i = 0; i < active_ribbons_size; i++)
    {
        active_ribbons[i]->update(dt);
    }
}

void xFXRibbonRender()
{
    sort_ribbons();

    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0x0);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0x1);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)0x2);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)0x1);

    xFXRibbon* prev = NULL;

    for (U32 i = 0; i < active_ribbons_size; i++)
    {
        xFXRibbon* ribbon = active_ribbons[i];

        if (ribbon->visible())
        {
            if (prev == NULL || ribbon->render_compare(*prev))
            {
                ribbon->start_render();
                prev = ribbon;
            }

            ribbon->render();
        }
    }
}

void xFXAuraInit()
{
    if (gAuraTex == NULL)
    {
        gAuraTex = (RwTexture*)xSTFindAsset(0x8074A95F, NULL);
    }

    memset(sAura, 0, sizeof(sAura));
}

void xFXAuraSetup()
{
    gAuraTex = (RwTexture*)xSTFindAsset(0x8074A95F, NULL);
}

void xFXAuraAdd(void* parent, xVec3* pos, iColor_tag* color, F32 size)
{
    S32 i;
    _xFXAura* ap;

    ap = sAura;

    for (i = 0; i < AURA_COUNT; i++)
    {
        if (ap->parent == parent)
        {
            ap->frame = gFrameCount;
            ap->pos = *pos;
            ap->color = *color;
            ap->size = size;
            return;
        }
        ap++;
    }

    ap = sAura;

    for (i = 0; i < AURA_COUNT; i++)
    {
        if (ap->parent == NULL)
        {
            ap->frame = gFrameCount;
            ap->parent = parent;
            ap->pos = *pos;
            ap->color = *color;
            ap->size = size;
            ap->dangle[0] = 0.5f;
            ap->dangle[1] = -0.5f;
            return;
        }
        ap++;
    }
}

void xFXAuraUpdate(F32 dt)
{
    _xFXAura* ap;
    S32 i;

    if (gAuraTex == NULL)
    {
        return;
    }

    sAuraPulseAng[0] += 2.0f * dt;
    sAuraPulseAng[1] += 2.3f * dt;

    if (sAuraPulseAng[0] > 6.2831855f)
    {
        sAuraPulseAng[0] -= 6.2831855f;
    }

    if (sAuraPulseAng[1] > 6.2831855f)
    {
        sAuraPulseAng[1] -= 6.2831855f;
    }

    sAuraPulse[0] = 0.1f * isin(sAuraPulseAng[0]);
    sAuraPulse[1] = 0.1f * isin(sAuraPulseAng[1]);

    sAuraAngle[0].angle += 0.5f * dt;

    if (sAuraAngle[0].angle > 3.1415927f)
    {
        sAuraAngle[0].angle -= 6.2831855f;
    }

    sAuraAngle[0].cc = icos(sAuraAngle[0].angle);
    sAuraAngle[0].ss = isin(sAuraAngle[0].angle);

    sAuraAngle[1].angle = -sAuraAngle[0].angle;
    sAuraAngle[1].cc = sAuraAngle[0].cc;
    sAuraAngle[1].ss = -sAuraAngle[0].ss;

    ap = sAura;

    for (i = 0; i < AURA_COUNT; i++)
    {
        if (ap->frame != gFrameCount)
        {
            ap->parent = NULL;
        }
        ap++;
    }
}

static void RenderRotatedBillboard(xVec3* pos, _xFXAuraAngle* rot, U32 count, F32 width, F32 height,
                                   iColor_tag tint, U32 flipUV)
{
    U32 i;
    RxObjSpace3DVertex vert[384];
    xVec3 rtv;
    xVec3 upv;
    xVec3 v;
    RxObjSpace3DVertex* vp;
    xMat4x3* cammat;
    xVec3* rt;
    xVec3* up;
    F32 zero;
    F32 one;
    F32 hwidth;
    F32 hheight;
    F32 ndy;
    F32 nearclip;
    xVec3 myat;
    F32 camdist;
    F32 at_offset;
    F32 scale;
    F32 dx;
    F32 dy;

    RwCamera* cam = (RwCamera*)RwEngineInstance->curCamera;

    cammat = (xMat4x3*)&((RwFrame*)cam->object.object.parent)->modelling;

    if (pos == NULL || cammat == NULL)
    {
        return;
    }

    rt = &cammat->right;
    up = &cammat->up;

    width *= 1.0f + sAuraPulse[0];
    height *= 1.0f + sAuraPulse[1];

    if (flipUV)
    {
        zero = 0.999f;
    }
    else
    {
        zero = 0.0f;
    }

    if (flipUV)
    {
        one = 0.0f;
    }
    else
    {
        one = 0.999f;
    }

    nearclip = 0.1f + cam->nearPlane;

    hwidth = 0.5f * width;
    hheight = 0.5f * height;

    vp = vert;

    for (i = 0; i < count; i++, pos++)
    {
        xVec3Sub(&myat, &pos[i], &cammat->pos);
        camdist = xVec3Normalize(&myat, &myat);

        if (camdist < nearclip)
        {
            continue;
        }

        at_offset = -2.0f + camdist;

        if (at_offset <= nearclip)
        {
            at_offset = nearclip;
        }

        scale = at_offset / camdist;

        xVec3SMul(&v, &myat, at_offset - camdist);
        xVec3Add(&v, pos, &v);

        dx = hwidth * scale;
        dy = hheight * scale;

        xVec3SMul(&rtv, rt, dx);
        ndy = -dy;
        xVec3SMul(&upv, up, ndy);

        if (rot != NULL)
        {
            xVec3SMul(&rtv, rt, -dx * rot[i].ss);
            xVec3SMul(&upv, up, ndy * rot[i].cc);
        }

        F32 px0 = upv.x + (v.x + rtv.x);
        F32 py0 = upv.y + (v.y + rtv.y);
        F32 pz0 = upv.z + (v.z + rtv.z);
        RwIm3DVertexSetPos(&vp[0], px0, py0, pz0);
        RwIm3DVertexSetRGBA(&vp[0], tint.r, tint.g, tint.b, tint.a);
        RwIm3DVertexSetUV(&vp[0], zero, zero);

        if (rot != NULL)
        {
            xVec3SMul(&rtv, rt, dx * rot[i].cc);
            xVec3SMul(&upv, up, ndy * rot[i].ss);
        }

        F32 px1 = upv.x + (v.x + rtv.x);
        F32 py1 = upv.y + (v.y + rtv.y);
        F32 pz1 = upv.z + (v.z + rtv.z);
        RwIm3DVertexSetPos(&vp[1], px1, py1, pz1);
        RwIm3DVertexSetRGBA(&vp[1], tint.r, tint.g, tint.b, tint.a);
        RwIm3DVertexSetUV(&vp[1], one, zero);

        if (rot != NULL)
        {
            xVec3SMul(&rtv, rt, -dx * rot[i].cc);
            xVec3SMul(&upv, up, dy * rot[i].ss);
        }

        F32 px2 = upv.x + (v.x + rtv.x);
        F32 py2 = upv.y + (v.y + rtv.y);
        F32 pz2 = upv.z + (v.z + rtv.z);
        RwIm3DVertexSetPos(&vp[2], px2, py2, pz2);
        RwIm3DVertexSetRGBA(&vp[2], tint.r, tint.g, tint.b, tint.a);
        RwIm3DVertexSetUV(&vp[2], zero, one);

        F32 px3 = upv.x + (v.x + rtv.x);
        F32 py3 = upv.y + (v.y + rtv.y);
        F32 pz3 = upv.z + (v.z + rtv.z);
        RwIm3DVertexSetPos(&vp[3], px3, py3, pz3);
        RwIm3DVertexSetRGBA(&vp[3], tint.r, tint.g, tint.b, tint.a);
        RwIm3DVertexSetUV(&vp[3], zero, one);

        if (rot != NULL)
        {
            xVec3SMul(&rtv, rt, dx * rot[i].cc);
            xVec3SMul(&upv, up, ndy * rot[i].ss);
        }

        F32 px4 = upv.x + (v.x + rtv.x);
        F32 py4 = upv.y + (v.y + rtv.y);
        F32 pz4 = upv.z + (v.z + rtv.z);
        RwIm3DVertexSetPos(&vp[4], px4, py4, pz4);
        RwIm3DVertexSetRGBA(&vp[4], tint.r, tint.g, tint.b, tint.a);
        RwIm3DVertexSetUV(&vp[4], one, zero);

        if (rot != NULL)
        {
            xVec3SMul(&rtv, rt, dx * rot[i].ss);
            xVec3SMul(&upv, up, dy * rot[i].cc);
        }

        F32 px5 = upv.x + (v.x + rtv.x);
        F32 py5 = upv.y + (v.y + rtv.y);
        F32 pz5 = upv.z + (v.z + rtv.z);
        RwIm3DVertexSetPos(&vp[5], px5, py5, pz5);
        RwIm3DVertexSetRGBA(&vp[5], tint.r, tint.g, tint.b, tint.a);
        RwIm3DVertexSetUV(&vp[5], one, one);

        vp += 6;
    }

    RwIm3DTransform(vert, count * 6, NULL, rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ);
    RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
    RwIm3DEnd();
}

void xFXAuraRender()
{
    S32 fogstate;
    _xFXAura* ap;
    _xFXAuraAngle* auraAng;

    if (gAuraTex != NULL)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)gAuraTex->raster);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0x1);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)0x5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)0x2);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0x1);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0x0);
        RwRenderStateGet(rwRENDERSTATEFOGENABLE, &fogstate);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)0x0);

        ap = sAura;

        for (S32 i = 0; i < 32; i++)
        {
            if (ap->frame == gFrameCount)
            {
                auraAng = &sAuraAngle[0];
                RenderRotatedBillboard(&ap->pos, auraAng, 1, ap->size, ap->size, ap->color, 0);
                auraAng = &sAuraAngle[1];
                RenderRotatedBillboard(&ap->pos, auraAng, 1, ap->size, ap->size, ap->color, 1);
            }
            ap++;
        }

        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)0x5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)0x6);
        RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)0x1);
        RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)0x1);
        RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)fogstate);

        ap = sAura;

        for (S32 i = 0; i < 32; i++)
        {
            if (ap->frame == gFrameCount && ap->parent != 0)
            {
                zEntPickup_RenderOne((xEnt*)ap->parent);
            }
            ap++;
        }
    }
}

void xFXStartup()
{
}

void xFXShutdown()
{
}

void xFXSceneInit()
{
}

void xFXSceneSetup()
{
    DrawRingSetup();
    xFXAuraSetup();
}

void xFXSceneReset()
{
}

void xFXScenePrepare()
{
}

void xFXSceneFinish()
{
    DrawRingSceneExit();
    gAuraTex = NULL;
}

void xParInterp::set(F32 value1, F32 value2, F32 freq, U32 interp)
{
    this->val[0] = value1;
    this->val[1] = value2;
    this->freq = freq;
    if (freq != 0.0f)
    {
        this->oofreq = 1.0f / freq;
    }
    else
    {
        this->oofreq = 0.0f;
    }
    this->interp = interp;
}

void xFXRibbon::debug_init(const char*, const char*)
{
}

void xFXRibbon::update_curve_tweaks()
{
}

void xFXRibbon::debug_update_curve()
{
}

bool xFXRibbon::need_update() const
{
    bool result = false;

    if (visible() || debug_need_update())
    {
        result = true;
    }

    return result;
}

bool xFXRibbon::debug_need_update() const
{
    return false;
}

bool xFXRibbon::visible() const
{
    return !joints.empty();
}

void xFXRibbon::debug_update(F32)
{
}

F32 xFXRibbon::get_age(const joint_data& joint) const
{
    return 0.001f * (mtime - joint.born);
}
