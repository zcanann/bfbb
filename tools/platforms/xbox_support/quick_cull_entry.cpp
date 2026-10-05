#include "xQuickCull.h"
// Diagnostic reachability only; these APIs and dependencies are complete real TUs.
// Host and runtime bytes are excluded from matching coverage.
extern "C" S32 __cdecl xbox_source_entry(
    U32 which, xQCControl* ctrl, xQCData* q, const xQCData* q2,
    const xBound* bound, const xLine3* line, const xRay3* ray,
    const xSphere* sphere, const xBox* box, const xMat4x3* mat,
    F32 t, F32 u, F32 v, F32 w, F32 y, F32 z)
{
    switch (which)
    {
    case 0:
        xQuickCullInit(ctrl, t, u, v, w, y, z);
        break;
    case 1:
        xQuickCullInit(ctrl, box);
        break;
    case 2:
        return xQuickCullIsects(q, q2);
    case 3:
        xQuickCullForBound(ctrl, q, bound);
        break;
    case 4:
        xQuickCullForLine(ctrl, q, line);
        break;
    case 5:
        xQuickCullForRay(ctrl, q, ray);
        break;
    case 6:
        xQuickCullForSphere(ctrl, q, sphere);
        break;
    case 7:
        xQuickCullForBox(ctrl, q, box);
        break;
    case 8:
        xQuickCullForOBB(ctrl, q, box, mat);
        break;
    case 9:
        xQuickCullForEverything(q);
        break;
    }
    return 0;
}
