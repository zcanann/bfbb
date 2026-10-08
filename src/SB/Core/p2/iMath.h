#ifndef IMATH_H
#define IMATH_H

#include <math.h>
#include <types.h>

// PS2 absolute value is the single-instruction abs.s primitive. It is an
// asm volatile in the original platform header: retail turns off constant
// propagation, CSE and loop-test elision in every function that inlines it
// (unfolded NULL tests, dead blocks, recomputed clamps), which only an
// asm volatile in the inlined body reproduces.
inline F32 iabs_asm(F32 x)
{
    asm volatile("abs.s %0, %1" : "=f"(x) : "f"(x));
    return x;
}
#define iabs(x) iabs_asm((float)(x))

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
