#ifndef BAPIPEW_H
#define BAPIPEW_H

#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

typedef struct rxPipelineGlobals rxPipelineGlobals;
struct rxPipelineGlobals
{
    RwUInt8 pad[0x3c];
    RxPipeline* currentAtomicPipeline;
    RxPipeline* currentWorldSectorPipeline;
    RxPipeline* currentMaterialPipeline;
    RxPipeline* genericAtomicPipeline;
    RxPipeline* genericWorldSectorPipeline;
    RxPipeline* genericMaterialPipeline;
    RxPipeline* platformAtomicPipeline;
    RxPipeline* platformWorldSectorPipeline;
    RxPipeline* platformMaterialPipeline;
};

extern RwInt32 _rxPipelineGlobalsOffset;

#define RXPIPELINEGLOBAL(var)                                                                      \
    (RWPLUGINOFFSET(rxPipelineGlobals, RwEngineInstance, _rxPipelineGlobalsOffset)->var)

/* Platform specific pipeline construction (wrldpipe.c) */
extern RwBool _rpCreatePlatformMaterialPipelines(void);
extern void _rpDestroyPlatformMaterialPipelines(void);
extern RwBool _rpCreatePlatformWorldSectorPipelines(void);
extern void _rpDestroyPlatformWorldSectorPipelines(void);
extern RwBool _rpCreatePlatformAtomicPipelines(void);
extern void _rpDestroyPlatformAtomicPipelines(void);

#endif
