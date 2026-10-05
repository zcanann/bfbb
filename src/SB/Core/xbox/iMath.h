#ifndef IMATH_H
#define IMATH_H

#include <types.h>
#include <cmath>

#define iabs(x) ((F32)fabs(x))

inline F32 isin(F32 x)
{
    return std::sinf(x);
}

inline F32 icos(F32 x)
{
    return std::cosf(x);
}

inline F32 itan(F32 x)
{
    return std::tanf(x);
}

#endif
