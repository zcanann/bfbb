#include "iAnimSKB.h"

#include <rwcore.h>
#include <rtslerp.h>

#include <limits.h>

void iAnimEvalSKB(iAnimSKBHeader* data, F32 time, U32 flags, xVec3* tran, xQuat* quat)
{
    U32 i, tidx, bcount, tcount;
    iAnimSKBKey* keys;
    F32* times;
    U16* offsets;
    S32 asdf; // unused
    F32 scalex, scaley, scalez;

    tcount = data->TimeCount;
    bcount = data->BoneCount;

    keys = (iAnimSKBKey*)(data + 1);
    times = (F32*)(keys + data->KeyCount);
    offsets = (U16*)(times + tcount);

    if (time < 0.0f)
    {
        time = 0.0f;
    }

    if (time > times[tcount - 1])
    {
        time = times[tcount - 1];
    }

    tidx = (tcount - 1) % 4;

    while (times[tidx] < time)
    {
        tidx += 4;
    }

    while (tidx && time <= times[tidx])
    {
        tidx--;
    }

    offsets += tidx * bcount;

    if (flags & 0x1)
    {
        bcount = 1;
    }

    if (flags & 0x2)
    {
        bcount--;
        offsets++;
    }

    if (tcount == 1)
    {
        scalex = data->Scale[0];
        scaley = data->Scale[1];
        scalez = data->Scale[2];

        for (i = 0; i < bcount; i++, quat++, tran++)
        {
            iAnimSKBKey* k = &keys[i * 2];

            quat->v.x = k->Quat[0] * (1.0f / SHRT_MAX);
            quat->v.y = k->Quat[1] * (1.0f / SHRT_MAX);
            quat->v.z = k->Quat[2] * (1.0f / SHRT_MAX);
            quat->s = k->Quat[3] * (1.0f / SHRT_MAX);

            tran->x = scalex * k->Tran[0];
            tran->y = scaley * k->Tran[1];
            tran->z = scalez * k->Tran[2];
        }
    }
    else
    {
        scalex = data->Scale[0];
        scaley = data->Scale[1];
        scalez = data->Scale[2];

        for (i = 0; i < bcount; quat++, tran++, i++)
        {
            RtQuatSlerpCache qcache;
            RtQuat q1, q2;
            RwReal time1, time2, lerp;
            F32 x0, y0, z0, x1, y1, z1;
            iAnimSKBKey* k = &keys[*offsets];
            U32 costheta, theta; // unused

            offsets++;

            time1 = time - times[k->TimeIndex];
            time2 = times[k[1].TimeIndex] - times[k[0].TimeIndex];
            lerp = time1 / time2;

            q1.imag.x = k[0].Quat[0] * (1.0f / SHRT_MAX);
            q1.imag.y = k[0].Quat[1] * (1.0f / SHRT_MAX);
            q1.imag.z = k[0].Quat[2] * (1.0f / SHRT_MAX);
            q1.real = k[0].Quat[3] * (1.0f / SHRT_MAX);

            q2.imag.x = k[1].Quat[0] * (1.0f / SHRT_MAX);
            q2.imag.y = k[1].Quat[1] * (1.0f / SHRT_MAX);
            q2.imag.z = k[1].Quat[2] * (1.0f / SHRT_MAX);
            q2.real = k[1].Quat[3] * (1.0f / SHRT_MAX);

            RtQuatSetupSlerpCache(&q1, &q2, &qcache);
            RtQuatSlerp((RtQuat*)quat, &q1, &q2, lerp, &qcache);

            // The two endpoints are scaled into their own values before the lerp: retail
            // rounds scale*Tran to F32 here, so this must not fold into a fused fmsubs.
            x0 = scalex * k[0].Tran[0];
            y0 = scaley * k[0].Tran[1];
            z0 = scalez * k[0].Tran[2];
            x1 = scalex * k[1].Tran[0];
            y1 = scaley * k[1].Tran[1];
            z1 = scalez * k[1].Tran[2];

            tran->x = lerp * (x1 - x0) + x0;
            tran->y = lerp * (y1 - y0) + y0;
            tran->z = lerp * (z1 - z0) + z0;
        }
    }
}

F32 iAnimDurationSKB(iAnimSKBHeader* data)
{
    return ((F32*)((iAnimSKBKey*)(data + 1) + data->KeyCount))[data->TimeCount - 1];
}

// The retail object carries out-of-line copies of std::fabsf and of math.h's
// inline fabs right here, between iAnimDurationSKB and _iAnimSKBAdjustTranslate,
// and this is the only definition of fabsf__3stdFf in the whole DOL (zNPCTypeBossPlankton
// calls it). SB is built with -inline off, so a plain `inline` definition is dropped when
// nothing in this file calls it -- the two FABS() sites below expand to the __fabs
// intrinsic instead. __declspec(weak) reproduces the retail linkage and placement.
namespace std
{
    __declspec(weak) float fabsf(float x)
    {
        return (float)fabs(x);
    }
}

void _iAnimSKBAdjustTranslate(iAnimSKBHeader* data, U32 bone, F32* starttran, F32* endtran)
{
    S32 ipos;
    U32 i, idx, keyfirst, keylast, kcount, bcount, tcount;
    F32 outScale[3];
    F32 pos;
    F32 factor[3];
    F32 oldmax[3] = { 0.0f, 0.0f, 0.0f };
    F32 newmax[3] = { 0.0f, 0.0f, 0.0f };
    F32 timefirst, timelast;
    iAnimSKBKey* keys;
    F32* times;
    U16* offsets;

    tcount = data->TimeCount;
    bcount = data->BoneCount;
    kcount = data->KeyCount;

    keys = (iAnimSKBKey*)(data + 1);
    times = (F32*)(keys + kcount);
    offsets = (U16*)(times + tcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + bcount * (tcount - 2)] + 1;

    timefirst = times[0];
    timelast = times[tcount - 1];

    for (i = 0; i < kcount; i++)
    {
        for (idx = 0; idx < 3; idx++)
        {
            if (starttran[idx] || endtran[idx])
            {
                pos = data->Scale[idx] * keys[i].Tran[idx];

                if (FABS(pos) > oldmax[idx])
                {
                    oldmax[idx] = FABS(pos);
                }

                if (i >= keyfirst && i <= keylast)
                {
                    pos += starttran[idx] +
                           (endtran[idx] - starttran[idx]) *
                               (times[keys[i].TimeIndex] - timefirst) / (timelast - timefirst);

                    if (FABS(pos) > newmax[idx])
                    {
                        newmax[idx] = FABS(pos);
                    }
                }
            }
        }
    }

    for (idx = 0; idx < 3; idx++)
    {
        if (starttran[idx] || endtran[idx])
        {
            if (newmax[idx] > oldmax[idx])
            {
                outScale[idx] = data->Scale[idx] * newmax[idx] / oldmax[idx];
            }
            else
            {
                outScale[idx] = data->Scale[idx];
            }

            factor[idx] = 1.0f / outScale[idx];
        }
    }

    for (i = 0; i < kcount; i++)
    {
        for (idx = 0; idx < 3; idx++)
        {
            if (starttran[idx] || endtran[idx])
            {
                if (i >= keyfirst && i <= keylast)
                {
                    pos = starttran[idx] +
                          (endtran[idx] - starttran[idx]) *
                              (times[keys[i].TimeIndex] - timefirst) / (timelast - timefirst);

                    ipos = factor[idx] * (data->Scale[idx] * keys[i].Tran[idx] + pos);

                    if (ipos < -32767)
                    {
                        ipos = -32767;
                    }
                    else if (ipos > 32767)
                    {
                        ipos = 32767;
                    }

                    keys[i].Tran[idx] = ipos;
                }
                else if (data->Scale[idx] != outScale[idx])
                {
                    pos = data->Scale[idx] * keys[i].Tran[idx];

                    ipos = pos * factor[idx];

                    if (ipos < -32767)
                    {
                        ipos = -32767;
                    }
                    else if (ipos > 32767)
                    {
                        ipos = 32767;
                    }

                    keys[i].Tran[idx] = ipos;
                }
            }
        }
    }

    for (idx = 0; idx < 3; idx++)
    {
        if (starttran[idx] || endtran[idx])
        {
            data->Scale[idx] = outScale[idx];
        }
    }
}

// Retail bug preserved: maxTran is never read. The caller in zEntPlayer.cpp passes the
// capacity of tranList (128), but nothing here bounds the number of xVec3s written.
S32 _iAnimSKBExtractTranslate(iAnimSKBHeader* data, U32 bone, xVec3* tranArray, S32 maxTran)
{
    U32 i, j, keylast, tcount;
    iAnimSKBKey* keys;
    F32* times;
    U16* offsets;
    xVec3* lastTran;
    S32 tranFound;
    S32 lastTime, currTime;
    F32 lerp;
    F32 tranx, trany, tranz;

    tranFound = 0;
    lastTime = -1;

    tcount = data->TimeCount;

    keys = (iAnimSKBKey*)(data + 1);
    times = (F32*)(keys + data->KeyCount);
    offsets = (U16*)(times + tcount);

    keylast = offsets[bone + (tcount - 2) * data->BoneCount] + 1;

    for (i = offsets[bone]; i <= keylast; i++)
    {
        currTime = (S32)(30.0f * times[keys[i].TimeIndex]);

        tranx = data->Scale[0] * keys[i].Tran[0];
        trany = data->Scale[1] * keys[i].Tran[1];
        tranz = data->Scale[2] * keys[i].Tran[2];

        if (lastTime >= 0 && currTime > lastTime + 1)
        {
            lastTran = tranArray - 1;

            for (j = 1; j < currTime - lastTime; j++)
            {
                lerp = j / (F32)(currTime - lastTime);

                tranArray->x = lastTran->x + lerp * (tranx - lastTran->x);
                tranArray->y = lastTran->y + lerp * (trany - lastTran->y);
                tranArray->z = lastTran->z + lerp * (tranz - lastTran->z);

                tranArray++;
                tranFound++;
            }
        }

        if (lastTime != currTime)
        {
            tranArray->x = tranx;
            tranArray->y = trany;
            tranArray->z = tranz;

            tranArray++;
            tranFound++;
        }

        lastTime = currTime;

        keys[i].Tran[0] = 0;
        keys[i].Tran[1] = 0;
        keys[i].Tran[2] = 0;
    }

    return tranFound;
}
