#include <rwsdk/rwcore.h>
#include <dolphin/os.h>
#include <dolphin/vi.h>

#include "rwsdk/driver/gcn/dlprivate.h"

/* Device system requests */
enum RwCoreDeviceSystemFn
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
    rwDEVICESYSTEMGETMAXTEXTURESIZE,
    rwDEVICESYSTEMDEVICEFNFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

/* Standard device functions */
enum RwStandardFunctions
{
    rwSTANDARDNASTANDARD = 0,
    rwSTANDARDCAMERABEGINUPDATE = 1,
    rwSTANDARDRGBTOPIXEL = 2,
    rwSTANDARDPIXELTORGB = 3,
    rwSTANDARDRASTERCREATE = 4,
    rwSTANDARDRASTERDESTROY = 5,
    rwSTANDARDIMAGEGETRASTER = 6,
    rwSTANDARDRASTERSETIMAGE = 7,
    rwSTANDARDTEXTURESETRASTER = 8,
    rwSTANDARDIMAGEFINDRASTERFORMAT = 9,
    rwSTANDARDCAMERAENDUPDATE = 10,
    rwSTANDARDSETRASTERCONTEXT = 11,
    rwSTANDARDRASTERSUBRASTER = 12,
    rwSTANDARDRASTERCLEARRECT = 13,
    rwSTANDARDRASTERCLEAR = 14,
    rwSTANDARDRASTERLOCK = 15,
    rwSTANDARDRASTERUNLOCK = 16,
    rwSTANDARDRASTERRENDER = 17,
    rwSTANDARDRASTERRENDERSCALED = 18,
    rwSTANDARDRASTERRENDERFAST = 19,
    rwSTANDARDRASTERSHOWRASTER = 20,
    rwSTANDARDCAMERACLEAR = 21,
    rwSTANDARDHINTRENDERF2B = 22,
    rwSTANDARDRASTERLOCKPALETTE = 23,
    rwSTANDARDRASTERUNLOCKPALETTE = 24,
    rwSTANDARDNATIVETEXTUREGETSIZE = 25,
    rwSTANDARDNATIVETEXTUREREAD = 26,
    rwSTANDARDNATIVETEXTUREWRITE = 27,
    rwSTANDARDRASTERGETMIPLEVELS = 28,
    rwSTANDARDNUMOFSTANDARD = 29,
    rwSTANDARDFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

typedef struct RwStandard RwStandard;
struct RwStandard
{
    RwInt32 nStandard;
    RwStandardFunc fpStandard;
};

typedef struct RwRwDeviceGlobals RwRwDeviceGlobals;
struct RwRwDeviceGlobals
{
    RwCamera* curCamera;
    RwMemoryFunctions* memFuncs;
};

typedef struct _rwGCFrame _rwGCFrame;
struct _rwGCFrame
{
    void* XFBCopy;
    void* FIFOWritePtr;
};

/* Video mode used for a render mode supplied by the application */
#define rwDLVIDEOMODEUSER 42

#define rwDLNUMVIDEOMODES 1

RwBool _rwDlCameraBeginUpdate(void* unused1, void* cameraIn, RwInt32 unused3);
RwBool _rwDlCameraEndUpdate(void* unused1, void* unused2, RwInt32 unused3);
RwBool _rwDlCameraClear(void* cameraIn, void* colorIn, RwInt32 clearFlags);
RwBool _rwDlRasterShowRaster(void* unused1, void* unused2, RwInt32 unused3);

/* The C compiler emits these small-BSS declarations in reverse order. */
RwRwDeviceGlobals dgGGlobals;
GXRenderModeObj* _RwDlRenderMode;
RwInt32 _RwDlHalfHeight;
static GXFifoObj* _RwDlDefaultFifoObj;
static void* _RwDlDefaultFifo;
static RwUInt16 _RwDlFrameSwap[3];
static OSThreadQueue _RwDlWaitingDoneRender;
static volatile RwBool _RwDlFrameGo;
static volatile RwBool _RwDlFrameWait;
static volatile RwBool _RwDlFrameReadyOnToken;
static RwBool _RwDlBreakPointEnabled;
static volatile RwInt32 _RwDlFrameTokenCurrent;
static volatile RwInt32 _RwDlFrameTokenNew;
static volatile RwInt32 _RwDlFrameNew;
static volatile RwInt32 _RwDlFrameCurrent;
static void* _RwGCXFBDisp;
static void* _RwGCXFBCopy;
static void* _RwGCXFB2;
static void* _RwGCXFB1;
static void* _RwDl_FIFO_XFB;
RwInt32 _RwDlFSAA;
RwInt32 _RwGameCubeVideoMode;
GXPixelFmt _RwDlCurPixelFormat;
GXPixelFmt _RwDlPixelFormat;
static RwBool _RwDlCopyClear;

static RwVideoMode _RwDlVideoModes[4] = {
    { 640, 480, 24, rwVIDEOMODEEXCLUSIVE, 0, 0 },
    { 640, 528, 24, rwVIDEOMODEEXCLUSIVE, 0, 0 },
    { 640, 480, 24, rwVIDEOMODEEXCLUSIVE, 0, 0 },
    { 0, 0, 0, rwVIDEOMODEEXCLUSIVE, 0, 0 },
};

RwInt32 _RwDlFSAATop = TRUE;

static RwUInt32 _RwDlFifoSize = 256 * 1024;
static RwBool _RwDlFirstFrame = TRUE;
static RwInt32 _RwDlLatency = 3;
static RwUInt8 _RwDlRetraceCount = 1;
static RwUInt8 _RwDlRetraceMinCount = 1;

static _rwGCFrame _RwGCFrameQueue[3];

static void _rwDlBreakNext(void)
{
    static RwInt32 swap;
    RwUInt32 nextFrame = (_RwDlFrameCurrent + 1) % _RwDlLatency;

    if (nextFrame == _RwDlFrameNew)
    {
        GXDisableBreakPt();
        _RwDlBreakPointEnabled = FALSE;
    }
    else
    {
        GXEnableBreakPt(_RwGCFrameQueue[nextFrame].FIFOWritePtr);
    }

    _RwDlFrameCurrent = nextFrame;

    OSWakeupThread(&_RwDlWaitingDoneRender);

    if (!_RwDlFSAA)
    {
        _RwDlFrameReadyOnToken = TRUE;
    }
    else if (swap == 1)
    {
        /* Both halves of the anti-aliased frame have been rendered */
        _RwDlFrameReadyOnToken = TRUE;
        swap = 0;
    }
    else
    {
        swap = 1;
    }
}

static void _rwDlBreakPtCallback(void)
{
    if (_RwGCFrameQueue[_RwDlFrameCurrent].XFBCopy != _RwGCXFBDisp)
    {
        _rwDlBreakNext();
    }
    else
    {
        /* Wait for the frame buffer to be released by the display */
        _RwDlFrameWait = TRUE;
    }
}

static void _rwDlVIPreRetraceCallback(u32 retraceCount)
{
    RwInt16 token;

    _RwDlRetraceCount++;

    token = _RwDlFrameSwap[_RwDlFrameTokenCurrent];

    if ((_RwDlFrameReadyOnToken == TRUE) && _rwDlTokenQueryDone((RwUInt16)token) &&
        (_RwDlRetraceCount >= _RwDlRetraceMinCount))
    {
        _RwDlRetraceCount = 0;

        if (_RwGCXFBDisp == _RwGCXFB1)
        {
            _RwGCXFBDisp = _RwGCXFB2;
        }
        else
        {
            _RwGCXFBDisp = _RwGCXFB1;
        }
        VISetNextFrameBuffer(_RwGCXFBDisp);

        if (_RwDlFirstFrame)
        {
            VISetBlack(FALSE);
            _RwDlFirstFrame = FALSE;
        }

        VIFlush();

        _RwDlFrameTokenCurrent = (_RwDlFrameTokenCurrent + 1) % _RwDlLatency;
        _RwDlFrameReadyOnToken = FALSE;
        _RwDlFrameGo = TRUE;
    }
}

static void _rwDlVIPostRetraceCallback(u32 retraceCount)
{
    if (_RwDlFrameWait && _RwDlFrameGo)
    {
        _rwDlBreakNext();

        _RwDlFrameWait = FALSE;
    }

    _RwDlFrameGo = FALSE;
}

static RwBool _rwDlNullStandard(void* out, void* inOut, RwInt32 in)
{
    return FALSE;
}

static RwBool _rwDlDeviceSystemStandards(RwStandardFunc* standardFunctions,
                                         RwInt32 numStandardsFunctions)
{
    RwInt32 i;
    RwInt32 numDriverFunctions;
    RwStandard rwDlStandards[] = {
        { rwSTANDARDCAMERABEGINUPDATE, _rwDlCameraBeginUpdate },
        { rwSTANDARDCAMERAENDUPDATE, _rwDlCameraEndUpdate },
        { rwSTANDARDCAMERACLEAR, _rwDlCameraClear },
        { rwSTANDARDRASTERSHOWRASTER, _rwDlRasterShowRaster },
        { rwSTANDARDRGBTOPIXEL, _rwDlRGBToPixel },
        { rwSTANDARDPIXELTORGB, _rwDlPixelToRGB },
        { rwSTANDARDRASTERSETIMAGE, _rwDlRasterSetFromImage },
        { rwSTANDARDIMAGEGETRASTER, _rwDlImageGetFromRaster },
        { rwSTANDARDRASTERDESTROY, _rwDlRasterDestroy },
        { rwSTANDARDRASTERCREATE, _rwDlRasterCreate },
        { rwSTANDARDIMAGEFINDRASTERFORMAT, _rwDlImageFindRasterFormat },
        { rwSTANDARDTEXTURESETRASTER, _rwDlTextureSetRaster },
        { rwSTANDARDRASTERLOCK, _rwDlRasterLock },
        { rwSTANDARDRASTERUNLOCK, _rwDlRasterUnlock },
        { rwSTANDARDRASTERLOCKPALETTE, _rwDlRasterLockPalette },
        { rwSTANDARDRASTERUNLOCKPALETTE, _rwDlRasterUnlockPalette },
        { rwSTANDARDRASTERCLEAR, _rwDlRasterClear },
        { rwSTANDARDRASTERCLEARRECT, _rwDlRasterClearRect },
        { rwSTANDARDRASTERRENDER, _rwDlRasterRender },
        { rwSTANDARDRASTERRENDERSCALED, _rwDlRasterRenderScaled },
        { rwSTANDARDRASTERRENDERFAST, _rwDlRasterRenderFast },
        { rwSTANDARDSETRASTERCONTEXT, _rwDlSetRasterContext },
        { rwSTANDARDRASTERSUBRASTER, _rwDlRasterSubRaster },
        { rwSTANDARDNATIVETEXTUREGETSIZE, _rwDlNativeTextureGetSize },
        { rwSTANDARDNATIVETEXTUREWRITE, _rwDlNativeTextureWrite },
        { rwSTANDARDNATIVETEXTUREREAD, _rwDlNativeTextureRead },
        { rwSTANDARDRASTERGETMIPLEVELS, _rwDlRasterGetNumMipLevels },
    };

    numDriverFunctions = sizeof(rwDlStandards) / sizeof(RwStandard);

    for (i = 0; i < numStandardsFunctions; i++)
    {
        standardFunctions[i] = _rwDlNullStandard;
    }

    while (numDriverFunctions--)
    {
        if ((rwDlStandards->nStandard < numStandardsFunctions) && (rwDlStandards->nStandard >= 0))
        {
            standardFunctions[rwDlStandards[numDriverFunctions].nStandard] =
                rwDlStandards[numDriverFunctions].fpStandard;
        }
    }

    return TRUE;
}

static GXRenderModeObj _RwGameCubeRenderModeObj;

static void _rwDlRenderModeSelect(GXRenderModeObj* renderMode, GXPixelFmt pixFmt)
{
    if (renderMode)
    {
        /* Application supplied render mode */
        _RwGameCubeRenderModeObj = *renderMode;
        _RwDlRenderMode = &_RwGameCubeRenderModeObj;
        _RwGameCubeVideoMode = rwDLVIDEOMODEUSER;

        _RwDlPixelFormat = (_RwDlRenderMode->aa) ? GX_PF_RGB565_Z16 : pixFmt;
        _RwDlCurPixelFormat = _RwDlPixelFormat;

        _RwDlVideoModes[3].width = _RwDlRenderMode->fbWidth;
        _RwDlVideoModes[3].height = _RwDlRenderMode->efbHeight;

        switch (pixFmt)
        {
        case GX_PF_RGB8_Z24:
        case GX_PF_RGBA6_Z24:
        {
            _RwDlVideoModes[3].depth = 24;
            break;
        }
        case GX_PF_RGB565_Z16:
        {
            _RwDlVideoModes[3].depth = 16;
            break;
        }
        default:
        {
            RWERROR((E_RW_DEVICEERROR, "Invalid pixel format"));
            break;
        }
        }
    }
    else
    {
        switch (VIGetTvFormat())
        {
        case VI_NTSC:
        {
            _RwDlRenderMode = &GXNtsc480IntDf;
            _RwGameCubeVideoMode = 0;
            break;
        }
        case VI_PAL:
        case VI_EURGB60:
        {
            _RwDlRenderMode = &GXPal528IntDf;
            _RwGameCubeVideoMode = 1;
            break;
        }
        case VI_MPAL:
        {
            _RwDlRenderMode = &GXMpal480IntDf;
            _RwGameCubeVideoMode = 2;
            break;
        }
        default:
        {
            RWERROR((E_RW_DEVICEERROR, "Invalid TV format"));
            break;
        }
        }
    }
}

static void _rwDlRenderModeInit(GXRenderModeObj* renderMode, GXPixelFmt pixFmt)
{
    VIConfigure(renderMode);
    VISetBlack(TRUE);
    VIFlush();

    if (renderMode->field_rendering)
    {
        GXSetViewportJitter(0.0f, 0.0f, (f32)renderMode->fbWidth, (f32)renderMode->xfbHeight, 0.0f,
                            1.0f, VIGetNextField() ^ 1);
    }
    else
    {
        GXSetViewport(0.0f, 0.0f, (f32)renderMode->fbWidth, (f32)renderMode->xfbHeight, 0.0f,
                      1.0f);
    }

    GXSetScissor(0, 0, renderMode->fbWidth, renderMode->efbHeight);
    GXSetScissorBoxOffset(0, 0);

    GXSetDispCopySrc(0, 0, renderMode->fbWidth, renderMode->efbHeight);
    GXSetDispCopyDst(renderMode->fbWidth, renderMode->xfbHeight);
    GXSetCopyFilter(renderMode->aa, renderMode->sample_pattern, GX_TRUE, renderMode->vfilter);

    if (renderMode->aa && (renderMode->xfbHeight == renderMode->viHeight))
    {
        /* Anti-aliased rendering is done in two halves */
        GXSetDispCopyYScale(1.0f);

        _RwDlHalfHeight = renderMode->xfbHeight / 2;
        _RwDlFSAA = TRUE;
        _RwDlFSAATop = TRUE;
    }
    else
    {
        GXSetDispCopyYScale((f32)renderMode->xfbHeight / (f32)renderMode->efbHeight);

        _RwDlFSAA = FALSE;
    }

    GXSetFieldMode(renderMode->field_rendering, (renderMode->xfbHeight < renderMode->viHeight));

    GXSetPixelFmt(pixFmt, GX_ZC_LINEAR);

    _RwDlFirstFrame = TRUE;

    VIWaitForRetrace();
    if (_RwDlRenderMode->viTVmode & 1)
    {
        VIWaitForRetrace();
    }
}

RwBool _rwDeviceRegisterPlugin(void)
{
    _rwDlRasterPluginAttach();
    _rwDlTexturePluginAttach();

    return TRUE;
}

RwDevice* _rwDeviceGetHandle(void);

static RwBool _rwDlSystem(RwInt32 request, void* out, void* inOut, RwInt32 in)
{
    switch (request)
    {
    case rwDEVICESYSTEMUSEMODE:
    {
        return (in < rwDLNUMVIDEOMODES);
    }
    case rwDEVICESYSTEMGETNUMMODES:
    {
        *(RwInt32*)out = rwDLNUMVIDEOMODES;
        return TRUE;
    }
    case rwDEVICESYSTEMGETMODEINFO:
    {
        if (in < rwDLNUMVIDEOMODES)
        {
            RwVideoMode* vpMode = (RwVideoMode*)out;

            switch (_RwGameCubeVideoMode)
            {
            case 0:
            {
                *vpMode = _RwDlVideoModes[0];
                break;
            }
            case 1:
            case 5:
            {
                *vpMode = _RwDlVideoModes[1];
                break;
            }
            case 2:
            {
                *vpMode = _RwDlVideoModes[2];
                break;
            }
            case rwDLVIDEOMODEUSER:
            {
                *vpMode = _RwDlVideoModes[3];
                break;
            }
            }

            return TRUE;
        }

        return FALSE;
    }
    case rwDEVICESYSTEMGETMODE:
    {
        *(RwInt32*)out = 0;
        return TRUE;
    }
    case rwDEVICESYSTEMFOCUS:
    {
        return TRUE;
    }
    case rwDEVICESYSTEMREGISTER:
    {
        RwDevice* deviceOut = (RwDevice*)out;

        *deviceOut = *_rwDeviceGetHandle();
        dgGGlobals.memFuncs = (RwMemoryFunctions*)inOut;

        return TRUE;
    }
    case rwDEVICESYSTEMOPEN:
    {
        RwGameCubeDeviceConfig* deviceConfig = (RwGameCubeDeviceConfig*)((RwEngineOpenParams*)inOut)->displayID;
        RwUInt32 XFBSize;

        if (deviceConfig)
        {
            _rwDlRenderModeSelect((GXRenderModeObj*)deviceConfig->renderMode,
                                  (GXPixelFmt)deviceConfig->pixFmt);
            _RwDlFifoSize = deviceConfig->fifoSize;
        }
        else
        {
            _rwDlRenderModeSelect(NULL, _RwDlPixelFormat);
        }

        XFBSize = VIPadFrameBufferWidth(_RwDlRenderMode->fbWidth) * _RwDlRenderMode->xfbHeight *
                  VI_DISPLAY_PIX_SZ;

        /* The FIFO and both external frame buffers share one allocation */
        _RwDl_FIFO_XFB = RwMalloc(_RwDlFifoSize + XFBSize * 2 + 31);

        _RwDlDefaultFifo = (void*)(((RwUInt32)_RwDl_FIFO_XFB + 31) & ~31);
        DCInvalidateRange(_RwDlDefaultFifo, _RwDlFifoSize);

        _RwGCXFB1 = (RwUInt8*)_RwDlDefaultFifo + _RwDlFifoSize;
        _RwGCXFB2 = (RwUInt8*)_RwGCXFB1 + XFBSize;
        _RwGCXFBDisp = _RwGCXFB1;
        _RwGCXFBCopy = _RwGCXFB2;

        return TRUE;
    }
    case rwDEVICESYSTEMCLOSE:
    {
        GXFlush();

        RwFree(_RwDl_FIFO_XFB);

        _RwGCXFB1 = NULL;
        _RwGCXFB2 = NULL;
        _RwGCXFBDisp = NULL;
        _RwGCXFBCopy = NULL;
        _RwDlDefaultFifo = NULL;
        _RwDl_FIFO_XFB = NULL;

        return TRUE;
    }
    case rwDEVICESYSTEMSTART:
    {
        static RwBool gxInit = FALSE;

        if (!gxInit)
        {
            _RwDlDefaultFifoObj = GXInit(_RwDlDefaultFifo, _RwDlFifoSize);
            gxInit = TRUE;
        }
        else
        {
            GXFifoObj tmpFIFO;

            GXInitFifoBase(&tmpFIFO, _RwDlDefaultFifo, _RwDlFifoSize);
            GXSetCPUFifo(&tmpFIFO);
            GXSetGPFifo(&tmpFIFO);

            GXInitFifoBase(_RwDlDefaultFifoObj, _RwDlDefaultFifo, _RwDlFifoSize);
            GXSetCPUFifo(_RwDlDefaultFifoObj);
            GXSetGPFifo(_RwDlDefaultFifoObj);
        }

        while (_RwDlTokenLastSeen != GXReadDrawSync())
        {
            GXSetDrawSync(_RwDlTokenLastSeen);
        }

        _rwDlRenderModeInit(_RwDlRenderMode, _RwDlPixelFormat);

        GXSetDispCopyGamma(GX_GM_1_0);

        _RwDlRetraceCount = _RwDlRetraceMinCount;

        VISetPreRetraceCallback(_rwDlVIPreRetraceCallback);
        VISetPostRetraceCallback(_rwDlVIPostRetraceCallback);
        GXSetBreakPtCallback(_rwDlBreakPtCallback);

        OSInitThreadQueue(&_RwDlWaitingDoneRender);

        return TRUE;
    }
    case rwDEVICESYSTEMFINALIZESTART:
    {
        _rwDlRenderStateOpen();
        return TRUE;
    }
    case rwDEVICESYSTEMINITIATESTOP:
    {
        _rwDlRenderStateClose();
        return TRUE;
    }
    case rwDEVICESYSTEMSTOP:
    {
        /* Wait for the GPU to finish any outstanding frames */
        while ((_RwDlFrameReadyOnToken == TRUE) || (_RwDlFrameTokenNew != _RwDlFrameTokenCurrent))
        {
        }

        return TRUE;
    }
    case rwDEVICESYSTEMINITPIPELINE:
    {
        return TRUE;
    }
    case rwDEVICESYSTEMSTANDARDS:
    {
        return _rwDlDeviceSystemStandards((RwStandardFunc*)out, in);
    }
    case rwDEVICESYSTEMGETMAXTEXTURESIZE:
    {
        *(RwInt32*)out = 1024;
        return TRUE;
    }
    default:
    {
        break;
    }
    }

    return FALSE;
}

RwBool _rwDlCameraClear(void* cameraIn, void* colorIn, RwInt32 clearFlags)
{
    RwRect rect;
    RwRaster* raster;

    if (!_RwDlCopyClear)
    {
        raster = RwCameraGetRaster((RwCamera*)cameraIn);

        if (!_RwDlFSAA)
        {
            rect.x = raster->nOffsetX;
            rect.y = raster->nOffsetY;
            rect.w = raster->width;
            rect.h = raster->height;
        }
        else
        {
            rect.x = raster->nOffsetX;
            rect.y = raster->nOffsetY << 1;
            rect.w = raster->width;
            rect.h = raster->height << 1;
        }

        _rwDlRasterCamera_ZClearRect(raster, &rect, (RwRGBA*)colorIn, clearFlags);
    }
    else if (clearFlags & rwCAMERACLEARIMAGE)
    {
        /* The frame buffer is cleared by the display copy */
        GXSetCopyClear(*(GXColor*)colorIn, GX_MAX_Z24);
    }

    return TRUE;
}

RwMatrix _RwDlInvCamLTM;

RwBool _rwDlCameraBeginUpdate(void* unused1, void* cameraIn, RwInt32 unused3)
{
    static f32 projVector[7] = { GX_PERSPECTIVE, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
    RwCamera* camera = (RwCamera*)cameraIn;
    RwRaster* raster;
    RwMatrix viewoffset;

    GXSetCurrentGXThread();

    dgGGlobals.curCamera = camera;

    viewoffset.flags = 0;
    viewoffset.right.x = 1.0f;
    viewoffset.right.y = 0.0f;
    viewoffset.right.z = 0.0f;
    viewoffset.up.x = 0.0f;
    viewoffset.up.y = 1.0f;
    viewoffset.up.z = 0.0f;
    viewoffset.at.x = -camera->viewOffset.x;
    viewoffset.at.y = camera->viewOffset.y;
    viewoffset.at.z = 1.0f;
    viewoffset.pos.x = camera->viewOffset.x;
    viewoffset.pos.y = -camera->viewOffset.y;
    viewoffset.pos.z = 0.0f;

    _RwDlInvCamLTM.flags = rwMATRIXTYPEORTHONORMAL | rwMATRIXINTERNALIDENTITY;
    RwMatrixInvert(&_RwDlInvCamLTM, RwFrameGetLTM(RwCameraGetFrame(camera)));
    RwMatrixTransform(&_RwDlInvCamLTM, &viewoffset, rwCOMBINEPOSTCONCAT);

    projVector[1] = camera->recipViewWindow.x;
    projVector[3] = camera->recipViewWindow.y;

    if (camera->projectionType == rwPARALLEL)
    {
        projVector[0] = GX_ORTHOGRAPHIC;
        projVector[5] = -1.0f / (camera->farPlane - camera->nearPlane);
        projVector[6] = camera->farPlane * projVector[5];
    }
    else
    {
        projVector[0] = GX_PERSPECTIVE;
        projVector[5] = -camera->nearPlane / (camera->farPlane - camera->nearPlane);
        projVector[6] = camera->farPlane * projVector[5];
    }

    GXSetProjectionv(projVector);

    raster = RwCameraGetRaster(camera);

    if (raster->cType & rwRASTERTYPECAMERATEXTURE)
    {
        RwGameCubeRasterExtension* rasExt = RASTEREXTFROMRASTER(raster->parent);

        if (rasExt->flags & 1)
        {
            if (_RwDlCurPixelFormat != GX_PF_RGBA6_Z24)
            {
                GXSetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
                _RwDlCurPixelFormat = GX_PF_RGBA6_Z24;
            }
        }
        else
        {
            if (_RwDlCurPixelFormat != GX_PF_RGB8_Z24)
            {
                GXSetPixelFmt(GX_PF_RGB8_Z24, GX_ZC_LINEAR);
                _RwDlCurPixelFormat = GX_PF_RGB8_Z24;
            }
        }

        GXSetViewport((f32)raster->nOffsetX, (f32)raster->nOffsetY, (f32)raster->width,
                      (f32)raster->height, 0.0f, 1.0f);
        GXSetScissor(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height);
        GXSetScissorBoxOffset(0, 0);
    }
    else
    {
        if (_RwDlCurPixelFormat != _RwDlPixelFormat)
        {
            GXSetPixelFmt(_RwDlPixelFormat, GX_ZC_LINEAR);
            _RwDlCurPixelFormat = _RwDlPixelFormat;
        }

        if (!_RwDlFSAA)
        {
            if (_RwDlRenderMode->field_rendering)
            {
                GXSetViewportJitter((f32)raster->nOffsetX, (f32)raster->nOffsetY,
                                    (f32)raster->width, (f32)raster->height, 0.0f, 1.0f,
                                    VIGetNextField() ^ 1);
            }
            else
            {
                GXSetViewport((f32)raster->nOffsetX, (f32)raster->nOffsetY, (f32)raster->width,
                              (f32)raster->height, 0.0f, 1.0f);
            }

            GXSetScissor(raster->nOffsetX, raster->nOffsetY, raster->width, raster->height);
        }
        else
        {
            if (_RwDlRenderMode->field_rendering)
            {
                GXSetViewportJitter((f32)raster->nOffsetX, (f32)raster->nOffsetY,
                                    (f32)raster->width, (f32)raster->height, 0.0f, 1.0f,
                                    VIGetNextField() ^ 1);
            }
            else
            {
                GXSetViewport((f32)raster->nOffsetX, (f32)(raster->nOffsetY << 1),
                              (f32)raster->width, (f32)(raster->height << 1), 0.0f, 1.0f);
            }

            if (_RwDlFSAATop)
            {
                if (((raster->nOffsetY + raster->height) << 1) <= _RwDlHalfHeight + 2)
                {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, _RwDlRenderMode->fbWidth,
                                 raster->height << 1);
                }
                else if ((raster->nOffsetY << 1) > _RwDlHalfHeight + 2)
                {
                    GXSetScissor(0, 0, _RwDlRenderMode->fbWidth, _RwDlHalfHeight + 2);
                }
                else
                {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, _RwDlRenderMode->fbWidth,
                                 _RwDlHalfHeight + 2);
                }

                GXSetScissorBoxOffset(0, 0);
            }
            else
            {
                if ((raster->nOffsetY << 1) >= _RwDlHalfHeight - 2)
                {
                    GXSetScissor(raster->nOffsetX, raster->nOffsetY << 1, _RwDlRenderMode->fbWidth,
                                 raster->height << 1);
                }
                else if (((raster->nOffsetY + raster->height) << 1) < _RwDlHalfHeight + 2)
                {
                    GXSetScissor(0, _RwDlHalfHeight - 2, _RwDlRenderMode->fbWidth,
                                 _RwDlHalfHeight + 2);
                }
                else
                {
                    GXSetScissor(raster->nOffsetX, _RwDlHalfHeight - 2, _RwDlRenderMode->fbWidth,
                                 raster->height << 1);
                }

                GXSetScissorBoxOffset(0, _RwDlHalfHeight - 2);
            }
        }
    }

    return TRUE;
}

RwBool _rwDlCameraEndUpdate(void* unused1, void* unused2, RwInt32 unused3)
{
    dgGGlobals.curCamera = NULL;

    return TRUE;
}

RwBool _rwDlRasterShowRaster(void* unused1, void* unused2, RwInt32 unused3)
{
    void* tmpRead;
    void* tmpWrite;
    BOOL interruptsEnabled;
    volatile RwBool waitFrame;

    /* Wait if the GPU is too far behind */
    interruptsEnabled = OSDisableInterrupts();
    waitFrame = (((_RwDlFrameNew - _RwDlFrameCurrent) == -1) ||
                 ((_RwDlFrameNew - _RwDlFrameCurrent) == (_RwDlLatency - 1)))
                    ? TRUE
                    : FALSE;
    OSRestoreInterrupts(interruptsEnabled);

    if (waitFrame == TRUE)
    {
        OSSleepThread(&_RwDlWaitingDoneRender);
    }

    if (!_RwDlFSAA)
    {
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &tmpRead, &tmpWrite);

        interruptsEnabled = OSDisableInterrupts();
        _RwGCFrameQueue[_RwDlFrameNew].FIFOWritePtr = tmpWrite;
        _RwGCFrameQueue[_RwDlFrameNew].XFBCopy = _RwGCXFBCopy;
        _RwDlFrameSwap[_RwDlFrameTokenNew] = _RwDlTokenCurrent;
        _RwDlFrameNew = (_RwDlFrameNew + 1) % _RwDlLatency;
        _RwDlFrameTokenNew = (_RwDlFrameTokenNew + 1) % _RwDlLatency;
        OSRestoreInterrupts(interruptsEnabled);

        if (!_RwDlBreakPointEnabled)
        {
            _RwDlBreakPointEnabled = TRUE;
            GXEnableBreakPt(tmpWrite);
        }

        GXCopyDisp(_RwGCXFBCopy, (GXBool)_RwDlCopyClear);

        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;

        GXFlush();

        _RwGCXFBCopy = (_RwGCXFBCopy == _RwGCXFB1) ? _RwGCXFB2 : _RwGCXFB1;
    }
    else if (_RwDlFSAATop)
    {
        /* Top half of an anti-aliased frame */
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &tmpRead, &tmpWrite);

        interruptsEnabled = OSDisableInterrupts();
        _RwGCFrameQueue[_RwDlFrameNew].FIFOWritePtr = tmpWrite;
        _RwGCFrameQueue[_RwDlFrameNew].XFBCopy = _RwGCXFBCopy;
        _RwDlFrameNew = (_RwDlFrameNew + 1) % _RwDlLatency;
        OSRestoreInterrupts(interruptsEnabled);

        if (!_RwDlBreakPointEnabled)
        {
            _RwDlBreakPointEnabled = TRUE;
            GXEnableBreakPt(tmpWrite);
        }

        GXCopyDisp(_RwGCXFBCopy, (GXBool)_RwDlCopyClear);

        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;

        GXFlush();

        _RwDlFSAATop = FALSE;
    }
    else
    {
        RwInt32 bufferOffset;

        /* Bottom half of an anti-aliased frame */
        GXFlush();
        GXGetFifoPtrs(GXGetCPUFifo(), &tmpRead, &tmpWrite);

        interruptsEnabled = OSDisableInterrupts();
        _RwGCFrameQueue[_RwDlFrameNew].FIFOWritePtr = tmpWrite;
        _RwGCFrameQueue[_RwDlFrameNew].XFBCopy = _RwGCXFBCopy;
        _RwDlFrameSwap[_RwDlFrameTokenNew] = _RwDlTokenCurrent;
        _RwDlFrameNew = (_RwDlFrameNew + 1) % _RwDlLatency;
        _RwDlFrameTokenNew = (_RwDlFrameTokenNew + 1) % _RwDlLatency;
        OSRestoreInterrupts(interruptsEnabled);

        if (!_RwDlBreakPointEnabled)
        {
            _RwDlBreakPointEnabled = TRUE;
            GXEnableBreakPt(tmpWrite);
        }

        GXSetCopyClamp(GX_CLAMP_BOTTOM);
        GXSetDispCopySrc(0, 2, _RwDlRenderMode->fbWidth, (u16)(_RwDlRenderMode->efbHeight - 2));

        bufferOffset = VIPadFrameBufferWidth(_RwDlRenderMode->fbWidth) *
                       (_RwDlRenderMode->efbHeight - 2) * VI_DISPLAY_PIX_SZ;
        GXCopyDisp((RwUInt8*)_RwGCXFBCopy + bufferOffset, (GXBool)_RwDlCopyClear);

        GXSetDrawSync(_RwDlTokenCurrent);
        _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;

        GXSetCopyClamp((GXFBClamp)(GX_CLAMP_TOP | GX_CLAMP_BOTTOM));
        GXSetDispCopySrc(0, 0, _RwDlRenderMode->fbWidth, _RwDlRenderMode->efbHeight);

        GXFlush();

        _RwGCXFBCopy = (_RwGCXFBCopy == _RwGCXFB1) ? _RwGCXFB2 : _RwGCXFB1;

        _RwDlFSAATop = TRUE;
    }

    return TRUE;
}

RwDevice* _rwDeviceGetHandle(void)
{
    static RwDevice rwDlDriverDevice = {
        1.0f, /* gamma correction */
        _rwDlSystem,
        0.0f, /* z buffer near */
        0.99999994f, /* z buffer far */
        _rwDlSetRenderState,
        _rwDlGetRenderState,
        (RwIm2DRenderLineFunction)_rwDlIm2DRenderLine,
        (RwIm2DRenderTriangleFunction)_rwDlIm2DRenderTriangle,
        (RwIm2DRenderPrimitiveFunction)_rwDlIm2DRenderPrimitive,
        (RwIm2DRenderIndexedPrimitiveFunction)_rwDlIm2DRenderIndexedPrimitive,
        NULL,
        NULL,
        NULL,
        NULL,
    };

    return &rwDlDriverDevice;
}

void _rwDlTransformSetup(RwMatrix* ltm, RwBool normals)
{
    RwMatrix tmpMtx;
    RwMatrix* transMtx;
    f32 mtx[3][4];

    if (ltm)
    {
        tmpMtx.flags = rwMATRIXTYPEORTHONORMAL | rwMATRIXINTERNALIDENTITY;
        RwMatrixMultiply(&tmpMtx, ltm, &_RwDlInvCamLTM);
        transMtx = &tmpMtx;
    }
    else
    {
        transMtx = &_RwDlInvCamLTM;
    }

    /* RenderWare matrices are column major and right handed */
    mtx[0][0] = -transMtx->right.x;
    mtx[0][1] = -transMtx->up.x;
    mtx[0][2] = -transMtx->at.x;
    mtx[0][3] = -transMtx->pos.x;
    mtx[1][0] = transMtx->right.y;
    mtx[1][1] = transMtx->up.y;
    mtx[1][2] = transMtx->at.y;
    mtx[1][3] = transMtx->pos.y;
    mtx[2][0] = -transMtx->right.z;
    mtx[2][1] = -transMtx->up.z;
    mtx[2][2] = -transMtx->at.z;
    mtx[2][3] = -transMtx->pos.z;

    GXLoadPosMtxImm(mtx, GX_PNMTX0);

    if (normals)
    {
        GXLoadNrmMtxImm(mtx, GX_PNMTX0);
    }

    GXSetCurrentMtx(GX_PNMTX0);
}

void RwGameCubeCameraTextureFlush(RwRaster* raster, RwBool boxFilter)
{
    RwUInt32 offset;
    RwRaster* dstParent = raster->parent;
    RwGameCubeRasterExtension* dstRasExt = RASTEREXTFROMRASTER(dstParent);

    GXSetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL);

    if (boxFilter)
    {
        GXSetTexCopySrc((u16)(raster->nOffsetX << 1), (u16)(raster->nOffsetY << 1),
                        (u16)(raster->width << 1), (u16)(raster->height << 1));
    }
    else
    {
        GXSetTexCopySrc((u16)raster->nOffsetX, (u16)raster->nOffsetY, (u16)raster->width,
                        (u16)raster->height);
    }

    GXSetTexCopyDst((u16)dstParent->width, (u16)dstParent->height, (GXTexFmt)dstRasExt->format,
                    (GXBool)boxFilter);

    switch (dstParent->depth)
    {
    case 4:
    {
        offset = ((raster->nOffsetX << 3) + ((dstParent->width + 7) & ~7) * raster->nOffsetY) >> 1;
        break;
    }
    case 8:
    {
        offset = (raster->nOffsetX << 2) + ((dstParent->width + 7) & ~7) * raster->nOffsetY;
        break;
    }
    case 16:
    {
        offset = ((raster->nOffsetX << 2) + ((dstParent->width + 3) & ~3) * raster->nOffsetY) << 1;
        break;
    }
    case 32:
    {
        offset = ((raster->nOffsetX << 2) + ((dstParent->width + 3) & ~3) * raster->nOffsetY) << 2;
        break;
    }
    case 24:
    default:
    {
        RWERROR((E_RW_INVRASTERDEPTH));
        return;
    }
    }

    GXCopyTex(dstRasExt->pixels + offset, (GXBool)_RwDlCopyClear);
    GXPixModeSync();

    GXSetCopyFilter(_RwDlRenderMode->aa, _RwDlRenderMode->sample_pattern, GX_TRUE,
                    _RwDlRenderMode->vfilter);

    if (dstRasExt->region)
    {
        GXInvalidateTexRegion((GXTexRegion*)dstRasExt->region);
    }
    else
    {
        GXInvalidateTexAll();
    }
}

void RwGameCubeGetXFBs(void** xfbDisp, void** xfbCopy)
{
    /* Wait for the GPU to catch up */
    while ((_RwDlFrameReadyOnToken == TRUE) || (_RwDlFrameTokenNew != _RwDlFrameTokenCurrent))
    {
    }

    *xfbDisp = _RwGCXFBDisp;
    *xfbCopy = _RwGCXFBCopy;
}

void RwGameCubeSetMinRetraceCount(RwUInt8 minCount)
{
    _RwDlRetraceMinCount = minCount;
}
