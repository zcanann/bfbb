#ifndef PS2_RWCORE_H
#define PS2_RWCORE_H

// Statement macros shared by the RenderWare SDK headers.
#ifndef MACRO_START
#define MACRO_START do
#endif
#ifndef MACRO_STOP
#define MACRO_STOP while (0)
#endif

// Minimal PS2 RenderWare geometry declarations recovered from retail DWARF.
typedef unsigned char RwUInt8;
typedef unsigned short RwUInt16;
typedef signed short RwInt16;
typedef unsigned int RwUInt32;
typedef signed int RwInt32;
typedef float RwReal;
typedef char RwChar;

struct RwV2d
{
    RwReal x, y;
};
typedef RwInt32 RwBool;

struct RxObjSpace3DVertex;

struct RwSurfaceProperties
{
    RwReal ambient;
    RwReal specular;
    RwReal diffuse;
};

struct RwV3d
{
    RwReal x, y, z;
};

struct RwLine
{
    RwV3d start;
    RwV3d end;
};

union RwSplitBits
{
    RwReal nReal;
    RwInt32 nInt;
    RwUInt32 nUInt;
};

struct RwSphere
{
    RwV3d center;
    float radius;
};

struct RwTexCoords
{
    float u;
    float v;
};

struct RwBBox
{
    RwV3d sup;
    RwV3d inf;
};

struct RwObject
{
    RwUInt8 type;
    RwUInt8 subType;
    RwUInt8 flags;
    RwUInt8 privateFlags;
    void* parent;
};

struct RwLLLink
{
    RwLLLink* next;
    RwLLLink* prev;
};

struct RwLinkList
{
    RwLLLink link;
};

struct RwRGBA
{
    RwUInt8 red, green, blue, alpha;
};

enum RwFogType
{
    rwFOGTYPENAFOGTYPE = 0,
    rwFOGTYPELINEAR = 1,
    rwFOGTYPEEXPONENTIAL = 2,
    rwFOGTYPEEXPONENTIAL2 = 3,
    rwFOGTYPEFORCEENUMSIZEINT = 0x7fffffff
};

struct RwRGBAReal
{
    RwReal red, green, blue, alpha;
};

struct RwObjectHasFrame
{
    RwObject object;
    RwLLLink lFrame;
    RwObjectHasFrame* (*sync)(RwObjectHasFrame*);
};

// Original PS2 embeddings place matrices on 16-byte boundaries (see PS2_FRAME_PTANK_PICKUP.md).
struct RwMatrixTag
{
    RwV3d right;
    RwUInt32 flags;
    RwV3d up;
    RwUInt32 pad1;
    RwV3d at;
    RwUInt32 pad2;
    RwV3d pos;
    RwUInt32 pad3;
} __attribute__((aligned(16)));

typedef RwMatrixTag RwMatrix;

enum RwOpCombineType
{
    rwCOMBINEREPLACE = 0,
    rwCOMBINEPRECONCAT = 1,
    rwCOMBINEPOSTCONCAT = 2,
    rwOPCOMBINETYPEFORCEENUMSIZEINT = 0x7fffffff
};

// Complete original PS2 immediate-mode vertex and texture storage.
struct RxColorUnion
{
    union
    {
        RwRGBA preLitColor;
        RwRGBA color;
    };
};

struct RxObjSpace3DVertex
{
    RwV3d objVertex;
    RxColorUnion c;
    RwV3d objNormal;
    RwReal u;
    RwReal v;
};

typedef RxObjSpace3DVertex RxObjSpace3DLitVertex;
typedef RxObjSpace3DLitVertex RwIm3DVertex;

struct RwTexDictionary;

struct RwRaster
{
    RwRaster* parent;
    RwUInt8* cpPixels;
    RwUInt8* palette;
    RwInt32 width, height, depth;
    RwInt32 stride;
    RwInt16 nOffsetX, nOffsetY;
    RwUInt8 cType;
    RwUInt8 cFlags;
    RwUInt8 privateFlags;
    RwUInt8 cFormat;
    RwUInt8* originalPixels;
    RwInt32 originalWidth;
    RwInt32 originalHeight;
    RwInt32 originalStride;
};

struct RwTexture
{
    RwRaster* raster;
    RwTexDictionary* dict;
    RwLLLink lInDictionary;
    RwChar name[32];
    RwChar mask[32];
    RwUInt32 filterAddressing;
    RwInt32 refCount;
};

struct RwFrame
{
    RwObject object;
    RwLLLink inDirtyListLink;
    RwMatrix modelling;
    RwMatrix ltm;
    RwLinkList objectList;
    struct RwFrame* child;
    struct RwFrame* next;
    struct RwFrame* root;
};

struct RwCamera;
struct RwRaster;
struct RwTexture;
struct RwFrame;
struct RxPipeline;
struct RwResEntry;

#define RWFORCEENUMSIZEINT ((RwInt32)((~((RwUInt32)0)) >> 1))
#define RWPLUGINOFFSET(_type, _base, _offset) ((_type*)((RwUInt8*)(_base) + (_offset)))

// Complete SDK rendering-state declarations.
enum RwRasterType
{
    rwRASTERTYPENORMAL = 0x00,
    rwRASTERTYPEZBUFFER = 0x01,
    rwRASTERTYPECAMERA = 0x02,
    rwRASTERTYPETEXTURE = 0x04,
    rwRASTERTYPECAMERATEXTURE = 0x05,
    rwRASTERTYPEMASK = 0x07,
    rwRASTERDONTALLOCATE = 0x80,
    rwRASTERTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};


enum RwShadeMode
{
    rwSHADEMODENASHADEMODE = 0,
    rwSHADEMODEFLAT,
    rwSHADEMODEGOURAUD,
    rwSHADEMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

enum RwTextureFilterMode
{
    rwFILTERNAFILTERMODE = 0,
    rwFILTERNEAREST,
    rwFILTERLINEAR,
    rwFILTERMIPNEAREST,
    rwFILTERMIPLINEAR,
    rwFILTERLINEARMIPNEAREST,
    rwFILTERLINEARMIPLINEAR,
    rwTEXTUREFILTERMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

enum RwBlendFunction
{
    rwBLENDNABLEND = 0,
    rwBLENDZERO,
    rwBLENDONE,
    rwBLENDSRCCOLOR,
    rwBLENDINVSRCCOLOR,
    rwBLENDSRCALPHA,
    rwBLENDINVSRCALPHA,
    rwBLENDDESTALPHA,
    rwBLENDINVDESTALPHA,
    rwBLENDDESTCOLOR,
    rwBLENDINVDESTCOLOR,
    rwBLENDSRCALPHASAT,
    rwBLENDFUNCTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

enum RwTextureAddressMode
{
    rwTEXTUREADDRESSNATEXTUREADDRESS = 0,
    rwTEXTUREADDRESSWRAP,
    rwTEXTUREADDRESSMIRROR,
    rwTEXTUREADDRESSCLAMP,
    rwTEXTUREADDRESSBORDER,
    rwTEXTUREADDRESSMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

struct RxRenderStateVector
{
    RwUInt32 Flags;
    RwShadeMode ShadeMode;
    RwBlendFunction SrcBlend;
    RwBlendFunction DestBlend;
    RwRaster* TextureRaster;
    RwTextureAddressMode AddressModeU;
    RwTextureAddressMode AddressModeV;
    RwTextureFilterMode FilterMode;
    RwRGBA BorderColor;
    RwFogType FogType;
    RwRGBA FogColor;
};

// Complete SDK camera types; original PS2 layouts are verified in all debug regions.
struct RwPlane
{
    RwV3d normal;
    RwReal distance;
};

enum RwCameraClearMode
{
    rwCAMERACLEARIMAGE = 0x1,
    rwCAMERACLEARZ = 0x2,
    rwCAMERACLEARSTENCIL = 0x4,
    rwCAMERACLEARMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

enum RwCameraProjection
{
    rwNACAMERAPROJECTION = 0,
    rwPERSPECTIVE = 1,
    rwPARALLEL = 2,
    rwCAMERAPROJECTIONFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RwCameraProjection RwCameraProjection;

enum RwFrustumTestResult
{
    rwSPHEREOUTSIDE = 0,
    rwSPHEREBOUNDARY = 1,
    rwSPHEREINSIDE = 2,
    rwFRUSTUMTESTRESULTFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RwFrustumTestResult RwFrustumTestResult;

struct RwFrustumPlane
{
    RwPlane plane;
    RwUInt8 closestX;
    RwUInt8 closestY;
    RwUInt8 closestZ;
    RwUInt8 pad;
};

typedef RwCamera* (*RwCameraBeginUpdateFunc)(RwCamera* camera);
typedef RwCamera* (*RwCameraEndUpdateFunc)(RwCamera* camera);

struct RwCamera
{
    RwObjectHasFrame object;
    RwCameraProjection projectionType;
    RwCameraBeginUpdateFunc beginUpdate;
    RwCameraEndUpdateFunc endUpdate;
    RwMatrix viewMatrix;
    RwRaster* frameBuffer;
    RwRaster* zBuffer;
    RwV2d viewWindow;
    RwV2d recipViewWindow;
    RwV2d viewOffset;
    RwReal nearPlane;
    RwReal farPlane;
    RwReal fogPlane;
    RwReal zScale, zShift;
    RwFrustumPlane frustumPlanes[6];
    RwBBox frustumBoundBox;
    RwV3d frustumCorners[8];
};

#define rwObjectGetParent(object) (((const RwObject*)(object))->parent)
#define RwTextureGetRasterMacro(_tex) ((_tex)->raster)
#define RwTextureGetRaster(_tex) RwTextureGetRasterMacro(_tex)
#define rwObjectHasFrameSetFrame(object, frame) _rwObjectHasFrameSetFrame(object, frame)
#define RwCameraGetCurrentCamera() ((RwCamera*)RWSRCGLOBAL(curCamera))
#define RwCameraGetFrame(_camera) ((RwFrame*)rwObjectGetParent((_camera)))
#define RwCameraGetViewWindow(_camera) (&((_camera)->viewWindow))

#define RwV3dSubMacro(o, a, b)                                                                     \
    MACRO_START                                                                                    \
    {                                                                                              \
        (o)->x = (((a)->x) - ((b)->x));                                                            \
        (o)->y = (((a)->y) - ((b)->y));                                                            \
        (o)->z = (((a)->z) - ((b)->z));                                                            \
    }                                                                                              \
    MACRO_STOP

#define RwV3dScaleMacro(o, a, s)                                                                   \
    MACRO_START                                                                                    \
    {                                                                                              \
        (o)->x = (((a)->x) * ((s)));                                                               \
        (o)->y = (((a)->y) * ((s)));                                                               \
        (o)->z = (((a)->z) * ((s)));                                                               \
    }                                                                                              \
    MACRO_STOP

#define RwV3dDotProductMacro(a, b)                                                                 \
    ((((((((a)->x) * ((b)->x))) + ((((a)->y) * ((b)->y))))) + ((((a)->z) * ((b)->z)))))

#define RwV3dCrossProductMacro(o, a, b)                                                            \
    MACRO_START                                                                                    \
    {                                                                                              \
        (o)->x = (((((a)->y) * ((b)->z))) - ((((a)->z) * ((b)->y))));                              \
        (o)->y = (((((a)->z) * ((b)->x))) - ((((a)->x) * ((b)->z))));                              \
        (o)->z = (((((a)->x) * ((b)->y))) - ((((a)->y) * ((b)->x))));                              \
    }                                                                                              \
    MACRO_STOP

extern "C" {
RxRenderStateVector* RxRenderStateVectorLoadDriverState(RxRenderStateVector* rsvp);
RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in);
RwFrame* RwFrameCreate(void);
RwMatrix* RwFrameGetLTM(RwFrame* frame);
RwCamera* RwCameraCreate(void);
RwCamera* RwCameraEndUpdate(RwCamera* camera);
RwCamera* RwCameraBeginUpdate(RwCamera* camera);
RwCamera* RwCameraClear(RwCamera* camera, RwRGBA* colour, RwInt32 clearMode);
RwCamera* RwCameraSetViewWindow(RwCamera* camera, const RwV2d* viewWindow);
RwCamera* RwCameraSetProjection(RwCamera* camera, RwCameraProjection projection);
RwCamera* RwCameraSetNearClipPlane(RwCamera* camera, RwReal nearClip);
RwCamera* RwCameraSetFarClipPlane(RwCamera* camera, RwReal farClip);
RwFrustumTestResult RwCameraFrustumTestSphere(const RwCamera* camera, const RwSphere* sphere);
RwTexture* RwTextureCreate(RwRaster* raster);
RwBool RwTextureDestroy(RwTexture* texture);
RwBool RwCameraDestroy(RwCamera* camera);
RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags);
RwBool RwRasterDestroy(RwRaster* raster);
RwBool RwFrameDestroy(RwFrame* frame);
RwFrame* RwFrameRotate(RwFrame* frame, const RwV3d* axis, RwReal angle, RwOpCombineType combine);
RwFrame* RwFrameTranslate(RwFrame* frame, const RwV3d* v, RwOpCombineType combine);
RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* matrix, RwOpCombineType combine);
RwBool _rwFrameSyncDirty(void);
void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
}

extern "C" {
RwMatrix* RwMatrixUpdate(RwMatrix* matrix);
RwMatrix* RwMatrixInvert(RwMatrix* matrixOut, const RwMatrix* matrixIn);
}

// RenderWare SDK matrix assignment macro.
#define RwMatrixCopyMacro(_target, _source) (*(_target) = *(_source))

// RenderWare SDK matrix classifications and geometry API.
enum RwMatrixType
{
    rwMATRIXTYPENORMAL = 0x00000001,
    rwMATRIXTYPEORTHOGONAL = 0x00000002,
    rwMATRIXTYPEORTHONORMAL = 0x00000003,
    rwMATRIXTYPEMASK = 0x00000003,
    rwMATRIXTYPEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

extern "C" {
RwMatrix* RwMatrixScale(RwMatrix* matrix, const RwV3d* scale, RwOpCombineType combineOp);
RwV3d* RwV3dTransformPoints(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints,
                          const RwMatrix* matrix);
}

#define RwFrameGetMatrix(_f) (&(_f)->modelling)

#endif
