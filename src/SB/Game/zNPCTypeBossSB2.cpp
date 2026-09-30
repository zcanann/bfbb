
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
#include "zNPCMgr.h"
#include "zNPCTypeBossPatrick.h"
#include "zNPCTypeBossPlankton.h"
#include "zRenderState.h"
#include "zLightning.h"
#include "zNPCTypeRobot.h"
#include "zSurface.h"
#include "zEntCruiseBubble.h"
#include "zScene.h"
#include "zEnv.h"
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

U32 xSndPlay3DFade(U32 id, F32 vol, F32 pitch, U32 priority, U32 flags, const xVec3* pos,
                   F32 innerRadius, F32 outerRadius, sound_category category, F32 fade, F32 delay);
F32 xSCurveInverse(F32 val);
bool xSphereHitsBound(const xSphere& o, const xBound& b);
void xBoundGetSphere(xSphere& o, const xBound& bound);
U32 iModelTagSetup(xModelTagWithNormal* tag, RpAtomic* model, F32 x, F32 y, F32 z);
void iModelTagEval(RpAtomic* model, const xModelTagWithNormal* tag, RwMatrixTag* mat, xVec3* dest,
                   xVec3* normal);
U8 xOBBHitsOBB(const xBox& a, const xMat4x3& amat, const xBox& b, const xMat4x3& bmat);

zNPCB_SB2* zNPCB_SB2::_singleton;

namespace
{
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
        U32 values;
        inode* curve;
        U32 nodes;
        U32 active_node;

        void init(u32 values, const void* curve, u32 nodes, const char*, const char**,
                  const tweak_callback*, void*);

        void eval_linear(F32 t, F32* value);
        void find_active_node(F32 t);
        void eval_smooth(F32 t, F32* value);
        F32 clamp_t(F32 t) const;
        F32 end_t() const;
        inode* get_node(u32 index) const;
        F32 start_t() const;
    };

    struct sound_data_type
    {
        U32 id;
        U32 handle;
        const xVec3* loc;
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
        F32 scale;
    };

    struct platform_hook
    {
        char* name;
    };

    struct node_hook
    {
        char* name;
        S32 model;
        bool midpoint;
        S32 points;
        xVec3 pos[3];
    };

    struct hand_hook
    {
        xVec3 head[4];
        xVec3 tail[4];
    };

    struct say_group
    {
        const zNPCNewsFish::say_enum* list;
        U32 size;
    };

    static char* sound_asset_names[10][4];
    static U32 sound_asset_ids[10][4];
    static S32 sound_asset_names_size[10];
    static sound_data_type sound_data[10];

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

    static response_curve rc_scale;

    static xBinaryCamera boss_cam = {
        { { 6.0f, 3.0f, 2.0f },
          { 0.2f, 2.2f, -1.0f },
          { 1.0f, 0.2f, 1.5f },
          10.0f,
          10.0f,
          10.0f,
          10.0f,
          50.0f,
          -PI },
    };

    static const sound_asset sound_assets[12] = {
        { 0, "RSB_laugh", 0, 0 },      { 1, "RSB_kah", 0, 0 },      { 2, "RSB_chop_windup", 0, 0 },
        { 3, "RSB_chop_swing", 0, 0 }, { 4, "RSB_swipe", 0, 0 },    { 5, "RSB_foot_loop", 0, 1 },
        { 6, "RSB_armhit1", 0, 0 },    { 6, "RSB_armhit2", 0, 0 },  { 7, "RSB_armhit1", 0, 0 },
        { 7, "RSB_armhit2", 0, 0 },    { 8, "RSB_armsmash", 0, 0 }, { 9, "RSB_foot_impact", 0, 0 },
    };

    static const curve_node scale_curve[4] = {
        { 0.0f, 0.0f },
        { 0.1f, 0.4f },
        { 1.0f, 1.25f },
        { 1.5f, 1.0f },
    };

    struct sequence_entry
    {
        S32 goal;
        F32 delay;
    };

    // clang-format off
    static const sequence_entry sequence[9][16] = {
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2SWIPE, 0.5f },
          { 0, -1.0f } },
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2SWIPE, 0.01f },
          { NPC_GOAL_BOSSSB2SWIPE, 0.5f }, { 0, -1.0f } },
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2CHOP, 0.01f },
          { NPC_GOAL_BOSSSB2SWIPE, 0.01f }, { NPC_GOAL_BOSSSB2SWIPE, 0.5f },
          { 0, -1.0f } },
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2CHOP, 0.0f },
          { NPC_GOAL_BOSSSB2CHOP, 0.01f }, { NPC_GOAL_BOSSSB2SWIPE, 0.01f },
          { NPC_GOAL_BOSSSB2SWIPE, 0.5f }, { 0, -1.0f } },
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2CHOP, 0.0f },
          { NPC_GOAL_BOSSSB2CHOP, 0.0f }, { NPC_GOAL_BOSSSB2CHOP, 0.01f },
          { NPC_GOAL_BOSSSB2SWIPE, 0.01f }, { NPC_GOAL_BOSSSB2SWIPE, 0.5f },
          { 0, -1.0f } },
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2KARATE, 0.01f },
          { NPC_GOAL_BOSSSB2CHOP, 0.0f }, { NPC_GOAL_BOSSSB2CHOP, 0.0f },
          { NPC_GOAL_BOSSSB2CHOP, 0.01f }, { NPC_GOAL_BOSSSB2SWIPE, 0.01f },
          { NPC_GOAL_BOSSSB2SWIPE, 0.5f }, { 0, -1.0f } },
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2KARATE, 0.0f },
          { NPC_GOAL_BOSSSB2KARATE, 0.01f }, { NPC_GOAL_BOSSSB2CHOP, 0.0f },
          { NPC_GOAL_BOSSSB2CHOP, 0.0f }, { NPC_GOAL_BOSSSB2CHOP, 0.01f },
          { NPC_GOAL_BOSSSB2SWIPE, 0.01f }, { NPC_GOAL_BOSSSB2SWIPE, 0.5f },
          { 0, -1.0f } },
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2KARATE, 0.0f },
          { NPC_GOAL_BOSSSB2KARATE, 0.01f }, { NPC_GOAL_BOSSSB2CHOP, 0.0f },
          { NPC_GOAL_BOSSSB2CHOP, 0.0f }, { NPC_GOAL_BOSSSB2CHOP, 0.0f },
          { NPC_GOAL_BOSSSB2CHOP, 0.01f }, { NPC_GOAL_BOSSSB2SWIPE, 0.01f },
          { NPC_GOAL_BOSSSB2SWIPE, 0.5f }, { 0, -1.0f } },
        { { NPC_GOAL_BOSSSB2IDLE, 0.5f }, { NPC_GOAL_BOSSSB2KARATE, 0.0f },
          { NPC_GOAL_BOSSSB2KARATE, 0.0f }, { NPC_GOAL_BOSSSB2KARATE, 0.01f },
          { NPC_GOAL_BOSSSB2CHOP, 0.0f }, { NPC_GOAL_BOSSSB2CHOP, 0.0f },
          { NPC_GOAL_BOSSSB2CHOP, 0.0f }, { NPC_GOAL_BOSSSB2CHOP, 0.01f },
          { NPC_GOAL_BOSSSB2SWIPE, 0.01f }, { NPC_GOAL_BOSSSB2SWIPE, 0.5f },
          { 0, -1.0f } },
    };
    // clang-format on

    static const bool dizzy_round[9] = {
        false, false, true, false, false, true, false, false, true
    };

    static const node_hook node_hooks[9] = {
        { "SB_NODE_05", 3, false, 3,
          { { 3.167f, 7.652f, 2.051f }, { 2.423f, 7.988f, 2.058f }, { 3.555f, 8.631f, 2.058f } } },
        { "SB_NODE_06", 3, true, 3,
          { { 3.993f, 14.411f, 2.019f }, { 2.816f, 14.778f, 2.019f }, { 3.899f, 14.949f, 2.019f } } },
        { "SB_NODE_07", 3, false, 2,
          { { 4.377f, 10.433f, -0.135f }, { 4.542f, 11.823f, -0.267f }, { 0.0f, 0.0f, 0.0f } } },
        { "SB_NODE_04", 1, false, 2,
          { { 9.769f, 6.602f, 0.119f }, { 9.769f, 6.602f, 1.281f }, { 0.0f, 0.0f, 0.0f } } },
        { "SB_NODE_01", 3, false, 3,
          { { -3.102f, 7.363f, 2.051f }, { -3.304f, 8.16f, 2.055f }, { -2.493f, 7.565f, 2.051f } } },
        { "SB_NODE_02", 3, true, 3,
          { { -4.01f, 13.799f, 2.091f }, { -4.009f, 15.017f, 2.11f }, { -3.392f, 14.42f, 2.11f } } },
        { "SB_NODE_03", 3, false, 2,
          { { -4.486f, 10.148f, -0.174f }, { -4.486f, 11.007f, -0.109f }, { 0.0f, 0.0f, 0.0f } } },
        { "SB_NODE_08", 2, false, 2,
          { { -9.769f, 6.602f, 0.119f }, { -9.769f, 6.602f, 1.281f }, { 0.0f, 0.0f, 0.0f } } },
        { "SB_NODE_09", 3, false, 2,
          { { -0.002f, 11.142f, 4.366f }, { -0.002f, 11.616f, 4.203f }, { 0.0f, 0.0f, 0.0f } } },
    };

    static const hand_hook hand_hooks[2] = {
        { { { 7.733f, 6.602f, -1.906f },
            { 7.733f, 8.075f, -1.906f },
            { 12.781f, 6.602f, -1.906f },
            { 12.781f, 8.075f, -1.906f } },
          { { 7.733f, 6.602f, 2.259f },
            { 7.733f, 8.075f, 2.259f },
            { 12.775f, 6.602f, 2.259f },
            { 12.775f, 8.075f, 2.259f } } },
        { { { -12.78f, 6.602f, -1.906f },
            { -12.78f, 8.075f, -1.906f },
            { -7.733f, 6.602f, -1.906f },
            { -7.733f, 8.075f, -1.906f } },
          { { -12.775f, 6.602f, 2.259f },
            { -12.775f, 8.075f, 2.259f },
            { -7.733f, 6.602f, 2.259f },
            { -7.733f, 8.075f, 2.259f } } },
    };

    static const platform_hook platform_hooks[16] = {
        { "PLAT_SB_FLIPPER_01" }, { "PLAT_SB_FLIPPER_02" }, { "PLAT_SB_FLIPPER_03" },
        { "PLAT_SB_FLIPPER_04" }, { "PLAT_SB_FLIPPER_05" }, { "PLAT_SB_FLIPPER_06" },
        { "PLAT_SB_FLIPPER_07" }, { "PLAT_SB_FLIPPER_08" }, { "PLAT_SB_FLIPPER_09" },
        { "PLAT_SB_FLIPPER_10" }, { "PLAT_SB_FLIPPER_11" }, { "PLAT_SB_FLIPPER_12" },
        { "PLAT_SB_FLIPPER_13" }, { "PLAT_SB_FLIPPER_14" }, { "PLAT_SB_FLIPPER_15" },
        { "PLAT_SB_FLIPPER_16" },
    };

    static const platform_hook slug_hooks[3] = {
        { "SB_SLUG_KAH" }, { "SB_SLUG_RAH" }, { "SB_SLUG_TAY" },
    };

    static const zNPCNewsFish::say_enum say_intro[1] = { zNPCNewsFish::SAY_B302_INTRO };

    static const zNPCNewsFish::say_enum say_hit_player[6] = {
        zNPCNewsFish::SAY_HIT_PLAYER_1, zNPCNewsFish::SAY_HIT_PLAYER_2,
        zNPCNewsFish::SAY_HIT_PLAYER_3, zNPCNewsFish::SAY_HIT_PLAYER_4,
        zNPCNewsFish::SAY_HIT_PLAYER_5, zNPCNewsFish::SAY_HIT_PLAYER_6,
    };

    static const zNPCNewsFish::say_enum say_hit_boss_1[4] = {
        zNPCNewsFish::SAY_HIT_BOSS_1,
        zNPCNewsFish::SAY_HIT_BOSS_2,
        zNPCNewsFish::SAY_SB_HIT_BOSS_2,
        zNPCNewsFish::SAY_SB_HIT_BOSS_3,
    };

    static const zNPCNewsFish::say_enum say_hit_boss_2[3] = {
        zNPCNewsFish::SAY_HIT_BOSS_2,
        zNPCNewsFish::SAY_SB_HIT_BOSS_3,
        zNPCNewsFish::SAY_ROBOT_HIT,
    };

    static const zNPCNewsFish::say_enum say_hit_boss_3[1] = { zNPCNewsFish::SAY_SB_HIT_BOSS_1 };

    static const zNPCNewsFish::say_enum say_hit_fail[1] = { zNPCNewsFish::SAY_ROBOT_HIT_FAIL };

    static const zNPCNewsFish::say_enum say_hit_last[1] = { zNPCNewsFish::SAY_HIT_LAST };

    static const zNPCNewsFish::say_enum say_spun[2] = {
        zNPCNewsFish::SAY_SPIN,
        zNPCNewsFish::SAY_SB_ROUGH_RIDE,
    };

    static const zNPCNewsFish::say_enum say_vuln[7] = {
        zNPCNewsFish::SAY_SB_VULN_1,   zNPCNewsFish::SAY_SB_VULN_2,
        zNPCNewsFish::SAY_SB_VULN_3,   zNPCNewsFish::SAY_SB_VULN_4,
        zNPCNewsFish::SAY_SB_VULN_5,   zNPCNewsFish::SAY_ROBOT_VULN_1,
        zNPCNewsFish::SAY_ROBOT_VULN_2,
    };

    static const zNPCNewsFish::say_enum say_stun[4] = {
        zNPCNewsFish::SAY_ROBOT_DIZZY,
        zNPCNewsFish::SAY_ROBOT_STUN_1,
        zNPCNewsFish::SAY_ROBOT_STUN_2,
        zNPCNewsFish::SAY_ROBOT_STUN_3,
    };

    static const zNPCNewsFish::say_enum say_return[1] = { zNPCNewsFish::SAY_SB_BACK };

    static const zNPCNewsFish::say_enum say_tactics[1] = { zNPCNewsFish::SAY_ROBOT_TACTICS };

    static const zNPCNewsFish::say_enum say_fall[1] = { zNPCNewsFish::SAY_SB_HIT_FAIL_2 };

    static const say_group say_set[13] = {
        { say_intro, 1 },      { say_hit_player, 6 }, { say_hit_boss_1, 4 },
        { say_hit_boss_2, 3 }, { say_hit_boss_3, 1 }, { say_hit_fail, 1 },
        { say_hit_last, 1 },   { say_spun, 2 },       { say_vuln, 7 },
        { say_stun, 4 },       { say_return, 1 },     { say_tactics, 1 },
        { say_fall, 1 },
    };

    F32 max(F32 f0, F32 f1);

    static void init_sound()
    {
        memset(sound_asset_names_size, 0, sizeof(sound_asset_names_size));

        for (S32 i = 0; i < 12; i++)
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

        for (S32 i = 0; i < 10; i++)
        {
            sound_data[i].id = 0;
            sound_data[i].handle = 0;
        }
    }

    void reset_sound()
    {
        for (S32 i = 0; i < 10; ++i)
        {
            sound_data[i].handle = 0;
        }
    }

    S32 play_sound(int which, const xVec3* pos, F32 volume)
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

    void kill_sound(int which, U32 handle)
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

    void set_yaw_matrix(xMat3x3& mat, F32 yaw)
    {
        F32 s = isin(yaw);
        F32 c = icos(yaw);

        mat.right.assign(c, 0.0f, -s);
        mat.up.assign(0.0f, 1.0f, 0.0f);
        mat.at.assign(s, 0.0f, c);
    }

    void set_alpha_blend(xModelInstance* model)
    {
        model->PipeFlags &= ~0xFF0C;
        model->PipeFlags |= 0x6508;
    }

    void init_bound_entity(xEnt& ent, U32 id, xModelInstance* model, xMat4x3* mat)
    {
        memset(&ent, 0, sizeof(xEnt));

        ent.id = id;
        ent.baseType = 0xc;
        ent.collType = XENT_COLLTYPE_STAT;
        ent.chkby = XENT_COLLTYPE_PLYR;
        ent.penby = XENT_COLLTYPE_PLYR;
        ent.baseFlags = 0x21;
        ent.moreFlags = 0;
        ent.model = model;
        ent.collModel = model;
        ent.bound.type = XBOUND_TYPE_OBB;
        ent.bound.mat = mat;

        xGridBoundInit(&ent.gridb, &ent);
    }

    void parallelepiped_to_obb(xBound& obb, xVec3* loc)
    {
        xVec3* head_loc = loc;
        xVec3* tail_loc = loc + 4;

        obb.type = XBOUND_TYPE_OBB;

        xVec3 head_sum = head_loc[0] + head_loc[1] + head_loc[2] + head_loc[3];
        xVec3 tail_sum = tail_loc[0] + tail_loc[1] + tail_loc[2] + tail_loc[3];
        xVec3& center = obb.box.center;

        center = (head_sum + tail_sum) * 0.125f;

        head_loc[0] -= center;
        head_loc[1] -= center;
        head_loc[2] -= center;
        head_loc[3] -= center;
        tail_loc[0] -= center;
        tail_loc[1] -= center;
        tail_loc[2] -= center;
        tail_loc[3] -= center;

        xMat4x3& mat = *obb.mat;

        mat.right = head_sum - tail_sum;
        mat.at = head_loc[0] + head_loc[1] + tail_loc[0] + tail_loc[1];
        mat.at -= head_loc[2] + head_loc[3] + tail_loc[2] + tail_loc[3];
        mat.right.normalize();
        mat.up = mat.at.cross(mat.right).normal();
        mat.at = mat.right.cross(mat.up);
        mat.pos = center;

        xVec3& upper = obb.box.box.upper;
        xVec3& lower = obb.box.box.lower;

        upper.assign(mat.right.dot(head_loc[0]), mat.up.dot(head_loc[0]), mat.at.dot(head_loc[0]));
        upper.set_abs();

        const xVec3* it = head_loc + 1;
        const xVec3* end = head_loc + 8;

        for (; it != end; it++)
        {
            upper.x = max(upper.x, xabs(mat.right.dot(*it)));
            upper.y = max(upper.y, xabs(mat.up.dot(*it)));
            upper.z = max(upper.z, xabs(mat.at.dot(*it)));
        }

        lower = -upper;
    }

    F32 max(F32 f0, F32 f1)
    {
        if (f0 > f1)
        {
            return f0;
        }
        return f1;
    }


} // namespace

xAnimTable* ZNPC_AnimTable_BossSB2()
{
    xAnimTable* table = xAnimTableNew("zNPCB_SB2_Karate", NULL, 0);
    S32 anim_list[32];
    S32 n = 0;

    anim_list[n++] = ANIM_Idle01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Idle01], 0x10, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_Idle02;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Idle02], 0, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_Taunt01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Taunt01], 0, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_SmackLeft01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SmackLeft01], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_SmackRight01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SmackRight01], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_ChopLeftBegin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopLeftBegin], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_ChopLeftLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopLeftLoop], 0, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_ChopLeftEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopLeftEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_ChopRightBegin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopRightBegin], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_ChopRightLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopRightLoop], 0, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_ChopRightEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ChopRightEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_SwipeLeftBegin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeLeftBegin], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_SwipeLeftLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeLeftLoop], 0x10, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_SwipeLeftEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeLeftEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_SwipeRightBegin;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeRightBegin], 0x20, 0, 1, NULL, NULL, 0,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_SwipeRightLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeRightLoop], 0x10, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_SwipeRightEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_SwipeRightEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_Dizzy01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Dizzy01], 0x10, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_Hit01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Hit01], 0, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_Hit02;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Hit02], 0, 0, 1, NULL, NULL, 0, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_ReturnIdle01;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_ReturnIdle01], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_KarateStart;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_KarateStart], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_KarateLoop;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_KarateLoop], 0x10, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    anim_list[n++] = ANIM_KarateEnd;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_KarateEnd], 0x20, 0, 1, NULL, NULL, 0, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);

    anim_list[n++] = 0;

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
    xModelInstance * m;

    _singleton = this;
    boss_cam.init();
    init_sound();
    zNPCCommon::Init(asset);
    this->cfg_npc->dst_castShadow = 30.0f;
    memset(&this->flag.face_player, 0, 0x10);
    this->said_intro = 0;

    m = this->model;
    this->models[0] = m;

    m = m->Next;
    this->models[1] = m;

    m = m->Next;
    this->models[2] = m;

    this->models[3] = m->Next;

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

namespace
{
    void response_curve::init(u32 values, const void* curve, u32 nodes, const char*, const char**,
                              const tweak_callback*, void*)
    {
        this->values = values;
        this->curve = (inode*)curve;
        this->nodes = nodes;
        this->active_node = 0;
    }
} // namespace

void zNPCB_SB2::ParseINI()
{
    zNPCCommon::ParseINI();
    tweak.load(this->parmdata, this->pdatsize);
}

namespace
{
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
            auto_tweak::load_param<F32, F32>(spin.vel, 1.0f, 0.0f, 100000.0f, ap, apsize, "spin.vel");
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
            auto_tweak::load_param<F32, F32>(karate.emit_arc, DEG2RAD(1), 0.01f, 1000000000.0f, ap,
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
            auto_tweak::load_param<F32, F32>(karate.fire_vel, 1.0f, 0.01f, 1000000000.0f, ap,
                                             apsize, "karate.fire_vel");
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
            auto_tweak::load_param<F32, F32>(bounds[0].yaw, DEG2RAD(1), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[0].yaw");
        }
        if (init)
        {
            bounds[0].pitch = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[0].pitch, DEG2RAD(1), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[0].pitch");
        }
        if (init)
        {
            bounds[0].roll = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[0].roll, DEG2RAD(1), -1000000000.0f,
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
            auto_tweak::load_param<F32, F32>(bounds[1].yaw, DEG2RAD(1), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[1].yaw");
        }
        if (init)
        {
            bounds[1].pitch = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[1].pitch, DEG2RAD(1), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[1].pitch");
        }
        if (init)
        {
            bounds[1].roll = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[1].roll, DEG2RAD(1), -1000000000.0f,
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
            auto_tweak::load_param<F32, F32>(bounds[2].yaw, DEG2RAD(1), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[2].yaw");
        }
        if (init)
        {
            bounds[2].pitch = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[2].pitch, DEG2RAD(1), -1000000000.0f,
                                             1000000000.0f, ap, apsize, "bounds[2].pitch");
        }
        if (init)
        {
            bounds[2].roll = 0.0f;
            auto_tweak::load_param<F32, F32>(bounds[2].roll, DEG2RAD(1), -1000000000.0f,
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
            const sound_asset& asset = sound_assets[sound[SOUND_TAUNT].asset];
            sound_data[SOUND_TAUNT].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_KARATE].asset = sound_asset_ids[1][0];
            const sound_asset& asset = sound_assets[sound[SOUND_KARATE].asset];
            sound_data[SOUND_KARATE].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_CHOP_WINDUP].asset = sound_asset_ids[2][0];
            const sound_asset& asset = sound_assets[sound[SOUND_CHOP_WINDUP].asset];
            sound_data[SOUND_CHOP_WINDUP].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_CHOP_SWING].asset = sound_asset_ids[3][0];
            const sound_asset& asset = sound_assets[sound[SOUND_CHOP_SWING].asset];
            sound_data[SOUND_CHOP_SWING].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_SWIPE].asset = sound_asset_ids[4][0];
            const sound_asset& asset = sound_assets[sound[SOUND_SWIPE].asset];
            sound_data[SOUND_SWIPE].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_KARATE_SLUG].asset = sound_asset_ids[5][0];
            const sound_asset& asset = sound_assets[sound[SOUND_KARATE_SLUG].asset];
            sound_data[SOUND_KARATE_SLUG].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_CHOP_HIT].asset = sound_asset_ids[6][0];
            const sound_asset& asset = sound_assets[sound[SOUND_CHOP_HIT].asset];
            sound_data[SOUND_CHOP_HIT].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_KARATE_HIT].asset = sound_asset_ids[7][1];
            const sound_asset& asset = sound_assets[sound[SOUND_KARATE_HIT].asset];
            sound_data[SOUND_KARATE_HIT].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_HIT_SLAP].asset = sound_asset_ids[8][0];
            const sound_asset& asset = sound_assets[sound[SOUND_HIT_SLAP].asset];
            sound_data[SOUND_HIT_SLAP].id = xStrHash(asset.name);
        }
        if (init)
        {
            sound[SOUND_HIT_FLAIL].asset = sound_asset_ids[9][0];
            const sound_asset& asset = sound_assets[sound[SOUND_HIT_FLAIL].asset];
            sound_data[SOUND_HIT_FLAIL].id = xStrHash(asset.name);
        }
    }

} // namespace

void zNPCB_SB2::Setup()
{
    xEnt* ent; 
    xSphere o;

    this->create_glow_light();
    this->init_nodes();
    zNPCBoss::Setup();

    for (S32 i = 0; i < 16; i++)
    {
        platforms[i].ent = NULL;

        ent = (xEnt*)zSceneFindObject(xStrHash(platform_hooks[i].name));

        if (ent == NULL || !xEntValidType(ent->baseType))
        {
            continue;
        }

        platforms[i].ent = ent;
        platforms[i].mat = *(xMat3x3*)ent->model->Mat;

        xBoundGetSphere(o, ent->bound);

        platforms[i].radius = o.r;
    }

    if (models[3]->Surf == NULL)
    {
        models[3]->Surf = &create_surface();
    }

    if (models[0]->Surf == NULL)
    {
        models[0]->Surf = &create_surface();
    }

    ((zSurfaceProps*)models[3]->Surf->moprops)->asset->game_damage_type = 0;
    ((zSurfaceProps*)models[0]->Surf->moprops)->asset->game_damage_type = 7;

    scan_cronies();

    newsfish = (zNPCNewsFish*)zSceneFindObject(xStrHash("NPC_NEWSCASTER"));

    if (newsfish != NULL)
    {
        newsfish->TalkOnScreen(1);
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

U32 zNPCB_SB2::AnimPick(S32 animID, en_NPC_GOAL_SPOT gspot, xGoal* goal)
{
    static const S32 idle_table[8][2] = {
        { ANIM_Dizzy01, ANIM_ReturnIdle01 },
        { ANIM_KarateLoop, ANIM_KarateEnd },
        { ANIM_ChopLeftLoop, ANIM_ChopLeftEnd },
        { ANIM_ChopRightLoop, ANIM_ChopRightEnd },
        { ANIM_SwipeLeftBegin, ANIM_SwipeLeftEnd },
        { ANIM_SwipeRightBegin, ANIM_SwipeRightEnd },
        { ANIM_SwipeLeftLoop, ANIM_SwipeLeftEnd },
        { ANIM_SwipeRightLoop, ANIM_SwipeRightEnd },
    };

    S32 index;

    switch (animID)
    {
    case NPC_GOAL_BOSSSB2IDLE:
    {
        index = ANIM_Idle01;

        for (U32 i = 0; i < 8; i++)
        {
            if (g_hash_bossanim[idle_table[i][0]] == AnimCurStateID())
            {
                index = idle_table[i][1];
                break;
            }
        }
        break;
    }
    case NPC_GOAL_BOSSSB2TAUNT:
        index = ANIM_Taunt01;
        break;
    case NPC_GOAL_BOSSSB2HUNT:
        if (flag.dizzy)
        {
            index = ANIM_Dizzy01;
        }
        else
        {
            index = ANIM_Idle01;

            for (U32 i = 0; i < 8; i++)
            {
                if (g_hash_bossanim[idle_table[i][0]] == AnimCurStateID())
                {
                    index = idle_table[i][1];
                    break;
                }
            }
        }
        break;
    case NPC_GOAL_BOSSSB2SWIPE:
        index = ANIM_Idle01;
        break;
    case NPC_GOAL_BOSSSB2CHOP:
        index = ANIM_Idle01;
        break;
    case NPC_GOAL_BOSSSB2DIZZY:
        index = ANIM_Dizzy01;
        break;
    case NPC_GOAL_BOSSSB2HIT:
        if (!flag.dizzy)
        {
            index = ANIM_Hit01;
            play_sound(SOUND_HIT_FLAIL, &sound_loc.body, 1.0f);
        }
        else if (g_hash_bossanim[ANIM_SwipeLeftLoop] == AnimCurStateID())
        {
            index = ANIM_SmackLeft01;
            play_sound(SOUND_HIT_SLAP, &sound_loc.hand[LEFT_HAND], 1.0f);
        }
        else if (g_hash_bossanim[ANIM_SwipeRightLoop] == AnimCurStateID())
        {
            index = ANIM_SmackRight01;
            play_sound(SOUND_HIT_SLAP, &sound_loc.hand[RIGHT_HAND], 1.0f);
        }
        else
        {
            index = ANIM_Hit02;
            play_sound(SOUND_HIT_FLAIL, &sound_loc.body, 1.0f);
        }
        break;
    case NPC_GOAL_BOSSSB2KARATE:
        index = ANIM_Idle01;
        break;
    default:
        index = ANIM_Idle01;
        break;
    }

    if (index > -1)
    {
        return g_hash_bossanim[index];
    }

    return 0;
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
    xEntCollis* collis = globals.player.ent.collis;

    xEnt* ent;

    if (!(collis->colls->flags & 1) || (ent = (xEnt*)collis->colls->optr) == NULL ||
        ent->baseType != eBaseTypePlatform)
    {
        return NULL;
    }

    platform_data* pCurPlat = platforms;
    platform_data* pLastPlat = &pCurPlat[16];

    for (; pCurPlat != pLastPlat; pCurPlat++)
    {
        if (pCurPlat->ent == ent)
        {
            return pCurPlat;
        }
    }

    return NULL;
}

void zNPCB_SB2::activate_hand(zNPCB_SB2::hand_enum hand, bool hit_platforms)
{
    hands[hand].hurt_player = TRUE;
    hands[hand].hit_platforms = hit_platforms;
    hands[hand].ent->penby = 0x10;
}

void zNPCB_SB2::deactivate_hand(zNPCB_SB2::hand_enum hand)
{
    hands[hand].hit_platforms = FALSE;
    hands[hand].hurt_player = FALSE;
    hands[hand].ent->penby = 0x10;
}

bool zNPCB_SB2::player_on_ground() const
{
    const xVec3& loc = (const xVec3&)globals.player.ent.model->Mat->pos;

    if (globals.player.Health == 0)
    {
        return 0;
    }

    if (loc.y >= tweak.ground_y + tweak.ground_zone_height)
    {
        return 0;
    }

    const xVec3& home = get_home();

    F32 dx = loc.x - home.x;
    F32 dz = loc.z - home.z;

    xVec2 d = {};
    d.x = dx;
    d.y = dz;

    return d.length2() < tweak.ground_radius * tweak.ground_radius;
}

void zNPCB_SB2::emit_slug(zNPCB_SB2::slug_enum which)
{
    slug_data& slug = slugs[which];

    if (slug.ent != NULL)
    {
        slug.stage = SLUG_AIM;
        slug.time = 0.0f;
        slug.stage_delay = tweak.karate.aim_time;
        slug.dist = slug.vel = 0.0f;
        slug.abandoned = 0;

        const xMat4x3* skin = (const xMat4x3*)model->Mat;

        xMat4x3Mul(&slug.mat, &skin[4], skin);

        xVec3 offset;

        xMat3x3RMulVec(&offset, &slug.mat, &tweak.karate.emit_offset);
        slug.mat.pos += offset;

        F32 launch_ang = which - 1.0f;

        launch_ang *= tweak.karate.emit_arc;

        slug.move_dir.assign(isin(launch_ang), icos(launch_ang), 0.0f);

        F32 accel_time = tweak.karate.aim_accel_time;

        slug.accel = tweak.karate.aim_dist /
                     (accel_time * (tweak.karate.aim_time - 2.0f * accel_time + 0.5f * accel_time));
        slug.max_vel = slug.accel * accel_time;

        slug.ent->flags |= 1;
        slug.ent->model->Alpha = 1.0f;

        play_sound(SOUND_KARATE, &sound_loc.mouth, 1.0f);

        if (slug.sound_handle)
        {
            kill_sound(SOUND_KARATE_SLUG, slug.sound_handle);
        }

        slug.sound_handle = play_sound(SOUND_KARATE_SLUG, &slug.mat.pos, 1.0f);
    }
}

bool zNPCB_SB2::slugs_ready() const
{
    const slug_data* pCurSlug = slugs;
    const slug_data* pLastSlug = &pCurSlug[MAX_SLUG];

    for (; pCurSlug != pLastSlug; pCurSlug++)
    {
        if (pCurSlug->stage != SLUG_AIM || pCurSlug->stage_delay > 0.0f)
        {
            return 0;
        }
    }

    return 1;
}

bool zNPCB_SB2::slugs_inactive() const
{
    const slug_data* pCurSlug = slugs;
    const slug_data* pLastSlug = &pCurSlug[MAX_SLUG];

    for (; pCurSlug != pLastSlug; pCurSlug++)
    {
        if (pCurSlug->stage != SLUG_INACTIVE)
        {
            return 0;
        }
    }

    return 1;
}

void zNPCB_SB2::fire_slug(zNPCB_SB2::slug_enum which, zNPCB_SB2::platform_data& target)
{
    slug_data& slug = slugs[which];

    slug.stage = SLUG_DELAY;
    slug.spun = 0;
    slug.target = &target;
    slug.stage_delay = tweak.karate.delay_fire[which];
    slug.dist = slug.vel = 0.0f;
    slug.accel = tweak.karate.fire_accel;
    slug.max_vel = tweak.karate.fire_vel;

    xVec3 offset = (const xVec3&)target.ent->model->Mat->pos - slug.mat.pos;
    offset.y += tweak.karate.target_yoffset;

    slug.end_dist = xsqrt(offset.x * offset.x + offset.z * offset.z);

    F32 idist = 1.0f / slug.end_dist;

    slug.dmat.at.assign(offset.x * idist, 0.0f, offset.z * idist);
    slug.dmat.up.assign(0.0f, 1.0f, 0.0f);
    slug.dmat.right.assign(slug.dmat.at.z, 0.0f, -slug.dmat.at.x);
    slug.dmat.pos = slug.mat.pos;

    slug.ydist = 0.0f;
    slug.yvel = 0.0f;
    slug.end_ydist = offset.y;
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

bool zNPCB_SB2::player_damaged() const
{
    return player_damage_timer > 0.0f;
}

S32 zNPCB_SB2::platform_index(const zNPCB_SB2::platform_data& p) const
{
    return &p - platforms;
}

S32 zNPCB_SB2::next_goal()
{
    if (flag.dizzy)
    {
        return NPC_GOAL_BOSSSB2DIZZY;
    }

    if (player_on_ground())
    {
        return NPC_GOAL_BOSSSB2HUNT;
    }

    if (player_damaged())
    {
        return NPC_GOAL_BOSSSB2TAUNT;
    }

    if (stage_delay > 0.0f)
    {
        return NPC_GOAL_BOSSSB2IDLE;
    }

    stage++;

    if (sequence[round][stage].goal == 0)
    {
        stage = 0;
    }

    stage_delay = sequence[round][stage].delay;

    return sequence[round][stage].goal;
}

void zNPCB_SB2::reset_stage()
{
    stage = -1;
    stage_delay = 0;
}

void zNPCB_SB2::set_vulnerable(bool vulnerable)
{
    if (vulnerable == flag.vulnerable)
    {
        return;
    }

    flag.vulnerable = vulnerable;

    S32 dflags = vulnerable ? 0x1F000 : 0;

    node_data* pCurNode = nodes;
    node_data* pLastNode = &pCurNode[9];

    for (; pCurNode != pLastNode; pCurNode++)
    {
        pCurNode->ent->dasset->dflags = dflags;
    }
}

void zNPCB_SB2::decompose()
{
}

void zNPCB_SB2::update_turn(F32 dt)
{
    if (flag.face_player)
    {
        const xVec3& player_loc = (const xVec3&)globals.player.ent.model->Mat->pos;
        const xVec3& loc = location();

        turn.dir.assign(player_loc.x - loc.x, player_loc.z - loc.z);
        turn.dir.normalize();
    }

    xVec3& loc3 = location();
    xVec3& start3 = (xVec3&)model->Mat->at;
    xVec2 start_dir = { start3.x, start3.z };

    if (!turning())
    {
        return;
    }

    F32 start = xatan2(start_dir.x, start_dir.y);
    F32 end = xatan2(turn.dir.x, turn.dir.y);
    F32 diff = end - start;

    if (diff > PI)
    {
        diff -= 2.0f * PI;
    }
    else if (diff < -PI)
    {
        diff += 2.0f * PI;
    }

    F32 yaw = start;

    diff += yaw;

    xAccelMove(yaw, turn.vel, turn.accel, dt, diff, turn.max_vel);
    set_yaw_matrix(frame->mat, yaw);
}

void zNPCB_SB2::update_halt(F32 dt)
{
    // NOTE: retail really does leave `s` uninitialised here; xAccelStop takes it by
    // reference and reads it. Reproduced as-is.
    F32 s;
    F32 old_yaw = move.yaw;
    F32 yaw_accel = (move.yaw_vel < 0.0f) ? xabs(move.turn_accel) : -xabs(move.turn_accel);
    F32 accel = (move.vel < 0.0f) ? xabs(move.accel) : -xabs(move.accel);

    xAccelStop(move.yaw, move.yaw_vel, yaw_accel, dt);
    xAccelStop(s, move.vel, accel, dt);

    if (xfeq0(s) || xabs(old_yaw - move.yaw) <= 0.01f)
    {
        flag.move = MOVE_NONE;
    }
    else
    {
        xVec2 loc = { location().x, location().z };

        set_location(loc + move.dir * s);
    }
}

void zNPCB_SB2::update_follow(F32 dt)
{
    xVec2 loc = { location().x, location().z };
    xVec2 offset = move.dest - loc;
    F32 dist = offset.length();
    xVec2 dir;

    if (!xfeq0(dist))
    {
        dir = offset * (1.0f / dist);
    }
    else if (!xfeq0(move.vel))
    {
        dir = move.dir;
    }
    else
    {
        flag.move = MOVE_NONE;
        set_location(move.dest);
        return;
    }

    F32 dyaw = xatan2(dir.x, dir.y) - move.yaw;

    if (dyaw > PI)
    {
        dyaw -= 2.0f * PI;
    }
    else if (dyaw < -PI)
    {
        dyaw += 2.0f * PI;
    }

    xAccelMove(move.yaw, move.yaw_vel, move.turn_accel, dt, move.yaw + dyaw, move.turn_max_vel);
    move.yaw = xrmod(move.yaw);
    move.dir.assign(isin(move.yaw), icos(move.yaw));

    if (flag.face_follow)
    {
        turn.dir = move.dir;
    }

    if (dist < 1.0f)
    {
        move.dir = dir;
    }

    F32 end_s = move.dir.dot(offset);

    if (end_s < 0.0f)
    {
        end_s = 0.0f;
    }

    F32 s = 0.0f;

    xAccelMove(s, move.vel, move.accel, dt, end_s, move.max_vel);
    set_location(loc + move.dir * s);
}

void zNPCB_SB2::update_ymove(F32 dt)
{
    xVec3 loc = location();

    ymove.time += dt;

    F32 t = ymove.time / ymove.end_time;

    if (t >= 1.0f)
    {
        loc.y = ymove.end;
        flag.move = MOVE_NONE;
    }
    else
    {
        F32 s = xSCurve(t);
        loc.y = ymove.begin + s * (ymove.end - ymove.begin);
    }

    set_location(loc);
}

void zNPCB_SB2::update_move(F32 dt)
{
    switch (flag.move)
    {
    case MOVE_HALT:
        update_halt(dt);
        break;
    case MOVE_FOLLOW:
        update_follow(dt);
        break;
    case MOVE_Y:
        update_ymove(dt);
        break;
    }
}

void zNPCB_SB2::update_camera(F32 dt)
{
    zCameraDisableTracking(CO_BOSS);

    if (!(zCameraIsTrackingDisabled() & ~CO_BOSS))
    {
        boss_cam.update(dt);
    }
}

void zNPCB_SB2::update_nodes(F32 dt)
{
    node_pulse = xrmod(node_pulse + tweak.nodes.pulse_rate * dt);

    F32 range = tweak.nodes.pulse_max - tweak.nodes.pulse_min;

    set_glow_light_intensity(range * (0.5f * isin(node_pulse) + 0.5f) + tweak.nodes.pulse_min);

    for (S32 i = 0; i < 9; i++)
    {
        zEntDestructObj* ent = nodes[i].ent;

        if (ent == NULL)
        {
            continue;
        }

        if (zEntDestructObj_isDestroyed(ent))
        {
            xLightKit* kit = nodes[i].old_light_kit;

            ent->model->LightKit = kit;
            ent->lightKit = kit;
        }
        else
        {
            if (nodes[i].old_light_kit == NULL)
            {
                nodes[i].old_light_kit = ent->lightKit;
            }

            ent->model->LightKit = &glow_light.kit;
            ent->lightKit = &glow_light.kit;
        }
    }
}

void zNPCB_SB2::init_nodes()
{
    for (S32 i = 0; i < 9; i++)
    {
        xBase* base = zSceneFindObject(xStrHash(node_hooks[i].name));

        if (base == NULL || base->baseType != eBaseTypeDestructObj ||
            ((zEntDestructObj*)base)->model == NULL)
        {
            nodes[i].ent = NULL;
        }
        else
        {
            nodes[i].ent = (zEntDestructObj*)base;
            nodes[i].old_light_kit = NULL;
        }
    }

    bind_nodes();
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

void zNPCB_SB2::move_nodes()
{
    for (S32 i = 0; i < 9; i++)
    {
        node_data& n = nodes[i];

        if (n.ent == NULL)
        {
            continue;
        }

        xVec3 loc;
        xVec3 norm;
        xVec3 uploc;
        RpAtomic* m = n.skin_model;
        RwMatrixTag* skin_mat = n.skin_mat;

        if (node_hooks[i].points == 3)
        {
            xVec3 rightloc;

            iModelTagEval(m, &n.v3.tag[0], skin_mat, &loc);
            iModelTagEval(m, &n.v3.tag[1], skin_mat, &uploc);
            iModelTagEval(m, &n.v3.tag[2], skin_mat, &rightloc);

            norm = (rightloc - loc).cross(uploc - loc);
            norm.up_normalize();
        }
        else
        {
            iModelTagEval(m, &n.v2n1.tag, skin_mat, &loc, &norm);
            iModelTagEval(m, &n.v2n1.uptag, skin_mat, &uploc);
        }

        xMat4x3 mat;

        mat.up = uploc - loc;
        mat.at = norm;
        mat.right = mat.up.cross(mat.at).up_normalize();
        mat.up = mat.at.cross(mat.right);

        if (node_hooks[i].midpoint)
        {
            mat.pos = (loc + uploc) * 0.5f;
        }
        else
        {
            mat.pos = loc;
        }

        xEntReposition(*n.ent, mat);
    }
}

void zNPCB_SB2::render_nodes()
{
    xLightKit* old_light = xLightKit_GetCurrent(globals.currWorld);

    xLightKit_Enable(&glow_light.kit, globals.currWorld);

    for (S32 i = 0; i < 9; i++)
    {
        zEntDestructObj* ent = nodes[i].ent;

        if (ent != NULL && !iModelCull(ent->model->Data, ent->model->Mat))
        {
            iModelRender(ent->model->Data, ent->model->Mat);
        }
    }

    xLightKit_Enable(old_light, globals.currWorld);
}

void zNPCB_SB2::bind_nodes()
{
    flag.nodes_taken = false;

    for (S32 i = 0; i < 9; i++)
    {
        zEntDestructObj* ent = nodes[i].ent;

        if (ent == NULL)
        {
            nodes[i].skin_model = NULL;
            nodes[i].skin_mat = NULL;
        }
        else
        {
            ent->baseFlags &= (U16)0xfff7;
            nodes[i].skin_model = models[node_hooks[i].model]->Data;
            nodes[i].skin_mat = models[node_hooks[i].model]->Mat;
        }
    }

    setup_node_tags();
}

void zNPCB_SB2::rebind_nodes(RpAtomic* skin_model, RwMatrixTag* skin_mat)
{
    RpAtomic* skin_models[4];

    skin_models[0] = skin_model;

    for (S32 i = 1; i < 4; i++)
    {
        skin_models[i] = iModelFile_RWMultiAtomic(skin_models[i - 1]);

        if (skin_models[i] == NULL)
        {
            return;
        }
    }

    flag.nodes_taken = true;

    for (S32 i = 0; i < 9; i++)
    {
        zEntDestructObj* ent = nodes[i].ent;

        if (ent == NULL)
        {
            nodes[i].skin_model = NULL;
            nodes[i].skin_mat = NULL;
        }
        else
        {
            ent->baseFlags &= 0xfff7;
            nodes[i].skin_model = skin_models[node_hooks[i].model];
            nodes[i].skin_mat = skin_mat;
        }
    }

    setup_node_tags();
}

void zNPCB_SB2::setup_node_tags()
{
    for (S32 i = 0; i < 9; i++)
    {
        RpAtomic* m = nodes[i].skin_model;

        if (m == NULL)
        {
            continue;
        }

        if (node_hooks[i].points == 3)
        {
            for (S32 j = 0; j < 3; j++)
            {
                iModelTagSetup(&nodes[i].v3.tag[j], m, node_hooks[i].pos[j].x,
                               node_hooks[i].pos[j].y, node_hooks[i].pos[j].z);
            }
        }
        else
        {
            xVec3 loc1 = node_hooks[i].pos[1];

            iModelTagSetup(&nodes[i].v2n1.tag, m, node_hooks[i].pos[0].x,
                           node_hooks[i].pos[0].y, node_hooks[i].pos[0].z);
            iModelTagSetup(&nodes[i].v2n1.uptag, m, loc1.x, loc1.y, loc1.z);
        }
    }
}

void zNPCB_SB2::check_life()
{
    S32 old_life = life;

    life = 0;

    for (S32 i = 0; i < 9; i++)
    {
        if (nodes[i].ent != NULL && !zEntDestructObj_isDestroyed(nodes[i].ent))
        {
            life++;
        }
    }

    update_round();

    if (life < old_life)
    {
        ouchie();

        for (S32 i = life; i < old_life; i++)
        {
            zEntEvent(this, this, 0x1d7);
        }

        if (life <= 0)
        {
            zEntEvent(this, this, 0x24);
        }
    }
}

void zNPCB_SB2::ouchie()
{
    if (psy_instinct->GIDOfActive() != NPC_GOAL_BOSSSB2HIT)
    {
        set_vulnerable(false);
        psy_instinct->GoalSet(NPC_GOAL_BOSSSB2HIT, 1);
    }
}

void zNPCB_SB2::update_round()
{
    S32 old_round = round;

    round = 9 - life;

    if (round != old_round)
    {
        reset_stage();
    }

    if (round > old_round)
    {
        flag.dizzy = dizzy_round[round];
    }
}

xSurface& zNPCB_SB2::create_surface()
{
    xSurface* surf = (xSurface*)xMemAlloc(gActiveHeap, sizeof(xSurface), 0);
    zSurfaceProps* props = (zSurfaceProps*)xMemAlloc(gActiveHeap, sizeof(zSurfaceProps), 0);
    zSurfAssetBase* asset = (zSurfAssetBase*)xMemAlloc(gActiveHeap, sizeof(zSurfAssetBase), 0);

    xSurface& defsurf = zSurfaceGetDefault();

    *surf = defsurf;
    *props = *(zSurfaceProps*)defsurf.moprops;
    *asset = *props->asset;

    surf->moprops = props;
    props->asset = asset;

    return *surf;
}

void zNPCB_SB2::init_hands()
{
    S32 model_lookup[2] = { 1, 2 };

    for (S32 i = 0; i < 2; i++)
    {
        hands[i].hit_platforms = FALSE;
        hands[i].hurt_player = FALSE;
        hands[i].ent = &bounds[i].ent;

        init_bound_entity(*hands[i].ent, i, models[model_lookup[i]],
                          (xMat4x3*)xMemAlloc(gActiveHeap, sizeof(xMat4x3), 0));

        hands[i].ent->baseFlags |= 0x10;
        hands[i].ent->moreFlags &= 0xfd;
        hands[i].ent->flags |= 1;
        hands[i].ent->pflags |= 0x40;

        for (S32 j = 0; j < 4; j++)
        {
            xVec3 t = hand_hooks[i].tail[j];

            iModelTagSetup(&hands[i].head_tag[j], hands[i].ent->model->Data,
                           hand_hooks[i].head[j].x, hand_hooks[i].head[j].y,
                           hand_hooks[i].head[j].z);
            iModelTagSetup(&hands[i].tail_tag[j], hands[i].ent->model->Data, t.x, t.y, t.z);
        }

        if (hands[i].ent->model->Surf == NULL)
        {
            hands[i].ent->model->Surf = &create_surface();
        }

        ((zSurfaceProps*)hands[i].ent->model->Surf->moprops)->asset->game_damage_type = 0;
    }
}

void zNPCB_SB2::move_hand(zNPCB_SB2::hand_data& hand, F32 dt)
{
    xVec3 loc[8];
    xVec3* head_loc = loc;
    xVec3* tail_loc = loc + 4;

    for (S32 i = 0; i < 4; i++)
    {
        xModelInstance& m = *hand.ent->model;

        iModelTagEval(m.Data, &hand.head_tag[i], m.Mat, &head_loc[i]);
        iModelTagEval(m.Data, &hand.tail_tag[i], m.Mat, &tail_loc[i]);
    }

    xBound& obb = hand.ent->bound;
    const xVec3 old_loc = obb.mat->pos;

    parallelepiped_to_obb(obb, loc);
    xQuickCullForBound(&obb.qcd, &obb);
    zGridUpdateEnt(hand.ent);

    hand.radius = obb.box.box.upper.length();

    if (!hand.hurt_player)
    {
        return;
    }

    const xVec3 offset = obb.mat->pos - old_loc;
    xVec3 player_offset = (const xVec3&)globals.player.ent.model->Mat->pos - old_loc;

    if (offset.dot(player_offset) > 0.0f &&
        offset.length2() / (dt * dt) >= tweak.damage_speed * tweak.damage_speed)
    {
        bool damaged = player_damaged();
        S32 damage_type = 7;

        if (damaged)
        {
            damage_type = 0;
        }

        ((zSurfaceProps*)hand.ent->model->Surf->moprops)->asset->game_damage_type = damage_type;
        hand.ent->penby = 0;

        if (hand.hit_platforms)
        {
            check_platform_smack(hand);
        }
    }
    else
    {
        ((zSurfaceProps*)hand.ent->model->Surf->moprops)->asset->game_damage_type = 0;
        hand.ent->penby = 0x10;
    }
}

void zNPCB_SB2::spin_platform(zNPCB_SB2::platform_data& p, const xVec3& axis, F32 accel,
                              F32 max_vel)
{
    p.stopping = FALSE;
    p.spin.axis = axis;
    p.spin.ang = 0.0f;
    p.spin.end_ang = 1e38f;
    p.spin.max_vel = max_vel;
    p.spin.accel = accel;

    if (&p == player_platform())
    {
        say(7);
    }
}

void zNPCB_SB2::check_platform_smack(zNPCB_SB2::hand_data& hand)
{
    platform_data* it = platforms;
    platform_data* end = it + 16;

    for (; it != end; it++)
    {
        if (it->ent == NULL || it->spin.accel > 0.0f)
        {
            continue;
        }

        xBound& hand_bound = hand.ent->bound;
        xBound& plat_bound = it->ent->bound;
        xVec3 offset = plat_bound.sph.center - hand_bound.sph.center;

        if (offset.length2() > (hand.radius + it->radius) * (hand.radius + it->radius))
        {
            continue;
        }

        if (!xOBBHitsOBB(hand_bound.box.box, *hand_bound.mat, plat_bound.box.box, *plat_bound.mat))
        {
            continue;
        }

        F32 min_xzdist = tweak.spin.min_dist;
        xVec3 axis;

        if (offset.x * offset.x + offset.z * offset.z > min_xzdist * min_xzdist)
        {
            axis = it->mat.right;

            if (it->mat.at.dot(offset) > 0.0f)
            {
                axis.invert();
            }
        }
        else
        {
            axis = it->mat.at;
            axis.invert();
        }

        axis.normalize();

        spin_platform(*it, axis, tweak.spin.accel, tweak.spin.vel);
        play_sound(SOUND_CHOP_HIT, (const xVec3*)&it->ent->model->Mat->pos, 1.0f);
    }
}

void zNPCB_SB2::update_platforms(F32 dt)
{
    platform_data* it = platforms;
    platform_data* end = it + 16;

    for (; it != end; it++)
    {
        if (it->ent == NULL || it->spin.accel <= 0.0f)
        {
            continue;
        }

        xAccelMove(it->spin.ang, it->spin.vel, it->spin.accel, dt, it->spin.end_ang, it->spin.max_vel);
        it->spin.ang = xfmod(it->spin.ang, 1.0f);

        if (it->spin.vel >= it->spin.max_vel)
        {
            it->stopping = TRUE;
            it->spin.accel = tweak.spin.decel;
        }

        if (it->spin.vel >= tweak.spin.collide_vel)
        {
            it->ent->chkby &= 0xef;
            it->ent->penby &= 0xef;
        }
        else
        {
            it->ent->chkby |= 0x10;
            it->ent->penby |= 0x10;
        }

        xMat3x3& mat = *(xMat3x3*)it->ent->model->Mat;

        if (it->stopping)
        {
            it->spin.end_ang = 0.5f * std::floorf(2.0f * it->spin.ang + 0.5f);

            if (it->spin.vel >= -1e-5f && it->spin.vel <= 1e-5f &&
                xabs(it->spin.ang - it->spin.end_ang) <= 0.001f)
            {
                it->spin.ang = 0.0f;
                it->spin.vel = 0.0f;
                it->spin.accel = 0.0f;
                it->ent->chkby |= 0x10;
                it->ent->penby |= 0x10;
                continue;
            }
        }

        xMat3x3 rot_mat;

        xMat3x3Rot(&rot_mat, &it->spin.axis, 2.0f * PI * it->spin.ang);
        xMat3x3Mul(&mat, &it->mat, &rot_mat);
    }
}

void zNPCB_SB2::init_bounds()
{
    for (S32 i = 2; i < 5; i++)
    {
        init_bound_entity(bounds[i].ent, i, model, &bounds[i].mat);
    }
}

void zNPCB_SB2::reset_bounds()
{
    bound_data* it = &bounds[2];

    for (S32 i = 0; i < 3; i++, it++)
    {
        xBound& bound = it->ent.bound;

        if (tweak.bounds[i].bone >= (S32)model->BoneCount)
        {
            tweak.bounds[i].bone = model->BoneCount - 1;
        }

        if (tweak.bounds[i].is_sphere)
        {
            bound.type = XBOUND_TYPE_SPHERE;
            bound.sph.r = tweak.bounds[i].radius;
            bound.mat = NULL;
        }
        else
        {
            bound.type = XBOUND_TYPE_OBB;
            bound.box.box.upper = tweak.bounds[i].extent;
            bound.box.box.lower = -bound.box.box.upper;
            bound.mat = &it->mat;
            xMat3x3Euler(&it->rot_mat, tweak.bounds[i].yaw, tweak.bounds[i].pitch,
                         tweak.bounds[i].roll);
        }

        if (tweak.bounds[i].damage_player)
        {
            it->ent.penby = 0;
            it->ent.model = it->ent.collModel = models[0];
        }
        else
        {
            it->ent.penby = 0x10;
            it->ent.model = it->ent.collModel = models[3];
        }
    }
}

void zNPCB_SB2::update_bounds()
{
    bound_data* it = &bounds[2];

    for (S32 i = 0; i < 3; i++, it++)
    {
        xBound& bound = it->ent.bound;
        S32 bone = tweak.bounds[i].bone;
        const xMat4x3* bone_mat;
        xMat4x3 buffer_mat;

        if (bone == 0)
        {
            bone_mat = (const xMat4x3*)model->Mat;
        }
        else
        {
            const xMat4x3* skin = (const xMat4x3*)model->Mat;

            bone_mat = &buffer_mat;
            xMat4x3Mul(&buffer_mat, &skin[bone], skin);
        }

        xVec3 offset;

        xMat3x3RMulVec(&offset, bone_mat, &tweak.bounds[i].offset);
        bound.sph.center = bone_mat->pos + offset;

        if (!tweak.bounds[i].is_sphere)
        {
            xMat3x3Mul(bound.mat, &it->rot_mat, bone_mat);
            bound.mat->pos = bound.sph.center;
        }

        if (tweak.bounds[i].damage_player)
        {
            it->ent.penby = 0;

            bool damaged = player_damaged();
            S32 which = 0;

            if (damaged)
            {
                which = 3;
            }

            it->ent.model = it->ent.collModel = models[which];
        }
        else
        {
            it->ent.penby = 0x10;
            it->ent.model = it->ent.collModel = models[3];
        }

        xQuickCullForBound(&bound.qcd, &bound);
        zGridUpdateEnt(&it->ent);
    }
}

void zNPCB_SB2::init_slugs()
{
    for (S32 i = 0; i < 3; i++)
    {
        slug_data& slug = slugs[i];

        slug.stage = SLUG_INACTIVE;
        slug.ent = NULL;
        slug.sound_handle = 0;

        xBase* base = zSceneFindObject(xStrHash(slug_hooks[i].name));

        if (base == NULL || !xEntValidType(base->baseType))
        {
            continue;
        }

        slug.ent = (xEnt*)base;
        slug.ent->flags &= ~1;
        slug.ent->penby = 0;

        if (slug.ent->baseType == eBaseTypeStatic)
        {
            ((zEntSimpleObj*)slug.ent)->sflags |= 8;
        }

        if (slug.ent->model->Surf == NULL)
        {
            slug.ent->model->Surf = &create_surface();
        }

        ((zSurfaceProps*)slug.ent->model->Surf->moprops)->asset->game_damage_type = 7;
    }
}

void zNPCB_SB2::update_aim_slug(zNPCB_SB2::slug_data& slug, F32 dt)
{
    const xMat4x3* skin = (const xMat4x3*)model->Mat;

    xMat4x3Mul(&slug.mat, &skin[4], skin);

    xVec3 offset;

    xMat3x3RMulVec(&offset, &slug.mat, &tweak.karate.emit_offset);
    slug.mat.pos += offset;

    xVec3 dir;

    xMat3x3RMulVec(&dir, &slug.mat, &slug.move_dir);

    slug.stage_delay -= dt;

    if (slug.stage_delay <= 0.0f)
    {
        slug.mat.pos += dir * tweak.karate.aim_dist;

        if (slug.abandoned)
        {
            if (slug.sound_handle)
            {
                kill_sound(SOUND_KARATE_SLUG, slug.sound_handle);
                slug.sound_handle = 0;
            }

            slug.stage = SLUG_DYING;
        }
    }
    else
    {
        xAccelMove(slug.dist, slug.vel, slug.accel, dt, tweak.karate.aim_dist, slug.max_vel);
        slug.mat.pos += dir * slug.dist;
    }
}

void zNPCB_SB2::update_delay_slug(zNPCB_SB2::slug_data& slug, F32 dt)
{
    if (slug.abandoned)
    {
        if (slug.sound_handle)
        {
            kill_sound(SOUND_KARATE_SLUG, slug.sound_handle);
            slug.sound_handle = 0;
        }

        slug.stage = SLUG_DYING;
    }
    else
    {
        slug.stage_delay -= dt;

        if (slug.stage_delay <= 0.0f)
        {
            slug.stage = SLUG_FIRE;
        }
    }
}
void zNPCB_SB2::update_dying_slug(zNPCB_SB2::slug_data& slug, F32 dt)
{
    static F32 fade_time = 1.0f;

    slug.ent->model->Alpha -= fade_time * dt;

    if (!(slug.ent->model->Alpha > 0.0f))
    {
        slug.stage = SLUG_INACTIVE;
        slug.ent->flags &= ~1;
        slug.ent->model->Alpha = 0.0f;

        if (slug.sound_handle)
        {
            kill_sound(SOUND_KARATE_SLUG, slug.sound_handle);
            slug.sound_handle = 0;
        }
    }
}

void zNPCB_SB2::update_fire_slug(zNPCB_SB2::slug_data& slug, F32 dt)
{
    xAccelMove(slug.dist, slug.vel, tweak.karate.fire_accel, dt, tweak.karate.fire_vel);
    xAccelMove(slug.ydist, slug.yvel, tweak.karate.drop_accel, dt, slug.end_ydist,
               tweak.karate.drop_vel);

    xVec3 offset = { 0.0f, slug.ydist, slug.dist };

    xMat4x3Toworld(&offset, &slug.dmat, &offset);
    slug.mat.pos = offset;

    if (!slug.spun && slug.dist >= slug.end_dist)
    {
        spin_platform(*slug.target, slug.target->mat.at, tweak.spin.accel, tweak.spin.vel);
        play_sound(SOUND_KARATE_HIT, (const xVec3*)&slug.target->ent->model->Mat->pos, 1.0f);
        slug.spun = TRUE;
    }

    if (slug.dist > tweak.karate.kill_dist)
    {
        slug.stage = SLUG_INACTIVE;
        slug.ent->flags &= ~1;
        slug.ent->model->Alpha = 0.0f;

        if (slug.sound_handle)
        {
            kill_sound(SOUND_KARATE_SLUG, slug.sound_handle);
            slug.sound_handle = 0;
        }
    }
    else if (slug.dist > tweak.karate.fade_dist)
    {
        slug.ent->model->Alpha =
            1.0f - (slug.dist - tweak.karate.fade_dist) /
                       (tweak.karate.kill_dist - tweak.karate.fade_dist);
    }
    else
    {
        slug.ent->model->Alpha = 1.0f;
    }
}

void zNPCB_SB2::slug_interp(F32 time, F32& scale)
{
    F32 ct = rc_scale.clamp_t(time);

    static bool use_smooth = true;

    if (use_smooth)
    {
        rc_scale.eval_smooth(ct, &scale);
    }
    else
    {
        rc_scale.eval_linear(ct, &scale);
    }
}

namespace
{
    void response_curve::eval_linear(F32 t, F32* value)
    {
        F32* end = value + values;

        find_active_node(t);

        inode* n1 = curve + active_node;
        inode* n2 = curve + (active_node + 1);

        F32 t0 = n1->t;
        F32 dt = n2->t - t0;

        if (dt >= -1e-5f && dt <= 1e-5f)
        {
            for (F32* v1 = n1->value; value != end; value++, v1++)
            {
                *value = *v1;
            }
        }
        else
        {
            F32 u = (t - t0) / dt;

            for (F32 *v1 = n1->value, *v2 = n2->value; value != end; value++, v1++, v2++)
            {
                *value = u * (*v2 - *v1) + *v1;
            }
        }
    }

    void response_curve::find_active_node(F32 t)
    {
        u32 stride = values * sizeof(F32) + sizeof(node);
        U8* it = (U8*)curve + stride * active_node;

        while (true)
        {
            if (t < ((inode*)it)->t)
            {
                it -= stride;
                active_node--;
                continue;
            }

            if (t > ((inode*)(it + stride))->t)
            {
                it += stride;
                active_node++;
                continue;
            }

            return;
        }
    }

    F32 response_curve::clamp_t(F32 t) const
    {
        return range_limit(t, start_t(), end_t());
    }

    inode* response_curve::get_node(u32 index) const
    {
        return (inode*)((U8*)curve + index * (sizeof(node) + values * sizeof(F32)));
    }

    F32 response_curve::start_t() const
    {
        return curve->t;
    }

    F32 response_curve::end_t() const
    {
        return get_node(nodes - 1)->t;
    }

    void response_curve::eval_smooth(F32 t, F32* value)
    {
        if (nodes == 2)
        {
            eval_linear(t, value);
            return;
        }

        F32* end = value + values;

        find_active_node(t);

        U32 index = active_node;
        inode* n0 = curve + index;
        inode* n1 = curve + (index + 1);

        F32 dt = n1->t - n0->t;

        if (xfeq0(dt))
        {
            for (F32* a = n0->value; value != end; value++, a++)
            {
                *value = *a;
            }
        }
        else
        {
            F32 u = (t - n0->t) / dt;
            F32 u2 = u * u;
            F32 u3 = u2 * u;

            F32 cm = u2 + -0.5f * u3 + -0.5f * u;
            F32 c0 = 1.5f * u3 + -2.5f * u2 + 1.0f;
            F32 c1 = -1.5f * u3 + 2.0f * u2 + 0.5f * u;
            F32 cp = 0.5f * u3 + -0.5f * u2;

            if (index != 0 && index < nodes - 2)
            {
                F32* pm = curve[index - 1].value;
                F32* pp = curve[index + 2].value;

                for (F32 *a = n0->value, *b = n1->value; value != end;
                     value++, a++, b++, pm++, pp++)
                {
                    *value = cm * *pm + c0 * *a + c1 * *b + cp * *pp;
                }
            }
            else if (index != 0)
            {
                F32* p = curve[index - 1].value;
                F32 cb = c1 + cp;

                for (F32 *a = n0->value, *b = n1->value; value != end; value++, a++, b++, p++)
                {
                    *value = cm * *p + c0 * *a + cb * *b;
                }
            }
            else
            {
                F32 ca = cm + c0;

                for (F32 *a = n0->value, *b = n1->value, *p = curve[index + 2].value;
                     value != end; value++, a++, b++, p++)
                {
                    *value = ca * *a + c1 * *b + cp * *p;
                }
            }
        }
    }
} // namespace

void zNPCB_SB2::update_slugs(F32 dt)
{
    for (slug_data *it = slugs, *end = it + MAX_SLUG; it != end; it++)
    {
        switch (it->stage)
        {
        case SLUG_AIM:
            update_aim_slug(*it, dt);
            break;
        case SLUG_DELAY:
            update_delay_slug(*it, dt);
            break;
        case SLUG_DYING:
            update_dying_slug(*it, dt);
            break;
        case SLUG_FIRE:
            update_fire_slug(*it, dt);
            break;
        default:
            continue;
        }

        it->time += dt;

        F32 scale;

        slug_interp(it->time, scale);

        if (scale <= 0.0f || it->ent->model->Alpha <= 0.0f)
        {
            it->ent->flags &= ~1;
        }
        else
        {
            it->ent->flags |= 1;
            it->ent->model->Scale = scale;
            xEntReposition(*it->ent, it->mat);
        }
    }
}

void zNPCB_SB2::scan_cronies()
{
    st_XORDEREDARRAY* npclist = zNPCMgr_GetNPCList();

    for (S32 i = 0; i < npclist->cnt; i++)
    {
        zNPCCommon* npc = (zNPCCommon*)npclist->list[i];

        if (npc->SelfType() == NPC_TYPE_BOSSPLANKTON)
        {
            plankton = (zNPCBPlankton*)npc;
            return;
        }
    }
}

void zNPCB_SB2::check_hit_fail()
{
    bool exploding = cruise_bubble::exploding() > 0.0f;

    if (exploding && !flag.cruise_exploding)
    {
        flag.cruise_exploding = true;
        flag.cruise_hit_target = false;
        flag.cruise_hit_body = false;
    }

    if (!exploding && flag.cruise_exploding)
    {
        if (!flag.cruise_hit_target && flag.cruise_hit_body)
        {
            say(5);
        }

        flag.cruise_exploding = false;
    }
    else if (flag.cruise_exploding)
    {
        if (!flag.cruise_hit_target)
        {
            S32 hits_size;
            xEnt** hits = cruise_bubble::get_explode_hits(hits_size);

            xEnt** pCurHit = hits;
            xEnt** pLastHit = &pCurHit[hits_size];

            for (; pCurHit != pLastHit; pCurHit++)
            {
                xEnt* hit = *pCurHit;

                if (hit == (xEnt*)plankton)
                {
                    flag.cruise_hit_target = true;
                    break;
                }

                platform_data* pCurPlat = platforms;
                platform_data* pLastPlat = &pCurPlat[16];

                for (; pCurPlat != pLastPlat; pCurPlat++)
                {
                    if (hit == pCurPlat->ent)
                    {
                        flag.cruise_hit_target = true;
                        break;
                    }
                }

                if (flag.cruise_hit_target)
                {
                    break;
                }
            }
        }

        if (!flag.cruise_hit_target && !flag.cruise_hit_body)
        {
            xSphere o;

            cruise_bubble::get_explode_sphere(o.center, o.r);

            bound_data* pCurBound = bounds;
            bound_data* pLastBound = &pCurBound[5];

            for (; pCurBound != pLastBound; pCurBound++)
            {
                if (xSphereHitsBound(o, pCurBound->ent.bound))
                {
                    flag.cruise_hit_body = true;
                    return;
                }
            }
        }
    }
}

void zNPCB_SB2::create_glow_light()
{
    memset(&glow_light, 0, sizeof(glow_light));

    xLightKit* dlk = NULL;
    U32& total = glow_light.kit.lightCount;
    U32 id = globals.sceneCur->zen->easset->objectLightKit;

    if (id != 0)
    {
        dlk = (xLightKit*)xSTFindAsset(id, NULL);
    }

    total = 1;
    glow_light.kit.tagID = 'TIKL';
    glow_light.kit.lightList = glow_light.light;
    glow_light.light[0].type = 1;
    glow_light.light[0].color.red = 1.0f;
    glow_light.light[0].color.green = 1.0f;
    glow_light.light[0].color.blue = 1.0f;
    glow_light.light[0].color.alpha = 1.0f;

    if (dlk != NULL)
    {
        for (U32 i = 0; i < dlk->lightCount && total < 8; i++)
        {
            xLightKitLight& src = dlk->lightList[i];
            xLightKitLight& dst = glow_light.light[glow_light.kit.lightCount];

            if (src.type != 1)
            {
                dst = src;
                dst.platLight = NULL;
                total++;
            }
        }
    }

    xLightKit_Prepare(&glow_light.kit);
}

void zNPCB_SB2::destroy_glow_light()
{
    xLightKit_Destroy(&glow_light.kit);
}

void zNPCB_SB2::set_glow_light_intensity(F32 intensity)
{
    glow_light.light[0].color.red = intensity;
    glow_light.light[0].color.green = intensity;
    glow_light.light[0].color.blue = intensity;
    RpLightSetColor(glow_light.light[0].platLight, &glow_light.light[0].color);
}

void zNPCB_SB2::say(int which)
{
    if (newsfish != NULL)
    {
        newsfish->say(say_set[which].list, say_set[which].size, 0, -1);
    }
}

xFactoryInst* zNPCGoalBossSB2Intro::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Intro(who, *(zNPCB_SB2*)info);
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

S32 zNPCGoalBossSB2Intro::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (owner.delay >= tweak.intro_time)
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BOSSSB2IDLE;
    }

    return 0;
}

S32 zNPCGoalBossSB2Intro::Exit(F32 dt, void* updCtxt)
{
    zEntPlayerControlOn(CONTROL_OWNER_BOSS);
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBossSB2Idle::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Idle(who, *(zNPCB_SB2*)info);
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

S32 zNPCGoalBossSB2Idle::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (transitioning)
    {
        if (g_hash_bossanim[ANIM_Idle01] != owner.AnimCurStateID())
        {
            return 0;
        }

        transitioning = FALSE;
        owner.flag.face_player = true;
        owner.delay = 0.0f;
    }

    if (owner.player_on_ground())
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BOSSSB2HUNT;
    }

    if (owner.delay >= owner.stage_delay)
    {
        owner.stage_delay = 0.0f;
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }

    return 0;
}

xFactoryInst* zNPCGoalBossSB2Taunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Taunt(who, *(zNPCB_SB2*)info);
}

S32 zNPCGoalBossSB2Taunt::Enter(F32 dt, void* updCtxt)
{
    play_sound(0, &owner.sound_loc.mouth , 1.0f);
    owner.flag.face_player = 1;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Taunt::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (owner.AnimTimeRemain(NULL) < dt + 0.001f)
    {
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }

    return 0;
}

S32 zNPCGoalBossSB2Taunt::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBossSB2Dizzy::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Dizzy(who, *(zNPCB_SB2*)info);
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
    if (sicked != false && owner.player_on_ground() == 0)  //Not compared correctly
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

S32 zNPCGoalBossSB2Dizzy::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (!owner.flag.dizzy)
    {
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }

    if (owner.player_on_ground())
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BOSSSB2HUNT;
    }

    return 0;
}

xFactoryInst* zNPCGoalBossSB2Hit::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Hit(who, *(zNPCB_SB2*)info);
}

S32 zNPCGoalBossSB2Hit::Enter(F32 dt, void* updCtxt) 
{
    owner.flag.face_player = 1;
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
    return new (who, grow) zNPCGoalBossSB2Hunt(who, *(zNPCB_SB2*)info);
}

S32 zNPCGoalBossSB2Hit::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (owner.AnimTimeRemain(NULL) < dt + 0.001f)
    {
        if (owner.life <= 0)
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BOSSSB2DEATH;
        }

        if (owner.flag.dizzy)
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BOSSSB2DIZZY;
        }

        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }

    return 0;
}

S32 zNPCGoalBossSB2Hunt::Enter(F32 dt, void* updCtxt)
{
    owner.flag.face_player = true;
    owner.reset_stage();
    following = FALSE;
    owner.delay = 0.0f;
    owner.ymove.begin = owner.start_location().y;
    owner.ymove.end = owner.ymove.begin + tweak.hunt.height;

    F32 t = range_limit((owner.location().y - owner.ymove.begin) / tweak.hunt.height, 0.0f, 1.0f);

    owner.ymove.end_time = tweak.hunt.move_time;
    owner.ymove.time = owner.ymove.end_time * xSCurveInverse(t);
    owner.flag.move = zNPCB_SB2::MOVE_Y;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

F32 xSCurveInverse(F32 val)
{
    F32 half = 0.5f * val;

    if (0.25f + half < 0.5f)
    {
        return xsqrt(half);
    }

    return 1.0f - xsqrt(0.5f * (1.0f - val));
}

S32 zNPCGoalBossSB2Hunt::Exit(F32 dt, void* updCtxt)
{
    owner.ymove.end = owner.start_location().y;
    owner.ymove.begin = owner.ymove.begin + tweak.hunt.height;

    F32 t = range_limit((owner.ymove.end - owner.location().y) / tweak.hunt.height, 0.0f, 1.0f);

    owner.ymove.end_time = tweak.hunt.move_time;
    owner.ymove.time = owner.ymove.end_time * xSCurveInverse(t);
    owner.flag.move = zNPCB_SB2::MOVE_Y;

    owner.plankton->here_boy();
    owner.say(10);

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSB2Hunt::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (!owner.flag.dizzy && owner.player_damaged())
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BOSSSB2TAUNT;
    }

    bool on_ground = owner.player_on_ground();

    if (!following)
    {
        if (on_ground)
        {
            if (owner.delay >= tweak.hunt.warm_up)
            {
                following = TRUE;
                owner.plankton->sickum();
                owner.delay = 0.0f;
                owner.say(12);
            }

            return 0;
        }

        *trantype = GOAL_TRAN_SET;

        return owner.next_goal();
    }

    if (!on_ground)
    {
        if (owner.delay >= tweak.hunt.cool_down)
        {
            owner.plankton->here_boy();
            *trantype = GOAL_TRAN_SET;

            return owner.next_goal();
        }

        return 0;
    }

    owner.delay = 0.0f;

    return 0;
}

xFactoryInst* zNPCGoalBossSB2Swipe::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Swipe(who, *(zNPCB_SB2*)info);
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

S32 zNPCGoalBossSB2Swipe::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (!started)
    {
        if (can_start())
        {
            started = TRUE;
            owner.flag.face_player = false;
            owner.activate_hand(owner.active_hand, FALSE);
            owner.AnimStart(begin_anim, 0);
            play_sound(SOUND_SWIPE, &owner.sound_loc.hand[owner.active_hand], 1.0f);
        }

        if (owner.player_on_ground())
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BOSSSB2HUNT;
        }

        return 0;
    }

    if (!holding &&
        (begin_anim != owner.AnimCurStateID() || owner.AnimTimeRemain(NULL) < dt + 0.001f))
    {
        owner.delay = 0.0f;
        holding = TRUE;
    }

    if (holding && !said && owner.delay > tweak.help.delay_vuln)
    {
        if (!cruise_bubble::active())
        {
            owner.say(8);
        }

        said = TRUE;
    }

    if (holding && owner.player_on_ground())
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BOSSSB2HUNT;
    }

    if (holding && owner.delay >= tweak.swipe.hold_time)
    {
        *trantype = GOAL_TRAN_SET;
        return owner.next_goal();
    }

    return 0;
}

bool zNPCGoalBossSB2Swipe::can_start() const
{
    zNPCB_SB2::platform_data* platform = owner.player_platform();
    return platform != NULL;
}

xFactoryInst* zNPCGoalBossSB2Chop::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Chop(who, *(zNPCB_SB2*)info);
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

S32 zNPCGoalBossSB2Chop::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (!started)
    {
        if (owner.player_on_ground())
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BOSSSB2HUNT;
        }

        if (owner.player_damaged())
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BOSSSB2TAUNT;
        }

        if (targetted)
        {
            if (owner.delay >= tweak.chop.delay)
            {
                started = TRUE;
                owner.AnimStart(loop_anim, 1);
                play_sound(SOUND_CHOP_SWING, &owner.sound_loc.hand[owner.active_hand], 1.0f);
            }
        }
        else if (can_start())
        {
            owner.flag.face_player = false;

            if (g_hash_bossanim[ANIM_Idle01] == owner.AnimCurStateID())
            {
                started = TRUE;
                owner.AnimStart(begin_anim, 0);
                play_sound(SOUND_CHOP_WINDUP, &owner.sound_loc.hand[owner.active_hand], 1.0f);
            }
            else
            {
                targetted = TRUE;
                owner.delay = 0.0f;
            }
        }

        return 0;
    }

    if (loop_anim == owner.AnimCurStateID() && owner.AnimTimeRemain(NULL) < dt + 0.001f)
    {
        S32 next = owner.next_goal();

        if (next != NPC_GOAL_BOSSSB2CHOP)
        {
            *trantype = GOAL_TRAN_SET;
            return next;
        }

        targetted = FALSE;
        started = FALSE;
        owner.flag.face_player = true;
    }

    return 0;
}

bool zNPCGoalBossSB2Chop::can_start() const
{
    zNPCB_SB2::platform_data* target = owner.player_platform();

    if (target == NULL)
    {
        return FALSE;
    }

    xBound& bound = target->ent->bound;
    xMat4x3& mat = *bound.mat;
    xVec3 offset = mat.pos - owner.location();
    xVec3& facing = owner.facing();
    F32 facing_yaw = xatan2(facing.x, facing.z);
    F32 target_yaw = xatan2(offset.x, offset.z);
    F32 dyaw = xrmod(PI + (facing_yaw - target_yaw));

    return xabs(dyaw - PI) < 0.19634955f;
}

xFactoryInst* zNPCGoalBossSB2Karate::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Karate(who, *(zNPCB_SB2*)info);
}

S32 zNPCGoalBossSB2Karate::Enter(F32 dt, void* updCtxt)
{
    started = FALSE;
    owner.flag.face_player = TRUE;

    U8* pCurEmitted = emitted;
    U8* pLastEmitted = &pCurEmitted[zNPCB_SB2::MAX_SLUG];

    for (; pCurEmitted != pLastEmitted; pCurEmitted++)
    {
        *pCurEmitted = FALSE;
    }

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSB2Karate::Exit(F32 dt, void* updCtxt)
{
    owner.abandon_slugs();
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSB2Karate::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    if (!started)
    {
        if (can_start())
        {
            started = TRUE;
            owner.delay = 0.0f;

            if (g_hash_bossanim[ANIM_Idle01] == owner.AnimCurStateID())
            {
                owner.AnimStart(g_hash_bossanim[ANIM_KarateStart], 0);
            }
        }

        if (owner.player_on_ground())
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BOSSSB2HUNT;
        }

        if (owner.player_damaged())
        {
            *trantype = GOAL_TRAN_SET;
            return NPC_GOAL_BOSSSB2TAUNT;
        }

        return 0;
    }

    S32 count = 0;

    for (S32 i = 0; i < 3; i++)
    {
        if (!emitted[i] && owner.delay >= tweak.karate.delay_emit[i])
        {
            owner.emit_slug((zNPCB_SB2::slug_enum)i);
            emitted[i] = TRUE;
        }

        if (emitted[i])
        {
            count++;
        }
    }

    if (owner.slugs_ready())
    {
        zNPCB_SB2::platform_data* platform = owner.player_platform();

        if (platform != NULL)
        {
            S32 index = owner.platform_index(*platform);

            owner.fire_slug(zNPCB_SB2::SLUG_KAH, owner.platforms[(index - 1) & 0xf]);
            owner.fire_slug(zNPCB_SB2::SLUG_RAH, owner.platforms[index]);
            owner.fire_slug(zNPCB_SB2::SLUG_TAY, owner.platforms[(index + 1) & 0xf]);
            owner.flag.face_player = false;
        }
    }

    if (owner.player_on_ground())
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BOSSSB2HUNT;
    }

    if (owner.player_damaged())
    {
        *trantype = GOAL_TRAN_SET;
        return NPC_GOAL_BOSSSB2TAUNT;
    }

    if (count == 3 && owner.slugs_inactive())
    {
        S32 goal = owner.next_goal();

        if (goal != NPC_GOAL_BOSSSB2KARATE)
        {
            *trantype = GOAL_TRAN_SET;
            return goal;
        }

        U8* it = emitted;
        U8* end = it + 3;

        started = FALSE;
        owner.flag.face_player = true;

        while (it != end)
        {
            *it++ = FALSE;
        }
    }

    return 0;
}

bool zNPCGoalBossSB2Karate::can_start() const
{
    zNPCB_SB2::platform_data* platform = owner.player_platform();
    return platform != NULL;
}

xFactoryInst* zNPCGoalBossSB2Death::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBossSB2Death(who, *(zNPCB_SB2*)info);
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

        v *= scale;

        value = v;
    }

    template <>
    inline void load_param<bool, S32>(bool& value, S32, S32, S32, xModelAssetParam* ap, U32 apsize,
                               const char* name)
    {
        value = zParamGetInt(ap, apsize, name, value) != 0;
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
} // namespace auto_tweak

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

void zNPCB_SB2::set_location(const xVec2& loc)
{
    // Retail really does splat the scalar through xVec3::operator=(F32) here, and
    // really does aim the second one at &pos.z rather than at &pos.
    (xVec3&)model->Mat->pos.x = frame->mat.pos.x = loc.x;
    (xVec3&)model->Mat->pos.z = frame->mat.pos.z = loc.y;
}

void zNPCB_SB2::set_location(const xVec3& loc)
{
    (xVec3&)model->Mat->pos = frame->mat.pos = loc;
}

xVec3& zNPCB_SB2::start_location() const
{
    return reinterpret_cast<xVec3&>(this->asset->pos);
}

xVec3& zNPCB_SB2::facing() const
{
    return reinterpret_cast<xVec3&>(this->model->Mat->at);
}
