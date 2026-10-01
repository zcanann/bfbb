#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpmatfx.h>
#include <dolphin/gx.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"
#include "rwsdk/plugin/matfx/matfxprivate.h"
#include "rwsdk/plugin/matfx/gcn/mtgcnprivate.h"

#define rxGCMATFXTEXCOORDSET1 0x80

#define MATFXUVANIMTEXMTX(_mtx, _matrix)                                                           \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_mtx)[0][0] = (_matrix)->right.x;                                                         \
        (_mtx)[0][1] = (_matrix)->up.x;                                                            \
        (_mtx)[0][2] = 0.0f;                                                                       \
        (_mtx)[0][3] = (_matrix)->pos.x;                                                           \
        (_mtx)[1][0] = (_matrix)->right.y;                                                         \
        (_mtx)[1][1] = (_matrix)->up.y;                                                            \
        (_mtx)[1][2] = 0.0f;                                                                       \
        (_mtx)[1][3] = (_matrix)->pos.y;                                                           \
    }                                                                                              \
    MACRO_STOP

extern RwMatrix _RwDlInvCamLTM;

extern RpGameCubeVtxFmt* RpGameCubeVtxFmtCreate(void);
extern RwBool RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* vtxFmt);
extern RpGameCubeVtxFmt* RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* vtxFmt,
                                                   RpGameCubeCompType type, RwBool nbt);
extern RpGeometry* RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry, RpGameCubeVtxFmt* vtxFmt);
extern RwBool RxGameCubePreInstanceGetOptimize(void);
extern void RxGameCubePreInstanceSetOptimize(RwBool optimize);

extern RxNodeDefinition* RxNodeDefinitionGetGameCubeAtomicAllInOne(void);
extern RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void);
extern RxGameCubeAllInOneCallBack _rxGameCubeAllInOneGetInstanceCallBack(RxPipelineNode* node);
extern RxGameCubeAllInOneCallBack _rxGameCubeAllInOneGetReinstanceCallBack(RxPipelineNode* node);
extern RxPipelineNode* _rxGameCubeAllInOneSetInstanceCallBack(RxPipelineNode* node,
                                                              RxGameCubeAllInOneCallBack callback);
extern RxPipelineNode*
_rxGameCubeAllInOneSetReinstanceCallBack(RxPipelineNode* node, RxGameCubeAllInOneCallBack callback);
extern RxPipelineNode* RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node,
                                                           RxGameCubeAllInOneCallBack callback);

extern RwBool _rpGameCubeMTPipeDataQueryNBTs(RxGameCubePipeData* pipeData);
extern void _rpGameCubeMTPipeDataCalcNBTs(RxGameCubePipeData* pipeData, RpGameCubeVtxFmt* vtxFmt,
                                          RwInt32 numVerts);
extern void _rpGameCubeMTMeshRenderCallBack(RxGameCubeDisplayList* dList, RpMaterial* material,
                                            void* object, RxGameCubePipeData* pipeData,
                                            RwMatrix* objectLTM);

typedef struct rpMatFXStateCache rpMatFXStateCache;
struct rpMatFXStateCache
{
    RwBool channelDefault;
    RpMaterial* channelMaterial;
    RwBool tevDefault;
};

RpGameCubeVtxFmt* _rpGCMatFXVtxFmtNBT;
static RxPipeline* _RpMatFXAtomicPipe;
static RxPipeline* _RpMatFXWorldSectorPipe;

static RxGameCubeAllInOneCallBack DefaultSectorInstanceCallBack;
static RxGameCubeAllInOneCallBack DefaultAtomicReinstanceCallBack;
static RxGameCubeAllInOneCallBack DefaultAtomicInstanceCallBack;

static rpMatFXStateCache FXStateCache;

static void _rxGCChannelLightingSetup(RxGameCubePipeData* pipeData)
{
    static const GXColor opaqueWhite = { 255, 255, 255, 255 };
    static const GXColor opaqueBlack = { 0, 0, 0, 255 };

    RwInt32 flags;
    GXColorSrc ambColorSrc;

    flags = pipeData->flags;

    if (pipeData->numLights > 0)
    {
        if (flags & rpGEOMETRYPRELIT)
        {
            ambColorSrc = GX_SRC_VTX;
        }
        else
        {
            ambColorSrc = GX_SRC_REG;

            if (!pipeData->ambientLight)
            {
                GXSetChanAmbColor(GX_COLOR0A0, opaqueBlack);
            }
        }

        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, ambColorSrc, GX_SRC_REG, pipeData->lightMask,
                      GX_DF_CLAMP, GX_AF_SPOT);

        if (!(flags & rpGEOMETRYMODULATEMATERIALCOLOR))
        {
            GXSetChanMatColor(GX_COLOR0A0, opaqueWhite);
        }
    }
    else if (flags & rpGEOMETRYPRELIT)
    {
        if (flags & rpGEOMETRYMODULATEMATERIALCOLOR)
        {
            GXSetNumChans(1);
            GXSetChanCtrl(GX_COLOR0A0, GX_TRUE, GX_SRC_VTX, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                          GX_AF_NONE);
        }
        else
        {
            GXSetNumChans(1);
            GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE,
                          GX_AF_NONE);
        }
    }
    else
    {
        if (!pipeData->ambientLight && !(flags & rpGEOMETRYMODULATEMATERIALCOLOR))
        {
            GXSetChanMatColor(GX_COLOR0A0, opaqueBlack);
        }

        GXSetNumChans(1);
        GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_NONE,
                      GX_AF_NONE);
    }
}

static void _rxGCChannelMaterialSetup(RxGameCubePipeData* pipeData, RpMaterial* material)
{
    GXColor color;
    RwReal ambCoeff;
    RwRGBAReal* ambColor;
    RwRGBA* matColor;

    ambColor = &pipeData->ambientLightColor;
    matColor = &material->color;

    if (pipeData->numLights > 0)
    {
        if (pipeData->ambientLight && !(pipeData->flags & rpGEOMETRYPRELIT))
        {
            ambCoeff = 255.0f * material->surfaceProps.ambient;

            color.r = (RwUInt8)(ambCoeff * ambColor->red);
            color.g = (RwUInt8)(ambCoeff * ambColor->green);
            color.b = (RwUInt8)(ambCoeff * ambColor->blue);
            color.a = 255;

            GXSetChanAmbColor(GX_COLOR0A0, color);
        }

        if (pipeData->flags & rpGEOMETRYMODULATEMATERIALCOLOR)
        {
            GXSetChanMatColor(GX_COLOR0A0, *(GXColor*)matColor);
        }
    }
    else if (pipeData->flags & rpGEOMETRYPRELIT)
    {
        if (pipeData->flags & rpGEOMETRYMODULATEMATERIALCOLOR)
        {
            GXSetChanMatColor(GX_COLOR0A0, *(GXColor*)matColor);
        }
    }
    else if (pipeData->ambientLight)
    {
        ambCoeff = material->surfaceProps.ambient;

        if (pipeData->flags & rpGEOMETRYMODULATEMATERIALCOLOR)
        {
            color.r = (RwUInt8)(matColor->red * (ambCoeff * ambColor->red));
            color.g = (RwUInt8)(matColor->green * (ambCoeff * ambColor->green));
            color.b = (RwUInt8)(matColor->blue * (ambCoeff * ambColor->blue));
            color.a = matColor->alpha;
        }
        else
        {
            ambCoeff *= 255.0f;

            color.r = (RwUInt8)(ambCoeff * ambColor->red);
            color.g = (RwUInt8)(ambCoeff * ambColor->green);
            color.b = (RwUInt8)(ambCoeff * ambColor->blue);
            color.a = 255;
        }

        GXSetChanMatColor(GX_COLOR0A0, color);
    }
    else if (pipeData->flags & rpGEOMETRYMODULATEMATERIALCOLOR)
    {
        color.r = color.g = color.b = 0;
        color.a = matColor->alpha;

        GXSetChanMatColor(GX_COLOR0A0, color);
    }
}

static void _rxGCTevDefaultSetup(RxGameCubePipeData* pipeData)
{
    GXSetNumTevStages(1);

    if (pipeData->flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE,
                          GX_PTIDENTITY);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    }
    else
    {
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }
}

static void _rpDlMatFXStateCacheInit(void)
{
    FXStateCache.tevDefault = FALSE;
    FXStateCache.channelDefault = FALSE;
    FXStateCache.channelMaterial = NULL;
}

static void ProjectionMatrixInit(RwReal texMtx[3][4], RwMatrix* objectLTM, RwFrame* frame,
                                 RwReal scale, RwReal offset)
{
    RwMatrix tmpMatrix;
    RwMatrix invFrameLTM;
    RwMatrix* matrix;

    if (frame != NULL)
    {
        RwMatrixInvert(&invFrameLTM, RwFrameGetLTM(frame));
        matrix = &invFrameLTM;
    }
    else
    {
        matrix = &_RwDlInvCamLTM;
    }

    if (objectLTM != NULL)
    {
        RwMatrixMultiply(&tmpMatrix, objectLTM, matrix);
        matrix = &tmpMatrix;
    }

    texMtx[0][0] = scale * matrix->right.x;
    texMtx[0][1] = scale * matrix->up.x;
    texMtx[0][2] = scale * matrix->at.x;
    texMtx[0][3] = offset;
    texMtx[1][0] = scale * matrix->right.y;
    texMtx[1][1] = scale * matrix->up.y;
    texMtx[1][2] = scale * matrix->at.y;
    texMtx[1][3] = offset;
}

static void _rpGCMatFxEnvMatrixSetup(RwMatrix* objectLTM, RwFrame* frame)
{
    RwReal texMtx[3][4];

    ProjectionMatrixInit(texMtx, objectLTM, frame, -0.5f, 0.5f);
    GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);
}

static void SetSingleTextureWithAlphaComp(RwTexture* texture)
{
    RwGameCubeRasterExtension* rasExt;

    _rwDlTextureSet(texture, 0);

    if (texture != NULL && texture->raster != NULL)
    {
        rasExt = RASTEREXTFROMRASTER(RwRasterGetParent(texture->raster));
        _rwDlRenderStateSetZCompLoc((rasExt->flags & 1) ? FALSE : TRUE);
    }
}

#define MATFXCHANNELSETUP(_pipeData, _material)                                                    \
    MACRO_START                                                                                    \
    {                                                                                              \
        if (!FXStateCache.channelDefault)                                                          \
        {                                                                                          \
            _rxGCChannelLightingSetup(_pipeData);                                                  \
            FXStateCache.channelDefault = TRUE;                                                    \
        }                                                                                          \
                                                                                                   \
        if (FXStateCache.channelMaterial != (_material))                                           \
        {                                                                                          \
            _rxGCChannelMaterialSetup(_pipeData, _material);                                       \
            FXStateCache.channelMaterial = (_material);                                            \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

#define MATFXTEVDEFAULTSETUP(_pipeData)                                                            \
    MACRO_START                                                                                    \
    {                                                                                              \
        if (!FXStateCache.tevDefault)                                                              \
        {                                                                                          \
            _rxGCTevDefaultSetup(_pipeData);                                                       \
            FXStateCache.tevDefault = TRUE;                                                        \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

static void MeshRenderStandard(RpMesh* mesh, RxGameCubeDisplayList* dList,
                               RxGameCubePipeData* pipeData)
{
    MATFXCHANNELSETUP(pipeData, mesh->material);
    MATFXTEVDEFAULTSETUP(pipeData);

    SetSingleTextureWithAlphaComp(mesh->material->texture);

    GXCallDisplayList(dList->displayList, dList->size);
}

static void MeshRenderUVAnim(RpMesh* mesh, RxGameCubeDisplayList* dList,
                             RxGameCubePipeData* pipeData)
{
    MatFXUVAnimData* uvAnimData;
    RwMatrix* matrix;
    RwReal mtx[3][4];

    uvAnimData = &(*MATFXMATERIALGETDATA(mesh->material))->data[0].data.uvAnim;

    MATFXCHANNELSETUP(pipeData, mesh->material);
    MATFXTEVDEFAULTSETUP(pipeData);

    matrix = uvAnimData->baseTransform;
    if (matrix != NULL)
    {
        MATFXUVANIMTEXMTX(mtx, matrix);

        GXLoadTexMtxImm(mtx, GX_TEXMTX0, GX_MTX2x4);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_FALSE,
                          GX_PTIDENTITY);

        FXStateCache.tevDefault = FALSE;
    }

    SetSingleTextureWithAlphaComp(mesh->material->texture);

    GXCallDisplayList(dList->displayList, dList->size);
}

static void MeshRenderDual(RpMesh* mesh, RxGameCubeDisplayList* dList, RxGameCubePipeData* pipeData)
{
    RwInt32 i;
    RwBlendFunction srcBlend;
    RwBlendFunction dstBlend;
    RxGameCubeVertexBuffer* vbHeader;
    MatFXDualData* dualData;

    i = 0;
    dualData = &(*MATFXMATERIALGETDATA(mesh->material))->data[0].data.dual;
    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);

    MATFXCHANNELSETUP(pipeData, mesh->material);
    MATFXTEVDEFAULTSETUP(pipeData);

    SetSingleTextureWithAlphaComp(mesh->material->texture);

    GXCallDisplayList(dList->displayList, dList->size);

    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void*)&srcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void*)&dstBlend);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)dualData->srcBlendMode);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dualData->dstBlendMode);

    if (pipeData->flags & rxGCMATFXTEXCOORDSET1)
    {
        while (vbHeader->attr[i].attr != GX_VA_TEX1)
        {
            i++;
        }

        GXSetArray(GX_VA_TEX0, vbHeader->attr[i].array, vbHeader->attr[i].stride);
    }

    SetSingleTextureWithAlphaComp(dualData->texture);

    GXCallDisplayList(dList->displayList, dList->size);

    if (pipeData->flags & rxGCMATFXTEXCOORDSET1)
    {
        GXSetArray(GX_VA_TEX0, vbHeader->attr[i - 1].array, vbHeader->attr[i - 1].stride);
    }

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dstBlend);
}

static void MeshRenderUVAnimDual(RpMesh* mesh, RxGameCubeDisplayList* dList,
                                 RxGameCubePipeData* pipeData)
{
    RwInt32 i;
    RwBlendFunction srcBlend;
    RwBlendFunction dstBlend;
    RxGameCubeVertexBuffer* vbHeader;
    MatFXDualData* dualData;
    MatFXUVAnimData* uvAnimData;
    RwReal mtx[3][4];

    i = 0;
    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);
    uvAnimData = &(*MATFXMATERIALGETDATA(mesh->material))->data[0].data.uvAnim;
    dualData = &(*MATFXMATERIALGETDATA(mesh->material))->data[1].data.dual;

    MATFXCHANNELSETUP(pipeData, mesh->material);
    MATFXTEVDEFAULTSETUP(pipeData);

    if (uvAnimData->baseTransform != NULL)
    {
        MATFXUVANIMTEXMTX(mtx, uvAnimData->baseTransform);

        GXLoadTexMtxImm(mtx, GX_TEXMTX0, GX_MTX2x4);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_FALSE,
                          GX_PTIDENTITY);

        FXStateCache.tevDefault = FALSE;
    }

    SetSingleTextureWithAlphaComp(mesh->material->texture);

    GXCallDisplayList(dList->displayList, dList->size);

    if (uvAnimData->dualTransform != NULL)
    {
        MATFXUVANIMTEXMTX(mtx, uvAnimData->dualTransform);

        GXLoadTexMtxImm(mtx, GX_TEXMTX0, GX_MTX2x4);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0, GX_FALSE,
                          GX_PTIDENTITY);

        FXStateCache.tevDefault = FALSE;
    }
    else if (uvAnimData->baseTransform != NULL)
    {
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE,
                          GX_PTIDENTITY);
    }

    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void*)&srcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void*)&dstBlend);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)dualData->srcBlendMode);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dualData->dstBlendMode);

    if (pipeData->flags & rxGCMATFXTEXCOORDSET1)
    {
        while (vbHeader->attr[i].attr != GX_VA_TEX1)
        {
            i++;
        }

        GXSetArray(GX_VA_TEX0, vbHeader->attr[i].array, vbHeader->attr[i].stride);
    }

    SetSingleTextureWithAlphaComp(dualData->texture);

    GXCallDisplayList(dList->displayList, dList->size);

    if (pipeData->flags & rxGCMATFXTEXCOORDSET1)
    {
        GXSetArray(GX_VA_TEX0, vbHeader->attr[i - 1].array, vbHeader->attr[i - 1].stride);
    }

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dstBlend);
}

static void MeshRenderEnvMap(RpMesh* mesh, RxGameCubeDisplayList* dList, RwMatrix* objectLTM,
                             RxGameCubePipeData* pipeData)
{
    MatFXEnvMapData* envMapData = &(*MATFXMATERIALGETDATA(mesh->material))->data[0].data.envMap;
    RwBlendFunction srcBlend;
    RwBlendFunction dstBlend;
    GXColor shiney = { 255, 255, 255, 255 };

    MATFXCHANNELSETUP(pipeData, mesh->material);
    MATFXTEVDEFAULTSETUP(pipeData);

    _rwDlTextureSet(mesh->material->texture, 0);
    _rwDlRenderStateSetZCompLoc(TRUE);

    GXCallDisplayList(dList->displayList, dList->size);

    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void*)&srcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void*)&dstBlend);

    RwRenderStateSet(rwRENDERSTATESRCBLEND,
                     (void*)(envMapData->useFrameBufferAlpha ? rwBLENDDESTALPHA : rwBLENDONE));
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

    GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP,
                  GX_AF_NONE);
    GXSetChanCtrl(GX_COLOR1A1, GX_FALSE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT_NULL, GX_DF_CLAMP,
                  GX_AF_NONE);

    if (envMapData->coef < 1.0f)
    {
        shiney.a = 255;
        shiney.r = shiney.g = shiney.b = (RwUInt8)(255.9f * envMapData->coef);
    }
    else
    {
        shiney.a = 255;
        shiney.b = 255;
        shiney.g = 255;
        shiney.r = 255;
    }

    GXSetChanMatColor(GX_COLOR0A0, shiney);

    GXSetNumTexGens(1);
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_NRM, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY);
    GXSetNumTevStages(1);
    GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

    FXStateCache.channelMaterial = NULL;
    FXStateCache.channelDefault = FALSE;
    FXStateCache.tevDefault = FALSE;

    _rwDlTextureSet(envMapData->texture, 0);

    _rpGCMatFxEnvMatrixSetup(objectLTM, envMapData->frame);

    GXCallDisplayList(dList->displayList, dList->size);

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dstBlend);
}

static void MeshRenderBumpMap(RpMesh* mesh, RxGameCubeDisplayList* dList, RwMatrix* objectLTM,
                              RxGameCubePipeData* pipeData, RwBool envMap)
{
    MatFXBumpMapData* bumpData;
    RwReal bumpMtx[3][4];
    GXTevStageID i;

    bumpData = &(*MATFXMATERIALGETDATA(mesh->material))->data[0].data.bumpMap;

    MATFXCHANNELSETUP(pipeData, mesh->material);

    FXStateCache.tevDefault = FALSE;

    _rwDlTextureSet(bumpData->texture, 0);
    _rwDlRenderStateSetZCompLoc(TRUE);

    ProjectionMatrixInit(bumpMtx, objectLTM, bumpData->frame,
                         -bumpData->invBumpWidth * bumpData->coef, 0.0f);
    GXLoadTexMtxImm(bumpMtx, GX_TEXMTX0, GX_MTX2x4);

    if (envMap)
    {
        MatFXEnvMapData* envData;
        RwReal envMtx[3][4];

        envData = &(*MATFXMATERIALGETDATA(mesh->material))->data[1].data.envMap;

        _rwDlTextureSet(envData->texture, 1);

        ProjectionMatrixInit(envMtx, objectLTM, envData->frame, -0.5f, 0.5f);
        GXLoadTexMtxImm(envMtx, GX_TEXMTX1, GX_MTX2x4);

        GXSetNumTexGens(3);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE,
                          GX_PTIDENTITY);
        GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_NRM, GX_TEXMTX0, GX_FALSE,
                          GX_PTIDENTITY);
        GXSetTexCoordGen2(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_NRM, GX_TEXMTX1, GX_FALSE,
                          GX_PTIDENTITY);
    }
    else
    {
        GXSetNumTexGens(2);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE,
                          GX_PTIDENTITY);
        GXSetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX2x4, GX_TG_NRM, GX_TEXMTX0, GX_FALSE,
                          GX_PTIDENTITY);
    }

    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GXSetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_1);
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASC, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVREG0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_KONST, GX_CA_ZERO, GX_CA_TEXA, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

    GXSetTevOrder(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP0, GX_COLOR_NULL);
    GXSetTevIndirect(GX_TEVSTAGE1, GX_INDTEXSTAGE0, GX_ITF_8, GX_ITB_NONE, GX_ITM_OFF, GX_ITW_OFF,
                     GX_ITW_OFF, GX_TRUE, GX_FALSE, GX_ITBA_OFF);
    GXSetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_APREV, GX_CC_C0, GX_CC_ZERO);
    GXSetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GXSetTevAlphaIn(GX_TEVSTAGE1, GX_CA_TEXA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
    GXSetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

    if (envMap)
    {
        MatFXEnvMapData* envData;
        GXColor col;

        envData = &(*MATFXMATERIALGETDATA(mesh->material))->data[1].data.envMap;

        if (envData->coef < 1.0f)
        {
            col.a = (RwUInt8)(255.9f * envData->coef);
        }
        else
        {
            col.a = 255;
        }

        GXSetTevKColor(GX_KCOLOR0, col);

        GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR_NULL);
        GXSetTevKAlphaSel(GX_TEVSTAGE2, GX_TEV_KASEL_K0_A);
        GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_APREV, GX_CC_C0, GX_CC_CPREV);
        GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

        if (envData->useFrameBufferAlpha)
        {
            GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_ZERO, GX_CA_KONST, GX_CA_APREV, GX_CA_ZERO);
        }
        else
        {
            GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_KONST, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        }

        GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

        i = GX_TEVSTAGE3;

        GXSetTevOrder(GX_TEVSTAGE3, GX_TEXCOORD2, GX_TEXMAP1, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE3, GX_CC_ZERO, GX_CC_APREV, GX_CC_TEXC, GX_CC_CPREV);
        GXSetTevColorOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE3, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE3, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    }
    else
    {
        i = GX_TEVSTAGE2;

        GXSetTevOrder(GX_TEVSTAGE2, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevColorIn(GX_TEVSTAGE2, GX_CC_ZERO, GX_CC_APREV, GX_CC_C0, GX_CC_CPREV);
        GXSetTevColorOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        GXSetTevAlphaIn(GX_TEVSTAGE2, GX_CA_RASA, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO);
        GXSetTevAlphaOp(GX_TEVSTAGE2, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    }

    GXSetNumTevStages((RwUInt8)(i + 1));

    GXCallDisplayList(dList->displayList, dList->size);

    GXSetTevDirect(GX_TEVSTAGE1);
}

void _rpDlMatFXMeshRender(RpMesh* mesh, RxGameCubeDisplayList* dList, void* object, RwMatrix* ltm,
                          RxGameCubePipeData* pipeData)
{
    RpMultiTexture* multiTexture;
    rpMatFXMaterialData* matFXData;

    multiTexture = RpMaterialGetMultiTexture(mesh->material, rwID_GAMECUBE);
    if (multiTexture != NULL)
    {
        MATFXCHANNELSETUP(pipeData, mesh->material);

        FXStateCache.tevDefault = FALSE;

        _rpGameCubeMTMeshRenderCallBack(dList, mesh->material, object, pipeData, ltm);
        return;
    }

    matFXData = *MATFXMATERIALGETDATA(mesh->material);
    if (matFXData != NULL)
    {
        switch (matFXData->flags)
        {
        case rpMATFXEFFECTDUAL:
            MeshRenderDual(mesh, dList, pipeData);
            break;
        case rpMATFXEFFECTUVTRANSFORM:
            MeshRenderUVAnim(mesh, dList, pipeData);
            break;
        case rpMATFXEFFECTDUALUVTRANSFORM:
            MeshRenderUVAnimDual(mesh, dList, pipeData);
            break;
        case rpMATFXEFFECTENVMAP:
            MeshRenderEnvMap(mesh, dList, ltm, pipeData);
            break;
        case rpMATFXEFFECTBUMPMAP:
            MeshRenderBumpMap(mesh, dList, ltm, pipeData, FALSE);
            break;
        case rpMATFXEFFECTBUMPENVMAP:
            MeshRenderBumpMap(mesh, dList, ltm, pipeData, TRUE);
            break;
        default:
            MeshRenderStandard(mesh, dList, pipeData);
            break;
        }
    }
    else
    {
        MeshRenderStandard(mesh, dList, pipeData);
    }
}

void* _rpGCMatFXAtomicInstanceCallBack(void* object, RxGameCubePipeData* pipeData)
{
    RwBool instOpt;
    RpGeometry* geom;
    RpGameCubeVtxFmt* vtxFmt;

    if (_rpGameCubeMTPipeDataQueryNBTs(pipeData))
    {
        geom = ((RpAtomic*)object)->geometry;

        vtxFmt = GEOMVTXFMT(geom);
        if (vtxFmt == NULL)
        {
            RpGameCubeGeometrySetVtxFmt(geom, _rpGCMatFXVtxFmtNBT);
            vtxFmt = _rpGCMatFXVtxFmtNBT;
        }

        instOpt = RxGameCubePreInstanceGetOptimize();
        if (instOpt == TRUE)
        {
            RxGameCubePreInstanceSetOptimize(FALSE);
        }

        object = DefaultAtomicInstanceCallBack(object, pipeData);

        if (instOpt == TRUE)
        {
            RxGameCubePreInstanceSetOptimize(TRUE);
        }

        if (object != NULL)
        {
            _rpGameCubeMTPipeDataCalcNBTs(pipeData, vtxFmt, geom->numVertices);
        }

        return object;
    }

    return DefaultAtomicInstanceCallBack(object, pipeData);
}

void* _rpGCMatFXAtomicReinstanceCallBack(void* object, RxGameCubePipeData* pipeData)
{
    RpAtomic* atomic;
    RpGeometry* geometry;
    RpGameCubeVtxFmt* vtxFmt;
    RwBool regenNBTs;
    RwUInt32 locked;
    RwUInt32 morph;

    atomic = (RpAtomic*)object;
    geometry = atomic->geometry;
    regenNBTs = FALSE;

    vtxFmt = GEOMVTXFMT(geometry);
    if (vtxFmt != NULL && vtxFmt->nbt && (geometry->flags & rpGEOMETRYNORMALS))
    {
        locked = geometry->lockedSinceLastInst & (rpGEOMETRYLOCKPOLYGONS | rpGEOMETRYLOCKVERTICES |
                                                  rpGEOMETRYLOCKNORMALS | rpGEOMETRYLOCKTEXCOORDS);

        morph = (geometry->numMorphTargets != 1 && (atomic->interpolator.flags & 1));

        regenNBTs = (locked || morph);
    }

    if (DefaultAtomicReinstanceCallBack(object, pipeData) == NULL)
    {
        return NULL;
    }

    if (regenNBTs)
    {
        _rpGameCubeMTPipeDataCalcNBTs(pipeData, vtxFmt, geometry->numVertices);
    }

    return object;
}

void* _rpGCMatFXSectorInstanceCallBack(void* object, RxGameCubePipeData* pipeData)
{
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 numVerts;

    if (DefaultSectorInstanceCallBack(object, pipeData) == NULL)
    {
        return NULL;
    }

    vtxFmt = WORLDVTXFMT(RWSRCGLOBAL(curWorld));
    if (vtxFmt != NULL && vtxFmt->nbt)
    {
        numVerts = ((RpWorldSector*)object)->numVertices;
        _rpGameCubeMTPipeDataCalcNBTs(pipeData, vtxFmt, numVerts);
    }

    return object;
}

void* _rpGCMatFXRenderCallback(void* object, RxGameCubePipeData* pipeData)
{
    RwInt32 numMeshes;
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* dList;
    RpMesh* mesh;
    RwMatrix* ltm;
    RpMeshHeader* meshHeader;

    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);
    dList = (RxGameCubeDisplayList*)((RxGameCubeVertexAttr*)(vbHeader + 1) +
                                     (vbHeader->numAttrArrays - 1));

    vbHeader->token = _RwDlTokenCurrent;

    if (RwObjectGetType(object) == rpATOMIC)
    {
        meshHeader = ((RpAtomic*)object)->geometry->mesh;
        ltm = RwFrameGetLTM(RpAtomicGetFrame((RpAtomic*)object));
        _rwDlVtxFmtSetup(GEOMVTXFMT(((RpAtomic*)object)->geometry), pipeData);
    }
    else
    {
        ltm = NULL;
        meshHeader = ((RpWorldSector*)object)->mesh;
        _rwDlVtxFmtSetup(WORLDVTXFMT(RWSRCGLOBAL(curWorld)), pipeData);
    }

    _rwDlTransformSetup(ltm, (pipeData->flags & rpGEOMETRYNORMALS) >> 4);

    _rpDlMatFXStateCacheInit();

    mesh = (RpMesh*)(meshHeader + 1);
    numMeshes = meshHeader->numMeshes;

    while (numMeshes--)
    {
        _rpDlMatFXMeshRender(mesh, dList, object, ltm, pipeData);
        dList++;
        mesh++;
    }

    return object;
}

static RxPipeline* MatFXAtomicPipelineCreate(void)
{
    RxPipeline* pipe;
    RxPipeline* lpipe;
    RxNodeDefinition* nodeDef;
    RxPipelineNode* node;

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        pipe->pluginId = rwID_MATERIALEFFECTSPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL)
        {
            nodeDef = RxNodeDefinitionGetGameCubeAtomicAllInOne();
            RxLockedPipeUnlock(RxLockedPipeAddFragment(lpipe, NULL, nodeDef, NULL));

            node = RxPipelineFindNodeByName(pipe, nodeDef->name, NULL, NULL);

            DefaultAtomicInstanceCallBack = _rxGameCubeAllInOneGetInstanceCallBack(node);
            DefaultAtomicReinstanceCallBack = _rxGameCubeAllInOneGetReinstanceCallBack(node);

            _rxGameCubeAllInOneSetInstanceCallBack(node, _rpGCMatFXAtomicInstanceCallBack);
            _rxGameCubeAllInOneSetReinstanceCallBack(node, _rpGCMatFXAtomicReinstanceCallBack);
            RxGameCubeAllInOneSetRenderCallBack(node, _rpGCMatFXRenderCallback);

            return pipe;
        }

        _rxPipelineDestroy(pipe);
    }

    return NULL;
}

static RxPipeline* MatFXWorldSectorPipelineCreate(void)
{
    RxPipeline* pipe;
    RxPipeline* lpipe;
    RxNodeDefinition* nodeDef;
    RxPipelineNode* node;

    pipe = RxPipelineCreate();
    if (pipe != NULL)
    {
        pipe->pluginId = rwID_MATERIALEFFECTSPLUGIN;

        lpipe = RxPipelineLock(pipe);
        if (lpipe != NULL)
        {
            nodeDef = RxNodeDefinitionGetGameCubeWorldSectorAllInOne();
            RxLockedPipeUnlock(RxLockedPipeAddFragment(lpipe, NULL, nodeDef, NULL));

            node = RxPipelineFindNodeByName(pipe, nodeDef->name, NULL, NULL);

            DefaultSectorInstanceCallBack = _rxGameCubeAllInOneGetInstanceCallBack(node);

            _rxGameCubeAllInOneSetInstanceCallBack(node, _rpGCMatFXSectorInstanceCallBack);
            RxGameCubeAllInOneSetRenderCallBack(node, _rpGCMatFXRenderCallback);

            return pipe;
        }

        _rxPipelineDestroy(pipe);
    }

    return NULL;
}

RwBool _rpMatFXPipelinesCreate(void)
{
    _rpGCMatFXVtxFmtNBT = RpGameCubeVtxFmtCreate();
    RpGameCubeVtxFmtSetNormal(_rpGCMatFXVtxFmtNBT, rpF32, TRUE);

    _RpMatFXAtomicPipe = MatFXAtomicPipelineCreate();
    _RpMatFXWorldSectorPipe = MatFXWorldSectorPipelineCreate();

    return TRUE;
}

RwBool _rpMatFXPipelinesDestroy(void)
{
    if (_rpGCMatFXVtxFmtNBT != NULL)
    {
        RpGameCubeVtxFmtDestroy(_rpGCMatFXVtxFmtNBT);
        _rpGCMatFXVtxFmtNBT = NULL;
    }

    if (_RpMatFXAtomicPipe != NULL)
    {
        _rxPipelineDestroy(_RpMatFXAtomicPipe);
        _RpMatFXAtomicPipe = NULL;
    }

    if (_RpMatFXWorldSectorPipe != NULL)
    {
        _rxPipelineDestroy(_RpMatFXWorldSectorPipe);
        _RpMatFXWorldSectorPipe = NULL;
    }

    return TRUE;
}

RpAtomic* _rpMatFXPipelineAtomicSetup(RpAtomic* atomic)
{
    atomic->pipeline = _RpMatFXAtomicPipe;

    return atomic;
}

RpWorldSector* _rpMatFXPipelineWorldSectorSetup(RpWorldSector* worldSector)
{
    worldSector->pipeline = _RpMatFXWorldSectorPipe;

    return worldSector;
}

RwBool _rpMatFXSetupDualRenderState(MatFXDualData* dualData, RwRenderState nState)
{
    return TRUE;
}

RwTexture* _rpMatFXSetupBumpMapTexture(const RwTexture* baseTexture, const RwTexture* effectTexture)
{
    RwTexture* texture;

    texture = _rpMatFXTextureMaskCreate(baseTexture, effectTexture);

    return texture;
}

RxPipeline* RpMatFXGetGameCubePipeline(RpMatFXGameCubePipeline gamecubePipeline)
{
    switch (gamecubePipeline)
    {
    case rpMATFXGAMECUBEATOMICPIPELINE:
        return _RpMatFXAtomicPipe;
    case rpMATFXGAMECUBEWORLDSECTORPIPELINE:
        return _RpMatFXWorldSectorPipe;
    default:
        return NULL;
    }
}
