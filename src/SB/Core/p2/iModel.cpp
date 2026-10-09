#include "iModel.h"

#include <types.h>
#include <string.h>
#include <rwplcore.h>
#include <rpworld.h>
#include <rwsdk/rpskin.h>
#include <rphanim.h>
#include <rpusrdat.h>

#include "iAnim.h"
#include "xLightKit.h"
#include "zGlobals.h"
#include "xMath3.h"
#include "xModel.h"
#include "xMathInlines.h"

#define MAX2(a, b) ((a) >= (b) ? (a) : (b))
#define MAX3(a, b, c) (MAX2((a), MAX2((b), (c))))

#define IMODEL_MAX_ATOMICS 256
#define IMODEL_MAX_DIRECTIONAL_LIGHTS 4
#define IMODEL_MAX_MATERIALS 16

static U32 gLastAtomicCount;
static RpAtomic* gLastAtomicList[IMODEL_MAX_ATOMICS];
static RpLight* sEmptyDirectionalLight[IMODEL_MAX_DIRECTIONAL_LIGHTS];
static RpLight* sEmptyAmbientLight;
static RwRGBA sMaterialColor[IMODEL_MAX_MATERIALS];
static RwTexture* sMaterialTexture[IMODEL_MAX_MATERIALS];
static U8 sMaterialAlpha[IMODEL_MAX_MATERIALS];
static U32 sMaterialIdx;
static U32 sMaterialFlags;
static RpAtomic* sLastMaterial;

static RwFrame* GetChildFrameHierarchy(RwFrame* frame, void* data)
{
    RpHAnimHierarchy* hierarchy = RpHAnimFrameGetHierarchy(frame);
    if (hierarchy == NULL)
    {
        RwFrameForAllChildren(frame, GetChildFrameHierarchy, data);
        return frame;
    }

    *(RpHAnimHierarchy**)data = hierarchy;
    return NULL;
}

static RpHAnimHierarchy* GetHierarchy(RpAtomic* model)
{
    RpHAnimHierarchy* hierarchy = NULL;
    GetChildFrameHierarchy(RpAtomicGetFrame(model), &hierarchy);
    return hierarchy;
}

void iModelInit()
{
    RwFrame* frame;
    S32 i;

    iModelInitFastPipes();

    RwRGBAReal black = { 0.0f, 0.0f, 0.0f, 0.0f };
    if (sEmptyDirectionalLight[0] == NULL)
    {
        for (i = 0; i < IMODEL_MAX_DIRECTIONAL_LIGHTS; i++)
        {
            sEmptyDirectionalLight[i] = RpLightCreate(rpLIGHTDIRECTIONAL);
            RpLightSetColor(sEmptyDirectionalLight[i], &black);
            frame = RwFrameCreate();
            _rwObjectHasFrameSetFrame(sEmptyDirectionalLight[i], frame);
        }
        sEmptyAmbientLight = RpLightCreate(rpLIGHTAMBIENT);
        RpLightSetColor(sEmptyAmbientLight, &black);
    }
}

static RpAtomic* FindAtomicCallback(RpAtomic* atomic, void*)
{
    RpHAnimHierarchy* pHier = GetHierarchy(atomic);
    RpSkin* pSkin = RpSkinGeometryGetSkin(atomic->geometry);

    if (pSkin != NULL && pHier == NULL)
    {
        pHier = RpHAnimHierarchyCreate(RpSkinGetNumBones(pSkin), NULL, NULL,
                                       rpHANIMHIERARCHYLOCALSPACEMATRICES, 0x24);
        RpHAnimFrameSetHierarchy(RpAtomicGetFrame(atomic), pHier);
    }
    if (pHier != NULL && pSkin != NULL)
    {
        RpSkinAtomicSetHAnimHierarchy(atomic, pHier);
    }
    if (pHier != NULL)
    {
        pHier->flags = rpHANIMHIERARCHYLOCALSPACEMATRICES;
    }
    if (gLastAtomicCount < IMODEL_MAX_ATOMICS)
    {
        gLastAtomicList[gLastAtomicCount++] = atomic;
    }

    RwFrameGetRoot(RpAtomicGetFrame(atomic));
    return atomic;
}

static RpAtomic* iModelStreamRead(RwStream* stream)
{
    RpClump* clump;
    U32 i;
    U32 maxIndex;
    F32 maxRadius;
    F32 testRadius;

    if (stream == NULL)
    {
        return NULL;
    }

    if (!RwStreamFindChunk(stream, 0x10, NULL, NULL))
    {
        RwStreamClose(stream, NULL);
        return NULL;
    }

    clump = RpClumpStreamRead(stream);
    RwStreamClose(stream, NULL);

    if (clump == NULL)
    {
        return NULL;
    }

    gLastAtomicCount = 0;
    RpClumpForAllAtomics(clump, FindAtomicCallback, NULL);

    if (gLastAtomicCount > 1)
    {
        maxRadius = -1.0f;
        maxIndex = 0;

        for (i = 0; i < gLastAtomicCount; i++)
        {
            if (gLastAtomicList[i]->boundingSphere.radius > maxRadius)
            {
                maxRadius = gLastAtomicList[i]->boundingSphere.radius;
                maxIndex = i;
            }
        }

        for (i = 0; i < gLastAtomicCount; i++)
        {
            if (i != maxIndex)
            {
                testRadius = xVec3Dist((xVec3*)&gLastAtomicList[i]->boundingSphere.center,
                                       (xVec3*)&gLastAtomicList[maxIndex]->boundingSphere.center) +
                             gLastAtomicList[i]->boundingSphere.radius;
                if (testRadius > maxRadius)
                {
                    maxRadius = testRadius;
                }
            }
        }

        maxRadius *= 1.05f;

        for (i = 0; i < gLastAtomicCount; i++)
        {
            if (i != maxIndex)
            {
                gLastAtomicList[i]->boundingSphere.center =
                    gLastAtomicList[maxIndex]->boundingSphere.center;
            }
            gLastAtomicList[i]->boundingSphere.radius = maxRadius;
            gLastAtomicList[i]->interpolator.flags &= ~rpINTERPOLATORDIRTYSPHERE;
        }
    }

    return gLastAtomicList[0];
}

RpAtomic* iModelFileNew(void* buffer, U32 size)
{
    RwMemory rwmem;
    rwmem.start = (RwUInt8*)buffer;
    rwmem.length = size;

    return iModelStreamRead(RwStreamOpen(rwSTREAMMEMORY, rwSTREAMREAD, &rwmem));
}

void iModelUnload(RpAtomic* userdata)
{
    RpClump* clump;
    RwFrame* frame;
    RwFrame* root;

    clump = userdata->clump;
    frame = (RwFrame*)clump->object.parent;
    if (frame != NULL)
    {
        root = RwFrameGetRoot(frame);
        if (root != NULL)
        {
            frame = root;
        }
        RwFrameDestroyHierarchy(frame);
        clump->object.parent = NULL;
    }
    if (clump != NULL)
    {
        RpClumpDestroy(clump);
    }
}

static RpAtomic* NextAtomicCallback(RpAtomic* atomic, void* data)
{
    RpAtomic** nextModel = (RpAtomic**)data;

    if (*nextModel == atomic)
    {
        *nextModel = NULL;
    }
    else if (*nextModel == NULL)
    {
        *nextModel = atomic;
    }
    return atomic;
}

RpAtomic* iModelFile_RWMultiAtomic(RpAtomic* model)
{
    RpClump* clump;
    RpAtomic* nextModel;

    if (model == NULL)
    {
        return NULL;
    }

    clump = model->clump;
    nextModel = model;
    RpClumpForAllAtomics(clump, NextAtomicCallback, &nextModel);
    return nextModel;
}

U32 iModelNumBones(RpAtomic* model)
{
    RpHAnimHierarchy* hierarchy = GetHierarchy(model);
    return hierarchy == NULL ? 0 : hierarchy->numNodes;
}

// The animation matrix stack stays in vf20-vf23 between bones. Each new
// quaternion is converted and composed with that current parent in VU0.
static inline void model_matrix_identity(RwMatrix* stack)
{
    asm volatile("vmulx.xyzw vf20, vf0, vf0x\n"
                 "vmulx.xyzw vf21, vf0, vf0x\n"
                 "vmulx.xyzw vf22, vf0, vf0x\n"
                 "vmulx.xyzw vf23, vf0, vf0x\n"
                 "vaddw.x vf20, vf20, vf0w\n"
                 "vaddw.y vf21, vf21, vf0w\n"
                 "vaddw.z vf22, vf22, vf0w\n"
                 "sqc2 vf20, 0x0(%0)\n"
                 "sqc2 vf21, 0x10(%0)\n"
                 "sqc2 vf22, 0x20(%0)\n"
                 "sqc2 vf23, 0x30(%0)\n"
                 : : "r"(stack) : "memory");
}

static inline void model_matrix_push(RwMatrix* stack)
{
    asm volatile("sqc2 vf20, 0x0(%0)\n"
                 "sqc2 vf21, 0x10(%0)\n"
                 "sqc2 vf22, 0x20(%0)\n"
                 "sqc2 vf23, 0x30(%0)\n"
                 : : "r"(stack) : "memory");
}

static inline void model_matrix_bone(const xQuat* quat, const xVec3* tran, RwMatrix* mat)
{
    // The packed quaternion kernel reserves a4-a6 for MMI shuffles.
    asm volatile("lq a4, 0x0(%0)\n"
                 "mtsah zero, 0x2\n"
                 "vaddw.xyz vf5, vf0, vf0w\n"
                 "qmtc2 a4, vf1\n"
                 "pcpyud a5, a4, zero\n"
                 "vadd.xyz vf1, vf1, vf1\n"
                 "pcpyld a5, a4, a5\n"
                 "pexew a6, a4\n"
                 "qmtc2 a5, vf3\n"
                 "qfsrv a6, a6, a6\n"
                 "vsubx.x vf3, vf0, vf3x\n"
                 "qfsrv a5, a6, a6\n"
                 "qmtc2 a6, vf2\n"
                 "qfsrv a5, a5, a5\n"
                 "vsuby.y vf2, vf0, vf2y\n"
                 "qmtc2 a5, vf4\n"
                 "lw a4, 0x0(%1)\n"
                 "lw a5, 0x4(%1)\n"
                 "lw a6, 0x8(%1)\n"
                 "vmulaz.xyz ACC, vf3, vf1z\n"
                 "vmaddaw.x ACC, vf5, vf0w\n"
                 "vsubz.z vf4, vf0, vf4z\n"
                 "vmsuby.xyz vf16, vf2, vf1y\n"
                 "vmulax.xyz ACC, vf2, vf1x\n"
                 "vmaddaw.y ACC, vf5, vf0w\n"
                 "vmsubz.xyz vf17, vf4, vf1z\n"
                 "vmulay.xyz ACC, vf4, vf1y\n"
                 "pextlw a4, a5, a4\n"
                 "vmaddaw.z ACC, vf5, vf0w\n"
                 "pcpyld a4, a6, a4\n"
                 "vmsubx.xyz vf18, vf3, vf1x\n"
                 "qmtc2 a4, vf19\n"
                 "vmulax.xyz ACC, vf20, vf16x\n"
                 "vmadday.xyz ACC, vf21, vf16y\n"
                 "vmaddz.xyz vf9, vf22, vf16z\n"
                 "vmulax.xyz ACC, vf20, vf17x\n"
                 "vmadday.xyz ACC, vf21, vf17y\n"
                 "vmaddz.xyz vf10, vf22, vf17z\n"
                 "vmulax.xyz ACC, vf20, vf18x\n"
                 "vmadday.xyz ACC, vf21, vf18y\n"
                 "vmaddz.xyz vf11, vf22, vf18z\n"
                 "vmulx.w vf9, vf0, vf0x\n"
                 "vmulax.xyz ACC, vf20, vf19x\n"
                 "vmadday.xyz ACC, vf21, vf19y\n"
                 "vmaddaz.xyz ACC, vf22, vf19z\n"
                 "vmaddw.xyz vf12, vf23, vf0w\n"
                 "sqc2 vf9, 0x0(%2)\n"
                 "sqc2 vf10, 0x10(%2)\n"
                 "sqc2 vf11, 0x20(%2)\n"
                 "sqc2 vf12, 0x30(%2)\n"
                 : : "r"(quat), "r"(tran), "r"(mat) : "a4", "a5", "a6", "memory");
}

static inline void model_matrix_pop(const RwMatrix* stack)
{
    asm volatile("lqc2 vf20, 0x0(%0)\n"
                 "lqc2 vf21, 0x10(%0)\n"
                 "lqc2 vf22, 0x20(%0)\n"
                 "lqc2 vf23, 0x30(%0)\n"
                 : : "r"(stack) : "memory");
}

static inline void model_matrix_advance()
{
    asm volatile("vmulw.xyzw vf20, vf9, vf0w\n"
                 "vmulw.xyzw vf21, vf10, vf0w\n"
                 "vmulw.xyzw vf22, vf11, vf0w\n"
                 "vmulw.xyzw vf23, vf12, vf0w\n"
                 : : : "memory");
}

void iModelAnimMatrices(RpAtomic* model, xQuat* quat, xVec3* tran, RwMatrix* mat)
{
    RpHAnimHierarchy* pHierarchy = GetHierarchy(model);
    RwMatrix matrixStack[32];
    RwMatrix* pMatrixStackTop;
    RpHAnimNodeInfo* pCurrentFrame;
    S32 pCurrentFrameFlags;
    S32 i, numFrames;

    if (pHierarchy != NULL)
    {
        pMatrixStackTop = matrixStack;
        model_matrix_identity(pMatrixStackTop);
        numFrames = pHierarchy->numNodes;
        ++pMatrixStackTop;
        pCurrentFrame = pHierarchy->pNodeInfo;
        for (i = 0; i < numFrames; ++i)
        {
            pCurrentFrameFlags = pCurrentFrame->flags;
            if (pCurrentFrameFlags & 2)
            {
                model_matrix_push(pMatrixStackTop);
                ++pMatrixStackTop;
            }
            model_matrix_bone(quat, tran, mat);
            if (pCurrentFrameFlags & 1)
            {
                model_matrix_pop(--pMatrixStackTop);
            }
            else
            {
                model_matrix_advance();
            }
            ++mat;
            ++quat;
            ++tran;
            ++pCurrentFrame;
        }
    }
}

void iModelRender(RpAtomic* model, RwMatrix* mat)
{
    RpHAnimHierarchy* hierarchy;
    RwMatrix* pAnimOldMatrix;
    RwFrame* frame;

    hierarchy = GetHierarchy(model);
    if (hierarchy != NULL)
    {
        pAnimOldMatrix = hierarchy->pMatrixArray;
        hierarchy->pMatrixArray = mat + 1;
    }

    frame = RpAtomicGetFrame(model);
    frame->ltm = *mat;
    RwMatrixUpdate(&frame->ltm);

    if (iModelHack_DisablePrelight)
    {
        model->geometry->flags &= ~rpGEOMETRYPRELIT;
    }

    RpAtomicRender(model);

    if (iModelHack_DisablePrelight && model->geometry->preLitLum != NULL)
    {
        model->geometry->flags |= rpGEOMETRYPRELIT;
    }

    if (hierarchy != NULL)
    {
        hierarchy->pMatrixArray = pAnimOldMatrix;
    }
}

// Transform the local sphere, retain the largest axis scale, and test the
// camera's four packed side planes followed by its two depth planes.
S32 iModelCull(RpAtomic* model, RwMatrix* mat)
{
    U32 outside, y, z, radius;
    const RwSphere* sphere = &model->boundingSphere;
    RwSphere* worldSphere = &model->worldBoundingSphere;
    asm volatile("lqc2 vf10, 0x0(%5)\n"
                 "lqc2 vf11, 0x10(%5)\n"
                 "vmul.xyz vf15, vf10, vf10\n"
                 "lqc2 vf12, 0x20(%5)\n"
                 "vmul.xyz vf16, vf11, vf11\n"
                 "lqc2 vf13, 0x30(%5)\n"
                 "vmul.xyz vf17, vf12, vf12\n"
                 "vaddy.x vf15, vf15, vf15y\n"
                 "lw %0, 0x0(%4)\n"
                 "vaddy.x vf16, vf16, vf16y\n"
                 "lw %1, 0x4(%4)\n"
                 "vaddy.x vf17, vf17, vf17y\n"
                 "lw %2, 0x8(%4)\n"
                 "vaddz.x vf15, vf15, vf15z\n"
                 "vaddz.x vf16, vf16, vf16z\n"
                 "vaddz.x vf17, vf17, vf17z\n"
                 "vmax.x vf15, vf15, vf16\n"
                 "vmax.x vf15, vf15, vf17\n"
                 "lw %3, 0xc(%4)\n"
                 "pextlw %0, %1, %0\n"
                 "pextlw %2, %3, %2\n"
                 "vsqrt Q, vf15x\n"
                 "pcpyld %0, %2, %0\n"
                 "qmtc2 %0, vf2\n"
                 "vmulax.xyz ACC, vf10, vf2x\n"
                 "vmadday.xyz ACC, vf11, vf2y\n"
                 "vmaddaz.xyz ACC, vf12, vf2z\n"
                 "vmaddw.xyz vf1, vf13, vf0w\n"
                 "lqc2 vf6, 0x30(%6)\n"
                 "vwaitq\n"
                 "vmulq.w vf1, vf2, Q\n"
                 "lqc2 vf3, 0x0(%6)\n"
                 "lqc2 vf4, 0x10(%6)\n"
                 "lqc2 vf5, 0x20(%6)\n"
                 "vaddaw.xyzw ACC, vf6, vf1w\n"
                 "vmsubax.xyzw ACC, vf3, vf1x\n"
                 "vmsubay.xyzw ACC, vf4, vf1y\n"
                 "vmsubz.xyzw vf2, vf5, vf1z\n"
                 "lqc2 vf3, 0x40(%6)\n"
                 "lqc2 vf4, 0x50(%6)\n"
                 "lqc2 vf5, 0x60(%6)\n"
                 "qmfc2 %0, vf2\n"
                 "lqc2 vf6, 0x70(%6)\n"
                 "pcgtw %0, zero, %0\n"
                 "ppach %0, zero, %0\n"
                 "qmfc2 %2, vf1\n"
                 "sw %2, 0x0(%7)\n"
                 "pextuw %3, zero, %2\n"
                 "prot3w %2, %2\n"
                 "sw %2, 0x4(%7)\n"
                 "sw %3, 0x8(%7)\n"
                 "pextuw %3, zero, %3\n"
                 "sw %3, 0xc(%7)\n"
                 : "=&r"(outside), "=&r"(y), "=&r"(z), "=&r"(radius) : "r"(sphere), "r"(mat), "r"(globals.camera.frustplane), "r"(worldSphere) : "memory");
    asm volatile("vaddaw.xy ACC, vf6, vf1w\n"
                 : : : "memory");
    if (outside)
    {
        goto outside_frustum;
    }
    asm volatile("vmsubax.xy ACC, vf3, vf1x\n"
                 "vmsubay.xy ACC, vf4, vf1y\n"
                 "vmsubz.xy vf2, vf5, vf1z\n"
                 "qmfc2 %0, vf2\n"
                 "pcgtw %0, zero, %0\n"
                 : "=r"(outside) : : "memory");
    if (outside)
    {
        goto outside_frustum;
    }
    return 0;

outside_frustum:
    return 1;
}

S32 iModelSphereCull(xSphere* sphere)
{
    return RwCameraFrustumTestSphere(RwCameraGetCurrentCamera(), (RwSphere*)sphere) ==
           rwSPHEREOUTSIDE;
}

S32 iModelCullPlusShadow(RpAtomic* model, RwMatrix* mat, xVec3* shadowVec, S32* shadowOutside)
{
    F32 xScale2, yScale2, zScale2;
    RwV3d *right, *up, *at;
    RwCamera* cam;
    RwSphere worldsph;
    const RwFrustumPlane* frustumPlane;
    S32 numPlanes;
    F32 nDot;
    F32 sDot;

    cam = RwCameraGetCurrentCamera();

    RwV3dTransformPoints(&worldsph.center, &model->boundingSphere.center, 1, mat);

    right = &mat->right;
    up = &mat->up;
    at = &mat->at;
    xScale2 = SQR(right->x) + SQR(right->y) + SQR(right->z);
    yScale2 = SQR(up->x) + SQR(up->y) + SQR(up->z);
    zScale2 = SQR(at->x) + SQR(at->y) + SQR(at->z);
    worldsph.radius = model->boundingSphere.radius * xsqrt(MAX3(xScale2, yScale2, zScale2));
    model->worldBoundingSphere = worldsph;

    numPlanes = 6;
    frustumPlane = cam->frustumPlanes;
    while (numPlanes--)
    {
        nDot = RwV3dDotProductMacro(&worldsph.center, &frustumPlane->plane.normal);
        nDot -= frustumPlane->plane.distance;

        if (nDot > worldsph.radius)
        {
            goto shadow_test;
        }

        frustumPlane++;
    }

    *shadowOutside = 0;
    return 0;

shadow_test:
    sDot = RwV3dDotProductMacro((RwV3d*)shadowVec, &frustumPlane->plane.normal);
    sDot -= frustumPlane->plane.distance;

    if (sDot > worldsph.radius)
    {
        *shadowOutside = 1;
        return 1;
    }

    frustumPlane++;
    while (numPlanes--)
    {
        nDot = RwV3dDotProductMacro(&worldsph.center, &frustumPlane->plane.normal);
        nDot -= frustumPlane->plane.distance;

        sDot = RwV3dDotProductMacro((RwV3d*)shadowVec, &frustumPlane->plane.normal);
        sDot -= frustumPlane->plane.distance;

        if (nDot > worldsph.radius && sDot > worldsph.radius)
        {
            *shadowOutside = 1;
            return 1;
        }

        frustumPlane++;
    }

    *shadowOutside = 0;
    return 1;
}

U32 iModelVertCount(RpAtomic* model)
{
    return model->geometry->numVertices;
}

static inline void SkinNormals(xVec3* dest, const xVec3* normal, const RwMatrix* mat,
                        const RwMatrix* skinmat, const F32* wt, const U32* idx, U32 count)
{
    U32 catMatFlags[2] = { 0, 0 };
    RwMatrix* catmat = (RwMatrix*)giAnimScratch;
    const RwMatrix* rootmat = mat;
    mat++;

    while (count != 0)
    {
        for (U32 i = 0; i < 4; i++)
        {
            U32 midx = (*idx >> (i * 8)) & 0xFF;
            if (!(catMatFlags[midx >> 5] & (1 << (midx & 31))))
            {
                RwMatrix* cm = catmat + midx;
                xMat3x3Mul((xMat3x3*)cm, (const xMat3x3*)(skinmat + midx),
                           (const xMat3x3*)(mat + midx));
                xMat3x3Normalize((xMat3x3*)cm, (xMat3x3*)cm);
                catMatFlags[midx >> 5] |= 1 << (midx & 31);
            }
        }

        xVec3 accumV;
        accumV.x = accumV.y = accumV.z = 0.0f;

        RwMatrix* pMatrix;
        const F32* fwt = wt;
        U32 wtidx = *idx;
        U32 maxwt = 4;

        while (*fwt && maxwt)
        {
            pMatrix = catmat + (wtidx & 0xFF);
            wtidx >>= 8;

            accumV.x += *fwt * (pMatrix->right.x * normal->x + pMatrix->up.x * normal->y +
                                pMatrix->at.x * normal->z);
            accumV.y += *fwt * (pMatrix->right.y * normal->x + pMatrix->up.y * normal->y +
                                pMatrix->at.y * normal->z);
            accumV.z += *fwt * (pMatrix->right.z * normal->x + pMatrix->up.z * normal->y +
                                pMatrix->at.z * normal->z);

            fwt++;
            maxwt--;
        }

        dest->x = rootmat->right.x * accumV.x + rootmat->up.x * accumV.y +
                  rootmat->at.x * accumV.z;
        dest->y = rootmat->right.y * accumV.x + rootmat->up.y * accumV.y +
                  rootmat->at.y * accumV.z;
        dest->z = rootmat->right.z * accumV.x + rootmat->up.z * accumV.y +
                  rootmat->at.z * accumV.z;

        normal++;
        idx++;
        wt += 4;
        count--;
        dest++;
    }
}

U32 iModelNormalEval(xVec3* out, const RpAtomic& m, const RwMatrixTag* mat, U32 index, S32 size,
                     const xVec3* in)
{
    RpGeometry* geom = RpAtomicGetGeometry(&m);

    if (in == NULL)
    {
        S32 max_size = geom->numVertices - index;
        if (size < 0 || size > max_size)
        {
            size = max_size;
        }
        in = (const xVec3*)geom->morphTarget[0].normals;
    }

    if (size <= 0)
    {
        return 0;
    }

    in += index;

    RpSkin* skin = RpSkinGeometryGetSkin(geom);
    if (skin != NULL)
    {
        const RwMatrix* skin_mats = RpSkinGetSkinToBoneMatrices(skin);
        const F32* bone_weights = (const F32*)RpSkinGetVertexBoneWeights(skin) + index;
        const U32* bone_indices = RpSkinGetVertexBoneIndices(skin) + index;
        SkinNormals(out, in, mat, skin_mats, bone_weights, bone_indices, size);
    }
    else
    {
        xMat4x3 nmat;
        xMat3x3Normalize(&nmat, (xMat3x3*)&mat);
        nmat.pos.assign(0.0f, 0.0f, 0.0f);
        RwV3dTransformPoints((RwV3d*)out, (const RwV3d*)in, size, (RwMatrix*)&nmat);
    }

    return size;
}

static U32 iModelTagUserData(xModelTag* tag, RpAtomic* model, F32 x, F32 y, F32 z, S32 closeV)
{
    S32 i, count;
    RpUserDataArray *array, *testarray;
    F32 distSqr, closeDistSqr;
    S32 numTags, t;
    xModelTag* tagList;

    count = RpGeometryGetUserDataArrayCount(model->geometry);
    array = NULL;

    for (i = 0; i < count; i++)
    {
        testarray = RpGeometryGetUserDataArray(model->geometry, i);
        if (strcmp(testarray->name, "HI_Tags") == 0)
        {
            array = testarray;
            break;
        }
    }

    if (array == NULL)
    {
        memset(tag, 0, sizeof(xModelTag));
        return 0;
    }

    numTags = *(S32*)array->data;
    closeDistSqr = 1.0e9f;
    tagList = (xModelTag*)((S32*)array->data + 1);

    if (closeV < 0 || closeV > numTags)
    {
        closeV = 0;
        for (t = 0; t < numTags; t++)
        {
            distSqr = SQR(tagList[t].v.x - x) + SQR(tagList[t].v.y - y) + SQR(tagList[t].v.z - z);
            if (distSqr < closeDistSqr)
            {
                closeV = t;
                closeDistSqr = distSqr;
            }
        }
        if (tag != NULL)
        {
            *tag = tagList[closeV];
        }
    }
    else
    {
        if (tag != NULL)
        {
            *tag = tagList[closeV];
        }
    }

    return closeV;
}

static U32 iModelTagInternal(xModelTag* tag, RpAtomic* model, F32 x, F32 y, F32 z, S32 closeV)
{
    RpGeometry* geom;
    RwV3d* vert;
    S32 v, numV;
    F32 distSqr, closeDistSqr;
    RpSkin* skin;
    const RwMatrixWeights* wt;

    geom = RpAtomicGetGeometry(model);
    vert = geom->morphTarget[0].verts;

    if (vert == NULL)
    {
        return iModelTagUserData(tag, model, x, y, z, closeV);
    }

    numV = geom->numVertices;
    closeDistSqr = 1.0e9f;

    if (closeV < 0 || closeV > numV)
    {
        closeV = 0;
        for (v = 0; v < numV; v++)
        {
            distSqr = SQR(vert[v].x - x) + SQR(vert[v].y - y) + SQR(vert[v].z - z);
            if (distSqr < closeDistSqr)
            {
                closeV = v;
                closeDistSqr = distSqr;
            }
        }
        if (tag != NULL)
        {
            tag->v.x = x;
            tag->v.y = y;
            tag->v.z = z;
        }
    }
    else
    {
        if (tag != NULL)
        {
            tag->v.x = vert[closeV].x;
            tag->v.y = vert[closeV].y;
            tag->v.z = vert[closeV].z;
        }
    }

    if (tag != NULL)
    {
        skin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(model));
        if (skin != NULL)
        {
            wt = RpSkinGetVertexBoneWeights(skin) + closeV;
            tag->matidx = RpSkinGetVertexBoneIndices(skin)[closeV];
            tag->wt[0] = wt->w0;
            tag->wt[1] = wt->w1;
            tag->wt[2] = wt->w2;
            tag->wt[3] = wt->w3;
        }
        else
        {
            tag->matidx = 0;
            tag->wt[0] = 0.0f;
            tag->wt[1] = 0.0f;
            tag->wt[2] = 0.0f;
            tag->wt[3] = 0.0f;
        }
    }

    return closeV;
}

U32 iModelTagSetup(xModelTag* tag, RpAtomic* model, F32 x, F32 y, F32 z)
{
    return iModelTagInternal(tag, model, x, y, z, -1);
}

U32 iModelTagSetup(xModelTagWithNormal* tag, RpAtomic* model, F32 x, F32 y, F32 z)
{
    U32 index = iModelTagInternal(tag, model, x, y, z, -1);
    xVec3* normals = (xVec3*)model->geometry->morphTarget[0].normals;
    tag->normal = normals[index];
    return index;
}

void iModelTagEval(RpAtomic* model, const xModelTagWithNormal* tag, RwMatrix* mat, xVec3* dest,
                   xVec3* normal)
{
    iModelTagEval(model, tag, mat, dest);
    if (tag->wt[0])
    {
        RpSkin* skin = RpSkinGeometryGetSkin(RpAtomicGetGeometry(model));
        const RwMatrix* skinmat = RpSkinGetSkinToBoneMatrices(skin);
        SkinNormals(normal, &tag->normal, mat, skinmat, tag->wt, &tag->matidx, 1);
    }
    else
    {
        RwV3dTransformPoints((RwV3d*)normal, (const RwV3d*)&tag->normal, 1, mat);
    }
}

void iModelSetMaterialAlpha(RpAtomic* model, U8 alpha)
{
    U32 i;
    RpGeometry* geom = RpAtomicGetGeometry(model);
    RpMaterial* material;
    RwRGBA* col;

    if (model != sLastMaterial)
    {
        sMaterialFlags = 0;
    }

    geom->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;
    sMaterialIdx = geom->matList.numMaterials;

    for (i = 0; i < geom->matList.numMaterials; i++)
    {
        material = geom->matList.materials[i];
        col = &material->color;
        sMaterialAlpha[i] = col->alpha;

        RwRGBA new_col = *col;
        new_col.alpha = alpha;
        material->color = new_col;
    }

    sMaterialFlags |= 0x1;
    sLastMaterial = model;
}

void iModelResetMaterial(RpAtomic* model)
{
    U32 i;
    RpGeometry* geom;
    RpMaterial* material;

    if (model != sLastMaterial)
    {
        sMaterialFlags = 0;
    }

    geom = model->geometry;

    for (i = 0; i < geom->matList.numMaterials; i++)
    {
        material = geom->matList.materials[i];

        if ((sMaterialFlags & 0x3) == 0x3)
        {
            RwRGBA newColor = sMaterialColor[i];
            newColor.alpha = sMaterialAlpha[i];
            material->color = newColor;
        }
        else
        {
            if (sMaterialFlags & 0x2)
            {
                RwRGBA newColor = sMaterialColor[i];
                newColor.alpha = material->color.alpha;
                material->color = newColor;
            }
            if (sMaterialFlags & 0x1)
            {
                RwRGBA newColor = material->color;
                newColor.alpha = sMaterialAlpha[i];
                material->color = newColor;
            }
        }

        if (sMaterialFlags & 0x4)
        {
            RpMaterialSetTexture(material, sMaterialTexture[i]);
        }
    }

    sMaterialFlags = 0;
}

static RpMaterial* iModelSetMaterialTextureCB(RpMaterial* material, void* data)
{
    sMaterialTexture[sMaterialIdx] = material->texture;
    sMaterialIdx++;
    RpMaterialSetTexture(material, (RwTexture*)data);
    return material;
}

void iModelSetMaterialTexture(RpAtomic* model, void* texture)
{
    RpGeometry* geom;

    if (model != sLastMaterial)
    {
        sMaterialFlags = 0;
    }

    geom = model->geometry;
    sMaterialIdx = 0;
    RpGeometryForAllMaterials(geom, iModelSetMaterialTextureCB, texture);

    sMaterialFlags |= 0x4;
    sLastMaterial = model;
}

namespace
{
    inline void U8_COLOR_CLAMP(U8& destu8, F32 srcf32)
    {
        if (srcf32 < 0.0f)
            srcf32 = 0.0f;
        else if (srcf32 > 255.0f)
            srcf32 = 255.0f;
        destu8 = (U8)srcf32;
    }
} // namespace

static RpMaterial* iModelMaterialMulCB(RpMaterial* material, void* data)
{
    const RwRGBA* rw_col = &material->color;
    RwRGBA col = sMaterialColor[sMaterialIdx++] = *rw_col;
    F32 tmp;
    F32* mods = (F32*)data;

    tmp = col.red * mods[0];
    U8_COLOR_CLAMP(col.red, tmp);

    tmp = col.green * mods[1];
    U8_COLOR_CLAMP(col.green, tmp);

    tmp = col.blue * mods[2];
    U8_COLOR_CLAMP(col.blue, tmp);

    material->color = col;

    return material;
}

void iModelMaterialMul(RpAtomic* model, F32 rm, F32 gm, F32 bm)
{
    RpGeometry* geom = RpAtomicGetGeometry(model);

    if (model != sLastMaterial)
    {
        sMaterialFlags = 0;
    }

    geom->flags |= rpGEOMETRYMODULATEMATERIALCOLOR;

    F32 cols[3];
    cols[0] = rm;
    cols[1] = gm;
    cols[2] = bm;

    sMaterialIdx = 0;

    RpGeometryForAllMaterials(geom, iModelMaterialMulCB, cols);

    sMaterialFlags |= 0x2;
    sLastMaterial = model;
}
