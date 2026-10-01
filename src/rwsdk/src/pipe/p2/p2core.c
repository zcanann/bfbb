#include <rwsdk/rwcore.h>
#include <string.h>

#define rwPLUGIN_ID 1

#define RXNODEMAXOUTPUTS 32

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

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

typedef struct rwPipeGlobals rwPipeGlobals;
struct rwPipeGlobals
{
    RwFreeList* pipesFreeList;
    RxRenderStateVector defaultRenderState;
    RwLinkList allPipelines;
    RwUInt32 maxNodesPerPipe;
};

typedef struct RxExecutionContext RxExecutionContext;
struct RxExecutionContext
{
    RxPipeline* pipeline;
    RxPipelineNode* currentNode;
    RwInt32 exitCode;
    RwUInt32 pad;
    RxPipelineNodeParam params;
};

extern RwInt32 _rxPipelineGlobalsOffset;
extern RxExecutionContext _rxExecCtxGlobal;

#define RXPIPELINEGLOBAL(var)                                                                      \
    (RWPLUGINOFFSET(rwPipeGlobals, RwEngineInstance, _rxPipelineGlobalsOffset)->var)

RwInt32 _rxHeapInitialSize = 0x1000;
RwInt32 _rxPipelineMaxNodes = 64;

RxHeap* _rxHeapGlobal;
RwBool RxPipelineInstanced;

static RwInt32 _rxPipesFreeListBlockSize = 64;
static RwInt32 _rxPipesFreeListPreallocBlocks = 1;
static RwFreeList _rxPipesFreeList;

RwBool _rxPipelineClose(void)
{
    if (RxPipelineInstanced)
    {
        RwFreeListDestroy(RXPIPELINEGLOBAL(pipesFreeList));
        RXPIPELINEGLOBAL(pipesFreeList) = NULL;

        RxHeapDestroy(_rxHeapGlobal);
        _rxHeapGlobal = NULL;

        RxPipelineInstanced = FALSE;
    }

    return TRUE;
}

RwBool _rxPipelineOpen(void)
{
    if (!RxPipelineInstanced)
    {
        _rxHeapGlobal = RxHeapCreate(_rxHeapInitialSize);
        if (_rxHeapGlobal == NULL)
        {
            return FALSE;
        }

        RXPIPELINEGLOBAL(pipesFreeList) =
            RwFreeListCreateAndPreallocateSpace(sizeof(RxPipeline), _rxPipesFreeListBlockSize,
                                                sizeof(RwUInt32), _rxPipesFreeListPreallocBlocks,
                                                &_rxPipesFreeList);
        if (RXPIPELINEGLOBAL(pipesFreeList) == NULL)
        {
            RxHeapDestroy(_rxHeapGlobal);
            _rxHeapGlobal = NULL;
            return FALSE;
        }

        RXPIPELINEGLOBAL(maxNodesPerPipe) = _rxPipelineMaxNodes;

        RxRenderStateVectorSetDefaultRenderStateVector(&RXPIPELINEGLOBAL(defaultRenderState));

        RXPIPELINEGLOBAL(allPipelines).link.prev = NULL;
        RXPIPELINEGLOBAL(allPipelines).link.next = NULL;

        RxPipelineInstanced = TRUE;

        return TRUE;
    }

    return FALSE;
}

RxPipelineNode* PipelineNodeDestroy(RxPipelineNode* node, RxPipeline* pipeline)
{
    RwInt32 nodeIndex;
    RxPipelineNodeTopSortData* topSortData;
    RxPipelineNodeTopSortData* nextTopSortData;
    RwUInt32* output;
    RwUInt32* nextOutput;
    RwUInt32 i;
    RwUInt32 j;

    if (!pipeline->locked)
    {
        if (node->nodeDef->nodeMethods.pipelineNodeTerm != NULL)
        {
            node->nodeDef->nodeMethods.pipelineNodeTerm(node);
        }

        node->nodeDef->InputPipesCnt--;

        if (node->nodeDef->InputPipesCnt == 0)
        {
            if (node->nodeDef->nodeMethods.nodeTerm != NULL)
            {
                node->nodeDef->nodeMethods.nodeTerm(node->nodeDef);
            }

            if (node->nodeDef->editable)
            {
                RwFree(node->nodeDef);
                node->nodeDef = NULL;
            }
        }

        if (node->initializationData != NULL)
        {
            RwFree(node->initializationData);
            node->initializationData = NULL;
            node->initializationDataSize = 0;
        }

        memset(node, 0, sizeof(RxPipelineNode));
    }
    else
    {
        if (node->initializationData != NULL)
        {
            RwFree(node->initializationData);
            node->initializationData = NULL;
            node->initializationDataSize = 0;
        }

        if (node->nodeDef->InputPipesCnt == 0 && node->nodeDef->editable)
        {
            RwFree(node->nodeDef);
            node->nodeDef = NULL;
        }

        nodeIndex = node - pipeline->nodes;

        if (nodeIndex < pipeline->numNodes - 1)
        {
            output = (RwUInt32*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
            output += nodeIndex * RXNODEMAXOUTPUTS;
            nextOutput = output + RXNODEMAXOUTPUTS;
            for (i = nodeIndex; i < pipeline->numNodes - 1; i++)
            {
                memcpy(output, nextOutput, RXNODEMAXOUTPUTS * sizeof(RwUInt32));
                output = nextOutput;
                nextOutput += RXNODEMAXOUTPUTS;
            }

            output = (RwUInt32*)(pipeline->nodes + RXPIPELINEGLOBAL(maxNodesPerPipe));
            output += RXPIPELINEGLOBAL(maxNodesPerPipe) * RXNODEMAXOUTPUTS;
            topSortData = (RxPipelineNodeTopSortData*)output;
            nextTopSortData = topSortData + 1;
            for (i = nodeIndex; i < pipeline->numNodes - 1; i++)
            {
                memcpy(topSortData, nextTopSortData, sizeof(RxPipelineNodeTopSortData));
                topSortData = nextTopSortData;
                nextTopSortData++;
            }

            for (i = nodeIndex; i < pipeline->numNodes - 1; i++)
            {
                memcpy(&pipeline->nodes[i], &pipeline->nodes[i + 1], sizeof(RxPipelineNode));
                pipeline->nodes[i].outputs -= RXNODEMAXOUTPUTS;
                pipeline->nodes[i].topSortData--;
            }

            for (i = 0; i < pipeline->numNodes - 1; i++)
            {
                for (j = 0; j < pipeline->nodes[i].numOutputs; j++)
                {
                    if (pipeline->nodes[i].outputs[j] >= nodeIndex)
                    {
                        if (nodeIndex == pipeline->nodes[i].outputs[j])
                        {
                            pipeline->nodes[i].outputs[j] = (RwUInt32)-1;
                        }
                        else
                        {
                            pipeline->nodes[i].outputs[j]--;
                        }
                    }
                }
            }
        }
    }

    pipeline->numNodes--;

    return node;
}

RxHeap* RxHeapGetGlobalHeap(void)
{
    return _rxHeapGlobal;
}

static void ExecuteNode(RxPipeline* pipeline, RxPipelineNode* node, RxPipelineNodeParam* params)
{
    RwUInt32 exitCode;
    const RxNodeDefinition* nodeDef;

    nodeDef = node->nodeDef;
    exitCode = nodeDef->nodeMethods.nodeBody(node, params);
    if (!exitCode)
    {
        _rxExecCtxGlobal.exitCode = exitCode;
    }
}

RxPipeline* RxPipelineExecute(RxPipeline* pipeline, void* data, RwBool heapReset)
{
    if (heapReset && _rxHeapGlobal->dirty)
    {
        _rxHeapReset(_rxHeapGlobal);
    }

    _rxExecCtxGlobal.exitCode = TRUE;
    _rxExecCtxGlobal.pipeline = pipeline;
    _rxExecCtxGlobal.params.dataParam = data;
    _rxExecCtxGlobal.params.heap = _rxHeapGlobal;

    pipeline->embeddedPacketState = rxPKST_PACKETLESS;

    ExecuteNode(pipeline, pipeline->nodes, &_rxExecCtxGlobal.params);

    if (pipeline->embeddedPacketState > rxPKST_UNUSED)
    {
        pipeline->embeddedPacketState = rxPKST_INUSE;
        _rxPacketDestroy(pipeline->embeddedPacket);
    }

    _rxExecCtxGlobal.params.dataParam = NULL;
    _rxExecCtxGlobal.pipeline = NULL;
    _rxExecCtxGlobal.params.heap = NULL;

    if (_rxExecCtxGlobal.exitCode)
    {
        return pipeline;
    }

    return NULL;
}

RxPipeline* RxPipelineCreate(void)
{
    RxPipeline* pipeline;

    pipeline = (RxPipeline*)RwFreeListAlloc(RXPIPELINEGLOBAL(pipesFreeList));
    if (pipeline != NULL)
    {
        memset(pipeline, 0, sizeof(RxPipeline));
        pipeline->locked = FALSE;
        return pipeline;
    }

    RWERROR((E_RW_NOMEM, sizeof(RxPipeline)));
    return NULL;
}

void _rxPipelineDestroy(RxPipeline* Pipeline)
{
    RwUInt32 numNodes;
    RwUInt32 i;
    RxPipelineNode* Node;

    if (Pipeline != NULL)
    {
        Node = Pipeline->nodes;
        numNodes = Pipeline->numNodes;
        for (i = 0; i < numNodes; i++)
        {
            PipelineNodeDestroy(Node, Pipeline);
            Node++;
        }

        Pipeline->nodes = NULL;

        if (Pipeline->superBlock != NULL)
        {
            RwFree(Pipeline->superBlock);
            Pipeline->superBlock = NULL;
            Pipeline->superBlockSize = 0;
        }

        RwFreeListFree(RXPIPELINEGLOBAL(pipesFreeList), Pipeline);
    }
}
