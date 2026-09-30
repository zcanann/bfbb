#include "xSpline.h"

#include <types.h>
#include <rwplcore.h>
#include <xMathInlines.h>
#include <xMemMgr.h>
#include <mem.h>
#include <xVec3.h>

#include <PowerPC_EABI_Support\MSL_C\MSL_Common\cmath>

static F32 sBasisUniformBspline[4][4];
static F32 sBasisBezier[4][4] = {
    { -1.0f, 3.0f, -3.0f, 1.0f },
    { 3.0f, -6.0f, 3.0f, 0.0f },
    { -3.0f, 3.0f, 0.0f, 0.0f },
    { 1.0f, 0.0f, 0.0f, 0.0f },
};
static F32 sBasisHermite[4][4] = {
    { 2.0f, -3.0f, 0.0f, 1.0f },
    { 1.0f, -2.0f, 1.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f, 0.0f },
    { -2.0f, 3.0f, 0.0f, 0.0f },
};

void Tridiag_Solve(F32* a, F32* b, F32* c, xVec3* d, xVec3* x, S32 n)
{
    S32 j;

    F32* a_temp;
    F32* b_temp;
    F32* c_temp;
    xVec3* delta;

    S32 vec_offset;
    F32 beta;
    F32* gamma;

    F32* c_prime;
    F32* d_prime;

    c_prime = (F32*)RwMalloc(n << 2);
    d_prime = (F32*)RwMalloc(n * 0xC);

    c_prime[0] = *c / *b;

    d_prime[0] = d->x / *b;
    d_prime[1] = d->y / *b;
    d_prime[2] = d->z / *b;

    a_temp = a + 1;
    b_temp = b + 1;
    c_temp = c + 1;
    delta = d + 1;
    vec_offset = 0xC;

    if (n > 1)
    {
        for (j = 1; j < n; j += 1)
        {
            beta = b_temp[0];
            gamma = (F32*)((S32)d_prime + vec_offset - 0xc);
            vec_offset = vec_offset + 0xC;
            b_temp = b_temp + 1;
            c_temp = c_temp + 1;
            *b_temp = *b_temp - *a_temp * c_prime[0];
            c_prime[1] = *c_temp / beta;
            d_prime[3] = (delta->x - *a_temp * gamma[0]) / beta;
            d_prime[4] = (delta->y - *a_temp * gamma[1]) / beta;
            *c_temp = *a_temp;
            a_temp = a_temp + 1;
            delta = delta + 1;
            d_prime[5] = (*(&delta->z) - *c_temp * gamma[2]) / beta;
            c_prime = c_prime + 1;
            d_prime = d_prime + 3;
        }
    }

    j = n - 2;
    c_prime = d_prime + (n - 1) * 3;
    delta->x = c_prime[0];
    delta->y = c_prime[1];
    delta->z = c_prime[2];
    vec_offset = j * 0xC;
    d_prime = d_prime + j * 3;
    delta = x + j;
    if (n > 1)
    {
        for (j = 0; j >= 0; j -= 1)
        {
            d_prime = (F32*)((S32)&x[1].x + vec_offset);
            vec_offset = vec_offset - 0xC;
            delta->x = *d_prime - *c_prime * *d_prime;
            delta->y = d_prime[1] - *c_prime * d_prime[1];
            beta = *c_prime;
            c_prime = d_prime + 2;
            c_prime = c_prime - 1;
            d_prime = d_prime - 3;
            delta->z = *c_prime - beta * d_prime[2];
            delta = delta - 1;
        }
    }

    RwFree(c_prime);
    RwFree(d_prime);

    return;
}

void Interpolate_Bspline(xVec3* data, xVec3* control, F32* knots, U32 nodata)
{
    F32* alpha = (F32*)RwMalloc(nodata * sizeof(F32));
    F32* beta = (F32*)RwMalloc(nodata * sizeof(F32));
    F32* gamma = (F32*)RwMalloc(nodata * sizeof(F32));

    alpha[0] = alpha[nodata - 1] = 0.0f;
    beta[0] = beta[nodata - 1] = 1.0f;
    gamma[0] = gamma[nodata - 1] = 0.0f;

    for (U32 i = 1; i < nodata - 1; i++)
    {
        F32 t1 = knots[i + 1];
        F32 t2 = knots[i + 2];
        F32 t3 = knots[i + 3];
        F32 t4 = knots[i + 4];
        F32 t5 = knots[i + 5];

        alpha[i] = (t4 - t3) * (t4 - t3) / (t4 - t1);
        beta[i] = (t3 - t1) * (t4 - t3) / (t4 - t1) + (t5 - t3) * (t3 - t2) / (t5 - t2);
        gamma[i] = (t3 - t2) * (t3 - t2) / (t5 - t2);

        alpha[i] /= t4 - t2;
        beta[i] /= t4 - t2;
        gamma[i] /= t4 - t2;
    }

    Tridiag_Solve(alpha, beta, gamma, data, control + 1, nodata);

    control[0] = control[1];
    control[nodata + 1] = control[nodata];

    RwFree(alpha);
    RwFree(beta);
    RwFree(gamma);
}

// Implementation of Composite Simpson's 1/3 Rule to calculate arc length
F32 ArcLength3(xCoef3* coef, F64 ustart, F64 uend)
{
    U32 i;
    F64 E;
    F64 D;
    F64 C;
    F64 B;
    F64 A;
    F64 h;
    F64 sum;
    F64 u;

    F64 y0 = coef->y.a[0];
    F64 x0 = coef->x.a[0];
    F64 z0 = coef->z.a[0];
    F64 y1 = coef->y.a[1];
    F64 x1 = coef->x.a[1];
    F64 z1 = coef->z.a[1];
    F64 y2 = coef->y.a[2];
    F64 x2 = coef->x.a[2];
    F64 z2 = coef->z.a[2];

    F64 x0sq = x0 * x0;
    F64 y0sq = y0 * y0;
    F64 z0sq = z0 * z0;
    E = 9.0 * (x0sq + y0sq + z0sq);
    D = 12.0 * (x0 * x1 + y0 * y1 + z0 * z1);
    C = 6.0 * (x0 * x2 + y0 * y2 + z0 * z2) + 4.0 * (x1 * x1 + y1 * y1 + z1 * z1);
    B = 4.0 * (x1 * x2 + y1 * y2 + z1 * z2);
    A = x2 * x2 + y2 * y2 + z2 * z2;

    h = (uend - ustart) / 50.0;
    sum = 0.0;
    u = ustart + h;

    for (i = 2; i <= 50; i++)
    {
        if (i & 1)
        {
            sum += 2.0 * sqrt(A + u * (B + u * (C + u * (D + E * u))));
        }
        else
        {
            sum += 4.0 * sqrt(A + u * (B + u * (C + u * (D + E * u))));
        }
        u += h;
    }

    return h *
           (sum + sqrt(A + ustart * (B + ustart * (C + ustart * (D + E * ustart)))) +
            sqrt(A + uend * (B + uend * (C + uend * (D + E * uend))))) /
           3.0;
}

void EvalCoef3(xCoef3* coef, F32 u, U32 deriv, xVec3* o)
{
    switch ((S32)deriv)
    {
    case 0:
        o->x =
            (u * ((u * (((coef->x).a[0] * u) + (coef->x).a[1])) + (coef->x).a[2])) + (coef->x).a[3];
        o->y =
            (u * ((u * (((coef->y).a[0] * u) + (coef->y).a[1])) + (coef->y).a[2])) + (coef->y).a[3];
        o->z =
            (u * ((u * (((coef->z).a[0] * u) + (coef->z).a[1])) + (coef->z).a[2])) + (coef->z).a[3];
        return;
    case 1:
        o->x = (u * ((2.0f * (coef->x).a[1]) + (3.0f * (coef->x).a[0] * u))) + (coef->x).a[2];
        o->y = (u * ((2.0f * (coef->y).a[1]) + (3.0f * (coef->y).a[0] * u))) + (coef->y).a[2];
        o->z = (u * ((2.0f * (coef->z).a[1]) + (3.0f * (coef->z).a[0] * u))) + (coef->z).a[2];
        return;
    case 2:
        o->x = (2.0f * (coef->x).a[1]) + (6.0f * (coef->x).a[0] * u);
        o->y = (2.0f * (coef->y).a[1]) + (6.0f * (coef->y).a[0] * u);
        o->z = (2.0f * (coef->z).a[1]) + (6.0f * (coef->z).a[0] * u);
        return;
    case 3:
        o->x = 6.0f * (coef->x).a[0];
        o->y = 6.0f * (coef->y).a[0];
        o->z = 6.0f * (coef->z).a[0];
        return;
    default:
        o->x = 0.0f;
        o->y = 0.0f;
        o->z = 0.0f;
        return;
    }
}

void BasisToCoef3(xCoef3* coef, F32 (*N)[4], xVec3* v1, xVec3* v2, xVec3* v3, xVec3* v4)
{
    for (S32 i = 0; i < 4; i++)
    {
        coef->x.a[i] = v1->x * N[0][i] + N[1][i] * v2->x + N[2][i] * v3->x + N[3][i] * v4->x;
        coef->y.a[i] = v1->y * N[0][i] + N[1][i] * v2->y + N[2][i] * v3->y + N[3][i] * v4->y;
        coef->z.a[i] = v1->z * N[0][i] + N[1][i] * v2->z + N[2][i] * v3->z + N[3][i] * v4->z;
    }
}

void CoefToUnity3(xCoef3* coef1, xCoef3* coef2, F32 f1, F32 f2)
{
    F32 fdiff = f2 - f1;
    xCoef* c1 = &coef1->x;
    xCoef* c2 = &coef2->x;

    for (S32 i = 0; i < 3; i++)
    {
        F32 a0 = c2->a[0];
        F32 a1 = c2->a[1];
        F32 a2 = c2->a[2];
        F32 a3 = c2->a[3];

        F32 factor = 3.0f * a0 * fdiff;

        c1->a[0] = fdiff * (fdiff * a0 * fdiff);
        c1->a[1] = (f1 * (fdiff * factor)) + (fdiff * (a1 * fdiff));
        c1->a[2] = (a2 * fdiff) + ((f1 * (f1 * factor)) + (f1 * 2.0f * a1 * fdiff));
        c1->a[3] = a3 + ((a2 * f1) + ((f1 * (f1 * a0 * f1)) + (f1 * a1 * f1)));

        c1++;
        c2++;
    }
}

void BasisBspline(F32 (*N)[4], F32* t)
{
    U32 i;
    U32 k;
    U32 c;
    F32 d1;
    F32 d2;

    N[0][3] = 0.0;
    N[1][3] = 0.0;
    N[2][3] = 0.0;
    N[3][3] = 1.0;
    N[4][3] = 0.0;
    N[5][3] = 0.0;
    N[6][3] = 0.0;

    F32* Ntemp;
    Ntemp = 0;

    for (i = 2; i < 5; i += 1)
    {
        if (i != 0)
        {
            for (k = 0; k < 8; k += 1)
            {
                d2 = t[k + i - 1] - *t;
                if (d2 != 0.0)
                {
                    d2 = 1.0 / d2;
                }
                d1 = t[k + i] - t[1];
                if (d1 != 0.0)
                {
                    d1 = 1.0 / d1;
                }
                if (i > 1)
                {
                    Ntemp = Ntemp - i;
                    for (c = 0; c < 4; c += 1)
                    {
                        *Ntemp = *Ntemp + d1 * t[k + i] * *(Ntemp + 4) + d2 * -*t * *Ntemp;
                        Ntemp = Ntemp + 1;
                    }
                }
                (*N)[0] = Ntemp[0];
                (*N)[1] = Ntemp[1];
                (*N)[2] = Ntemp[2];
                (*N)[3] = Ntemp[3];
                t += 1;
                N += 1;
            }
        }
    }
    return;
}

F32 ClampBspline(xSpline3* spl, F32 u)
{
    if (u < 0.0f)
    {
        u = 0.0f;
    }
    if (u > spl->knot[spl->N + 3])
    {
        u = spl->knot[spl->N + 3];
    }
    return u;
}

S32 SegBspline(xSpline3* spl, F32 u)
{
    U32 seg_min = 3;
    U32 seg_max = spl->N + 3;

    while (seg_min + 1 != seg_max)
    {
        U32 seg_guess = (seg_max + seg_min) >> 1;
        if (spl->knot[seg_guess] >= u)
        {
            seg_max = seg_guess;
        }
        else
        {
            seg_min = seg_guess;
        }
    }

    return seg_min - 3;
}

void EvalBspline3(xSpline3* spl, F32 u, U32 deriv, xVec3* o)
{
    F32 N[7][4];
    xCoef3 coef;

    u = ClampBspline(spl, u);
    S32 seg = SegBspline(spl, u);
    BasisBspline(N, &spl->knot[seg]);
    BasisToCoef3(&coef, N, spl->bctrl + seg, spl->bctrl + seg + 1, spl->bctrl + seg + 2,
                 spl->bctrl + seg + 3);
    EvalCoef3(&coef, u, deriv, o);
}

xCoef3* CoefSeg3(xSpline3* spl, U32 seg, xCoef3* tempCoef)
{
    F32 N[7][4];

    switch (spl->type)
    {
    case 1:
        return &spl->coef[seg];
    case 2:
        BasisToCoef3(tempCoef, sBasisHermite, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        break;
    case 3:
        BasisToCoef3(tempCoef, sBasisBezier, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        break;
    case 4:
        BasisBspline(N, &spl->knot[seg]);
        BasisToCoef3(tempCoef, N, spl->bctrl + seg, spl->bctrl + seg + 1,
                     spl->bctrl + seg + 2, spl->bctrl + seg + 3);
        CoefToUnity3(tempCoef, tempCoef, spl->knot[seg + 3], spl->knot[seg + 4]);
        break;
    }

    return tempCoef;
}

void xSpline3_EvalSeg(xSpline3* spl, F32 u, U32 deriv, xVec3* o)
{
    xCoef3 tempCoef;
    F32 temp_u;
    U32 seg;

    if (spl->type == 4)
    {
        EvalBspline3(spl, u, deriv, o);
        return;
    }

    if (u < 0.0f)
    {
        u = 0.0f;
    }

    temp_u = std::floorf(u);
    seg = temp_u;
    if (seg >= spl->N)
    {
        u = 1.0f;
        seg = spl->N - 1;
    }
    else
    {
        u -= temp_u;
    }

    switch (spl->type)
    {
    case 1:
        EvalCoef3(spl->coef + seg, u, deriv, o);
        break;
    case 2:
        BasisToCoef3(&tempCoef, sBasisHermite, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        EvalCoef3(&tempCoef, u, deriv, o);
        break;
    case 3:
        BasisToCoef3(&tempCoef, sBasisBezier, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        EvalCoef3(&tempCoef, u, deriv, o);
        break;
    }
}

F32 ArcEvalIterate(xSpline3* spl, F32 s, U32 deriv, xVec3* o, U32 iterations)
{
    xCoef3* coef;
    xCoef3 tempCoef;

    U32 seg;
    S32 min;
    S32 max;
    S32 test;
    S32 segmul;

    F32 utest;
    F32 arctest;
    F32 umin;
    F32 umax;
    F32 smin;
    F32 smax;
    F32 arclengthmax;

    min = -1;
    max = spl->arcSample * spl->N - 1;
    if (max != 0)
    {
        while (min + 1 != max)
        {
            test = min + max >> 1;
            segmul = test;
            if (s <= spl->arcLength[test])
            {
                segmul = min;
                max = test;
            }
            min = segmul;
        }
    }

    seg = max / spl->arcSample;
    min = seg * spl->arcSample;
    umin = (F32)(max - min) / (F32)spl->arcSample;
    umax = (F32)((max + 1) - min) / (F32)spl->arcSample;
    if (max > 1)
    {
        smax = spl->arcLength[max - 1];
    }
    else
    {
        smax = 0.0;
    }

    arclengthmax = spl->arcLength[max];
    if (min - 1 > 0)
    {
        smin = spl->arcLength[min - 1];
    }
    else
    {
        smin = 0.0;
    }
    coef = CoefSeg3(spl, seg, &tempCoef);

    if (s <= smax)
    {
        EvalCoef3(coef, umin, deriv, o);
        umin = (F32)(S32)seg + umin;
    }
    else if (arclengthmax <= s)
    {
        EvalCoef3(coef, umax, deriv, o);
        umin = (F32)(S32)seg + umax;
    }
    else
    {
        smax = smax - smin;
        arclengthmax = arclengthmax - smin;
        smax = smax;
        if (iterations != 0)
        {
            for (iterations = iterations; iterations != 0; iterations -= 1)
            {
                utest = umin + (umax - umin) * 0.5;
                arctest = ArcLength3(coef, 0.0, utest);
                umin = utest;
                smax = arctest;
                if (s - smin <= arctest)
                {
                    umax = utest;
                    arclengthmax = arctest;
                }
            }
        }
        if (arclengthmax - smax != 0.0)
        {
            umax = umin + ((umax - umin) * ((s - smin) - smax)) / (arclengthmax - smax);
            if (umax > 0.0)
            {
                umin = 0.0;
            }
            else
            {
                umin = 1.0;
                if (umax <= 1.0)
                {
                    umin = umax;
                }
            }
        }
        EvalCoef3(coef, umin, deriv, o);
        umin = (F32)seg + umin;
    }
    return umin;
}

F32 xSpline3_EvalArcApprox(xSpline3* spl, F32 s, U32 deriv, xVec3* o)
{
    if (spl->arcLength != NULL)
    {
        return ArcEvalIterate(spl, s, deriv, o, 0);
    }
    else
    {
        xSpline3_EvalSeg(spl, s, deriv, o);
    }
    return s;
}

void xSpline3_ArcInit(xSpline3* spl, U32 sample)
{
    xCoef3 tempCoef;
    F32 len = 0.0f;
    F32 arcsum;
    U32 allocSample;
    U32 idx;
    U32 k;
    U32 seg;
    xCoef3* coef;
    U32 i;

    if (sample < 1)
    {
        sample = 1;
    }

    spl->arcSample = sample;

    allocSample = spl->arcLength != NULL ? spl->allocN * spl->arcSample : 0;
    if (allocSample < spl->N * sample)
    {
        spl->arcLength = (F32*)xMemAlloc(gActiveHeap, spl->arcSample * spl->allocN * sizeof(F32), 0);
    }

    arcsum = 0.0f;
    k = 0;
    for (i = 0; i < spl->N; i++)
    {
        coef = CoefSeg3(spl, i, &tempCoef);
        idx = k;

        for (seg = 0; seg < sample; seg++)
        {
            len = ArcLength3(coef, 0.0, (F32)(seg + 1) / (F32)sample);
            spl->arcLength[idx++] = arcsum + len;
        }

        arcsum += len;
        k += sample;
    }
}

xSpline3* AllocSpline3(xVec3* points, F32* time, U32 numpoints, U32 numalloc, U32 flags, U32 type)
{
    xSpline3* spl;

    spl = (xSpline3*)xMemAlloc(gActiveHeap, 0x2c, 0);
    if (numalloc < numpoints)
    {
        numalloc = numpoints;
    }

    spl->type = (ushort)type;
    spl->flags = (ushort)flags;
    spl->N = numpoints - 1;
    spl->allocN = numalloc - 1;
    spl->p12 = (xVec3*)0x0;
    spl->bctrl = (xVec3*)0x0;
    spl->knot = (float*)0x0;
    spl->coef = (xCoef3*)0x0;
    spl->arcSample = 0;
    spl->arcLength = (float*)0x0;
    spl->points = (xVec3*)xMemAlloc(gActiveHeap, (spl->allocN + 1) * 0xc, 0);
    memcpy(spl->points, points, (spl->N + 1) * 0xc);

    if (time != (F32*)0x0)
    {
        spl->time = (F32*)xMemAlloc(gActiveHeap, (spl->allocN + 1) * 4, 0);
        memcpy(spl->time, time, (spl->N + 1) * 4);
    }
    else
    {
        spl->time = (F32*)0x0;
    }
    return spl;
}

xSpline3* xSpline3_Bezier(xVec3* points, F32* time, U32 numpoints, U32 numalloc, xVec3* p1,
                          xVec3* p2)
{
    xSpline3* spl = AllocSpline3(points, time, numpoints, numalloc, 0, 3);
    spl->p12 = (xVec3*)xMemAlloc(gActiveHeap, spl->allocN * 2 * sizeof(xVec3), 0);

    if (p1 == NULL || p2 == NULL)
    {
        xSpline3_Catmullize(spl);
    }
    else
    {
        for (U32 i = 0; i < spl->N; i++)
        {
            spl->p12[i * 2] = p1[i];
            spl->p12[i * 2 + 1] = p2[i];
        }
    }

    return spl;
}

void xSpline3_Update(xSpline3* spl)
{
    if ((u16)spl->type == 4)
    {
        Interpolate_Bspline(spl->points, spl->bctrl, spl->knot, spl->N + 1);
    }
    if ((xSpline3*)spl->arcLength != NULL)
    {
        xSpline3_ArcInit(spl, (u32)spl->arcSample);
    }
}

void xSpline3_Catmullize(xSpline3* spl)
{
    xSpline3_Update(spl);
}
