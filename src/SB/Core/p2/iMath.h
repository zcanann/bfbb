#ifndef IMATH_H
#define IMATH_H

#include <math.h>
#include <types.h>

#define iabs(x) __s_abs((float)(x))

inline F32 isin(F32 x)
{
    return sinf(x);
}

inline F32 icos(F32 x)
{
    return cosf(x);
}

inline F32 itan(F32 x)
{
    return tanf(x);
}

#endif
