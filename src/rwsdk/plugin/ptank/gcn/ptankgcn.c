#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpptank.h>

#define rwID_PTANKPLUGIN 0x12F

typedef void* (*RxGameCubeAllInOneInstanceCallBack)(void* object, void* pipeData);
typedef void* (*RxGameCubeAllInOneReinstanceCallBack)(void* object, void* pipeData);
typedef void (*RxGameCubeAllInOneRenderCallBack)(RwResEntry* repEntry, void* object,
                                                 RwUInt8 type, RwUInt32 flags);

extern RxPipelineNode* RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node,
                                                           RxGameCubeAllInOneRenderCallBack cb);
extern RxPipelineNode* _rxGameCubeAllInOneSetInstanceCallBack(RxPipelineNode* node,
                                                              RxGameCubeAllInOneInstanceCallBack cb);
extern RxPipelineNode*
_rxGameCubeAllInOneSetReinstanceCallBack(RxPipelineNode* node,
                                         RxGameCubeAllInOneReinstanceCallBack cb);

extern void _rxPTankGameCubeRenderCallBack(RwResEntry* repEntry, void* object, RwUInt8 type,
                                           RwUInt32 flags);

RxPipeline* _rxPTankGameCubeRenderPipeline;

void* PTankOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    RxPipeline* pipe;
    RxLockedPipe* lpipe;
    RxNodeDefinition* allInOne;
    RxPipelineNode* node;

    pipe = RxPipelineCreate();
    pipe->pluginId = rwID_PTANKPLUGIN;
    pipe->pluginData = 0;

    lpipe = RxPipelineLock(pipe);
    allInOne = RxNodeDefinitionGetGameCubeAtomicAllInOne();
    lpipe = RxLockedPipeAddFragment(lpipe, (RwUInt32*)NULL, allInOne, (RxNodeDefinition*)NULL);
    RxLockedPipeUnlock(lpipe);

    node = RxPipelineFindNodeByName(pipe, allInOne->name, (RxPipelineNode*)NULL, (RwInt32*)NULL);

    RxGameCubeAllInOneSetRenderCallBack(node, _rxPTankGameCubeRenderCallBack);
    _rxGameCubeAllInOneSetInstanceCallBack(node, (RxGameCubeAllInOneInstanceCallBack)NULL);
    _rxGameCubeAllInOneSetReinstanceCallBack(node, (RxGameCubeAllInOneReinstanceCallBack)NULL);

    _rxPTankGameCubeRenderPipeline = pipe;

    return instance;
}

void* PTankClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (_rxPTankGameCubeRenderPipeline)
    {
        _rxPipelineDestroy(_rxPTankGameCubeRenderPipeline);
        _rxPTankGameCubeRenderPipeline = (RxPipeline*)NULL;
    }

    return instance;
}
