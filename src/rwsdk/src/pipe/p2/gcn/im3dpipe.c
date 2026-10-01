#include <rwsdk/rwcore.h>

#define rwID_COREPLUGIN 0x01

typedef struct rwIm3DRenderPipelines rwIm3DRenderPipelines;
struct rwIm3DRenderPipelines
{
    RxPipeline* triList;
    RxPipeline* triFan;
    RxPipeline* triStrip;
    RxPipeline* lineList;
    RxPipeline* polyLine;
    RxPipeline* pointList;
};

extern RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void);
extern RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void);

RwBool _rwIm3DCreatePlatformTransformPipeline(RxPipeline** globalPipe)
{
    RxPipeline* pipe;
    RxPipeline* lpipe;

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        pipe->pluginId = rwID_COREPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL)
        {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetGameCubeImmInstance(),
                                            NULL);
            pipe = RxLockedPipeUnlock(lpipe);
            if (pipe != NULL)
            {
                *globalPipe = pipe;
                RwIm3DSetTransformPipeline(pipe);
                return TRUE;
            }
        }

        _rxPipelineDestroy(pipe);
    }

    return FALSE;
}

void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline** globalPipe)
{
    RwIm3DSetTransformPipeline(NULL);

    if (*globalPipe != NULL)
    {
        _rxPipelineDestroy(*globalPipe);
        *globalPipe = NULL;
    }
}

void _rwIm3DDestroyPlatformRenderPipelines(rwIm3DRenderPipelines* globalPipes)
{
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPETRILIST);
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPETRIFAN);
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPETRISTRIP);
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPELINELIST);
    RwIm3DSetRenderPipeline(NULL, rwPRIMTYPEPOLYLINE);

    if (globalPipes->triList != NULL)
    {
        _rxPipelineDestroy(globalPipes->triList);
    }

    globalPipes->triList = NULL;
    globalPipes->triFan = NULL;
    globalPipes->triStrip = NULL;
    globalPipes->lineList = NULL;
    globalPipes->polyLine = NULL;
}

RwBool _rwIm3DCreatePlatformRenderPipelines(rwIm3DRenderPipelines* globalPipes)
{
    RxPipeline* pipe;
    RxPipeline* lpipe;

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        pipe->pluginId = rwID_COREPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL)
        {
            lpipe = RxLockedPipeAddFragment(lpipe, NULL, RxNodeDefinitionGetGameCubeSubmitNoLight(),
                                            NULL);
            pipe = RxLockedPipeUnlock(lpipe);
            if (pipe != NULL)
            {
                globalPipes->triList = pipe;
                globalPipes->triFan = pipe;
                globalPipes->triStrip = pipe;
                globalPipes->lineList = pipe;
                globalPipes->polyLine = pipe;

                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPETRILIST);
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPETRIFAN);
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPETRISTRIP);
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPELINELIST);
                RwIm3DSetRenderPipeline(pipe, rwPRIMTYPEPOLYLINE);

                return TRUE;
            }
        }

        _rxPipelineDestroy(pipe);
    }

    return FALSE;
}
