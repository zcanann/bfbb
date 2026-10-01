#include <rwsdk/rwcore.h>

#include "rwsdk/driver/gcn/dlprivate.h"

typedef struct __rwFilterParams __rwFilterParams;
struct __rwFilterParams
{
    GXTexFilter min;
    GXTexFilter mag;
};

static __rwFilterParams _RwDlFilterModeConvTable[7] = {
    { GX_NEAR, GX_NEAR }, /* rwFILTERNAFILTERMODE */
    { GX_NEAR, GX_NEAR }, /* rwFILTERNEAREST */
    { GX_LINEAR, GX_LINEAR }, /* rwFILTERLINEAR */
    { GX_NEAR_MIP_NEAR, GX_NEAR }, /* rwFILTERMIPNEAREST */
    { GX_LIN_MIP_NEAR, GX_LINEAR }, /* rwFILTERMIPLINEAR */
    { GX_NEAR_MIP_LIN, GX_NEAR }, /* rwFILTERLINEARMIPNEAREST */
    { GX_LIN_MIP_LIN, GX_LINEAR }, /* rwFILTERLINEARMIPLINEAR */
};

static GXTexWrapMode _RwDlAddressConvTable[5] = {
    GX_CLAMP, /* rwTEXTUREADDRESSNATEXTUREADDRESS */
    GX_REPEAT, /* rwTEXTUREADDRESSWRAP */
    GX_MIRROR, /* rwTEXTUREADDRESSMIRROR */
    GX_CLAMP, /* rwTEXTUREADDRESSCLAMP */
    GX_CLAMP, /* rwTEXTUREADDRESSBORDER */
};

static RwTexture* _RwDlTextureCache[8] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

RwInt32 _RwGameCubeTextureExtOffset;

static void* _rwDlTextureConst(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    TEXTUREEXTFROMTEXTURE(object)->flags = rwDLTEXTUREEXTDEFAULTLOD;

    return object;
}

static void* _rwDlTextureDest(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    RwInt32 i;

    for (i = 0; i < 8; i++)
    {
        if (object == _RwDlTextureCache[i])
        {
            _RwDlTextureCache[i] = NULL;
        }
    }

    return object;
}

void _rwDlTextureCacheInit(void)
{
    RwUInt32 numTextures = 8;

    while (numTextures--)
    {
        _RwDlTextureCache[numTextures] = NULL;
    }
}

void _rwDlTexturePluginAttach(void)
{
    _RwGameCubeTextureExtOffset =
        RwTextureRegisterPlugin(sizeof(_rwDlTextureExt), rwID_DLDRIVERPLUGIN, _rwDlTextureConst,
                                _rwDlTextureDest, NULL);
}

static void _rwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias, RwBool biasClamp,
                                     RwBool edgeLod, RwUInt32 maxAniso, RwUInt32 name)
{
    RwRaster* raster = texture->raster;
    _rwDlTextureExt* texExt = TEXTUREEXTFROMTEXTURE(texture);
    RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster->parent);
    RwInt32 format = RwRasterGetFormat(raster);
    GXTexFilter minTexFilter;
    GXTexFilter magTexFilter;

    if (format & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        RwUInt32 tlutName;
        RwTextureFilterMode filterMode;

        if (texExt->flags & rwDLTEXTUREEXTPRELOADED)
        {
            tlutName = GXGetTexObjTlut(&texExt->texObj);
        }
        else
        {
            tlutName = name;
        }

        GXInitTexObjCI(&texExt->texObj, rasExt->pixels, (u16)raster->width, (u16)raster->height,
                       (GXCITexFmt)rasExt->format,
                       _RwDlAddressConvTable[RwTextureGetAddressingU(texture)],
                       _RwDlAddressConvTable[RwTextureGetAddressingV(texture)],
                       (GXBool)((format & rwRASTERFORMATMIPMAP) ? GX_TRUE : GX_FALSE), tlutName);

        filterMode = RwTextureGetFilterMode(texture);

        /* Colour index textures cannot filter between mip levels */
        if ((filterMode == rwFILTERLINEARMIPLINEAR) || (filterMode == rwFILTERLINEARMIPNEAREST))
        {
            minTexFilter = _RwDlFilterModeConvTable[rwFILTERMIPLINEAR].min;
            magTexFilter = _RwDlFilterModeConvTable[rwFILTERMIPLINEAR].mag;
        }
        else
        {
            minTexFilter = _RwDlFilterModeConvTable[filterMode].min;
            magTexFilter = _RwDlFilterModeConvTable[filterMode].mag;
        }
    }
    else
    {
        GXInitTexObj(&texExt->texObj, rasExt->pixels, (u16)raster->width, (u16)raster->height,
                     (GXTexFmt)rasExt->format,
                     _RwDlAddressConvTable[RwTextureGetAddressingU(texture)],
                     _RwDlAddressConvTable[RwTextureGetAddressingV(texture)],
                     (GXBool)((format & rwRASTERFORMATMIPMAP) ? GX_TRUE : GX_FALSE));

        minTexFilter = _RwDlFilterModeConvTable[RwTextureGetFilterMode(texture)].min;
        magTexFilter = _RwDlFilterModeConvTable[RwTextureGetFilterMode(texture)].mag;
    }

    GXInitTexObjLOD(&texExt->texObj, minTexFilter, magTexFilter, 0.0f, (f32)rasExt->maxLOD, lodBias,
                    (GXBool)biasClamp, (GXBool)edgeLod, (GXAnisotropy)maxAniso);

    texExt->flags = ((texExt->flags & 0xFFFF0000) | (texture->filterAddressing & 0x0000FFFF)) &
                    ~rwDLTEXTUREEXTDEFAULTLOD;
}

void _rwDlTextureSet(RwTexture* texture, RwInt32 index)
{
    RwRaster* raster;
    RwGameCubeRasterExtension* rasExt;
    _rwDlTextureExt* texExt;

    if (!texture)
    {
        texture = _RwDlTexture;
        _rwDlTextureSetRaster(texture, _RwDlRasterWhite, 0);
    }

    raster = texture->raster;
    texExt = TEXTUREEXTFROMTEXTURE(texture);
    rasExt = RASTEREXTFROMRASTER(raster->parent);

    rasExt->token = _RwDlTokenCurrent;

    if (RwRasterGetFormat(raster) & (rwRASTERFORMATPAL4 | rwRASTERFORMATPAL8))
    {
        if (texExt->flags & rwDLTEXTUREEXTDEFAULTLOD)
        {
            _rwGameCubeTextureSetLOD(texture, 0.0f, TRUE, TRUE, GX_ANISO_1, index);
        }
        else if ((texture->filterAddressing & 0xFFFF) != (texExt->flags & 0xFFFF))
        {
            _rwGameCubeTextureSetLOD(texture, GXGetTexObjLODBias(&texExt->texObj),
                                     GXGetTexObjBiasClamp(&texExt->texObj),
                                     GXGetTexObjEdgeLOD(&texExt->texObj),
                                     GXGetTexObjMaxAniso(&texExt->texObj), index);
        }
        else if (texture != _RwDlTextureCache[index])
        {
            if (!(texExt->flags & rwDLTEXTUREEXTPRELOADED) &&
                (GXGetTexObjTlut(&texExt->texObj) != (RwUInt32)index))
            {
                _rwGameCubeTextureSetLOD(texture, GXGetTexObjLODBias(&texExt->texObj),
                                         GXGetTexObjBiasClamp(&texExt->texObj),
                                         GXGetTexObjEdgeLOD(&texExt->texObj),
                                         GXGetTexObjMaxAniso(&texExt->texObj), index);
            }
        }
        else
        {
            return;
        }

        if (!(texExt->flags & rwDLTEXTUREEXTPRELOADED))
        {
            GXLoadTlut(&rasExt->tlutObj, index);
        }
    }
    else
    {
        if (texExt->flags & rwDLTEXTUREEXTDEFAULTLOD)
        {
            _rwGameCubeTextureSetLOD(texture, 0.0f, TRUE, TRUE, GX_ANISO_1, 0);
        }
        else if ((texture->filterAddressing & 0xFFFF) != (texExt->flags & 0xFFFF))
        {
            _rwGameCubeTextureSetLOD(texture, GXGetTexObjLODBias(&texExt->texObj),
                                     GXGetTexObjBiasClamp(&texExt->texObj),
                                     GXGetTexObjEdgeLOD(&texExt->texObj),
                                     GXGetTexObjMaxAniso(&texExt->texObj), 0);
        }
        else if (texture == _RwDlTextureCache[index])
        {
            return;
        }
    }

    if (!(texExt->flags & rwDLTEXTUREEXTPRELOADED))
    {
        GXLoadTexObj(&texExt->texObj, (GXTexMapID)index);
    }
    else
    {
        GXLoadTexObjPreLoaded(&texExt->texObj, (GXTexRegion*)rasExt->region, (GXTexMapID)index);
    }

    _RwDlTextureCache[index] = texture;
}

void RwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias, RwBool biasClamp, RwBool edgeLod,
                             RwUInt32 maxAniso)
{
    _rwGameCubeTextureSetLOD(texture, lodBias, biasClamp, edgeLod, maxAniso, 0);
}
