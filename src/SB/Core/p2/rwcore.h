#ifndef PS2_RWCORE_H
#define PS2_RWCORE_H

// Minimal PS2 RenderWare geometry declarations recovered from retail DWARF.
typedef unsigned char RwUInt8;
typedef unsigned int RwUInt32;
typedef signed int RwInt32;
typedef float RwReal;

struct RwV3d
{
    RwReal x, y, z;
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

struct RwFrame;
struct RxPipeline;

#endif
