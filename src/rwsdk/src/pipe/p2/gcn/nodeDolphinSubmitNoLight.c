#include <rwsdk/rwcore.h>
#include <dolphin/gx.h>

#define rwPLUGIN_ID 1

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_INVALIDPRIMTYPE 0x25

typedef struct RxCamSpace3DVertex RxCamSpace3DVertex;
typedef struct RxMeshStateVector RxMeshStateVector;

typedef struct _rwIm3DPoolStash _rwIm3DPoolStash;
struct _rwIm3DPoolStash
{
    RwUInt32 flags;
    RwMatrix* ltm;
    RwUInt32 numVerts;
    RxObjSpace3DVertex* objVerts;
    RxCamSpace3DVertex* camVerts;
    void* devVerts;
    RxMeshStateVector* meshState;
    RxRenderStateVector* renderState;
    RxPipeline* pipeline;
    RwPrimitiveType primType;
    RwImVertexIndex* indices;
    RwUInt32 numIndices;
};

typedef struct rwIm3DPool rwIm3DPool;
struct rwIm3DPool
{
    RwUInt16 numElements;
    RwUInt16 pad;
    void* elements;
    RwInt32 stride;
    _rwIm3DPoolStash stash;
};

extern void _rwDlTransformSetup(RwMatrix* ltm, RwBool normals);
extern void _rwDlTextureRasterFlush(void);

static GXPrimitive _rwDlPrimConvTbl[7] = {
    (GXPrimitive)0,   GX_LINES,       GX_LINESTRIP, GX_TRIANGLES,
    GX_TRIANGLESTRIP, GX_TRIANGLEFAN, GX_POINTS,
};

static rwIm3DPool* _rwDlImmPool;

static RwBool _rwDlImmInstanceNode(RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    _rwDlImmPool = (rwIm3DPool*)params->dataParam;

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetGameCubeImmInstance(void)
{
    static RwChar _ImmInstance_csl[] = "ImmInstance.csl";

    static RxNodeDefinition nodeImmInstanceCSL = {
        _ImmInstance_csl,           { _rwDlImmInstanceNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 0, NULL, NULL, 0, NULL }, 0,
        (RxNodeDefEditable)FALSE,   0
    };

    return &nodeImmInstanceCSL;
}

static void _rw3DRenderPrimitiveInit(_rwIm3DPoolStash* stash)
{
    _rwDlTransformSetup(stash->ltm, FALSE);

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

    if (stash->flags & rwIM3D_VERTEXUV)
    {
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);

        GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE);
        GXSetNumTexGens(1);
        GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE,
                          GX_PTIDENTITY);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);

        _rwDlTextureRasterFlush();
    }
    else
    {
        GXSetNumTexGens(0);
        GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }
}

#define SUBMITVERTEX(_vert)                                                                        \
    MACRO_START                                                                                    \
    {                                                                                              \
        GXPosition3f32((_vert)->x, (_vert)->y, (_vert)->z);                                        \
        GXColor4u8((_vert)->r, (_vert)->g, (_vert)->b, (_vert)->a);                                \
    }                                                                                              \
    MACRO_STOP

#define SUBMITVERTEXUV(_vert)                                                                      \
    MACRO_START                                                                                    \
    {                                                                                              \
        GXPosition3f32((_vert)->x, (_vert)->y, (_vert)->z);                                        \
        GXColor4u8((_vert)->r, (_vert)->g, (_vert)->b, (_vert)->a);                                \
        GXTexCoord2f32((_vert)->u, (_vert)->v);                                                    \
    }                                                                                              \
    MACRO_STOP

static RwBool DlSubmitNode(RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RxObjSpace3DVertex* verts;
    _rwIm3DPoolStash* stash;
    RwImVertexIndex* indices;

    verts = (RxObjSpace3DVertex*)_rwDlImmPool->elements;
    stash = &_rwDlImmPool->stash;

    if (stash->indices != NULL)
    {
        indices = stash->indices;

        _rw3DRenderPrimitiveInit(stash);

        GXBegin(_rwDlPrimConvTbl[stash->primType], GX_VTXFMT0, (RwUInt16)stash->numIndices);

        switch (stash->primType)
        {
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
        {
            RwUInt16 numVertices = (RwUInt16)stash->numIndices;

            if (stash->flags & rwIM3D_VERTEXUV)
            {
                while (numVertices--)
                {
                    RxObjSpace3DVertex* vert = &verts[*indices++];

                    SUBMITVERTEXUV(vert);
                }
            }
            else
            {
                while (numVertices--)
                {
                    RxObjSpace3DVertex* vert = &verts[*indices++];

                    SUBMITVERTEX(vert);
                }
            }
            break;
        }
        case rwPRIMTYPETRILIST:
        {
            RwUInt16 numTriangles = (RwUInt16)(stash->numIndices / 3);

            if (stash->flags & rwIM3D_VERTEXUV)
            {
                while (numTriangles--)
                {
                    RxObjSpace3DVertex* vert;

                    vert = &verts[*indices++];
                    SUBMITVERTEXUV(vert);
                    vert = &verts[*indices++];
                    SUBMITVERTEXUV(vert);
                    vert = &verts[*indices++];
                    SUBMITVERTEXUV(vert);
                }
            }
            else
            {
                while (numTriangles--)
                {
                    RxObjSpace3DVertex* vert;

                    vert = &verts[*indices++];
                    SUBMITVERTEX(vert);
                    vert = &verts[*indices++];
                    SUBMITVERTEX(vert);
                    vert = &verts[*indices++];
                    SUBMITVERTEX(vert);
                }
            }
            break;
        }
        case rwPRIMTYPELINELIST:
        {
            RwUInt16 numLines = (RwUInt16)(stash->numIndices / 2);

            if (stash->flags & rwIM3D_VERTEXUV)
            {
                while (numLines--)
                {
                    RxObjSpace3DVertex* vert;

                    vert = &verts[*indices++];
                    SUBMITVERTEXUV(vert);
                    vert = &verts[*indices++];
                    SUBMITVERTEXUV(vert);
                }
            }
            else
            {
                while (numLines--)
                {
                    RxObjSpace3DVertex* vert;

                    vert = &verts[*indices++];
                    SUBMITVERTEX(vert);
                    vert = &verts[*indices++];
                    SUBMITVERTEX(vert);
                }
            }
            break;
        }
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE));
            break;
        }

        GXEnd();
    }
    else
    {
        _rw3DRenderPrimitiveInit(stash);

        GXBegin(_rwDlPrimConvTbl[stash->primType], GX_VTXFMT0, _rwDlImmPool->numElements);

        switch (stash->primType)
        {
        case rwPRIMTYPEPOLYLINE:
        case rwPRIMTYPETRISTRIP:
        case rwPRIMTYPETRIFAN:
        {
            RwUInt16 numVertices = _rwDlImmPool->numElements;

            if (stash->flags & rwIM3D_VERTEXUV)
            {
                while (numVertices--)
                {
                    RxObjSpace3DVertex* vert = verts++;

                    SUBMITVERTEXUV(vert);
                }
            }
            else
            {
                while (numVertices--)
                {
                    RxObjSpace3DVertex* vert = verts++;

                    SUBMITVERTEX(vert);
                }
            }
            break;
        }
        case rwPRIMTYPETRILIST:
        {
            RwUInt16 numTriangles = (RwUInt16)(_rwDlImmPool->numElements / 3);

            if (stash->flags & rwIM3D_VERTEXUV)
            {
                while (numTriangles--)
                {
                    RxObjSpace3DVertex* vert;

                    vert = verts;
                    SUBMITVERTEXUV(vert);
                    vert = verts + 1;
                    SUBMITVERTEXUV(vert);
                    vert = verts + 2;
                    SUBMITVERTEXUV(vert);
                    verts += 3;
                }
            }
            else
            {
                while (numTriangles--)
                {
                    RxObjSpace3DVertex* vert;

                    vert = verts;
                    SUBMITVERTEX(vert);
                    vert = verts + 1;
                    SUBMITVERTEX(vert);
                    vert = verts + 2;
                    SUBMITVERTEX(vert);
                    verts += 3;
                }
            }
            break;
        }
        case rwPRIMTYPELINELIST:
        {
            RwUInt16 numLines = (RwUInt16)(_rwDlImmPool->numElements >> 1);

            if (stash->flags & rwIM3D_VERTEXUV)
            {
                while (numLines--)
                {
                    RxObjSpace3DVertex* vert;

                    vert = verts++;
                    SUBMITVERTEXUV(vert);
                    vert = verts++;
                    SUBMITVERTEXUV(vert);
                }
            }
            else
            {
                while (numLines--)
                {
                    RxObjSpace3DVertex* vert;

                    vert = verts++;
                    SUBMITVERTEX(vert);
                    vert = verts++;
                    SUBMITVERTEX(vert);
                }
            }
            break;
        }
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE));
            break;
        }

        GXEnd();
    }

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetGameCubeSubmitNoLight(void)
{
    static RwChar _SubmitNoLight_csl[] = "SubmitNoLight.csl";

    static RxNodeDefinition nodeDlSubmitNoLightCSL = {
        _SubmitNoLight_csl,         { DlSubmitNode, NULL, NULL, NULL, NULL, NULL, NULL },
        { 0, NULL, NULL, 0, NULL }, 0,
        (RxNodeDefEditable)FALSE,   0
    };

    return &nodeDlSubmitNoLightCSL;
}
