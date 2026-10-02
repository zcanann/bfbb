#include <rwsdk/rwcore.h>
#include <string.h>
#include <dolphin/os.h>

#include "rwsdk/driver/gcn/dlprivate.h"

RwInt32 _RwGameCubeRasterExtOffset;

void _rwDlRasterPluginAttach(void)
{
    _RwGameCubeRasterExtOffset = RwRasterRegisterPlugin(sizeof(RwGameCubeRasterExtension),
                                                        rwID_DLDRIVERPLUGIN, NULL, NULL, NULL);
}

static RwUInt32 DlRasterGetMipLevelSize(RwRaster* raster, RwUInt8 mipLevel)
{
    RwUInt32 width;
    RwUInt32 height;
    RwUInt32 size;
    RwRaster* parent = raster->parent;

    if (raster->privateFlags & rwRASTERPIXELLOCKED)
    {
        width = parent->width;
        height = parent->height;
    }
    else
    {
        width = parent->width >> mipLevel;
        height = parent->height >> mipLevel;

        width = width ? width : 1;
        height = height ? height : 1;
    }

    switch (raster->depth)
    {
    case 4:
    {
        /* 8x8 texel tiles */
        width = (width + 7) & ~7;
        height = (height + 7) & ~7;
        size = (width * height) / 2;
        break;
    }
    case 8:
    {
        /* 8x4 texel tiles */
        width = (width + 7) & ~7;
        height = (height + 3) & ~3;
        size = width * height;
        break;
    }
    case 16:
    {
        /* 4x4 texel tiles */
        width = (width + 3) & ~3;
        height = (height + 3) & ~3;
        size = width * height * 2;
        break;
    }
    case 32:
    {
        /* 4x4 texel tiles */
        width = (width + 3) & ~3;
        height = (height + 3) & ~3;
        size = width * height * 4;
        break;
    }
    case 24:
    default:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        return 0;
    }
    }

    return (size + 31) & ~31;
}

static RwUInt32 DlRasterGetMipLevelOffset(RwRaster* raster, RwUInt8 level)
{
    RwUInt32 offset = 0;

    while (level--)
    {
        offset += DlRasterGetMipLevelSize(raster, level);
    }

    return offset;
}

RwBool _rwDlRasterGetNumMipLevels(void* mipLevels, void* rasterIn, RwInt32 flags);

RwUInt32 _rwDlRasterGetSize(RwRaster* raster)
{
    RwUInt32 size = 0;
    RwInt32 numMipLevels;

    _rwDlRasterGetNumMipLevels(&numMipLevels, raster, 0);

    while (numMipLevels--)
    {
        size += DlRasterGetMipLevelSize(raster, (RwUInt8)numMipLevels);
    }

    return size;
}

RwUInt32 _rwDlRasterGetStride(RwRaster* raster, RwUInt8 level)
{
    RwUInt32 stride;
    RwInt32 width = raster->parent->width >> level;

    width = width ? width : 1;

    switch (raster->depth)
    {
    case 4:
    {
        stride = ((width + 7) & ~7) >> 1;
        break;
    }
    case 8:
    {
        stride = (width + 7) & ~7;
        break;
    }
    case 16:
    {
        stride = ((width + 3) & ~3) << 1;
        break;
    }
    case 32:
    {
        stride = ((width + 3) & ~3) << 2;
        break;
    }
    case 24:
    default:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        return 0;
    }
    }

    return stride;
}

RwBool _rwDlRasterGetNumMipLevels(void* mipLevels, void* rasterIn, RwInt32 flags)
{
    RwRaster* raster = (RwRaster*)rasterIn;
    RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster->parent);

    if (rasExt->maxLOD != 0xFF)
    {
        *(RwInt32*)mipLevels = rasExt->maxLOD + 1;
    }
    else
    {
        RwUInt8 numLevels;
        if (RwRasterGetFormat(raster) & rwRASTERFORMATMIPMAP)
        {
            if (raster->width > raster->height)
            {
                numLevels = (RwUInt8)_rwDlFindMSB(raster->width) + 1;
            }
            else
            {
                numLevels = (RwUInt8)_rwDlFindMSB(raster->height) + 1;
            }
        }
        else
        {
            numLevels = 1;
        }
        *(RwInt32*)mipLevels = numLevels;
    }

    return TRUE;
}

static void DlRasterTile(void* dstBuffer, void* srcBuffer, RwInt32 width, RwInt32 height,
                         RwInt32 depth, RwInt32 stride)
{
    switch (depth)
    {
    case 4:
    {
        /* 8x8 texel tiles, 4 bits per texel */
        RwInt32 tilesX = ((width + 7) & ~7) >> 3;
        RwInt32 y;

        for (y = 0; y < height; y++)
        {
            RwUInt8* line = (RwUInt8*)srcBuffer + y * stride;
            RwInt32 tiles = tilesX * (y >> 3);
            RwInt32 tdy = (y & 7) << 3;
            RwInt32 x;

            for (x = 0; x < width; x += 8)
            {
                RwUInt32 numPixels = tdy + ((tiles + (x >> 3)) << 6);
                RwUInt32 byteOffset = numPixels >> 1;

                memcpy((RwUInt8*)dstBuffer + byteOffset, line + (x >> 1), 4);
            }
        }
        break;
    }
    case 8:
    {
        /* 8x4 texel tiles, 8 bits per texel */
        RwInt32 tilesX = ((width + 7) & ~7) >> 3;
        RwInt32 y;

        for (y = 0; y < height; y++)
        {
            RwUInt8* line = (RwUInt8*)srcBuffer + y * stride;
            RwInt32 tiles = tilesX * (y >> 2);
            RwInt32 tdy = (y & 3) << 3;
            RwInt32 x;

            for (x = 0; x < width; x += 8)
            {
                RwUInt32 numPixels = tdy + ((tiles + (x >> 3)) << 5);
                RwUInt32 byteOffset = numPixels;

                memcpy((RwUInt8*)dstBuffer + byteOffset, line + x, 8);
            }
        }
        break;
    }
    case 16:
    {
        /* 4x4 texel tiles, 16 bits per texel */
        RwInt32 tilesX = ((width + 3) & ~3) >> 2;
        RwInt32 y;

        for (y = 0; y < height; y++)
        {
            RwUInt16* line = (RwUInt16*)((RwUInt8*)srcBuffer + y * stride);
            RwInt32 tiles = tilesX * (y >> 2);
            RwInt32 tdy = (y & 3) << 2;
            RwInt32 x;

            for (x = 0; x < width; x += 4)
            {
                RwUInt32 numPixels = tdy + ((tiles + (x >> 2)) << 4);
                RwUInt32 byteOffset = numPixels << 1;

                memcpy((RwUInt8*)dstBuffer + byteOffset, line + x, 8);
            }
        }
        break;
    }
    case 32:
    {
        /* 4x4 texel tiles, stored as an AR block followed by a GB block */
        RwInt32 tilesX = ((width + 3) & ~3) >> 2;
        RwInt32 y;

        for (y = 0; y < height; y++)
        {
            RwUInt8* line = (RwUInt8*)srcBuffer + y * stride;
            RwInt32 tiles = tilesX * (y >> 2);
            RwInt32 tdy = (y & 3) << 2;
            RwInt32 x;
            RwUInt32 tb = 0;

            for (x = 0; x < width; x++)
            {
                RwUInt32 index;

                if (!(x & 3))
                {
                    tb = (tiles + (x >> 2)) << 4;
                }

                index = (tb << 2) + ((tdy + (x & 3)) << 1);

                *(RwUInt16*)((RwUInt8*)dstBuffer + index) = ((RwUInt16*)line)[x << 1];
                *(RwUInt16*)((RwUInt8*)dstBuffer + (index + 32)) = ((RwUInt16*)line)[(x << 1) + 1];
            }
        }
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

static void DlRasterUntile(void* dstBuffer, void* srcBuffer, RwInt32 width, RwInt32 height,
                         RwInt32 depth, RwInt32 stride)
{
    switch (depth)
    {
    case 4:
    {
        /* 8x8 texel tiles, 4 bits per texel */
        RwInt32 tilesX = ((width + 7) & ~7) >> 3;
        RwInt32 y;

        for (y = 0; y < height; y++)
        {
            RwUInt8* line = (RwUInt8*)dstBuffer + y * stride;
            RwInt32 tiles = tilesX * (y >> 3);
            RwInt32 tdy = (y & 7) << 3;
            RwInt32 x;

            for (x = 0; x < width; x += 8)
            {
                RwUInt32 numPixels = tdy + ((tiles + (x >> 3)) << 6);
                RwUInt32 byteOffset = numPixels >> 1;

                memcpy(line + (x >> 1), (RwUInt8*)srcBuffer + byteOffset, 4);
            }
        }
        break;
    }
    case 8:
    {
        /* 8x4 texel tiles, 8 bits per texel */
        RwInt32 tilesX = ((width + 7) & ~7) >> 3;
        RwInt32 y;

        for (y = 0; y < height; y++)
        {
            RwUInt8* line = (RwUInt8*)dstBuffer + y * stride;
            RwInt32 tiles = tilesX * (y >> 2);
            RwInt32 tdy = (y & 3) << 3;
            RwInt32 x;

            for (x = 0; x < width; x += 8)
            {
                RwUInt32 numPixels = tdy + ((tiles + (x >> 3)) << 5);
                RwUInt32 byteOffset = numPixels;

                memcpy(line + x, (RwUInt8*)srcBuffer + byteOffset, 8);
            }
        }
        break;
    }
    case 16:
    {
        /* 4x4 texel tiles, 16 bits per texel */
        RwInt32 tilesX = ((width + 3) & ~3) >> 2;
        RwInt32 y;

        for (y = 0; y < height; y++)
        {
            RwUInt16* line = (RwUInt16*)((RwUInt8*)dstBuffer + y * stride);
            RwInt32 tiles = tilesX * (y >> 2);
            RwInt32 tdy = (y & 3) << 2;
            RwInt32 x;

            for (x = 0; x < width; x += 4)
            {
                RwUInt32 numPixels = tdy + ((tiles + (x >> 2)) << 4);
                RwUInt32 byteOffset = numPixels << 1;

                memcpy(line + x, (RwUInt8*)srcBuffer + byteOffset, 8);
            }
        }
        break;
    }
    case 32:
    {
        /* 4x4 texel tiles, stored as an AR block followed by a GB block */
        RwInt32 tilesX = ((width + 3) & ~3) >> 2;
        RwInt32 y;

        for (y = 0; y < height; y++)
        {
            RwUInt8* line = (RwUInt8*)dstBuffer + y * stride;
            RwInt32 tiles = tilesX * (y >> 2);
            RwInt32 tdy = (y & 3) << 2;
            RwInt32 x;
            RwUInt32 tb = 0;

            for (x = 0; x < width; x++)
            {
                RwUInt32 index;

                if (!(x & 3))
                {
                    tb = (tiles + (x >> 2)) << 4;
                }

                index = tdy + (x & 3);

                ((RwUInt16*)line)[x << 1] = *(RwUInt16*)((RwUInt8*)srcBuffer + ((tb << 2) + (index << 1)));
                ((RwUInt16*)line)[(x << 1) + 1] = *(RwUInt16*)((RwUInt8*)srcBuffer + ((tb << 2) + (index << 1) + 32));
            }
        }
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

RwBool _rwDlRasterLock(void* pixelsIn, void* rasterIn, RwInt32 accessMode)
{
    RwUInt8** pixels;
    RwRaster* raster = (RwRaster*)rasterIn;
    RwRaster* parentRaster;
    RwGameCubeRasterExtension* rasExt;
    RwUInt8* lockedPixels;
    RwInt32 pitch;
    RwUInt8 mipLevel;

    mipLevel = (RwUInt8)((accessMode & 0xFF00) >> 8);
    pixels = (RwUInt8**)pixelsIn;
    parentRaster = raster->parent;
    rasExt = RASTEREXTFROMRASTER(parentRaster);

    switch (RwRasterGetType(raster))
    {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
    {
        lockedPixels = rasExt->pixels + DlRasterGetMipLevelOffset(raster, mipLevel);

        pitch = _rwDlRasterGetStride(raster, mipLevel);

        if (!(accessMode & rwRASTERLOCKRAW))
        {
            RwInt32 height = parentRaster->height >> mipLevel;

            height = height ? height : 1;

            rasExt->lockedBuffer = (RwUInt8*)RwMalloc(pitch * height);
            if (!rasExt->lockedBuffer)
            {
                return FALSE;
            }
        }

        if (parentRaster == raster)
        {
            RwInt32 width = raster->width >> mipLevel;
            RwInt32 height = raster->height >> mipLevel;

            width = width ? width : 1;
            height = height ? height : 1;

            raster->originalWidth = raster->width;
            raster->originalHeight = raster->height;
            raster->width = width;
            raster->height = height;

            if (!(accessMode & rwRASTERLOCKRAW))
            {
                raster->cpPixels = rasExt->lockedBuffer;
            }
            else
            {
                raster->cpPixels = lockedPixels;
            }
        }
        else if (!(accessMode & rwRASTERLOCKRAW))
        {
            switch (raster->depth)
            {
            case 4:
            {
                raster->cpPixels =
                    rasExt->lockedBuffer + pitch * raster->nOffsetY + (raster->nOffsetX >> 1);
                break;
            }
            case 8:
            {
                raster->cpPixels = rasExt->lockedBuffer + pitch * raster->nOffsetY + raster->nOffsetX;
                break;
            }
            case 16:
            {
                raster->cpPixels =
                    rasExt->lockedBuffer + pitch * raster->nOffsetY + (raster->nOffsetX << 1);
                break;
            }
            case 32:
            {
                raster->cpPixels =
                    rasExt->lockedBuffer + pitch * raster->nOffsetY + (raster->nOffsetX << 2);
                break;
            }
            case 24:
            default:
            {
                RWERROR((E_RW_INVRASTERDEPTH));
                return FALSE;
            }
            }
        }
        else
        {
            RWERROR((E_RW_INVRASTERLOCKREQ));
            return FALSE;
        }

        rasExt->lockedPixels = lockedPixels;
        raster->stride = pitch;
        rasExt->lockedMipLevel = mipLevel;

        if (accessMode & rwRASTERLOCKREAD)
        {
            if (RwRasterGetType(raster) == rwRASTERTYPECAMERATEXTURE)
            {
                DCFlushRange(rasExt->lockedPixels, DlRasterGetMipLevelSize(raster, mipLevel));
            }

            raster->privateFlags |= rwRASTERPIXELLOCKEDREAD;
            parentRaster->privateFlags |= rwRASTERPIXELLOCKEDREAD;

            if (!(accessMode & rwRASTERLOCKRAW))
            {
                DlRasterUntile(rasExt->lockedBuffer, rasExt->lockedPixels, parentRaster->width,
                               parentRaster->height, raster->depth, raster->stride);
            }
        }

        if (accessMode & rwRASTERLOCKWRITE)
        {
            raster->privateFlags |= rwRASTERPIXELLOCKEDWRITE;
            parentRaster->privateFlags |= rwRASTERPIXELLOCKEDWRITE;

            /* Make sure the GPU has finished with the texture before it is overwritten */
            if (rasExt->token == _RwDlTokenCurrent)
            {
                GXSetDrawSync(_RwDlTokenCurrent);
                _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;
            }

            while (!_rwDlTokenQueryDone(rasExt->token))
            {
            }
        }

        if (accessMode & rwRASTERLOCKRAW)
        {
            raster->privateFlags |= rwRASTERPIXELLOCKEDRAW;
            parentRaster->privateFlags |= rwRASTERPIXELLOCKEDRAW;
        }
        break;
    }
    case rwRASTERTYPEZBUFFER:
    case rwRASTERTYPECAMERA:
    default:
    {
        RWERROR((E_RW_INVRASTERLOCKREQ));
        return FALSE;
    }
    }

    *pixels = raster->cpPixels;

    return TRUE;
}

RwBool _rwDlRasterUnlock(void* unused1, void* rasterIn, RwInt32 unused3)
{
    RwRaster* raster = (RwRaster*)rasterIn;
    RwRaster* parentRaster = raster->parent;
    RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(parentRaster);

    switch (RwRasterGetType(raster))
    {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
    {
        if (raster->privateFlags & rwRASTERPIXELLOCKEDWRITE)
        {
            if (!(raster->privateFlags & rwRASTERPIXELLOCKEDRAW))
            {
                DlRasterTile(rasExt->lockedPixels, rasExt->lockedBuffer, parentRaster->width,
                             parentRaster->height, raster->depth, raster->stride);
            }

            DCFlushRange(rasExt->lockedPixels,
                         DlRasterGetMipLevelSize(raster, rasExt->lockedMipLevel));
            GXInvalidateTexAll();
        }

        if (parentRaster == raster)
        {
            raster->width = raster->originalWidth;
            raster->height = raster->originalHeight;
        }

        if (!(raster->privateFlags & rwRASTERPIXELLOCKEDRAW))
        {
            RwFree(rasExt->lockedBuffer);
            rasExt->lockedBuffer = NULL;
        }

        raster->stride = 0;
        raster->cpPixels = NULL;

        if ((raster->privateFlags & rwRASTERPIXELLOCKEDWRITE) &&
            (raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8)) && (rasExt->lockedMipLevel == 0))
        {
            rasExt->lockedMipLevel = 0xFF;
            raster->privateFlags &= ~(rwRASTERPIXELLOCKED | rwRASTERPIXELLOCKEDRAW);
            parentRaster->privateFlags &= ~(rwRASTERPIXELLOCKED | rwRASTERPIXELLOCKEDRAW);

            RwTextureRasterGenerateMipmaps(raster, NULL);
        }
        else
        {
            rasExt->lockedMipLevel = 0xFF;
            raster->privateFlags &= ~(rwRASTERPIXELLOCKED | rwRASTERPIXELLOCKEDRAW);
            parentRaster->privateFlags &= ~(rwRASTERPIXELLOCKED | rwRASTERPIXELLOCKEDRAW);
        }
        break;
    }
    case rwRASTERTYPEZBUFFER:
    case rwRASTERTYPECAMERA:
    default:
    {
        RWERROR((E_RW_INVRASTERUNLOCKREQ));
        return FALSE;
    }
    }

    return TRUE;
}

RwBool _rwDlRasterLockPalette(void* paletteIn, void* rasterIn, RwInt32 accessMode)
{
    RwRaster* raster = (RwRaster*)rasterIn;

    switch (RwRasterGetType(raster))
    {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    {
        if ((raster == raster->parent) && (raster->palette == NULL))
        {
            RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster);

            if (accessMode & rwRASTERLOCKREAD)
            {
                raster->privateFlags |= rwRASTERPALETTELOCKEDREAD;
            }

            if (accessMode & rwRASTERLOCKWRITE)
            {
                raster->privateFlags |= rwRASTERPALETTELOCKEDWRITE;
            }

            raster->palette = rasExt->palette;
            *(RwUInt8**)paletteIn = raster->palette;
        }

        return TRUE;
    }
    case rwRASTERTYPEZBUFFER:
    case rwRASTERTYPECAMERA:
    case rwRASTERTYPECAMERATEXTURE:
    default:
    {
        RWERROR((E_RW_INVRASTERLOCKREQ));
        break;
    }
    }

    return FALSE;
}

RwBool _rwDlRasterUnlockPalette(void* unused1, void* rasterIn, RwInt32 unused3)
{
    RwRaster* raster = (RwRaster*)rasterIn;

    switch (RwRasterGetType(raster))
    {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    {
        if (raster == raster->parent)
        {
            RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster);

            if (raster->privateFlags & rwRASTERPALETTELOCKEDWRITE)
            {
                DCFlushRange(rasExt->palette, (1 << raster->depth) * sizeof(RwUInt16));
            }

            raster->privateFlags &= ~rwRASTERPALETTELOCKED;
            raster->palette = NULL;
        }

        return TRUE;
    }
    case rwRASTERTYPEZBUFFER:
    case rwRASTERTYPECAMERA:
    case rwRASTERTYPECAMERATEXTURE:
    default:
    {
        RWERROR((E_RW_INVRASTERUNLOCKREQ));
        break;
    }
    }

    return FALSE;
}

static RwBool DlGetRasterFormat(RwRaster* raster, RwInt32 flags)
{
    RwUInt32 format;
    RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster->parent);

    raster->cType = (RwUInt8)(flags & rwRASTERTYPEMASK);
    raster->cFlags = (RwUInt8)(flags & 0xF8);

    format = flags & rwRASTERFORMATMASK;

    switch (raster->cType)
    {
    case rwRASTERTYPECAMERATEXTURE:
    {
        if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
        {
            RWERROR((E_RW_DEVICEERROR, "rwRASTERTYPECAMERATEXTURE can not be palletized"));
            return FALSE;
        }
    }
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    {
        if (!(format & rwRASTERFORMATPIXELFORMATMASK))
        {
            /* Pick a default pixel format for the requested depth */
            switch (raster->depth)
            {
            case 0:
            {
                if (raster->cType == rwRASTERTYPECAMERATEXTURE)
                {
                    rasExt->format = GX_TF_RGB565;
                    rasExt->flags = 0;
                    raster->depth = 16;
                    format |= rwRASTERFORMAT565;
                }
                else
                {
                    rasExt->format = GX_TF_RGB5A3;
                    format |= rwRASTERFORMAT1555;
                    rasExt->flags = 1;

                    if (format & rwRASTERFORMATPAL4)
                    {
                        raster->depth = 4;
                    }
                    else if (format & rwRASTERFORMATPAL8)
                    {
                        raster->depth = 8;
                    }
                    else
                    {
                        raster->depth = 16;
                    }
                }
                break;
            }
            case 4:
            {
                if (format & rwRASTERFORMATPAL4)
                {
                    rasExt->format = GX_TF_C4;
                    rasExt->tlutFmt = GX_TL_RGB5A3;
                    format |= rwRASTERFORMAT1555;
                    rasExt->flags = 1;
                }
                else
                {
                    rasExt->format = GX_TF_I4;
                    rasExt->flags = 0;
                }
                break;
            }
            case 8:
            {
                if (format & rwRASTERFORMATPAL8)
                {
                    rasExt->format = GX_TF_C8;
                    rasExt->tlutFmt = GX_TL_RGB5A3;
                    format |= rwRASTERFORMAT1555;
                    rasExt->flags = 1;
                }
                else
                {
                    rasExt->format = GX_TF_I8;
                    rasExt->flags = 0;
                }
                break;
            }
            case 16:
            {
                rasExt->format = GX_TF_RGB5A3;
                format |= rwRASTERFORMAT1555;
                rasExt->flags = 1;
                break;
            }
            case 32:
            {
                rasExt->format = GX_TF_RGBA8;
                format |= rwRASTERFORMAT8888;
                rasExt->flags = 1;
                break;
            }
            default:
            {
                RWERROR((E_RW_INVRASTERDEPTH));
                return FALSE;
            }
            }
        }
        else
        {
            switch (format & rwRASTERFORMATPIXELFORMATMASK)
            {
            case rwRASTERFORMAT555:
            {
                if (format & rwRASTERFORMATPAL4)
                {
                    rasExt->format = GX_TF_C4;
                    rasExt->tlutFmt = GX_TL_RGB5A3;
                    raster->depth = 4;
                }
                else if (format & rwRASTERFORMATPAL8)
                {
                    rasExt->format = GX_TF_C8;
                    rasExt->tlutFmt = GX_TL_RGB5A3;
                    raster->depth = 8;
                }
                else
                {
                    rasExt->format = GX_TF_RGB5A3;
                    raster->depth = 16;
                }
                rasExt->flags = 0;
                break;
            }
            case rwRASTERFORMAT565:
            {
                if (format & rwRASTERFORMATPAL4)
                {
                    rasExt->format = GX_TF_C4;
                    rasExt->tlutFmt = GX_TL_RGB565;
                    raster->depth = 4;
                }
                else if (format & rwRASTERFORMATPAL8)
                {
                    rasExt->format = GX_TF_C8;
                    rasExt->tlutFmt = GX_TL_RGB565;
                    raster->depth = 8;
                }
                else
                {
                    rasExt->format = GX_TF_RGB565;
                    raster->depth = 16;
                }
                rasExt->flags = 0;
                break;
            }
            case rwRASTERFORMAT1555:
            case rwRASTERFORMAT4444:
            {
                if (format & rwRASTERFORMATPAL4)
                {
                    rasExt->format = GX_TF_C4;
                    rasExt->tlutFmt = GX_TL_RGB5A3;
                    raster->depth = 4;
                }
                else if (format & rwRASTERFORMATPAL8)
                {
                    rasExt->format = GX_TF_C8;
                    rasExt->tlutFmt = GX_TL_RGB5A3;
                    raster->depth = 8;
                }
                else
                {
                    rasExt->format = GX_TF_RGB5A3;
                    raster->depth = 16;
                }
                rasExt->flags = 1;
                break;
            }
            case rwRASTERFORMAT888:
            {
                if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
                {
                    RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMAT888 invalid format"));
                    return FALSE;
                }
                rasExt->format = GX_TF_RGBA8;
                rasExt->flags = 0;
                raster->depth = 32;
                break;
            }
            case rwRASTERFORMAT8888:
            {
                if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
                {
                    RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMAT8888 invalid format"));
                    return FALSE;
                }
                rasExt->format = GX_TF_RGBA8;
                rasExt->flags = 1;
                raster->depth = 32;
                break;
            }
            case rwRASTERFORMATLUM8:
            {
                RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMATLUM8 invalid format"));
                return FALSE;
            }
            case rwRASTERFORMAT16:
            {
                RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMAT16 invalid format"));
                return FALSE;
            }
            case rwRASTERFORMAT24:
            {
                RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMAT24 invalid format"));
                return FALSE;
            }
            case rwRASTERFORMAT32:
            {
                RWERROR((E_RW_DEVICEERROR, "rwRASTERFORMAT32 invalid format"));
                return FALSE;
            }
            default:
            {
                RWERROR((E_RW_INVRASTERFORMAT));
                return FALSE;
            }
            }
        }
        break;
    }
    case rwRASTERTYPEZBUFFER:
    case rwRASTERTYPECAMERA:
    {
        if (!(format & rwRASTERFORMATPIXELFORMATMASK))
        {
            if (_RwDlRenderMode->aa)
            {
                rasExt->format = GX_TF_RGB565;
                rasExt->flags = 0;
                raster->depth = 16;
                format |= rwRASTERFORMAT565;
            }
            else
            {
                rasExt->format = GX_TF_RGBA8;
                rasExt->flags = 0;
                raster->depth = 32;
                format |= rwRASTERFORMAT888;
            }
        }
        else if (!((format == rwRASTERFORMAT16) && _RwDlRenderMode->aa) ||
                 ((format == rwRASTERFORMAT24) && !_RwDlRenderMode->aa))
        {
            RWERROR((E_RW_INVRASTERFORMAT));
            return FALSE;
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERFORMAT));
        return FALSE;
    }
    }

    raster->cFormat = (RwUInt8)(format >> 8);

    return TRUE;
}

RwBool _rwDlTextureRasterCreate(RwRaster* raster, RwUInt8 numLods)
{
    RwUInt32 rasterSize;
    RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster);

    rasExt->maxLOD = numLods - 1;

    rasterSize = _rwDlRasterGetSize(raster);

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        RwUInt32 paletteSize = (1 << raster->depth) * sizeof(RwUInt16);
        RwUInt32 tmpRasterSize = rasterSize + paletteSize + 31;

        rasExt->memory = (RwUInt8*)RwMalloc(tmpRasterSize);
        if (!rasExt->memory)
        {
            RWERROR((E_RW_NOMEM, tmpRasterSize));
            return FALSE;
        }

        /* Texture data must be 32 byte aligned */
        rasExt->pixels = (RwUInt8*)(((RwUInt32)rasExt->memory + 31) & ~31);
        rasExt->palette = rasExt->pixels + rasterSize;

        GXInitTlutObj(&rasExt->tlutObj, rasExt->palette, (GXTlutFmt)rasExt->tlutFmt,
                      (u16)(1 << raster->depth));
    }
    else
    {
        rasExt->memory = (RwUInt8*)RwMalloc(rasterSize + 31);
        if (!rasExt->memory)
        {
            RWERROR((E_RW_NOMEM, rasterSize + 31));
            return FALSE;
        }

        /* Texture data must be 32 byte aligned */
        rasExt->pixels = (RwUInt8*)(((RwUInt32)rasExt->memory + 31) & ~31);

        if (raster->cType == rwRASTERTYPECAMERATEXTURE)
        {
            DCInvalidateRange(rasExt->pixels, rasterSize);
        }
    }

    return TRUE;
}

RwBool _rwDlRasterCreate(void* unused1, void* rasterIn, RwInt32 flags)
{
    RwRaster* raster = (RwRaster*)rasterIn;
    RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster);

    raster->stride = 0;

    rasExt->format = 0xFF;
    rasExt->tlutFmt = 0xFF;
    rasExt->flags = 0;
    rasExt->memory = NULL;
    rasExt->pixels = NULL;
    rasExt->palette = NULL;
    rasExt->lockedPixels = NULL;
    rasExt->lockedBuffer = NULL;
    rasExt->region = NULL;
    rasExt->token = _RwDlTokenLastSeen;
    rasExt->maxLOD = 0xFF;
    rasExt->lockedMipLevel = 0xFF;

    if (!DlGetRasterFormat(raster, flags))
    {
        return FALSE;
    }

    if (raster->width && raster->height)
    {
        switch (raster->cType)
        {
        case rwRASTERTYPENORMAL:
        case rwRASTERTYPETEXTURE:
        case rwRASTERTYPECAMERATEXTURE:
        {
            if (!(raster->cFlags & rwRASTERDONTALLOCATE))
            {
                RwUInt8 numLods;

                if (RwRasterGetFormat(raster) & rwRASTERFORMATMIPMAP)
                {
                    if (raster->width > raster->height)
                    {
                        numLods = (RwUInt8)_rwDlFindMSB(raster->width) + 1;
                    }
                    else
                    {
                        numLods = (RwUInt8)_rwDlFindMSB(raster->height) + 1;
                    }
                }
                else
                {
                    numLods = 1;
                }

                if (!_rwDlTextureRasterCreate(raster, numLods))
                {
                    RWERROR((E_RW_DEVICEERROR, "Failed to create surface for texture"));
                    return FALSE;
                }
            }
            break;
        }
        case rwRASTERTYPEZBUFFER:
        case rwRASTERTYPECAMERA:
        {
            raster->cFlags = rwRASTERDONTALLOCATE;
            break;
        }
        default:
        {
            RWERROR((E_RW_INVRASTERFORMAT));
            return FALSE;
        }
        }
    }
    else
    {
        raster->cFlags = rwRASTERDONTALLOCATE;
        return TRUE;
    }

    return TRUE;
}

RwBool _rwDlRasterDestroy(void* unused1, void* rasterIn, RwInt32 unused3)
{
    RwRaster* raster = (RwRaster*)rasterIn;

    if ((raster->parent == raster) && !(raster->cFlags & rwRASTERDONTALLOCATE))
    {
        switch (raster->cType)
        {
        case rwRASTERTYPENORMAL:
        case rwRASTERTYPETEXTURE:
        case rwRASTERTYPECAMERATEXTURE:
        {
            RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster);

            /* Wait for the GPU to finish with the texture */
            if (rasExt->token == _RwDlTokenCurrent)
            {
                GXSetDrawSync(_RwDlTokenCurrent);
                _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;
            }

            while (!_rwDlTokenQueryDone(rasExt->token))
            {
            }

            if (_RwDlTexture && (raster == _RwDlTexture->raster))
            {
                _rwDlTextureSetRaster(_RwDlTexture, NULL, 0);
            }

            RwFree(rasExt->memory);
            break;
        }
        case rwRASTERTYPEZBUFFER:
        case rwRASTERTYPECAMERA:
        {
            break;
        }
        default:
        {
            RWERROR((E_RW_INVRASTERFORMAT));
            return FALSE;
        }
        }
    }

    return TRUE;
}

RwBool _rwDlTextureSetRaster(void* textureIn, void* rasterIn, RwInt32 flags)
{
    RwTexture* texture = (RwTexture*)textureIn;

    texture->raster = (RwRaster*)rasterIn;
    TEXTUREEXTFROMTEXTURE(texture)->flags = rwDLTEXTUREEXTDEFAULTLOD;

    return TRUE;
}

RwBool _rwDlRasterSubRaster(void* raster, void* pIn, RwInt32 flags)
{
    RwRaster* ras = (RwRaster*)raster;
    RwRaster* rpIn = (RwRaster*)pIn;

    ras->stride = rpIn->stride;
    ras->depth = rpIn->depth;
    ras->cType = rpIn->cType;
    ras->cFormat = rpIn->cFormat;
    ras->cpPixels = NULL;

    return TRUE;
}
