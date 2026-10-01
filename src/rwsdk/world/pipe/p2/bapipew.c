#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/bapipew.h"

RxPipeline* RpWorldSetDefaultSectorPipeline(RxPipeline* pipeline)
{
    if (pipeline == NULL)
    {
        if (RXPIPELINEGLOBAL(platformWorldSectorPipeline) != NULL)
        {
            pipeline = RXPIPELINEGLOBAL(platformWorldSectorPipeline);
        }
        else
        {
            pipeline = NULL;
        }
    }

    RXPIPELINEGLOBAL(currentWorldSectorPipeline) = pipeline;

    return pipeline;
}

RxPipeline* RpAtomicSetDefaultPipeline(RxPipeline* pipeline)
{
    if (pipeline == NULL)
    {
        if (RXPIPELINEGLOBAL(platformAtomicPipeline) != NULL)
        {
            pipeline = RXPIPELINEGLOBAL(platformAtomicPipeline);
        }
        else
        {
            pipeline = NULL;
        }
    }

    RXPIPELINEGLOBAL(currentAtomicPipeline) = pipeline;

    return pipeline;
}

void _rpWorldPipelineClose(void)
{
    _rpDestroyPlatformWorldSectorPipelines();
    _rpDestroyPlatformAtomicPipelines();
    _rpDestroyPlatformMaterialPipelines();
}

RwBool _rpWorldPipelineOpen(void)
{
    RwBool success;

    success = _rpCreatePlatformMaterialPipelines();
    if (success)
    {
        success = _rpCreatePlatformAtomicPipelines();
    }
    if (success)
    {
        success = _rpCreatePlatformWorldSectorPipelines();
    }

    if (success)
    {
        return TRUE;
    }

    _rpWorldPipelineClose();
    return FALSE;
}

RwBool _rpWorldPipeAttach(void)
{
    return TRUE;
}
