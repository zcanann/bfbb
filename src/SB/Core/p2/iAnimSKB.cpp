#include "iAnimSKB.h"

#include <rwcore.h>

// The evaluator keeps decoded keys and slerp intermediates in VU0 registers.
// vf20 holds translation scales and 1/32767; vf22-vf31 hold the slerp table.
extern F32 slerpPolynomial[24];

static inline void prepare_skb_slerp()
{
    asm volatile("lqc2 vf1, 0x0(%0)\n"
                 "lqc2 vf2, 0x10(%0)\n"
                 "lqc2 vf22, 0x20(%0)\n"
                 "lqc2 vf23, 0x30(%0)\n"
                 "lqc2 vf24, 0x40(%0)\n"
                 "lqc2 vf25, 0x50(%0)\n"
                 "vaddx.xyz vf26, vf0, vf1x\n"
                 "vaddy.xyz vf27, vf0, vf1y\n"
                 "vaddz.xyz vf28, vf0, vf1z\n"
                 "vaddw.xyz vf29, vf0, vf1w\n"
                 "vaddx.xyz vf30, vf0, vf2x\n"
                 "vaddy.xyz vf31, vf0, vf2y\n"
                 : : "r"(slerpPolynomial) : "memory");
}

static inline void prepare_skb_scale(const F32* scale)
{
    // Pack the three scales and the quaternion divisor into one VU register.
    // The retail packing kernel uses a0-a2 as scratch registers.
    asm volatile("lw a0, 0x0(%0)\n"
                 "lw a1, 0x4(%0)\n"
                 "lw a2, 0x8(%0)\n"
                 "pextlw a0, a1, a0\n"
                 "lui a1, 0x3800\n"
                 "ori a1, a1, 0x100\n"
                 "pextlw a2, a1, a2\n"
                 "pcpyld a0, a2, a0\n"
                 "qmtc2 a0, vf20\n"
                 : : "r"(scale) : "a0", "a1", "a2", "memory");
}

static inline U32 unpack_skb_pair(const S16* packed, F32 lerp)
{
    // Each 16-byte key has four signed quaternion components followed by
    // three signed translation components. vf14 keeps t and 1-t; vf15 is
    // the quaternion dot product, and vf18 receives the interpolated position.
    U32 costheta, scratch;
    asm volatile("ldr %0, 0x0(%2)\n"
                 "ldl %0, 0x7(%2)\n"
                 "ldr %1, 0x10(%2)\n"
                 "ldl %1, 0x17(%2)\n"
                 "pextlh %0, %0, zero\n"
                 "psraw %0, %0, 16\n"
                 "qmtc2 %0, vf16\n"
                 "vitof0.xyzw vf16, vf16\n"
                 "pextlh %1, %1, zero\n"
                 "psraw %1, %1, 16\n"
                 "qmtc2 %1, vf17\n"
                 "vitof0.xyzw vf17, vf17\n"
                 "vmulw.xyzw vf16, vf16, vf20w\n"
                 "ldr %0, 0x8(%2)\n"
                 "ldl %0, 0xf(%2)\n"
                 "vmulw.xyzw vf17, vf17, vf20w\n"
                 "ldr %1, 0x18(%2)\n"
                 "ldl %1, 0x1f(%2)\n"
                 "pextlh %1, %1, zero\n"
                 "vmul.xyzw vf15, vf16, vf17\n"
                 "psraw %1, %1, 16\n"
                 "qmtc2 %1, vf19\n"
                 "vitof0.xyz vf19, vf19\n"
                 "vaddz.x vf15, vf15, vf15z\n"
                 "vaddw.y vf15, vf15, vf15w\n"
                 "pextlh %0, %0, zero\n"
                 "vmul.xyz vf19, vf19, vf20\n"
                 "psraw %0, %0, 16\n"
                 "qmtc2 %0, vf18\n"
                 "vitof0.xyz vf18, vf18\n"
                 "vaddy.x vf15, vf15, vf15y\n"
                 "mfc1 %0, %3\n"
                 "qmtc2 %0, vf14\n"
                 "vsubx.w vf14, vf0, vf14x\n"
                 "vmul.xyz vf18, vf18, vf20\n"
                 "vmulax.xyz ACC, vf19, vf14x\n"
                 "qmfc2 %0, vf15\n"
                 "dsll32 %0, %0, 0\n"
                 "dsra32 %0, %0, 0\n"
                 "vmaddw.xyz vf18, vf18, vf14w\n"
                 "vaddw.y vf14, vf0, vf14w\n"
                 : "=&r"(costheta), "=&r"(scratch) : "r"(packed), "f"(lerp) : "memory");
    return costheta;
}

static inline void negate_skb_quat()
{
    asm volatile("vabs.x vf15, vf15\n"
                 "vmulax.xyzw ACC, vf0, vf0x\n"
                 "vmsubw.xyzw vf17, vf17, vf0w\n"
                 : : : "memory");
}

static inline void store_skb_translation(xVec3* tran)
{
    U32 scratch;
    asm volatile("qmfc2 %0, vf18\n"
                 "sdr %0, 0x0(%1)\n"
                 "sdl %0, 0x7(%1)\n"
                 "pextuw %0, zero, %0\n"
                 "sw %0, 0x8(%1)\n"
                 : "=&r"(scratch) : "r"(tran) : "memory");
}

// Piecewise acos approximation, retaining the original signed bit tests.
// The positive, central and negative intervals use separate VU polynomials.
static inline void skb_acos(U32 costheta)
{
    asm volatile("vsubx.w vf3, vf0, vf2x\n"
                 "vmulw.w vf3, vf3, vf22w\n"
                 "vsqrt Q, vf3w\n"
                 : : : "memory");
    if ((S32)costheta >= 0x3f000000)
    {
        asm volatile("vmulw.w vf4, vf3, vf3w\n"
                 "vmulw.w vf5, vf4, vf4w\n"
                 : : : "memory");
        if (costheta < 0x3f800000)
        {
            U32 root;
            asm volatile("vaddaw.x ACC, vf0, vf3w\n"
                 "vaddq.x vf7, vf0, Q\n"
                 "cfc2 %0, vi22\n"
                 "srl %0, %0, 12\n"
                 "sll %0, %0, 12\n"
                 "qmtc2 %0, vf8\n"
                 "vmsubx.x vf9, vf8, vf8x\n"
                 "vaddx.x vf1, vf7, vf8x\n"
                 "vdiv Q, vf9x, vf1x\n"
                 "vmulaw.xyz ACC, vf24, vf3w\n"
                 "vmaddw.xyz vf6, vf25, vf4w\n"
                 "vaddax.x ACC, vf0, vf0x\n"
                 "vaddaw.y ACC, vf0, vf0w\n"
                 "vmaddaw.xy ACC, vf22, vf3w\n"
                 "vaddz.x vf5, vf0, vf6z\n"
                 "vmaddaw.xy ACC, vf23, vf4w\n"
                 "vmaddaw.xy ACC, vf6, vf4w\n"
                 "vaddq.x vf9, vf0, Q\n"
                 "vmaddaw.x ACC, vf5, vf5w\n"
                 "vmadd.xy vf6, vf0, vf0\n"
                 "vdiv Q, vf6x, vf6y\n"
                 "vadd.x vf7, vf7, vf7\n"
                 "vadda.x ACC, vf8, vf8\n"
                 "vmaddaw.x ACC, vf9, vf0w\n"
                 "vmaddaw.x ACC, vf9, vf0w\n"
                 "vwaitq\n"
                 "vmaddq.x vf1, vf7, Q\n"
                 : "=&r"(root) : : "memory");
        }
        else
        {
            asm volatile("vaddx.x vf1, vf0, vf0x\n"
                 : : : "memory");
        }
    }
    else
    {
        asm volatile("vmulx.x vf3, vf2, vf2x\n"
                 : : : "memory");
        U32 magnitude = costheta & 0x7fffffff;
        asm volatile("vmulx.x vf4, vf3, vf3x\n"
                 : : : "memory");
        if (magnitude < 0x3f000000)
        {
            asm volatile("vmulx.x vf5, vf4, vf4x\n"
                 "vmulax.xyz ACC, vf24, vf3x\n"
                 : : : "memory");
            if (magnitude > 0x23000000)
            {
                asm volatile("vmaddx.xyz vf6, vf25, vf4x\n"
                 "vaddax.x ACC, vf0, vf0x\n"
                 "vaddaw.y ACC, vf0, vf0w\n"
                 "vmaddax.xy ACC, vf22, vf3x\n"
                 "vmaddax.xy ACC, vf23, vf4x\n"
                 "vmaddax.xy ACC, vf6, vf4x\n"
                 "vmaddaz.x ACC, vf5, vf6z\n"
                 "vmadd.xy vf6, vf0, vf0\n"
                 "vdiv Q, vf6x, vf6y\n"
                 "vsubx.x vf4, vf0, vf2x\n"
                 "vaddz.x vf3, vf0, vf23z\n"
                 "vaddaw.x ACC, vf0, vf23w\n"
                 "vwaitq\n"
                 "vmsubaq.x ACC, vf2, Q\n"
                 "vmaddaw.x ACC, vf4, vf0w\n"
                 "vmaddw.x vf1, vf3, vf0w\n"
                 : : : "memory");
            }
            else
            {
                asm volatile("vaddw.x vf1, vf0, vf25w\n"
                 : : : "memory");
            }
        }
        else
        {
            asm volatile("vaddx.w vf3, vf0, vf2x\n"
                 "vmulw.w vf3, vf3, vf22w\n"
                 "vmulw.w vf4, vf3, vf3w\n"
                 : : : "memory");
            if (magnitude < 0x3f800000)
            {
                asm volatile("vsqrt Q, vf3w\n"
                 "vmulaw.xyz ACC, vf24, vf3w\n"
                 "vmaddw.xyz vf6, vf25, vf4w\n"
                 "vmulw.w vf5, vf4, vf4w\n"
                 "vaddax.x ACC, vf0, vf0x\n"
                 "vaddaw.y ACC, vf0, vf0w\n"
                 "vaddz.x vf5, vf0, vf6z\n"
                 "vmaddaw.xy ACC, vf22, vf3w\n"
                 "vmaddaw.xy ACC, vf23, vf4w\n"
                 "vmaddaw.xy ACC, vf6, vf4w\n"
                 "vsubq.x vf7, vf0, Q\n"
                 "vmaddaw.x ACC, vf5, vf5w\n"
                 "vmadd.xy vf6, vf0, vf0\n"
                 "vdiv Q, vf6x, vf6y\n"
                 "vaddw.x vf1, vf0, vf23w\n"
                 "vaddx.x vf7, vf7, vf7x\n"
                 "vaddz.x vf8, vf0, vf22z\n"
                 "vaddax.x ACC, vf1, vf1x\n"
                 "vwaitq\n"
                 "vmaddaq.x ACC, vf7, Q\n"
                 "vmaddaw.x ACC, vf7, vf0w\n"
                 "vmaddw.x vf1, vf8, vf0w\n"
                 : : : "memory");
            }
            else
            {
                asm volatile("vaddw.x vf1, vf0, vf24w\n"
                 : : : "memory");
            }
        }
    }
}

static inline void skb_slerp_weights()
{
    // Evaluate sin(t*theta), sin((1-t)*theta), and sin(theta) together.
    U32 theta;
    asm volatile("qmfc2 %0, vf1\n"
                 "vadd.xy vf1, vf14, vf0\n"
                 "vaddw.z vf1, vf0, vf0w\n"
                 "qmtc2 %0, vf2\n"
                 "vmulx.xyz vf1, vf1, vf2x\n"
                 "vmul.xyz vf2, vf1, vf1\n"
                 "vmul.xyz vf3, vf2, vf2\n"
                 "vmul.xyz vf4, vf3, vf3\n"
                 "vmula.xyz ACC, vf2, vf30\n"
                 "vmadd.xyz vf5, vf3, vf31\n"
                 "vmula.xyz ACC, vf2, vf28\n"
                 "vmadd.xyz vf6, vf3, vf29\n"
                 "vaddaw.xyz ACC, vf0, vf0w\n"
                 "vmadda.xyz ACC, vf3, vf27\n"
                 "vmadda.xyz ACC, vf2, vf26\n"
                 "vmadda.xyz ACC, vf3, vf6\n"
                 "vmadd.xyz vf11, vf4, vf5\n"
                 "vmul.xyz vf11, vf11, vf1\n"
                 "vdiv Q, vf0w, vf11z\n"
                 "vwaitq\n"
                 "vmulq.xy vf14, vf11, Q\n"
                 : "=&r"(theta) : : "memory");
}

static inline void store_skb_quat(xQuat* quat)
{
    asm volatile("vmulax.xyzw ACC, vf17, vf14x\n"
                 "vmaddy.xyzw vf16, vf16, vf14y\n"
                 "sqc2 vf16, 0x0(%0)\n"
                 : : "r"(quat) : "memory");
}

void iAnimEvalSKB(iAnimSKBHeader* data, F32 time, U32 flags, xVec3* tran, xQuat* quat)
{
    U32 i, tidx, bcount, tcount;
    iAnimSKBKey* keys;
    F32* times;
    U16* offsets;

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
    if (flags & 1)
    {
        bcount = 1;
    }
    if (flags & 2)
    {
        --bcount;
        ++offsets;
    }

    if (tcount == 1)
    {
        for (i = 0; i < bcount; ++i, ++quat, ++tran)
        {
            iAnimSKBKey* k = &keys[i * 2];
            quat->v.x = k->Quat[0] * (1.0f / 32767.0f);
            quat->v.y = k->Quat[1] * (1.0f / 32767.0f);
            quat->v.z = k->Quat[2] * (1.0f / 32767.0f);
            quat->s = k->Quat[3] * (1.0f / 32767.0f);
            tran->x = data->Scale[0] * k->Tran[0];
            tran->y = data->Scale[1] * k->Tran[1];
            tran->z = data->Scale[2] * k->Tran[2];
        }
    }
    else
    {
        prepare_skb_slerp();
        prepare_skb_scale(data->Scale);
        for (i = 0; i < bcount; ++quat, ++tran, ++i)
        {
            iAnimSKBKey* k = &keys[*offsets++];
            F32 time1 = time - times[k->TimeIndex];
            F32 time2 = times[k[1].TimeIndex] - times[k->TimeIndex];
            F32 lerp = time1 / time2;
            U32 costheta = unpack_skb_pair(k->Quat, lerp);
            if ((S32)costheta < 0)
            {
                negate_skb_quat();
                costheta &= 0x7fffffff;
            }
            store_skb_translation(tran);
            asm volatile("qmtc2 %0, vf2" : : "r"(costheta));
            if (costheta < 0x3f7fff58)
            {
                skb_acos(costheta);
                skb_slerp_weights();
            }
            store_skb_quat(quat);
        }
    }
}

static inline F32 skb_abs(F32 value)
{
    asm volatile("abs.s %0, %0" : "+f"(value));
    return value;
}

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
    F32 timefirst, timelast;
    iAnimSKBKey* keys;
    F32* times;
    U16* offsets;

    kcount = data->KeyCount;
    bcount = data->BoneCount;
    tcount = data->TimeCount;
    F32 oldmax[3] = {};
    F32 newmax[3] = {};

    keys = (iAnimSKBKey*)(data + 1);
    times = (F32*)(keys + kcount);
    offsets = (U16*)(times + tcount);

    keyfirst = offsets[bone];
    keylast = offsets[bone + (tcount - 2) * bcount] + 1;

    timefirst = times[0];
    timelast = times[tcount - 1];

    for (i = 0; i < kcount; i++)
    {
        for (idx = 0; idx < 3; idx++)
        {
            if (starttran[idx] || endtran[idx])
            {
                pos = data->Scale[idx] * keys[i].Tran[idx];

                if (skb_abs(pos) > oldmax[idx])
                {
                    oldmax[idx] = skb_abs(pos);
                }

                if (i >= keyfirst && i <= keylast)
                {
                    pos += starttran[idx] +
                           (endtran[idx] - starttran[idx]) *
                               (times[keys[i].TimeIndex] - timefirst) / (timelast - timefirst);

                    if (skb_abs(pos) > newmax[idx])
                    {
                        newmax[idx] = skb_abs(pos);
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

    keylast = offsets[bone + data->BoneCount * (tcount - 2)] + 1;

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
