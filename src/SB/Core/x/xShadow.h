#ifndef XSHADOW_H
#define XSHADOW_H

#include "xEnt.h"

#include <rwcore.h>
#include <rpworld.h>

struct xShadowPoly
{
    xVec3 vert[3];
    xVec3 norm;
};

struct xShadowCache
{
    xVec3 pos;
    F32 radius;
    U32 entCount;
    U32 polyCount;
    F32 polyRayDepth[5];
    U16 castOnEnt;
    U16 castOnPoly;
    xEnt* ent[16];
    xShadowPoly poly[256];
};

struct xShadowMgr
{
    xEnt* ent;
    xShadowCache* cache;
    int priority;
    int cacheReady;
};

void xShadowInit();
void xShadow_ListAdd(xEnt* ent);
void xShadowSetWorld(RpWorld* world);
void xShadowSetLight(xVec3* target_pos, xVec3* in_vec, F32 dst_cast);
void xShadowManager_Init(S32 numEnts);
void xShadowManager_Reset();
void xShadowManager_Remove(xEnt* ent);
void xShadowManager_Render();
void xShadowRender(xEnt* ent, F32 max_dist);
void xShadowRenderWorld(xVec3* center, F32 radius, F32 max_dist);
extern F32 gShadowObjectRadius;
// Retail returns U32 here: the call sites compare with cmplwi.
U32 xShadowReceiveShadowSetup(xEnt* ent);
void xShadowReceiveShadow(xEnt* ent, F32 shadowFactor, S32 shadowMode,
                          RwMatrixTag* shadowMat, RwRaster* shadowRast);
void xShadowVertical_FillCache(xShadowCache* cache, xVec3* pos, F32 r, F32 depth,
                               F32 minNormY);
// Defined in xShadow.cpp; xCM.cpp draws the fade quad with it too.
int Im2DRenderQuad(float x1, float y1, float x2, float y2, float z, float recipCamZ,
                   float uvOffset);
void xShadowVertical_DrawCache(xShadowCache* cache, F32 shadowFactor, F32 fadeDist,
                               S32 shadowMode, RwMatrixTag* shadowMat,
                               RwRaster* shadowRast);

#endif
