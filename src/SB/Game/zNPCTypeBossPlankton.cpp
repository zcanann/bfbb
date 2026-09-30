#include "zNPCTypeBossPlankton.h"
#include "xDebug.h"
#include "xGroup.h"
#include "xMathInlines.h"
#include "zNPCMgr.h"
#include "zNPCTypes.h"

#include <types.h>
#include <string.h>

U32 xSndPlay3DFade(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, const xVec3* pos,
                   F32 innerRadius, F32 outerRadius, sound_category category, F32 fade, F32 delay);

namespace std
{
    float fabsf(float x);
}

#define ANIM_Unknown 0
#define ANIM_Idle01 1 // 0x4
#define ANIM_Taunt01 3 // 0xC
#define ANIM_move 66 // 0x42
#define ANIM_stun_begin 67 //0x43
#define ANIM_stun_loop 68 //0x44
#define ANIM_stun_end 69 //0x45
#define ANIM_attack_beam_begin 70 //0x46
#define ANIM_attack_beam_loop 71 //0x47
#define ANIM_attack_beam_end 72 //0x48
#define ANIM_attack_wall_begin 73 //0x49
#define ANIM_attack_wall_loop 74 //0x4a
#define ANIM_attack_wall_end 75 //0x4b
#define ANIM_attack_missle 76 //0x4c
#define ANIM_attack_bomb 77 //0x4d

#define SOUND_HOVER 0
#define SOUND_HIT 1
#define SOUND_BOLT_FIRE 2
#define SOUND_BOLT_FLY 3
#define SOUND_BOLT_HIT 4
#define SOUND_CHARGE 5

template <>
inline F32 range_limit<F32>(F32 v, F32 minv, F32 maxv)
{
    if (v <= minv)
    {
        return minv;
    }

    if (v >= maxv)
    {
        return maxv;
    }

    return v;
}

namespace auto_tweak
{
    template <>
    inline void load_param<S32, S32>(S32& value, S32 scale, S32 lo, S32 hi, xModelAssetParam* ap,
                              U32 apsize, const char* name)
    {
        S32 v = zParamGetInt(ap, apsize, name, value);
        if (v < lo)
        {
            v = lo;
        }
        else if (v > hi)
        {
            v = hi;
        }
        v = v * scale;
        value = v;
    }

    template <>
    inline void load_param<xVec3, S32>(xVec3& value, S32, S32, S32, xModelAssetParam* ap, U32 apsize,
                                const char* name)
    {
        xVec3 def = value;
        zParamGetVector(ap, apsize, name, def, &value);
    }

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
}

// These structs were used in deadstripped functions.
// This function is here to force the symbols to be linked.
//
// The target opens .rodata with the same unreferenced all-zero templates other
// units carry -- three more of 0x0C, seven of 0x28, one more 0x0C and one of
// 0x10 -- which offsets every later .rodata relocation.
void __deadstripped_zNPCTypeBossPlankton_head()
{
    const char _406[0x0C] = {};
    const char _410[0x0C] = {};
    const char _441[0x0C] = {};

    const char _607[0x28] = {};
    const char _608[0x28] = {};
    const char _609[0x28] = {};
    const char _610[0x28] = {};
    const char _611[0x28] = {};
    const char _612[0x28] = {};
    const char _613[0x28] = {};

    const char _781[0x0C] = {};
    const char _842[0x10] = {};
}

namespace
{
    struct sound_data_type
    {
        U32 id;
        U32 handle;
        const xVec3* loc;
        F32 volume;
    };

    struct sound_property
    {
        U32 asset;
        F32 volume;
        F32 range_inner;
        F32 range_outer;
        F32 delay;
        F32 fade_time;
    };

    struct sound_asset
    {
        S32 group;
        char* name;
        U32 priority;
        U32 flags;
    };

    typedef xLaserBoltEmitter::effect_data effect_data;

    static effect_data beam_launch_effect[2] = { { FX_TYPE_CALLBACK }, { FX_TYPE_CALLBACK } };

    static effect_data beam_head_effect[1] = { { FX_TYPE_DECAL_DIST, FX_ORIENT_PATH, 3.0f } };

    static effect_data beam_impact_effect[3] = { { FX_TYPE_PARTICLE, FX_ORIENT_HIT_NORM },
                                                 { FX_TYPE_CALLBACK },
                                                 { FX_TYPE_CALLBACK } };

    static effect_data beam_death_effect[1] = { { FX_TYPE_PARTICLE } };

    static effect_data beam_kill_effect[1] = { { FX_TYPE_CALLBACK } };

    static char* sound_asset_names[6][10];
    static U32 sound_asset_ids[6][10];
    static S32 sound_asset_names_size[6];
    static sound_data_type sound_data[6];

    struct tweak_group
    {
        xVec3 accel;
        xVec3 max_vel;
        F32 turn_accel;
        F32 turn_max_vel;
        F32 ground_y;
        F32 ground_radius;
        F32 hit_vel;
        F32 hit_max_dist;
        F32 idle_time;
        F32 min_arena_dist;
        struct
        {
            F32 min_ang;
            F32 max_ang;
            F32 min_delay;
            F32 max_delay;
        } follow;
        struct
        {
            F32 fuse_dist;
            F32 fuse_delay;
        } help;
        struct
        {
            xVec3 accel;
            xVec3 max_vel;
            F32 stun_duration;
            F32 obstruct_angle;
        } mode_buddy;
        struct
        {
            xVec3 accel;
            xVec3 max_vel;
            F32 stun_duration;
        } mode_harass;
        struct
        {
            F32 height;
            F32 radius;
            F32 beam_interval;
            F32 beam_duration;
            F32 beam_dist;
        } hunt;
        struct
        {
            F32 rate;
            F32 time_warm_up;
            F32 time_fire;
            F32 gun_tilt_min;
            F32 gun_tilt_max;
            F32 max_dist;
            F32 emit_dist;
            xLaserBoltEmitter::config fx;
        } beam;
        struct
        {
            F32 safety_dist;
            F32 safety_height;
            F32 attack_dist;
            F32 attack_height;
            F32 stun_time;
        } harass;
        struct
        {
            F32 duration;
            F32 accel;
            F32 max_vel;
        } flank;
        struct
        {
            F32 accel;
            F32 max_vel;
            F32 dist;
        } fall;
        struct
        {
            F32 duration;
            F32 move_delay_min;
            F32 move_delay_max;
            F32 accel;
            F32 max_vel;
        } evade;
        struct
        {
            xVec3 center;
            struct
            {
                F32 radius;
                F32 height;
            } attack;
            struct
            {
                F32 radius;
                F32 height;
            } safety;
        } arena;
        sound_property sound[6];
        void* context;
        tweak_callback cb_move;
        tweak_callback cb_arena;
        tweak_callback cb_ground;
        tweak_callback cb_beam;
        tweak_callback cb_help;
        tweak_callback cb_sound;
        tweak_callback cb_sound_asset;

        void load(xModelAssetParam*, U32);
        void register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char*);
    };

    static tweak_group tweak;

    static const sound_asset sound_assets[29] = {
        { 0, "RSB_foot_loop", 0, 3 },       { 0, "fan_loop", 0, 3 },
        { 0, "Rocket_burn_loop", 0, 3 },    { 0, "RP_whirr_loop", 0, 3 },
        { 0, "RP_whirr2_loop", 0, 3 },      { 0, "Glove_hover", 0, 3 },
        { 0, "Glove_pursuit", 0, 3 },       { 1, "Prawn_FF_hit", 0, 0 },
        { 1, "Prawn_hit", 0, 0 },           { 1, "Door_metal_shut", 0, 0 },
        { 1, "Ghostplat_fall", 0, 0 },      { 1, "ST_death", 0, 0 },
        { 1, "RP_Bwrrzt", 0, 0 },           { 1, "RP_chunk", 0, 0 },
        { 1, "b201_rp_exhale", 0, 0 },      { 2, "RP_laser_alt", 0, 0 },
        { 3, "RP_laser_loop", 0, 1 },       { 3, "ElecArc_alt_b", 0, 1 },
        { 3, "Laser_lrg_fire_loop", 0, 1 }, { 3, "Laser_sm_fire_loop", 0, 1 },
        { 4, "RB_stalact_brk", 0, 0 },      { 4, "Volcano_blast", 0, 0 },
        { 4, "RP_laser_thunk", 0, 0 },      { 4, "RP_pfft", 0, 0 },
        { 4, "RP_thwash", 0, 0 },           { 5, "RP_charge_whirr", 0, 0 },
        { 5, "B101_SC_jump", 0, 0 },        { 5, "KJ_Charge", 0, 0 },
        { 5, "Laser_med_pwrup1", 0, 0 }
    };

    // One more unreferenced all-zero xVec3 template the target holds between
    // sound_assets and beam_ring_curve.
    void __deadstripped_zNPCTypeBossPlankton_sound()
    {
        const char _896[0x0C] = {};
    }

    static const xDecalEmitter::curve_node beam_ring_curve[2] = {
        { 0.0f, { 255, 255, 255, 255 }, 0.0f },
        { 1.0f, { 255, 255, 255, 0 }, 1.0f }
    };

    static const xDecalEmitter::curve_node beam_glow_curve[3] = {
        { 0.0f, { 255, 255, 255, 255 }, 0.0f },
        { 0.5f, { 255, 0, 255, 255 }, 4.0f },
        { 1.0f, { 255, 0, 0, 0 }, 6.0f }
    };

    struct say_group
    {
        const zNPCNewsFish::say_enum* list;
        U32 size;
    };

    static const zNPCNewsFish::say_enum say_intro[] = { zNPCNewsFish::SAY_B303_INTRO_1,
                                                        zNPCNewsFish::SAY_B303_INTRO_2 };

    static const zNPCNewsFish::say_enum say_fuse_near[] = { zNPCNewsFish::SAY_B303_FUSE_NEAR,
                                                            zNPCNewsFish::SAY_ROBOT_VULN_2,
                                                            zNPCNewsFish::SAY_SB_VULN_3,
                                                            zNPCNewsFish::SAY_SB_VULN_4 };

    static const zNPCNewsFish::say_enum say_fuse_hit[] = { zNPCNewsFish::SAY_B303_FUSE_HIT,
                                                           zNPCNewsFish::SAY_HIT_BOSS_1,
                                                           zNPCNewsFish::SAY_SB_HIT_BOSS_1 };

    static const zNPCNewsFish::say_enum say_hit_boss_1[] = {
        zNPCNewsFish::SAY_HIT_BOSS_1,    zNPCNewsFish::SAY_HIT_BOSS_2,
        zNPCNewsFish::SAY_SB_HIT_BOSS_2, zNPCNewsFish::SAY_SB_HIT_BOSS_3,
        zNPCNewsFish::SAY_ROBOT_HIT,     zNPCNewsFish::SAY_ROBOT_STUN_1
    };

    static const zNPCNewsFish::say_enum say_hit_boss_2[] = { zNPCNewsFish::SAY_B303_BRAIN_HELP_1,
                                                             zNPCNewsFish::SAY_B303_BRAIN_HELP_2,
                                                             zNPCNewsFish::SAY_B303_BRAIN_HELP_3 };

    static const zNPCNewsFish::say_enum say_hit_boss_3[] = {
        zNPCNewsFish::SAY_B303_BRAIN_HELP_1, zNPCNewsFish::SAY_B303_BRAIN_HELP_2,
        zNPCNewsFish::SAY_B303_BRAIN_HELP_3, zNPCNewsFish::SAY_SB_VULN_1,
        zNPCNewsFish::SAY_SB_VULN_2,         zNPCNewsFish::SAY_SB_VULN_5
    };

    static const zNPCNewsFish::say_enum say_hit_boss_4[] = { zNPCNewsFish::SAY_HIT_LAST };

    static const zNPCNewsFish::say_enum say_hit_player[] = {
        zNPCNewsFish::SAY_HIT_PLAYER_1, zNPCNewsFish::SAY_HIT_PLAYER_2,
        zNPCNewsFish::SAY_HIT_PLAYER_3, zNPCNewsFish::SAY_HIT_PLAYER_4,
        zNPCNewsFish::SAY_HIT_PLAYER_5, zNPCNewsFish::SAY_HIT_PLAYER_6
    };

    static const say_group say_set[8] = { { say_intro, 2 },      { say_fuse_near, 4 },
                                          { say_fuse_hit, 3 },   { say_hit_boss_1, 6 },
                                          { say_hit_boss_2, 3 }, { say_hit_boss_3, 6 },
                                          { say_hit_boss_4, 1 }, { say_hit_player, 6 } };

    xVec3* get_player_loc()
    {
        return (xVec3*)&globals.player.ent.model->Mat->pos;
    }

    void init_sound()
    {
        memset(sound_asset_names_size, 0, sizeof(sound_asset_names_size));

        for (S32 i = 0; i < 29; i++)
        {
            const sound_asset& asset = sound_assets[i];
            if (asset.name == NULL)
            {
                continue;
            }

            S32& total = sound_asset_names_size[asset.group];
            sound_asset_names[asset.group][total] = asset.name;
            sound_asset_ids[asset.group][total] = i;
            total++;
        }

        memset(sound_data, 0, sizeof(sound_data));

        for (S32 i = 0; i < 6; i++)
        {
            sound_data[i].id = 0;
            sound_data[i].handle = 0;
        }
    }

    void reset_sound()
    {
        for (S32 i = 0; i < 6; ++i)
        {
            sound_data[i].handle = 0;
        }
    }

    U32 play_sound(int which, const xVec3* pos, F32 volume)
    {
        const sound_property& snd = tweak.sound[which];
        sound_data_type& data = sound_data[which];
        const sound_asset& asset = sound_assets[snd.asset];

        if ((asset.flags & 2) && data.handle != 0)
        {
            return data.handle;
        }

        if (asset.flags & 1)
        {
            data.handle =
                xSndPlay3DFade(data.id, volume * snd.volume, 1.0f, asset.priority, 0x800, pos,
                               snd.range_inner, snd.range_outer, SND_CAT_GAME, 0.0f, snd.delay);
        }
        else
        {
            data.handle = xSndPlay3D(data.id, volume * snd.volume, 1.0f, asset.priority, 0x800, pos,
                                     snd.range_inner, snd.range_outer, SND_CAT_GAME, snd.delay);
        }

        data.loc = pos;
        data.volume = volume;
        return data.handle;
    }

    void kill_sound(S32 which, U32 handle)
    {
        sound_data_type& data = sound_data[which];
        const sound_property& snd = tweak.sound[which];
        const sound_asset& asset = sound_assets[snd.asset];

        if (asset.flags & 1)
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
        const sound_asset& asset = sound_assets[snd.asset];
        if (asset.flags & 1)
        {
            xSndStopFade(handle, snd.fade_time);
        }
        else
        {
            xSndStop(handle);
        }

        data.handle = 0;
    }

    void play_beam_fly_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        if (bolt.context == NULL)
        {
            bolt.context = (void*)play_sound(SOUND_BOLT_FLY, &bolt.loc, 1.0f);
        }
    }

    void kill_beam_fly_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        if (bolt.context != NULL)
        {
            kill_sound(3, (U32)bolt.context);
            bolt.context = NULL;
        }
    }

    void play_beam_fire_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        play_sound(SOUND_BOLT_FIRE, &bolt.origin, 1.0f);
    }

    void play_beam_hit_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        play_sound(SOUND_BOLT_HIT, &bolt.loc, 1.0f);
    }

} // namespace

inline xVec3& zNPCBPlankton::location() const
{
    return reinterpret_cast<xVec3&>(this->model->Mat->pos);
}

inline void zNPCBPlankton::face_player()
{
    flag.face_player = true;
}

inline void zNPCBPlankton::render_debug()
{
}

inline bool zNPCBPlankton::turning() const
{
    const xVec2 at = { model->Mat->at.x, model->Mat->at.z };

    return !xfeq0(turn.vel) ||
           (!xfeq0(turn.accel) &&
            !(turn.dir.x > turn.dir.y && xabs(turn.dir.x - at.x) < 0.001f) &&
            !(turn.dir.x < turn.dir.y && xabs(turn.dir.y - at.y) < 0.001f));
}

inline void zNPCBPlankton::take_control()
{
    if (crony != NULL)
    {
        crony->HoldUpDude();
    }
}

inline F32 zNPCBPlankton::get_orbit_yaw(const xVec3& loc) const
{
    return xatan2(loc.x - orbit.center.x, loc.z - orbit.center.z);
}

inline void zNPCBPlankton::set_location(const xVec3& loc)
{
    reinterpret_cast<xVec3&>(model->Mat->pos) = frame->mat.pos = loc;
}

inline void zNPCBPlankton::give_control()
{
    if (crony != NULL)
    {
        crony->ThanksImDone();
    }
}

inline void zNPCBPlankton::enable_emitter(xParEmitter& p1) const
{
    p1.emit_flags |= 1;
}

inline void zNPCBPlankton::disable_emitter(xParEmitter& p1) const
{
    p1.emit_flags &= 0xFE;
}

inline U8 zNPCBPlankton::ColChkFlags() const
{
    return 0;
}

inline U8 zNPCBPlankton::ColPenFlags() const
{
    return 0;
}

inline U8 zNPCBPlankton::ColChkByFlags() const
{
    return 16;
}

inline U8 zNPCBPlankton::ColPenByFlags() const
{
    return 16;
}

inline U8 zNPCBPlankton::PhysicsFlags() const
{
    return 3;
}

inline S32 zNPCBPlankton::IsAlive()
{
    return 1;
}

xAnimTable* ZNPC_AnimTable_BossPlankton()
{
    S32 anim_list[32]; //dwarf says it should be 32, matches less with 15

    xAnimTable* table = xAnimTableNew("zNPCBPlankton", NULL, 0);
    S32 anim_size = 0;

    anim_list[anim_size++] = ANIM_Idle01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Idle01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_Taunt01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Taunt01], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_move;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_move], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_stun_begin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_begin], 0x20, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_stun_loop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_loop], 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_stun_end;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_end], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_attack_beam_begin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_begin], 0x20, 0, 1.0f, NULL, NULL,
                       0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_attack_beam_loop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_loop], 0x10, 0, 1.0f, NULL, NULL,
                       0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_attack_beam_end;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_end], 0x20, 0, 1.0f, NULL, NULL,
                       0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_attack_wall_begin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_begin], 0x20, 0, 1.0f, NULL, NULL,
                       0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_attack_wall_loop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_loop], 0x10, 0, 1.0f, NULL, NULL,
                       0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_attack_wall_end;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_end], 0x20, 0, 1.0f, NULL, NULL,
                       0.0f, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_attack_missle;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_missle], 0x20, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size++] = ANIM_attack_bomb;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_bomb], 0x20, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_size] = ANIM_Unknown;

    NPCC_BuildStandardAnimTran(table, g_strz_bossanim, anim_list, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_stun_begin],
                            g_strz_bossanim[ANIM_stun_loop], 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_stun_loop], g_strz_bossanim[ANIM_stun_end],
                            0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_attack_beam_loop], 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0,
                            0.1f, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_attack_beam_end], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0,
                            0.1f, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_loop],
                            g_strz_bossanim[ANIM_attack_beam_end], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0,
                            0.1f, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_begin],
                            g_strz_bossanim[ANIM_attack_wall_loop], 0, 0, 0x10, 0, 0.0f, 0.0f, 0, 0,
                            0.1f, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_loop],
                            g_strz_bossanim[ANIM_attack_wall_end], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0,
                            0.1f, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_Taunt01], g_strz_bossanim[ANIM_stun_begin],
                            0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_move], g_strz_bossanim[ANIM_stun_begin], 0,
                            0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_loop],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_end],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_begin],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_loop],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_end],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_missle],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_bomb],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, 0.0f, 0.0f, 0, 0, 0.1f,
                            0);

    return table;
}

zNPCBPlankton::zNPCBPlankton(S32 myType) : zNPCBoss(myType)
{
    memset(&flag, 0, sizeof(flag));
}

void zNPCBPlankton::Init(xEntAsset* asset) //66%
{
    ::init_sound();
    zNPCCommon::Init(asset);
    flg_move = 1;
    flg_vuln = 1;
    xNPCBasic::RestoreColFlags();
    territory_size = 0;
    played_intro = 0;
    init_beam();
    model->Anim->BeforeAnimMatrices = aim_gun;
}

void zNPCBPlankton::Setup()
{
    U32 tmpVar;

    zNPCBoss::Setup();
    zNPCBPlankton::setup_beam();
    tmpVar = xStrHash("NPC_NEWSCASTER");
    newsfish = (zNPCNewsFish*)zSceneFindObject(tmpVar);
}

void zNPCBPlankton::PostSetup()
{
    xUpdateCull_SetCB(globals.updateMgr, this, xUpdateCull_AlwaysTrueCB, NULL);
}

void zNPCBPlankton::Reset()
{
    if (newsfish != NULL)
    {
        newsfish->Reset();
    }

    ::reset_sound();
    zNPCCommon::Reset();
    reset_beam();
    memset(&flag, 0, sizeof(flag));
    face_player();

    turn.vel = 0.0f;
    move.vel = 0.0f;
    move.dest = location();
    flag.move = MOVE_ORBIT;
    ambush_delay = 0.0f;
    old_player_health = 0;

    scan_cronies();

    if (crony != NULL)
    {
        mode = MODE_BUDDY;
        stun_duration = tweak.mode_buddy.stun_duration;
        newsfish = NULL;
    }
    else
    {
        mode = MODE_HARASS;
        active_territory = 0;
        stun_duration = tweak.mode_harass.stun_duration;

        if (newsfish != NULL)
        {
            newsfish->TalkOnScreen(1);
        }
    }

    reset_speed();
    refresh_orbit();
    follow_player();
    reset_territories();

    if (mode == MODE_BUDDY)
    {
        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONIDLE, 1);
    }
    else
    {
        vanish();
        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONAMBUSH, 1);
    }
}

void zNPCBPlankton::Destroy()
{
    zNPCCommon::Destroy();
}

void zNPCBPlankton::Process(xScene* xscn, F32 dt)
{
    if (!flag.updated)
    {
        flag.updated = true;

        if (!played_intro)
        {
            say(0, 0, true);
            played_intro = true;
        }
    }

    beam.update(dt);
    delay = delay + dt;

    if (mode == MODE_HARASS && player_left_territory())
    {
        ambush_delay = 0.0f;
        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONAMBUSH, 1);
    }

    if (!(SomethingWonderful() & 0x23))
    {
        psy_instinct->Timestep(dt, NULL);
    }

    if (flag.face_player)
    {
        RwMatrixTag* mat = globals.player.ent.model->Mat;
        xVec3& loc = location();

        turn.dir.assign(mat->pos.x - loc.x, mat->pos.z - loc.z);
        turn.dir.normalize();
    }

    update_follow(dt);
    update_turn(dt);
    update_move(dt);
    update_animation(dt);

    if (check_player_damage())
    {
        zEntPlayer_Damage(this, 1);
    }

    update_aim_gun(dt);
    update_dialog(dt);

    if (beam.visible())
    {
        flg_xtrarend |= 2;
    }

    zNPCCommon::Process(xscn, dt);
}

S32 zNPCBPlankton::SysEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                            xBase* toParamWidget, S32* handled)
{
    *handled = 0;
    return zNPCCommon::SysEvent(from, to, toEvent, toParam, toParamWidget, handled);

    // ((zNPCCommon*) 0x1b8???
}

void zNPCBPlankton::Render()
{
    xNPCBasic::Render();
    zNPCBPlankton::render_debug();
}

void zNPCBPlankton::RenderExtraPostParticles()
{
    if ((beam.visible() & 0xff) != 0)
    {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)5);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)2);
        beam.render();
    }
}

void zNPCBPlankton::ParseINI()
{
    zNPCCommon::ParseINI();
    tweak.load(parmdata, pdatsize);
}

namespace
{

    void tweak_group::load(xModelAssetParam* ap, U32 apsize)
    {
        register_tweaks(true, ap, apsize, NULL);
    }

    void tweak_group::register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char*)
    {
        xVec3 V0 = { 0.0f, 0.0f, 0.0f };

        if (init)
        {
            turn_accel = 540.0f;
            auto_tweak::load_param<F32, F32>(turn_accel, DEG2RAD(1), 0.01f, 1000000000.0f, ap,
                                             apsize, "turn_accel");
        }
        if (init)
        {
            turn_max_vel = 180.0f;
            auto_tweak::load_param<F32, F32>(turn_max_vel, DEG2RAD(1), 0.01f, 1000000000.0f, ap,
                                             apsize, "turn_max_vel");
        }
        if (init)
        {
            ground_y = -1.38f;
            auto_tweak::load_param<F32, F32>(ground_y, 1.0f, -1000000000.0f, 1000000000.0f, ap,
                                             apsize, "ground_y");
        }
        if (init)
        {
            ground_radius = 12.0f;
            auto_tweak::load_param<F32, F32>(ground_radius, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "ground_radius");
        }
        if (init)
        {
            hit_vel = 5.0f;
            auto_tweak::load_param<F32, F32>(hit_vel, 1.0f, 0.0f, 100000.0f, ap, apsize, "hit_vel");
        }
        if (init)
        {
            hit_max_dist = 5.0f;
            auto_tweak::load_param<F32, F32>(hit_max_dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "hit_max_dist");
        }
        if (init)
        {
            idle_time = 3.0f;
            auto_tweak::load_param<F32, F32>(idle_time, 1.0f, 0.0f, 10.0f, ap, apsize, "idle_time");
        }
        if (init)
        {
            min_arena_dist = 3.0f;
            auto_tweak::load_param<F32, F32>(min_arena_dist, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "min_arena_dist");
        }
        if (init)
        {
            help.fuse_dist = 8.0f;
            auto_tweak::load_param<F32, F32>(help.fuse_dist, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "help.fuse_dist");
        }
        if (init)
        {
            help.fuse_delay = 3.0f;
            auto_tweak::load_param<F32, F32>(help.fuse_delay, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "help.fuse_delay");
        }
        if (init)
        {
            follow.min_ang = 15.0f;
            auto_tweak::load_param<F32, F32>(follow.min_ang, DEG2RAD(1), 0.0f, 180.0f, ap, apsize,
                                             "follow.min_ang");
        }
        if (init)
        {
            follow.max_ang = 30.0f;
            auto_tweak::load_param<F32, F32>(follow.max_ang, DEG2RAD(1), 0.0f, 180.0f, ap, apsize,
                                             "follow.max_ang");
        }
        if (init)
        {
            follow.min_delay = 0.0f;
            auto_tweak::load_param<F32, F32>(follow.min_delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "follow.min_delay");
        }
        if (init)
        {
            follow.max_delay = 2.0f;
            auto_tweak::load_param<F32, F32>(follow.max_delay, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "follow.max_delay");
        }
        if (init)
        {
            mode_buddy.accel = xVec3::create(10.0f, 20.0f, 20.0f);
            auto_tweak::load_param<xVec3, S32>(mode_buddy.accel, 0, 0, 0, ap, apsize,
                                               "mode_buddy.accel");
        }
        if (init)
        {
            mode_buddy.max_vel = xVec3::create(20.0f, 10.0f, 20.0f);
            auto_tweak::load_param<xVec3, S32>(mode_buddy.max_vel, 0, 0, 0, ap, apsize,
                                               "mode_buddy.max_vel");
        }
        if (init)
        {
            mode_buddy.stun_duration = 1.0f;
            auto_tweak::load_param<F32, F32>(mode_buddy.stun_duration, 1.0f, 0.0f, 100.0f, ap,
                                             apsize, "mode_buddy.stun_duration");
        }
        if (init)
        {
            mode_buddy.obstruct_angle = 45.0f;
            auto_tweak::load_param<F32, F32>(mode_buddy.obstruct_angle, DEG2RAD(1), 0.0f, 90.0f,
                                             ap, apsize, "mode_buddy.obstruct_angle");
        }
        if (init)
        {
            mode_harass.accel = xVec3::create(10.0f, 5.0f, 10.0f);
            auto_tweak::load_param<xVec3, S32>(mode_harass.accel, 0, 0, 0, ap, apsize,
                                               "mode_harass.accel");
        }
        if (init)
        {
            mode_harass.max_vel = xVec3::create(20.0f, 10.0f, 10.0f);
            auto_tweak::load_param<xVec3, S32>(mode_harass.max_vel, 0, 0, 0, ap, apsize,
                                               "mode_harass.max_vel");
        }
        if (init)
        {
            mode_harass.stun_duration = 2.0f;
            auto_tweak::load_param<F32, F32>(mode_harass.stun_duration, 1.0f, 0.0f, 10.0f, ap,
                                             apsize, "mode_harass.stun_duration");
        }
        if (init)
        {
            hunt.height = 3.0f;
            auto_tweak::load_param<F32, F32>(hunt.height, 1.0f, -10.0f, 10.0f, ap, apsize,
                                             "hunt.height");
        }
        if (init)
        {
            hunt.radius = 5.0f;
            auto_tweak::load_param<F32, F32>(hunt.radius, 1.0f, 1.0f, 100.0f, ap, apsize,
                                             "hunt.radius");
        }
        if (init)
        {
            hunt.beam_interval = 3.0f;
            auto_tweak::load_param<F32, F32>(hunt.beam_interval, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "hunt.beam_interval");
        }
        if (init)
        {
            hunt.beam_duration = 3.0f;
            auto_tweak::load_param<F32, F32>(hunt.beam_duration, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "hunt.beam_duration");
        }
        if (init)
        {
            hunt.beam_dist = 7.5f;
            auto_tweak::load_param<F32, F32>(hunt.beam_dist, 1.0f, 1.0f, 100.0f, ap, apsize,
                                             "hunt.beam_dist");
        }
        if (init)
        {
            beam.rate = 6.0f;
            auto_tweak::load_param<F32, F32>(beam.rate, 1.0f, 0.01f, 100000.0f, ap, apsize,
                                             "beam.rate");
        }
        if (init)
        {
            beam.time_warm_up = 0.5f;
            auto_tweak::load_param<F32, F32>(beam.time_warm_up, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "beam.time_warm_up");
        }
        if (init)
        {
            beam.time_fire = 5.0f;
            auto_tweak::load_param<F32, F32>(beam.time_fire, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "beam.time_fire");
        }
        if (init)
        {
            beam.gun_tilt_min = -80.0f;
            auto_tweak::load_param<F32, F32>(beam.gun_tilt_min, DEG2RAD(1), -180.0f, 180.0f, ap,
                                             apsize, "beam.gun_tilt_min");
        }
        if (init)
        {
            beam.gun_tilt_max = 20.0f;
            auto_tweak::load_param<F32, F32>(beam.gun_tilt_max, DEG2RAD(1), -180.0f, 180.0f, ap,
                                             apsize, "beam.gun_tilt_max");
        }
        if (init)
        {
            beam.max_dist = 25.0f;
            auto_tweak::load_param<F32, F32>(beam.max_dist, 1.0f, 1.0f, 100.0f, ap, apsize,
                                             "beam.max_dist");
        }
        if (init)
        {
            beam.emit_dist = 0.5f;
            auto_tweak::load_param<F32, F32>(beam.emit_dist, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "beam.emit_dist");
        }
        if (init)
        {
            beam.fx.radius = 0.7f;
            auto_tweak::load_param<F32, F32>(beam.fx.radius, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "beam.fx.radius");
        }
        if (init)
        {
            beam.fx.length = 2.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.length, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "beam.fx.length");
        }
        if (init)
        {
            beam.fx.vel = 20.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.vel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "beam.fx.vel");
        }
        if (init)
        {
            beam.fx.fade_dist = 15.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.fade_dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "beam.fx.fade_dist");
        }
        if (init)
        {
            beam.fx.kill_dist = 20.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.kill_dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "beam.fx.kill_dist");
        }
        if (init)
        {
            beam.fx.safe_dist = 2.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.safe_dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "beam.fx.safe_dist");
        }
        if (init)
        {
            beam.fx.hit_radius = 0.2f;
            auto_tweak::load_param<F32, F32>(beam.fx.hit_radius, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "beam.fx.hit_radius");
        }
        if (init)
        {
            beam.fx.rand_ang = 6.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.rand_ang, DEG2RAD(1), 0.0f, 360.0f, ap,
                                             apsize, "beam.fx.rand_ang");
        }
        if (init)
        {
            beam.fx.scar_life = 3.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.scar_life, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "beam.fx.scar_life");
        }
        if (init)
        {
            beam.fx.bolt_uv[0].x = 0.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.bolt_uv[0].x, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "beam.fx.bolt_uv[0].x");
        }
        if (init)
        {
            beam.fx.bolt_uv[0].y = 0.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.bolt_uv[0].y, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "beam.fx.bolt_uv[0].y");
        }
        if (init)
        {
            beam.fx.bolt_uv[1].x = 1.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.bolt_uv[1].x, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "beam.fx.bolt_uv[1].x");
        }
        if (init)
        {
            beam.fx.bolt_uv[1].y = 1.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.bolt_uv[1].y, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "beam.fx.bolt_uv[1].y");
        }
        if (init)
        {
            beam.fx.hit_interval = 2;
            auto_tweak::load_param<S32, S32>(beam.fx.hit_interval, 1, 0, 100, ap, apsize,
                                             "beam.fx.hit_interval");
        }
        if (init)
        {
            beam.fx.damage = 1.0f;
            auto_tweak::load_param<F32, F32>(beam.fx.damage, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "beam.fx.damage");
        }
        if (init)
        {
            harass.safety_dist = 2.0f;
            auto_tweak::load_param<F32, F32>(harass.safety_dist, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "harass.safety_dist");
        }
        if (init)
        {
            harass.safety_height = -4.0f;
            auto_tweak::load_param<F32, F32>(harass.safety_height, 1.0f, -100.0f, 100.0f, ap,
                                             apsize, "harass.safety_height");
        }
        if (init)
        {
            harass.attack_dist = 5.0f;
            auto_tweak::load_param<F32, F32>(harass.attack_dist, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "harass.attack_dist");
        }
        if (init)
        {
            harass.attack_height = 2.0f;
            auto_tweak::load_param<F32, F32>(harass.attack_height, 1.0f, -100.0f, 100.0f, ap,
                                             apsize, "harass.attack_height");
        }
        if (init)
        {
            harass.stun_time = 6.0f;
            auto_tweak::load_param<F32, F32>(harass.stun_time, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "harass.stun_time");
        }
        if (init)
        {
            flank.duration = 2.0f;
            auto_tweak::load_param<F32, F32>(flank.duration, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "flank.duration");
        }
        if (init)
        {
            flank.accel = 5.0f;
            auto_tweak::load_param<F32, F32>(flank.accel, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "flank.accel");
        }
        if (init)
        {
            flank.max_vel = 20.0f;
            auto_tweak::load_param<F32, F32>(flank.max_vel, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "flank.max_vel");
        }
        if (init)
        {
            fall.accel = 4.0f;
            auto_tweak::load_param<F32, F32>(fall.accel, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "fall.accel");
        }
        if (init)
        {
            fall.max_vel = 50.0f;
            auto_tweak::load_param<F32, F32>(fall.max_vel, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "fall.max_vel");
        }
        if (init)
        {
            fall.dist = 30.0f;
            auto_tweak::load_param<F32, F32>(fall.dist, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "fall.dist");
        }
        if (init)
        {
            evade.duration = 6.0f;
            auto_tweak::load_param<F32, F32>(evade.duration, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "evade.duration");
        }
        if (init)
        {
            evade.move_delay_min = 1.0f;
            auto_tweak::load_param<F32, F32>(evade.move_delay_min, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "evade.move_delay_min");
        }
        if (init)
        {
            evade.move_delay_max = 1.5f;
            auto_tweak::load_param<F32, F32>(evade.move_delay_max, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "evade.move_delay_max");
        }
        if (init)
        {
            evade.accel = 5.0f;
            auto_tweak::load_param<F32, F32>(evade.accel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "evade.accel");
        }
        if (init)
        {
            evade.max_vel = 10.0f;
            auto_tweak::load_param<F32, F32>(evade.max_vel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "evade.max_vel");
        }
        if (init)
        {
            arena.center = V0;
            auto_tweak::load_param<xVec3, S32>(arena.center, 0, 0, 0, ap, apsize, "arena.center");
        }
        if (init)
        {
            arena.attack.radius = 14.0f;
            auto_tweak::load_param<F32, F32>(arena.attack.radius, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "arena.attack.radius");
        }
        if (init)
        {
            arena.attack.height = 11.0f;
            auto_tweak::load_param<F32, F32>(arena.attack.height, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "arena.attack.height");
        }
        if (init)
        {
            arena.safety.radius = 12.0f;
            auto_tweak::load_param<F32, F32>(arena.safety.radius, 1.0f, 0.01, 100.0f, ap, apsize,
                                             "arena.safety.radius");
        }
        if (init)
        {
            arena.safety.height = 14.0f;
            auto_tweak::load_param<F32, F32>(arena.safety.height, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "arena.safety.height");
        }
        if (init)
        {
            sound[SOUND_HOVER].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_HOVER].volume");
        }
        if (init)
        {
            sound[SOUND_HOVER].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HOVER].range_inner");
        }
        if (init)
        {
            sound[SOUND_HOVER].range_outer = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HOVER].range_outer");
        }
        if (init)
        {
            sound[SOUND_HOVER].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_HOVER].delay");
        }
        if (init)
        {
            sound[SOUND_HOVER].fade_time = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HOVER].fade_time, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HOVER].fade_time");
        }
        if (init)
        {
            sound[SOUND_HIT].volume = 0.5f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT].volume, 1.0f, 0.0f, 1.0f, ap, apsize,
                                             "sound[SOUND_HIT].volume");
        }
        if (init)
        {
            sound[SOUND_HIT].range_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HIT].range_inner");
        }
        if (init)
        {
            sound[SOUND_HIT].range_outer = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HIT].range_outer");
        }
        if (init)
        {
            sound[SOUND_HIT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_HIT].delay");
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].volume = 0.5f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FIRE].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_BOLT_FIRE].volume");
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FIRE].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_FIRE].range_inner");
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].range_outer = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FIRE].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_FIRE].range_outer");
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FIRE].delay, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_BOLT_FIRE].delay");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].volume = 0.2f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_BOLT_FLY].volume");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_FLY].range_inner");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].range_outer = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_FLY].range_outer");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].delay = 0.1f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_BOLT_FLY].delay");
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].fade_time = 0.1f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_FLY].fade_time, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_BOLT_FLY].fade_time");
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_HIT].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_BOLT_HIT].volume");
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_HIT].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_HIT].range_inner");
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].range_outer = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_HIT].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_BOLT_HIT].range_outer");
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_BOLT_HIT].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_BOLT_HIT].delay");
        }
        if (init)
        {
            sound[SOUND_CHARGE].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_CHARGE].volume");
        }
        if (init)
        {
            sound[SOUND_CHARGE].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_CHARGE].range_inner");
        }
        if (init)
        {
            sound[SOUND_CHARGE].range_outer = 20.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_CHARGE].range_outer");
        }
        if (init)
        {
            sound[SOUND_CHARGE].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHARGE].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_CHARGE].delay");
        }
        if (init)
        {
            sound[SOUND_HOVER].asset = sound_asset_ids[0][4];
            const sound_asset& asset = sound_assets[sound[SOUND_HOVER].asset];
            sound_data[SOUND_HOVER].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_HIT].asset = sound_asset_ids[1][3];
            const sound_asset& asset = sound_assets[sound[SOUND_HIT].asset];
            sound_data[SOUND_HIT].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].asset = sound_asset_ids[2][0];
            const sound_asset& asset = sound_assets[sound[SOUND_BOLT_FIRE].asset];
            sound_data[SOUND_BOLT_FIRE].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].asset = sound_asset_ids[3][3];
            const sound_asset& asset = sound_assets[sound[SOUND_BOLT_FLY].asset];
            sound_data[SOUND_BOLT_FLY].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].asset = sound_asset_ids[4][3];
            const sound_asset& asset = sound_assets[sound[SOUND_BOLT_HIT].asset];
            sound_data[SOUND_BOLT_HIT].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_CHARGE].asset = sound_asset_ids[5][3];
            const sound_asset& asset = sound_assets[sound[SOUND_CHARGE].asset];
            sound_data[SOUND_CHARGE].id = xStrHash(asset.name);
        }
    }

} // namespace

void zNPCBPlankton::ParseLinks()
{
    zNPCCommon::ParseLinks();

    memset(territory, 0, sizeof(territory));

    xLinkAsset* it = link;
    xLinkAsset* end = it + linkCount;

    for (; it != end; it++)
    {
        if (it->dstEvent == 0x133)
        {
            xBase* child = zSceneFindObject(it->dstAssetID);
            S32 index = (S32)it->param[0];

            if (index > 0 && index <= 8)
            {
                territory_data& t = territory[index - 1];

                if (t.origin == NULL)
                {
                    load_territory(index, *child);

                    if (t.origin == NULL || t.platform == NULL)
                    {
                        memset(&t, 0, sizeof(t));
                    }
                }
            }
        }
    }

    territory_size = 0;

    for (S32 i = 0; i < 8; i++)
    {
        if (territory[i].origin != NULL)
        {
            if (i != territory_size)
            {
                territory[territory_size] = territory[i];
            }

            territory_size++;
        }
    }
}

void zNPCBPlankton::SelfSetup()
{
    xBehaveMgr* bmgr = xBehaveMgr_GetSelf();
    this->psy_instinct = bmgr->Subscribe(this, NULL);
    xPsyche* psy = this->psy_instinct;
    psy->BrainBegin();
    for (S32 i = NPC_GOAL_BPLANKTONIDLE; i <= NPC_GOAL_BPLANKTONBOMB; i++)
    {
        psy->AddGoal(i, this);
    }
    psy->BrainEnd();
    psy->SetSafety(NPC_GOAL_BPLANKTONIDLE);
}

void zNPCBPlankton::Damage(en_NPC_DAMAGE_TYPE damtype, xBase* from, const xVec3* vec_hit)
{
    psy_instinct->GIDOfActive();

    switch (damtype)
    {
    case DMGTYP_ABOVE:
    case DMGTYP_BELOW:
    case DMGTYP_SIDE:
    case DMGTYP_HITBYTOSS:
    case DMGTYP_ROPE:
    case DMGTYP_CRUISEBUBBLE:
    case DMGTYP_PROJECTILE:
    case DMGTYP_BUBBOWL:
    {
        if (vec_hit != NULL)
        {
            impart_velocity(*vec_hit * tweak.hit_vel);
        }
        stun();
        break;
    }
    }
}

U32 zNPCBPlankton::AnimPick(S32 rawgoal, en_NPC_GOAL_SPOT gspot, xGoal* goal)
{
    U32 animId = 0;
    S32 index = 1;

    switch (rawgoal)
    {
    case NPC_GOAL_BPLANKTONIDLE:
    case NPC_GOAL_BPLANKTONATTACK:
    case NPC_GOAL_BPLANKTONAMBUSH:
    case NPC_GOAL_BPLANKTONFLANK:
    case NPC_GOAL_BPLANKTONEVADE:
    case NPC_GOAL_BPLANKTONHUNT:
    {
        index = 1;
        break;
    }
    case NPC_GOAL_BPLANKTONTAUNT:
    {
        index = 3;
        break;
    }
    case NPC_GOAL_BPLANKTONMOVE:
    {
        index = 0x42;
        break;
    }
    case NPC_GOAL_BPLANKTONSTUN:
    case NPC_GOAL_BPLANKTONFALL:
    case NPC_GOAL_BPLANKTONDIZZY:
    {
        index = 0x43;
        break;
    }
    case NPC_GOAL_BPLANKTONBEAM:
    {
        index = 0x46;
        break;
    }
    case NPC_GOAL_BPLANKTONWALL:
    {
        index = 0x49;
        break;
    }
    case NPC_GOAL_BPLANKTONMISSLE:
    {
        index = 0x4c;
        break;
    }
    case NPC_GOAL_BPLANKTONBOMB:
    {
        index = 0x4d;
        break;
    }

    default:
        index = 1;
        break;
    }

    if (index > -1)
    {
        return animId = g_hash_bossanim[index];
    }

    return animId;
}

S32 zNPCBPlankton::next_goal()
{
    if (mode == MODE_BUDDY)
    {
        if (flag.hunt)
        {
            return NPC_GOAL_BPLANKTONHUNT;
        }

        return crony_attacking() ? NPC_GOAL_BPLANKTONATTACK - 1 : NPC_GOAL_BPLANKTONATTACK;
    }

    return NPC_GOAL_BPLANKTONEVADE;
}

void zNPCBPlankton::refresh_orbit()
{
    if (mode == MODE_BUDDY)
    {
        if (flag.hunt)
        {
            const xVec3 oldcenter = orbit.center;

            orbit.center = *get_player_loc();
            orbit.center.y = tweak.arena.center.y + tweak.hunt.height;
            orbit.radius = tweak.hunt.radius;
            move.dest += orbit.center - oldcenter;
        }
        else if (flag.attacking)
        {
            orbit.center = tweak.arena.center;
            orbit.center.y += tweak.arena.attack.height;
            orbit.radius = tweak.arena.attack.radius;
        }
        else
        {
            orbit.center = tweak.arena.center;
            orbit.center.y += tweak.arena.safety.height;
            orbit.radius = tweak.arena.safety.radius;
        }
    }
    else
    {
        territory_data& t = territory[active_territory];
        xMovePointAsset& mp = *t.origin->asset;

        orbit.center = mp.pos;
        orbit.radius = mp.zoneRadius;

        if (flag.attacking)
        {
            orbit.radius += tweak.harass.attack_dist;
            orbit.center.y += tweak.harass.attack_height;
        }
        else
        {
            orbit.radius += tweak.harass.safety_dist;
            orbit.center.y += tweak.harass.safety_height;
        }
    }
}

void zNPCBPlankton::scan_cronies()
{
    st_XORDEREDARRAY* npclist = zNPCMgr_GetNPCList();

    crony = NULL;

    for (S32 i = 0; i < npclist->cnt; i++)
    {
        zNPCCommon* npc = (zNPCCommon*)npclist->list[i];

        if (npc->SelfType() == NPC_TYPE_BOSSBOBBY)
        {
            crony = (zNPCBoss*)npc;
            return;
        }
    }
}

namespace
{

    void update_move_orbit(xVec3& loc, zNPCBPlankton::move_info& move, const xVec3& center, F32 dt,
                           bool clamp);

    void set_yaw_matrix(xMat3x3& mat, F32 yaw)
    {
        F32 s = isin(yaw);
        F32 c = icos(yaw);

        mat.right.assign(c, 0.0f, -s);
        mat.up.assign(0.0f, 1.0f, 0.0f);
        mat.at.assign(s, 0.0f, c);
    }

    void update_move_accel(xVec3& loc, zNPCBPlankton::move_info& move, F32 dt)
    {
        xAccelMove(loc.x, move.vel.x, move.accel.x, dt, move.max_vel.x);
        xAccelMove(loc.y, move.vel.y, move.accel.y, dt, move.max_vel.y);
        xAccelMove(loc.z, move.vel.z, move.accel.z, dt, move.max_vel.z);
    }

    void update_move_stop(xVec3& loc, zNPCBPlankton::move_info& move, F32 dt)
    {
        xAccelStop(loc.x, move.vel.x, move.accel.x, dt);
        xAccelStop(loc.y, move.vel.y, move.accel.y, dt);
        xAccelStop(loc.z, move.vel.z, move.accel.z, dt);
    }

    void update_move_orbit(xVec3& loc, zNPCBPlankton::move_info& move, const xVec3& center, F32 dt,
                           bool xfree)
    {
        const xVec3 loc_pt = { loc.x, center.y, loc.z };
        const xVec3 dest_pt = { move.dest.x, center.y, move.dest.z };
        xVec3 loc_flat = loc_pt - center;
        xVec3 dest_flat = dest_pt - center;

        F32 loc_dist = loc_flat.length();
        F32 dest_dist = dest_flat.length();
        F32 loc_ang = xatan2(loc_flat.x, loc_flat.z);
        F32 dest_ang = xatan2(dest_flat.x, dest_flat.z);
        F32 dang = xrmod(PI + (dest_ang - loc_ang)) - PI;

        xVec3 target = { dang * dest_dist, move.dest.y - loc.y, dest_dist - loc_dist };
        xVec3 disp = { 0.0f, 0.0f, 0.0f };
        xVec3 dir;
        F32 ang = loc_ang;

        if (xfree)
        {
            xAccelMove(disp.x, move.vel.x, move.accel.x, dt, move.max_vel.x);
        }
        else
        {
            xAccelMove(disp.x, move.vel.x, move.accel.x, dt, target.x, move.max_vel.x);
        }
        xAccelMove(disp.y, move.vel.y, move.accel.y, dt, target.y, move.max_vel.y);
        xAccelMove(disp.z, move.vel.z, move.accel.z, dt, target.z, move.max_vel.z);

        if (xfeq0(loc_dist))
        {
            dir.assign(1.0f, 0.0f, 0.0f);
        }
        else
        {
            F32 inv = 1.0f / loc_dist;

            dir = loc_flat * inv;
            ang = disp.x * inv + ang;
        }

        loc.x = dir.x * disp.z + (loc_dist * isin(ang) + center.x);
        loc.y = loc.y + disp.y;
        loc.z = dir.z * disp.z + (loc_dist * icos(ang) + center.z);
    }

} // namespace

void zNPCBPlankton::update_turn(F32 dt)
{
    location();

    xVec2 at = { model->Mat->at.x, model->Mat->at.z };

    if (turning())
    {
        F32 cur_yaw = xatan2(at.x, at.y);
        F32 target_yaw = xatan2(turn.dir.x, turn.dir.y);
        F32 diff = target_yaw - cur_yaw;

        if (diff > PI)
        {
            diff -= 6.2831855f;
        }
        else if (diff < -PI)
        {
            diff += 6.2831855f;
        }

        F32 new_yaw = cur_yaw;
        xAccelMove(new_yaw, turn.vel, turn.accel, dt, new_yaw + diff, turn.max_vel);
        set_yaw_matrix(frame->mat, new_yaw);
    }
}

void zNPCBPlankton::update_move(F32 dt)
{
    switch (flag.move)
    {
    case MOVE_ACCEL:
    {
        update_move_accel(frame->mat.pos, move, dt);
        break;
    }
    case MOVE_STOP:
    {
        update_move_stop(frame->mat.pos, move, dt);
        break;
    }
    case MOVE_ORBIT:
    {
        update_move_orbit(frame->mat.pos, move, orbit.center, dt, false);
        break;
    }
    }
}

namespace
{
    xVec3 world_to_ring_vel(const xVec3& world_vel, const xVec3& loc, const xVec3& center)
    {
        xVec2 vel2 = { world_vel.x, world_vel.z };
        xVec2 offset = { loc.x - center.x, loc.z - center.z };
        F32 dist = offset.length();
        xVec2 dir = offset / dist;
        xVec2 perp = { -dir.y, dir.x };

        F32 radial = vel2.dot(dir);
        F32 tangential = vel2.dot(perp);

        return xVec3::create(tangential, world_vel.y, radial);
    }

    xVec3 ring_to_world_vel(const xVec3& ring_vel, const xVec3& loc, const xVec3& center)
    {
        xVec2 offset = { loc.x - center.x, loc.z - center.z };
        xVec2 dir = offset.normal();
        xVec2 perp = { -dir.y, dir.x };

        xVec3 out = { ring_vel.x * perp.x + ring_vel.z * perp.y, ring_vel.y,
                      ring_vel.x * dir.x + ring_vel.z * dir.y };

        return out;
    }
} // namespace

void zNPCBPlankton::reset_territories()
{
    territory_data* t = territory;
    territory_data* t_end = t + territory_size;

    for (; t != t_end; t++)
    {
        t->fuse_destroyed = 0;
        t->fuse_detected = 0;
        t->fuse_detect_time = 0.0f;
    }
}

void zNPCBPlankton::update_dialog(F32 dt)
{
    if (mode != MODE_BUDDY)
    {
        if (globals.player.Health < old_player_health && old_player_health != 0)
        {
            say(7, 0, false);
        }
        old_player_health = globals.player.Health;

        xVec3* player_loc = get_player_loc();

        S32 i = active_territory - 1;
        if (i < 0)
        {
            i = 0;
        }

        for (; i <= active_territory; i++)
        {
            territory_data& t = territory[i];

            if (t.fuse != NULL && !t.fuse_destroyed && zEntDestructObj_isDestroyed(t.fuse))
            {
                say(2, 0, false);
                t.fuse_destroyed = 1;
                return;
            }
        }

        for (S32 i = 0; i < active_territory; i++)
        {
            territory_data& t = territory[i];

            if (t.crony_size > 0 && t.fuse != NULL && !t.fuse_detected &&
                !zEntDestructObj_isDestroyed(t.fuse))
            {
                xVec3 diff = *player_loc - reinterpret_cast<xVec3&>(t.fuse->model->Mat->pos);

                if (diff.length2() <= tweak.help.fuse_dist * tweak.help.fuse_dist)
                {
                    t.fuse_detect_time = t.fuse_detect_time + dt;

                    if (t.fuse_detect_time >= tweak.help.fuse_delay)
                    {
                        t.fuse_detected = 1;
                        say(1, 0, false);
                        return;
                    }
                }
                else
                {
                    t.fuse_detect_time = 0.0f;
                }
            }
        }
    }
}

void zNPCBPlankton::update_animation(F32)
{
}

void zNPCBPlankton::update_follow(F32 dt)
{
    switch (flag.follow)
    {
    case FOLLOW_PLAYER:
    {
        update_follow_player(dt);
        break;
    }
    case FOLLOW_CAMERA:
    {
        update_follow_camera(dt);
        break;
    }
    }
}

void zNPCBPlankton::update_follow_player(F32 dt)
{
    follow.delay = follow.delay + dt;

    if (follow.delay >= follow.max_delay ||
        xabs(orbit_yaw_offset(move.dest, *get_player_loc())) > tweak.follow.max_ang)
    {
        move.dest = player_orbit();
        move.dest = random_orbit(move.dest, 0.0f, tweak.follow.min_ang);
        follow.delay = 0.0f;

        F32 min_delay = tweak.follow.min_delay;
        F32 max_delay = tweak.follow.max_delay;

        follow.max_delay = min_delay + (max_delay - min_delay) * xurand();
    }
}

void zNPCBPlankton::update_follow_camera(F32 dt)
{
    const xVec3 target = orbit.center + globals.camera.mat.at * orbit.radius;

    follow.delay = follow.delay + dt;

    if (follow.delay >= follow.max_delay ||
        std::fabsf(orbit_yaw_offset(move.dest, target)) > tweak.follow.max_ang)
    {
        move.dest = random_orbit(target, 0.0f, tweak.follow.min_ang);
        follow.delay = 0.0f;
        F32 min_delay = tweak.follow.min_delay;
        F32 max_delay = tweak.follow.max_delay;

        follow.max_delay = min_delay + (max_delay - min_delay) * xurand();
    }
}

// Three more unreferenced all-zero xVec3 templates, ahead of the { -1, 0, 0 }
// one zNPCBPlankton::update_aim_gun creates.
void __deadstripped_zNPCTypeBossPlankton_aim()
{
    const char _2236[0x0C] = {};
    const char _2239[0x0C] = {};
    const char _2244[0x0C] = {};
}

void zNPCBPlankton::update_aim_gun(F32)
{
    if (flag.aim_gun)
    {
        xVec3 gun_loc = xModelGetBoneLocation(*model, 0x15);

        xVec3* player_loc = get_player_loc();
        xVec3 to_player = *player_loc - gun_loc;

        F32 dist_xz = xsqrt(to_player.x * to_player.x + to_player.z * to_player.z);
        F32 pitch = xatan2(to_player.y, dist_xz);
        pitch = xrmod(PI + pitch) - PI;
        pitch = range_limit<F32>(pitch, tweak.beam.gun_tilt_min, tweak.beam.gun_tilt_max);

        xVec3 axis = { -1.0f, 0.0f, 0.0f };
        xQuatFromAxisAngle(&gun_tilt, &axis, pitch);
    }
}

bool zNPCBPlankton::check_player_damage()
{
    bool damage = false;

    return globals.player.cheat_mode ? false : damage;
}

F32 zNPCBPlankton::orbit_yaw_offset(const xVec3& p0, const xVec3& p1) const
{
    xVec2 center = { orbit.center.x, orbit.center.z };
    xVec2 loc0 = { p0.x, p0.z };
    xVec2 loc1 = { p1.x, p1.z };
    xVec2 v0 = loc0 - center;
    xVec2 v1 = loc1 - center;

    F32 cross = v0.y * v1.x - v0.x * v1.y;

    return xrmod(PI + xatan2(cross, v0.dot(v1))) - PI;
}

void zNPCBPlankton::load_territory(S32 index, xBase& child)
{
    territory_data& t = territory[index - 1];

    switch (child.baseType)
    {
    case eBaseTypeGroup:
    {
        U32 i = 0;
        U32 size = xGroupGetCount((xGroup*)&child);

        for (; i < size; i++)
        {
            xBase* entry = xGroupGetItemPtr((xGroup*)&child, i);
            load_territory(index, *entry);
        }

        break;
    }
    case eBaseTypeMovePoint:
    {
        t.origin = (zMovePoint*)&child;
        break;
    }
    case eBaseTypeNPC:
    {
        if (t.crony_size < 8)
        {
            t.crony[t.crony_size] = (zNPCCommon*)&child;
            t.crony_size++;
        }

        break;
    }
    case eBaseTypeTimer:
    {
        t.timer = (xTimer*)&child;
        break;
    }
    case eBaseTypeDestructObj:
    {
        t.fuse = (zEntDestructObj*)&child;
        break;
    }
    default:
    {
        if (xEntValidType(child.baseType))
        {
            t.platform = (xEnt*)&child;
        }

        break;
    }
    }
}

void zNPCBPlankton::init_beam()
{
    beam.init(31, "Plankton\'s Beam");
    beam.set_texture("plankton_laser_bolt");
    beam.cfg = tweak.beam.fx;
    beam.refresh_config();

    beam_ring.init(127, "Plankton\'s Beam Rings");
    beam_ring.set_curve(beam_ring_curve, 2);
    beam_ring.set_texture("bubble");
    beam_ring.set_default_config();
    beam_ring.cfg.flags = 0;
    beam_ring.cfg.life_time = 0.25f;
    beam_ring.cfg.blend_src = 5;
    beam_ring.cfg.blend_dst = 2;
    beam_ring.refresh_config();

    beam_glow.init(7, "Plankton\'s Beam Glow");
    beam_glow.set_curve(beam_glow_curve, 3);
    beam_glow.set_texture("fx_firework");
    beam_glow.set_default_config();
    beam_glow.cfg.flags = 0;
    beam_glow.cfg.life_time = 0.25f;
    beam_glow.cfg.blend_src = 5;
    beam_glow.cfg.blend_dst = 2;
    beam_glow.refresh_config();
}

void zNPCBPlankton::setup_beam()
{
    beam_launch_effect[0].callback.fp = play_beam_fire_sound;
    beam_launch_effect[0].callback.context = this;
    beam_launch_effect[1].callback.fp = play_beam_fly_sound;
    beam_launch_effect[1].callback.context = this;
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_LAUNCH, beam_launch_effect, 2);

    beam_head_effect[0].decal = &beam_ring;
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_HEAD, beam_head_effect, 1);

    beam_impact_effect[0].par = zParEmitterFind("PAREMIT_BPLANK_SPARKS");
    beam_impact_effect[1].callback.fp = kill_beam_fly_sound;
    beam_impact_effect[1].callback.context = this;
    beam_impact_effect[2].callback.fp = play_beam_hit_sound;
    beam_impact_effect[2].callback.context = this;
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_IMPACT, beam_impact_effect, 3);

    beam_death_effect[0].par = zParEmitterFind("PAREMIT_BPLANK_PLASMA");
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_DEATH, beam_death_effect, 1);

    beam_kill_effect[0].callback.fp = kill_beam_fly_sound;
    beam_kill_effect[0].callback.context = this;
    beam.attach_effects(xLaserBoltEmitter::FX_WHEN_KILL, beam_kill_effect, 1);

    beam_charge = zParEmitterFind("PAREMIT_BPLANK_CHARGE");
}

void zNPCBPlankton::reset_beam()
{
    beam.reset();
}

void zNPCBPlankton::vanish()
{
    flags = flags & 0xfe;
    flags = flags | 0x40;
    pflags = 0;
    moreFlags = 0;
    chkby = 0;
    penby = 0;
    flags2.flg_colCheck = 0;
    flags2.flg_penCheck = 0;
    kill_sound(NULL);
}

void zNPCBPlankton::reappear()
{
    flags = flags | 1;
    flags = flags & 0xbf;
    pflags = 0;
    moreFlags = 16;
    chkby = 16;
    penby = 16;
    flags2.flg_colCheck = 0;
    flags2.flg_penCheck = 0;
    play_sound(0, (xVec3*)&bound.pad[3], 1.0f);
}

xVec3 zNPCBPlankton::random_orbit(const xVec3& loc, F32 min_ang, F32 max_ang) const
{
    xVec3 diff = loc - orbit.center;
    F32 ang;

    F32 d2 = diff.length2();
    if (xfeq0(d2))
    {
        ang = 0.0f;
    }
    else
    {
        F32 dist = xsqrt(d2);
        F32 inv = 1.0f / dist;
        ang = xatan2(diff.x * inv, diff.z * inv);
    }

    if (max_ang > PI)
    {
        max_ang = PI;
    }

    F32 delta = (max_ang - min_ang) * xurand() + min_ang;
    if ((xrand() >> 13) & 1)
    {
        delta *= -1.0f;
    }

    F32 yaw = ang + delta;

    xVec3 out = orbit.center;
    out.x += orbit.radius * isin(yaw);
    out.z += orbit.radius * icos(yaw);
    return out;
}

xVec3 zNPCBPlankton::player_orbit() const
{
    F32 offset = std::fabsf(orbit_yaw_offset(location(), *get_player_loc()));
    F32 t = xAccelMoveTime(orbit.radius * offset, move.accel.x, 1000000000.0f, move.max_vel.x);

    xVec3 predicted;
    zEntPlayer_PredictPos(&predicted, t, 1.0f, 1);

    xVec2 center = { orbit.center.x, orbit.center.z };
    xVec2 ppos = { predicted.x, predicted.z };
    xVec2 diff = ppos - center;
    F32 d2 = diff.length2();

    if (d2 <= 0.001f)
    {
        return location();
    }

    xVec2 dir = diff * (1.0f / xsqrt(d2));
    xVec2 pos = center + dir * orbit.radius;

    return xVec3::create(pos.x, orbit.center.y, pos.y);
}

U8 zNPCBPlankton::crony_attacking() const
{
    if (crony == NULL)
    {
        return 0;
    }

    return crony->AttackTimeLeft() > 0.0f;
}

void zNPCBPlankton::stun()
{
    S32 gid = psy_instinct->GIDOfActive();

    if (gid == NPC_GOAL_BPLANKTONSTUN || gid == NPC_GOAL_BPLANKTONFALL ||
        gid == NPC_GOAL_BPLANKTONDIZZY)
    {
        return;
    }

    play_sound(SOUND_HIT, (xVec3*)&bound.pad[3], 1.0f);

    if (mode == MODE_BUDDY)
    {
        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONSTUN, 1);
    }
    else
    {
        territory_data& t = territory[active_territory];

        if (t.timer != NULL)
        {
            zEntEvent(this, t.timer, 0x12);
        }

        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONFALL, 1);

        S32 count = 0;
        for (S32 i = 0; i < territory_size; i++)
        {
            if (territory[i].fuse != NULL && !zEntDestructObj_isDestroyed(territory[i].fuse))
            {
                count++;
            }
        }

        switch (count)
        {
        case 1:
        {
            say(6, 0, false);
            break;
        }
        case 2:
        {
            say(5, 0, false);
            break;
        }
        case 3:
        {
            say(4, 0, false);
            break;
        }
        default:
        {
            say(3, 0, false);
            break;
        }
        }
    }
}

U8 zNPCBPlankton::cronies_dead() const
{
    const territory_data& t = territory[active_territory];
    zNPCCommon* const* it = t.crony;
    zNPCCommon* const* it_end = it + t.crony_size;

    for (; it != it_end; it++)
    {
        if ((*it)->IsAlive())
        {
            return 0;
        }
    }

    return 1;
}

void zNPCBPlankton::impart_velocity(const xVec3& vel)
{
    switch (flag.move)
    {
    case MOVE_ORBIT:
    {
        xVec3 add = world_to_ring_vel(vel, location(), orbit.center);

        add.y = 0.0f;

        const xVec2 diff = { location().x - orbit.center.x, location().z - orbit.center.z };
        const F32 max_dist = orbit.radius + tweak.hit_max_dist;

        if (diff.length2() > max_dist * max_dist)
        {
            add.z = 0.0f;
        }

        move.vel += add;
        break;
    }
    case MOVE_NONE:
    case MOVE_ACCEL:
    case MOVE_STOP:
    default:
    {
        move.vel += vel;
        break;
    }
    }
}

void zNPCBPlankton::next_territory()
{
    if (have_cronies())
    {
        active_territory++;

        if (active_territory >= territory_size)
        {
            active_territory = territory_size - 1;
        }
    }
}

U8 zNPCBPlankton::have_cronies() const
{
    const territory_data& t = territory[active_territory];

    return t.crony_size > 0;
}

U8 zNPCBPlankton::move_to_player_territory()
{
    xEntCollis* coll = globals.player.ent.collis;

    if (!(coll->colls[0].flags & 1) || coll->colls[0].optr == NULL)
    {
        return 0;
    }

    xEnt* platform = (xEnt*)coll->colls[0].optr;

    for (S32 i = 0; i < territory_size; i++)
    {
        if (!(territory[i].crony_size > 0) && platform == territory[i].platform)
        {
            active_territory = i;
            return 1;
        }
    }

    return 0;
}

U8 zNPCBPlankton::player_left_territory() const
{
    const territory_data& t = territory[active_territory];
    const xCollis& coll = globals.player.ent.collis->colls[0];
    xEnt* platform = (xEnt*)coll.optr;

    if (t.crony_size > 0 || t.platform == platform || !(coll.flags & 1) || platform == NULL)
    {
        return 0;
    }

    for (S32 i = 0; i < territory_size; i++)
    {
        const territory_data& t = territory[i];

        if (!(t.crony_size > 0) && platform == t.platform && i != active_territory)
        {
            return 1;
        }
    }

    return 0;
}

void zNPCBPlankton::say(S32 which, S32 flags, bool pair)
{
    if (newsfish != NULL)
    {
        if (pair)
        {
            newsfish->say(say_set[which].list[0], 1);
            newsfish->say(say_set[which].list[1], 2);
        }
        else
        {
            newsfish->say(say_set[which].list, say_set[which].size, flags, -1);
        }
    }
}

void zNPCBPlankton::sickum()
{
    if (psy_instinct->GIDOfActive() != NPC_GOAL_BPLANKTONHUNT)
    {
        flag.hunt = true;
        psy_instinct->GoalSet(NPC_GOAL_BPLANKTONHUNT, 1);
    }
}

void zNPCBPlankton::here_boy()
{
    flag.hunt = 0;
}

void zNPCBPlankton::follow_player()
{
    flag.follow = FOLLOW_PLAYER;
    follow.delay = follow.max_delay = 0.0f;
    flag.move = MOVE_ORBIT;
}

void zNPCBPlankton::follow_camera()
{
    flag.follow = FOLLOW_CAMERA;
    follow.delay = follow.max_delay = 0.0f;
    flag.move = MOVE_ORBIT;
}

void zNPCBPlankton::reset_speed()
{
    turn.accel = tweak.turn_accel;
    turn.max_vel = tweak.turn_max_vel;

    if (mode == MODE_BUDDY)
    {
        move.accel = tweak.mode_buddy.accel;
        move.max_vel = tweak.mode_buddy.max_vel;
    }
    else
    {
        move.accel = tweak.mode_harass.accel;
        move.max_vel = tweak.mode_harass.max_vel;
    }
}

void zNPCBPlankton::halt(F32 accel)
{
    flag.follow = FOLLOW_NONE;

    if (flag.move == MOVE_ORBIT)
    {
        move.vel = ring_to_world_vel(move.vel, location(), orbit.center);
    }

    flag.move = MOVE_STOP;

    F32 ax;
    if (move.vel.x < 0.0f)
    {
        ax = accel;
    }
    else
    {
        ax = -accel;
    }
    move.accel.x = ax;

    F32 ay;
    if (move.vel.y < 0.0f)
    {
        ay = accel;
    }
    else
    {
        ay = -accel;
    }
    move.accel.y = ay;

    F32 az;
    if (move.vel.z < 0.0f)
    {
        az = accel;
    }
    else
    {
        az = -accel;
    }
    move.accel.z = az;
}

void zNPCBPlankton::fall(F32 accel, F32 max_vel)
{
    flag.follow = FOLLOW_NONE;

    if (flag.move == MOVE_ORBIT)
    {
        move.vel = ring_to_world_vel(move.vel, location(), orbit.center);
    }

    flag.move = MOVE_ACCEL;
    move.accel.assign(accel, -accel, accel);
    move.max_vel.assign(0.0f, max_vel, 0.0f);
}

void zNPCBPlankton::aim_gun(xAnimPlay* play, xQuat* q, xVec3* v, S32 count)
{
    zNPCBPlankton* npc = (zNPCBPlankton*)(xEnt*)play->Object;

    if (npc->flag.aim_gun)
    {
        q[21] = npc->gun_tilt;
    }
}

xFactoryInst* zNPCGoalBPlanktonIdle::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonIdle(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonIdle::Enter(F32 dt, void* updCtxt)
{
    F32 yaw;
    F32 offset;

    owner.reappear();
    owner.flag.attacking = false;
    owner.refresh_orbit();
    owner.reset_speed();
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    get_yaw(yaw, offset);
    apply_yaw(yaw);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonIdle::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    F32 yaw;
    F32 offset;

    if (!owner.crony_attacking())
    {
        owner.take_control();
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONATTACK;
    }

    get_yaw(yaw, offset);

    if (owner.follow.delay >= owner.follow.max_delay || xabs(offset) > tweak.follow.max_ang)
    {
        apply_yaw(yaw);
    }

    return 0;
}

S32 zNPCGoalBPlanktonIdle::Exit(F32 dt, void* ctxt)
{
    owner.refresh_orbit();
    return xGoal::Exit(dt, ctxt);
}

void zNPCGoalBPlanktonIdle::get_yaw(F32& optimal, F32& dist) const
{
    F32 player_yaw = owner.get_orbit_yaw(*get_player_loc());
    F32 cur_yaw = owner.get_orbit_yaw(owner.location());
    F32 wrapped = xrmod(PI + (cur_yaw - player_yaw)) - PI;

    optimal = tweak.mode_buddy.obstruct_angle + tweak.follow.min_ang;
    if (wrapped < 0.0f)
    {
        optimal = -optimal;
    }
    optimal = optimal + player_yaw;

    dist = xrmod(PI + (optimal - cur_yaw)) - PI;
}

void zNPCGoalBPlanktonIdle::apply_yaw(F32 yaw)
{
    F32 offset = tweak.follow.min_ang * (2.0f * xurand() - 1.0f);
    F32 y = yaw + offset;

    owner.move.dest = owner.orbit.center;
    owner.move.dest.x = owner.move.dest.x + owner.orbit.radius * isin(y);
    owner.move.dest.z = owner.move.dest.z + owner.orbit.radius * icos(y);

    owner.follow.delay = 0.0f;

    F32 min_delay = tweak.follow.min_delay;
    F32 max_delay = tweak.follow.max_delay;

    owner.follow.max_delay = min_delay + (max_delay - min_delay) * xurand();
}

xFactoryInst* zNPCGoalBPlanktonAttack::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonAttack(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonAttack::Enter(F32 dt, void* updCtxt)
{
    owner.reappear();
    owner.flag.attacking = true;
    owner.refresh_orbit();
    owner.follow_player();
    owner.delay = 0.0f;
    owner.face_player();
    owner.reset_speed();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonAttack::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonAttack::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (owner.crony_attacking())
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONIDLE;
    }

    if (owner.delay >= tweak.idle_time)
    {
        owner.beam_duration = tweak.beam.time_fire;
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONBEAM;
    }

    return 0;
}

xFactoryInst* zNPCGoalBPlanktonAmbush::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonAmbush(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonAmbush::Enter(F32 dt, void* ctxt)
{
    owner.flag.attacking = false;
    owner.AnimSetState(g_hash_bossanim[ANIM_Idle01], 0.0f);
    owner.refresh_orbit();
    owner.halt(FLOAT_MAX);
    owner.vanish();
    owner.set_location(owner.random_orbit(*get_player_loc(), 0.0f, DEG2RAD(45)));

    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonAmbush::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

S32 zNPCGoalBPlanktonAmbush::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (owner.have_cronies())
    {
        if (owner.cronies_dead())
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BPLANKTONFLANK;
        }
    }
    else
    {
        owner.ambush_delay = owner.ambush_delay - dt;

        if (owner.ambush_delay <= 0.0f && owner.move_to_player_territory())
        {
            owner.refresh_orbit();
            owner.set_location(owner.random_orbit(*get_player_loc(), 0.0f, DEG2RAD(45)));

            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BPLANKTONFLANK;
        }
    }

    return 0;
}

xFactoryInst* zNPCGoalBPlanktonFlank::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonFlank(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonFlank::Enter(F32 dt, void* updCtxt)
{
    owner.reappear();
    owner.flag.attacking = true;

    const xVec3 target = owner.orbit.center + globals.camera.mat.at * owner.orbit.radius;
    owner.set_location(owner.random_orbit(target, 0.0f, tweak.follow.min_ang));

    owner.refresh_orbit();
    owner.follow_camera();
    owner.reset_speed();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonFlank::Exit(F32 dt, void* updCtxt)
{
    owner.reset_speed();
    owner.follow_player();
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonFlank::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    xVec3& loc = owner.location();

    if (xabs(loc.y - owner.orbit.center.y) <= 0.1f)
    {
        owner.beam_duration = tweak.beam.time_fire;
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONBEAM;
    }

    return 0;
}

xFactoryInst* zNPCGoalBPlanktonEvade::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonEvade(who, *(zNPCBPlankton*)info);
}

namespace
{
    xVec3 ring_to_world_loc(const xVec3& ring, const xVec3& center);
    xVec3 world_to_ring_loc(const xVec3& loc, const xVec3& center);
} // namespace

S32 zNPCGoalBPlanktonEvade::Enter(F32 dt, void* updCtxt)
{
    owner.reappear();
    owner.flag.attacking = true;
    owner.reset_speed();
    owner.face_player();
    owner.flag.move = zNPCBPlankton::MOVE_ORBIT;
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    owner.reset_speed();
    owner.move.accel.x = tweak.evade.accel;
    owner.move.max_vel.x = tweak.evade.max_vel;
    owner.delay = 0.0f;
    evade_delay = owner.delay + tweak.evade.move_delay_min +
                  (tweak.evade.move_delay_max - tweak.evade.move_delay_min) * xurand();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonEvade::Exit(F32 dt, void* updCtxt)
{
    owner.reset_speed();
    owner.halt(1e38f);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonEvade::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (owner.delay >= tweak.evade.duration)
    {
        owner.beam_duration = tweak.beam.time_fire;
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONBEAM;
    }

    if (owner.delay >= evade_delay)
    {
        xVec3 ring_loc = world_to_ring_loc(owner.location(), owner.orbit.center);

        if (owner.move.vel.x < 0.0f)
        {
            ring_loc.x = (PI / 2) * ring_loc.z + ring_loc.x;
        }
        else
        {
            ring_loc.x = -((PI / 2) * ring_loc.z - ring_loc.x);
        }

        ring_loc.y = 0.0f;
        ring_loc.z = owner.orbit.radius;

        owner.move.dest = ring_to_world_loc(ring_loc, owner.orbit.center);

        evade_delay = owner.delay + tweak.evade.move_delay_min +
                      (tweak.evade.move_delay_max - tweak.evade.move_delay_min) * xurand();
    }

    return 0;
}

namespace
{
    xVec3 ring_to_world_loc(const xVec3& ring, const xVec3& center)
    {
        if (xfeq0(ring.z))
        {
            return xVec3::create(center.x, ring.y + center.y, center.z);
        }

        F32 ang = ring.x / ring.z;

        return xVec3::create(ring.z * isin(ang) + center.x, center.y + ring.y,
                             ring.z * icos(ang) + center.z);
    }

    xVec3 world_to_ring_loc(const xVec3& loc, const xVec3& center)
    {
        xVec2 offset = { loc.x - center.x, loc.z - center.z };
        F32 dist = offset.length();
        xVec3 out = { dist * xatan2(offset.x, offset.y), loc.y - center.y, dist };

        return out;
    }
} // namespace

// The last unreferenced all-zero xVec3 template, after the one
// world_to_ring_loc creates.
void __deadstripped_zNPCTypeBossPlankton_tail()
{
    const char _2634[0x0C] = {};
}

xFactoryInst* zNPCGoalBPlanktonHunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonHunt(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonHunt::Enter(F32 dt, void* updCtxt)
{
    owner.reappear();
    get_player_loc();
    owner.flag.attacking = true;
    owner.delay = 0.0f;
    owner.reset_speed();
    owner.refresh_orbit();
    owner.follow_camera();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonHunt::Exit(F32 dt, void* updCtxt)
{
    owner.refresh_orbit();
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonHunt::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (!owner.flag.hunt)
    {
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }

    owner.refresh_orbit();

    if ((*get_player_loc() - owner.location()).length2() >=
        tweak.hunt.beam_dist * tweak.hunt.beam_dist)
    {
        return 0;
    }

    xVec3 pdiff = *get_player_loc() - player_loc;
    player_loc = *get_player_loc();

    if (owner.delay >= tweak.hunt.beam_interval || pdiff.length2() <= 0.0001f)
    {
        owner.beam_duration = tweak.hunt.beam_duration;
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONBEAM;
    }

    return 0;
}

xFactoryInst* zNPCGoalBPlanktonTaunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonTaunt(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonTaunt::Enter(F32 dt, void* updCtxt)
{
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonTaunt::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonTaunt::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

xFactoryInst* zNPCGoalBPlanktonMove::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonMove(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonMove::Enter(F32 dt, void* updCtxt)
{
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonMove::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonMove::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

xFactoryInst* zNPCGoalBPlanktonStun::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonStun(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonStun::Enter(F32 dt, void* updCtxt)
{
    owner.reappear();
    owner.delay = 0.0f;
    owner.flag.follow = owner.FOLLOW_NONE;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonStun::Exit(F32 dt, void* updCtxt)
{
    owner.give_control();
    owner.flag.follow = zNPCBPlankton::FOLLOW_PLAYER;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonStun::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    xAnimState* state = owner.AnimCurState();

    if (state->ID == g_hash_bossanim[ANIM_stun_loop])
    {
        if (owner.delay >= owner.stun_duration)
        {
            owner.AnimStart(g_hash_bossanim[ANIM_stun_end], 0);
        }
    }
    else
    {
        bool at_end = (state->ID == g_hash_bossanim[ANIM_stun_end]);

        if ((state->ID != g_hash_bossanim[ANIM_stun_begin] && !at_end) ||
            (at_end && dt > owner.AnimTimeRemain(NULL)))
        {
            *trantype = GOAL_TRAN_SET;
            return owner.next_goal();
        }
    }

    return 0;
}

xFactoryInst* zNPCGoalBPlanktonFall::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonFall(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonFall::Enter(F32 dt, void* updCtxt)
{
    owner.reappear();
    owner.delay = 0.0f;
    owner.flag.follow = zNPCBPlankton::FOLLOW_NONE;
    owner.fall(tweak.fall.accel, tweak.fall.max_vel);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonFall::Exit(F32 dt, void* updCtxt)
{
    owner.flag.follow = zNPCBPlankton::FOLLOW_PLAYER;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonFall::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (owner.orbit.center.y - owner.location().y >= tweak.fall.dist)
    {
        owner.next_territory();
        owner.ambush_delay = tweak.harass.stun_time;
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BPLANKTONAMBUSH;
    }

    return 0;
}

xFactoryInst* zNPCGoalBPlanktonDizzy::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonDizzy(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonDizzy::Enter(F32 dt, void* updCtxt)
{
    owner.give_control();
    owner.delay = 0.0f;
    owner.flag.follow = owner.FOLLOW_NONE;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonDizzy::Exit(F32 dt, void* updCtxt)
{
    owner.flag.follow = zNPCBPlankton::FOLLOW_PLAYER;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonDizzy::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    xAnimState* state = owner.AnimCurState();

    if (state->ID == g_hash_bossanim[ANIM_stun_loop])
    {
        if (owner.delay >= owner.stun_duration && dt > owner.AnimTimeRemain(NULL))
        {
            owner.AnimStart(g_hash_bossanim[ANIM_stun_end], 0);
        }
    }
    else
    {
        bool at_end = (state->ID == g_hash_bossanim[ANIM_stun_end]);

        if ((state->ID != g_hash_bossanim[ANIM_stun_begin] && !at_end) ||
            (at_end && dt > owner.AnimTimeRemain(NULL)))
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BPLANKTONIDLE;
        }
    }

    return 0;
}

xFactoryInst* zNPCGoalBPlanktonBeam::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonBeam(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonBeam::Enter(F32 dt, void* updCtxt)
{
    owner.reappear();
    substate = SS_WARM_UP;
    owner.delay = 0.0f;
    emitted = 0;
    owner.flag.aim_gun = true;
    owner.flag.follow = owner.FOLLOW_NONE;
    owner.enable_emitter(*owner.beam_charge);
    play_sound(5, (xVec3*)&owner.bound.pad[3], 1.0f); // dunno how to get this to call properly
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonBeam::Exit(F32 dt, void* updCtxt)
{
    owner.flag.aim_gun = false;
    owner.flag.follow = zNPCBPlankton::FOLLOW_PLAYER;
    owner.disable_emitter(*owner.beam_charge);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonBeam::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    switch (substate)
    {
    case SS_WARM_UP:
    {
        update_warm_up(dt);
        break;
    }
    case SS_FIRE:
    {
        update_fire(dt);
        break;
    }
    case SS_COOL_DOWN:
    {
        update_cool_down(dt);
        break;
    }
    case SS_DONE:
    {
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }
    }

    return 0;
}

void zNPCGoalBPlanktonBeam::update_warm_up(F32 dt)
{
    if (owner.delay >= tweak.beam.time_warm_up)
    {
        owner.delay = 0.0f;
        substate = SS_FIRE;
        owner.disable_emitter(*owner.beam_charge);
    }
}

void zNPCGoalBPlanktonBeam::update_fire(F32 dt)
{
    if (owner.delay >= owner.beam_duration || !owner.flag.attacking)
    {
        owner.AnimStart(g_hash_bossanim[ANIM_attack_beam_end], 0);
        owner.flag.aim_gun = false;
        substate = SS_COOL_DOWN;
    }
    else
    {
        if ((*get_player_loc() - owner.location()).length2() >=
            tweak.beam.max_dist * tweak.beam.max_dist)
        {
            substate = SS_COOL_DOWN;
        }
        else
        {
            xMat4x3 mat;
            mat.pos = xModelGetBoneLocation(*owner.model, 0x16);

            xMat3x3LookVec3(mat, *get_player_loc() - mat.pos);

            mat.pos += mat.at * tweak.beam.emit_dist;

            emitted = emitted + dt * tweak.beam.rate;
            S32 count = (S32)emitted;
            emitted = emitted - count;

            if (count > 0)
            {
                for (S32 i = 0; i < count; i++)
                {
                    owner.beam.emit(mat.pos, mat.at);
                    owner.beam_glow.emit(mat, -1);
                }
            }
        }
    }
}

void zNPCGoalBPlanktonBeam::update_cool_down(F32 dt)
{
    xAnimState* state = owner.AnimCurState();

    if (state->ID != g_hash_bossanim[ANIM_attack_beam_end] ||
        owner.AnimTimeRemain(NULL) < 0.001f + dt)
    {
        substate = SS_DONE;
    }
}

xFactoryInst* zNPCGoalBPlanktonWall::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonWall(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonWall::Enter(F32 dt, void* updCtxt)
{
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonWall::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonWall::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

xFactoryInst* zNPCGoalBPlanktonMissle::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonMissle(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonMissle::Enter(F32 dt, void* updCtxt)
{
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonMissle::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonMissle::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

xFactoryInst* zNPCGoalBPlanktonBomb::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonBomb(who, *(zNPCBPlankton*)info);
}

S32 zNPCGoalBPlanktonBomb::Enter(F32 dt, void* updCtxt)
{
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonBomb::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonBomb::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}
