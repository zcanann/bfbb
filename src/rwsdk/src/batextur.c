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
#define rwTEXTURENAMEBUFFERLENGTH 256
#define rwTEXTUREMAXMIPLEVELS 64
#define rwTEXTUREMAXMIPLEVELS16 16

#define E_RW_INVIMAGEDEPTH 0x80000009

extern void* memcpy(void* dst, const void* src, RwUInt32 size);

#define E_RW_STRINGTOOLONG 0x8000001e
#define E_RW_READTEXMASK 0x16

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef RwTexture* (*RwTextureCallBackFind)(const RwChar* name);
typedef RwRaster* (*RwTextureCallBackMipmapGenerate)(RwRaster* raster, RwImage* image);
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

static RwModuleInfo textureModule;
static RwTexDictionary* dummyTexDict;

static RwFreeList _rwTextureFreeList;
static RwFreeList _rwTexDictionaryFreeList;

static RwInt32 _rwTextureFreeListBlockSize = 128;
static RwInt32 _rwTextureFreeListPreallocBlocks = 1;
static RwInt32 _rwTexDictionaryFreeListBlockSize = 5;
static RwInt32 _rwTexDictionaryFreeListPreallocBlocks = 1;

RwPluginRegistry textureTKList = { sizeof(RwTexture),       sizeof(RwTexture),      0, 0,
                                   (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

RwPluginRegistry texDictTKList = { sizeof(RwTexDictionary), sizeof(RwTexDictionary), 0, 0,
                                   (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwBool TextureCompareName(const RwChar* string1, const RwChar* string2)
{
    while (*string1 && *string2)
    {
        RwChar char1 = *string1;
        RwChar char2 = *string2;

        if (char1 >= 'a' && char1 <= 'z')
        {
            char1 -= 'a' - 'A';
        }

        if (char2 >= 'a' && char2 <= 'z')
        {
            char2 -= 'a' - 'A';
        }

        if (char1 != char2)
        {
            return FALSE;
        }

        string1++;
        string2++;
    }

    if (*string1 == *string2)
    {
        return TRUE;
    }

    return FALSE;
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

    extension[1] = valid ? character[mipLevel] : '\0';

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
    if (newImage)
    {
        RwImageAllocatePixels(newImage);

        _rwPalQuantMatchImage(newImage->cpPixels, newImage->stride, newImage->depth, FALSE,
                              palQuant, *image);

        memcpy(newImage->palette, palette, (1 << depth) << 2);

        RwImageDestroy(*image);
        *image = newImage;
    }
    else
    {
        return FALSE;
    }

    _rwPalQuantTerm(palQuant);

    return TRUE;
}

static RwBool PalettizeMipmaps(RwRGBA* palette, RwImage* srcImage, RwImage** images,
                               RwInt32 numLevels, RwInt32 depth)
{
    RwInt32 palQuant[4];
    RwInt32 palSize;
    RwInt32 i;

    /* If every level already shares one palette there is nothing to do */
    if (images[0]->palette)
    {
        palSize = 1 << depth;

        for (i = 1; i < numLevels; i++)
        {
            RwUInt32* basePal = (RwUInt32*)images[0]->palette;
            RwUInt32* levelPal = (RwUInt32*)images[i]->palette;
            RwInt32 j;

            if (!basePal || !levelPal)
            {
                i = rwTEXTUREMAXMIPLEVELS;
                break;
            }

            for (j = 0; j < palSize; j++)
            {
                if (basePal[j] != levelPal[j])
                {
                    i = rwTEXTUREMAXMIPLEVELS;
                    break;
                }
            }
        }

        if (i == numLevels)
        {
            memcpy(palette, images[0]->palette, (1 << images[0]->depth) * sizeof(RwRGBA));
            return TRUE;
        }
    }

    if (!_rwPalQuantInit(palQuant))
    {
        return FALSE;
    }

    for (i = 0; i < numLevels; i++)
    {
        _rwPalQuantAddImage(palQuant, images[i], ((RwReal)1));
    }

    _rwPalQuantResolvePalette(palette, 1 << depth, palQuant);

    for (i = 0; i < numLevels; i++)
    {
        RwImage* oldImage = images[i];
        RwImage* newImage;

        newImage = RwImageCreate(oldImage->width, oldImage->height, depth);
        if (newImage)
        {
            RwImageAllocatePixels(newImage);
            _rwPalQuantMatchImage(newImage->cpPixels, newImage->stride, newImage->depth, FALSE,
                                  palQuant, oldImage);

            /* All levels share the one palette */
            newImage->palette = palette;
            images[i] = newImage;

            if (oldImage != srcImage)
            {
                RwImageDestroy(oldImage);
            }
        }
        else
        {
            return FALSE;
        }
    }

    _rwPalQuantTerm(palQuant);

    return TRUE;
}

#define rwTextureCopyName(_dst, _src)                                                              \
    MACRO_START                                                                                    \
    {                                                                                              \
        rwstrncpy((_dst), (_src), rwTEXTURENAMEBUFFERLENGTH);                                      \
        if (rwstrlen(_src) >= rwTEXTURENAMEBUFFERLENGTH)                                           \
        {                                                                                          \
            RWERROR((E_RW_STRINGTOOLONG, (_src), rwTEXTURENAMEBUFFERLENGTH,                        \
                     rwTEXTURENAMEBUFFERLENGTH - 1, (_src)[rwTEXTURENAMEBUFFERLENGTH - 1]));       \
            (_dst)[rwTEXTURENAMEBUFFERLENGTH - 1] = '\0';                                          \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

static RwImage* TextureImageReadAndSize(const RwChar* name, const RwChar* maskName,
                                        RwInt32 rasterType, RwInt32* width, RwInt32* height,
                                        RwInt32* depth, RwInt32* format)
{
    RwImage* image;
    RwChar imageName[rwTEXTURENAMEBUFFERLENGTH];
    RwChar imageMaskName[rwTEXTURENAMEBUFFERLENGTH];
    const RwChar* extension;

    rwTextureCopyName(imageName, name);
    extension = RwImageFindFileType(name);
    if (extension)
    {
        rwstrcat(imageName, extension);
    }

    imageMaskName[0] = '\0';
    if (maskName && maskName[0])
    {
        rwTextureCopyName(imageMaskName, maskName);
        extension = RwImageFindFileType(maskName);
        if (extension)
        {
            rwstrcat(imageMaskName, extension);
        }
    }

    image = RwImageReadMaskedImage(imageName, imageMaskName);
    if (!image)
    {
        return (RwImage*)NULL;
    }

    if (!*width || !*height)
    {
        if (!RwImageFindRasterFormat(image, rasterType, width, height, depth, format))
        {
            RwImageDestroy(image);
            RWERROR((E_RW_INVIMAGEDEPTH));
            return (RwImage*)NULL;
        }
    }

    /* Resize to what the raster wants */
    if (image->width != *width || image->height != *height)
    {
        RwInt32 imageDepth = image->depth;
        RwImage* resampled;

        if (imageDepth != 32)
        {
            RwImage* origImage = image;

            image = RwImageCreate(origImage->width, origImage->height, 32);
            if (!image)
            {
                RwImageDestroy(origImage);
                return (RwImage*)NULL;
            }

            if (!RwImageAllocatePixels(image))
            {
                RwImageDestroy(image);
                RwImageDestroy(origImage);
                return (RwImage*)NULL;
            }

            RwImageCopy(image, origImage);
            RwImageDestroy(origImage);
        }

        resampled = RwImageCreate(*width, *height, 32);
        if (!resampled)
        {
            RwImageDestroy(image);
            return (RwImage*)NULL;
        }

        if (!RwImageAllocatePixels(resampled))
        {
            RwImageDestroy(resampled);
            RwImageDestroy(image);
            return (RwImage*)NULL;
        }

        RwImageResample(resampled, image);
        RwImageDestroy(image);
        image = resampled;

        /* Return to a palettised image if that is what we started with */
        if (imageDepth == 4)
        {
            PalettizeImage(&image, imageDepth);
        }
        else if (imageDepth == 8)
        {
            PalettizeImage(&image, imageDepth);
        }
    }

    return image;
}

static RwTexture* TextureDefaultNormalRead(const RwChar* name, const RwChar* maskName)
{
    RwImage* image;
    RwTexture* texture;
    RwRaster* raster;
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwInt32 format;
    RwChar imageName[rwTEXTURENAMEBUFFERLENGTH];
    RwChar imageMaskName[rwTEXTURENAMEBUFFERLENGTH];
    RwRGBA palette[256];

    rwTextureCopyName(imageName, name);

    imageMaskName[0] = '\0';
    if (maskName && maskName[0])
    {
        rwTextureCopyName(imageMaskName, maskName);
    }

    RwTextureGenerateMipmapName(imageName, imageMaskName, 0, rwRASTERTYPETEXTURE);

    width = 0;
    height = 0;
    image = TextureImageReadAndSize(imageName, imageMaskName, rwRASTERTYPETEXTURE, &width, &height,
                                    &depth, &format);
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

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        if (RwRasterGetFormat(raster) & rwRASTERFORMATPAL4)
        {
            PalettizeMipmaps(palette, (RwImage*)NULL, &image, 1, 4);
        }
        else
        {
            PalettizeMipmaps(palette, (RwImage*)NULL, &image, 1, 8);
        }

        image->palette = palette;
    }

    RwImageGammaCorrect(image);

    if (!RwRasterSetFromImage(raster, image))
    {
        RwRasterDestroy(raster);
        RwImageDestroy(image);
        return (RwTexture*)NULL;
    }

    RwImageDestroy(image);

    texture = RwTextureCreate(raster);
    if (!texture)
    {
        RwRasterDestroy(raster);
        return (RwTexture*)NULL;
    }

    RwTextureSetName(texture, name);

    if (maskName)
    {
        RwTextureSetMaskName(texture, maskName);
    }
    else
    {
        RwTextureSetMaskName(texture, "");
    }

    return texture;
}

static RwTexture* TextureDefaultMipmapRead(const RwChar* name, const RwChar* maskName)
{
    RwImage* images[rwTEXTUREMAXMIPLEVELS16];
    RwRaster* raster;
    RwTexture* texture;
    RwInt32 rasterType;
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwInt32 format;
    RwInt32 i;
    RwInt32 numLevels;
    RwChar imageName[rwTEXTURENAMEBUFFERLENGTH];
    RwChar imageMaskName[rwTEXTURENAMEBUFFERLENGTH];
    RwRGBA palette[256];

    rwTextureCopyName(imageName, name);

    imageMaskName[0] = '\0';
    if (maskName && maskName[0])
    {
        rwTextureCopyName(imageMaskName, maskName);
    }

    rasterType = rwRASTERTYPETEXTURE;
    if (RWTEXTUREGLOBAL(haveTextureMipmaps))
    {
        rasterType |= rwRASTERFORMATMIPMAP;

        if (RWTEXTUREGLOBAL(haveTextureAutoMipmaps))
        {
            rasterType |= rwRASTERFORMATAUTOMIPMAP;
        }
    }

    RwTextureGenerateMipmapName(imageName, imageMaskName, 0, rasterType);

    width = 0;
    height = 0;
    images[0] = TextureImageReadAndSize(imageName, imageMaskName, rasterType, &width, &height,
                                        &depth, &format);
    if (!images[0])
    {
        return (RwTexture*)NULL;
    }

    raster = RwRasterCreate(width, height, depth, format);
    if (!raster)
    {
        RwImageDestroy(images[0]);
        return (RwTexture*)NULL;
    }

    if (format & rwRASTERFORMATMIPMAP)
    {
        if (format & rwRASTERFORMATAUTOMIPMAP)
        {
            /* The driver builds the other levels */
            if (!RwRasterSetFromImage(raster, images[0]))
            {
                RwRasterDestroy(raster);
                RwImageDestroy(images[0]);
                return (RwTexture*)NULL;
            }

            RwImageDestroy(images[0]);
        }
        else
        {
            numLevels = RwRasterGetNumLevels(raster);

            /* Read each level from its own file */
            for (i = 1; i < numLevels; i++)
            {
                rwTextureCopyName(imageName, name);

                imageMaskName[0] = '\0';
                if (maskName && maskName[0])
                {
                    rwTextureCopyName(imageMaskName, maskName);
                }

                RwTextureGenerateMipmapName(imageName, imageMaskName, (RwUInt8)i, rasterType);

                RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH);
                width = RwRasterGetWidth(raster);
                height = RwRasterGetHeight(raster);
                depth = RwRasterGetDepth(raster);
                format = RwRasterGetFormat(raster) | raster->cType;
                RwRasterUnlock(raster);

                images[i] = TextureImageReadAndSize(imageName, imageMaskName, rasterType, &width,
                                                    &height, &depth, &format);
                if (!images[i])
                {
                    while (--i >= 0)
                    {
                        RwImageDestroy(images[i]);
                    }

                    RwRasterDestroy(raster);
                    return (RwTexture*)NULL;
                }
            }

            if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
            {
                if (RwRasterGetFormat(raster) & rwRASTERFORMATPAL4)
                {
                    PalettizeMipmaps(palette, (RwImage*)NULL, images, numLevels, 4);
                }
                else
                {
                    PalettizeMipmaps(palette, (RwImage*)NULL, images, numLevels, 8);
                }

                /* The levels share a palette, so correct it once */
                RwImageGammaCorrect(images[0]);
            }
            else
            {
                for (i = 0; i < numLevels; i++)
                {
                    RwImageGammaCorrect(images[i]);
                }
            }

            for (i = 0; i < numLevels; i++)
            {
                if (RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH))
                {
                    if (!RwRasterSetFromImage(raster, images[i]))
                    {
                        for (; i < numLevels; i++)
                        {
                            RwImageDestroy(images[i]);
                        }

                        RwRasterDestroy(raster);
                        return (RwTexture*)NULL;
                    }

                    RwRasterUnlock(raster);
                }

                RwImageDestroy(images[i]);
            }
        }
    }
    else
    {
        RwImageGammaCorrect(images[0]);

        if (!RwRasterSetFromImage(raster, images[0]))
        {
            RwRasterDestroy(raster);
            RwImageDestroy(images[0]);
            return (RwTexture*)NULL;
        }

        RwImageDestroy(images[0]);
    }

    texture = RwTextureCreate(raster);
    if (!texture)
    {
        RwRasterDestroy(raster);
        return (RwTexture*)NULL;
    }

    RwTextureSetName(texture, name);

    if (maskName)
    {
        RwTextureSetMaskName(texture, maskName);
    }
    else
    {
        RwTextureSetMaskName(texture, "");
    }

    return texture;
}

static RwTexture* TextureDefaultRead(const RwChar* name, const RwChar* maskName)
{
    if (RWTEXTUREGLOBAL(haveTextureMipmaps))
    {
        return TextureDefaultMipmapRead(name, maskName);
    }

    return TextureDefaultNormalRead(name, maskName);
}

static RwRaster* TextureRasterDefaultBuildMipmaps(RwRaster* raster, RwImage* image)
{
    RwImage* images[rwTEXTUREMAXMIPLEVELS16];
    RwRGBA palette[256];
    RwInt32 i;
    RwInt32 numLevels;
    RwUInt8 autoMipmap;
    RwInt32 width = raster->width;
    RwInt32 height = raster->height;

    if (!image)
    {
        images[0] = RwImageCreate(width, height, 32);
        if (images[0])
        {
            if (!RwImageAllocatePixels(images[0]))
            {
                return (RwRaster*)NULL;
            }

            RwImageSetFromRaster(images[0], raster);
        }
    }
    else if (image->depth != 32)
    {
        images[0] = RwImageCreate(width, height, 32);
        if (images[0])
        {
            if (!RwImageAllocatePixels(images[0]))
            {
                return (RwRaster*)NULL;
            }

            RwImageCopy(images[0], image);
        }
    }
    else
    {
        images[0] = image;
    }

    if (!images[0])
    {
        return (RwRaster*)NULL;
    }

    /* Stop the driver regenerating the levels while we fill them */
    autoMipmap = raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8);
    raster->cFormat &= ~autoMipmap;

    numLevels = RwRasterGetNumLevels(raster);

    for (i = 1; i < numLevels; i++)
    {
        images[i] = (RwImage*)NULL;

        if (RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKREAD))
        {
            images[i] = RwImageCreateResample(images[i - 1], raster->width, raster->height);
            RwRasterUnlock(raster);
        }

        if (!images[i])
        {
            while (--i >= 0)
            {
                if (images[i] != image)
                {
                    RwImageDestroy(images[i]);
                }
            }

            raster->cFormat |= autoMipmap;
            return (RwRaster*)NULL;
        }
    }

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        if (RwRasterGetFormat(raster) & rwRASTERFORMATPAL4)
        {
            if (!PalettizeMipmaps(palette, image, images, numLevels, 4))
            {
                /* NOTE: retail returns from inside this loop on its first pass */
                for (i = 0; i < numLevels; i++)
                {
                    if (images[i] != image)
                    {
                        RwImageDestroy(images[i]);
                    }

                    raster->cFormat |= autoMipmap;
                    return (RwRaster*)NULL;
                }
            }
        }
        else
        {
            if (!PalettizeMipmaps(palette, image, images, numLevels, 8))
            {
                /* NOTE: retail returns from inside this loop on its first pass */
                for (i = 0; i < numLevels; i++)
                {
                    if (images[i] != image)
                    {
                        RwImageDestroy(images[i]);
                    }

                    raster->cFormat |= autoMipmap;
                    return (RwRaster*)NULL;
                }
            }
        }

        RwImageGammaCorrect(images[0]);
    }
    else
    {
        for (i = 0; i < numLevels; i++)
        {
            RwImageGammaCorrect(images[i]);
        }
    }

    for (i = 0; i < numLevels; i++)
    {
        if (RwRasterLock(raster, (RwUInt8)i, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH))
        {
            RwRasterSetFromImage(raster, images[i]);
            RwRasterUnlock(raster);
        }

        if (images[i] != image)
        {
            RwImageDestroy(images[i]);
        }
    }

    raster->cFormat |= autoMipmap;

    return raster;
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
    RwBool result = TRUE;

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
        result = TRUE;
    }

    return result;
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
            RWERROR((E_RW_READTEXMASK, name, "(null)"));
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
    if (RWTEXTUREGLOBAL(mipmapNameBuffer))
    {
        RwFree(RWTEXTUREGLOBAL(mipmapNameBuffer));
        RWTEXTUREGLOBAL(mipmapNameBuffer) = (RwChar*)NULL;
        RWTEXTUREGLOBAL(mipmapNameBufferSize) = 0;
    }

    if (RWTEXTUREGLOBAL(textureFreeList) && RWTEXTUREGLOBAL(texDictFreeList))
    {
        rwTextureGlobals* globals =
            RWPLUGINOFFSET(rwTextureGlobals, RwEngineInstance, textureModule.globalsOffset);
        RwLLLink* cur = rwLinkListGetFirstLLLink(&globals->texDictList);
        RwLLLink* end = rwLinkListGetTerminator(&globals->texDictList);

        /* Only destroy the dummy dictionary if it is still registered */
        while (cur != end)
        {
            RwTexDictionary* dict = rwLLLinkGetData(cur, RwTexDictionary, lInInstance);

            cur = rwLLLinkGetNext(cur);

            if (dict == dummyTexDict)
            {
                RwTexDictionaryDestroy(dummyTexDict);
                dummyTexDict = (RwTexDictionary*)NULL;
                break;
            }
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

    rwLinkListInitialize(&RWTEXTUREGLOBAL(texDictList));

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
