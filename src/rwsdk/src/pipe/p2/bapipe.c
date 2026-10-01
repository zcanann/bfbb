#include <rwsdk/rwcore.h>

RwInt32 _rxPipelineGlobalsOffset;

void* _rwRenderPipelineOpen(void* instance, RwInt32 offset)
{
    _rxPipelineGlobalsOffset = offset;

    if (_rxPipelineOpen())
    {
        return instance;
    }

    return NULL;
}

void* _rwRenderPipelineClose(void* instance)
{
    _rxPipelineClose();

    return instance;
}

RwBool _rwPipeAttach(void)
{
    return TRUE;
}

void _rwPipeInitForCamera(const RwCamera* camera)
{
}
