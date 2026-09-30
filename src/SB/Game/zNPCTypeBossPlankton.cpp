#include "zNPCTypeBossPlankton.h"
#include "xDebug.h"
#include "xGroup.h"
#include "zNPCMgr.h"
#include "zScene.h"

#include <types.h>

namespace std
{
    float fabsf(float);
}

#define f1585 1.0f
#define f1586 0.0f
#define f1657 0.2f
#define f1658 0.1f

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

namespace
{
    struct sound_data_type
    {
        U32 id;
        U32 handle;
        xVec3* loc;
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

    struct bolt;

    struct effect_data
    {
        struct effect_callback
        {
            void (*fp)(bolt&, void*);
            void* context;
        };

        fx_type_enum type;
        fx_orient_enum orient;
        F32 rate;
        union
        {
            xParEmitter* par;
            xDecalEmitter* decal;
            effect_callback callback;
        };
        F32 irate;
    };

    static effect_data beam_launch_effect[2]; // size: 0x30, address: 0x4E0E60
    static effect_data beam_head_effect[1]; // size: 0x18, address: 0x4E0E90
    static effect_data beam_impact_effect[3]; // size: 0x48, address: 0x4E0EB0
    static effect_data beam_death_effect[1]; // size: 0x18, address: 0x5E53F0
    static effect_data beam_kill_effect[1];

    static U32 sound_asset_ids[6][10];
    static sound_data_type sound_data[6];

    static const sound_asset sound_assets[29] = {
        { 0, "RSB_foot_loop", 0, 3 },       { 0, "fan_loop", 0, 3 },
        { 0, "Rocket_burn_loop", 0, 3 },    { 0, "RP_whirr_loop", 0, 3 },
        { 0, "RP_whirr2_loop", 0, 3 },      { 0, "Glove_hover", 0, 3 },
        { 0, "Glove_pursuit", 0, 3 },       { 1, "Prawn_FF_hit", 0, 0 },
        { 1, "Prawn_hit", 0, 0 },           { 1, "Door_metal_shut", 0, 0 },
        { 1, "Ghostplat_fall", 0, 0 },      { 1, "ST-death", 0, 0 },
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

    static const xDecalEmitter::curve_node beam_ring_curve[2] = {
        { 0.0f, { 255, 255, 255, 255 }, 0.0f },
        { 1.0f, { 255, 255, 255, 0 }, 1.0f },
    };

    static const xDecalEmitter::curve_node beam_glow_curve[3] = {
        { 0.0f, { 255, 255, 255, 255 }, 0.0f },
        { 0.5f, { 255, 0, 255, 255 }, 4.0f },
        { 1.0f, { 255, 0, 0, 0 }, 6.0f },
    };

    static const zNPCNewsFish::say_enum say_intro[2] = {
        zNPCNewsFish::SAY_B303_INTRO_1,
        zNPCNewsFish::SAY_B303_INTRO_2,
    };

    static const zNPCNewsFish::say_enum say_fuse_near[4] = {
        zNPCNewsFish::SAY_B303_FUSE_NEAR,
        zNPCNewsFish::SAY_ROBOT_VULN_2,
        zNPCNewsFish::SAY_SB_VULN_3,
        zNPCNewsFish::SAY_SB_VULN_4,
    };

    static const zNPCNewsFish::say_enum say_fuse_hit[3] = {
        zNPCNewsFish::SAY_B303_FUSE_HIT,
        zNPCNewsFish::SAY_HIT_BOSS_1,
        zNPCNewsFish::SAY_SB_HIT_BOSS_1,
    };

    static const zNPCNewsFish::say_enum say_hit_boss_1[6] = {
        zNPCNewsFish::SAY_HIT_BOSS_1,    zNPCNewsFish::SAY_HIT_BOSS_2,
        zNPCNewsFish::SAY_SB_HIT_BOSS_2, zNPCNewsFish::SAY_SB_HIT_BOSS_3,
        zNPCNewsFish::SAY_ROBOT_HIT,     zNPCNewsFish::SAY_ROBOT_STUN_1,
    };

    static const zNPCNewsFish::say_enum say_hit_boss_2[3] = {
        zNPCNewsFish::SAY_B303_BRAIN_HELP_1,
        zNPCNewsFish::SAY_B303_BRAIN_HELP_2,
        zNPCNewsFish::SAY_B303_BRAIN_HELP_3,
    };

    static const zNPCNewsFish::say_enum say_hit_boss_3[6] = {
        zNPCNewsFish::SAY_B303_BRAIN_HELP_1, zNPCNewsFish::SAY_B303_BRAIN_HELP_2,
        zNPCNewsFish::SAY_B303_BRAIN_HELP_3, zNPCNewsFish::SAY_SB_VULN_1,
        zNPCNewsFish::SAY_SB_VULN_2,         zNPCNewsFish::SAY_SB_VULN_5,
    };

    static const zNPCNewsFish::say_enum say_hit_boss_4[1] = {
        zNPCNewsFish::SAY_HIT_LAST,
    };

    static const zNPCNewsFish::say_enum say_hit_player[6] = {
        zNPCNewsFish::SAY_HIT_PLAYER_1, zNPCNewsFish::SAY_HIT_PLAYER_2,
        zNPCNewsFish::SAY_HIT_PLAYER_3, zNPCNewsFish::SAY_HIT_PLAYER_4,
        zNPCNewsFish::SAY_HIT_PLAYER_5, zNPCNewsFish::SAY_HIT_PLAYER_6,
    };

    static const struct
    {
        const zNPCNewsFish::say_enum* say;
        U32 size;
    } say_set[8] = {
        { say_intro, 2 },      { say_fuse_near, 4 },  { say_fuse_hit, 3 },
        { say_hit_boss_1, 6 }, { say_hit_boss_2, 3 }, { say_hit_boss_3, 6 },
        { say_hit_boss_4, 1 }, { say_hit_player, 6 },
    };

    xVec3* get_player_loc()
    {
        return (xVec3*)&globals.player.ent.model->Mat->pos;
    }

    S32 init_sound()
    {
        return 0;
    }

    void reset_sound()
    {
        for (S32 i = 0; i < 6; ++i)
        {
            sound_data[i].handle = 0;
        }
    }

    void* play_sound(int, const xVec3*, F32)
    {
        return NULL;
    }

    void* kill_sound(S32, U32)
    {
        return 0; // to-do
    }

    void* kill_sound(S32)
    {
        return 0;
    }

    void play_beam_fly_sound(xLaserBoltEmitter::bolt& bolt, void* unk)
    {
        if (bolt.context == NULL)
        {
            bolt.context = play_sound(SOUND_BOLT_FLY, &bolt.loc, 1.0f);
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

} // namespace

xAnimTable* ZNPC_AnimTable_BossPlankton()
{
    // clang-format off
    S32 ourAnims[32] = {            //dwarf says it should be 32, matches less with 15
        ANIM_Idle01,
        ANIM_Taunt01,
        ANIM_move,
        ANIM_stun_begin,
        ANIM_stun_loop,
        ANIM_stun_end,
        ANIM_attack_beam_begin,
        ANIM_attack_beam_loop,
        ANIM_attack_beam_end,
        ANIM_attack_wall_begin,
        ANIM_attack_wall_loop,
        ANIM_attack_wall_end,
        ANIM_attack_missle,
        ANIM_attack_bomb,
        ANIM_Unknown,
    };
    // clang-format on

    xAnimTable* table = xAnimTableNew("zNPCBPlankton", NULL, 0);

    xAnimTableNewState(table, g_strz_bossanim[ANIM_Idle01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 3;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_Taunt01], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x42;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_move], 0x10, 0, f1585, NULL, NULL, f1586, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x43;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_begin], 0x20, 0, f1585, NULL, NULL, f1586,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x44;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_loop], 0x10, 0, f1585, NULL, NULL, f1586,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x45;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_stun_end], 0x20, 0, f1585, NULL, NULL, f1586,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x46;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_begin], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x47;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_loop], 0x10, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x48;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_beam_end], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x49;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_begin], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x4a;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_loop], 0x10, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x4b;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_wall_end], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x4c;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_missle], 0x20, 0, f1585, NULL, NULL,
                       f1586, NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0x4d;
    xAnimTableNewState(table, g_strz_bossanim[ANIM_attack_bomb], 0x20, 0, f1585, NULL, NULL, f1586,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    ourAnims[0] = 0;

    NPCC_BuildStandardAnimTran(table, g_strz_bossanim, ourAnims, 1, f1657);

    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_stun_begin],
                            g_strz_bossanim[ANIM_stun_loop], 0, 0, 0x10, 0, f1586, f1586, 0, 0,
                            f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_stun_loop], g_strz_bossanim[ANIM_stun_end],
                            0, 0, 0, 0, f1586, f1586, 0, 0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_attack_beam_loop], 0, 0, 0x10, 0, f1586, f1586, 0,
                            0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_attack_beam_end], 0, 0, 0, 0, f1586, f1586, 0, 0,
                            f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_loop],
                            g_strz_bossanim[ANIM_attack_beam_end], 0, 0, 0, 0, f1586, f1586, 0, 0,
                            f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_begin],
                            g_strz_bossanim[ANIM_attack_wall_loop], 0, 0, 0x10, 0, f1586, f1586, 0,
                            0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_loop],
                            g_strz_bossanim[ANIM_attack_wall_end], 0, 0, 0, 0, f1586, f1586, 0, 0,
                            f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_Taunt01], g_strz_bossanim[ANIM_stun_begin],
                            0, 0, 0, 0, f1586, f1586, 0, 0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_move], g_strz_bossanim[ANIM_stun_begin], 0,
                            0, 0, 0, f1586, f1586, 0, 0, f1658, 0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_begin],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_loop],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_beam_end],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_begin],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_loop],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_wall_end],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_missle],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);
    xAnimTableNewTransition(table, g_strz_bossanim[ANIM_attack_bomb],
                            g_strz_bossanim[ANIM_stun_begin], 0, 0, 0, 0, f1586, f1586, 0, 0, f1658,
                            0);

    return table;
}

zNPCBPlankton::zNPCBPlankton(S32 myType) : zNPCBoss(myType)
{
    memset(&flag, 0, sizeof(flag));
}

void zNPCBPlankton::Init(xEntAsset* asset)
{
    init_sound();
    zNPCCommon::Init(asset);
    flg_move = 1;
    flg_vuln = 1;
    RestoreColFlags();
    territory_size = 0;
    played_intro = false;
    init_beam();
    model->Anim->BeforeAnimMatrices = aim_gun;
}

void zNPCBPlankton::Setup()
{
    zNPCBoss::Setup();
    setup_beam();
    newsfish = (zNPCNewsFish*)zSceneFindObject(xStrHash("NPC_NEWSCASTER"));
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

    reset_sound();
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
    delay += dt;

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
        xVec3& player_loc = *(xVec3*)&globals.player.ent.model->Mat->pos;
        const xVec3& loc = location();
        turn.dir.assign(player_loc.x - loc.x, player_loc.z - loc.z);
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
}

void zNPCBPlankton::Render()
{
    xNPCBasic::Render();
    zNPCBPlankton::render_debug();
}

void zNPCBPlankton::RenderExtraPostParticles()
{
    if (beam.visible())
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
            sound_data[SOUND_HOVER].id = xStrHash(sound_assets[sound[SOUND_HOVER].asset].name);
        }
        if (init)
        {
            sound[SOUND_HIT].asset = sound_asset_ids[1][3];
            sound_data[SOUND_HIT].id = xStrHash(sound_assets[sound[SOUND_HIT].asset].name);
        }
        if (init)
        {
            sound[SOUND_BOLT_FIRE].asset = sound_asset_ids[2][0];
            sound_data[SOUND_BOLT_FIRE].id =
                xStrHash(sound_assets[sound[SOUND_BOLT_FIRE].asset].name);
        }
        if (init)
        {
            sound[SOUND_BOLT_FLY].asset = sound_asset_ids[3][3];
            sound_data[SOUND_BOLT_FLY].id =
                xStrHash(sound_assets[sound[SOUND_BOLT_FLY].asset].name);
        }
        if (init)
        {
            sound[SOUND_BOLT_HIT].asset = sound_asset_ids[4][3];
            sound_data[SOUND_BOLT_HIT].id =
                xStrHash(sound_assets[sound[SOUND_BOLT_HIT].asset].name);
        }
        if (init)
        {
            sound[SOUND_CHARGE].asset = sound_asset_ids[5][3];
            sound_data[SOUND_CHARGE].id = xStrHash(sound_assets[sound[SOUND_CHARGE].asset].name);
        }
    }
} // namespace

void zNPCBPlankton::ParseLinks()
{
    zNPCCommon::ParseLinks();

    memset(territory, 0, sizeof(territory));

    xLinkAsset* it = link;
    xLinkAsset* end = it + linkCount;

    for (; it != end; ++it)
    {
        if (it->dstEvent == eEventConnectToChild)
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

    for (S32 i = 0; i < 8; ++i)
    {
        if (territory[i].origin != NULL)
        {
            if (i != territory_size)
            {
                territory[territory_size] = territory[i];
            }

            ++territory_size;
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

void zNPCBPlankton::Damage(en_NPC_DAMAGE_TYPE damtype, xBase* who, const xVec3* vec_hit)
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
        if (vec_hit != NULL)
        {
            impart_velocity(*vec_hit * tweak.hit_vel);
        }

        stun();
        break;
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
    // Not in the correct order?
    S32 tempR;
    U32 cronyAttack;

    if (mode == 0)
    {
        if (flag.hunt == false)
        {
            cronyAttack = crony_attacking();
            tempR =
                ((0 - (cronyAttack & 0xff) | cronyAttack & 0xff) >> 0x1f) + NPC_GOAL_BPLANKTONHUNT;
            //NPC_GOAL_BPLANKTONATTACK
        }
        else
        {
            tempR = NPC_GOAL_BPLANKTONATTACK;
            //NPC_GOAL_BPLANKTONHUNT
        }
    }
    else
    {
        tempR = NPC_GOAL_BPLANKTONEVADE;
        //NPC_GOAL_BPLANKTONHUNT
    }
    return tempR;
}

void zNPCBPlankton::refresh_orbit()
{
    if (mode == MODE_BUDDY)
    {
        if (flag.hunt)
        {
            xVec3 oldcenter = orbit.center;
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
        xMovePointAsset& mp = *territory[active_territory].origin->asset;
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
    st_XORDEREDARRAY* npcs = zNPCMgr_GetNPCList();

    crony = NULL;

    for (S32 i = 0; i < npcs->cnt; ++i)
    {
        zNPCCommon* npc = (zNPCCommon*)npcs->list[i];

        if (npc->SelfType() == NPC_TYPE_BOSSBOBBY)
        {
            crony = (zNPCBoss*)npc;
            break;
        }
    }
}

namespace
{
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
                           bool xfree);
} // namespace

void zNPCBPlankton::update_move(F32 dt)
{
    switch (flag.move)
    {
    case MOVE_ACCEL:
        update_move_accel(frame->mat.pos, move, dt);
        break;
    case MOVE_STOP:
        update_move_stop(frame->mat.pos, move, dt);
        break;
    case MOVE_ORBIT:
        update_move_orbit(frame->mat.pos, move, orbit.center, dt, false);
        break;
    }
}

void zNPCBPlankton::reset_territories()
{
    territory_data* it = territory;
    territory_data* end = it + territory_size;

    for (; it != end; ++it)
    {
        it->fuse_detected = false;
        it->fuse_destroyed = false;
        it->fuse_detect_time = 0.0f;
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
        update_follow_player(dt);
        break;
    case FOLLOW_CAMERA:
        update_follow_camera(dt);
        break;
    }
}

void zNPCBPlankton::update_follow_player(F32 dt)
{
    follow.delay += dt;

    if (follow.delay >= follow.max_delay ||
        fabs(orbit_yaw_offset(move.dest, *get_player_loc())) > tweak.follow.max_ang)
    {
        move.dest = player_orbit();
        move.dest = random_orbit(move.dest, 0.0f, tweak.follow.min_ang);
        follow.delay = 0.0f;

        F32 min = tweak.follow.min_delay;
        F32 max = tweak.follow.max_delay;
        follow.max_delay = (max - min) * xurand() + min;
    }
}

void zNPCBPlankton::update_follow_camera(F32 dt)
{
    xVec3 target = orbit.center + globals.camera.mat.at * orbit.radius;

    follow.delay += dt;

    if (follow.delay >= follow.max_delay ||
        std::fabsf(orbit_yaw_offset(move.dest, target)) > tweak.follow.max_ang)
    {
        move.dest = random_orbit(target, 0.0f, tweak.follow.min_ang);
        follow.delay = 0.0f;

        F32 min = tweak.follow.min_delay;
        F32 max = tweak.follow.max_delay;
        follow.max_delay = (max - min) * xurand() + min;
    }
}

bool zNPCBPlankton::check_player_damage()
{
    if (globals.player.cheat_mode)
    {
        return false;
    }

    return false;
}

void zNPCBPlankton::load_territory(S32 index, xBase& child)
{
    territory_data& t = territory[index - 1];

    switch (child.baseType)
    {
    case eBaseTypeGroup:
    {
        for (U32 i = 0, size = xGroupGetCount((xGroup*)&child); i < size; ++i)
        {
            xBase* entry = xGroupGetItemPtr((xGroup*)&child, i);
            load_territory(index, *entry);
        }

        break;
    }
    case eBaseTypeMovePoint:
        t.origin = (zMovePoint*)&child;
        break;
    case eBaseTypeNPC:
        if (t.crony_size < 8)
        {
            t.crony[t.crony_size] = (zNPCCommon*)&child;
            ++t.crony_size;
        }
        break;
    case eBaseTypeTimer:
        t.timer = (xTimer*)&child;
        break;
    case eBaseTypeDestructObj:
        t.fuse = (zEntDestructObj*)&child;
        break;
    default:
        if (xEntValidType(child.baseType))
        {
            t.platform = (xEnt*)&child;
        }
        break;
    }
}

void zNPCBPlankton::init_beam()
{
    beam.init(31, "Plankton's Beam");
    beam.set_texture("plankton_laser_bolt");
    beam.cfg = tweak.beam.fx;
    beam.refresh_config();

    beam_ring.init(127, "Plankton's Beam Rings");
    beam_ring.set_curve(beam_ring_curve, 2);
    beam_ring.set_texture("bubble");
    beam_ring.set_default_config();
    beam_ring.cfg.flags = 0;
    beam_ring.cfg.life_time = 0.25f;
    beam_ring.cfg.blend_src = 5;
    beam_ring.cfg.blend_dst = 2;
    beam_ring.refresh_config();

    beam_glow.init(7, "Plankton's Beam Glow");
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

bool zNPCBPlankton::crony_attacking() const
{
    if (crony == NULL)
    {
        return false;
    }

    return crony->AttackTimeLeft() > 0.0f;
}

bool zNPCBPlankton::cronies_dead() const
{
    const territory_data& t = territory[active_territory];
    zNPCCommon* const* it = t.crony;
    zNPCCommon* const* end = it + t.crony_size;

    for (; it != end; ++it)
    {
        if ((*it)->IsAlive())
        {
            return false;
        }
    }

    return true;
}

void zNPCBPlankton::next_territory()
{
    if (have_cronies())
    {
        ++active_territory;

        if (active_territory >= territory_size)
        {
            active_territory = territory_size - 1;
        }
    }
}

bool zNPCBPlankton::have_cronies() const
{
    return territory[active_territory].crony_size > 0;
}

bool zNPCBPlankton::move_to_player_territory()
{
    xCollis& coll = globals.player.ent.collis->colls[0];

    if (!(coll.flags & 0x1))
    {
        return false;
    }

    xEnt* platform = (xEnt*)coll.optr;

    if (platform == NULL)
    {
        return false;
    }

    for (S32 i = 0; i < territory_size; ++i)
    {
        territory_data& t = territory[i];

        if (t.crony_size <= 0 && t.platform == platform)
        {
            active_territory = i;
            return true;
        }
    }

    return false;
}

bool zNPCBPlankton::player_left_territory() const
{
    const territory_data& t = territory[active_territory];
    xCollis& coll = globals.player.ent.collis->colls[0];
    xEnt* platform = (xEnt*)coll.optr;

    if (t.crony_size > 0 || t.platform == platform || !(coll.flags & 0x1) || platform == NULL)
    {
        return false;
    }

    for (S32 i = 0; i < territory_size; ++i)
    {
        const territory_data& t = territory[i];

        if (t.crony_size <= 0 && platform == t.platform && i != active_territory)
        {
            return true;
        }
    }

    return false;
}

void zNPCBPlankton::say(S32 which, S32 flags, bool random)
{
    if (newsfish == NULL)
    {
        return;
    }

    if (random)
    {
        newsfish->say(say_set[which].say[0], 1);
        newsfish->say(say_set[which].say[1], 2);
    }
    else
    {
        newsfish->say(say_set[which].say, say_set[which].size, flags, -1);
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

void zNPCBPlankton::aim_gun(xAnimPlay* play, xQuat* quat, xVec3* tran, int tranCount)
{
    zNPCBPlankton& owner = *(zNPCBPlankton*)play->Object;

    if (owner.flag.aim_gun)
    {
        quat[21] = owner.gun_tilt;
    }
}

xFactoryInst* zNPCGoalBPlanktonIdle::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonIdle(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonIdle::Enter(F32 dt, void* ctxt)
{
    F32 tmpFloat;
    F32 local_24[3];

    owner.reappear();
    owner.flag.attacking = false;
    owner.refresh_orbit();
    owner.reset_speed();
    owner.flag.follow = owner.FOLLOW_NONE;
    get_yaw(tmpFloat, dt);
    apply_yaw(tmpFloat);
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonIdle::Exit(F32 dt, void* ctxt)
{
    owner.refresh_orbit();
    return xGoal::Exit(dt, ctxt);
}

xFactoryInst* zNPCGoalBPlanktonAttack::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonAttack(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonAttack::Enter(F32 dt, void* ctxt)
{
    owner.reappear();
    owner.flag.attacking = true;
    owner.refresh_orbit();
    owner.follow_player();
    owner.delay = 0.0f;
    owner.face_player();
    owner.reset_speed();
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonAttack::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

xFactoryInst* zNPCGoalBPlanktonAmbush::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonAmbush(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonAmbush::Enter(F32 dt, void* ctxt)
{
    return zNPCGoalCommon::Enter(dt, ctxt);
}

S32 zNPCGoalBPlanktonAmbush::Exit(F32 dt, void* ctxt)
{
    return xGoal::Exit(dt, ctxt);
}

xFactoryInst* zNPCGoalBPlanktonFlank::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonFlank(who, (zNPCBPlankton&)*info);
}

xFactoryInst* zNPCGoalBPlanktonEvade::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonEvade(who, (zNPCBPlankton&)*info);
}

xFactoryInst* zNPCGoalBPlanktonHunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonHunt(who, (zNPCBPlankton&)*info);
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

xFactoryInst* zNPCGoalBPlanktonTaunt::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonTaunt(who, (zNPCBPlankton&)*info);
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
    return new (who, grow) zNPCGoalBPlanktonMove(who, (zNPCBPlankton&)*info);
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
    return new (who, grow) zNPCGoalBPlanktonStun(who, (zNPCBPlankton&)*info);
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
    owner.flag.follow = owner.FOLLOW_PLAYER;
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBPlanktonFall::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonFall(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonFall::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBPlanktonDizzy::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonDizzy(who, (zNPCBPlankton&)*info);
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
    return xGoal::Exit(dt, updCtxt);
}

xFactoryInst* zNPCGoalBPlanktonBeam::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonBeam(who, (zNPCBPlankton&)*info);
}

S32 zNPCGoalBPlanktonBeam::Enter(F32 dt, void* updCtxt)
{
    owner.reappear();
    substate = SS_WARM_UP;
    owner.delay = 0.0f;
    emitted = 0;
    owner.flag.aim_gun = true;
    owner.flag.follow = owner.FOLLOW_NONE;
    owner.enable_emitter((xParEmitter&)owner.beam_charge);
    play_sound(5, (xVec3*)&owner.bound.pad[3], 1.0f); // dunno how to get this to call properly
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBPlanktonBeam::Exit(F32 dt, void* updCtxt)
{
    owner.flag.aim_gun = false;
    owner.flag.follow = owner.FOLLOW_PLAYER;
    owner.disable_emitter((xParEmitter&)owner.beam_charge);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBPlanktonBeam::Process(en_trantype* trantype, F32 dt, void* unk,
                                   xScene* xscn) // void* should be someting else.
// cross reference other files for the answer

// Im probably just dumb, but i dont get how this should be written.
{
    S32 tempProcess;
    tempProcess = emitted;
    if (tempProcess != 2)
    {
        zNPCGoalBPlanktonBeam::update_warm_up(dt);
    }
    else if (tempProcess < 2)
    {
        if (tempProcess == 0)
        {
            zNPCGoalBPlanktonBeam::update_fire(dt);
        }
        else
        {
            zNPCGoalBPlanktonBeam::update_cool_down(dt);
        }
    }
    else if (tempProcess < 4)
    {
        unk = 0;
        tempProcess = owner.next_goal();
        return tempProcess;
    }

    return 0;
}

xFactoryInst* zNPCGoalBPlanktonWall::create(S32 who, RyzMemGrow* grow, void* info)
{
    return new (who, grow) zNPCGoalBPlanktonWall(who, (zNPCBPlankton&)*info);
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
    return new (who, grow) zNPCGoalBPlanktonMissle(who, (zNPCBPlankton&)*info);
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
    return new (who, grow) zNPCGoalBPlanktonBomb(who, (zNPCBPlankton&)*info);
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

xVec3& zNPCBPlankton::location() const
{
    return reinterpret_cast<xVec3&>(this->model->Mat->pos);
}

void zNPCBPlankton::render_debug()
{
}

void zNPCBPlankton::enable_emitter(xParEmitter& p1) const
{
    p1.emit_flags |= 1;
}

void zNPCBPlankton::disable_emitter(xParEmitter& p1) const
{
    p1.emit_flags &= 0xFE;
}

U8 zNPCBPlankton::ColChkFlags() const
{
    return 0;
}

U8 zNPCBPlankton::ColPenFlags() const
{
    return 0;
}

U8 zNPCBPlankton::ColChkByFlags() const
{
    return 16;
}

U8 zNPCBPlankton::ColPenByFlags() const
{
    return 16;
}

U8 zNPCBPlankton::PhysicsFlags() const
{
    return 3;
}

S32 zNPCBPlankton::IsAlive()
{
    return 1;
}
