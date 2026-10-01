#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

RwBool _RwDlPreInstanceOptimize = TRUE;

void _rxGCResEntryWaitDone(RwResEntry* resEntry)
{
    RxGameCubeVertexBuffer* vbHeader;

    vbHeader = (RxGameCubeVertexBuffer*)(resEntry + 1);

    if (vbHeader->token == _RwDlTokenCurrent)
    {
        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;
    }

    while (!_rwDlTokenQueryDone(vbHeader->token))
    {
    }

    GXInvalidateVtxCache();
}

void* _rxGCDefaultRenderCallback(void* object, RxGameCubePipeData* pipeData)
{
    RwUInt32 numMeshes;
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* dList;
    RpMesh* mesh;
    RwMatrix* ltm;
    RwDlMatFunc matFunc;
    RpAtomic* atomic;
    RwTexture* texture;

    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);
    dList = (RxGameCubeDisplayList*)((RxGameCubeVertexAttr*)(vbHeader + 1) +
                                    (vbHeader->numAttrArrays - 1));

    vbHeader->token = _RwDlTokenCurrent;

    if (RwObjectGetType(object) == rpATOMIC)
    {
        atomic = (RpAtomic*)object;
        ltm = RwFrameGetLTM(RpAtomicGetFrame(atomic));
        _rwDlVtxFmtSetup(GEOMVTXFMT(atomic->geometry), pipeData);
    }
    else
    {
        ltm = NULL;
        _rwDlVtxFmtSetup(WORLDVTXFMT(RWSRCGLOBAL(curWorld)), pipeData);
    }

    if (pipeData->flags & rpGEOMETRYNORMALS)
    {
        _rwDlTransformSetup(ltm, TRUE);
    }
    else
    {
        _rwDlTransformSetup(ltm, FALSE);
    }

    matFunc = _rwDlObjectRenderSetup(pipeData->flags, pipeData->lightMask, pipeData->ambientLight,
                                     vbHeader->flags & 1);

    numMeshes = pipeData->meshHeader->numMeshes;
    mesh = (RpMesh*)(pipeData->meshHeader + 1);

    if (pipeData->flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        while (numMeshes--)
        {
            texture = mesh->material->texture;
            _rwDlTextureSet(texture, 0);

            if (texture != NULL && texture->raster != NULL)
            {
                _rwDlRenderStateSetZCompLoc(
                    (RASTEREXTFROMRASTER(RwRasterGetParent(texture->raster))->flags & 1) ^ 1);
            }

            if (matFunc != NULL)
            {
                matFunc(&pipeData->ambientLightColor, &mesh->material->color,
                        mesh->material->surfaceProps.ambient);
            }

            GXCallDisplayList(dList->displayList, dList->size);

            dList++;
            mesh++;
        }
    }
    else
    {
        while (numMeshes--)
        {
            if (matFunc != NULL)
            {
                matFunc(&pipeData->ambientLightColor, &mesh->material->color,
                        mesh->material->surfaceProps.ambient);
            }

            GXCallDisplayList(dList->displayList, dList->size);

            dList++;
            mesh++;
        }
    }

    return object;
}

void _rxGameCubeAllInOneSetInstanceCallBack(RxPipelineNode* node, RxGameCubeAllInOneCallBack callback)
{
    _rxGameCubeAllInOneNodeData* nodeData;

    nodeData = (_rxGameCubeAllInOneNodeData*)node->privateData;
    nodeData->instanceCallback = callback;
}

RxGameCubeAllInOneCallBack _rxGameCubeAllInOneGetInstanceCallBack(RxPipelineNode* node)
{
    _rxGameCubeAllInOneNodeData* nodeData;

    nodeData = (_rxGameCubeAllInOneNodeData*)node->privateData;
    return nodeData->instanceCallback;
}

void _rxGameCubeAllInOneSetReinstanceCallBack(RxPipelineNode* node,
                                              RxGameCubeAllInOneCallBack callback)
{
    _rxGameCubeAllInOneNodeData* nodeData;

    nodeData = (_rxGameCubeAllInOneNodeData*)node->privateData;
    nodeData->reinstanceCallback = callback;
}

RxGameCubeAllInOneCallBack _rxGameCubeAllInOneGetReinstanceCallBack(RxPipelineNode* node)
{
    _rxGameCubeAllInOneNodeData* nodeData;

    nodeData = (_rxGameCubeAllInOneNodeData*)node->privateData;
    return nodeData->reinstanceCallback;
}

void RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node, RxGameCubeAllInOneCallBack callback)
{
    _rxGameCubeAllInOneNodeData* nodeData;

    nodeData = (_rxGameCubeAllInOneNodeData*)node->privateData;
    nodeData->renderCallback = callback;
}

void RxGameCubePreInstanceSetOptimize(RwBool optimize)
{
    _RwDlPreInstanceOptimize = optimize;
}

RwBool RxGameCubePreInstanceGetOptimize(void)
{
    return _RwDlPreInstanceOptimize;
}
