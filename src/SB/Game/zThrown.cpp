#include "zThrown.h"

#include "zGlobals.h"
#include <types.h>
#include <string.h>

#include "zEvent.h"
#include "zScene.h"

#include "xEntBoulder.h"
#include "xMathInlines.h"
#include "xstransvc.h"
#include "zEntButton.h"
#include "zFX.h"
#include "zGoo.h"
#include "zNPCTypeCommon.h"
#include "zSurface.h"

extern xEnt* gReticleTarget;

static void zThrown_AddTempFrame(zThrownStruct* thrown);
static void zFruit_ColorFade(zThrownStruct* thrown);
static void zThrown_Update(xEnt* ent, xScene* sc, F32 dt);
static S32 zThrownCollide_CauseDamage(zThrownStruct* thrown, xEntCollis* collis);
static void zThrownCollide_ThrowFruit(zThrownStruct* thrown, xEntCollis* collis, F32* bounce,
                                      F32* friction);
static void zThrownCollide_ThrowFreeze(zThrownStruct* thrown, xEntCollis* collis, float* bounce,
                                       float* friction);
static void zThrownCollide_DestructObj(zThrownStruct* thrown, xEntCollis* collis, F32* bounce,
                                       F32* friction);
static void zThrownCollide_BSandyHead(zThrownStruct* thrown, xEntCollis* collis, F32* bounce,
                                      F32* friction);
static void zThrownCollide_Tiki(zThrownStruct* thrown, xEntCollis* collis, F32* bounce,
                                F32* friction);
static void zThrownCollide_StoneTiki(zThrownStruct* thrown, xEntCollis* collis, F32* bounce,
                                     F32* friction);
static void checkAgainstButtons(xEnt* ent);
static xBase* zThrown_ButtonIteratorCB(xBase* b, zScene* scn, void* user);

void zEntPlayer_PatrickLaunch(xEnt* patLauncher);
void zFXGooFreeze(RpAtomic* atomic, const xVec3* center, xVec3* ref_parPosVec);

static zThrownStruct zThrownList[32];
static U32 zThrownCount;
static U32 sThrowButtonMask;
static F32 sSNDLandTimer;
static U32 sFruitIsFreezy;
static U32 sDebugDepth;

static LaunchStats l_normal = {10.0f, 15.0f, 0.2f};
static CarryableStats c_normal = {};
static CarryableStats c_fruit = {24.0f};

static ThrowableStats zThrowableModels[23] = {
    {"__default__", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"tiki_wooden", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"reallybigrock", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"b3_roof_chunk_sm_a", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"b3_roof_chunk_sm_b", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"b3_roof_chunk_sm_c", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"b3_roof_chunk_sm_d", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"b3_roof_chunk_sm_e", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"b3_roof_chunk_sm_f", zThrownCollide_DestructObj, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"tiki_wooden_bind", zThrownCollide_Tiki, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"tiki_thunder_bind", zThrownCollide_Tiki, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"tiki_stone_bind", zThrownCollide_StoneTiki, &c_normal, &l_normal,{}, NULL, 0.0f, 0x0, 0x0, NULL},
    {"fruit_freezy", zThrownCollide_ThrowFreeze, &c_fruit, &l_normal, {}, "fruit_freezy_shrapnel", 0.77f, 0x0, 0x0, NULL},
    {"fruit_freezy_bind", zThrownCollide_ThrowFreeze, &c_fruit, &l_normal, {}, "fruit_freezy_shrapnel", 0.77f, 0x0, 0x0, NULL},
    {"fruit_glow_blue", zThrownCollide_ThrowFruit, &c_fruit, &l_normal, {32, 32, 128, 255}, NULL, 0.92f, 0x0, 0x0, NULL},
    {"fruit_glow_green", zThrownCollide_ThrowFruit, &c_fruit, &l_normal, {32, 128, 32, 255}, NULL, 0.92f, 0x0, 0x0, NULL},
    {"fruit_glow_orange", zThrownCollide_ThrowFruit, &c_fruit, &l_normal, {128, 96, 32, 255}, NULL, 0.92f, 0x0, 0x0, NULL},
    {"fruit_glow_red", zThrownCollide_ThrowFruit, &c_fruit, &l_normal, {128, 32, 32, 255}, NULL, 0.92f, 0x0, 0x0, NULL},
    {"fruit_glow_yellow", zThrownCollide_ThrowFruit, &c_fruit, &l_normal, {128, 128, 32, 255}, NULL, 0.92f, 0x0, 0x0, NULL},
    {"fruit_throw", zThrownCollide_ThrowFruit, &c_fruit, &l_normal, {32, 32, 32, 255}, "fruit_throw_shrapnel", 0.92f, 0x0, 0x0, NULL},
    {"fruit_throw_bind", zThrownCollide_ThrowFruit, &c_fruit, &l_normal, {32, 32, 32, 255}, "fruit_throw_shrapnel", 0.92f, 0x0, 0x0, NULL},
    {"boss_sa_head_bind", zThrownCollide_BSandyHead, &c_normal, &l_normal, {}, NULL, 0.0f, 0x0, 0x0, NULL},
    {},
};

static void zFruit_Update(xEnt* ent, xScene* sc, F32 dt);

void zThrown_Setup(zScene* sc)
{
    ThrowableStats* stats;
    char tmpstr[256];

    F32 airTime;

    c_fruit.killTimer = globals.player.carry.fruitLifetime;
    airTime = xsqrt((2.0f * globals.player.carry.throwHeight) / globals.player.carry.throwGravity);
    l_normal.throwSpeedY = globals.player.carry.throwGravity * airTime;
    l_normal.throwSpeedXZ = globals.player.carry.throwDistance / (2.0f * airTime);

    for (stats = zThrowableModels; stats->name != NULL; stats++)
    {
        stats->nameHash = xStrHash(stats->name);
        strcpy(tmpstr, stats->name);
        strcat(tmpstr, ".MINF");
        stats->nameHashMINF = xStrHash(tmpstr);
        if (stats->shrapName != NULL)
        {
            stats->shrapAsset = (zShrapnelAsset*)xSTFindAsset(xStrHash(stats->shrapName), NULL);
        }
        else
        {
            stats->shrapAsset = NULL;
        }
    }

    for (U32 i = 0; i < sc->num_ents; i++)
    {
        if (((xEnt*)sc->base[i])->baseType == eBaseTypeStatic ||
            ((xEnt*)sc->base[i])->baseType == eBaseTypeDestructObj)
        {
            for (stats = zThrowableModels; stats->name != NULL; stats++)
            {
                if (stats->nameHash == ((xEnt*)sc->base[i])->asset->modelInfoID ||
                    stats->nameHashMINF == ((xEnt*)sc->base[i])->asset->modelInfoID)
                {
                    ((xEnt*)sc->base[i])->moreFlags |= XENT_MORE_FLAGS_0x8;
                    ((xEnt*)sc->base[i])->asset->moreFlags |= XENT_MORE_FLAGS_0x8;
                    break;
                }
            }
        }
    }
}

static void zThrown_AddTempFrame(zThrownStruct* thrown)
{
    xEnt* ent = thrown->ent;
    if (ent->frame == NULL)
    {
        memset(&thrown->frame, 0, sizeof(xEntFrame));
        ent->frame = &thrown->frame;
        ent->frame->mat = *(xMat4x3*)ent->model->Mat;
    }
}

static void zFruit_ColorFade(zThrownStruct* thrown)
{
    static F32 fruitPattern[9][2] = { { 6.0f, 1.0f }, { 3.2f, 0.3f },  { 3.0f, 1.0f },
                                      { 2.2f, 0.3f }, { 2.0f, 1.0f },  { 1.2f, 0.3f },
                                      { 1.0f, 1.0f }, { 0.0f, 0.15f }, { -1.0f, 0.0f } };

    xModelInstance* mod = thrown->ent->model;

    if (!thrown->killTimer || thrown->killTimer > fruitPattern[0][0])
    {
        mod->RedMultiplier = mod->GreenMultiplier = mod->BlueMultiplier = fruitPattern[0][1];
    }
    else
    {
        F32 t = thrown->killTimer;
        F32* p = &fruitPattern[0][0];
        while (t < p[2])
        {
            p += 2;
        }
        F32 f = (t - p[0]) / (p[2] - p[0]);
        f = f * p[3] + (1.0f - f) * p[1];
        mod->RedMultiplier = mod->GreenMultiplier = mod->BlueMultiplier = f;
    }
}

static void zFruit_Update(xEnt* ent, xScene* sc, F32 dt)
{
    U32 i;

    if (ent->model->Scale.x)
    {
        if (1e-5f == ent->model->Scale.x)
        {
            if (xVec3Dist((xVec3*)&ent->model->Mat->pos,
                          (xVec3*)&globals.player.ent.model->Mat->pos) < 1.4f)
            {
                xEntBeginUpdate(ent, sc, dt);
                xEntEndUpdate(ent, sc, dt);
                return;
            }
            ent->chkby |= XENT_COLLTYPE_PLYR;
        }

        ent->model->Scale.x += dt / 0.5f;
        ent->model->Scale.y = ent->model->Scale.x;
        ent->model->Scale.z = ent->model->Scale.x;

        if (ent->model->Scale.x >= 1.0f)
        {
            ent->model->Scale.x = 0.0f;
            zThrown_Remove(ent);
        }

        xEntBeginUpdate(ent, sc, dt);
        xEntEndUpdate(ent, sc, dt);
    }
    else
    {
        for (i = 0; i < zThrownCount; i++)
        {
            if (ent == zThrownList[i].ent)
            {
                break;
            }
        }

        zThrownStruct* thrown = &zThrownList[i];

        xEntBeginUpdate(ent, sc, dt);
        xEntEndUpdate(ent, sc, dt);

        if (thrown->killTimer)
        {
            thrown->killTimer -= dt;
            if (thrown->killTimer <= 0.0f)
            {
                thrown->killTimer = 0.0f;
                zFruit_ColorFade(thrown);

                if (globals.player.carry.grabbed == ent)
                {
                    globals.player.carry.grabbed = NULL;
                    gReticleTarget = NULL;
                }
                if (gReticleTarget == ent)
                {
                    gReticleTarget = NULL;
                }

                zShrapnelAsset* shrap = thrown->stats->shrapAsset;
                if (shrap != NULL && shrap->initCB != NULL)
                {
                    shrap->initCB(shrap, ent->model, NULL, NULL);
                }

                xEntReset(ent);
                zEntEvent(ent, eEventCollision_Visible_On);
                ent->model->Scale.x = 1e-5f;
                ent->model->Scale.y = 1e-5f;
                ent->model->Scale.z = 1e-5f;
                ent->chkby &= 0xef;
                ent->baseFlags &= 0xffef;

                for (i = 0; i < zThrownCount; i++)
                {
                    if (zThrownList[i].stackEnt == ent)
                    {
                        zThrownList[i].stackEnt = NULL;
                        zThrownList[i].ent->update = zThrown_Update;
                    }
                    if (zThrownList[i].stackTgt == ent)
                    {
                        zThrownList[i].stackTgt = NULL;
                    }
                }
            }
            else
            {
                zFruit_ColorFade(thrown);
            }
        }
    }
}

static void Recurse_TranslateStack(xEnt* ent, xVec3* delta)
{
    sDebugDepth++;
    for (U32 i = 0; i < zThrownCount; i++)
    {
        if (zThrownList[i].stackEnt == ent)
        {
            Recurse_TranslateStack(zThrownList[i].ent, delta);
            zThrownList[i].ent->model->Mat->pos.x += delta->x;
            zThrownList[i].ent->model->Mat->pos.y += delta->y;
            zThrownList[i].ent->model->Mat->pos.z += delta->z;
        }
    }
    sDebugDepth--;
}

static void zThrown_Update(xEnt* ent, xScene* sc, F32 dt)
{
    xEntCollis collis;
    xSweptSphere sws;
    xBound bound;
    U32 i;
    U32 killIt;

    killIt = 0;

    for (i = 0; i < zThrownCount; i++)
    {
        if (ent == zThrownList[i].ent)
        {
            break;
        }
    }

    zThrownStruct* thrown = &zThrownList[i];

    if (ent->baseType == eBaseTypeNPC)
    {
        xVec3 pos = *(xVec3*)&ent->model->Mat->pos;
        thrown->oldupdate(ent, sc, dt);
        *(xVec3*)&ent->model->Mat->pos = pos;
        if (ent->frame != NULL)
        {
            ent->frame->mat.pos = pos;
        }
    }

    xVec3 delta;
    xVec3 dir;
    xVec3 start;
    xVec3 center;
    F32 bounce;
    F32 friction;

    if (thrown->collResetTimer)
    {
        thrown->collResetTimer -= dt;
        if (thrown->collResetTimer < 0.0f)
        {
            thrown->collResetTimer = 0.0f;
            ent->chkby |= XENT_COLLTYPE_PLYR;
        }
    }

    xEntCollis* oldcollis = ent->collis;
    U8 oldpflags = ent->pflags;
    U8 oldcollType = ent->collType;
    delta.x = -ent->model->Mat->pos.x;
    delta.y = -ent->model->Mat->pos.y;
    delta.z = -ent->model->Mat->pos.z;
    xEntBoundUpdateCallback oldbupdate = ent->bupdate;

    ent->bupdate = NULL;
    ent->collis = &collis;
    collis.chk = 0x2e;
    collis.pen = 0x2e;
    collis.post = NULL;
    collis.depenq = NULL;
    ent->frame->vel = thrown->vel;
    ent->pflags |= XENT_PFLAGS_HAS_GRAVITY;
    ent->collType = XENT_COLLTYPE_PLYR;

    bound = ent->bound;

    switch (bound.type)
    {
    case XBOUND_TYPE_BOX:
    case XBOUND_TYPE_OBB:
    {
        ent->bound.sph.r = (0.5f * (bound.box.box.upper.x - bound.box.box.lower.x) <
                            0.5f * (bound.box.box.upper.z - bound.box.box.lower.z))
                               ? 0.5f * (bound.box.box.upper.x - bound.box.box.lower.x)
                               : 0.5f * (bound.box.box.upper.z - bound.box.box.lower.z);
        ent->bound.sph.center.x = 0.5f * (bound.box.box.upper.x + bound.box.box.lower.x);
        ent->bound.sph.center.y = bound.box.box.lower.y + ent->bound.sph.r;
        ent->bound.sph.center.z = 0.5f * (bound.box.box.upper.z + bound.box.box.lower.z);
        if (bound.type == XBOUND_TYPE_OBB)
        {
            xMat4x3Toworld(&ent->bound.sph.center, bound.mat, &ent->bound.sph.center);
        }
        break;
    }
    }

    ent->bound.type = XBOUND_TYPE_SPHERE;
    xDrawSphere(&ent->bound.sph, 0xc0006);

    if (thrown->stackTgt != NULL)
    {
        ent->bound.sph.center.y -= 0.25f * ent->bound.sph.r;
        ent->bound.sph.r *= 0.75f;
    }

    if (ent->baseType == eBaseTypeNPC)
    {
        xEntBeginUpdate(ent, sc, 0.0f);
    }
    else
    {
        xEntBeginUpdate(ent, sc, dt);
    }

    F32 oldGravity = globals.sceneCur->gravity;
    globals.sceneCur->gravity = -globals.player.carry.throwGravity;
    xEntApplyPhysics(ent, sc, dt);
    globals.sceneCur->gravity = oldGravity;

    if (thrown->stackTgt != NULL)
    {
        xEntFrame* frame = ent->frame;
        xMat4x3* tmat = (xMat4x3*)thrown->stackTgt->model->Mat;
        if (frame->vel.x * (tmat->pos.x - frame->mat.pos.x) +
                frame->vel.z * (tmat->pos.z - frame->mat.pos.z) <=
            0.0f)
        {
            dir.x = frame->vel.x;
            dir.y = 0.0f;
            dir.z = frame->vel.z;
            xVec3Normalize(&dir, &dir);

            frame = ent->frame;
            tmat = (xMat4x3*)thrown->stackTgt->model->Mat;

            F32 d =
                dir.x * (tmat->pos.x - frame->mat.pos.x) + dir.z * (tmat->pos.z - frame->mat.pos.z);
            F32 nx = d * dir.x + frame->mat.pos.x;
            F32 nz = d * dir.z + frame->mat.pos.z;
            if ((nx - tmat->pos.x) * (nx - tmat->pos.x) +
                    (nz - tmat->pos.z) * (nz - tmat->pos.z) <
                0.02f)
            {
                frame->mat.pos.x = nx;
                ent->frame->mat.pos.z = nz;
                ent->frame->vel.x = 0.0f;
                ent->frame->vel.z = 0.0f;
            }
            else if (frame->vel.x || frame->vel.z)
            {
                thrown->stackTgt = NULL;
            }
        }
    }

    if (thrown->drv.driver != NULL)
    {
        xEntDriveUpdate(&thrown->drv, sc, dt, NULL);
    }

    thrown->oldcollpos = ent->frame->mat.pos;

    if (ent->baseType == eBaseTypeBoulder)
    {
        xVec3Init(&((xEntBoulder*)ent)->vel, 0.0f, 0.0f, 0.0f);
        xEntBoulder_RealBUpdate(ent, &ent->frame->mat.pos);
    }
    else if (ent->bupdate != NULL)
    {
        ent->bupdate(ent, &ent->frame->mat.pos);
    }
    else
    {
        xEntDefaultBoundUpdate(ent, &ent->frame->mat.pos);
    }

    for (i = 0; i < zThrownCount; i++)
    {
        if (zThrownList[i].stackEnt == ent)
        {
            zThrownList[i].ent->chkby = 0;
        }
        if (zThrownList[i].stackTgt == ent)
        {
            zThrownList[i].ent->chkby = 0;
        }
    }

    start.x = ent->bound.sph.center.x + ent->model->Mat->pos.x - ent->frame->mat.pos.x;
    start.y = ent->bound.sph.center.y + ent->model->Mat->pos.y - ent->frame->mat.pos.y;
    start.z = ent->bound.sph.center.z + ent->model->Mat->pos.z - ent->frame->mat.pos.z;
    xSweptSpherePrepare(&sws, &start, &ent->bound.sph.center, ent->bound.sph.r);

    if (sws.dist > 0.05f && xSweptSphereToScene(&sws, globals.sceneCur, ent, XENT_COLLTYPE_PLYR))
    {
        F32 d = sws.dist - sws.curdist;
        F32 lim = 0.25f * ent->bound.sph.r;
        if (d < lim)
        {
            lim = d;
        }
        F32 t = (sws.curdist + lim) / sws.dist;
        ent->frame->mat.pos.x =
            t * (ent->frame->mat.pos.x - ent->model->Mat->pos.x) + ent->model->Mat->pos.x;
        ent->frame->mat.pos.y =
            t * (ent->frame->mat.pos.y - ent->model->Mat->pos.y) + ent->model->Mat->pos.y;
        ent->frame->mat.pos.z =
            t * (ent->frame->mat.pos.z - ent->model->Mat->pos.z) + ent->model->Mat->pos.z;

        if (ent->baseType == eBaseTypeBoulder)
        {
            xVec3Init(&((xEntBoulder*)ent)->vel, 0.0f, 0.0f, 0.0f);
            xEntBoulder_RealBUpdate(ent, &ent->frame->mat.pos);
        }
        else if (ent->bupdate != NULL)
        {
            ent->bupdate(ent, &ent->frame->mat.pos);
        }
        else
        {
            xEntDefaultBoundUpdate(ent, &ent->frame->mat.pos);
        }
    }

    xEntCollide(ent, sc, dt);

    for (i = 0; i < zThrownCount; i++)
    {
        if (zThrownList[i].stackEnt == ent)
        {
            zThrownList[i].ent->chkby = 0x18;
        }
        if (zThrownList[i].stackTgt == ent)
        {
            zThrownList[i].ent->chkby = 0x18;
        }
    }

    if (ent->collis->colls[0].flags & k_HIT_IT)
    {
        thrown->stackTgt = NULL;
    }

    xEntEndUpdate(ent, sc, dt);

    if (thrown->stats->collCB != NULL &&
        (collis.stat_eidx > collis.stat_sidx || collis.dyn_eidx > collis.dyn_sidx ||
         collis.npc_eidx > collis.npc_sidx || collis.env_eidx > collis.env_sidx))
    {
        if (0.0f == sSNDLandTimer)
        {
            xSndPlay3D(xStrHash("Tfruit_land4"), 1.0f, 0.0f, 0, 0, ent, 5.0f, 30.0f, SND_CAT_GAME,
                       0.0f);
        }
        sSNDLandTimer = 0.2f;

        if (ent->baseType == eBaseTypeNPC)
        {
            xCollis* coll = ent->collis->colls;
            xCollis* collEnd = coll + ent->collis->idx;
            for (; coll < collEnd; coll++)
            {
                if (coll->flags & k_HIT_IT)
                {
                    xSurface* surf = zSurfaceGetSurface(coll);
                    if (surf != NULL && surf->state == 0 && surf->moprops != NULL)
                    {
                        zSurfaceProps* prop = ((zSurfaceProps**)surf->moprops)[0];
                        if (prop != NULL && *(U8*)&prop->texanim[0].mode != 0)
                        {
                            ((zNPCCommon*)ent)->Damage(DMGTYP_DAMAGE_SURFACE, NULL, NULL);
                        }
                    }
                }
            }
        }

        thrown->stats->collCB(thrown, &collis, &bounce, &friction);

        if (0.0f == bounce && 0.0f == friction)
        {
            killIt = 1;
            if (ent->baseType != eBaseTypeBoulder)
            {
                ent->frame->vel.x = 0.0f;
                ent->frame->vel.y = 0.0f;
                ent->frame->vel.z = 0.0f;
            }
            else
            {
                xVec3Init(&((xEntBoulder*)ent)->vel, 0.0f, 0.0f, 0.0f);
            }
        }
        else
        {
            for (i = 0; i < 18; i++)
            {
                if (collis.colls[i].flags & k_HIT_IT)
                {
                    F32 d = -collis.colls[i].hdng.x * thrown->vel.x +
                            -collis.colls[i].hdng.y * thrown->vel.y +
                            -collis.colls[i].hdng.z * thrown->vel.z;
                    F32 px = -collis.colls[i].hdng.x * d;
                    F32 py = -collis.colls[i].hdng.y * d;
                    F32 pz = -collis.colls[i].hdng.z * d;
                    F32 tx = thrown->vel.x - px;
                    F32 ty = thrown->vel.y - py;
                    F32 tz = thrown->vel.z - pz;
                    ent->frame->vel.x =
                        thrown->vel.x - (1.0f + bounce) * px - (1.0f - friction) * tx;
                    ent->frame->vel.y =
                        thrown->vel.y - (1.0f + bounce) * py - (1.0f - friction) * ty;
                    ent->frame->vel.z =
                        thrown->vel.z - (1.0f + bounce) * pz - (1.0f - friction) * tz;
                    break;
                }
            }
        }
    }

    thrown->vel = ent->frame->vel;
    ent->collis = oldcollis;
    ent->pflags = oldpflags;
    ent->collType = oldcollType;
    ent->bupdate = oldbupdate;

    switch (bound.type)
    {
    case XBOUND_TYPE_BOX:
    {
        ent->bound.type = XBOUND_TYPE_BOX;
        F32 hx = 0.5f * (bound.box.box.upper.x - bound.box.box.lower.x);
        F32 hy = (bound.box.box.upper.y - bound.box.box.lower.y) - ent->bound.sph.r;
        F32 hz = 0.5f * (bound.box.box.upper.z - bound.box.box.lower.z);
        F32 r = ent->bound.sph.r;
        center = ent->bound.sph.center;
        ent->bound.box.box.lower.x = center.x - hx;
        ent->bound.box.box.lower.y = center.y - r;
        ent->bound.box.box.lower.z = center.z - hz;
        ent->bound.box.box.upper.x = center.x + hx;
        ent->bound.box.box.upper.y = center.y + hy;
        ent->bound.box.box.upper.z = center.z + hz;
        ent->bound.box.center.x = 0.5f * (ent->bound.box.box.lower.x + ent->bound.box.box.upper.x);
        ent->bound.box.center.y = 0.5f * (ent->bound.box.box.lower.y + ent->bound.box.box.upper.y);
        ent->bound.box.center.z = 0.5f * (ent->bound.box.box.lower.z + ent->bound.box.box.upper.z);
        break;
    }
    case XBOUND_TYPE_OBB:
        ent->bound.type = bound.type;
        ent->bound.box = bound.box;
        ent->bound.mat = bound.mat;
        break;
    case XBOUND_TYPE_SPHERE:
        if (thrown->stackTgt != NULL)
        {
            ent->bound.sph.r = bound.sph.r;
            ent->bound.sph.center.y += 0.25f * bound.sph.r;
        }
        break;
    }

    delta.x += ent->model->Mat->pos.x;
    delta.y += ent->model->Mat->pos.y;
    delta.z += ent->model->Mat->pos.z;
    Recurse_TranslateStack(ent, &delta);

    if (thrown->killTimer)
    {
        thrown->killTimer -= dt;
        if (thrown->killTimer < 0.0f)
        {
            thrown->killTimer = 0.0f;
            zFruit_ColorFade(thrown);
            ent->update = zFruit_Update;

            if (globals.player.carry.grabbed == ent)
            {
                globals.player.carry.grabbed = NULL;
                gReticleTarget = NULL;
            }
            if (gReticleTarget == ent)
            {
                gReticleTarget = NULL;
            }

            zShrapnelAsset* shrap = thrown->stats->shrapAsset;
            if (shrap != NULL && shrap->initCB != NULL)
            {
                shrap->initCB(shrap, ent->model, NULL, NULL);
            }

            xEntReset(ent);
            zEntEvent(ent, eEventCollision_Visible_On);
            ent->model->Scale.x = 1e-5f;
            ent->model->Scale.y = 1e-5f;
            ent->model->Scale.z = 1e-5f;
            ent->chkby &= 0xef;
            ent->baseFlags &= 0xffef;

            for (i = 0; i < zThrownCount; i++)
            {
                if (zThrownList[i].stackEnt == ent)
                {
                    zThrownList[i].stackEnt = NULL;
                    zThrownList[i].ent->update = zThrown_Update;
                }
                if (zThrownList[i].stackTgt == ent)
                {
                    zThrownList[i].stackTgt = NULL;
                }
            }
            return;
        }
        zFruit_ColorFade(thrown);
    }

    checkAgainstButtons(ent);
    zFX_SpawnBubbleTrail((xVec3*)&ent->model->Mat->pos, 1);

    if (ent->bupdate != NULL)
    {
        ent->bupdate(ent, (xVec3*)&ent->model->Mat->pos);
    }
    else
    {
        xEntDefaultBoundUpdate(ent, (xVec3*)&ent->model->Mat->pos);
    }

    if (thrown->drv.driver == NULL)
    {
        if ((collis.colls[0].flags & k_HIT_IT) && collis.colls[0].optr != NULL &&
            collis.colls[0].optr == thrown->driveLastFloor &&
            ((xEnt*)collis.colls[0].optr)->frame != NULL && thrown->vel.y <= 0.0f)
        {
            thrown->driveDebounce++;
            if (thrown->driveDebounce > 2)
            {
                xEntDriveMount(&thrown->drv, (xEnt*)collis.colls[0].optr, 0.1f, NULL);
            }
        }
        else
        {
            thrown->driveDebounce = 0;
        }
    }
    else if (!(collis.colls[0].flags & k_HIT_IT) || collis.colls[0].optr != thrown->drv.driver)
    {
        thrown->driveDebounce++;
        if (thrown->driveDebounce > 4)
        {
            xEntDriveDismount(&thrown->drv, 0.3f);
        }
    }
    else
    {
        thrown->driveDebounce = 0;
    }

    thrown->driveLastFloor = (xEnt*)collis.colls[0].optr;

    if (sSNDLandTimer)
    {
        sSNDLandTimer -= dt;
        if (sSNDLandTimer < 0.0f)
        {
            sSNDLandTimer = 0.0f;
        }
    }

    if (killIt)
    {
        if (thrown->stats->carry == &c_fruit)
        {
            if (thrown->stackEnt != NULL)
            {
                ent->update = zFruit_Update;
            }
        }
        else
        {
            zThrown_Remove(ent);
        }
    }
}

void zThrown_Reset()
{
    for (U32 i = 0; i < zThrownCount; i++)
    {
        zThrownList[i].ent->update = zThrownList[i].oldupdate;
        zThrownList[i].ent->baseFlags &= 0xff7f;
        if (zThrownList[i].ent->frame == &zThrownList[i].frame)
        {
            zThrownList[i].ent->frame = NULL;
        }
        xModelInstance* mod = zThrownList[i].ent->model;
        mod->RedMultiplier = mod->GreenMultiplier = mod->BlueMultiplier = 1.0f;
        zThrownList[i].ent->chkby |= XENT_COLLTYPE_PLYR;
        zThrownList[i].ent->baseFlags |= (U16)zThrownList[i].oldRecShadow;
    }
    zThrownCount = 0;
}

void zThrown_LaunchVel(xEnt* ent, xVec3* vel)
{
    ThrowableStats* stats;
    zThrownStruct* newThrown;
    U32 i;

    stats = zThrowableModels + 1;
    while (stats->name != NULL)
    {
        if (stats->nameHash == ent->asset->modelInfoID ||
            stats->nameHashMINF == ent->asset->modelInfoID)
        {
            break;
        }
        stats++;
    }
    if (stats->name == NULL)
    {
        stats = zThrowableModels;
    }

    newThrown = NULL;
    if (ent->update == zFruit_Update)
    {
        for (i = 0; i < zThrownCount; i++)
        {
            if (zThrownList[i].ent == ent)
            {
                newThrown = &zThrownList[i];
                break;
            }
        }
    }
    else
    {
        zThrown_Remove(ent);
        newThrown = &zThrownList[zThrownCount];
        zThrownCount++;
        newThrown->killTimer = stats->carry->killTimer;
        newThrown->stats = stats;
        newThrown->oldupdate = ent->update;
        newThrown->ent = ent;
        newThrown->stackEnt = NULL;
        newThrown->stackTgt = NULL;
        newThrown->patLauncher = NULL;
        newThrown->oldRecShadow = ent->baseFlags & 0x10;
        xEntDriveInit(&newThrown->drv, ent);
        newThrown->driveDebounce = 0;
        newThrown->driveLastFloor = NULL;
    }

    newThrown->vel = *vel;
    newThrown->collResetTimer = stats->launch->collResetTimer;
    ent->update = zThrown_Update;
    ent->baseFlags |= (U16)0x80;
    zThrown_AddTempFrame(newThrown);
}

void zThrown_LaunchDir(xEnt* ent, xVec3* dir)
{
    ThrowableStats* stats;
    xVec3 vel;

    globals.player.carry.flyingToTarget = NULL;

    stats = zThrowableModels + 1;
    while (stats->name != NULL)
    {
        if (stats->nameHash == ent->asset->modelInfoID ||
            stats->nameHashMINF == ent->asset->modelInfoID)
        {
            break;
        }
        stats++;
    }
    if (stats->name == NULL)
    {
        stats = zThrowableModels;
    }

    F32 speedXZ = stats->launch->throwSpeedXZ;
    vel.x = speedXZ * dir->x;
    vel.y = stats->launch->throwSpeedY;
    vel.z = speedXZ * dir->z;

    zThrown_LaunchVel(ent, &vel);
}

S32 zThrown_LaunchPos(xEnt* ent, xVec3* pos, xVec3* dir)
{
    S32 result;
    ThrowableStats* stats;
    xVec3 vel;

    stats = zThrowableModels + 1;
    while (stats->name != NULL)
    {
        if (stats->nameHash == ent->asset->modelInfoID ||
            stats->nameHashMINF == ent->asset->modelInfoID)
        {
            break;
        }
        stats++;
    }
    if (stats->name == NULL)
    {
        stats = zThrowableModels;
    }

    F32 speedY;
    F32 gravity = globals.player.carry.throwGravity;
    speedY = stats->launch->throwSpeedY;
    F32 apexTime = speedY / gravity;
    F32 apexY;

    if (ent->baseType == eBaseTypeBoulder)
    {
        apexY = -(apexTime * ((0.5f * gravity) * apexTime) -
                  (speedY * apexTime + (ent->bound.sph.center.y - ent->bound.sph.r)));
    }
    else
    {
        apexY = -(apexTime * ((0.5f * gravity) * apexTime) -
                  (speedY * apexTime + ent->model->Mat->pos.y));
    }

    if (apexY < 0.5f + pos->y)
    {
        result = 0;
        vel.x = stats->launch->throwSpeedXZ * dir->x;
        vel.y = stats->launch->throwSpeedY;
        vel.z = stats->launch->throwSpeedXZ * dir->z;
    }
    else
    {
        F32 fallTime = xsqrt((2.0f * (apexY - pos->y)) / gravity);
        F32 dx = ent->model->Mat->pos.x - pos->x;
        F32 dz = ent->model->Mat->pos.z - pos->z;
        F32 d2 = dx * dx;
        F32 d2z = dz * dz;
        d2 = d2 + d2z;
        F32 speedXZ = xsqrt(d2) / (fallTime + apexTime);
        result = 1;
        vel.x = speedXZ * dir->x;
        vel.y = stats->launch->throwSpeedY;
        vel.z = speedXZ * dir->z;
    }

    zThrown_LaunchVel(ent, &vel);
    return result;
}

void zThrown_LaunchStack(xEnt* ent, xEnt* stackTgt)
{
    xVec3 pos;
    xVec3 dir;
    xBox box;

    switch (stackTgt->bound.type)
    {
    case XBOUND_TYPE_SPHERE:
        pos.y = stackTgt->bound.sph.center.y + stackTgt->bound.sph.r;
        break;
    case XBOUND_TYPE_BOX:
        pos.y = stackTgt->bound.box.box.upper.y;
        break;
    case XBOUND_TYPE_OBB:
        xBoxInitBoundOBB(&box, &stackTgt->bound.box.box, stackTgt->bound.mat);
        pos.y = box.upper.y;
        break;
    default:
        pos.y = stackTgt->model->Mat->pos.y;
        break;
    }

    pos.y += 0.2f;
    pos.x = stackTgt->model->Mat->pos.x;
    pos.z = stackTgt->model->Mat->pos.z;
    dir.x = pos.x - ent->model->Mat->pos.x;
    dir.y = 0.0f;
    dir.z = pos.z - ent->model->Mat->pos.z;

    xVec3Normalize(&dir, &dir);

    if (zThrown_LaunchPos(ent, &pos, &dir))
    {
        U32 i;
        for (i = 0; i < zThrownCount; i++)
        {
            if (ent == zThrownList[i].ent)
            {
                break;
            }
        }
        zThrownStruct* listInd = &zThrownList[i];
        listInd->stackTgt = stackTgt;
    }
}

void zThrown_PatrickLauncher(xEnt* ent, xEnt* launcher)
{
    int i = 0;

    for (; i < zThrownCount; i++)
    {
        if (ent == zThrownList[i].ent)
            break;
    }

    if (i != zThrownCount)
    {
        zThrownStruct* listInd = &zThrownList[i];
        listInd->patLauncher = launcher;
    }
}

void zThrown_AddFruit(xEnt* ent)
{
    U32 i;
    ThrowableStats* stats;
    zThrownStruct* newThrown;

    for (i = 0; i < zThrownCount; i++)
    {
        if (zThrownList[i].ent == ent)
        {
            zThrownList[i].stackEnt = NULL;
            zThrownList[i].stackTgt = NULL;
            ent->update = zFruit_Update;
            ent->baseFlags |= (U16)0x80;
            if (zThrownList[i].drv.driver != NULL)
            {
                xEntDriveDismount(&zThrownList[i].drv, 1e-5f);
            }
            zThrownList[i].driveDebounce = 0;
            zThrownList[i].driveLastFloor = NULL;
            return;
        }
    }

    stats = zThrowableModels + 1;
    while (stats->name != NULL)
    {
        if (stats->nameHash == ent->asset->modelInfoID ||
            stats->nameHashMINF == ent->asset->modelInfoID)
        {
            break;
        }
        stats++;
    }

    if (stats->name == NULL || stats->carry != &c_fruit)
    {
        return;
    }

    newThrown = &zThrownList[zThrownCount];
    zThrownCount++;
    newThrown->killTimer = stats->carry->killTimer;
    newThrown->stats = stats;
    newThrown->oldupdate = ent->update;
    newThrown->ent = ent;
    newThrown->stackEnt = NULL;
    newThrown->stackTgt = NULL;
    newThrown->oldRecShadow = ent->baseFlags & 0x10;
    ent->update = zFruit_Update;
    ent->baseFlags |= (U16)0x80;
    zThrown_AddTempFrame(newThrown);
    xEntDriveInit(&newThrown->drv, ent);
    newThrown->driveDebounce = 0;
    newThrown->driveLastFloor = NULL;
}

void zThrown_Remove(xEnt* ent)
{
    U32 i;

    for (i = 0; i < zThrownCount; i++)
    {
        if (zThrownList[i].ent == ent)
        {
            zThrownList[i].ent->update = zThrownList[i].oldupdate;
            ent->baseFlags &= 0xff7f;
            if (zThrownList[i].ent->frame == &zThrownList[i].frame)
            {
                zThrownList[i].ent->frame = NULL;
            }
            xModelInstance* mod = zThrownList[i].ent->model;
            mod->RedMultiplier = mod->GreenMultiplier = mod->BlueMultiplier = 1.0f;
            ent->baseFlags |= (U16)zThrownList[i].oldRecShadow;

            void (*collCB)(zThrownStruct*, xEntCollis*, F32*, F32*) = zThrownList[i].stats->collCB;

            zThrownList[i] = zThrownList[zThrownCount - 1];
            zThrownCount--;

            if (ent->baseType == eBaseTypeNPC)
            {
                ((zNPCCommon*)ent)->SetCarryState(zNPCCARRY_NONE);
            }
            if (ent->baseType == eBaseTypeBoulder)
            {
                ((xEntBoulder*)ent)->vel = zThrownList[i].vel;
            }
            if (!xEntIsVisible(ent))
            {
                return;
            }
            if (collCB != zThrownCollide_StoneTiki && collCB != zThrownCollide_ThrowFreeze &&
                collCB != zThrownCollide_BSandyHead && collCB != zThrownCollide_ThrowFruit)
            {
                return;
            }
            ent->chkby |= XENT_COLLTYPE_PLYR;
            return;
        }
    }
}

S32 zThrown_KillFruit(xEnt* ent)
{
    for (S32 i = 0; i < zThrownCount; i++)
    {
        if (zThrownList[i].ent == ent)
        {
            if (zThrownList[i].stats->carry == &c_fruit)
            {
                zThrownList[i].killTimer = 1e-6f;
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

static S32 zThrownCollide_CauseDamage(zThrownStruct* thrown, xEntCollis* collis)
{
    S32 result = 0;

    for (U32 i = k_XCOLLS_IDX_COUNT; i < 18 && (collis->colls[i].flags & k_HIT_IT); i++)
    {
        xEnt* other = (xEnt*)collis->colls[i].optr;
        if (other != NULL)
        {
            xEnt* pat = thrown->patLauncher;
            if (pat != NULL && pat == other)
            {
                zEntPlayer_PatrickLaunch(pat);
                thrown->patLauncher = NULL;
                return 1;
            }

            if (other->baseType == eBaseTypeDestructObj)
            {
                zEntEvent(other, eEventHit_Throw);
                result = 1;
            }
            else if (other->baseType == eBaseTypeNPC)
            {
                ((zNPCCommon*)other)->Damage(DMGTYP_HITBYTOSS, NULL, NULL);
                result = 1;
            }
            else if (other->baseType == eBaseTypeButton)
            {
                zEntEvent(other, eEventHit_Throw);
                zEntButton_Press((_zEntButton*)other, sThrowButtonMask);
                zEntButton_Hold((_zEntButton*)other, 0x12000);
            }
            else if (other->moreFlags & XENT_MORE_FLAGS_HITTABLE)
            {
                zEntEvent(other, eEventHit_Throw);
                zEntEvent(other, eEventHit);
            }
            else if (other->moreFlags & XENT_MORE_FLAGS_HITTABLE)
            {
                zEntEvent(other, eEventHit_Throw);
                zEntEvent(other, eEventHit);
            }

            F32 depth;
            if (sFruitIsFreezy && other->model != NULL && zGooIs(other, depth, 0))
            {
                xVec3 pos = *(xVec3*)&thrown->ent->model->Mat->pos;
                pos.x = collis->colls[i].tohit.x * collis->colls[i].dist + pos.x;
                pos.y = collis->colls[i].tohit.y * collis->colls[i].dist + pos.y;
                pos.z = collis->colls[i].tohit.z * collis->colls[i].dist + pos.z;
                zFXGooFreeze(other->model->Data, &pos, (xVec3*)&other->model->Mat->pos);
                zEntEvent(other, eEventPlatPause, 0.25f, 0.0f, 0.0f, 0.0f);
            }

            if (zGooIs(other, depth, 0))
            {
                result = 1;
            }
        }
    }

    return result;
}

static void zThrownCollide_ThrowFruit(zThrownStruct* thrown, xEntCollis* collis, F32* bounce,
                               F32* friction)
{
    U32 idx;

    sThrowButtonMask = 0x80;

    if (zThrownCollide_CauseDamage(thrown, collis))
    {
        *bounce = 0.0f;
        *friction = 0.0f;
        thrown->killTimer = 1e-6f;
        return;
    }

    if (collis->colls[0].flags & k_HIT_IT)
    {
        F32 stackHeight;
        xEnt* landEnt = (xEnt*)collis->colls[0].optr;
        if (landEnt != NULL && landEnt->baseType == eBaseTypeStatic &&
            zThrown_IsFruit(landEnt, &stackHeight))
        {
            U32 i;
            F32 killTimer = 1000.0f;
            for (i = 0; i < zThrownCount; i++)
            {
                if (zThrownList[i].ent == landEnt)
                {
                    killTimer = zThrownList[i].killTimer;
                }
            }

            F32 dx = ((xMat4x3*)thrown->ent->model->Mat)->pos.x -
                     ((xMat4x3*)landEnt->model->Mat)->pos.x;
            F32 dz = ((xMat4x3*)thrown->ent->model->Mat)->pos.z -
                     ((xMat4x3*)landEnt->model->Mat)->pos.z;
            if (dx * dx + dz * dz < 0.0225f && killTimer > 0.1f)
            {
                ((xMat4x3*)thrown->ent->model->Mat)->pos.y =
                    ((xMat4x3*)landEnt->model->Mat)->pos.y + stackHeight;
                *bounce = 0.0f;
                *friction = 0.0f;
                thrown->stackEnt = landEnt;
                return;
            }
        }

        {
            xEnt* landEnt = (xEnt*)collis->colls[0].optr;
            if (landEnt != NULL && landEnt->baseType == eBaseTypeButton &&
                xabs(collis->colls[0].norm.y) < 0.9659258f)
            {
                *friction = 1.0f;
                *bounce = globals.player.carry.fruitWallBounce;
                return;
            }
        }

        F32 speed = xVec3Length(&thrown->vel);
        if (speed > globals.player.carry.fruitFloorDecayMax)
        {
            *bounce = globals.player.carry.fruitFloorBounce;
            *friction = globals.player.carry.fruitFloorFriction;
        }
        else if (speed < globals.player.carry.fruitFloorDecayMin)
        {
            *bounce = 0.0f;
            *friction = 0.0f;
        }
        else
        {
            F32 pct =
                (speed - globals.player.carry.fruitFloorDecayMin) /
                (globals.player.carry.fruitFloorDecayMax - globals.player.carry.fruitFloorDecayMin);
            *bounce = pct * globals.player.carry.fruitFloorBounce;
            *friction = pct * globals.player.carry.fruitFloorFriction;
        }
        return;
    }

    idx = 0;
    if (collis->env_eidx > collis->env_sidx)
    {
        idx = collis->env_sidx;
    }
    else if (collis->dyn_eidx > collis->dyn_sidx)
    {
        idx = collis->dyn_sidx;
    }
    else if (collis->stat_eidx > collis->stat_sidx)
    {
        idx = collis->stat_sidx;
    }

    if (idx != 0)
    {
        F32 stackHeight;
        xEnt* landEnt = (xEnt*)collis->colls[idx].optr;
        if (landEnt != NULL && landEnt->baseType == eBaseTypeStatic &&
            zThrown_IsFruit(landEnt, &stackHeight))
        {
            U32 i;
            F32 killTimer = 1000.0f;
            for (i = 0; i < zThrownCount; i++)
            {
                if (zThrownList[i].ent == landEnt)
                {
                    killTimer = zThrownList[i].killTimer;
                }
            }

            F32 dx = thrown->oldcollpos.x - ((xMat4x3*)landEnt->model->Mat)->pos.x;
            F32 dy = thrown->oldcollpos.y - ((xMat4x3*)landEnt->model->Mat)->pos.y;
            F32 dz = thrown->oldcollpos.z - ((xMat4x3*)landEnt->model->Mat)->pos.z;
            if (dx * dx + dz * dz < 0.0225f && dy > 0.8f && killTimer > 0.1f)
            {
                ((xMat4x3*)thrown->ent->model->Mat)->pos.x = thrown->oldcollpos.x;
                ((xMat4x3*)thrown->ent->model->Mat)->pos.y =
                    ((xMat4x3*)landEnt->model->Mat)->pos.y + stackHeight;
                ((xMat4x3*)thrown->ent->model->Mat)->pos.z = thrown->oldcollpos.z;
                *bounce = 0.0f;
                *friction = 0.0f;
                thrown->stackEnt = landEnt;
                return;
            }
        }

        *friction = 1.0f;
        if (collis->colls[idx].norm.y < -0.5f)
        {
            *bounce = globals.player.carry.fruitCeilingBounce;
        }
        else
        {
            *bounce = globals.player.carry.fruitWallBounce;
        }
    }
    else
    {
        *bounce = 0.0f;
        *friction = 0.0f;
    }
}

static void zThrownCollide_ThrowFreeze(zThrownStruct* thrown, xEntCollis* collis, float* bounce,
                                float* friction)
{
    sFruitIsFreezy = 1;
    zThrownCollide_ThrowFruit(thrown, collis, bounce, friction);
    sFruitIsFreezy = 0;
}

static void zThrownCollide_DestructObj(zThrownStruct* thrown, xEntCollis* collis, F32* bounce,
                                F32* friction)
{
    sThrowButtonMask = 0x40;
    zThrownCollide_CauseDamage(thrown, collis);
    *bounce = 0.0f;
    *friction = 0.0f;
    zEntEvent(thrown->ent, eEventDestroy);
}

static void zThrownCollide_BSandyHead(zThrownStruct* thrown, xEntCollis* collis, F32* bounce,
                               F32* friction)
{
    sThrowButtonMask = 0x40;
    zThrownCollide_CauseDamage(thrown, collis);
    *bounce = 0.0f;
    *friction = 0.0f;
}

static void zThrownCollide_Tiki(zThrownStruct* thrown, xEntCollis* collis, F32* bounce, F32* friction)
{
    sThrowButtonMask = 0x40;
    zThrownCollide_CauseDamage(thrown, collis);
    *bounce = 0.0f;
    *friction = 0.0f;
    zEntEvent(thrown->ent, eEventDestroy);
}

static void zThrownCollide_StoneTiki(zThrownStruct* thrown, xEntCollis* collis, F32* bounce, F32* friction)
{
    xEnt* hitent;
    U32 collidx;
    U32 collfound;

    *bounce = 0.0f;
    *friction = 0.0f;
    sThrowButtonMask = 0x40;
    zThrownCollide_CauseDamage(thrown, collis);

    for (collidx = k_XCOLLS_IDX_COUNT;
         collidx < 18 && (collis->colls[collidx].flags & k_HIT_IT); collidx++)
    {
        hitent = (xEnt*)collis->colls[collidx].optr;
        if (hitent != NULL)
        {
            if (hitent->baseType == eBaseTypeNPC &&
                (((zNPCCommon*)hitent)->SelfType() & 0xffffff00) == (NPC_TYPE_FISH & 0xffffff00))
            {
                zEntEvent(thrown->ent, eEventKill);
            }
            if (hitent->baseType == eBaseTypeTeleportBox)
            {
                zEntEvent(thrown->ent, eEventKill);
            }
        }
    }

    collfound = 0;
    if (collis->env_eidx > collis->env_sidx)
    {
        collfound = collis->env_sidx;
    }
    else if (collis->dyn_eidx > collis->dyn_sidx)
    {
        collfound = collis->dyn_sidx;
    }
    else if (collis->stat_eidx > collis->stat_sidx)
    {
        collfound = collis->stat_sidx;
    }

    if (collfound != 0)
    {
        if (collis->colls[collfound].norm.y < 0.707f)
        {
            *friction = 1.0f;
            *bounce = globals.player.carry.fruitWallBounce;
        }
    }
    else
    {
        *bounce = 0.0f;
        *friction = 0.0f;
    }
}

// WIP.
S32 zThrown_IsFruit(xEnt* ent, F32* stackHeight)
{
    ThrowableStats* stats;

    stats = zThrowableModels + 1;
    while (stats->name != NULL)
    {
        if (stats->nameHash == ent->asset->modelInfoID ||
            stats->nameHashMINF == ent->asset->modelInfoID)
        {
            break;
        }
        stats++;
    }

    if (stats != NULL && stats->carry == &c_fruit)
    {
        if (stackHeight != NULL)
        {
            *stackHeight = stats->stackHeight;
        }
        return 1;
    }
    return 0;
}

S32 zThrown_IsStacked(xEnt* ent)
{
    for (S32 i = 0; i < zThrownCount; i++)
    {
        if ((zThrownList[i].stackEnt == ent) ||
            (zThrownList[i].stackEnt != 0 && (zThrownList[i].ent == ent)))
        {
            return 1;
        }
    }

    return 0;
}

static void checkAgainstButtons(xEnt* ent)
{
    struct
    {
        xVec3 center;
        F32 unk;
    } data;
    xVec3Copy(&data.center, xBoundCenter(&ent->bound));
    data.unk = 0.5f;
    zSceneForAllBase(zThrown_ButtonIteratorCB, 0x18, (void*)&data);
}

static xBase* zThrown_ButtonIteratorCB(xBase* b, zScene* scn, void* user)
{
    xCollis coll;

    coll.flags = 0;
    xSphereHitsModel((xSphere*)user, ((xEnt*)b)->model, &coll);
    if (coll.flags & k_HIT_IT)
    {
        zEntButton_Hold((_zEntButton*)b, 0x10000);
    }
    return b;
}

void xDrawSphere(const xSphere*, U32)
{
}
