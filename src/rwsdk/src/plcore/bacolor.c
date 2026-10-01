#include <rwsdk/rwcore.h>

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

static RwModuleInfo colorModule;

void* _rwColorOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    colorModule.numInstances++;

    return instance;
}

void* _rwColorClose(void* instance, RwInt32 offset, RwInt32 size)
{
    colorModule.numInstances--;

    return instance;
}
