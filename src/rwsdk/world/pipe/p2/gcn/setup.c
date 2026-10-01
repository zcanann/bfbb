#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

/* GX_TG_TEX0 as numbered by the Dolphin SDK revision RenderWare was built against */
#define rwDLTEXGENSRCTEX0 ((GXTexGenSrc)12)

static const GXColor OpaqueWhite = { 255, 255, 255, 255 };
static const GXColor OpaqueBlack = { 0, 0, 0, 255 };

/* Lit, textured, prelit, modulated by the material color */
static void MatFunc1(RwRGBAReal* ambientColor, RwRGBA* materialColor, RwReal ambientCoef)
{
    GXColor ambMatColor;

    ambMatColor.r = (RwUInt8)((RwReal)materialColor->red * (ambientColor->red * ambientCoef));
    ambMatColor.g = (RwUInt8)((RwReal)materialColor->green * (ambientColor->green * ambientCoef));
    ambMatColor.b = (RwUInt8)((RwReal)materialColor->blue * (ambientColor->blue * ambientCoef));
    ambMatColor.a = 0;

    GXSetTevColor(GX_TEVREG0, *(GXColor*)materialColor);
    GXSetTevColor(GX_TEVREG1, ambMatColor);
}

/* Lit, textured, prelit, not modulated */
static void MatFunc2(RwRGBAReal* ambientColor, RwRGBA* materialColor, RwReal ambientCoef)
{
    GXColor ambColor;
    RwReal ambCoef;

    ambCoef = 255.0f * ambientCoef;

    ambColor.r = (RwUInt8)(ambientColor->red * ambCoef);
    ambColor.g = (RwUInt8)(ambientColor->green * ambCoef);
    ambColor.b = (RwUInt8)(ambientColor->blue * ambCoef);
    ambColor.a = 0;

    GXSetTevColor(GX_TEVREG1, ambColor);
}

/* Lit, not prelit, not modulated */
static void MatFunc3(RwRGBAReal* ambientColor, RwRGBA* materialColor, RwReal ambientCoef)
{
    GXColor ambColor;
    RwReal ambCoef;

    ambCoef = 255.0f * ambientCoef;

    ambColor.r = (RwUInt8)(ambientColor->red * ambCoef);
    ambColor.g = (RwUInt8)(ambientColor->green * ambCoef);
    ambColor.b = (RwUInt8)(ambientColor->blue * ambCoef);
    ambColor.a = 0;

    GXSetChanAmbColor(GX_COLOR0, ambColor);
}

/* Lit, not prelit, modulated by the material color */
static void MatFunc4(RwRGBAReal* ambientColor, RwRGBA* materialColor, RwReal ambientCoef)
{
    GXColor ambColor;
    RwReal ambCoef;

    ambCoef = 255.0f * ambientCoef;

    ambColor.r = (RwUInt8)(ambientColor->red * ambCoef);
    ambColor.g = (RwUInt8)(ambientColor->green * ambCoef);
    ambColor.b = (RwUInt8)(ambientColor->blue * ambCoef);
    ambColor.a = 0;

    GXSetChanMatColor(GX_COLOR0A0, *(GXColor*)materialColor);
    GXSetChanAmbColor(GX_COLOR0, ambColor);
}

/* Prelit, modulated by the material color */
static void MatFunc5(RwRGBAReal* ambientColor, RwRGBA* materialColor, RwReal ambientCoef)
{
    GXSetChanMatColor(GX_COLOR0A0, *(GXColor*)materialColor);
}

/* Unlit, not prelit, not modulated */
static void MatFunc6(RwRGBAReal* ambientColor, RwRGBA* materialColor, RwReal ambientCoef)
{
    GXColor ambColor;
    RwReal ambCoef;

    ambCoef = 255.0f * ambientCoef;

    ambColor.r = (RwUInt8)(ambientColor->red * ambCoef);
    ambColor.g = (RwUInt8)(ambientColor->green * ambCoef);
    ambColor.b = (RwUInt8)(ambientColor->blue * ambCoef);
    ambColor.a = 0;

    GXSetChanMatColor(GX_COLOR0, ambColor);
}

RwDlMatFunc _rwDlObjectRenderSetup(RwUInt32 flags, RwUInt32 lightMask, RwBool ambient,
                                   RwBool prelightAlpha)
{
    RwUInt32 textured;
    RwDlMatFunc matFunc;
    RwUInt8 chanLightingColEnable;
    RwUInt8 chanLightingAlpEnable;
    GXColorSrc chanMatColSrc;
    GXColorSrc chanMatAlpSrc;
    GXColorSrc chanAmbColSrc;
    GXColorSrc chanAmbAlpSrc;
    RwUInt8 numTevStages;
    RwUInt32 prelit;

    textured = flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2);
    matFunc = NULL;

    if (textured)
    {
        prelit = flags & rpGEOMETRYPRELIT;

        if (prelit && ambient == TRUE)
        {
            if (lightMask)
            {
                if (flags & rpGEOMETRYMODULATEMATERIALCOLOR)
                {
                    matFunc = MatFunc1;
                }
                else
                {
                    GXSetTevColor(GX_TEVREG0, OpaqueWhite);
                    matFunc = MatFunc2;
                }

                chanMatColSrc = GX_SRC_REG;
                chanMatAlpSrc = GX_SRC_REG;
                GXSetChanMatColor(GX_COLOR0A0, OpaqueWhite);
                chanLightingColEnable = TRUE;
                chanAmbColSrc = GX_SRC_VTX;

                if (prelightAlpha == TRUE)
                {
                    chanAmbAlpSrc = GX_SRC_VTX;
                    chanLightingAlpEnable = TRUE;
                }
                else
                {
                    chanAmbAlpSrc = GX_SRC_REG;
                    GXSetChanAmbColor(GX_ALPHA0, OpaqueBlack);
                    chanLightingAlpEnable = FALSE;
                }
            }
            else
            {
                if (flags & rpGEOMETRYMODULATEMATERIALCOLOR)
                {
                    matFunc = MatFunc1;
                }
                else
                {
                    GXSetTevColor(GX_TEVREG0, OpaqueWhite);
                    matFunc = MatFunc2;
                }

                chanAmbColSrc = GX_SRC_REG;
                chanAmbAlpSrc = GX_SRC_REG;
                chanMatColSrc = GX_SRC_VTX;

                if (prelightAlpha == TRUE)
                {
                    chanMatAlpSrc = GX_SRC_VTX;
                }
                else
                {
                    chanMatAlpSrc = GX_SRC_REG;
                    GXSetChanMatColor(GX_ALPHA0, OpaqueBlack);
                }

                chanLightingColEnable = FALSE;
                chanLightingAlpEnable = FALSE;
            }

            /* Stage 0: rasterized color * ambient/material + lit, stage 1: * texture */
            numTevStages = 2;
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_C0, GX_CC_C1);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_RASA, GX_CA_A0, GX_CA_A1);
        }
        else
        {
            if (lightMask)
            {
                if (flags & rpGEOMETRYMODULATEMATERIALCOLOR)
                {
                    chanMatColSrc = GX_SRC_REG;
                    chanMatAlpSrc = GX_SRC_REG;
                    chanLightingColEnable = TRUE;

                    if (prelit)
                    {
                        chanAmbColSrc = GX_SRC_VTX;

                        if (prelightAlpha == TRUE)
                        {
                            chanAmbAlpSrc = GX_SRC_VTX;
                            chanLightingAlpEnable = TRUE;
                        }
                        else
                        {
                            chanAmbAlpSrc = GX_SRC_REG;
                            GXSetChanAmbColor(GX_ALPHA0, OpaqueBlack);
                            chanLightingAlpEnable = FALSE;
                        }

                        matFunc = MatFunc5;
                    }
                    else
                    {
                        chanAmbColSrc = GX_SRC_REG;
                        chanAmbAlpSrc = GX_SRC_REG;
                        chanLightingAlpEnable = FALSE;
                        GXSetChanAmbColor(GX_ALPHA0, OpaqueBlack);
                        matFunc = MatFunc4;
                    }
                }
                else
                {
                    chanMatColSrc = GX_SRC_REG;
                    chanMatAlpSrc = GX_SRC_REG;
                    GXSetChanMatColor(GX_COLOR0A0, OpaqueWhite);
                    chanLightingColEnable = TRUE;

                    if (prelit)
                    {
                        chanAmbColSrc = GX_SRC_VTX;

                        if (prelightAlpha == TRUE)
                        {
                            chanAmbAlpSrc = GX_SRC_VTX;
                            chanLightingAlpEnable = TRUE;
                        }
                        else
                        {
                            chanAmbAlpSrc = GX_SRC_REG;
                            chanLightingAlpEnable = FALSE;
                        }
                    }
                    else
                    {
                        matFunc = MatFunc3;
                        chanAmbColSrc = GX_SRC_REG;
                        chanAmbAlpSrc = GX_SRC_REG;
                        chanLightingAlpEnable = FALSE;
                    }
                }
            }
            else
            {
                if (flags & rpGEOMETRYMODULATEMATERIALCOLOR)
                {
                    chanMatColSrc = GX_SRC_REG;
                    chanMatAlpSrc = GX_SRC_REG;
                    chanLightingColEnable = TRUE;

                    if (prelit)
                    {
                        chanAmbColSrc = GX_SRC_VTX;

                        if (prelightAlpha == TRUE)
                        {
                            chanAmbAlpSrc = GX_SRC_VTX;
                            chanLightingAlpEnable = TRUE;
                        }
                        else
                        {
                            chanAmbAlpSrc = GX_SRC_REG;
                            GXSetChanAmbColor(GX_ALPHA0, OpaqueBlack);
                            chanLightingAlpEnable = FALSE;
                        }

                        matFunc = MatFunc5;
                    }
                    else
                    {
                        chanAmbColSrc = GX_SRC_REG;
                        chanAmbAlpSrc = GX_SRC_REG;
                        chanLightingAlpEnable = FALSE;
                        GXSetChanAmbColor(GX_ALPHA0, OpaqueBlack);
                        matFunc = MatFunc4;
                    }
                }
                else
                {
                    if (prelit)
                    {
                        chanMatColSrc = GX_SRC_VTX;

                        if (prelightAlpha == TRUE)
                        {
                            chanMatAlpSrc = GX_SRC_VTX;
                        }
                        else
                        {
                            chanMatAlpSrc = GX_SRC_REG;
                            GXSetChanMatColor(GX_ALPHA0, OpaqueBlack);
                        }
                    }
                    else
                    {
                        chanMatColSrc = GX_SRC_REG;
                        chanMatAlpSrc = GX_SRC_REG;
                        GXSetChanMatColor(GX_ALPHA0, OpaqueBlack);
                        matFunc = MatFunc6;
                    }

                    chanLightingColEnable = FALSE;
                    chanLightingAlpEnable = FALSE;
                    chanAmbColSrc = GX_SRC_REG;
                    chanAmbAlpSrc = GX_SRC_REG;
                }
            }

            /* Single stage: rasterized color * texture */
            numTevStages = 1;
            GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_TEXC, GX_CC_ZERO);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_RASA, GX_CA_TEXA, GX_CA_ZERO);
        }
    }
    else
    {
        if (lightMask)
        {
            chanMatColSrc = GX_SRC_REG;
            chanMatAlpSrc = GX_SRC_REG;
            GXSetChanMatColor(GX_COLOR0A0, OpaqueWhite);
            chanLightingColEnable = TRUE;

            if (flags & rpGEOMETRYPRELIT)
            {
                chanAmbColSrc = GX_SRC_VTX;

                if (prelightAlpha == TRUE)
                {
                    chanAmbAlpSrc = GX_SRC_VTX;
                    chanLightingAlpEnable = TRUE;
                }
                else
                {
                    chanAmbAlpSrc = GX_SRC_REG;
                    chanLightingAlpEnable = FALSE;
                }
            }
            else
            {
                chanAmbColSrc = GX_SRC_REG;
                chanAmbAlpSrc = GX_SRC_REG;
                chanLightingAlpEnable = FALSE;
                GXSetChanAmbColor(GX_COLOR0A0, OpaqueBlack);
            }
        }
        else
        {
            if (flags & rpGEOMETRYPRELIT)
            {
                chanMatColSrc = GX_SRC_VTX;

                if (prelightAlpha == TRUE)
                {
                    chanMatAlpSrc = GX_SRC_VTX;
                }
                else
                {
                    chanMatAlpSrc = GX_SRC_REG;
                    GXSetChanMatColor(GX_ALPHA0, OpaqueBlack);
                }
            }
            else
            {
                chanMatColSrc = GX_SRC_REG;
                chanMatAlpSrc = GX_SRC_REG;
                GXSetChanMatColor(GX_COLOR0A0, OpaqueBlack);
            }

            chanAmbColSrc = GX_SRC_REG;
            chanAmbAlpSrc = GX_SRC_REG;
            chanLightingColEnable = FALSE;
            chanLightingAlpEnable = FALSE;
        }

        numTevStages = 1;

        if (flags & rpGEOMETRYMODULATEMATERIALCOLOR)
        {
            matFunc = MatFunc1;
        }
        else
        {
            GXSetTevColor(GX_TEVREG0, OpaqueWhite);
            matFunc = MatFunc2;
        }

        GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_RASC, GX_CC_C0, GX_CC_C1);
        GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_RASA, GX_CA_A0, GX_CA_A1);
    }

    GXSetNumTevStages(numTevStages);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

    if (numTevStages > 1)
    {
        GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_CPREV, GX_CC_TEXC, GX_CC_ZERO);
        GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_APREV, GX_CA_TEXA, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR_NULL);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, rwDLTEXGENSRCTEX0, GX_IDENTITY, GX_FALSE,
                          GX_PTIDENTITY);
    }
    else if (textured)
    {
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, rwDLTEXGENSRCTEX0, GX_IDENTITY, GX_FALSE,
                          GX_PTIDENTITY);
    }
    else
    {
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    }

    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0, chanLightingColEnable, chanAmbColSrc, chanMatColSrc, lightMask,
                  GX_DF_CLAMP, GX_AF_SPOT);
    GXSetChanCtrl(GX_ALPHA0, chanLightingAlpEnable, chanAmbAlpSrc, chanMatAlpSrc, GX_LIGHT_NULL,
                  GX_DF_NONE, GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_ALPHA1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                  GX_AF_NONE);

    return matFunc;
}
