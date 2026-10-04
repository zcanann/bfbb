#include <rwsdk/rwcore.h>
#include <string.h>

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

#define E_RX_DEP_DEPENDENCIESMISMATCH 0x1d
#define E_RX_DEP_DUPLICATECLUSTERDEFS 0x1e
#define E_RX_DEP_NULLCLUSTERDEF 0x1f
#define E_RX_DEP_OUTOFMEMORY 0x20

typedef struct RwScopeTrace RwScopeTrace;
typedef struct RwReqEntry RwReqEntry;

struct RwScopeTrace
{
    RwReqEntry* head;
    RwScopeTrace* continuation;
    RwScopeTrace* next;
    RwScopeTrace* parent;
};

struct RwReqEntry
{
    RxClusterDefinition* clusterDef;
    RxClusterValidityReq required;
    RwUInt32 inputs;
    RwScopeTrace* st;
    RwReqEntry* next;
    rxReq* req;
    RwUInt32 outbf;
    RwUInt32 assignedslot;
    RxPipelineNode* originatingNode;
};

struct rxReq
{
    RwUInt32 numEntries;
    RwUInt32 maxEntries;
    RwUInt32 usedSlots;
    RwReqEntry* entries;
    RxPipelineNode* node;
};

typedef void (*RxEnumPipelineClustersCallBack)(RxClusterDefinition* clusterDef,
                                               RwUInt32 numPipelineClusters, void* data);

#define REQGETENTRY(_req, _n) (((_n) < (_req)->numEntries) ? &(_req)->entries[(_n)] : NULL)

extern void* StalacTiteAlloc(RwUInt32 size);
extern void* StalacMiteAlloc(RwUInt32 size);
extern RwUInt32 PipelineCalcNumUniqueClusters(RxPipeline* pipeline);
extern void _rx_rxRadixExchangeSort(void* elements, RwUInt32 numElements, RwUInt32 elementSize,
                                    RwUInt32 elementKeyOffset, RwUInt32 keyLo, RwUInt32 keyHi);

static rxReq* _ReqCreate(RxPipelineNode* node, RwUInt32 numClusters)
{
    rxReq* req;

    req = (rxReq*)StalacTiteAlloc(sizeof(rxReq));
    if (req != NULL)
    {
        req->entries = (RwReqEntry*)StalacTiteAlloc(numClusters * sizeof(RwReqEntry));
        if (req->entries != NULL)
        {
            req->maxEntries = numClusters;
            req->numEntries = 0;
            req->node = node;
            req->usedSlots = 0;

            return req;
        }
    }

    return NULL;
}

static RwReqEntry* _ReqSearch4Cluster(rxReq* req, RxClusterDefinition* clusterDef)
{
    RwReqEntry* reqentry;
    RwUInt32 n;

    if (req->numEntries > 0)
    {
        reqentry = req->entries;

        for (n = 0; n < req->numEntries; n++)
        {
            if (reqentry->clusterDef == clusterDef)
            {
                return reqentry;
            }

            reqentry++;
        }
    }

    return NULL;
}

static RwReqEntry* _ReqAddEntry(rxReq* req, RxClusterDefinition* clusterDef,
                                RxClusterValidityReq required, RwUInt32 inputs,
                                RxPipelineNode* originatingNode)
{
    RwReqEntry* reqentry;

    reqentry = &req->entries[req->numEntries];
    req->numEntries++;

    reqentry->clusterDef = clusterDef;
    reqentry->required = required;
    reqentry->inputs = inputs;
    reqentry->next = NULL;
    reqentry->req = req;
    reqentry->st = NULL;
    reqentry->outbf = 0;
    reqentry->assignedslot = (RwUInt32)-1;
    reqentry->originatingNode = originatingNode;

    return reqentry;
}

static RwReqEntry* _ReqMergeEntry(rxReq* req, RxClusterDefinition* clusterDef,
                                  RxClusterValidityReq required, RwUInt32 inputs,
                                  RxPipelineNode* originatingNode)
{
    RwReqEntry* reqentry;

    reqentry = _ReqSearch4Cluster(req, clusterDef);
    if (reqentry != NULL)
    {
        if (required == rxCLREQ_REQUIRED)
        {
            reqentry->required = rxCLREQ_REQUIRED;
            reqentry->originatingNode = originatingNode;
        }

        return reqentry;
    }

    return _ReqAddEntry(req, clusterDef, required, inputs, originatingNode);
}

static void _ReqDeleteEntry(rxReq* req, RwReqEntry* reqentry)
{
    if (reqentry != &req->entries[req->numEntries - 1])
    {
        *reqentry = req->entries[req->numEntries - 1];
    }

    req->numEntries--;
}

static RwUInt32 _IoSpecSearch4Cluster(RxIoSpec* iospec, RxClusterDefinition* clusterDef)
{
    RwUInt32 i;

    for (i = 0; i < iospec->numClustersOfInterest; i++)
    {
        if (iospec->clustersOfInterest[i].clusterDef == clusterDef)
        {
            return i;
        }
    }

    return (RwUInt32)-1;
}

static void _PropDownElimPath(RxPipeline* pipeline, RxPipelineNode* node,
                              RxClusterDefinition* clusterDef)
{
    RwUInt32 i;
    RxIoSpec* iospec;
    RwReqEntry* reqentry;
    RxOutputSpec* outspec;
    RwUInt32 n;
    RxClusterValid presinout;

    iospec = &node->nodeDef->io;

    reqentry = _ReqSearch4Cluster(node->topSortData->req, clusterDef);
    if (reqentry != NULL && --reqentry->inputs == 0)
    {
        _ReqDeleteEntry(node->topSortData->req, reqentry);

        for (i = 0; i < node->numOutputs; i++)
        {
            if (node->outputs[i] != (RwUInt32)-1)
            {
                outspec = &node->nodeDef->io.outputs[i];

                n = _IoSpecSearch4Cluster(iospec, clusterDef);
                if (n == (RwUInt32)-1)
                {
                    presinout = outspec->allOtherClusters;
                }
                else
                {
                    presinout = outspec->outputClusters[n];
                }

                if (presinout == rxCLVALID_NOCHANGE)
                {
                    _PropDownElimPath(pipeline, &pipeline->nodes[node->outputs[i]], clusterDef);
                }
            }
        }
    }
}

static RwScopeTrace* _ScopeTraceCreate(RwScopeTrace** headref)
{
    RwScopeTrace* scopetrace;

    scopetrace = (RwScopeTrace*)StalacTiteAlloc(sizeof(RwScopeTrace));
    if (scopetrace != NULL)
    {
        scopetrace->continuation = NULL;
        scopetrace->head = NULL;
        scopetrace->parent = NULL;
        scopetrace->next = *headref;
        *headref = scopetrace;

        return scopetrace;
    }

    return NULL;
}

static void _ScopeTraceAddEntry(RwScopeTrace* scopetrace, RwReqEntry* reqentry)
{
    reqentry->next = scopetrace->head;
    scopetrace->head = reqentry;
}

static void _ScopeTraceMerge(RwScopeTrace* p, RwScopeTrace* q, RwScopeTrace** headref)
{
    RwScopeTrace* pfinalpage;

    while (p->parent != NULL)
    {
        p = p->parent;
    }

    while (q->parent != NULL)
    {
        q = q->parent;
    }

    if (p != q)
    {
        pfinalpage = p;
        while (pfinalpage->continuation != NULL)
        {
            pfinalpage = pfinalpage->continuation;
        }

        pfinalpage->continuation = q;
        q->parent = p;

        while (*headref != q)
        {
            headref = &(*headref)->next;
        }

        *headref = (*headref)->next;
    }
}

static RwUInt32 _PropagateDependenciesAndKillDeadPaths(RxPipeline* pipeline)
{
    RxPipelineNode* node;
    RwUInt32 numUniqueClusters;
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 k;
    RxIoSpec* iospec;

    i = pipeline->numNodes;
    node = &pipeline->nodes[pipeline->numNodes - 1];

    do
    {
        iospec = &node->nodeDef->io;

        for (j = 0; j < iospec->numClustersOfInterest; j++)
        {
            RxClusterDefinition* cluster = iospec->clustersOfInterest[j].clusterDef;

            if (cluster == NULL)
            {
                RWERROR((E_RX_DEP_NULLCLUSTERDEF, node->nodeDef->name));
                return E_RX_DEP_NULLCLUSTERDEF;
            }

            for (k = j + 1; k < iospec->numClustersOfInterest; k++)
            {
                RxClusterDefinition* cluster2 = iospec->clustersOfInterest[k].clusterDef;

                if (cluster == cluster2)
                {
                    RWERROR((E_RX_DEP_DUPLICATECLUSTERDEFS, node->nodeDef->name, cluster2->name));
                    return E_RX_DEP_DUPLICATECLUSTERDEFS;
                }
            }
        }

        numUniqueClusters = PipelineCalcNumUniqueClusters(pipeline);

        if ((node->topSortData->req = _ReqCreate(node, numUniqueClusters)) == NULL)
        {
            RWERROR((E_RX_DEP_OUTOFMEMORY));
            return E_RX_DEP_OUTOFMEMORY;
        }

        for (j = 0; j < node->numOutputs; j++)
        {
            if (node->outputs[j] != (RwUInt32)-1)
            {
                RxPipelineNode* outnode = &pipeline->nodes[node->outputs[j]];
                RxOutputSpec* outspec = &node->nodeDef->io.outputs[j];

                for (k = 0; k < outnode->topSortData->req->numEntries; k++)
                {
                    RwReqEntry* reqentry = REQGETENTRY(outnode->topSortData->req, k);
                    RwUInt32 n;
                    RxClusterValid presinout;

                    n = _IoSpecSearch4Cluster(iospec, reqentry->clusterDef);
                    if (n == (RwUInt32)-1)
                    {
                        presinout = outspec->allOtherClusters;
                    }
                    else
                    {
                        presinout = outspec->outputClusters[n];
                    }

                    if (presinout == rxCLVALID_NOCHANGE)
                    {
                        if (_ReqMergeEntry(node->topSortData->req, reqentry->clusterDef,
                                           reqentry->required, node->topSortData->numIns,
                                           reqentry->originatingNode) == NULL)
                        {
                            RWERROR((E_RX_DEP_OUTOFMEMORY));
                            return E_RX_DEP_OUTOFMEMORY;
                        }
                    }
                    else if (reqentry->required == rxCLREQ_REQUIRED)
                    {
                        if (presinout != rxCLVALID_VALID)
                        {
                            RWERROR((E_RX_DEP_DEPENDENCIESMISMATCH, reqentry->clusterDef->name,
                                     reqentry->originatingNode->nodeDef->name, node->nodeDef->name,
                                     j, outspec->name));
                            return E_RX_DEP_DEPENDENCIESMISMATCH;
                        }
                    }
                    else if (presinout == rxCLVALID_INVALID)
                    {
                        _PropDownElimPath(pipeline, outnode, reqentry->clusterDef);
                    }
                }
            }
        }

        for (j = 0; j < iospec->numClustersOfInterest; j++)
        {
            if (iospec->inputRequirements[j] != rxCLREQ_DONTWANT)
            {
                if (_ReqMergeEntry(node->topSortData->req, iospec->clustersOfInterest[j].clusterDef,
                                   iospec->inputRequirements[j], node->topSortData->numIns,
                                   node) == NULL)
                {
                    RWERROR((E_RX_DEP_OUTOFMEMORY));
                    return E_RX_DEP_OUTOFMEMORY;
                }
            }
        }

        node--;
    } while (--i);

    return 0;
}

static RwUInt32 _ForAllNodeReqsAddOutputClustersAndBuildContinuityBitfields(RxPipeline* pipeline)
{
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 k;
    RxPipelineNode* node;
    RxIoSpec* iospec;

    i = pipeline->numNodes;
    node = pipeline->nodes;

    do
    {
        iospec = &node->nodeDef->io;

        for (j = 0; j < iospec->numClustersOfInterest; j++)
        {
            if (iospec->clustersOfInterest[j].forcePresent)
            {
                if (_ReqMergeEntry(node->topSortData->req, iospec->clustersOfInterest[j].clusterDef,
                                   rxCLREQ_DONTWANT, 1, node) == NULL)
                {
                    RWERROR((E_RX_DEP_OUTOFMEMORY));
                    return E_RX_DEP_OUTOFMEMORY;
                }
            }
        }

        for (j = 0; j < node->numOutputs; j++)
        {
            if (node->outputs[j] != (RwUInt32)-1)
            {
                RxPipelineNode* outnode = &pipeline->nodes[node->outputs[j]];
                RxOutputSpec* outspec = &node->nodeDef->io.outputs[j];

                for (k = 0; k < outnode->topSortData->req->numEntries; k++)
                {
                    RwReqEntry* reqentry = REQGETENTRY(outnode->topSortData->req, k);
                    RwUInt32 n;
                    RxClusterValid presinout;
                    RwReqEntry* drivingreqe;

                    n = _IoSpecSearch4Cluster(iospec, reqentry->clusterDef);
                    if (n == (RwUInt32)-1)
                    {
                        presinout = outspec->allOtherClusters;
                    }
                    else
                    {
                        presinout = outspec->outputClusters[n];
                    }

                    if (presinout == rxCLVALID_INVALID)
                    {
                        continue;
                    }

                    if (presinout != rxCLVALID_NOCHANGE)
                    {
                        drivingreqe = _ReqMergeEntry(node->topSortData->req, reqentry->clusterDef,
                                                     rxCLREQ_DONTWANT, 1, node);
                        if (drivingreqe == NULL)
                        {
                            RWERROR((E_RX_DEP_OUTOFMEMORY));
                            return E_RX_DEP_OUTOFMEMORY;
                        }
                    }
                    else
                    {
                        drivingreqe =
                            _ReqSearch4Cluster(node->topSortData->req, reqentry->clusterDef);
                    }

                    if (drivingreqe != NULL)
                    {
                        drivingreqe->outbf |= 1 << j;
                    }
                }
            }
        }

        node++;
    } while (--i);

    return 0;
}

static RwUInt32 _TraceClusterScopes(RxPipeline* pipeline, RwScopeTrace** stheadref)
{
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 k;
    RxPipelineNode* node;

    i = pipeline->numNodes;
    node = pipeline->nodes;

    do
    {
        for (j = 0; j < node->topSortData->req->numEntries; j++)
        {
            RwReqEntry* reqentry = REQGETENTRY(node->topSortData->req, j);

            if (reqentry->st == NULL)
            {
                if ((reqentry->st = _ScopeTraceCreate(stheadref)) == NULL)

                {
                    RWERROR((E_RX_DEP_OUTOFMEMORY));
                    return E_RX_DEP_OUTOFMEMORY;
                }

                _ScopeTraceAddEntry(reqentry->st, reqentry);
            }
        }

        for (k = 0; k < node->numOutputs; k++)
        {
            if (node->outputs[k] != (RwUInt32)-1)
            {
                RxPipelineNode* outnode = &pipeline->nodes[node->outputs[k]];

                for (j = 0; j < outnode->topSortData->req->numEntries; j++)
                {
                    RwReqEntry* reqentry = REQGETENTRY(outnode->topSortData->req, j);

                    if (reqentry->required != rxCLREQ_DONTWANT)
                    {
                        RwReqEntry* drivingreqe;

                        drivingreqe =
                            _ReqSearch4Cluster(node->topSortData->req, reqentry->clusterDef);
                        if (drivingreqe != NULL && (drivingreqe->outbf & (1 << k)))
                        {
                            if (reqentry->st == NULL)
                            {
                                _ScopeTraceAddEntry(drivingreqe->st, reqentry);
                                reqentry->st = drivingreqe->st;
                            }
                            else
                            {
                                _ScopeTraceMerge(reqentry->st, drivingreqe->st, stheadref);
                            }
                        }
                    }
                }
            }
        }

        node++;
    } while (--i);

    return 0;
}

static RwUInt32 _AssignClusterSlots(RxPipeline* pipeline, RwScopeTrace** stheadref)
{
    RwUInt32 numslotsused;
    RwScopeTrace* st;

    numslotsused = 0;

    for (st = *stheadref; st != NULL; st = st->next)
    {
        RwScopeTrace* st2;
        RwUInt32 unavailslots;
        RwUInt32 assignedslot;

        unavailslots = 0;

        st2 = st;
        do
        {
            RwReqEntry* reqentry;

            for (reqentry = st2->head; reqentry != NULL; reqentry = reqentry->next)
            {
                unavailslots |= reqentry->req->usedSlots;
            }

            st2 = st2->continuation;
        } while (st2 != NULL);

        assignedslot = 0;
        while (unavailslots & 1)
        {
            unavailslots >>= 1;
            assignedslot++;
        }

        if (assignedslot >= numslotsused)
        {
            numslotsused = assignedslot + 1;
        }

        st2 = st;
        do
        {
            RwReqEntry* reqentry;

            for (reqentry = st2->head; reqentry != NULL; reqentry = reqentry->next)
            {
                reqentry->assignedslot = assignedslot;
                reqentry->req->usedSlots |= 1 << assignedslot;
            }

            st2 = st2->continuation;
        } while (st2 != NULL);
    }

    pipeline->packetNumClusterSlots = numslotsused;

    return 0;
}

static RwUInt32 _EnumPipelineClusters(RwScopeTrace* head, RxEnumPipelineClustersCallBack fp,
                                      void* callbackprivatedata)
{
    RwUInt32 numpipelineclusters;
    RwScopeTrace* st;
    RwScopeTrace* stdedupe;

    numpipelineclusters = 0;

    for (st = head; st != NULL; st = st->next)
    {
        for (stdedupe = head; stdedupe != st; stdedupe = stdedupe->next)
        {
            if (stdedupe->head->clusterDef == st->head->clusterDef)
            {
                break;
            }
        }

        if (stdedupe == st)
        {
            if (fp != NULL)
            {
                fp(st->head->clusterDef, numpipelineclusters, callbackprivatedata);
            }

            numpipelineclusters++;
        }
    }

    return numpipelineclusters;
}

static RwUInt32 _CountHeadNodeRqdsAndOpts(RxPipeline* pipeline)
{
    RxPipelineNode* node;
    RwUInt32 i;
    RwUInt32 count;
    RwReqEntry* reqentry;

    node = pipeline->nodes;
    count = 0;

    for (i = 0; i < node->topSortData->req->numEntries; i++)
    {
        reqentry = REQGETENTRY(node->topSortData->req, i);

        if (reqentry->required == rxCLREQ_REQUIRED || reqentry->required == rxCLREQ_OPTIONAL)
        {
            count++;
        }
    }

    return count;
}

static void _WriteHeadNodeRqdsAndOpts2PipelineRequirements(RxPipeline* pipeline)
{
    RxPipelineNode* node;
    RwUInt32 i;
    RwUInt32 count;
    RwReqEntry* reqentry;
    RxPipelineRequiresCluster* reqcl;

    node = pipeline->nodes;
    count = 0;

    for (i = 0; i < node->topSortData->req->numEntries; i++)
    {
        reqentry = REQGETENTRY(node->topSortData->req, i);

        if (reqentry->required == rxCLREQ_REQUIRED || reqentry->required == rxCLREQ_OPTIONAL)
        {
            reqcl = &pipeline->inputRequirements[count++];

            reqcl->clusterDef = reqentry->clusterDef;
            reqcl->rqdOrOpt = reqentry->required;
            reqcl->slotIndex = reqentry->assignedslot;
        }
    }

    pipeline->numInputRequirements = count;
}

static void _MyEnumPipelineClustersCallBack(RxClusterDefinition* clusterdef,
                                            RwUInt32 numPipelineClusters, void* data)
{
    RxPipelineCluster** array;
    RxPipelineCluster* pipelineCluster;

    array = (RxPipelineCluster**)data;

    pipelineCluster = (RxPipelineCluster*)StalacMiteAlloc(sizeof(RxPipelineCluster));
    if (*array == NULL)
    {
        *array = pipelineCluster;
    }

    pipelineCluster->clusterRef = clusterdef;
    pipelineCluster->creationAttributes = clusterdef->defaultAttributes;
}

static RwUInt32 _ForAllNodesWriteClusterAllocations(RxPipeline* pipeline, RwScopeTrace* stHead)
{
    RwUInt32 numPipelineRequiresClusters;
    RwUInt32 numPipelineClusters;
    RxPipelineCluster* pipelineClusters;
    RxPipelineNode* node;
    RwUInt8* titeBase;
    RwUInt8* miteBase;
    RwUInt32 slotsContinueAND;
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 k;

    numPipelineRequiresClusters = _CountHeadNodeRqdsAndOpts(pipeline);
    numPipelineClusters = _EnumPipelineClusters(stHead, NULL, NULL);

    titeBase = (RwUInt8*)StalacTiteAlloc(0);
    miteBase = (RwUInt8*)StalacMiteAlloc(0);
    memset(miteBase, 0, titeBase - miteBase);

    pipeline->embeddedPacket = (RxPacket*)StalacMiteAlloc(
        sizeof(RxPacket) + (pipeline->packetNumClusterSlots - 1) * sizeof(RxCluster));

    pipeline->embeddedPacket->numClusters = (RwUInt16)pipeline->packetNumClusterSlots;
    pipeline->embeddedPacket->pipeline = pipeline;
    pipeline->embeddedPacketState = rxPKST_PACKETLESS;

    pipelineClusters = NULL;

    _EnumPipelineClusters(stHead, _MyEnumPipelineClustersCallBack, &pipelineClusters);

    for (i = 0; i < pipeline->numNodes; i++)
    {
        node = &pipeline->nodes[i];

        if (pipeline->packetNumClusterSlots != 0)
        {
            node->slotClusterRefs = (RxPipelineCluster**)StalacMiteAlloc(
                pipeline->packetNumClusterSlots * sizeof(RxPipelineCluster*));
        }

        node->slotsContinue =
            (RwUInt32*)StalacMiteAlloc((pipeline->packetNumClusterSlots + 1) * sizeof(RwUInt32));

        if (node->nodeDef->io.numClustersOfInterest != 0)
        {
            node->inputToClusterSlot = (RwUInt32*)StalacMiteAlloc(
                node->nodeDef->io.numClustersOfInterest * sizeof(RwUInt32));
        }

        if (node->nodeDef->pipelineNodePrivateDataSize != 0)
        {
            node->privateData =
                StalacMiteAlloc((node->nodeDef->pipelineNodePrivateDataSize + 3) & ~3);
        }

        slotsContinueAND = (RwUInt32)-1;

        for (j = 0; j < node->topSortData->req->numEntries; j++)
        {
            RxPipelineCluster* pipelineCluster = NULL;
            RwReqEntry* reqEntry = REQGETENTRY(node->topSortData->req, j);

            for (k = 0; k < numPipelineClusters; k++)
            {
                if (pipelineClusters[k].clusterRef == reqEntry->clusterDef)
                {
                    pipelineCluster = &pipelineClusters[k];
                    break;
                }
            }

            node->slotClusterRefs[reqEntry->assignedslot] = pipelineCluster;
            node->slotsContinue[reqEntry->assignedslot + 1] = reqEntry->outbf;
            slotsContinueAND &= reqEntry->outbf;
        }

        node->slotsContinue[0] = slotsContinueAND;

        for (j = 0; j < node->nodeDef->io.numClustersOfInterest; j++)
        {
            node->inputToClusterSlot[j] = (RwUInt32)-1;

            for (k = 0; k < pipeline->packetNumClusterSlots; k++)
            {
                if (node->slotClusterRefs[k] != NULL &&
                    node->slotClusterRefs[k]->clusterRef ==
                        node->nodeDef->io.clustersOfInterest[j].clusterDef)
                {
                    node->inputToClusterSlot[j] = k;
                    break;
                }
            }
        }
    }

    if (numPipelineRequiresClusters != 0)
    {
        pipeline->inputRequirements = (RxPipelineRequiresCluster*)StalacMiteAlloc(
            numPipelineRequiresClusters * sizeof(RxPipelineRequiresCluster));
    }

    _WriteHeadNodeRqdsAndOpts2PipelineRequirements(pipeline);

    _rx_rxRadixExchangeSort(pipeline->inputRequirements, pipeline->numInputRequirements,
                            sizeof(RxPipelineRequiresCluster), 0, 0, 0xFFFFFFFF);

    return 0;
}

RwUInt32 _rxChaseDependencies(RxPipeline* pipeline)
{
    RwScopeTrace* sthead;
    RwUInt32 result;

    sthead = NULL;

    result = _PropagateDependenciesAndKillDeadPaths(pipeline);
    if (result == 0)
    {
        result = _ForAllNodeReqsAddOutputClustersAndBuildContinuityBitfields(pipeline);
        if (result == 0)
        {
            result = _TraceClusterScopes(pipeline, &sthead);
            if (result == 0)
            {
                result = _AssignClusterSlots(pipeline, &sthead);
                if (result == 0)
                {
                    result = _ForAllNodesWriteClusterAllocations(pipeline, sthead);
                }
            }
        }
    }

    return result;
}
