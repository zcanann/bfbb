#ifndef PS2_RTQUAT_H
#define PS2_RTQUAT_H

// RenderWare quaternion toolkit (rtquat.h) subset used by the PS2 platform layer.

#include <rwcore.h>

typedef struct RtQuat RtQuat;
struct RtQuat
{
    RwV3d imag;
    RwReal real;
};

#endif
