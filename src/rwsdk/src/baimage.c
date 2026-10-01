#include <string.h>
#include <math.h>
#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

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

#define E_RW_INVIMAGEFORMAT 0x80000008
#define E_RW_INVIMAGEDEPTH 0x80000009
#define E_RW_INVIMAGESIZE 0x8000000A
#define E_RW_NOMEM 0x80000013
#define E_RW_NULLP 0x80000016

#define rwIMAGEALLOCATED 0x01
#define rwIMAGEGAMMACORRECTED 0x02

#define rwIMAGEALIGNMENT sizeof(RwUInt32)
#define rwIMAGEFORMATEXTENSIONLENGTH 20

#define rwSTANDARDPIXELTORGB 3

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))
#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))
#define RwFexist(_name) (RWSRCGLOBAL(fileFuncs).rwfexist((_name)))

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef RwImage* (*RwImageCallBackRead)(const RwChar* imageName);
typedef RwImage* (*RwImageCallBackWrite)(RwImage* image, const RwChar* imageName);

typedef struct rwImageFormat rwImageFormat;
struct rwImageFormat
{
    RwChar lcExtension[rwIMAGEFORMATEXTENSIONLENGTH];
    RwChar ucExtension[rwIMAGEFORMATEXTENSIONLENGTH];
    RwImageCallBackRead readImage;
    RwImageCallBackWrite writeImage;
    rwImageFormat* nextFormat;
};

typedef struct rwImageGlobals rwImageGlobals;
struct rwImageGlobals
{
    RwFreeList* imageFreeList;
    RwChar* imagePath;
    RwInt32 imagePathSize;
    RwUInt8 gammaTable[256];
    RwUInt8 invGammaTable[256];
    RwReal gammaVal;
    RwUInt8* scratchMem;
    RwInt32 scratchMemSize;
    RwFreeList* imageFormatFreeList;
    rwImageFormat* imageFormats;
};

#define RWIMAGEGLOBAL(var)                                                                         \
    (RWPLUGINOFFSET(rwImageGlobals, RwEngineInstance, imageModule.globalsOffset)->var)

typedef RwChar* (*rwImagePathCallBack)(RwChar* pathname, void* data);

typedef struct _imageReadData _imageReadData;
struct _imageReadData
{
    RwImageCallBackRead readImage;
    RwImage* image;
};

extern RwBool _rwpathisabsolute(const RwChar* path);

static RwPluginRegistry imageTKList = { sizeof(RwImage),        sizeof(RwImage),        0, 0,
                                        (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwInt32 _rwImageFreeListBlockSize = 128;
static RwInt32 _rwImageFreeListPreallocBlocks = 1;
static RwInt32 _rwImageFormatFreeListPreallocBlocks = 1;
static RwModuleInfo imageModule;
static RwFreeList _rwImageFreeList;
static RwFreeList _rwImageFormatFreeList;

void* _rwImageOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    imageModule.globalsOffset = offset;

    RWIMAGEGLOBAL(imageFreeList) =
        RwFreeListCreateAndPreallocateSpace(imageTKList.sizeOfStruct, _rwImageFreeListBlockSize,
                                            rwIMAGEALIGNMENT, _rwImageFreeListPreallocBlocks,
                                            &_rwImageFreeList);
    if (!RWIMAGEGLOBAL(imageFreeList))
    {
        return NULL;
    }

    RWIMAGEGLOBAL(imageFormatFreeList) = RwFreeListCreateAndPreallocateSpace(
        sizeof(rwImageFormat), _rwImageFreeListBlockSize, rwIMAGEALIGNMENT,
        _rwImageFormatFreeListPreallocBlocks, &_rwImageFormatFreeList);
    if (!RWIMAGEGLOBAL(imageFormatFreeList))
    {
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFreeList));
        RWIMAGEGLOBAL(imageFreeList) = NULL;
        return NULL;
    }

    RWIMAGEGLOBAL(imagePathSize) = 256;
    RWIMAGEGLOBAL(imagePath) = (RwChar*)RwMalloc(RWIMAGEGLOBAL(imagePathSize));
    if (!RWIMAGEGLOBAL(imagePath))
    {
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFormatFreeList));
        RWIMAGEGLOBAL(imageFormatFreeList) = NULL;
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFreeList));
        RWIMAGEGLOBAL(imageFreeList) = NULL;
        return NULL;
    }

    RWIMAGEGLOBAL(imagePath)[0] = '\0';

    imageModule.numInstances++;

    RwImageSetGamma((RwReal)1.0);

    RWIMAGEGLOBAL(imageFormats) = NULL;

    RWIMAGEGLOBAL(scratchMemSize) = 256;
    RWIMAGEGLOBAL(scratchMem) = (RwUInt8*)RwMalloc(RWIMAGEGLOBAL(scratchMemSize));
    if (!RWIMAGEGLOBAL(scratchMem))
    {
        RwFree(RWIMAGEGLOBAL(imagePath));
        RWIMAGEGLOBAL(imagePath) = NULL;
        RWIMAGEGLOBAL(imagePathSize) = 0;
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFormatFreeList));
        RWIMAGEGLOBAL(imageFormatFreeList) = NULL;
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFreeList));
        RWIMAGEGLOBAL(imageFreeList) = NULL;
        return NULL;
    }

    return instance;
}

void* _rwImageClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWIMAGEGLOBAL(scratchMem))
    {
        RwFree(RWIMAGEGLOBAL(scratchMem));
        RWIMAGEGLOBAL(scratchMem) = NULL;
        RWIMAGEGLOBAL(scratchMemSize) = 0;
    }

    if (RWIMAGEGLOBAL(imagePath))
    {
        RwFree(RWIMAGEGLOBAL(imagePath));
        RWIMAGEGLOBAL(imagePath) = NULL;
        RWIMAGEGLOBAL(imagePathSize) = 0;
    }

    while (RWIMAGEGLOBAL(imageFormats))
    {
        rwImageFormat* formatToDestroy = RWIMAGEGLOBAL(imageFormats);

        RWIMAGEGLOBAL(imageFormats) = formatToDestroy->nextFormat;
        RwFreeListFree(RWIMAGEGLOBAL(imageFormatFreeList), formatToDestroy);
    }

    if (RWIMAGEGLOBAL(imageFormatFreeList))
    {
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFormatFreeList));
        RWIMAGEGLOBAL(imageFormatFreeList) = NULL;
    }

    if (RWIMAGEGLOBAL(imageFreeList))
    {
        RwFreeListDestroy(RWIMAGEGLOBAL(imageFreeList));
        RWIMAGEGLOBAL(imageFreeList) = NULL;
    }

    imageModule.numInstances--;

    return instance;
}

static RwUInt8* ImageGetScratchMem(RwInt32 size)
{
    if (size > RWIMAGEGLOBAL(scratchMemSize))
    {
        RwUInt8* newBuf;

        if (RWIMAGEGLOBAL(scratchMem))
        {
            newBuf = (RwUInt8*)RwRealloc(RWIMAGEGLOBAL(scratchMem), size);
        }
        else
        {
            newBuf = (RwUInt8*)RwMalloc(size);
        }

        if (!newBuf)
        {
            RWERROR((E_RW_NOMEM, size));
            return NULL;
        }

        RWIMAGEGLOBAL(scratchMem) = newBuf;
        RWIMAGEGLOBAL(scratchMemSize) = size;
    }

    return RWIMAGEGLOBAL(scratchMem);
}

void _rwImageGammaCorrectArrayOfRGBA(RwRGBA* rgbaOut, RwRGBA* rgbaIn, RwInt32 numEls)
{
    RwUInt8* gammaTab = RWIMAGEGLOBAL(gammaTable);

    while (numEls--)
    {
        rgbaOut->red = gammaTab[rgbaIn->red];
        rgbaOut->green = gammaTab[rgbaIn->green];
        rgbaOut->blue = gammaTab[rgbaIn->blue];
        rgbaOut->alpha = rgbaIn->alpha;

        rgbaOut++;
        rgbaIn++;
    }
}

RwImage* RwImageCreate(RwInt32 width, RwInt32 height, RwInt32 depth)
{
    RwImage* image;

    image = (RwImage*)RwFreeListAlloc(RWIMAGEGLOBAL(imageFreeList));
    if (!image)
    {
        return NULL;
    }

    image->width = width;
    image->height = height;
    image->depth = depth;
    image->cpPixels = NULL;
    image->palette = NULL;
    image->flags = 0;

    _rwPluginRegistryInitObject(&imageTKList, image);

    return image;
}

RwBool RwImageDestroy(RwImage* image)
{
    if (image->flags & rwIMAGEALLOCATED)
    {
        RwImageFreePixels(image);
    }

    _rwPluginRegistryDeInitObject(&imageTKList, image);

    RwFreeListFree(RWIMAGEGLOBAL(imageFreeList), image);

    return TRUE;
}

RwImage* RwImageAllocatePixels(RwImage* image)
{
    RwUInt32 imageDepth = image->depth;
    RwBool imagePalette = FALSE;
    RwUInt32 paletteSize;
    RwUInt32 pixelsSize;
    RwUInt32 totalSize;

    if (imageDepth == 4 || imageDepth == 8)
    {
        imagePalette = TRUE;
    }

    paletteSize = imagePalette ? (1 << imageDepth) * sizeof(RwRGBA) : 0;

    image->stride = (image->depth + 7) >> 3;
    image->stride *= image->width;
    image->stride = (image->stride + 3) & ~3;

    pixelsSize = image->stride * image->height;
    totalSize = pixelsSize + paletteSize;

    image->cpPixels = (RwUInt8*)RwMalloc(totalSize);
    if (!image->cpPixels)
    {
        RWERROR((E_RW_NOMEM, totalSize));
        return NULL;
    }

    /* The palette lives after the pixels */
    image->palette = imagePalette ? (RwRGBA*)(image->cpPixels + pixelsSize) : NULL;
    image->flags |= rwIMAGEALLOCATED;

    return image;
}

RwImage* RwImageFreePixels(RwImage* image)
{
    RwFree(image->cpPixels);

    image->cpPixels = NULL;
    image->palette = NULL;
    image->flags &= ~rwIMAGEALLOCATED;

    return image;
}

RwImage* RwImageMakeMask(RwImage* image)
{
    RwInt32 i;

    switch (image->depth)
    {
    case 4:
    case 8:
    {
        RwInt32 palSize = 1 << image->depth;
        RwRGBA* rpPal = image->palette;

        for (i = 0; i < palSize; i++)
        {
            RwInt32 nOpacity = rpPal[i].red;

            if (rpPal[i].green > nOpacity)
            {
                nOpacity = rpPal[i].green;
            }

            if (rpPal[i].blue > nOpacity)
            {
                nOpacity = rpPal[i].blue;
            }

            rpPal[i].alpha = (RwUInt8)nOpacity;
        }
        break;
    }
    case 32:
    {
        RwUInt8* cpSpan = image->cpPixels;

        for (i = 0; i < image->height; i++)
        {
            RwRGBA* rpCur = (RwRGBA*)cpSpan;
            RwInt32 j;

            for (j = 0; j < image->width; j++)
            {
                RwInt32 nOpacity = rpCur[j].red;

                if (rpCur[j].green > nOpacity)
                {
                    nOpacity = rpCur[j].green;
                }

                if (rpCur[j].blue > nOpacity)
                {
                    nOpacity = rpCur[j].blue;
                }

                rpCur[j].alpha = (RwUInt8)nOpacity;
            }

            cpSpan += image->stride;
        }
        break;
    }
    }

    return image;
}

RwImage* RwImageApplyMask(RwImage* image, const RwImage* mask)
{
    RwInt32 i;
    RwInt32 j;

    if (image->width != mask->width || image->height != mask->height)
    {
        RWERROR((E_RW_INVIMAGESIZE));
        return NULL;
    }

    switch (image->depth)
    {
    case 4:
    case 8:
    {
        /* Promote the image to 32 bits so it can hold an alpha channel */
        RwImage* tempImage = RwImageCreate(image->width, image->height, image->depth);

        if (!tempImage)
        {
            return NULL;
        }

        if (!RwImageAllocatePixels(tempImage))
        {
            RwImageDestroy(tempImage);
            return NULL;
        }

        RwImageCopy(tempImage, image);

        if (image->flags & rwIMAGEALLOCATED)
        {
            RwImageFreePixels(image);
        }

        image->depth = 32;
        RwImageAllocatePixels(image);
        RwImageCopy(image, tempImage);

        RwImageFreePixels(tempImage);
        RwImageDestroy(tempImage);
    }
        /* Fall through */
    case 32:
    {
        const RwUInt8* cpSrc = mask->cpPixels;
        const RwRGBA* rpPal = mask->palette;
        RwUInt8* cpDst = image->cpPixels;

        for (i = 0; i < image->height; i++)
        {
            RwRGBA* dstRGB = (RwRGBA*)cpDst;

            switch (mask->depth)
            {
            case 4:
            case 8:
            {
                const RwUInt8* srcInd = cpSrc;

                for (j = 0; j < image->width; j++)
                {
                    dstRGB->alpha = rpPal[*srcInd].alpha;
                    srcInd++;
                    dstRGB++;
                }
                break;
            }
            case 32:
            {
                const RwRGBA* srcRGB = (const RwRGBA*)cpSrc;

                for (j = 0; j < image->width; j++)
                {
                    dstRGB->alpha = srcRGB->alpha;
                    srcRGB++;
                    dstRGB++;
                }
                break;
            }
            }

            cpSrc += mask->stride;
            cpDst += image->stride;
        }
        break;
    }
    default:
        RWERROR((E_RW_INVIMAGEDEPTH));
        return NULL;
    }

    return image;
}

/* Calls back with every full name the image path produces for the file */
static const RwChar* ImagePathForAllFullNames(const RwChar* filename, RwInt32 extraBytes,
                                              rwImagePathCallBack callBack, void* data)
{
    RwInt32 pathsize;
    RwChar* fullname;
    RwChar* pathElement;
    RwChar* nextPathElement;
    RwInt32 pathElementLength;

    pathElement = RWIMAGEGLOBAL(imagePath);

    if (_rwpathisabsolute(filename) || !pathElement || !*pathElement)
    {
        pathsize = extraBytes + rwstrlen(filename);

        fullname = (RwChar*)ImageGetScratchMem(pathsize);
        if (!fullname)
        {
            return NULL;
        }

        rwstrcpy(fullname, filename);
        callBack(fullname, data);
    }
    else
    {
        while (pathElement && *pathElement)
        {
            rwstrchr(pathElement, ';');
            nextPathElement = rwstrchr(pathElement, ';');
            if (nextPathElement)
            {
                pathElementLength = nextPathElement - pathElement;
                nextPathElement++;
            }
            else
            {
                pathElementLength = rwstrlen(pathElement);
            }

            pathsize = pathElementLength + rwstrlen(filename) + extraBytes;

            fullname = (RwChar*)ImageGetScratchMem(pathsize);
            if (!fullname)
            {
                return NULL;
            }

            memcpy(fullname, pathElement, pathElementLength);
            rwstrcpy(fullname + pathElementLength, filename);

            if (!callBack(fullname, data))
            {
                return filename;
            }

            pathElement = nextPathElement;
        }
    }

    return filename;
}

static RwChar* ImageAttempRead(RwChar* pathname, void* data)
{
    _imageReadData* imageData = (_imageReadData*)data;

    if (RwFexist(pathname))
    {
        imageData->image = imageData->readImage(pathname);
        if (imageData->image)
        {
            /* Got it, stop looking */
            return NULL;
        }
    }

    return pathname;
}

RwImage* RwImageRead(const RwChar* imageName)
{
    const RwChar* lastSeparator;
    const RwChar* testSeparator;
    const RwChar* extender;
    rwImageFormat* imageFormat;
    _imageReadData imageData;

    /* Find the start of the file name proper */
    testSeparator = rwstrrchr(imageName, ':');
    lastSeparator = testSeparator ? testSeparator : imageName;

    testSeparator = rwstrrchr(lastSeparator, '/');
    lastSeparator = testSeparator ? testSeparator : lastSeparator;

    testSeparator = rwstrrchr(lastSeparator, '\\');
    lastSeparator = testSeparator ? testSeparator : lastSeparator;

    extender = rwstrrchr(lastSeparator, '.');
    if (extender)
    {
        imageFormat = RWIMAGEGLOBAL(imageFormats);
        while (imageFormat)
        {
            if (!rwstrcmp(imageFormat->lcExtension, extender) ||
                !rwstrcmp(imageFormat->ucExtension, extender))
            {
                if (imageFormat->readImage)
                {
                    imageData.readImage = imageFormat->readImage;
                    imageData.image = NULL;

                    ImagePathForAllFullNames(imageName, 5, ImageAttempRead, &imageData);

                    return imageData.image;
                }

                return NULL;
            }

            imageFormat = imageFormat->nextFormat;
        }

        return NULL;
    }

    return NULL;
}

static RwChar* ImageDetermineExtender(RwChar* pathname, void* data)
{
    RwChar** extender = (RwChar**)data;
    rwImageFormat* imageFormat;
    RwChar* extPos;

    extPos = pathname + rwstrlen(pathname);

    imageFormat = RWIMAGEGLOBAL(imageFormats);
    while (imageFormat)
    {
        rwstrcpy(extPos, imageFormat->lcExtension);
        if (RwFexist(pathname))
        {
            *extender = imageFormat->lcExtension;
            return NULL;
        }

        rwstrcpy(extPos, imageFormat->ucExtension);
        if (RwFexist(pathname))
        {
            *extender = imageFormat->ucExtension;
            return NULL;
        }

        imageFormat = imageFormat->nextFormat;
    }

    return pathname;
}

const RwChar* RwImageFindFileType(const RwChar* imageName)
{
    RwChar* extender = NULL;

    ImagePathForAllFullNames(imageName, rwIMAGEFORMATEXTENSIONLENGTH, ImageDetermineExtender,
                             &extender);

    return extender;
}

RwImage* RwImageReadMaskedImage(const RwChar* imageName, const RwChar* maskName)
{
    RwImage* image;
    RwImage* mask;

    image = RwImageRead(imageName);
    if (image)
    {
        if (maskName && maskName[0])
        {
            mask = RwImageRead(maskName);
            if (!mask)
            {
                RwImageDestroy(image);
                return NULL;
            }

            if (!RwImageMakeMask(mask))
            {
                RwImageDestroy(image);
                RwImageDestroy(mask);
                return NULL;
            }

            if (!RwImageApplyMask(image, mask))
            {
                RwImageDestroy(image);
                RwImageDestroy(mask);
                return NULL;
            }

            RwImageDestroy(mask);
        }

        return image;
    }

    return NULL;
}

RwRGBA* RwRGBASetFromPixel(RwRGBA* rgbOut, RwUInt32 pixelValue, RwInt32 rasterFormat)
{
    RWSRCGLOBAL(stdFunc[rwSTANDARDPIXELTORGB])(rgbOut, &pixelValue, rasterFormat);

    return rgbOut;
}

static RwBool ImageStraightCopy(RwImage* ipDestin, const RwImage* ipSource)
{
    RwInt32 i;
    RwInt32 nSpanLength;
    const RwUInt8* cpSrc;
    RwUInt8* cpDst;

    nSpanLength = ((ipDestin->depth + 7) >> 3) * ipDestin->width;

    cpSrc = ipSource->cpPixels;
    cpDst = ipDestin->cpPixels;

    for (i = 0; i < ipDestin->height; i++)
    {
        memcpy(cpDst, cpSrc, nSpanLength);

        cpSrc += ipSource->stride;
        cpDst += ipDestin->stride;
    }

    return TRUE;
}

static RwBool ImageConvertDepth(RwImage* ipDestin, const RwImage* ipSource)
{
    RwBool result = FALSE;
    RwInt32 i;
    RwInt32 j;
    RwInt32 width = ipDestin->width;
    RwInt32 height = ipDestin->height;
    RwUInt32 switchKey = (ipSource->depth << 8) | ipDestin->depth;
    RwUInt8* cpDst;
    const RwRGBA* rpSrcPalette;
    const RwUInt8* cpSrc;

    rpSrcPalette = ipSource->palette;
    cpSrc = ipSource->cpPixels;
    cpDst = ipDestin->cpPixels;

    switch (switchKey)
    {
    case 0x0404:
    case 0x0808:
    case 0x2020:
        result = TRUE;
        break;
    case 0x0408:
        /* Palette indices widen without change */
        for (i = 0; i < height; i++)
        {
            memcpy(cpDst, cpSrc, width);

            cpSrc += ipSource->stride;
            cpDst += ipDestin->stride;
        }

        result = TRUE;
        break;
    case 0x0420:
    case 0x0820:
        for (i = 0; i < height; i++)
        {
            RwRGBA* rpDst = (RwRGBA*)cpDst;

            for (j = 0; j < width; j++)
            {
                rpDst[j] = rpSrcPalette[cpSrc[j]];
            }

            cpSrc += ipSource->stride;
            cpDst += ipDestin->stride;
        }

        result = TRUE;
        break;
    case 0x0804:
    case 0x2004:
    case 0x2008:
    default:
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }

    return result;
}

RwImage* RwImageCopy(RwImage* destImage, const RwImage* sourceImage)
{
    if (destImage->depth == sourceImage->depth)
    {
        if (destImage->palette && sourceImage->palette && sourceImage->depth <= 8)
        {
            memcpy(destImage->palette, sourceImage->palette,
                   (1 << sourceImage->depth) * sizeof(RwRGBA));
        }

        ImageStraightCopy(destImage, sourceImage);
    }
    else if (!ImageConvertDepth(destImage, sourceImage))
    {
        destImage = NULL;
    }

    destImage->flags &= ~rwIMAGEGAMMACORRECTED;
    destImage->flags |= sourceImage->flags & rwIMAGEGAMMACORRECTED;

    return destImage;
}

RwImage* RwImageGammaCorrect(RwImage* image)
{
    switch (image->depth)
    {
    case 4:
    case 8:
    {
        RwRGBA* pal = image->palette;
        RwUInt32 palSize = 1 << image->depth;

        if (!pal)
        {
            RWERROR((E_RW_NULLP));
            return NULL;
        }

        _rwImageGammaCorrectArrayOfRGBA(pal, pal, palSize);
        break;
    }
    case 32:
    {
        RwUInt8* curLine = image->cpPixels;
        RwInt32 width = image->width;
        RwInt32 height = image->height;
        RwInt32 y;

        if (!curLine)
        {
            RWERROR((E_RW_NULLP));
            return NULL;
        }

        for (y = 0; y < height; y++)
        {
            _rwImageGammaCorrectArrayOfRGBA((RwRGBA*)curLine, (RwRGBA*)curLine, width);
            curLine += image->stride;
        }
        break;
    }
    default:
        RWERROR((E_RW_INVIMAGEFORMAT));
        return NULL;
    }

    image->flags |= rwIMAGEGAMMACORRECTED;

    return image;
}

RwBool RwImageSetGamma(RwReal gammaValue)
{
    RwReal nGammaInv;
    RwInt32 i;

    RWIMAGEGLOBAL(gammaVal) = gammaValue;
    nGammaInv = (RwReal)1.0 / gammaValue;

    RWIMAGEGLOBAL(gammaTable)[0] = 0;
    RWIMAGEGLOBAL(invGammaTable)[0] = 0;

    for (i = 1; i < 256; i++)
    {
        RwReal nT = (RwReal)i / (RwReal)255.0;
        RwReal scaled;

        scaled = (RwReal)255.0 * (RwReal)pow(nT, nGammaInv);
        RWIMAGEGLOBAL(gammaTable)[i] = (RwUInt8)((RwReal)0.5 + scaled);

        scaled = (RwReal)255.0 * (RwReal)pow(nT, gammaValue);
        RWIMAGEGLOBAL(invGammaTable)[i] = (RwUInt8)((RwReal)0.5 + scaled);
    }

    return TRUE;
}
