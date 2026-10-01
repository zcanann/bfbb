#ifndef ZNPCTYPEBOSSPLANKTON_H
#define ZNPCTYPEBOSSPLANKTON_H

#include "zNPCTypeBoss.h"
#include "zNPCTypeVillager.h"
#include "zNPCGoalCommon.h"
#include "zEntDestructObj.h"

#include "xDecal.h"
#include "xLaserBolt.h"
#include "xTimer.h"
#include "zNPCGoals.h"
#include "xParEmitter.h"
#include "xLaserBolt.h"

namespace auto_tweak
{
    template <class T1, class T2>
    void load_param(T1&, T2, T2, T2, xModelAssetParam*, U32, const char*);
};

struct zNPCBPlankton : zNPCBoss
{
    enum move_enum
    {
        MOVE_NONE,
        MOVE_ACCEL,
        MOVE_STOP,
        MOVE_ORBIT
    };

    enum follow_enum
    {
        FOLLOW_NONE,
        FOLLOW_PLAYER,
        FOLLOW_CAMERA
    };

    enum mode_enum
    {
        MODE_BUDDY,
        MODE_HARASS
    };

    struct move_info
    {
        xVec3 dest;
        xVec3 vel;
        xVec3 accel;
        xVec3 max_vel;
    };

    struct territory_data
    {
        zMovePoint* origin;
        xEnt* platform;
        zEntDestructObj* fuse;
        xTimer* timer;
        zNPCCommon* crony[8];
        S32 crony_size;
        U8 fuse_detected;
        U8 fuse_destroyed;
        F32 fuse_detect_time;
    };

    struct
    {
        bool updated; //0x2b4
        bool face_player; //0x2b5
        bool attacking; //0x2b6
        bool hunt; //0x2b7
        bool aim_gun;
        move_enum move;
        follow_enum follow;
    } flag;
    mode_enum mode;
    F32 delay; //0x2c8
    xQuat gun_tilt;
    F32 ambush_delay;
    F32 beam_duration; // 0x2e0
    F32 stun_duration; // 0x2e4
    xDecalEmitter beam_ring;
    xDecalEmitter beam_glow;
    xLaserBoltEmitter beam;
    xParEmitter* beam_charge;
    struct
    {
        xVec3 center;
        F32 radius;
    } orbit;
    struct
    {
        xVec2 dir;
        F32 vel;
        F32 accel;
        F32 max_vel;
    } turn;
    move_info move;
    struct
    {
        F32 delay;
        F32 max_delay;
    } follow;
    struct
    {
        U8 moreFlags; //0x4ac
    } old;
    zNPCBoss* crony;
    territory_data territory[8];
    S32 territory_size; //0x694
    S32 active_territory;
    zNPCNewsFish* newsfish;
    U32 old_player_health; //0x6a0
    U8 played_intro; //0x6a4

    zNPCBPlankton(S32 myType);
    void Init(xEntAsset*);
    void Setup();
    void PostSetup();
    void Reset();
    void Destroy();
    void Process(xScene*, F32);
    S32 SysEvent(xBase*, xBase*, unsigned int, const F32*, xBase*, int*);
    void Render();
    void RenderExtraPostParticles();
    void ParseINI();
    void ParseLinks();
    void SelfSetup();
    void Damage(en_NPC_DAMAGE_TYPE, xBase*, const xVec3*);
    U32 AnimPick(int, en_NPC_GOAL_SPOT, xGoal*);
    S32 next_goal();
    void scan_cronies();
    void update_turn(F32);
    void update_move(F32);
    bool check_player_damage();
    void reset_territories();
    void update_animation(F32);
    void update_follow(F32);
    void update_follow_player(F32);
    void update_follow_camera(F32);
    void update_aim_gun(F32);
    void update_dialog(F32);
    void init_beam();
    void setup_beam();
    void reset_beam();
    void vanish();
    void reappear();
    U8 crony_attacking() const;
    void stun();
    U8 cronies_dead() const;
    void impart_velocity(const xVec3&);
    void next_territory();
    U8 have_cronies() const;
    U8 move_to_player_territory();
    U8 player_left_territory() const;
    void say(int, int, bool);
    void sickum();
    static void aim_gun(xAnimPlay*, xQuat*, xVec3*, int);
    void here_boy();
    void follow_player();
    void follow_camera();
    void reset_speed();
    void halt(F32);
    void fall(F32, F32);
    void refresh_orbit();
    F32 orbit_yaw_offset(const xVec3&, const xVec3&) const;
    xVec3 random_orbit(const xVec3&, F32, F32) const;
    xVec3 player_orbit() const;
    void load_territory(S32, xBase&);

    xVec3& location() const
    {
        return reinterpret_cast<xVec3&>(this->model->Mat->pos);
    }
    void face_player()
    {
        flag.face_player = true;
    }
    void render_debug()
    {
    }
    bool turning() const
    {
        const xVec2 at = { model->Mat->at.x, model->Mat->at.z };

        return !xfeq0(turn.vel) ||
               (!xfeq0(turn.accel) &&
                !(turn.dir.x > turn.dir.y && xabs(turn.dir.x - at.x) < 0.001f) &&
                !(turn.dir.x < turn.dir.y && xabs(turn.dir.y - at.y) < 0.001f));
    }
    void take_control()
    {
        if (crony != NULL)
        {
            crony->HoldUpDude();
        }
    }
    F32 get_orbit_yaw(const xVec3& loc) const
    {
        return xatan2(loc.x - orbit.center.x, loc.z - orbit.center.z);
    }
    void set_location(const xVec3& loc)
    {
        reinterpret_cast<xVec3&>(model->Mat->pos) = frame->mat.pos = loc;
    }
    void give_control()
    {
        if (crony != NULL)
        {
            crony->ThanksImDone();
        }
    }
    void enable_emitter(xParEmitter& p1) const
    {
        p1.emit_flags |= 1;
    }
    void disable_emitter(xParEmitter& p1) const
    {
        p1.emit_flags &= 0xFE;
    }
    U8 ColChkFlags() const
    {
        return 0;
    }
    U8 ColPenFlags() const
    {
        return 0;
    }
    U8 ColChkByFlags() const
    {
        return 16;
    }
    U8 ColPenByFlags() const
    {
        return 16;
    }
    U8 PhysicsFlags() const
    {
        return 3;
    }
    S32 IsAlive()
    {
        return 1;
    }
};

struct zNPCGoalBPlanktonIdle : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonIdle(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);

    void get_yaw(F32&, F32&) const;
    void apply_yaw(F32);
};

struct zNPCGoalBPlanktonAttack : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonAttack(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonAmbush : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonAmbush(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonFlank : zNPCGoalCommon
{
    F32 accel;
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonFlank(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonEvade : zNPCGoalCommon
{
    F32 evade_delay;
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonEvade(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonHunt : zNPCGoalCommon
{
    xVec3 player_loc;
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonHunt(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonTaunt : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonTaunt(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonMove : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonMove(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonStun : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonStun(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonFall : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonFall(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32 dt, void* updCtxt);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonDizzy : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonDizzy(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonBeam : zNPCGoalCommon
{
    enum substate_enum
    {
        SS_WARM_UP,
        SS_FIRE,
        SS_COOL_DOWN,
        SS_DONE
    };

    F32 emitted;
    substate_enum substate;
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonBeam(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
    void update_cool_down(F32);
    void update_warm_up(F32);
    void update_fire(F32);
};

struct zNPCGoalBPlanktonWall : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonWall(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonMissle : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonMissle(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

struct zNPCGoalBPlanktonBomb : zNPCGoalCommon
{
    zNPCBPlankton& owner;

    zNPCGoalBPlanktonBomb(S32 goalID, zNPCBPlankton& npc) : zNPCGoalCommon(goalID), owner(npc)
    {
    }

    static xFactoryInst* create(S32 who, RyzMemGrow* grow, void* info);
    S32 Enter(F32, void*);
    S32 Exit(F32, void*);
    S32 Process(en_trantype*, F32, void*, xScene*);
};

xAnimTable* ZNPC_AnimTable_BossPlankton();

#endif
