
#include "zNPCTypeBossSB2.h"
#include "PowerPC_EABI_Support/MSL_C++/MSL_Common/Include/new.h"
#include "xLightKit.h"
#include "zNPCGoalCommon.h"
#include <types.h>
#include "string.h"
#include "iModel.h"
#include "xCollide.h"
#include "xMath.h"
#include "xMath3.h"
#include "xstransvc.h"
#include "xSnd.h"
#include "xVec3.h"
#include "xDebug.h"

#include "zCamera.h"
#include "zEntSimpleObj.h"
#include "zEntDestructObj.h"
#include "zGlobals.h"
#include "zGrid.h"
#include "zNPCTypeBossPatrick.h"
#include "zRenderState.h"
#include "zLightning.h"
#include "zNPCTypeRobot.h"
#include "zSurface.h"
#include "zNPCTypeVillager.h"
#include <xMathInlines.h>

#define ANIM_Unknown 0 //0x0
#define ANIM_Idle01 1 // 0x4
#define ANIM_Idle02 2 // 0x8
#define ANIM_Taunt01 3 // 0xC
#define ANIM_Hit01 7 //0x1c
#define ANIM_Hit02 8 //0x20
#define ANIM_Dizzy01 10 //0x28
#define ANIM_SmashHitLeft 46
#define ANIM_SmashHitRight 47
#define ANIM_SmackLeft01 48
#define ANIM_SmackRight01 49
#define ANIM_ChopLeftBegin 50
#define ANIM_ChopLeftLoop 51
#define ANIM_ChopLeftEnd 52
#define ANIM_ChopRightBegin 53
#define ANIM_ChopRightLoop 54
#define ANIM_ChopRightEnd 55
#define ANIM_SwipeLeftBegin 56
#define ANIM_SwipeLeftLoop 57
#define ANIM_SwipeLeftEnd 58
#define ANIM_SwipeRightBegin 59
#define ANIM_SwipeRightLoop 60
#define ANIM_SwipeRightEnd 61
#define ANIM_ReturnIdle01 62
#define ANIM_KarateStart 63
#define ANIM_KarateLoop 64
#define ANIM_KarateEnd 65

#define SOUND_TAUNT 0
#define SOUND_KARATE 1
#define SOUND_CHOP_WINDUP 2
#define SOUND_CHOP_SWING 3
#define SOUND_SWIPE 4
#define SOUND_KARATE_SLUG 5
#define SOUND_CHOP_HIT 6
#define SOUND_KARATE_HIT 7
#define SOUND_HIT_SLAP 8
#define SOUND_HIT_FLAIL 9

zNPCB_SB2* zNPCB_SB2::_singleton;

namespace
{
    struct sound_data_type
    {
        U32 id;
        U32 handle;
        xVec3* loc;
        F32 volume;
    };

    struct sound_asset
    {
        S32 group;
        char* name;
        U32 priority;
        U32 flags;
    };

    struct curve_node
    {
        F32 t;
        F32 value;
    };

    struct platform_hook
    {
        const char* name;
    };

    static const char* sound_asset_names[10][4];
    static U32 sound_asset_ids[10][4];
    static U32 sound_asset_names_size[10];
    static sound_data_type sound_data[10];

    struct node
    {
        F32 t;
    };

    struct inode : node
    {
        F32 value[1];
    };

    struct response_curve
    {
        unsigned long values;
        inode* curve;
        unsigned long nodes;
        unsigned long active_node;

        void init(unsigned long values, const void* curve, unsigned long nodes, const char*,
                  const char**, const tweak_callback*, void*);
        void eval_linear(F32 t, F32* out);
        void eval_smooth(F32 t, F32* out);
        void find_active_node(F32 t);
        F32 clamp_t(F32 t) const;
        F32 start_t() const;
        F32 end_t() const;
        const inode& get_node(unsigned long index) const;
    };

    void response_curve::init(unsigned long values, const void* curve, unsigned long nodes,
                              const char*, const char**, const tweak_callback*, void*)
    {
        this->values = values;
        this->curve = (inode*)curve;
        this->nodes = nodes;
        this->active_node = 0;
    }

    response_curve rc_scale;

    xBinaryCamera boss_cam = {
        {
            { 6.0f, 3.0f, 2.0f },
            { 0.2f, 2.2f, -1.0f },
            { 1.0f, 0.2f, 1.5f },
            10.0f,
            10.0f,
            10.0f,
            10.0f,
            50.0f,
            -3.1415927f,
        },
    };

    static const sound_asset sound_assets[12] = {
        { 0, "RSB_laugh", 0, 0 },      { 1, "RSB_kah", 0, 0 },      { 2, "RSB_chop_windup", 0, 0 },
        { 3, "RSB_chop_swing", 0, 0 }, { 4, "RSB_swipe", 0, 0 },    { 5, "RSB_foot_loop", 0, 1 },
        { 6, "RSB_armhit1", 0, 0 },    { 6, "RSB_armhit2", 0, 0 },  { 7, "RSB_armhit1", 0, 0 },
        { 7, "RSB_armhit2", 0, 0 },    { 8, "RSB_armsmash", 0, 0 }, { 9, "RSB_foor_impact", 0, 0 },
    };

    const platform_hook platform_hooks[16] = {
        { "PLAT_SB_FLIPPER_01" },
        { "PLAT_SB_FLIPPER_02" },
        { "PLAT_SB_FLIPPER_03" },
        { "PLAT_SB_FLIPPER_04" },
        { "PLAT_SB_FLIPPER_05" },
        { "PLAT_SB_FLIPPER_06" },
        { "PLAT_SB_FLIPPER_07" },
        { "PLAT_SB_FLIPPER_08" },
        { "PLAT_SB_FLIPPER_09" },
        { "PLAT_SB_FLIPPER_10" },
        { "PLAT_SB_FLIPPER_11" },
        { "PLAT_SB_FLIPPER_12" },
        { "PLAT_SB_FLIPPER_13" },
        { "PLAT_SB_FLIPPER_14" },
        { "PLAT_SB_FLIPPER_15" },
        { "PLAT_SB_FLIPPER_16" },
    };

    const curve_node scale_curve[4] = {
        { 0.0f, 0.0f },
        { 0.1f, 0.4f },
        { 1.0f, 1.25f },
        { 1.5f, 1.0f },
    };

    void set_alpha_blend(xModelInstance* model)
    {
        model->PipeFlags &= ~0xFF0C;
        model->PipeFlags |= 0x6508;
    }

    F32 max(F32 f0, F32 f1)
    {
        if (f0 > f1)
        {
            return f0;
        }
        return f1;
    }

    void reset_sound()
    {
        for (S32 i = 0; i < 10; ++i)
        {
            sound_data[i].handle = 0;
        }
    }

    struct sound_property
    {
        U32 asset;
        F32 volume;
        F32 range_inner;
        F32 range_outer;
        F32 delay;
        F32 fade_time;
    };

    struct tweak_group
    {
        F32 accel;
        F32 max_vel;
        F32 turn_accel;
        F32 turn_max_vel;
        F32 arena_radius;
        F32 ground_y;
        F32 ground_radius;
        F32 ground_zone_height;
        F32 move_radius;
        F32 damage_speed;
        F32 player_damage_time;
        F32 intro_time;
        struct
        {
            F32 pulse_rate;
            F32 pulse_min;
            F32 pulse_max;
        } nodes;
        struct
        {
            F32 min_dist;
            F32 vel;
            F32 accel;
            F32 decel;
            F32 collide_vel;
        } spin;
        struct
        {
            F32 delay_vuln;
        } help;
        struct
        {
            F32 delay;
        } chop;
        struct
        {
            F32 hold_time;
        } swipe;
        struct
        {
            xVec3 emit_offset;
            F32 emit_arc;
            F32 aim_dist;
            F32 aim_time;
            F32 aim_accel_time;
            F32 fire_vel;
            F32 fire_accel;
            F32 drop_vel;
            F32 drop_accel;
            F32 target_yoffset;
            F32 fade_dist;
            F32 kill_dist;
            F32 delay_emit[3];
            F32 delay_fire[3];
        } karate;
        struct
        {
            F32 warm_up;
            F32 cool_down;
            F32 height;
            F32 move_time;
        } hunt;
        struct
        {
            bool is_sphere;
            bool damage_player;
            S32 bone;
            xVec3 offset;
            F32 radius;
            xVec3 extent;
            F32 yaw;
            F32 pitch;
            F32 roll;
        } bounds[3];
        sound_property sound[10];
        void* context;
        tweak_callback cb_arena;
        tweak_callback cb_ground;
        tweak_callback cb_move_radius;
        tweak_callback cb_bounds;
        tweak_callback cb_hunt_move;
        tweak_callback cb_sound;
        tweak_callback cb_sound_asset;

        void load(xModelAssetParam* ap, U32 apsize);
        void register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char*);
    };

    static tweak_group tweak;

    static void init_sound()
    {
        memset(sound_asset_names_size, 0, sizeof(sound_asset_names_size));

        for (S32 i = 0; i < 12; i++)
        {
            const sound_asset& asset = sound_assets[i];
            if (asset.name != NULL)
            {
                U32& size = sound_asset_names_size[asset.group];
                sound_asset_names[asset.group][size] = asset.name;
                sound_asset_ids[asset.group][size] = i;
                size++;
            }
        }

        memset(sound_data, 0, sizeof(sound_data));

        for (S32 i = 0; i < 10; i++)
        {
            sound_data[i].id = 0;
            sound_data[i].handle = 0;
        }
    }

    U32 play_sound(S32 which, const xVec3* loc, F32 volume)
    {
        sound_data_type& data = sound_data[which];
        const sound_property& prop = tweak.sound[which];
        const sound_asset& asset = sound_assets[prop.asset];

        if ((asset.flags & 0x2) && data.handle != 0)
        {
            return data.handle;
        }

        if (asset.flags & 0x1)
        {
            data.handle = xSndPlay3DFade(data.id, volume * prop.volume, 1.0f, asset.priority,
                                         0x800, loc, prop.range_inner, prop.range_outer,
                                         SND_CAT_GAME, 0.0f, prop.delay);
        }
        else
        {
            data.handle = xSndPlay3D(data.id, volume * prop.volume, 1.0f, asset.priority, 0x800,
                                     loc, prop.range_inner, prop.range_outer, SND_CAT_GAME,
                                     prop.delay);
        }

        data.loc = (xVec3*)loc;
        data.volume = volume;
        return data.handle;
    }

    void kill_sound(S32 which, U32 handle)
    {
        sound_data_type& data = sound_data[which];
        const sound_property& prop = tweak.sound[which];
        const sound_asset& asset = sound_assets[prop.asset];

        if (asset.flags & 0x1)
        {
            xSndStopFade(handle, prop.fade_time);
        }
        else
        {
            xSndStop(handle);
        }

        data.handle = 0;
    }

    void tweak_group::load(xModelAssetParam* params, U32 size)
    {
        tweak_group::register_tweaks(true, params, size, NULL);
    }

    void tweak_group::register_tweaks(bool init, xModelAssetParam* ap, U32 apsize, const char*)
    {
        if (init)
        {
            accel = 2.0f;
            auto_tweak::load_param<F32, F32>(accel, 1.0f, 0.0f, 1000000000.0f, ap, apsize, "accel");
        }
        if (init)
        {
            max_vel = 5.0f;
            auto_tweak::load_param<F32, F32>(max_vel, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "max_vel");
        }
        if (init)
        {
            turn_accel = 540.0f;
            auto_tweak::load_param<F32, F32>(turn_accel, DEG2RAD(10), 0.01f, 1000000000.0f, ap,
                                             apsize, "turn_accel");
        }
        if (init)
        {
            turn_max_vel = 180.0f;
            auto_tweak::load_param<F32, F32>(turn_max_vel, DEG2RAD(10), 0.01f, 1000000000.0f, ap,
                                             apsize, "turn_max_vel");
        }
        if (init)
        {
            arena_radius = 12.0f;
            auto_tweak::load_param<F32, F32>(arena_radius, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "arena_radius");
        }
        if (init)
        {
            ground_y = 0.0f;
            auto_tweak::load_param<F32, F32>(ground_y, 1.0f, -1000000000.0f, 1000000000.0f, ap,
                                             apsize, "ground_y");
        }
        if (init)
        {
            ground_radius = 31.0f;
            auto_tweak::load_param<F32, F32>(ground_radius, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "ground_radius");
        }
        if (init)
        {
            ground_zone_height = 6.0f;
            auto_tweak::load_param<F32, F32>(ground_zone_height, 1.0f, 0.0f, 1000000000.0f, ap,
                                             apsize, "ground_zone_height");
        }
        if (init)
        {
            move_radius = 27.0f;
            auto_tweak::load_param<F32, F32>(move_radius, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "move_radius");
        }
        if (init)
        {
            damage_speed = 50.0f;
            auto_tweak::load_param<F32, F32>(damage_speed, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "damage_speed");
        }
        if (init)
        {
            player_damage_time = 1.0f;
            auto_tweak::load_param<F32, F32>(player_damage_time, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "player_damage_time");
        }
        if (init)
        {
            intro_time = 0.0f;
            auto_tweak::load_param<F32, F32>(intro_time, 1.0f, 0.0f, 100.0f, ap, apsize,
                                             "intro_time");
        }
        if (init)
        {
            nodes.pulse_rate = 15.0f;
            auto_tweak::load_param<F32, F32>(nodes.pulse_rate, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "nodes.pulse_rate");
        }
        if (init)
        {
            nodes.pulse_min = 0.0f;
            auto_tweak::load_param<F32, F32>(nodes.pulse_min, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "nodes.pulse_min");
        }
        if (init)
        {
            nodes.pulse_max = 0.5f;
            auto_tweak::load_param<F32, F32>(nodes.pulse_max, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "nodes.pulse_max");
        }
        if (init)
        {
            spin.min_dist = 0.5f;
            auto_tweak::load_param<F32, F32>(spin.min_dist, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "spin.min_dist");
        }
        if (init)
        {
            spin.vel = 5.0f;
            auto_tweak::load_param<F32, F32>(spin.vel, 1.0f, 0.0f, 10.0f, ap, apsize, "spin.vel");
        }
        if (init)
        {
            spin.accel = 20.0f;
            auto_tweak::load_param<F32, F32>(spin.accel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "spin.accel");
        }
        if (init)
        {
            spin.decel = 5.0f;
            auto_tweak::load_param<F32, F32>(spin.decel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "spin.decel");
        }
        if (init)
        {
            spin.collide_vel = 3.0f;
            auto_tweak::load_param<F32, F32>(spin.collide_vel, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "spin.collide_vel");
        }
        if (init)
        {
            help.delay_vuln = 1.0f;
            auto_tweak::load_param<F32, F32>(help.delay_vuln, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "help.delay_vuln");
        }
        if (init)
        {
            chop.delay = 0.3f;
            auto_tweak::load_param<F32, F32>(chop.delay, 1.0f, 0.0f, 100000.0f, ap, apsize,
                                             "chop.delay");
        }
        if (init)
        {
            swipe.hold_time = 3.5f;
            auto_tweak::load_param<F32, F32>(swipe.hold_time, 1.0f, 0.0f, 1000000000.0f, ap, apsize,
                                             "swipe.hold_time");
        }
        if (init)
        {
            karate.emit_offset = xVec3::create(0.0f, -1.0f, 3.0f);
            auto_tweak::load_param<xVec3, S32>(karate.emit_offset, 0, 0, 0, ap, apsize,
                                               "karate.emit_offset");
        }
        if (init)
        {
            karate.emit_arc = 55.0f;
            auto_tweak::load_param<F32, F32>(karate.emit_arc, DEG2RAD(10), 0.01f, 1000000000.0f, ap,
                                             apsize, "karate.emit_arc");
        }
        if (init)
        {
            karate.aim_dist = 7.0f;
            auto_tweak::load_param<F32, F32>(karate.aim_dist, 1.0f, 0.01f, 100.0f, ap, apsize,
                                             "karate.aim_dist");
        }
        if (init)
        {
            karate.aim_time = 1.0f;
            auto_tweak::load_param<F32, F32>(karate.aim_time, 1.0f, 0.001f, 100.0f, ap, apsize,
                                             "karate.aim_time");
        }
        if (init)
        {
            karate.aim_accel_time = 0.2f;
            auto_tweak::load_param<F32, F32>(karate.aim_accel_time, 1.0f, 0.001f, 100.0f, ap,
                                             apsize, "karate.aim_accel_time");
        }
        if (init)
        {
            karate.fire_vel = 5.0f;
            auto_tweak::load_param<F32, F32>(karate.fire_vel, 1.0f, 0.01f, 100000000.0f, ap, apsize,
                                             "karate.fire_vel");
        }
        if (init)
        {
            karate.fire_accel = 10.0f;
            auto_tweak::load_param<F32, F32>(karate.fire_accel, 1.0f, 0.01f, 1000000000.0f, ap,
                                             apsize, "karate.fire_accel");
        }
        if (init)
        {
            karate.drop_vel = 5.0f;
            auto_tweak::load_param<F32, F32>(karate.drop_vel, 1.0f, 0.1f, 100.0f, ap, apsize,
                                             "karate.drop_vel");
        }
        if (init)
        {
            karate.drop_accel = 10.0f;
            auto_tweak::load_param<F32, F32>(karate.drop_accel, 1.0f, 0.1f, 100.0f, ap, apsize,
                                             "karate.drop_accel");
        }
        if (init)
        {
            karate.target_yoffset = 1.0f;
            auto_tweak::load_param<F32, F32>(karate.target_yoffset, 1.0f, 0.1f, 100.0f, ap, apsize,
                                             "karate.target_yoffset");
        }
        if (init)
        {
            karate.fade_dist = 10.0f;
            auto_tweak::load_param<F32, F32>(karate.fade_dist, 1.0f, 0.1f, 100.0f, ap, apsize,
                                             "karate.fade_dist");
        }
        if (init)
        {
            karate.kill_dist = 15.0f;
            auto_tweak::load_param<F32, F32>(karate.kill_dist, 1.0f, 0.1f, 100.f, ap, apsize,
                                             "karate.kill_dist");
        }
        if (init)
        {
            karate.delay_emit[0] = 0.3f;
            auto_tweak::load_param<F32, F32>(karate.delay_emit[0], 1.0f, 0.001f, 100.0f, ap, apsize,
                                             "karate.delay_emit[0]");
        }
        if (init)
        {
            karate.delay_emit[1] = 0.6f;
            auto_tweak::load_param<F32, F32>(karate.delay_emit[1], 1.0f, 0.001f, 100.0f, ap, apsize,
                                             "karate.delay_emit[1]");
        }
        if (init)
        {
            karate.delay_emit[2] = 0.9f;
            auto_tweak::load_param<F32, F32>(karate.delay_emit[2], 1.0f, 0.001f, 100.0f, ap, apsize,
                                             "karate.delay_emit[2]");
        }
        if (init)
        {
            karate.delay_fire[0] = 2.75;
            auto_tweak::load_param<F32, F32>(karate.delay_fire[0], 1.0f, 0.001f, 100.0f, ap, apsize,
                                             "karate.delay_fire[0]");
        }
        if (init)
        {
            karate.delay_fire[1] = 0.0f;
            auto_tweak::load_param<F32, F32>(karate.delay_fire[1], 1.0f, 0.001f, 100.0f, ap, apsize,
                                             "karate.delay_fire[1]");
        }
        if (init)
        {
            karate.delay_fire[2] = 2.75;
            auto_tweak::load_param<F32, F32>(karate.delay_fire[2], 1.0f, 0.001f, 100.0f, ap, apsize,
                                             "karate.delay_fire[2]");
        }
        if (init)
        {
            hunt.warm_up = 0.5f;
            auto_tweak::load_param<F32, F32>(hunt.warm_up, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "hunt.warm_up");
        }
        if (init)
        {
            hunt.cool_down = 0.5f;
            auto_tweak::load_param<F32, F32>(hunt.cool_down, 1.0f, 0.0f, 10.0f, ap, apsize,
                                             "hunt.cool_down");
        }
        if (init)
        {
            hunt.height = 3.0f;
            auto_tweak::load_param<F32, F32>(hunt.height, 1.0f, -10.0f, 10.0f, ap, apsize,
                                             "hunt.height");
        }
        if (init)
        {
            hunt.move_time = 1.0f;
            auto_tweak::load_param<F32, F32>(hunt.move_time, 1.0f, 0.01, 10.0f, ap, apsize,
                                             "hunt.move_time");
        }
        if (init)
        {
            bounds[0].is_sphere = FALSE;
            auto_tweak::load_param<bool, S32>(bounds[0].is_sphere, 0, 0, 0, ap, apsize,
                                              "bounds[0].is_sphere");
        }
        if (init)
        {
            bounds[0].damage_player = FALSE;
            auto_tweak::load_param<bool, S32>(bounds[0].damage_player, 0, 0, 0, ap, apsize,
                                              "bounds[0].damage_player");
        }
        if (init)
        {
            bounds[0].bone = 2;
            auto_tweak::load_param<S32, S32>(bounds[0].bone, 1, 0, 63, ap, apsize,
                                             "bounds[0].bone");
        }
        if (init)
        {
            bounds[0].offset = xVec3::create(0.0f, 4.0f, 0.0f);
            auto_tweak::load_param<xVec3, S32>(bounds[0].offset, 0, 0, 0, ap, apsize,
                                               "bounds[0].offset");
        }
        if (init)
        {
            bounds[0].radius = 1.0f;
            auto_tweak::load_param<F32, F32>(bounds[0].radius, 1.0f, 0.0f, 1000000000.0f, ap,
                                             apsize, "bounds[0].radius");
        }
        if (init)
        {
            bounds[0].extent = xVec3::create(4.0f, 5.5f, 2.0f);
            auto_tweak::load_param<xVec3, S32>(bounds[0].extent, 0, 0, 0, ap, apsize,
                                               "bounds[0].extent");
        }
        if (init)
        {
            bounds[0].yaw = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[0].yaw, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[0].yaw");
        }
        if (init)
        {
            bounds[0].pitch = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[0].pitch, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[0].pitch");
        }
        if (init)
        {
            bounds[0].roll = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[0].roll, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[0].roll");
        }
        if (init)
        {
            bounds[1].is_sphere = FALSE;
            auto_tweak::load_param<bool, S32>(bounds[1].is_sphere, 0, 0, 0, ap, apsize,
                                              "bounds[1].is_sphere");
        }
        if (init)
        {
            bounds[1].damage_player = FALSE;
            auto_tweak::load_param<bool, S32>(bounds[1].damage_player, 0, 0, 0, ap, apsize,
                                              "bounds[1].damage_player");
        }
        if (init)
        {
            bounds[1].bone = 6;
            auto_tweak::load_param<S32, S32>(bounds[1].bone, 1, 0, 63, ap, apsize,
                                             "bounds[1].bone");
        }
        if (init)
        {
            bounds[1].offset = xVec3::create(0.0f, -1.5f, 0.1f);
            auto_tweak::load_param<xVec3, S32>(bounds[1].offset, 0, 0, 0, ap, apsize,
                                               "bounds[1].offset");
        }
        if (init)
        {
            bounds[1].radius = 1.0f;
            auto_tweak::load_param<F32, F32>(bounds[1].radius, 1.0f, 0.0f, 1000000000.0f, ap,
                                             apsize, "bounds[1].radius");
        }
        if (init)
        {
            bounds[1].extent = xVec3::create(6.5f, 2.8f, 2.5f);
            auto_tweak::load_param<xVec3, S32>(bounds[1].extent, 0, 0, 0, ap, apsize,
                                               "bounds[1].extent");
        }
        if (init)
        {
            bounds[1].yaw = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[1].yaw, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[1].yaw");
        }
        if (init)
        {
            bounds[1].pitch = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[1].pitch, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[1].pitch");
        }
        if (init)
        {
            bounds[1].roll = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[1].roll, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[1].roll");
        }
        if (init)
        {
            bounds[2].is_sphere = FALSE;
            auto_tweak::load_param<bool, S32>(bounds[2].is_sphere, 0, 0, 0, ap, apsize,
                                              "bounds[2].is_sphere");
        }
        if (init)
        {
            bounds[2].damage_player = TRUE;
            auto_tweak::load_param<bool, S32>(bounds[2].damage_player, 0, 0, 0, ap, apsize,
                                              "bounds[2].damage_player");
        }
        if (init)
        {
            bounds[2].bone = 2;
            auto_tweak::load_param<S32, S32>(bounds[2].bone, 1, 0, 63, ap, apsize,
                                             "bounds[2].bone");
        }
        if (init)
        {
            bounds[2].offset = xVec3::create(0.0f, -7.0f, 0.2f);
            auto_tweak::load_param<xVec3, S32>(bounds[2].offset, 0, 0, 0, ap, apsize,
                                               "bounds[2].offset");
        }
        if (init)
        {
            bounds[2].radius = 1.0f;
            auto_tweak::load_param<F32, F32>(bounds[2].radius, 1.0f, 0.0f, 1000000000.0f, ap,
                                             apsize, "bounds[2].radius");
        }
        if (init)
        {
            bounds[2].extent = xVec3::create(3.0f, 6.0f, 1.0f);
            auto_tweak::load_param<xVec3, S32>(bounds[2].extent, 0, 0, 0, ap, apsize,
                                               "bounds[2].extent");
        }
        if (init)
        {
            bounds[2].yaw = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[2].yaw, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[2].yaw");
        }
        if (init)
        {
            bounds[2].pitch = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[2].pitch, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[2].pitch");
        }
        if (init)
        {
            bounds[2].roll = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[2].roll, DEG2RAD(10), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[2].roll");
        }
        if (init)
        {
            sound[SOUND_TAUNT].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_TAUNT].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_TAUNT].volume");
        }
        if (init)
        {
            sound[SOUND_TAUNT].range_inner = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_TAUNT].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_TAUNT].range_inner");
        }
        if (init)
        {
            sound[SOUND_TAUNT].range_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_TAUNT].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_TAUNT].range_outer");
        }
        if (init)
        {
            sound[SOUND_TAUNT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_TAUNT].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_TAUNT].delay");
        }
        if (init)
        {
            sound[SOUND_KARATE].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_KARATE].volume");
        }
        if (init)
        {
            sound[SOUND_KARATE].range_inner = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_KARATE].range_inner");
        }
        if (init)
        {
            sound[SOUND_KARATE].range_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_KARATE].range_outer");
        }
        if (init)
        {
            sound[SOUND_KARATE].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_KARATE].delay");
        }
        if (init)
        {
            sound[SOUND_CHOP_WINDUP].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_WINDUP].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_CHOP_WINDUP].volume");
        }
        if (init)
        {
            sound[SOUND_CHOP_WINDUP].range_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_WINDUP].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_CHOP_WINDUP].range_inner");
        }
        if (init)
        {
            sound[SOUND_CHOP_WINDUP].range_outer = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_WINDUP].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_CHOP_WINDUP].range_outer");
        }
        if (init)
        {
            sound[SOUND_CHOP_WINDUP].delay = 0.2f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_WINDUP].delay, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_CHOP_WINDUP].delay");
        }
        if (init)
        {
            sound[SOUND_CHOP_SWING].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_SWING].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_CHOP_SWING].volume");
        }
        if (init)
        {
            sound[SOUND_CHOP_SWING].range_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_SWING].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_CHOP_SWING].range_inner");
        }
        if (init)
        {
            sound[SOUND_CHOP_SWING].range_outer = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_SWING].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_CHOP_SWING].range_outer");
        }
        if (init)
        {
            sound[SOUND_CHOP_SWING].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_SWING].delay, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_CHOP_SWING].delay");
        }
        if (init)
        {
            sound[SOUND_SWIPE].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_SWIPE].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_SWIPE].volume");
        }
        if (init)
        {
            sound[SOUND_SWIPE].range_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_SWIPE].range_inner, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_SWIPE].range_inner");
        }
        if (init)
        {
            sound[SOUND_SWIPE].range_outer = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_SWIPE].range_outer, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_SWIPE].range_outer");
        }
        if (init)
        {
            sound[SOUND_SWIPE].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_SWIPE].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_SWIPE].delay");
        }
        if (init)
        {
            sound[SOUND_KARATE_SLUG].volume = 0.25f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_SLUG].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_KARATE_SLUG].volume");
        }
        if (init)
        {
            sound[SOUND_KARATE_SLUG].range_inner = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_SLUG].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_KARATE_SLUG].range_inner");
        }
        if (init)
        {
            sound[SOUND_KARATE_SLUG].range_outer = 15.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_SLUG].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_KARATE_SLUG].range_outer");
        }
        if (init)
        {
            sound[SOUND_KARATE_SLUG].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_SLUG].delay, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_KARATE_SLUG].delay");
        }
        if (init)
        {
            sound[SOUND_KARATE_SLUG].fade_time = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_SLUG].fade_time, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_KARATE_SLUG].fade_time");
        }
        if (init)
        {
            sound[SOUND_CHOP_HIT].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_HIT].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_CHOP_HIT].volume");
        }
        if (init)
        {
            sound[SOUND_CHOP_HIT].range_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_HIT].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_CHOP_HIT].range_inner");
        }
        if (init)
        {
            sound[SOUND_CHOP_HIT].range_outer = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_HIT].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_CHOP_HIT].range_outer");
        }
        if (init)
        {
            sound[SOUND_CHOP_HIT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_CHOP_HIT].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_CHOP_HIT].delay");
        }
        if (init)
        {
            sound[SOUND_KARATE_HIT].volume = 0.5f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_HIT].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_KARATE_HIT].volume");
        }
        if (init)
        {
            sound[SOUND_KARATE_HIT].range_inner = 10.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_HIT].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_KARATE_HIT].range_inner");
        }
        if (init)
        {
            sound[SOUND_KARATE_HIT].range_outer = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_HIT].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_KARATE_HIT].range_outer");
        }
        if (init)
        {
            sound[SOUND_KARATE_HIT].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_KARATE_HIT].delay, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_KARATE_HIT].delay");
        }
        if (init)
        {
            sound[SOUND_HIT_SLAP].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT_SLAP].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_HIT_SLAP].volume");
        }
        if (init)
        {
            sound[SOUND_HIT_SLAP].range_inner = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT_SLAP].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_HIT_SLAP].range_inner");
        }
        if (init)
        {
            sound[SOUND_HIT_SLAP].range_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT_SLAP].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_HIT_SLAP].range_outer");
        }
        if (init)
        {
            sound[SOUND_HIT_SLAP].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT_SLAP].delay, 1.0f, 0.0f, 100000.0f, ap,
                                             apsize, "sound[SOUND_HIT_SLAP].delay");
        }
        if (init)
        {
            sound[SOUND_HIT_FLAIL].volume = 1.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT_FLAIL].volume, 1.0f, 0.0f, 1.0f, ap,
                                             apsize, "sound[SOUND_HIT_FLAIL].volume");
        }
        if (init)
        {
            sound[SOUND_HIT_FLAIL].range_inner = 30.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT_FLAIL].range_inner, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_HIT_FLAIL].range_inner");
        }
        if (init)
        {
            sound[SOUND_HIT_FLAIL].range_outer = 50.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT_FLAIL].range_outer, 1.0f, 0.0f,
                                             100000.0f, ap, apsize,
                                             "sound[SOUND_HIT_FLAIL].range_outer");
        }
        if (init)
        {
            sound[SOUND_HIT_FLAIL].delay = 0.0f;
            auto_tweak::load_param<F32, F32>(sound[SOUND_HIT_FLAIL].delay, 1.0f, 0.0f, 100000.0f,
                                             ap, apsize, "sound[SOUND_HIT_FLAIL].delay");
        }
        if (init)
        {
            sound[SOUND_TAUNT].asset = sound_asset_ids[0][0];
            sound_data[SOUND_TAUNT].id = xStrHash(sound_assets[sound[SOUND_TAUNT].asset].name);
        }
        if (init)
        {
            sound[SOUND_KARATE].asset = sound_asset_ids[1][0];
            sound_data[SOUND_KARATE].id = xStrHash(sound_assets[sound[SOUND_KARATE].asset].name);
        }
        if (init)
        {
            sound[SOUND_CHOP_WINDUP].asset = sound_asset_ids[2][0];
            sound_data[SOUND_CHOP_WINDUP].id =
                xStrHash(sound_assets[sound[SOUND_CHOP_WINDUP].asset].name);
        }
        if (init)
        {
            sound[SOUND_CHOP_SWING].asset = sound_asset_ids[3][0];
            sound_data[SOUND_CHOP_SWING].id =
                xStrHash(sound_assets[sound[SOUND_CHOP_SWING].asset].name);
        }
        if (init)
        {
            sound[SOUND_SWIPE].asset = sound_asset_ids[4][0];
            sound_data[SOUND_SWIPE].id = xStrHash(sound_assets[sound[SOUND_SWIPE].asset].name);
        }
        if (init)
        {
            sound[SOUND_KARATE_SLUG].asset = sound_asset_ids[5][0];
            sound_data[SOUND_KARATE_SLUG].id =
                xStrHash(sound_assets[sound[SOUND_KARATE_SLUG].asset].name);
        }
        if (init)
        {
            sound[SOUND_CHOP_HIT].asset = sound_asset_ids[6][0];
            sound_data[SOUND_CHOP_HIT].id =
                xStrHash(sound_assets[sound[SOUND_CHOP_HIT].asset].name);
        }
        if (init)
        {
            sound[SOUND_KARATE_HIT].asset = sound_asset_ids[7][1];
            sound_data[SOUND_KARATE_HIT].id =
                xStrHash(sound_assets[sound[SOUND_KARATE_HIT].asset].name);
        }
        if (init)
        {
            sound[SOUND_HIT_SLAP].asset = sound_asset_ids[8][0];
            sound_data[SOUND_HIT_SLAP].id =
                xStrHash(sound_assets[sound[SOUND_CHOP_HIT].asset].name);
        }
        if (init)
        {
            sound[SOUND_HIT_FLAIL].asset = sound_asset_ids[9][0];
            sound_data[SOUND_HIT_FLAIL].id =
                xStrHash(sound_assets[sound[SOUND_HIT_FLAIL].asset].name);
        }
    }

    void response_curve::eval_linear(F32 t, F32* out)
    {
        F32* end = out + values;
        find_active_node(t);

        const inode& n0 = curve[active_node];
        const inode& n1 = curve[active_node + 1];
        F32 dt = n1.t - n0.t;

        if (dt >= -0.00001f && dt <= 0.00001f)
        {
            const F32* v = n0.value;
            for (; out != end; ++out, ++v)
            {
                *out = *v;
            }
        }
        else
        {
            F32 frac = (t - n0.t) / dt;
            const F32* v0 = n0.value;
            const F32* v1 = n1.value;
            for (; out != end; ++out, ++v0, ++v1)
            {
                *out = frac * (*v1 - *v0) + *v0;
            }
        }
    }

    void response_curve::eval_smooth(F32 t, F32* out)
    {
        if (nodes == 2)
        {
            eval_linear(t, out);
            return;
        }

        F32* end = out + values;
        find_active_node(t);

        const inode& n0 = curve[active_node];
        const inode& n1 = curve[active_node + 1];
        F32 dt = n1.t - n0.t;

        if (dt >= -0.00001f && dt <= 0.00001f)
        {
            const F32* v = n0.value;
            for (; out != end; ++out, ++v)
            {
                *out = *v;
            }
            return;
        }

        F32 u = (t - n0.t) / dt;
        F32 u2 = u * u;
        F32 u3 = u2 * u;

        F32 c0 = -0.5f * u3 + u2 + -0.5f * u;
        F32 c1 = 1.5f * u3 + -2.5f * u2 + 1.0f;
        F32 c2 = -1.5f * u3 + 2.0f * u2 + 0.5f * u;
        F32 c3 = 0.5f * u3 + -0.5f * u2;

        if (active_node != 0 && active_node < nodes - 2)
        {
            const F32* v0 = curve[active_node - 1].value;
            const F32* v1 = n0.value;
            const F32* v2 = n1.value;
            const F32* v3 = curve[active_node + 2].value;
            for (; out != end; ++out, ++v0, ++v1, ++v2, ++v3)
            {
                *out = c0 * *v0 + c1 * *v1 + c2 * *v2 + c3 * *v3;
            }
        }
        else if (active_node != 0)
        {
            F32 c23 = c2 + c3;
            const F32* v0 = curve[active_node - 1].value;
            const F32* v1 = n0.value;
            const F32* v2 = n1.value;
            for (; out != end; ++out, ++v0, ++v1, ++v2)
            {
                *out = c0 * *v0 + c1 * *v1 + c23 * *v2;
            }
        }
        else
        {
            F32 c01 = c0 + c1;
            const F32* v1 = n0.value;
            const F32* v2 = n1.value;
            const F32* v3 = curve[active_node + 2].value;
            for (; out != end; ++out, ++v1, ++v2, ++v3)
            {
                *out = c01 * *v1 + c2 * *v2 + c3 * *v3;
            }
        }
    }

    void response_curve::find_active_node(F32 t)
    {
        unsigned long size = values * sizeof(F32) + sizeof(node);
        const U8* it = (const U8*)curve + size * active_node;

        while (true)
        {
            if (t < ((const inode*)it)->t)
            {
                it -= size;
                --active_node;
            }
            else if (t > ((const inode*)(it + size))->t)
            {
                it += size;
                ++active_node;
            }
            else
            {
                break;
            }
        }
    }

    F32 response_curve::clamp_t(F32 t) const
    {
        return range_limit(t, start_t(), end_t());
    }

    F32 response_curve::end_t() const
    {
        return get_node(nodes - 1).t;
    }

    const inode& response_curve::get_node(unsigned long index) const
    {
        return *(const inode*)((const U8*)curve + index * (values * sizeof(F32) + sizeof(node)));
    }

    F32 response_curve::start_t() const
    {
        return curve->t;
    }

} // namespace

xAnimTable* ZNPC_AnimTable_BossSB2()
{
    S32 anim_list[32];
    xAnimTable* table = xAnimTableNew("zNPCB_SB2_Karate", NULL, 0);
    S32 anim_count = 0;

    //24 new state
    //15  new transition
    anim_list[anim_count++] = ANIM_Idle01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Idle01], 0x10, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_Idle02;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Idle02], 0, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_Taunt01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Taunt01], 0, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_SmackLeft01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SmackLeft01], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_SmackRight01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SmackRight01], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_ChopLeftBegin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopLeftBegin], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_ChopLeftLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopLeftLoop], 0, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_ChopLeftEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopLeftEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_ChopRightBegin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopRightBegin], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_ChopRightLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopRightLoop], 0, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_ChopRightEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopRightEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_SwipeLeftBegin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeLeftBegin], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_SwipeLeftLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeLeftLoop], 0x10, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_SwipeLeftEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeLeftEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_SwipeRightBegin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeRightBegin], 0x20, 0, 1, NULL, NULL, 0,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_SwipeRightLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeRightLoop], 0x10, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_SwipeRightEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeRightEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_Dizzy01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Dizzy01], 0x10, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_Hit01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Hit01], 0, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_Hit02;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Hit02], 0, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_ReturnIdle01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ReturnIdle01], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_KarateStart;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_KarateStart], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_KarateLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_KarateLoop], 0x10, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[anim_count++] = ANIM_KarateEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_KarateEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);

    anim_list[anim_count++] = ANIM_Unknown;

    NPCC_BuildStandardAnimTran(table, g_strz_bossanim, anim_list, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_SmackLeft01], g_strz_bossanim[ANIM_Dizzy01],
                            0, 0, 0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_SmackRight01],
                            g_strz_bossanim[ANIM_Dizzy01], 0, 0, 0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_ReturnIdle01], g_strz_bossanim[ANIM_Idle01],
                            0, 0, 0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_KarateStart],
                            g_strz_bossanim[ANIM_KarateLoop], 0, 0, 0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_KarateEnd], g_strz_bossanim[ANIM_Idle01], 0,
                            0, 0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_ChopLeftBegin],
                            g_strz_bossanim[ANIM_ChopLeftLoop], 0, 0, 0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_ChopLeftLoop],
                            g_strz_bossanim[ANIM_ChopLeftEnd], 0, 0, 0, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_ChopRightBegin],
                            g_strz_bossanim[ANIM_ChopRightLoop], 0, 0, 0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_ChopRightLoop],
                            g_strz_bossanim[ANIM_ChopRightEnd], 0, 0, 0, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_SwipeLeftBegin],
                            g_strz_bossanim[ANIM_SwipeLeftLoop], 0, 0, 0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_SwipeLeftLoop],
                            g_strz_bossanim[ANIM_SwipeLeftEnd], 0, 0, 0, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_SwipeRightBegin],
                            g_strz_bossanim[ANIM_SwipeRightLoop], 0, 0, 0x10, 0, 0, 0, 0, 0, 0.1,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_SwipeRightLoop],
                            g_strz_bossanim[ANIM_SwipeRightEnd], 0, 0, 0, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_Hit01], g_strz_bossanim[ANIM_Idle01], 0, 0,
                            0x10, 0, 0, 0, 0, 0, 0.1, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_Hit02], g_strz_bossanim[ANIM_Dizzy01], 0, 0,
                            0x10, 0, 0, 0, 0, 0, 0.1, 0);

    return table;
}

void zNPCB_SB2::Init(xEntAsset* asset)
{
    _singleton = this;
    boss_cam.init();
    init_sound();
    zNPCCommon::Init(asset);
    this->cfg_npc->dst_castShadow = 30.0f;
    memset(&this->flag, 0, sizeof(this->flag));
    this->said_intro = 0;

    xModelInstance* m = this->model;
    this->models[0] = m;
    m = m->Next;
    this->models[1] = m;
    m = m->Next;
    this->models[2] = m;
    m = m->Next;
    this->models[3] = m;

    this->models[0]->Data->boundingSphere.radius = 100.0f;
    this->models[1]->Data->boundingSphere.radius = 100.0f;
    this->models[2]->Data->boundingSphere.radius = 100.0f;
    this->models[3]->Data->boundingSphere.radius = 100.0f;

    set_alpha_blend(this->models[0]);
    this->init_hands();
    this->init_bounds();
    this->reset_bounds();

    this->penby = FALSE;
    this->bupdate = 0;
    this->bound.type = TRUE;
    this->bound.sph.center.y = 1e38f;
    this->bound.sph.r = 0.0f;

    rc_scale.init(1, scale_curve, 4, NULL, NULL, NULL, NULL);

    this->init_slugs();
}

void zNPCB_SB2::Setup()
{
    this->create_glow_light();
    this->init_nodes();
    zNPCBoss::Setup();

    for (S32 i = 0; i < 16; i++)
    {
        platform_data& platform = this->platforms[i];
        platform.ent = NULL;

        xEnt* ent = (xEnt*)zSceneFindObject(xStrHash(platform_hooks[i].name));
        if (ent != NULL && xEntValidType(ent->baseType))
        {
            platform.ent = ent;
            platform.mat = *(xMat3x3*)ent->model->Mat;

            xSphere o;
            xBoundGetSphere(o, ent->bound);
            platform.radius = o.r;
        }
    }

    if (this->models[3]->Surf == NULL)
    {
        this->models[3]->Surf = &create_surface();
    }

    if (this->models[0]->Surf == NULL)
    {
        this->models[0]->Surf = &create_surface();
    }

    ((zSurfaceProps*)this->models[3]->Surf->moprops)->asset->game_damage_type = 0;
    ((zSurfaceProps*)this->models[0]->Surf->moprops)->asset->game_damage_type = 7;

    this->scan_cronies();

    this->newsfish = (zNPCNewsFish*)zSceneFindObject(xStrHash("NPC_NEWSCASTER"));
    if (this->newsfish != NULL)
    {
        this->newsfish->TalkOnScreen(1);
    }
}

void zNPCB_SB2::SelfSetup()
{
    xBehaveMgr* bmgr = xBehaveMgr_GetSelf();
    xPsyche* psy = 0;
    this->psy_instinct = bmgr->Subscribe(this, NULL);
    this->psy_instinct->BrainBegin();
    for (S32 i = NPC_GOAL_BOSSSB2INTRO; i <= NPC_GOAL_BOSSSB2DEATH; i++)
    {
        psy_instinct->AddGoal(i, this);
    }
    psy_instinct->AddGoal(NPC_GOAL_LIMBO, this);
    psy_instinct->BrainEnd();
    psy_instinct->SetSafety(NPC_GOAL_BOSSSB2IDLE);
}

void zNPCB_SB2::Reset()
{
    if(this->newsfish != 0)
    {
        this->newsfish->Reset();
    }
    
    reset_sound();
    zNPCCommon::Reset();
    memset(&flag.face_player, 0 , 0x10);

    for (S32 i = 0;  i < 9; i++)
    {
        if (nodes[i].ent != NULL)
        {
            zEntDestructObj_Reset(nodes[i].ent, globals.sceneCur);
            xEntShow(nodes[i].ent);
        }
    }

    for (S32 i = 0; i < 16; i++)
    {
        if (platforms[i].ent != NULL)
        {
            zEntEvent(this, platforms[i].ent, 10);
        }
    }

    this->life = 0;
    this->round = 0;

    this->check_life();
    this->choose_hand();
    this->reset_speed();
    this->show_nodes();

    this->turn.vel = 0.0f;
    this->node_pulse = 0.0f;
    this->flag.face_player = TRUE;
    this->flag.move = MOVE_NONE;
    this->player_damage_timer = 0.0f;
    this->old_player_health = 0;

    this->reset_stage();
    this->set_vulnerable(TRUE);
    zCameraDisableTracking(CO_BOSS);
    boss_cam.start(globals.camera);
    psy_instinct->GoalSet(NPC_GOAL_BOSSSB2INTRO, 0);

}

void zNPCB_SB2::Destroy()
{
    zNPCB_SB2::destroy_glow_light();
    zNPCCommon::Destroy();
}

void zNPCB_SB2::Process(xScene* xscn, F32 dt)
{
    if (this->flag.updated == FALSE)
    {
        boss_cam.set_targets((xVec3&)globals.player.ent.model->Mat->pos, location(),
                             5.0f);
        this->flag.updated = TRUE;
    }

    this->check_life();
    this->player_damage_timer = this->player_damage_timer - dt;

    if ((globals.player.Health < this->old_player_health) && this->old_player_health != 0)
    {
        this->player_damage_timer = tweak.player_damage_time;
        say(1);
    }

    this->old_player_health = globals.player.Health;

    if ((SomethingWonderful() & 0x23) == 0)
    {
        delay = delay + dt;
        this->psy_instinct->Timestep(dt, 0);
    }

    this->update_nodes(dt);
    this->update_slugs(dt);
    this->update_move(dt);
    this->update_turn(dt);
    this->update_camera(dt);
    this->check_hit_fail();
    zNPCCommon::Process(xscn, dt);
}

void zNPCB_SB2::NewTime(xScene* xscn, F32 dt)
{
    xVec3 tmp;

    if (this->flag.nodes_taken == NULL)
    {
        this->move_nodes();
    }

    for (S32 i = 0; i < 2; i++)
    {
        this->move_hand(this->hands[i], dt);
    }

    this->update_bounds();
    this->update_platforms(dt);
    this->update_slugs(dt);

    this->models[1]->Flags = this->models[1]->Flags & 0xefff;
    this->models[2]->Flags = this->models[2]->Flags & 0xefff;

    this->sound_loc.mouth = xModelGetBoneLocation(*model, 4);
    this->sound_loc.body = this->sound_loc.mouth;
    this->sound_loc.hand[0] = xModelGetBoneLocation(*model, 16);
    this->sound_loc.hand[1] = xModelGetBoneLocation(*model, 21);

    zNPCCommon::NewTime(xscn, dt);
}

void zNPCB_SB2::decompose()
{
}

void zNPCB_SB2::show_nodes()
{
    for (S32 i = 0; i < 9; i++)
    {
        if (nodes[i].ent != NULL)
        {
            xEntShow(nodes[i].ent);
        }
    }
}

void zNPCB_SB2::ouchie()
{
    xPsyche* psy = psy_instinct;
    S32 tempOuch;
    tempOuch = psy_instinct->GIDOfActive();
    if (tempOuch != NPC_GOAL_BOSSSB2HIT)
    {
        set_vulnerable(false);
        psy_instinct->GoalSet(NPC_GOAL_BOSSSB2HIT, 1);
    }
    
}

void zNPCB_SB2::Render()
{
    xNPCBasic::Render();
    zNPCB_SB2::render_debug();
}

F32 zNPCB_SB2::AttackTimeLeft()
{
    if (flag.dizzy != false)
    {
        return 0.0f;
    }
    return 1e38f;
}

void zNPCB_SB2::HoldUpDude()
{
}

void zNPCB_SB2::ThanksImDone()
{
    flag.dizzy = false;
}

void zNPCB_SB2::reset_speed()
{
    turn.accel = tweak.turn_accel;
    turn.max_vel = tweak.turn_max_vel;
}

zNPCB_SB2::platform_data* zNPCB_SB2::player_platform()
{
    xCollis& coll = globals.player.ent.collis->colls[0];

    if (!(coll.flags & 1) || coll.optr == NULL ||
        ((xBase*)coll.optr)->baseType != eBaseTypePlatform)
    {
        return NULL;
    }

    platform_data* it = platforms;
    platform_data* end = it + 16;
    for (; it != end; ++it)
    {
        if (it->ent == coll.optr)
        {
            return it;
        }
    }

    return NULL;
}

void zNPCB_SB2::activate_hand(zNPCB_SB2::hand_enum hand, bool hit_platforms)
{
    hands[hand].hurt_player = TRUE;
    hands[hand].hit_platforms = hit_platforms;
    hands[hand].ent->penby = XENT_COLLTYPE_PLYR;
}

void zNPCB_SB2::deactivate_hand(zNPCB_SB2::hand_enum hand)
{
    hands[hand].hit_platforms = FALSE;
    hands[hand].hurt_player = FALSE;
    hands[hand].ent->penby = XENT_COLLTYPE_PLYR;
}

bool zNPCB_SB2::player_on_ground() const
{
    const xVec3& player_loc = *(xVec3*)&globals.player.ent.model->Mat->pos;

    if (globals.player.Health == 0)
    {
        return false;
    }

    if (player_loc.y >= tweak.ground_y + tweak.ground_zone_height)
    {
        return false;
    }

    const xVec3& home = get_home();
    F32 dx = player_loc.x - home.x;
    F32 dz = player_loc.z - home.z;
    xVec2 offset = { dx, dz };
    return offset.length2() < tweak.ground_radius * tweak.ground_radius;
}

void zNPCB_SB2::emit_slug(zNPCB_SB2::slug_enum which)
{
    slug_data& slug = slugs[which];
    F32 launch_ang; 
    F32 accel_time;
    
    if (slug.ent->id != 0)
    {
        slug.stage = (zNPCB_SB2::slug_stage)1;
        slug.time = 0.0f;
        slug.stage_delay = tweak.karate.aim_time;
        slug.vel = 0.0f;
        slug.dist = 0.0f;
        slug.abandoned = 0;

        xMat4x3Mul((xMat4x3*)model->Mat, 0 , 0);
        xMat3x3RMulVec(&tweak.karate.emit_offset, 0, 0);
    }
}

void zNPCB_SB2::fire_slug(zNPCB_SB2::slug_enum which, zNPCB_SB2::platform_data& target)
{
    slug_data& slug = slugs[which]; 
    xVec3 offset; 
    F32 idist;

    slug.stage = SLUG_DELAY;
}

void zNPCB_SB2::abandon_slugs()
{
    slug_data* pCurSlug = &slugs[0];
    slug_data* pLastSlug = &pCurSlug[3];
    for (; pCurSlug != pLastSlug; pCurSlug++)
    {
         pCurSlug->abandoned = 1;
    }
}

void zNPCB_SB2::reset_stage()
{
    stage = -1;
    stage_delay = 0;
}

void zNPCB_SB2::destroy_glow_light()
{
    xLightKit_Destroy(&glow_light.kit);
}

void zNPCB_SB2::say(U32)
{
}

xFactoryInst* zNPCGoalBossSB2Intro::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Intro(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Intro::Enter(F32 dt, void* updCtxt)
{
    if (owner.said_intro == 0)
    {
        owner.say(0);
        owner.said_intro = 1;
    }
    owner.delay = 0.0f;
    zEntPlayerControlOff(CONTROL_OWNER_BOSS);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Intro::Exit(F32 dt, void* updCtxt)
{
    zEntPlayerControlOn(CONTROL_OWNER_BOSS);
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBossSB2Idle::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Idle(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Idle::Enter(F32 dt, void* updCtxt)
{
    transitioning = 1;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Idle::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBossSB2Taunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Taunt(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Taunt::Enter(F32 dt, void* updCtxt)
{
    play_sound(0, &owner.sound_loc.mouth , 1.0f);
    owner.flag.face_player = 1;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Taunt::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBossSB2Dizzy::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Dizzy(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Dizzy::Enter(F32 dt, void* updCtxt)
{
    sicked = 0;
    owner.flag.face_player = 0;
    owner.delay = 0;
    owner.set_vulnerable(false);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Dizzy::Exit(F32 dt, void* updCtxt)
{
    S32 tempDizzy;
    owner.set_vulnerable(true);
    if (sicked && !owner.player_on_ground())
    {
        owner.plankton->here_boy();
    }
    if (owner.life == 1)
    {
        owner.say(6);
    }
    else if (owner.flag.dizzy == false)
    {
        owner.say(0xb);
    }
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBossSB2Hit::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Hit(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Hit::Enter(F32 dt, void* updCtxt)
{
    owner.flag.face_player = true;
    owner.set_vulnerable(false);

    if (owner.flag.dizzy)
    {
        owner.say(9);
    }
    else if (owner.life > 4)
    {
        owner.say(2);
    }
    else if (owner.life > 2)
    {
        owner.say(3);
    }
    else if (owner.life > 0)
    {
        owner.say(4);
    }

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Hit::Exit(F32 dt, void* updCtxt)
{
    owner.set_vulnerable(true);
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBossSB2Hunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Hunt(who, (zNPCB_SB2&)*info);
}

xFactoryInst* zNPCGoalBossSB2Swipe::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Swipe(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Swipe::Enter(F32 dt, void* updCtxt)
{
    owner.flag.face_player = 1;
    said = 0;
    holding = 0;
    started = 0;

    owner.choose_hand();

    if (owner.active_hand == 0)
    {
        begin_anim = g_hash_bossanim[56];
        loop_anim = g_hash_bossanim[57];
        end_anim = g_hash_bossanim[58];
    }
    else
    {
        begin_anim = g_hash_bossanim[59];
        loop_anim = g_hash_bossanim[60];
        end_anim = g_hash_bossanim[61];
    }

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Swipe::Exit(F32 dt, void* updCtxt)
{
    owner.flag.face_player = true;
    owner.deactivate_hand(owner.active_hand);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSB2Swipe::can_start() const
{
    return owner.player_platform() != NULL;
}

xFactoryInst* zNPCGoalBossSB2Chop::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Chop(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Chop::Enter(F32 dt, void* updCtxt)
{
    targetted = 0;
    started = 0;
    owner.flag.face_player = true;

    owner.choose_hand();
    owner.activate_hand(owner.active_hand, true);

    if (owner.active_hand == 0)
    {
        begin_anim = g_hash_bossanim[50];
        loop_anim = g_hash_bossanim[51];
        end_anim = g_hash_bossanim[52];
    }
    else
    {
        begin_anim = g_hash_bossanim[53];
        loop_anim = g_hash_bossanim[54];
        end_anim = g_hash_bossanim[55];
    }

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Chop::Exit(F32 dt, void* updCtxt)
{
    owner.deactivate_hand(owner.active_hand);
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBossSB2Karate::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Karate(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Karate::Enter(F32 dt, void* updCtxt)
{
    started = FALSE;
    owner.flag.face_player = true;

    U8* it = emitted;
    U8* end = it + 3;
    for (; it != end; ++it)
    {
        *it = FALSE;
    }

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Karate::Exit(F32 dt, void* updCtxt)
{
    owner.abandon_slugs();
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSB2Karate::can_start() const
{
    return owner.player_platform() != NULL;
}

xFactoryInst* zNPCGoalBossSB2Death::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Death(who, (zNPCB_SB2&)*info);
}

S32 zNPCGoalBossSB2Death::Enter(F32 dt, void* updCtxt)
{
    owner.decompose();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Death::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSB2Death::Process(en_trantype*, F32, void*, xScene*)
{
    return 0;
}

void zNPCB_SB2::choose_hand()
{
    S32 r = xrand();
    S32 b = (r >> 13) & 1;
    this->active_hand = (b == 0 ? LEFT_HAND : RIGHT_HAND);
}

xVec3& zNPCB_SB2::location() const
{
    return reinterpret_cast<xVec3&>(this->model->Mat->pos);
}

void zNPCB_SB2::render_debug()
{
}

xVec3& zNPCB_SB2::get_home() const
{
    return reinterpret_cast<xVec3&>(this->asset->pos);
}

xVec3& zNPCB_SB2::start_location() const
{
    return reinterpret_cast<xVec3&>(this->asset->pos);
}

xVec3& zNPCB_SB2::facing() const
{
    return reinterpret_cast<xVec3&>(this->model->Mat->at);
}
