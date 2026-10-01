#ifndef XLASERBOLT_INL
#define XLASERBOLT_INL

#include "xLaserBolt.h"

inline void xLaserBoltEmitter::perturb_dir(xVec3& dir, F32 rand_angle)
{
    xVec3 temp = { 0.0f, 0.0f, 0.0f };
    xMat3x3 mat;

    temp.x = (xurand() - 0.5f) * rand_angle;
    temp.y = (xurand() - 0.5f) * rand_angle;
    temp.z = (xurand() - 0.5f) * rand_angle;

    xMat3x3Euler(&mat, &temp);
    xMat3x3LMulVec(&dir, &mat, &dir);
}

#endif
