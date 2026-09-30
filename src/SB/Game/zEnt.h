#ifndef ZENT_H
#define ZENT_H

#include "xEnt.h"
#include "xAnim.h"

#include "zScene.h"
#include "zEvent.h"

//For inline/weak functions
#include "xSnd.h"

struct zScene;

struct _ShadowParams
{
    U32 type;
    F32 at;
    F32 rad;
};

struct zEnt : xEnt
{
    xAnimTable* atbl;
};

void checkpoint_collision_hack(zEnt* ent);
char* zParamGetString(xModelAssetParam* param, U32 size, char* tok, char* def);
S32 zParamGetFloatList(xModelAssetParam* param, U32 size, const char* tok, S32 count, F32* def,
                       F32* result);
void zEntGetShadowParams(xEnt* ent, xVec3* center, F32* radius, xEntShadow::radius_enum rtype);
S32 zParamGetVector(xModelAssetParam* param, U32 size, const char* tok, xVec3 result, xVec3*);
S32 zParamGetVector(xModelAssetParam* param, U32 size, char* tok, xVec3 result, xVec3*);
S32 zParamGetFloatList(xModelAssetParam* param, U32 size, char* tok, S32 count, F32* def,
                       F32* result);
S32 zParamGetFloatList(xModelAssetParam* param, U32 size, char* tok, S32 count, F32* def,
                       F32* result);
F32 zParamGetFloat(xModelAssetParam* param, U32 size, const char* tok, F32 def);
F32 zParamGetFloat(xModelAssetParam* param, U32 size, char* tok, F32 def);
S32 zParamGetInt(xModelAssetParam* param, U32 size, const char* tok, S32 def);
S32 zParamGetInt(xModelAssetParam* param, U32 size, char* tok, S32 def);
xModelAssetParam* zEntGetModelParams(U32 assetID, U32* size);
void zEntAnimEvent_AutoAnim(zEnt* ent, U32 animEvent, const F32* animParam);
xAnimTable* xEnt_AnimTable_AutoEventSmall();
void zEntAnimEvent(zEnt* ent, U32 animEvent, const F32* animParam);
void zEntParseModelInfo(xEnt* ent, U32 assetID);
xModelInstance* zEntRecurseModelInfo(void* info, xEnt* ent);
void zEntEventAllOfType(U32 toEvent, U32 type);
void zEntEventAll(xBase* from, U32 fromEvent, U32 toEvent, F32* toParam);
void zEntUpdate(zEnt* ent, zScene* scene, F32 elapsedSec);
void zEntReset(zEnt* ent);
void zEntLoad(zEnt* ent, xSerial* s);
void zEntSave(zEnt* ent, xSerial* s);
void zEntSetup(zEnt* ent);
void zEntInit(zEnt* ent, xEntAsset* asset, U32 type);

// Weak functions retail emitted into zEnt.o. The name says x-layer, but the
// body is in zEnt.cpp, so this header is where the declaration belongs.
// (xEntGetFrame used to be re-declared here as well; xEnt.h already had it.)
WEAK void xModelAnimCollStop(xModelInstance& m);
// Retail inlined this into some callers and not others, so neither choice is
// right for every TU. Callers that expand it (zFX, zEntTeleportBox) gain 12
// exact functions between them. Two TUs must NOT expand it and define
// XSNDPLAY3D_OUT_OF_LINE before including: zEnt.cpp, which defines the body
// below at a position its object layout depends on, and zEntDestructObj.cpp,
// where expanding it adds a .sdata2 literal that shifts the unit's pool and
// costs four functions. Both are Matching units, so both must stay exact.
#ifdef XSNDPLAY3D_OUT_OF_LINE
void xSndPlay3D(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, const xVec3* pos, F32 radius,
                sound_category category, F32 delay);
#else
inline void xSndPlay3D(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, const xVec3* pos,
                       F32 radius, sound_category category, F32 delay)
{
    xSndPlay3D(id, vol, pitch, priority, flags, pos, radius / 4.0f, radius, category, delay);
}
#endif

#endif
