#include <string.h>
#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

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

#define E_RW_BADENGINESTATE 0x80000001
#define E_RW_NOMEM 0x80000013
#define E_RW_NULLP 0x80000016
#define E_RW_DEVICEERROR 0x18

#define MAKECHUNKID(vendorID, chunkID) (((vendorID & 0xFFFFFF) << 8) | (chunkID & 0xFF))

#define rwID_VECTORMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x01)
#define rwID_MATRIXMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x02)
#define rwID_FRAMEMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x03)
#define rwID_STREAMMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x04)
#define rwID_CAMERAMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x05)
#define rwID_IMAGEMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x06)
#define rwID_RASTERMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x07)
#define rwID_TEXTUREMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x08)
#define rwID_PIPEMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x09)
#define rwID_IMMEDIATEMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0A)
#define rwID_RESOURCESMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0B)
#define rwID_COLORMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0D)
#define rwID_ERRORMODULE MAKECHUNKID(rwVENDORID_CRITERIONINT, 0x0F)

/* Sizes of each core module's per-engine globals */
#define rwERRORGLOBALSSIZE 0x08
#define rwVECTORGLOBALSSIZE 0x10
#define rwCOLORGLOBALSSIZE 0x00
#define rwMATRIXGLOBALSSIZE 0x18
#define rwFRAMEGLOBALSSIZE 0x04
#define rwSTREAMGLOBALSSIZE 0x04
#define rwCAMERAGLOBALSSIZE 0x04
#define rwIMAGEGLOBALSSIZE 0x220
#define rwRASTERGLOBALSSIZE 0x64
#define rwTEXTUREGLOBALSSIZE 0x34
#define rwPIPEGLOBALSSIZE 0x60
#define rwIMMEDIATEGLOBALSSIZE 0x74
#define rwRESOURCESGLOBALSSIZE 0x28

enum RwDeviceSystemFn
{
    rwDEVICESYSTEMOPEN = 0x00,
    rwDEVICESYSTEMCLOSE,
    rwDEVICESYSTEMSTART,
    rwDEVICESYSTEMSTOP,
    rwDEVICESYSTEMREGISTER,
    rwDEVICESYSTEMGETNUMMODES,
    rwDEVICESYSTEMGETMODEINFO,
    rwDEVICESYSTEMUSEMODE,
    rwDEVICESYSTEMFOCUS,
    rwDEVICESYSTEMINITPIPELINE,
    rwDEVICESYSTEMGETMODE,
    rwDEVICESYSTEMSTANDARDS,
    rwDEVICESYSTEMGETTEXMEMSIZE,
    rwDEVICESYSTEMGETNUMSUBSYSTEMS,
    rwDEVICESYSTEMGETSUBSYSTEMINFO,
    rwDEVICESYSTEMGETCURRENTSUBSYSTEM,
    rwDEVICESYSTEMSETSUBSYSTEM,
    rwDEVICESYSTEMFINALIZESTART,
    rwDEVICESYSTEMINITIATESTOP,
    rwDEVICESYSTEMFNFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

#define rwSTANDARDNUMOFSTANDARD 29

extern RwDevice* _rwDeviceGetHandle(void);
extern RwBool _rwDeviceRegisterPlugin(void);
extern RwBool _rwPipeAttach(void);

extern RwBool _rwStringOpen(void);
extern void _rwStringClose(void);
extern RwBool _rwMemoryOpen(const RwMemoryFunctions* memFuncs);
extern void _rwMemoryClose(void);
extern RwBool _rwFileSystemOpen(void);
extern void _rwFileSystemClose(void);
extern RwBool _rwPluginRegistryOpen(void);
extern RwBool _rwPluginRegistryClose(void);
extern void _rwFreeListEnable(RwBool enabled);
extern void* _rwFreeListAllocReal(RwFreeList* freeList);
extern RwFreeList* _rwFreeListFreeReal(RwFreeList* freeList, void* entry);

extern void* _rwErrorOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwErrorClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwVectorOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwVectorClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwColorOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwColorClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwMatrixOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwMatrixClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwFrameOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwFrameClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwStreamModuleOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwStreamModuleClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwCameraOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwCameraClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwImageOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwImageClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwRasterOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwRasterClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwTextureOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwTextureClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwRenderPipelineOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwRenderPipelineClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwIm3DOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwIm3DClose(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwResourcesOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rwResourcesClose(void* instance, RwInt32 offset, RwInt32 size);

RwGlobals* RwEngineInstance;
static RwUInt32 engineInstancesOpened;

static RwPluginRegistry engineTKList = { sizeof(RwGlobals),       sizeof(RwGlobals),      0, 0,
                                         (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwBool CorePluginAttach(void)
{
    RwInt32 state;

    state = _rwPluginRegistryAddPlugin(&engineTKList, rwERRORGLOBALSSIZE, rwID_ERRORMODULE,
                                       _rwErrorOpen, _rwErrorClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwVECTORGLOBALSSIZE, rwID_VECTORMODULE,
                                        _rwVectorOpen, _rwVectorClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwCOLORGLOBALSSIZE, rwID_COLORMODULE,
                                        _rwColorOpen, _rwColorClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwMATRIXGLOBALSSIZE, rwID_MATRIXMODULE,
                                        _rwMatrixOpen, _rwMatrixClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwFRAMEGLOBALSSIZE, rwID_FRAMEMODULE,
                                        _rwFrameOpen, _rwFrameClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwSTREAMGLOBALSSIZE, rwID_STREAMMODULE,
                                        _rwStreamModuleOpen, _rwStreamModuleClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwCAMERAGLOBALSSIZE, rwID_CAMERAMODULE,
                                        _rwCameraOpen, _rwCameraClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwIMAGEGLOBALSSIZE, rwID_IMAGEMODULE,
                                        _rwImageOpen, _rwImageClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwRASTERGLOBALSSIZE, rwID_RASTERMODULE,
                                        _rwRasterOpen, _rwRasterClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwTEXTUREGLOBALSSIZE, rwID_TEXTUREMODULE,
                                        _rwTextureOpen, _rwTextureClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwPIPEGLOBALSSIZE, rwID_PIPEMODULE,
                                        _rwRenderPipelineOpen, _rwRenderPipelineClose, NULL);
    state |= _rwPipeAttach();
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwIMMEDIATEGLOBALSSIZE,
                                        rwID_IMMEDIATEMODULE, _rwIm3DOpen, _rwIm3DClose, NULL);
    state |= _rwPluginRegistryAddPlugin(&engineTKList, rwRESOURCESGLOBALSSIZE,
                                        rwID_RESOURCESMODULE, _rwResourcesOpen, _rwResourcesClose,
                                        NULL);

    return state >= 0;
}

static void* MallocWrapper(RwFreeList* fl)
{
    return RwMalloc(fl->entrySize);
}

static RwFreeList* FreeWrapper(RwFreeList* fl, void* pData)
{
    RwFree(pData);

    return fl;
}

static RwGlobals staticGlobals;

static RwBool EngineOpen(RwDevice* device, RwEngineOpenParams* initParams);

RwBool _rwDeviceSystemRequest(RwDevice* device, RwInt32 requestID, void* pOut, void* pInOut,
                              RwInt32 numIn)
{
    RwBool result;

    result = device->fpSystem(requestID, pOut, pInOut, numIn);

    if (!result)
    {
        /* Devices need not support these */
        if (requestID == rwDEVICESYSTEMFINALIZESTART || requestID == rwDEVICESYSTEMINITIATESTOP)
        {
            result = TRUE;
        }
    }

    if (!result)
    {
        RWERROR((E_RW_DEVICEERROR, requestID));
    }

    return result;
}

static RwBool EngineOpen(RwDevice* device, RwEngineOpenParams* initParams)
{
    void* instance;

    RwEngineInstance = (RwGlobals*)RwMalloc(engineTKList.sizeOfStruct);
    instance = RwEngineInstance;
    if (instance)
    {
        memcpy(instance, &staticGlobals, sizeof(RwGlobals));

        _rwDeviceSystemRequest(device, rwDEVICESYSTEMREGISTER, &RWSRCGLOBAL(dOpenDevice),
                               &RWSRCGLOBAL(memoryFuncs), 0);

        if (_rwDeviceSystemRequest(device, rwDEVICESYSTEMOPEN, NULL, initParams, 0))
        {
            _rwDeviceSystemRequest(device, rwDEVICESYSTEMSTANDARDS, RWSRCGLOBAL(stdFunc), NULL,
                                   rwSTANDARDNUMOFSTANDARD);

            engineInstancesOpened++;

            return TRUE;
        }

        /* Back out onto the static globals */
        RwEngineInstance = &staticGlobals;
        memcpy(RwEngineInstance, instance, sizeof(RwGlobals));
        RwFree(instance);

        return FALSE;
    }

    RWERROR((E_RW_NOMEM, engineTKList.sizeOfStruct));
    return FALSE;
}

RwUInt32 _rwGetNumEngineInstances(void)
{
    return engineInstancesOpened;
}

RwUInt32 RwEngineGetVersion(void)
{
    return rwLIBRARYCURRENTVERSION;
}

RwInt32 RwEngineRegisterPlugin(RwInt32 size, RwUInt32 pluginID, RwPluginObjectConstructor initCB,
                               RwPluginObjectDestructor termCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPlugin(&engineTKList, size, pluginID, initCB, termCB, NULL);

    return plug;
}

RwInt32 RwEngineGetPluginOffset(RwUInt32 pluginID)
{
    RwInt32 offset;

    offset = _rwPluginRegistryGetPluginOffset(&engineTKList, pluginID);

    return offset;
}

RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeinfo, RwInt32 modeIndex)
{
    if (!_rwDeviceSystemRequest(&RWSRCGLOBAL(dOpenDevice), rwDEVICESYSTEMGETMODEINFO, modeinfo,
                                NULL, modeIndex))
    {
        modeinfo = NULL;
    }

    return modeinfo;
}

RwInt32 RwEngineGetCurrentVideoMode(void)
{
    RwInt32 curMode;

    if (_rwDeviceSystemRequest(&RWSRCGLOBAL(dOpenDevice), rwDEVICESYSTEMGETMODE, &curMode, NULL,
                               0))
    {
        return curMode;
    }

    return -1;
}

RwBool RwEngineStop(void)
{
    RwBool result;
    RwDevice* const device = &RWSRCGLOBAL(dOpenDevice);

    _rwDeviceSystemRequest(device, rwDEVICESYSTEMINITIATESTOP, NULL, NULL, 0);

    _rwPluginRegistryDeInitObject(&engineTKList, RwEngineInstance);

    result = _rwDeviceSystemRequest(device, rwDEVICESYSTEMSTOP, NULL, NULL, 0);
    if (result)
    {
        RWSRCGLOBAL(engineStatus) = rwENGINESTATUSOPENED;
    }

    return result;
}

RwBool RwEngineStart(void)
{
    RwDevice* const device = &RWSRCGLOBAL(dOpenDevice);

    if (_rwDeviceSystemRequest(device, rwDEVICESYSTEMSTART, NULL, NULL, 0))
    {
        if (_rwPluginRegistryInitObject(&engineTKList, RwEngineInstance))
        {
            RwImageSetGamma(RWSRCGLOBAL(dOpenDevice).gammaCorrection);

            _rwDeviceSystemRequest(device, rwDEVICESYSTEMFINALIZESTART, NULL, NULL, 0);

            RWSRCGLOBAL(engineStatus) = rwENGINESTATUSSTARTED;

            return TRUE;
        }

        _rwDeviceSystemRequest(device, rwDEVICESYSTEMSTOP, NULL, NULL, 0);
    }

    return FALSE;
}

RwBool RwEngineClose(void)
{
    RwBool result;
    void* instance;

    result = _rwDeviceSystemRequest(&RWSRCGLOBAL(dOpenDevice), rwDEVICESYSTEMCLOSE, NULL, NULL, 0);
    if (result)
    {
        /* Move back onto the static globals */
        instance = RwEngineInstance;
        RwEngineInstance = &staticGlobals;
        memcpy(RwEngineInstance, instance, sizeof(RwGlobals));
        RwFree(instance);

        engineInstancesOpened--;

        RWSRCGLOBAL(engineStatus) = rwENGINESTATUSINITED;
    }

    return result;
}

RwBool RwEngineOpen(RwEngineOpenParams* initParams)
{
    RwBool result;
    RwDevice* device;

    if (!RwEngineInstance)
    {
        RwEngineInstance = &staticGlobals;
    }

    result = (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSINITED);
    if (result)
    {
        result = (initParams != NULL);
        if (result)
        {
            device = _rwDeviceGetHandle();

            result = (device != NULL);
            if (result)
            {
                result = EngineOpen(device, initParams);
                if (result)
                {
                    RWSRCGLOBAL(engineStatus) = rwENGINESTATUSOPENED;
                }
            }
        }
        else
        {
            RWERROR((E_RW_NULLP));
        }
    }
    else
    {
        RWERROR((E_RW_BADENGINESTATE));
    }

    return result;
}

RwBool RwEngineTerm(void)
{
    RwBool result;

    result = (engineInstancesOpened == 0);
    if (result)
    {
        _rwPluginRegistryClose();
        _rwFileSystemClose();
        _rwMemoryClose();

        RWSRCGLOBAL(engineStatus) = rwENGINESTATUSIDLE;
    }

    return result;
}

RwBool RwEngineInit(const RwMemoryFunctions* memFuncs, RwUInt32 initFlags, RwUInt32 resArenaSize)
{
    RwBool result = FALSE;

    RwEngineInstance = &staticGlobals;

    if (initFlags & rwENGINEINITNOFREELISTS)
    {
        RWSRCGLOBAL(memoryAlloc) = MallocWrapper;
        RWSRCGLOBAL(memoryFree) = FreeWrapper;
        _rwFreeListEnable(FALSE);
    }
    else
    {
        RWSRCGLOBAL(memoryAlloc) = _rwFreeListAllocReal;
        RWSRCGLOBAL(memoryFree) = _rwFreeListFreeReal;
        _rwFreeListEnable(TRUE);
    }

    RWSRCGLOBAL(resArenaInitSize) = resArenaSize;

    if (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSIDLE)
    {
        result = _rwStringOpen();
        if (result)
        {
            result = _rwMemoryOpen(memFuncs);
            if (result)
            {
                result = _rwFileSystemOpen();
                if (result)
                {
                    result = _rwPluginRegistryOpen();
                    if (result)
                    {
                        result = CorePluginAttach();
                        if (result)
                        {
                            result = _rwDeviceRegisterPlugin();
                            if (result)
                            {
                                RWSRCGLOBAL(engineStatus) = rwENGINESTATUSINITED;
                                return result;
                            }
                        }

                        _rwPluginRegistryClose();
                    }

                    _rwFileSystemClose();
                }

                _rwMemoryClose();
            }

            _rwStringClose();
        }
    }

    return result;
}
