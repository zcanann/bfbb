#ifndef AUTO_TWEAK_H
#define AUTO_TWEAK_H

#include "zEnt.h"

namespace auto_tweak
{
    template <class T1, class T2>
    void load_param(T1&, T2, T2, T2, xModelAssetParam*, U32, const char*);

    template <>
    inline void load_param<F32, F32>(F32& value, F32 scale, F32 lo, F32 hi, xModelAssetParam* ap,
                                   U32 apsize, const char* name)
    {
        value = zParamGetFloat(ap, apsize, name, value);
        if (value < lo)
        {
            value = lo;
        }
        else if (value > hi)
        {
            value = hi;
        }
        value = value * scale;
    }

    template <>
    inline void load_param<S32, S32>(S32& value, S32 scale, S32 lo, S32 hi, xModelAssetParam* ap,
                                   U32 apsize, const char* name)
    {
        S32 result = zParamGetInt(ap, apsize, name, value);
        if (result < lo)
        {
            result = lo;
        }
        else if (result > hi)
        {
            result = hi;
        }
        result *= scale;
        value = result;
    }
} // namespace auto_tweak

#endif
