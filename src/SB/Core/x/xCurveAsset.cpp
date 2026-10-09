#include "xCurveAsset.h"

#include "xMath.h"

#include <stdlib.h>

#if defined(PS2)
// The PS2 runtime performs the integer absolute value out of line.
#pragma inline_intrinsics off

static inline void cycle_curve_time(F32& t, F32 min_t, F32 max_t, const xCurveClamp& clamp)
{
    F32 curve_length = max_t - min_t;
    S32 curve_shift = (t - min_t) / curve_length;
    if (t < min_t)
    {
        curve_shift--;
    }
    t -= curve_shift * curve_length;
    if (clamp == xCC_OSCILLATE && abs(curve_shift % 2) == 1)
    {
        t = min_t + (curve_length - (t - min_t));
    }
}
#endif

F32 xCurveAssetEvaluate(const xCurveAsset* curve_asset, F32 t)
{
    F32 max_t = curve_asset->delta * (curve_asset->numPoints - 1);

    if (curve_asset->clamp == xCC_CONSTANT)
    {
#if defined(XBOX)
        t = MIN(t, max_t);
        t = MAX(t, 0.0f);
#else
        F32 curve_length = MIN(t, max_t);

        t = MAX(curve_length, 0.0f);
#endif
    }
    else
    {
#if defined(PS2)
        cycle_curve_time(t, 0.0f, max_t, curve_asset->clamp);
#else
        S32 curve_shift = t / max_t;

        if (t < 0.0f)
        {
            curve_shift--;
        }

        t -= curve_shift * max_t;

        if (curve_asset->clamp == xCC_OSCILLATE && abs(curve_shift % 2) == 1)
        {
            t = max_t - t;
        }
#endif
    }

    U32 last_point = t / curve_asset->delta;
    F32 u = (t - (last_point * curve_asset->delta)) / curve_asset->delta;

    return (1.0f - u) * curve_asset->points[last_point] + u * curve_asset->points[last_point + 1];
}

#if defined(PS2)
#pragma inline_intrinsics reset
#endif
