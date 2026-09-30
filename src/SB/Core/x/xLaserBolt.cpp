#include "xLaserBolt.h"
#include "xMathInlines.h"
#include "xString.h"
#include "xstransvc.h"
#include "iParMgr.h"
#include "zEvent.h"
#include "zBase.h"
#include "zNPCTypeCommon.h"
#include "zEntDestructObj.h"
#include "containers.h"

#include <types.h>

void xLaserBoltEmitter::init(u32 max_bolts, const char* texture_name)
{
    this->bolt_raster = NULL;
    memset(fxsize, 0, sizeof(fxsize));
    this->bolts.init(max_bolts);
    debug_init(texture_name);
}


void xLaserBoltEmitter::set_texture(const char* name)
{
    set_texture(xStrHash(name));
}

void xLaserBoltEmitter::set_texture(U32 aid)
{
    set_texture((RwTexture*)xSTFindAsset(aid, NULL));
}

void xLaserBoltEmitter::set_texture(RwTexture* tex)
{
    if (tex == NULL)
    {
        this->bolt_raster = NULL;
    }
    else
    {
        set_texture(tex->raster);
    }
}

void xLaserBoltEmitter::set_texture(RwRaster* raster)
{
    this->bolt_raster = raster;
}

void xLaserBoltEmitter::reset()
{
    static_queue<bolt>::iterator it = this->bolts.begin();
    while (it != this->bolts.end())
    {
        bolt& b = *it;

        effect_data* itfx = this->fx[6];
        effect_data* endfx = itfx + this->fxsize[6];
        while (itfx != endfx)
        {
            emit_fx(*itfx, b, b.hit_dist, b.hit_dist, (1.0f / 60.0f));
            itfx++;
        }
        
        ++it;
    }

    this->bolts.clear();
    start_collide = 0;

    for (S32 i = FX_WHEN_LAUNCH; i < MAX_FX_WHEN; i++)
    {
        reset_fx((fx_when_enum)i);
    }
}

void xLaserBoltEmitter::refresh_config()
{
    F32 alpha;
    if (this->cfg.kill_dist <= this->cfg.fade_dist)
    {
        alpha = 0.0f;
    }
    else
    {
        alpha = 255.0f / (this->cfg.kill_dist - this->cfg.fade_dist);
    }
    this->ialpha = alpha;
}

void xLaserBoltEmitter::emit(const xVec3& loc, const xVec3& dir)
{
    if (this->bolts.full())
    {
        this->bolts.pop_back();
    }

    bolt& b = this->bolts.push_front();
    b.origin = b.loc = loc;
    b.dir = dir;
    b.dist = b.prev_dist = b.prev_check_dist = 0.0f;
    b.hit_dist = cfg.kill_dist;
    b.hit_ent = NULL;
    b.emitted = 0.0f;
    b.context = NULL;

    if (cfg.rand_ang > 0.0f)
    {
        perturb_dir(b.dir, cfg.rand_ang);
    }

    if (cfg.hit_interval > 0)
    {
        pre_collide(b);
    }

    start_collide++;

    effect_data* fx = this->fx[0];
    effect_data* fx_end = fx + this->fxsize[0];
    while (fx != fx_end)
    {
        emit_fx(*fx, b, 0.0f, 0.0f, (1.0f / 60.0f));
        fx++;
    }
}

void xLaserBoltEmitter::update(F32 dt)
{
    debug_update(dt);

    S32 ci = this->cfg.hit_interval > 0 ? start_collide % this->cfg.hit_interval : -1000000000;

    static_queue<bolt>::iterator it = this->bolts.begin();
    while (it != this->bolts.end())
    {
        bolt& b = *it;

        U8 collided = b.dist >= b.hit_dist;
        F32 prev_dist = b.dist;

        update(b, dt);

        b.loc = b.origin + b.dir * (b.dist >= b.hit_dist ? b.hit_dist : b.dist);

        if (b.dist >= this->cfg.kill_dist)
        {
            effect_data* itfx = this->fx[6];
            effect_data* endfx = itfx + this->fxsize[6];
            while (itfx != endfx)
            {
                emit_fx(*itfx, b, b.hit_dist, b.hit_dist, dt);
                itfx++;
            }

            this->bolts.erase(it, this->bolts.end());
            break;
        }

        update_fx(b, prev_dist, dt);

        if ((b.dist >= b.hit_dist) && (b.hit_ent != NULL))
        {
            apply_damage(b);
            b.hit_ent = NULL;
        }

        ci++;
        if (ci >= this->cfg.hit_interval)
        {
            ci -= this->cfg.hit_interval;

            if ((b.dist < b.hit_dist) && (b.dist >= this->cfg.safe_dist))
            {
                collide_update(b);
            }
        }

        if (!collided && (b.dist >= b.hit_dist))
        {
            effect_data* itfx = this->fx[1];
            effect_data* endfx = itfx + this->fxsize[1];
            while (itfx != endfx)
            {
                emit_fx(*itfx, b, b.hit_dist, b.hit_dist, dt);
                itfx++;
            }
        }

        ++it;
    }

    start_collide++;
}

void xLaserBoltEmitter::render()
{
    debug_render();

    S32 size;
    RxObjSpace3DVertex* verts = get_vert_buffer(size);
    RxObjSpace3DVertex* v = verts;

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, bolt_raster);

    static_queue<bolt>::iterator it = bolts.begin();
    while (it != bolts.end())
    {
        S32 used = v - verts;
        if (size - used < 6)
        {
            flush_verts(verts, used);
            v = verts;
        }

        v = render(*it, v);
        ++it;
    }

    if (v != verts)
    {
        flush_verts(verts, v - verts);
    }
}

void xLaserBoltEmitter::attach_effects(fx_when_enum when, effect_data* fx, size_t fxsize)
{
    this->fx[when] = fx;
    this->fxsize[when] = fxsize;
    reset_fx(when);
    debug_refresh_effects(when);
}

void xLaserBoltEmitter::pre_collide(bolt& b)
{
    xScene& scene = *globals.sceneCur;

    xRay3 ray;
    ray.origin = b.origin;
    ray.dir = b.dir;
    ray.min_t = this->cfg.safe_dist;
    ray.max_t = b.hit_dist;
    ray.flags = 0x800;

    xCollis coll;
    coll.flags = 0x300;
    xRayHitsSceneFlags(&scene, &ray, &coll, XENT_COLLTYPE_PLYR, 0x22);

    if (coll.flags & 0x1)
    {
        b.hit_dist = coll.dist;
        b.hit_norm = coll.norm;
        b.hit_ent = (xEnt*)coll.optr;
    }

    log_collide_statics(coll.flags & 0x1);
}

void xLaserBoltEmitter::collide_update(bolt& b)
{
    xScene& scene = *globals.sceneCur;

    xRay3 ray;
    ray.origin = b.origin;
    ray.dir = b.dir;
    ray.min_t = b.prev_check_dist - this->cfg.length;
    ray.max_t = b.dist;
    ray.flags = 0xC00;

    if (ray.min_t < this->cfg.safe_dist)
    {
        ray.min_t = this->cfg.safe_dist;
    }

    xCollis player_coll;
    player_coll.flags = 0x300;
    xRayHitsBound(&ray, &globals.player.ent.bound, &player_coll);

    if (player_coll.flags & 0x1)
    {
        ray.max_t = player_coll.dist;
    }

    xCollis scene_coll;
    scene_coll.flags = 0x300;
    xRayHitsSceneFlags(&scene, &ray, &scene_coll, XENT_COLLTYPE_PLYR, 0xC);

    if (scene_coll.flags & 0x1)
    {
        b.hit_dist = scene_coll.dist;
        b.hit_norm = scene_coll.norm;
        b.hit_ent = (xEnt*)scene_coll.optr;
    }
    else if (player_coll.flags & 0x1)
    {
        b.hit_dist = player_coll.dist;
        b.hit_norm = player_coll.norm;
        b.hit_ent = &globals.player.ent;
    }

    b.prev_check_dist = b.dist;

    log_collide_dynamics(scene_coll.flags & 0x1 || player_coll.flags & 0x1);
}

RxObjSpace3DVertex* xLaserBoltEmitter::render(bolt& b, RxObjSpace3DVertex *vert) 
{       
    F32 dist0 = b.prev_dist - this->cfg.length; 
    if (dist0 < 0.0f)
    {
        dist0 = 0.0f;
    }

    // Both arms assigning is deliberate: the retail codegen has an empty
    // "then" block here (bne/b pair), which only this shape reproduces.
    F32 dist1 = b.dist;
    if (dist1 <= b.hit_dist)
    {
        dist1 = b.dist;
    }
    else
    {
        dist1 = b.hit_dist;
    }

    if (dist0 >= dist1)
    {
        return vert;
    }

    xVec3 loc0 = b.origin + b.dir * dist0;
    xVec3 loc1 = b.origin + b.dir * dist1;
    xMat4x3& cam_mat = globals.camera.mat;
    xVec3 dir = (loc1 - loc0).normal();
    xVec3 right = dir.cross(cam_mat.at);

    F32 len2 = right.length2();
    if (xfeq0(len2))
    {
        right.assign(1.0f, 0.0f, 0.0f);
    }
    else
    {
        right *= 1.0f / xsqrt(len2);
    }

    xVec3 half_right = right * (0.5f * this->cfg.radius);

    U8 alpha;
    if (b.dist <= this->cfg.fade_dist)
    {
        alpha = 255;
    }
    else
    {
        alpha = 255.5f - this->ialpha * (b.dist - this->cfg.fade_dist);
    }

    set_bolt_verts(vert, loc0, loc1, alpha, half_right);
    return vert + 6;
}

RxObjSpace3DVertex* xLaserBoltEmitter::get_vert_buffer(S32& dat)
{
    dat = (U32)0x1e0;
    return gRenderBuffer.m_vertex;
}

void xLaserBoltEmitter::apply_damage(xLaserBoltEmitter::bolt& b)
{
    if (b.hit_ent->baseType == eBaseTypeNPC)
    {
        zNPCCommon* npc = (zNPCCommon*)b.hit_ent;
        npc->Damage(DMGTYP_PROJECTILE, NULL, &b.dir);
    }
    else if (b.hit_ent->baseType == eBaseTypeDestructObj)
    {
        zEntDestructObj_Hit((zEntDestructObj*)b.hit_ent, 0x40000);
    }
    else
    {
        zEntEvent(b.hit_ent, eEventHit, cfg.damage, 0.0f, 0.0f, 0.0f);
    }
}

void xLaserBoltEmitter::reset_fx(fx_when_enum when)
{
    effect_data* cur_fx = this->fx[when];
    effect_data* fx_end = cur_fx + this->fxsize[when];
    while (cur_fx != fx_end)
    {        
        cur_fx->irate = cur_fx->rate > 0.0f ? 1.0f / cur_fx->rate : 0.0f;
        cur_fx++;
    }
}

void xLaserBoltEmitter::update_fx(bolt& b, F32 prev_dist, F32 dt)
{
    F32 tail_dist = b.dist - this->cfg.length;
    if (b.dist < this->cfg.length)
    {
        effect_data* itfx = this->fx[2];
        effect_data* endfx = itfx + this->fxsize[2];
        while (itfx != endfx)
        {
            emit_fx(*itfx, b, 0.0f, 0.0f, dt);
            itfx++;
        }
    }
    else if (tail_dist < b.hit_dist)
    {
        F32 from_dist = prev_dist - this->cfg.length;
        
        effect_data* itfx = this->fx[5];
        effect_data* endfx = itfx + this->fxsize[5];
        if (from_dist < 0.0f)
        {
            from_dist = 0.0f;
        }

        while (itfx != endfx)
        {
            emit_fx(*itfx, b, from_dist, tail_dist, dt);
            itfx++;
        }
    }

    if (b.dist < b.hit_dist)
    {
        effect_data* itfx = this->fx[4];
        effect_data* endfx = itfx + this->fxsize[4];
        while (itfx != endfx)
        {
            emit_fx(*itfx, b, prev_dist, b.dist, dt);
            itfx++;
        }
    }
    else if (tail_dist < b.hit_dist)
    {
        effect_data* itfx = this->fx[3];
        effect_data* endfx = itfx + this->fxsize[3];
        while (itfx != endfx)
        {
            emit_fx(*itfx, b, b.hit_dist, b.hit_dist, dt);
            itfx++;
        }
    }
}

void xLaserBoltEmitter::emit_particle(effect_data& effect, bolt& b, F32 from_dist, F32 to_dist, F32 dt)
{
    xParEmitter& pe = *effect.par;
    xParEmitterAsset& pea = *pe.tasset;
    F32 velmag = pea.vel.y;
    
    switch(effect.orient)
    {
    case FX_ORIENT_PATH:
        pea.vel = b.dir * velmag;
        break;
    case FX_ORIENT_IPATH:
        pea.vel = b.dir * -velmag;
        break;
    case FX_ORIENT_HIT_NORM:
        pea.vel = b.hit_norm * velmag;
        break;
    case FX_ORIENT_HIT_REFLECT:
        break;
    }

    if (pea.emit_type == eParEmitterLine)
    {
        pea.e_line.pos1 = b.origin + b.dir * from_dist;
        pea.e_line.pos2 = b.origin + b.dir * to_dist;
        xParEmitterEmit(&pe, dt);
    }
    else
    {
        xVec3 oldloc = pea.pos;
        pea.pos += b.origin + b.dir * to_dist;

        xParEmitterEmit(&pe, dt);
        pea.pos = oldloc;
    }

    pea.vel.y = velmag;
}

void xLaserBoltEmitter::emit_decal(effect_data& effect, bolt& b, F32 from_dist, F32 to_dist, F32 dt)
{
    xMat4x3 mat;

    switch (effect.orient)
    {
    case FX_ORIENT_DEFAULT:
    case FX_ORIENT_PATH:
    case FX_ORIENT_IPATH:
        xMat3x3LookVec3(mat, b.dir);
        break;
    case FX_ORIENT_HIT_NORM:
        xMat3x3LookVec3(mat, b.hit_norm);
        break;
    case FX_ORIENT_HIT_REFLECT:
        break;
    }

    mat.pos = b.origin + b.dir * to_dist;
    effect.decal->emit(mat, -1);
}

void xLaserBoltEmitter::emit_decal_dist(effect_data& effect, bolt& b, F32 from_dist, F32 to_dist, F32 dt)
{
    F32 start_dist = (1.0f - b.emitted) * effect.irate;
    start_dist = from_dist + start_dist;
    b.emitted += effect.rate * (to_dist - from_dist);

    S32 total = b.emitted;
    b.emitted -= total;

    if (total <= 0)
    {
        return;
    }

    xMat4x3 mat;
    switch (effect.orient)
    {
    case FX_ORIENT_DEFAULT:
    case FX_ORIENT_PATH:
    case FX_ORIENT_IPATH:
        xMat3x3LookVec3(mat, b.dir);
        break;
    case FX_ORIENT_HIT_NORM:
        xMat3x3LookVec3(mat, b.hit_norm);
        break;
    case FX_ORIENT_HIT_REFLECT:
        break;
    }

    xVec3 dloc = b.dir * effect.irate;
    mat.pos = b.origin + b.dir * start_dist;
    for (S32 i = 0; i < total; i++, mat.pos += dloc)
    {
        if (((xDecalEmitter*)effect.par)->full())
        {
            break;
        }

        ((xDecalEmitter*)effect.par)->emit(mat, -1);
    }
}
