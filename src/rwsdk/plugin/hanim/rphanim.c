#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rtquat.h>
#include <rwsdk/rtanim.h>
#include <rwsdk/rphanim.h>
#include <string.h>

#define rwID_HANIMPLUGIN 0x11E

#define rpHANIMSTREAMCURRENTVERSION 0x100
#define rpHANIMSTDKEYFRAMETYPEID 0x1

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

typedef struct RpHAnimKeyFrame RpHAnimKeyFrame;
struct RpHAnimKeyFrame
{
    RpHAnimKeyFrame* prevFrame;
    RwReal time;
    RtQuat q;
    RwV3d t;
};

typedef struct RpHAnimInterpFrame RpHAnimInterpFrame;
struct RpHAnimInterpFrame
{
    RpHAnimKeyFrame* keyFrame1;
    RpHAnimKeyFrame* keyFrame2;
    RtQuat q;
    RwV3d t;
};

typedef struct RpHAnimAtomicGlobalVars RpHAnimAtomicGlobalVars;
struct RpHAnimAtomicGlobalVars
{
    RwInt32 engineOffset;
    RwFreeList* HAnimFreeList;
};

typedef struct RpHAnimFrameExtension RpHAnimFrameExtension;
struct RpHAnimFrameExtension
{
    RwInt32 id;
    RpHAnimHierarchy* hierarchy;
};

#define RPHANIMFRAMEGETDATA(_frame) \
    (RWPLUGINOFFSET(RpHAnimFrameExtension, _frame, RpHAnimAtomicGlobals.engineOffset))

static RwInt32 _rpHAnimHierarchyFreeListBlockSize = 128;
static RwInt32 _rpHAnimHierarchyFreeListPreallocBlocks = 1;
static RwFreeList _rpHAnimHierarchyFreeList;

RpHAnimAtomicGlobalVars RpHAnimAtomicGlobals;

static void* HAnimOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    RtAnimInterpolatorInfo interpInfo;

    if (!(RpHAnimAtomicGlobals.HAnimFreeList = RwFreeListCreateAndPreallocateSpace(
              sizeof(RpHAnimHierarchy), _rpHAnimHierarchyFreeListBlockSize, 4,
              _rpHAnimHierarchyFreeListPreallocBlocks, &_rpHAnimHierarchyFreeList)))
    {
        instance = NULL;
    }

    interpInfo.typeID = rpHANIMSTDKEYFRAMETYPEID;
    interpInfo.interpKeyFrameSize = sizeof(RpHAnimInterpFrame);
    interpInfo.animKeyFrameSize = sizeof(RpHAnimKeyFrame);
    interpInfo.keyFrameApplyCB = RpHAnimKeyFrameApply;
    interpInfo.keyFrameBlendCB = RpHAnimKeyFrameBlend;
    interpInfo.keyFrameInterpolateCB = RpHAnimKeyFrameInterpolate;
    interpInfo.keyFrameAddCB = RpHAnimKeyFrameAdd;
    interpInfo.keyFrameMulRecipCB = RpHAnimKeyFrameMulRecip;
    interpInfo.keyFrameStreamReadCB = RpHAnimKeyFrameStreamRead;
    interpInfo.keyFrameStreamWriteCB = (RtAnimKeyFrameStreamWriteCallBack)RpHAnimKeyFrameStreamWrite;
    interpInfo.keyFrameStreamGetSizeCB =
        (RtAnimKeyFrameStreamGetSizeCallBack)RpHAnimKeyFrameStreamGetSize;
    interpInfo.customDataSize = 0;

    RtAnimRegisterInterpolationScheme(&interpInfo);

    return instance;
}

static void* HAnimClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RpHAnimAtomicGlobals.HAnimFreeList)
    {
        RwFreeListDestroy(RpHAnimAtomicGlobals.HAnimFreeList);
        RpHAnimAtomicGlobals.HAnimFreeList = (RwFreeList*)NULL;
    }

    return instance;
}

static void* HAnimConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* frameExt = RPHANIMFRAMEGETDATA(object);

    frameExt->hierarchy = (RpHAnimHierarchy*)NULL;
    frameExt->id = -1;

    return object;
}

static void* HAnimDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    RpHAnimFrameExtension* frameExt = RPHANIMFRAMEGETDATA(object);
    RpHAnimHierarchy* pHierarchy = frameExt->hierarchy;

    if (pHierarchy)
    {
        RwInt32 frameNum;

        for (frameNum = 0; frameNum < pHierarchy->numNodes; frameNum++)
        {
            pHierarchy->pNodeInfo[frameNum].pFrame = (RwFrame*)NULL;
        }

        if (pHierarchy->parentFrame == (RwFrame*)object)
        {
            RpHAnimHierarchyDestroy(pHierarchy);
        }

        frameExt->hierarchy = (RpHAnimHierarchy*)NULL;
    }

    frameExt->id = -1;

    return object;
}

static void* HAnimCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    RpHAnimHierarchy* srcHierarchy;
    const RpHAnimFrameExtension* srcFrameExt = RPHANIMFRAMEGETDATA(srcObject);
    RpHAnimFrameExtension* dstFrameExt = RPHANIMFRAMEGETDATA(dstObject);
    RpHAnimHierarchy* dstHierarchy;
    RwInt32 i;

    dstFrameExt->id = srcFrameExt->id;

    srcHierarchy = srcFrameExt->hierarchy;

    if (srcHierarchy && !(srcHierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY))
    {
        dstHierarchy = RpHAnimHierarchyCreate(
            srcHierarchy->numNodes, (RwUInt32*)NULL, (RwInt32*)NULL,
            (RpHAnimHierarchyFlag)srcHierarchy->flags,
            srcHierarchy->currentAnim->maxInterpKeyFrameSize);

        for (i = 0; i < dstHierarchy->numNodes; i++)
        {
            dstHierarchy->pNodeInfo[i].pFrame = (RwFrame*)NULL;
            dstHierarchy->pNodeInfo[i].flags = srcHierarchy->pNodeInfo[i].flags;
            dstHierarchy->pNodeInfo[i].nodeIndex = srcHierarchy->pNodeInfo[i].nodeIndex;
            dstHierarchy->pNodeInfo[i].nodeID = srcHierarchy->pNodeInfo[i].nodeID;
        }

        dstFrameExt->hierarchy = dstHierarchy;
        dstHierarchy->parentFrame = (RwFrame*)dstObject;
    }

    return dstObject;
}

static RwStream* HAnimWrite(RwStream* stream, RwInt32 binaryLength, const void* object,
                            RwInt32 offset, RwInt32 size)
{
    RwInt32 i;
    const RpHAnimFrameExtension* frameExt;
    RpHAnimNodeInfo* pNodeInfo;
    RpHAnimHierarchy* animHierarchy;
    RwInt32 streamVersion;
    RwInt32 numNodes;

    streamVersion = rpHANIMSTREAMCURRENTVERSION;
    if (!RwStreamWriteInt32(stream, &streamVersion, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    frameExt = RPHANIMFRAMEGETDATA(object);
    if (!RwStreamWriteInt32(stream, (RwInt32*)&frameExt->id, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    animHierarchy = frameExt->hierarchy;

    if (animHierarchy && !(animHierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY))
    {
        if (!RwStreamWriteInt32(stream, &animHierarchy->numNodes, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamWriteInt32(stream, &animHierarchy->flags, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamWriteInt32(stream, &animHierarchy->currentAnim->maxInterpKeyFrameSize,
                                sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        pNodeInfo = animHierarchy->pNodeInfo;

        for (i = 0; i < animHierarchy->numNodes; i++)
        {
            if (!RwStreamWriteInt32(stream, &pNodeInfo->nodeID, sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

            if (!RwStreamWriteInt32(stream, &pNodeInfo->nodeIndex, sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

            if (!RwStreamWriteInt32(stream, &pNodeInfo->flags, sizeof(RwInt32)))
            {
                return (RwStream*)NULL;
            }

            pNodeInfo++;
        }
    }
    else
    {
        numNodes = 0;
        if (!RwStreamWriteInt32(stream, &numNodes, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }
    }

    return stream;
}

static RwStream* HAnimRead(RwStream* stream, RwInt32 binaryLength, void* object, RwInt32 offset,
                           RwInt32 size)
{
    RwInt32 i;
    RwInt32 numNodes;
    RwInt32 streamVersion;
    RpHAnimNodeInfo* pNodeInfo;
    RpHAnimHierarchy* pHierarchy;
    RpHAnimFrameExtension* frameExt;
    RpHAnimHierarchyFlag flags;
    RwInt32 maxInterpKeyFrameSize;
    void* ptr;

    frameExt = RPHANIMFRAMEGETDATA(object);

    if (!RwStreamReadInt32(stream, &streamVersion, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (streamVersion != rpHANIMSTREAMCURRENTVERSION)
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamReadInt32(stream, &frameExt->id, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamReadInt32(stream, &numNodes, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (numNodes > 0)
    {
        if (!RwStreamReadInt32(stream, (RwInt32*)&flags, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        if (!RwStreamReadInt32(stream, &maxInterpKeyFrameSize, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        pHierarchy = (RpHAnimHierarchy*)RwFreeListAlloc(RpHAnimAtomicGlobals.HAnimFreeList);
        memset(pHierarchy, 0, sizeof(RpHAnimHierarchy));

        if (pHierarchy)
        {
            pHierarchy->currentAnim = RtAnimInterpolatorCreate(numNodes, maxInterpKeyFrameSize);
            pHierarchy->flags = flags;
            pHierarchy->parentFrame = (RwFrame*)object;
            pHierarchy->numNodes = numNodes;
            pHierarchy->parentHierarchy = pHierarchy;

            if (pHierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
            {
                pHierarchy->pMatrixArray = (RwMatrix*)NULL;
                pHierarchy->pMatrixArrayUnaligned = NULL;
            }
            else
            {
                ptr = RwMalloc(sizeof(RwMatrix) * numNodes + 15);
                pHierarchy->pMatrixArray = (RwMatrix*)(((RwUInt32)ptr + 15) & ~15);
                pHierarchy->pMatrixArrayUnaligned = ptr;
            }

            pHierarchy->pNodeInfo = (RpHAnimNodeInfo*)RwMalloc(sizeof(RpHAnimNodeInfo) * numNodes);

            pNodeInfo = pHierarchy->pNodeInfo;

            for (i = 0; i < pHierarchy->numNodes; i++)
            {
                if (!RwStreamReadInt32(stream, &pNodeInfo->nodeID, sizeof(RwInt32)))
                {
                    return (RwStream*)NULL;
                }

                if (!RwStreamReadInt32(stream, &pNodeInfo->nodeIndex, sizeof(RwInt32)))
                {
                    return (RwStream*)NULL;
                }

                if (!RwStreamReadInt32(stream, &pNodeInfo->flags, sizeof(RwInt32)))
                {
                    return (RwStream*)NULL;
                }

                pNodeInfo->pFrame = (RwFrame*)NULL;
                pNodeInfo++;
            }
        }

        frameExt->hierarchy = pHierarchy;
    }

    return stream;
}

static RwInt32 HAnimSize(const void* object, RwInt32 offset, RwInt32 size)
{
    RwBool needToStream;
    const RpHAnimFrameExtension* frameExt = RPHANIMFRAMEGETDATA(object);
    RwInt32 streamSize;

    needToStream = (frameExt->id != -1 || frameExt->hierarchy);

    if (needToStream)
    {
        streamSize = 3 * sizeof(RwInt32);

        if (frameExt->hierarchy &&
            !(frameExt->hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY))
        {
            streamSize += 2 * sizeof(RwInt32) +
                          frameExt->hierarchy->numNodes * (3 * sizeof(RwInt32));
        }

        return streamSize;
    }

    return 0;
}

RwBool RpHAnimPluginAttach(void)
{
    RwInt32 streamOffset;
    RwBool success;

    if (RwEngineRegisterPlugin(0, rwID_HANIMPLUGIN, HAnimOpen, HAnimClose) < 0)
    {
        return FALSE;
    }

    RpHAnimAtomicGlobals.engineOffset =
        RwFrameRegisterPlugin(sizeof(RpHAnimFrameExtension), rwID_HANIMPLUGIN, HAnimConstructor,
                              HAnimDestructor, HAnimCopy);

    streamOffset = RwFrameRegisterPluginStream(rwID_HANIMPLUGIN, HAnimRead, HAnimWrite, HAnimSize);

    success = FALSE;

    if (streamOffset >= 0 && RpHAnimAtomicGlobals.engineOffset >= 0)
    {
        success = TRUE;
    }

    return success;
}

RpHAnimHierarchy* RpHAnimHierarchyCreate(RwInt32 numNodes, RwUInt32* nodeFlags, RwInt32* nodeIDs,
                                         RpHAnimHierarchyFlag flags, RwInt32 maxInterpKeyFrameSize)
{
    void* ptr;
    RpHAnimHierarchy* pHierarchy;
    RwInt32 node;

    ptr = RwFreeListAlloc(RpHAnimAtomicGlobals.HAnimFreeList);
    pHierarchy = (RpHAnimHierarchy*)ptr;

    pHierarchy->currentAnim = RtAnimInterpolatorCreate(numNodes, maxInterpKeyFrameSize);
    pHierarchy->flags = flags;
    pHierarchy->numNodes = numNodes;
    pHierarchy->parentFrame = (RwFrame*)NULL;

    if (!(flags & rpHANIMHIERARCHYNOMATRICES))
    {
        ptr = RwMalloc(sizeof(RwMatrix) * numNodes + 15);
        pHierarchy->pMatrixArray = (RwMatrix*)(((RwUInt32)ptr + 15) & ~15);
        pHierarchy->pMatrixArrayUnaligned = ptr;
    }
    else
    {
        pHierarchy->pMatrixArray = (RwMatrix*)NULL;
        pHierarchy->pMatrixArrayUnaligned = NULL;
    }

    pHierarchy->pNodeInfo = (RpHAnimNodeInfo*)RwMalloc(sizeof(RpHAnimNodeInfo) * numNodes);

    for (node = 0; node < numNodes; node++)
    {
        pHierarchy->pNodeInfo[node].pFrame = (RwFrame*)NULL;

        if (nodeIDs)
        {
            pHierarchy->pNodeInfo[node].nodeID = nodeIDs[node];
        }

        pHierarchy->pNodeInfo[node].nodeIndex = node;

        if (nodeFlags)
        {
            pHierarchy->pNodeInfo[node].flags = nodeFlags[node];
        }
    }

    pHierarchy->parentHierarchy = pHierarchy;

    return pHierarchy;
}

RpHAnimHierarchy* RpHAnimHierarchyDestroy(RpHAnimHierarchy* hierarchy)
{
    RwFrame* parentFrame = hierarchy->parentFrame;

    if (!(hierarchy->flags & rpHANIMHIERARCHYSUBHIERARCHY))
    {
        if (!(hierarchy->flags & rpHANIMHIERARCHYNOMATRICES))
        {
            RwFree(hierarchy->pMatrixArrayUnaligned);
        }

        RwFree(hierarchy->pNodeInfo);
    }

    hierarchy->pMatrixArrayUnaligned = NULL;
    hierarchy->pMatrixArray = (RwMatrix*)NULL;
    hierarchy->pNodeInfo = (RpHAnimNodeInfo*)NULL;

    RtAnimInterpolatorDestroy(hierarchy->currentAnim);

    RwFreeListFree(RpHAnimAtomicGlobals.HAnimFreeList, hierarchy);

    if (parentFrame)
    {
        RPHANIMFRAMEGETDATA(parentFrame)->hierarchy = (RpHAnimHierarchy*)NULL;
    }

    return (RpHAnimHierarchy*)NULL;
}

RwBool RpHAnimFrameSetHierarchy(RwFrame* frame, RpHAnimHierarchy* hierarchy)
{
    RpHAnimFrameExtension* frameExt = RPHANIMFRAMEGETDATA(frame);

    if (frameExt->hierarchy)
    {
        frameExt->hierarchy->parentFrame = (RwFrame*)NULL;
    }

    frameExt->hierarchy = hierarchy;

    if (hierarchy)
    {
        hierarchy->parentFrame = frame;
    }

    return TRUE;
}

RpHAnimHierarchy* RpHAnimFrameGetHierarchy(RwFrame* frame)
{
    return RPHANIMFRAMEGETDATA(frame)->hierarchy;
}
