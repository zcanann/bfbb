#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwTEXDICTIONARY 6

#define rwPLUGIN_ID 1

#define rwTEXTUREALIGNMENT 4
#define rwTEXDICTIONARYALIGNMENT 4

#define rwSTANDARDTEXTURESETRASTER 8

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

#define rwTEXTUREBASENAMELENGTH 32

extern void* memcpy(void* dst, const void* src, RwUInt32 size);

#define E_RW_STRINGTOOLONG 0x8000001e
#define E_RW_READTEXMASK 0x8000000a
#define E_RW_READTEX 0x8000000b

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef RwTexture* (*RwTextureCallBackFind)(const RwChar* name);
typedef RwBool (*RwTextureCallBackMipmapGenerate)(RwRaster* raster, RwImage* image);
typedef RwBool (*RwTextureCallBackMipmapName)(RwChar* name, RwChar* maskName, RwUInt8 mipLevel,
                                              RwInt32 format);

typedef struct rwTextureGlobals rwTextureGlobals;
struct rwTextureGlobals
{
    RwLinkList texDictList; /* 0x00 */
    RwFreeList* textureFreeList; /* 0x08 */
    RwFreeList* texDictFreeList; /* 0x0C */
    RwTexDictionary* currentTexDict; /* 0x10 */
    RwTextureCallBackRead textureReadCallBack; /* 0x14 */
    RwTextureCallBackFind textureFindCallBack; /* 0x18 */
    RwBool haveTextureMipmaps; /* 0x1C */
    RwBool haveTextureAutoMipmaps; /* 0x20 */
    RwChar* mipmapNameBuffer; /* 0x24 */
    RwUInt16 mipmapNameBufferSize; /* 0x28 */
    RwUInt16 pad; /* 0x2A */
    RwTextureCallBackMipmapGenerate textureMipmapGenerateCallBack; /* 0x2C */
    RwTextureCallBackMipmapName textureMipmapNameCallBack; /* 0x30 */
};

#define RWTEXTUREGLOBAL(var)                                                                       \
    (RWPLUGINOFFSET(rwTextureGlobals, RwEngineInstance, textureModule.globalsOffset)->var)

extern RwBool _rwPalQuantInit(void* pQuant);
extern void _rwPalQuantAddImage(void* pQuant, RwImage* image, RwReal weight);
extern void _rwPalQuantResolvePalette(RwRGBA* pal, RwInt32 palSize, void* pQuant);
extern void _rwPalQuantMatchImage(RwUInt8* dstPixels, RwInt32 dstStride, RwInt32 dstDepth,
                                  RwBool dither, void* pQuant, RwImage* image);
extern void _rwPalQuantTerm(void* pQuant);

static RwTexDictionary* dummyTexDict;
static RwModuleInfo textureModule;

static RwFreeList _rwTextureFreeList;
static RwFreeList _rwTexDictionaryFreeList;

RwInt32 _rwTextureFreeListBlockSize = 128;
RwInt32 _rwTextureFreeListPreallocBlocks = 1;
RwInt32 _rwTexDictionaryFreeListBlockSize = 5;
RwInt32 _rwTexDictionaryFreeListPreallocBlocks = 1;

static RwPluginRegistry textureTKList = { sizeof(RwTexture),       sizeof(RwTexture),      0, 0,
                                          (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwPluginRegistry texDictTKList = { sizeof(RwTexDictionary), sizeof(RwTexDictionary), 0, 0,
                                          (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwBool TextureCompareName(const RwChar* string1, const RwChar* string2)
{
    while (*string1 && *string2)
    {
        RwChar char1 = *string1;
        RwChar char2 = *string2;

        if ((char1 > '`') && (char1 < '{'))
        {
            char1 -= 0x20;
        }

        if ((char2 > '`') && (char2 < '{'))
        {
            char2 -= 0x20;
        }

        if (char1 != char2)
        {
            return FALSE;
        }

        string1++;
        string2++;
    }

    return (*string1 == *string2);
}

static RwBool TextureDefaultMipmapName(RwChar* name, RwChar* maskName, RwUInt8 mipLevel,
                                       RwInt32 format)
{
    static const RwChar character[] = "0123456789abcdef";
    RwChar extension[3];
    RwBool valid = FALSE;

    extension[0] = 'm';

    if ((mipLevel != 0) && (mipLevel < 16))
    {
        valid = TRUE;
    }

    if (valid)
    {
        extension[1] = character[mipLevel];
    }
    else
    {
        extension[1] = '\0';
    }

    extension[2] = '\0';

    if (extension[1] != '\0')
    {
        rwstrcat(name, extension);

        if ((maskName != NULL) && (*maskName != '\0'))
        {
            rwstrcat(maskName, extension);
        }
    }

    return TRUE;
}

static RwBool PalettizeImage(RwImage** image, RwInt32 depth)
{
    void* palQuant[4];
    RwRGBA palette[256];
    RwImage* newImage;

    if (!_rwPalQuantInit(palQuant))
    {
        return FALSE;
    }

    _rwPalQuantAddImage(palQuant, *image, ((RwReal)1));
    _rwPalQuantResolvePalette(palette, 1 << depth, palQuant);

    newImage = RwImageCreate((*image)->width, (*image)->height, depth);
    if (!newImage)
    {
        return FALSE;
    }

    RwImageAllocatePixels(newImage);

    _rwPalQuantMatchImage(newImage->cpPixels, newImage->stride, newImage->depth, FALSE, palQuant,
                          *image);

    memcpy(newImage->palette, palette, (1 << depth) << 2);

    RwImageDestroy(*image);
    *image = newImage;

    _rwPalQuantTerm(palQuant);

    return TRUE;
}

static RwBool PalettizeMipmaps(RwImage** image, RwInt32 numLevels, RwInt32 depth)
{
    RwInt32 i;

    for (i = 0; i < numLevels; i++)
    {
        if (!PalettizeImage(&image[i], depth))
        {
            return FALSE;
        }
    }

    return TRUE;
}

static RwImage* TextureImageReadAndSize(const RwChar* name, RwInt32* width, RwInt32* height,
                                        RwInt32* depth, RwInt32* format)
{
    RwImage* image;

    image = RwImageReadMaskedImage(name, (const RwChar*)NULL);
    if (!image)
    {
        return (RwImage*)NULL;
    }

    RwImageFindRasterFormat(image, rwRASTERTYPETEXTURE, width, height, depth, format);

    return image;
}

static RwTexture* TextureDefaultNormalRead(const RwChar* name, const RwChar* maskName);
static RwTexture* TextureDefaultMipmapRead(const RwChar* name, const RwChar* maskName);

static RwTexture* TextureDefaultRead(const RwChar* name, const RwChar* maskName)
{
    if (RWTEXTUREGLOBAL(haveTextureMipmaps))
    {
        return TextureDefaultMipmapRead(name, maskName);
    }

    return TextureDefaultNormalRead(name, maskName);
}

static RwTexture* TextureDefaultNormalRead(const RwChar* name, const RwChar* maskName)
{
    RwImage* image;
    RwRaster* raster;
    RwTexture* texture;
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwInt32 format;

    image = TextureImageReadAndSize(name, &width, &height, &depth, &format);
    if (!image)
    {
        return (RwTexture*)NULL;
    }

    raster = RwRasterCreate(width, height, depth, format);
    if (!raster)
    {
        RwImageDestroy(image);

        return (RwTexture*)NULL;
    }

    RwRasterSetFromImage(raster, image);
    RwImageDestroy(image);

    texture = RwTextureCreate(raster);
    if (!texture)
    {
        RwRasterDestroy(raster);

        return (RwTexture*)NULL;
    }

    RwTextureSetName(texture, name);
    RwTextureSetMaskName(texture, maskName);

    return texture;
}

static RwTexture* TextureDefaultMipmapRead(const RwChar* name, const RwChar* maskName)
{
    RwTexture* texture;

    texture = TextureDefaultNormalRead(name, maskName);
    if (!texture)
    {
        return (RwTexture*)NULL;
    }

    RwTextureRasterGenerateMipmaps(texture->raster, (RwImage*)NULL);

    return texture;
}

static RwBool TextureRasterDefaultBuildMipmaps(RwRaster* raster, RwImage* image)
{
    RwInt32 numLevels;
    RwInt32 i;

    numLevels = RwRasterGetNumLevels(raster);

    for (i = 1; i < numLevels; i++)
    {
        RwImage* mipImage;

        RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKWRITE);
        mipImage = RwImageCreateResample(image, 1, 1);
        RwRasterSetFromImage(raster, mipImage);
        RwImageDestroy(mipImage);
        RwRasterUnlock(raster);
    }

    return TRUE;
}

static RwTexture* TextureDefaultFind(const RwChar* name)
{
    rwTextureGlobals* globals =
        RWPLUGINOFFSET(rwTextureGlobals, RwEngineInstance, textureModule.globalsOffset);

    if (globals->currentTexDict)
    {
        return RwTexDictionaryFindNamedTexture(globals->currentTexDict, name);
    }
    else
    {
        RwLLLink* cur = rwLinkListGetFirstLLLink(&globals->texDictList);
        RwLLLink* end = rwLinkListGetTerminator(&globals->texDictList);

        while (cur != end)
        {
            RwTexDictionary* dict = rwLLLinkGetData(cur, RwTexDictionary, lInInstance);
            RwTexture* texture = RwTexDictionaryFindNamedTexture(dict, name);

            if (texture)
            {
                return texture;
            }

            cur = rwLLLinkGetNext(cur);
        }
    }

    return (RwTexture*)NULL;
}

RwBool RwTextureSetReadCallBack(RwTextureCallBackRead fpCallBack)
{
    RWTEXTUREGLOBAL(textureReadCallBack) = fpCallBack;

    return TRUE;
}

RwBool RwTextureSetMipmapping(RwBool enable)
{
    RWTEXTUREGLOBAL(haveTextureMipmaps) = enable;

    return TRUE;
}

RwBool RwTextureGetMipmapping(void)
{
    return RWTEXTUREGLOBAL(haveTextureMipmaps);
}

RwBool RwTextureSetAutoMipmapping(RwBool enable)
{
    RWTEXTUREGLOBAL(haveTextureAutoMipmaps) = enable;

    return TRUE;
}

RwBool RwTextureGetAutoMipmapping(void)
{
    return RWTEXTUREGLOBAL(haveTextureAutoMipmaps);
}

RwTexture* RwTextureSetRaster(RwTexture* texture, RwRaster* raster)
{
    if (raster)
    {
        if (RWSRCGLOBAL(stdFunc)[rwSTANDARDTEXTURESETRASTER](texture, raster, 0))
        {
            return texture;
        }

        return (RwTexture*)NULL;
    }

    texture->raster = (RwRaster*)NULL;

    return texture;
}

RwTexDictionary* RwTexDictionaryCreate(void)
{
    RwTexDictionary* dict;

    dict = (RwTexDictionary*)RwFreeListAlloc(RWTEXTUREGLOBAL(texDictFreeList));
    if (!dict)
    {
        return (RwTexDictionary*)NULL;
    }

    rwObjectInitialize(dict, rwTEXDICTIONARY, 0);

    rwLinkListAddLLLink(&RWTEXTUREGLOBAL(texDictList), &dict->lInInstance);
    rwLinkListInitialize(&dict->texturesInDict);

    _rwPluginRegistryInitObject(&texDictTKList, dict);

    return dict;
}

RwBool RwTexDictionaryDestroy(RwTexDictionary* dict)
{
    if (RWTEXTUREGLOBAL(currentTexDict) == dict)
    {
        RWTEXTUREGLOBAL(currentTexDict) = (RwTexDictionary*)NULL;
    }

    RwTexDictionaryForAllTextures(dict, (RwTextureCallBack)RwTextureDestroy, NULL);

    _rwPluginRegistryDeInitObject(&texDictTKList, dict);

    rwLinkListRemoveLLLink(&dict->lInInstance);

    RwFreeListFree(RWTEXTUREGLOBAL(texDictFreeList), dict);

    return TRUE;
}

const RwTexDictionary* RwTexDictionaryForAllTextures(const RwTexDictionary* dict,
                                                     RwTextureCallBack fpCallBack, void* pData)
{
    RwLLLink* cur;
    RwLLLink* next;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink((RwLinkList*)&dict->texturesInDict);
    end = rwLinkListGetTerminator((RwLinkList*)&dict->texturesInDict);

    while (cur != end)
    {
        RwTexture* texture = rwLLLinkGetData(cur, RwTexture, lInDictionary);

        next = rwLLLinkGetNext(cur);

        if (!fpCallBack(texture, pData))
        {
            break;
        }

        cur = next;
    }

    return dict;
}

RwTexture* RwTextureCreate(RwRaster* raster)
{
    RwTexture* texture;

    texture = (RwTexture*)RwFreeListAlloc(RWTEXTUREGLOBAL(textureFreeList));
    if (texture)
    {
        texture->dict = (RwTexDictionary*)NULL;
        texture->name[0] = '\0';
        texture->mask[0] = '\0';
        texture->raster = raster;
        texture->refCount = 1;

        texture->filterAddressing = 0;
        RwTextureSetAddressing(texture, rwTEXTUREADDRESSWRAP);
        RwTextureSetFilterMode(texture, rwFILTERNEAREST);

        _rwPluginRegistryInitObject(&textureTKList, texture);
    }

    return texture;
}

RwBool RwTextureDestroy(RwTexture* texture)
{
    texture->refCount--;

    if (texture->refCount <= 0)
    {
        texture->refCount++;

        _rwPluginRegistryDeInitObject(&textureTKList, texture);

        if (texture->dict)
        {
            rwLinkListRemoveLLLink(&texture->lInDictionary);
        }

        if (texture->raster)
        {
            RwRasterDestroy(texture->raster);
            texture->raster = (RwRaster*)NULL;
        }

        texture->refCount--;

        RwFreeListFree(RWTEXTUREGLOBAL(textureFreeList), texture);
    }

    return TRUE;
}

RwTexture* RwTextureSetName(RwTexture* texture, const RwChar* name)
{
    rwstrncpy(texture->name, name, rwTEXTUREBASENAMELENGTH);

    if (rwstrlen(name) >= rwTEXTUREBASENAMELENGTH)
    {
        RWERROR((E_RW_STRINGTOOLONG, name, rwTEXTUREBASENAMELENGTH, rwTEXTUREBASENAMELENGTH - 1,
                 name[rwTEXTUREBASENAMELENGTH - 1]));

        texture->name[rwTEXTUREBASENAMELENGTH - 1] = '\0';
    }

    return texture;
}

RwTexture* RwTextureSetMaskName(RwTexture* texture, const RwChar* maskName)
{
    rwstrncpy(texture->mask, maskName, rwTEXTUREBASENAMELENGTH);

    if (rwstrlen(maskName) >= rwTEXTUREBASENAMELENGTH)
    {
        RWERROR((E_RW_STRINGTOOLONG, maskName, rwTEXTUREBASENAMELENGTH, rwTEXTUREBASENAMELENGTH - 1,
                 maskName[rwTEXTUREBASENAMELENGTH - 1]));

        texture->mask[rwTEXTUREBASENAMELENGTH - 1] = '\0';
    }

    return texture;
}

RwTexture* RwTexDictionaryAddTexture(RwTexDictionary* dict, RwTexture* texture)
{
    if (texture->dict)
    {
        rwLinkListRemoveLLLink(&texture->lInDictionary);
    }

    texture->dict = dict;

    rwLinkListAddLLLink(&dict->texturesInDict, &texture->lInDictionary);

    return texture;
}

RwTexture* RwTexDictionaryRemoveTexture(RwTexture* texture)
{
    if (texture->dict)
    {
        texture->dict = (RwTexDictionary*)NULL;

        rwLinkListRemoveLLLink(&texture->lInDictionary);
    }

    return texture;
}

RwTexture* RwTexDictionaryFindNamedTexture(RwTexDictionary* dict, const RwChar* name)
{
    RwLLLink* cur = rwLinkListGetFirstLLLink(&dict->texturesInDict);
    RwLLLink* end = rwLinkListGetTerminator(&dict->texturesInDict);

    while (cur != end)
    {
        RwTexture* texture = rwLLLinkGetData(cur, RwTexture, lInDictionary);

        if (RwTextureGetName(texture) && TextureCompareName(RwTextureGetName(texture), name))
        {
            return texture;
        }

        cur = rwLLLinkGetNext(cur);
    }

    return (RwTexture*)NULL;
}

RwTexDictionary* RwTexDictionaryGetCurrent(void)
{
    return RWTEXTUREGLOBAL(currentTexDict);
}

RwBool RwTextureGenerateMipmapName(RwChar* name, RwChar* maskName, RwUInt8 mipLevel, RwInt32 format)
{
    RwTextureCallBackMipmapName callBack = RWTEXTUREGLOBAL(textureMipmapNameCallBack);

    if (!callBack)
    {
        return FALSE;
    }

    return callBack(name, maskName, mipLevel, format);
}

RwTexture* RwTextureRead(const RwChar* name, const RwChar* maskName)
{
    RwTexture* texture;

    texture = RWTEXTUREGLOBAL(textureFindCallBack)(name);
    if (texture)
    {
        texture->refCount++;

        return texture;
    }

    texture = RWTEXTUREGLOBAL(textureReadCallBack)(name, maskName);
    if (!texture)
    {
        if (maskName)
        {
            RWERROR((E_RW_READTEXMASK, name, maskName));
        }
        else
        {
            RWERROR((E_RW_READTEX, name));
        }

        return (RwTexture*)NULL;
    }

    if (RWTEXTUREGLOBAL(currentTexDict))
    {
        RwTexDictionaryAddTexture(RWTEXTUREGLOBAL(currentTexDict), texture);
    }

    return texture;
}

RwInt32 RwTextureRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                RwPluginObjectConstructor constructCB,
                                RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&textureTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);
}

RwBool RwTextureRasterGenerateMipmaps(RwRaster* raster, RwImage* image)
{
    return RWTEXTUREGLOBAL(textureMipmapGenerateCallBack)(raster, image) ? TRUE : FALSE;
}

void* _rwTextureClose(void* instance, RwInt32 offset, RwInt32 size)
{
    rwTextureGlobals* globals =
        RWPLUGINOFFSET(rwTextureGlobals, RwEngineInstance, textureModule.globalsOffset);

    if (globals->mipmapNameBuffer)
    {
        RwFree(globals->mipmapNameBuffer);
        globals->mipmapNameBuffer = (RwChar*)NULL;
        globals->mipmapNameBufferSize = 0;
    }

    if (globals->textureFreeList && globals->texDictFreeList)
    {
        RwTexDictionary* dict = dummyTexDict;
        RwLLLink* cur = rwLinkListGetFirstLLLink(&globals->texDictList);
        RwLLLink* end = rwLinkListGetTerminator(&globals->texDictList);
        RwBool found = FALSE;

        while (cur != end)
        {
            RwTexDictionary* cursor = rwLLLinkGetData(cur, RwTexDictionary, lInInstance);

            cur = rwLLLinkGetNext(cur);

            if (cursor == dummyTexDict)
            {
                found = TRUE;
                break;
            }
        }

        if (found)
        {
            RwLLLink* texCur;
            RwLLLink* texEnd;

            if (globals->currentTexDict == dummyTexDict)
            {
                globals->currentTexDict = (RwTexDictionary*)NULL;
            }

            texCur = rwLinkListGetFirstLLLink(&dict->texturesInDict);
            texEnd = rwLinkListGetTerminator(&dict->texturesInDict);

            while (texCur != texEnd)
            {
                RwTexture* texture = rwLLLinkGetData(texCur, RwTexture, lInDictionary);

                texCur = rwLLLinkGetNext(texCur);

                if (!RwTextureDestroy(texture))
                {
                    break;
                }
            }

            _rwPluginRegistryDeInitObject(&texDictTKList, dict);

            rwLinkListRemoveLLLink(&dict->lInInstance);

            RwFreeListFree(globals->texDictFreeList, dict);

            dummyTexDict = (RwTexDictionary*)NULL;
        }
    }

    if (RWTEXTUREGLOBAL(textureFreeList))
    {
        RwFreeListDestroy(RWTEXTUREGLOBAL(textureFreeList));
        RWTEXTUREGLOBAL(textureFreeList) = (RwFreeList*)NULL;
    }

    if (RWTEXTUREGLOBAL(texDictFreeList))
    {
        RwFreeListDestroy(RWTEXTUREGLOBAL(texDictFreeList));
        RWTEXTUREGLOBAL(texDictFreeList) = (RwFreeList*)NULL;
    }

    textureModule.numInstances--;

    return instance;
}

void* _rwTextureOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    rwTextureGlobals* globals;

    textureModule.globalsOffset = offset;

    RWTEXTUREGLOBAL(textureFreeList) =
        RwFreeListCreateAndPreallocateSpace((&textureTKList)->sizeOfStruct,
                                            _rwTextureFreeListBlockSize, rwTEXTUREALIGNMENT,
                                            _rwTextureFreeListPreallocBlocks, &_rwTextureFreeList);

    if (!RWTEXTUREGLOBAL(textureFreeList))
    {
        return NULL;
    }

    RWTEXTUREGLOBAL(texDictFreeList) = RwFreeListCreateAndPreallocateSpace(
        (&texDictTKList)->sizeOfStruct, _rwTexDictionaryFreeListBlockSize, rwTEXDICTIONARYALIGNMENT,
        _rwTexDictionaryFreeListPreallocBlocks, &_rwTexDictionaryFreeList);

    globals = RWPLUGINOFFSET(rwTextureGlobals, RwEngineInstance, textureModule.globalsOffset);

    if (!globals->texDictFreeList)
    {
        RwFreeListDestroy(globals->textureFreeList);
        RWTEXTUREGLOBAL(textureFreeList) = (RwFreeList*)NULL;

        return NULL;
    }

    rwLinkListInitialize(&globals->texDictList);

    textureModule.numInstances++;

    dummyTexDict = RwTexDictionaryCreate();
    RWTEXTUREGLOBAL(currentTexDict) = dummyTexDict;

    globals = RWPLUGINOFFSET(rwTextureGlobals, RwEngineInstance, textureModule.globalsOffset);

    if (!globals->currentTexDict)
    {
        RwFreeListDestroy(globals->texDictFreeList);
        RWTEXTUREGLOBAL(texDictFreeList) = (RwFreeList*)NULL;
        RwFreeListDestroy(RWTEXTUREGLOBAL(textureFreeList));
        RWTEXTUREGLOBAL(textureFreeList) = (RwFreeList*)NULL;

        return NULL;
    }

    globals->haveTextureMipmaps = FALSE;
    RWTEXTUREGLOBAL(haveTextureAutoMipmaps) = FALSE;
    RWTEXTUREGLOBAL(textureFindCallBack) = TextureDefaultFind;
    RWTEXTUREGLOBAL(textureReadCallBack) = TextureDefaultRead;
    RWTEXTUREGLOBAL(textureMipmapGenerateCallBack) = TextureRasterDefaultBuildMipmaps;
    RWTEXTUREGLOBAL(textureMipmapNameCallBack) = TextureDefaultMipmapName;
    RWTEXTUREGLOBAL(mipmapNameBuffer) = (RwChar*)NULL;
    RWTEXTUREGLOBAL(mipmapNameBufferSize) = 0;

    return instance;
}
