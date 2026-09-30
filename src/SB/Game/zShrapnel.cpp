#include <types.h>
#include "iModel.h"

#include "xColor.h"
#include "xMathInlines.h"
#include "xSnd.h"
#include "xstransvc.h"

#include "zGlobals.h"
#include "zShrapnel.h"
#include "zLightning.h"

static xMat4x3 tmpMat;
static zFrag sFragPool[150];
static zFrag sFirstFreeFrag;
static zFrag sFirstActiveFrag;
static zFrag sProjectileList;
static zFrag sLightningList;
static zFrag sParticleList;
static zFrag sSoundList;
static _tagLightningAdd sLightningAddInfo;
static zFragProjectileAsset sCinProj;

static S32 sNumActiveFrags;
static RpAtomic* sCinModel;
static void (*sCinCB)(zFrag*, zFragAsset*);
static zFrag* sCinFrag;

static void zShrapnel_DestructObjInit(zShrapnelAsset* shrap, xModelInstance* parent,
                                      xVec3* initOffset, void (*cb)(zFrag*, zFragAsset*));
static void zShrapnel_BB03FloorInit(zShrapnelAsset* shrap, xModelInstance* parent,
                                    xVec3* initOffset, void (*cb)(zFrag*, zFragAsset*));
static void zShrapnel_BB03FloorChildInit(zShrapnelAsset* shrap, xModelInstance* parent,
                                         xVec3* initOffset, void (*cb)(zFrag*, zFragAsset*));
static void zShrapnel_GlobalRobotInit(zShrapnelAsset* shrap, xModelInstance* parent,
                                      xVec3* initOffset, void (*cb)(zFrag*, zFragAsset*));
static void zShrapnel_SpongebobInit(zShrapnelAsset* shrap, xModelInstance* parent,
                                    xVec3* initOffset, void (*cb)(zFrag*, zFragAsset*));

static zShrapnelInitTable sShrapnelTable[6] = {
    { "destruct_obj_shrapnel", zShrapnel_DestructObjInit, 0 },
    { "bb03_floor_shrapnel", zShrapnel_BB03FloorInit, 0 },
    { "bb03_floor_child_shrapnel", zShrapnel_BB03FloorChildInit, 0 },
    { "allrobots_shrapnel", zShrapnel_GlobalRobotInit, 0 },
    { "spongebob_shrapnel", zShrapnel_SpongebobInit, 0 },
    { NULL, NULL, 0 }
};

zFrag* zFrag_Alloc(zFragType type)
{
    if (sNumActiveFrags == 150)
    {
        return NULL;
    }

    zFrag* result = sFirstFreeFrag.next;
    if (result->prev != NULL)
    {
        result->prev->next = result->next;
    }

    if (result->next != NULL)
    {
        result->next->prev = result->prev;
    }

    result->next = sFirstActiveFrag.next;
    result->prev = &sFirstActiveFrag;

    if (result->next != NULL)
    {
        result->next->prev = result;
    }

    if (result->prev != NULL)
    {
        result->prev->next = result;
    }

    sNumActiveFrags++;
    result->type = type;

    return result;
}

// equivalent
void zFrag_Free(zFrag* frag)
{
    frag->type = eFragInactive;

    if (frag->next != NULL)
    {
        frag->next->prev = frag->prev;
    }

    if (frag->prev != NULL)
    {
        frag->prev->next = frag->next;
    }

    sNumActiveFrags--;
    frag->next = sFirstFreeFrag.next;
    frag->prev = &sFirstFreeFrag;

    if (frag->next != NULL)
    {
        frag->next->prev = frag;
    }

    if (frag->prev != NULL)
    {
        frag->prev->next = frag;
    }
}

// equivalent
void zShrapnel_GameInit()
{
    zShrapnelInitTable* curr = sShrapnelTable;
    S32 i = 0;
    while (curr->name != NULL)
    {
        curr->ID = xStrHash(curr->name);
        i++;
        curr = (zShrapnelInitTable*)(sShrapnelTable + i);
    }

    sLightningAddInfo.type = 3;
    sLightningAddInfo.flags = 0x30;
    sLightningAddInfo.thickness = 1.0f;
    sLightningAddInfo.color = xColorFromRGBA(255, 255, 255, 255);
    sLightningAddInfo.arc_height = 0.1f;

    sCinProj.type = eFragProjectile;
    sCinProj.id = 0;
    sCinProj.parentID[0] = 0;
    sCinProj.parentID[1] = 0;
    sCinProj.lifetime = 0.0f;
    sCinProj.delay = 0.0f;
    sCinProj.modelInfoID = 0;
    sCinProj.modelFile = NULL;
    sCinProj.launch.type = eFragLocBone;
    xVec3Init((xVec3*)&sCinProj.launch.info.bone.offset, 0.0f, 0.0f, 0.0f);
    sCinProj.launch.info.bone.index = 0;
    sCinProj.vel.type = eFragLocBone;
    xVec3Init((xVec3*)&sCinProj.vel.info.bone.offset, 0.0f, 0.0f, 0.0f);
    sCinProj.vel.info.bone.index = 0;
    sCinProj.bounce = 0.0f;
    sCinProj.maxBounces = -1;
    sCinProj.flags = 0x48;
    sCinProj.childID = 0;
    sCinProj.child = NULL;
    sCinProj.minScale = 1.0f;
    sCinProj.maxScale = 1.0f;
    sCinProj.scaleCurveID = 0;
    sCinProj.scaleCurve = NULL;
    sCinProj.gravity = 0.0f;
}

void zShrapnel_ProjectileSceneInit(zFragProjectileAsset* asset)
{
    if (asset->modelInfoID != 0)
    {
        asset->modelFile = (RpAtomic*)xSTFindAsset(asset->modelInfoID, NULL);
    }

    if (asset->scaleCurveID != 0)
    {
        asset->scaleCurve = (xCurveAsset*)xSTFindAsset(asset->scaleCurveID, NULL);
    }

    if (asset->childID != 0)
    {
        asset->child = (zShrapnelAsset*)xSTFindAsset(asset->childID, NULL);
    }
}

void zShrapnel_ParticleSceneInit(zFragParticleAsset* asset)
{
    if (asset->parEmitterID != 0)
    {
        asset->parEmitter = zParEmitterFind(asset->parEmitterID);
    }
}

// equivalent
void zShrapnel_SetShrapnelAssetInitCB(zShrapnelAsset* sasset)
{
    sasset->initCB = zShrapnel_DefaultInit;
    zShrapnelInitTable* curr = sShrapnelTable;
    S32 i = 0;

    while (curr->name != NULL)
    {
        if (curr->ID == sasset->shrapnelID)
        {
            sasset->initCB = curr->initCB;
            return;
        }
        i++;
        curr = (zShrapnelInitTable*)(sShrapnelTable + i);
    }
}

void zShrapnel_SceneInit(zScene* sc)
{
    S32 i;

    sFirstActiveFrag.next = NULL;
    sFragPool[0].type = eFragInactive;
    sFirstFreeFrag.next = &sFragPool[0];

    for (i = 1; i < 150; i++)
    {
        sFragPool[i - 1].next = &sFragPool[i];
        sFragPool[i].prev = &sFragPool[i - 1];
        sFragPool[i].type = eFragInactive;
    }

    sFragPool[0].prev = &sFirstFreeFrag;
    sFragPool[149].next = NULL;

    sNumActiveFrags = 0;
    sProjectileList.next = NULL;
    sLightningList.next = NULL;
    sParticleList.next = NULL;
    sSoundList.next = NULL;

    sCinModel = (RpAtomic*)xSTFindAsset(xStrHash("frag_generic_wrench"), NULL);

    S32 count = xSTAssetCountByType('SHRP');
    for (i = 0; i < count; i++)
    {
        zShrapnelAsset* sasset = (zShrapnelAsset*)xSTFindAssetByType('SHRP', i, NULL);
        zShrapnel_SetShrapnelAssetInitCB(sasset);

        S32 j;
        zFragAsset* fasset = (zFragAsset*)(sasset + 1);
        for (j = 0; j < sasset->fassetCount; j++)
        {
            switch (fasset->type)
            {
            case eFragProjectile:
                zShrapnel_ProjectileSceneInit((zFragProjectileAsset*)fasset);
                fasset = (zFragAsset*)((zFragProjectileAsset*)fasset + 1);
                break;
            case eFragParticle:
                zShrapnel_ParticleSceneInit((zFragParticleAsset*)fasset);
                fasset = (zFragAsset*)((zFragParticleAsset*)fasset + 1);
                break;
            case eFragSound:
                fasset = (zFragAsset*)((zFragSoundAsset*)fasset + 1);
                break;
            case eFragLightning:
                fasset = (zFragAsset*)((zFragLightningAsset*)fasset + 1);
                break;
            }
        }
    }
}

void zShrapnel_Update(F32 dt)
{
    if (sNumActiveFrags == 0)
        return;

    zFrag* curr = sFirstActiveFrag.next;
    while (curr != NULL)
    {
        zFrag* next = curr->next;

        if (curr->delay > 0.0f)
        {
            curr->delay -= dt;
        }
        else
        {
            if (curr->update != NULL)
            {
                curr->update(curr, dt);
            }
        }
        curr = next;
    }

    if (sProjectileList.next != NULL)
    {
        zFrag_ProjectileManager(dt);
    }

    if (sLightningList.next != NULL)
    {
        zFrag_LightningManager(dt);
    }

    if (sParticleList.next != NULL)
    {
        zFrag_ParticleManager(dt);
    }

    if (sSoundList.next != NULL)
    {
        zFrag_SoundManager(dt);
    }
}

void zShrapnel_Reset()
{
    if (sNumActiveFrags == 0)
        return;

    zFrag* frag = sProjectileList.next;
    while (frag != NULL)
    {
        zFrag* next = frag->next;

        if (frag->info.projectile.model != NULL)
        {
            xModelInstanceFree(frag->info.projectile.model);
        }
        zFrag_Free(frag);
        frag = next;
    }
}

void zShrapnel_Render()
{
    if (sNumActiveFrags != 0 && sProjectileList.next != NULL)
    {
        zFrag_ProjectileRenderer();
    }
}

void zShrapnel_DefaultInit(zShrapnelAsset* shrap, xModelInstance* parent, xVec3* initVel,
                           void (*cb)(zFrag*, zFragAsset*))
{
    if (shrap == NULL || parent == NULL || parent->Mat == NULL)
    {
        return;
    }

    zShrapnelParentList* curr;
    zShrapnelParentList* plist;
    zFrag* frag;
    zFragAsset* fasset;
    S32 i;

    plist = (zShrapnelParentList*)xMemPushTemp(shrap->fassetCount * sizeof(zShrapnelParentList));
    fasset = (zFragAsset*)(shrap + 1);
    curr = plist;
    i = 0;

    while (i < shrap->fassetCount)
    {
        frag = zFrag_Alloc(fasset->type);
        if (frag == NULL)
        {
            break;
        }

        for (S32 k = 0; k < 2; k++)
        {
            if (fasset->parentID[k] == 0)
            {
                frag->parent[k] = parent;
            }
            else
            {
                S32 j = 0;
                while (j < i)
                {
                    if (fasset->parentID[k] == plist[j].parentID)
                    {
                        break;
                    }
                    j++;
                }
                frag->parent[k] = plist[j].parentModel;
            }
        }

        zFrag_DefaultInit(frag, fasset);

        curr->parentID = fasset->id;
        if (frag->type == eFragInactive)
        {
            curr->parentModel = parent;
        }
        else if (fasset->type == eFragProjectile)
        {
            curr->parentModel = frag->info.projectile.model;
            if (initVel != NULL)
            {
                xVec3AddTo(&frag->info.projectile.path.initVel, initVel);
            }
        }
        else
        {
            curr->parentModel = frag->parent[0];
        }

        if (cb != NULL)
        {
            cb(frag, fasset);
        }

        switch (fasset->type)
        {
        case eFragProjectile:
            fasset = (zFragAsset*)((zFragProjectileAsset*)fasset + 1);
            break;
        case eFragParticle:
            fasset = (zFragAsset*)((zFragParticleAsset*)fasset + 1);
            break;
        case eFragSound:
            fasset = (zFragAsset*)((zFragSoundAsset*)fasset + 1);
            break;
        case eFragLightning:
            fasset = (zFragAsset*)((zFragLightningAsset*)fasset + 1);
            break;
        }

        curr++;
        i++;
    }

    xMemPopTemp(plist);
}

static void CinFragCB(zFrag* frag, zFragAsset* asset)
{
    F32 time = frag->delay + frag->lifetime + 1.0f;

    if (time > sCinFrag->lifetime)
    {
        sCinFrag->lifetime = time;
    }

    if (sCinCB != NULL)
    {
        sCinCB(frag, asset);
    }
}

// equivalent
void zShrapnel_CinematicInit(zShrapnelAsset* shrap, RpAtomic* cinModel, RwMatrixTag* animMat,
                             xVec3* initVel, void (*cb)(zFrag*, zFragAsset*))
{
    S32 i;
    zFrag* frag;
    xModelInstance* model;

    if (cinModel == NULL || shrap == NULL || animMat == NULL)
        return;

    frag = zFrag_Alloc(eFragProjectile);
    frag->parent[0] = NULL;
    frag->parent[1] = NULL;
    sCinProj.modelFile = cinModel;

    zFrag_DefaultInit(frag, &sCinProj);

    model = frag->info.projectile.model;
    if (model == NULL)
    {
        zFrag_Free(frag);
    }
    else
    {
        model->Data = sCinModel;
        xMat3x3Copy((xMat4x3*)model->Mat, &g_I3);
        xVec3Copy((xVec3*)&model->Mat->pos, (xVec3*)&animMat->pos);

        for (i = 1; i < model->BoneCount; i++)
        {
            xMat4x3Copy((xMat4x3*)model->Mat + i, (xMat4x3*)animMat + i);
        }

        sCinCB = cb;
        sCinFrag = frag;
        shrap->initCB(shrap, model, initVel, CinFragCB);
        sCinCB = NULL;
        sCinFrag = NULL;
        sCinProj.modelFile = NULL;
    }
}

void zFragLoc_Setup(zFragLocation* loc, xModelInstance* parent)
{
    if ((S32)(loc->type & 0xfffffffe) == 4)
    {
        iModelTagSetup(&loc->info.tag, parent->Data, loc->info.tag.v.x, loc->info.tag.v.y,
                       loc->info.tag.v.z);
    }
}

void zFragLoc_InitMat(zFragLocation* loc, xMat4x3* mat, xModelInstance* parent)
{
    xVec3 tmpVec;

    switch ((loc->type) & 0xfffffffe)
    {
    case eFragLocBone:
    case eFragLocBoneLocal:
    {
        S32 index = loc->info.bone.index;
        if (index >= parent->BoneCount)
        {
            index = 0;
        }

        if (index == 0)
        {
            xMat4x3Copy(mat, (xMat4x3*)parent->Mat);
        }
        else
        {
            xMat4x3Mul(mat, (xMat4x3*)(parent->Mat + index), (xMat4x3*)parent->Mat);
        }
        break;
    }
    case eFragLocTag:
        xMat4x3Identity(mat);
        iModelTagEval(parent->Data, &loc->info.tag, parent->Mat, &mat->pos);
        break;
    }

    switch ((loc->type) & 0xfffffffe)
    {
    case eFragLocBone:
        xMat3x3RMulVec(&tmpVec, (xMat3x3*)parent->Mat, &loc->info.bone.offset);
        xVec3AddTo(&mat->pos, &tmpVec);
        break;
    case eFragLocBoneLocal:
        xMat3x3RMulVec(&tmpVec, (xMat3x3*)mat, &loc->info.bone.offset);
        xVec3AddTo(&mat->pos, &tmpVec);
        break;
    }
}

void zFragLoc_InitVec(zFragLocation* loc, xVec3* vec, xModelInstance* parent)
{
    xVec3 tmpVec;
    xMat4x3 mat;

    switch ((loc->type) & 0xfffffffe)
    {
    case eFragLocBone:
    case eFragLocBoneLocal:
    {
        S32 index = loc->info.bone.index;
        if (index >= parent->BoneCount)
        {
            index = 0;
        }

        if (index == 0)
        {
            xMat4x3Copy(&mat, (xMat4x3*)parent->Mat);
        }
        else
        {
            xMat4x3Mul(&mat, (xMat4x3*)(parent->Mat + index), (xMat4x3*)parent->Mat);
        }
        break;
    }
    case eFragLocTag:
        iModelTagEval(parent->Data, &loc->info.tag, parent->Mat, vec);
        return;
    }

    switch ((loc->type) & 0xfffffffe)
    {
    case eFragLocBone:
        xMat3x3RMulVec(&tmpVec, (xMat3x3*)parent->Mat, &loc->info.bone.offset);
        xVec3AddTo(&mat.pos, &tmpVec);
        break;
    case eFragLocBoneLocal:
        xMat3x3RMulVec(&tmpVec, (xMat3x3*)&mat, &loc->info.bone.offset);
        xVec3AddTo(&mat.pos, &tmpVec);
        break;
    }

    xVec3Copy(vec, &mat.pos);
}

void zFragLoc_InitDir(zFragLocation* loc, xVec3* vec, xModelInstance* parent)
{
    switch ((loc->type) & 0xfffffffe)
    {
    case eFragLocBone:
        xMat3x3RMulVec(vec, (xMat3x3*)parent->Mat, &loc->info.bone.offset);
        break;
    case eFragLocBoneLocal:
    {
        S32 index = loc->info.bone.index;
        if (index >= parent->BoneCount)
        {
            index = 0;
        }

        if (index == 0)
        {
            xMat4x3Copy(&tmpMat, (xMat4x3*)parent->Mat);
            xMat3x3RMulVec(vec, (xMat3x3*)parent->Mat, &loc->info.bone.offset);
        }
        else
        {
            xMat4x3 tmpMat;
            xMat4x3Mul(&tmpMat, (xMat4x3*)(parent->Mat + index), (xMat4x3*)parent->Mat);
            xMat3x3RMulVec(vec, (xMat3x3*)&tmpMat, &loc->info.bone.offset);
        }
        break;
    }
    case eFragLocTag:
        iModelTagEval(parent->Data, &loc->info.tag, parent->Mat, vec);
        break;
    }
}

void zFrag_DefaultInit(zFrag* frag, zFragAsset* fasset)
{
    frag->alivetime = 0.0f;
    frag->lifetime = fasset->lifetime;
    frag->delay = fasset->delay;

    switch (fasset->type)
    {
    case eFragProjectile:
    {
        zFragProjectileAsset* passet = (zFragProjectileAsset*)fasset;

        frag->info.projectile.fasset = passet;
        frag->update = zFrag_DefaultProjectileUpdate;

        if (passet->modelFile != NULL)
        {
            frag->info.projectile.model = xModelInstanceAlloc(passet->modelFile, NULL, 0, 0, NULL);
        }
        else
        {
            frag->info.projectile.model = NULL;
        }

        if (frag->info.projectile.model == NULL)
        {
            zFrag_Free(frag);
        }
        else if (frag->info.projectile.model != NULL)
        {
            if (frag->parent[0] != NULL && (passet->flags & 0x10) == 0)
            {
                zFrag_ProjectileSetupPath(frag, passet);
            }

            frag->info.projectile.model->LightKit = globals.player.ent.lightKit;
            frag->info.projectile.alpha = 1.0f;
            frag->info.projectile.numBounces = 0;
            frag->info.projectile.scale = passet->minScale;

            if (passet->scaleCurve != NULL)
            {
                frag->info.projectile.scale *= xCurveAssetEvaluate(passet->scaleCurve, 0.0f);
            }
        }
        break;

    }
    case eFragLightning:
    {
        zFragLightningAsset* lasset = (zFragLightningAsset*)fasset;

        frag->info.lightning.fasset = lasset;
        frag->update = zFrag_DefaultLightningUpdate;

        if (frag->parent[0] != NULL && frag->parent[1] != NULL)
        {
            zFragLoc_Setup(&lasset->start, frag->parent[0]);
            zFragLoc_Setup(&lasset->end, frag->parent[1]);
        }
        break;

    }
    case eFragParticle:
    {
        zFragParticleAsset* prasset = (zFragParticleAsset*)fasset;

        frag->info.particle.fasset = prasset;
        frag->update = zFrag_DefaultParticleUpdate;

        if (frag->parent[0] != NULL)
        {
            zFragLoc_Setup(&prasset->source, frag->parent[0]);
            zFragLoc_Setup(&prasset->vel, frag->parent[0]);
        }
        break;

    }
    case eFragSound:
    {
        zFragSoundAsset* sasset = (zFragSoundAsset*)fasset;

        frag->info.sound.fasset = sasset;
        frag->update = zFrag_DefaultSoundUpdate;

        if (frag->parent[0] != NULL)
        {
            zFragLoc_Setup(&sasset->source, frag->parent[0]);
            zFragLoc_InitVec(&sasset->source, &frag->info.sound.location, frag->parent[0]);
        }
        break;
    }
    case eFragShockwave:
        break;
    }
}

void zFrag_DefaultParticleUpdate(zFrag* frag, F32 param_2)
{
    zFragParticleAsset* passet = frag->info.particle.fasset;

    zFragLoc_InitVec(&passet->source, &passet->emit.pos, frag->parent[0]);
    zFragLoc_InitDir(&passet->vel, &passet->emit.vel, frag->parent[0]);

    if (frag->prev != NULL)
    {
        frag->prev->next = frag->next;
    }

    if (frag->next != NULL)
    {
        frag->next->prev = frag->prev;
    }

    frag->next = sParticleList.next;
    frag->prev = &sParticleList;

    if (frag->prev != NULL)
    {
        frag->prev->next = frag;
    }

    if (frag->next != NULL)
    {
        frag->next->prev = frag;
    }
}

void zFrag_ParticleManager(F32 dt)
{
    zFrag* frag = sParticleList.next;

    while (frag != NULL)
    {
        zFrag* next = frag->next;
        zFragParticleAsset* passet = frag->info.particle.fasset;

        frag->lifetime -= dt;
        frag->alivetime += dt;

        if (frag->lifetime < 0.0f)
        {
            zFrag_Free(frag);
        }
        else
        {
            if ((passet->source.type & eFragLocBoneUpdated) != 0)
            {
                zFragLoc_InitVec(&passet->source, &passet->emit.pos, frag->parent[0]);
            }

            if ((passet->vel.type & eFragLocBoneUpdated) != 0)
            {
                zFragLoc_InitDir(&passet->vel, &passet->emit.vel, frag->parent[0]);
            }

            frag = next;

            if (passet->parEmitter != NULL)
            {
                xParEmitterEmitCustom(passet->parEmitter, dt, &passet->emit);
            }
        }
        frag = next;
    }
}

void zFrag_ProjectileCollData(zFrag* frag)
{
    xCollis colls;

    frag->info.projectile.tColl = 1e38f;
    frag->info.projectile.path.initPos.y -=
        frag->info.projectile.model->Data->boundingSphere.radius *
        frag->info.projectile.parentScale;
    xParabolaHitsEnv(&frag->info.projectile.path, globals.sceneCur->env, &colls);

    if ((colls.flags & 1) != 0)
    {
        frag->info.projectile.tColl = colls.dist;
        xVec3Copy(&frag->info.projectile.N, &colls.norm);
    }
    else
    {
        frag->info.projectile.tColl = 1e38f;
    }
    frag->info.projectile.path.initPos.y +=
        frag->info.projectile.model->Data->boundingSphere.radius *
        frag->info.projectile.parentScale;
}

void zFrag_ProjectileSetupPath(zFrag* frag, zFragProjectileAsset* passet)
{
    xVec3 tmpVec;

    zFragLoc_Setup(&passet->launch, frag->parent[0]);
    zFragLoc_InitMat(&passet->launch, (xMat4x3*)frag->info.projectile.model->Mat, frag->parent[0]);
    xVec3Copy(&frag->info.projectile.path.initPos, (xVec3*)&frag->info.projectile.model->Mat->pos);
    frag->info.projectile.t = 0.0f;
    frag->info.projectile.parentScale =
        xVec3Length((xVec3*)&frag->info.projectile.model->Mat->right);

    F32 scale = frag->info.projectile.parentScale;
    if (scale > 1.0001f || (scale < 0.9999f && !(scale < 0.0001f && scale > -0.0001f)))
    {
        xVec3SMulBy((xVec3*)&frag->info.projectile.model->Mat->right, 1.0f / scale);
        xVec3SMulBy((xVec3*)&frag->info.projectile.model->Mat->up,
                    1.0f / frag->info.projectile.parentScale);
        xVec3SMulBy((xVec3*)&frag->info.projectile.model->Mat->at,
                    1.0f / frag->info.projectile.parentScale);
    }
    else
    {
        frag->info.projectile.parentScale = 1.0f;
    }

    if (passet->flags & 8)
    {
        zFragLoc_Setup(&passet->vel, frag->parent[0]);
        zFragLoc_InitDir(&passet->vel, &frag->info.projectile.path.initVel, frag->parent[0]);
    }
    else
    {
        xVec3Sub(&frag->info.projectile.path.initVel,
                 (xVec3*)&frag->info.projectile.model->Mat->pos,
                 (xVec3*)&frag->parent[0]->Mat->pos);
        xVec3SMulBy(&frag->info.projectile.path.initVel, 0.25f);
        if (frag->info.projectile.path.initVel.y < 0.0f)
        {
            frag->info.projectile.path.initVel.y = 0.0f;
        }
        frag->info.projectile.path.initVel.y += 3.0f * xurand() + 4.0f;
        frag->info.projectile.path.initVel.x += 6.0f * xurand() - 3.0f;
        frag->info.projectile.path.initVel.z += 6.0f * xurand() - 3.0f;
    }

    if (frag->info.projectile.fasset->flags & 0x20)
    {
        xVec3Copy(&frag->info.projectile.axis, &frag->info.projectile.path.initVel);
        F32 len2 = xVec3Length2(&frag->info.projectile.axis);
        if (len2 > 1e-5f)
        {
            xVec3SMulBy(&frag->info.projectile.axis, 1.0f / xsqrt(len2));
        }
        else if (frag->info.projectile.path.gravity < 0.0f)
        {
            xVec3Init(&frag->info.projectile.axis, 0.0f, 1.0f, 0.0f);
        }
        else
        {
            xVec3Init(&frag->info.projectile.axis, 0.0f, -1.0f, 0.0f);
        }
        xVec3Inv(&tmpVec, &frag->info.projectile.axis);
        xMat3x3LookVec((xMat3x3*)frag->info.projectile.model->Mat, &tmpVec);
    }
    else
    {
        if (frag->info.projectile.path.initVel.x < 0.01f &&
            frag->info.projectile.path.initVel.x > -0.01f &&
            frag->info.projectile.path.initVel.z < 0.01f &&
            frag->info.projectile.path.initVel.z > -0.01f)
        {
            xVec3Init(&frag->info.projectile.axis, 1.0f, 0.0f, 0.0f);
            frag->info.projectile.angVel = 5.0f;
        }
        else
        {
            xVec3Init(&frag->info.projectile.axis, frag->info.projectile.path.initVel.z, 0.0f,
                      -frag->info.projectile.path.initVel.x);
            frag->info.projectile.angVel =
                3.0f * xVec3Normalize(&frag->info.projectile.axis, &frag->info.projectile.axis) +
                4.0f;
        }
        frag->info.projectile.angVel *= 0.5f + xurand();
    }

    frag->info.projectile.path.gravity = passet->gravity;
}

void zFrag_DefaultProjectileUpdate(zFrag* frag, F32 param_2)
{
    zFragProjectileAsset* passet = frag->info.projectile.fasset;

    if (passet->flags & 0x10)
    {
        zFrag_ProjectileSetupPath(frag, passet);
    }

    if (passet->flags & 1)
    {
        frag->info.projectile.path.minTime = 0.0f;
        frag->info.projectile.path.maxTime = 1.0f;
        zFrag_ProjectileCollData(frag);
        frag->info.projectile.t = 0.0f;
    }

    if (frag->prev != NULL)
    {
        frag->prev->next = frag->next;
    }

    if (frag->next != NULL)
    {
        frag->next->prev = frag->prev;
    }

    frag->next = sProjectileList.next;
    frag->prev = &sProjectileList;

    if (frag->prev != NULL)
    {
        frag->prev->next = frag;
    }

    if (frag->next != NULL)
    {
        frag->next->prev = frag;
    }
}

void zFrag_DeleteProjectile(zFrag* frag)
{
    xVec3 finalVel;
    zShrapnelAsset* child = frag->info.projectile.fasset->child;

    if (child != NULL && child->initCB != NULL)
    {
        xParabolaEvalVel(&frag->info.projectile.path, &finalVel, frag->info.projectile.t);
        child->initCB(child, frag->info.projectile.model, &finalVel, NULL);
    }

    if (frag->info.projectile.model != NULL)
    {
        xModelInstanceFree(frag->info.projectile.model);
    }

    zFrag_Free(frag);
}

void zFrag_ProjectileManager(F32 dt)
{
    xVec3 tanVel;
    xVec3 oldPos;
    xMat3x3 spin;
    xVec3 back;
    xVec3 newAxis;
    F32 percent;
    xVec3 uVar1;

    zFrag* frag = sProjectileList.next;

    while (frag != NULL)
    {
        zFrag* next = frag->next;

        frag->lifetime -= dt;
        frag->alivetime += dt;

        if (frag->lifetime < 0.0f)
        {
            zFrag_DeleteProjectile(frag);
        }
        else
        {
            frag->info.projectile.t = frag->info.projectile.t + dt;
            S32 killed = FALSE;

            if ((frag->info.projectile.fasset->flags & 1) != 0)
            {
                if (frag->info.projectile.t > frag->info.projectile.tColl)
                {
                    frag->info.projectile.numBounces++;

                    if (frag->info.projectile.fasset->maxBounces >= 0 &&
                        frag->info.projectile.numBounces > frag->info.projectile.fasset->maxBounces)
                    {
                        xVec3Init(&frag->info.projectile.path.initVel, 0.0f, 0.0f, 0.0f);
                        zFrag_DeleteProjectile(frag);
                        killed = TRUE;
                    }
                    else
                    {
                        xParabolaRecenter(&frag->info.projectile.path, frag->info.projectile.tColl);

                        percent =
                            xVec3Dot(&frag->info.projectile.N, &frag->info.projectile.path.initVel);
                        xVec3AddScaled(&frag->info.projectile.path.initVel,
                                       &frag->info.projectile.N,
                                       -(1.0f + frag->info.projectile.fasset->bounce) * percent);
                        xVec3AddScaled(&frag->info.projectile.path.initPos,
                                       &frag->info.projectile.path.initVel,
                                       frag->info.projectile.t - frag->info.projectile.tColl);
                        frag->info.projectile.path.minTime = 0.0f;
                        frag->info.projectile.path.maxTime = frag->lifetime;

                        if ((frag->info.projectile.fasset->flags & 0x20) == 0)
                        {
                            xVec3Cross(&frag->info.projectile.axis, &frag->info.projectile.N,
                                       &frag->info.projectile.path.initVel);
                            xVec3Normalize(&frag->info.projectile.axis,
                                           &frag->info.projectile.axis);
                            xVec3Copy(&tanVel, &frag->info.projectile.path.initVel);
                            xVec3AddScaled(&tanVel, &frag->info.projectile.N,
                                           percent * frag->info.projectile.fasset->bounce);
                            percent = xVec3Length(&tanVel);

                            frag->info.projectile.angVel =
                                (percent /
                                 (frag->info.projectile.model->Data->boundingSphere.radius *
                                  frag->info.projectile.parentScale));
                        }
                        zFrag_ProjectileCollData(frag);
                        frag->info.projectile.t = 0.0f;
                    }
                }
                else if (frag->info.projectile.t > frag->info.projectile.path.maxTime)
                {
                    frag->info.projectile.path.minTime = frag->info.projectile.path.maxTime;
                    frag->info.projectile.path.maxTime += 1.0f;
                    zFrag_ProjectileCollData(frag);
                }
            }

            if (!killed)
            {
                zFragProjectileAsset* proj = frag->info.projectile.fasset;
                F32 minScale = proj->minScale;

                if (minScale != proj->maxScale || proj->scaleCurve != NULL)
                {
                    F32 totalTime = frag->alivetime / (frag->lifetime + frag->alivetime);
                    F32 newScale = totalTime * (proj->maxScale - minScale) + minScale;

                    if (proj->scaleCurve != NULL)
                    {
                        if ((proj->flags & 4) != 0)
                        {
                            newScale *= xCurveAssetEvaluate(proj->scaleCurve, totalTime);
                        }
                        else
                        {
                            newScale *= xCurveAssetEvaluate(proj->scaleCurve, frag->alivetime);
                        }
                    }
                    frag->info.projectile.scale = newScale;
                }

                xVec3Copy(&oldPos, (xVec3*)&frag->info.projectile.model->Mat->pos);
                xVec3Copy((xVec3*)&frag->info.projectile.model->Mat->pos,
                          &frag->info.projectile.path.initPos);
                xVec3AddScaled((xVec3*)&frag->info.projectile.model->Mat->pos,
                               &frag->info.projectile.path.initVel, frag->info.projectile.t);
                frag->info.projectile.model->Mat->pos.y -=
                    frag->info.projectile.t *
                    ((0.5f * frag->info.projectile.path.gravity) * frag->info.projectile.t);

                if ((frag->info.projectile.fasset->flags & 0x20) != 0)
                {
                    xVec3Sub(&back, (xVec3*)&frag->info.projectile.model->Mat->pos, &oldPos);
                    percent = xVec3Length2(&back);

                    if (percent > 1e-5f)
                    {
                        percent = xsqrt(percent);
                        xVec3SMul((xVec3*)&frag->info.projectile.axis, &back, 1.0f / percent);
                    }

                    xVec3Inv(&newAxis, (xVec3*)&frag->info.projectile.axis);
                    xMat3x3LookVec((xMat3x3*)frag->info.projectile.model->Mat, &newAxis);
                }
                else
                {
                    xMat3x3Rot(&spin, &frag->info.projectile.axis,
                               dt * frag->info.projectile.angVel);
                    xMat3x3Mul((xMat3x3*)frag->info.projectile.model->Mat,
                               (xMat3x3*)frag->info.projectile.model->Mat, &spin);
                }

                if ((frag->info.projectile.fasset->flags & 2) != 0)
                {
                    xParabolaEvalVel(&frag->info.projectile.path, &uVar1, frag->info.projectile.t);

                    U32 numBubbles = 0.2f * xVec3LengthFast(&uVar1);
                    if (numBubbles < 1)
                    {
                        numBubbles = 1;
                    }

                    zFX_SpawnBubbleTrail((xVec3*)&frag->info.projectile.model->Mat->pos,
                                         numBubbles);
                }
            }
        }
        frag = next;
    }
}

void zFrag_DefaultLightningUpdate(zFrag* frag, F32 param_2)
{
    xVec3 start, end;
    zFragLightningAsset* lasset = frag->info.lightning.fasset;

    sLightningAddInfo.start = &start;
    sLightningAddInfo.end = &end;

    zFragLoc_InitVec(&lasset->start, sLightningAddInfo.start, frag->parent[0]);
    zFragLoc_InitVec(&lasset->end, sLightningAddInfo.end, frag->parent[1]);

    frag->info.lightning.lightning = zLightningAdd(&sLightningAddInfo);

    if (frag->prev != NULL)
    {
        frag->prev->next = frag->next;
    }

    if (frag->next != NULL)
    {
        frag->next->prev = frag->prev;
    }

    frag->next = sLightningList.next;
    frag->prev = &sLightningList;

    if (frag->prev != NULL)
    {
        frag->prev->next = frag;
    }

    if (frag->next != NULL)
    {
        frag->next->prev = frag;
    }
}

// equivalent
void zFrag_LightningManager(F32 dt)
{
    xVec3 start;
    xVec3 end;
    zFrag* frag = sLightningList.next;

    while (frag != NULL)
    {
        zFragLightningAsset* lasset;
        zLightning* inst;
        zFrag* next;

        next = frag->next;
        inst = frag->info.lightning.lightning;
        lasset = frag->info.lightning.fasset;

        frag->lifetime -= dt;
        frag->alivetime += dt;

        if (frag->lifetime < 0.0f)
        {
            zLightningKill(inst);
            zFrag_Free(frag);
        }
        else
        {
            if ((lasset->start.type & eFragLocBoneUpdated) != 0 ||
                (lasset->end.type & eFragLocBoneUpdated) != 0)
            {
                if ((lasset->start.type & eFragLocBoneUpdated) != 0)
                {
                    zFragLoc_InitVec(&lasset->start, &start, frag->parent[0]);
                }
                else
                {
                    xVec3Copy(&start, (xVec3*)((U8*)inst + 0x8));
                }

                if ((lasset->end.type & eFragLocBoneUpdated) != 0)
                {
                    zFragLoc_InitVec(&lasset->end, &end, frag->parent[1]);
                }
                else
                {
                    xVec3Copy(&end, (xVec3*)((U8*)inst + 0x14));
                }
                zLightningModifyEndpoints(inst, &start, &end);
            }
        }
        frag = next;
    }
}

void zFrag_DefaultSoundUpdate(zFrag* frag, F32 param_2)
{
    zFragSoundAsset* sasset = frag->info.sound.fasset;

    if (sasset->delay > 0.0001f)
    {
        zFragLoc_InitVec(&sasset->source, &frag->info.sound.location, frag->parent[0]);
    }

    frag->info.sound.soundID =
        xSndPlay3D(sasset->assetID, 0.77f * sasset->volume, 0.0f, 0, 0, &frag->info.sound.location,
                   sasset->innerRadius, sasset->outerRadius, SND_CAT_GAME, 0.0f);

    if (frag->prev != NULL)
    {
        frag->prev->next = frag->next;
    }

    if (frag->next != NULL)
    {
        frag->next->prev = frag->prev;
    }

    frag->next = sSoundList.next;
    frag->prev = &sSoundList;

    if (frag->prev != NULL)
    {
        frag->prev->next = frag;
    }

    if (frag->next != NULL)
    {
        frag->next->prev = frag;
    }
}

void zFrag_SoundManager(F32 dt)
{
    zFrag* frag = sSoundList.next;

    while (frag != NULL)
    {
        zFrag* next = frag->next;

        frag->lifetime -= dt;
        frag->alivetime += dt;

        if (frag->lifetime < 0.0f)
        {
            if (frag->info.sound.soundID != 0)
            {
                xSndStop(frag->info.sound.soundID);
            }
            zFrag_Free(frag);
        }
        frag = next;
    }
}

void zFrag_ProjectileRenderer()
{
    xLightKit_Enable(globals.player.ent.lightKit, globals.currWorld);

    for (zFrag* frag = sProjectileList.next; frag != NULL; frag = frag->next)
    {
        if (frag->info.projectile.model != NULL && !(frag->info.projectile.fasset->flags & 0x40))
        {
            F32 scale = frag->info.projectile.scale * frag->info.projectile.parentScale;
            frag->info.projectile.model->Scale.z = scale;
            frag->info.projectile.model->Scale.y = scale;
            frag->info.projectile.model->Scale.x = scale;
            frag->info.projectile.model->Alpha = frag->info.projectile.alpha;

            if (frag->lifetime < 1.0f && frag->info.projectile.fasset->child == NULL)
            {
                frag->info.projectile.model->Alpha *= frag->lifetime;
            }

            xModelRender(frag->info.projectile.model);
        }
    }

    xLightKit_Enable(NULL, globals.currWorld);
}

static void zShrapnel_DestructObjInit(zShrapnelAsset* shrap, xModelInstance* parent, xVec3* initVel,
                                      void (*cb)(zFrag*, zFragAsset*))
{
    xVec3 center;
    xVec3 offset;
    xVec3 tmpVec;
    xMat4x3 mat;
    zFrag* frag;
    zFragProjectileAsset* passet;
    S32 i;
    S32 count;

    if (shrap == NULL || parent == NULL)
    {
        return;
    }

    xMat4x3Copy(&mat, (xMat4x3*)parent->Mat);
    F32 scale = xVec3Length(&mat.right);
    F32 inv = 1.0f / scale;
    xVec3SMulBy(&mat.right, inv);
    xVec3SMulBy(&mat.up, inv);
    xVec3SMulBy(&mat.at, inv);

    xMat3x3RMulVec(&center, (xMat3x3*)&mat, (xVec3*)&parent->Data->boundingSphere.center);
    xVec3SMulBy(&center, scale);
    xVec3AddTo(&center, &mat.pos);

    F32 radius2;
    F32 radius = 0.75f * (parent->Data->boundingSphere.radius * scale);

    radius2 = radius * radius;
    count = (S32)(radius * radius2);
    if (count < 3)
    {
        count = 3;
    }
    if (count > 10)
    {
        count = 10;
    }

    for (i = 0; i < count; i++)
    {
        S32 idx = (S32)(shrap->fassetCount * xurand());
        if (idx >= shrap->fassetCount)
        {
            idx = shrap->fassetCount - 1;
        }

        passet = (zFragProjectileAsset*)(shrap + 1) + idx;
        frag = zFrag_Alloc(passet->type);
        if (frag == NULL)
        {
            return;
        }

        frag->alivetime = 0.0f;
        frag->lifetime = passet->lifetime;
        frag->delay = passet->delay;
        frag->info.projectile.fasset = passet;
        frag->update = zFrag_DefaultProjectileUpdate;

        if (passet->modelFile != NULL)
        {
            frag->info.projectile.model = xModelInstanceAlloc(passet->modelFile, NULL, 0, 0, NULL);
        }
        else
        {
            frag->info.projectile.model = NULL;
        }

        if (frag->info.projectile.model == NULL)
        {
            zFrag_Free(frag);
        }
        else if (frag->info.projectile.model != NULL)
        {
            offset.x = radius * (2.0f * xurand() - 1.0f);
            offset.y = (2.0f * xurand() - 1.0f) * xsqrt(radius2 - offset.x * offset.x);
            offset.z = (2.0f * xurand() - 1.0f) *
                       xsqrt((radius2 - offset.x * offset.x) - offset.y * offset.y);

            frag->info.projectile.parentScale = 1.0f;
            xMat3x3Copy((xMat4x3*)frag->info.projectile.model->Mat, &mat);
            xVec3Copy((xVec3*)&frag->info.projectile.model->Mat->pos, &center);
            xVec3AddTo((xVec3*)&frag->info.projectile.model->Mat->pos, &offset);
            xVec3Copy(&frag->info.projectile.path.initPos,
                      (xVec3*)&frag->info.projectile.model->Mat->pos);
            frag->info.projectile.t = 0.0f;

            xVec3Sub(&frag->info.projectile.path.initVel,
                     (xVec3*)&frag->info.projectile.model->Mat->pos, (xVec3*)&parent->Mat->pos);
            xVec3SMulBy(&frag->info.projectile.path.initVel, 0.25f);
            if (frag->info.projectile.path.initVel.y < 0.0f)
            {
                frag->info.projectile.path.initVel.y = 0.0f;
            }
            frag->info.projectile.path.initVel.y += 2.0f * xurand() + 4.0f;
            frag->info.projectile.path.initVel.x += 2.0f * xurand() - 1.0f;
            frag->info.projectile.path.initVel.z += 2.0f * xurand() - 1.0f;

            if (frag->info.projectile.fasset->flags & 0x20)
            {
                xVec3Copy(&frag->info.projectile.axis, &frag->info.projectile.path.initVel);
                F32 len2 = xVec3Length2(&frag->info.projectile.axis);
                if (len2 > 1e-5f)
                {
                    xVec3SMulBy(&frag->info.projectile.axis, 1.0f / xsqrt(len2));
                }
                else if (frag->info.projectile.path.gravity < 0.0f)
                {
                    xVec3Init(&frag->info.projectile.axis, 0.0f, 1.0f, 0.0f);
                }
                else
                {
                    xVec3Init(&frag->info.projectile.axis, 0.0f, -1.0f, 0.0f);
                }
                xVec3Inv(&tmpVec, &frag->info.projectile.axis);
                xMat3x3LookVec((xMat3x3*)frag->info.projectile.model->Mat, &tmpVec);
            }
            else
            {
                if (frag->info.projectile.path.initVel.x < 0.01f &&
                    frag->info.projectile.path.initVel.x > -0.01f &&
                    frag->info.projectile.path.initVel.z < 0.01f &&
                    frag->info.projectile.path.initVel.z > -0.01f)
                {
                    xVec3Init(&frag->info.projectile.axis, 1.0f, 0.0f, 0.0f);
                    frag->info.projectile.angVel = 5.0f;
                }
                else
                {
                    xVec3Init(&frag->info.projectile.axis, frag->info.projectile.path.initVel.z,
                              0.0f, -frag->info.projectile.path.initVel.x);
                    frag->info.projectile.angVel =
                        3.0f * xVec3Normalize(&frag->info.projectile.axis,
                                              &frag->info.projectile.axis) +
                        4.0f;
                }
            }

            frag->info.projectile.angVel *= 0.5f + xurand();
            frag->info.projectile.path.gravity = passet->gravity;
            frag->info.projectile.numBounces = 0;
        }
    }
}

static void zShrapnel_BB03FloorInit(zShrapnelAsset* shrap, xModelInstance* parent, xVec3* initVel,
                                    void (*cb)(zFrag*, zFragAsset*))
{
    zFragProjectileAsset* passets[3];
    xVec3 origin;
    xVec3 uAxis;
    xVec3 vAxis;
    zFragProjectileAsset* passet;
    zFrag* frag;
    S32 u;
    S32 v;

    if (shrap == NULL || parent == NULL)
    {
        return;
    }

    if (shrap->fassetCount != 4)
    {
        return;
    }

    passets[0] = (zFragProjectileAsset*)(shrap + 1);
    if (passets[0]->type != eFragProjectile)
    {
        return;
    }

    passets[1] = passets[0] + 1;
    if (passets[1]->type != eFragProjectile)
    {
        return;
    }

    passets[2] = passets[1] + 1;
    if (passets[2]->type != eFragProjectile)
    {
        return;
    }

    zFragSoundAsset* sasset = (zFragSoundAsset*)(passets[2] + 1);
    if (sasset->type != eFragSound)
    {
        return;
    }

    frag = zFrag_Alloc(sasset->type);
    if (frag != NULL)
    {
        frag->parent[0] = parent;
        frag->parent[1] = parent;
        zFrag_DefaultInit(frag, sasset);
    }

    zFragLoc_Setup(&passets[0]->launch, parent);
    zFragLoc_InitVec(&passets[0]->launch, &origin, parent);
    zFragLoc_Setup(&passets[1]->launch, parent);
    zFragLoc_InitVec(&passets[1]->launch, &uAxis, parent);
    zFragLoc_Setup(&passets[2]->launch, parent);
    zFragLoc_InitVec(&passets[2]->launch, &vAxis, parent);
    xVec3SubFrom(&uAxis, &origin);
    xVec3SubFrom(&vAxis, &origin);

    for (u = -2; u < 3; u++)
    {
        for (v = -2; v < 3; v++)
        {
            S32 idx = (S32)(3.0 * xurand());
            if (idx >= 3)
            {
                idx = 2;
            }

            frag = zFrag_Alloc(eFragProjectile);
            if (frag == NULL)
            {
                return;
            }

            passet = passets[idx];
            frag->lifetime = passet->lifetime;
            frag->delay = passet->delay;
            frag->info.projectile.fasset = passet;
            frag->update = zFrag_DefaultProjectileUpdate;

            if (passet->modelFile != NULL)
            {
                frag->info.projectile.model =
                    xModelInstanceAlloc(passet->modelFile, NULL, 0, 0, NULL);
            }
            else
            {
                frag->info.projectile.model = NULL;
            }

            if (frag->info.projectile.model == NULL)
            {
                zFrag_Free(frag);
                return;
            }

            if (frag->info.projectile.model != NULL && parent != NULL)
            {
                zFragLoc_InitMat(&frag->info.projectile.fasset->launch,
                                 (xMat4x3*)frag->info.projectile.model->Mat, parent);
                xVec3Copy(&frag->info.projectile.path.initPos, &origin);
                xVec3AddScaled(&frag->info.projectile.path.initPos, &uAxis, u);
                xVec3AddScaled(&frag->info.projectile.path.initPos, &vAxis, v);
                frag->info.projectile.t = 0.0f;
                xVec3Init(&frag->info.projectile.path.initVel, 0.0f,
                          5.0f * frag->lifetime - 12.0f / frag->lifetime, 0.0f);

                F32 r = xurand();
                frag->info.projectile.axis.x = r;
                frag->info.projectile.axis.y = 0.0f;
                frag->info.projectile.axis.z = xsqrt(-(r * r - 1.0f));
                frag->info.projectile.angVel = 1.5f * xurand();
                frag->info.projectile.path.gravity = 10.0f;
                frag->info.projectile.numBounces = 0;
            }
        }
    }
}

static void BB03FloorChildCB(zFrag* frag, zFragAsset* fasset)
{
    if (frag->type == eFragProjectile)
    {
        frag->info.projectile.path.initVel.x = 4.0f * (xurand() - 0.5f);
        frag->info.projectile.path.initVel.y = 2.0f * (1.0f + xurand());
        frag->info.projectile.path.initVel.z = 4.0f * (xurand() - 0.5f);
    }
}

static void zShrapnel_BB03FloorChildInit(zShrapnelAsset* shrap, xModelInstance* parent,
                                         xVec3* initVel, void (*cb)(zFrag*, zFragAsset*))
{
    zShrapnel_DefaultInit(shrap, parent, initVel, BB03FloorChildCB);
}

static void zShrapnel_GlobalRobotInit(zShrapnelAsset* shrap, xModelInstance* parent, xVec3* initVel,
                                      void (*cb)(zFrag*, zFragAsset*))
{
    zShrapnelParentList* curr;
    zShrapnelParentList* plist;
    zFrag* frag;
    zFragAsset* fasset;
    S32 i;
    xVec3 pos;

    plist = (zShrapnelParentList*)xMemPushTemp(shrap->fassetCount * sizeof(zShrapnelParentList));
    xVec3Copy(&pos, (xVec3*)&parent->Mat->pos);
    pos.y += 0.5f;
    zFX_SpawnBubbleHit(&pos, 0x50);

    fasset = (zFragAsset*)(shrap + 1);
    curr = plist;
    i = 0;

    while (i < shrap->fassetCount)
    {
        frag = zFrag_Alloc(fasset->type);
        if (frag == NULL)
        {
            break;
        }

        for (S32 k = 0; k < 2; k++)
        {
            if (fasset->parentID[k] == 0)
            {
                frag->parent[k] = parent;
            }
            else
            {
                S32 j = 0;
                while (j < i)
                {
                    if (fasset->parentID[k] == plist[j].parentID)
                    {
                        break;
                    }
                    j++;
                }
                frag->parent[k] = plist[j].parentModel;
            }
        }

        zFrag_DefaultInit(frag, fasset);

        curr->parentID = fasset->id;
        if (frag->type == eFragInactive)
        {
            curr->parentModel = parent;
        }
        else if (fasset->type == eFragProjectile)
        {
            curr->parentModel = frag->info.projectile.model;
            frag->info.projectile.fasset->flags |= 2;
            frag->info.projectile.path.initVel.x *= 3.5f;
            frag->info.projectile.path.initVel.y += 2.0f;
            frag->info.projectile.path.initVel.z *= 3.5f;
        }
        else
        {
            curr->parentModel = frag->parent[0];
        }

        switch (fasset->type)
        {
        case eFragProjectile:
            fasset = (zFragAsset*)((zFragProjectileAsset*)fasset + 1);
            break;
        case eFragParticle:
            fasset = (zFragAsset*)((zFragParticleAsset*)fasset + 1);
            break;
        case eFragSound:
            fasset = (zFragAsset*)((zFragSoundAsset*)fasset + 1);
            break;
        case eFragLightning:
            fasset = (zFragAsset*)((zFragLightningAsset*)fasset + 1);
            break;
        }

        curr++;
        i++;
    }

    xMemPopTemp(plist);
}

static void zShrapnel_SpongebobInit(zShrapnelAsset* shrap, xModelInstance* parent, xVec3* initVel,
                                    void (*cb)(zFrag*, zFragAsset*))
{
    zShrapnelParentList* curr;
    zShrapnelParentList* plist;
    zFrag* frag;
    zFragAsset* fasset;
    S32 i;
    xVec3 pos;

    plist = (zShrapnelParentList*)xMemPushTemp(shrap->fassetCount * sizeof(zShrapnelParentList));
    xVec3Copy(&pos, (xVec3*)&parent->Mat->pos);
    pos.y += 0.5f;
    zFX_SpawnBubbleHit(&pos, 0x50);

    fasset = (zFragAsset*)(shrap + 1);
    curr = plist;
    i = 0;

    while (i < shrap->fassetCount)
    {
        frag = zFrag_Alloc(fasset->type);
        if (frag == NULL)
        {
            break;
        }

        for (S32 k = 0; k < 2; k++)
        {
            if (fasset->parentID[k] == 0)
            {
                frag->parent[k] = parent;
            }
            else
            {
                S32 j = 0;
                while (j < i)
                {
                    if (fasset->parentID[k] == plist[j].parentID)
                    {
                        break;
                    }
                    j++;
                }
                frag->parent[k] = plist[j].parentModel;
            }
        }

        zFrag_DefaultInit(frag, fasset);

        curr->parentID = fasset->id;
        if (frag->type == eFragInactive)
        {
            curr->parentModel = parent;
        }
        else if (fasset->type == eFragProjectile)
        {
            curr->parentModel = frag->info.projectile.model;
            if (initVel != NULL)
            {
                xVec3AddTo(&frag->info.projectile.path.initVel, initVel);
            }
            frag->info.projectile.path.initVel.x *= 0.25f * (0.5f + xurand());
            frag->info.projectile.path.initVel.y *= 0.6f * (0.5f + xurand());
            frag->info.projectile.path.initVel.z *= 0.25f * (0.5f + xurand());
            frag->info.projectile.angVel *= 0.3f * (0.5f + xurand());
            frag->info.projectile.parentScale = 1.0f;
        }
        else
        {
            curr->parentModel = frag->parent[0];
        }

        switch (fasset->type)
        {
        case eFragProjectile:
            fasset = (zFragAsset*)((zFragProjectileAsset*)fasset + 1);
            break;
        case eFragParticle:
            fasset = (zFragAsset*)((zFragParticleAsset*)fasset + 1);
            break;
        case eFragSound:
            fasset = (zFragAsset*)((zFragSoundAsset*)fasset + 1);
            break;
        case eFragLightning:
            fasset = (zFragAsset*)((zFragLightningAsset*)fasset + 1);
            break;
        }

        curr++;
        i++;
    }

    xMemPopTemp(plist);
}
