#ifndef ILIGHT_H
#define ILIGHT_H

#include "xFColor.h"
#include "xMath3.h"
#include <rpworld.h>

// Complete PS2 retail iLight layout, 60 bytes.
struct iLight
{
    U32 type;
    RpLight* hw;
    xSphere sph;
    F32 radius_sq;
    _xFColor color;
    xVec3 dir;
    F32 coneangle;
};

#endif
