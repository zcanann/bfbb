#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwFRAME 0

#define rwFRAMEPRIVATEHIERARCHYSYNCLTM 0x01
#define rwFRAMEPRIVATEHIERARCHYSYNCOBJ 0x02
#define rwFRAMEPRIVATESUBTREESYNCLTM 0x04
#define rwFRAMEPRIVATESUBTREESYNCOBJ 0x08

#define rwFRAMEPRIVATEHIERARCHYSYNC                                                                \
    (rwFRAMEPRIVATEHIERARCHYSYNCLTM | rwFRAMEPRIVATEHIERARCHYSYNCOBJ)
#define rwFRAMEPRIVATESUBTREESYNC (rwFRAMEPRIVATESUBTREESYNCLTM | rwFRAMEPRIVATESUBTREESYNCOBJ)

#define rwFRAMEALIGNMENT 4

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

#define rwMatrixInitialize(m, type) ((m)->flags = (RwUInt32)(type))

#define rwFrameSetDirty(_f)                                                                        \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwUInt8 privFlags = rwObjectGetPrivateFlags((_f)->root);                                   \
        if (!(privFlags & rwFRAMEPRIVATEHIERARCHYSYNC))                                            \
        {                                                                                          \
            rwLinkListAddLLLink(&RWSRCGLOBAL(dirtyFrameList), &((_f)->root->inDirtyListLink));     \
        }                                                                                          \
        rwObjectSetPrivateFlags((_f)->root, privFlags | rwFRAMEPRIVATEHIERARCHYSYNC);              \
        rwObjectSetPrivateFlags((_f), rwObjectGetPrivateFlags(_f) | rwFRAMEPRIVATESUBTREESYNC);    \
    }                                                                                              \
    MACRO_STOP

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rwFrameGlobals rwFrameGlobals;
struct rwFrameGlobals
{
    RwFreeList* frameFreeList;
};

#define RWFRAMEGLOBAL(var)                                                                         \
    (RWPLUGINOFFSET(rwFrameGlobals, RwEngineInstance, frameModule.globalsOffset)->var)

extern void _rwFrameSyncHierarchyLTM(RwFrame* frame);

static RwModuleInfo frameModule;

RwInt32 _rwFrameFreeListBlockSize = 50;
RwInt32 _rwFrameFreeListPreallocBlocks = 1;

static RwPluginRegistry frameTKList = { sizeof(RwFrame),         sizeof(RwFrame),        0, 0,
                                        (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

void* _rwFrameOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    static RwFreeList frameFreeList;

    frameModule.globalsOffset = offset;

    RWFRAMEGLOBAL(frameFreeList) =
        RwFreeListCreateAndPreallocateSpace(frameTKList.sizeOfStruct, _rwFrameFreeListBlockSize,
                                            rwFRAMEALIGNMENT, _rwFrameFreeListPreallocBlocks,
                                            &frameFreeList);

    if (!RWFRAMEGLOBAL(frameFreeList))
    {
        return NULL;
    }

    rwLinkListInitialize(&RWSRCGLOBAL(dirtyFrameList));

    frameModule.numInstances++;

    return instance;
}

void* _rwFrameClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWFRAMEGLOBAL(frameFreeList))
    {
        RwFreeListDestroy(RWFRAMEGLOBAL(frameFreeList));
        RWFRAMEGLOBAL(frameFreeList) = (RwFreeList*)NULL;
    }

    frameModule.numInstances--;

    return instance;
}

static void rwSetHierarchyRoot(RwFrame* frame, RwFrame* root)
{
    RwFrame* child;

    frame->root = root;

    for (child = frame->child; child; child = child->next)
    {
        rwSetHierarchyRoot(child, root);
    }
}

RwFrame* RwFrameCreate(void)
{
    RwFrame* frame;

    frame = (RwFrame*)RwFreeListAlloc(RWFRAMEGLOBAL(frameFreeList));
    if (!frame)
    {
        return (RwFrame*)NULL;
    }

    rwObjectInitialize(frame, rwFRAME, 0);
    rwLinkListInitialize(&frame->objectList);

    rwMatrixInitialize(&frame->modelling, rwMATRIXTYPEORTHONORMAL);
    RwMatrixSetIdentity(&frame->modelling);
    rwMatrixInitialize(&frame->ltm, rwMATRIXTYPEORTHONORMAL);
    RwMatrixSetIdentity(&frame->ltm);

    frame->child = (RwFrame*)NULL;
    frame->next = (RwFrame*)NULL;
    frame->root = frame;

    _rwPluginRegistryInitObject(&frameTKList, frame);

    return frame;
}

static void FrameDestroyRecurseDeInitLeaf(RwFrame* frame)
{
    _rwPluginRegistryDeInitObject(&frameTKList, frame);

    if (rwObjectTestPrivateFlags(frame, rwFRAMEPRIVATEHIERARCHYSYNC))
    {
        rwLinkListRemoveLLLink(&frame->inDirtyListLink);
    }
}

RwBool RwFrameDestroy(RwFrame* frame)
{
    RwFrame* child;

    _rwPluginRegistryDeInitObject(&frameTKList, frame);

    if (rwObjectGetParent(frame))
    {
        RwFrameRemoveChild(frame);
    }

    if (rwObjectTestPrivateFlags(frame, rwFRAMEPRIVATEHIERARCHYSYNC))
    {
        rwLinkListRemoveLLLink(&frame->inDirtyListLink);
    }

    child = frame->child;
    while (child)
    {
        rwObjectSetParent(child, NULL);
        child = child->next;
    }

    RwFreeListFree(RWFRAMEGLOBAL(frameFreeList), frame);

    return TRUE;
}

static void rwFrameDestroyRecurseDestroyLeaf(RwFrame* frame)
{
    FrameDestroyRecurseDeInitLeaf(frame);

    RwFreeListFree(RWFRAMEGLOBAL(frameFreeList), frame);
}

static void rwFrameDestroyRecurse(RwFrame* frame)
{
    RwFrame* child;
    RwFrame* next;

    if (frame)
    {
        child = frame->child;
        while (child)
        {
            next = child->next;
            rwFrameDestroyRecurse(child);
            child = next;
        }

        rwFrameDestroyRecurseDestroyLeaf(frame);
    }
}

RwBool RwFrameDestroyHierarchy(RwFrame* frame)
{
    rwFrameDestroyRecurse(frame);

    return TRUE;
}

RwFrame* RwFrameUpdateObjects(RwFrame* frame)
{
    rwFrameSetDirty(frame);

    return frame;
}

RwFrame* RwFrameAddChild(RwFrame* parent, RwFrame* child)
{
    if (rwObjectGetParent(child))
    {
        RwFrameRemoveChild(child);
    }

    child->next = parent->child;
    parent->child = child;
    rwObjectSetParent(child, parent);

    rwSetHierarchyRoot(child, parent->root);

    if (rwObjectTestPrivateFlags(child, rwFRAMEPRIVATEHIERARCHYSYNC))
    {
        rwLinkListRemoveLLLink(&child->inDirtyListLink);
        rwObjectSetPrivateFlags(child,
                                rwObjectGetPrivateFlags(child) & ~rwFRAMEPRIVATEHIERARCHYSYNC);
    }

    rwFrameSetDirty(child);

    return parent;
}

RwFrame* RwFrameRemoveChild(RwFrame* child)
{
    RwFrame* parent = (RwFrame*)rwObjectGetParent(child);
    RwFrame* curFrame = parent->child;

    if (curFrame == child)
    {
        parent->child = child->next;
    }
    else
    {
        while (curFrame->next != child)
        {
            curFrame = curFrame->next;
        }

        curFrame->next = child->next;
    }

    rwObjectSetParent(child, NULL);
    child->next = (RwFrame*)NULL;

    rwSetHierarchyRoot(child, child);

    rwFrameSetDirty(child);

    return child;
}

RwFrame* RwFrameForAllChildren(RwFrame* frame, RwFrameCallBack callBack, void* data)
{
    RwFrame* curFrame = frame->child;

    while (curFrame)
    {
        RwFrame* next = curFrame->next;

        if (!callBack(curFrame, data))
        {
            return frame;
        }

        curFrame = next;
    }

    return frame;
}

RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* v, RwOpCombineType combine)
{
    RwMatrixTranslate(&frame->modelling, v, combine);

    rwFrameSetDirty(frame);

    return frame;
}

RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* m, RwOpCombineType combine)
{
    RwMatrixTransform(&frame->modelling, m, combine);

    rwFrameSetDirty(frame);

    return frame;
}

RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, RwReal angle, RwOpCombineType combine)
{
    RwMatrixRotate(&frame->modelling, axis, angle, combine);

    rwFrameSetDirty(frame);

    return frame;
}

RwFrame* RwFrameOrthoNormalize(RwFrame* frame)
{
    RwMatrixOrthoNormalize(&frame->modelling, &frame->modelling);

    rwFrameSetDirty(frame);

    return frame;
}

RwMatrix* RwFrameGetLTM(RwFrame* frame)
{
    RwFrame* root = frame->root;

    if (rwObjectTestPrivateFlags(root, rwFRAMEPRIVATEHIERARCHYSYNCLTM))
    {
        _rwFrameSyncHierarchyLTM(root);
    }

    return &frame->ltm;
}

RwFrame* RwFrameGetRoot(const RwFrame* frame)
{
    return frame->root;
}

RwBool RwFrameDirty(const RwFrame* frame)
{
    return rwObjectTestPrivateFlags(frame->root, rwFRAMEPRIVATEHIERARCHYSYNC);
}

RwInt32 RwFrameRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&frameTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);
}
