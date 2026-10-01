#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwFRAMEPRIVATEHIERARCHYSYNCLTM 0x01
#define rwFRAMEPRIVATEHIERARCHYSYNCOBJ 0x02
#define rwFRAMEPRIVATESUBTREESYNCLTM 0x04
#define rwFRAMEPRIVATESUBTREESYNCOBJ 0x08

#define rwFRAMEPRIVATEHIERARCHYSYNC                                                                \
    (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ)
#define rwFRAMEPRIVATESUBTREESYNC (rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ)

/* Tell every object attached to the frame that it has moved */
#define rwFrameSyncObjects(_frame)                                                                     MACRO_START                                                                                        {                                                                                                      if (!rwLinkListEmpty(&(_frame)->objectList))                                                       {                                                                                                      RwLLLink* current = rwLinkListGetFirstLLLink(&(_frame)->objectList);                               RwLLLink* end = rwLinkListGetTerminator(&(_frame)->objectList);                                                                                                                                       while (current != end)                                                                             {                                                                                                      RwObjectHasFrame* object = rwLLLinkGetData(current, RwObjectHasFrame, lFrame);                                                                                                                        object->sync(object);                                                                              current = rwLLLinkGetNext(current);                                                            }                                                                                              }                                                                                              }                                                                                                  MACRO_STOP

static void FrameSyncHierarchyRecurse(RwFrame* frame, RwInt32 flags)
{
    while (frame)
    {
        RwInt32 accumflags = flags | rwObjectGetPrivateFlags(frame);

        if (accumflags & rwFRAMEPRIVATESUBTREESYNCLTM)
        {
            RwMatrixMultiply(&frame->ltm, &frame->modelling,
                             &((RwFrame*)rwObjectGetParent(frame))->ltm);
        }

        rwFrameSyncObjects(frame);

        rwObjectSetPrivateFlags(frame, rwObjectGetPrivateFlags(frame) & ~rwFRAMEPRIVATESUBTREESYNC);

        FrameSyncHierarchyRecurse(frame->child, accumflags);

        frame = frame->next;
    }
}

static void FrameSyncHierarchyRecurseNoLTM(RwFrame* frame)
{
    while (frame)
    {
        rwFrameSyncObjects(frame);

        rwObjectSetPrivateFlags(frame,
                                rwObjectGetPrivateFlags(frame) & ~rwFRAMEPRIVATESUBTREESYNCOBJ);

        FrameSyncHierarchyRecurseNoLTM(frame->child);

        frame = frame->next;
    }
}

static void FrameSyncHierarchy(RwFrame* frame)
{
    RwInt32 oldFlags = rwObjectGetPrivateFlags(frame);

    if (oldFlags & rwFRAMEPRIVATEHIERARCHYSYNCLTM)
    {
        if (oldFlags & rwFRAMEPRIVATESUBTREESYNCLTM)
        {
            RwMatrixCopy(&frame->ltm, &frame->modelling);
        }

        rwFrameSyncObjects(frame);

        FrameSyncHierarchyRecurse(frame->child, oldFlags & rwFRAMEPRIVATESUBTREESYNCLTM);
    }
    else
    {
        rwFrameSyncObjects(frame);

        FrameSyncHierarchyRecurseNoLTM(frame->child);
    }

    rwObjectSetPrivateFlags(frame,
                            oldFlags & ~(rwFRAMEPRIVATEHIERARCHYSYNC | rwFRAMEPRIVATESUBTREESYNC));
}

RwBool _rwFrameSyncDirty(void)
{
    RwLLLink* lpFrameCur;
    RwLLLink* lpFrameEnd;

    lpFrameCur = rwLinkListGetFirstLLLink(&RWSRCGLOBAL(dirtyFrameList));
    lpFrameEnd = rwLinkListGetTerminator(&RWSRCGLOBAL(dirtyFrameList));
    while (lpFrameCur != lpFrameEnd)
    {
        RwFrame* rootFrame = rwLLLinkGetData(lpFrameCur, RwFrame, inDirtyListLink);

        FrameSyncHierarchy(rootFrame);

        lpFrameCur = rwLLLinkGetNext(lpFrameCur);
    }

    rwLinkListInitialize(&RWSRCGLOBAL(dirtyFrameList));

    return TRUE;
}

static void FrameSyncHierarchyLTMRecurse(RwFrame* frame, RwInt32 flags)
{
    while (frame)
    {
        RwInt32 accumflags = flags | rwObjectGetPrivateFlags(frame);

        if (accumflags & rwFRAMEPRIVATESUBTREESYNCLTM)
        {
            RwMatrixMultiply(&frame->ltm, &frame->modelling,
                             &((RwFrame*)rwObjectGetParent(frame))->ltm);

            rwObjectSetPrivateFlags(frame, rwObjectGetPrivateFlags(frame) &
                                               ~rwFRAMEPRIVATESUBTREESYNCLTM);
        }

        FrameSyncHierarchyLTMRecurse(frame->child, accumflags);

        frame = frame->next;
    }
}

void _rwFrameSyncHierarchyLTM(RwFrame* frame)
{
    RwInt32 oldFlags = rwObjectGetPrivateFlags(frame);

    if (oldFlags & rwFRAMEPRIVATESUBTREESYNCLTM)
    {
        RwMatrixCopy(&frame->ltm, &frame->modelling);
    }

    FrameSyncHierarchyLTMRecurse(frame->child, oldFlags);

    rwObjectSetPrivateFlags(frame, oldFlags & ~(rwFRAMEPRIVATEHIERARCHYSYNCLTM |
                                                rwFRAMEPRIVATESUBTREESYNCLTM));
}
