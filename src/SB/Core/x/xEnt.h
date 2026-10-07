#ifndef XENT_H
#define XENT_H

#include <rwcore.h>
#include <rpworld.h>
#include <string.h>

#include "xBase.h"
#include "xMath3.h"
#include "xModel.h"
#include "xLightKit.h"
#include "xGrid.h"
#include "xBound.h"
#include "xFFX.h"
#include "xCollide.h"

struct xEntAsset : xBaseAsset
{
    // Offset: 0x8
    U8 flags;
    U8 subtype;
    U8 pflags;
    U8 moreFlags;
    U8 pad;
    //U8 padding[3]; // this padding is added automatically. it should not be here

    // Offset: 0x10
    U32 surfaceID;

    // Offset: 0x14
    xVec3 ang;

    // Offset: 0x20
    xVec3 pos;

    // Offset: 0x2C
    xVec3 scale;

    // Offset: 0x38
    F32 redMult;
    F32 greenMult;
    F32 blueMult;
    F32 seeThru;

    // Offset: 0x48
    F32 seeThruSpeed;
    U32 modelInfoID;
    U32 animListID;
};

struct xEnt;
struct xScene;

struct xEntCollis
{
    U8 chk;
    U8 pen;
    U8 env_sidx;
    U8 env_eidx;

    U8 npc_sidx;
    U8 npc_eidx;
    U8 dyn_sidx;
    U8 dyn_eidx;

    U8 stat_sidx;
    U8 stat_eidx;
    U8 idx;

    xCollis colls[18];
    void (*post)(xEnt*, xScene*, F32, xEntCollis*);
    U32 (*depenq)(xEnt*, xEnt*, xScene*, F32, xCollis*);
};

struct xShadowSimpleCache;
struct xEntShadow;

#include "xEntTypes.h"

struct xEnt::anim_coll_data
{
    U32 flags;
    U32 bones;
    xMat4x3 old_mat;
    xMat4x3 new_mat;
    U32 verts_size;
    xVec3* verts;
    xVec3* normals;
};

// Ent flags (xEnt::flags)
#define XENT_IS_VISIBLE ((U8)(1 << 0))
#define XENT_IS_STACKED ((U8)(1 << 1))
#define XENT_0x10 ((U8)(1 << 4))
#define XENT_0x40 ((U8)(1 << 6))
#define XENT_0x80 ((U8)(1 << 7))

// Physics flags (xEnt::pflags)
#define XENT_PFLAGS_IS_MOVING ((U8)(1 << 0))
#define XENT_PFLAGS_HAS_VELOCITY ((U8)(1 << 1))
#define XENT_PFLAGS_HAS_GRAVITY ((U8)(1 << 2))
#define XENT_PFLAGS_HAS_DRAG ((U8)(1 << 3))
#define XENT_PFLAGS_HAS_FRICTION ((U8)(1 << 4))

// More ent flags (xEnt::moreFlags)
#define XENT_MORE_FLAGS_0x8 ((U8)1 << 3)
#define XENT_MORE_FLAGS_HITTABLE ((U8)1 << 4)
#define XENT_MORE_FLAGS_ANIM_COLL ((U8)1 << 5)

// Collision types (xEnt::collType)
#define XENT_COLLTYPE_NONE (U8)0
#define XENT_COLLTYPE_TRIG ((U8)(1 << (0)))
#define XENT_COLLTYPE_STAT ((U8)(1 << (1)))
#define XENT_COLLTYPE_DYN ((U8)(1 << (2)))
#define XENT_COLLTYPE_NPC ((U8)(1 << (3)))
#define XENT_COLLTYPE_PLYR ((U8)(1 << (4)))
#define XENT_COLLTYPE_ENV ((U8)(1 << (5)))

// Size: 0x40
struct xEntShadow
{
    enum radius_enum
    {
        RADIUS_CACHE,
        RADIUS_RASTER,
        MAX_RADIUS
    };

    xVec3 pos;
    xVec3 vec;
    RpAtomic* shadowModel;
    F32 dst_cast;
    F32 radius[2];
};

extern S32 xent_entent;

#if defined(PS2)
inline xMat4x3* xEntGetFrame(const xEnt* ent)
{
    return xModelGetFrame(ent->model);
}
#else
xMat4x3* xEntGetFrame(const xEnt* ent);
#endif
#if defined(PS2)
inline void xEntEnable(xEnt* ent)
{
    xBaseEnable(ent);
}
#else
void xEntEnable(xEnt* ent);
#endif
xVec3* xEntGetCenter(const xEnt* ent);
xVec3* xEntGetPos(const xEnt* ent);
#if defined(PS2)
inline U32 xEntIsVisible(const xEnt* ent)
{
    return (ent->flags & 0x81) == 0x1;
}
#else
U32 xEntIsVisible(const xEnt* ent);
#endif
void xEntHide(xEnt* ent);
void xEntShow(xEnt* ent);
void xEntInitShadow(xEnt& ent, xEntShadow& shadow);
void xEntReposition(xEnt& ent, const xMat4x3& mat);
bool xEntValidType(U8 type);
void xEntAnimateCollision(xEnt& ent, bool on);
xBox* xEntGetAllEntsBox();
void xEntSetNostepNormAngle(F32 angle);
void xEntCollideWalls(xEnt* p, xScene* sc, F32 dt);
void xEntCollideCeiling(xEnt* p, xScene* sc, F32 dt);
void xEntCollideFloor(xEnt* p, xScene* sc, F32 dt);
xEnt* xEntCollCheckOneEntNoDepen(xEnt* ent, xScene* sc, void* data);
void xEntCollCheckNPCs(xEnt* p, xScene* sc, xEnt* (*hitIt)(xEnt*, xScene*, void*));
void xEntCollCheckDyns(xEnt* p, xScene* sc, xEnt* (*hitIt)(xEnt*, xScene*, void*));
void xEntCollCheckStats(xEnt* p, xScene* sc, xEnt* (*hitIt)(xEnt*, xScene*, void*));
void xEntCollCheckNPCsByGrid(xEnt* p, xScene* sc, xEnt* (*hitIt)(xEnt*, xScene*, void*));
void xEntCollCheckByGrid(xEnt* p, xScene* sc, xEnt* (*hitIt)(xEnt*, xScene*, void*));
void xEntCollCheckEnv(xEnt* p, xScene* sc);
void xEntEndCollide(xEnt* ent, xScene* sc, F32 dt);
void xEntBeginCollide(xEnt* ent, xScene* sc, F32 dt);
void xEntCollide(xEnt* ent, xScene* sc, F32 dt);
void xEntApplyPhysics(xEnt* ent, xScene* sc, F32 dt);
void xEntMove(xEnt* ent, xScene* sc, F32 dt);
void xEntMotionToMatrix(xEnt* ent, xEntFrame* frame);
void xEntDefaultTranslate(xEnt* ent, xVec3* dpos, xMat4x3* dmat);
void xEntDefaultBoundUpdate(xEnt* ent, xVec3* pos);
void xEntEndUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntBeginUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntUpdate(xEnt* ent, xScene* sc, F32 dt);
void xEntRender(xEnt* ent);
void xEntRestorePipeline(xSurface*, RpAtomic* model);
void xEntRestorePipeline(xModelInstance* model);
void xEntSetupPipeline(xSurface* surf, RpAtomic* model);
void xEntSetupPipeline(xModelInstance* model);
void xEntAddToPos(xEnt* ent, const xVec3* v);
xModelInstance* xEntLoadModel(xEnt* ent, RpAtomic* imodel);
void xEntReset(xEnt* ent);
void xEntLoad(xEnt* ent, xSerial* s);
void xEntSave(xEnt* ent, xSerial* s);
void xEntSetup(xEnt* ent);
void xEntInitForType(xEnt* ent);
void xEntInit(xEnt* ent, xEntAsset* asset);
void xEntAddHittableFlag(xEnt* ent);
void xEntSceneExit();
void xEntSceneInit();
void xEntSetTimePassed(F32 sec);

#if !defined(PS2)
inline void xEntHide(xEnt* ent)
{
    ent->flags &= ~0x1;
}

inline void xEntShow(xEnt* ent)
{
    ent->flags |= 0x1;
}

#endif

#if defined(PS2)
#include "xEntPosition.h"
#else
inline xVec3* xEntGetPos(const xEnt* ent)
{
    return &xModelGetFrame(ent->model)->pos;
}
#endif

inline xVec3* xEntGetCenter(const xEnt* ent)
{
    return (xVec3*)xBoundCenter(&ent->bound);
}

#endif
