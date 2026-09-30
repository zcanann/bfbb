#include "xSpline.h"

#include <types.h>
#include <rwplcore.h>
#include <xMathInlines.h>
#include <xMemMgr.h>
#include <mem.h>
#include <xVec3.h>

// MSL's <cmath> is not reachable from here; the target calls floorf__3stdFf.
namespace std
{
    float floorf(float x);
}

static F32 sBasisUniformBspline[4][4];
static F32 sBasisBezier[4][4] = { { -1.0f, 3.0f, -3.0f, 1.0f },
                                  { 3.0f, -6.0f, 3.0f, 0.0f },
                                  { -3.0f, 3.0f, 0.0f, 0.0f },
                                  { 1.0f, 0.0f, 0.0f, 0.0f } };
static F32 sBasisHermite[4][4] = { { 2.0f, -3.0f, 0.0f, 1.0f },
                                   { 1.0f, -2.0f, 1.0f, 0.0f },
                                   { 1.0f, -1.0f, 0.0f, 0.0f },
                                   { -2.0f, 3.0f, 0.0f, 0.0f } };

void Tridiag_Solve(F32* a, F32* b, F32* c, xVec3* d, xVec3* x, S32 n)
{
    S32 j;
    F32 beta;
    F32* gamma;
    xVec3* delta;

    gamma = (F32*)RwMalloc(n * sizeof(F32));
    delta = (xVec3*)RwMalloc(n * sizeof(xVec3));

    gamma[0] = c[0] / b[0];

    delta[0].x = d[0].x / b[0];
    delta[0].y = d[0].y / b[0];
    delta[0].z = d[0].z / b[0];

    for (j = 1; j < n; j++)
    {
        beta = b[j] - a[j] * gamma[j - 1];
        gamma[j] = c[j] / beta;
        delta[j].x = (d[j].x - a[j] * delta[j - 1].x) / beta;
        delta[j].y = (d[j].y - a[j] * delta[j - 1].y) / beta;
        delta[j].z = (d[j].z - a[j] * delta[j - 1].z) / beta;
    }

    j = n - 1;
    x[j].x = delta[j].x;
    x[j].y = delta[j].y;
    x[j].z = delta[j].z;

    for (j = n - 2; j >= 0; j--)
    {
        x[j].x = delta[j].x - gamma[j] * x[j + 1].x;
        x[j].y = delta[j].y - gamma[j] * x[j + 1].y;
        x[j].z = delta[j].z - gamma[j] * x[j + 1].z;
    }

    RwFree(gamma);
    RwFree(delta);
}

void Interpolate_Bspline(xVec3* data, xVec3* control, F32* knots, U32 nodata)
{
    F32* alpha;
    F32* beta;
    F32* gamma;

    U32 i;

    F32 diff_31;
    F32 diff_43;
    F32 diff_53;
    F32 diff_41;
    F32 diff_32;
    F32 diff_52;
    F32 diff_42;

    F32 t1;
    F32 t2;
    F32 t3;
    F32 t4;
    F32 t5;

    alpha = (F32*)RwMalloc(nodata * 4);
    beta = (F32*)RwMalloc(nodata * 4);
    gamma = (F32*)RwMalloc(nodata * 4);

    beta[0] = beta[nodata - 1] = 1.0f;
    alpha[0] = alpha[nodata - 1] = 0.0f;
    gamma[0] = gamma[nodata - 1] = 0.0f;

    for (i = 1; i < nodata - 1; i += 1)
    {
        t1 = knots[i + 1];
        t2 = knots[i + 2];
        t3 = knots[i + 3];
        t4 = knots[i + 4];
        t5 = knots[i + 5];

        diff_31 = t3 - t1;
        diff_43 = t4 - t3;
        diff_32 = t3 - t2;
        diff_53 = t5 - t3;
        diff_41 = t4 - t1;
        diff_52 = t5 - t2;

        alpha[i] = (diff_43 * diff_43) / diff_41;
        beta[i] = (diff_31 * diff_43) / diff_41 + (diff_53 * diff_32) / diff_52;
        gamma[i] = (diff_32 * diff_32) / diff_52;

        diff_42 = t4 - t2;
        alpha[i] = alpha[i] / diff_42;
        beta[i] = beta[i] / diff_42;
        gamma[i] = gamma[i] / diff_42;
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
    F64 x1 = coef->x.a[1];
    F64 y1 = coef->y.a[1];
    F64 z1 = coef->z.a[1];
    F64 y2 = coef->y.a[2];
    F64 x2 = coef->x.a[2];
    F64 z2 = coef->z.a[2];

    F64 xx = x0 * x0;
    F64 yy = y0 * y0;
    F64 zz = z0 * z0;

    E = 9.0 * (xx + yy + zz);
    D = 12.0 * (x0 * x1 + y0 * y1 + z0 * z1);
    C = 6.0 * (x0 * x2 + y0 * y2 + z0 * z2) + 4.0 * (x1 * x1 + y1 * y1 + z1 * z1);
    B = 4.0 * (x1 * x2 + y1 * y2 + z1 * z2);
    A = x2 * x2 + y2 * y2 + z2 * z2;
    h = (uend - ustart) / 50.0;
    sum = 0.0;
    u = ustart + h;

    for (i = 2; i <= 50; i += 1)
    {
        if (i & 1)
        {
            sum = sum + 2.0 * sqrt(A + u * (B + u * (C + u * (D + E * u))));
        }
        else
        {
            sum = sum + 4.0 * sqrt(A + u * (B + u * (C + u * (D + E * u))));
        }
        u = u + h;
    }

    return (h * (sum + sqrt(A + ustart * (B + ustart * (C + ustart * (D + E * ustart)))) +
                 sqrt(A + uend * (B + uend * (C + uend * (D + E * uend)))))) /
           3.0;
}

// We don't have the implementation provided
double sqrt(double x)
{
    if (x > 0.0)
    {
        F64 guess = __frsqrte(x);
        guess = 0.5 * guess * -(guess * guess * x - 3);
        guess = 0.5 * guess * -(guess * guess * x - 3);
        guess = 0.5 * guess * -(guess * guess * x - 3);
        guess = 0.5 * guess * -(guess * guess * x - 3);
        return x * guess;
    }
    else if (0.0 == x)
    {
        return 0.0;
    }
    else if (x != 0.0)
    {
        // Retail returns NaN for every negative input; the INFINITY arm below
        // is unreachable. Preserved deliberately.
        return NAN;
    }
    else
    {
        return INFINITY;
    }
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
    S32 i;

    for (i = 0; i < 4; i++)
    {
        coef->x.a[i] =
            (N[3][i] * v4->x) + ((N[2][i] * v3->x) + ((v1->x * N[0][i]) + (N[1][i] * v2->x)));
        coef->y.a[i] =
            (N[3][i] * v4->y) + ((N[2][i] * v3->y) + ((v1->y * N[0][i]) + (N[1][i] * v2->y)));
        coef->z.a[i] =
            (N[3][i] * v4->z) + ((N[2][i] * v3->z) + ((v1->z * N[0][i]) + (N[1][i] * v2->z)));
    }
}

void CoefToUnity3(xCoef3* coef1, xCoef3* coef2, F32 f1, F32 f2)
{
    F32 fdiff;
    F32 coef2_0;
    F32 coef2_1;
    F32 coef2_2;
    F32 coef2_3;

    F32 factor;
    S32 i;
    xCoef* c1;
    xCoef* c2;

    fdiff = f2 - f1;
    c1 = &coef1->x;
    c2 = &coef2->x;
    for (i = 3; i != 0; i -= 1)
    {
        coef2_0 = c2->a[0];
        coef2_1 = c2->a[1];
        coef2_2 = c2->a[2];
        coef2_3 = c2->a[3];

        factor = 3.0f * coef2_0 * fdiff;

        c1->a[0] = fdiff * (fdiff * (coef2_0 * fdiff));
        c1->a[1] = (f1 * (fdiff * factor)) + (fdiff * (coef2_1 * fdiff));
        c1->a[2] = (coef2_2 * fdiff) + ((f1 * (f1 * factor)) + (f1 * (2.0f * coef2_1 * fdiff)));
        c1->a[3] =
            coef2_3 + ((coef2_2 * f1) + ((f1 * (f1 * (coef2_0 * f1))) + (f1 * (coef2_1 * f1))));

        c1++;
        c2++;
    }
}

void BasisBspline(F32 (*N)[4], F32* t)
{
    U32 k;
    U32 i;
    U32 c;
    F32 d2;
    F32 d1;

    N[0][3] = 0.0f;
    N[1][3] = 0.0f;
    N[2][3] = 0.0f;
    N[3][3] = 1.0f;
    N[4][3] = 0.0f;
    N[5][3] = 0.0f;
    N[6][3] = 0.0f;

    for (i = 2; i <= 4; i++)
    {
        for (k = 0; k < 8 - i; k++)
        {
            F32 Ntemp[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

            d2 = t[k + i - 1] - t[k];
            if (d2 != 0.0f)
            {
                d2 = 1.0f / d2;
            }

            d1 = t[k + i] - t[k + 1];
            if (d1 != 0.0f)
            {
                d1 = 1.0f / d1;
            }

            for (c = 5 - i; c <= 3; c++)
            {
                Ntemp[c - 1] += d2 * N[k][c] - d1 * N[k + 1][c];
                Ntemp[c] += d2 * (-t[k] * N[k][c]) + d1 * (t[k + i] * N[k + 1][c]);
            }

            N[k][0] = Ntemp[0];
            N[k][1] = Ntemp[1];
            N[k][2] = Ntemp[2];
            N[k][3] = Ntemp[3];
        }
    }
}

F32 ClampBspline(xSpline3* spl, F32 u)
{
    if (u < 0.0f)
    {
        u = 0.0f;
    }
    if (u > spl->knot[spl->N + 3])
    {
        return spl->knot[spl->N + 3];
    }
    return u;
}

S32 SegBspline(xSpline3* spl, F32 u)
{
    U32 seg_total;
    U32 seg_guess;
    U32 seg_min;
    U32 seg_max;

    seg_min = 3;
    seg_max = spl->N + 3;

    while ((U32)(seg_min + 1) != seg_max)
    {
        seg_total = seg_max + seg_min;
        seg_guess = seg_total >> 1;
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
    F32 clamp_result;
    S32 seg_result;
    xVec3* temp_vec;

    clamp_result = ClampBspline(spl, u);
    seg_result = SegBspline(spl, clamp_result);
    BasisBspline(N, &spl->knot[seg_result]);
    temp_vec = spl->bctrl + seg_result;
    BasisToCoef3(&coef, N, temp_vec, temp_vec + 1, temp_vec + 2, temp_vec + 3);
    EvalCoef3(&coef, clamp_result, deriv, o);
}

xCoef3* CoefSeg3(xSpline3* spl, U32 seg, xCoef3* tempCoef)
{
    F32 N[7][4];

    F32* temp_knotseg;
    xVec3* temp_bctrl;

    switch (spl->type)
    {
    case 1:
        return spl->coef + seg;
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
        temp_bctrl = spl->bctrl + seg;
        BasisToCoef3(tempCoef, N, temp_bctrl, temp_bctrl + 1, temp_bctrl + 2, temp_bctrl + 3);
        temp_knotseg = &spl->knot[seg];
        CoefToUnity3(tempCoef, tempCoef, temp_knotseg[3], temp_knotseg[4]);
        break;
    }

    return tempCoef;
}

void xSpline3_EvalSeg(xSpline3* spl, F32 u, U32 deriv, xVec3* o)
{
    xCoef3 tempCoef;
    F32 temp_u;
    U32 seg;

    if ((U16)spl->type == 4)
    {
        EvalBspline3(spl, u, deriv, o);
        return;
    }
    if (u < 0.0f)
    {
        u = 0.0f;
    }
    temp_u = std::floorf(u);
    seg = (U32)temp_u;
    if (seg >= spl->N)
    {
        u = 1.0f;
        seg = spl->N - 1;
    }
    else
    {
        u = u - temp_u;
    }

    switch (spl->type)
    {
    case 1:
        EvalCoef3(spl->coef + seg, u, deriv, o);
        return;
    case 2:
        BasisToCoef3(&tempCoef, sBasisHermite, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        EvalCoef3(&tempCoef, u, deriv, o);
        return;
    case 3:
        BasisToCoef3(&tempCoef, sBasisBezier, spl->points + seg, spl->p12 + seg * 2,
                     spl->p12 + seg * 2 + 1, spl->points + seg + 1);
        EvalCoef3(&tempCoef, u, deriv, o);
        return;
    }
}

F32 ArcEvalIterate(xSpline3* spl, F32 s, U32 deriv, xVec3* o, U32 iterations)
{
    xCoef3* coef;
    xCoef3 tempCoef;

    F32 umin;
    F32 smax;
    F32 smin;
    F32 umax;
    F32 utest;
    F32 arctest;
    F32 arclengthmax;

    S32 min;
    S32 max;
    S32 test;
    S32 seg;

    min = -1;
    max = spl->arcSample * spl->N - 1;
    while (min + 1 != max)
    {
        test = (min + max) >> 1;
        if (s > spl->arcLength[test])
        {
            min = test;
        }
        else
        {
            max = test;
        }
    }

    seg = max / (S32)spl->arcSample;
    min = seg * spl->arcSample;
    umin = (F32)(max - min) / (F32)spl->arcSample;
    umax = (F32)((max + 1) - min) / (F32)spl->arcSample;
    if (max - 1 >= 0)
    {
        smax = spl->arcLength[max - 1];
    }
    else
    {
        smax = 0.0f;
    }

    arclengthmax = spl->arcLength[max];
    if (min - 1 >= 0)
    {
        smin = spl->arcLength[min - 1];
    }
    else
    {
        smin = 0.0f;
    }
    coef = CoefSeg3(spl, seg, &tempCoef);

    if (s <= smax)
    {
        EvalCoef3(coef, umin, deriv, o);
        return (F32)seg + umin;
    }

    if (s >= arclengthmax)
    {
        EvalCoef3(coef, umax, deriv, o);
        return (F32)seg + umax;
    }

    s = s - smin;
    smax = smax - smin;
    arclengthmax = arclengthmax - smin;

    while (iterations != 0)
    {
        utest = umin + 0.5f * (umax - umin);
        arctest = ArcLength3(coef, 0.0, utest);
        if (s > arctest)
        {
            umin = utest;
            smax = arctest;
        }
        else
        {
            umax = utest;
            arclengthmax = arctest;
        }
        iterations -= 1;
    }

    if (0.0f == arclengthmax - smax)
    {
        utest = umin;
    }
    else
    {
        utest = umin + ((umax - umin) * (s - smax)) / (arclengthmax - smax);
        if (utest < 0.0f)
        {
            utest = 0.0f;
        }
        else if (utest > 1.0f)
        {
            utest = 1.0f;
        }
    }

    EvalCoef3(coef, utest, deriv, o);
    return (F32)seg + utest;
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
    U32 i;
    U32 seg;
    F32 len;
    F32 arcsum;
    xCoef3 tempCoef;
    xCoef3* coef;

    len = 0.0f;
    if (sample < 1)
    {
        sample = 1;
    }

    spl->arcSample = sample;
    if (spl->arcLength != NULL)
    {
        i = spl->allocN * spl->arcSample;
    }
    else
    {
        i = 0;
    }

    if (i < spl->N * sample)
    {
        spl->arcLength = (F32*)xMemAlloc(gActiveHeap, spl->arcSample * spl->allocN * 4, 0);
    }

    arcsum = 0.0f;
    for (seg = 0; seg < spl->N; seg += 1)
    {
        coef = CoefSeg3(spl, seg, &tempCoef);
        for (i = 0; i < sample; i += 1)
        {
            len = ArcLength3(coef, 0.0, (F32)(i + 1) / (F32)sample);
            spl->arcLength[seg * sample + i] = arcsum + len;
        }
        arcsum = arcsum + len;
    }
}

xSpline3* AllocSpline3(xVec3* points, F32* time, U32 numpoints, U32 numalloc, U32 flags, U32 type)
{
    xSpline3* spl;

    spl = (xSpline3*)xMemAlloc(gActiveHeap, sizeof(xSpline3), 0);
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
    xSpline3* spl;
    U32 i;

    spl = AllocSpline3(points, time, numpoints, numalloc, 0U, 3U);
    spl->p12 = (xVec3*)xMemAlloc(gActiveHeap, sizeof(xVec3) * (spl->allocN * 2), 0);
    if ((p1 == NULL) || (p2 == NULL))
    {
        xSpline3_Catmullize(spl);
    }
    else
    {
        for (i = 0; i < spl->N; i += 1)
        {
            spl->p12[2 * i] = p1[i];
            spl->p12[2 * i + 1] = p2[i];
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
