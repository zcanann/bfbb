#include <rwsdk/rwcore.h>
#include <rwsdk/driver/gcn/dlrendst.h>
#include <dolphin/vi.h>
#include <string.h>

#include "rwsdk/driver/gcn/dlprivate.h"

static RwRaster* _RwDlRasterTarget;

static void _rwDlRasterRenderQuadInit(RwRect* rect)
{
    static f32 projVector[7] = { GX_ORTHOGRAPHIC, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f };
    static f32 posMatrix[3][4] = {
        { 1.0f, 0.0f, 0.0f, 0.5f },
        { 0.0f, 1.0f, 0.0f, 0.5f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
    };

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevOp(GX_TEVSTAGE0, GX_REPLACE);

    GXSetNumChans(0);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);

    if (_RwDlRenderMode->field_rendering)
    {
        GXSetViewportJitter(0.0f, 0.0f, (f32)_RwDlRenderMode->fbWidth,
                            (f32)_RwDlRenderMode->xfbHeight, 0.0f, 1.0f, VIGetNextField() ^ 1);
    }
    else
    {
        GXSetViewport(0.0f, 0.0f, (f32)_RwDlRenderMode->fbWidth, (f32)_RwDlRenderMode->xfbHeight,
                      0.0f, 1.0f);
    }

    if (!_RwDlFSAA)
    {
        GXSetScissor(_RwDlRasterTarget->nOffsetX + rect->x, _RwDlRasterTarget->nOffsetY + rect->y,
                     rect->w, rect->h);
    }
    else if (_RwDlFSAATop)
    {
        RwInt32 y = (_RwDlRasterTarget->nOffsetY << 1) + rect->y;

        if (rect->h + y <= _RwDlHalfHeight + 2)
        {
            GXSetScissor(_RwDlRasterTarget->nOffsetX + rect->x, y, rect->w, rect->h);
        }
        else if (y > _RwDlHalfHeight + 2)
        {
            GXSetScissor(0, 0, _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
        }
        else
        {
            GXSetScissor(_RwDlRasterTarget->nOffsetX + rect->x, y, rect->w,
                         (_RwDlHalfHeight + 2) - y);
        }

        GXSetScissorBoxOffset(0, 0);
    }
    else
    {
        RwInt32 y = (_RwDlRasterTarget->nOffsetY << 1) + rect->y;

        if (y >= _RwDlHalfHeight - 2)
        {
            GXSetScissor(_RwDlRasterTarget->nOffsetX + rect->x, y, rect->w, rect->h);
        }
        else if (rect->h + y < _RwDlHalfHeight - 2)
        {
            GXSetScissor(0, _RwDlHalfHeight - 2, _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
        }
        else
        {
            GXSetScissor(_RwDlRasterTarget->nOffsetX + rect->x, _RwDlHalfHeight - 2, rect->w,
                         (rect->h + y) - (_RwDlHalfHeight - 2));
        }

        GXSetScissorBoxOffset(0, _RwDlHalfHeight - 2);
    }

    projVector[1] = 2.0f / (f32)_RwDlRenderMode->fbWidth;
    projVector[3] = -2.0f / (f32)_RwDlRenderMode->xfbHeight;

    GXSetProjectionv(projVector);
    GXLoadPosMtxImm(posMatrix, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

static void _rwDlRasterRenderQuad(RwRaster* raster, RwRect* rect, RwBool scaled, RwBool alpha)
{
    RwBool fogEnable;
    RwTextureFilterMode filterMode;
    RwTextureAddressMode addressModeU;
    RwTextureAddressMode addressModeV;
    RwRaster* curRaster;
    RwRaster* parentRaster = raster->parent;

    /* Save the render states we are about to change */
    _rwDlGetRenderState(rwRENDERSTATEFOGENABLE, &fogEnable);
    _rwDlGetRenderState(rwRENDERSTATETEXTUREFILTER, &filterMode);
    _rwDlGetRenderState(rwRENDERSTATETEXTUREADDRESSU, &addressModeU);
    _rwDlGetRenderState(rwRENDERSTATETEXTUREADDRESSV, &addressModeV);
    _rwDlGetRenderState(rwRENDERSTATETEXTURERASTER, &curRaster);

    GXSetCullMode(GX_CULL_NONE);

    if (alpha)
    {
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    }
    else
    {
        GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    }

    _rwDlSetRenderState(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREFILTER,
                        (void*)(scaled ? rwFILTERLINEAR : rwFILTERNEAREST));
    _rwDlSetRenderState(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
    _rwDlSetRenderState(rwRENDERSTATETEXTURERASTER, (void*)parentRaster);

    _rwDlTextureRasterFlush();

    _rwDlRasterRenderQuadInit(rect);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    if (scaled)
    {
        RwReal recipWidth;
        RwReal recipHeight;

        recipWidth = 1.0f / (RwReal)parentRaster->width;
        recipHeight = 1.0f / (RwReal)parentRaster->height;

        GXPosition2s16(_RwDlRasterTarget->nOffsetX + rect->x, _RwDlRasterTarget->nOffsetY + rect->y);
        GXTexCoord2f32((RwReal)raster->nOffsetX * recipWidth,
                       (RwReal)raster->nOffsetY * recipHeight);

        GXPosition2s16(_RwDlRasterTarget->nOffsetX + rect->x,
                       _RwDlRasterTarget->nOffsetY + rect->y + rect->h);
        GXTexCoord2f32((RwReal)raster->nOffsetX * recipWidth,
                       recipHeight * (RwReal)(raster->nOffsetY + raster->height));

        GXPosition2s16(_RwDlRasterTarget->nOffsetX + rect->x + rect->w,
                       _RwDlRasterTarget->nOffsetY + rect->y + rect->h);
        GXTexCoord2f32(recipWidth * (RwReal)(raster->nOffsetX + raster->width),
                       recipHeight * (RwReal)(raster->nOffsetY + raster->height));

        GXPosition2s16(_RwDlRasterTarget->nOffsetX + rect->x + rect->w,
                       _RwDlRasterTarget->nOffsetY + rect->y);
        GXTexCoord2f32(recipWidth * (RwReal)(raster->nOffsetX + raster->width),
                       (RwReal)raster->nOffsetY * recipHeight);
    }
    else
    {
        RwReal recipWidth;
        RwReal recipHeight;

        recipWidth = 1.0f / (RwReal)parentRaster->width;
        recipHeight = 1.0f / (RwReal)parentRaster->height;

        GXPosition2s16(_RwDlRasterTarget->nOffsetX + rect->x, _RwDlRasterTarget->nOffsetY + rect->y);
        GXTexCoord2f32((RwReal)raster->nOffsetX * recipWidth,
                       (RwReal)raster->nOffsetY * recipHeight);

        GXPosition2s16(_RwDlRasterTarget->nOffsetX + rect->x,
                       _RwDlRasterTarget->nOffsetY + rect->y + raster->height);
        GXTexCoord2f32((RwReal)raster->nOffsetX * recipWidth,
                       recipHeight * (RwReal)(raster->nOffsetY + raster->height));

        GXPosition2s16(_RwDlRasterTarget->nOffsetX + rect->x + raster->width,
                       _RwDlRasterTarget->nOffsetY + rect->y + raster->height);
        GXTexCoord2f32(recipWidth * (RwReal)(raster->nOffsetX + raster->width),
                       recipHeight * (RwReal)(raster->nOffsetY + raster->height));

        GXPosition2s16(_RwDlRasterTarget->nOffsetX + rect->x + raster->width,
                       _RwDlRasterTarget->nOffsetY + rect->y);
        GXTexCoord2f32(recipWidth * (RwReal)(raster->nOffsetX + raster->width),
                       (RwReal)raster->nOffsetY * recipHeight);
    }

    GXEnd();

    /* Restore the render state */
    GXSetBlendMode(GX_BM_BLEND, _RwDlStateCache.gxSrcBlend, _RwDlStateCache.gxDstBlend,
                   GX_LO_CLEAR);
    GXSetCullMode((GXCullMode)(_RwDlStateCache.cullMode - 1));

    _rwDlSetRenderState(rwRENDERSTATEFOGENABLE, (void*)fogEnable);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREFILTER, (void*)filterMode);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREADDRESSU, (void*)addressModeU);
    _rwDlSetRenderState(rwRENDERSTATETEXTUREADDRESSV, (void*)addressModeV);
    _rwDlSetRenderState(rwRENDERSTATETEXTURERASTER, (void*)curRaster);
}

static RwBool _rwDlRasterRenderGeneric(RwRaster* raster, RwRect* rect, RwBool scaled, RwBool alpha)
{
    RwRect scissorRect;

    scissorRect.x = rect->x;
    scissorRect.y = rect->y;

    if (scaled)
    {
        scissorRect.w = rect->w;
        scissorRect.h = rect->h;
    }
    else
    {
        scissorRect.w = raster->width;
        scissorRect.h = raster->height;
    }

    switch (_RwDlRasterTarget->cType)
    {
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
    {
        switch (raster->cType)
        {
        case rwRASTERTYPENORMAL:
        case rwRASTERTYPEZBUFFER:
        case rwRASTERTYPECAMERA:
        case rwRASTERTYPETEXTURE:
        case rwRASTERTYPECAMERATEXTURE:
        {
            RWERROR((E_RW_DEVICEERROR, "SRC, DST Raster render combination not supported"));
            return FALSE;
            break;
        }
        }
        break;
    }
    case rwRASTERTYPECAMERA:
    {
        switch (raster->cType)
        {
        case rwRASTERTYPENORMAL:
        case rwRASTERTYPETEXTURE:
        case rwRASTERTYPECAMERATEXTURE:
        {
            _rwDlRasterRenderQuad(raster, &scissorRect, scaled, alpha);
            break;
        }
        case rwRASTERTYPEZBUFFER:
        case rwRASTERTYPECAMERA:
        {
            RWERROR((E_RW_DEVICEERROR, "SRC, DST Raster render combination not supported"));
            return FALSE;
            break;
        }
        }
        break;
    }
    case rwRASTERTYPEZBUFFER:
    default:
    {
        RWERROR((E_RW_DEVICEERROR, "SRC, DST Raster render combination not supported"));
        return FALSE;
        break;
    }
    }

    return TRUE;
}

RwBool _rwDlRasterRender(void* rasterIn, void* rectIn, RwInt32 flags)
{
    return _rwDlRasterRenderGeneric((RwRaster*)rasterIn, (RwRect*)rectIn, FALSE, TRUE);
}

RwBool _rwDlRasterRenderFast(void* rasterIn, void* rectIn, RwInt32 flags)
{
    return _rwDlRasterRenderGeneric((RwRaster*)rasterIn, (RwRect*)rectIn, FALSE, FALSE);
}

RwBool _rwDlRasterRenderScaled(void* rasterIn, void* rectIn, RwInt32 flags)
{
    return _rwDlRasterRenderGeneric((RwRaster*)rasterIn, (RwRect*)rectIn, TRUE, TRUE);
}

static void _rwDlRasterCamera_ZClearRectInit(RwRaster* raster)
{
    static f32 projVector[7] = { GX_ORTHOGRAPHIC, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f };
    static f32 posMatrix[3][4] = {
        { 1.0f, 0.0f, 0.0f, 0.5f },
        { 0.0f, 1.0f, 0.0f, 0.5f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
    };

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);

    GXSetNumTexGens(0);

    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);

    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);

    if (raster->cType == rwRASTERTYPECAMERATEXTURE)
    {
        RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster->parent);

        /* Render to texture uses a fixed size EFB region */
        if (rasExt->flags & 1)
        {
            if (_RwDlPixelFormat != GX_PF_RGBA6_Z24)
            {
                GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
                _RwDlCurPixelFormat = GX_PF_RGBA6_Z24;
            }
        }
        else
        {
            if (_RwDlPixelFormat != GX_PF_RGB8_Z24)
            {
                GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
                _RwDlCurPixelFormat = GX_PF_RGB8_Z24;
            }
        }

        GXSetViewport(0.0f, 0.0f, 640.0f, 528.0f, 0.0f, 1.0f);
        GXSetScissor(0, 0, 640, 528);

        projVector[1] = 2.0f / 639.0f;
        projVector[3] = -2.0f / 527.0f;
    }
    else
    {
        if (_RwDlCurPixelFormat != _RwDlPixelFormat)
        {
            GXSetPixelFmt(_RwDlPixelFormat, GX_ZC_LINEAR);
            _RwDlCurPixelFormat = _RwDlPixelFormat;
        }

        if (_RwDlRenderMode->field_rendering)
        {
            GXSetViewportJitter(0.0f, 0.0f, (f32)_RwDlRenderMode->fbWidth,
                                (f32)_RwDlRenderMode->xfbHeight, 0.0f, 1.0f,
                                VIGetNextField() ^ 1);
        }
        else
        {
            GXSetViewport(0.0f, 0.0f, (f32)_RwDlRenderMode->fbWidth,
                          (f32)_RwDlRenderMode->xfbHeight, 0.0f, 1.0f);
        }

        if (!_RwDlFSAA)
        {
            GXSetScissor(0, 0, _RwDlRenderMode->fbWidth, _RwDlRenderMode->xfbHeight);
        }
        else if (_RwDlFSAATop)
        {
            GXSetScissor(0, 0, _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
            GXSetScissorBoxOffset(0, 0);
        }
        else
        {
            GXSetScissor(0, _RwDlHalfHeight - 2, _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
            GXSetScissorBoxOffset(0, _RwDlHalfHeight - 2);
        }

        projVector[1] = 2.0f / (f32)_RwDlRenderMode->fbWidth;
        projVector[3] = -2.0f / (f32)_RwDlRenderMode->xfbHeight;
    }

    GXSetProjectionv(projVector);
    GXLoadPosMtxImm(posMatrix, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

void _rwDlRasterCamera_ZClearRect(RwRaster* raster, RwRect* rect, RwRGBA* color,
                                  RwInt32 clearFlags)
{
    RwBool enableFog = FALSE;
    GXColor clearColor;

    if (clearFlags & rwCAMERACLEARIMAGE)
    {
        clearColor.r = color->red;
        clearColor.g = color->green;
        clearColor.b = color->blue;
        clearColor.a = color->alpha;

        if (clearFlags & rwCAMERACLEARZ)
        {
            GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
        }
        else
        {
            GXSetZMode(GX_TRUE, GX_ALWAYS, GX_FALSE);
        }
    }
    else if (clearFlags & rwCAMERACLEARZ)
    {
        clearColor.r = 0;
        clearColor.g = 0;
        clearColor.b = 0;
        clearColor.a = 255;

        GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
        GXSetColorUpdate(GX_FALSE);
    }
    else
    {
        return;
    }

    GXSetChanMatColor(GX_COLOR0A0, clearColor);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GXSetCullMode(GX_CULL_NONE);

    _rwDlRenderStateSetZCompLoc(TRUE);

    if (_RwDlStateCache.fogEnable)
    {
        _rwDlRenderStateFogEnable(FALSE);
        enableFog = TRUE;
    }

    _rwDlRasterCamera_ZClearRectInit(raster);

    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition3f32((RwReal)rect->x, (RwReal)rect->y, 0.99999994f);
    GXPosition3f32((RwReal)rect->x, (RwReal)(rect->y + rect->h), 0.99999994f);
    GXPosition3f32((RwReal)(rect->x + rect->w), (RwReal)(rect->y + rect->h), 0.99999994f);
    GXPosition3f32((RwReal)(rect->x + rect->w), (RwReal)rect->y, 0.99999994f);

    GXEnd();

    if (enableFog)
    {
        _rwDlRenderStateFogEnable(TRUE);
    }

    GXSetZMode(GX_TRUE, _RwDlStateCache.zCompare, (GXBool)_RwDlStateCache.zWriteEnable);
    GXSetBlendMode(GX_BM_BLEND, _RwDlStateCache.gxSrcBlend, _RwDlStateCache.gxDstBlend,
                   GX_LO_CLEAR);
    GXSetCullMode((GXCullMode)(_RwDlStateCache.cullMode - 1));
    GXSetColorUpdate(GX_TRUE);
}

static RwBool _rwDlRasterClearGeneric(RwRaster* raster, RwRect* rect, RwInt32 packedColor)
{
    switch (_RwDlRasterTarget->cType)
    {
    case rwRASTERTYPECAMERA:
    {
        RwRGBA color;

        RwRGBASetFromPixel(&color, packedColor, RwRasterGetFormat(_RwDlRasterTarget));

        rect->x += _RwDlRasterTarget->nOffsetX;
        rect->y += _RwDlRasterTarget->nOffsetY;

        _rwDlRasterCamera_ZClearRect(raster, rect, &color, rwCAMERACLEARIMAGE);
        break;
    }
    case rwRASTERTYPEZBUFFER:
    {
        rect->x += _RwDlRasterTarget->nOffsetX;
        rect->y += _RwDlRasterTarget->nOffsetY;

        _rwDlRasterCamera_ZClearRect(raster, rect, NULL, rwCAMERACLEARZ);
        break;
    }
    case rwRASTERTYPENORMAL:
    case rwRASTERTYPETEXTURE:
    case rwRASTERTYPECAMERATEXTURE:
    {
        RwBool rasterLocked = FALSE;

        if ((_RwDlRasterTarget->parent == _RwDlRasterTarget) && !_RwDlRasterTarget->nOffsetX &&
            !_RwDlRasterTarget->nOffsetY && (_RwDlRasterTarget->width == rect->w) &&
            (_RwDlRasterTarget->height == rect->h))
        {
            /* Whole raster */
            if (!(_RwDlRasterTarget->privateFlags & rwRASTERPIXELLOCKEDWRITE))
            {
                rasterLocked = TRUE;
                RwRasterLock(_RwDlRasterTarget, 0, rwRASTERLOCKWRITE | rwRASTERLOCKRAW);
            }

            if ((packedColor == 0) || (packedColor == -1))
            {
                memset(_RwDlRasterTarget->cpPixels, packedColor,
                       _RwDlRasterTarget->stride * _RwDlRasterTarget->height);

                if (rasterLocked)
                {
                    RwRasterUnlock(_RwDlRasterTarget);
                }

                return TRUE;
            }
        }
        else
        {
            if (!(_RwDlRasterTarget->privateFlags & rwRASTERPIXELLOCKEDWRITE))
            {
                rasterLocked = TRUE;
                RwRasterLock(_RwDlRasterTarget, 0, rwRASTERLOCKREADWRITE);
            }
        }

        switch (_RwDlRasterTarget->depth)
        {
        case 4:
        {
            RwUInt32 y;
            RwUInt8 color = (RwUInt8)(((packedColor << 4) & 0xF0) | (packedColor & 0x0F));

            for (y = 0; y < rect->h; y++)
            {
                RwUInt8* pixel = _RwDlRasterTarget->cpPixels +
                                 _RwDlRasterTarget->stride * (rect->y + y) + (rect->x >> 1);

                memset(pixel, color, rect->w >> 1);
            }
            break;
        }
        case 8:
        {
            RwUInt32 y;
            RwUInt8 color = (RwUInt8)packedColor;

            for (y = 0; y < rect->h; y++)
            {
                RwUInt8* pixel =
                    _RwDlRasterTarget->cpPixels + _RwDlRasterTarget->stride * (rect->y + y) + rect->x;

                memset(pixel, color, rect->w);
            }
            break;
        }
        case 16:
        {
            RwUInt32 y;
            RwUInt16 color = (RwUInt16)packedColor;

            for (y = 0; y < rect->h; y++)
            {
                RwUInt32 x;
                RwUInt16* pixel =
                    (RwUInt16*)(_RwDlRasterTarget->cpPixels +
                                _RwDlRasterTarget->stride * (rect->y + y) + (rect->x << 1));

                for (x = 0; x < rect->w; x++)
                {
                    *pixel++ = color;
                }
            }
            break;
        }
        case 32:
        {
            RwUInt32 y;

            for (y = 0; y < rect->h; y++)
            {
                RwUInt32 x;
                RwUInt32* pixel =
                    (RwUInt32*)(_RwDlRasterTarget->cpPixels +
                                _RwDlRasterTarget->stride * (rect->y + y) + (rect->x << 2));

                for (x = 0; x < rect->w; x++)
                {
                    *pixel++ = packedColor;
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

        if (rasterLocked)
        {
            RwRasterUnlock(_RwDlRasterTarget);
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVRASTERFORMAT));
        return FALSE;
    }
    }

    return TRUE;
}

RwBool _rwDlRasterClearRect(void* unused1, void* rectIn, RwInt32 packedColor)
{
    return _rwDlRasterClearGeneric(_RwDlRasterTarget, (RwRect*)rectIn, packedColor);
}

RwBool _rwDlRasterClear(void* unused1, void* unused2, RwInt32 packedColor)
{
    RwRect rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = _RwDlRasterTarget->width;
    rect.h = _RwDlRasterTarget->height;

    return _rwDlRasterClearGeneric(_RwDlRasterTarget, &rect, packedColor);
}

RwBool _rwDlSetRasterContext(void* unused1, void* rasIn, RwInt32 unused3)
{
    _RwDlRasterTarget = (RwRaster*)rasIn;

    return TRUE;
}
