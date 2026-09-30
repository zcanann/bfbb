#include "xCutscene.h"
#include "xSnd.h"
#include "xAnim.h"
#include "xDebug.h"
#include "xModelBucket.h"
#include "xSnd.h"

#include "iCutscene.h"
#include "iModel.h"
#include "zCamera.h"
#include "zGlobals.h"
#include "iAnim.h"
#include "xMorph.h"
#include "xShadow.h"
#include "xLightKit.h"
#include "xString.h"
#include "xstransvc.h"
#include "iParMgr.h"
#include "xModel.h"

#include <types.h>
#include <string.h>
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\cmath>

struct xCutsceneMphFrame
{
    F32 Time;
    U32 Flags;
};

struct xCutsceneMphRun
{
    S32 VertIdx;
    S32 VertCt;
};

struct JDeltaRun
{
    S32 VertIdx;
    S32 VertCt;
};

struct JDeltaTarget
{
    S32 skipSize;
    S32 numRuns;
    S32 numVerts;
    F32 scale;
    JDeltaRun runs[1];
};

extern F32 ShadowStrength;
void xShadowCameraUpdate(void* model, void (*renderCB)(void*), xVec3* center, F32 radius,
                         S32 shadowMode);

xCutscene sActiveCutscene;
U32 sCutTocCount;
xCutsceneInfo* sCutTocInfo;
extern RwGlobals* RwEngineInstance;
static xModelInstance sCutsceneFakeModel[8];

void xCutscene_Init(void* toc)
{
    S32 i;

    memset(&sActiveCutscene, 0, sizeof(xCutscene));
    sCutTocCount = 0;
    sCutTocInfo = NULL;

    if (toc != NULL)
    {
        sCutTocCount = *(U32*)toc;
        sCutTocInfo = (xCutsceneInfo*)((U32*)toc + 1);
    }

    for (i = 0; i < 8; i++)
    {
        sCutsceneFakeModel[i].Mat =
            (RwMatrix*)xMemAlloc(gActiveHeap, sizeof(RwMatrixTag) * 65, 0);
        sCutsceneFakeModel[i].Bucket =
            (xModelBucket**)xMemAlloc(gActiveHeap, sizeof(xModelBucket*) * 2, 0);
        sCutsceneFakeModel[i].Bucket[0] =
            (xModelBucket*)xMemAlloc(gActiveHeap, sizeof(xModelBucket), 0);
        sCutsceneFakeModel[i].Bucket[1] = sCutsceneFakeModel[i].Bucket[0];
        sCutsceneFakeModel[i].PipeFlags = 0x6530;
        sCutsceneFakeModel[i].RedMultiplier = 1.0f;
        sCutsceneFakeModel[i].GreenMultiplier = 1.0f;
        sCutsceneFakeModel[i].BlueMultiplier = 1.0f;
        sCutsceneFakeModel[i].Alpha = 1.0f;
    }
}

xCutscene* xCutscene_Create(U32 id)
{
    xCutscene* csn;
    U32 i;
    U32 maxload;

    xSndPauseAll(1, 1);
    csn = &sActiveCutscene;
    memset(csn, 0, sizeof(xCutscene));
    sActiveCutscene.PlaybackSpeed = 1.0f;

    xCutsceneInfo* cnfo = sCutTocInfo;

    for (i = 0; i < sCutTocCount; i++)
    {
        if (cnfo->AssetID == id)
        {
            maxload = cnfo->MaxBufEven + cnfo->MaxBufOdd;
            if (cnfo->MaxModel > maxload)
            {
                maxload = cnfo->MaxModel;
            }

            csn->RawBuf = RwMalloc(maxload + 0x3c);
            csn->AlignBuf = csn->RawBuf;

            while ((U32)csn->AlignBuf & 0x3f)
            {
                csn->AlignBuf = (void*)((U32)csn->AlignBuf + 4);
            }

            csn->Info = cnfo;
            csn->Data = (xCutsceneData*)&cnfo[1];
            csn->TimeChunkOffs = (U32*)&csn->Data[cnfo->NumData];
            csn->Visibility = csn->TimeChunkOffs + (cnfo->NumTime + 1);
            csn->BreakList = (xCutsceneBreak*)(csn->Visibility + cnfo->VisSize);
            csn->Play = (xCutsceneTime*)csn->AlignBuf;
            csn->Stream = (xCutsceneTime*)((U8*)csn->AlignBuf + cnfo->MaxBufEven);
            return csn;
        }

        cnfo = (xCutsceneInfo*)((U8*)cnfo + cnfo->HeaderSize);
    }

    return NULL;
}

S32 xCutscene_Destroy(xCutscene* csn)
{
    U32 i;

    csn->Ready = 0;
    xSndSetExternalCallback(NULL);

    if (csn->SndStarted)
    {
        xSndStop(csn->SndHandle[0]);
        if (csn->SndNumChannel == 2)
        {
            xSndStop(csn->SndHandle[1]);
        }
        xSndUpdate();
        csn->SndStarted = 0;
    }

    xSndPauseAll(0, 0);
    xSndUpdate();

    if (csn->Opened)
    {
        iCSFileClose(csn);
    }

    for (i = 0; i < csn->Info->NumData; i++)
    {
        if ((csn->Data[i].DataType & 0x80000000) && csn->Data[i].DataPtr != NULL)
        {
            if ((csn->Data[i].DataType & 0xfffffff) == XCUTSCENEDATA_TYPE_JDELTAMODEL)
            {
                RwFree(csn->Data[i].DataPtr);
            }
            else
            {
                iModelUnload((RpAtomic*)csn->Data[i].DataPtr);
            }

            csn->Data[i].DataType &= 0xfffffff;
        }
    }

    RwFree(csn->RawBuf);
    memset(csn, 0, sizeof(xCutscene));
    return 1;
}

S32 xCutscene_LoadStart(xCutscene* csn)
{
    U32 cnt;

    cnt = iCSFileOpen(csn);

    if (cnt <= 0)
    {
        return 0;
    }

    csn->Opened = 1;
    return 1;
}

F32 xCutsceneConvertBreak(F32 time, xCutsceneBreak* breaklist, U32 breakcount, S32 idx)
{
    U32 i;

    for (i = 0; i < breakcount; i++)
    {
        if (idx == breaklist[i].Index && breaklist[i].Time - time > 0.0f &&
            breaklist[i].Time - time < (1.0f / 30.0f))
        {
            return breaklist[i].Time - (1.0f / 30.0f);
        }
    }

    return time;
}

S32 xCutscene_Update(xCutscene* csn, F32 dt)
{
    if ((csn->SndStarted == FALSE) && (csn->SndNumChannel != 0))
    {
        if (csn->SndNumChannel == 1)
        {
            xSndSetPitch(csn->SndHandle[0], 0.0f);
        }
        else if (csn->SndNumChannel == 2)
        {
            xSndStartStereo(csn->SndHandle[0], csn->SndHandle[1], 0.0f);
        }
        csn->SndStarted = TRUE;
    }

    csn->Time = csn->PlaybackSpeed * dt + csn->Time;
    csn->CamTime = xCutsceneConvertBreak(csn->Time, csn->BreakList, csn->Info->BreakCount, -1);

    if (csn->Time > csn->Play->EndTime || csn->BadReadPause)
    {
        if (csn->PlayIndex == csn->Info->NumTime - 1)
        {
            csn->Time = csn->Play->EndTime;
            return 0;
        }

        if (csn->BadReadPause && csn->Waiting == FALSE)
        {
            csn->BadReadPause = FALSE;
            xCutscene_SetSpeed(csn, csn->BadReadSpeed);
        }

        if (csn->CamTime != csn->Time)
        {
            return 1;
        }

        if (csn->Waiting)
        {
            csn->Time = csn->Play->EndTime;
            csn->CamTime =
                xCutsceneConvertBreak(csn->Time, csn->BreakList, csn->Info->BreakCount, -1);

            if (csn->BadReadPause == FALSE)
            {
                csn->BadReadSpeed = csn->PlaybackSpeed;
                xCutscene_SetSpeed(csn, 0.0f);
                csn->BadReadPause = TRUE;
            }

            return 1;
        }

        xCutsceneTime* oldChunk = csn->Play;
        csn->Play = csn->Stream;
        csn->Stream = oldChunk;
        csn->PlayIndex = csn->PlayIndex + 1;

        if (csn->PlayIndex + 1 < csn->Info->NumTime)
        {
            iCSFileAsyncRead(csn, csn->Stream,
                             csn->TimeChunkOffs[csn->PlayIndex + 2] -
                                 csn->TimeChunkOffs[csn->PlayIndex + 1]);
        }
    }

    return 1;
}

void xCutscene_SetSpeed(xCutscene* csn, F32 speed)
{
    if (csn->BadReadPause)
        return;

    if (speed > 4.0f)
    {
        speed = 4.0f;
    }

    if (speed < 0.001f)
    {
        speed = 0.0f;
    }

    csn->PlaybackSpeed = speed;

    F32 semitones;
    if (speed)
    {
        semitones = xlog(speed) / 0.057762269f;
    }
    else
    {
        semitones = -99999.0f;
    }

    for (S32 i = 0; i < (S32)csn->SndNumChannel; i++)
    {
        xSndSetPitch(csn->SndHandle[i], semitones);
    }
}

F32 xlog(F32 x)
{
    return std::logf(x);
}

float std::logf(float x)
{
    return (float)log((double)x);
}

// A camera chunk's payload sits directly after its xCutsceneData header: the
// number of fly keys, then the keys themselves.
struct xCutsceneCameraData
{
    U32 NumKeys;
    zFlyKey Keys[1];
};

void xCutscene_SetCamera(xCutscene* csn, xCamera* cam)
{
    xCutsceneData* data;
    U32 i;
    U32 dataIndex;
    F32 camFOV;
    xMat4x3 camMat;
    xMat3x3 tmpMat;
    xQuat quats[2];
    xQuat qresult;
    F32 invlerp;
    F32 lerp;
    U32 count;
    S32 frame;

    data = (xCutsceneData*)&csn->Play[1];

    for (i = 0; i < csn->Play->NumData; i++)
    {
        if (data->DataType == XCUTSCENEDATA_TYPE_CAMERA)
        {
            frame = (S32)std::floorf(30.0f * csn->CamTime);
            xCutsceneCameraData* camData = (xCutsceneCameraData*)(data + 1);
            zFlyKey* keys = camData->Keys;
            dataIndex = camData->NumKeys;

            if (frame < keys[0].frame)
            {
                lerp = 0.0f;
            }
            else if (frame >= keys[dataIndex - 1].frame)
            {
                lerp = 1.0f;
                keys += dataIndex - 2;
            }
            else
            {
                lerp = 30.0f * csn->CamTime - std::floorf(30.0f * csn->CamTime);
                keys += frame - keys[0].frame;
            }

            invlerp = 1.0f - lerp;

            for (count = 0; count < 2; count++)
            {
                tmpMat.right.x = -keys[count].matrix[0];
                tmpMat.right.y = -keys[count].matrix[1];
                tmpMat.right.z = -keys[count].matrix[2];
                tmpMat.up.x = keys[count].matrix[3];
                tmpMat.up.y = keys[count].matrix[4];
                tmpMat.up.z = keys[count].matrix[5];
                tmpMat.at.x = -keys[count].matrix[6];
                tmpMat.at.y = -keys[count].matrix[7];
                tmpMat.at.z = -keys[count].matrix[8];
                xQuatFromMat(&quats[count], &tmpMat);
            }

            xQuatSlerp(&qresult, &quats[0], &quats[1], lerp);
            xQuatToMat(&qresult, &camMat);
            xVec3Lerp(&camMat.pos, (xVec3*)&keys[0].matrix[9], (xVec3*)&keys[1].matrix[9], lerp);

            camFOV =
                114.59155f *
                std::atan(12.7f * (keys[0].aperture[0] * invlerp + keys[1].aperture[0] * lerp) /
                          (keys[0].focal * invlerp + keys[1].focal * lerp));
            cam->mat = camMat;
            gCameraLastFov = 0.0f;
            xCameraSetFOV(&xglobals->camera, camFOV);
        }

        data = (xCutsceneData*)((U8*)data + 0x10 + ((data->ChunkSize + 0xf) & 0xfffffff0));
    }
}

static void xcsCalcAnimMatrices(RwMatrixTag* animMat, RpAtomic* model, xCutsceneAnimHdr* ahdr,
                                F32 time, U32 tworoot)
{
    xQuat quatresult[65];
    xVec3 tranresult[65];

    void* afile = &ahdr[1];
    iAnimEval(afile, time, 0x0, tranresult, quatresult);

    if (iModelNumBones(model) != 0)
    {
        xMat4x3Identity((xMat4x3*)animMat);
        animMat->pos.x = ahdr->Translate[0];
        animMat->pos.y = ahdr->Translate[1];
        animMat->pos.z = ahdr->Translate[2];

        if (tworoot)
        {
            xMat4x3 m1;
            xMat4x3 m2;

            quatresult[1].s = -quatresult[1].s;
            quatresult[2].s = -quatresult[2].s;
            xQuatToMat(&quatresult[1], &m1);
            xQuatToMat(&quatresult[2], &m2);

            m1.pos = tranresult[1];
            m2.pos = tranresult[2];

            xMat4x3Mul(&m1, &m2, &m1);

            tranresult[2].x = 0.0f;
            tranresult[2].y = 0.0f;
            tranresult[2].z = 0.0f;
            quatresult[2].v.x = 0.0f;
            quatresult[2].v.y = 0.0f;
            quatresult[2].v.z = 0.0f;
            quatresult[2].s = 1.0f;

            xQuatFromMat(&quatresult[1], &m1);

            quatresult[1].s = -quatresult[1].s;
            tranresult[1] = m1.pos;
        }

        U32 numbone = iModelNumBones(model);
        U32 boneidx = 0;
        xQuat* qqq = quatresult;
        xVec3* ttt = tranresult;
        while (boneidx < numbone && boneidx <= ahdr->RootIndex)
        {
            animMat->pos.x += ttt->x;
            animMat->pos.y += ttt->y;
            animMat->pos.z += ttt->z;

            ttt->x = 0.0f;
            ttt->y = 0.0f;
            ttt->z = 0.0f;

            if (FABS(qqq->s) < 0.9999f)
            {
                break;
            }

            boneidx++;
            qqq++;
            ttt++;
        }

        iModelAnimMatrices(model, quatresult, tranresult, &animMat[1]);
    }
    else
    {
        xQuatToMat(quatresult, (xMat4x3*)animMat);
        animMat->pos.x = tranresult[0].x + ahdr->Translate[0];
        animMat->pos.y = tranresult[0].y + ahdr->Translate[1];
        animMat->pos.z = tranresult[0].z + ahdr->Translate[2];
    }
}

static void JDeltaEval(RpAtomic* model, void* deltaModel, void* deltaAnim, F32 time)
{
    F32 outweight[128];
    S32 i;
    S32 numFrames;
    S32 numRun;
    S32 numWeights;
    F32 lerp;
    F32 invlerp;
    RwV3d* outverts;

    numFrames = *((S32*)deltaAnim + 1);
    numWeights = *((S32*)deltaAnim + 2);
    F32* times = (F32*)deltaAnim + 3;
    F32* weights = times + numFrames;

    if (time <= times[0] || numFrames == 1)
    {
        memcpy(outweight, weights, numWeights * sizeof(F32));
    }
    else if (time >= times[numFrames - 1])
    {
        memcpy(outweight, weights + (numFrames - 1) * numWeights, numWeights * sizeof(F32));
    }
    else
    {
        while (time > times[1])
        {
            weights += numWeights;
            times++;
        }

        lerp = (time - times[0]) / (times[1] - times[0]);
        invlerp = 1.0f - lerp;

        for (i = 0; i < numWeights; i++)
        {
            outweight[i] = invlerp * weights[i] + lerp * weights[i + numWeights];
        }
    }

    RpGeometryLock(model->geometry, 2);

    JDeltaTarget* dtgt = (JDeltaTarget*)((U8*)deltaModel + 8);
    outverts = model->geometry->morphTarget->verts;
    numRun = dtgt->numRuns;

    if (dtgt->scale)
    {
        F32 scale = dtgt->scale;
        S16* svert =
            (S16*)((U8*)deltaModel +
                   ((((U8*)&dtgt->runs[numRun] - (U8*)deltaModel) + 0xf) & 0xfffffff0));

        for (i = 0; i < numRun; i++)
        {
            S32 j = dtgt->runs[i].VertIdx;
            S32 cmpval = j + dtgt->runs[i].VertCt;

            for (; j < cmpval; j++)
            {
                outverts[j].x = scale * svert[0];
                outverts[j].y = scale * svert[1];
                outverts[j].z = scale * svert[2];
                svert += 3;
            }
        }
    }
    else
    {
        RwV3d* vert =
            (RwV3d*)((U8*)deltaModel +
                     ((((U8*)&dtgt->runs[numRun] - (U8*)deltaModel) + 0xf) & 0xfffffff0));

        for (i = 0; i < numRun; i++)
        {
            S32 j = dtgt->runs[i].VertIdx;
            S32 cmpval = dtgt->runs[i].VertIdx + dtgt->runs[i].VertCt;

            for (; j < cmpval; j++)
            {
                outverts[j] = *vert;
                vert++;
            }
        }
    }

    F32* currweight = outweight;
    dtgt = (JDeltaTarget*)((U8*)dtgt + dtgt->skipSize);

    while (numWeights)
    {
        numRun = dtgt->numRuns;

        if (*currweight)
        {
            if (dtgt->scale)
            {
                F32 scale = dtgt->scale * *currweight;
                S16* svert =
                    (S16*)((U8*)deltaModel +
                           ((((U8*)&dtgt->runs[numRun] - (U8*)deltaModel) + 0xf) & 0xfffffff0));

                for (i = 0; i < numRun; i++)
                {
                    S32 j = dtgt->runs[i].VertIdx;
                    S32 cmpval = j + dtgt->runs[i].VertCt;

                    for (; j < cmpval; j++)
                    {
                        outverts[j].x += scale * svert[0];
                        outverts[j].y += scale * svert[1];
                        outverts[j].z += scale * svert[2];
                        svert += 3;
                    }
                }
            }
            else
            {
                F32 scale = *currweight;
                RwV3d* vert =
                    (RwV3d*)((U8*)deltaModel +
                             ((((U8*)&dtgt->runs[numRun] - (U8*)deltaModel) + 0xf) & 0xfffffff0));

                for (i = 0; i < numRun; i++)
                {
                    S32 j = dtgt->runs[i].VertIdx;
                    S32 cmpval = j + dtgt->runs[i].VertCt;

                    for (; j < cmpval; j++)
                    {
                        outverts[j].x += scale * vert->x;
                        outverts[j].y += scale * vert->y;
                        outverts[j].z += scale * vert->z;
                        vert++;
                    }
                }
            }
        }

        currweight++;
        numWeights--;
        dtgt = (JDeltaTarget*)((U8*)dtgt + dtgt->skipSize);
    }

    RpGeometryUnlock(model->geometry);
}

void xVec3Lerp(xVec3* out, const xVec3* a, const xVec3* b, float alpha)
{
    out->x = a->x + (b->x - a->x) * alpha;
    out->y = a->y + (b->y - a->y) * alpha;
    out->z = a->z + (b->z - a->z) * alpha;
}

void CutsceneShadowRender(CutsceneShadowModel* smod)
{
    RpAtomic* model = smod->model;
    U32 bits = smod->shadowBits;

    while (model != NULL)
    {
        if ((bits & 1) != 0)
        {
            iModelRender(model, smod->animMat);
        }
        model = iModelFile_RWMultiAtomic(model);
        bits >>= 1;
    }
}

void xCutscene_Render(xCutscene* csn, xEnt**, S32*, F32*)
{
    U32 i;
    U32 dataIndex;
    U32 animIndex;
    U32 mphIndex;
    U32 visFlags;
    U32 visIdx;
    U32 fakeCount;
    U32 hasAlpha;
    U32 boundhack;
    U32 tworoot;
    U32 noshadow;
    xCutsceneData* data;
    RpAtomic* model;
    RpAtomic* shadowModel;
    RwMatrixTag animMat[65];
    xVec3* camVec;
    U32 tempSize;
    F32 radius;
    F32 animTime;
    F32 maxRadius;
    U32 viscnt;
    U32 subIndex;
    U32 frameMin;
    U32 frameMax;
    U32 frameIndex;
    U32 shadowBits;
    S32 matnum;
    U32 morphAnimIndex;
    U32 morphModelIndex;
    U32 numFrame;
    U32 numRun;
    U32 skipsize;
    xVec3* currtmp;
    xVec3* outv;
    U32 j;
    U32 cmpval;
    xShadowCache scache;
    static xVec3 shadVec = { 0.0f, -1.0f, 0.0f };

    fakeCount = 0;
    XCSNNosey* nosey = csn->cb_nosey;
    camVec = (xVec3*)&((RwFrame*)((RwCamera*)RwEngineInstance->curCamera)->object.object.parent)
                 ->modelling.pos;
    data = (xCutsceneData*)&csn->Play[1];
    animIndex = 0;

    for (i = 0; i < csn->Play->NumData; i++)
    {
        if (data->DataType == XCUTSCENEDATA_TYPE_ANIMATION)
        {
            animTime = xCutsceneConvertBreak(csn->CamTime, csn->BreakList, csn->Info->BreakCount,
                                             animIndex);
            model = NULL;
            boundhack = 0;
            tworoot = 0;
            noshadow = 0;

            for (dataIndex = 0; dataIndex < csn->Info->NumData; dataIndex++)
            {
                if (((csn->Data[dataIndex].DataType & 0xfffffff) == XCUTSCENEDATA_TYPE_RW_MODEL) &&
                    (data->AssetID == csn->Data[dataIndex].AssetID))
                {
                    model = (RpAtomic*)csn->Data[dataIndex].DataPtr;
                    boundhack = csn->Data[dataIndex].DataType & 0x40000000;
                    tworoot = csn->Data[dataIndex].DataType & 0x20000000;
                    noshadow = csn->Data[dataIndex].DataType & 0x10000000;
                    break;
                }
            }

            if (model == NULL)
            {
                model = (RpAtomic*)xSTFindAsset(data->AssetID, &tempSize);
            }

            if (model != NULL)
            {
                maxRadius = model->boundingSphere.radius;
                shadowModel = model;
                RpAtomic* tmpModel = iModelFile_RWMultiAtomic(model);

                while (tmpModel != NULL)
                {
                    maxRadius = (maxRadius > tmpModel->boundingSphere.radius)
                                    ? maxRadius
                                    : tmpModel->boundingSphere.radius;
                    tmpModel = iModelFile_RWMultiAtomic(tmpModel);
                }

                visFlags = 0xffffffff;

                if (csn->Info->VisCount)
                {
                    viscnt = csn->Info->VisCount;
                    U32* currvis = csn->Visibility;
                    frameIndex = (U32)(30.0f * animTime);

                    while (viscnt)
                    {
                        subIndex = currvis[0] >> 16;
                        frameMin = currvis[1] & 0xffff;
                        frameMax = currvis[1] >> 16;

                        if ((currvis[0] & 0xffff) == animIndex)
                        {
                            j = (frameIndex > frameMax - frameMin) ? frameMax - frameMin
                                                                   : frameIndex;

                            if (!((currvis[2 + (j >> 5)] >> (j & 31)) & 1))
                            {
                                visFlags &= ~(1 << subIndex);
                            }
                        }

                        currvis += ((frameMax - frameMin) >> 5) + 3;
                        viscnt--;
                    }
                }

                xcsCalcAnimMatrices(animMat, model, (xCutsceneAnimHdr*)&data[1],
                                    animTime - csn->Play->StartTime, tworoot);

                if (nosey != NULL && (nosey->flg_nosey & 2))
                {
                    nosey->UpdatedAnimated(model, animMat, animIndex, i);
                }

                shadowBits = 0;
                visIdx = 0;
                radius = 1.5f * maxRadius;

                while (model != NULL)
                {
                    if (visFlags & (1 << visIdx))
                    {
                        RpGeometry* geom = model->geometry;
                        hasAlpha = 0;
                        RwTexture* tex = geom->matList.materials[0]->texture;

                        if (tex != NULL && (xStricmp(tex->name, "tv_close") == 0 ||
                                            (xStricmp(tex->name, "robot_2a_tar-tar") == 0 &&
                                             geom->numTriangles == 0x35)))
                        {
                            hasAlpha = 1;
                        }
                        else
                        {
                            for (matnum = 0; matnum < geom->matList.numMaterials; matnum++)
                            {
                                if (geom->matList.materials[matnum]->color.alpha != 0xff)
                                {
                                    hasAlpha = 1;
                                    break;
                                }
                            }
                        }

                        xCutsceneData* mphdata = (xCutsceneData*)&csn->Play[1];

                        for (mphIndex = 0; mphIndex < csn->Play->NumData; mphIndex++)
                        {
                            morphAnimIndex = mphdata->AssetID & 0xffff;
                            morphModelIndex = mphdata->AssetID >> 16;

                            if ((mphdata->DataType == XCUTSCENEDATA_TYPE_MORPHTARGET ||
                                 mphdata->DataType == XCUTSCENEDATA_TYPE_JDELTAANIM) &&
                                morphAnimIndex == animIndex && morphModelIndex == visIdx)
                            {
                                if (mphdata->DataType == XCUTSCENEDATA_TYPE_MORPHTARGET)
                                {
                                    numFrame = ((U32*)&mphdata[1])[0];
                                    numRun = ((U32*)&mphdata[1])[1];
                                    xCutsceneMphFrame* mphFrame = (xCutsceneMphFrame*)((U32*)&mphdata[1] + 2);
                                    xCutsceneMphRun* mphRun = (xCutsceneMphRun*)&mphFrame[numFrame];
                                    xMorphTargetFile* mphFile = (xMorphTargetFile*)((U8*)mphdata +
                                                                  ((numFrame * 2 + numRun * 2 + 5) *
                                                                   4 & 0xfffffff0) +
                                                                  0x10);

                                    S16* v_array[4] = { NULL, NULL, NULL, NULL };
                                    S16 weight[4] = { 0, 0, 0, 0 };

                                    for (frameIndex = 1; frameIndex < numFrame - 1; frameIndex++)
                                    {
                                        if (mphFrame[frameIndex].Time >= animTime)
                                        {
                                            break;
                                        }
                                    }

                                    skipsize = (mphFile->NumVerts * 3 + 7) & 0xfffffff8;
                                    if (mphFile->Flags & 1)
                                    {
                                        skipsize *= 2;
                                    }

                                    v_array[0] =
                                        (S16*)((U8*)mphFile +
                                               skipsize * (frameIndex - 1) * 2 + 0x20);
                                    v_array[1] =
                                        (S16*)((U8*)mphFile + skipsize * frameIndex * 2 + 0x20);

                                    weight[1] =
                                        (S16)(16384.0f *
                                              (animTime - mphFrame[frameIndex - 1].Time) /
                                              (mphFrame[frameIndex].Time -
                                               mphFrame[frameIndex - 1].Time));
                                    if (weight[1] > 0x4000)
                                    {
                                        weight[1] = 0x4000;
                                    }
                                    weight[0] = 0x4000 - weight[1];

                                    xVec3* csnTmpArray = (xVec3*)&gRenderArr;
                                    FastS16weight2((F32*)csnTmpArray, v_array, weight,
                                                   mphFile->NumVerts * 3,
                                                   0.000061035156f * mphFile->Scale);

                                    RpGeometryLock(model->geometry, 2);

                                    currtmp = csnTmpArray;
                                    outv = (xVec3*)model->geometry->morphTarget->verts;

                                    for (dataIndex = 0; dataIndex < numRun; dataIndex++)
                                    {
                                        j = mphRun[dataIndex].VertIdx;
                                        cmpval =
                                            mphRun[dataIndex].VertIdx + mphRun[dataIndex].VertCt;

                                        for (; j < cmpval; j++)
                                        {
                                            outv[j] = *currtmp;
                                            currtmp++;
                                        }
                                    }

                                    RpGeometryUnlock(model->geometry);
                                }
                                else
                                {
                                    void* deltaAnim = (void*)((U8*)mphdata + 0x10);
                                    void* deltaModel = NULL;

                                    for (dataIndex = 0; dataIndex < csn->Info->NumData; dataIndex++)
                                    {
                                        if (((csn->Data[dataIndex].DataType & 0xfffffff) ==
                                             XCUTSCENEDATA_TYPE_JDELTAMODEL) &&
                                            (data->AssetID == csn->Data[dataIndex].AssetID))
                                        {
                                            deltaModel = csn->Data[dataIndex].DataPtr;
                                            break;
                                        }
                                    }

                                    JDeltaEval(model, deltaModel, deltaAnim, animTime);
                                }

                                // Retail emits this block twice: once here on the
                                // morph/JDelta path and once after the search loop.
                                // Reproduced faithfully.
                                if (hasAlpha)
                                {
                                    if (fakeCount < 8)
                                    {
                                        sCutsceneFakeModel[fakeCount].Data = model;
                                        sCutsceneFakeModel[fakeCount].BoneCount =
                                            iModelNumBones(model);
                                        memcpy(sCutsceneFakeModel[fakeCount].Mat, animMat,
                                               sizeof(RwMatrixTag) * 65);
                                        sCutsceneFakeModel[fakeCount].Flags = 1;
                                        sCutsceneFakeModel[fakeCount].FadeStart = 100.0f;
                                        sCutsceneFakeModel[fakeCount].FadeEnd = 100.0f;
                                        sCutsceneFakeModel[fakeCount].Bucket[0]->Data = model;
                                        sCutsceneFakeModel[fakeCount].Alpha =
                                            model->geometry->matList.materials[0]->color.alpha /
                                            255.0f;
                                        xModelBucket_Add(&sCutsceneFakeModel[fakeCount]);
                                        fakeCount++;
                                    }
                                }
                                else
                                {
                                    iModelCull(model, animMat);
                                    iModelRender(model, animMat);
                                    model->worldBoundingSphere.radius = radius;
                                    if (iModelNumBones(model) >= 5)
                                    {
                                        shadowBits |= 1 << visIdx;
                                    }
                                }

                                goto nextmodel;
                            }

                            mphdata = (xCutsceneData*)((U8*)mphdata + 0x10 +
                                                       ((mphdata->ChunkSize + 0xf) & 0xfffffff0));
                        }

                        if (hasAlpha)
                        {
                            if (fakeCount < 8)
                            {
                                sCutsceneFakeModel[fakeCount].Data = model;
                                sCutsceneFakeModel[fakeCount].BoneCount = iModelNumBones(model);
                                memcpy(sCutsceneFakeModel[fakeCount].Mat, animMat,
                                       sizeof(RwMatrixTag) * 65);
                                sCutsceneFakeModel[fakeCount].Flags = 1;
                                sCutsceneFakeModel[fakeCount].FadeStart = 100.0f;
                                sCutsceneFakeModel[fakeCount].FadeEnd = 100.0f;
                                sCutsceneFakeModel[fakeCount].Bucket[0]->Data = model;
                                sCutsceneFakeModel[fakeCount].Alpha =
                                    model->geometry->matList.materials[0]->color.alpha / 255.0f;
                                xModelBucket_Add(&sCutsceneFakeModel[fakeCount]);
                                fakeCount++;
                            }
                        }
                        else
                        {
                            iModelCull(model, animMat);
                            iModelRender(model, animMat);
                            model->worldBoundingSphere.radius = radius;
                            if (iModelNumBones(model) >= 5)
                            {
                                shadowBits |= 1 << visIdx;
                            }
                        }

                    nextmodel:;
                    }

                    visIdx++;
                    model = iModelFile_RWMultiAtomic(model);
                }

                if (shadowBits && xVec3Dist(camVec, (xVec3*)&animMat[0].pos) < 25.0f &&
                    !boundhack && !noshadow)
                {
                    CutsceneShadowModel smod;
                    xVec3 center = *(xVec3*)&animMat[0].pos;
                    maxRadius = 2.4f * maxRadius;
                    smod.model = shadowModel;
                    smod.animMat = animMat;
                    smod.shadowBits = shadowBits;
                    xLightKit* lkit = gLastLightKit;
                    xLightKit_Enable(NULL, globals.currWorld);
                    xShadowSetLight(&center, &shadVec, 1.0f);
                    xShadowVertical_FillCache(&scache, &center, maxRadius, 6.0f, 0.0871557f);
                    xShadowCameraUpdate(&smod, (void (*)(void*))CutsceneShadowRender, &center,
                                        maxRadius, 0);
                    xShadowVertical_DrawCache(&scache, ShadowStrength, 0.0f, 0, NULL, NULL);
                    xLightKit_Enable(lkit, globals.currWorld);
                }
            }

            animIndex++;
        }

        data = (xCutsceneData*)((U8*)data + 0x10 + ((data->ChunkSize + 0xf) & 0xfffffff0));
    }

    if (nosey != NULL && (nosey->flg_nosey & 1))
    {
        nosey->CanRenderNow();
    }
}

xCutscene* xCutscene_CurrentCutscene()
{
    return &sActiveCutscene;
}

namespace std
{
    float atanf(float x);
}

float std::atan(float x)
{
    return std::atanf(x);
}

float std::atanf(float x)
{
    return (float)::atan((double)x);
}
