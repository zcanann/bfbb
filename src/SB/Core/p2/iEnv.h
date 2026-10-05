#ifndef IENV_H
#define IENV_H

#include "types.h"
#include <rpworld.h>

struct xJSPHeader;
struct xEnvAsset;

// Retail iEnv is 48 bytes and begins at offset 16 inside xEnv.
struct iEnv
{
    RpWorld* world;
    RpWorld* collision;
    RpWorld* fx;
    RpWorld* camera;
    xJSPHeader* jsp;
    RpLight* light[2];
    RwFrame* light_frame[2];
    S32 memlvl;
} __attribute__((aligned(16)));

void iEnvLoad(iEnv* env, const void* data, U32 datasize, S32 dataType);
void iEnvFree(iEnv* env);
void iEnvDefaultLighting(iEnv* env);
void iEnvLightingBasics(iEnv* env, xEnvAsset* asset);
void iEnvRender(iEnv* env);
void iEnvEndRenderFX(iEnv* env);

inline RwBBox* iEnvGetBBox(iEnv* env)
{
    return &env->world->boundingBox;
}

#endif
