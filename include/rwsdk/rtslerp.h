#ifndef RTSLERP_H
#define RTSLERP_H

#include <rwsdk/rwcore.h>
#include <rwsdk/rtquat.h>
#include <rwsdk/rtslerp.h>

/* Masks for specifying which matrices to store by reference */
#define rtSLERPREFNONE 0x00
#define rtSLERPREFSTARTMAT 0x01
#define rtSLERPREFENDMAT 0x02
#define rtSLERPREFALL (~rtSLERPREFNONE)

typedef struct RtSlerp RtSlerp;

struct RtSlerp
{
    RwInt32 matRefMask; /* Which matrices do we NOT own */
    RwMatrix* startMat; /* The start matrix */
    RwMatrix* endMat; /* The end matrix */
    RwV3d axis; /* The axis of rotation for the slerp */
    RwReal angle; /* The angle (in degrees) between src & dest */
    RwBool useLerp; /* If true, lerps are used instead of slerps */
};

/* C compatibility: these headers use bare tag names as types. */
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

#define RtQuatSlerp(qpResult, qpFrom, qpTo, rT, sCache)                                            \
    RtQuatSlerpMacro(qpResult, qpFrom, qpTo, rT, sCache)

#ifdef __cplusplus
extern "C" {
#endif

extern RtSlerp* RtSlerpCreate(RwInt32 nMatRefMask);
extern void RtSlerpDestroy(RtSlerp* spSlerp);
extern RtSlerp* RtSlerpInitialize(RtSlerp* spSlerp, RwMatrix* mpMat1, RwMatrix* mpMat2);
extern RwMatrix* RtSlerpGetMatrix(RtSlerp* spSlerp, RwMatrix* mpResultMat, RwReal nDelta);
extern RtSlerp* RtSlerpSetLerp(RtSlerp* spSlerp, RwBool bUseLerp);

extern void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache);

#ifdef __cplusplus
}
#endif

#endif