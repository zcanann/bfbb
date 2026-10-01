#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"
#include "rwsdk/plugin/skin2/skin.h"

#define rwMatrixInitialize(_m, _t) ((_m)->flags = (RwUInt32)(_t))

#define SKINALIGN4(_ptr) ((RwUInt8*)(((RwUInt32)(_ptr) + 3) & ~3))
#define SKINALIGN32(_size) (((_size) + 31) & ~31)

/* The vertex arrays are held in a child resource entry: a token, then the
 * owning entry just before the 32 byte aligned array */
#define SKINVTXARRAYHEADERSIZE (sizeof(RwUInt16) + sizeof(RwResEntry*))
#define SKINVTXARRAYGETRESENTRY(_array) (((RwResEntry**)(_array))[-1])

typedef void (*RxGameCubeAllInOneRenderCallBack)(RwResEntry* repEntry, void* object,
                                                 RwUInt8 type, RwUInt32 flags);

extern RxPipelineNode* RxGameCubeAllInOneSetRenderCallBack(RxPipelineNode* node,
                                                           RxGameCubeAllInOneCallBack cb);
extern RxPipelineNode* _rxGameCubeAllInOneSetInstanceCallBack(RxPipelineNode* node,
                                                              RxGameCubeAllInOneCallBack cb);
extern RxPipelineNode* _rxGameCubeAllInOneSetReinstanceCallBack(RxPipelineNode* node,
                                                                RxGameCubeAllInOneCallBack cb);
extern RxGameCubeAllInOneCallBack _rxGameCubeAllInOneGetReinstanceCallBack(RxPipelineNode* node);

extern RwResEntry* _rwDlGeometrySkinInstanceOptimized(RpGeometry* geometry, void* owner,
                                                      RwResEntry** resEntryOwner);
extern RwResEntry* _rwDlGeometrySkinInstanceFast(RpGeometry* geometry, void* owner,
                                                 RwResEntry** resEntryOwner);

extern RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                               RwResEntryDestroyNotify destroyNotify);

extern RwMatrix _RwDlInvCamLTM;

extern void _rpSkinMatrixBlendUpdateASM(RwMatrix* dstMat, const RwMatrix* invBoneToSkinMat,
                                        const RwMatrix* boneMatrices, const RwMatrix* invLTM,
                                        const RwUInt8* usedBoneList, RwUInt32 numUsedBones);

extern void _rwDlSkinUpdate2Weights(const RpSkin* skin, const RwMatrix* matrixCache, void* vertices,
                                    void* normals, RwUInt32 numVertices, RwUInt32 vertexSize,
                                    RwUInt32 normalSize, RwUInt32 normalPad);
extern void _rwDlSkinUpdate3Weights(const RpSkin* skin, const RwMatrix* matrixCache, void* vertices,
                                    void* normals, RwUInt32 numVertices, RwUInt32 vertexSize,
                                    RwUInt32 normalSize, RwUInt32 normalPad);
extern void _rwDlSkinUpdate4Weights(const RpSkin* skin, const RwMatrix* matrixCache, void* vertices,
                                    void* normals, RwUInt32 numVertices, RwUInt32 vertexSize,
                                    RwUInt32 normalSize, RwUInt32 normalPad);

extern void _rpSkinBlendBody(RpSkin* skin, RwMatrix* matrixCache, void* vertices, void* normals,
                             RpGameCubeVtxFmt* vtxFmt, RwInt32 numVertices);

SkinGlobals _rpSkinGlobals = { 0, 0, 0, { (RwMatrix*)NULL, NULL }, 0, (RwFreeList*)NULL,
                               { 0, 0 }, { { (RxPipeline*)NULL } }, (SkinSplitData*)NULL };

static RxGameCubeAllInOneCallBack _RwDlDefaultReinstanceCallBack;

static void _rpSkinMainResEntryCB(RwResEntry* resEntry)
{
    RxGameCubeVertexBuffer* vbHeader = (RxGameCubeVertexBuffer*)(resEntry + 1);
    RpGeometry* geometry;
    RpSkin* skin;

    RXGCVERTEXBUFFERWAITDONE(vbHeader);

    if (vbHeader->attr[0].array)
    {
        RwResourcesFreeResEntry(SKINVTXARRAYGETRESENTRY(vbHeader->attr[0].array));
    }

    /* Give the original vertex buffers back to the main entry */
    if (RwObjectGetType(resEntry->owner) == rpATOMIC)
    {
        geometry = ((RpAtomic*)resEntry->owner)->geometry;
        skin = *RPSKINGEOMETRYGETDATA(geometry);

        vbHeader->attr[0].array = skin->platformData.vertices;
        if (geometry->flags & rpGEOMETRYNORMALS)
        {
            vbHeader->attr[1].array = skin->platformData.normals;
        }
    }
    else
    {
        geometry = (RpGeometry*)resEntry->owner;
        skin = *RPSKINGEOMETRYGETDATA(geometry);

        vbHeader->attr[0].array = skin->platformData.vertices;
        if (geometry->flags & rpGEOMETRYNORMALS)
        {
            vbHeader->attr[1].array = skin->platformData.normals;
        }
    }

    skin->platformData.vertices = NULL;
    skin->platformData.normals = NULL;

    GXInvalidateVtxCache();
}

static void _rpSkinResEntryWaitDone(RwResEntry* resEntry)
{
    RxGameCubeVertexBuffer* vbHeader = (RxGameCubeVertexBuffer*)(resEntry + 1);

    RXGCVERTEXBUFFERWAITDONE(vbHeader);

    GXInvalidateVtxCache();
}

RwBool _rpSkinVertexBuffersUpdate(RpSkin* skin, RpAtomic* atomic, RxGameCubeVertexBuffer* vbHeader,
                                  RxGameCubePipeData* pipeData)
{
    RwResEntry* childResEntry;
    RwResEntry* parentResEntry;
    RpGeometry* geometry = atomic->geometry;
    RwUInt32 size;
    RpGameCubeVtxFmt* vtxFmt;

    RwResourcesUseResEntry(pipeData->resEntry);
    pipeData->resEntry->destroyNotify = _rpSkinMainResEntryCB;

    /* Keep hold of the original (unskinned) buffers */
    if (!skin->platformData.vertices)
    {
        skin->platformData.vertices = vbHeader->attr[0].array;
        vbHeader->attr[0].array = NULL;

        if (geometry->flags & rpGEOMETRYNORMALS)
        {
            skin->platformData.normals = vbHeader->attr[1].array;
            vbHeader->attr[1].array = NULL;
        }
    }

    /* The skinned buffers can't be reused while the GPU is still reading them */
    if (vbHeader->attr[0].array)
    {
        if (!_rwDlTokenQueryDone(
                ((RxGameCubeVertexBuffer*)(SKINVTXARRAYGETRESENTRY(vbHeader->attr[0].array) + 1))
                    ->token))
        {
            SKINVTXARRAYGETRESENTRY(vbHeader->attr[0].array)->ownerRef = (RwResEntry**)NULL;
            vbHeader->attr[0].array = NULL;

            if (geometry->flags & rpGEOMETRYNORMALS)
            {
                vbHeader->attr[1].array = NULL;
            }
        }
        else
        {
            GXInvalidateVtxCache();
        }
    }

    if (!vbHeader->attr[0].array)
    {
        vtxFmt = GEOMVTXFMT(geometry);

        if (vtxFmt)
        {
            RwUInt32 vtxSizeConvTable[5] = { 3, 3, 6, 6, 12 };

            size = SKINALIGN32(geometry->numVertices * vtxSizeConvTable[vtxFmt->pos]) +
                   SKINVTXARRAYHEADERSIZE;

            if (geometry->flags & rpGEOMETRYNORMALS)
            {
                size += SKINALIGN32(geometry->numVertices *
                                    (vtxSizeConvTable[vtxFmt->norm] * (vtxFmt->nbt ? 3 : 1)));
            }
        }
        else
        {
            size = SKINALIGN32(geometry->numVertices * sizeof(RwV3d)) + SKINVTXARRAYHEADERSIZE;

            if (geometry->flags & rpGEOMETRYNORMALS)
            {
                size += SKINALIGN32(geometry->numVertices * sizeof(RwV3d));
            }
        }

        childResEntry = RwResourcesAllocateResEntry(atomic, (RwResEntry**)&vbHeader->attr[0].array,
                                                    size, _rpSkinResEntryWaitDone);

        if (geometry->numMorphTargets == 1)
        {
            parentResEntry = geometry->repEntry;
        }
        else
        {
            parentResEntry = atomic->repEntry;
        }

        if (!childResEntry || !parentResEntry)
        {
            if (childResEntry)
            {
                RwResourcesFreeResEntry(childResEntry);
            }

            return FALSE;
        }

        vbHeader->attr[0].array =
            (void*)(((RwUInt32)vbHeader->attr[0].array + sizeof(RwResEntry) +
                     SKINVTXARRAYHEADERSIZE + 31) & ~31);

        if (geometry->flags & rpGEOMETRYNORMALS)
        {
            vbHeader->attr[1].array =
                (RwUInt8*)vbHeader->attr[0].array +
                SKINALIGN32(geometry->numVertices * vbHeader->attr[0].stride);
        }

        SKINVTXARRAYGETRESENTRY(vbHeader->attr[0].array) = childResEntry;
    }
    else
    {
        RwResourcesUseResEntry(SKINVTXARRAYGETRESENTRY(vbHeader->attr[0].array));
    }

    ((RxGameCubeVertexBuffer*)(SKINVTXARRAYGETRESENTRY(vbHeader->attr[0].array) + 1))->token =
        _RwDlTokenCurrent;

    return TRUE;
}

void _rpSkinMatrixBlendUpdate(RwMatrix* matDst, const RpSkin* skin, const RwMatrix* ltm,
                              RpHAnimHierarchy* hierarchy)
{
    if (hierarchy)
    {
        if (hierarchy->flags & rpHANIMHIERARCHYNOMATRICES)
        {
            RwUInt32 i;
            RwMatrix invLTM;
            const RwMatrix* pInvLTM;

            if (skin->vertexMaps.maxWeights > 1)
            {
                rwMatrixInitialize(&invLTM, 0);
                RwMatrixInvert(&invLTM, ltm);
                pInvLTM = &invLTM;
            }
            else
            {
                pInvLTM = &_RwDlInvCamLTM;
            }

            for (i = 0; i < skin->boneData.numUsedBones; i++)
            {
                RwMatrix tmpMatrix;

                rwMatrixInitialize(&tmpMatrix, 0);

                RwMatrixMultiply(&tmpMatrix, &skin->boneData.invBoneToSkinMat[skin->boneData.usedBoneList[i]],
                                 RwFrameGetLTM(hierarchy->pNodeInfo[skin->boneData.usedBoneList[i]].pFrame));

                RwMatrixMultiply(&matDst[skin->boneData.usedBoneList[i]], &tmpMatrix, pInvLTM);
            }
        }
        else if (hierarchy->flags & rpHANIMHIERARCHYLOCALSPACEMATRICES)
        {
            if (skin->vertexMaps.maxWeights > 1)
            {
                RwUInt32 i;

                for (i = 0; i < skin->boneData.numUsedBones; i++)
                {
                    RwMatrixMultiply(&matDst[skin->boneData.usedBoneList[i]], &skin->boneData.invBoneToSkinMat[skin->boneData.usedBoneList[i]],
                                     &hierarchy->pMatrixArray[skin->boneData.usedBoneList[i]]);
                }
            }
            else
            {
                RwMatrix camLTM;

                rwMatrixInitialize(&camLTM, 0);
                RwMatrixMultiply(&camLTM, ltm, &_RwDlInvCamLTM);

                _rpSkinMatrixBlendUpdateASM(matDst, skin->boneData.invBoneToSkinMat,
                                            hierarchy->pMatrixArray, &camLTM,
                                            skin->boneData.usedBoneList,
                                            skin->boneData.numUsedBones);
            }
        }
        else
        {
            RwMatrix invLTM;
            const RwMatrix* pInvLTM;

            if (skin->vertexMaps.maxWeights > 1)
            {
                rwMatrixInitialize(&invLTM, 0);
                RwMatrixInvert(&invLTM, ltm);
                pInvLTM = &invLTM;
            }
            else
            {
                pInvLTM = &_RwDlInvCamLTM;
            }

            _rpSkinMatrixBlendUpdateASM(matDst, skin->boneData.invBoneToSkinMat,
                                        hierarchy->pMatrixArray, pInvLTM,
                                        skin->boneData.usedBoneList, skin->boneData.numUsedBones);
        }
    }
}

void _rpSkinBlendBody(RpSkin* skin, RwMatrix* matrixCache, void* vertices, void* normals,
                      RpGameCubeVtxFmt* vtxFmt, RwInt32 numVertices)
{
    RwUInt32 gqr5value;
    register RwUInt32 tmp;
    register RwUInt32 weightGQR;
    register RwUInt32 posGQR;
    register RwUInt32 normGQR;
    RwUInt32 vertexSize;
    RwUInt32 normalSize;
    RwUInt32 normalPad;

    /* Weights are unsigned bytes scaled by 1/128 */
    weightGQR = 0x07040000;
    posGQR = 0;
    normGQR = 0;

    if (vtxFmt != NULL)
    {
        RwUInt8 vtxFmtTypeConvTable[5] = { 4, 6, 5, 7, 0 };
        RwUInt8 vtxFmtSizeConvTable[5] = { 1, 1, 2, 2, 4 };
        RwUInt8 vtxFmtNormConvTable[5] = { 0, 6, 0, 14, 0 };

        vertexSize = vtxFmtSizeConvTable[vtxFmt->pos];

        posGQR = vtxFmtTypeConvTable[vtxFmt->pos] | (vtxFmt->posFrac << 8);
        posGQR |= posGQR << 16;

        normalSize = vtxFmtSizeConvTable[vtxFmt->norm];

        normGQR = vtxFmtTypeConvTable[vtxFmt->norm] | (vtxFmtNormConvTable[vtxFmt->norm] << 8);
        normGQR |= normGQR << 16;

        normalPad = vtxFmt->nbt ? normalSize * 6 : 0;
    }
    else
    {
        vertexSize = sizeof(RwReal);
        normalSize = sizeof(RwReal);
        normalPad = 0;
    }

    asm
    {
        mfspr tmp, GQR5
        stw tmp, gqr5value
        mtspr GQR5, weightGQR
        mtspr GQR6, posGQR
        mtspr GQR7, normGQR
    }

    /* Skin positions only, in place, when there are no normals */
    if (skin->platformData.normals == NULL)
    {
        skin->platformData.normals = skin->platformData.vertices;
    }

    switch (skin->vertexMaps.maxWeights)
    {
    case 1:
        break;
    case 2:
        _rwDlSkinUpdate2Weights(skin, matrixCache, vertices, normals, numVertices, vertexSize,
                                normalSize, normalPad);
        break;
    case 3:
        _rwDlSkinUpdate3Weights(skin, matrixCache, vertices, normals, numVertices, vertexSize,
                                normalSize, normalPad);
        break;
    case 4:
        _rwDlSkinUpdate4Weights(skin, matrixCache, vertices, normals, numVertices, vertexSize,
                                normalSize, normalPad);
        break;
    }

    asm
    {
        lwz tmp, gqr5value
        mtspr GQR5, tmp
    }

    if (skin->platformData.normals == skin->platformData.vertices)
    {
        skin->platformData.normals = NULL;
        DCFlushRange(vertices, numVertices * (vertexSize * 3));
    }
    else
    {
        DCFlushRange(vertices, SKINALIGN32(numVertices * (vertexSize * 3)) +
                                   numVertices * (normalPad + normalSize * 3));
    }
}

void* _rpSkinInstanceCallback(void* object, RxGameCubePipeData* pipeData)
{
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = RpSkinGeometryGetSkin(geometry);
    void* owner;
    RwResEntry** ownerRef;

    if (geometry->numMorphTargets != 1)
    {
        owner = atomic;
        ownerRef = &atomic->repEntry;
    }
    else
    {
        owner = geometry;
        ownerRef = &geometry->repEntry;
    }

    if (skin->vertexMaps.maxWeights > 1)
    {
        /* Skinned on the CPU, so the vertices are instanced as normal */
        if (geometry->flags & rpGEOMETRYNATIVEINSTANCE)
        {
            if (_RwDlPreInstanceOptimize == TRUE)
            {
                pipeData->resEntry = _rwDlGeometrySkinInstanceOptimized(geometry, owner, ownerRef);
            }
            else
            {
                pipeData->resEntry = _rwDlGeometryInstanceFast(geometry, owner, ownerRef);
            }
        }
        else
        {
            pipeData->resEntry = _rwDlGeometryInstanceFast(geometry, owner, ownerRef);
        }
    }
    else
    {
        if (geometry->flags & rpGEOMETRYNATIVEINSTANCE)
        {
            if (_RwDlPreInstanceOptimize == TRUE)
            {
                pipeData->resEntry = _rwDlGeometrySkinInstanceOptimized(geometry, owner, ownerRef);
            }
            else
            {
                pipeData->resEntry = _rwDlGeometrySkinInstanceFast(geometry, owner, ownerRef);
            }
        }
        else
        {
            pipeData->resEntry = _rwDlGeometrySkinInstanceFast(geometry, owner, ownerRef);
        }
    }

    geometry->lockedSinceLastInst = 0;

    return object;
}

void* _rpSkinAtomicReinstanceCallBack(void* object, RxGameCubePipeData* pipeData)
{
    RxGameCubeVertexBuffer* vbHeader;
    RpAtomic* atomic = (RpAtomic*)object;
    RpGeometry* geometry = atomic->geometry;
    RpSkin* skin = *RPSKINGEOMETRYGETDATA(geometry);

    if (skin->vertexMaps.maxWeights > 1)
    {
        vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);

        if (!skin->platformData.vertices)
        {
            if (!(geometry->flags & rpGEOMETRYNATIVE))
            {
                _RwDlDefaultReinstanceCallBack(object, pipeData);
            }

            skin->platformData.vertices = vbHeader->attr[0].array;
            vbHeader->attr[0].array = NULL;

            if (geometry->flags & rpGEOMETRYNORMALS)
            {
                skin->platformData.normals = vbHeader->attr[1].array;
                vbHeader->attr[1].array = NULL;
            }
        }
        else if (!(geometry->flags & rpGEOMETRYNATIVE))
        {
            /* Reinstance into the original buffers */
            void* positions = vbHeader->attr[0].array;

            vbHeader->attr[0].array = skin->platformData.vertices;

            if (geometry->flags & rpGEOMETRYNORMALS)
            {
                void* normals = vbHeader->attr[1].array;

                vbHeader->attr[1].array = skin->platformData.normals;
                _RwDlDefaultReinstanceCallBack(object, pipeData);
                vbHeader->attr[1].array = normals;
            }
            else
            {
                _RwDlDefaultReinstanceCallBack(object, pipeData);
            }

            vbHeader->attr[0].array = positions;
        }

        if (!_rpSkinVertexBuffersUpdate(skin, atomic, vbHeader, pipeData))
        {
            return NULL;
        }

        _rpSkinMatrixBlendUpdate(_rpSkinGlobals.matrixCache.aligned, skin,
                                 RwFrameGetLTM((RwFrame*)rwObjectGetParent(atomic)),
                                 *RPSKINATOMICGETDATA(atomic));

        _rpSkinBlendBody(skin, _rpSkinGlobals.matrixCache.aligned, vbHeader->attr[0].array,
                         (geometry->flags & rpGEOMETRYNORMALS) ? vbHeader->attr[1].array : NULL,
                         GEOMVTXFMT(geometry), geometry->numVertices);
    }
    else
    {
        _rpSkinMatrixBlendUpdate(_rpSkinGlobals.matrixCache.aligned, skin,
                                 RwFrameGetLTM((RwFrame*)rwObjectGetParent(atomic)),
                                 *RPSKINATOMICGETDATA(atomic));
    }

    return object;
}

/* Load a bone matrix into a hardware matrix slot (column major, right handed) */
#define SKINLOADBONEMATRIX(_mtx, _matrix, _index, _normals)                                        \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwBool _loadNormals = (_normals);                                                          \
                                                                                                   \
        (_mtx)[0][0] = -(_matrix)->right.x;                                                        \
        (_mtx)[0][1] = -(_matrix)->up.x;                                                           \
        (_mtx)[0][2] = -(_matrix)->at.x;                                                           \
        (_mtx)[0][3] = -(_matrix)->pos.x;                                                          \
        (_mtx)[1][0] = (_matrix)->right.y;                                                         \
        (_mtx)[1][1] = (_matrix)->up.y;                                                            \
        (_mtx)[1][2] = (_matrix)->at.y;                                                            \
        (_mtx)[1][3] = (_matrix)->pos.y;                                                           \
        (_mtx)[2][0] = -(_matrix)->right.z;                                                        \
        (_mtx)[2][1] = -(_matrix)->up.z;                                                           \
        (_mtx)[2][2] = -(_matrix)->at.z;                                                           \
        (_mtx)[2][3] = -(_matrix)->pos.z;                                                          \
                                                                                                   \
        GXLoadPosMtxImm((_mtx), (_index));                                                         \
                                                                                                   \
        if (_loadNormals)                                                                          \
        {                                                                                          \
            GXLoadNrmMtxImm((_mtx), (_index));                                                     \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

void* _rpSkinRenderCallback(void* object, RxGameCubePipeData* pipeData)
{
    RwUInt32 numMeshes;
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* dList;
    RpSkin* skin;
    RpMesh* mesh;
    RwMatrix* ltm;
    RpGameCubeVtxFmt* fmt;
    RwDlMatFunc matFunc;
    RwUInt32 i;
    RpAtomic* atomic;
    RwTexture* texture;
    RwGameCubeRasterExtension* rasExt;

    atomic = (RpAtomic*)object;

    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);
    dList = (RxGameCubeDisplayList*)((RxGameCubeVertexAttr*)(vbHeader + 1) +
                                    (vbHeader->numAttrArrays - 1));

    vbHeader->token = _RwDlTokenCurrent;

    ltm = RwFrameGetLTM(RpAtomicGetFrame(atomic));
    _rwDlVtxFmtSetup(GEOMVTXFMT(atomic->geometry), pipeData);

    skin = RpSkinGeometryGetSkin(atomic->geometry);

    if (skin->vertexMaps.maxWeights > 1)
    {
        /* Vertices were blended on the CPU */
        _rwDlTransformSetup(ltm, (pipeData->flags & rpGEOMETRYNORMALS) ? TRUE : FALSE);
    }
    else
    {
        /* Each vertex selects its bone matrix */
        GXSetVtxDesc(GX_VA_PNMTXIDX, GX_DIRECT);
    }

    fmt = GEOMVTXFMT(atomic->geometry);
    if (fmt == NULL)
    {
        fmt = _rpGameCubeVtxFmtGetDefault();
    }

    matFunc = _rwDlObjectRenderSetup(pipeData->flags, pipeData->lightMask, pipeData->ambientLight,
                                     vbHeader->flags & 1);

    numMeshes = pipeData->meshHeader->numMeshes;
    mesh = (RpMesh*)(pipeData->meshHeader + 1);

    if (skin->vertexMaps.maxWeights == 1 && skin->skinSplitData.numMeshes == 0)
    {
        for (i = 0; i < skin->boneData.numUsedBones; i++)
        {
            f32 mtx[3][4];
            RwMatrix* matrix;

            matrix = &_rpSkinGlobals.matrixCache.aligned[skin->boneData.usedBoneList[i]];
            SKINLOADBONEMATRIX(mtx, matrix, i * 3,
                               (pipeData->flags & rpGEOMETRYNORMALS) ? TRUE : FALSE);
        }
    }

    if (pipeData->flags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        while (numMeshes--)
        {
            texture = mesh->material->texture;
            _rwDlTextureSet(texture, 0);

            if (texture != NULL && texture->raster != NULL)
            {
                rasExt = RASTEREXTFROMRASTER(RwRasterGetParent(texture->raster));
                _rwDlRenderStateSetZCompLoc((rasExt->flags & 1) ^ 1);
            }

            if (matFunc != NULL)
            {
                matFunc(&pipeData->ambientLightColor, &mesh->material->color,
                        mesh->material->surfaceProps.ambient);
            }

            if (skin->vertexMaps.maxWeights == 1 && skin->skinSplitData.numMeshes != 0)
            {
                RwUInt8* rleCount;
                RwUInt8 index;
                RwUInt8 count;
                RwUInt8 j;
                RwUInt32 slot;
                RwBool normals;

                slot = 0;
                normals = (pipeData->flags & rpGEOMETRYNORMALS) ? TRUE : FALSE;

                rleCount = &skin->skinSplitData
                                .meshRLECount[(pipeData->meshHeader->numMeshes - (numMeshes + 1)) * 2];
                index = rleCount[0] * 2;
                count = rleCount[1];

                for (j = 0; j < count; j++)
                {
                    RwUInt8* rle;
                    RwUInt32 bone;
                    RwUInt32 run;
                    RwUInt32 k;

                    rle = &skin->skinSplitData.meshRLE[index + j * 2];
                    bone = rle[0];
                    run = rle[1];

                    for (k = 0; k < run; k++, bone++)
                    {
                        f32 mtx[3][4];
                        RwMatrix* matrix;

                        matrix = &_rpSkinGlobals.matrixCache.aligned[bone];
                        SKINLOADBONEMATRIX(mtx, matrix, slot, normals);
                        slot += 3;
                    }
                }
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

            if (skin->vertexMaps.maxWeights == 1 && skin->skinSplitData.numMeshes != 0)
            {
                RwUInt8* rleCount;
                RwUInt8 index;
                RwUInt8 count;
                RwUInt8 j;
                RwUInt32 slot;
                RwBool normals;

                slot = 0;
                normals = (pipeData->flags & rpGEOMETRYNORMALS) ? TRUE : FALSE;

                rleCount = &skin->skinSplitData
                                .meshRLECount[(pipeData->meshHeader->numMeshes - (numMeshes + 1)) * 2];
                index = rleCount[0] * 2;
                count = rleCount[1];

                for (j = 0; j < count; j++)
                {
                    RwUInt8* rle;
                    RwUInt32 bone;
                    RwUInt32 run;
                    RwUInt32 k;

                    rle = &skin->skinSplitData.meshRLE[index + j * 2];
                    bone = rle[0];
                    run = rle[1];

                    for (k = 0; k < run; k++, bone++)
                    {
                        f32 mtx[3][4];
                        RwMatrix* matrix;

                        matrix = &_rpSkinGlobals.matrixCache.aligned[bone];
                        SKINLOADBONEMATRIX(mtx, matrix, slot, normals);
                        slot += 3;
                    }
                }
            }

            GXCallDisplayList(dList->displayList, dList->size);

            dList++;
            mesh++;
        }
    }

    return object;
}

static RpSkin* _rpSkinCreate(RpSkin* skin, RwUInt32 numVertices)
{
    RwUInt32 i;
    RwUInt32 j;
    RwUInt32 k;
    RwReal tmp;
    RwUInt32 index1;
    RwUInt32 index2;

    skin->platformData.vertices = NULL;
    skin->platformData.normals = NULL;
    skin->platformData.weights = (RwUInt8*)NULL;
    skin->platformData.indices = (RwUInt8*)NULL;

    if (skin->vertexMaps.maxWeights > 1)
    {
        RwUInt8* weights;
        RwUInt8* indices;

        /* Sort each vertex's weights, largest first */
        for (i = 0; i < numVertices; i++)
        {
            for (j = 0; j < rpSKINMAXWEIGHTS - 1; j++)
            {
                for (k = 1; k < rpSKINMAXWEIGHTS - j; k++)
                {
                    if ((&skin->vertexMaps.matrixWeights[i].w0 + j)[k] >
                        (&skin->vertexMaps.matrixWeights[i].w0)[j])
                    {
                        tmp = (&skin->vertexMaps.matrixWeights[i].w0)[j];
                        (&skin->vertexMaps.matrixWeights[i].w0)[j] =
                            (&skin->vertexMaps.matrixWeights[i].w0 + j)[k];
                        (&skin->vertexMaps.matrixWeights[i].w0 + j)[k] = tmp;

                        index1 = (skin->vertexMaps.matrixIndices[i] >> (j * 8)) & 0xFF;
                        index2 = (skin->vertexMaps.matrixIndices[i] >> ((j + k) * 8)) & 0xFF;
                        skin->vertexMaps.matrixIndices[i] &= ~(0xFF << (j * 8));
                        skin->vertexMaps.matrixIndices[i] |= index2 << (j * 8);
                        skin->vertexMaps.matrixIndices[i] &= ~(0xFF << ((j + k) * 8));
                        skin->vertexMaps.matrixIndices[i] |= index1 << ((j + k) * 8);
                    }
                }
            }
        }

        /* Weights are quantized to 1.7 */
        skin->platformData.weights =
            (RwUInt8*)RwMalloc(numVertices * (2 * skin->vertexMaps.maxWeights) + 3);

        weights = skin->platformData.weights;

        for (i = 0; i < numVertices; i++)
        {
            RwUInt32 j;
            RwUInt8 total = 0;

            for (j = 0; j < skin->vertexMaps.maxWeights; j++)
            {
                *weights = (RwUInt8)(128.0f * (&skin->vertexMaps.matrixWeights[i].w0)[j]);
                total += *weights;
                weights++;
            }

            /* Make up any rounding error */
            if (total < 128)
            {
                for (j = 0; j < skin->vertexMaps.maxWeights; j++)
                {
                    *(weights - (skin->vertexMaps.maxWeights - j)) += 1;
                    total++;

                    if (total == 128)
                    {
                        break;
                    }
                }
            }
        }

        skin->platformData.indices =
            skin->platformData.weights + skin->vertexMaps.maxWeights * numVertices;
        skin->platformData.indices = SKINALIGN4(skin->platformData.indices);

        indices = skin->platformData.indices;

        for (i = 0; i < numVertices; i++)
        {
            RwUInt32 j;

            for (j = 0; j < skin->vertexMaps.maxWeights; j++)
            {
                *indices++ = (RwUInt8)(skin->vertexMaps.matrixIndices[i] >> (j * 8));
            }
        }
    }

    return skin;
}

RpGeometry* _rpSkinInitialize(RpGeometry* geometry)
{
    RpSkin* skin = *RPSKINGEOMETRYGETDATA(geometry);

    if (skin)
    {
        if (geometry->flags & rpGEOMETRYNATIVE)
        {
            if (geometry->flags & rpGEOMETRYNORMALS)
            {
                geometry->lockedSinceLastInst |= rpGEOMETRYLOCKVERTICES | rpGEOMETRYLOCKNORMALS;
            }
            else
            {
                geometry->lockedSinceLastInst |= rpGEOMETRYLOCKVERTICES;
            }

            skin->platformData.vertices = NULL;
            skin->platformData.normals = NULL;
        }
        else
        {
            if (!_rpSkinCreate(skin, geometry->numVertices))
            {
                return (RpGeometry*)NULL;
            }
        }
    }

    return geometry;
}

RpGeometry* _rpSkinDeinitialize(RpGeometry* geometry)
{
    RpSkin* skin = *RPSKINGEOMETRYGETDATA(geometry);

    if (skin->platformData.weights)
    {
        RwFree(skin->platformData.weights);
        skin->platformData.weights = (RwUInt8*)NULL;
        skin->platformData.indices = (RwUInt8*)NULL;
    }

    skin->platformData.vertices = NULL;
    skin->platformData.normals = NULL;

    return geometry;
}

RxPipeline* _rpSkinPipelineCreate(RwUInt32 type, RxGameCubeAllInOneCallBack instanceCB,
                                  RxGameCubeAllInOneCallBack reinstanceCB,
                                  RxGameCubeAllInOneCallBack renderCB)
{
    RxPipeline* pipe;
    RxLockedPipe* lpipe;
    RxNodeDefinition* allInOne;
    RxPipelineNode* node;

    pipe = RxPipelineCreate();
    pipe->pluginId = rwID_SKINPLUGIN;
    pipe->pluginData = type;

    lpipe = RxPipelineLock(pipe);
    allInOne = RxNodeDefinitionGetGameCubeAtomicAllInOne();
    lpipe = RxLockedPipeAddFragment(lpipe, (RwUInt32*)NULL, allInOne, (RxNodeDefinition*)NULL);
    RxLockedPipeUnlock(lpipe);

    node = RxPipelineFindNodeByName(pipe, allInOne->name, (RxPipelineNode*)NULL, (RwInt32*)NULL);

    if (instanceCB)
    {
        _rxGameCubeAllInOneSetInstanceCallBack(node, instanceCB);
    }

    if (reinstanceCB)
    {
        _RwDlDefaultReinstanceCallBack = _rxGameCubeAllInOneGetReinstanceCallBack(node);
        _rxGameCubeAllInOneSetReinstanceCallBack(node, reinstanceCB);
    }

    if (renderCB)
    {
        RxGameCubeAllInOneSetRenderCallBack(node, renderCB);
    }

    return pipe;
}
