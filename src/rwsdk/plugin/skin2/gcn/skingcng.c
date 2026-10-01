#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/plugin/skin2/skin.h"

extern RxPipeline* _rpSkinPipelineCreate(RwUInt32 type, void* instanceCB, void* reinstanceCB,
                                         void* renderCB);
extern void* _rpSkinInstanceCallback(void* object, void* pipeData);
extern void* _rpSkinAtomicReinstanceCallBack(void* object, void* pipeData);
extern void* _rpSkinRenderCallback(void* object, void* pipeData);

RwBool _rpSkinPipelinesCreate(RwUInt32 pipes)
{
    RxPipeline** pipelines = _rpSkinGlobals.platform.pipelines;

    if (pipes & rpSKINPIPELINESKINGENERIC)
    {
        pipelines[rpSKINTYPEGENERIC] =
            _rpSkinPipelineCreate(rpSKINTYPEGENERIC, _rpSkinInstanceCallback,
                                  _rpSkinAtomicReinstanceCallBack, _rpSkinRenderCallback);
    }

    return TRUE;
}

RwBool _rpSkinPipelinesDestroy(void)
{
    RxPipeline** pipelines = _rpSkinGlobals.platform.pipelines;

    if (pipelines[rpSKINTYPEGENERIC])
    {
        _rxPipelineDestroy(pipelines[rpSKINTYPEGENERIC]);
        pipelines[rpSKINTYPEGENERIC] = (RxPipeline*)NULL;
    }

    return TRUE;
}

RpAtomic* _rpSkinPipelinesAttach(RpAtomic* atomic, RpSkinType type)
{
    atomic->pipeline = _rpSkinGlobals.platform.pipelines[rpSKINTYPEGENERIC];

    return atomic;
}
