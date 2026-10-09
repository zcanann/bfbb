#ifndef PS2_RWPLCORE_H
#define PS2_RWPLCORE_H

#include <types.h>
#include <rwcore.h>
#include <stdarg.h>

// Complete RenderWare 3.5 engine declarations, from include/rwsdk/rwplcore.h.
// RwSky2DVertex is complete in rwim2d.h; engine callbacks only need its pointer.
typedef char RwChar;
struct RwSky2DVertex;
typedef RwSky2DVertex RwIm2DVertex;
typedef RwUInt16 RxVertexIndex;
typedef RxVertexIndex RwImVertexIndex;


// RenderWare 3.5 video-mode declaration, verified in the PS2 originals.
enum RwVideoModeFlag
{
    rwVIDEOMODEEXCLUSIVE = 0x0001,
    rwVIDEOMODEINTERLACE = 0x0002,
    rwVIDEOMODEFFINTERLACE = 0x0004,
    rwVIDEOMODE_PS2_FSAASHRINKBLIT = 0x0100,
    rwVIDEOMODE_PS2_FSAAREADCIRCUIT = 0x0200,
    rwVIDEOMODE_XBOX_WIDESCREEN = 0x0100,
    rwVIDEOMODE_XBOX_PROGRESSIVE = 0x0200,
    rwVIDEOMODE_XBOX_FIELD = 0x0400,
    rwVIDEOMODE_XBOX_10X11PIXELASPECT = 0x0800,
    rwVIDEOMODEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

struct RwVideoMode
{
    RwInt32 width;
    RwInt32 height;
    RwInt32 depth;
    RwVideoModeFlag flags;
    RwInt32 refRate;
    RwInt32 format;
};

extern "C" {
RwVideoMode* RwEngineGetVideoModeInfo(RwVideoMode* modeInfo, RwInt32 modeIndex);
RwInt32 RwEngineGetCurrentVideoMode(void);
}

enum RwRenderState
{
    rwRENDERSTATENARENDERSTATE = 0,
    rwRENDERSTATETEXTURERASTER,
    rwRENDERSTATETEXTUREADDRESS,
    rwRENDERSTATETEXTUREADDRESSU,
    rwRENDERSTATETEXTUREADDRESSV,
    rwRENDERSTATETEXTUREPERSPECTIVE,
    rwRENDERSTATEZTESTENABLE,
    rwRENDERSTATESHADEMODE,
    rwRENDERSTATEZWRITEENABLE,
    rwRENDERSTATETEXTUREFILTER,
    rwRENDERSTATESRCBLEND,
    rwRENDERSTATEDESTBLEND,
    rwRENDERSTATEVERTEXALPHAENABLE,
    rwRENDERSTATEBORDERCOLOR,
    rwRENDERSTATEFOGENABLE,
    rwRENDERSTATEFOGCOLOR,
    rwRENDERSTATEFOGTYPE,
    rwRENDERSTATEFOGDENSITY,
    rwRENDERSTATECULLMODE = 20,
    rwRENDERSTATESTENCILENABLE,
    rwRENDERSTATESTENCILFAIL,
    rwRENDERSTATESTENCILZFAIL,
    rwRENDERSTATESTENCILPASS,
    rwRENDERSTATESTENCILFUNCTION,
    rwRENDERSTATESTENCILFUNCTIONREF,
    rwRENDERSTATESTENCILFUNCTIONMASK,
    rwRENDERSTATESTENCILFUNCTIONWRITEMASK,
    rwRENDERSTATEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RwRenderState RwRenderState;

enum RwPrimitiveType
{
    rwPRIMTYPENAPRIMTYPE = 0,
    rwPRIMTYPELINELIST = 1,
    rwPRIMTYPEPOLYLINE = 2,
    rwPRIMTYPETRILIST = 3,
    rwPRIMTYPETRISTRIP = 4,
    rwPRIMTYPETRIFAN = 5,
    rwPRIMTYPEPOINTLIST = 6,
    rwPRIMITIVETYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RwPrimitiveType RwPrimitiveType;

typedef int (*vecSprintfFunc)(RwChar* buffer, const RwChar* format, ...);
typedef int (*vecVsprintfFunc)(RwChar* buffer, const RwChar* format, va_list argptr);
typedef RwChar* (*vecStrcpyFunc)(RwChar* dest, const RwChar* srce);
typedef RwChar* (*vecStrncpyFunc)(RwChar* dest, const RwChar* srce, size_t size);
typedef RwChar* (*vecStrcatFunc)(RwChar* dest, const RwChar* srce);
typedef RwChar* (*vecStrncatFunc)(RwChar* dest, const RwChar* srce, size_t size);
typedef RwChar* (*vecStrrchrFunc)(const RwChar* string, int findThis);
typedef RwChar* (*vecStrchrFunc)(const RwChar* string, int findThis);
typedef RwChar* (*vecStrstrFunc)(const RwChar* string, const RwChar* findThis);
typedef int (*vecStrcmpFunc)(const RwChar* string1, const RwChar* string2);
typedef int (*vecStrncmpFunc)(const RwChar* string1, const RwChar* string2, size_t max_size);
typedef int (*vecStricmpFunc)(const RwChar* string1, const RwChar* string2);
typedef size_t (*vecStrlenFunc)(const RwChar* string);
typedef RwChar* (*vecStruprFunc)(RwChar* string);
typedef RwChar* (*vecStrlwrFunc)(RwChar* string);
typedef RwChar* (*vecStrtokFunc)(RwChar* string, const RwChar* delimit);
typedef int (*vecSscanfFunc)(const RwChar* buffer, const RwChar* format, ...);

struct RwStringFunctions
{
    vecSprintfFunc vecSprintf;
    vecVsprintfFunc vecVsprintf;
    vecStrcpyFunc vecStrcpy;
    vecStrncpyFunc vecStrncpy;
    vecStrcatFunc vecStrcat;
    vecStrncatFunc vecStrncat;
    vecStrrchrFunc vecStrrchr;
    vecStrchrFunc vecStrchr;
    vecStrstrFunc vecStrstr;
    vecStrcmpFunc vecStrcmp;
    vecStrncmpFunc vecStrncmp;
    vecStricmpFunc vecStricmp;
    vecStrlenFunc vecStrlen;
    vecStruprFunc vecStrupr;
    vecStrlwrFunc vecStrlwr;
    vecStrtokFunc vecStrtok;
    vecSscanfFunc vecSscanf;
};

struct RwMemoryFunctions
{
    void* (*rwmalloc)(size_t size);
    void (*rwfree)(void* mem);
    void* (*rwrealloc)(void* mem, size_t newSize);
    void* (*rwcalloc)(size_t numObj, size_t sizeObj);
};

struct RwFreeList
{
    RwUInt32 entrySize; // 0x0
    RwUInt32 entriesPerBlock; // 0x4
    RwUInt32 heapSize; // 0x8
    RwUInt32 alignment; // 0xC
    RwLinkList blockList; // 0x10
    RwUInt32 flags; // 0x18
    RwLLLink link; // 0x1C
};

typedef void (*RwFreeListCallBack)(void* pMem, void* pData);
typedef void* (*RwMemoryAllocFn)(RwFreeList* fl);
typedef RwFreeList* (*RwMemoryFreeFn)(RwFreeList* fl, void* pData);

typedef RwBool (*rwFnFexist)(const RwChar* name);
typedef void* (*rwFnFopen)(const RwChar* name, const RwChar* mode);
typedef int (*rwFnFclose)(void* fptr);
typedef size_t (*rwFnFread)(void* addr, size_t size, size_t count, void* fptr);
typedef size_t (*rwFnFwrite)(const void* addr, size_t size, size_t count, void* fptr);
typedef RwChar* (*rwFnFgets)(RwChar* buffer, int maxLen, void* fptr);
typedef int (*rwFnFputs)(const RwChar* buffer, void* fptr);
typedef int (*rwFnFeof)(void* fptr);
typedef int (*rwFnFseek)(void* fptr, long offset, int origin);
typedef int (*rwFnFflush)(void* fptr);
typedef int (*rwFnFtell)(void* fptr);

struct RwFileFunctions
{
    rwFnFexist rwfexist;
    rwFnFopen rwfopen;
    rwFnFclose rwfclose;
    rwFnFread rwfread;
    rwFnFwrite rwfwrite;
    rwFnFgets rwfgets;
    rwFnFputs rwfputs;
    rwFnFeof rwfeof;
    rwFnFseek rwfseek;
    rwFnFflush rwfflush;
    rwFnFtell rwftell;
};

typedef RwBool (*RwStandardFunc)(void* pOut, void* pInOut, RwInt32 nI);

typedef RwBool (*RwSystemFunc)(RwInt32 nOption, void* pOut, void* pInOut, RwInt32 nIn);
typedef RwBool (*RwRenderStateSetFunction)(RwRenderState nState, void* pParam);
typedef RwBool (*RwRenderStateGetFunction)(RwRenderState nState, void* pParam);
typedef RwBool (*RwIm2DRenderLineFunction)(RwIm2DVertex* vertices, RwInt32 numVertices,
                                           RwInt32 vert1, RwInt32 vert2);
typedef RwBool (*RwIm2DRenderTriangleFunction)(RwIm2DVertex* vertices, RwInt32 numVertices,
                                               RwInt32 vert1, RwInt32 vert2, RwInt32 vert3);
typedef RwBool (*RwIm2DRenderPrimitiveFunction)(RwPrimitiveType primType, RwIm2DVertex* vertices,
                                                RwInt32 numVertices);
typedef RwBool (*RwIm2DRenderIndexedPrimitiveFunction)(RwPrimitiveType primType,
                                                       RwIm2DVertex* vertices, RwInt32 numVertices,
                                                       RwImVertexIndex* indices,
                                                       RwInt32 numIndices);
typedef RwBool (*RwIm3DRenderLineFunction)(RwInt32 vert1, RwInt32 vert2);
typedef RwBool (*RwIm3DRenderTriangleFunction)(RwInt32 vert1, RwInt32 vert2, RwInt32 vert3);
typedef RwBool (*RwIm3DRenderPrimitiveFunction)(RwPrimitiveType primType);
typedef RwBool (*RwIm3DRenderIndexedPrimitiveFunction)(RwPrimitiveType primtype,
                                                       RwImVertexIndex* indices,
                                                       RwInt32 numIndices);

struct RwDevice
{
    RwReal gammaCorrection; // 0x0
    RwSystemFunc fpSystem; // 0x4
    RwReal zBufferNear; // 0x8
    RwReal zBufferFar; // 0xC
    RwRenderStateSetFunction fpRenderStateSet; // 0x10
    RwRenderStateGetFunction fpRenderStateGet; // 0x14
    RwIm2DRenderLineFunction fpIm2DRenderLine; // 0x18
    RwIm2DRenderTriangleFunction fpIm2DRenderTriangle; // 0x1C
    RwIm2DRenderPrimitiveFunction fpIm2DRenderPrimitive; // 0x20
    RwIm2DRenderIndexedPrimitiveFunction fpIm2DRenderIndexedPrimitive; // 0x24
    RwIm3DRenderLineFunction fpIm3DRenderLine; // 0x28
    RwIm3DRenderTriangleFunction fpIm3DRenderTriangle; // 0x2C
    RwIm3DRenderPrimitiveFunction fpIm3DRenderPrimitive; // 0x30
    RwIm3DRenderIndexedPrimitiveFunction fpIm3DRenderIndexedPrimitive; // 0x34
};

struct RwMetrics
{
    RwUInt32 numTriangles;
    RwUInt32 numProcTriangles;
    RwUInt32 numVertices;
    RwUInt32 numTextureUploads;
    RwUInt32 sizeTextureUploads;
    RwUInt32 numResourceAllocs;
    void* devSpecificMetrics;
};

enum RwEngineStatus
{
    rwENGINESTATUSIDLE = 0,
    rwENGINESTATUSINITED = 1,
    rwENGINESTATUSOPENED = 2,
    rwENGINESTATUSSTARTED = 3,
    rwENGINESTATUSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RwEngineStatus RwEngineStatus;

struct RwGlobals
{
    void* curCamera; // 0x0
    void* curWorld; // 0x4
    RwUInt16 renderFrame; // 0x8
    RwUInt16 lightFrame; // 0xA
    RwUInt16 pad[2]; // 0xC
    RwDevice dOpenDevice; // 0x10
    RwStandardFunc stdFunc[29]; // 0x48
    RwLinkList dirtyFrameList; // 0xBC
    RwFileFunctions fileFuncs; // 0xC4
    RwStringFunctions stringFuncs; // 0xF0
    RwMemoryFunctions memoryFuncs; // 0x134
    RwMemoryAllocFn memoryAlloc; // 0x144
    RwMemoryFreeFn memoryFree; // 0x148
    RwMetrics* metrics; // 0x14C
    RwEngineStatus engineStatus; // 0x150
    RwUInt32 resArenaInitSize; // 0x154
};

// Retail PS2 uses this named storage directly, rather than the GC engine pointer.
// DWARF declares unsigned int[4096]; allocator loads are at +0x134/+0x138.
extern RwUInt32 ourGlobals[4096];
#define RwEngineInstance ((RwGlobals*)ourGlobals)
#define RWSRCGLOBAL(variable) (((RwGlobals*)RwEngineInstance)->variable)
#define rwstrcmp RWSRCGLOBAL(stringFuncs).vecStrcmp
#define RwMalloc(_s) ((RWSRCGLOBAL(memoryFuncs).rwmalloc)((_s)))
#define RwFree(_p) ((RWSRCGLOBAL(memoryFuncs).rwfree)((_p)))
#define RwCalloc(_n, _s) ((RWSRCGLOBAL(memoryFuncs).rwcalloc)((_n), (_s)))
#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

// RenderWare resource entries; the PS2 originals describe this 24-byte layout.
typedef void (*RwResEntryDestroyNotify)(RwResEntry* resEntry);
struct RwResEntry
{
    RwLLLink link;
    RwInt32 size;
    void* owner;
    RwResEntry** ownerRef;
    RwResEntryDestroyNotify destroyNotify;
};

// RenderWare SDK stream, chunk and error declarations, from include/rwsdk/rwplcore.h.
enum RwCorePluginID
{
    rwID_NAOBJECT = 0x00,
    rwID_STRUCT = 0x01,
    rwID_STRING = 0x02,
    rwID_EXTENSION = 0x03,
    rwID_CAMERA = 0x05,
    rwID_TEXTURE = 0x06,
    rwID_MATERIAL = 0x07,
    rwID_MATLIST = 0x08,
    rwID_ATOMICSECT = 0x09,
    rwID_PLANESECT = 0x0A,
    rwID_WORLD = 0x0B,
    rwID_SPLINE = 0x0C,
    rwID_MATRIX = 0x0D,
    rwID_FRAMELIST = 0x0E,
    rwID_GEOMETRY = 0x0F,
    rwID_CLUMP = 0x10,
    rwID_LIGHT = 0x12,
    rwID_UNICODESTRING = 0x13,
    rwID_ATOMIC = 0x14,
    rwID_TEXTURENATIVE = 0x15,
    rwID_TEXDICTIONARY = 0x16,
    rwID_ANIMDATABASE = 0x17,
    rwID_IMAGE = 0x18,
    rwID_SKINANIMATION = 0x19,
    rwID_GEOMETRYLIST = 0x1A,
    rwID_HANIMANIMATION = 0x1B,
    rwID_TEAM = 0x1C,
    rwID_CROWD = 0x1D,
    rwID_DMORPHANIMATION = 0x1E,
    rwID_RIGHTTORENDER = 0x1f,
    rwID_MTEFFECTNATIVE = 0x20,
    rwID_MTEFFECTDICT = 0x21,
    rwID_TEAMDICTIONARY = 0x22,
    rwID_PITEXDICTIONARY = 0x23,
    rwID_TOC = 0x24,
    rwID_PRTSTDGLOBALDATA = 0x25,
    rwID_ALTPIPE = 0x26,
    rwID_PIPEDS = 0x27,
    rwID_PATCHMESH = 0x28,
    rwID_COREPLUGINIDMAX = 0x29,
    rwCOREPLUGINIDFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RwCorePluginID RwCorePluginID;

enum RwStreamType
{
    rwNASTREAM = 0,
    rwSTREAMFILE,
    rwSTREAMFILENAME,
    rwSTREAMMEMORY,
    rwSTREAMCUSTOM,
    rwSTREAMTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RwStreamType RwStreamType;

enum RwStreamAccessType
{
    rwNASTREAMACCESS = 0,
    rwSTREAMREAD,
    rwSTREAMWRITE,
    rwSTREAMAPPEND,
    rwSTREAMACCESSTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RwStreamAccessType RwStreamAccessType;

struct RwMemory
{
    RwUInt8* start;
    RwUInt32 length;
};

struct RwStream;

struct RwError
{
    RwInt32 pluginID;
    RwInt32 errorCode;
};

struct RwChunkHeaderInfo
{
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 version;
    RwUInt32 buildNum;
    RwBool isComplex;
};

extern "C" {
RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType accessType, const void* pData);
RwBool RwStreamClose(RwStream* stream, void* pData);
RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut);
RwStream* RwStreamReadChunkHeaderInfo(RwStream* stream, RwChunkHeaderInfo* chunkHeaderInfo);
void* RwMemNative32(void* mem, RwUInt32 size);
RwError* RwErrorGet(RwError* code);
}

// RenderWare 3.5 engine start-up API used by the PS2 platform layer.
struct RwMemoryFunctions;

struct RwEngineOpenParams
{
    void* displayID;
};

enum RwEngineInitFlag
{
    rwENGINEINITFREELISTS = 0,
    rwENGINEINITNOFREELISTS = 0x1,
    rwENGINEINITFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

extern "C" {
RwBool RwEngineInit(const RwMemoryFunctions* memFuncs, RwUInt32 initFlags, RwUInt32 resArenaSize);
RwBool RwEngineOpen(RwEngineOpenParams* initParams);
RwBool RwEngineStart(void);
RwBool RwEngineStop(void);
RwBool RwEngineClose(void);
RwBool RwEngineTerm(void);
RwInt32 RwEngineGetNumVideoModes(void);
RwBool RwEngineSetVideoMode(RwInt32 modeIndex);
RwBool RwResourcesSetArenaSize(RwUInt32 size);
RwInt32 RwResourcesGetArenaSize(void);
RwBool RwResourcesEmptyArena(void);
RwBool RwResourcesFreeResEntry(RwResEntry* entry);
RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                      RwResEntryDestroyNotify destroyNotify);
}

#endif
