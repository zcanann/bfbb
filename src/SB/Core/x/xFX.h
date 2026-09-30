#ifndef XFX_H
#define XFX_H

#include "xMath3.h"
#include "containers.h"
#include "iColor.h"
#include "iDraw.h"
#include "iModel.h"

#include <rwcore.h>
#include <rpworld.h>

class xModelInstance;

struct xFXRing
{
    U32 texture;
    F32 lifetime;
    xVec3 pos;
    F32 time; // 0x14
    F32 ring_radius;
    F32 ring_radius_delta; // 0x1c
    F32 ring_tilt;
    F32 ring_tilt_delta; // 0x24?
    F32 ring_height; // 0x28
    F32 ring_height_delta;
    iColor_tag ring_color;
    U16 ring_segs;
    U8 u_repeat;
    U8 v_repeat;
    xFXRing** parent;
};

struct xFXRibbon
{
    struct config
    {
        F32 life_time;
        U32 blend_src;
        U32 blend_dst;
        F32 pivot;
    };

    struct joint_data
    {
        U32 flags;
        U32 born;
        xVec3 loc;
        xVec3 norm;
        F32 orient;
        F32 scale;
        F32 alpha;
    };

    struct curve_node
    {
        F32 time;
        iColor_tag color;
        F32 scale;
    };

    static tier_queue_allocator joint_alloc;

    config cfg;
    bool activated;
    RwRaster* raster;
    tier_queue<joint_data> joints;
    curve_node* curve;
    U32 curve_size;
    U32 curve_index;
    F32 ilife;
    U32 mtime;
    U32 mlife;

    bool visible() const;

    bool need_update() const;

    S32 render_compare(const xFXRibbon& other) const;

    F32 get_age(const joint_data& joint) const;

    void update(F32 dt);

    bool debug_need_update() const;

    void clear();

    void init(const char*, const char*);
    void init(S32, const char*);
    void set_texture(const char* name);
    void set_texture(U32);
    void set_texture(RwTexture* texture);
    void set_curve(const curve_node* curve, size_t size);
    // Used by the target but declared nowhere; signatures decoded from the
    // mangled names. The bodies are still to be written in xFX.cpp.
    void get_normal(xVec3&, const xVec3&, F32);
    void refresh_joint(joint_data&, const tier_queue<joint_data>::iterator&);
    void eval_joint(const joint_data&, iColor_tag&, F32&);
    void render_strip(RxObjSpace3DVertex*, tier_queue<joint_data>::iterator, u32);
    void refresh_config();
    void set_default_config();
    void update_curve_tweaks();
    void debug_init(const char*, const char*);
    void debug_update_curve();
    void debug_update(F32);
    void insert(const xVec3&, const xVec3&, F32, F32, unsigned int);
    void insert(const xVec3&, F32, F32, F32, U32);
    void activate();
    void deactivate();
    void start_render();
    void render();
    void set_raster(RwRaster*);
};

// Specialized in xFX.cpp. The declaration has to precede the first
// instantiation, which is xFXRibbon::clear() below.
template <> void tier_queue<xFXRibbon::joint_data>::clear();

inline void xFXRibbon::clear()
{
    joints.clear();
}

struct xFXStreakElem
{
    U32 flag;
    xVec3 p[2];
    F32 a;
};

struct xFXStreak
{
    U32 flags;
    F32 frequency;
    F32 alphaFadeRate;
    F32 alphaStart;
    F32 elapsed;
    F32 lifetime;
    U32 head;
    iColor_tag color_a;
    iColor_tag color_b;
    RwTexture* texturePtr;
    RwRaster* textureRasterPtr;
    xFXStreakElem elem[50];
};

struct xFXShineElem
{
    U32 flag;
    xVec3 p;
    xVec3 vel;
    F32 lifetime;
    F32 a;
    iColor_tag cola;
    iColor_tag colb;
};

struct xFXShine
{
    U32 flags;
    xVec3* ppos;
    xVec3 pos;
    F32 spd;
    F32 width;
    F32 frequency;
    F32 elapsed;
    F32 lifetimeElemMax;
    F32 lifetimeMax;
    F32 lifetime;
    F32 rotateSpeed;
    F32 rotateZ;
    iColor_tag color_a;
    iColor_tag color_b;
    RwTexture* texturePtr;
    RwRaster* textureRasterPtr;
    xFXShineElem elem[100];
};

struct xFXBubbleParams
{
    U32 pass1 : 1;
    U32 pass2 : 1;
    U32 pass3 : 1;
    U32 padding : 5;
    U8 pass1_alpha;
    U8 pass2_alpha;
    U8 pass3_alpha;
    U32 pass1_fbmsk;
    U32 fresnel_map;
    F32 fresnel_map_coeff;
    U32 env_map;
    F32 env_map_coeff;
};

#define RING_COUNT 8

extern xFXStreak sStreakList[10];
extern xFXShine sShineList[2];
extern xFXRing ringlist[RING_COUNT];

extern RpAtomic* (*gAtomicRenderCallBack)(RpAtomic*);

void xFXInit();
U32 xFXShineStart(const xVec3*, F32, F32, F32, F32, U32, const iColor_tag*, const iColor_tag*, F32,
                  S32);
void xFXStartup();
void xFXShutdown();
void xFXInit();
xFXRing* xFXRingCreate(const xVec3* pos, const xFXRing* params);
void xFXRingRender();
void xFX_SceneEnter(RpWorld* world);
void xFX_SceneExit(RpWorld* world);
void xFXUpdate(F32 dt);
RpAtomic* AtomicDisableMatFX(RpAtomic* atomic);
void xFXPreAllocMatFX(RpClump* clump);

RpAtomic* xFXBubbleRender(RpAtomic* atomic);
RpAtomic* xFXShinyRender(RpAtomic* atomic);
RpMaterial* MaterialSetEnvMap2(RpMaterial* material, void* data);

void xFXanimUV2PSetTexture(RwTexture* texture);
void xFXanimUVSetTranslation(const xVec3* trans);
void xFXanimUV2PSetTranslation(const xVec3* trans);
void xFXanimUVSetScale(const xVec3* scale);
void xFXanimUV2PSetScale(const xVec3* scale);
void xFXanimUVSetAngle(F32 angle);
void xFXanimUV2PSetAngle(F32 angle);
RpAtomic* xFXanimUVAtomicSetup(RpAtomic* atomic);
U32 xFXanimUVCreate();
void xFXFireworksInit(const char* fireworksTrailEmitter, const char* fireworksEmitter1,
                      const char* fireworksEmitter2, const char* fireworksSound,
                      const char* fireworksLaunchSound);
void xFXFireworksLaunch(F32 countdownTime, const xVec3* pos, F32 fuelTime);
void xFXFireworksUpdate(F32 dt);
void xFXStreakInit();
void xFXStreakUpdate(F32 dt);
void xFXStreakUpdate(U32 streakID, const xVec3*, const xVec3*);
void xFXStreakRender();
U32 xFXStreakStart(F32 frequency, F32 alphaFadeRate, F32 alphaStart, U32 textureID,
                   const iColor_tag* edge_a, const iColor_tag* edge_b, S32 taper);
void xFXStreakStop(U32);
void xFXShineInit();
void xFXShineUpdate(F32 dt);
void xFXShineRender();
RpAtomic* xFXAtomicEnvMapSetup(RpAtomic* atomic, U32 envmapID, F32 shininess);
void xFXRibbonSceneEnter();
void xFXRibbonUpdate(F32 dt);
void xFXRibbonRender();
void xFXAuraSetup();
void xFXAuraInit();
void xFXAuraUpdate(F32 dt);
void xFXAuraRender();
void xFXRenderProximityFade(const xModelInstance& model, F32 near_dist, F32 far_dist);
void xFXSceneInit();
void xFXSceneSetup();
void xFXSceneReset();
void xFXScenePrepare();
void xFXSceneFinish();

#endif
