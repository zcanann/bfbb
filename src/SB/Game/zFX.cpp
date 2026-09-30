#include "zFX.h"

#include "rpworld.h"
#include "rpskin.h"
#include "rwplcore.h"
#include "iAnim.h"
#include "xDebug.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xFX.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMathInlines.h"

#include "xSnd.h"
#include "zEnt.h"
#include "zGlobals.h"
#include "zGoo.h"
#include "zScene.h"
#include "zTextBox.h"

#include <stdio.h>
#include <types.h>
#include <string.h>
#include <stdlib.h>
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\cmath>

void zParPTankSpawnBubbles(xVec3* pos, xVec3* vel, U32 count, F32 scale);
void zParPTankSpawnMenuBubbles(xVec3* pos, xVec3* vel, U32 count);
S32 zParPTankBubblesAvailable();

void xBoundGetSphere(xSphere& o, const xBound& bound);
void xModelSetScale(xModelInstance* model, const xVec3& scale);
void xSCurve(F32& p, F32& v, F32& a, F32 t);

// These structs were used in deadstripped functions.
// These functions are here to force the symbols to be linked. The template
// between them, @558, is tweak_callback::create_change's own, so the definition
// sits where the target's .rodata order puts it.
void __deadstripped_zFX()
{
    const char _446[0x0C] = {};
    const char _447[0x0C] = {};
    const char _451[0x0C] = {};
    const char _482[0x0C] = {};

    const char _553[0x28] = {};
    const char _554[0x28] = {};
    const char _555[0x28] = {};
    const char _556[0x28] = {};
    const char _557[0x28] = {};
}

tweak_callback tweak_callback::create_change(void (*on_change)(const tweak_info&))
{
    tweak_callback cb = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

    cb.on_change = (void (*)(tweak_info&))on_change;

    return cb;
}

void __deadstripped_zFX2()
{
    const char _559[0x28] = {};
}

// Retail carries these two file-scope statics in .rodata, both 16 bytes of
// { 0.0f, 0.0f, 1.0f, 1.0f }, and nothing in this unit references them --
// their only consumers were deadstripped. Same pair as xHud.cpp and
// zEntPlayerBungeeState.cpp.
const basic_rect<F32> screen_bounds = { 0.0f, 0.0f, 1.0f, 1.0f };
const basic_rect<F32> default_adjust = { 0.0f, 0.0f, 1.0f, 1.0f };

const xFXRing sPatrickStunRing[3] = { { 0x741b0566,
                                        1.0f,
                                        { 0.0f, 0.0f, 0.0f },
                                        0.0f,
                                        0.5f,
                                        4.0f,
                                        PI / 2.0f,
                                        PI / -2.0f,
                                        0.5f,
                                        -0.25f,
                                        { 255, 255, 255, 255 },
                                        32,
                                        3,
                                        1,
                                        NULL },
                                      { 0x741b0566,
                                        1.5f,
                                        { 0.0f, 0.0f, 0.0f },
                                        0.0f,
                                        0.75f,
                                        5.0f,
                                        PI / 2.0f,
                                        PI / -2.0f,
                                        0.375f,
                                        -0.1875f,
                                        { 127, 127, 127, 127 },
                                        32,
                                        2,
                                        1,
                                        NULL },
                                      { 0x741b0566,
                                        2.0f,
                                        { 0.0f, 0.0f, 0.0f },
                                        0.0f,
                                        1.0f,
                                        6.0f,
                                        PI / 2.0f,
                                        PI / -2.0f,
                                        0.25f,
                                        -0.125f,
                                        { 63, 63, 63, 63 },
                                        32,
                                        1,
                                        1,
                                        NULL } };
const xFXRing sThunderRing[1] = {
    0x741b0566, 0.5f,  { 0.0f, 0.0f, 0.0f },   0.0f, 0.0f, 5.0f, PI / 2.0f, PI / -2.0f,
    0.75f,      -0.6f, { 255, 255, 255, 255 }, 32,   2,    1,    NULL
};
const xFXRing sHammerRing[1] = {
    0x741b0566, 0.75f, { 0.0f, 0.0f, 0.0f },   0.0f, 0.4f, 2.4f, PI / 2.0f, PI / -2.0f,
    0.6f,       -0.6f, { 255, 255, 255, 127 }, 32,   2,    1,    NULL
};
const xFXRing sPorterRing[2] = {
    { 0x741b0566,
      1.2f,
      { 0.0f, 0.0f, 0.0f },
      0.1f,
      0.0f,
      3.0f,
      PI / 2.0f,
      PI / -2.0f,
      0.6f,
      -0.6f,
      { 180, 180, 45, 127 },
      16,
      2,
      1,
      NULL },
    { 0x741b0566,
      0.5f,
      { 0.0f, 0.25f, 0.0f },
      0.0f,
      1.5f,
      -3.0f,
      PI / -2.0f,
      PI / 2.0f,
      1.2f,
      -2.4f,
      { 64, 128, 128, 127 },
      8,
      2,
      1,
      NULL },
};
const xFXRing sMuscleArmRing[1] = {
    0x741b0566, 3.0f,  { 0.0f, 0.0f, 0.0f },   0.0f, 0.0f, 90.0f, PI / 2.0f, 0.0f,
    0.6f,       30.0f, { 255, 255, 255, 160 }, 48,   1,    1,     NULL
};

static const float defaultGooTimes[4] = { 1e-5f, 2.0f, 15.0f, 2.0f };
static const float defaultGooWarbc[4] = { 0.25f, 2.0f, 0.25f, 1.2f };

static const xVec3 bubblewall_scale = { 2.4f, 2.4f, 2.4f };
static const xVec3 bubblewall_velscale = { 1.0f, 0.5f, 0.5f };

zFXGooInstance zFXGooInstances[24];
U32 gFXSurfaceFlags = 0;

void xDrawSphere2(const xVec3*, F32, U32)
{
}

void on_spawn_bubble_wall(const tweak_info& tweak)
{
    zFX_SpawnBubbleWall();
}

void xModelSetScale(xModelInstance* model, const xVec3& scale)
{
    for (; model != NULL; model = model->Next)
    {
        model->Scale = scale;
    }
}

static void init_poppers();
static void setup_entrails(zScene&);
void zFX_SceneEnter(RpWorld* world)
{
    xFXanimUV2PSetTexture(NULL);
    xFX_SceneEnter(world);
    zFXGoo_SceneEnter();
    init_poppers();
    setup_entrails(*globals.sceneCur);

    static tweak_callback cb_spawn_bubble_wall =
        tweak_callback::create_change(on_spawn_bubble_wall);
    xDebugAddTweak("FX|Spawn Bubble Wall", "Go!", &cb_spawn_bubble_wall, NULL, 0);
}

xVec3 bubblehit_pos_rnd = { 0.25f, 0.25f, 0.25f };
xVec3 bubblehit_vel_rnd = { 6.0f, 6.0f, 6.0f };
xVec3 bubbletrail_pos_rnd = { 0.25f, 0.25f, 0.25f };
xVec3 bubbletrail_vel_rnd = { 0.25f, 0.25f, 0.25f };

F32 bubblehit_vel_scale = 1.0f;

void zFX_SceneExit(RpWorld* world)
{
    xFX_SceneExit(world);
    zFXGoo_SceneExit();
}

void zFX_SceneReset()
{
    zFXGoo_SceneReset();
    reset_poppers();
    reset_entrails();
}

void zFXPatrickStun(const xVec3* pos)
{
    xFXRingCreate(pos, &sPatrickStunRing[0]);
    xFXRingCreate(pos, &sPatrickStunRing[1]);
    xFXRingCreate(pos, &sPatrickStunRing[2]);
}

void zFXHammer(const xVec3* pos)
{
    xFXRingCreate(pos, &sHammerRing[0]);
    zFX_SpawnBubbleSlam(pos, (xrand() & 31) + 32, 0.15f, 12.0f, 2.0f);
}

void zFXPorterWave(const xVec3* pos)
{
    xFXRingCreate(pos, &sPorterRing[0]);
    xFXRingCreate(pos, &sPorterRing[1]);
}

xFXRing* zFXMuscleArmWave(const xVec3* pos)
{
    return xFXRingCreate(pos, &sMuscleArmRing[0]);
}

static ztextbox* goo_timer_textbox = NULL;
static RwTexture* g_txtr_gooFrozen = NULL;
// WIP
void zFXGooEnable(RpAtomic* atomic, S32 freezeGroup)
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;
    g_txtr_gooFrozen = NULL;
    for (i = 0; i < 24; i++, goo++)
    {
        if (goo->state == zFXGooStateInactive)
        {
            break;
        }
        if (goo->atomic == atomic)
        {
            return;
        }
    }
    if (i == 24)
    {
        return;
    }

    goo->freezeGroup = freezeGroup;
    RpGeometry* geom = RpAtomicGetGeometry(atomic);
    S32 numVertices = geom->numVertices;
    S32 numTriangles = geom->numTriangles;
    if (geom->preLitLum == NULL)
    {
        RpGeometry* new_geom = RpGeometryCreate(numVertices, numTriangles, 0x7E);
        RpMorphTarget* new_morph = new_geom->morphTarget;
        RwV3d* verts = geom->morphTarget->verts;
        RwV3d* new_verts = new_morph->verts;
        RwV3d* normals = geom->morphTarget->normals;
        RwV3d* new_normals = new_morph->normals;
        RwRGBA* new_preLitLum = new_geom->preLitLum;
        RwTexCoords* texCoords = geom->texCoords[0];
        RwTexCoords* new_texCoords = new_geom->texCoords[0];
        for (i = 0; i < numVertices; i++, verts++, new_verts++, normals++, new_normals++,
            new_preLitLum++, texCoords++, new_texCoords++)
        {
            new_verts[0] = verts[0];
            new_normals[0] = normals[0];

            new_preLitLum[0].red = 0xff;
            new_preLitLum[0].green = 0xff;
            new_preLitLum[0].blue = 0xff;
            new_preLitLum[0].alpha = 0xff;

            new_texCoords[0] = texCoords[0];
        }

        RpTriangle* orig_triangles = geom->triangles;
        RpTriangle* triangles = new_geom->triangles;
        for (i = 0; i < numTriangles; i++, orig_triangles++, triangles++)
        {
            RwUInt16 vert1, vert2, vert3;
            RpGeometryTriangleGetVertexIndices(geom, orig_triangles, &vert1, &vert2, &vert3);
            RpGeometryTriangleSetVertexIndices(new_geom, triangles, vert1, vert2, vert3);

            RpMaterial* orig_material = RpGeometryTriangleGetMaterial(geom, orig_triangles);
            RpGeometryTriangleSetMaterial(new_geom, triangles, orig_material);
        }

        RwSphere boundingSphere;
        RpMorphTargetCalcBoundingSphere(new_morph, &boundingSphere);
        new_morph->boundingSphere = boundingSphere;
        RpGeometryUnlock(new_geom);
        RpAtomicSetGeometry(atomic, new_geom, 0);
        geom = new_geom;
    }

    xVec3* orig_verts = (xVec3*)xMemAllocSize(sizeof(xVec3) * numVertices);
    RwRGBA* orig_colors = (RwRGBA*)xMemAllocSize(sizeof(RwRGBA) * numVertices);
    RwTexCoords* orig_uvs = (RwTexCoords*)xMemAllocSize(sizeof(RwTexCoords) * numVertices);
    memcpy(orig_verts, geom->morphTarget->verts, (S32)sizeof(xVec3) * numVertices);
    memcpy(orig_colors, geom->preLitLum, (S32)sizeof(RwRGBA) * numVertices);
    memcpy(orig_uvs, geom->texCoords[0], (S32)sizeof(RwTexCoords) * numVertices);
    RpAtomicSetRenderCallBack(atomic, &zFXGooRenderAtomic);
    goo->atomic = atomic;
    goo->orig_verts = orig_verts;
    goo->orig_colors = orig_colors;
    goo->orig_uvs = orig_uvs;
    memcpy(goo->warbc, defaultGooWarbc, sizeof(defaultGooWarbc));
    goo->w0 = goo->warbc[0];
    goo->w2 = goo->warbc[2];
    memcpy(goo->state_time, defaultGooTimes, sizeof(defaultGooTimes));

    goo->state = zFXGooStateNormal;
    goo->time = 0.0f;
    goo->min = 3.0f;
    goo->max = 4.0f;
    goo->ref_parentPos = NULL;

    if (gCurEnv != NULL)
    {
        gCurEnv->easset->climateFlags = 2;
        gCurEnv->easset->climateStrengthMin = 0.0f;
        gCurEnv->easset->climateStrengthMax = 0.0f;
        xClimateInitAsset(&gClimate, gCurEnv->easset);
    }
}

void zFXGoo_SceneEnter()
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;
    for (i = 0; i < 24; i++)
    {
        memset(&goo->atomic, 0, sizeof(RpAtomic*));
        goo->state = zFXGooStateInactive;
        goo++;
    }
    U32 gameID = xStrHash("FREEZY_TIMER_TEXTBOX");
    goo_timer_textbox = (ztextbox*)zSceneFindObject(gameID);
}

void zFXGoo_SceneReset()
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;

    for (i = 0; i < 24;)
    {
        if (goo->state != zFXGooStateInactive)
        {
            goo->state = zFXGooStateNormal;
        }
        i++;
        goo++;
    }
}

void zFXGoo_SceneExit()
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;
    for (i = 0; i < 0x18; i++)
    {
        memset(&goo->atomic, 0, sizeof(RpAtomic*));
        goo->state = zFXGooStateInactive;
        goo++;
    }
}

void zFXGooUpdateInstance(zFXGooInstance* goo, F32 dt)
{
    S32 s;
    zFXGooState old_state = goo->state;
    if (goo->state != zFXGooStateNormal)
    {
        goo->time += dt;
        if (goo->time >= goo->timer)
        {
            goo->state = (zFXGooState)(((S32)goo->state + 1) & 0x3);
            goo->timer = (goo->time + goo->state_time[goo->state]) - (goo->time - goo->timer);
        }
    }

    if (xabs(goo->state_time[goo->state] < 1e-5f))
    {
        goo->state_time[goo->state] = 1e-5f;
    }
    goo->alpha = 1.0f - ((goo->timer - goo->time) / goo->state_time[goo->state]);
    goo->alpha = CLAMP(goo->alpha, 0.0f, 1.0f);

    switch (goo->state)
    {
    case zFXGooStateNormal:
    {
        if (old_state == zFXGooStateMelting)
        {
            zGooMeltFinished(goo->atomic);
        }
        goo->alpha = 0.0f;
        break;
    }
    case zFXGooStateFreezing:
    {
        F32 rate = xpow(1.1f, 60.0f * dt);
        goo->min *= rate;
        goo->max *= rate;
        break;
    }
    case zFXGooStateFrozen:
    {
        goo->alpha = 1.0f;
        if (goo->timer - goo->time <= 2.5f)
        {
            xClimateSetSnow(0.0f);
            if (!xSndIsPlaying(0x7bc0c0ce))
            {
                xSndPlay3D(0x7bc0c0ce, 10.0f, 0.0f, 0x80, 0x10000, &goo->center, 0.0f, SND_CAT_GAME,
                           0.0f);
            }
        }
        break;
    }
    case zFXGooStateMelting:
    {
        goo->alpha = 1.0f - goo->alpha;
        F32 rate = xpow(0.9090909f, 60.0f * dt);
        goo->min *= rate;
        goo->max *= rate;
        break;
    }
    }

    goo->warbc[0] = goo->w0 * (1.0f - goo->alpha);
    goo->warbc[2] = goo->w2 * (1.0f - goo->alpha);
    F32 tmp = xpow(1.0f - goo->alpha, 1.5f);
    goo->warb_time += tmp * dt;

    if (goo->alpha < 1.0f && goo->atomic != NULL)
    {
        RpGeometry* geom = RpAtomicGetGeometry(goo->atomic);
        if (geom != NULL && RpGeometryLock(geom, 2))
        {
            F32 warb_time = goo->warb_time;
            xVec3* verts = goo->orig_verts;
            RwV3d* morphVerts = geom->morphTarget->verts;
            for (s = 0; s < geom->numVertices; s++, verts++, morphVerts++)
            {
                F32 a = xfmod(goo->warbc[1] * (verts->x + warb_time), 2 * PI);
                F32 b = xfmod(goo->warbc[3] * (verts->z + warb_time), 2 * PI);
                F32 c = isin(a);

                a = goo->warbc[0] * c + verts->y;

                F32 d = isin(b);

                morphVerts->y = goo->warbc[2] * d + a;
            }

            RpGeometryUnlock(geom);
        }
    }

    if (goo_timer_textbox != NULL)
    {
        F32 freeze_time = zFXGooFreezeTimeLeft();

        if (freeze_time > 0.0f)
        {
            S32 len = freeze_time;
            if (len > 0x63)
            {
                len = 0x63;
            }

            static char counter_text[] = "##:##";
            sprintf(counter_text, "%02d", len);
            goo_timer_textbox->set_text(counter_text);
            goo_timer_textbox->activate();
        }
        else
        {
            goo_timer_textbox->deactivate();
        }
    }
}

void zFXGooUpdate(F32 dt)
{
    S32 i;
    zFXGooInstance* pGoo = &zFXGooInstances[0];

    for (i = 0; i < 24; i++)
    {
        if (pGoo->state != zFXGooStateInactive)
        {
            zFXGooUpdateInstance(pGoo, dt);
        }
        pGoo++;
    }
}

RpAtomic* zFXGooRenderAtomic(class RpAtomic* atomic)
{
    if (g_txtr_gooFrozen == NULL)
    {
        g_txtr_gooFrozen = (RwTexture*)xSTFindAsset(0x13401f, NULL);
    }

    // The default atomic render must run unconditionally: this callback replaces
    // the goo entity's own render callback, so skipping it makes the goo surface
    // invisible. Retail's `bf 2, <+6>` jumps only past the xSTFindAsset block and
    // falls straight into `lwz 12 / mtctr / bctrl`.
    (*gAtomicRenderCallBack)(atomic);

    S32 i;
    zFXGooInstance* goo = zFXGooInstances;
    for (i = 0; i < 24; i++, goo++)
    {
        if (goo->state == zFXGooStateInactive)
        {
            continue;
        }
        if (goo->atomic == atomic)
        {
            break;
        }
    }

    xDrawSetColor(0xff, 0x80, 0, 0xff);

    xVec3 refPos;
    if (goo->ref_parentPos != NULL)
    {
        refPos = *goo->ref_parentPos;
    }
    else
    {
        refPos = g_O3;
    }

    if (i != 24 && goo->state != zFXGooStateInactive && goo->state != zFXGooStateNormal)
    {
        RwIm3DVertex* vertexBuffer = gRenderBuffer.m_vertex;
        U32 numVerts = 0;
        RwIm3DVertex* currentVertBuf;
        if (g_txtr_gooFrozen != NULL)
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RwTextureGetRaster(g_txtr_gooFrozen));
        }
        else
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
        }
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

        RpGeometry* geom = RpAtomicGetGeometry(atomic);
        RwV3d* verts = geom->morphTarget->verts;
        RwRGBA* preLitLum = geom->preLitLum;
        if (xabs(goo->max - goo->min) < 1e-5f)
        {
            goo->max += 1e-5f;
        }

        F32 a = (-255.0f * goo->alpha) / (goo->max - goo->min);
        F32 b = (255.0f * goo->alpha * goo->min) / (goo->max - goo->min);

        U8* bytes = (U8*)xMemPushTemp(geom->numVertices);

        for (i = 0; i < geom->numVertices; i++)
        {
            xVec3 tmp;
            xVec3Sub(&tmp, (xVec3*)&verts[i], &goo->center);
            F32 c = a * xVec3Length2(&tmp) + b;
            c = CLAMP(c, 0.0f, 255.0f);
            bytes[i] = (U8)c;
        }

        RpTriangle* tri = geom->triangles;
        for (i = 0; i < geom->numTriangles; i++, tri++)
        {
            if (numVerts > 0x1dd)
            {
                if (RwIm3DTransform(vertexBuffer, numVerts, NULL, 0x19) != NULL)
                {
                    RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
                    RwIm3DEnd();
                }
                numVerts = 0;
            }

            currentVertBuf = &vertexBuffer[numVerts];
            xVec3 a = *(xVec3*)&verts[tri->vertIndex[0]];
            xVec3 b = *(xVec3*)&verts[tri->vertIndex[1]];
            xVec3 c = *(xVec3*)&verts[tri->vertIndex[2]];

            a += refPos;
            b += refPos;
            c += refPos;
            numVerts += 3;

            F32 ax = a.x;
            F32 ay = 0.02f + a.y;
            F32 az = a.z;
            RwIm3DVertexSetPos(&currentVertBuf[0], ax, ay, az);
            F32 bx = b.x;
            F32 by = 0.02f + b.y;
            F32 bz = b.z;
            RwIm3DVertexSetPos(&currentVertBuf[1], bx, by, bz);
            F32 cx = c.x;
            F32 cy = 0.02f + c.y;
            F32 cz = c.z;
            RwIm3DVertexSetPos(&currentVertBuf[2], cx, cy, cz);

            RwUInt16 ai = tri->vertIndex[0];
            U8 ar = preLitLum[ai].red;
            U8 ag = preLitLum[ai].green;
            U8 ab = preLitLum[ai].blue;
            U8 aa = bytes[ai];
            RwIm3DVertexSetRGBA(&currentVertBuf[0], ar, ag, ab, aa);

            RwUInt16 bi = tri->vertIndex[1];
            U8 br = preLitLum[bi].red;
            U8 bg = preLitLum[bi].green;
            U8 bb = preLitLum[bi].blue;
            U8 ba = bytes[bi];
            RwIm3DVertexSetRGBA(&currentVertBuf[1], br, bg, bb, ba);

            RwUInt16 ci = tri->vertIndex[2];
            U8 cr = preLitLum[ci].red;
            U8 cg = preLitLum[ci].green;
            U8 cb = preLitLum[ci].blue;
            U8 ca = bytes[ci];
            RwIm3DVertexSetRGBA(&currentVertBuf[2], cr, cg, cb, ca);

            currentVertBuf[0].u = goo->orig_uvs[tri->vertIndex[0]].u;
            currentVertBuf[1].u = goo->orig_uvs[tri->vertIndex[1]].u;
            currentVertBuf[2].u = goo->orig_uvs[tri->vertIndex[2]].u;
            currentVertBuf[0].v = goo->orig_uvs[tri->vertIndex[0]].v;
            currentVertBuf[1].v = goo->orig_uvs[tri->vertIndex[1]].v;
            currentVertBuf[2].v = goo->orig_uvs[tri->vertIndex[2]].v;
        }

        if (RwIm3DTransform(vertexBuffer, numVerts, NULL, 0x19) != NULL)
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
            RwIm3DEnd();
        }
        xMemPopTemp(bytes);
    }

    return atomic;
}

void zFXUpdate(F32 dt)
{
    zFXGooUpdate(dt);
    update_poppers(dt);
    update_entrails(dt);
    xFXUpdate(dt);
}

void zFXGooFreeze(RpAtomic* atomic, const xVec3* center, xVec3* ref_parPosVec)
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;
    S32 freezeGroup = -1;

    for (i = 0; i < 24; i++, goo++)
    {
        if ((goo->state == 0) && (goo->atomic == atomic))
        {
            freezeGroup = goo->freezeGroup;
            break;
        }
    }

    if (freezeGroup != -1)
    {
        xSndPlay3D(0x7bc0c0ce, 10.0f, 0.0f, 0x80, 0, center, 0.0f, SND_CAT_GAME, 0.0f);
        xSndPlay3D(0xb9b1d325, 10.0f, 0.0f, 0x80, 0, center, 0.0f, SND_CAT_GAME, 0.0f);
        xClimateSetSnow(1.0f);

        goo = zFXGooInstances;
        for (i = 0; i < 24; i++, goo++)
        {
            if (goo->freezeGroup == freezeGroup)
            {
                goo->state = zFXGooStateFreezing;
                goo->time = 0.0f;
                goo->timer = goo->time + goo->state_time[goo->state];

                goo->center = *center;
                goo->min = 3.0f;
                goo->max = 4.0f;
                goo->ref_parentPos = ref_parPosVec;

                if (ref_parPosVec != NULL)
                {
                    goo->pos_parentOnFreeze = *ref_parPosVec;
                }
                else
                {
                    goo->pos_parentOnFreeze = g_O3;
                }
            }
        }
    }
}

S32 zFXGooIs(xEnt* obj, F32& depth, U32 playerCheck)
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;

    for (i = 0; i < 24; i++, goo++)
    {
        if (goo->state == zFXGooStateInactive)
        {
            continue;
        }
        if (goo->atomic == obj->model->Data)
        {
            break;
        }
    }

    if (i == 24)
    {
        return TRUE;
    }

    if (goo->state == zFXGooStateNormal)
    {
        return TRUE;
    }

    xVec3 pos;
    xVec3Sub(&pos, (xVec3*)&globals.player.ent.model->Mat->pos, &goo->center);

    if (xVec3Dot(&pos, &pos) > goo->max)
    {
        return TRUE;
    }
    depth = 0.0f;
    if (playerCheck)
    {
        globals.player.ForceSlipperyFriction = 0.2f;
        globals.player.ForceSlipperyTimer = 0.1f;
    }

    return FALSE;
}

void zFXGooEventSetWarb(xEnt* ent, const F32* warb)
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;
    for (i = 0; i < 24; i++, goo++)
    {
        if (goo->state == zFXGooStateInactive)
        {
            continue;
        }
        if (goo->atomic == ent->model->Data)
        {
            break;
        }
    }

    if (i == 24)
    {
        return;
    }

    memcpy(goo->warbc, warb, sizeof(goo->warbc));
    goo->w0 = goo->warbc[0];
    goo->w2 = goo->warbc[2];
}

void zFXGooEventSetFreezeDuration(xEnt* ent, F32 duration)
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;
    S32 freezeGroup = -1;
    for (i = 0; i < 24; i++, goo++)
    {
        if (goo->state == zFXGooStateInactive)
        {
            continue;
        }
        if (goo->atomic == ent->model->Data)
        {
            freezeGroup = goo->freezeGroup;
            break;
        }
    }

    if (freezeGroup == -1)
    {
        return;
    }

    goo = zFXGooInstances;
    for (i = 0; i < 24; i++, goo++)
    {
        if (goo->freezeGroup == freezeGroup)
        {
            goo->state_time[zFXGooStateFrozen] = duration;
        }
    }
}

void zFXGooEventMelt(xEnt* ent)
{
    S32 i;
    zFXGooInstance* goo = zFXGooInstances;
    S32 freezeGroup = -1;
    for (i = 0; i < 24; i++, goo++)
    {
        if (goo->state != zFXGooStateFrozen)
        {
            continue;
        }
        if (goo->atomic == ent->model->Data)
        {
            freezeGroup = goo->freezeGroup;
            break;
        }
    }

    if (freezeGroup == -1)
    {
        return;
    }

    goo = zFXGooInstances;
    for (i = 0; i < 24; i++, goo++)
    {
        if (goo->freezeGroup == freezeGroup)
        {
            goo->timer = 0.0f;
        }
    }
}

F32 zFXGooFreezeTimeLeft()
{
    zFXGooInstance* goo = zFXGooInstances;
    zFXGooInstance* end = goo + 24;
    F32 maxTime = 0.0f;
    for (; goo != end; goo++)
    {
        F32 time = 0.0f;
        switch (goo->state)
        {
        case zFXGooStateFreezing:
        {
            time += goo->state_time[zFXGooStateFrozen];
        }
        case zFXGooStateFrozen:
        {
            time += goo->state_time[zFXGooStateMelting];
        }
        case zFXGooStateMelting:
        {
            time += (goo->timer - goo->time);
            break;
        }
        }

        if (maxTime < time)
        {
            maxTime = time;
        }
    }
    return maxTime;
}

void zFX_SpawnBubbleHit(const xVec3* pos, U32 num, const xVec3* pos_rnd, const xVec3* vel_rnd,
                        float vel_scale);
void zFX_SpawnBubbleTrail(const xVec3* pos, U32 num, const xVec3* pos_rnd, const xVec3* vel_rnd);
void zFX_SpawnBubbleTrailNoNegRandVel(const xVec3* pos, U32 num, const xVec3* pos_rnd,
                                      const xVec3* vel_rnd);

void zFX_SpawnBubbleHit(const xVec3* pos, U32 num)
{
    zFX_SpawnBubbleHit(pos, num, &bubblehit_pos_rnd, &bubblehit_vel_rnd, bubblehit_vel_scale);
}

void zFX_SpawnBubbleHit(const xVec3* pos, U32 num, const xVec3* pos_rnd, const xVec3* vel_rnd,
                        float vel_scale)
{
    if (num == 0)
    {
        return;
    }
    if (pos_rnd == NULL)
    {
        pos_rnd = &bubblehit_pos_rnd;
    }
    if (vel_rnd == NULL)
    {
        vel_rnd = &bubblehit_vel_rnd;
    }

    xVec3* posbuf = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;
    if (posbuf == NULL)
    {
        return;
    }

    xVec3* pp = posbuf;
    xVec3* vp = velbuf;
    for (S32 j = 0; j < (S32)num; j++, pp++, vp++)
    {
        *pp = *pos;
        pp->x = pos_rnd->x * (xurand() - 0.5f) + pp->x;
        pp->y = pos_rnd->y * (xurand() - 0.5f) + pp->y;
        pp->z = pos_rnd->z * (xurand() - 0.5f) + pp->z;
        vp->x = xurand() - 0.5f;
        vp->y = xurand() - 0.5f;
        vp->z = xurand() - 0.5f;
        xVec3NormalizeFast(vp, vp);
        xVec3ScaleC(vp, vp, vel_scale, vel_scale, vel_scale);
        vp->x = vel_rnd->x * (xurand() - 0.5f) + vp->x;
        vp->y = vel_rnd->y * (xurand() - 0.5f) + vp->y;
        vp->z = vel_rnd->z * (xurand() - 0.5f) + vp->z;
    }

    zParPTankSpawnBubbles(posbuf, velbuf, num, 1.0f);
    xMemPopTemp(posbuf);
}

void zFX_SpawnBubbleTrail(const xVec3* pos, U32 num)
{
    zFX_SpawnBubbleTrail(pos, num, &bubbletrail_pos_rnd, &bubbletrail_vel_rnd);
}

void zFX_SpawnBubbleTrail(const xVec3* pos, U32 num, const xVec3* pos_rnd, const xVec3* vel_rnd)
{
    if (num < 1)
    {
        return;
    }
    if (pos_rnd == NULL)
    {
        pos_rnd = &bubbletrail_pos_rnd;
    }
    if (vel_rnd == NULL)
    {
        vel_rnd = &bubbletrail_vel_rnd;
    }

    xVec3* posbuf = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;
    if (posbuf == NULL)
    {
        return;
    }

    xVec3* pp = posbuf;
    xVec3* vp = velbuf;
    for (S32 j = 0; j < (S32)num; j++, pp++, vp++)
    {
        *pp = *pos;
        pp->x = pos_rnd->x * (xurand() - 0.5f) + pp->x;
        pp->y = pos_rnd->y * (xurand() - 0.5f) + pp->y;
        pp->z = pos_rnd->z * (xurand() - 0.5f) + pp->z;
        vp->x = vel_rnd->x * (xurand() - 0.5f);
        vp->y = vel_rnd->y * (xurand() - 0.5f);
        vp->z = vel_rnd->z * (xurand() - 0.5f);
    }

    zParPTankSpawnBubbles(posbuf, velbuf, num, 1.0f);
    xMemPopTemp(posbuf);
}

void zFX_SpawnBubbleTrailNoNegRandVel(const xVec3* pos, U32 num, const xVec3* pos_rnd,
                                      const xVec3* vel_rnd)
{
    if (pos_rnd == NULL)
    {
        pos_rnd = &bubbletrail_pos_rnd;
    }
    if (vel_rnd == NULL)
    {
        vel_rnd = &bubbletrail_vel_rnd;
    }

    xVec3* posbuf = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;
    if (posbuf == NULL)
    {
        return;
    }

    xVec3* pp = posbuf;
    xVec3* vp = velbuf;
    for (S32 j = 0; j < (S32)num; j++, pp++, vp++)
    {
        *pp = *pos;
        pp->x = pos_rnd->x * (xurand() - 0.5f) + pp->x;
        pp->y = pos_rnd->y * (xurand() - 0.5f) + pp->y;
        pp->z = pos_rnd->z * (xurand() - 0.5f) + pp->z;
        vp->x = vel_rnd->x * xurand();
        vp->y = vel_rnd->y * xurand();
        vp->z = vel_rnd->z * xurand();
    }

    zParPTankSpawnBubbles(posbuf, velbuf, num, 1.0f);
    xMemPopTemp(posbuf);
}

void zFX_SpawnBubbleTrail(const xVec3* pos_beg, const xVec3* pos_end, U32 num, const xVec3* pos_rnd,
                          const xVec3* vel_rnd)
{
    if (pos_rnd == NULL)
    {
        pos_rnd = &bubbletrail_pos_rnd;
    }
    if (vel_rnd == NULL)
    {
        vel_rnd = &bubbletrail_vel_rnd;
    }

    xVec3* posbuf = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;
    if (posbuf == NULL)
    {
        return;
    }

    xVec3 offset = *pos_end - *pos_beg;

    xVec3* pp = posbuf;
    xVec3* vp = velbuf;
    for (S32 j = 0; j < (S32)num; j++, pp++, vp++)
    {
        *pp = *pos_beg + offset * xurand();
        pp->x = pos_rnd->x * (xurand() - 0.5f) + pp->x;
        pp->y = pos_rnd->y * (xurand() - 0.5f) + pp->y;
        pp->z = pos_rnd->z * (xurand() - 0.5f) + pp->z;
        vp->x = vel_rnd->x * (xurand() - 0.5f);
        vp->y = vel_rnd->y * (xurand() - 0.5f);
        vp->z = vel_rnd->z * (xurand() - 0.5f);
    }

    zParPTankSpawnBubbles(posbuf, velbuf, num, 1.0f);
    xMemPopTemp(posbuf);
}

void zFX_SpawnBubbleTrail(const xVec3* pos_beg, const xVec3* pos_end, const xVec3* vel_beg,
                          const xVec3* vel_end, U32 num, const xVec3* pos_rnd, const xVec3* vel_rnd,
                          F32 size)
{
    if (pos_rnd == NULL)
    {
        pos_rnd = &bubbletrail_pos_rnd;
    }
    if (vel_rnd == NULL)
    {
        vel_rnd = &bubbletrail_vel_rnd;
    }

    xVec3* posbuf = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;
    if (posbuf == NULL)
    {
        return;
    }

    xVec3 offset = *pos_end - *pos_beg;
    xVec3 vel_offset = *vel_end - *vel_beg;

    xVec3* pp = posbuf;
    xVec3* vp = velbuf;
    for (S32 j = 0; j < (S32)num; j++, pp++, vp++)
    {
        F32 t = xurand();
        *pp = *pos_beg + offset * t;
        *vp = *vel_beg + vel_offset * t;
        pp->x = pos_rnd->x * (xurand() - 0.5f) + pp->x;
        pp->y = pos_rnd->y * (xurand() - 0.5f) + pp->y;
        pp->z = pos_rnd->z * (xurand() - 0.5f) + pp->z;
        vp->x = vel_rnd->x * (xurand() - 0.5f) + vp->x;
        vp->y = vel_rnd->y * (xurand() - 0.5f) + vp->y;
        vp->z = vel_rnd->z * (xurand() - 0.5f) + vp->z;
    }

    zParPTankSpawnBubbles(posbuf, velbuf, num, size);
    xMemPopTemp(posbuf);
}

void zFX_SpawnBubbleMenuTrail(const xVec3* pos, U32 num, const xVec3* pos_rnd, const xVec3* vel_rnd)
{
    if (pos_rnd == NULL)
    {
        pos_rnd = &bubbletrail_pos_rnd;
    }
    if (vel_rnd == NULL)
    {
        vel_rnd = &bubbletrail_vel_rnd;
    }

    xVec3* posbuf = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;
    if (posbuf == NULL)
    {
        return;
    }

    xVec3* pp = posbuf;
    xVec3* vp = velbuf;
    for (S32 j = 0; j < (S32)num; j++, pp++, vp++)
    {
        *pp = *pos;
        pp->x = pos_rnd->x * (xurand() - 0.5f) + pp->x;
        pp->y = pos_rnd->y * (xurand() - 0.5f) + pp->y;
        pp->z = pos_rnd->z * (xurand() - 0.5f) + pp->z;
        vp->x = vel_rnd->x * (xurand() - 0.5f);
        vp->y = vel_rnd->y * (xurand() - 0.5f);
        vp->z = vel_rnd->z * (xurand() - 0.5f);
    }

    zParPTankSpawnMenuBubbles(posbuf, velbuf, num);
    xMemPopTemp(posbuf);
}

void zFX_SpawnBubbleWall()
{
    RwCamera* camera = RwCameraGetCurrentCamera();
    if (camera == NULL)
    {
        return;
    }

    RwMatrix* mat = RwFrameGetMatrix(RwCameraGetFrame(camera));

    xVec3 pos[100];
    xVec3 vel[100];

    // Retail hoisted these six loads out of the loop. Our compiler does not, so
    // they are hoisted by hand; the declaration order fixes the register
    // assignment and the assignment order fixes the emitted load order.
    F32 sy, sx, vsz, vsy, vsx, sz;
    sx = bubblewall_scale.x;
    sy = bubblewall_scale.y;
    sz = bubblewall_scale.z;
    vsx = bubblewall_velscale.x;
    vsy = bubblewall_velscale.y;
    vsz = bubblewall_velscale.z;

    xVec3* pp = pos;
    xVec3* vp = vel;
    for (U32 i = 0; i < 50; i++, pp++, vp++)
    {
        pp->x = mat->pos.x + (xurand() - 0.5f) + sx * (xurand() - 0.5f);
        pp->y = mat->pos.y + (xurand() - 0.5f) + sy * (xurand() - 0.5f);
        pp->z = mat->pos.z + (xurand() - 0.5f) + sz * (xurand() - 0.5f);

        xVec3 offset;
        xVec3ScaleC(&offset, (xVec3*)&mat->at, 1.2f, 1.2f, 1.2f);
        xVec3Add(pp, pp, &offset);

        vp->x = vsx * (xurand() - 0.5f);
        vp->y = vsy * (xurand() - 0.5f);
        vp->z = vsz * (xurand() - 0.5f);
    }

    zParPTankSpawnBubbles(pos, vel, 50, 1.0f);
}

void zFX_SpawnBubbleSlam(const xVec3* pos, U32 num, F32 rang, F32 bvel, F32 rvel)
{
    xVec3* posbuf = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;
    if (posbuf == NULL)
    {
        return;
    }

    F32 yvel = 0.25f * rvel;

    xVec3* pp = posbuf;
    xVec3* vp = velbuf;
    for (U32 j = 0; j < num; j++, pp++, vp++)
    {
        *pp = *pos;
        pp->y += 0.2f;

        F32 ang = (2 * PI * j) / num;
        ang = rang * (xurand() - 0.5f) + ang;
        vp->x = bvel * icos(ang);
        vp->y = 0.0f;
        vp->z = bvel * isin(ang);
        vp->x = rvel * (xurand() - 0.5f) + vp->x;
        vp->y = yvel * (xurand() - 0.5f) + vp->y;
        vp->z = rvel * (xurand() - 0.5f) + vp->z;
    }

    zParPTankSpawnBubbles(posbuf, velbuf, num, 1.0f);
    xMemPopTemp(posbuf);
}

void zFX_SpawnBubbleBlast(const xVec3* pos, U32 num, F32 radius, F32 blast_vel, F32 rand_vel)
{
    xVec3* end;
    xVec3* buffer = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    if (buffer == NULL)
    {
        return;
    }

    end = buffer + num;
    xVec3* itl = buffer;
    xVec3* itv = end;
    for (; itl != end; itl++, itv++)
    {
        F32 ang = 2 * PI * xurand();
        F32 uz = 2.0f * xurand() - 1.0f;
        F32 r = xsqrt(-(uz * uz - 1.0f));
        itv->assign(r * icos(ang), r * isin(ang), uz);
        *itl = *pos + *itv * radius;
        *itv *= blast_vel;

        xVec3 rvel = { 0.0f, 0.0f, 0.0f };
        rvel.x = xurand() - 0.5f;
        rvel.y = xurand() - 0.5f;
        rvel.z = xurand() - 0.5f;
        *itv += rvel * rand_vel;
    }

    zParPTankSpawnBubbles(buffer, end, num, 1.0f);
    xMemPopTemp(buffer);
}

namespace
{
    struct entrail_data
    {
        U16 flags;
        U16 type;
        xEnt* ent;
        xVec3 loc;
        xVec3 vel;
        F32 emitted;

        void reset();
        void update(F32);
    };

    struct entrail_type
    {
        char* model_name;
        S32 bone;
        F32 rate;
        F32 cull_dist;
        xVec3 offset;
        xVec3 offset_rand;
        xVec3 vel;
        xVec3 vel_rand;
    };

    entrail_type entrail_types[2] = {
        { "hovering_platform_wheel_bind", 0, 70.0f, 25.0f, { 0.0f, 0.5f, 0.0f },
          { 0.4f, 0.4f, 0.4f }, { 0.0f, -3.0f, 0.0f }, { 1.5f, 0.0f, 1.5f } },
        { "plat_support_double_bind", 0, 70.0f, 25.0f, { 0.0f, -2.0f, 0.0f },
          { 0.4f, 0.4f, 0.4f }, { 0.0f, -3.0f, 0.0f }, { 1.5f, 0.0f, 1.5f } }
    };

    enum state_enum
    {
        STATE_NONE,
        STATE_OFF,
        STATE_ON
    };

    struct popper_data
    {
        state_enum state;
        xEnt* ent;
        RpAtomic* atomic[4];
        U32 atomic_size;
        F32 time;
        F32 end_time;
        union
        {
            xVec3 model_scale;
            U32 pipe_flags;
        };
        F32 rate;
        F32 vel;
        F32 rloc;
        F32 rvel;
        F32 emitted;
        S32 faces;
        F32 radius;
        F32 area;
        F32 weight[768];

        S32 find_weight(F32 w) const;
    };

    popper_data poppers[8];
    entrail_data* entrails;
    U32 entrails_size;

    S32 count_faces(xModelInstance* mdl);
    void add_popper_tweaks();
    void add_entrail_tweaks();
    bool validate_popper(const xEnt& ent);
    F32 get_triangle_area(const xVec3& a, const xVec3& b, const xVec3& c);
    void eval_tri(xVec3* vert, xVec3* norm, const xMat4x3* mat, const RpGeometry* geom,
                  const RpTriangle* tri);
    void SkinXformVertAndNormal(xVec3* dst_verts, xVec3* dst_normals, const xVec3* verts,
                                const xVec3* normals, const xMat4x3* mat, const xMat4x3* bone_mats,
                                const F32* weights, const U32* bone_idx, const U16* idx, U32 count);
    void random_point_on_triangle(xVec3& loc, xVec3& norm, const xVec3* v, const xVec3* n);
    void emit_popper_bubbles(popper_data& popper, S32 emit, F32 scale, F32 vel_add);
    void set_popper_alpha(popper_data& data, F32 alpha);
    void destroy_popper(popper_data& data);
    void update_popper(popper_data& popper, F32 dt);

    bool model_is_preinstanced(RpAtomic* atomic)
    {
        RpGeometry* geom = RpAtomicGetGeometryMacro(atomic);
        if (geom == NULL)
        {
            return TRUE;
        }

        return !(geom->morphTarget != NULL && geom->morphTarget->verts != NULL);
    }

    bool setup_popper_emitter(popper_data& popper)
    {
        popper.faces = count_faces(popper.ent->model);
        if (popper.faces <= 0)
        {
            return FALSE;
        }
        if (popper.faces > 768)
        {
            return FALSE;
        }

        F32* w = popper.weight;
        popper.area = 0.0f;
        popper.atomic_size = 0;

        for (xModelInstance* mdl = popper.ent->model; mdl != NULL; mdl = mdl->Next)
        {
            if (popper.atomic_size >= 4)
            {
                break;
            }

            popper.atomic[popper.atomic_size] = mdl->Data;

            RpGeometry* geom = mdl->Data->geometry;
            const xVec3* vert = (const xVec3*)geom->morphTarget->verts;
            const RpTriangle* tri = geom->triangles;
            F32* end = w + geom->numTriangles;
            for (; w != end; w++)
            {
                popper.area += get_triangle_area(vert[tri->vertIndex[0]], vert[tri->vertIndex[1]],
                                               vert[tri->vertIndex[2]]);
                tri++;
                *w = popper.area;
            }

            popper.atomic_size++;
        }

        if (popper.atomic_size == 0)
        {
            return FALSE;
        }

        return TRUE;
    }

    F32 get_triangle_area(const xVec3& a, const xVec3& b, const xVec3& c)
    {
        xVec3 ab = { 0.0f, 0.0f, 0.0f };
        ab.x = b.x - a.x;
        ab.y = b.y - a.y;
        ab.z = b.z - a.z;

        xVec3 ac = { 0.0f, 0.0f, 0.0f };
        ac.x = c.x - a.x;
        ac.y = c.y - a.y;
        ac.z = c.z - a.z;

        xVec3 n = { 0.0f, 0.0f, 0.0f };
        n.x = ab.y * ac.z - ac.y * ab.z;
        n.y = ab.z * ac.x - ac.z * ab.x;
        n.z = ab.x * ac.y - ac.x * ab.y;

        return 0.5f * n.length();
    }

    S32 count_faces(xModelInstance* mdl)
    {
        int i = 0;
        for (; mdl != NULL; mdl = mdl->Next)
        {
            i += mdl->Data->geometry->numTriangles;
        }
        return i;
    }

    void eval_tri(xVec3* vert, xVec3* norm, const xMat4x3* mat, const RpGeometry* geom,
                  const RpTriangle* tri)
    {
        RpSkin* skin = RpSkinGeometryGetSkin((RpGeometry*)geom);
        const xVec3* in_vert = (const xVec3*)geom->morphTarget->verts;
        const xVec3* in_norm = (const xVec3*)geom->morphTarget->normals;

        if (skin != NULL)
        {
            const xMat4x3* skinmat = (const xMat4x3*)RpSkinGetSkinToBoneMatrices(skin);
            const F32* vert_bone_weight = (const F32*)RpSkinGetVertexBoneWeights(skin);
            const U32* vert_bone_index = RpSkinGetVertexBoneIndices(skin);

            SkinXformVertAndNormal(vert, norm, in_vert, in_norm, mat, skinmat, vert_bone_weight,
                                   vert_bone_index, tri->vertIndex, 3);
            return;
        }

        xMat4x3Toworld(&vert[0], mat, &in_vert[tri->vertIndex[0]]);
        xMat4x3Toworld(&vert[1], mat, &in_vert[tri->vertIndex[1]]);
        xMat4x3Toworld(&vert[2], mat, &in_vert[tri->vertIndex[2]]);
        xMat3x3RMulVec(&norm[0], mat, &in_norm[tri->vertIndex[0]]);
        norm[0].up_normalize();
        xMat3x3RMulVec(&norm[1], mat, &in_norm[tri->vertIndex[1]]);
        norm[1].up_normalize();
        xMat3x3RMulVec(&norm[2], mat, &in_norm[tri->vertIndex[2]]);
        norm[2].up_normalize();
    }

    void SkinXformVertAndNormal(xVec3* dst_verts, xVec3* dst_normals, const xVec3* verts,
                                const xVec3* normals, const xMat4x3* mat, const xMat4x3* bone_mats,
                                const F32* weights, const U32* bone_idx, const U16* idx, U32 count)
    {
        U32 done[2] = { 0, 0 };
        U32 mask;
        U32 shift;
        xMat4x3* scratch = (xMat4x3*)giAnimScratch;

        for (; count != 0; count--)
        {
            U16 vidx = *idx;
            const xVec3* vert = &verts[vidx];
            const xVec3* norm = &normals[vidx];
            U32 bones = bone_idx[vidx];
            const F32* wt = &weights[vidx * 4];

            shift = 0;
            for (U32 j = 0; j < 4; j++, shift += 8)
            {
                U32 b = bones >> shift;
                U32 word = (b >> 5) & 7;
                mask = 1 << (b & 0x1f);
                U32 bi = b & 0xff;
                if (!(mask & done[word]))
                {
                    xMat4x3Mul(&scratch[bi], &bone_mats[bi], &mat[bi] + 1);
                    done[word] |= mask;
                }
            }

            xVec3 acc = { 0.0f, 0.0f, 0.0f };

            U32 nbones = bones;
            const F32* w = wt;
            for (U32 k = 4; *w && k != 0; k--)
            {
                xVec3 tmp;
                U32 bi = bones & 0xff;
                bones >>= 8;
                xMat4x3Toworld(&tmp, &scratch[bi], vert);
                tmp *= *w;
                acc += tmp;
                w++;
            }
            xMat4x3Toworld(dst_verts, mat, &acc);

            acc = 0.0f;
            w = wt;
            for (U32 k = 4; *w && k != 0; k--)
            {
                xVec3 tmp;
                U32 bi = nbones & 0xff;
                nbones >>= 8;
                xMat3x3RMulVec(&tmp, &scratch[bi], norm);
                tmp *= *w;
                acc += tmp;
                w++;
            }
            xMat3x3RMulVec(dst_normals, mat, &acc);

            dst_verts++;
            dst_normals++;
            idx++;
        }
    }

    void random_point_on_triangle(xVec3& loc, xVec3& norm, const xVec3* v, const xVec3* n)
    {
        F32 s = xurand();
        F32 t = xurand();
        if (s + t > 1.0f)
        {
            s = 1.0f - s;
            t = 1.0f - t;
        }

        loc.x = v[0].x + s * (v[1].x - v[0].x) + t * (v[2].x - v[0].x);
        loc.y = v[0].y + s * (v[1].y - v[0].y) + t * (v[2].y - v[0].y);
        loc.z = v[0].z + s * (v[1].z - v[0].z) + t * (v[2].z - v[0].z);
        norm.x =
            n[0].x + s * (n[1].x - n[0].x) + t * (n[2].x - n[0].x);
        norm.y =
            n[0].y + s * (n[1].y - n[0].y) + t * (n[2].y - n[0].y);
        norm.z =
            n[0].z + s * (n[1].z - n[0].z) + t * (n[2].z - n[0].z);
        norm.up_normalize();
    }

    void random_surface_point(xVec3& loc, xVec3& norm, const popper_data& popper)
    {
        const xMat4x3* mat = (const xMat4x3*)popper.ent->model->Mat;

        S32 which = popper.find_weight(popper.area * xurand());

        RpGeometry* geom;
        RpAtomic* const* a = popper.atomic;
        RpAtomic* const* end = a + popper.atomic_size;
        for (; a != end; a++)
        {
            geom = (*a)->geometry;
            S32 size = geom->numTriangles;
            if (which < size)
            {
                break;
            }
            which -= size;
        }

        xVec3 v[3];
        xVec3 n[3];
        eval_tri(v, n, mat, geom, &geom->triangles[which]);
        random_point_on_triangle(loc, norm, v, n);
    }

    S32 popper_data::find_weight(F32 w) const
    {
        S32 lo = 0;
        S32 hi = faces;
        do
        {
            S32 mid = (lo + hi) / 2;
            if (w <= weight[mid])
            {
                hi = mid;
            }
            else
            {
                lo = mid + 1;
            }
        } while (lo < hi);

        return lo;
    }

    popper_data* find_popper(xEnt* ent)
    {
        popper_data* p = poppers;
        popper_data* end = &poppers[8];
        for (; p != end; p++)
        {
            if (p->ent == ent)
            {
                return p;
            }
        }
        return NULL;
    }

    popper_data* find_free_popper()
    {
        popper_data* found = NULL;
        popper_data* end = &poppers[8];
        for (popper_data* p = poppers; p != end; p++)
        {
            if (p->state == STATE_NONE)
            {
                if (p->ent == NULL)
                {
                    return p;
                }
                found = p;
            }
        }
        return found;
    }

    void emit_popper_bubbles(popper_data& popper, S32 emit, F32 scale, F32 vel_add)
    {
        if (popper.ent == NULL && popper.ent->model == NULL)
        {
            return;
        }

        S32 max_emit = zParPTankBubblesAvailable();
        if (max_emit <= 0)
        {
            return;
        }
        if (emit > max_emit)
        {
            emit = max_emit;
        }

        xVec3* end_loc;
        xVec3* buffer = (xVec3*)xMemPushTemp(2 * sizeof(xVec3) * emit);
        if (buffer == NULL)
        {
            return;
        }

        end_loc = buffer + emit;
        xVec3* loc = buffer;
        xVec3* v = end_loc;
        xModelInstance* model = popper.ent->model;

        xMat3x3 oldmat;
        if (model->Scale.x != 0.0f)
        {
            xMat3x3* mat = (xMat3x3*)model->Mat;
            oldmat = *mat;
            mat->right *= model->Scale.x;
            mat->up *= model->Scale.y;
            mat->at *= model->Scale.z;
        }

        F32 svel = popper.vel * scale + vel_add;
        F32 rloc = popper.rloc * scale;
        F32 rvel = popper.rvel * scale;

        for (; loc != end_loc; loc++, v++)
        {
            random_surface_point(*loc, *v, popper);
            *v *= svel;

            if (popper.rloc != 0.0f)
            {
                loc->x = rloc * (xurand() - 0.5f) + loc->x;
                loc->y = rloc * (xurand() - 0.5f) + loc->y;
                loc->z = rloc * (xurand() - 0.5f) + loc->z;
            }
            if (popper.rvel != 0.0f)
            {
                v->x = rvel * (xurand() - 0.5f) + v->x;
                v->y = rvel * (xurand() - 0.5f) + v->y;
                v->z = rvel * (xurand() - 0.5f) + v->z;
            }
        }

        if (model->Scale.x != 0.0f)
        {
            *(xMat3x3*)model->Mat = oldmat;
        }

        zParPTankSpawnBubbles(buffer, end_loc, emit, scale);
        xMemPopTemp(buffer);
    }

    void emit_popper_bubbles_immediate(popper_data& data)
    {
        F32 size;
        F32 count = data.rate;
        if (data.area <= 2.0f)
        {
            size = 1.0f;
            count = count * (0.5f * data.area);
        }
        else
        {
            size = 0.5f * data.area;
        }
        if ((S32)count > 0)
        {
            emit_popper_bubbles(data, (S32)count, size, 0.0f);
        }
    }

    void update_popper(popper_data& popper, F32 dt)
    {
        popper.time += dt;
        if (popper.time > popper.end_time)
        {
            dt = popper.time - popper.end_time;
            popper.time = popper.end_time;
        }

        F32 area = popper.area;
        F32 vel_add = 0.0f;
        F32 rate = popper.rate;
        F32 t = popper.time / popper.end_time;

        switch (popper.state)
        {
        case STATE_ON:
        {
            F32 p, a;
            xSCurve(p, vel_add, a, t);
            area = area * p;
            vel_add *= popper.radius;
            xVec3 scale;
            if (popper.model_scale.x == 0.0f)
            {
                scale = p;
            }
            else
            {
                scale = popper.model_scale * p;
            }
            xModelSetScale(popper.ent->model, scale);
            break;
        }
        case STATE_OFF:
        {
            set_popper_alpha(popper, xSCurve(1.0f - t));
            break;
        }
        }

        F32 size;
        F32 emit = rate;
        if (area <= 4.0f)
        {
            emit *= area;
            size = 1.0f;
        }
        else
        {
            emit *= 4.0f;
            size = 0.5f * xsqrt(area);
        }

        popper.emitted += emit * dt;
        if (popper.emitted >= 1.0f)
        {
            S32 n = popper.emitted;
            popper.emitted -= n;
            emit_popper_bubbles(popper, n, size, vel_add);
        }
    }

} // namespace

void xSCurve(F32& p, F32& v, F32& a, F32 t)
{
    if (t <= 0.5f)
    {
        a = 4.0f;
        v = t * a;
        p = (0.5f * t) * v;
    }
    else
    {
        F32 d = t - 1.0f;
        a = -4.0f;
        v = a * d;
        p = 0.5f * v * d + 1.0f;
    }
}

namespace
{
    void set_popper_alpha(popper_data& data, F32 alpha)
    {
        xEntShow(data.ent);
        xModelInstance* p = data.ent->model;
        p->Alpha = alpha;
        p->Flags |= (U16)0x4000;
        p->PipeFlags = (p->PipeFlags & ~0xc) | 8;
    }

    void destroy_popper(popper_data& data)
    {
        switch (data.state)
        {
        case STATE_ON:
        {
            xModelSetScale(data.ent->model, data.model_scale);
            break;
        }
        case STATE_OFF:
        {
            xEntHide(data.ent);
            xModelInstance* m = data.ent->model;
            m->Alpha = 1.0f;
            m->Flags &= 0xbfff;
            m->PipeFlags = (m->PipeFlags & ~0xc) | data.pipe_flags;
            break;
        }
        }

        data.state = STATE_NONE;
    }

    popper_data* grab_popper(xEnt& ent)
    {
        popper_data* data = find_popper(&ent);
        if (data != NULL)
        {
            if (data->faces == count_faces(ent.model))
            {
                return data;
            }
        }
        else
        {
            data = find_free_popper();
            if (data == NULL)
            {
                return NULL;
            }
            data->ent = &ent;
        }

        if (!setup_popper_emitter(*data))
        {
            data->ent = NULL;
            return NULL;
        }

        return data;
    }
} // namespace

void init_poppers()
{
    reset_poppers();
    add_popper_tweaks();
}

namespace
{
    void add_popper_tweaks()
    {
    }
} // namespace

void reset_poppers()
{
    popper_data* pData;
    popper_data* pEnd = &poppers[sizeof(poppers) / sizeof(popper_data)];
    for (pData = &poppers[0]; pData != pEnd; pData++)
    {
        pData->state = STATE_NONE;
        pData->ent = NULL;
    }
}

void update_poppers(F32 dt)
{
    popper_data* p = poppers;
    popper_data* end = &poppers[8];
    for (; p != end; p++)
    {
        if (p->state != STATE_NONE)
        {
            update_popper(*p, dt);
            if (p->time >= p->end_time)
            {
                destroy_popper(*p);
            }
        }
    }
}

void zFXPopOn(xEnt& ent, F32 rate, F32 time)
{
    if (!validate_popper(ent))
    {
        return;
    }

    popper_data* popper = grab_popper(ent);
    if (popper == NULL)
    {
        return;
    }

    if (rate == 0.0f)
    {
        rate = 200.0f;
    }
    if (time == 0.0f)
    {
        time = 0.25f;
    }
    else if (time < 0.0f)
    {
        time = 0.0f;
    }

    if (time <= 0.0f)
    {
        popper->state = STATE_ON;
        emit_popper_bubbles_immediate(*popper);
        popper->state = STATE_NONE;
        xEntShow(&ent);
    }
    else
    {
        xSphere o;
        xBoundGetSphere(o, ent.bound);
        popper->radius = o.r;
        popper->state = STATE_ON;
        popper->time = 0.0f;
        popper->end_time = time;
        popper->rate = rate;
        popper->vel = 0.0f;
        popper->rloc = 0.5f;
        popper->rvel = 1.0f;
        popper->emitted = 0.0f;
        popper->model_scale = ent.model->Scale;

        const xVec3 tiny = { 0.001f, 0.001f, 0.001f };
        xModelSetScale(popper->ent->model, tiny);
    }
}

namespace
{
    bool validate_popper(const xEnt& ent)
    {
        if (ent.model == NULL || ent.model->Data == NULL)
        {
            return FALSE;
        }

        if (model_is_preinstanced(ent.model->Data))
        {
            return FALSE;
        }

        xVec3 scale = ent.model->Scale;
        for (xModelInstance* m = ent.model->Next; m != NULL; m = m->Next)
        {
            if (!(xabs(scale.x - m->Scale.x) <= 0.1f) || !(xabs(scale.y - m->Scale.y) <= 0.1f) ||
                !(xabs(scale.z - m->Scale.z) <= 0.1f))
            {
                return FALSE;
            }
        }

        return TRUE;
    }
} // namespace

void zFXPopOff(xEnt& ent, F32 rate, F32 time)
{
    if (!validate_popper(ent))
    {
        return;
    }

    popper_data* popper = grab_popper(ent);
    if (popper == NULL)
    {
        return;
    }

    if (rate == 0.0f)
    {
        rate = 200.0f;
    }
    if (time == 0.0f)
    {
        time = 0.25f;
    }
    else if (time < 0.0f)
    {
        time = 0.0f;
    }

    if (time <= 0.0f)
    {
        popper->state = STATE_OFF;
        emit_popper_bubbles_immediate(*popper);
        popper->state = STATE_NONE;
        xEntHide(&ent);
    }
    else
    {
        xSphere o;
        xBoundGetSphere(o, ent.bound);
        popper->radius = o.r;
        popper->state = STATE_OFF;
        popper->time = 0.0f;
        popper->end_time = time;
        popper->rate = rate;
        popper->vel = 0.0f;
        popper->rloc = 0.0f;
        popper->rvel = 0.25f;
        popper->emitted = 0.0f;
        popper->pipe_flags = ent.model->PipeFlags & 0xc;

        set_popper_alpha(*popper, 0.0f);
    }
}

namespace
{
    void add_entrail_tweaks()
    {
    }
} // namespace

void reset_entrails()
{
    entrail_data* pData;
    entrail_data* pEnd = &entrails[entrails_size];
    for (pData = entrails; pData != pEnd; pData++)
    {
        pData->reset();
    }
}

namespace
{
    void entrail_data::reset()
    {
        flags = 0;
        emitted = 0.0f;
    }
} // namespace

void update_entrails(F32 dt)
{
    entrail_data* it;
    entrail_data* end = &entrails[entrails_size];
    for (it = entrails; it != end; it++)
    {
        it->update(dt);
    }
}

namespace
{
    void entrail_data::update(F32 dt)
    {
        if (ent == NULL)
        {
            return;
        }

        if (!xEntIsVisible(ent))
        {
            flags &= ~1;
            return;
        }

        entrail_type& t = entrail_types[type];

        const xVec3& campos = globals.camera.mat.pos;
        xMat4x3* frame = xEntGetFrame(ent);
        F32 dx = campos.x - frame->pos.x;
        F32 dy = campos.y - frame->pos.y;
        F32 dz = campos.z - frame->pos.z;
        const xVec3 dist = { dx, dy, dz };

        if (dist.length2() > t.cull_dist * t.cull_dist)
        {
            flags &= ~1;
            return;
        }

        if (t.bone > ent->model->BoneCount)
        {
            return;
        }

        if (!(flags & 1))
        {
            emitted = 0.0f;
        }

        emitted += t.rate * dt;

        S32 emit = emitted;
        if (emit <= 0)
        {
            return;
        }
        emitted -= emit;

        xMat4x3 mat;
        xModelGetBoneMat(mat, *ent->model, t.bone);

        xVec3 new_loc;
        xVec3 new_vel;
        xMat4x3Toworld(&new_loc, &mat, &t.offset);
        xMat3x3RMulVec(&new_vel, &mat, &t.vel);

        if (flags & 1)
        {
            zFX_SpawnBubbleTrail(&loc, &new_loc, &vel, &new_vel, emit, &t.offset_rand, &t.vel_rand,
                                 1.0f);
        }
        else
        {
            zFX_SpawnBubbleTrail(&new_loc, &new_loc, &new_vel, &new_vel, emit, &t.offset_rand,
                                 &t.vel_rand, 1.0f);
        }

        loc = new_loc;
        vel = new_vel;
        flags |= 1;
    }
} // namespace

static void setup_entrails(zScene& s)
{
    U32 hash[2];
    U32 hash_dff[2];
    U32 hash_minf[2];

    for (U32 i = 0; i < 2; i++)
    {
        hash[i] = xStrHash(entrail_types[i].model_name);
        hash_dff[i] = xStrHashCat(hash[i], ".dff");
        hash_minf[i] = xStrHashCat(hash[i], ".minf");
    }

    add_entrail_tweaks();

    entrails_size = 0;

    {
        zEnt** it = s.ents;
        zEnt** end = it + s.num_ents;
        for (; it != end; it++)
        {
            xEnt* ent = *it;
            if (!(ent->baseFlags & 0x20))
            {
                continue;
            }

            U32 model = ent->asset->modelInfoID;
            for (U32 i = 0; i < 2; i++)
            {
                if (model == hash[i] || model == hash_dff[i] || model == hash_minf[i])
                {
                    entrails_size++;
                    break;
                }
            }
        }
    }

    if (entrails_size == 0)
    {
        return;
    }

    entrails = (entrail_data*)xMemAlloc(gActiveHeap, entrails_size * sizeof(entrail_data), 0);

    U32 count = 0;
    zEnt** it = s.ents;
    zEnt** end = it + s.num_ents;
    for (; it != end; it++)
    {
        zEnt* ent = *it;
        if (!(ent->baseFlags & 0x20))
        {
            continue;
        }

        U32 model = ent->asset->modelInfoID;
        for (U32 i = 0; i < 2; i++)
        {
            if (model == hash[i] || model == hash_dff[i] || model == hash_minf[i])
            {
                entrails[count].reset();
                entrails[count].type = i;
                entrails[count].ent = ent;
                count++;
                break;
            }
        }
    }
}

void zFXGooUpdateInstance(zFXGooInstance*, F32);

xVec3& xVec3::up_normalize()
{
    return safe_normalize(xVec3::m_UnitAxisY);
}
