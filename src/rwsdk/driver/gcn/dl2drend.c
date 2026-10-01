#include <rwsdk/rwcore.h>
#include <dolphin/vi.h>

#include "rwsdk/driver/gcn/dlprivate.h"

static GXPrimitive _rwDlPrimConvTbl[7] = {
    (GXPrimitive)0, /* rwPRIMTYPENAPRIMTYPE */
    GX_LINES, /* rwPRIMTYPELINELIST */
    GX_LINESTRIP, /* rwPRIMTYPEPOLYLINE */
    GX_TRIANGLES, /* rwPRIMTYPETRILIST */
    GX_TRIANGLESTRIP, /* rwPRIMTYPETRISTRIP */
    GX_TRIANGLEFAN, /* rwPRIMTYPETRIFAN */
    GX_POINTS, /* rwPRIMTYPEPOINTLIST */
};

static f32 _rwDlProjectionMatrix[7];

#define DlIm2DVertexPosColor(_vert)                                                                \
    MACRO_START                                                                                    \
    {                                                                                              \
        GXPosition3f32((_vert)->x, (_vert)->y, (_vert)->z);                                        \
        GXColor4u8((_vert)->emissiveColor.red, (_vert)->emissiveColor.green,                       \
                   (_vert)->emissiveColor.blue, (_vert)->emissiveColor.alpha);                     \
    }                                                                                              \
    MACRO_STOP

#define DlIm2DVertexPosColorTex(_vert)                                                             \
    MACRO_START                                                                                    \
    {                                                                                              \
        GXPosition3f32((_vert)->x, (_vert)->y, (_vert)->z);                                        \
        GXColor4u8((_vert)->emissiveColor.red, (_vert)->emissiveColor.green,                       \
                   (_vert)->emissiveColor.blue, (_vert)->emissiveColor.alpha);                     \
        GXTexCoord2f32((_vert)->u, (_vert)->v);                                                    \
    }                                                                                              \
    MACRO_STOP

void _rw2DRenderPrimitiveInit(void)
{
    static f32 projVector[7] = { GX_ORTHOGRAPHIC, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f };
    static f32 posMatrix[3][4] = {
        { 1.0f, 0.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
    };
    RwCamera* camera;
    RwRaster* raster;

    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    GXSetNumTevStages(1);

    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);

    if (_RwDlTexture->raster)
    {
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

        GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        GXSetNumTexGens(1);
        GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

        _rwDlTextureRasterFlush();
    }
    else
    {
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }

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

    camera = (RwCamera*)RWSRCGLOBAL(curCamera);
    raster = RwCameraGetRaster(camera);

    projVector[1] = 2.0f / (f32)_RwDlRenderMode->fbWidth;
    projVector[3] = -2.0f / (f32)_RwDlRenderMode->xfbHeight;

    GXGetProjectionv(_rwDlProjectionMatrix);
    GXSetProjectionv(projVector);

    /* Offset by half a pixel so texels map onto pixel centres */
    posMatrix[0][3] = 0.5f + (f32)raster->nOffsetX;
    posMatrix[1][3] = 0.5f + (f32)raster->nOffsetY;

    GXLoadPosMtxImm(posMatrix, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

static void _rw2DRenderPrimativeTerm(void)
{
    RwCamera* camera = (RwCamera*)RWSRCGLOBAL(curCamera);
    RwRaster* raster = RwCameraGetRaster(camera);

    if (raster != raster->parent)
    {
        /* Restore the sub raster viewport */
        if (!_RwDlFSAA)
        {
            if (_RwDlRenderMode->field_rendering)
            {
                GXSetViewportJitter((f32)raster->nOffsetX, (f32)raster->nOffsetY,
                                    (f32)raster->width, (f32)raster->height, 0.0f, 1.0f,
                                    VIGetNextField() ^ 1);
            }
            else
            {
                GXSetViewport((f32)raster->nOffsetX, (f32)raster->nOffsetY, (f32)raster->width,
                              (f32)raster->height, 0.0f, 1.0f);
            }

            GXSetScissor(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height);
        }
        else
        {
            if (_RwDlRenderMode->field_rendering)
            {
                GXSetViewportJitter((f32)raster->nOffsetX, (f32)raster->nOffsetY,
                                    (f32)raster->width, (f32)raster->height, 0.0f, 1.0f,
                                    VIGetNextField() ^ 1);
            }
            else
            {
                GXSetViewport((f32)raster->nOffsetX, (f32)(raster->nOffsetY << 1),
                              (f32)raster->width, (f32)(raster->height << 1), 0.0f, 1.0f);
            }

            if (_RwDlFSAATop)
            {
                if (((raster->nOffsetY + raster->height) << 1) <= _RwDlHalfHeight + 2)
                {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, _RwDlRenderMode->fbWidth,
                                 raster->height << 1);
                }
                else if ((raster->nOffsetY << 1) > _RwDlHalfHeight + 2)
                {
                    GXSetScissor(0, 0, _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
                }
                else
                {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, _RwDlRenderMode->fbWidth,
                                 _RwDlHalfHeight + 2);
                }

                GXSetScissorBoxOffset(0, 0);
            }
            else
            {
                if ((raster->nOffsetY << 1) >= _RwDlHalfHeight - 2)
                {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, _RwDlRenderMode->fbWidth,
                                 raster->height << 1);
                }
                else if (((raster->nOffsetY + raster->height) << 1) < _RwDlHalfHeight + 2)
                {
                    GXSetScissor(0, _RwDlHalfHeight - 2, _RwDlRenderMode->fbWidth,
                                 _RwDlHalfHeight + 2);
                }
                else
                {
                    GXSetScissor(raster->nOffsetX, _RwDlHalfHeight - 2, _RwDlRenderMode->fbWidth,
                                 raster->height << 1);
                }

                GXSetScissorBoxOffset(0, _RwDlHalfHeight - 2);
            }
        }
    }

    GXSetProjectionv(_rwDlProjectionMatrix);
}

RwBool _rwDlIm2DRenderTriangle(RwIm2DVertex* verts, RwInt32 numVerts, RwInt32 vert1, RwInt32 vert2,
                               RwInt32 vert3)
{
    RwIm2DVertex* dlVert0 = &verts[vert1];
    RwIm2DVertex* dlVert1 = &verts[vert2];
    RwIm2DVertex* dlVert2 = &verts[vert3];

    _rw2DRenderPrimitiveInit();

    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);

    if (_RwDlTexture->raster)
    {
        DlIm2DVertexPosColorTex(dlVert0);
        DlIm2DVertexPosColorTex(dlVert1);
        DlIm2DVertexPosColorTex(dlVert2);
    }
    else
    {
        DlIm2DVertexPosColor(dlVert0);
        DlIm2DVertexPosColor(dlVert1);
        DlIm2DVertexPosColor(dlVert2);
    }

    GXEnd();

    _rw2DRenderPrimativeTerm();

    return TRUE;
}

RwBool _rwDlIm2DRenderLine(RwIm2DVertex* verts, RwInt32 numVerts, RwInt32 vert1, RwInt32 vert2)
{
    RwIm2DVertex* dlVert0 = &verts[vert1];
    RwIm2DVertex* dlVert1 = &verts[vert2];

    _rw2DRenderPrimitiveInit();

    GXBegin(GX_LINES, GX_VTXFMT0, 2);

    if (_RwDlTexture->raster)
    {
        DlIm2DVertexPosColorTex(dlVert0);
        DlIm2DVertexPosColorTex(dlVert1);
    }
    else
    {
        DlIm2DVertexPosColor(dlVert0);
        DlIm2DVertexPosColor(dlVert1);
    }

    GXEnd();

    _rw2DRenderPrimativeTerm();

    return TRUE;
}

RwBool _rwDlIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* verts, RwInt32 numVertices)
{
    _rw2DRenderPrimitiveInit();

    GXBegin(_rwDlPrimConvTbl[primType], GX_VTXFMT0, (u16)numVertices);

    switch (primType)
    {
    case rwPRIMTYPEPOLYLINE:
    case rwPRIMTYPETRISTRIP:
    case rwPRIMTYPETRIFAN:
    {
        if (_RwDlTexture->raster)
        {
            while (numVertices--)
            {
                DlIm2DVertexPosColorTex(verts);
                verts++;
            }
        }
        else
        {
            while (numVertices--)
            {
                DlIm2DVertexPosColor(verts);
                verts++;
            }
        }
        break;
    }
    case rwPRIMTYPELINELIST:
    {
        RwInt32 numLines = numVertices >> 1;

        if (_RwDlTexture->raster)
        {
            while (numLines--)
            {
                DlIm2DVertexPosColorTex(&verts[0]);
                DlIm2DVertexPosColorTex(&verts[1]);
                verts += 2;
            }
        }
        else
        {
            while (numLines--)
            {
                DlIm2DVertexPosColor(&verts[0]);
                DlIm2DVertexPosColor(&verts[1]);
                verts += 2;
            }
        }
        break;
    }
    case rwPRIMTYPETRILIST:
    {
        RwInt32 numTriangles = numVertices / 3;

        if (_RwDlTexture->raster)
        {
            while (numTriangles--)
            {
                DlIm2DVertexPosColorTex(&verts[0]);
                DlIm2DVertexPosColorTex(&verts[1]);
                DlIm2DVertexPosColorTex(&verts[2]);
                verts += 3;
            }
        }
        else
        {
            while (numTriangles--)
            {
                DlIm2DVertexPosColor(&verts[0]);
                DlIm2DVertexPosColor(&verts[1]);
                DlIm2DVertexPosColor(&verts[2]);
                verts += 3;
            }
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVPRIMTYPE));
        break;
    }
    }

    GXEnd();

    _rw2DRenderPrimativeTerm();

    return TRUE;
}

RwBool _rwDlIm2DRenderIndexedPrimitive(RwPrimitiveType primType, RwIm2DVertex* verts,
                                       RwInt32 numVertices, RwImVertexIndex* indices,
                                       RwInt32 numIndices)
{
    _rw2DRenderPrimitiveInit();

    GXBegin(_rwDlPrimConvTbl[primType], GX_VTXFMT0, (u16)numIndices);

    switch (primType)
    {
    case rwPRIMTYPEPOLYLINE:
    case rwPRIMTYPETRISTRIP:
    case rwPRIMTYPETRIFAN:
    {
        if (_RwDlTexture->raster)
        {
            while (numIndices--)
            {
                RwIm2DVertex* curVert = &verts[*indices++];

                DlIm2DVertexPosColorTex(curVert);
            }
        }
        else
        {
            while (numIndices--)
            {
                RwIm2DVertex* curVert = &verts[*indices++];

                DlIm2DVertexPosColor(curVert);
            }
        }
        break;
    }
    case rwPRIMTYPELINELIST:
    {
        RwInt32 numLines = numIndices >> 1;

        if (_RwDlTexture->raster)
        {
            while (numLines--)
            {
                RwIm2DVertex* curVert;

                curVert = &verts[*indices++];
                DlIm2DVertexPosColorTex(curVert);
                curVert = &verts[*indices++];
                DlIm2DVertexPosColorTex(curVert);
            }
        }
        else
        {
            while (numLines--)
            {
                RwIm2DVertex* curVert;

                curVert = &verts[*indices++];
                DlIm2DVertexPosColor(curVert);
                curVert = &verts[*indices++];
                DlIm2DVertexPosColor(curVert);
            }
        }
        break;
    }
    case rwPRIMTYPETRILIST:
    {
        RwInt32 numTriangles = numIndices / 3;

        if (_RwDlTexture->raster)
        {
            while (numTriangles--)
            {
                RwIm2DVertex* curVert;

                curVert = &verts[*indices++];
                DlIm2DVertexPosColorTex(curVert);
                curVert = &verts[*indices++];
                DlIm2DVertexPosColorTex(curVert);
                curVert = &verts[*indices++];
                DlIm2DVertexPosColorTex(curVert);
            }
        }
        else
        {
            while (numTriangles--)
            {
                RwIm2DVertex* curVert;

                curVert = &verts[*indices++];
                DlIm2DVertexPosColor(curVert);
                curVert = &verts[*indices++];
                DlIm2DVertexPosColor(curVert);
                curVert = &verts[*indices++];
                DlIm2DVertexPosColor(curVert);
            }
        }
        break;
    }
    default:
    {
        RWERROR((E_RW_INVPRIMTYPE));
        break;
    }
    }

    GXEnd();

    _rw2DRenderPrimativeTerm();

    return TRUE;
}
