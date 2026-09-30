#include "iFX.h"

#include <types.h>

#include <dolphin/gx.h>
#include <dolphin/mtx.h>

#include <rpworld.h>
#include <rwsdk/driver/gcn/dlrendst.h>

// RenderWare's GameCube "AllInOne" pipeline is not described by any header in
// this tree, so the pieces this file needs are reconstructed here.

// RwObject::type of an RpAtomic.
enum
{
    rpATOMIC = 1
};

struct RwGameCubeRasterExt
{
    U32 unk_00;
    U32 unk_04;
    U32 unk_08;
    U32 format; // 0x0C, a GX texture format
    U32 unk_10;
    U32 flags; // 0x14, bit 0 records that the raster wants z-compare after texturing
};

struct rwGameCubeVtxStream
{
    void* data;
    U32 info;
};

struct rwGameCubeDisplayList
{
    void* data;
    U32 size;
};

// Instanced geometry, stored immediately after the RwResEntry. Both trailing
// arrays are variable length and lie back to back, so the display list for the
// first mesh is dl[numStreams - 1].
struct rwGameCubeResEntryHeader
{
    U16 serialNum; // 0x00
    U16 unk_02;
    U32 flags; // 0x04
    U32 numStreams; // 0x08
    rwGameCubeVtxStream streams[1]; // 0x0C
    rwGameCubeDisplayList dl[1]; // 0x14
};

struct RxGameCubePipeData
{
    RwResEntry* resEntry; // 0x00
    RpMeshHeader* meshHeader; // 0x04
    U32 flags; // 0x08, RpGeometryFlag bits
    RwRGBAReal matCol; // 0x0C
    U32 unk_1c;
    U32 unk_20;
};

typedef void (*rwGameCubeMatColCallBack)(RwRGBAReal* dst, const RwRGBA* matCol, F32 ambient);
typedef void* (*RxGameCubeAllInOneRenderCallBack)(void* object, RxGameCubePipeData* gcPipeData);

extern "C" {
extern U16 _RwDlTokenCurrent;
extern S32 _rpDlGeomVtxFmtOffset;
extern S32 _rpDlWorldVtxFmtOffset;
extern U32 _RwGameCubeRasterExtOffset;

void _rwDlVtxFmtSetup(U32 vtxFmt, RxGameCubePipeData* gcPipeData);
void _rwDlTransformSetup(RwMatrix* ltm, RwBool hasNormals);
rwGameCubeMatColCallBack _rwDlObjectRenderSetup(U32 flags, U32 unk20, U32 unk1c, U32 hdrFlags);
void _rwDlTextureSet(RwTexture* texture, U32 stage);
void RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node,
                                         RxGameCubeAllInOneRenderCallBack callBack);
}

extern F32 xFXanimUVRotMat0[2];
extern F32 xFXanimUVRotMat1[2];
extern F32 xFXanimUVTrans[2];
extern F32 xFXanimUVScale[2];

static void* _iGCUVRenderCallback(void* object, RxGameCubePipeData* gcPipeData)
{
    RwResEntry* resEntry = gcPipeData->resEntry;
    rwGameCubeResEntryHeader* header = (rwGameCubeResEntryHeader*)(resEntry + 1);

    U32 numExtraStreams = header->numStreams - 1;
    rwGameCubeDisplayList* dlist = &header->dl[numExtraStreams];

    header->serialNum = _RwDlTokenCurrent;

    RwMatrix* ltm;
    if (((RwObject*)object)->type == rpATOMIC)
    {
        ltm = RwFrameGetLTM((RwFrame*)((RwObject*)object)->parent);
        _rwDlVtxFmtSetup(*RWPLUGINOFFSET(U32, ((RpAtomic*)object)->geometry, _rpDlGeomVtxFmtOffset),
                         gcPipeData);
    }
    else
    {
        ltm = NULL;
        _rwDlVtxFmtSetup(*RWPLUGINOFFSET(U32, RWSRCGLOBAL(curWorld), _rpDlWorldVtxFmtOffset),
                         gcPipeData);
    }

    if (gcPipeData->flags & rpGEOMETRYNORMALS)
    {
        _rwDlTransformSetup(ltm, TRUE);
    }
    else
    {
        _rwDlTransformSetup(ltm, FALSE);
    }

    rwGameCubeMatColCallBack matColCB = _rwDlObjectRenderSetup(
        gcPipeData->flags, gcPipeData->unk_20, gcPipeData->unk_1c, header->flags & 1);

    // NOTE: the scale is written into the constant column of the 2x4 texture
    // matrix rather than scaling the rotation, exactly as retail does.
    Mtx uvMtx = { 0 };
    uvMtx[0][0] = xFXanimUVRotMat0[0];
    uvMtx[0][1] = xFXanimUVRotMat0[1];
    uvMtx[0][2] = xFXanimUVTrans[0];
    uvMtx[0][3] = xFXanimUVScale[0];
    uvMtx[1][0] = xFXanimUVRotMat1[0];
    uvMtx[1][1] = xFXanimUVRotMat1[1];
    uvMtx[1][2] = xFXanimUVTrans[1];
    uvMtx[1][3] = xFXanimUVScale[1];

    GXLoadTexMtxImm(uvMtx, GX_TEXMTX0, GX_MTX2x4);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);

    RpMeshHeader* meshHeader = gcPipeData->meshHeader;
    U32 numMeshes = meshHeader->numMeshes;
    RpMesh* mesh = (RpMesh*)(meshHeader + 1);

    if (gcPipeData->flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        while (numMeshes--)
        {
            RwTexture* texture = mesh->material->texture;
            _rwDlTextureSet(texture, 0);
            if (texture != NULL)
            {
                RwRaster* raster = texture->raster;
                if (raster != NULL)
                {
                    RwGameCubeRasterExt* ext = RWPLUGINOFFSET(RwGameCubeRasterExt, raster->parent,
                                                              _RwGameCubeRasterExtOffset);
                    _rwDlRenderStateSetZCompLoc((ext->flags & 1) ^ 1);
                }
            }
            if (matColCB != NULL)
            {
                matColCB(&gcPipeData->matCol, &mesh->material->color,
                         mesh->material->surfaceProps.ambient);
            }
            GXCallDisplayList(dlist->data, dlist->size);
            dlist++;
            mesh++;
        }
    }
    else
    {
        while (numMeshes--)
        {
            if (matColCB != NULL)
            {
                matColCB(&gcPipeData->matCol, &mesh->material->color,
                         mesh->material->surfaceProps.ambient);
            }
            GXCallDisplayList(dlist->data, dlist->size);
            dlist++;
            mesh++;
        }
    }

    return object;
}

RxPipeline* iFXanimUVCreatePipe()
{
    RxPipeline* newPipe = RxPipelineCreate();

    if (newPipe != NULL)
    {
        RxLockedPipe* lPipe = RxPipelineLock(newPipe);

        if (lPipe != NULL)
        {
            RxNodeDefinition* gcAll = RxNodeDefinitionGetGameCubeAtomicAllInOne();
            RxPipelineNode* plNode;

            lPipe = RxLockedPipeAddFragment(lPipe, NULL, gcAll, NULL);
            RxLockedPipeUnlock(lPipe);

            plNode = RxPipelineFindNodeByName(newPipe, gcAll->name, NULL, NULL);
            RxGameCubeAllInOneSetRenderCallBack(plNode, _iGCUVRenderCallback);

            return newPipe;
        }

        _rxPipelineDestroy(newPipe);
        newPipe = NULL;
    }

    return newPipe;
}
