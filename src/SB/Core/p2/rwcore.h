#ifndef PS2_RWCORE_H
#define PS2_RWCORE_H

// Minimal PS2 RenderWare geometry declarations recovered from retail DWARF.
typedef unsigned char RwUInt8;
typedef unsigned short RwUInt16;
typedef unsigned int RwUInt32;
typedef signed int RwInt32;
typedef float RwReal;
typedef RwInt32 RwBool;

struct RwV3d
{
    RwReal x, y, z;
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

struct RwCamera;
struct RwFrame;
struct RxPipeline;
struct RwResEntry;

extern "C" {
RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in);
RwFrame* RwFrameCreate(void);
RwBool RwFrameDestroy(RwFrame* frame);
RwFrame* RwFrameTransform(RwFrame* frame, const RwMatrix* matrix, RwOpCombineType combine);
RwBool _rwFrameSyncDirty(void);
void _rwObjectHasFrameSetFrame(void* object, RwFrame* frame);
}

#endif
