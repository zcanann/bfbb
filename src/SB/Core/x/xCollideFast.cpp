#include "xCollideFast.h"
#include "iCollideFast.h"
#if defined(PS2)
#include "iMath3.h"
#endif
#include "xMath.h"

#include <types.h>

void xCollideFastInit(xScene* sc)
{
    iCollideFastInit(sc);
}

U32 xRayHitsSphereFast(const xRay3* r, const xSphere* s)
{
    xVec3 length;
    xVec3Sub(&length, &r->origin, &s->center);
    F32 dVar3 = xVec3Dot(&length, &length) - SQR(s->r);
    if (dVar3 <= 0.0f)
    {
        return 1;
    }

    if (r->flags & (1 << 11) && dVar3 > r->max_t * (2.0f * s->r + r->max_t))
    {
        return 0;
    }

    F32 dot = xVec3Dot(&length, &r->dir);
    if (dot >= 0.0f)
    {
        return 0;
    }
    return SQR(dot) >= dVar3;
}

U32 xRayHitsBoxFast(const xRay3* r, const xBox* b)
{
    xIsect isect;
    iBoxIsectRay(b, r, &isect);
    return isect.penned <= 0.0f || isect.contained <= 0.0f;
}
