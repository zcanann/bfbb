#include <rwsdk/rwcore.h>
#include <rwsdk/rtanim.h>

#ifndef MAKECHUNKID
#define MAKECHUNKID(vendorID, chunkID) (((vendorID & 0xFFFFFF) << 8) | (chunkID & 0xFF))
#endif

#define rwID_ANIMTOOLKIT MAKECHUNKID(rwVENDORID_CRITERIONTK, 0xB7)

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwID_ANIMTOOLKIT;                                                  \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RT_ANIM_INTERP_IDINUSE 0
#define E_RT_ANIM_INTERP_BLOCKFULL 1

#define rtANIMMAXINTERPOLATORS 16

RtAnimInterpolatorInfo RtAnimInterpolatorInfoBlock[rtANIMMAXINTERPOLATORS];
RwInt32 RtAnimInterpolatorInfoBlockNumEntries;

RwBool RtAnimRegisterInterpolationScheme(RtAnimInterpolatorInfo* interpolatorInfo)
{
    RwInt32 i;

    if (RtAnimInterpolatorInfoBlockNumEntries < rtANIMMAXINTERPOLATORS)
    {
        for (i = 0; i < RtAnimInterpolatorInfoBlockNumEntries; i++)
        {
            if (RtAnimInterpolatorInfoBlock[i].typeID == interpolatorInfo->typeID)
            {
                RWERROR((E_RT_ANIM_INTERP_IDINUSE));
                return FALSE;
            }
        }

        RtAnimInterpolatorInfoBlock[RtAnimInterpolatorInfoBlockNumEntries] = *interpolatorInfo;
        RtAnimInterpolatorInfoBlockNumEntries++;

        return TRUE;
    }

    RWERROR((E_RT_ANIM_INTERP_BLOCKFULL));
    return FALSE;
}

RtAnimInterpolator* RtAnimInterpolatorCreate(RwInt32 numNodes, RwInt32 maxInterpKeyFrameSize)
{
    RtAnimInterpolator* anim;

    anim = (RtAnimInterpolator*)RwMalloc(sizeof(RtAnimInterpolator) +
                                         numNodes * maxInterpKeyFrameSize);

    anim->numNodes = numNodes;
    anim->pCurrentAnim = (RtAnimAnimation*)NULL;
    anim->pNextFrame = NULL;
    anim->currentTime = 0.0f;
    anim->pAnimCallBack = (RtAnimCallBack)NULL;
    anim->animCallBackTime = -1.0f;
    anim->pAnimCallBackData = NULL;
    anim->pAnimLoopCallBack = (RtAnimCallBack)NULL;
    anim->pAnimLoopCallBackData = NULL;
    anim->currentInterpKeyFrameSize = maxInterpKeyFrameSize;
    anim->currentAnimKeyFrameSize = -1;
    anim->maxInterpKeyFrameSize = maxInterpKeyFrameSize;
    anim->isSubInterpolator = FALSE;
    anim->offsetInParent = 0;
    anim->parentAnimation = anim;
    anim->keyFrameApplyCB = (RtAnimKeyFrameApplyCallBack)NULL;
    anim->keyFrameInterpolateCB = (RtAnimKeyFrameInterpolateCallBack)NULL;
    anim->keyFrameBlendCB = (RtAnimKeyFrameBlendCallBack)NULL;
    anim->keyFrameAddCB = (RtAnimKeyFrameAddCallBack)NULL;

    return anim;
}

void RtAnimInterpolatorDestroy(RtAnimInterpolator* anim)
{
    RwFree(anim);
}
