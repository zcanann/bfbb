#ifndef XENTPOSITION_H
#define XENTPOSITION_H

#include "xEntTypes.h"
#include "xModelTypes.h"

inline xVec3* xEntGetPos(const xEnt* ent)
{
    return &((xMat4x3*)ent->model->Mat)->pos;
}

#endif
