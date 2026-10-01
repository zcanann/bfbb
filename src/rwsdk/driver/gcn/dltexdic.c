#include <rwsdk/rwcore.h>
#include <dolphin/os.h>
#include <string.h>

#include "rwsdk/driver/gcn/dlprivate.h"

typedef struct _rwDlNativeTexture _rwDlNativeTexture;
struct _rwDlNativeTexture
{
    RwInt32 id; /* 0x00 */
    RwInt32 filterAndAddress; /* 0x04 */
    RwUInt32 maxAniso; /* 0x08 */
    RwBool biasClamp; /* 0x0C */
    RwBool edgeLod; /* 0x10 */
    RwReal lodBias; /* 0x14 */
    RwChar name[32]; /* 0x18 */
    RwChar mask[32]; /* 0x38 */
};

typedef struct _rwDlNativeRaster _rwDlNativeRaster;
struct _rwDlNativeRaster
{
    RwInt32 formatType; /* 0x00 */
    RwUInt16 width; /* 0x04 */
    RwUInt16 height; /* 0x06 */
    RwUInt8 depth; /* 0x08 */
    RwUInt8 numMipLevels; /* 0x09 */
    RwUInt8 format; /* 0x0A */
    RwUInt8 tlutFormat; /* 0x0B */
    RwBool alpha; /* 0x0C */
};

RwBool _rwDlNativeTextureGetSize(void* sizeIn, void* textureIn, RwInt32 flags)
{
    RwUInt32 size;
    RwRaster* raster = ((RwTexture*)textureIn)->raster;

    if (!raster)
    {
        *(RwUInt32*)sizeIn = rwCHUNKHEADERSIZE + sizeof(_rwDlNativeTexture);
        return TRUE;
    }

    size = rwCHUNKHEADERSIZE + sizeof(_rwDlNativeTexture) + sizeof(_rwDlNativeRaster);

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        size += (1 << raster->depth) * sizeof(RwUInt16);
    }

    size += sizeof(RwUInt32);
    size += _rwDlRasterGetSize(raster);

    *(RwUInt32*)sizeIn = size;

    return TRUE;
}

RwBool _rwDlNativeTextureWrite(void* streamIn, void* textureIn, RwInt32 flags)
{
    _rwDlNativeTexture nativeTexture;
    _rwDlNativeRaster nativeRaster;
    RwUInt32 size;
    RwInt32 bytesLeftToWrite;
    RwRaster* raster;
    RwGameCubeRasterExtension* rasExt;
    _rwDlTextureExt* texExt;
    RwUInt32 paletteSize;

    _rwDlNativeTextureGetSize(&bytesLeftToWrite, textureIn, 0);
    bytesLeftToWrite -= rwCHUNKHEADERSIZE;

    if (!RwStreamWriteChunkHeader((RwStream*)streamIn, rwID_STRUCT, bytesLeftToWrite))
    {
        return FALSE;
    }

    nativeTexture.id = rwID_GAMECUBE;
    nativeTexture.filterAndAddress = (RwTextureGetAddressingU((RwTexture*)textureIn) << 8) |
                                     RwTextureGetFilterMode((RwTexture*)textureIn) |
                                     (RwTextureGetAddressingV((RwTexture*)textureIn) << 12);

    texExt = TEXTUREEXTFROMTEXTURE(textureIn);

    if (texExt->flags & 0x01000000)
    {
        nativeTexture.maxAniso = GX_ANISO_1;
        nativeTexture.biasClamp = TRUE;
        nativeTexture.edgeLod = TRUE;
        nativeTexture.lodBias = 0.0f;
    }
    else
    {
        nativeTexture.maxAniso = GXGetTexObjMaxAniso(&texExt->texObj);
        nativeTexture.biasClamp = GXGetTexObjBiasClamp(&texExt->texObj);
        nativeTexture.edgeLod = GXGetTexObjEdgeLOD(&texExt->texObj);
        nativeTexture.lodBias = GXGetTexObjLODBias(&texExt->texObj);
    }

    memcpy(nativeTexture.name, ((RwTexture*)textureIn)->name, sizeof(nativeTexture.name));
    memcpy(nativeTexture.mask, ((RwTexture*)textureIn)->mask, sizeof(nativeTexture.mask));

    if (!RwStreamWrite((RwStream*)streamIn, &nativeTexture, sizeof(nativeTexture)))
    {
        return FALSE;
    }

    bytesLeftToWrite -= sizeof(nativeTexture);

    raster = ((RwTexture*)textureIn)->raster;
    rasExt = RASTEREXTFROMRASTER(raster->parent);

    nativeRaster.formatType = (raster->cFormat << 8) | raster->cType;
    nativeRaster.width = (RwUInt16)raster->width;
    nativeRaster.height = (RwUInt16)raster->height;
    nativeRaster.depth = (RwUInt8)raster->depth;
    nativeRaster.numMipLevels = (RwUInt8)RwRasterGetNumLevels(raster);
    nativeRaster.format = (RwUInt8)rasExt->format;
    nativeRaster.tlutFormat = (RwUInt8)rasExt->tlutFmt;
    nativeRaster.alpha = rasExt->flags & 1;

    if (!RwStreamWrite((RwStream*)streamIn, &nativeRaster, sizeof(nativeRaster)))
    {
        return FALSE;
    }

    bytesLeftToWrite -= sizeof(nativeRaster);

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        paletteSize = (1 << raster->depth) * sizeof(RwUInt16);

        if (!RwStreamWrite((RwStream*)streamIn, rasExt->palette, paletteSize))
        {
            return FALSE;
        }

        bytesLeftToWrite -= paletteSize;
    }

    size = _rwDlRasterGetSize(raster);

    if (!RwStreamWrite((RwStream*)streamIn, &size, sizeof(size)))
    {
        return FALSE;
    }

    bytesLeftToWrite -= sizeof(size);

    if (!RwStreamWrite((RwStream*)streamIn, rasExt->pixels, size))
    {
        return FALSE;
    }

    bytesLeftToWrite -= size;

    return TRUE;
}

RwBool _rwDlNativeTextureRead(void* streamIn, void* textureIn, RwInt32 flags)
{
    _rwDlNativeTexture nativeTexture;
    _rwDlNativeRaster nativeRaster;
    RwUInt32 length;
    RwUInt32 version;
    RwUInt32 autoMipmap;
    RwUInt32 size;
    RwTexture* texture;
    RwRaster* raster;
    RwGameCubeRasterExtension* rasExt;

    if (!RwStreamFindChunk((RwStream*)streamIn, rwID_STRUCT, &length, &version))
    {
        return FALSE;
    }

    if (!((version >= rwLIBRARYBASEVERSION) && (version <= rwLIBRARYCURRENTVERSION)))
    {
        return FALSE;
    }

    if (RwStreamRead((RwStream*)streamIn, &nativeTexture, sizeof(nativeTexture)) != sizeof(nativeTexture))
    {
        return FALSE;
    }

    if (nativeTexture.id != rwID_GAMECUBE)
    {
        return FALSE;
    }

    if (RwStreamRead((RwStream*)streamIn, &nativeRaster, sizeof(nativeRaster)) != sizeof(nativeRaster))
    {
        return FALSE;
    }

    raster = RwRasterCreate(nativeRaster.width, nativeRaster.height, nativeRaster.depth,
                            nativeRaster.formatType | rwRASTERDONTALLOCATE);
    if (!raster)
    {
        return FALSE;
    }

    rasExt = RASTEREXTFROMRASTER(raster);
    rasExt->format = nativeRaster.format;
    rasExt->tlutFmt = nativeRaster.tlutFormat;
    rasExt->flags = (nativeRaster.alpha != 0);

    if (!_rwDlTextureRasterCreate(raster, nativeRaster.numMipLevels))
    {
        RwRasterDestroy(raster);
        return FALSE;
    }

    raster->cFlags &= ~rwRASTERDONTALLOCATE;

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        RwUInt32 size = (1 << raster->depth) * sizeof(RwUInt16);

        if (RwStreamRead((RwStream*)streamIn, rasExt->palette, size) != size)
        {
            return FALSE;
        }

        DCFlushRange(rasExt->palette, (1 << raster->depth) * sizeof(RwUInt16));
    }

    autoMipmap = raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8);
    raster->cFormat &= ~autoMipmap;

    if (RwStreamRead((RwStream*)streamIn, &size, sizeof(size)) != sizeof(size))
    {
        return FALSE;
    }

    if (RwStreamRead((RwStream*)streamIn, rasExt->pixels, size) != size)
    {
        return FALSE;
    }

    DCFlushRange(rasExt->pixels, size);
    GXInvalidateTexAll();

    raster->cFormat |= autoMipmap;

    texture = RwTextureCreate(raster);
    if (!texture)
    {
        RwRasterDestroy(raster);
        return FALSE;
    }

    RwTextureSetFilterMode(texture, nativeTexture.filterAndAddress & rwTEXTUREFILTERMODEMASK);
    RwTextureSetAddressingU(texture,
                            (nativeTexture.filterAndAddress & rwTEXTUREADDRESSINGUMASK) >> 8);
    RwTextureSetAddressingV(texture,
                            (nativeTexture.filterAndAddress & rwTEXTUREADDRESSINGVMASK) >> 12);

    RwTextureSetName(texture, nativeTexture.name);
    RwTextureSetMaskName(texture, nativeTexture.mask);

    RwGameCubeTextureSetLOD(texture, nativeTexture.lodBias, nativeTexture.biasClamp,
                            nativeTexture.edgeLod, nativeTexture.maxAniso);

    *(RwTexture**)textureIn = texture;

    return TRUE;
}
