#include <rwsdk/rwcore.h>
#include <string.h>
#include <stdarg.h>

#define rwPLUGIN_ID 1

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_NOMEM 0x80000013
#define E_RW_NULLP 0x80000016
#define E_RX_CYCLICPIPELINE 0x1c
#define E_RX_UNSATISFIEDREQUIREMENTS 0x22
#define E_RX_INVALIDENTRYPOINT 0x24
#define E_RX_TOOMANYCLUSTERS 0x28
#define E_RX_TOOMANYOUTPUTS 0x29
#define E_RX_TOOMANYNODES 0x2a
#define E_RX_UNLOCKEDPIPE 0x34

#define RXNODEMAXOUTPUTS 32
#define RXNODEMAXCLUSTERSOFINTEREST 32

typedef struct rwPipeGlobals rwPipeGlobals;
struct rwPipeGlobals
{
    RwFreeList* pipesFreeList;
    RxRenderStateVector defaultRenderState;
    RwLinkList allPipelines;
    RwUInt32 maxNodesPerPipe;
};

extern RwInt32 _rxPipelineGlobalsOffset;

#define RXPIPELINEGLOBAL(var)                                                                      \
    (RWPLUGINOFFSET(rwPipeGlobals, RwEngineInstance, _rxPipelineGlobalsOffset)->var)

typedef struct P2MemoryLimits P2MemoryLimits;
struct P2MemoryLimits
{
    RwUInt8* StalacTiteBase;
    RwUInt8* StalacMiteBase;
};

typedef struct tagTopSortData tagTopSortData;
struct tagTopSortData
{
    RxPipeline* pipeline;
    RwUInt32 nodesArraySlot;
};

#define FIXUPPOINTER(_type, _ptr, _diff) ((_type)((RwUInt8*)(_ptr) + (_diff)))

extern RxPipelineNode* PipelineNodeDestroy(RxPipelineNode* node, RxPipeline* pipeline);
extern RwUInt32 _rxChaseDependencies(RxPipeline* pipeline);

static P2MemoryLimits gMemoryLimits = { NULL, NULL };

void* StalacTiteAlloc(RwUInt32 size)
{
    size = (size + 3) & ~3;

    gMemoryLimits.StalacTiteBase -= size;
    if (gMemoryLimits.StalacTiteBase < gMemoryLimits.StalacMiteBase)
    {
        gMemoryLimits.StalacTiteBase += size;
        RWERROR((E_RW_NOMEM, size));
        return NULL;
    }

    return gMemoryLimits.StalacTiteBase;
}

void* StalacMiteAlloc(RwUInt32 size)
{
    RwUInt8* mite;

    size = (size + 3) & ~3;

    mite = gMemoryLimits.StalacMiteBase + size;
    gMemoryLimits.StalacMiteBase = mite;
    if (mite > gMemoryLimits.StalacTiteBase)
    {
        gMemoryLimits.StalacMiteBase = mite - size;
        RWERROR((E_RW_NOMEM, size));
        return NULL;
    }

    return mite - size;
}

RwUInt32 PipelineCalcNumUniqueClusters(RxPipeline* pipeline)
{
    RxClusterDefinition* lastAddress;
    RxClusterDefinition* newAddress;
    RwUInt32 numUniqueClusters;
    RwUInt32 i;
    RwUInt32 j;
    RxNodeDefinition* nodeDef;
    RxClusterDefinition* address;

    numUniqueClusters = 0;
    newAddress = NULL;

    for (;;)
    {
        lastAddress = newAddress;
        newAddress = (RxClusterDefinition*)0xFFFFFFFF;

        for (i = 0; i < pipeline->numNodes; i++)
        {
            nodeDef = pipeline->nodes[i].nodeDef;

            for (j = 0; j < nodeDef->io.numClustersOfInterest; j++)
            {
                address = nodeDef->io.clustersOfInterest[j].clusterDef;
                if (address > lastAddress && address < newAddress)
                {
                    newAddress = address;
                }
            }
        }

        if (newAddress == (RxClusterDefinition*)0xFFFFFFFF)
        {
            break;
        }

        numUniqueClusters++;
    }

    return numUniqueClusters;
}

static __inline RwBool ReallocAndFixupSuperBlock(RxPipeline* pipeline, RwUInt32 newSize)
{
    void* oldBlock;
    void* newBlock;
    RwUInt32 i;
    RwUInt32 numNodes;
    RwInt32 diff;

    oldBlock = pipeline->superBlock;

    newBlock = RwRealloc(oldBlock, newSize);
    if (newBlock != NULL)
    {
        numNodes = pipeline->numNodes;
        diff = (RwUInt8*)newBlock - (RwUInt8*)oldBlock;

        pipeline->superBlock = newBlock;
        pipeline->superBlockSize = newSize;
        pipeline->nodes = (RxPipelineNode*)pipeline->superBlock;

        if (pipeline->embeddedPacket != NULL)
        {
            pipeline->embeddedPacket = FIXUPPOINTER(RxPacket*, pipeline->embeddedPacket, diff);
        }

        if (pipeline->inputRequirements != NULL)
        {
            pipeline->inputRequirements =
                FIXUPPOINTER(RxPipelineRequiresCluster*, pipeline->inputRequirements, diff);
        }

        for (i = 0; i < numNodes; i++)
        {
            if (pipeline->nodes[i].outputs != NULL)
            {
                pipeline->nodes[i].outputs =
                    FIXUPPOINTER(RwUInt32*, pipeline->nodes[i].outputs, diff);
            }

            if (pipeline->nodes[i].slotClusterRefs != NULL)
            {
                pipeline->nodes[i].slotClusterRefs =
                    FIXUPPOINTER(RxPipelineCluster**, pipeline->nodes[i].slotClusterRefs, diff);
            }

            if (pipeline->nodes[i].slotsContinue != NULL)
            {
                pipeline->nodes[i].slotsContinue =
                    FIXUPPOINTER(RwUInt32*, pipeline->nodes[i].slotsContinue, diff);
            }

            if (pipeline->nodes[i].privateData != NULL)
            {
                pipeline->nodes[i].privateData =
                    FIXUPPOINTER(void*, pipeline->nodes[i].privateData, diff);
            }

            if (pipeline->nodes[i].inputToClusterSlot != NULL)
            {
                pipeline->nodes[i].inputToClusterSlot =
                    FIXUPPOINTER(RwUInt32*, pipeline->nodes[i].inputToClusterSlot, diff);
            }

            if (pipeline->nodes[i].topSortData != NULL)
            {
                pipeline->nodes[i].topSortData =
                    FIXUPPOINTER(RxPipelineNodeTopSortData*, pipeline->nodes[i].topSortData, diff);
            }
        }
    }
    else
    {
        RWERROR((E_RW_NOMEM, newSize));
        return FALSE;
    }

    return TRUE;
}

static RwBool LockPipelineExpandData(RxPipeline* dstPipe, RxPipeline* srcPipe)
{
    RxPipelineNodeTopSortData* topSortArray;
    RwUInt32* outputs;
    RwInt32 i;

    if (dstPipe != srcPipe)
    {
        for (i = srcPipe->numNodes - 1; i >= 0; i--)
        {
            memcpy(&dstPipe->nodes[i], &srcPipe->nodes[i], sizeof(RxPipelineNode));

            dstPipe->nodes[i].slotClusterRefs = NULL;
            dstPipe->nodes[i].slotsContinue = NULL;
            dstPipe->nodes[i].privateData = NULL;
            dstPipe->nodes[i].inputToClusterSlot = NULL;

            if (dstPipe->nodes[i].initializationDataSize != 0)
            {
                dstPipe->nodes[i].initializationData =
                    RwMalloc(dstPipe->nodes[i].initializationDataSize);
                if (dstPipe->nodes[i].initializationData == NULL)
                {
                    RWERROR((E_RW_NOMEM, dstPipe->nodes[i].initializationDataSize));
                    return FALSE;
                }

                memcpy(dstPipe->nodes[i].initializationData, srcPipe->nodes[i].initializationData,
                       dstPipe->nodes[i].initializationDataSize);
            }
        }

        dstPipe->numNodes = srcPipe->numNodes;
    }

    outputs = (RwUInt32*)(dstPipe->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
    for (i = srcPipe->numNodes - 1; i >= 0; i--)
    {
        dstPipe->nodes[i].outputs = outputs + i * RXNODEMAXOUTPUTS;

        if (srcPipe->nodes[i].outputs != NULL)
        {
            memcpy(dstPipe->nodes[i].outputs, srcPipe->nodes[i].outputs,
                   RXNODEMAXOUTPUTS * sizeof(RwUInt32));
        }
    }

    topSortArray = (RxPipelineNodeTopSortData*)(outputs + RXPIPELINEGLOBAL(maxNodesPerPipe) *
                                                              RXNODEMAXOUTPUTS);
    for (i = 0; i < srcPipe->numNodes; i++)
    {
        topSortArray[i].numIns = 0;
        topSortArray[i].numInsVisited = 0;
        topSortArray[i].req = NULL;

        dstPipe->nodes[i].topSortData = &topSortArray[i];
    }

    return TRUE;
}

static RwUInt32 CalcNodesOutputsCompactedMemSize(RxPipeline* pipeline)
{
    RwUInt32 size;
    RwUInt32 i;

    size = pipeline->numNodes * sizeof(RxPipelineNode);

    for (i = 0; i < pipeline->numNodes; i++)
    {
        size += pipeline->nodes->numOutputs * sizeof(RwUInt32);
    }

    return size;
}

static RwUInt32 CalcUnlockPersistentMemSize(RxPipeline* pipeline, RwUInt32 numClusters)
{
    RwUInt32 blockSize;
    RxPipelineNode* node;
    RwUInt32 i;

    blockSize = numClusters * sizeof(RxPipelineCluster) +
                numClusters * sizeof(RxPipelineRequiresCluster) +
                pipeline->numNodes * numClusters * sizeof(RxPipelineCluster*) +
                pipeline->numNodes * (numClusters + 1) * sizeof(RwUInt32);

    node = pipeline->nodes;
    for (i = 0; i < pipeline->numNodes; i++)
    {
        if (node->nodeDef->pipelineNodePrivateDataSize != 0)
        {
            blockSize += node->nodeDef->pipelineNodePrivateDataSize;
        }

        blockSize += node->nodeDef->io.numClustersOfInterest * sizeof(RwUInt32);
        node++;
    }

    blockSize += sizeof(RxPacket) + (numClusters - 1) * sizeof(RxCluster);

    return blockSize;
}

static RwBool _NodeCreate(RxPipeline* pipeline, RxPipelineNode* node, RxNodeDefinition* nodespec)
{
    RxPipelineNodeTopSortData* topSortData;
    RwUInt32* outputs;
    RwBool result;
    RwUInt32 n;

    result = TRUE;
    n = nodespec->io.numOutputs;

    memset(node, 0, sizeof(RxPipelineNode));

    if (n > RXNODEMAXOUTPUTS)
    {
        RWERROR((E_RX_TOOMANYOUTPUTS));
        result = FALSE;
    }

    if (nodespec->io.numClustersOfInterest > RXNODEMAXCLUSTERSOFINTEREST)
    {
        RWERROR((E_RX_TOOMANYCLUSTERS));
        result = FALSE;
    }

    if (n >= RXPIPELINEGLOBAL(maxNodesPerPipe))
    {
        RWERROR((E_RX_TOOMANYNODES));
        result = FALSE;
    }

    if (result)
    {
        outputs = (RwUInt32*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
        outputs += pipeline->numNodes * RXNODEMAXOUTPUTS;

        node->outputs = outputs;
        node->numOutputs = n;

        for (n = 0; n < node->numOutputs; n++)
        {
            outputs[n] = (RwUInt32)-1;
        }

        topSortData =
            (RxPipelineNodeTopSortData*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
        topSortData =
            (RxPipelineNodeTopSortData*)((RwUInt32*)topSortData +
                                         RXPIPELINEGLOBAL(maxNodesPerPipe) * RXNODEMAXOUTPUTS);
        topSortData += pipeline->numNodes;

        topSortData->numIns = 0;
        topSortData->numInsVisited = 0;
        topSortData->req = NULL;

        node->topSortData = topSortData;
        node->initializationData = NULL;
        node->initializationDataSize = 0;
        node->nodeDef = nodespec;

        pipeline->numNodes++;
    }

    return result;
}

static void PipelineTallyInputs(RxPipeline* pipeline)
{
    RxPipelineNode* nodes;
    RwUInt32 i;
    RwInt32 j;
    RwUInt32* outputs;

    nodes = pipeline->nodes;

    for (i = 0; i < pipeline->numNodes; i++)
    {
        if (nodes->nodeDef != NULL && nodes->numOutputs != 0)
        {
            outputs = nodes->outputs;

            j = nodes->numOutputs;
            do
            {
                if ((RwInt32)*outputs != -1)
                {
                    pipeline->nodes[*outputs].topSortData->numIns++;
                }

                outputs++;
            } while (--j);
        }

        nodes++;
    }
}

static void PipelineTopSort(tagTopSortData* data, RwUInt32 nodeIndex)
{
    RxPipelineNode* curNode;
    RwUInt32 i;
    RwUInt32 j;

    i = data->nodesArraySlot;
    j = nodeIndex;

    if (i != j)
    {
        RwUInt32 tmpOutput;
        RwUInt32* outputsI;
        RwUInt32* outputsJ;
        RxPipelineNodeTopSortData tempTopSortData;
        RxPipelineNodeTopSortData* topSortDataI;
        RxPipelineNodeTopSortData* topSortDataJ;
        RxPipelineNode tmpNode;
        RwUInt32 k;
        RwUInt32 l;

        outputsI = data->pipeline->nodes[i].outputs;
        outputsJ = data->pipeline->nodes[j].outputs;

        for (k = 0; k < RXNODEMAXOUTPUTS; k++)
        {
            tmpOutput = outputsI[k];
            outputsI[k] = outputsJ[k];
            outputsJ[k] = tmpOutput;
        }

        data->pipeline->nodes[i].outputs = outputsJ;
        data->pipeline->nodes[j].outputs = outputsI;

        topSortDataI = data->pipeline->nodes[i].topSortData;
        topSortDataJ = data->pipeline->nodes[j].topSortData;

        tempTopSortData = *topSortDataI;
        *topSortDataI = *topSortDataJ;
        *topSortDataJ = tempTopSortData;

        data->pipeline->nodes[i].topSortData = topSortDataJ;
        data->pipeline->nodes[j].topSortData = topSortDataI;

        tmpNode = data->pipeline->nodes[i];
        data->pipeline->nodes[i] = data->pipeline->nodes[j];
        data->pipeline->nodes[j] = tmpNode;

        for (k = 0; k < data->pipeline->numNodes; k++)
        {
            RxPipelineNode* node = &data->pipeline->nodes[k];

            for (l = 0; l < node->numOutputs; l++)
            {
                if (i == node->outputs[l])
                {
                    node->outputs[l] = j;
                }
                else if (j == node->outputs[l])
                {
                    node->outputs[l] = i;
                }
            }
        }
    }

    curNode = &data->pipeline->nodes[data->nodesArraySlot];
    data->nodesArraySlot++;

    if (curNode->numOutputs != 0)
    {
        for (i = 0; i < curNode->numOutputs; i++)
        {
            RwUInt32 outIndex = curNode->outputs[i];

            if (outIndex != (RwUInt32)-1)
            {
                RxPipelineNode* outNode = &data->pipeline->nodes[outIndex];

                outNode->topSortData->numInsVisited++;
                if (outNode->topSortData->numIns == outNode->topSortData->numInsVisited)
                {
                    PipelineTopSort(data, outIndex);
                }
            }
        }
    }
}

static RwUInt32 PipelineNode2Index(RxPipeline* pipeline, RxPipelineNode* node)
{
    RwUInt32 nodeIndex;

    nodeIndex = node - pipeline->nodes;
    if (&pipeline->nodes[nodeIndex] == node && nodeIndex < pipeline->numNodes)
    {
        return nodeIndex;
    }

    return (RwUInt32)-1;
}

static __inline RxPipeline* PipelineUnlockTopSort(RxPipeline* pipeline)
{
    tagTopSortData data;
    RwUInt32 i;

    data.pipeline = pipeline;
    data.nodesArraySlot = 0;

    {
        RxPipelineNode* node = pipeline->nodes;

        for (i = 0; i < pipeline->numNodes; i++)
        {
            if (node->nodeDef != NULL)
            {
                node->topSortData->numInsVisited = 0;
                node->topSortData->numIns = 0;
            }

            node++;
        }
    }

    PipelineTallyInputs(pipeline);

    if (pipeline->nodes[pipeline->entryPoint].topSortData->numIns != 0)
    {
        RWERROR((E_RX_INVALIDENTRYPOINT));
        return NULL;
    }

    for (i = 0; i < pipeline->numNodes; i++)
    {
        if (i != pipeline->entryPoint && pipeline->nodes[i].topSortData->numIns == 0)
        {
            RWERROR((E_RX_UNSATISFIEDREQUIREMENTS));
            return NULL;
        }
    }

    PipelineTopSort(&data, pipeline->entryPoint);

    for (i = 0; i < pipeline->numNodes; i++)
    {
        if (pipeline->nodes[i].topSortData->numIns != pipeline->nodes[i].topSortData->numInsVisited)
        {
            RWERROR((E_RX_CYCLICPIPELINE));
            return NULL;
        }
    }

    pipeline->entryPoint = 0;

    return pipeline;
}

static RwUInt32* RxPipelineNodeFindOutputByIndex(RxPipelineNode* node, RwUInt32 outputindex)
{
    if (node != NULL && node->nodeDef != NULL && outputindex < node->numOutputs)
    {
        return &node->outputs[outputindex];
    }

    return NULL;
}

static RxPipelineNode* RxPipelineNodeFindInput(RxPipelineNode* node)
{
    if (node != NULL && node->nodeDef != NULL)
    {
        return node;
    }

    return NULL;
}

RxPipeline* RxLockedPipeUnlock(RxLockedPipe* pipeline)
{
    RwUInt32 numUniqueClusters;
    RwUInt32 unlockStartBlockSize;
    RwUInt32 unlockEndBlockSize;
    RwUInt32 topSortBlockSize;
    RwUInt32 depChaseBlockSize;
    RwUInt32 totalOutputs;
    RwUInt32* newOutputs;
    RwUInt32* outputs;
    RwBool error;
    RwUInt32 doneNodes;
    RwInt32 i;

    if (pipeline != NULL && pipeline->locked)
    {
        if (pipeline->numNodes != 0)
        {
            RxPipelineNodeTopSortData* newTopSortData;
            RxPipelineNodeTopSortData* topSortData;

            totalOutputs = 0;
            error = FALSE;
            doneNodes = 0;

            if (pipeline->entryPoint >= pipeline->numNodes ||
                pipeline->nodes[pipeline->entryPoint].nodeDef == NULL)
            {
                RWERROR((E_RX_INVALIDENTRYPOINT));
                return NULL;
            }

            numUniqueClusters = PipelineCalcNumUniqueClusters(pipeline);

            unlockEndBlockSize =
                RXPIPELINEGLOBAL(maxNodesPerPipe) * sizeof(RxPipelineNode) +
                RXPIPELINEGLOBAL(maxNodesPerPipe) * RXNODEMAXOUTPUTS * sizeof(RwUInt32) +
                RXPIPELINEGLOBAL(maxNodesPerPipe) * sizeof(RxPipelineNodeTopSortData);

            topSortBlockSize =
                CalcNodesOutputsCompactedMemSize(pipeline) +
                RXPIPELINEGLOBAL(maxNodesPerPipe) * sizeof(RxPipelineNodeTopSortData);

            depChaseBlockSize = pipeline->numNodes * numUniqueClusters * 0x24 +
                                pipeline->numNodes * numUniqueClusters * 0x10 +
                                pipeline->numNodes * 0x14;

            unlockStartBlockSize = depChaseBlockSize + topSortBlockSize +
                                   CalcUnlockPersistentMemSize(pipeline, numUniqueClusters);

            if (unlockStartBlockSize < unlockEndBlockSize)
            {
                unlockStartBlockSize = unlockEndBlockSize;
            }

            if (unlockStartBlockSize > pipeline->superBlockSize)
            {
                if (!ReallocAndFixupSuperBlock(pipeline, unlockStartBlockSize))
                {
                    return NULL;
                }
            }

            gMemoryLimits.StalacTiteBase = (RwUInt8*)pipeline->superBlock + unlockStartBlockSize;
            gMemoryLimits.StalacMiteBase = NULL;

            pipeline = PipelineUnlockTopSort(pipeline);
            if (pipeline == NULL)
            {
                return NULL;
            }

            newTopSortData = (RxPipelineNodeTopSortData*)((RwUInt8*)pipeline->superBlock +
                                                          unlockStartBlockSize) -
                             1;
            outputs = (RwUInt32*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
            topSortData = (RxPipelineNodeTopSortData*)(outputs + RXPIPELINEGLOBAL(maxNodesPerPipe) *
                                                                     RXNODEMAXOUTPUTS) +
                          (pipeline->numNodes - 1);

            for (i = pipeline->numNodes - 1; i >= 0; i--)
            {
                memcpy(newTopSortData, topSortData, sizeof(RxPipelineNodeTopSortData));
                pipeline->nodes[i].topSortData = newTopSortData;

                newTopSortData--;
                topSortData--;
            }

            newOutputs = (RwUInt32*)(pipeline->nodes + pipeline->numNodes);
            for (i = 0; i < pipeline->numNodes; i++)
            {
                if (pipeline->nodes[i].numOutputs == 0)
                {
                    pipeline->nodes[i].outputs = NULL;
                }
                else
                {
                    memcpy(newOutputs, outputs, pipeline->nodes[i].numOutputs * sizeof(RwUInt32));
                    pipeline->nodes[i].outputs = newOutputs;
                }

                totalOutputs += pipeline->nodes[i].numOutputs;
                newOutputs += pipeline->nodes[i].numOutputs;
                outputs += RXNODEMAXOUTPUTS;
            }

            gMemoryLimits.StalacMiteBase = (RwUInt8*)(outputs + totalOutputs);
            gMemoryLimits.StalacTiteBase = (RwUInt8*)topSortData;

            if (_rxChaseDependencies(pipeline) != 0)
            {
                return NULL;
            }

            if (!ReallocAndFixupSuperBlock(pipeline, gMemoryLimits.StalacMiteBase -
                                                         (RwUInt8*)pipeline->superBlock))
            {
                goto unlockFailed;
            }

            {
                for (i = 0; i < pipeline->numNodes; i++)
                {
                    pipeline->nodes[i].topSortData = NULL;
                }

                for (i = pipeline->numNodes - 1; i >= 0; i--)
                {
                    RxPipelineNode* node = &pipeline->nodes[i];
                    RxNodeDefinition* nodeDef = node->nodeDef;

                    if (nodeDef->InputPipesCnt++ == 0 && nodeDef->nodeMethods.nodeInit != NULL &&
                        !nodeDef->nodeMethods.nodeInit(nodeDef))
                    {
                        error = TRUE;
                        doneNodes = pipeline->numNodes - 1 - i;
                        break;
                    }

                    if (nodeDef->nodeMethods.pipelineNodeInit != NULL &&
                        !nodeDef->nodeMethods.pipelineNodeInit(node))
                    {
                        nodeDef->InputPipesCnt--;
                        if (nodeDef->InputPipesCnt == 0 && nodeDef->nodeMethods.nodeTerm != NULL)
                        {
                            nodeDef->nodeMethods.nodeTerm(nodeDef);
                        }

                        error = TRUE;
                        doneNodes = pipeline->numNodes - 1 - i;
                        break;
                    }
                }

                if (!error)
                {
                    for (i = pipeline->numNodes - 1; i >= 0; i--)
                    {
                        RxPipelineNode* node = &pipeline->nodes[i];

                        if (node->nodeDef->nodeMethods.pipelineNodeConfig != NULL &&
                            !node->nodeDef->nodeMethods.pipelineNodeConfig(node, pipeline))
                        {
                            doneNodes = pipeline->numNodes;
                            error = TRUE;
                            break;
                        }
                    }
                }

                if (error)
                {
                    for (i = pipeline->numNodes - doneNodes; i < pipeline->numNodes; i++)
                    {
                        RxNodeDefinition* nodeDef = pipeline->nodes[i].nodeDef;

                        if (nodeDef->nodeMethods.pipelineNodeTerm != NULL)
                        {
                            nodeDef->nodeMethods.pipelineNodeTerm(&pipeline->nodes[i]);
                        }

                        nodeDef->InputPipesCnt--;
                        if (nodeDef->InputPipesCnt == 0 && nodeDef->nodeMethods.nodeTerm != NULL)
                        {
                            nodeDef->nodeMethods.nodeTerm(nodeDef);
                        }
                    }

                    goto unlockFailed;
                }
            }
        }

        pipeline->locked = FALSE;
        return pipeline;
    }

    if (pipeline == NULL)
    {
        RWERROR((E_RW_NULLP));
    }
    else
    {
        RWERROR((E_RX_UNLOCKEDPIPE));
    }

    return NULL;

unlockFailed:
    LockPipelineExpandData(pipeline, pipeline);
    return NULL;
}

RxLockedPipe* RxPipelineLock(RxPipeline* pipeline)
{
    RwUInt32 lockedBlockSize;
    RwUInt32 n;

    if (!pipeline->locked)
    {
        lockedBlockSize = RXPIPELINEGLOBAL(maxNodesPerPipe) * sizeof(RxPipelineNode) +
                          RXPIPELINEGLOBAL(maxNodesPerPipe) * RXNODEMAXOUTPUTS * sizeof(RwUInt32) +
                          RXPIPELINEGLOBAL(maxNodesPerPipe) * sizeof(RxPipelineNodeTopSortData);

        if (pipeline->nodes != NULL)
        {
            if (lockedBlockSize > pipeline->superBlockSize)
            {
                if (!ReallocAndFixupSuperBlock(pipeline, lockedBlockSize))
                {
                    return NULL;
                }
            }

            if (!LockPipelineExpandData(pipeline, pipeline))
            {
                return NULL;
            }
        }
        else
        {
            pipeline->superBlock = RwMalloc(lockedBlockSize);
            if (pipeline->superBlock == NULL)
            {
                RWERROR((E_RW_NOMEM, lockedBlockSize));
                return NULL;
            }

            pipeline->superBlockSize = lockedBlockSize;
            pipeline->nodes = (RxPipelineNode*)pipeline->superBlock;
        }

        pipeline->locked = TRUE;

        if (pipeline->nodes != NULL)
        {
            for (n = 0; n < pipeline->numNodes; n++)
            {
                const RxNodeMethods* const nodeMethods = &pipeline->nodes[n].nodeDef->nodeMethods;

                if (nodeMethods->pipelineNodeTerm != NULL)
                {
                    nodeMethods->pipelineNodeTerm(&pipeline->nodes[n]);
                }

                if (--pipeline->nodes[n].nodeDef->InputPipesCnt == 0 &&
                    nodeMethods->nodeTerm != NULL)
                {
                    nodeMethods->nodeTerm(pipeline->nodes[n].nodeDef);
                }

                pipeline->nodes[n].slotClusterRefs = NULL;
            }
        }
    }

    return pipeline;
}

RxPipelineNode* RxPipelineFindNodeByName(RxPipeline* pipeline, const RwChar* name,
                                         RxPipelineNode* start, RwInt32* nodeIndex)
{
    RwBool check;
    RxPipelineNode* node;
    RwInt32 n;

    check = (pipeline != NULL && name != NULL && pipeline->numNodes != 0);
    if (check)
    {
        node = pipeline->nodes;
        n = pipeline->numNodes;

        if (start != NULL)
        {
            while (node != start && n > 0)
            {
                node++;
                n--;
            }

            node++;
            n--;
        }

        while (n > 0)
        {
            if (node->nodeDef != NULL && !rwstrcmp(node->nodeDef->name, name))
            {
                if (nodeIndex != NULL)
                {
                    *nodeIndex = pipeline->numNodes - n;
                }

                return node;
            }

            node++;
            n--;
        }
    }

    if (nodeIndex != NULL)
    {
        *nodeIndex = -1;
    }

    return NULL;
}

RxLockedPipe* RxLockedPipeAddFragment(RxLockedPipe* pipeline, RwUInt32* firstIndex,
                                      RxNodeDefinition* nodeDef0, ...)
{
    va_list va;
    RxNodeDefinition* nodeDef;
    RwUInt32 oldnumnodes;
    RwUInt32 fragLength;
    RwUInt32 n;
    RxPipelineNode* prevnode;

    if (pipeline != NULL && pipeline->locked)
    {
        va_start(va, nodeDef0);

        fragLength = 0;
        nodeDef = nodeDef0;
        while (nodeDef != NULL)
        {
            fragLength++;
            nodeDef = va_arg(va, RxNodeDefinition*);
        }

        if (fragLength != 0)
        {
            prevnode = NULL;
            oldnumnodes = pipeline->numNodes;

            if (oldnumnodes + fragLength > RXPIPELINEGLOBAL(maxNodesPerPipe))
            {
                RWERROR((E_RX_TOOMANYNODES));
                return NULL;
            }

            va_start(va, nodeDef0);

            nodeDef = nodeDef0;
            n = 0;
            while (nodeDef != NULL)
            {
                RxPipelineNode* node = &pipeline->nodes[oldnumnodes + n];

                if (!_NodeCreate(pipeline, node, nodeDef))
                {
                    break;
                }

                if (prevnode != NULL)
                {
                    if (!RxLockedPipeAddPath(pipeline, RxPipelineNodeFindOutputByIndex(prevnode, 0),
                                             RxPipelineNodeFindInput(node)))
                    {
                        PipelineNodeDestroy(node, pipeline);
                        break;
                    }
                }

                prevnode = node;
                n++;
                nodeDef = va_arg(va, RxNodeDefinition*);
            }

            if (n == fragLength)
            {
                if (firstIndex != NULL)
                {
                    *firstIndex = oldnumnodes;
                }

                return pipeline;
            }

            while (n--)
            {
                PipelineNodeDestroy(&pipeline->nodes[n + oldnumnodes], pipeline);
            }
        }
    }
    else if (pipeline == NULL)
    {
        RWERROR((E_RW_NULLP));
    }
    else
    {
        RWERROR((E_RX_UNLOCKEDPIPE));
    }

    return NULL;
}

RxPipeline* RxLockedPipeAddPath(RxLockedPipe* pipeline, RxNodeOutput out, RxNodeInput in)
{
    RwUInt32 nodeIndex;

    if (pipeline != NULL && pipeline->locked && out != NULL && *out == (RwUInt32)-1 && in != NULL &&
        in->nodeDef != NULL)
    {
        nodeIndex = PipelineNode2Index(pipeline, in);
        if (nodeIndex != (RwUInt32)-1)
        {
            *out = nodeIndex;
            return pipeline;
        }
    }

    return NULL;
}
