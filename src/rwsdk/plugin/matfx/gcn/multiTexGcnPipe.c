#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"
#include "rwsdk/plugin/matfx/gcn/mtgcnprivate.h"

#define rwID_GCNMULTITEXPLUGIN MAKECHUNKID(rwVENDORID_CRITERIONTK, 0x29)

/* RpGameCubeMTConfig flags */
#define rpGAMECUBEMTCONFIGREGS 0x01
#define rpGAMECUBEMTCONFIGKREGS 0x02
#define rpGAMECUBEMTCONFIGBLEND 0x04
#define rpGAMECUBEMTCONFIGZCOMPAFTERTEX 0x08
#define rpGAMECUBEMTCONFIGCHANNELCOLOR 0x10
#define rpGAMECUBEMTCONFIGENVMTX 0x20

/* per-element flags */
#define rpGAMECUBEMTDISABLE 0x01
#define rpGAMECUBEMTTEXMTX2x4 0x01
#define rpGAMECUBEMTTEXMTXTRANSLATE 0x02
#define rpGAMECUBEMTTEXGENNORMALIZE 0x02
#define rpGAMECUBEMTTEXGENSCALEMANUALLY 0x04

enum _TevRasSrc
{
    NATevRasSrc = 0,
    TEVRAS_NORM = 1,
    TEVRAS_CH0A0 = 2
};
typedef enum _TevRasSrc _TevRasSrc;

typedef RpMaterial* (*RpGameCubeMTCallBack)(RpMaterial* material, void* object,
                                            RxGameCubePipeData* pipeData, void* data);

typedef struct rpMultiTextureGameCubeExt rpMultiTextureGameCubeExt;
struct rpMultiTextureGameCubeExt
{
    RpGameCubeMTCallBack preRenderCallBack;
    RpGameCubeMTCallBack postRenderCallBack;
    void* callBackData;
};

typedef struct rpGameCubeMTGlobals rpGameCubeMTGlobals;
struct rpGameCubeMTGlobals
{
    RwUInt32 numFrames;
    RwFrame** frames;
};

#define RPGAMECUBEMTGLOBAL(var)                                                                    \
    (RWPLUGINOFFSET(rpGameCubeMTGlobals, RwEngineInstance, _rpGameCubeMTEngineOffset)->var)

extern RwMatrix _RwDlInvCamLTM;

RwInt32 _rpGameCubeMTEngineOffset;

static void* GameCubeMTOpen(void* object, RwInt32 offset, RwInt32 size)
{
    memset(RWPLUGINOFFSET(rpGameCubeMTGlobals, RwEngineInstance, _rpGameCubeMTEngineOffset), 0,
           sizeof(rpGameCubeMTGlobals));

    return object;
}

static void* GameCubeMTClose(void* object, RwInt32 offset, RwInt32 size)
{
    return object;
}

RwBool _rpGameCubeMTPipeDataQueryNBTs(RxGameCubePipeData* pipeData)
{
    RpMeshHeader* meshHeader;
    RpMesh* mesh;
    RwInt32 i;
    RpMultiTexture* multiTexture;
    RpGameCubeMTConfig* config;
    RwInt32 j;
    RpGameCubeTexGen* texGen;

    meshHeader = pipeData->meshHeader;
    mesh = (RpMesh*)(meshHeader + 1);

    for (i = 0; i < meshHeader->numMeshes; i++, mesh++)
    {
        multiTexture = RpMaterialGetMultiTexture(mesh->material, rwID_GAMECUBE);
        if (multiTexture != NULL && multiTexture->effect != NULL &&
            multiTexture->effect->platformID == rwID_GAMECUBE)
        {
            config = RpGameCubeMTEffectGetConfig(multiTexture->effect);

            for (j = 0; j < config->numTexGens; j++)
            {
                texGen = &config->texGens[j];

                if (texGen->srcParam == GX_TG_BINRM || texGen->srcParam == GX_TG_TANGENT ||
                    (texGen->func >= GX_TG_BUMP0 && texGen->func <= GX_TG_BUMP7))
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static RwMatrix* GetTexFrameMatrix(RpGameCubeTexFrameID refFrame, RwMatrix* objectLTM,
                                   RwMatrix* scratch)
{
    RwMatrix* matrix;
    RwUInt32 index;
    RwUInt32 numFrames;
    RwFrame** frames;
    RwMatrix invLTM;

    if (refFrame >= rpGAMECUBETEXFRAME_MISC0)
    {
        index = refFrame - rpGAMECUBETEXFRAME_MISC0;
        numFrames = RPGAMECUBEMTGLOBAL(numFrames);
        frames = RPGAMECUBEMTGLOBAL(frames);

        if (index < numFrames && frames != NULL && frames[index] != NULL)
        {
            if (objectLTM != NULL)
            {
                RwMatrixInvert(&invLTM, RwFrameGetLTM(frames[index]));
                RwMatrixMultiply(scratch, objectLTM, &invLTM);
            }
            else
            {
                RwMatrixInvert(scratch, RwFrameGetLTM(frames[index]));
            }

            return scratch;
        }

        if (objectLTM != NULL)
        {
            return objectLTM;
        }

        matrix = scratch;
        RwMatrixSetIdentity(matrix);

        return matrix;
    }
    else if (refFrame == rpGAMECUBETEXFRAME_CAMERA)
    {
        if (objectLTM != NULL)
        {
            RwMatrixMultiply(scratch, objectLTM, &_RwDlInvCamLTM);
            matrix = scratch;
        }
        else
        {
            matrix = &_RwDlInvCamLTM;
        }

        return matrix;
    }
    else if (refFrame == rpGAMECUBETEXFRAME_WORLD)
    {
        if (objectLTM != NULL)
        {
            return objectLTM;
        }

        return NULL;
    }

    return NULL;
}

RpMTEffect* _rpGameCubeMTEffectSend(RpMTEffect* effect, RwUInt32 numTextures, RwUInt8* coordMap,
                                    RwMatrix* objectLTM)
{
    RwInt32 i;
    _TevRasSrc rasSrc;
    RwInt8 texGenTexture[8];
    RpGameCubeMTConfig* config;

    config = RpGameCubeMTEffectGetConfig(effect);

    if (config->flags & rpGAMECUBEMTCONFIGREGS)
    {
        GXSetTevColorS10(GX_TEVREG0, *(GXColorS10*)&config->reg[0]);
        GXSetTevColorS10(GX_TEVREG1, *(GXColorS10*)&config->reg[1]);
        GXSetTevColorS10(GX_TEVREG2, *(GXColorS10*)&config->reg[2]);
        GXSetTevColorS10(GX_TEVPREV, *(GXColorS10*)&config->reg[3]);
    }

    if (config->flags & rpGAMECUBEMTCONFIGKREGS)
    {
        GXSetTevKColor(GX_KCOLOR0, *(GXColor*)&config->kreg[0]);
        GXSetTevKColor(GX_KCOLOR1, *(GXColor*)&config->kreg[1]);
        GXSetTevKColor(GX_KCOLOR2, *(GXColor*)&config->kreg[2]);
        GXSetTevKColor(GX_KCOLOR3, *(GXColor*)&config->kreg[3]);
    }

    if (config->flags & rpGAMECUBEMTCONFIGENVMTX)
    {
        RwMatrix tmpMatrix;
        RwMatrix* matrix;
        RwReal texMtx[3][4];

        if (objectLTM != NULL)
        {
            matrix = RwMatrixMultiply(&tmpMatrix, objectLTM, &_RwDlInvCamLTM);
        }
        else
        {
            matrix = &_RwDlInvCamLTM;
        }

        texMtx[0][0] = -0.5f * matrix->right.x;
        texMtx[0][1] = -0.5f * matrix->up.x;
        texMtx[0][2] = -0.5f * matrix->at.x;
        texMtx[0][3] = 0.5f;
        texMtx[1][0] = -0.5f * matrix->right.y;
        texMtx[1][1] = -0.5f * matrix->up.y;
        texMtx[1][2] = -0.5f * matrix->at.y;
        texMtx[1][3] = 0.5f;

        GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);
    }

    for (i = 0; i < config->numTexMtx; i++)
    {
        RpGameCubeTexMtx* texMtx;
        RwMatrix temp;
        RwMatrix* matrix;
        RwReal mtx[3][4];
        RwReal mtx2[3][4];

        texMtx = &config->texMtx[i];

        matrix = GetTexFrameMatrix(texMtx->refFrame, objectLTM, &temp);
        if (matrix != NULL)
        {
            mtx[0][0] = matrix->right.x;
            mtx[0][1] = matrix->up.x;
            mtx[0][2] = matrix->at.x;
            mtx[1][0] = matrix->right.y;
            mtx[1][1] = matrix->up.y;
            mtx[1][2] = matrix->at.y;
            mtx[2][0] = matrix->right.z;
            mtx[2][1] = matrix->up.z;
            mtx[2][2] = matrix->at.z;

            if (texMtx->flags & rpGAMECUBEMTTEXMTXTRANSLATE)
            {
                mtx[0][3] = matrix->pos.x;
                mtx[1][3] = matrix->pos.y;
                mtx[2][3] = matrix->pos.z;
            }
            else
            {
                mtx[2][3] = mtx[1][3] = mtx[0][3] = 0.0f;
            }

            PSMTXConcat(texMtx->data, mtx, mtx2);
            GXLoadTexMtxImm(mtx2, texMtx->offset,
                            (GXTexMtxType)(texMtx->flags & rpGAMECUBEMTTEXMTX2x4));
        }
        else
        {
            GXLoadTexMtxImm(texMtx->data, texMtx->offset,
                            (GXTexMtxType)(texMtx->flags & rpGAMECUBEMTTEXMTX2x4));
        }
    }

    for (i = 0; i < config->numIndMtx; i++)
    {
        RpGameCubeIndMtx* indMtx;
        RwMatrix scratch;
        RwMatrix* matrix;
        RwReal(*pMtx)[3];
        RwReal mtx[2][3];

        indMtx = &config->indMtx[i];

        matrix = GetTexFrameMatrix(indMtx->refFrame, objectLTM, &scratch);
        if (matrix != NULL)
        {
            mtx[0][0] = indMtx->data[0][0] * matrix->right.x +
                        indMtx->data[0][1] * matrix->right.y + indMtx->data[0][2] * matrix->right.z;
            mtx[0][1] = indMtx->data[0][0] * matrix->up.x + indMtx->data[0][1] * matrix->up.y +
                        indMtx->data[0][2] * matrix->up.z;
            mtx[0][2] = indMtx->data[0][0] * matrix->at.x + indMtx->data[0][1] * matrix->at.y +
                        indMtx->data[0][2] * matrix->at.z;
            mtx[1][0] = indMtx->data[1][0] * matrix->right.x +
                        indMtx->data[1][1] * matrix->right.y + indMtx->data[1][2] * matrix->right.z;
            mtx[1][1] = indMtx->data[1][0] * matrix->up.x + indMtx->data[1][1] * matrix->up.y +
                        indMtx->data[1][2] * matrix->up.z;
            mtx[1][2] = indMtx->data[1][0] * matrix->at.x + indMtx->data[1][1] * matrix->at.y +
                        indMtx->data[1][2] * matrix->at.z;

            pMtx = mtx;
        }
        else
        {
            pMtx = indMtx->data;
        }

        GXSetIndTexMtx((GXIndTexMtxID)indMtx->id, pMtx, (s8)indMtx->scale);
    }

    rasSrc = TEVRAS_NORM;
    if (config->flags & rpGAMECUBEMTCONFIGCHANNELCOLOR)
    {
        rasSrc = TEVRAS_CH0A0;
    }

    for (i = 0; i < config->numTexGens; i++)
    {
        texGenTexture[i] = -1;
    }

    if (config->numIndStages != 0)
    {
        GXSetNumIndStages(config->numIndStages);

        for (i = 0; i < config->numIndStages; i++)
        {
            RpGameCubeIndStage* stage;

            stage = &config->indStages[i];

            if (!(stage->flags & rpGAMECUBEMTDISABLE))
            {
                GXSetIndTexOrder((GXIndTexStageID)i, (GXTexCoordID)stage->texCoordID,
                                 (GXTexMapID)stage->texMapID);
                GXSetIndTexCoordScale((GXIndTexStageID)i, (GXIndTexScale)stage->scaleS,
                                      (GXIndTexScale)stage->scaleT);

                texGenTexture[stage->texCoordID] = (RwInt8)stage->texMapID;
            }
        }
    }

    if (config->numTevStages != 0)
    {
        GXSetNumTevStages(config->numTevStages);

        for (i = 0; i < config->numTevStages; i++)
        {
            RpGameCubeTevStage* stage;
            RwUInt32 channelID;

            stage = &config->tevStages[i];

            if (!(stage->flags & rpGAMECUBEMTDISABLE))
            {
                if (stage->indirect != 0)
                {
                    RpGameCubeTevInd ind;

                    RpGameCubeTevIndUnpack(&ind, &stage->indirect);
                    GXSetTevIndirect((GXTevStageID)i, (GXIndTexStageID)ind.indStage,
                                     (GXIndTexFormat)ind.format, (GXIndTexBiasSel)ind.biasSel,
                                     (GXIndTexMtxID)ind.matrixSel, (GXIndTexWrap)ind.wrapS,
                                     (GXIndTexWrap)ind.wrapT, ind.addPrev, ind.utcLod,
                                     (GXIndTexAlphaSel)ind.alphaSel);
                }
                else
                {
                    GXSetTevDirect((GXTevStageID)i);
                }

                GXSetTevColorIn((GXTevStageID)i, (GXTevColorArg)stage->op.colorA,
                                (GXTevColorArg)stage->op.colorB, (GXTevColorArg)stage->op.colorC,
                                (GXTevColorArg)stage->op.colorD);
                GXSetTevAlphaIn((GXTevStageID)i, (GXTevAlphaArg)stage->op.alphaA,
                                (GXTevAlphaArg)stage->op.alphaB, (GXTevAlphaArg)stage->op.alphaC,
                                (GXTevAlphaArg)stage->op.alphaD);

                channelID = stage->channelID;
                if ((RwInt32)channelID <= GX_COLOR1A1 && rasSrc == TEVRAS_CH0A0)
                {
                    channelID = GX_COLOR0A0;
                }

                GXSetTevOrder((GXTevStageID)i, (GXTexCoordID)stage->texCoordID,
                              (GXTexMapID)stage->texMapID, (GXChannelID)channelID);
                GXSetTevKColorSel((GXTevStageID)i, (GXTevKColorSel)stage->op.colorSel);
                GXSetTevKAlphaSel((GXTevStageID)i, (GXTevKAlphaSel)stage->op.alphaSel);
                GXSetTevColorOp((GXTevStageID)i, (GXTevOp)stage->op.colorOp,
                                (GXTevBias)stage->op.colorBias, (GXTevScale)stage->op.colorScale,
                                stage->op.colorClamp, (GXTevRegID)stage->op.colorOutReg);
                GXSetTevAlphaOp((GXTevStageID)i, (GXTevOp)stage->op.alphaOp,
                                (GXTevBias)stage->op.alphaBias, (GXTevScale)stage->op.alphaScale,
                                stage->op.alphaClamp, (GXTevRegID)stage->op.alphaOutReg);

                texGenTexture[stage->texCoordID] = (RwInt8)stage->texMapID;
            }
        }
    }

    GXSetNumTexGens(config->numTexGens);

    for (i = 0; i < config->numTexGens; i++)
    {
        RpGameCubeTexGen* texGen;
        GXTexGenSrc src;

        texGen = &config->texGens[i];

        if (!(texGen->flags & rpGAMECUBEMTDISABLE))
        {
            src = (GXTexGenSrc)texGen->srcParam;

            if (coordMap != NULL && src >= GX_TG_TEX0 && src <= GX_TG_TEX7 && texGenTexture[i] >= 0)
            {
                src = (GXTexGenSrc)(coordMap[texGenTexture[i]] + GX_TG_TEX0);
            }

            GXSetTexCoordGen2((GXTexCoordID)i, (GXTexGenType)texGen->func, src, texGen->mtx,
                              (texGen->flags >> 1) & 1, texGen->postMtx);

            if (texGen->flags & rpGAMECUBEMTTEXGENSCALEMANUALLY)
            {
                GXSetTexCoordScaleManually((GXTexCoordID)i, GX_TRUE, texGen->scaleS,
                                           texGen->scaleT);
            }
        }
    }

    if (config->flags & rpGAMECUBEMTCONFIGZCOMPAFTERTEX)
    {
        _rwDlRenderStateSetZCompLoc(FALSE);
    }
    else
    {
        _rwDlRenderStateSetZCompLoc(TRUE);
    }

    if (config->flags & rpGAMECUBEMTCONFIGBLEND)
    {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)config->srcBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)config->destBlend);
    }

    return effect;
}

static void _rpGameCubeMTEffectClean(RpMTEffect* effect)
{
    RpGameCubeMTConfig* config;
    RwInt32 i;

    config = RpGameCubeMTEffectGetConfig(effect);

    for (i = 0; i < config->numTexGens; i++)
    {
        if (config->texGens[i].flags & rpGAMECUBEMTTEXGENSCALEMANUALLY)
        {
            GXSetTexCoordScaleManually((GXTexCoordID)i, GX_FALSE, 0, 0);
        }
    }

    if (config->numIndStages != 0)
    {
        GXSetNumIndStages(0);

        for (i = 0; i < config->numIndStages; i++)
        {
            GXSetIndTexCoordScale((GXIndTexStageID)i, GX_ITS_1, GX_ITS_1);
        }

        for (i = 0; i < config->numTevStages; i++)
        {
            if (config->tevStages[i].indirect != 0)
            {
                GXSetTevDirect((GXTevStageID)i);
            }
        }
    }
}

void _rpGameCubeMTMeshRenderCallBack(RxGameCubeDisplayList* dList, RpMaterial* material,
                                     void* object, RxGameCubePipeData* pipeData,
                                     RwMatrix* objectLTM)
{
    RpMultiTexture* mt;
    rpMultiTextureGameCubeExt* mtExt;
    RwBlendFunction srcBlend;
    RwBlendFunction dstBlend;
    RwUInt32 i;

    mt = RpMaterialGetMultiTexture(material, rwID_GAMECUBE);
    mtExt = (rpMultiTextureGameCubeExt*)mt->platformData;

    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void*)&srcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void*)&dstBlend);

    for (i = 0; i < mt->numTextures; i++)
    {
        _rwDlTextureSet(mt->textures[i], i);
    }

    if (mtExt->preRenderCallBack != NULL)
    {
        mtExt->preRenderCallBack(material, object, pipeData, mtExt->callBackData);
    }

    if (mt->effect != NULL)
    {
        _rpGameCubeMTEffectSend(mt->effect, mt->numTextures, mt->coords, objectLTM);
        GXCallDisplayList(dList->displayList, dList->size);
        _rpGameCubeMTEffectClean(mt->effect);
    }
    else
    {
        GXCallDisplayList(dList->displayList, dList->size);
    }

    if (mtExt->postRenderCallBack != NULL)
    {
        mtExt->postRenderCallBack(material, object, pipeData, mtExt->callBackData);
    }

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dstBlend);
}

RwBool _rpGameCubeMTPipePluginAttach(void)
{
    _rpGameCubeMTEngineOffset = RwEngineRegisterPlugin(
        sizeof(rpGameCubeMTGlobals), rwID_GCNMULTITEXPLUGIN, GameCubeMTOpen, GameCubeMTClose);

    return _rpGameCubeMTEngineOffset >= 0;
}
