#ifndef XMATHINLINES_H
#define XMATHINLINES_H

#include <types.h>
#include <math.h>

F32 xfmod(F32 a, F32 b);
F32 xatan2(F32 y, F32 x);
F32 xasin(F32 x);
F32 xacos(F32 x);
F32 xexp(F32 x);

inline F32 SQ(F32 x)
{
    return x * x;
}

inline F32 xpow(F32 x, F32 y)
{
    return std::powf(x, y);
}

inline U8 LERP(F32 x, U8 y, U8 z)
{
    return (U8)(x * (z - y)) + y;
}

inline F32 LERP(F32 x, F32 y, F32 z)
{
    return (x * (z - y)) + y;
}

inline F32 EASE(F32 rhs)
{
    return rhs * ((rhs * 3.0f) - (rhs * 2.0f) * rhs);
}

inline F32 SMOOTH(F32 x, F32 y, F32 z)
{
    return (z - y) * EASE(x) + y;
}

inline void xsqrtfast(F32& out, F32 x)
{
    out = std::sqrtf(x);
}

// Inline in retail: every caller's TU emits its own weak copy (and its pool
// literals); xBound.o owns the copy that survives linking.
inline F32 xsqrt(F32 x)
{
    const F32 half = 0.5f;
    const F32 three = 3.0f;

    if (x <= 0.0f || isinf(x))
    {
        return x;
    }

    F32 guess = __frsqrte(x);
    guess = half * guess * (three - guess * guess * x);

    if (guess > 0.0000099999997f)
    {
        return 1.0f / guess;
    }

    return 100000.0f;
}

#endif
