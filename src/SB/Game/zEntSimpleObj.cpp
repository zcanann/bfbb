#include "zEntSimpleObj.h"

#include <types.h>

struct zSimpleMgr
{
    xSphere worldBound; // 0x00
    F32 lodDist[4]; // 0x10
    U16 entFlags; // 0x20
    U8 lastlod; // 0x22
    U8 padA; // 0x23
    xModelBucket** lodBucket[4]; // 0x24
    RwMatrixTag* mat; // 0x34
    zEntSimpleObj* ent; // 0x38
    U32 padB; // 0x3C
};

static U32 sMgrCount;
static zSimpleMgr* sMgrList;
static u32 sSimpleCustomCount;
static xEnt** sSimpleCustomList;

void zEntSimpleObj_MgrInit(zEntSimpleObj** entList, U32 entCount)
{
    zEntSimpleObj** tempEntPtr;
    u32 i;
    zEntSimpleObj** tempEntList;
    u32 tempEntCount;
    u32 custEntCount;
    u32 trailerHash;
    zSimpleMgr* smgr;
    s32 custIndex;
    s32 sflags;
    u8 moreFlags;
    zEntSimpleObj* ent;
    zEntSimpleObj* ent3;
    zEntSimpleObj* ent2;

    sMgrCount = 0;
    sMgrList = NULL;
    sSimpleCustomCount = 0;
    sSimpleCustomList = NULL;
    if (entCount != 0)
    {
        tempEntList = (zEntSimpleObj**)RwMalloc(entCount * sizeof(zEntSimpleObj*));
        tempEntCount = 0;
        custEntCount = 0;
        trailerHash = xStrHash("trailer_hitch\0xEntAutoEventSimple");
        tempEntPtr = tempEntList;
        i = 0U;
        while (i < entCount)
        {
            ent = entList[i];
            sflags = ent->sflags;
            if (!(sflags & 0x10))
            {
                if ((ent->update != (xEntUpdateCallback)zEntSimpleObj_Update) ||
                    (ent->render != zEntSimpleObj_Render) ||
                    (ent->eventFunc != (xBaseEventCB)zEntSimpleObjEventCB) ||
                    (ent->move != NULL) ||
                    (moreFlags = ent->moreFlags, (((moreFlags & 8) == 0) == 0)) ||
                    (moreFlags & 0x20) || (ent->miscflags & 1) || (ent->atbl != NULL) ||
                    (sflags & 4) || (sflags & 8) || (trailerHash == ent->asset->modelInfoID) ||
                    (ent->baseType == eBaseTypeTrackPhysics) || (ent->driver != NULL))
                {
                    custIndex = entCount;
                    custIndex -= 1;
                    custIndex -= custEntCount;
                    custEntCount += 1;
                    tempEntList[custIndex] = ent;
                    ent2 = entList[i];
                    if ((ent2->driver != NULL) && (ent2->move == NULL))
                    {
                        ent2->move = zEntSimpleObj_Move;
                        ent3 = entList[i];
                        ent3->pflags |= 1;
                        entList[i]->frame = (xEntFrame*)xMemAlloc(gActiveHeap, 0xE4U, 0);
                    }
                }
                else
                {
                    tempEntCount += 1;
                    ent->baseFlags |= 0x80;
                    *tempEntPtr = entList[i];
                    tempEntPtr += 1;
                }
            }
            i += 1;
        }

        if (custEntCount != 0)
        {
            sSimpleCustomCount = custEntCount;
            sSimpleCustomList =
                (xEnt**)xMemAlloc(gActiveHeap, custEntCount * sizeof(xEnt*), 0);
            for (i = 0; i < custEntCount; i++)
            {
                sSimpleCustomList[i] = (xEnt*)tempEntList[(entCount - 1) - i];
            }
        }
        else
        {
            sSimpleCustomCount = 0;
            sSimpleCustomList = NULL;
        }
        if (tempEntCount == 0)
        {
            RwFree(tempEntList);
            return;
        }
        sMgrCount = tempEntCount;
        sMgrList = (zSimpleMgr*)xMemAlloc(gActiveHeap, tempEntCount * sizeof(zSimpleMgr), 0x40);
        tempEntPtr = tempEntList;
        i = 0U;
        smgr = sMgrList;
        while (i < tempEntCount)
        {
            RpAtomic* model = (*tempEntPtr)->model->Data;
            RwSphere oldbound = model->boundingSphere;

            model->boundingSphere.radius *= 1.1f;
            iModelCull(model, (*tempEntPtr)->model->Mat);
            model->boundingSphere = oldbound;

            smgr->worldBound.center.x = model->worldBoundingSphere.center.x;
            smgr->worldBound.center.y = model->worldBoundingSphere.center.y;
            smgr->worldBound.center.z = model->worldBoundingSphere.center.z;
            smgr->worldBound.r = model->worldBoundingSphere.radius;

            zLODTable* lod = zLOD_Get(*tempEntPtr);
            if (lod != NULL)
            {
                RwMatrixTag* m = (*tempEntPtr)->model->Mat;
                F32 distscale = SQR(m->right.x) + SQR(m->right.y) + SQR(m->right.z);

                if (distscale < 0.0001f)
                {
                    distscale = 1.0f;
                }

                smgr->lodDist[0] = lod->lodDist[0] ? lod->lodDist[0] * distscale : 1e38f;
                smgr->lodDist[1] = lod->lodDist[1] ? lod->lodDist[1] * distscale : 1e38f;
                smgr->lodDist[2] = lod->lodDist[2] ? lod->lodDist[2] * distscale : 1e38f;
                smgr->lodDist[3] =
                    lod->noRenderDist ? lod->noRenderDist * distscale : 1e38f;

                smgr->lodBucket[0] = lod->baseBucket;
                smgr->lodBucket[1] = lod->lodBucket[0];
                smgr->lodBucket[2] = lod->lodBucket[1];
                smgr->lodBucket[3] = lod->lodBucket[2];

                if (smgr->lodBucket[1] == NULL)
                {
                    smgr->lodDist[0] = 1e38f;
                }

                if (smgr->lodBucket[2] == NULL)
                {
                    smgr->lodDist[1] = 1e38f;
                }

                if (smgr->lodBucket[3] == NULL)
                {
                    smgr->lodDist[2] = 1e38f;
                }
            }
            else
            {
                smgr->lodDist[0] = 1e38f;
                smgr->lodDist[1] = 1e38f;
                smgr->lodDist[2] = 1e38f;
                smgr->lodDist[3] = 1e38f;

                smgr->lodBucket[0] = (*tempEntPtr)->model->Bucket;
                smgr->lodBucket[1] = NULL;
                smgr->lodBucket[2] = NULL;
                smgr->lodBucket[3] = NULL;
            }

            smgr->entFlags = (*tempEntPtr)->flags;
            smgr->mat = (*tempEntPtr)->model->Mat;
            smgr->ent = *tempEntPtr;
            smgr->lastlod = 0xFF;

            xEntUpdate(*tempEntPtr, globals.sceneCur, 0.0f);

            smgr++;
            tempEntPtr += 1;
            i += 1;
        }
        RwFree(tempEntList);
    }
}

void zEntSimpleObj_MgrUpdateRender(RpWorld* world, F32 dt)
{
    u32 i;
    xVec3* campos;
    zSimpleMgr* smgr;
    zEntSimpleObj* ent;
    f32 camdist2;
    u8 picklod;
    xModelInstance* model;
    f32 duration;
    xQuat* q0;
    xVec3* t0;

    campos = &globals.camera.mat.pos;

    smgr = sMgrList;
    for (i = 0; i < sMgrCount; i++, smgr++)
    {
        ent = smgr->ent;
        if (xEntIsVisible(ent) != 0U)
        {
            camdist2 = SQR(campos->x - smgr->worldBound.center.x) +
                       SQR(campos->y - smgr->worldBound.center.y) +
                       SQR(campos->z - smgr->worldBound.center.z);
            if (!(camdist2 > smgr->lodDist[3]) && (iModelSphereCull(&smgr->worldBound) == 0))
            {
                picklod = 0;
                if (camdist2 > smgr->lodDist[0])
                {
                    picklod = 1;
                    if (camdist2 > smgr->lodDist[1])
                    {
                        picklod = 2;
                        if (camdist2 > smgr->lodDist[2])
                        {
                            picklod = 3;
                        }
                    }
                }
                model = ent->model;
                model->Flags &= (u16)0xFBFF;
                smgr->lastlod = picklod;
                model->Bucket = smgr->lodBucket[picklod];
                model->Data = (*model->Bucket)->OriginalData;
                if (picklod == 0)
                {
                    xModelInstance* m = model->Next;
                    while (m != NULL)
                    {
                        m->Flags = (u16)(m->Flags & 0xFBFF);
                        m = m->Next;
                    }
                }
                else
                {
                    xModelInstance* m = model->Next;
                    while (m != NULL)
                    {
                        m->Flags = (u16)(m->Flags | 0x400);
                        m = m->Next;
                    }
                }
                if ((ent->anim != NULL) && (zGameIsPaused() == 0))
                {
                    duration = iAnimDuration(ent->anim);
                    ent->animTime += dt;
                    if (ent->animTime >= duration)
                    {
                        ent->animTime -= duration;
                    }
                    q0 = (xQuat*)giAnimScratch;
                    t0 = (xVec3*)(q0 + IANIM_MAXBONES);
                    iAnimEval(ent->anim, ent->animTime, 0U, t0, q0);
                    model = ent->model;
                    iModelAnimMatrices(model->Data, q0, t0, model->Mat + 1);
                }
                xLightKit_Enable(ent->lightKit, globals.currWorld);
                zEntSimpleObj_Render(ent);
                if ((picklod == 0) && ((u16)xrand() < 0x55U))
                {
                    xVec3 blob_posrnd = { 0.25f, 1.0f, 0.25f };
                    xVec3 pos;

                    xVec3Copy(&pos, (xVec3*)&ent->model->Mat->pos);
                    pos.y += (0.25f * xurand()) + 0.25f;
                    zFX_SpawnBubbleTrail(&pos, (xrand() & 7) + 1, &blob_posrnd, NULL);
                }
            }
        }
    }
}

void zEntSimpleObj_MgrCustomUpdate(zScene* s, F32 dt)
{
    s32 var_r31;
    u32 var_r30;
    xEnt* temp_r3;

    var_r31 = 0;
    var_r30 = 0U;

    while (var_r30 < (u32)sSimpleCustomCount)
    {
        temp_r3 = sSimpleCustomList[var_r31];
        if (!(temp_r3->baseFlags & 0x40))
        {
            temp_r3->update(temp_r3, s, dt);
        }
        var_r31 += 1;
        var_r30 += 1;
    }
}

void zEntSimpleObj_MgrCustomRender()
{
    s32 var_r30;
    u32 var_r29;
    xEnt* temp_r3;

    var_r30 = 0;
    var_r29 = 0U;

    while (var_r29 < (u32)sSimpleCustomCount)
    {
        xLightKit_Enable(sSimpleCustomList[var_r30]->lightKit, globals.currWorld);
        temp_r3 = sSimpleCustomList[var_r30];
        temp_r3->render(temp_r3);
        var_r30 += 1;
        var_r29 += 1;
    }
}

void zEntSimpleObj_Render(xEnt* ent)
{
    if (ent->model == NULL || xEntIsVisible(ent) == FALSE)
    {
        return;
    }

    xModelRender(ent->model);
}

void zEntTrackPhysics_Init(void* ent, void* asset)
{
    zEntSimpleObj_Init((zEntSimpleObj*)ent, (xEntAsset*)asset, 1);
}

void zEntSimpleObj_Init(void* ent, void* asset)
{
    zEntSimpleObj_Init((zEntSimpleObj*)ent, (xEntAsset*)asset, 0);
}

void zEntSimpleObj_Init(zEntSimpleObj* ent, xEntAsset* asset, bool physparams)
{
    U32 tmpsize;
    void* animData;
    RpAtomic* modelData;
    U32 temp_r3_4;
    U32 animBoneCount;
    xModelInstance* temp_r3;
    xAnimPlay* temp_r3_2;
    xAnimPlay* temp_r3_3;
    xAnimTable* temp_r4;
    xSimpleObjAsset* sasset;

    zEntInit((zEnt*)ent, asset, 0x53494D50U);

    if (physparams != 0)
    {
        ent->baseType = 0x3F;
    }

    // Deliberate: both arms are identical. The original picked between two asset layouts
    // that begin at the same offset, so the target emits no branch here.
    if (physparams != 0)
    {
        sasset = (xSimpleObjAsset*)(asset + 1);
    }
    else
    {
        sasset = (xSimpleObjAsset*)(asset + 1);
    }

    ent->sasset = sasset;
    ent->sflags = 0;
    ent->pflags = 0;
    ent->penby |= 0x10;
    if (ent->sasset->collType & XENT_COLLTYPE_STAT)
    {
        ent->chkby = 0x18;
    }
    else
    {
        ent->chkby = 0;
    }
    ent->move = NULL;
    ent->update = (xEntUpdateCallback)zEntSimpleObj_Update;
    ent->eventFunc = (xBaseEventCB)zEntSimpleObjEventCB;
    ent->render = zEntSimpleObj_Render;
    if ((u8)ent->linkCount != 0)
    {
        if (physparams != 0)
        {
            ent->link = (xLinkAsset*)((char*)ent->asset + 0x9C);
        }
        else
        {
            ent->link = (xLinkAsset*)((char*)ent->asset + 0x60);
        }
    }
    else
    {
        ent->link = NULL;
    }
    ent->eventFunc = (xBaseEventCB)zEntSimpleObjEventCB;
    modelData = (RpAtomic*)xSTFindAsset(asset->modelInfoID, &tmpsize);
    animData = NULL;
    if (!(ent->miscflags & 1) && (ent->asset->modelInfoID != 0U) &&
        (temp_r3 = ent->model, ((temp_r3 == NULL) == 0)) &&
        (temp_r3_2 = temp_r3->Anim, ((temp_r3_2 == NULL) == 0)) &&
        (temp_r4 = temp_r3_2->Table, ((temp_r4 == NULL) == 0)) &&
        (strcmp(&"trailer_hitch\0xEntAutoEventSimple"[0xE], temp_r4->Name) == 0))
    {
        temp_r3_3 = ent->model->Anim;
        xAnimPlaySetState(temp_r3_3->Single, temp_r3_3->Table->StateList, 0.0f);
        ent->miscflags |= 1;
    }
    else
    {
        temp_r3_4 = asset->animListID;
        if ((temp_r3_4 != 0) && (ent->atbl == NULL))
        {
            animData = xSTFindAsset(temp_r3_4, &tmpsize);
            if ((animData != NULL) &&
                ((animBoneCount = iAnimBoneCount(animData), ((animBoneCount == 0U) != 0)) ||
                 (animBoneCount != iModelNumBones(modelData))))
            {
                animData = NULL;
            }
        }
    }
    ent->anim = animData;
    ent->animTime = 0.0f;
    zEntReset((zEnt*)ent);
}

void zEntSimpleObj_Move(xEnt*, xScene*, F32, xEntFrame*)
{
}

void zEntSimpleObj_Update(zEntSimpleObj* ent, xScene* sc, float dt)
{
    void* temp_r3;
    f32 temp_f0;
    f32 duration;
    xModelInstance* temp_r3_2;
    xModelInstance* temp_r4;
    xQuat* q0;
    xVec3* t0;

    xEntUpdate((xEnt*)ent, sc, dt);
    temp_r3 = ent->anim;
    if (temp_r3 != NULL)
    {
        temp_r4 = ent->model;
        if ((temp_r4 != NULL) && !(temp_r4->Flags & 0x400))
        {
            duration = iAnimDuration(temp_r3);
            ent->animTime += dt;
            temp_f0 = ent->animTime;
            if (temp_f0 >= duration)
            {
                ent->animTime = temp_f0 - duration;
            }
            q0 = (xQuat*)giAnimScratch;
            t0 = (xVec3*)(q0 + IANIM_MAXBONES);
            iAnimEval(ent->anim, ent->animTime, 0U, t0, q0);
            temp_r3_2 = ent->model;
            iModelAnimMatrices(temp_r3_2->Data, q0, t0, temp_r3_2->Mat + 0x1);
        }
    }
}

void zEntSimpleObj_Setup(zEntSimpleObj* ent)
{
    zEntSetup((zEnt*)ent);
}

void zEntSimpleObj_Save(zEntSimpleObj* ent, xSerial* s)
{
    zEntSave((zEnt*)ent, s);
}

void zEntSimpleObj_Load(zEntSimpleObj* ent, xSerial* s)
{
    zEntLoad((zEnt*)ent, s);
}

void zEntSimpleObj_Reset(zEntSimpleObj* ent, xScene* scene)
{
    xEntBoundUpdateCallback temp_r12;

    zEntReset((zEnt*)ent);
    ent->animTime = 0.0f;
    ent->chkby &= 0xE3;
    if (ent->sasset->collType & 2)
    {
        ent->chkby |= 0x18;
    }
    temp_r12 = ent->bupdate;
    if (temp_r12 != NULL)
    {
        temp_r12(ent, (xVec3*)&ent->model->Mat->pos);
        return;
    }
    xEntDefaultBoundUpdate((xEnt*)ent, (xVec3*)&ent->model->Mat->pos);
}

s32 zEntSimpleObjEventCB(xBase* from, xBase* to, U32 toEvent, const F32* toParam, xBase* base3)
{
    zEntSimpleObj* s = (zEntSimpleObj*)to;

    switch (toEvent)
    {
    case eEventFastVisible:
    case eEventVisible:
        xEntShow((xEnt*)to);
        if ((toParam != NULL) && ((S32)(0.5f + toParam[0]) == 0x4D))
        {
            zFXPopOn((xEnt&)*to, toParam[1], toParam[2]);
        }
        break;
    case eEventFastInvisible:
    case eEventInvisible:
        xEntHide((xEnt*)to);
        if ((toParam != NULL) && ((S32)(0.5f + toParam[0]) == 0x4D))
        {
            zFXPopOff((xEnt&)*to, toParam[1], toParam[2]);
        }
        break;
    case eEventCollision_Visible_On:
        xEntShow((xEnt*)to);
        if ((toParam != NULL) && ((S32)(0.5f + toParam[0]) == 0x4D))
        {
            zFXPopOn((xEnt&)*to, toParam[1], toParam[2]);
        }
        /* fallthrough */
    case eEventCollisionOn:
        s->chkby = 0x18;
        if (s->bupdate != NULL)
        {
            s->bupdate((xEnt*)to, (xVec3*)&s->model->Mat->pos);
        }
        else
        {
            xEntDefaultBoundUpdate((xEnt*)to, (xVec3*)&s->model->Mat->pos);
        }
        break;
    case eEventCollision_Visible_Off:
        xEntHide((xEnt*)to);
        if ((toParam != NULL) && ((S32)(0.5f + toParam[0]) == 0x4D))
        {
            zFXPopOff((xEnt&)*to, toParam[1], toParam[2]);
        }
        /* fallthrough */
    case eEventCollisionOff:
        s->chkby = 0;
        if (s->bupdate != NULL)
        {
            s->bupdate((xEnt*)to, (xVec3*)&s->model->Mat->pos);
        }
        else
        {
            xEntDefaultBoundUpdate((xEnt*)to, (xVec3*)&s->model->Mat->pos);
        }
        break;
    case eEventCameraCollideOn:
        zCollGeom_CamEnable((xEnt*)to);
        break;
    case eEventCameraCollideOff:
        zCollGeom_CamDisable((xEnt*)to);
        break;
    case eEventReset:
        zEntSimpleObj_Reset((zEntSimpleObj*)to, globals.sceneCur);
        break;
    case eEventAnimPlay:
    case eEventAnimPlayLoop:
    case eEventAnimStop:
    case eEventAnimPause:
    case eEventAnimResume:
    case eEventAnimTogglePause:
    case eEventAnimPlayRandom:
    case eEventAnimPlayMaybe:
        zEntAnimEvent((zEnt*)to, toEvent, toParam);
        break;
    case eEventSetSkyDome:
        xSkyDome_AddEntity((xEnt*)to, (s32)toParam[0], (s32)toParam[1]);
        break;
    case eEventSetGoo:
        zGooAdd((xEnt*)to, toParam[0], (s32)toParam[1]);
        break;
    case eEventGooSetWarb:
        zFXGooEventSetWarb((xEnt*)to, toParam);
        break;
    case eEventGooSetFreezeDuration:
        zFXGooEventSetFreezeDuration((xEnt*)to, toParam[0]);
        break;
    case eEventGooMelt:
        zFXGooEventMelt((xEnt*)to);
        break;
    case eEventLaunchShrapnel:
    {
        zShrapnelAsset* shrap = (zShrapnelAsset*)base3;
        if (shrap != NULL && shrap->initCB != NULL)
        {
            shrap->initCB(shrap, s->model, NULL, NULL);
        }
        break;
    }
    case eEventDestroy:
        xEntHide((xEnt*)to);
        break;
    }
    return 1;
}
