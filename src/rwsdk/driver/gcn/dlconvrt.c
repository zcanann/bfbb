#include <rwsdk/rwcore.h>

#include "rwsdk/driver/gcn/dlprivate.h"
#include "rwsdk/driver/common/palquant.h"

RwInt32 _rwDlFindMSB(RwInt32 num)
{
    RwInt32 pos = -1;

    while (num)
    {
        num >>= 1;
        pos++;
    }

    return pos;
}

static RwUInt32 _rwDlConv8888To555(RwRGBA* pixIn)
{
    RwUInt32 pixOut;

    pixOut = 0x8000 | (((RwUInt32)pixIn->red << 7) & 0x7C00) | (((RwUInt32)pixIn->green << 2) & 0x03E0) |
             ((RwUInt32)pixIn->blue >> 3);

    return pixOut;
}

static RwUInt32 _rwDlConv8888To565(RwRGBA* pixIn)
{
    RwUInt32 pixOut;

    pixOut = (((RwUInt32)pixIn->red << 8) & 0xF800) | (((RwUInt32)pixIn->green << 3) & 0x07E0) |
             ((RwUInt32)pixIn->blue >> 3);

    return pixOut;
}

static RwUInt32 _rwDlConv8888To555or3444(RwRGBA* pixIn)
{
    RwUInt32 pixOut;

    if (pixIn->alpha != 0xFF)
    {
        pixOut = (((RwUInt32)pixIn->alpha << 7) & 0x7000) | (((RwUInt32)pixIn->red << 4) & 0x0F00) |
                 ((RwUInt32)pixIn->green & 0x00F0) | ((RwUInt32)pixIn->blue >> 4);
    }
    else
    {
        pixOut = _rwDlConv8888To555(pixIn);
    }

    return pixOut;
}

static RwUInt32 _rwDlConv8888ToDl888(RwRGBA* pixIn)
{
    RwUInt32 pixOut;

    pixOut = 0xFF000000 | (pixIn->red << 16) | (pixIn->green << 8) | pixIn->blue;

    return pixOut;
}

static RwUInt32 _rwDlConv8888ToDl8888(RwRGBA* pixIn)
{
    RwUInt32 pixOut;

    pixOut = (pixIn->alpha << 24) | (pixIn->red << 16) | (pixIn->green << 8) | pixIn->blue;

    return pixOut;
}

RwBool _rwDlRGBToPixel(void* pixelOut, void* colIn, RwInt32 format)
{
    RwRGBA* rgba = (RwRGBA*)colIn;
    RwInt32 pixVal;

    switch (format & rwRASTERFORMATPIXELFORMATMASK)
    {
    case rwRASTERFORMATDEFAULT:
    {
        pixVal = _rwDlConv8888To565(rgba);
        break;
    }
    case rwRASTERFORMATLUM8:
    {
        RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMATLUM8 not yet supported"));
        break;
    }
    case rwRASTERFORMAT1555:
    case rwRASTERFORMAT4444:
    {
        pixVal = _rwDlConv8888To555or3444(rgba);
        break;
    }
    case rwRASTERFORMAT555:
    {
        pixVal = _rwDlConv8888To555(rgba);
        break;
    }
    case rwRASTERFORMAT565:
    {
        pixVal = _rwDlConv8888To565(rgba);
        break;
    }
    case rwRASTERFORMAT8888:
    {
        pixVal = _rwDlConv8888ToDl8888(rgba);
        break;
    }
    case rwRASTERFORMAT888:
    {
        pixVal = _rwDlConv8888ToDl888(rgba);
        break;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERFORMAT));
        break;
    }
    }

    *(RwInt32*)pixelOut = *(RwInt32*)&pixVal;

    return TRUE;
}

static void _rwDlConv555To8888(RwRGBA* pixOut, RwUInt32 pixIn)
{
    pixOut->red = (RwUInt8)((pixIn >> 7) & 0xF8);
    pixOut->green = (RwUInt8)((pixIn >> 2) & 0xF8);
    pixOut->blue = (RwUInt8)((pixIn << 3) & 0xF8);
    pixOut->alpha = 0xFF;
}

static void _rwDlConv565To8888(RwRGBA* pixOut, RwUInt32 pixIn)
{
    pixOut->red = (RwUInt8)((pixIn >> 8) & 0xF8);
    pixOut->green = (RwUInt8)((pixIn >> 3) & 0xFC);
    pixOut->blue = (RwUInt8)((pixIn << 3) & 0xF8);
    pixOut->alpha = 0xFF;
}

static void _rwDlConv1555To8888(RwRGBA* pixOut, RwUInt32 pixIn)
{
    if (pixIn & 0x8000)
    {
        _rwDlConv555To8888(pixOut, pixIn);
    }
    else
    {
        pixOut->red = (RwUInt8)((pixIn >> 4) & 0xF0);
        pixOut->green = (RwUInt8)(pixIn & 0xF0);
        pixOut->blue = (RwUInt8)((pixIn << 4) & 0xF0);
        pixOut->alpha = 0x00;
    }
}

static void _rwDlConv4444To8888(RwRGBA* pixOut, RwUInt32 pixIn)
{
    if (pixIn & 0x8000)
    {
        _rwDlConv555To8888(pixOut, pixIn);
    }
    else
    {
        pixOut->red = (RwUInt8)((pixIn >> 4) & 0xF0);
        pixOut->green = (RwUInt8)(pixIn & 0xF0);
        pixOut->blue = (RwUInt8)((pixIn << 4) & 0xF0);
        pixOut->alpha = (RwUInt8)((pixIn >> 7) & 0xE0);
    }
}

static void _rwDlConvDl888To8888(RwRGBA* pixOut, RwUInt32 pixIn)
{
    pixOut->alpha = 0xFF;
    pixOut->red = (RwUInt8)(pixIn >> 16);
    pixOut->green = (RwUInt8)(pixIn >> 8);
    pixOut->blue = (RwUInt8)pixIn;
}

static void _rwDlConvDl8888To8888(RwRGBA* pixOut, RwUInt32 pixIn)
{
    pixOut->alpha = (RwUInt8)(pixIn >> 24);
    pixOut->red = (RwUInt8)(pixIn >> 16);
    pixOut->green = (RwUInt8)(pixIn >> 8);
    pixOut->blue = (RwUInt8)pixIn;
}

typedef void (*rwDlUnconvertFn)(RwRGBA* pixOut, RwUInt32 pixIn);

static rwDlUnconvertFn _rwDlSelectUnconvertFn(RwInt32 format)
{
    rwDlUnconvertFn result = NULL;

    switch (format & rwRASTERFORMATPIXELFORMATMASK)
    {
    case rwRASTERFORMATLUM8:
    {
        RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMATLUM8 not yet supported"));
        break;
    }
    case rwRASTERFORMAT555:
    {
        result = _rwDlConv555To8888;
        break;
    }
    case rwRASTERFORMAT565:
    {
        result = _rwDlConv565To8888;
        break;
    }
    case rwRASTERFORMAT1555:
    {
        result = _rwDlConv1555To8888;
        break;
    }
    case rwRASTERFORMAT4444:
    {
        result = _rwDlConv4444To8888;
        break;
    }
    case rwRASTERFORMAT8888:
    {
        result = _rwDlConvDl8888To8888;
        break;
    }
    case rwRASTERFORMAT888:
    {
        result = _rwDlConvDl888To8888;
        break;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERFORMAT));
        break;
    }
    }

    return result;
}

RwBool _rwDlPixelToRGB(void* rgbOut, void* pixel, RwInt32 format)
{
    RwRGBA* rgba = (RwRGBA*)rgbOut;
    RwInt32 pixVal = *(RwInt32*)pixel;

    switch (format & rwRASTERFORMATPIXELFORMATMASK)
    {
    case rwRASTERFORMATDEFAULT:
    {
        _rwDlSelectUnconvertFn(rwRASTERFORMAT565)(rgba, pixVal);
        break;
    }
    case rwRASTERFORMATLUM8:
    {
        RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMATLUM8 not yet supported"));
        break;
    }
    case rwRASTERFORMAT555:
    {
        _rwDlConv555To8888(rgba, pixVal);
        break;
    }
    case rwRASTERFORMAT1555:
    {
        _rwDlConv1555To8888(rgba, pixVal);
        break;
    }
    case rwRASTERFORMAT565:
    {
        _rwDlConv565To8888(rgba, pixVal);
        break;
    }
    case rwRASTERFORMAT4444:
    {
        _rwDlConv4444To8888(rgba, pixVal);
        break;
    }
    case rwRASTERFORMAT888:
    {
        _rwDlConvDl888To8888(rgba, pixVal);
        break;
    }
    case rwRASTERFORMAT8888:
    {
        _rwDlConvDl8888To8888(rgba, pixVal);
        break;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERFORMAT));
        break;
    }
    }

    return TRUE;
}

static void _rwDlImage4GetFromRaster(RwImage* image, RwRaster* raster)
{
    rwDlUnconvertFn unConvFn;

    unConvFn = _rwDlSelectUnconvertFn(RwRasterGetFormat(raster));

    switch (raster->depth)
    {
    case 4:
    {
        RwInt32 y;
        RwUInt16 paletteEntry;
        RwInt32 x;

        for (y = 0; y < 16; y++)
        {
            paletteEntry = ((RwUInt16*)raster->palette)[y];
            unConvFn(&image->palette[y], paletteEntry);
        }

        for (y = 0; y < raster->height; y++)
        {
            RwUInt8* dstPixel = image->cpPixels + image->stride * y;
            RwUInt8* srcPixel = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x += 2)
            {
                dstPixel[0] = (RwUInt8)((*srcPixel & 0xF0) >> 4);
                dstPixel[1] = (RwUInt8)(*srcPixel & 0x0F);
                srcPixel++;
                dstPixel += 2;
            }
        }
        break;
    }
    case 8:
    case 16:
    case 32:
    {
        RWERROR((E_RW_DEVICEERROR, "Conversion from 8/16/32bit rasters to 4bit images is not supported"));
        break;
    }
    case 24:
    default:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
    }
}

static void _rwDlImage8GetFromRaster(RwImage* image, RwRaster* raster)
{
    rwDlUnconvertFn unConvFn;

    unConvFn = _rwDlSelectUnconvertFn(RwRasterGetFormat(raster));

    switch (raster->depth)
    {
    case 4:
    {
        RwInt32 y;
        RwUInt16 paletteEntry;
        RwInt32 x;

        for (y = 0; y < 16; y++)
        {
            paletteEntry = ((RwUInt16*)raster->palette)[y];
            unConvFn(&image->palette[y], paletteEntry);
        }

        for (y = 0; y < raster->height; y++)
        {
            RwUInt8* dstPixel = image->cpPixels + image->stride * y;
            RwUInt8* srcPixel = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x += 2)
            {
                dstPixel[0] = (RwUInt8)((*srcPixel & 0xF0) >> 4);
                dstPixel[1] = (RwUInt8)(*srcPixel & 0x0F);
                srcPixel++;
                dstPixel += 2;
            }
        }
        break;
    }
    case 8:
    {
        RwInt32 y;
        RwUInt16 paletteEntry;
        RwInt32 x;

        for (y = 0; y < 256; y++)
        {
            paletteEntry = ((RwUInt16*)raster->palette)[y];
            unConvFn(&image->palette[y], paletteEntry);
        }

        for (y = 0; y < raster->height; y++)
        {
            RwUInt8* srcPixel = raster->cpPixels + raster->stride * y;
            RwUInt8* dstPixel = image->cpPixels + image->stride * y;

            for (x = 0; x < raster->width; x++)
            {
                *dstPixel = *srcPixel;
                srcPixel++;
                dstPixel++;
            }
        }
        break;
    }
    case 16:
    case 32:
    {
        RWERROR((E_RW_DEVICEERROR, "Conversion from 16/32bit rasters to 8bit images is not supported"));
        break;
    }
    case 24:
    default:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
    }
}

static void _rwDlImage32GetFromRaster(RwImage* image, RwRaster* raster)
{
    rwDlUnconvertFn unConvFn;

    unConvFn = _rwDlSelectUnconvertFn(RwRasterGetFormat(raster));

    switch (raster->depth)
    {
    case 4:
    {
        RwInt32 y;
        RwRGBA palette[16];
        RwUInt16 paletteEntry;
        RwInt32 x;

        for (y = 0; y < 16; y++)
        {
            paletteEntry = ((RwUInt16*)raster->palette)[y];
            unConvFn(&palette[y], paletteEntry);
        }

        for (y = 0; y < raster->height; y++)
        {
            RwRGBA* dstPixel = (RwRGBA*)(image->cpPixels + image->stride * y);
            RwUInt8* srcPixel = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x += 2)
            {
                dstPixel[0] = palette[(*srcPixel & 0xF0) >> 4];
                dstPixel[1] = palette[*srcPixel & 0x0F];
                srcPixel++;
                dstPixel += 2;
            }
        }
        break;
    }
    case 8:
    {
        RwInt32 y;
        RwRGBA palette[256];
        RwUInt16 paletteEntry;
        RwInt32 x;

        for (y = 0; y < 256; y++)
        {
            paletteEntry = ((RwUInt16*)raster->palette)[y];
            unConvFn(&palette[y], paletteEntry);
        }

        for (y = 0; y < raster->height; y++)
        {
            RwUInt8* srcPixel = raster->cpPixels + raster->stride * y;
            RwRGBA* dstPixel = (RwRGBA*)(image->cpPixels + image->stride * y);

            for (x = 0; x < raster->width; x++)
            {
                *dstPixel = palette[*srcPixel];
                srcPixel++;
                dstPixel++;
            }
        }
        break;
    }
    case 16:
    {
        RwInt32 j;
        RwInt32 i;

        for (j = 0; j < raster->height; j++)
        {
            RwUInt16* srcPixel = (RwUInt16*)(raster->cpPixels + raster->stride * j);
            RwRGBA* dstPixel = (RwRGBA*)(image->cpPixels + image->stride * j);

            for (i = 0; i < raster->width; i++)
            {
                unConvFn(dstPixel, *srcPixel);
                dstPixel++;
                srcPixel++;
            }
        }
        break;
    }
    case 24:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
    case 32:
    {
        RwInt32 y;
        RwInt32 x;

        for (y = 0; y < raster->height; y++)
        {
            RwUInt32* srcPixel = (RwUInt32*)(raster->cpPixels + raster->stride * y);
            RwRGBA* dstPixel = (RwRGBA*)(image->cpPixels + image->stride * y);

            for (x = 0; x < raster->width; x++)
            {
                unConvFn(dstPixel, *srcPixel);
                dstPixel++;
                srcPixel++;
            }
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
    }
}

RwBool _rwDlImageGetFromRaster(void* imageIn, void* rasterIn, RwInt32 flags)
{
    RwImage* image = (RwImage*)imageIn;
    RwRaster* raster = (RwRaster*)rasterIn;
    RwBool rasterLocked = FALSE;
    RwBool paletteLocked = FALSE;

    if (!(raster->privateFlags & rwRASTERPIXELLOCKEDREAD))
    {
        RwRasterLock(raster, 0, rwRASTERLOCKREAD);
        rasterLocked = TRUE;
    }

    if ((RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) &&
        !(raster->privateFlags & rwRASTERPALETTELOCKEDREAD))
    {
        RwRasterLockPalette(raster, rwRASTERLOCKREAD);
        paletteLocked = TRUE;
    }

    switch (image->depth)
    {
    case 4:
    {
        _rwDlImage4GetFromRaster(image, raster);
        break;
    }
    case 8:
    {
        _rwDlImage8GetFromRaster(image, raster);
        break;
    }
    case 32:
    {
        _rwDlImage32GetFromRaster(image, raster);
        break;
    }
    default:
    {
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
    }

    if (paletteLocked == TRUE)
    {
        RwRasterUnlockPalette(raster);
    }

    if (rasterLocked == TRUE)
    {
        RwRasterUnlock(raster);
    }

    return TRUE;
}

typedef RwUInt32 (*rwDlConvertFn)(RwRGBA* pixIn);

static rwDlConvertFn _rwDlSelectConvertFn(RwRaster* raster)
{
    rwDlConvertFn convFn = NULL;

    switch (RwRasterGetFormat(raster) & rwRASTERFORMATPIXELFORMATMASK)
    {
    case rwRASTERFORMATLUM8:
    {
        RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMATLUM8 not yet supported"));
        break;
    }
    case rwRASTERFORMAT555:
    {
        convFn = _rwDlConv8888To555;
        break;
    }
    case rwRASTERFORMAT565:
    {
        convFn = _rwDlConv8888To565;
        break;
    }
    case rwRASTERFORMAT1555:
    case rwRASTERFORMAT4444:
    {
        convFn = _rwDlConv8888To555or3444;
        break;
    }
    case rwRASTERFORMAT888:
    {
        convFn = _rwDlConv8888ToDl888;
        break;
    }
    case rwRASTERFORMAT8888:
    {
        convFn = _rwDlConv8888ToDl8888;
        break;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERFORMAT));
        break;
    }
    }

    return convFn;
}

static RwImage* _rwDolphinPalettizeImage(RwImage* srcImage, RwInt32 depth)
{
    RwPalQuant palQuant;
    RwImage* palImage;

    palImage = RwImageCreate(srcImage->width, srcImage->height, depth);
    if (!palImage)
    {
        return NULL;
    }

    RwImageAllocatePixels(palImage);

    if (!_rwPalQuantInit(&palQuant))
    {
        return NULL;
    }

    _rwPalQuantAddImage(&palQuant, srcImage, 1.0f);
    _rwPalQuantResolvePalette(palImage->palette, 1 << depth, &palQuant);
    _rwPalQuantMatchImage(palImage->cpPixels, palImage->stride, palImage->depth, FALSE, &palQuant,
                          srcImage);
    _rwPalQuantTerm(&palQuant);

    return palImage;
}

static void _rwDlRasterPalletized4SetFromImage(RwRaster* raster, RwImage* image)
{
    RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster->parent);
    rwDlConvertFn convFn;

    convFn = _rwDlSelectConvertFn(raster);

    switch (image->depth)
    {
    case 4:
    {
        RwInt32 y;

        for (y = 0; y < raster->height; y++)
        {
            RwInt32 x;
            RwUInt8* srcPixel = image->cpPixels + image->stride * y;
            RwUInt8* dstPixel = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x += 2)
            {
                *dstPixel = (RwUInt8)(((srcPixel[0] & 0x0F) << 4) | (srcPixel[1] & 0x0F));
                srcPixel += 2;
                dstPixel++;
            }
        }

        if (rasExt->lockedMipLevel == 0)
        {
            RwInt32 x;
            RwUInt16* palette = (RwUInt16*)raster->palette;

            for (x = 0; x < 16; x++)
            {
                palette[x] = (RwUInt16)convFn(&image->palette[x]);
            }
        }
        break;
    }
    case 8:
    case 32:
    {
        RwImage* palImage;

        palImage = _rwDolphinPalettizeImage(image, raster->depth);
        if (palImage)
        {
            _rwDlRasterPalletized4SetFromImage(raster, palImage);
            RwImageDestroy(palImage);
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
    }
}

static void _rwDlRasterPalletized8SetFromImage(RwRaster* raster, RwImage* image)
{
    rwDlConvertFn convFn;

    convFn = _rwDlSelectConvertFn(raster);

    switch (image->depth)
    {
    case 4:
    case 8:
    {
        RwInt32 y;
        RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster->parent);

        for (y = 0; y < raster->height; y++)
        {
            RwInt32 x;
            RwUInt8* srcPixel = image->cpPixels + image->stride * y;
            RwUInt8* dstPixel = raster->cpPixels + raster->stride * y;

            for (x = 0; x < raster->width; x++)
            {
                *dstPixel = *srcPixel;
                srcPixel++;
                dstPixel++;
            }
        }

        if (rasExt->lockedMipLevel == 0)
        {
            RwInt32 x;
            RwUInt16* palette = (RwUInt16*)raster->palette;

            for (x = 0; x < (1 << image->depth); x++)
            {
                palette[x] = (RwUInt16)convFn(&image->palette[x]);
            }
        }
        break;
    }
    case 32:
    {
        RwImage* palImage;

        palImage = _rwDolphinPalettizeImage(image, raster->depth);
        if (palImage)
        {
            _rwDlRasterPalletized8SetFromImage(raster, palImage);
            RwImageDestroy(palImage);
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
    }
}

static void _rwDlRaster16SetFromImage(RwRaster* raster, RwImage* image)
{
    rwDlConvertFn convFn;

    convFn = _rwDlSelectConvertFn(raster);

    switch (image->depth)
    {
    case 4:
    case 8:
    {
        RwInt32 y;
        RwUInt16 palette[256];

        for (y = 0; y < (1 << image->depth); y++)
        {
            palette[y] = (RwUInt16)convFn(&image->palette[y]);
        }

        for (y = 0; y < raster->height; y++)
        {
            RwInt32 x;
            RwUInt8* srcPixel = image->cpPixels + image->stride * y;
            RwUInt16* dstPixel = (RwUInt16*)(raster->cpPixels + raster->stride * y);

            for (x = 0; x < raster->width; x++)
            {
                *dstPixel = palette[*srcPixel];
                srcPixel++;
                dstPixel++;
            }
        }
        break;
    }
    case 32:
    {
        RwInt32 y;

        for (y = 0; y < raster->height; y++)
        {
            RwInt32 x;
            RwRGBA* srcPixel = (RwRGBA*)(image->cpPixels + image->stride * y);
            RwUInt16* dstPixel = (RwUInt16*)(raster->cpPixels + raster->stride * y);

            for (x = 0; x < raster->width; x++)
            {
                *dstPixel = (RwUInt16)convFn(srcPixel);
                dstPixel++;
                srcPixel++;
            }
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
    }
}

static void _rwDlRaster32SetFromImage(RwRaster* raster, RwImage* image)
{
    rwDlConvertFn convFn;

    convFn = _rwDlSelectConvertFn(raster);

    switch (image->depth)
    {
    case 4:
    case 8:
    {
        RwInt32 y;
        RwUInt32 palette[256];

        for (y = 0; y < (1 << image->depth); y++)
        {
            palette[y] = (RwUInt32)convFn(&image->palette[y]);
        }

        for (y = 0; y < raster->height; y++)
        {
            RwInt32 x;
            RwUInt8* srcPixel = image->cpPixels + image->stride * y;
            RwUInt32* dstPixel = (RwUInt32*)(raster->cpPixels + raster->stride * y);

            for (x = 0; x < raster->width; x++)
            {
                *dstPixel = palette[*srcPixel];
                srcPixel++;
                dstPixel++;
            }
        }
        break;
    }
    case 32:
    {
        RwInt32 y;

        for (y = 0; y < raster->height; y++)
        {
            RwInt32 x;
            RwRGBA* srcPixel = (RwRGBA*)(image->cpPixels + image->stride * y);
            RwUInt32* dstPixel = (RwUInt32*)(raster->cpPixels + raster->stride * y);

            for (x = 0; x < raster->width; x++)
            {
                *dstPixel = (RwUInt32)convFn(srcPixel);
                dstPixel++;
                srcPixel++;
            }
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVIMAGEDEPTH));
        break;
    }
    }
}

RwBool _rwDlRasterSetFromImage(void* rasterIn, void* imageIn, RwInt32 flags)
{
    RwImage* image = (RwImage*)imageIn;
    RwRaster* raster = (RwRaster*)rasterIn;
    RwInt32 format = RwRasterGetFormat(raster);
    RwBool rasterLocked = FALSE;
    RwBool paletteLocked = FALSE;

    if (raster->privateFlags & rwRASTERPIXELLOCKEDWRITE)
    {
        rasterLocked = TRUE;
    }

    if (!rasterLocked)
    {
        if (!RwRasterLock(raster, 0, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH))
        {
            return FALSE;
        }
    }

    if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        if (raster->privateFlags & rwRASTERPALETTELOCKEDWRITE)
        {
            paletteLocked = TRUE;
        }

        if (!paletteLocked)
        {
            if (!RwRasterLockPalette(raster, rwRASTERLOCKWRITE | rwRASTERLOCKNOFETCH))
            {
                return FALSE;
            }
        }
    }

    switch (raster->depth)
    {
    case 4:
    {
        _rwDlRasterPalletized4SetFromImage(raster, image);
        break;
    }
    case 8:
    {
        _rwDlRasterPalletized8SetFromImage(raster, image);
        break;
    }
    case 16:
    {
        _rwDlRaster16SetFromImage(raster, image);
        break;
    }
    case 24:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
    case 32:
    {
        _rwDlRaster32SetFromImage(raster, image);
        break;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        break;
    }
    }

    if ((format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8)) && !paletteLocked)
    {
        RwRasterUnlockPalette(raster);
    }

    if (!rasterLocked)
    {
        RwRasterUnlock(raster);
    }

    return TRUE;
}

static RwUInt32 _rwDlImageFindFormat(RwImage* image)
{
    RwInt32 depth = image->depth;
    RwUInt32 format;
    RwBool mask = FALSE;

    if ((depth == 4) || (depth == 8))
    {
        RwInt32 width = image->width;
        RwInt32 height = image->height;
        RwInt32 y;
        RwUInt8* cpIn = image->cpPixels;
        RwRGBA* rpPal = image->palette;

        for (y = 0; y < height; y++)
        {
            RwInt32 x;
            RwUInt8* cpInCur = cpIn;

            for (x = 0; x < width; x++)
            {
                if (rpPal[*cpInCur].alpha != 0xFF)
                {
                    mask = TRUE;

                    if (rpPal[*cpInCur].alpha > 0x0F)
                    {
                        format = rwRASTERFORMAT4444;
                        if (depth == 4)
                        {
                            format |= rwRASTERFORMATPAL4;
                        }
                        else
                        {
                            format |= rwRASTERFORMATPAL8;
                        }
                        return format;
                    }
                }

                cpInCur++;
            }

            cpIn += image->stride;
        }
    }
    else
    {
        RwInt32 width = image->width;
        RwInt32 height = image->height;
        RwInt32 y;
        RwUInt8* cpIn = image->cpPixels;

        for (y = 0; y < height; y++)
        {
            RwInt32 x;
            RwRGBA* rpInCur = (RwRGBA*)cpIn;

            for (x = 0; x < width; x++)
            {
                if (rpInCur->alpha != 0xFF)
                {
                    mask = TRUE;

                    if (rpInCur->alpha > 0x0F)
                    {
                        return rwRASTERFORMAT4444;
                    }
                }

                rpInCur++;
            }

            cpIn += image->stride;
        }
    }

    format = mask ? rwRASTERFORMAT1555 : rwRASTERFORMAT565;

    if (depth == 4)
    {
        format |= rwRASTERFORMATPAL4;
    }
    else if (depth == 8)
    {
        format |= rwRASTERFORMATPAL8;
    }

    return format;
}

RwBool _rwDlImageFindRasterFormat(void* rasterIn, void* imageIn, RwInt32 flags)
{
    RwRaster* raster = (RwRaster*)rasterIn;
    RwImage* image = (RwImage*)imageIn;
    RwInt32 format;

    raster->cType = (RwUInt8)(flags & rwRASTERTYPEMASK);
    raster->depth = 0;

    switch (flags & rwRASTERTYPEMASK)
    {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
    {
        raster->width = (image->width > 1024) ? 1024 : image->width;
        raster->height = (image->height > 1024) ? 1024 : image->height;

        if (flags & rwRASTERFORMATMIPMAP)
        {
            raster->width = 1 << _rwDlFindMSB(raster->width);
            raster->height = 1 << _rwDlFindMSB(raster->height);
        }

        format = _rwDlImageFindFormat(image);
        raster->cFormat =
            (RwUInt8)((format | (flags & (rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP))) >> 8);

        return TRUE;
    }
    case rwRASTERTYPEZBUFFER:
    case rwRASTERTYPECAMERA:
    {
        raster->cFormat = 0;
        raster->width = image->width;
        raster->height = image->height;

        return TRUE;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERFORMAT));
        return FALSE;
    }
    }
}
