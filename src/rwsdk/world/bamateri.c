#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include <string.h>

#define rwPLUGIN_ID 2

#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define RwStreamWriteChunkHeader(stream, type, size)                                               \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, 0xFFFF)

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

#define E_RW_BADVERSION 0x80000004

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rpMaterialGlobals rpMaterialGlobals;
struct rpMaterialGlobals
{
    RwFreeList* matFreeList;
};

typedef struct RpMaterialChunkInfo RpMaterialChunkInfo;
struct RpMaterialChunkInfo
{
    RwInt32 flags;
    RwRGBA color;
    RwInt32 unused;
    RwBool textured;
    RwSurfaceProperties surfaceProps;
};

extern RwStream* _rpReadMaterialRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off,
                                       RwInt32 size);
extern RwStream* _rpWriteMaterialRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off,
                                        RwInt32 size);
extern RwInt32 _rpSizeMaterialRights(const void* obj, RwInt32 off, RwInt32 size);
extern void _rpMaterialSetDefaultSurfaceProperties(const RwSurfaceProperties* surfaceProps);
extern void* _rpMaterialOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpMaterialClose(void* instance, RwInt32 offset, RwInt32 size);

#define RWMATERIALGLOBAL(var)                                                                      \
    (RWPLUGINOFFSET(rpMaterialGlobals, RwEngineInstance, materialModule.globalsOffset)->var)

static RwPluginRegistry materialTKList = { sizeof(RpMaterial),      sizeof(RpMaterial),     0, 0,
                                           (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwSurfaceProperties defaultSurfaceProperties = { (RwReal)1.0, (RwReal)1.0, (RwReal)1.0 };

static RwModuleInfo materialModule;

static RwUInt32 lastSeenRightsPluginId;
static RwUInt32 lastSeenExtraData;

static RwInt32 _rpMaterialFreeListBlockSize = 256;
static RwInt32 _rpMaterialFreeListPreallocBlocks = 1;
static RwFreeList _rpMaterialFreeList;

RwStream* _rpReadMaterialRights(RwStream* s, RwInt32 len, void* obj, RwInt32 off, RwInt32 size)
{
    if (!RwStreamReadInt32(s, (RwInt32*)&lastSeenRightsPluginId, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (len == (RwInt32)(sizeof(RwInt32) * 2))
    {
        if (!RwStreamReadInt32(s, (RwInt32*)&lastSeenExtraData, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }
    }

    return s;
}

RwStream* _rpWriteMaterialRights(RwStream* s, RwInt32 len, const void* obj, RwInt32 off,
                                 RwInt32 size)
{
    const RpMaterial* mat = (const RpMaterial*)obj;

    if (!RwStreamWriteInt32(s, (const RwInt32*)&mat->pipeline->pluginId, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWriteInt32(s, (const RwInt32*)&mat->pipeline->pluginData, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    return s;
}

RwInt32 _rpSizeMaterialRights(const void* obj, RwInt32 off, RwInt32 size)
{
    const RpMaterial* mat = (const RpMaterial*)obj;

    if (mat->pipeline && mat->pipeline->pluginId)
    {
        return sizeof(RwInt32) * 2;
    }

    return 0;
}

void _rpMaterialSetDefaultSurfaceProperties(const RwSurfaceProperties* surfaceProps)
{
    if (!surfaceProps)
    {
        defaultSurfaceProperties.ambient = (RwReal)1.0;
        defaultSurfaceProperties.diffuse = (RwReal)1.0;
        defaultSurfaceProperties.specular = (RwReal)1.0;
    }
    else
    {
        defaultSurfaceProperties = *surfaceProps;
    }
}

void* _rpMaterialOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    materialModule.globalsOffset = offset;

    RWMATERIALGLOBAL(matFreeList) = RwFreeListCreateAndPreallocateSpace(
        materialTKList.sizeOfStruct, _rpMaterialFreeListBlockSize, 4,
        _rpMaterialFreeListPreallocBlocks, &_rpMaterialFreeList);
    if (!RWMATERIALGLOBAL(matFreeList))
    {
        return NULL;
    }

    materialModule.numInstances++;

    return instance;
}

void* _rpMaterialClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWMATERIALGLOBAL(matFreeList))
    {
        RwFreeListDestroy(RWMATERIALGLOBAL(matFreeList));
        RWMATERIALGLOBAL(matFreeList) = (RwFreeList*)NULL;
    }

    materialModule.numInstances--;

    return instance;
}

RpMaterial* RpMaterialCreate(void)
{
    RpMaterial* material;
    RwRGBA color;

    material = (RpMaterial*)RwFreeListAlloc(RWMATERIALGLOBAL(matFreeList));
    if (!material)
    {
        return (RpMaterial*)NULL;
    }

    material->refCount = 1;

    color.red = 255;
    color.green = 255;
    color.blue = 255;
    color.alpha = 255;
    material->color = color;
    material->texture = (RwTexture*)NULL;
    material->pipeline = (RxPipeline*)NULL;
    material->surfaceProps = defaultSurfaceProperties;

    _rwPluginRegistryInitObject(&materialTKList, material);

    return material;
}

RwBool RpMaterialDestroy(RpMaterial* material)
{
    if (material->refCount == 1)
    {
        _rwPluginRegistryDeInitObject(&materialTKList, material);

        RpMaterialSetTexture(material, (RwTexture*)NULL);

        RwFreeListFree(RWMATERIALGLOBAL(matFreeList), material);
    }
    else
    {
        material->refCount--;
    }

    return TRUE;
}

RpMaterial* RpMaterialSetTexture(RpMaterial* material, RwTexture* texture)
{
    if (texture)
    {
        RwTextureAddRef(texture);
    }

    if (material->texture)
    {
        RwTextureDestroy(material->texture);
    }

    material->texture = texture;

    return material;
}

RwInt32 RpMaterialRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                 RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPlugin(&materialTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);

    return plug;
}

RwInt32 RpMaterialRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPluginStream(&materialTKList, pluginID, readCB, writeCB, getSizeCB);

    return plug;
}

RpMaterial* RpMaterialStreamRead(RwStream* stream)
{
    RwUInt32 size;
    RwUInt32 version;
    RpMaterial* material;
    RpMaterialChunkInfo mat;
    RwRGBA tmp;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return (RpMaterial*)NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        memset(&mat, 0, sizeof(mat));

        if (RwStreamRead(stream, &mat, size) != size)
        {
            return (RpMaterial*)NULL;
        }

        tmp = mat.color;
        RwMemNative32(&mat, sizeof(mat));
        mat.color = tmp;

        material = RpMaterialCreate();
        if (!material)
        {
            return (RpMaterial*)NULL;
        }

        material->color = mat.color;

        if (size <= (RwUInt32)((RwUInt8*)&mat.surfaceProps - (RwUInt8*)&mat) && version < 0x31000)
        {
            material->surfaceProps = defaultSurfaceProperties;
        }
        else
        {
            material->surfaceProps = mat.surfaceProps;
        }

        material->texture = (RwTexture*)NULL;

        if (mat.textured)
        {
            if (!RwStreamFindChunk(stream, rwID_TEXTURE, (RwUInt32*)NULL, &version))
            {
                RpMaterialDestroy(material);
                return (RpMaterial*)NULL;
            }

            if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
            {
                material->texture = RwTextureStreamRead(stream);
            }
            else
            {
                RpMaterialDestroy(material);
                RWERROR((E_RW_BADVERSION));
                return (RpMaterial*)NULL;
            }
        }

        lastSeenRightsPluginId = 0;
        lastSeenExtraData = 0;

        if (!_rwPluginRegistryReadDataChunks(&materialTKList, stream, material))
        {
            RpMaterialDestroy(material);
            return (RpMaterial*)NULL;
        }

        if (lastSeenRightsPluginId)
        {
            _rwPluginRegistryInvokeRights(&materialTKList, lastSeenRightsPluginId, material,
                                          lastSeenExtraData);
        }

        return material;
    }

    RWERROR((E_RW_BADVERSION));
    return (RpMaterial*)NULL;
}

RwUInt32 RpMaterialStreamGetSize(const RpMaterial* material)
{
    RwUInt32 size;

    size = sizeof(RpMaterialChunkInfo) + rwCHUNKHEADERSIZE;

    if (material->texture)
    {
        size += RwTextureStreamGetSize(material->texture) + rwCHUNKHEADERSIZE;
    }

    size += _rwPluginRegistryGetSize(&materialTKList, material) + rwCHUNKHEADERSIZE;

    return size;
}

const RpMaterial* RpMaterialStreamWrite(const RpMaterial* material, RwStream* stream)
{
    RpMaterialChunkInfo mat;
    const RwSurfaceProperties* source;
    const RwRGBA* color;

    if (!RwStreamWriteChunkHeader(stream, rwID_MATERIAL, RpMaterialStreamGetSize(material)))
    {
        return (const RpMaterial*)NULL;
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, sizeof(RpMaterialChunkInfo)))
    {
        return (const RpMaterial*)NULL;
    }

    mat.flags = 0;

    if (material->texture)
    {
        mat.textured = TRUE;
    }
    else
    {
        mat.textured = FALSE;
    }

    source = &material->surfaceProps;
    mat.surfaceProps = *source;

    RwMemLittleEndian32(&mat, sizeof(RpMaterialChunkInfo));

    color = &material->color;
    mat.color = *color;

    if (!RwStreamWrite(stream, &mat, sizeof(RpMaterialChunkInfo)))
    {
        return (const RpMaterial*)NULL;
    }

    if (material->texture)
    {
        if (!RwTextureStreamWrite(material->texture, stream))
        {
            return (const RpMaterial*)NULL;
        }
    }

    if (!_rwPluginRegistryWriteDataChunks(&materialTKList, stream, material))
    {
        return (const RpMaterial*)NULL;
    }

    return material;
}
