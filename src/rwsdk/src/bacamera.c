#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwCAMERA 4

#define rwPLUGIN_ID 1

#define rwCAMERAALIGNMENT 4

#define rwSTANDARDCAMERABEGINUPDATE 1
#define rwSTANDARDCAMERAENDUPDATE 10
#define rwSTANDARDCAMERACLEAR 21

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

#define rwMatrixInitialize(m, type) ((m)->flags = (RwUInt32)(type))

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_INVCAMERAPROJECTION 0x80000003, "Invalid projection type specified"

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rwCameraGlobals rwCameraGlobals;
struct rwCameraGlobals
{
    RwFreeList* cameraFreeList;
};

#define RWCAMERAGLOBAL(var)                                                                        \
    (RWPLUGINOFFSET(rwCameraGlobals, RwEngineInstance, cameraModule.globalsOffset)->var)

extern void _rwPipeInitForCamera(const RwCamera* camera);

static RwModuleInfo cameraModule;

static RwFreeList _rwCameraFreeList;

static RwInt32 _rwCameraFreeListBlockSize = 4;
static RwInt32 _rwCameraFreeListPreallocBlocks = 1;

RwPluginRegistry cameraTKList = { sizeof(RwCamera),        sizeof(RwCamera),       0, 0,
                                         (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

#define rwInvSqrtMacro(_result, _num) ((*(_result)) = _rwInvSqrt(_num))

#define rwFrustumPlaneSetClosest(_fp)                                                              \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_fp)->closestX = (RwUInt8)(((*(RwInt32*)&(_fp)->plane.normal.x) >> 31) + 1);              \
        (_fp)->closestY = (RwUInt8)(((*(RwInt32*)&(_fp)->plane.normal.y) >> 31) + 1);              \
        (_fp)->closestZ = (RwUInt8)(((*(RwInt32*)&(_fp)->plane.normal.z) >> 31) + 1);              \
    }                                                                                              \
    MACRO_STOP

#define CameraBuildClipPlanes(_camera, _ltm)                                                       \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwFrustumPlane* frustumPlane = (_camera)->frustumPlanes;                                   \
        RwV3d* corner = (_camera)->frustumCorners;                                                 \
        RwV3d vecA;                                                                                \
        RwV3d vecB;                                                                                \
        RwV3d vecC;                                                                                \
        RwReal recip;                                                                              \
                                                                                                   \
        frustumPlane[0].plane.normal = (_ltm)->at;                                                 \
        frustumPlane[0].plane.distance =                                                           \
            RwV3dDotProductMacro(&corner[4], &frustumPlane[0].plane.normal);                       \
        rwFrustumPlaneSetClosest(&frustumPlane[0]);                                                \
                                                                                                   \
        RwV3dNegateMacro(&frustumPlane[1].plane.normal, &frustumPlane[0].plane.normal);            \
        frustumPlane[1].plane.distance =                                                           \
            RwV3dDotProductMacro(&corner[0], &frustumPlane[1].plane.normal);                       \
        rwFrustumPlaneSetClosest(&frustumPlane[1]);                                                \
                                                                                                   \
        RwV3dSubMacro(&vecA, &corner[1], &corner[5]);                                              \
        RwV3dSubMacro(&vecB, &corner[6], &corner[5]);                                              \
        RwV3dSubMacro(&vecC, &corner[4], &corner[5]);                                              \
                                                                                                   \
        RwV3dCrossProductMacro(&frustumPlane[2].plane.normal, &vecA, &vecB);                       \
        _rwV3dNormalizeMacro(recip, &frustumPlane[2].plane.normal, &frustumPlane[2].plane.normal); \
        frustumPlane[2].plane.distance =                                                           \
            RwV3dDotProductMacro(&corner[1], &frustumPlane[2].plane.normal);                       \
        rwFrustumPlaneSetClosest(&frustumPlane[2]);                                                \
                                                                                                   \
        RwV3dCrossProductMacro(&frustumPlane[3].plane.normal, &vecC, &vecA);                       \
        _rwV3dNormalizeMacro(recip, &frustumPlane[3].plane.normal, &frustumPlane[3].plane.normal); \
        frustumPlane[3].plane.distance =                                                           \
            RwV3dDotProductMacro(&corner[1], &frustumPlane[3].plane.normal);                       \
        rwFrustumPlaneSetClosest(&frustumPlane[3]);                                                \
                                                                                                   \
        RwV3dNegateMacro(&frustumPlane[4].plane.normal, &frustumPlane[2].plane.normal);            \
        frustumPlane[4].plane.distance =                                                           \
            RwV3dDotProductMacro(&corner[3], &frustumPlane[4].plane.normal);                       \
        rwFrustumPlaneSetClosest(&frustumPlane[4]);                                                \
                                                                                                   \
        RwV3dNegateMacro(&frustumPlane[5].plane.normal, &frustumPlane[3].plane.normal);            \
        frustumPlane[5].plane.distance =                                                           \
            RwV3dDotProductMacro(&corner[3], &frustumPlane[5].plane.normal);                       \
        rwFrustumPlaneSetClosest(&frustumPlane[5]);                                                \
    }                                                                                              \
    MACRO_STOP

static void CameraSetZ(RwCamera* camera)
{
    RwReal nearScreenZ = RwIm2DGetNearScreenZ();
    RwReal farScreenZ = RwIm2DGetFarScreenZ();
    RwReal zDelta;
    RwReal zNear;
    RwReal zFar;
    RwReal zScale;

    switch (camera->projectionType)
    {
    case rwPARALLEL:
        zFar = camera->farPlane;
        zNear = camera->nearPlane;
        break;
    case rwPERSPECTIVE:
    default:
        zFar = ((RwReal)1) / camera->farPlane;
        zNear = ((RwReal)1) / camera->nearPlane;
        break;
    }

    zDelta = ((RwReal)0.0001) * (farScreenZ - nearScreenZ);
    farScreenZ -= zDelta;
    nearScreenZ += zDelta;

    zScale = (farScreenZ - nearScreenZ) / (zFar - zNear);
    camera->zScale = zScale;
    camera->zShift = ((RwReal)0.5) * ((farScreenZ + nearScreenZ) - zScale * (zFar + zNear));
}

static void CameraBuildPerspClipPlanes(RwCamera* camera)
{
    RwV3d vTmp;
    RwV3d vTmp2;
    RwV3d vRight;
    RwV3d vUp;
    RwV3d vCOP;
    RwMatrix* cameraLTM = &((RwFrame*)rwObjectGetParent(camera))->ltm;
    RwReal recip;
    RwReal scale;
    RwInt32 i;
    RwV3d* frustumVerts;
    RwFrustumPlane* frustumPlanes;
    RwV3d* target;

    frustumVerts = camera->frustumCorners;

    RwV3dScaleMacro(&vRight, &cameraLTM->right, camera->viewWindow.x);
    RwV3dScaleMacro(&vUp, &cameraLTM->up, camera->viewWindow.y);

    RwV3dScaleMacro(&vTmp2, &vRight, ((RwReal)2));
    RwV3dAddMacro(&vTmp, &cameraLTM->at, &vRight);
    RwV3dScaleMacro(&vCOP, &cameraLTM->right, -camera->viewOffset.x);
    scale = camera->viewOffset.y;
    RwV3dIncrementScaledMacro(&vCOP, &cameraLTM->up, scale);
    RwV3dAddMacro(&vTmp, &vTmp, &vUp);
    RwV3dScaleMacro(&vUp, &vUp, ((RwReal)2));
    frustumVerts[0] = vTmp;

    RwV3dSubMacro(&vTmp, &vTmp, &vTmp2);
    frustumVerts[1] = vTmp;
    RwV3dSubMacro(&vTmp, &vTmp, &vUp);
    frustumVerts[2] = vTmp;
    RwV3dAddMacro(&vTmp, &vTmp, &vTmp2);
    frustumVerts[3] = vTmp;

    for (i = 0; i < 4; i++)
    {
        target = &frustumVerts[i + 4];

        RwV3dSubMacro(&vTmp, &frustumVerts[i], &vCOP);
        RwV3dAddMacro(&frustumVerts[i], &vCOP, &cameraLTM->pos);
        RwV3dIncrementScaledMacro(&frustumVerts[i], &vTmp, camera->nearPlane);

        RwV3dAddMacro(target, &vCOP, &cameraLTM->pos);
        RwV3dIncrementScaledMacro(target, &vTmp, camera->farPlane);
    }

    frustumPlanes = camera->frustumPlanes;

    frustumPlanes[0].plane.normal = cameraLTM->at;
    frustumPlanes[0].plane.distance = RwV3dDotProductMacro(&frustumVerts[4], &cameraLTM->at);
    rwFrustumPlaneSetClosest(&frustumPlanes[0]);

    RwV3dNegateMacro(&frustumPlanes[1].plane.normal, &frustumPlanes[0].plane.normal);
    frustumPlanes[1].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[0], &frustumPlanes[1].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[1]);

    RwV3dSubMacro(&vTmp, &frustumVerts[1], &frustumVerts[5]);
    RwV3dSubMacro(&vTmp2, &frustumVerts[6], &frustumVerts[5]);
    RwV3dCrossProductMacro(&frustumPlanes[2].plane.normal, &vTmp, &vTmp2);
    _rwV3dNormalizeMacro(recip, &frustumPlanes[2].plane.normal, &frustumPlanes[2].plane.normal);
    frustumPlanes[2].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[1], &frustumPlanes[2].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[2]);

    RwV3dSubMacro(&vTmp2, &frustumVerts[4], &frustumVerts[5]);
    RwV3dCrossProductMacro(&frustumPlanes[3].plane.normal, &vTmp2, &vTmp);
    _rwV3dNormalizeMacro(recip, &frustumPlanes[3].plane.normal, &frustumPlanes[3].plane.normal);
    frustumPlanes[3].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[1], &frustumPlanes[3].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[3]);

    RwV3dSubMacro(&vTmp, &frustumVerts[3], &frustumVerts[7]);
    RwV3dSubMacro(&vTmp2, &frustumVerts[4], &frustumVerts[7]);
    RwV3dCrossProductMacro(&frustumPlanes[4].plane.normal, &vTmp, &vTmp2);
    _rwV3dNormalizeMacro(recip, &frustumPlanes[4].plane.normal, &frustumPlanes[4].plane.normal);
    frustumPlanes[4].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[3], &frustumPlanes[4].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[4]);

    RwV3dSubMacro(&vTmp2, &frustumVerts[6], &frustumVerts[7]);
    RwV3dCrossProductMacro(&frustumPlanes[5].plane.normal, &vTmp2, &vTmp);
    _rwV3dNormalizeMacro(recip, &frustumPlanes[5].plane.normal, &frustumPlanes[5].plane.normal);
    frustumPlanes[5].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[3], &frustumPlanes[5].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[5]);
}

static RwCamera* CameraBuildPerspViewMatrix(RwCamera* camera)
{
    RwReal scale;
    RwV3d vVector;
    const RwMatrix* cameraLTM = &((RwFrame*)rwObjectGetParent(camera))->ltm;
    RwMatrix* viewMatrix = &camera->viewMatrix;

    scale = ((RwReal)-0.5) * camera->recipViewWindow.x;
    RwV3dScaleMacro(&vVector, &cameraLTM->right, scale);
    scale = ((RwReal)0.5) - scale * camera->viewOffset.x;
    RwV3dIncrementScaledMacro(&vVector, &cameraLTM->at, scale);

    viewMatrix->right.x = vVector.x;
    viewMatrix->up.x = vVector.y;
    viewMatrix->at.x = vVector.z;
    viewMatrix->pos.x = ((RwReal)0.5) - (scale + RwV3dDotProductMacro(&cameraLTM->pos, &vVector));

    scale = ((RwReal)-0.5) * camera->recipViewWindow.y;
    RwV3dScaleMacro(&vVector, &cameraLTM->up, scale);
    scale = scale * camera->viewOffset.y + ((RwReal)0.5);
    RwV3dIncrementScaledMacro(&vVector, &cameraLTM->at, scale);

    viewMatrix->right.y = vVector.x;
    viewMatrix->up.y = vVector.y;
    viewMatrix->at.y = vVector.z;
    viewMatrix->pos.y = ((RwReal)0.5) - (scale + RwV3dDotProductMacro(&cameraLTM->pos, &vVector));

    viewMatrix->right.z = cameraLTM->at.x;
    viewMatrix->up.z = cameraLTM->at.y;
    viewMatrix->at.z = cameraLTM->at.z;
    viewMatrix->pos.z = -RwV3dDotProductMacro(&cameraLTM->pos, &cameraLTM->at);

    RwMatrixOptimize(viewMatrix, (const RwMatrixTolerance*)NULL);

    return camera;
}

static void CameraBuildParallelClipPlanes(RwCamera* camera)
{
    RwReal width;
    RwReal height;
    RwReal nearPlane;
    RwReal farPlane;
    RwReal offsetx;
    RwReal offsety;
    RwV3d vTmp;
    RwV3d vTmp2;
    RwReal recip;
    RwV3d* frustumVerts;
    RwMatrix* cameraLTM;
    RwFrustumPlane* frustumPlanes;

    cameraLTM = &((RwFrame*)rwObjectGetParent(camera))->ltm;
    width = camera->viewWindow.x;
    height = camera->viewWindow.y;
    nearPlane = camera->nearPlane;
    farPlane = camera->farPlane;
    offsetx = -camera->viewOffset.x;
    offsety = camera->viewOffset.y;
    frustumVerts = camera->frustumCorners;
    frustumPlanes = camera->frustumPlanes;

    frustumVerts[0].z = frustumVerts[1].z = frustumVerts[2].z = frustumVerts[3].z = nearPlane;
    frustumVerts[4].z = frustumVerts[5].z = frustumVerts[6].z = frustumVerts[7].z = farPlane;

    frustumVerts[0].x = frustumVerts[3].x = width + (((RwReal)1) - nearPlane) * offsetx;
    frustumVerts[1].x = frustumVerts[2].x = -width + (((RwReal)1) - nearPlane) * offsetx;
    frustumVerts[4].x = frustumVerts[7].x = width + (((RwReal)1) - farPlane) * offsetx;
    frustumVerts[5].x = frustumVerts[6].x = -width + (((RwReal)1) - farPlane) * offsetx;
    frustumVerts[0].y = frustumVerts[1].y = height + (((RwReal)1) - nearPlane) * offsety;
    frustumVerts[2].y = frustumVerts[3].y = -height + (((RwReal)1) - nearPlane) * offsety;
    frustumVerts[4].y = frustumVerts[5].y = height + (((RwReal)1) - farPlane) * offsety;
    frustumVerts[6].y = frustumVerts[7].y = -height + (((RwReal)1) - farPlane) * offsety;

    RwV3dTransformPoints(frustumVerts, frustumVerts, 8, cameraLTM);

    frustumPlanes[0].plane.normal = cameraLTM->at;
    frustumPlanes[0].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[4], &frustumPlanes[0].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[0]);

    RwV3dNegateMacro(&frustumPlanes[1].plane.normal, &frustumPlanes[0].plane.normal);
    frustumPlanes[1].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[0], &frustumPlanes[1].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[1]);

    RwV3dSubMacro(&vTmp, &frustumVerts[1], &frustumVerts[5]);
    RwV3dSubMacro(&vTmp2, &frustumVerts[6], &frustumVerts[5]);
    RwV3dCrossProductMacro(&frustumPlanes[2].plane.normal, &vTmp, &vTmp2);
    _rwV3dNormalizeMacro(recip, &frustumPlanes[2].plane.normal, &frustumPlanes[2].plane.normal);
    frustumPlanes[2].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[1], &frustumPlanes[2].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[2]);

    RwV3dSubMacro(&vTmp2, &frustumVerts[4], &frustumVerts[5]);
    RwV3dCrossProductMacro(&frustumPlanes[3].plane.normal, &vTmp2, &vTmp);
    _rwV3dNormalizeMacro(recip, &frustumPlanes[3].plane.normal, &frustumPlanes[3].plane.normal);
    frustumPlanes[3].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[1], &frustumPlanes[3].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[3]);

    RwV3dNegateMacro(&frustumPlanes[4].plane.normal, &frustumPlanes[2].plane.normal);
    frustumPlanes[4].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[3], &frustumPlanes[4].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[4]);

    RwV3dNegateMacro(&frustumPlanes[5].plane.normal, &frustumPlanes[3].plane.normal);
    frustumPlanes[5].plane.distance =
        RwV3dDotProductMacro(&frustumVerts[3], &frustumPlanes[5].plane.normal);
    rwFrustumPlaneSetClosest(&frustumPlanes[5]);
}

static RwCamera* CameraBuildParallelViewMatrix(RwCamera* camera)
{
    RwReal scale;
    RwV3d vVector;
    const RwMatrix* cameraLTM = &((RwFrame*)rwObjectGetParent(camera))->ltm;
    RwMatrix* viewMatrix = &camera->viewMatrix;

    scale = ((RwReal)-0.5) * camera->recipViewWindow.x;
    RwV3dScaleMacro(&vVector, &cameraLTM->right, scale);
    scale = -(scale * camera->viewOffset.x);
    RwV3dIncrementScaledMacro(&vVector, &cameraLTM->at, scale);

    viewMatrix->right.x = vVector.x;
    viewMatrix->up.x = vVector.y;
    viewMatrix->at.x = vVector.z;
    viewMatrix->pos.x = ((RwReal)0.5) - (scale + RwV3dDotProductMacro(&cameraLTM->pos, &vVector));

    scale = ((RwReal)-0.5) * camera->recipViewWindow.y;
    RwV3dScaleMacro(&vVector, &cameraLTM->up, scale);
    scale *= camera->viewOffset.y;
    RwV3dIncrementScaledMacro(&vVector, &cameraLTM->at, scale);

    viewMatrix->right.y = vVector.x;
    viewMatrix->up.y = vVector.y;
    viewMatrix->at.y = vVector.z;
    viewMatrix->pos.y = ((RwReal)0.5) - (scale + RwV3dDotProductMacro(&cameraLTM->pos, &vVector));

    viewMatrix->right.z = cameraLTM->at.x;
    viewMatrix->up.z = cameraLTM->at.y;
    viewMatrix->at.z = cameraLTM->at.z;
    viewMatrix->pos.z = -RwV3dDotProductMacro(&cameraLTM->pos, &cameraLTM->at);

    RwMatrixOptimize(viewMatrix, (const RwMatrixTolerance*)NULL);

    return camera;
}

static RwCamera* CameraSync(RwCamera* camera)
{
    if (camera->projectionType == rwPERSPECTIVE)
    {
        CameraBuildPerspViewMatrix(camera);
        CameraBuildPerspClipPlanes(camera);
    }
    else
    {
        CameraBuildParallelViewMatrix(camera);
        CameraBuildParallelClipPlanes(camera);
    }

    RwBBoxCalculate(&camera->frustumBoundBox, camera->frustumCorners, 8);

    return camera;
}

static RwCamera* CameraEndUpdate(RwCamera* camera)
{
    if (RWSRCGLOBAL(stdFunc)[rwSTANDARDCAMERAENDUPDATE](NULL, camera, 0))
    {
        RWSRCGLOBAL(curCamera) = NULL;

        return camera;
    }

    return (RwCamera*)NULL;
}

static RwCamera* CameraBeginUpdate(RwCamera* camera)
{
    RWSRCGLOBAL(curCamera) = (void*)camera;

    _rwFrameSyncDirty();

    if (RWSRCGLOBAL(stdFunc)[rwSTANDARDCAMERABEGINUPDATE](NULL, camera, 0))
    {
        _rwPipeInitForCamera(camera);

        return camera;
    }

    return (RwCamera*)NULL;
}

void* _rwCameraClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWCAMERAGLOBAL(cameraFreeList))
    {
        RwFreeListDestroy(RWCAMERAGLOBAL(cameraFreeList));
        RWCAMERAGLOBAL(cameraFreeList) = (RwFreeList*)NULL;
    }

    cameraModule.numInstances--;

    return instance;
}

void* _rwCameraOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    cameraModule.globalsOffset = offset;

    RWCAMERAGLOBAL(cameraFreeList) =
        RwFreeListCreateAndPreallocateSpace((&cameraTKList)->sizeOfStruct,
                                            _rwCameraFreeListBlockSize, rwCAMERAALIGNMENT,
                                            _rwCameraFreeListPreallocBlocks, &_rwCameraFreeList);

    if (!RWCAMERAGLOBAL(cameraFreeList))
    {
        return NULL;
    }

    cameraModule.numInstances++;

    return instance;
}

RwCamera* RwCameraEndUpdate(RwCamera* camera)
{
    return camera->endUpdate(camera);
}

RwCamera* RwCameraBeginUpdate(RwCamera* camera)
{
    return camera->beginUpdate(camera);
}

RwCamera* RwCameraSetViewOffset(RwCamera* camera, const RwV2d* offset)
{
    camera->viewOffset = *offset;

    if (rwObjectGetParent(camera))
    {
        RwFrameUpdateObjects((RwFrame*)rwObjectGetParent(camera));
    }

    return camera;
}

RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, RwReal nearClip)
{
    camera->nearPlane = nearClip;

    CameraSetZ(camera);

    if (rwObjectGetParent(camera))
    {
        RwFrameUpdateObjects((RwFrame*)rwObjectGetParent(camera));
    }

    return camera;
}

RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, RwReal farClip)
{
    camera->farPlane = farClip;

    CameraSetZ(camera);

    if (rwObjectGetParent(camera))
    {
        RwFrameUpdateObjects((RwFrame*)rwObjectGetParent(camera));
    }

    return camera;
}

RwFrustumTestResult RwCameraFrustumTestSphere(const RwCamera* camera, const RwSphere* sphere)
{
    const RwFrustumPlane* frustumPlane = camera->frustumPlanes;
    RwInt32 numPlanes = 6;
    RwFrustumTestResult result = rwSPHEREINSIDE;
    RwReal radius = sphere->radius;
    RwReal negRadius = -radius;
    RwV3d center;

    center.x = sphere->center.x;
    center.y = sphere->center.y;
    center.z = sphere->center.z;

    while (numPlanes--)
    {
        RwReal nDot = RwV3dDotProductMacro(&center, &frustumPlane->plane.normal) -
                      frustumPlane->plane.distance;

        if (nDot > radius)
        {
            return rwSPHEREOUTSIDE;
        }

        if (nDot > negRadius)
        {
            result = rwSPHEREBOUNDARY;
        }

        frustumPlane++;
    }

    return result;
}

RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, RwInt32 clearMode)
{
    if (!RWSRCGLOBAL(stdFunc)[rwSTANDARDCAMERACLEAR](camera, colour, clearMode))
    {
        return (RwCamera*)NULL;
    }

    return camera;
}

RwCamera* RwCameraShowRaster(RwCamera* camera, void* pDev, RwUInt32 flags)
{
    if (!RwRasterShowRaster(camera->frameBuffer, pDev, flags))
    {
        return (RwCamera*)NULL;
    }

    return camera;
}

RwCamera* RwCameraSetProjection(RwCamera* camera, RwCameraProjection projection)
{
    switch (projection)
    {
    case rwPERSPECTIVE:
    case rwPARALLEL:
        camera->projectionType = projection;

        if (rwObjectGetParent(camera))
        {
            RwFrameUpdateObjects((RwFrame*)rwObjectGetParent(camera));
        }

        CameraSetZ(camera);

        return camera;
    default:
        RWERROR((E_RW_INVCAMERAPROJECTION));
        break;
    }

    return (RwCamera*)NULL;
}

RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow)
{
    camera->viewWindow = *viewWindow;
    camera->recipViewWindow.x = ((RwReal)1) / camera->viewWindow.x;
    camera->recipViewWindow.y = ((RwReal)1) / camera->viewWindow.y;

    if (rwObjectGetParent(camera))
    {
        RwFrameUpdateObjects((RwFrame*)rwObjectGetParent(camera));
    }

    return camera;
}

RwInt32 RwCameraRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                               RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&cameraTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);
}

RwBool RwCameraDestroy(RwCamera* camera)
{
    _rwPluginRegistryDeInitObject(&cameraTKList, camera);

    _rwObjectHasFrameReleaseFrame(camera);

    RwFreeListFree(RWCAMERAGLOBAL(cameraFreeList), camera);

    return TRUE;
}

RwCamera* RwCameraCreate(void)
{
    RwCamera* camera;

    camera = (RwCamera*)RwFreeListAlloc(RWCAMERAGLOBAL(cameraFreeList));
    if (!camera)
    {
        return (RwCamera*)NULL;
    }

    rwObjectInitialize(camera, rwCAMERA, 0);

    camera->object.sync = (RwObjectHasFrameSyncFunction)CameraSync;
    camera->beginUpdate = CameraBeginUpdate;
    camera->endUpdate = CameraEndUpdate;

    camera->viewWindow.x = camera->viewWindow.y = ((RwReal)1);
    camera->recipViewWindow.x = camera->recipViewWindow.y = ((RwReal)1);
    camera->viewOffset.x = camera->viewOffset.y = ((RwReal)0);
    camera->nearPlane = ((RwReal)0.05);
    camera->farPlane = ((RwReal)10.0);
    camera->fogPlane = ((RwReal)5.0);

    camera->frameBuffer = (RwRaster*)NULL;
    camera->zBuffer = (RwRaster*)NULL;

    camera->projectionType = rwPERSPECTIVE;

    CameraSetZ(camera);

    rwMatrixInitialize(&camera->viewMatrix, 0);

    _rwPluginRegistryInitObject(&cameraTKList, camera);

    return camera;
}
