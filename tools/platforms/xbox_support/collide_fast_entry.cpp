// Reachability for the full fast-collision API. Host/dependencies get no credit.
#include "xCollideFast.h"

extern "C" U32 __cdecl xbox_source_entry(
    U32 which, xScene* scene, const xRay3* ray, const xSphere* sphere, const xBox* box)
{
    switch (which)
    {
    case 0: xCollideFastInit(scene); break;
    case 1: return xRayHitsSphereFast(ray, sphere);
    case 2: return xRayHitsBoxFast(ray, box);
    }
    return 0;
}
