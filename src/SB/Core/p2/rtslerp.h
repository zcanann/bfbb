#ifndef PS2_RTSLERP_H
#define PS2_RTSLERP_H

// RenderWare quaternion slerp toolkit (rtslerp.h) subset used by the PS2
// platform layer.

#include <rwcore.h>
#include <rtquat.h>

#define _RW_S1 ((float)-1.6666667163e-01)
#define _RW_S2 ((float)8.3333337680e-03)
#define _RW_S3 ((float)-1.9841270114e-04)
#define _RW_S4 ((float)2.7557314297e-06)
#define _RW_S5 ((float)-2.5050759689e-08)
#define _RW_S6 ((float)1.5896910177e-10)

#define RwSinMinusPiToPiMacro(result, x)                                                           \
    do                                                                                             \
    {                                                                                              \
        const float z = x * x;                                                                     \
        const float v = z * x;                                                                     \
        const float r = (_RW_S2 + z * (_RW_S3 + z * (_RW_S4 + z * (_RW_S5 + z * _RW_S6))));        \
        result = x + v * (_RW_S1 + z * r);                                                         \
    } while (0)

typedef struct RtQuatSlerpCache RtQuatSlerpCache;
struct RtQuatSlerpCache
{
    RtQuat raFrom;
    RtQuat raTo;
    RwReal omega;
    RwBool nearlyZeroOm;
};

#define RtQuatSlerpMacro(qpResult, qpFrom, qpTo, rT, sCache)                                       \
    MACRO_START                                                                                    \
    {                                                                                              \
        if ((rT) <= ((RwReal)0))                                                                   \
        {                                                                                          \
            /* t is before start */                                                                \
            *(qpResult) = *(qpFrom);                                                               \
        }                                                                                          \
        else if (((RwReal)1) <= (rT))                                                              \
        {                                                                                          \
            /* t is after end */                                                                   \
            *(qpResult) = *(qpTo);                                                                 \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            /* ... so t must be in the interior then */                                            \
            /* Calc coefficients rSclFrom, rSclTo */                                               \
            RwReal rSclFrom = ((RwReal)1) - (rT);                                                  \
            RwReal rSclTo = (rT);                                                                  \
                                                                                                   \
            if (!((sCache)->nearlyZeroOm))                                                         \
            {                                                                                      \
                /* Standard case: slerp */                                                         \
                /* SLERPMESSAGE(("Neither nearly ZERO nor nearly PI")); */                         \
                                                                                                   \
                rSclFrom *= (sCache)->omega;                                                       \
                RwSinMinusPiToPiMacro(rSclFrom, rSclFrom);                                         \
                rSclTo *= (sCache)->omega;                                                         \
                RwSinMinusPiToPiMacro(rSclTo, rSclTo);                                             \
            }                                                                                      \
                                                                                                   \
            /* Calc final values */                                                                \
            RwV3dScaleMacro(&(qpResult)->imag, &(sCache)->raFrom.imag, rSclFrom);                  \
            RwV3dIncrementScaledMacro(&(qpResult)->imag, &(sCache)->raTo.imag, rSclTo);            \
            (qpResult)->real =                                                                     \
                ((sCache)->raFrom.real * rSclFrom) + ((sCache)->raTo.real * rSclTo);               \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

#define RtQuatSlerp(qpResult, qpFrom, qpTo, rT, sCache)                                                RtQuatSlerpMacro(qpResult, qpFrom, qpTo, rT, sCache)

extern "C" {
void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache);
}

#endif
