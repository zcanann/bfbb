#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/bapipew.h"
#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

RwBool _rxWorldDevicePluginAttach(void)
{
    RwBool success;

    success = _rpDlVtxFmtPluginAttach();
    if (!success)
    {
        return FALSE;
    }

    success = _rpDlLightPluginAttach();

    return success ? TRUE : FALSE;
}

RwBool _rpCreatePlatformMaterialPipelines(void)
{
    return TRUE;
}

void _rpDestroyPlatformMaterialPipelines(void)
{
}

RwBool _rpCreatePlatformWorldSectorPipelines(void)
{
    RxPipeline* pipe;

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_WORLDPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL)
        {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL,
                                            RxNodeDefinitionGetGameCubeWorldSectorAllInOne(), NULL);

            lpipe = RxLockedPipeUnlock(lpipe);
            if (lpipe != NULL)
            {
                RXPIPELINEGLOBAL(platformWorldSectorPipeline) = pipe;
                RpWorldSetDefaultSectorPipeline(pipe);

                return TRUE;
            }
        }

        _rxPipelineDestroy(pipe);
    }

    return FALSE;
}

void _rpDestroyPlatformWorldSectorPipelines(void)
{
    RpWorldSetDefaultSectorPipeline(NULL);

    if (RXPIPELINEGLOBAL(platformWorldSectorPipeline) != NULL)
    {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(platformWorldSectorPipeline));
        RXPIPELINEGLOBAL(platformWorldSectorPipeline) = NULL;
    }
}

RwBool _rpCreatePlatformAtomicPipelines(void)
{
    RxPipeline* pipe;

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        RxLockedPipe* lpipe;

        pipe->pluginId = rwID_WORLDPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL)
        {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetGameCubeAtomicAllInOne(),
                                            NULL);

            lpipe = RxLockedPipeUnlock(lpipe);
            if (lpipe != NULL)
            {
                RXPIPELINEGLOBAL(platformAtomicPipeline) = pipe;
                RpAtomicSetDefaultPipeline(pipe);

                return TRUE;
            }
        }

        _rxPipelineDestroy(pipe);
    }

    return FALSE;
}

void _rpDestroyPlatformAtomicPipelines(void)
{
    RpAtomicSetDefaultPipeline(NULL);

    if (RXPIPELINEGLOBAL(platformAtomicPipeline) != NULL)
    {
        _rxPipelineDestroy(RXPIPELINEGLOBAL(platformAtomicPipeline));
        RXPIPELINEGLOBAL(platformAtomicPipeline) = NULL;
    }
}
