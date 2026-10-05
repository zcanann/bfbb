// Actual complete curve source is retained; this host receives no credit.
#include "xCurveAsset.h"

extern "C" F32 __cdecl xbox_source_entry(const xCurveAsset* curve, F32 time)
{
    return xCurveAssetEvaluate(curve, time);
}
