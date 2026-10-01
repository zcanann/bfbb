#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpcollis.h>
#include <rwsdk/rpcollbsptree.h>

#define rwID_COLLISPLUGIN 0x11D

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwID_COLLISPLUGIN;                                                 \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_NOMEM 0x80000013

/* The BSP tree and triangle map were allocated in the same block as the data */
#define rpCOLLISIONDATASINGLEBLOCK 0x01

typedef struct rpCollisionGlobals rpCollisionGlobals;
struct rpCollisionGlobals
{
    void* unused;
};

extern void _rpCollBSPTreeInit(RpCollBSPTree* tree, RwInt32 numLeafNodes);
extern RwInt32 _rpCollBSPTreeMemGetSize(RwInt32 numLeafNodes);
extern void _rpCollBSPTreeDestroy(RpCollBSPTree* tree);
extern RwInt32 _rpCollBSPTreeStreamGetSize(RpCollBSPTree* tree);
extern RpCollBSPTree* _rpCollBSPTreeStreamWrite(RpCollBSPTree* tree, RwStream* stream);
extern RpCollBSPTree* _rpCollBSPTreeStreamRead(RpCollBSPTree* tree, RwStream* stream);

RwInt32 _rpCollisionNumInstances;
RwInt32 _rpCollisionGlobalsOffset;
RwInt32 _rpCollisionAtomicDataOffset;
RwInt32 _rpCollisionGeometryDataOffset;
RwInt32 _rpCollisionWorldSectorDataOffset;

#define RPCOLLISIONDATA(_object, _offset) (*RWPLUGINOFFSET(RpCollisionData*, _object, _offset))

static void* CollisionOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    _rpCollisionNumInstances++;

    RWPLUGINOFFSET(rpCollisionGlobals, RwEngineInstance, _rpCollisionGlobalsOffset)->unused = NULL;

    return instance;
}

static void* CollisionClose(void* instance, RwInt32 offset, RwInt32 size)
{
    _rpCollisionNumInstances--;

    return instance;
}

static void CollisionDataFree(RpCollisionData* data)
{
    if (data)
    {
        if (!(data->flags & rpCOLLISIONDATASINGLEBLOCK))
        {
            _rpCollBSPTreeDestroy(data->tree);
        }

        RwFree(data);
    }
}

static void* CollisionDataDestroy(void* object, RwInt32 offset, RwInt32 size)
{
    RpCollisionData* data = RPCOLLISIONDATA(object, offset);

    if (data)
    {
        CollisionDataFree(data);

        RPCOLLISIONDATA(object, offset) = (RpCollisionData*)NULL;
    }

    return object;
}

static void* CollisionDataCreate(void* object, RwInt32 offset, RwInt32 size)
{
    RPCOLLISIONDATA(object, offset) = (RpCollisionData*)NULL;

    return object;
}

static void* CollisionAtomicInit(void* object, RwInt32 offset, RwInt32 size)
{
    RPCOLLISIONDATA(object, _rpCollisionAtomicDataOffset) = (RpCollisionData*)NULL;

    return object;
}

static RwStream* CollisionDataStreamWrite(RwStream* stream, RwInt32 binaryLength,
                                          const void* object, RwInt32 offset, RwInt32 size)
{
    RpCollisionData* data;
    RwInt32 i;
    RwUInt16* triangleMap;
    RwInt32 index;

    data = RPCOLLISIONDATA(object, offset);
    if (!data)
    {
        return stream;
    }

    if (!RwStreamWriteInt32(stream, (RwInt32*)&data->tree->numLeafNodes, sizeof(RwInt32)) ||
        !RwStreamWriteInt32(stream, &data->numTriangles, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!_rpCollBSPTreeStreamWrite(data->tree, stream))
    {
        return (RwStream*)NULL;
    }

    i = data->numTriangles;
    triangleMap = data->triangleMap;

    while (i--)
    {
        index = *triangleMap++;

        if (!RwStreamWriteInt32(stream, &index, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }
    }

    return stream;
}

static RwStream* CollisionDataStreamRead(RwStream* stream, RwInt32 binaryLength, void* object,
                                         RwInt32 offset, RwInt32 size)
{
    RpCollisionData* data;
    RwInt32 numLeafNodes;
    RwInt32 numTriangles;
    RwInt32 treeSize;
    RwInt32 dataSize;
    RwInt32 i;
    RwUInt16* triangleMap;
    RwInt32 index;

    if (binaryLength)
    {
        if (!RwStreamReadInt32(stream, &numLeafNodes, sizeof(RwInt32)) ||
            !RwStreamReadInt32(stream, &numTriangles, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }

        treeSize = _rpCollBSPTreeMemGetSize(numLeafNodes);
        dataSize = sizeof(RpCollisionData) + treeSize + numTriangles * sizeof(RwUInt16);

        data = (RpCollisionData*)RwMalloc(dataSize);
        if (!data)
        {
            RWERROR((E_RW_NOMEM, dataSize));
            return (RwStream*)NULL;
        }

        data->flags = rpCOLLISIONDATASINGLEBLOCK;
        data->tree = (RpCollBSPTree*)(data + 1);
        data->numTriangles = numTriangles;
        data->triangleMap = (RwUInt16*)((RwUInt8*)data->tree + treeSize);

        _rpCollBSPTreeInit(data->tree, numLeafNodes);

        if (!_rpCollBSPTreeStreamRead(data->tree, stream))
        {
            RwFree(data);
            return (RwStream*)NULL;
        }

        i = data->numTriangles;
        triangleMap = data->triangleMap;

        while (i--)
        {
            if (!RwStreamReadInt32(stream, &index, sizeof(RwInt32)))
            {
                RwFree(data);
                return (RwStream*)NULL;
            }

            *triangleMap++ = (RwUInt16)index;
        }

        RPCOLLISIONDATA(object, offset) = data;
    }

    return stream;
}

static RwInt32 CollisionDataStreamGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    RpCollisionData* data = RPCOLLISIONDATA(object, offset);

    if (data)
    {
        RwInt32 streamSize;

        streamSize = 2 * sizeof(RwInt32) + _rpCollBSPTreeStreamGetSize(data->tree);
        streamSize += data->numTriangles * sizeof(RwInt32);

        return streamSize;
    }

    return 0;
}

static RwBool CollisionWorldSectorPluginAttach(void)
{
    RwBool success;

    _rpCollisionGlobalsOffset = RwEngineRegisterPlugin(sizeof(rpCollisionGlobals), rwID_COLLISPLUGIN,
                                                       CollisionOpen, CollisionClose);
    success = (_rpCollisionGlobalsOffset >= 0);

    if (success)
    {
        _rpCollisionWorldSectorDataOffset =
            RpWorldSectorRegisterPlugin(sizeof(RpCollisionData*), rwID_COLLISPLUGIN,
                                        CollisionDataCreate, CollisionDataDestroy, NULL);
        success = (_rpCollisionWorldSectorDataOffset >= 0);

        if (success)
        {
            success = (RpWorldSectorRegisterPluginStream(rwID_COLLISPLUGIN, CollisionDataStreamRead,
                                                         CollisionDataStreamWrite,
                                                         CollisionDataStreamGetSize) >= 0);
        }
    }

    return success;
}

static RwBool CollisionGeometryPluginAttach(void)
{
    RwBool success;

    _rpCollisionAtomicDataOffset = RpAtomicRegisterPlugin(sizeof(RpCollisionData*), rwID_COLLISPLUGIN,
                                                          CollisionAtomicInit, NULL, NULL);
    success = (_rpCollisionAtomicDataOffset >= 0);

    if (success)
    {
        _rpCollisionGeometryDataOffset =
            RpGeometryRegisterPlugin(sizeof(RpCollisionData*), rwID_COLLISPLUGIN,
                                     CollisionDataCreate, CollisionDataDestroy, NULL);
        success = (_rpCollisionGeometryDataOffset >= 0);

        if (success)
        {
            success = (RpGeometryRegisterPluginStream(rwID_COLLISPLUGIN, CollisionDataStreamRead,
                                                      CollisionDataStreamWrite,
                                                      CollisionDataStreamGetSize) >= 0);
        }
    }

    return success;
}

RwBool RpCollisionPluginAttach(void)
{
    RwBool success;

    success = CollisionWorldSectorPluginAttach();

    if (success)
    {
        success = CollisionGeometryPluginAttach();
    }

    return success;
}
