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
};

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

struct RwCamera;
struct RwRaster;
struct RwTexture;
struct RwFrame;
struct RxPipeline;
struct RwResEntry;

#define RWFORCEENUMSIZEINT ((RwInt32)((~((RwUInt32)0)) >> 1))
#define RWPLUGINOFFSET(_type, _base, _offset) ((_type*)((RwUInt8*)(_base) + (_offset)))

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
RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in);
RwFrame* RwFrameCreate(void);
RwBool RwFrameDestroy(RwFrame* frame);
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

#endif
