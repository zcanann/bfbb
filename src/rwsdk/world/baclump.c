#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwCHUNKHEADERSIZE (sizeof(RwInt32) * 3)

#define rpATOMICPRIVATEWORLDBOUNDDIRTY 0x01

#define rpATOMICSAMEBOUNDINGSPHERE 0x01

#define rpATOMIC 1
#define rpCLUMP 2

#define RwRealMax(a, b) (((a) > (b)) ? (a) : (b))

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rpClumpGlobals rpClumpGlobals;
struct rpClumpGlobals
{
    RwFreeList* atomicFreeList;
    RwFreeList* clumpFreeList;
};

typedef struct rpClumpCameraExt rpClumpCameraExt;
struct rpClumpCameraExt
{
    RpClump* clump;
    RwLLLink inClumpLink;
};

typedef struct rpClumpLightExt rpClumpLightExt;
struct rpClumpLightExt
{
    RpClump* clump;
    RwLLLink inClumpLink;
};

typedef struct rpAtomicChunkInfo rpAtomicChunkInfo;
struct rpAtomicChunkInfo
{
    RwInt32 frameIndex;
    RwInt32 geomIndex;
    RwInt32 flags;
    RwInt32 unused;
};

typedef struct rxPipelineGlobalVars rxPipelineGlobalVars;
struct rxPipelineGlobalVars
{
    RwUInt8 pad[0x3c];
    RxPipeline* platformAtomicPipeline;
};

extern RwInt32 _rxPipelineGlobalsOffset;

#define CAMERAEXTFROMCAMERA(_camera)                                                               \
    (RWPLUGINOFFSET(rpClumpCameraExt, (_camera), _rpClumpCameraExtOffset))
#define CAMERAFROMCAMERAEXT(_ext) (RWPLUGINOFFSET(RwCamera, (_ext), -_rpClumpCameraExtOffset))
#define LIGHTEXTFROMLIGHT(_light)                                                                  \
    (RWPLUGINOFFSET(rpClumpLightExt, (_light), _rpClumpLightExtOffset))
#define LIGHTFROMLIGHTEXT(_ext) (RWPLUGINOFFSET(RpLight, (_ext), -_rpClumpLightExtOffset))

#define RWCLUMPGLOBAL(var)                                                                         \
    (RWPLUGINOFFSET(rpClumpGlobals, RwEngineInstance, clumpModule.globalsOffset)->var)

#define RXPIPELINEGLOBAL(var)                                                                      \
    (RWPLUGINOFFSET(rxPipelineGlobalVars, RwEngineInstance, _rxPipelineGlobalsOffset)->var)

static RwModuleInfo clumpModule;

RwInt32 _rpClumpCameraExtOffset = 0;
RwInt32 _rpClumpLightExtOffset = 0;

RwInt32 _rpAtomicFreeListBlockSize = 128;
RwInt32 _rpAtomicFreeListPreallocBlocks = 1;
RwInt32 _rpClumpFreeListBlockSize = 128;
RwInt32 _rpClumpFreeListPreallocBlocks = 1;

static RwFreeList _rpAtomicFreeList;
static RwFreeList _rpClumpFreeList;

static RwPluginRegistry atomicTKList = { sizeof(RpAtomic),        sizeof(RpAtomic),       0, 0,
                                         (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwPluginRegistry clumpTKList = { sizeof(RpClump),         sizeof(RpClump),        0, 0,
                                        (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwInt32 lastSeenRightsPluginId;
static RwInt32 lastSeenExtraData;

static RpClump* ClumpCallBack(RpClump* clump, void* data)
{
    return clump;
}

static void* ClumpDeInitCameraExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

static void* ClumpDeInitLightExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

static RpAtomic* CountAtomic(RpAtomic* atomic, void* data)
{
    (*((RwInt32*)data))++;

    return atomic;
}

static void* ClumpInitCameraExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    rpClumpCameraExt* cameraExt = CAMERAEXTFROMCAMERA(object);

    cameraExt->clump = (RpClump*)NULL;
    rwLLLinkInitialize(&cameraExt->inClumpLink);

    return object;
}

static void* ClumpInitLightExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    rpClumpLightExt* lightExt = LIGHTEXTFROMLIGHT(object);

    lightExt->clump = (RpClump*)NULL;
    rwLLLinkInitialize(&lightExt->inClumpLink);

    return object;
}

static void ClumpTidyDestroyAtomic(void* pMem, void* pData)
{
    RpAtomicDestroy((RpAtomic*)pMem);
}

static void ClumpTidyDestroyClump(void* pMem, void* pData)
{
    RpClumpDestroy((RpClump*)pMem);
}

RpClump* RpClumpAddAtomic(RpClump* clump, RpAtomic* atomic)
{
    rwLinkListAddLLLink(&clump->atomicList, &atomic->inClumpLink);
    atomic->clump = clump;

    return clump;
}

RpClump* RpClumpRemoveAtomic(RpClump* clump, RpAtomic* atomic)
{
    rwLinkListRemoveLLLink(&atomic->inClumpLink);
    atomic->clump = (RpClump*)NULL;

    return clump;
}

RpClump* RpClumpRemoveLight(RpClump* clump, RpLight* light)
{
    rpClumpLightExt* lightExt = LIGHTEXTFROMLIGHT(light);

    rwLinkListRemoveLLLink(&lightExt->inClumpLink);
    rwLLLinkInitialize(&lightExt->inClumpLink);
    lightExt->clump = (RpClump*)NULL;

    return clump;
}

RpClump* RpClumpRemoveCamera(RpClump* clump, RwCamera* camera)
{
    rpClumpCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    rwLinkListRemoveLLLink(&cameraExt->inClumpLink);
    rwLLLinkInitialize(&cameraExt->inClumpLink);
    cameraExt->clump = (RpClump*)NULL;

    return clump;
}

static RwInt32 _rpSizeAtomicRights(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpAtomic* atomic = (const RpAtomic*)object;

    if (atomic->pipeline && atomic->pipeline->pluginId)
    {
        return sizeof(RwInt32) * 2;
    }

    return 0;
}

RwInt32 RpAtomicGetPluginOffset(RwUInt32 pluginID)
{
    return _rwPluginRegistryGetPluginOffset(&atomicTKList, pluginID);
}

RwInt32 RpAtomicSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB)
{
    return _rwPluginRegistryAddPlgnStrmlwysCB(&atomicTKList, pluginID, alwaysCB);
}

RwInt32 RpAtomicSetStreamRightsCallBack(RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB)
{
    return _rwPluginRegistryAddPlgnStrmRightsCB(&atomicTKList, pluginID, rightsCB);
}

RwInt32 RpAtomicRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                               RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&atomicTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);
}

RwInt32 RpClumpRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&clumpTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);
}

RwInt32 RpAtomicRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    return _rwPluginRegistryAddPluginStream(&atomicTKList, pluginID, readCB, writeCB, getSizeCB);
}

static RwObjectHasFrame* AtomicSync(RwObjectHasFrame* object)
{
    RpAtomic* atomic = (RpAtomic*)object;

    if (atomic->interpolator.flags & rpINTERPOLATORDIRTYSPHERE)
    {
        _rpAtomicResyncInterpolatedSphere(atomic);
    }

    rwObjectSetPrivateFlags(atomic,
                            rwObjectGetPrivateFlags(atomic) | rpATOMICPRIVATEWORLDBOUNDDIRTY);

    return object;
}

RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame)
{
    rwObjectHasFrameSetFrame(atomic, frame);
    rwObjectSetPrivateFlags(atomic,
                            rwObjectGetPrivateFlags(atomic) | rpATOMICPRIVATEWORLDBOUNDDIRTY);

    return atomic;
}

RwInt32 RpClumpGetNumAtomics(RpClump* clump)
{
    RwInt32 numAtomics = 0;

    RpClumpForAllAtomics(clump, CountAtomic, &numAtomics);

    return numAtomics;
}

RwUInt32 RpAtomicStreamGetSize(RpAtomic* atomic)
{
    RwUInt32 size;

    size = rwCHUNKHEADERSIZE + sizeof(rpAtomicChunkInfo);
    size += rwCHUNKHEADERSIZE + RpGeometryStreamGetSize(atomic->geometry);
    size += rwCHUNKHEADERSIZE + _rwPluginRegistryGetSize(&atomicTKList, atomic);

    return size;
}

RpAtomic* AtomicDefaultRenderCallBack(RpAtomic* atomic)
{
    RxPipeline* pipeline;

    pipeline = atomic->pipeline;
    if (!pipeline)
    {
        pipeline = RXPIPELINEGLOBAL(platformAtomicPipeline);
    }

    if (RxPipelineExecute(pipeline, atomic, TRUE))
    {
        return atomic;
    }

    return (RpAtomic*)NULL;
}

RpClump* RpClumpForAllAtomics(RpClump* clump, RpAtomicCallBack callback, void* pData)
{
    RwLLLink* cur;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink(&clump->atomicList);
    end = rwLinkListGetTerminator(&clump->atomicList);
    while (cur != end)
    {
        RpAtomic* atomic = rwLLLinkGetData(cur, RpAtomic, inClumpLink);
        RwLLLink* next = rwLLLinkGetNext(cur);

        if (!callback(atomic, pData))
        {
            return clump;
        }

        cur = next;
    }

    return clump;
}

RpClump* RpClumpForAllCameras(RpClump* clump, RwCameraCallBack callback, void* pData)
{
    RwLLLink* cur;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink(&clump->cameraList);
    end = rwLinkListGetTerminator(&clump->cameraList);
    while (cur != end)
    {
        rpClumpCameraExt* cameraExt = rwLLLinkGetData(cur, rpClumpCameraExt, inClumpLink);
        RwCamera* camera = CAMERAFROMCAMERAEXT(cameraExt);
        RwLLLink* next = rwLLLinkGetNext(cur);

        if (!callback(camera, pData))
        {
            return clump;
        }

        cur = next;
    }

    return clump;
}

RpClump* RpClumpForAllLights(RpClump* clump, RpLightCallBack callback, void* pData)
{
    RwLLLink* cur;
    RwLLLink* end;

    cur = rwLinkListGetFirstLLLink(&clump->lightList);
    end = rwLinkListGetTerminator(&clump->lightList);
    while (cur != end)
    {
        rpClumpLightExt* lightExt = rwLLLinkGetData(cur, rpClumpLightExt, inClumpLink);
        RpLight* light = LIGHTFROMLIGHTEXT(lightExt);
        RwLLLink* next = rwLLLinkGetNext(cur);

        if (!callback(light, pData))
        {
            return clump;
        }

        cur = next;
    }

    return clump;
}

RwStream* _rpReadAtomicRights(RwStream* stream, RwInt32 lengthInBytes)
{
    if (!RwStreamReadInt32(stream, &lastSeenRightsPluginId, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (lengthInBytes == (RwInt32)(sizeof(RwInt32) * 2))
    {
        if (!RwStreamReadInt32(stream, &lastSeenExtraData, sizeof(RwInt32)))
        {
            return (RwStream*)NULL;
        }
    }

    return stream;
}

RwStream* _rpWriteAtomicRights(RwStream* stream, RwInt32 lengthInBytes, const void* object)
{
    if (!RwStreamWriteInt32(stream, (const RwInt32*)&((const RpAtomic*)object)->pipeline->pluginId,
                            sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (!RwStreamWriteInt32(stream,
                            (const RwInt32*)&((const RpAtomic*)object)->pipeline->pluginData,
                            sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    return stream;
}

RwBool _rpClumpRegisterExtensions(void)
{
    _rpClumpCameraExtOffset =
        RwCameraRegisterPlugin(sizeof(rpClumpCameraExt), rwID_CLUMP, ClumpInitCameraExt,
                               ClumpDeInitCameraExt, (RwPluginObjectCopy)NULL);
    if (_rpClumpCameraExtOffset < 0)
    {
        return FALSE;
    }

    _rpClumpLightExtOffset =
        RpLightRegisterPlugin(sizeof(rpClumpLightExt), rwID_CLUMP, ClumpInitLightExt,
                              ClumpDeInitLightExt, (RwPluginObjectCopy)NULL);
    if (_rpClumpLightExtOffset < 0)
    {
        return FALSE;
    }

    return TRUE;
}

RwBool RpAtomicInstance(RpAtomic* atomic)
{
    RpGeometry* geometry = atomic->geometry;

    if (geometry->numMorphTargets != 1)
    {
        return FALSE;
    }

    if (geometry->flags & rpGEOMETRYNATIVE)
    {
        return TRUE;
    }

    if (geometry->repEntry)
    {
        RwResourcesFreeResEntry(geometry->repEntry);
    }

    geometry->flags |= rpGEOMETRYNATIVEINSTANCE;

    atomic->renderCallBack(atomic);

    geometry->flags = (geometry->flags & ~rpGEOMETRYNATIVEINSTANCE) | rpGEOMETRYNATIVE;

    return TRUE;
}

void* _rpClumpClose(void* instance, RwInt32 offset, RwInt32 size)
{
    RwFreeListForAllUsed(RWCLUMPGLOBAL(clumpFreeList), ClumpTidyDestroyClump, NULL);
    RwFreeListForAllUsed(RWCLUMPGLOBAL(atomicFreeList), ClumpTidyDestroyAtomic, NULL);

    RwFreeListDestroy(RWCLUMPGLOBAL(atomicFreeList));
    RwFreeListDestroy(RWCLUMPGLOBAL(clumpFreeList));

    RWCLUMPGLOBAL(atomicFreeList) = (RwFreeList*)NULL;
    RWCLUMPGLOBAL(clumpFreeList) = (RwFreeList*)NULL;

    clumpModule.numInstances--;

    return instance;
}

void* _rpClumpOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    clumpModule.globalsOffset = offset;

    RWCLUMPGLOBAL(atomicFreeList) =
        RwFreeListCreateAndPreallocateSpace(atomicTKList.sizeOfStruct, _rpAtomicFreeListBlockSize,
                                            4, _rpAtomicFreeListPreallocBlocks, &_rpAtomicFreeList);
    if (RWCLUMPGLOBAL(atomicFreeList))
    {
        RWCLUMPGLOBAL(clumpFreeList) =
            RwFreeListCreateAndPreallocateSpace(clumpTKList.sizeOfStruct, _rpClumpFreeListBlockSize,
                                                4, _rpClumpFreeListPreallocBlocks,
                                                &_rpClumpFreeList);
        if (RWCLUMPGLOBAL(clumpFreeList))
        {
            clumpModule.numInstances++;

            return instance;
        }

        RwFreeListDestroy(RWCLUMPGLOBAL(atomicFreeList));
        RWCLUMPGLOBAL(atomicFreeList) = (RwFreeList*)NULL;
    }

    return NULL;
}

void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic)
{
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);
    RwInt32 startMT;
    RwInt32 endMT;

    if (!geometry)
    {
        return;
    }

    startMT = atomic->interpolator.startMorphTarget;
    endMT = atomic->interpolator.endMorphTarget;

    if ((startMT != endMT) && (startMT < geometry->numMorphTargets) &&
        (endMT < geometry->numMorphTargets))
    {
        const RpMorphTarget* mtA = &geometry->morphTarget[startMT];
        const RpMorphTarget* mtB = &geometry->morphTarget[endMT];
        RwReal scale = atomic->interpolator.recipTime * atomic->interpolator.position;

        atomic->boundingSphere.radius =
            mtA->boundingSphere.radius +
            scale * (mtB->boundingSphere.radius - mtA->boundingSphere.radius);

        RwV3dSubMacro(&atomic->boundingSphere.center, &mtB->boundingSphere.center,
                      &mtA->boundingSphere.center);
        RwV3dScaleMacro(&atomic->boundingSphere.center, &atomic->boundingSphere.center, scale);
        RwV3dAddMacro(&atomic->boundingSphere.center, &atomic->boundingSphere.center,
                      &mtA->boundingSphere.center);
    }
    else
    {
        if ((startMT >= geometry->numMorphTargets) || (endMT >= geometry->numMorphTargets))
        {
            atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
        }
        else
        {
            atomic->boundingSphere = geometry->morphTarget[startMT].boundingSphere;
        }
    }

    atomic->interpolator.flags &= ~rpINTERPOLATORDIRTYSPHERE;
    rwObjectSetPrivateFlags(atomic,
                            rwObjectGetPrivateFlags(atomic) | rpATOMICPRIVATEWORLDBOUNDDIRTY);
}

const RwSphere* RpAtomicGetWorldBoundingSphere(RpAtomic* atomic)
{
    RwFrame* frame = RpAtomicGetFrame(atomic);

    if (atomic->interpolator.flags & rpINTERPOLATORDIRTYSPHERE)
    {
        _rpAtomicResyncInterpolatedSphere(atomic);
    }

    if (RwFrameDirty(frame) || rwObjectTestPrivateFlags(atomic, rpATOMICPRIVATEWORLDBOUNDDIRTY))
    {
        RwMatrix* ltm = RwFrameGetLTM(frame);

        RwV3dTransformPoints(&atomic->worldBoundingSphere.center, &atomic->boundingSphere.center, 1,
                             ltm);

        if ((rwMatrixGetFlags(ltm) & rwMATRIXTYPEMASK) == rwMATRIXTYPEORTHONORMAL)
        {
            atomic->worldBoundingSphere.radius = atomic->boundingSphere.radius;
        }
        else
        {
            RwReal xScl = RwV3dDotProductMacro(&ltm->right, &ltm->right);
            RwReal yScl = RwV3dDotProductMacro(&ltm->up, &ltm->up);
            RwReal zScl = RwV3dDotProductMacro(&ltm->at, &ltm->at);
            RwReal scl = RwRealMax(xScl, RwRealMax(zScl, yScl));

            atomic->worldBoundingSphere.radius = atomic->boundingSphere.radius * _rwSqrt(scl);
        }

        rwObjectSetPrivateFlags(atomic,
                                rwObjectGetPrivateFlags(atomic) & ~rpATOMICPRIVATEWORLDBOUNDDIRTY);
    }

    return &atomic->worldBoundingSphere;
}

RpAtomic* RpAtomicCreate(void)
{
    RpAtomic* atomic;

    atomic = (RpAtomic*)RWSRCGLOBAL(memoryAlloc)(RWCLUMPGLOBAL(atomicFreeList));
    if (!atomic)
    {
        return (RpAtomic*)NULL;
    }

    rwObjectInitialize(atomic, rpATOMIC, 0);
    ((RwObjectHasFrame*)atomic)->sync = AtomicSync;
    atomic->repEntry = (RwResEntry*)NULL;

    rwObjectSetFlags(atomic, rpATOMICCOLLISIONTEST | rpATOMICRENDER);
    rwObjectSetPrivateFlags(atomic, rpATOMICPRIVATEWORLDBOUNDDIRTY);

    RpAtomicSetFrame(atomic, (RwFrame*)NULL);

    atomic->geometry = (RpGeometry*)NULL;

    atomic->boundingSphere.center.x = (RwReal)0.0;
    atomic->boundingSphere.center.y = (RwReal)0.0;
    atomic->boundingSphere.center.z = (RwReal)0.0;
    atomic->boundingSphere.radius = (RwReal)0.0;

    atomic->worldBoundingSphere.center.x = (RwReal)0.0;
    atomic->worldBoundingSphere.center.y = (RwReal)0.0;
    atomic->worldBoundingSphere.center.z = (RwReal)0.0;
    atomic->worldBoundingSphere.radius = (RwReal)0.0;

    RpAtomicSetRenderCallBack(atomic, AtomicDefaultRenderCallBack);

    atomic->interpolator.startMorphTarget = 0;
    atomic->interpolator.endMorphTarget = 0;
    atomic->interpolator.time = (RwReal)1.0;
    atomic->interpolator.recipTime = (RwReal)1.0;
    atomic->interpolator.position = (RwReal)0.0;
    atomic->interpolator.flags = rpINTERPOLATORDIRTYINSTANCE | rpINTERPOLATORDIRTYSPHERE;

    rwLLLinkInitialize(&atomic->inClumpLink);
    atomic->clump = (RpClump*)NULL;

    atomic->pipeline = (RxPipeline*)NULL;

    rwLinkListInitialize(&atomic->llWorldSectorsInAtomic);

    _rwPluginRegistryInitObject(&atomicTKList, atomic);

    return atomic;
}

RwBool RpAtomicDestroy(RpAtomic* atomic)
{
    _rwPluginRegistryDeInitObject(&atomicTKList, atomic);

    if (atomic->repEntry)
    {
        RwResourcesFreeResEntry(atomic->repEntry);
    }

    if (atomic->geometry)
    {
        RpAtomicSetGeometry(atomic, (RpGeometry*)NULL, 0);
    }

    _rwObjectHasFrameReleaseFrame(atomic);

    RWSRCGLOBAL(memoryFree)(RWCLUMPGLOBAL(atomicFreeList), atomic);

    return TRUE;
}

RpAtomic* RpAtomicSetGeometry(RpAtomic* atomic, RpGeometry* geometry, RwUInt32 flags)
{
    if (geometry != atomic->geometry)
    {
        if (geometry)
        {
            _rpGeometryAddRef(geometry);
        }

        if (atomic->geometry)
        {
            RpGeometryDestroy(atomic->geometry);
        }

        atomic->geometry = geometry;

        if (!(flags & rpATOMICSAMEBOUNDINGSPHERE))
        {
            RwFrame* frame;

            if (geometry)
            {
                atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
            }

            frame = RpAtomicGetFrame(atomic);
            if (frame && RpAtomicGetWorld(atomic))
            {
                RwFrameUpdateObjects(frame);
            }
        }
    }

    return atomic;
}

RwBool RpClumpDestroy(RpClump* clump)
{
    RwLLLink* cur;
    RwLLLink* next;
    RwLLLink* end;

    _rwPluginRegistryDeInitObject(&clumpTKList, clump);

    cur = rwLinkListGetFirstLLLink(&clump->atomicList);
    end = rwLinkListGetTerminator(&clump->atomicList);
    while (cur != end)
    {
        RpAtomic* atomic = rwLLLinkGetData(cur, RpAtomic, inClumpLink);

        next = rwLLLinkGetNext(cur);
        RpAtomicDestroy(atomic);
        cur = next;
    }

    cur = rwLinkListGetFirstLLLink(&clump->lightList);
    end = rwLinkListGetTerminator(&clump->lightList);
    while (cur != end)
    {
        rpClumpLightExt* lightExt = rwLLLinkGetData(cur, rpClumpLightExt, inClumpLink);
        RpLight* light = LIGHTFROMLIGHTEXT(lightExt);

        next = rwLLLinkGetNext(cur);
        RpClumpRemoveLight(lightExt->clump, light);
        RpLightDestroy(light);
        cur = next;
    }

    cur = rwLinkListGetFirstLLLink(&clump->cameraList);
    end = rwLinkListGetTerminator(&clump->cameraList);
    while (cur != end)
    {
        rpClumpCameraExt* cameraExt = rwLLLinkGetData(cur, rpClumpCameraExt, inClumpLink);
        RwCamera* camera = CAMERAFROMCAMERAEXT(cameraExt);

        next = rwLLLinkGetNext(cur);
        RpClumpRemoveCamera(cameraExt->clump, camera);
        RwCameraDestroy(camera);
        cur = next;
    }

    if (RpClumpGetFrame(clump))
    {
        RwFrameDestroyHierarchy(RpClumpGetFrame(clump));
    }

    RWSRCGLOBAL(memoryFree)(RWCLUMPGLOBAL(clumpFreeList), clump);

    return TRUE;
}
