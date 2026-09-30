#include "zLOD.h"

#include "xModel.h"
#include <types.h>

#include "xEnt.h"
#include "xMathInlines.h"

#include "zBase.h"
#include "zEntDestructObj.h"

static U32 sTableCount;
static zLODTable* sTableList;
static U32 sManagerIndex;
static U32 sManagerCount;
static zLODManager sManagerList[2048];

// Float memes
void AddToLODList(xModelInstance* model)
{
    for (U32 i = 0; i < sManagerCount; i++)
    {
        if (sManagerList[i].model == model)
        {
            return;
        }
    }

    for (U32 i = 0; i < sTableCount; i++)
    {
        if (sTableList[i].baseBucket != NULL &&
            (*sTableList[i].baseBucket)->OriginalData == model->Data)
        {
            xModelInstance* minst = model->Next;
            U32 numextra = 0;
            while (minst != NULL)
            {
                minst = minst->Next;
                numextra++;
            }
            if (sManagerCount < 2048)
            {
                F32 distscale = ((model->Mat->right).x * (model->Mat->right).x +
                                 (model->Mat->right).y * (model->Mat->right).y +
                                 (model->Mat->right).z * (model->Mat->right).z);
                minst = model;
                if (distscale < 0.0001f)
                {
                    distscale = 1.0f;
                }
                while (minst != NULL)
                {
                    minst->FadeEnd = xsqrt(distscale * sTableList[i].noRenderDist);
                    minst->FadeStart = minst->FadeEnd - 4.0f;
                    minst = minst->Next;
                }

                sManagerList[sManagerCount].numextra = numextra;
                sManagerList[sManagerCount].lod = &sTableList[i];
                sManagerList[sManagerCount].model = model;
                sManagerList[sManagerCount].adjustNoRenderDist =
                    (10.0f + xsqrt(sTableList[i].noRenderDist)) *
                    (10.0f + xsqrt(sTableList[i].noRenderDist));
                sManagerCount++;
                return;
            }
        }
    };
}

xEnt* AddToLODList(xEnt* ent, xScene* scene, void* v)
{
    if (!ent->model)
    {
        return ent;
    }

    AddToLODList(ent->model);

    if (ent->baseType == eBaseTypeDestructObj)
    {
        zEntDestructObj* destruct = (zEntDestructObj*)ent;
        if (destruct->hit_model != NULL)
        {
            AddToLODList(destruct->hit_model);
        }
        if (destruct->destroy_model != NULL)
        {
            AddToLODList(destruct->destroy_model);
        }
    }

    return ent;
}

// WIP
void zLOD_Setup(void)
{
    U32 tmpSize;
    U32 i;
    U32 j;
    void* data;
    U32 assetCount;
    zLODTable* tableCurr;

    sTableCount = 0;
    sTableList = NULL;
    sManagerCount = 0;

    assetCount = xSTAssetCountByType('LODT');
    if (assetCount == 0)
    {
        return;
    }

    for (i = 0; i < assetCount; i++)
    {
        data = xSTFindAssetByType('LODT', i, &tmpSize);
        sTableCount += *(S32*)data;
    }

    if (sTableCount == 0)
    {
        return;
    }

    sTableList = (zLODTable*)xMemAlloc(gActiveHeap, sTableCount * sizeof(zLODTable), 0);
    tableCurr = sTableList;

    for (i = 0; i < assetCount; i++)
    {
        data = xSTFindAssetByType('LODT', i, &tmpSize);
        memcpy(tableCurr, (S32*)data + 1, (*(S32*)data) * sizeof(zLODTable));
        tableCurr += *(S32*)data;
    }

    for (i = 0; i < sTableCount; i++)
    {
        sTableList[i].noRenderDist *= sTableList[i].noRenderDist;

        if (sTableList[i].baseBucket)
        {
            RpAtomic* model = (RpAtomic*)xSTFindAsset((U32)sTableList[i].baseBucket, NULL);
            if (model)
            {
                sTableList[i].baseBucket = xModelBucket_GetBuckets(model);
            }
            else
            {
                sTableList[i].baseBucket = NULL;
            }
        }

        for (j = 0; j < 3; j++)
        {
            if (sTableList[i].lodBucket[j])
            {
                RpAtomic* model = (RpAtomic*)xSTFindAsset((U32)sTableList[i].lodBucket[j], NULL);
                if (model)
                {
                    sTableList[i].lodBucket[j] = xModelBucket_GetBuckets(model);
                }
                else
                {
                    sTableList[i].lodBucket[j] = NULL;
                }
            }
            sTableList[i].lodDist[j] *= sTableList[i].lodDist[j];
        }
    }

    sManagerCount = 0;
    xSceneForAllEnts(globals.sceneCur, AddToLODList, 0);
}

// WIP
void zLOD_Update(U32 percent_update)
{
    xVec3* camPos = &globals.camera.mat.pos;

    if (sManagerCount == 0)
    {
        return;
    }

    U32 numUpdates = (sManagerCount * percent_update) / 100;
    if (numUpdates == 0)
    {
        numUpdates++;
    }

    while (numUpdates--)
    {
        sManagerIndex++;
        if (sManagerIndex >= sManagerCount)
            sManagerIndex = 0;

        zLODTable* lod = sManagerList[sManagerIndex].lod;
        xModelInstance* model = sManagerList[sManagerIndex].model;

        if (!lod)
        {
            continue;
        }

        F32 camdist2 = 0.0f;
        F32 distscale = model->Mat->right.x * model->Mat->right.x +
                        model->Mat->right.y * model->Mat->right.y +
                        model->Mat->right.z * model->Mat->right.z;
        if (distscale < 0.0001f)
            distscale = 1.0f;

        if (model->Mat)
        {
            camdist2 = ((camPos->x - model->Mat->pos.x) * (camPos->x - model->Mat->pos.x) +
                        (camPos->y - model->Mat->pos.y) * (camPos->y - model->Mat->pos.y) +
                        (camPos->z - model->Mat->pos.z) * (camPos->z - model->Mat->pos.z)) /
                       distscale;
        }

        if (camdist2 >= sManagerList[sManagerIndex].adjustNoRenderDist)
        {
            model->Flags |= 0x400;

            if (sManagerList[sManagerIndex].numextra)
            {
                for (xModelInstance* extra = model->Next; extra; extra = extra->Next)
                    extra->Flags |= 0x400;
            }
        }
        else
        {
            S32 lodIndex = 0;

            model->Flags &= (U16)~0x400;

            if (lod->baseBucket)
            {
                model->Bucket = lod->baseBucket;
                model->Data = (*model->Bucket)->OriginalData;
            }

            for (; lodIndex < 3 && lod->lodBucket[lodIndex] && camdist2 > lod->lodDist[lodIndex];
                 lodIndex++)
            {
                model->Bucket = lod->lodBucket[lodIndex];
                model->Data = (*model->Bucket)->OriginalData;
            }

            if (sManagerList[sManagerIndex].numextra)
            {
                if (lodIndex == 0)
                {
                    for (xModelInstance* extra = model->Next; extra; extra = extra->Next)
                        extra->Flags &= (U16)~0x400;
                }
                else
                {
                    for (xModelInstance* extra = model->Next; extra; extra = extra->Next)
                        extra->Flags |= 0x400;
                }
            }
        }
    }
}

zLODTable* zLOD_Get(xEnt* ent)
{
    if (!ent->model)
    {
        return 0;
    }

    for (S32 i = 0; i < sTableCount; i++)
    {
        if (sTableList[i].baseBucket != NULL)
        {
            if ((*sTableList[i].baseBucket)->OriginalData == ent->model->Data)
            {
                return &sTableList[i];
            }
        }
    }
    return 0;
}

// WIP
void zLOD_UseCustomTable(xEnt* ent, zLODTable* lod)
{
    U32 i;
    xModelInstance* model = ent->model;

    for (i = 0; i < sManagerCount; i++)
    {
        if (sManagerList[i].model == model)
        {
            sManagerList[i].lod = lod;
            sManagerList[i].adjustNoRenderDist =
                (10.0f + xsqrt(lod->noRenderDist)) * (10.0f + xsqrt(lod->noRenderDist));

            xVec3* camPos = &globals.camera.mat.pos;
            F32 camdist2 = 0.0f;
            F32 distscale = model->Mat->right.x * model->Mat->right.x +
                            model->Mat->right.y * model->Mat->right.y +
                            model->Mat->right.z * model->Mat->right.z;
            if (distscale < 0.0001f)
            {
                distscale = 1.0f;
            }

            if (model->Mat)
            {
                camdist2 = ((camPos->x - model->Mat->pos.x) * (camPos->x - model->Mat->pos.x) +
                            (camPos->y - model->Mat->pos.y) * (camPos->y - model->Mat->pos.y) +
                            (camPos->z - model->Mat->pos.z) * (camPos->z - model->Mat->pos.z)) /
                           distscale;
            }

            if (camdist2 >= sManagerList[i].adjustNoRenderDist)
            {
                model->Flags |= 0x400;

                if (sManagerList[sManagerIndex].numextra)
                {
                    for (xModelInstance* m = model->Next; m; m = m->Next)
                    {
                        m->Flags |= 0x400;
                    }
                }
            }
            else
            {
                S32 lodIndex = 0;

                model->Flags &= (U16)~0x400;

                if (lod->baseBucket)
                {
                    model->Bucket = lod->baseBucket;
                    model->Data = (*model->Bucket)->OriginalData;
                }

                for (;
                     lodIndex < 3 && lod->lodBucket[lodIndex] && camdist2 > lod->lodDist[lodIndex];
                     lodIndex++)
                {
                    model->Bucket = lod->lodBucket[lodIndex];
                    model->Data = (*model->Bucket)->OriginalData;
                }

                if (sManagerList[sManagerIndex].numextra)
                {
                    if (lodIndex == 0)
                    {
                        for (xModelInstance* m = model->Next; m; m = m->Next)
                        {
                            m->Flags &= (U16)~0x400;
                        }
                    }
                    else
                    {
                        for (xModelInstance* m = model->Next; m; m = m->Next)
                        {
                            m->Flags |= 0x400;
                        }
                    }
                }
            }

            return;
        }
    }
}
