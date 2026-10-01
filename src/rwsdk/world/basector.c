#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

extern void* _rpSectorOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpSectorClose(void* instance, RwInt32 offset, RwInt32 size);

static RwModuleInfo sectorModule;

RwPluginRegistry sectorTKList = { sizeof(RpWorldSector),   sizeof(RpWorldSector),  0, 0,
                                  (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

void* _rpSectorOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    sectorModule.numInstances++;

    return instance;
}

void* _rpSectorClose(void* instance, RwInt32 offset, RwInt32 size)
{
    sectorModule.numInstances--;

    return instance;
}

RwInt32 RpWorldSectorGetNumVertices(const RpWorldSector* sector)
{
    return sector->numVertices;
}

RpWorldSector* RpWorldSectorRender(RpWorldSector* sector)
{
    RpWorld* world = (RpWorld*)RWSRCGLOBAL(curWorld);

    return world->renderCallBack(sector);
}

RwInt32 RpWorldSectorRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                    RwPluginObjectConstructor constructCB,
                                    RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    RwInt32 plug;

    plug =
        _rwPluginRegistryAddPlugin(&sectorTKList, size, pluginID, constructCB, destructCB, copyCB);

    return plug;
}

RwInt32 RpWorldSectorRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                          RwPluginDataChunkWriteCallBack writeCB,
                                          RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPluginStream(&sectorTKList, pluginID, readCB, writeCB, getSizeCB);

    return plug;
}
