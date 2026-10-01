#include <rwsdk/rwcore.h>

#define rxCLFLAGS_EXTERNAL 0x02

typedef struct RxExecutionContext RxExecutionContext;
struct RxExecutionContext
{
    RxPipeline* pipeline;
    RxPipelineNode* currentNode;
    RwInt32 exitCode;
    RwUInt32 pad;
    RxPipelineNodeParam params;
};

extern RxHeap* _rxHeapGlobal;

RxExecutionContext _rxExecCtxGlobal;

void _rxPacketDestroy(RxPacket* packet)
{
    RxPipeline* pipeline;
    RwUInt32 n;
    RxCluster* cl;

    pipeline = packet->pipeline;
    cl = &packet->clusters[0];
    pipeline->embeddedPacketState = rxPKST_UNUSED;

    n = packet->numClusters;
    do
    {
        if (cl->clusterRef != NULL)
        {
            if (cl->data != NULL && !(cl->flags & rxCLFLAGS_EXTERNAL))
            {
                RxHeapFree(_rxHeapGlobal, cl->data);
            }

            /* flags and stride are cleared together */
            *(RwUInt32*)&cl->flags = 0;
            cl->data = NULL;
            cl->numAlloced = 0;
            cl->numUsed = 0;
            cl->clusterRef = NULL;
        }

        cl++;
    } while (--n);

    packet->flags = 0;
}
