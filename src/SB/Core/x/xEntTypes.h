#ifndef XENTTYPES_H
#define XENTTYPES_H

#include "xBase.h"
#include "xMath3.h"
#include "xGrid.h"
#include "xBound.h"

struct xEnt;
struct xScene;
struct xEntAsset;
struct xModelInstance;
struct xLightKit;
struct xEntFrame;
struct xEntCollis;
struct xFFX;
struct xShadowSimpleCache;
struct xEntShadow;

struct xEntFrame
{
    xMat4x3 mat;

    // Offset: 0x40
    xMat4x3 oldmat;

    // Offset: 0x80
    xVec3 oldvel;

    // Offset: 0x8C
    xRot oldrot;

    // Offset: 0x9C
    xRot drot;
    xRot rot;

    // Offset: 0xBC
    xVec3 dpos;

    // Offset: 0xC8
    xVec3 dvel;

    // Offset: 0xD4
    xVec3 vel;

    // Offset: 0xE0
    U32 mode;
}
#if defined(PS2)
__attribute__((aligned(16)))
#endif
;

typedef void (*xEntUpdateCallback)(xEnt*, xScene*, F32);
typedef void (*xEntBoundUpdateCallback)(xEnt*, xVec3*);
typedef void (*xEntMoveCallback)(xEnt*, xScene*, F32, xEntFrame*);
typedef void (*xEntRenderCallback)(xEnt*);
typedef void (*xEntTranslateCallback)(xEnt*, xVec3*, xMat4x3*);

// Size: 0xD0
struct xEnt : xBase
{
    struct anim_coll_data;

    // Offset: 0x10
    xEntAsset* asset;
    U16 idx; //0x14
    U16 num_updates;

    // Offset: 0x18
    U8 flags;
    U8 miscflags;
    U8 subType;

    // Offset: 0x1B
    U8 pflags; // p -> physics flags
    U8 moreFlags; //0x1c
    U8 isCulled;
    U8 driving_count;
    U8 num_ffx;

    // Offset: 0x20
    U8 collType; // XENT_COLLTYPE_* (defined in xEnt.h)
    U8 collLev;
    U8 chkby; // XENT_COLLTYPE_* bitmask
    U8 penby; // XENT_COLLTYPE_* bitmask

    // Offset: 0x24
    xModelInstance* model; // 0x704 in globals
    xModelInstance* collModel;
    xModelInstance* camcollModel;
    xLightKit* lightKit;

    // Offset: 0x34
    xEntUpdateCallback update;
    xEntUpdateCallback endUpdate;
    xEntBoundUpdateCallback bupdate;
    xEntMoveCallback move;

    // Offset: 0x44
    xEntRenderCallback render;
    xEntFrame* frame; // 0x728 in globals
    xEntCollis* collis; //0x4c

    // Offset: 0x50
    xGridBound gridb;

    // Offset: 0x64
    xBound bound;

    // Offset: 0xB0
    xEntTranslateCallback transl; //0xb0
    xFFX* ffx; //0xb4
    xEnt* driver;
    S32 driveMode;

    // Offset: 0xC0
    xShadowSimpleCache* simpShadow;
    xEntShadow* entShadow;
    anim_coll_data* anim_coll;
    void* user_data; // 0xCC
};

#if defined(PS2)
inline void xEntHide(xEnt* ent)
{
    ent->flags &= ~0x1;
}

inline void xEntShow(xEnt* ent)
{
    ent->flags |= 0x1;
}

#endif

#endif
