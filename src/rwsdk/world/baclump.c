#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include <string.h>

#define rwPLUGIN_ID 2

#define RwStreamWriteChunkHeader(stream, type, size)                                               \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, 0xFFFF)

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_BADVERSION 0x80000004
#define E_RW_READ 0x8000001A
#define E_RW_NOMEM 0x80000013

#define rwCHUNKHEADERSIZE (sizeof(RwInt32) * 3)

#define rpATOMICPRIVATEWORLDBOUNDDIRTY 0x01

#define rpATOMICSAMEBOUNDINGSPHERE 0x01

#define rpATOMIC 1
#define rpCLUMP 2

#define RwRealMax(a, b) (((a) >= (b)) ? (a) : (b))

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

typedef struct rpGeometryList rpGeometryList;
struct rpGeometryList
{
    RpGeometry** geometries;
    RwInt32 numGeoms;
};

typedef struct RpClumpChunkInfo RpClumpChunkInfo;
struct RpClumpChunkInfo
{
    RwInt32 numAtomics;
    RwInt32 numLights;
    RwInt32 numCameras;
};

typedef struct RpClumpStreamWriteStatus RpClumpStreamWriteStatus;
struct RpClumpStreamWriteStatus
{
    RwStream* stream;
    rwFrameList fl;
    rpGeometryList gl;
    RwBool success;
};

extern RwInt32 _rxPipelineGlobalsOffset;

extern RpClump* RpClumpCreate(void);
extern RpClump* RpClumpAddLight(RpClump* clump, RpLight* light);
extern RpClump* RpClumpAddCamera(RpClump* clump, RwCamera* camera);

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

static RwUInt32 lastSeenRightsPluginId;
static RwUInt32 lastSeenExtraData;

static void ClumpTidyDestroyClump(void* pMem, void* pData)
{
    RpClumpDestroy((RpClump*)pMem);
}

static void ClumpTidyDestroyAtomic(void* pMem, void* pData)
{
    RpAtomicDestroy((RpAtomic*)pMem);
}

RwStream* _rpReadAtomicRights(RwStream* stream, RwInt32 lengthInBytes)
{
    if (!RwStreamReadInt32(stream, (RwInt32*)&lastSeenRightsPluginId, sizeof(RwInt32)))
    {
        return (RwStream*)NULL;
    }

    if (lengthInBytes == (RwInt32)(sizeof(RwInt32) * 2))
    {
        if (!RwStreamReadInt32(stream, (RwInt32*)&lastSeenExtraData, sizeof(RwInt32)))
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

RwInt32 _rpSizeAtomicRights(const void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    const RpAtomic* atomic = (const RpAtomic*)object;

    if (atomic->pipeline && atomic->pipeline->pluginId)
    {
        return sizeof(RwInt32) * 2;
    }

    return 0;
}

static RpAtomic* CountAtomic(RpAtomic* atomic, void* data)
{
    (*((RwInt32*)data))++;

    return atomic;
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

static rpGeometryList* GeometryListDeinitialize(rpGeometryList* geomList)
{
    RwInt32 i;

    for (i = 0; i < geomList->numGeoms; i++)
    {
        RpGeometryDestroy(geomList->geometries[i]);
    }

    if (geomList->geometries)
    {
        RwFree(geomList->geometries);
        geomList->geometries = (RpGeometry**)NULL;
    }

    return geomList;
}

static rpGeometryList* GeometryListStreamRead(RwStream* stream, rpGeometryList* geomList)
{
    RwInt32 gl;
    RwInt32 i;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return (rpGeometryList*)NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        if (RwStreamRead(stream, &gl, sizeof(gl)) != sizeof(gl))
        {
            return (rpGeometryList*)NULL;
        }

        RwMemNative32(&gl, sizeof(gl));

        geomList->numGeoms = 0;

        if (gl > 0)
        {
            geomList->geometries = (RpGeometry**)RwMalloc(sizeof(RpGeometry*) * gl);
            if (!geomList->geometries)
            {
                RWERROR((E_RW_NOMEM, sizeof(RpGeometry*) * gl));
                return (rpGeometryList*)NULL;
            }
        }
        else
        {
            geomList->geometries = (RpGeometry**)NULL;
        }

        for (i = 0; i < gl; i++)
        {
            if (!RwStreamFindChunk(stream, rwID_GEOMETRY, (RwUInt32*)NULL, &version))
            {
                GeometryListDeinitialize(geomList);
                return (rpGeometryList*)NULL;
            }

            if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
            {
                if (!(geomList->geometries[i] = RpGeometryStreamRead(stream)))
                {
                    GeometryListDeinitialize(geomList);
                    return (rpGeometryList*)NULL;
                }

                geomList->numGeoms++;
            }
            else
            {
                GeometryListDeinitialize(geomList);
                return (rpGeometryList*)NULL;
            }
        }
    }
    else
    {
        RWERROR((E_RW_BADVERSION));
        return (rpGeometryList*)NULL;
    }

    return geomList;
}

static void* ClumpInitCameraExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    rpClumpCameraExt* cameraExt = CAMERAEXTFROMCAMERA(object);

    rwLLLinkInitialize(&cameraExt->inClumpLink);
    cameraExt->clump = (RpClump*)NULL;

    return object;
}

static void* ClumpDeInitCameraExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

static void* ClumpInitLightExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    rpClumpLightExt* lightExt = LIGHTEXTFROMLIGHT(object);

    rwLLLinkInitialize(&lightExt->inClumpLink);
    lightExt->clump = (RpClump*)NULL;

    return object;
}

static void* ClumpDeInitLightExt(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

static RpClump* ClumpCallBack(RpClump* clump, void* data)
{
    return clump;
}

static RwBool GeometryListFindGeometry(const rpGeometryList* geomList, const RpGeometry* geom,
                                       RwInt32* npIndex)
{
    RwInt32 i;

    for (i = 0; i < geomList->numGeoms; i++)
    {
        if (geomList->geometries[i] == geom)
        {
            if (npIndex)
            {
                *npIndex = i;
            }

            return TRUE;
        }
    }

    return FALSE;
}

static RwUInt32 ClumpAtomicStreamGetSize(RpAtomic* atomic)
{
    RwUInt32 size;

    size = rwCHUNKHEADERSIZE + sizeof(rpAtomicChunkInfo);
    size += rwCHUNKHEADERSIZE + _rwPluginRegistryGetSize(&atomicTKList, atomic);

    return size;
}

static RpAtomic* ClumpAtomicStreamWrite(RpAtomic* atomic, void* pData)
{
    RpClumpStreamWriteStatus* status = (RpClumpStreamWriteStatus*)pData;
    rpAtomicChunkInfo a;

    if (!status->gl.numGeoms)
    {
        if (!RwStreamWriteChunkHeader(status->stream, rwID_ATOMIC, RpAtomicStreamGetSize(atomic)))
        {
            status->success = FALSE;
            return (RpAtomic*)NULL;
        }
    }
    else
    {
        if (!RwStreamWriteChunkHeader(status->stream, rwID_ATOMIC,
                                      ClumpAtomicStreamGetSize(atomic)))
        {
            status->success = FALSE;
            return (RpAtomic*)NULL;
        }
    }

    if (!RwStreamWriteChunkHeader(status->stream, rwID_STRUCT, sizeof(rpAtomicChunkInfo)))
    {
        return (RpAtomic*)NULL;
    }

    a.flags = RpAtomicGetFlags(atomic);
    a.unused = 0;

    if (status->fl.numFrames)
    {
        if (!_rwFrameListFindFrame(&status->fl, RpAtomicGetFrame(atomic), &a.frameIndex))
        {
            status->success = FALSE;
            return (RpAtomic*)NULL;
        }
    }

    if (status->gl.numGeoms)
    {
        if (!GeometryListFindGeometry(&status->gl, atomic->geometry, &a.geomIndex))
        {
            status->success = FALSE;
            return (RpAtomic*)NULL;
        }
    }

    RwMemLittleEndian32(&a, sizeof(rpAtomicChunkInfo));

    if (!RwStreamWrite(status->stream, &a, sizeof(rpAtomicChunkInfo)))
    {
        status->success = FALSE;
        return (RpAtomic*)NULL;
    }

    if (!status->gl.numGeoms)
    {
        if (!RpGeometryStreamWrite(atomic->geometry, status->stream))
        {
            status->success = FALSE;
            return (RpAtomic*)NULL;
        }
    }

    if (!_rwPluginRegistryWriteDataChunks(&atomicTKList, status->stream, atomic))
    {
        status->success = FALSE;
        return (RpAtomic*)NULL;
    }

    return atomic;
}

static RpAtomic* ClumpAtomicStreamRead(RwStream* stream, rwFrameList* fl, rpGeometryList* gl)
{
    RwBool status;
    RwUInt32 size;
    RwUInt32 version;
    RpAtomic* atom;
    rpAtomicChunkInfo a;
    RpGeometry* geom;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        RWERROR((E_RW_READ));
        return (RpAtomic*)NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        memset(&a, 0, sizeof(a));

        status = (RwStreamRead(stream, &a, size) == size);
        if (!status)
        {
            RWERROR((E_RW_READ));
            return (RpAtomic*)NULL;
        }

        RwMemNative32(&a, sizeof(a));

        atom = RpAtomicCreate();
        if (!atom)
        {
            return (RpAtomic*)NULL;
        }

        rwObjectSetFlags(atom, a.flags);

        if (fl->numFrames)
        {
            RpAtomicSetFrame(atom, fl->frames[a.frameIndex]);
        }

        if (gl->numGeoms)
        {
            RpAtomicSetGeometry(atom, gl->geometries[a.geomIndex], 0);
        }
        else
        {
            if (!RwStreamFindChunk(stream, rwID_GEOMETRY, (RwUInt32*)NULL, &version))
            {
                RpAtomicDestroy(atom);
                RWERROR((E_RW_READ));
                return (RpAtomic*)NULL;
            }

            if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
            {
                geom = RpGeometryStreamRead(stream);
                if (!geom)
                {
                    RpAtomicDestroy(atom);
                    RWERROR((E_RW_READ));
                    return (RpAtomic*)NULL;
                }
            }
            else
            {
                RpAtomicDestroy(atom);
                RWERROR((E_RW_BADVERSION));
                return (RpAtomic*)NULL;
            }

            RpAtomicSetGeometry(atom, geom, 0);
            RpGeometryDestroy(geom);
        }

        lastSeenRightsPluginId = 0;
        lastSeenExtraData = 0;

        status = (NULL != _rwPluginRegistryReadDataChunks(&atomicTKList, stream, atom));
        if (!status)
        {
            RWERROR((E_RW_READ));
            return (RpAtomic*)NULL;
        }

        if (lastSeenRightsPluginId)
        {
            _rwPluginRegistryInvokeRights(&atomicTKList, lastSeenRightsPluginId, atom,
                                          lastSeenExtraData);
        }

        return atom;
    }

    RWERROR((E_RW_BADVERSION));
    return (RpAtomic*)NULL;
}

void _rpAtomicResyncInterpolatedSphere(RpAtomic* atomic)
{
    RpGeometry* geometry = RpAtomicGetGeometry(atomic);

    if (!geometry)
    {
        return;
    }

    if ((atomic->interpolator.startMorphTarget == atomic->interpolator.endMorphTarget) ||
        (atomic->interpolator.startMorphTarget >= geometry->numMorphTargets) ||
        (atomic->interpolator.endMorphTarget >= geometry->numMorphTargets))
    {
        if ((atomic->interpolator.startMorphTarget >= geometry->numMorphTargets) ||
            (atomic->interpolator.endMorphTarget >= geometry->numMorphTargets))
        {
            atomic->boundingSphere = geometry->morphTarget[0].boundingSphere;
        }
        else
        {
            atomic->boundingSphere =
                geometry->morphTarget[atomic->interpolator.startMorphTarget].boundingSphere;
        }
    }
    else
    {
        const RpMorphTarget* mtA = &geometry->morphTarget[atomic->interpolator.startMorphTarget];
        const RpMorphTarget* mtB = &geometry->morphTarget[atomic->interpolator.endMorphTarget];
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

        if ((rwMatrixGetFlags(ltm) & rwMATRIXTYPEMASK) != rwMATRIXTYPEORTHONORMAL)
        {
            RwReal xScl = RwV3dDotProductMacro(&ltm->right, &ltm->right);
            RwReal yScl = RwV3dDotProductMacro(&ltm->up, &ltm->up);
            RwReal zScl = RwV3dDotProductMacro(&ltm->at, &ltm->at);
            RwReal scl = RwRealMax(xScl, RwRealMax(yScl, zScl));

            atomic->worldBoundingSphere.radius = atomic->boundingSphere.radius * _rwSqrt(scl);
        }
        else
        {
            atomic->worldBoundingSphere.radius = atomic->boundingSphere.radius;
        }

        rwObjectSetPrivateFlags(atomic,
                                rwObjectGetPrivateFlags(atomic) & ~rpATOMICPRIVATEWORLDBOUNDDIRTY);
    }

    return &atomic->worldBoundingSphere;
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

RwInt32 RpClumpGetNumAtomics(RpClump* clump)
{
    RwInt32 numAtomics = 0;

    RpClumpForAllAtomics(clump, CountAtomic, &numAtomics);

    return numAtomics;
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

    atomic->boundingSphere.radius = (RwReal)0.0;
    atomic->boundingSphere.center.x = (RwReal)0.0;
    atomic->boundingSphere.center.y = (RwReal)0.0;
    atomic->boundingSphere.center.z = (RwReal)0.0;

    atomic->worldBoundingSphere.radius = (RwReal)0.0;
    atomic->worldBoundingSphere.center.x = (RwReal)0.0;
    atomic->worldBoundingSphere.center.y = (RwReal)0.0;
    atomic->worldBoundingSphere.center.z = (RwReal)0.0;

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

RpClump* RpClumpCreate(void)
{
    RpClump* clump;

    clump = (RpClump*)RWSRCGLOBAL(memoryAlloc)(RWCLUMPGLOBAL(clumpFreeList));
    if (!clump)
    {
        return clump;
    }

    rwObjectInitialize(clump, rpCLUMP, 0);
    RpClumpSetFrame(clump, (RwFrame*)NULL);

    rwLinkListInitialize(&clump->atomicList);
    rwLinkListInitialize(&clump->lightList);
    rwLinkListInitialize(&clump->cameraList);

    rwLLLinkInitialize(&clump->inWorldLink);

    clump->callback = ClumpCallBack;

    _rwPluginRegistryInitObject(&clumpTKList, clump);

    return clump;
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

RwBool RpAtomicDestroy(RpAtomic* atomic)
{
    _rwPluginRegistryDeInitObject(&atomicTKList, atomic);

    if (atomic->repEntry)
    {
        RwResourcesFreeResEntry(atomic->repEntry);
    }

    RpAtomicSetGeometry(atomic, (RpGeometry*)NULL, 0);

    _rwObjectHasFrameReleaseFrame(atomic);

    RWSRCGLOBAL(memoryFree)(RWCLUMPGLOBAL(atomicFreeList), atomic);

    return TRUE;
}

static RpAtomic* ClumpDestroyAtomic(RpAtomic* atomic, void* data)
{
    RpAtomicDestroy(atomic);

    return atomic;
}

static RpLight* ClumpDestroyLight(RpLight* light, void* data)
{
    RpClumpRemoveLight(LIGHTEXTFROMLIGHT(light)->clump, light);
    RpLightDestroy(light);

    return light;
}

static RwCamera* ClumpDestroyCamera(RwCamera* camera, void* data)
{
    RpClumpRemoveCamera(CAMERAEXTFROMCAMERA(camera)->clump, camera);
    RwCameraDestroy(camera);

    return camera;
}

RwBool RpClumpDestroy(RpClump* clump)
{
    _rwPluginRegistryDeInitObject(&clumpTKList, clump);

    RpClumpForAllAtomics(clump, ClumpDestroyAtomic, NULL);
    RpClumpForAllLights(clump, ClumpDestroyLight, NULL);
    RpClumpForAllCameras(clump, ClumpDestroyCamera, NULL);

    if (RpClumpGetFrame(clump))
    {
        RwFrameDestroyHierarchy(RpClumpGetFrame(clump));
    }

    RWSRCGLOBAL(memoryFree)(RWCLUMPGLOBAL(clumpFreeList), clump);

    return TRUE;
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

RpClump* RpClumpAddLight(RpClump* clump, RpLight* light)
{
    rpClumpLightExt* lightExt = LIGHTEXTFROMLIGHT(light);

    rwLinkListAddLLLink(&clump->lightList, &lightExt->inClumpLink);
    lightExt->clump = clump;

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

RpClump* RpClumpAddCamera(RpClump* clump, RwCamera* camera)
{
    rpClumpCameraExt* cameraExt = CAMERAEXTFROMCAMERA(camera);

    rwLinkListAddLLLink(&clump->cameraList, &cameraExt->inClumpLink);
    cameraExt->clump = clump;

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

RwUInt32 RpAtomicStreamGetSize(RpAtomic* atomic)
{
    RwUInt32 size;

    size = rwCHUNKHEADERSIZE + sizeof(rpAtomicChunkInfo);
    size += rwCHUNKHEADERSIZE + RpGeometryStreamGetSize(atomic->geometry);
    size += rwCHUNKHEADERSIZE + _rwPluginRegistryGetSize(&atomicTKList, atomic);

    return size;
}

RpAtomic* RpAtomicStreamWrite(RpAtomic* atomic, RwStream* stream)
{
    RpClumpStreamWriteStatus status;

    status.stream = stream;
    status.success = TRUE;
    status.fl.numFrames = 0;
    status.gl.numGeoms = 0;

    return ClumpAtomicStreamWrite(atomic, &status);
}

RpAtomic* RpAtomicStreamRead(RwStream* stream)
{
    rwFrameList fl;
    rpGeometryList gl;

    fl.numFrames = 0;
    gl.numGeoms = 0;

    return ClumpAtomicStreamRead(stream, &fl, &gl);
}

RpClump* RpClumpStreamRead(RwStream* stream)
{
    RwBool status;
    RwUInt32 size;
    RwUInt32 version;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        RWERROR((E_RW_READ));
        return (RpClump*)NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        RpClump* clump;
        RpAtomic* atom;
        RpClumpChunkInfo cl;
        rwFrameList fl;
        rpGeometryList gl;
        RwInt32 i;
        RwUInt32 chunkversion;

        if (version <= rwLIBRARYWARNVERSION)
        {
            RwInt32 numAtomics;

            status = (RwStreamRead(stream, &numAtomics, sizeof(numAtomics)) == sizeof(numAtomics));
            if (!status)
            {
                RWERROR((E_RW_READ));
                return (RpClump*)NULL;
            }

            RwMemNative32(&numAtomics, sizeof(numAtomics));

            cl.numAtomics = numAtomics;
            cl.numLights = 0;
            cl.numCameras = 0;
        }
        else
        {
            status = (RwStreamRead(stream, &cl, sizeof(cl)) == sizeof(cl));
            if (!status)
            {
                RWERROR((E_RW_READ));
                return (RpClump*)NULL;
            }

            RwMemNative32(&cl, sizeof(cl));
        }

        clump = RpClumpCreate();
        if (!clump)
        {
            return (RpClump*)NULL;
        }

        if (!RwStreamFindChunk(stream, rwID_FRAMELIST, (RwUInt32*)NULL, &chunkversion))
        {
            RpClumpDestroy(clump);
            RWERROR((E_RW_READ));
            return (RpClump*)NULL;
        }

        status = (NULL != _rwFrameListStreamRead(stream, &fl));
        if (!status)
        {
            RpClumpDestroy(clump);
            RWERROR((E_RW_READ));
            return (RpClump*)NULL;
        }

        RpClumpSetFrame(clump, fl.frames[0]);

        if (!RwStreamFindChunk(stream, rwID_GEOMETRYLIST, (RwUInt32*)NULL, &chunkversion))
        {
            _rwFrameListDeinitialize(&fl);
            RpClumpDestroy(clump);
            RWERROR((E_RW_READ));
            return (RpClump*)NULL;
        }

        status = (NULL != GeometryListStreamRead(stream, &gl));
        if (!status)
        {
            _rwFrameListDeinitialize(&fl);
            RpClumpDestroy(clump);
            RWERROR((E_RW_READ));
            return (RpClump*)NULL;
        }

        for (i = 0; i < cl.numAtomics; i++)
        {
            status = RwStreamFindChunk(stream, rwID_ATOMIC, (RwUInt32*)NULL, &version);
            if (status)
            {
                atom = ClumpAtomicStreamRead(stream, &fl, &gl);
                status = (atom != NULL);
            }

            if (!status)
            {
                GeometryListDeinitialize(&gl);
                _rwFrameListDeinitialize(&fl);
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return (RpClump*)NULL;
            }

            RpClumpAddAtomic(clump, atom);
        }

        for (i = 0; i < cl.numLights; i++)
        {
            RpLight* light;
            RwInt32 frameIndex;

            status = RwStreamFindChunk(stream, rwID_STRUCT, (RwUInt32*)NULL, (RwUInt32*)NULL) &&
                     RwStreamReadInt32(stream, &frameIndex, sizeof(RwInt32));
            if (status)
            {
                status = RwStreamFindChunk(stream, rwID_LIGHT, (RwUInt32*)NULL, (RwUInt32*)NULL);
            }

            if (status)
            {
                light = RpLightStreamRead(stream);
                status = (light != NULL);
            }

            if (!status)
            {
                GeometryListDeinitialize(&gl);
                _rwFrameListDeinitialize(&fl);
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return (RpClump*)NULL;
            }

            RpLightSetFrame(light, fl.frames[frameIndex]);
            RpClumpAddLight(clump, light);
        }

        for (i = 0; i < cl.numCameras; i++)
        {
            RwCamera* camera;
            RwInt32 frameIndex;

            status = RwStreamFindChunk(stream, rwID_STRUCT, (RwUInt32*)NULL, (RwUInt32*)NULL) &&
                     RwStreamReadInt32(stream, &frameIndex, sizeof(RwInt32));
            if (status)
            {
                status = RwStreamFindChunk(stream, rwID_CAMERA, (RwUInt32*)NULL, (RwUInt32*)NULL);
            }

            if (status)
            {
                camera = RwCameraStreamRead(stream);
                status = (camera != NULL);
            }

            if (!status)
            {
                GeometryListDeinitialize(&gl);
                _rwFrameListDeinitialize(&fl);
                RpClumpDestroy(clump);
                RWERROR((E_RW_READ));
                return (RpClump*)NULL;
            }

            RwCameraSetFrame(camera, fl.frames[frameIndex]);
            RpClumpAddCamera(clump, camera);
        }

        GeometryListDeinitialize(&gl);
        _rwFrameListDeinitialize(&fl);

        status = (NULL != _rwPluginRegistryReadDataChunks(&clumpTKList, stream, clump));
        if (!status)
        {
            RpClumpDestroy(clump);
            RWERROR((E_RW_READ));
            return (RpClump*)NULL;
        }

        return clump;
    }

    RWERROR((E_RW_BADVERSION));
    return (RpClump*)NULL;
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

RwInt32 RpAtomicSetStreamAlwaysCallBack(RwUInt32 pluginID, RwPluginDataChunkAlwaysCallBack alwaysCB)
{
    return _rwPluginRegistryAddPlgnStrmlwysCB(&atomicTKList, pluginID, alwaysCB);
}

RwInt32 RpAtomicSetStreamRightsCallBack(RwUInt32 pluginID, RwPluginDataChunkRightsCallBack rightsCB)
{
    return _rwPluginRegistryAddPlgnStrmRightsCB(&atomicTKList, pluginID, rightsCB);
}

RwInt32 RpAtomicGetPluginOffset(RwUInt32 pluginID)
{
    return _rwPluginRegistryGetPluginOffset(&atomicTKList, pluginID);
}

RpAtomic* RpAtomicSetFrame(RpAtomic* atomic, RwFrame* frame)
{
    rwObjectHasFrameSetFrame(atomic, frame);
    rwObjectSetPrivateFlags(atomic,
                            rwObjectGetPrivateFlags(atomic) | rpATOMICPRIVATEWORLDBOUNDDIRTY);

    return atomic;
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
