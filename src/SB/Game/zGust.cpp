#include "xstransvc.h"
#include "xMath.h"
#include "xMath3.h"
#include "xEvent.h"
#include "xString.h"
#include "xVec3.h"
#include "xVec3Inlines.h"
#if defined(PS2)
#include "xScene.h"
#endif

#include "zGust.h"
#include "zParEmitter.h"
#include "zScene.h"

#include <types.h>

static zGust* gusts;
static U16 ngusts;
static zParEmitter* sGustDustEmitter;
static zParEmitter* sGustDebrisEmitter;

S32 zGustEventCB(xBase* from, xBase* to, U32 toEvent, const float* toParam, xBase* b);

static void zGustInit(zGust* g, zGustAsset* a)
{
    xBaseInit(g, a);
    g->eventFunc = zGustEventCB;
    g->asset = a;
    g->flags = a->flags;

    if (g->linkCount)
    {
        g->link = (xLinkAsset*)&g->asset[1];
    }
    else
    {
        g->link = NULL;
    }

    g->debris_timer = 0.15f;
}

static void zGustSetup(zGust* g)
{
    g->volume = (zVolume*)zSceneFindObject(g->asset->volumeID);
    g->fx_volume = (zVolume*)zSceneFindObject(g->asset->effectID);
}

void zGustInit()
{
    ngusts = xSTAssetCountByType('GUST');

    U32 n = ngusts;

    if (n)
    {
        gusts = (zGust*)xMemAllocSize(sizeof(zGust) * n);
        for (U16 i = 0; i < ngusts; i++)
        {
            U32 size;
            zGustAsset* asset = (zGustAsset*)xSTFindAssetByType('GUST', i, &size);
            zGustInit(&gusts[i], asset);
        }
    }
    else
    {
        gusts = NULL;
    }
}

void zGustSetup()
{
    if (gusts)
    {
        for (U16 i = 0; i < ngusts; i++)
        {
            zGustSetup(&gusts[i]);
        }
        sGustDustEmitter = zParEmitterFind(xStrHash("PAREMIT_GUST_DUST"));
        sGustDebrisEmitter = zParEmitterFind(xStrHash("PAREMIT_GUST_DEBRIS"));
    }
}

void zGustTurnOn(zGust* g)

{
    g->flags |= 1;
    g->debris_timer = 0.15f;
}

void zGustTurnOff(zGust* g)
{
    g->flags &= ~1;
}

void zGustToggleOn(zGust* g)
{
    g->flags ^= 1;
    g->debris_timer = 0.15f;
}

zGust* zGustGetGust(U16 n)
{
    if (gusts)
    {
        return &gusts[n];
    }

    return NULL;
}

void zGustUpdateEnt(xEnt* ent, xScene* sc, float dt, void* gdata)
{
    U32 i;
    U32 j;
    U32 minidx;
    F32 minlerp;
    zGustData* data;
    xCollis coll;
    F32 lerpinc;
    xVec3* gvel;
    xVec3 dpos;

    if (!gusts)
        return;

    data = (zGustData*)gdata;

    coll.flags = 0;
    for (i = 0; i < ngusts; i++)
    {
        if (gusts[i].flags & 1)
        {
            xBoundHitsBound(&ent->bound, &gusts[i].volume->asset->bound, &coll);
            if (coll.flags & 1)
            {
                minlerp = 2.0f;
                for (j = 0; j < 4; j++)
                {
                    if (data->lerp[j] < minlerp)
                    {
                        minidx = j;
                    }
                    if (data->g[j] == &gusts[i])
                    {
                        break;
                    }
                }

                if (j == 4)
                {
                    data->g[minidx] = &gusts[i];
                    data->lerp[minidx] = 1.0E-7;
                }
            }
            else
            {
                for (j = 0; j < 4; j++)
                {
                    if (data->g[j] == &gusts[i] && data->lerp[j] == 1.0f)
                    {
                        data->lerp[j] = -1.0f;
                        break;
                    }
                }
            }
        }
    }

    for (i = 0; i < 4; i++)
    {
        if (data->g[i])
        {
            if (data->g[i]->flags & 1)
            {
                lerpinc = data->g[i]->asset->fade;
                if (dt >= lerpinc)
                {
                    if (!(data->lerp[i] < 0.0f))
                    {
                        data->lerp[i] = 1.0f;
                        continue;
                    }
                }
                else
                {
                    lerpinc = dt / lerpinc;
                    if (data->lerp[i] >= 0.0f)
                    {
                        data->lerp[i] += lerpinc;
                        if (data->lerp[i] >= 1.0f)
                        {
                            data->lerp[i] = 1.0f;
                        }
                        continue;
                    }
                    else
                    {
                        data->lerp[i] += lerpinc;
                        if (!(data->lerp[i] >= 0.0f))
                        {
                            continue;
                        }
                    }
                }
            }

            data->g[i] = NULL;
            data->lerp[i] = 0.0f;
        }
    }

    data->gust_on = 0;

    for (i = 0; i < 4; i++)
    {
        if (data->g[i])
        {
            data->gust_on = 1;

            gvel = &data->g[i]->asset->vel;
            xVec3SMul(&dpos, gvel, dt * xabs(data->lerp[i]));
            xVec3AddTo(&ent->frame->mat.pos, &dpos);
        }
    }
}

void zGustSave(zGust* ent, xSerial* s)
{
    xBaseSave(ent, s);
}

void zGustLoad(zGust* ent, xSerial* s)
{
    xBaseLoad(ent, s);
}

void zGustReset(zGust* g)
{
    xBaseReset(g, g->asset);
    g->flags = g->asset->flags;
}

S32 zGustEventCB(xBase* from, xBase* to, U32 toEvent, const float* toParam, xBase* b)
{
    switch (toEvent)
    {
    case eEventOn:
        zGustTurnOn((zGust*)to);
        break;

    case eEventOff:
        zGustTurnOff((zGust*)to);
        break;

    case eEventToggle:
        zGustToggleOn((zGust*)to);
        break;

    case eEventReset:
        zGustReset((zGust*)to);
        break;
    }

    return 1;
}

static void UpdateGustFX(zGust* g, float seconds)
{
    xBBox* box;
    xParEmitterCustomSettings info;
    zParEmitter* e;
    S32 total_debris;
    S32 vol_area;
    S32 i;

    if (g->asset->partMod <= 0.0f)
        return;

    g->debris_timer -= seconds;

    if (!(g->debris_timer <= 0.0f))
        return;

    box = &(!g->asset->effectID ? g->volume : g->fx_volume)->asset->bound.box;

    vol_area = ((box->box.upper.x - box->box.lower.x) * (box->box.upper.z - box->box.lower.z));

    if (vol_area > 1000)
    {
        g->debris_timer = 1000000.0f;
        return;
    }

    total_debris = vol_area >> 5;
    if (total_debris > 5)
        total_debris = 5;
    else if (total_debris < 1)
        total_debris = 1;

    g->debris_timer = xurand() * 0.15f + 0.15f;

    if (g->asset->flags & 2)
    {
        e = sGustDustEmitter;
    }
    else
    {
        e = sGustDebrisEmitter;
    }

    if (!e)
        return;

    info.custom_flags =
        eParEmitterCustomVel | eParEmitterCustomPos | eParEmitterCustomLife;

    for (i = 0; i < total_debris; i++)
    {
        if (g->asset->effectID == 0)
        {
            box = &g->volume->asset->bound.box;

            info.pos = box->box.lower;
            info.pos.x = (box->box.upper.x - box->box.lower.x) * xurand() + info.pos.x;
            info.pos.z = (box->box.upper.z - box->box.lower.z) * xurand() + info.pos.z;
            info.vel.x = 0.0f;
            info.vel.y = 5.0f;
            info.vel.z = 0.0f;
            info.life.val[0] = (box->box.upper.y - box->box.lower.y) / 5.0f;
            info.life.val[0] *= 2.0f;
        }
        else
        {
            box = &g->fx_volume->asset->bound.box;

            info.pos.x = (box->box.upper.x - box->box.lower.x) * xurand() + box->box.lower.x;
            info.pos.y = (box->box.upper.y - box->box.lower.y) * xurand() + box->box.lower.y;
            info.pos.z = (box->box.upper.z - box->box.lower.z) * xurand() + box->box.lower.z;
            info.vel.x = 1.5f * g->asset->vel.x;
            info.vel.y = 1.5f * g->asset->vel.y;
            info.vel.z = 1.5f * g->asset->vel.z;
            info.life.val[0] = 7.5f * g->asset->fade;
        }

        info.life.val[0] *= g->asset->partMod;
        xParEmitterEmitCustom(e, 1.0f / 30.0f, &info);
    }
}

void zGustUpdateFX(F32 seconds)
{
    for (S32 i = 0; i < ngusts; i++)
    {
        zGust* curr = &gusts[i];
        if (curr->flags & 1)
        {
            UpdateGustFX(curr, seconds);
        }
    }
}
