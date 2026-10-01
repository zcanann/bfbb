#include <rwsdk/rwcore.h>
#include <rwsdk/driver/gcn/dlrendst.h>
#include <string.h>

#include "rwsdk/driver/gcn/dlprivate.h"

static GXFogType _RwDlFogConvTable[4] = {
    GX_FOG_NONE, /* rwFOGTYPENAFOGTYPE */
    GX_FOG_LIN, /* rwFOGTYPELINEAR */
    GX_FOG_EXP, /* rwFOGTYPEEXPONENTIAL */
    GX_FOG_EXP2, /* rwFOGTYPEEXPONENTIAL2 */
};

static GXBlendFactor _RwDlBlendConvTable[12] = {
    GX_BL_ZERO, /* rwBLENDNABLEND */
    GX_BL_ZERO, /* rwBLENDZERO */
    GX_BL_ONE, /* rwBLENDONE */
    GX_BL_SRCCLR, /* rwBLENDSRCCOLOR */
    GX_BL_INVSRCCLR, /* rwBLENDINVSRCCOLOR */
    GX_BL_SRCALPHA, /* rwBLENDSRCALPHA */
    GX_BL_INVSRCALPHA, /* rwBLENDINVSRCALPHA */
    GX_BL_DSTALPHA, /* rwBLENDDESTALPHA */
    GX_BL_INVDSTALPHA, /* rwBLENDINVDESTALPHA */
    GX_BL_DSTCLR, /* rwBLENDDESTCOLOR */
    GX_BL_INVDSTCLR, /* rwBLENDINVDESTCOLOR */
    GX_BL_ZERO, /* rwBLENDSRCALPHASAT */
};

_rwDlStateCache _RwDlStateCache;

RwRaster* _RwDlRasterWhite;
RwTexture* _RwDlTexture;

void _rwDlRenderStateOpen(void)
{
    const GXColor white = { 255, 255, 255, 255 };
    RwUInt8* pixels;

    _RwDlStateCache.fogEnable = FALSE;
    _RwDlStateCache.fogType = rwFOGTYPELINEAR;
    _RwDlStateCache.packedFogColor = 0xFFFFFFFF;
    _RwDlStateCache.fogColor.r = 255;
    _RwDlStateCache.fogColor.g = 255;
    _RwDlStateCache.fogColor.b = 255;
    _RwDlStateCache.fogColor.a = 255;
    _RwDlStateCache.nearFogPlane = 5.0f;
    _RwDlStateCache.farFogPlane = 10.0f;
    _RwDlStateCache.nearPlane = 0.05f;
    _RwDlStateCache.farPlane = 10.0f;
    _RwDlStateCache.userFarFogPlane = FALSE;

    _RwDlStateCache.zWriteEnable = TRUE;
    _RwDlStateCache.zTestEnable = TRUE;
    _RwDlStateCache.zCompare = GX_LEQUAL;
    GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);

    _RwDlStateCache.zBeforeTex = TRUE;
    GXSetZCompLoc(GX_TRUE);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);

    _RwDlStateCache.comp0 = GX_GREATER;
    _RwDlStateCache.ref0 = 0;
    _RwDlStateCache.op = GX_AOP_AND;
    _RwDlStateCache.comp1 = GX_GREATER;
    _RwDlStateCache.ref1 = 0;

    _RwDlStateCache.srcBlend = rwBLENDSRCALPHA;
    _RwDlStateCache.dstBlend = rwBLENDINVSRCALPHA;
    _RwDlStateCache.gxSrcBlend = GX_BL_SRCALPHA;
    _RwDlStateCache.gxDstBlend = GX_BL_INVSRCALPHA;
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

    _RwDlStateCache.cullMode = rwCULLMODECULLBACK;
    GXSetCullMode(GX_CULL_FRONT);

    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

    GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_ALPHA1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);

    GXSetChanMatColor(GX_ALPHA0, white);
    GXSetChanMatColor(GX_ALPHA1, white);
    GXSetChanMatColor(GX_COLOR0, white);
    GXSetChanMatColor(GX_COLOR1, white);

    GXSetColorUpdate(GX_TRUE);
    GXSetAlphaUpdate(GX_TRUE);

    GXSetCopyClear(white, GX_MAX_Z24);

    GXSetCurrentMtx(GX_PNMTX0);

    _rwDlTextureCacheInit();

    /* A plain white texture is used when texturing is disabled */
    _RwDlTexture = RwTextureCreate(NULL);
    RwTextureSetFilterMode(_RwDlTexture, rwFILTERLINEAR);
    RwTextureSetAddressing(_RwDlTexture, rwTEXTUREADDRESSWRAP);

    _RwDlRasterWhite = RwRasterCreate(4, 4, 16, rwRASTERTYPETEXTURE | rwRASTERFORMAT565);

    pixels = RwRasterLock(_RwDlRasterWhite, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);
    memset(pixels, 0xFF, 4 * 4 * sizeof(RwUInt16));
    RwRasterUnlock(_RwDlRasterWhite);
}

void _rwDlRenderStateClose(void)
{
    RwRasterDestroy(_RwDlRasterWhite);
    _RwDlRasterWhite = NULL;

    RwTextureSetRaster(_RwDlTexture, NULL);
    RwTextureDestroy(_RwDlTexture);
    _RwDlTexture = NULL;
}

RwBool _rwDlGetRenderState(RwRenderState state, void* param)
{
    switch (state)
    {
    case rwRENDERSTATEFOGENABLE:
    {
        *(RwBool*)param = _RwDlStateCache.fogEnable;
        return TRUE;
    }
    case rwRENDERSTATEFOGTYPE:
    {
        *(RwFogType*)param = _RwDlStateCache.fogType;
        return TRUE;
    }
    case rwRENDERSTATEFOGCOLOR:
    {
        *(RwUInt32*)param = _RwDlStateCache.packedFogColor;
        return TRUE;
    }
    case rwRENDERSTATEFOGDENSITY:
    {
        return FALSE;
    }
    case rwRENDERSTATETEXTUREADDRESS:
    {
        if (RwTextureGetAddressingU(_RwDlTexture) == RwTextureGetAddressingV(_RwDlTexture))
        {
            *(RwTextureAddressMode*)param = RwTextureGetAddressingU(_RwDlTexture);
            return TRUE;
        }
        return FALSE;
    }
    case rwRENDERSTATETEXTUREADDRESSU:
    {
        *(RwTextureAddressMode*)param = RwTextureGetAddressingU(_RwDlTexture);
        return TRUE;
    }
    case rwRENDERSTATETEXTUREADDRESSV:
    {
        *(RwTextureAddressMode*)param = RwTextureGetAddressingV(_RwDlTexture);
        return TRUE;
    }
    case rwRENDERSTATETEXTUREFILTER:
    {
        *(RwTextureFilterMode*)param = RwTextureGetFilterMode(_RwDlTexture);
        return TRUE;
    }
    case rwRENDERSTATETEXTURERASTER:
    {
        *(RwRaster**)param = RwTextureGetRaster(_RwDlTexture);
        return TRUE;
    }
    case rwRENDERSTATEZWRITEENABLE:
    {
        *(RwBool*)param = _RwDlStateCache.zWriteEnable;
        return TRUE;
    }
    case rwRENDERSTATEZTESTENABLE:
    {
        *(RwBool*)param = _RwDlStateCache.zTestEnable;
        return TRUE;
    }
    case rwRENDERSTATESRCBLEND:
    {
        *(RwBlendFunction*)param = _RwDlStateCache.srcBlend;
        return TRUE;
    }
    case rwRENDERSTATEDESTBLEND:
    {
        *(RwBlendFunction*)param = _RwDlStateCache.dstBlend;
        return TRUE;
    }
    case rwRENDERSTATESHADEMODE:
    {
        *(RwShadeMode*)param = rwSHADEMODEGOURAUD;
        return TRUE;
    }
    case rwRENDERSTATEBORDERCOLOR:
    {
        return FALSE;
    }
    case rwRENDERSTATETEXTUREPERSPECTIVE:
    {
        *(RwBool*)param = TRUE;
        return TRUE;
    }
    case rwRENDERSTATECULLMODE:
    {
        *(RwCullMode*)param = _RwDlStateCache.cullMode;
        return TRUE;
    }
    case rwRENDERSTATENARENDERSTATE:
    default:
    {
        return FALSE;
    }
    }
}

RwBool _rwDlRenderStateFogEnable(RwBool fog)
{
    if (fog)
    {
        if (!_RwDlStateCache.fogEnable)
        {
            RwCamera* camera = (RwCamera*)RWSRCGLOBAL(curCamera);

            if (camera)
            {
                if (!_RwDlStateCache.userFarFogPlane)
                {
                    _RwDlStateCache.farFogPlane = camera->farPlane;
                }

                _RwDlStateCache.nearFogPlane = camera->fogPlane;
                _RwDlStateCache.nearPlane = camera->nearPlane;
                _RwDlStateCache.farPlane = camera->farPlane;
            }

            GXSetFog(_RwDlFogConvTable[_RwDlStateCache.fogType], _RwDlStateCache.nearFogPlane,
                     _RwDlStateCache.farFogPlane, _RwDlStateCache.nearPlane,
                     _RwDlStateCache.farPlane, _RwDlStateCache.fogColor);

            _RwDlStateCache.fogEnable = TRUE;
        }
    }
    else
    {
        if (_RwDlStateCache.fogEnable)
        {
            GXSetFog(GX_FOG_NONE, 5.0f, 10.0f, 0.05f, 10.0f, _RwDlStateCache.fogColor);

            _RwDlStateCache.fogEnable = FALSE;
        }
    }

    return TRUE;
}

static RwBool _rwDlRenderStateFogColor(RwUInt32 fogColor)
{
    if (fogColor != _RwDlStateCache.packedFogColor)
    {
        RwCamera* camera = (RwCamera*)RWSRCGLOBAL(curCamera);

        _RwDlStateCache.fogColor.a = (RwUInt8)((fogColor >> 24) & 0xFF);
        _RwDlStateCache.fogColor.r = (RwUInt8)((fogColor >> 16) & 0xFF);
        _RwDlStateCache.fogColor.g = (RwUInt8)((fogColor >> 8) & 0xFF);
        _RwDlStateCache.fogColor.b = (RwUInt8)(fogColor & 0xFF);

        GXSetFog(_RwDlFogConvTable[_RwDlStateCache.fogType], camera->fogPlane, camera->farPlane,
                 camera->nearPlane, camera->farPlane, _RwDlStateCache.fogColor);

        _RwDlStateCache.packedFogColor = fogColor;
    }

    return TRUE;
}

static RwBool _rwDlRenderStateFogType(RwFogType fogType)
{
    if (fogType != _RwDlStateCache.fogType)
    {
        RwCamera* camera;

        /* Only linear fog is supported */
        if (fogType != rwFOGTYPELINEAR)
        {
            return FALSE;
        }

        camera = (RwCamera*)RWSRCGLOBAL(curCamera);

        GXSetFog(_RwDlFogConvTable[fogType], camera->fogPlane, camera->farPlane, camera->nearPlane,
                 camera->farPlane, _RwDlStateCache.fogColor);

        _RwDlStateCache.fogType = fogType;
    }

    return TRUE;
}

static RwBool _rwDlRenderStateFogDensity(void)
{
    return FALSE;
}

static RwBool _rwDlRenderStateTextureAddress(RwTextureAddressMode addressMode)
{
    if (addressMode == rwTEXTUREADDRESSBORDER)
    {
        return FALSE;
    }

    RwTextureSetAddressing(_RwDlTexture, addressMode);

    return TRUE;
}

static RwBool _rwDlRenderStateTextureAddressU(RwTextureAddressMode addressMode)
{
    if (addressMode == rwTEXTUREADDRESSBORDER)
    {
        return FALSE;
    }

    RwTextureSetAddressingU(_RwDlTexture, addressMode);

    return TRUE;
}

static RwBool _rwDlRenderStateTextureAddressV(RwTextureAddressMode addressMode)
{
    if (addressMode == rwTEXTUREADDRESSBORDER)
    {
        return FALSE;
    }

    RwTextureSetAddressingV(_RwDlTexture, addressMode);

    return TRUE;
}

static RwBool _rwDlRenderStateTextureFilter(RwTextureFilterMode filterMode)
{
    RwTextureSetFilterMode(_RwDlTexture, filterMode);

    return TRUE;
}

static RwBool _rwDlRenderStateTextureRaster(RwRaster* raster)
{
    if (raster != _RwDlTexture->raster)
    {
        _rwDlTextureSetRaster(_RwDlTexture, raster, 0);
    }

    return TRUE;
}

void _rwDlRenderStateSetZCompLoc(RwInt32 zBeforeTex)
{
    if (_RwDlStateCache.zBeforeTex != zBeforeTex)
    {
        if (zBeforeTex == TRUE)
        {
            /* Alpha compare cannot be used when z buffering before texturing */
            GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
        }
        else
        {
            GXSetAlphaCompare(_RwDlStateCache.comp0, _RwDlStateCache.ref0, _RwDlStateCache.op,
                              _RwDlStateCache.comp1, _RwDlStateCache.ref1);
        }

        GXSetZCompLoc((GXBool)zBeforeTex);

        _RwDlStateCache.zBeforeTex = zBeforeTex;
    }
}

void _rwDlTextureRasterFlush(void)
{
    if (_RwDlTexture->raster)
    {
        RwGameCubeRasterExtension* rasExt;

        _rwDlTextureSet(_RwDlTexture, 0);

        rasExt = RASTEREXTFROMRASTER(_RwDlTexture->raster);

        /* Textures with alpha need the z test after texturing */
        _rwDlRenderStateSetZCompLoc((rasExt->flags & 1) ? FALSE : TRUE);
    }
}

static RwBool _rwDlRenderStateZWriteEnable(RwBool enable)
{
    if (enable)
    {
        if (!_RwDlStateCache.zWriteEnable)
        {
            GXSetZMode(GX_TRUE, _RwDlStateCache.zCompare, GX_TRUE);
            _RwDlStateCache.zWriteEnable = TRUE;
        }
    }
    else
    {
        if (_RwDlStateCache.zWriteEnable)
        {
            GXSetZMode(GX_TRUE, _RwDlStateCache.zCompare, GX_FALSE);
            _RwDlStateCache.zWriteEnable = FALSE;
        }
    }

    return TRUE;
}

static RwBool _rwDlRenderStateZTestEnable(RwBool enable)
{
    if (enable)
    {
        if (!_RwDlStateCache.zTestEnable)
        {
            GXSetZMode(GX_TRUE, GX_LEQUAL, (GXBool)_RwDlStateCache.zWriteEnable);
            _RwDlStateCache.zTestEnable = TRUE;
            _RwDlStateCache.zCompare = GX_LEQUAL;
        }
    }
    else
    {
        if (_RwDlStateCache.zTestEnable)
        {
            GXSetZMode(GX_TRUE, GX_ALWAYS, (GXBool)_RwDlStateCache.zWriteEnable);
            _RwDlStateCache.zTestEnable = FALSE;
            _RwDlStateCache.zCompare = GX_ALWAYS;
        }
    }

    return TRUE;
}

static RwBool _rwDlRenderStateSrcBlend(RwBlendFunction srcBlend)
{
    if (srcBlend != _RwDlStateCache.srcBlend)
    {
        GXBlendFactor gxSrcBlend;

        switch (srcBlend)
        {
        case rwBLENDZERO:
        case rwBLENDONE:
        case rwBLENDSRCALPHA:
        case rwBLENDINVSRCALPHA:
        case rwBLENDDESTALPHA:
        case rwBLENDINVDESTALPHA:
        case rwBLENDDESTCOLOR:
        case rwBLENDINVDESTCOLOR:
        {
            gxSrcBlend = _RwDlBlendConvTable[srcBlend];

            GXSetBlendMode(GX_BM_BLEND, gxSrcBlend, _RwDlStateCache.gxDstBlend, GX_LO_CLEAR);

            _RwDlStateCache.srcBlend = srcBlend;
            _RwDlStateCache.gxSrcBlend = gxSrcBlend;
            break;
        }
        case rwBLENDSRCALPHASAT:
        default:
        {
            return FALSE;
        }
        }
    }

    return TRUE;
}

static RwBool _rwDlRenderStateDstBlend(RwBlendFunction dstBlend)
{
    if (dstBlend != _RwDlStateCache.dstBlend)
    {
        GXBlendFactor gxDstBlend;

        switch (dstBlend)
        {
        case rwBLENDZERO:
        case rwBLENDONE:
        case rwBLENDSRCCOLOR:
        case rwBLENDINVSRCCOLOR:
        case rwBLENDSRCALPHA:
        case rwBLENDINVSRCALPHA:
        case rwBLENDDESTALPHA:
        case rwBLENDINVDESTALPHA:
        {
            gxDstBlend = _RwDlBlendConvTable[dstBlend];

            GXSetBlendMode(GX_BM_BLEND, _RwDlStateCache.gxSrcBlend, gxDstBlend, GX_LO_CLEAR);

            _RwDlStateCache.dstBlend = dstBlend;
            _RwDlStateCache.gxDstBlend = gxDstBlend;
            break;
        }
        case rwBLENDDESTCOLOR:
        case rwBLENDINVDESTCOLOR:
        default:
        {
            return FALSE;
        }
        }
    }

    return TRUE;
}

static RwBool _rwDlRenderStateCullMode(RwCullMode cullMode)
{
    if (cullMode != _RwDlStateCache.cullMode)
    {
        /* GX culls by the opposite winding */
        GXSetCullMode((GXCullMode)(cullMode - 1));
        _RwDlStateCache.cullMode = cullMode;
    }

    return TRUE;
}

RwBool _rwDlSetRenderState(RwRenderState state, void* param)
{
    RwBool result = FALSE;

    switch (state)
    {
    case rwRENDERSTATEFOGENABLE:
    {
        result = _rwDlRenderStateFogEnable((RwBool)param);
        break;
    }
    case rwRENDERSTATEFOGCOLOR:
    {
        result = _rwDlRenderStateFogColor((RwUInt32)param);
        break;
    }
    case rwRENDERSTATEFOGTYPE:
    {
        result = _rwDlRenderStateFogType((RwFogType)param);
        break;
    }
    case rwRENDERSTATEFOGDENSITY:
    {
        result = _rwDlRenderStateFogDensity();
        break;
    }
    case rwRENDERSTATETEXTUREADDRESS:
    {
        result = _rwDlRenderStateTextureAddress((RwTextureAddressMode)param);
        break;
    }
    case rwRENDERSTATETEXTUREADDRESSU:
    {
        result = _rwDlRenderStateTextureAddressU((RwTextureAddressMode)param);
        break;
    }
    case rwRENDERSTATETEXTUREADDRESSV:
    {
        result = _rwDlRenderStateTextureAddressV((RwTextureAddressMode)param);
        break;
    }
    case rwRENDERSTATETEXTUREFILTER:
    {
        result = _rwDlRenderStateTextureFilter((RwTextureFilterMode)param);
        break;
    }
    case rwRENDERSTATETEXTURERASTER:
    {
        result = _rwDlRenderStateTextureRaster((RwRaster*)param);
        break;
    }
    case rwRENDERSTATEZWRITEENABLE:
    {
        result = _rwDlRenderStateZWriteEnable((RwBool)param);
        break;
    }
    case rwRENDERSTATEZTESTENABLE:
    {
        result = _rwDlRenderStateZTestEnable((RwBool)param);
        break;
    }
    case rwRENDERSTATESRCBLEND:
    {
        result = _rwDlRenderStateSrcBlend((RwBlendFunction)param);
        break;
    }
    case rwRENDERSTATEDESTBLEND:
    {
        result = _rwDlRenderStateDstBlend((RwBlendFunction)param);
        break;
    }
    case rwRENDERSTATESHADEMODE:
    {
        result = ((RwShadeMode)param == rwSHADEMODEGOURAUD);
        break;
    }
    case rwRENDERSTATEBORDERCOLOR:
    {
        result = FALSE;
        break;
    }
    case rwRENDERSTATETEXTUREPERSPECTIVE:
    {
        result = (RwBool)param;
        break;
    }
    case rwRENDERSTATECULLMODE:
    {
        result = _rwDlRenderStateCullMode((RwCullMode)param);
        break;
    }
    case rwRENDERSTATENARENDERSTATE:
    default:
    {
        break;
    }
    }

    return result;
}

void RwGameCubeSetAlphaCompare(RwInt32 comp0, RwUInt8 ref0, RwInt32 op, RwInt32 comp1, RwUInt8 ref1)
{
    if (_RwDlStateCache.zBeforeTex != TRUE)
    {
        GXSetAlphaCompare(_RwDlStateCache.comp0, _RwDlStateCache.ref0, _RwDlStateCache.op,
                          _RwDlStateCache.comp1, _RwDlStateCache.ref1);
    }

    _RwDlStateCache.comp0 = (GXCompare)comp0;
    _RwDlStateCache.ref0 = ref0;
    _RwDlStateCache.op = (GXAlphaOp)op;
    _RwDlStateCache.comp1 = (GXCompare)comp1;
    _RwDlStateCache.ref1 = ref1;
}
