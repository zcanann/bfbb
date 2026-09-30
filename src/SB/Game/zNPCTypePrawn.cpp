#include "zNPCTypePrawn.h"

#include "rwcore.h"
#include "xDebug.h"

#include "xMemMgr.h"
#include "xGroup.h"
#include "xMath.h"
#include "xMathInlines.h"
#include "zEntCruiseBubble.h"
#include "zNPCTypeCommon.h"
#include "zRenderState.h"
#include <types.h>
#include <stdio.h>
#include <string.h>

#define ANIM_Unknown 0 // 0x0
#define ANIM_Idle01 1 // 0x04
#define ANIM_Idle02 2 // 0x08
#define ANIM_Idle03 3 // 0xC
#define ANIM_Fidget01 4 //
#define ANIM_Fidget02 5
#define ANIM_Fidget03 6
#define ANIM_Taunt01 7 // 0x1c
#define ANIM_Attack01 8 //0x20
#define ANIM_Damage01 9 //0x24
#define ANIM_Damage02 10 //0x28
#define ANIM_Death01 11 //0x2c
#define ANIM_AttackWindup01 12 //0x30
#define ANIM_AttackLoop01 13 //0x34
#define ANIM_AttackEnd01 14 //0x38
#define ANIM_SpawnKids01 15 //0x3C
#define ANIM_Attack02Windup01 16
#define ANIM_Attack02Loop01 17
#define ANIM_Attack02End01 18
#define ANIM_LassoGrab01 19

U32 xSndPlay3DFade(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, const xVec3* pos,
                   F32 innerRadius, F32 outerRadius, sound_category category, F32 fade, F32 delay);
void xDebugAddTweak(const char*, xVec3*, const tweak_callback*, void*, U32);

namespace
{
    struct sound_asset_type
    {
        S32 category;
        const char* name;
        U32 priority;
        U32 mode;
    };

    const sound_asset_type sound_assets[14] = {
        { 0, "Prawn_hit", 0, 0 },          { 0, "Prawn_FF_hit", 0, 0 },
        { 1, "B101_SC_jump", 0, 0 },       { 1, "Prawn_FF_hit", 0, 0 },
        { 1, "fanfare", 0, 0 },            { 1, "sax", 0, 0 },
        { 1, "Mon_alert", 0, 0 },          { 2, "RP_whirr_loop", 0, 3 },
        { 2, "PR_attack_loop", 0, 3 },     { 2, "Prawn_Attack_loop", 0, 3 },
        { 2, "Crystals_loop", 0, 3 },      { 2, "RSB_foot_loop", 0, 3 },
        { 3, "PR_attack_loop", 0, 1 },     { 3, "Prawn_Attack_loop", 0, 1 },
    };

    const char* sound_asset_names[4][6];
    U32 sound_asset_ids[4][6];
    S32 sound_asset_names_size[4];
    sound_data_type sound_data[4];

    struct tweak_group
    {
        F32 turn_accel;
        F32 spawn_delay;
        F32 repel_radius;
        struct
        {
            range_type pattern;
        } safe;
        struct
        {
            range_type pattern;
            F32 state_delay; // 0x1c
            F32 transition_delay;
        } begin;
        struct
        {
            F32 delay[3];
            range_type pattern;
            F32 state_delay;
            F32 transition_delay;
            F32 exhaust_vel;
            F32 knock_back;
            struct
            {
                S32 amount[3];
                F32 arc;
                F32 delay;
                F32 accel;
                F32 max_vel;
            } sweep;
            fire_type fire;
        } beam;
        _class_7 aim_lane;
        _class_14 lane;
        _class_25 danger;
        sound_property sound[4];
        void* context;
        tweak_callback cb_sound;
        tweak_callback cb_sound_asset;

        void load(xModelAssetParam* p, U32 i);
        void register_tweaks(bool init, xModelAssetParam* ap, U32 apsiz, const char* c);
    }; // namespace tweak_group

    static tweak_group tweak;

    void init_sound()
    {
        memset(sound_asset_names_size, 0, sizeof(sound_asset_names_size));

        for (S32 i = 0; i < 14; i++)
        {
            if (sound_assets[i].name == NULL)
            {
                continue;
            }

            S32 cat = sound_assets[i].category;
            S32& total = sound_asset_names_size[cat];
            sound_asset_names[cat][total] = sound_assets[i].name;
            sound_asset_ids[cat][total] = i;
            total++;
        }

        memset(sound_data, 0, sizeof(sound_data));

        for (S32 i = 0; i < 4; i++)
        {
            sound_data[i].id = 0;
            sound_data[i].handle = 0;
        }
    }

    void reset_sound()
    {
        sound_data[0].handle = 0;
        sound_data[1].handle = 0;
        sound_data[2].handle = 0;
        sound_data[3].handle = 0;
    }

    U32 play_sound(S32 which, const xVec3* loc, F32 volume)
    {
        const sound_property& snd = tweak.sound[which];
        sound_data_type& data = sound_data[which];
        const sound_asset_type& asset = sound_assets[snd.asset];

        if ((asset.mode & 2) && data.handle != 0)
        {
            return data.handle;
        }

        if (asset.mode & 1)
        {
            data.handle =
                xSndPlay3DFade(data.id, volume * snd.volume, 1.0f, asset.priority, 0x800, loc,
                               snd.range_inner, snd.range_outer, SND_CAT_GAME, 0.0f, snd.delay);
        }
        else
        {
            data.handle = xSndPlay3D(data.id, volume * snd.volume, 1.0f, asset.priority, 0x800, loc,
                                     snd.range_inner, snd.range_outer, SND_CAT_GAME, snd.delay);
        }

        data.loc = loc;
        data.volume = volume;
        return data.handle;
    }

    void kill_sound(S32 which, U32 handle)
    {
        sound_data_type& data = sound_data[which];
        const sound_property& snd = tweak.sound[which];
        const sound_asset_type& asset = sound_assets[snd.asset];

        if (asset.mode & 1)
        {
            xSndStopFade(handle, snd.fade_time);
        }
        else
        {
            xSndStop(handle);
        }

        data.handle = 0;
    }

    void kill_sound(S32 which)
    {
        sound_data_type& data = sound_data[which];

        U32 handle = data.handle;
        if (handle == 0)
        {
            return;
        }

        const sound_property& snd = tweak.sound[which];
        const sound_asset_type& asset = sound_assets[snd.asset];
        if (asset.mode & 1)
        {
            xSndStopFade(handle, snd.fade_time);
        }
        else
        {
            xSndStop(handle);
        }

        data.handle = 0;
    }
} // namespace

void aqua_beam::load(const aqua_beam::config& c, U32 i)
{
    void* a = xSTFindAsset(i, 0);
    aqua_beam::load(c, *(RpAtomic*)a);
}

void aqua_beam::load(const aqua_beam::config& c, RpAtomic& a)
{
    ring.model_data = &a;
    cfg = c;
    ring.queue.reset();
}

void aqua_beam::reset()
{
    firing = 0;
    bool tempBvar;

    while (!(tempBvar = ring.queue.empty()))
    {
        aqua_beam::kill_ring();
    }
    ring_sounds = 0;
}

void aqua_beam::start()
{
    firing = 1;
    time = 0.0f;
    ring.emit_time = -1000000000.0f;
}

void aqua_beam::stop()
{
    firing = 0;
}

void aqua_beam::update(F32 dt)
{
    time += dt;
    if ((cfg.duration >= 0.0f) && (time >= cfg.duration))
    {
        firing = 0;
    }
    update_rings(dt);
}

void aqua_beam::render()
{
    fixed_queue<aqua_beam::ring_segment, 31>::iterator entry = ring.queue.begin();
    while (entry != ring.queue.end())
    {
        render_ring(*entry);
        ++entry;
    }
}

bool aqua_beam::hits_sphere(const xSphere& o) const
{
    xVec3 center = o.center - cfg.ring.hit_offset;

    fixed_queue<aqua_beam::ring_segment, 31>::iterator it = ring.queue.begin();

    // Declared and initialised in opposite orders: retail hoists all three out of the
    // loop, and this is the split that reproduces its register/emission order.
    F32 radius;
    F32 grow;
    F32 hit_radius;

    hit_radius = cfg.ring.hit_radius;
    grow = cfg.ring.grow;
    radius = o.r;

    while (it != ring.queue.end())
    {
        aqua_beam::ring_segment& ring = *it;
        F32 maxdist = hit_radius * (grow * ring.dist + 1.0f) + radius;
        xVec3 delta = center - *(xVec3*)&ring.model->Mat->pos;

        if (!(delta.length2() > maxdist * maxdist) && FABS(delta.dot(ring.mat.at)) < maxdist)
        {
            return true;
        }
        ++it;
    }
    return false;
}

void aqua_beam::update_rings(F32 dt)
{
    if (firing || !ring.queue.empty())
    {
        if (firing && time >= ring.emit_time + cfg.ring.emit_delay)
        {
            ring.emit_time += cfg.ring.emit_delay;
            if (time >= ring.emit_time + cfg.ring.emit_delay)
            {
                ring.emit_time = time;
            }
            emit_ring();
        }

        fixed_queue<aqua_beam::ring_segment, 31>::iterator it = ring.queue.begin();
        while (it != ring.queue.end())
        {
            update_ring(it, dt);
            ++it;
        }

        while (!ring.queue.empty() && FABS(ring.queue.back().dist) >= cfg.ring.kill_dist)
        {
            kill_ring();
        }
    }
}

void aqua_beam::emit_ring()
{
    if (ring.queue.full())
    {
        kill_ring();
    }
    ring.queue.push_front();

    aqua_beam::ring_segment& segment = ring.queue.front();
    segment.origin = loc;
    segment.dist = 0.0f;
    segment.vel = cfg.ring.vel;
    segment.mat.right.assign(dir.z, 0.0f, -dir.x).normalize();
    segment.mat.up = dir.cross(segment.mat.right);
    segment.mat.at = dir;
    segment.mat.pos = segment.origin;
    segment.model = xModelInstanceAlloc(ring.model_data, NULL, 0, 0, NULL);

    ring_sounds++;
    if (ring_sounds % cfg.sound_interval == 0)
    {
        ring_sounds = 0;
        segment.sound_handle = play_sound(3, &segment.mat.pos, 1.0f);
    }
    else
    {
        segment.sound_handle = 0;
    }
}

void aqua_beam::kill_ring()
{
    aqua_beam::ring_segment& back = ring.queue.back();
    xModelInstanceFree(back.model);

    if (back.sound_handle != 0)
    {
        kill_sound(3, back.sound_handle);
    }
    ring.queue.pop_back();
}

void aqua_beam::update_ring(fixed_queue<aqua_beam::ring_segment, 31>::iterator entry, F32 dt)
{
    aqua_beam::ring_segment& segment = *entry;

    segment.vel = cfg.ring.accel * dt + segment.vel;
    segment.dist += dt * (0.5f * cfg.ring.accel * dt) + segment.vel * dt;
    segment.mat.pos = segment.origin + segment.mat.at * segment.dist;
}

void aqua_beam::render_ring(aqua_beam::ring_segment& r)
{
    xMat4x3* mat = (xMat4x3*)r.model->Mat;
    F32 size = r.dist * cfg.ring.grow + cfg.ring.size;

    *mat = r.mat;
    mat->right *= size;
    mat->up *= size;
    mat->at *= size;

    xModelInstance* model = r.model;
    model->Alpha = cfg.ring.alpha;

    F32 fade_dist = FABS(r.dist) - cfg.ring.fade_dist;
    if (fade_dist > 0.0f)
    {
        F32 max_fade_dist = cfg.ring.kill_dist - cfg.ring.fade_dist;
        if (fade_dist >= max_fade_dist)
        {
            model->Alpha = 0.0f;
        }
        else
        {
            model->Alpha *= 1.0f - fade_dist / max_fade_dist;
        }
    }

    xModelRender(r.model);
}

xAnimTable* ZNPC_AnimTable_Prawn()
{
    // clang-format off
    S32 ourAnims[] = {
        ANIM_Idle01,
        ANIM_Fidget01,
        ANIM_Fidget02,
        ANIM_Taunt01,
        ANIM_AttackWindup01,
        ANIM_AttackLoop01,
        ANIM_AttackLoop01,
        ANIM_AttackEnd01,
        ANIM_Damage01,
        ANIM_Damage02 NPCC_ANIM_LIST_END

    };
    // clang-format on
    xAnimTable* table = xAnimTableNew("zNPCPrawn", NULL, 0);

    xAnimTableNewState(table, g_strz_subbanim[ANIM_Idle01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Fidget01], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Fidget02], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Taunt01], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackWindup01], 0x20, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackLoop01], 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_AttackEnd01], 0x20, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Damage01], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_subbanim[ANIM_Damage02], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_subbanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackWindup01],
                            g_strz_subbanim[ANIM_AttackLoop01], 0, 0, 0x10, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackLoop01],
                            g_strz_subbanim[ANIM_AttackEnd01], 0, 0, 0, 0, 0, 0, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_subbanim[ANIM_AttackEnd01], g_strz_subbanim[ANIM_Idle01],
                            0, 0, 0x10, 0, 0, 0, 0, 0, 0.1f, 0);

    return table;
}

zNPCPrawn::zNPCPrawn(S32 myType) : zNPCSubBoss(myType)
{
    this->fighting = 0;
}

namespace
{
    struct television
    {
        RwCamera* cam;
        RwRaster* raster;
        RwRaster* bgraster;
        RpWorld* world;
        RwTexture* texture;
        U32 vert_buffer_used;
        RwRGBA bgcolor;
        F32 rcz;
        F32 w;
        F32 h;

        bool create(S32, S32);
        void destroy();
        void set_background(iColor_tag);
        void set_model_texture(xModelInstance&);
        void update(xModelInstance&, xLightKit*);
        void render_static();
        void render_background();
        void set_vert(rwGameCube2DVertex&, F32, F32, F32, F32);
        void move(const xVec3&, const xVec3&);
    };

    static television closeup;

    xBinaryCamera boss_cam = { { { 6.0f, 3.0f, 2.0f },
                                 { 0.2f, 2.2f, -1.0f },
                                 { 1.0f, 0.2f, 1.5f },
                                 10.0f,
                                 10.0f,
                                 10.0f,
                                 10.0f,
                                 50.0f,
                                 -3.1415927f } };

    zParEmitter* exhaust_emitter;
    xParEmitterCustomSettings exhaust_emitter_settings;

    S32 load_patterns(xModelAssetParam*, U32, const char*, zNPCPrawn::range_type*, S32);
    void set_yaw_matrix(xMat3x3&, F32);
    void mulat(xVec3&, const xMat3x3&, const xMat3x3&);
} // namespace

void zNPCPrawn::Init(xEntAsset* asset)
{
    boss_cam.init();
    init_sound();
    zNPCCommon::Init(asset);
    memset(&this->flag, 0, 1);
    this->flg_move = 1;
    this->flg_vuln = 0x100001;
    this->chkby = 0x10;
    this->penby = 0x10;
    this->beam.load(tweak.beam.fire, xStrHash("glow_ring_add.dff"));
    closeup.create(0x100, 0x100);
}

namespace
{
    bool television::create(S32 width, S32 height)
    {
        cam = NULL;
        bgraster = NULL;
        raster = NULL;
        world = NULL;
        memset(&bgcolor, 0x0, sizeof(bgcolor));

        w = (float)width;
        h = (float)height;
        cam = RwCameraCreate();
        if (cam == NULL)
        {
            destroy();
            return FALSE;
        }

        RwBBox worldBbox;
        worldBbox.sup.z = 100000.f;
        worldBbox.sup.y = 100000.f;
        worldBbox.sup.x = 100000.f;

        worldBbox.inf.z = -100000.f;
        worldBbox.inf.y = -100000.f;
        worldBbox.inf.x = -100000.f;

        world = RpWorldCreate(&worldBbox);
        if (world == NULL)
        {
            destroy();
            return FALSE;
        }

        RpWorldAddCamera(world, cam);
        _rwObjectHasFrameSetFrame(cam, RwFrameCreate());

        xVec2 windowSize = { 1.0f, 1.0f };
        RwCameraSetViewWindow(cam, (const RwV2d*)&windowSize);
        RwCameraSetProjection(cam, rwPERSPECTIVE);

        if (cam->object.object.parent == NULL)
        {
            destroy();
            return FALSE;
        }

        raster =
            RwRasterCreate(width, height, 32, rwRASTERGAMMACORRECTED | rwRASTERPIXELLOCKEDWRITE);
        if (raster == NULL)
        {
            destroy();
            return FALSE;
        }
        cam->frameBuffer = raster;
        if (cam == NULL)
        {
            destroy();
            return FALSE;
        }
        texture = RwTextureCreate(raster);
        if (texture == NULL)
        {
            destroy();
            return FALSE;
        }
        texture->filterAddressing = (texture->filterAddressing & 0xFFFFFF00) | rwFILTERLINEAR;

        RwCameraSetNearClipPlane(cam, 0.3f);
        RwCameraSetFarClipPlane(cam, 10000.0f);

        return TRUE;
    }

    void television::destroy()
    {
        if (texture != NULL)
        {
            RwTextureDestroy(texture);
        }
        else if (raster != NULL)
        {
            RwRasterDestroy(raster);
        }
        if (cam != NULL)
        {
            RwFrame* tmpframe = (RwFrame*)cam->object.object.parent;
            if (tmpframe != NULL)
            {
                _rwObjectHasFrameSetFrame(cam, NULL);
                RwFrameDestroy(tmpframe);
            }
            RpWorldRemoveCamera(world, cam);
            RwCameraDestroy(cam);
        }
        if (world != NULL)
        {
            RpWorldDestroy(world);
        }
        cam = NULL;
        raster = NULL;
        world = NULL;
    }
} // namespace

void zNPCPrawn::Setup()
{
    static const S32 vert_index[4] = { 2, 0, 1, 3 };

    this->disco = (z_disco_floor*)zSceneFindObject(xStrHash("DISCO_PRAWN"));
    this->closeups_used = 0;

    for (S32 i = 0; i < 8; i++)
    {
        char name[64];
        sprintf(name, "JUMBOTRON_%02d", i);

        xBase* obj = zSceneFindObject(xStrHash(name));
        if (obj == NULL || !xEntValidType(obj->baseType))
        {
            continue;
        }

        xEnt* ent = (xEnt*)obj;
        if (ent->model == NULL)
        {
            continue;
        }

        this->closeup_model[this->closeups_used] = ent->model;

        RpGeometry* geom = ent->model->Data->geometry;
        if (geom == NULL)
        {
            continue;
        }

        RpMorphTarget* morph = geom->morphTarget;
        if (morph == NULL)
        {
            continue;
        }

        xVec3* verts = (xVec3*)morph->verts;
        if (verts == NULL)
        {
            continue;
        }

        xVec3* dst = this->closeup_loc[this->closeups_used];
        const xMat4x3* mat = (const xMat4x3*)ent->model->Mat;
        S32 numverts = geom->numVertices;

        S32 j;
        for (j = 0; j < 4; j++)
        {
            if (vert_index[j] >= numverts)
            {
                break;
            }
            xMat4x3Toworld(&dst[j], mat, &verts[vert_index[j]]);
        }

        if (j < 4)
        {
            continue;
        }

        this->closeups_used++;
    }

    if (this->closeups_used)
    {
        closeup.set_background(xColorFromRGBA(0, 0, 0, 0xff));
    }

    zNPCSubBoss::Setup();
}

namespace
{
    void television::set_background(iColor_tag color)
    {
        this->bgraster = 0;
        this->bgcolor.red = color.r;
        this->bgcolor.green = color.g;
        this->bgcolor.blue = color.b;
        this->bgcolor.alpha = color.a;
    }
} // namespace

void zNPCPrawn::Reset()
{
    reset_sound();
    zNPCCommon::Reset();
    memset(&this->flag, 0, 1);
    decompose();
    reappear();
    show_model();

    this->life = this->cfg_npc->pts_damage;
    this->face_player = 1;

    for (S32 i = 0; i < 3; i++)
    {
        if (this->spawner[i] != NULL)
        {
            this->spawner[i]->Reset();
        }
    }

    update_round();

    this->turn.accel = tweak.turn_accel;
    this->turn.max_vel = this->cfg_npc->spd_turnMax;
    this->floor_state_index = 0;
    this->fighting = 0;
    this->first_update = 1;
    this->beam.reset();

    exhaust_emitter = zParEmitterFind("PAREMIT_PRAWN_EXHAUST");
    exhaust_emitter_settings.custom_flags = 0x300;
    exhaust_emitter_settings.pos = g_O3;
    exhaust_emitter_settings.vel = g_O3;

    this->psy_instinct->GoalSet(NPC_GOAL_PRAWNIDLE, 1);
}

void zNPCPrawn::Destroy()
{
    zNPCCommon::Destroy();
    closeup.destroy();
}

void zNPCPrawn::Process(xScene* xscn, F32 dt)
{
    if (this->first_update)
    {
        set_floor_state(FS_SAFE, true, true);
        this->first_update = 0;
    }

    repel_player();

    for (S32 i = 0; i < 3; i++)
    {
        if (this->spawner[i] != NULL)
        {
            this->spawner[i]->Timestep(dt);
        }
    }

    if (!this->fighting || this->psy_instinct->GIDOfActive() == NPC_GOAL_LIMBO)
    {
        zNPCCommon::Process(xscn, dt);
        return;
    }

    this->delay += dt;
    this->psy_instinct->Timestep(dt, NULL);

    if (this->face_player)
    {
        xVec3& ppos = *(xVec3*)&globals.player.ent.model->Mat->pos;
        xVec3& center = get_center();
        this->look_dir.assign(ppos.x - center.x, ppos.z - center.z);
        this->look_dir.normalize();
    }

    update_turn(dt);
    update_animation(dt);
    update_floor(dt);
    update_beam(dt);
    update_particles(dt);
    check_player_damage();
    update_camera(dt);

    zNPCCommon::Process(xscn, dt);
}

void zNPCPrawn::NewTime(xScene* xscn, float dt)
{
    zNPCCommon::NewTime(xscn, dt);
    zNPCPrawn::render_closeup();
}

S32 zNPCPrawn::SysEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                        xBase* toParamWidget, S32* handled)
{
    switch ((S32)toEvent)
    {
    case 0x134:
    case 0x135:
    case 0x136:
    case 0x137:
    case 0x138:
        break;
    case 0x1b5:
        start_fight();
        break;
    case 0x1d9:
        this->psy_instinct->GoalSet(NPC_GOAL_PRAWNDEATH, 1);
        break;
    default:
        *handled = 0;
        return zNPCCommon::SysEvent(from, to, toEvent, toParam, toParamWidget, handled);
    }

    return 1;
}

namespace
{
    S32 load_patterns(xModelAssetParam* ap, U32 apsize, const char* fmt,
                      zNPCPrawn::range_type* pattern, S32 count)
    {
        char name[128];
        zNPCPrawn::range_type* p = pattern;
        S32 i;

        for (i = 0; i < count; p++, i++)
        {
            sprintf(name, fmt, i);
            strcat(name, ".min");
            p->min = zParamGetInt(ap, apsize, name, p->min);

            sprintf(name, fmt, i);
            strcat(name, ".max");
            p->max = zParamGetInt(ap, apsize, name, p->max);

            if (p->min < 0 || p->max < 0)
            {
                break;
            }
        }
        return i;
    }
} // namespace

void zNPCPrawn::ParseINI()
{
    static const zNPCPrawn::range_type default_pattern[20] = {
        { 0, 1 },   { 3, 4 },   { 6, 11 },  { 13, 22 }, { 24, 27 },
        { 29, 35 }, { 37, 38 }, { 40, 45 }, { -1, -1 },
    };

    zNPCCommon::ParseINI();
    tweak.load(this->parmdata, this->pdatsize);
    tweak.turn_accel = zParamGetFloat(this->parmdata, this->pdatsize, "turn_accel", 5.0f);
    tweak.spawn_delay = zParamGetFloat(this->parmdata, this->pdatsize, "spawn_delay", 1.0f);
    tweak.repel_radius = zParamGetFloat(this->parmdata, this->pdatsize, "repel_radius", 4.2f);
    tweak.safe.pattern.min = zParamGetInt(this->parmdata, this->pdatsize, "safe.pattern.min", 0);
    tweak.safe.pattern.max = zParamGetInt(this->parmdata, this->pdatsize, "safe.pattern.max", 0);
    tweak.begin.pattern.min = zParamGetInt(this->parmdata, this->pdatsize, "begin.pattern.min", 0);
    tweak.begin.pattern.max = zParamGetInt(this->parmdata, this->pdatsize, "begin.pattern.max", 1);
    tweak.begin.state_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "begin.state_delay", 0.0f);
    tweak.begin.transition_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "begin.transition_delay", 1.0f);
    tweak.beam.delay[0] = zParamGetFloat(this->parmdata, this->pdatsize, "beam.delay[0]", 2.5f);
    tweak.beam.delay[1] = zParamGetFloat(this->parmdata, this->pdatsize, "beam.delay[1]", 4.5f);
    tweak.beam.delay[2] = zParamGetFloat(this->parmdata, this->pdatsize, "beam.delay[2]", 6.5f);
    tweak.beam.pattern.min = zParamGetInt(this->parmdata, this->pdatsize, "beam.pattern.min", 3);
    tweak.beam.pattern.max = zParamGetInt(this->parmdata, this->pdatsize, "beam.pattern.max", 0x13);
    tweak.beam.state_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.state_delay", 0.05f);
    tweak.beam.transition_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.transition_delay", 0.05f);
    tweak.beam.exhaust_vel =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.exhaust_vel", 15.0f);
    tweak.beam.knock_back = zParamGetFloat(this->parmdata, this->pdatsize, "beam.knock_back", 1.0f);
    tweak.beam.sweep.amount[0] =
        zParamGetInt(this->parmdata, this->pdatsize, "beam.sweep.amount[0]", 2);
    tweak.beam.sweep.amount[1] =
        zParamGetInt(this->parmdata, this->pdatsize, "beam.sweep.amount[1]", 3);
    tweak.beam.sweep.amount[2] =
        zParamGetInt(this->parmdata, this->pdatsize, "beam.sweep.amount[2]", 4);
    tweak.beam.sweep.arc =
        0.017453292f * zParamGetFloat(this->parmdata, this->pdatsize, "beam.sweep.arc", 20.0f);
    tweak.beam.sweep.delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.sweep.delay", 0.5f);
    tweak.beam.sweep.accel =
        0.017453292f * zParamGetFloat(this->parmdata, this->pdatsize, "beam.sweep.accel", 60.0f);
    tweak.beam.sweep.max_vel =
        0.017453292f * zParamGetFloat(this->parmdata, this->pdatsize, "beam.sweep.max_vel", 26.5f);
    tweak.beam.fire.duration = -1.0f;
    tweak.beam.fire.ring.size =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.size", 0.4f);
    tweak.beam.fire.ring.alpha =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.alpha", 1.0f);
    tweak.beam.fire.ring.vel =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.vel", 9.0f);
    tweak.beam.fire.ring.accel =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.accel", 10.0f);
    tweak.beam.fire.ring.emit_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.emit_delay", 0.1f);
    tweak.beam.fire.ring.grow =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.grow", 0.15f);
    tweak.beam.fire.ring.fade_dist =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.fade_dist", 15.0f);
    tweak.beam.fire.ring.kill_dist =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.kill_dist", 20.0f);
    tweak.beam.fire.ring.follow =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.follow", 0.0f);
    tweak.beam.fire.ring.hit_radius =
        zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.ring.hit_radius", 0.3f);
    zParamGetVector(this->parmdata, this->pdatsize, "beam.fire.ring.hit_offset",
                    xVec3::create(0.0f, 0.0f, 0.0f), &tweak.beam.fire.ring.hit_offset);
    tweak.beam.fire.emit_bone =
        zParamGetInt(this->parmdata, this->pdatsize, "beam.fire.emit_bone", 0x2b);
    zParamGetVector(this->parmdata, this->pdatsize, "beam.fire.offset",
                    xVec3::create(0.0f, 0.15f, 0.0f), &tweak.beam.fire.offset);
    tweak.beam.fire.yaw =
        0.017453292f * zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.yaw", 0.0f);
    tweak.beam.fire.pitch =
        0.017453292f * zParamGetFloat(this->parmdata, this->pdatsize, "beam.fire.pitch", 5.5f);
    this->precomp.sin_pitch = isin(tweak.beam.fire.pitch);
    this->precomp.cos_pitch = icos(tweak.beam.fire.pitch);
    tweak.aim_lane.duration =
        zParamGetFloat(this->parmdata, this->pdatsize, "aim_lane.duration", 2.0f);
    tweak.aim_lane.state_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "aim_lane.state_delay", 0.1f);
    tweak.aim_lane.transition_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "aim_lane.transition_delay", 0.1f);
    tweak.aim_lane.pattern.first =
        zParamGetInt(this->parmdata, this->pdatsize, "aim_lane.pattern.first", 0x15);
    tweak.aim_lane.pattern.range =
        zParamGetInt(this->parmdata, this->pdatsize, "aim_lane.pattern.range", 0x8);
    tweak.aim_lane.pattern.offset =
        zParamGetInt(this->parmdata, this->pdatsize, "aim_lane.pattern.offset", 0x9);
    tweak.aim_lane.pattern.size =
        zParamGetInt(this->parmdata, this->pdatsize, "aim_lane.pattern.size", 0x4);
    tweak.lane.duration[0] =
        zParamGetFloat(this->parmdata, this->pdatsize, "lane.duration[0]", 8.0f);
    tweak.lane.duration[1] =
        zParamGetFloat(this->parmdata, this->pdatsize, "lane.duration[1]", 7.0f);
    tweak.lane.duration[2] =
        zParamGetFloat(this->parmdata, this->pdatsize, "lane.duration[2]", 6.0f);
    tweak.lane.state_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "lane.state_delay", 0.0f);
    tweak.lane.transition_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "lane.transition_delay", 0.1f);
    tweak.lane.pattern.first =
        zParamGetInt(this->parmdata, this->pdatsize, "lane.pattern.first", 0x39);
    tweak.lane.pattern.range =
        zParamGetInt(this->parmdata, this->pdatsize, "lane.pattern.range", 0x8);
    tweak.lane.pattern.offset =
        zParamGetInt(this->parmdata, this->pdatsize, "lane.pattern.offset", 0x9);
    tweak.lane.pattern.size =
        zParamGetInt(this->parmdata, this->pdatsize, "lane.pattern.size", 0x4);
    tweak.danger.state_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "danger.state_delay", 0.2f);
    tweak.danger.transition_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "danger.transition_delay", 0.2f);
    tweak.danger.cycle_delay =
        zParamGetFloat(this->parmdata, this->pdatsize, "danger.cycle_delay", 6.0f);
    tweak.danger.pattern_offset =
        zParamGetInt(this->parmdata, this->pdatsize, "danger.pattern_offset", 0x5d);

    memcpy(tweak.danger.pattern, default_pattern, sizeof(tweak.danger.pattern));
    tweak.danger.pattern_size =
        load_patterns(this->parmdata, this->pdatsize, "tweak.danger.pattern[%d]",
                      (zNPCPrawn::range_type*)tweak.danger.pattern, 20);
}

void tweak_group::load(xModelAssetParam* p, U32 i)
{
    this->register_tweaks(TRUE, p, i, 0);
}

void tweak_group::register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char* c)
{
    if (init)
    {
        this->beam.fire.sound_interval = 2;
        auto_tweak::load_param<S32, S32>(this->beam.fire.sound_interval, 1, 1, 10, ap, apsize,
                                         "beam.fire.sound_interval");
    }
    if (init)
    {
        this->sound[0].volume = 1.0f;
        auto_tweak::load_param<F32, F32>(this->sound[0].volume, 1.0f, 0.0f, 1.0f, ap, apsize,
                                         "sound[SOUND_HIT].volume");
    }
    if (init)
    {
        this->sound[0].range_inner = 20.0f;
        auto_tweak::load_param<F32, F32>(this->sound[0].range_inner, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_HIT].range_inner");
    }
    if (init)
    {
        this->sound[0].range_outer = 40.0f;
        auto_tweak::load_param<F32, F32>(this->sound[0].range_outer, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_HIT].range_outer");
    }
    if (init)
    {
        this->sound[0].delay = 0.0f;
        auto_tweak::load_param<F32, F32>(this->sound[0].delay, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                         "sound[SOUND_HIT].delay");
    }
    if (init)
    {
        this->sound[1].volume = 1.0f;
        auto_tweak::load_param<F32, F32>(this->sound[1].volume, 1.0f, 0.0f, 1.0f, ap, apsize,
                                         "sound[SOUND_BEAM_BEGIN].volume");
    }
    if (init)
    {
        this->sound[1].range_inner = 20.0f;
        auto_tweak::load_param<F32, F32>(this->sound[1].range_inner, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_BEAM_BEGIN].range_inner");
    }
    if (init)
    {
        this->sound[1].range_outer = 40.0f;
        auto_tweak::load_param<F32, F32>(this->sound[1].range_outer, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_BEAM_BEGIN].range_outer");
    }
    if (init)
    {
        this->sound[1].delay = 0.3f;
        auto_tweak::load_param<F32, F32>(this->sound[1].delay, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                         "sound[SOUND_BEAM_BEGIN].delay");
    }
    if (init)
    {
        this->sound[2].volume = 0.4f;
        auto_tweak::load_param<F32, F32>(this->sound[2].volume, 1.0f, 0.0f, 1.0f, ap, apsize,
                                         "sound[SOUND_BEAM_LOOP].volume");
    }
    if (init)
    {
        this->sound[2].range_inner = 10.0f;
        auto_tweak::load_param<F32, F32>(this->sound[2].range_inner, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_BEAM_LOOP].range_inner");
    }
    if (init)
    {
        this->sound[2].range_outer = 30.0f;
        auto_tweak::load_param<F32, F32>(this->sound[2].range_outer, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_BEAM_LOOP].range_outer");
    }
    if (init)
    {
        this->sound[2].delay = 1.25f;
        auto_tweak::load_param<F32, F32>(this->sound[2].delay, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                         "sound[SOUND_BEAM_LOOP].delay");
    }
    if (init)
    {
        this->sound[2].fade_time = 1.25f;
        auto_tweak::load_param<F32, F32>(this->sound[2].fade_time, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_BEAM_LOOP].fade_time");
    }
    if (init)
    {
        this->sound[3].volume = 0.5f;
        auto_tweak::load_param<F32, F32>(this->sound[3].volume, 1.0f, 0.0f, 1.0f, ap, apsize,
                                         "sound[SOUND_RING].volume");
    }
    if (init)
    {
        this->sound[3].range_inner = 0.0f;
        auto_tweak::load_param<F32, F32>(this->sound[3].range_inner, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_RING].range_inner");
    }
    if (init)
    {
        this->sound[3].range_outer = 10.0f;
        auto_tweak::load_param<F32, F32>(this->sound[3].range_outer, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_RING].range_outer");
    }
    if (init)
    {
        this->sound[3].delay = 0.1f;
        auto_tweak::load_param<F32, F32>(this->sound[3].delay, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                         "sound[SOUND_RING].delay");
    }
    if (init)
    {
        this->sound[3].fade_time = 0.1f;
        auto_tweak::load_param<F32, F32>(this->sound[3].fade_time, 1.0f, 0.0f, 100000.0f, ap,
                                         apsize, "sound[SOUND_RING].fade_time");
    }
    if (init)
    {
        this->sound[0].asset = sound_asset_ids[0][0];
        const sound_asset_type& asset = sound_assets[this->sound[0].asset];
        sound_data[0].id = xStrHash(asset.name);
    }
    if (init)
    {
        this->sound[1].asset = sound_asset_ids[1][0];
        const sound_asset_type& asset = sound_assets[this->sound[1].asset];
        sound_data[1].id = xStrHash(asset.name);
    }
    if (init)
    {
        this->sound[2].asset = sound_asset_ids[2][0];
        const sound_asset_type& asset = sound_assets[this->sound[2].asset];
        sound_data[2].id = xStrHash(asset.name);
    }
    if (init)
    {
        this->sound[3].asset = sound_asset_ids[3][0];
        const sound_asset_type& asset = sound_assets[this->sound[3].asset];
        sound_data[3].id = xStrHash(asset.name);
    }
}

void zNPCPrawn::ParseLinks()
{
    zNPCCommon::ParseLinks();

    xLinkAsset* it = this->link;
    xLinkAsset* end = it + this->linkCount;

    for (; it != end; it++)
    {
        if (it->dstEvent == 0x133)
        {
            add_child(*zSceneFindObject(it->dstAssetID), (S32)it->param[0]);
        }
    }
}

void zNPCPrawn::SelfSetup()
{
    xBehaveMgr* bmgr;
    xPsyche* psy;

    bmgr = xBehaveMgr_GetSelf();
    psy_instinct = bmgr->Subscribe(this, 0);
    psy = psy_instinct;
    psy->BrainBegin();
    psy->AddGoal(NPC_GOAL_PRAWNIDLE, NULL);
    psy->AddGoal(NPC_GOAL_PRAWNBEAM, NULL);
    psy->AddGoal(NPC_GOAL_PRAWNBOWL, NULL);
    psy->AddGoal(NPC_GOAL_PRAWNDAMAGE, NULL);
    psy->AddGoal(NPC_GOAL_PRAWNDEATH, NULL);
    psy->AddGoal(NPC_GOAL_LIMBO, NULL);
    psy->BrainEnd();
    psy->SetSafety(NPC_GOAL_PRAWNIDLE);
}

void zNPCPrawn::Damage(en_NPC_DAMAGE_TYPE damage_type, xBase* base, const xVec3* vec)
{
    S32 state = this->psy_instinct->GIDOfActive();
    switch (damage_type)
    {
    case DMGTYP_BOULDER:
    case DMGTYP_BUBBOWL:
        if (state == NPC_GOAL_PRAWNBOWL)
        {
            set_life(this->life - 1);
        }
        break;
    default:
        break;
    }
}

void zNPCPrawn::DuploNotice(en_SM_NOTICES note, void* data)
{
    for (S32 i = 0; i < 3; i++)
    {
        if (this->spawner[i] != NULL && this->spawner[i]->Receivable(note, data))
        {
            this->spawner[i]->Notify(note, data);
            return;
        }
    }
}

U32 zNPCPrawn::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* goal)
{
    U32 animID = 0;
    S32 index;

    switch (gid)
    {
    case NPC_GOAL_PRAWNIDLE:
    case NPC_GOAL_PRAWNBOWL:
        index = ANIM_Idle01;
        break;
    case NPC_GOAL_PRAWNBEAM:
        index = ANIM_AttackWindup01;
        break;
    case NPC_GOAL_PRAWNDAMAGE:
        index = ANIM_Damage01;
        break;
    case NPC_GOAL_PRAWNDEATH:
        index = -1;
        break;
    default:
        index = ANIM_Idle01;
        break;
    }

    if (index > -1)
    {
        animID = g_hash_subbanim[index];
    }
    return animID;
}

void zNPCPrawn::Render()
{
    xNPCBasic::Render();
    isCulled = 0;
    beam.render();
    zNPCPrawn::render_debug();
}

void zNPCPrawn::update_round()
{
    S32 life = this->life;
    if (life == 0)
    {
        this->round = 3;
    }
    else
    {
        this->round = 2 - (S32)((life - 1) * 3) / this->cfg_npc->pts_damage;
    }
    S32 spawnerCount = sizeof(this->spawner) / sizeof(*this->spawner);
    for (S32 spawnerIndex = 0; spawnerIndex < spawnerCount; spawnerIndex++)
    {
        zNPCSpawner* current_spawner = this->spawner[spawnerIndex];
        if (current_spawner != NULL)
        {
            en_SM_NOTICES spawnerNotice = SM_NOTE_DUPRESUME;
            if (spawnerIndex > this->round)
            {
                spawnerNotice = SM_NOTE_DUPPAUSE;
            }
            current_spawner->Notify(spawnerNotice, NULL);
        }
    }
}

void zNPCPrawn::decompose()
{
    vanish();

    if (this->fighting != 0)
    {
        this->fighting = 0;
        set_floor_state(FS_SAFE, TRUE, TRUE);
        hide_model();

        S32 i = 0;
        do
        {
            if (this->spawner[i] != NULL)
            {
                this->spawner[i]->Notify(SM_NOTE_DUPDEAD, NULL);
                this->spawner[i]->Notify(SM_NOTE_KILLKIDS, NULL);
            }
            i++;
        } while (i < 3);

        zCameraEnableTracking(CO_BOSS);
        boss_cam.stop();
    }
}

zNPCSpawner* zNPCPrawn::make_spawner(S32 i)
{
    zNPCSpawner* spawner = this->spawner[i];
    if (spawner != NULL)
    {
        return spawner;
    }
    this->spawner[i] = zNPCSpawner_GetInstance();
    this->spawner[i]->Subscribe(this);
    this->spawner[i]->SetWaveMode(SM_WAVE_CONTINUOUS, tweak.spawn_delay, -1);
    return this->spawner[i];
}

void zNPCPrawn::add_child(xBase& child, S32 wave)
{
    switch (child.baseType)
    {
    case eBaseTypeNPC:
        make_spawner(wave)->AddSpawnNPC((zNPCCommon*)&child);
        break;
    case eBaseTypeMovePoint:
        make_spawner(wave)->AddSpawnPoint((zMovePoint*)&child);
        break;
    case eBaseTypeGroup:
    {
        U32 i = 0;
        U32 size = xGroupGetCount((xGroup*)&child);
        for (; i < size; i++)
        {
            xBase* entry = xGroupGetItemPtr((xGroup*)&child, i);
            add_child(*entry, wave);
        }
        break;
    }
    default:
        break;
    }
}

namespace
{
    void set_yaw_matrix(xMat3x3& mat, F32 yaw)
    {
        F32 s = isin(yaw);
        F32 c = icos(yaw);

        mat.right.assign(c, 0.0f, -s);
        mat.up.assign(0.0f, 1.0f, 0.0f);
        mat.at.assign(s, 0.0f, c);
    }
} // namespace

void zNPCPrawn::update_turn(F32 dt)
{
    get_center();

    RwMatrix* mat = this->model->Mat;
    xVec2 facing = { mat->at.x, mat->at.z };

    if (!turning())
    {
        return;
    }

    F32 cur = xatan2(facing.x, facing.y);
    F32 time_to_target;
    F32 diff = xatan2(this->look_dir.x, this->look_dir.y) - cur;
    if (diff > 3.1415927f)
    {
        diff -= 6.2831855f;
    }
    else if (diff < -3.1415927f)
    {
        diff += 6.2831855f;
    }

    bool decel = true;

    if (!(FABS(this->turn.vel) < 0.001f) && (diff < 0.0f ? 1 : 0) == (this->turn.vel < 0.0f ? 1 : 0))
    {
        decel = false;
    }

    if (decel)
    {
        time_to_target = 1.0e38f;
    }
    else
    {
        time_to_target = diff / this->turn.vel;
    }

    F32 time_to_stop = FABS(this->turn.vel / this->turn.accel);

    F32 dir = (time_to_target > time_to_stop) ? 1.0f : -1.0f;
    F32 sign = (diff >= 0.0f) ? 1.0f : -1.0f;

    F32 accel = this->turn.accel * (dir * sign);
    F32 dvel = accel * dt;
    F32 vel = this->turn.vel + dvel;
    F32 max_vel = this->turn.max_vel;

    if (FABS(vel) <= max_vel)
    {
        this->turn.vel = vel;
    }
    else if (FABS(this->turn.vel) <= max_vel)
    {
        this->turn.vel = range_limit<F32>(vel, -max_vel, max_vel);
    }
    else if ((vel < 0.0f ? 1 : 0) != (dvel < 0.0f ? 1 : 0))
    {
        this->turn.vel = vel;
    }

    F32 step = this->turn.vel * dt;
    if (time_to_target > time_to_stop)
    {
        if ((step < 0.0f ? 1 : 0) == (diff < 0.0f ? 1 : 0) && FABS(step) > FABS(diff))
        {
            this->turn.vel = 0.0f;
            step = diff;
        }
    }
    else if ((step < 0.0f ? 1 : 0) != (diff < 0.0f ? 1 : 0))
    {
        this->turn.vel = 0.0f;
        step = diff;
    }

    set_yaw_matrix(*(xMat3x3*)this->frame, cur + step);
}

void zNPCPrawn::update_animation(F32 dt)
{
    U32 idle_hash = g_hash_subbanim[ANIM_Idle01];
    U32 attack_hash = g_hash_subbanim[ANIM_AttackLoop01];

    U32 id = AnimCurStateID();
    if (id != idle_hash && id != attack_hash)
    {
        return;
    }

    F32 maxv;
    if (this->turn.max_vel < 0.001f)
    {
        maxv = 0.0f;
    }
    else
    {
        maxv = this->turn.vel / this->turn.max_vel;
    }

    range_limit<F32>(maxv, -1.0f, 1.0f);

    xAnimSingle* xas = this->model->Anim->Single;

    static bool registered = false;
    static F32 lerp = 1.0f;

    if (!registered)
    {
        registered = true;
        xDebugAddTweak("Temp|Prawn Anim Lerp", &lerp, 0.0f, 2.0f, NULL, NULL, 0);
    }

    xas->BilinearLerp[0] = lerp;
}

void zNPCPrawn::update_floor(F32 dt)
{
    if (this->floor_state == FS_DANGER && !this->pending.change)
    {
        this->floor_time += dt;
        if (this->floor_time >= tweak.danger.cycle_delay)
        {
            set_floor_state(FS_DANGER, false, true);
            this->floor_time = 0.0f;
        }
    }

    if (this->pending.change && this->disco->state_counter >= this->pending.counter)
    {
        apply_pending();
    }
}

void zNPCPrawn::update_beam(F32 dt)
{
    xMat4x3* rootmat = (xMat4x3*)this->model->Mat;
    xMat4x3& bonemat = ((xMat4x3*)this->model->Mat)[tweak.beam.fire.emit_bone];

    xVec3 loc = rootmat->pos + bonemat.pos + tweak.beam.fire.offset;

    xVec3 dir;
    mulat(dir, *(xMat3x3*)rootmat, *(xMat3x3*)&bonemat);
    dir.y = -this->precomp.sin_pitch;

    F32 yaw = xatan2(dir.x, dir.z) + tweak.beam.fire.yaw;
    F32 sin_yaw = isin(yaw);
    F32 cos_yaw = icos(yaw);
    dir.x = sin_yaw * this->precomp.cos_pitch;
    dir.z = cos_yaw * this->precomp.cos_pitch;

    this->beam.move(loc, dir);

    if (this->beam.active())
    {
        this->beam.update(dt);
        exhaust_emitter_settings.pos = loc;
        exhaust_emitter_settings.vel = dir * tweak.beam.exhaust_vel;
        xParEmitterEmitCustom(exhaust_emitter, dt, &exhaust_emitter_settings);
    }
}

namespace
{
    void mulat(xVec3& out, const xMat3x3& m, const xMat3x3& n)
    {
        out.x = m.at.x * n.right.x + m.at.y * n.up.x + m.at.z * n.at.x;
        out.y = m.at.x * n.right.y + m.at.y * n.up.y + m.at.z * n.at.y;
        out.z = m.at.x * n.right.z + m.at.y * n.up.z + m.at.z * n.at.z;
    }
} // namespace

void zNPCPrawn::update_particles(float)
{
}

void zNPCPrawn::update_camera(F32 dt)
{
    zCameraDisableTracking(CO_BOSS);
    if (!(zCameraIsTrackingDisabled() & ~CO_BOSS))
    {
        boss_cam.update(dt);
    }
}

void zNPCPrawn::start_fight()
{
    if (!this->fighting)
    {
        this->fighting = 1;
        set_floor_state(FS_BEGIN, true, false);
        zCameraDisableTracking(CO_BOSS);
        boss_cam.start(globals.camera);
        boss_cam.set_targets(*(xVec3*)&globals.player.ent.model->Mat->pos, this->bound.sph.center,
                             this->bound.sph.r);
    }
}

void zNPCPrawn::get_floor_info(zNPCPrawn::floor_state_enum s, zNPCPrawn::range_type& pattern,
                               F32& transition_delay, F32& state_delay)
{
    switch (s)
    {
    case FS_SAFE:
        pattern = *(zNPCPrawn::range_type*)&tweak.safe.pattern;
        transition_delay = 0.0f;
        state_delay = 1000000000.0f;
        break;
    case FS_BEGIN:
        pattern = *(zNPCPrawn::range_type*)&tweak.begin.pattern;
        transition_delay = tweak.begin.transition_delay;
        state_delay = tweak.begin.state_delay;
        break;
    case FS_BEAM:
        pattern = *(zNPCPrawn::range_type*)&tweak.beam.pattern;
        transition_delay = tweak.beam.transition_delay;
        state_delay = tweak.beam.state_delay;
        break;
    case FS_AIM_LANE:
        this->floor_state_index = xrand() % tweak.aim_lane.pattern.size;
        pattern.min =
            tweak.aim_lane.pattern.first + this->floor_state_index * tweak.aim_lane.pattern.offset;
        pattern.max = pattern.min + tweak.aim_lane.pattern.range - 1;
        transition_delay = tweak.aim_lane.transition_delay;
        state_delay = tweak.aim_lane.state_delay;
        break;
    case FS_LANE:
        pattern.min =
            tweak.lane.pattern.first + this->floor_state_index * tweak.lane.pattern.offset;
        pattern.max = pattern.min + tweak.lane.pattern.range - 1;
        transition_delay = tweak.lane.transition_delay;
        state_delay = tweak.lane.state_delay;
        break;
    case FS_DANGER:
        this->floor_state_index = xrand() % tweak.danger.pattern_size;
        pattern = *(zNPCPrawn::range_type*)&tweak.danger.pattern[this->floor_state_index];
        pattern.min += tweak.danger.pattern_offset;
        pattern.max += tweak.danger.pattern_offset;
        transition_delay = tweak.danger.transition_delay;
        state_delay = tweak.danger.state_delay;
        break;
    }
}

void zNPCPrawn::apply_pending()
{
    pending.change = 0;
    disco->set_state_range(pending.pattern.min, pending.pattern.max, SM_NPC_DEAD);
    disco->set_transition_delay(pending.transition_delay);
    disco->set_state_delay(pending.state_delay);
}

void zNPCPrawn::set_floor_state(zNPCPrawn::floor_state_enum s, bool immediate, bool force)
{
    if (s != this->floor_state || force)
    {
        this->pending.floor_state = s;
        get_floor_info(s, this->pending.pattern, this->pending.transition_delay,
                       this->pending.state_delay);

        if (immediate)
        {
            apply_pending();
            return;
        }

        z_disco_floor* disco = this->disco;
        U32 state_now = disco->state;
        U32 offset;
        if (state_now < disco->min_state || state_now > disco->max_state)
        {
            offset = 1;
        }
        else
        {
            offset = (disco->max_state - state_now) + 1;
        }
        this->pending.counter = disco->state_counter + offset;
        this->pending.change = 1;
    }
}

void zNPCPrawn::vanish()
{
    this->model->Flags &= 0xFFFC;
    this->flags &= 0xFE;
    this->flags |= 0x40;
    this->pflags = 0;
    this->moreFlags = 0;
    this->chkby = 0;
    this->penby = 0;
    this->flags2.flg_colCheck = 0;
    this->flags2.flg_penCheck = 0;
}

void zNPCPrawn::reappear()
{
    xModelInstance* temp_r6;

    temp_r6 = this->model;
    temp_r6->Flags |= 3;
    this->flags |= 1;
    this->flags &= 0xBF;
    this->pflags = 0;
    this->moreFlags = 0x10;
    this->chkby = 0x10;
    this->penby = 0x10;
    this->flags2.flg_colCheck = 0;
    this->flags2.flg_penCheck = 0;
}

void zNPCPrawn::render_closeup()
{
    if (this->closeups_used == 0)
    {
        return;
    }

    static xVec3 from = { 0.25f, 0.75f, 0.5f };
    static xVec3 to = { 0.0f, 0.5f, 0.0f };
    static bool registered = false;
    static F32 lerp = 1.0f;

    if (!registered)
    {
        registered = true;
        xDebugAddTweak("Temp|Prawn Render 2D From", &from, NULL, NULL, 0);
        xDebugAddTweak("Temp|Prawn Render 2D To", &to, NULL, NULL, 0);
    }

    const xMat4x3* mat = (const xMat4x3*)this->model->Mat;

    xVec3 world_from;
    xVec3 world_to;
    xMat4x3Toworld(&world_from, mat, &from);
    xMat4x3Toworld(&world_to, mat, &to);

    closeup.move(world_from, world_to);
    closeup.update(*this->model, this->lightKit);

    for (S32 i = 0; i < this->closeups_used; i++)
    {
        closeup.set_model_texture(*this->closeup_model[i]);
    }
}

namespace
{
    void television::set_model_texture(xModelInstance& model)
    {
        for (S32 bucketSlot = 0; bucketSlot < 2; bucketSlot++)
        {
            RpGeometry* modelGeometry = model.Bucket[bucketSlot]->Data->geometry;
            if (modelGeometry == 0 || modelGeometry->matList.numMaterials <= 0)
            {
                break;
            }
            RpMaterialSetTexture(modelGeometry->matList.materials[0], this->texture);
        }
    }

    void television::update(xModelInstance& model_inst, xLightKit* light_kit)
    {
        RwCamera* globalCamera = (RwCamera*)(RwEngineInstance->curCamera);
        if (globalCamera != NULL)
        {
            RwCameraEndUpdate(globalCamera);
            RwGameCubeCameraTextureFlush(globalCamera->frameBuffer, 0);
        }
        if (this->bgraster == NULL)
        {
            RwCameraClear(this->cam, &this->bgcolor, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
        }
        else
        {
            RwCameraClear(this->cam, &this->bgcolor, rwCAMERACLEARZ);
        }
        RwCameraBeginUpdate(this->cam);
        if (this->bgraster != NULL)
        {
            render_background();
        }
        zRenderState(SDRS_Default);

        if (light_kit != NULL)
        {
            xLightKit_Enable(light_kit, this->world);
        }
        xModelInstance* currentInstance = &model_inst;

        while (currentInstance != NULL)
        {
            // Retail bug: the visibility flag of the *head* instance is retested on every
            // iteration instead of the instance about to be drawn. Preserved as shipped.
            if (model_inst.Flags & 1)
            {
                iModelRender(currentInstance->Data, currentInstance->Mat);
            }
            currentInstance = currentInstance->Next;
        }
        if (light_kit != NULL)
        {
            // Doesn't use light_kit, but does a nullcheck?
            xLightKit_Enable(NULL, this->world);
        }
        render_static();
        RwCameraEndUpdate(this->cam);
        RwGameCubeCameraTextureFlush(this->cam->frameBuffer, 0);
        if (globalCamera != NULL)
        {
            RwCameraBeginUpdate(globalCamera);
        }
    }

    void television::render_static()
    {
    }

    void television::render_background()
    {
        zRenderState(SDRS_Fill);
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, this->bgraster);

        rwGameCube2DVertex* vert = (rwGameCube2DVertex*)xMemPushTemp(6 * sizeof(*vert));

        set_vert(vert[0], 0.0f, 0.0f, 0.0f, 0.0f);
        set_vert(vert[1], 0.0f, this->h, 0.0f, 1.0f);
        set_vert(vert[2], this->w, 0.0f, 1.0f, 0.0f);
        vert[3] = vert[2];
        vert[4] = vert[1];
        set_vert(vert[5], this->w, this->h, 1.0f, 1.0f);

        RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, vert, 6);
        xMemPopTemp(vert);
    }

    void television::set_vert(rwGameCube2DVertex& vert, F32 x, F32 y, F32 u, F32 v)
    {
        vert.x = x;
        vert.y = y;
        vert.z = 1.0f;
        vert.u = u;
        vert.v = v;
        vert.emissiveColor.red = 0xff;
        vert.emissiveColor.green = 0xff;
        vert.emissiveColor.blue = 0xff;
        vert.emissiveColor.alpha = 0xff;
    }

    void television::move(const xVec3& v1, const xVec3& v2)
    {
        xMat3x3 mat;

        RwFrame* frame = (RwFrame*)this->cam->object.object.parent;
        RwFrameTranslate(frame, (const RwV3d*)&v1, rwCOMBINEREPLACE);

        xMat3x3LookAt(&mat, &v2, &v1);

        RwMatrix* m = &frame->modelling;
        m->right = *(RwV3d*)&mat.right;
        m->at = *(RwV3d*)&mat.at;
        m->up = *(RwV3d*)&mat.up;
    }
} // namespace

void zNPCPrawn::set_life(S32 life)
{
    S32 oldlife = this->life;

    this->life = range_limit<S32>(life, 0, this->cfg_npc->pts_damage);

    S32 state = this->psy_instinct->GIDOfActive();
    if (state != NPC_GOAL_PRAWNDAMAGE && state != NPC_GOAL_PRAWNDEATH && this->life < oldlife)
    {
        this->psy_instinct->GoalSet(NPC_GOAL_PRAWNDAMAGE, 1);
        play_sound(0, &this->bound.sph.center, 1.0f);

        for (S32 i = this->life; i < oldlife; i++)
        {
            zEntEvent((xBase*)this, (xBase*)this, 0x1d7);
        }

        if (this->life <= 0)
        {
            zEntEvent((xBase*)this, (xBase*)this, 0x24);
        }
    }
    else
    {
        update_round();
    }
}

bool zNPCPrawn::check_player_damage()
{
    if (globals.player.cheat_mode)
    {
        return false;
    }

    if (!this->beam.hits_sphere(globals.player.ent.bound.sph))
    {
        return false;
    }

    xVec3 knock_vel = get_away() * tweak.beam.knock_back;
    zEntPlayer_Damage((xBase*)this, 1, &knock_vel);
    return true;
}

xVec3 zNPCPrawn::get_away() const
{
    xVec3 away;

    away.x = globals.player.ent.bound.sph.center.x - this->bound.sph.center.x;
    away.z = globals.player.ent.bound.sph.center.z - this->bound.sph.center.z;
    away.y = 0.0f;

    F32 dist2 = away.x * away.x + away.z * away.z;
    if (dist2 < 0.001f)
    {
        away.assign(0.0f, 1.0f, 0.0f);
    }
    else
    {
        F32 scale = 0.70710677f * (1.0f / xsqrt(dist2));
        away.assign(away.x * scale, 0.70710677f, away.z * scale);
    }

    return away;
}

void zNPCPrawn::repel_player() const
{
    if (globals.player.cheat_mode)
    {
        return;
    }

    xVec3 center = get_center();
    xVec3* player_loc = (xVec3*)&globals.player.ent.model->Mat->pos;
    xVec3* player_vel = &globals.player.ent.frame->vel;

    xVec3 offset = *player_loc - center;
    offset.y = 0.0f;

    F32 dist2 = offset.length2();
    if ((dist2 >= -1.0e-05f && dist2 <= 1.0e-05f) ||
        dist2 >= tweak.repel_radius * tweak.repel_radius)
    {
        return;
    }

    F32 dist = xsqrt(dist2);
    xVec3 dir = offset;
    dir *= 1.0f / dist;

    F32 into = player_vel->dot(dir);
    if (into < 0.0f)
    {
        *player_vel -= dir * into;
    }

    *player_loc += dir * (tweak.repel_radius - dist);
}

void zNPCPrawn::show_model()
{
    xModelInstance* inst = this->model;
    while (inst != NULL)
    {
        inst->Flags |= 3;
        inst = inst->Next;
    }
}

void zNPCPrawn::hide_model()
{
    xModelInstance* inst = this->model;
    while (inst != NULL)
    {
        inst->Flags &= 0xFFFC;
        inst = inst->Next;
    }
}

S32 zNPCGoalPrawnIdle::Enter(float dt, void* updCtxt)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);
    prawn.face_player = 1;
    prawn.delay = 0;
    prawn.set_floor_state(prawn.FS_DANGER, false, false);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalPrawnIdle::Exit(float dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalPrawnIdle::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    if (prawn.delay >= tweak.beam.delay[prawn.round])
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_PRAWNBEAM;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalPrawnBeam::Enter(F32 dt, void* updCtxt)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    prawn.face_player = 0;
    prawn.set_floor_state(prawn.FS_BEAM, true, false);
    this->sweeps = 0;
    this->substate = SS_AIM;

    xVec3& facing = prawn.get_facing();
    RwMatrix* mat = globals.player.ent.model->Mat;
    xVec3* at = (xVec3*)&mat->at;

    if (at->length2() < 0.001f)
    {
        this->sweep_dir = ((xrand() >> 17) & 1) ? -1.0f : 1.0f;
    }
    else
    {
        this->sweep_dir = (facing.z * at->x - at->z * facing.x < 0.0f) ? -1.0f : 1.0f;
    }

    init_look_dir();
    play_sound(1, &prawn.beam.loc, 1.0f);
    play_sound(2, &prawn.beam.loc, 1.0f);

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalPrawnBeam::Exit(F32 dt, void* updCtxt)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    prawn.beam.stop();
    prawn.turn.accel = tweak.turn_accel;
    prawn.turn.max_vel = prawn.cfg_npc->spd_turnMax;
    kill_sound(2);

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalPrawnBeam::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    switch (this->substate)
    {
    case SS_AIM:
        this->substate = (substate_enum)update_aim(dt);
        break;
    case SS_FIRE:
        this->substate = (substate_enum)update_fire(dt);
        break;
    case SS_HOLD:
        this->substate = (substate_enum)update_hold(dt);
        break;
    case SS_SWEEP:
        this->substate = (substate_enum)update_sweep(dt);
        break;
    case SS_STOP:
        this->substate = (substate_enum)update_stop(dt);
        break;
    default:
        break;
    }

    if (this->substate >= MAX_SS)
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_PRAWNBOWL;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalPrawnBeam::update_aim(F32 dt)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);
    // SS_AIM (0) while still turning, SS_FIRE (1) once the aim is settled.
    return prawn.turning() ? 0 : 1;
}

S32 zNPCGoalPrawnBeam::update_fire(F32 dt)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);
    prawn.beam.start();
    prawn.delay = 0.0f;
    prawn.turn.accel = tweak.beam.sweep.accel;
    prawn.turn.max_vel = tweak.beam.sweep.max_vel;
    return SS_HOLD;
}

S32 zNPCGoalPrawnBeam::update_hold(F32 dt)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    if (prawn.delay < tweak.beam.sweep.delay)
    {
        return SS_HOLD;
    }

    if (this->sweeps >= tweak.beam.sweep.amount[prawn.round])
    {
        prawn.AnimStart(g_hash_subbanim[ANIM_AttackEnd01], 0);
        return SS_STOP;
    }

    this->sweeps++;
    this->sweep_dir *= -1.0f;
    return SS_SWEEP;
}

S32 zNPCGoalPrawnBeam::update_sweep(F32 dt)
{
    RwMatrix* mat;
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    mat = globals.player.ent.model->Mat;

    xVec3& center = prawn.get_center();
    xVec3& facing = prawn.get_facing();

    xVec2 delta = { 0.0f, 0.0f };
    delta.x = mat->pos.x - center.x;
    delta.y = mat->pos.z - center.z;

    F32 goal = xatan2(delta.x, delta.y) + this->sweep_dir * tweak.beam.sweep.arc;
    F32 cur = xatan2(facing.x, facing.z);

    F32 turn_diff = xrmod(3.1415927f + (goal - cur)) - 3.1415927f;
    if ((turn_diff < 0.0f ? 1 : 0) == (this->sweep_dir < 0.0f ? 1 : 0))
    {
        goal += (0.5f * this->sweep_dir) * tweak.beam.sweep.arc;
        prawn.look_dir.x = isin(goal);
        prawn.look_dir.y = icos(goal);
        return SS_SWEEP;
    }

    prawn.delay = 0.0f;
    prawn.look_dir.assign(facing.x, facing.z);
    prawn.look_dir.normalize();
    return SS_HOLD;
}

S32 zNPCGoalPrawnBeam::update_stop(F32 dt)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    if (prawn.AnimCurStateID() == g_hash_subbanim[ANIM_AttackEnd01] &&
        prawn.AnimTimeRemain(NULL) < 0.001f + dt)
    {
        return SS_STOP;
    }
    return MAX_SS;
}

void zNPCGoalPrawnBeam::init_look_dir()
{
    RwMatrix* mat;
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    mat = globals.player.ent.model->Mat;

    xVec3& center = prawn.get_center();

    xVec2 delta = { 0.0f, 0.0f };
    delta.x = mat->pos.x - center.x;
    delta.y = mat->pos.z - center.z;

    F32 goal = xatan2(delta.x, delta.y) + this->sweep_dir * tweak.beam.sweep.arc;

    prawn.look_dir.x = isin(goal);
    prawn.look_dir.y = icos(goal);
}

S32 zNPCGoalPrawnBowl::Enter(float dt, void* updCtxt)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);
    prawn.face_player = 1;
    prawn.set_floor_state(prawn.FS_AIM_LANE, false, false);
    prawn.delay = 0;
    aiming = 1;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalPrawnBowl::Exit(float dt, void* updCtxt)
{
    zNPCPrawn& prawn = *(zNPCPrawn*)psyche->clt_owner;
    prawn.set_floor_state(prawn.FS_BEGIN, true, false);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalPrawnBowl::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    if (this->aiming)
    {
        if (prawn.delay >= tweak.aim_lane.duration)
        {
            prawn.set_floor_state(prawn.FS_LANE, false, false);
            this->aiming = 0;
            prawn.delay = 0.0f;
        }
    }
    else if (prawn.delay >= tweak.lane.duration[prawn.round])
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_PRAWNIDLE;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalPrawnDamage::Enter(float dt, void* updCtxt)
{
    zNPCPrawn& prawn = *(zNPCPrawn*)psyche->clt_owner;
    prawn.set_floor_state(prawn.FS_DANGER, false, false);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalPrawnDamage::Exit(float dt, void* updCtxt)
{
    zNPCPrawn& prawn = *(zNPCPrawn*)this->psyche->clt_owner;
    prawn.update_round();
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalPrawnDamage::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCPrawn& prawn = *((zNPCPrawn*)this->psyche->clt_owner);

    xAnimState* state = prawn.AnimCurState();
    if ((state->ID != g_hash_subbanim[ANIM_Damage01] &&
         state->ID != g_hash_subbanim[ANIM_Damage02]) ||
        prawn.AnimTimeRemain(NULL) < 0.001f + dt)
    {
        *trantype = GOAL_TRAN_SET;
        if (prawn.life <= 0)
        {
            return NPC_GOAL_PRAWNDEATH;
        }
        return NPC_GOAL_PRAWNIDLE;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalPrawnDeath::Enter(float dt, void* updCtxt)
{
    zNPCPrawn& prawn = *(zNPCPrawn*)this->psyche->clt_owner;
    prawn.decompose();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalPrawnDeath::Exit(float dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalPrawnDeath::Process(en_trantype* trantype, float dt, void* updCtxt, xScene* xscn)
{
    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

void xDebugAddTweak(const char*, xVec3*, const tweak_callback*, void*, U32)
{
}

xVec3& zNPCPrawn::get_center() const
{
    return reinterpret_cast<xVec3&>(this->model->Mat->pos);
}

void zNPCPrawn::render_debug()
{
}

bool zNPCPrawn::turning() const
{
    bool result = false;
    xVec2 facing = { 0.0f, 0.0f };
    RwMatrix* mat = this->model->Mat;

    facing.x = mat->at.x;
    facing.y = mat->at.z;

    if (!(this->turn.vel >= -1.0e-05f && this->turn.vel <= 1.0e-05f) ||
        (!(this->turn.accel >= -1.0e-05f && this->turn.accel <= 1.0e-05f) &&
         (!(this->look_dir.x > this->look_dir.y) ||
          !(FABS(this->look_dir.x - facing.x) < 0.001f)) &&
         (!(this->look_dir.x < this->look_dir.y) || !(FABS(this->look_dir.y - facing.y) < 0.001f))))
    {
        result = true;
    }

    return result;
}

bool aqua_beam::active() const
{
    return firing || !ring.queue.empty();
}

void aqua_beam::move(const xVec3& new_loc, const xVec3& new_dir)
{
    loc = new_loc;
    dir = new_dir;
}

xVec3& zNPCPrawn::get_facing() const
{
    return reinterpret_cast<xVec3&>(this->model->Mat->at);
}

U8 zNPCPrawn::ColChkFlags() const
{
    return 0;
}

U8 zNPCPrawn::ColPenFlags() const
{
    return 0;
}

U8 zNPCPrawn::ColChkByFlags() const
{
    return 16;
}

U8 zNPCPrawn::ColPenByFlags() const
{
    return 16;
}

U8 zNPCPrawn::PhysicsFlags() const
{
    return 3;
}

S32 zNPCPrawn::IsAlive()
{
    return this->life > 0;
}

namespace auto_tweak
{
    template <>
    inline void load_param<F32, F32>(F32& value, F32 scale, F32 lo, F32 hi, xModelAssetParam* ap,
                              U32 apsize, const char* name)
    {
        value = zParamGetFloat(ap, apsize, name, value);
        if (value < lo)
        {
            value = lo;
        }
        else if (value > hi)
        {
            value = hi;
        }
        value = value * scale;
    }

    template <>
    inline void load_param<S32, S32>(S32& value, S32 scale, S32 lo, S32 hi, xModelAssetParam* ap,
                              U32 apsize, const char* name)
    {
        S32 result = zParamGetInt(ap, apsize, name, value);
        if (result < lo)
        {
            result = lo;
        }
        else if (result > hi)
        {
            result = hi;
        }
        result *= scale;
        value = result;
    }
} // namespace auto_tweak
