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

RwInt32 _rwCameraFreeListBlockSize = 4;
RwInt32 _rwCameraFreeListPreallocBlocks = 1;

static RwPluginRegistry cameraTKList = { sizeof(RwCamera),        sizeof(RwCamera),       0, 0,
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
    RwFrame* frame = (RwFrame*)rwObjectGetParent(camera);
    RwMatrix* ltm = &frame->ltm;
    RwV3d* corner = camera->frustumCorners;
    RwV3d right;
    RwV3d up;
    RwV3d offset;
    RwInt32 i;

    RwV3dScaleMacro(&right, &ltm->right, camera->viewWindow.x);
    RwV3dScaleMacro(&up, &ltm->up, camera->viewWindow.y);

    corner[0].x = ltm->at.x + right.x + up.x;
    corner[0].y = ltm->at.y + right.y + up.y;
    corner[0].z = ltm->at.z + right.z + up.z;

    corner[1].x = corner[0].x - ((RwReal)2) * right.x;
    corner[1].y = corner[0].y - ((RwReal)2) * right.y;
    corner[1].z = corner[0].z - ((RwReal)2) * right.z;

    corner[2].x = corner[1].x - ((RwReal)2) * up.x;
    corner[2].y = corner[1].y - ((RwReal)2) * up.y;
    corner[2].z = corner[1].z - ((RwReal)2) * up.z;

    corner[3].x = corner[2].x + ((RwReal)2) * right.x;
    corner[3].y = corner[2].y + ((RwReal)2) * right.y;
    corner[3].z = corner[2].z + ((RwReal)2) * right.z;

    offset.x = ltm->right.x * -camera->viewOffset.x + ltm->up.x * camera->viewOffset.y;
    offset.y = ltm->right.y * -camera->viewOffset.x + ltm->up.y * camera->viewOffset.y;
    offset.z = ltm->right.z * -camera->viewOffset.x + ltm->up.z * camera->viewOffset.y;

    for (i = 0; i < 4; i++)
    {
        RwV3d dir = corner[i];

        corner[i].x = offset.x + ltm->pos.x;
        corner[i].y = offset.y + ltm->pos.y;
        corner[i].z = offset.z + ltm->pos.z;
        corner[i].x += (dir.x - offset.x) * camera->nearPlane;
        corner[i].y += (dir.y - offset.y) * camera->nearPlane;
        corner[i].z += (dir.z - offset.z) * camera->nearPlane;

        corner[i + 4].x = offset.x + ltm->pos.x;
        corner[i + 4].y = offset.y + ltm->pos.y;
        corner[i + 4].z = offset.z + ltm->pos.z;
        corner[i + 4].x += (dir.x - offset.x) * camera->farPlane;
        corner[i + 4].y += (dir.y - offset.y) * camera->farPlane;
        corner[i + 4].z += (dir.z - offset.z) * camera->farPlane;
    }

    CameraBuildClipPlanes(camera, ltm);
}

static RwCamera* CameraBuildPerspViewMatrix(RwCamera* camera)
{
    RwFrame* frame = (RwFrame*)rwObjectGetParent(camera);
    RwMatrix* ltm = &frame->ltm;
    RwV3d col;
    RwReal recip;
    RwReal shift;

    recip = ((RwReal)-0.5) * camera->recipViewWindow.x;
    shift = ((RwReal)0.5) - recip * camera->viewOffset.x;

    col.x = ltm->right.x * recip + ltm->at.x * shift;
    col.y = ltm->right.y * recip + ltm->at.y * shift;
    col.z = ltm->right.z * recip + ltm->at.z * shift;

    camera->viewMatrix.right.x = col.x;
    camera->viewMatrix.up.x = col.y;
    camera->viewMatrix.at.x = col.z;
    camera->viewMatrix.pos.x = ((RwReal)0.5) - (shift + RwV3dDotProductMacro(&ltm->pos, &col));

    recip = ((RwReal)-0.5) * camera->recipViewWindow.y;
    shift = recip * camera->viewOffset.y + ((RwReal)0.5);

    col.x = ltm->up.x * recip + ltm->at.x * shift;
    col.y = ltm->up.y * recip + ltm->at.y * shift;
    col.z = ltm->up.z * recip + ltm->at.z * shift;

    camera->viewMatrix.right.y = col.x;
    camera->viewMatrix.up.y = col.y;
    camera->viewMatrix.at.y = col.z;
    camera->viewMatrix.pos.y = ((RwReal)0.5) - (shift + RwV3dDotProductMacro(&ltm->pos, &col));

    camera->viewMatrix.right.z = ltm->at.x;
    camera->viewMatrix.up.z = ltm->at.y;
    camera->viewMatrix.at.z = ltm->at.z;
    camera->viewMatrix.pos.z = -RwV3dDotProductMacro(&ltm->pos, &ltm->at);

    RwMatrixOptimize(&camera->viewMatrix, (const RwMatrixTolerance*)NULL);

    return camera;
}

static void CameraBuildParallelClipPlanes(RwCamera* camera)
{
    RwFrame* frame = (RwFrame*)rwObjectGetParent(camera);
    RwV3d* corner = camera->frustumCorners;
    RwReal viewWindowX = camera->viewWindow.x;
    RwReal viewWindowY = camera->viewWindow.y;
    RwReal nearPlane = camera->nearPlane;
    RwReal farPlane = camera->farPlane;
    RwReal offsetX;
    RwReal offsetY;

    corner[0].z = corner[1].z = corner[2].z = corner[3].z = nearPlane;
    corner[4].z = corner[5].z = corner[6].z = corner[7].z = farPlane;

    offsetX = (((RwReal)1) - nearPlane) * -camera->viewOffset.x;
    offsetY = (((RwReal)1) - nearPlane) * camera->viewOffset.y;

    corner[0].x = corner[3].x = viewWindowX + offsetX;
    corner[1].x = corner[2].x = -viewWindowX + offsetX;
    corner[0].y = corner[1].y = viewWindowY + offsetY;
    corner[2].y = corner[3].y = -viewWindowY + offsetY;

    offsetX = (((RwReal)1) - farPlane) * -camera->viewOffset.x;
    offsetY = (((RwReal)1) - farPlane) * camera->viewOffset.y;

    corner[4].x = corner[7].x = viewWindowX + offsetX;
    corner[5].x = corner[6].x = -viewWindowX + offsetX;
    corner[4].y = corner[5].y = viewWindowY + offsetY;
    corner[6].y = corner[7].y = -viewWindowY + offsetY;

    RwV3dTransformPoints(corner, corner, 8, &frame->ltm);

    CameraBuildClipPlanes(camera, (&frame->ltm));
}

static RwCamera* CameraBuildParallelViewMatrix(RwCamera* camera)
{
    RwFrame* frame = (RwFrame*)rwObjectGetParent(camera);
    RwMatrix* ltm = &frame->ltm;
    RwV3d col;
    RwReal recip;
    RwReal shift;

    recip = ((RwReal)-0.5) * camera->recipViewWindow.x;
    shift = -(recip * camera->viewOffset.x);

    col.x = ltm->right.x * recip + ltm->at.x * shift;
    col.y = ltm->right.y * recip + ltm->at.y * shift;
    col.z = ltm->right.z * recip + ltm->at.z * shift;

    camera->viewMatrix.right.x = col.x;
    camera->viewMatrix.up.x = col.y;
    camera->viewMatrix.at.x = col.z;
    camera->viewMatrix.pos.x = ((RwReal)0.5) - (shift + RwV3dDotProductMacro(&ltm->pos, &col));

    recip = ((RwReal)-0.5) * camera->recipViewWindow.y;
    shift = recip * camera->viewOffset.y;

    col.x = ltm->up.x * recip + ltm->at.x * shift;
    col.y = ltm->up.y * recip + ltm->at.y * shift;
    col.z = ltm->up.z * recip + ltm->at.z * shift;

    camera->viewMatrix.right.y = col.x;
    camera->viewMatrix.up.y = col.y;
    camera->viewMatrix.at.y = col.z;
    camera->viewMatrix.pos.y = ((RwReal)0.5) - (shift + RwV3dDotProductMacro(&ltm->pos, &col));

    camera->viewMatrix.right.z = ltm->at.x;
    camera->viewMatrix.up.z = ltm->at.y;
    camera->viewMatrix.at.z = ltm->at.z;
    camera->viewMatrix.pos.z = -RwV3dDotProductMacro(&ltm->pos, &ltm->at);

    RwMatrixOptimize(&camera->viewMatrix, (const RwMatrixTolerance*)NULL);

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
    RwFrustumTestResult result = rwSPHEREINSIDE;
    const RwFrustumPlane* frustumPlane = camera->frustumPlanes;
    RwInt32 numPlanes = 6;
    RwReal radius = sphere->radius;
    RwReal negRadius = -radius;
    RwV3d center = sphere->center;

    while (numPlanes--)
    {
        RwReal dist = RwV3dDotProductMacro(&center, &frustumPlane->plane.normal) -
                      frustumPlane->plane.distance;

        if (dist > radius)
        {
            return rwSPHEREOUTSIDE;
        }

        if (dist > negRadius)
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
        break;
    default:
        RWERROR((E_RW_INVCAMERAPROJECTION));

        camera = (RwCamera*)NULL;
        break;
    }

    return camera;
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
