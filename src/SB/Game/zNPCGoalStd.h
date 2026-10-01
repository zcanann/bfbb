#ifndef ZNPCGOALSTD_H
#define ZNPCGOALSTD_H

#include "zNPCGoalCommon.h"
#include "zNPCHazard.h"

struct zNPCGoalLoopAnim : zNPCGoalCommon
{
    S32 flg_loopanim;
    U32 anid_stage[3];
    S32 cnt_loop;
    F32 lastAnimTime;
    U32 origAnimFlags;
    U32 animWeMolested;

    zNPCGoalLoopAnim(S32 goalID) : zNPCGoalCommon(goalID)
    {
        SetFlags(1 << 1);
    }

    void TriggerExit()
    {
        cnt_loop = 0;
    }

    void MolestLoopAnim();
    void UnmolestAnim();
    void LoopCountSet(S32 num);
    void UseDefaultAnims();
    void ValidateStages();

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
};

struct zNPCGoalIdle : zNPCGoalCommon
{
    S32 flg_idle;

    zNPCGoalIdle(S32 goalID) : zNPCGoalCommon(goalID)
    {
        SetFlags((1 << 3) | (1 << 2));
        flg_npcgauto &= ~((1 << 2) | (1 << 1));
    }

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
    virtual S32 Suspend(F32 dt, void* updCtxt);
    virtual S32 Resume(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
    virtual S32 NPCMessage(NPCMsg* mail);
};

struct zNPCGoalWaiting : zNPCGoalLoopAnim
{
    S32 flg_waiting;
    F32 tmr_waiting;

    zNPCGoalWaiting(S32 goalID) : zNPCGoalLoopAnim(goalID)
    {
    }

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
    virtual S32 Resume(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
};

struct zNPCGoalWander : zNPCGoalCommon
{
    S32 flg_wand;
    F32 tmr_remain;
    F32 rad_wand;
    xVec3 pos_home;
    F32 tmr_minwalk;
    F32 tmr_newdir;
    xVec3 dir_cur;

    zNPCGoalWander(S32 goalID) : zNPCGoalCommon(goalID)
    {
        SetFlags((1 << 2) | (1 << 1));
        flg_wand = 0xFFFF0000;
    }

    void VerticalWander(F32 spd_dt, const xVec3* vec_dest);
    void CalcNewDir();

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Resume(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
};

struct zNPCGoalPatrol : zNPCGoalCommon
{
    S32 flg_patrol;
    F32 tmr_wait;
    xVec3 pos_midpnt[4];
    S32 idx_midpnt;

    zNPCGoalPatrol(S32 goalID) : zNPCGoalCommon(goalID)
    {
        SetFlags((1 << 2) | (1 << 1));
    }

    void DoOnArriveStuff();
    void PickTransition(S32* goal, en_trantype* trantype);
    void MoveNormal(F32 dt);
    void MoveSpline(F32 dt);
    void Chk_AutoSmooth();
    void MoveAutoSmooth(F32 dt);

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
    virtual S32 Resume(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
};

struct zNPCGoalPushAnim : zNPCGoalCommon
{
    S32 flg_pushanim;
    F32 lastAnimTime;

    zNPCGoalPushAnim(S32 goalID) : zNPCGoalCommon(goalID)
    {
        SetFlags((1 << 2) | (1 << 1));
    }

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
    virtual S32 Resume(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
};

struct zNPCGoalFidget : zNPCGoalPushAnim
{
    zNPCGoalFidget(S32 goalID) : zNPCGoalPushAnim(goalID)
    {
    }

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
};

struct zNPCGoalNoManLand : zNPCGoalCommon
{
    zNPCGoalNoManLand(S32 goalID) : zNPCGoalCommon(goalID)
    {
    }
};

struct zNPCGoalDead : zNPCGoalCommon
{
    S32 flg_deadinfo;
    U8 old_moreFlags;

    zNPCGoalDead(S32 goalID) : zNPCGoalCommon(goalID)
    {
        SetFlags((1 << 2) | (1 << 1));
    }

    void DieQuietly()
    {
        flg_deadinfo |= (1 << 0);
        flg_deadinfo &= ~(1 << 1);
    }
    void DieWithAWhimper();
    void DieWithABang();

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);

protected:
    ~zNPCGoalDead();
};

struct zNPCGoalLimbo : zNPCGoalDead
{
    zNPCGoalLimbo(S32 goalID) : zNPCGoalDead(goalID)
    {
    }

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 NPCMessage(NPCMsg* mail);
};

struct zNPCGoalDEVAnimCycle : zNPCGoalCommon
{
    zNPCGoalDEVAnimCycle(S32 goalID) : zNPCGoalCommon(goalID)
    {
        flg_npcgauto &= ~((1 << 2) | (1 << 1));
    }

    xAnimState* ASTGetNext(xAnimState* ast);

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
    virtual S32 NPCMessage(NPCMsg* mail);
};

struct zNPCGoalDEVAnimSpin : zNPCGoalCommon
{
    U32 origAnimFlags;
    U32 animWeMolested;

    zNPCGoalDEVAnimSpin(S32 goalID) : zNPCGoalCommon(goalID)
    {
        flg_npcgauto &= ~((1 << 2) | (1 << 1));
    }

    void ASTMolestAnim(xAnimState* state);
    void ASTUnmolestAnim();

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
    virtual S32 NPCMessage(NPCMsg* mail);
};

struct zNPCGoalDEVHero : zNPCGoalCommon
{
    zNPCGoalDEVHero(S32 goalID) : zNPCGoalCommon(goalID)
    {
        flg_npcgauto &= ~((1 << 2) | (1 << 1));
    }

    virtual S32 Enter(F32 dt, void* updCtxt);
    virtual S32 Exit(F32 dt, void* updCtxt);
    virtual S32 Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene);
    virtual S32 NPCMessage(NPCMsg* mail);
};

xFactoryInst* GOALCreate_Standard(S32 who, RyzMemGrow* grow, void*);
void GOALDestroy_Goal(xFactoryInst* inst);

inline void zNPCGoalDead::DieWithAWhimper()
{
    flg_deadinfo &= ~1;
    flg_deadinfo |= 2;
}

inline void zNPCGoalDead::DieWithABang()
{
    flg_deadinfo &= ~1;
    flg_deadinfo &= ~2;
}

#endif
