#include "iAnimSKB.h"

#include <rwcore.h>

F32 iAnimDurationSKB(iAnimSKBHeader* data)
{
    return ((F32*)((iAnimSKBKey*)(data + 1) + data->KeyCount))[data->TimeCount - 1];
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
