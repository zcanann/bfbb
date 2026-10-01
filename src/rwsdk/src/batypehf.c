#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame)
{
    RwObjectHasFrame* ohf = (RwObjectHasFrame*)object;

    if (rwObjectGetParent(ohf))
    {
        rwLinkListRemoveLLLink(&ohf->lFrame);
    }

    rwObjectSetParent(ohf, frame);

    if (frame)
    {
        rwLinkListAddLLLink(&frame->objectList, &ohf->lFrame);
        RwFrameUpdateObjects(frame);
    }
}

void _rwObjectHasFrameReleaseFrame(void* object)
{
    RwObjectHasFrame* ohf = (RwObjectHasFrame*)object;

    if (rwObjectGetParent(ohf))
    {
        rwLinkListRemoveLLLink(&ohf->lFrame);
    }
}
