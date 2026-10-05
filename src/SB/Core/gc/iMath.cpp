#include "iMath.h"

#if defined(XBOX)
#include <cmath>
#else
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\cmath>
#endif

F32 isin(F32 x)
{
    return std::sinf(x);
}

#if !defined(INLINE) && !defined(XBOX)
float std::sinf(float x)
{
    return (float)sin((double)x);
}
#endif

F32 icos(F32 x)
{
    return std::cosf(x);
}

#if !defined(INLINE) && !defined(XBOX)
float std::cosf(float x)
{
    return (float)cos((double)x);
}
#endif

F32 itan(F32 x)
{
    return std::tanf(x);
}

#if !defined(INLINE) && !defined(XBOX)
float std::tanf(float x)
{
    return (float)tan((double)x);
}
#endif
