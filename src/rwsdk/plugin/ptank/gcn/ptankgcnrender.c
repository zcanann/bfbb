#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpptank.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

/* Color source selection, stored in the platform flags (see ptankgcncallbacks.c) */
#define rpPTANKGCNCOLORPP 0x00010000
#define rpPTANKGCNCOLORPPV 0x00020000
#define rpPTANKGCNCOLORCSV 0x00040000
#define rpPTANKGCNCOLORCS 0x00080000

#define RwRGBAFromRwRGBARealMacro(_o, _i)                                                          \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_o)->red = (RwUInt8)(((RwReal)255.0) * (_i)->red + ((RwReal)0.5));                        \
        (_o)->green = (RwUInt8)(((RwReal)255.0) * (_i)->green + ((RwReal)0.5));                    \
        (_o)->blue = (RwUInt8)(((RwReal)255.0) * (_i)->blue + ((RwReal)0.5));                      \
        (_o)->alpha = (RwUInt8)(((RwReal)255.0) * (_i)->alpha + ((RwReal)0.5));                    \
    }                                                                                              \
    MACRO_STOP

typedef void (*PTankGameCubeVtxRenderFunc)(RwInt32 numPrims, RwUInt32 flags, RpPTankData* data);

static const GXColor OpaqueWhite = { 255, 255, 255, 255 };
static const GXColor OpaqueBlack = { 0, 0, 0, 255 };

static void PTankGameCubeVtxFmtSetup(RpGameCubeVtxFmt* fmt, RxGameCubePipeData* pipeData)
{
    GXClearVtxDesc();

    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, (GXCompType)fmt->pos, fmt->posFrac);

    if (pipeData->flags & rpGEOMETRYNORMALS)
    {
        GXSetVtxDesc(GX_VA_NRM, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, (GXCompType)fmt->norm, 0);
    }

    if (pipeData->flags & rpGEOMETRYPRELIT)
    {
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, (GXCompType)fmt->preLight, 0);
    }

    if (pipeData->flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, (GXCompType)fmt->texCoord[0],
                        fmt->texCoordFrac[0]);
    }
}

static void PTankGameCubeVtxRender(RpAtomic* atomic, RxGameCubePipeData* pipeData)
{
    RpPTankAtomicExtPrv* pTankPrvData;
    RpPTankData* pTankData;
    RpGeometry* geometry;
    PTankGameCubeVtxRenderFunc vtxRender;
    RwUInt16 actPCount;
    RwUInt32 flags;
    register RwUInt32 posGQR;
    register RwUInt32 normGQR;
    RpGameCubeVtxFmt* vtxFmt;

    pTankPrvData = RPATOMICPTANKPLUGINDATA(atomic);
    pTankData = &pTankPrvData->publicData;
    geometry = atomic->geometry;
    vtxRender = (PTankGameCubeVtxRenderFunc)pTankPrvData->insPosCB;
    actPCount = pTankPrvData->actPCount;
    flags = pTankPrvData->platFlags;

    GXBegin(GX_QUADS, GX_VTXFMT0, (u16)(actPCount << 2));

    posGQR = 0;
    normGQR = 0;

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt != NULL)
    {
        RwUInt8 vtxFmtTypeConvTable[5] = { 4, 6, 5, 7, 0 };
        RwUInt8 vtxFmtNormConvTable[5] = { 0, 6, 0, 14, 0 };

        posGQR = vtxFmtTypeConvTable[vtxFmt->pos] | (vtxFmt->posFrac << 8);
        posGQR |= posGQR << 16;

        normGQR = vtxFmtTypeConvTable[vtxFmt->norm] | (vtxFmtNormConvTable[vtxFmt->norm] << 8);
        normGQR |= normGQR << 16;
    }

    asm { mtspr GQR6, posGQR }
    asm { mtspr GQR7, normGQR }

    vtxRender(actPCount, flags, pTankData);
}

void* _rxPTankGameCubeRenderCallBack(void* object, RxGameCubePipeData* pipeData)
{
    RwRGBA ambient;
    GXColorSrc chanAmbSrc;
    GXColorSrc chanMatSrc;
    GXBool lightingEnable;
    RwUInt32 flags;
    RpAtomic* atomic;
    RpMesh* mesh;
    RpPTankAtomicExtPrv* pTankPrvData;
    RpGameCubeVtxFmt* fmt;
    RwTexture* texture;
    RwGameCubeRasterExtension* rasExt;

    atomic = (RpAtomic*)object;
    pTankPrvData = RPATOMICPTANKPLUGINDATA(atomic);
    flags = pTankPrvData->platFlags;
    fmt = GEOMVTXFMT(atomic->geometry);

    PTankGameCubeVtxFmtSetup(fmt, pipeData);

    _rwDlTransformSetup((RwMatrix*)NULL, (pipeData->flags & rpGEOMETRYNORMALS) ? TRUE : FALSE);

    if (pipeData->numLights > 0)
    {
        lightingEnable = GX_TRUE;

        if (flags & (rpPTANKGCNCOLORPP | rpPTANKGCNCOLORPPV | rpPTANKGCNCOLORCSV))
        {
            chanAmbSrc = GX_SRC_VTX;
        }
        else
        {
            chanAmbSrc = GX_SRC_REG;

            if (flags & rpPTANKGCNCOLORCS)
            {
                GXSetChanAmbColor(GX_COLOR0A0, *(GXColor*)&pTankPrvData->publicData.cColor);
            }
            else if (pipeData->ambientLight)
            {
                RwRGBAFromRwRGBARealMacro(&ambient, &pipeData->ambientLightColor);
                GXSetChanAmbColor(GX_COLOR0A0, *(GXColor*)&ambient);
            }
            else
            {
                GXSetChanAmbColor(GX_COLOR0A0, OpaqueBlack);
            }
        }

        chanMatSrc = GX_SRC_REG;
        GXSetChanMatColor(GX_COLOR0A0, OpaqueWhite);
    }
    else
    {
        lightingEnable = GX_FALSE;
        chanAmbSrc = GX_SRC_REG;

        if (flags & rpPTANKGCNCOLORCS)
        {
            chanMatSrc = GX_SRC_REG;
            GXSetChanMatColor(GX_COLOR0A0, *(GXColor*)&pTankPrvData->publicData.cColor);
        }
        else if (flags & (rpPTANKGCNCOLORPP | rpPTANKGCNCOLORPPV | rpPTANKGCNCOLORCSV))
        {
            chanMatSrc = GX_SRC_VTX;
        }
        else
        {
            chanMatSrc = GX_SRC_REG;

            if (pipeData->ambientLight)
            {
                RwRGBAFromRwRGBARealMacro(&ambient, &pipeData->ambientLightColor);
                GXSetChanMatColor(GX_COLOR0A0, *(GXColor*)&ambient);
            }
            else
            {
                GXSetChanMatColor(GX_COLOR0A0, OpaqueBlack);
            }
        }
    }

    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0, lightingEnable, chanAmbSrc, chanMatSrc, pipeData->lightMask,
                  GX_DF_CLAMP, GX_AF_SPOT);
    GXSetChanCtrl(GX_ALPHA0, lightingEnable, chanAmbSrc, chanMatSrc, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);

    GXSetNumTevStages(1);

    if (pipeData->flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        GXSetNumTexGens(1);
        GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    }
    else
    {
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }

    mesh = (RpMesh*)(pipeData->meshHeader + 1);

    if (pipeData->flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        texture = mesh->material->texture;
        _rwDlTextureSet(texture, 0);

        if (texture != NULL && texture->raster != NULL)
        {
            rasExt = RASTEREXTFROMRASTER(RwRasterGetParent(texture->raster));
            _rwDlRenderStateSetZCompLoc((rasExt->flags & 1) ^ 1);
        }
    }

    PTankGameCubeVtxRender(atomic, pipeData);

    return object;
}
