#include "iCollide.h"
#include "xColor.h"
#include "xCounter.h"
#include "xDraw.h"
#include "xEnt.h"
#include "xEntBoulder.h"
#include "xFX.h"
#include "xModel.h"
#include "xScrFx.h"
#include "xVec3.h"
#include "xMath.h"
#include "xMath3.h"
#include "xMathInlines.h"
#include "zNPCGoalRobo.h"
#include "zNPCGoals.h"
#include "zNPCGoalStd.h"
#include "zNPCHazard.h"
#include "zNPCSndLists.h"
#include "zNPCSupport.h"
#include "zNPCTypeCommon.h"
#include "zNPCTypeRobot.h"
#include "zEntTeleportBox.h"
#include "zGlobals.h"
#include "zGoo.h"
#include "zSurface.h"
#include "zNPCGoalCommon.h"
#include "zGameExtras.h"
#include "zNPCSupplement.h"
#include "zParPTank.h"
#include <stdlib.h>

enum en_copcntr
{
    ROBOCOP_CNTR_FODDER = 0,
    ROBOCOP_CNTR_HAMMER = 1,
    ROBOCOP_CNTR_TARTAR = 2,
    ROBOCOP_CNTR_GLOVE = 3,
    ROBOCOP_CNTR_MONSOON = 4,
    ROBOCOP_CNTR_SLEEPY = 5,
    ROBOCOP_CNTR_CHUCK = 6,
    ROBOCOP_CNTR_TUBELET = 7,
    ROBOCOP_CNTR_SLICK = 8,
    ROBOCOP_CNTR_FODBOMB = 9,
    ROBOCOP_CNTR_FODBZZT = 10,
    ROBOCOP_CNTR_CHOMPER = 11,
    ROBOCOP_CNTR_ARFARF = 12,
    ROBOCOP_CNTR_ARFDOG = 13,
    ROBOCOP_CNTR_PLANKTON = 14,
    ROBOCOP_CNTR_NOMORE = 15,
    ROBOCOP_CNTR_FORCE = 2147483647,
};

struct RoboCopMap
{
    S32 ntyp_robotype;
    en_copcntr idx_copCounter;
};

void NPCC_DrawPlayerPredict(S32, F32, F32);
S32 NPCC_LineHitsBound(xVec3* a, xVec3* b, xBound* bnd, xCollis* callers_colrec);
S32 NPCC_chk_hitEnt(xEnt* tgt, xBound* bnd, xCollis* collide);
void zEntPlayer_LassoNotify(en_LASSO_EVENT event);
void NPCC_rotHPB(xMat3x3* mat, F32 heading, F32 pitch, F32 bank);
void NPAR_EmitTubeSpiral(const xVec3* pos, const xVec3* vel, F32 lifespan);
xVec3* NPCC_upDir(xEnt* ent);
S32 zSurfaceGetDamageType(const xSurface* surf);
F32 NPCC_DstSqPlyrToPos(const xVec3* pos);
void NPCC_GenSmooth(xVec3** pos_base, xVec3** pos_mid);
extern S32 g_needMusician;
void zFX_SpawnBubbleTrailNoNegRandVel(const xVec3* pos, U32 num, const xVec3* pos_rnd,
                                      const xVec3* vel_rnd);

static xCollis g_SharedCollisRecordList[6] = { { k_HIT_0xF00 | k_HIT_CALC_HDNG } };
static xCollis g_SharedCollisRecord = { k_HIT_0xF00 | k_HIT_CALC_HDNG };

// These structs were used in deadstripped functions.
// This function is here to force the symbols to be linked.
void __deadstripped_zNPCGoalRobo()
{
    const char _437[0x0C] = {};
    const char _438[0x0C] = {};
    const char _442[0x0C] = {};
    const char _473[0x0C] = {};

    const char _626[0x28] = {};
    const char _627[0x28] = {};
    const char _628[0x28] = {};
    const char _629[0x28] = {};
    const char _630[0x28] = {};
    const char _631[0x28] = {};
    const char _632[0x28] = {};

    const char _1200[0x0C] = {};
}

// .text (12360)

xFactoryInst* GOALCreate_Robotic(S32 who, RyzMemGrow* grow, void*)
{
    xGoal* goal = NULL; // r16
    switch (who)
    {
    case NPC_GOAL_NOTICE:
        goal = new (who, grow) zNPCGoalNotice(who);
        break;
    case NPC_GOAL_TAUNT:
        goal = new (who, grow) zNPCGoalTaunt(who);
        break;
    case NPC_GOAL_EVADE:
        goal = new (who, grow) zNPCGoalEvade(who);
        break;
    case NPC_GOAL_GOHOME:
        goal = new (who, grow) zNPCGoalGoHome(who);
        break;
    case NPC_GOAL_CHASE:
        goal = new (who, grow) zNPCGoalChase(who);
        break;
    case NPC_GOAL_ALERT:
        goal = new (who, grow) zNPCGoalAlert(who);
        break;
    case NPC_GOAL_ALERTFODDER:
        goal = new (who, grow) zNPCGoalAlertFodder(who);
        break;
    case NPC_GOAL_ALERTFODBOMB:
        goal = new (who, grow) zNPCGoalAlertFodBomb(who);
        break;
    case NPC_GOAL_ALERTCHOMPER:
        goal = new (who, grow) zNPCGoalAlertChomper(who);
        break;
    case NPC_GOAL_ALERTFODBZZT:
        goal = new (who, grow) zNPCGoalAlertFodBzzt(who);
        break;
    case NPC_GOAL_ALERTHAMMER:
        goal = new (who, grow) zNPCGoalAlertHammer(who);
        break;
    case NPC_GOAL_ALERTTARTAR:
        goal = new (who, grow) zNPCGoalAlertTarTar(who);
        break;
    case NPC_GOAL_ALERTGLOVE:
        goal = new (who, grow) zNPCGoalAlertGlove(who);
        break;
    case NPC_GOAL_ALERTMONSOON:
        goal = new (who, grow) zNPCGoalAlertMonsoon(who);
        break;
    case NPC_GOAL_ALERTSLEEPY:
        goal = new (who, grow) zNPCGoalAlertSleepy(who);
        break;
    case NPC_GOAL_ALERTARF:
        goal = new (who, grow) zNPCGoalAlertArf(who);
        break;
    case NPC_GOAL_ALERTPUPPY:
        goal = new (who, grow) zNPCGoalAlertPuppy(who);
        break;
    case NPC_GOAL_ALERTCHUCK:
        goal = new (who, grow) zNPCGoalAlertChuck(who);
        break;
    case NPC_GOAL_ALERTTUBELET:
        goal = new (who, grow) zNPCGoalAlertTubelet(who);
        break;
    case NPC_GOAL_ALERTSLICK:
        goal = new (who, grow) zNPCGoalAlertSlick(who);
        break;
    case NPC_GOAL_ATTACKCQC:
        goal = new (who, grow) zNPCGoalAttackCQC(who);
        break;
    case NPC_GOAL_ATTACKFODDER:
        goal = new (who, grow) zNPCGoalAttackFodder(who);
        break;
    case NPC_GOAL_ATTACKCHOMPER:
        goal = new (who, grow) zNPCGoalAttackChomper(who);
        break;
    case NPC_GOAL_ATTACKHAMMER:
        goal = new (who, grow) zNPCGoalAttackHammer(who);
        break;
    case NPC_GOAL_ATTACKTARTAR:
        goal = new (who, grow) zNPCGoalAttackTarTar(who);
        break;
    case NPC_GOAL_ATTACKMONSOON:
        goal = new (who, grow) zNPCGoalAttackMonsoon(who);
        break;
    case NPC_GOAL_ATTACKARF:
        goal = new (who, grow) zNPCGoalAttackArf(who);
        break;
    case NPC_GOAL_ATTACKARFMELEE:
        goal = new (who, grow) zNPCGoalAttackArfMelee(who);
        break;
    case NPC_GOAL_ATTACKCHUCK:
        goal = new (who, grow) zNPCGoalAttackChuck(who);
        break;
    case NPC_GOAL_ATTACKSLICK:
        goal = new (who, grow) zNPCGoalAttackSlick(who);
        break;
    case NPC_GOAL_LASSOBASE:
        goal = new (who, grow) zNPCGoalLassoBase(who);
        break;
    case NPC_GOAL_LASSOGRAB:
        goal = new (who, grow) zNPCGoalLassoGrab(who);
        break;
    case NPC_GOAL_LASSOTHROW:
        goal = new (who, grow) zNPCGoalLassoThrow(who);
        break;
    case NPC_GOAL_EVILPAT:
        goal = new (who, grow) zNPCGoalEvilPat(who);
        break;
    case NPC_GOAL_STUNNED:
        goal = new (who, grow) zNPCGoalStunned(who);
        break;
    case NPC_GOAL_PATCARRY:
        goal = new (who, grow) zNPCGoalPatCarry(who);
        break;
    case NPC_GOAL_PATTHROW:
        goal = new (who, grow) zNPCGoalPatThrow(who);
        break;
    case NPC_GOAL_DOGLAUNCH:
        goal = new (who, grow) zNPCGoalDogLaunch(who);
        break;
    case NPC_GOAL_DOGBARK:
        goal = new (who, grow) zNPCGoalDogBark(who);
        break;
    case NPC_GOAL_DOGDASH:
        goal = new (who, grow) zNPCGoalDogDash(who);
        break;
    case NPC_GOAL_DOGPOUNCE:
        goal = new (who, grow) zNPCGoalDogPounce(who);
        break;
    case NPC_GOAL_TELEPORT:
        goal = new (who, grow) zNPCGoalTeleport(who);
        break;
    case NPC_GOAL_HOKEYPOKEY:
        goal = new (who, grow) zNPCGoalHokeyPokey(who);
        break;
    case NPC_GOAL_DAMAGE:
        goal = new (who, grow) zNPCGoalDamage(who);
        break;
    case NPC_GOAL_WOUND:
        goal = new (who, grow) zNPCGoalWound(who);
        break;
    case NPC_GOAL_BASHED:
        goal = new (who, grow) zNPCGoalBashed(who);
        break;
    case NPC_GOAL_KNOCK:
        goal = new (who, grow) zNPCGoalKnock(who);
        break;
    case NPC_GOAL_AFTERLIFE:
        goal = new (who, grow) zNPCGoalAfterlife(who);
        break;
    case NPC_GOAL_RESPAWN:
        goal = new (who, grow) zNPCGoalRespawn(who);
        break;
    case NPC_GOAL_DEFLATE:
        goal = new (who, grow) zNPCGoalDeflate(who);
        break;
    case NPC_GOAL_TUBEPAL:
        goal = new (who, grow) zNPCGoalTubePal(who);
        break;
    case NPC_GOAL_TUBEDUCKLING:
        goal = new (who, grow) zNPCGoalTubeDuckling(who);
        break;
    case NPC_GOAL_TUBEATTACK:
        goal = new (who, grow) zNPCGoalTubeAttack(who);
        break;
    case NPC_GOAL_TUBELASSO:
        goal = new (who, grow) zNPCGoalTubeLasso(who);
        break;
    case NPC_GOAL_TUBEBIRTH:
        goal = new (who, grow) zNPCGoalTubeBirth(who);
        break;
    case NPC_GOAL_TUBEBONKED:
        goal = new (who, grow) zNPCGoalTubeBonked(who);
        break;
    case NPC_GOAL_TUBEDYING:
        goal = new (who, grow) zNPCGoalTubeDying(who);
        break;
    case NPC_GOAL_TUBEDEAD:
        goal = new (who, grow) zNPCGoalTubeDead(who);
        break;
    }

    return goal;
}

S32 zNPCGoalNotice::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 dir_dest;

    xVec3Sub(&dir_dest, xEntGetPos(&globals.player.ent), npc->Pos());
    dir_dest.y = 0.0f;

    if (xVec3Length2(&dir_dest) > 1.0f)
    {
        xVec3Normalize(&dir_dest, &dir_dest);
        npc->TurnToFace(dt, &dir_dest, 12.566371f);
    }

    npc->VelStop();
    return zNPCGoalPushAnim::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalTaunt::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->SndPlayRandom(NPC_STYP_LAUGH);
    return zNPCGoalLoopAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalTaunt::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 dir_dest;
    F32 timeToLaugh;

    npc->VelStop();

    F32 half_dur = (0.5f * npc->AnimDuration(NULL));

    timeToLaugh = MAX(half_dur, 0.5f);

    if (globals.player.DamageTimer > timeToLaugh)
    {
        this->cnt_loop = MAX(this->cnt_loop, 1);
    }

    xVec3Sub(&dir_dest, xEntGetPos(&globals.player.ent), npc->Pos());
    dir_dest.y = 0.0f;

    if (xVec3Length2(&dir_dest) > 1.0f)
    {
        xVec3Normalize(&dir_dest, &dir_dest);
        npc->TurnToFace(dt, &dir_dest, -1.0f);
    }

    return zNPCGoalLoopAnim::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalEvade::Enter(F32 dt, void* updCtxt)
{
    flg_evade = 0;

    if (xrand() & 0x00800000)
    {
        flg_evade |= 1;
    }
    else
    {
        flg_evade |= 2;
    }

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalEvade::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)(this->psyche->clt_owner);
    S32 nextgoal = 0;

    if (this->psyche->TimerGet(XPSY_TYMR_CURGOAL) > 5.0f)
    {
        *trantype = GOAL_TRAN_POP;
        nextgoal = 1;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        xVec3 dir_dest;
        xVec3Sub(&dir_dest, xEntGetPos(&globals.player.ent), npc->Pos());
        dir_dest.y = 0.0f;
        if (xVec3Length2(&dir_dest) > 1.0f)
        {
            xVec3Normalize(&dir_dest, &dir_dest);
            npc->TurnToFace(dt, &dir_dest, -1.0);
        }
        npc->ThrottleAdjust(dt, 0.0f, -1.0f);
        npc->ThrottleApply(dt, NPCC_faceDir(npc), 0);
        nextgoal = xGoal::Process(trantype, dt, updCtxt, NULL);
    }

    return nextgoal;
}

S32 zNPCGoalGoHome::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    U32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 dir_dest;
    xVec3Sub(&dir_dest, npc->Pos(), npc->arena.Pos());
    if (npc->flg_move & 2)
    {
        dir_dest.y = 0.0f;
    }
    F32 a = xVec3Length2(&dir_dest);
    F32 b = npc->arena.Radius(1.0f) * 0.3f;
    if (a < b)
    {
        npc->ThrottleAdjust(dt, 0.0f, -1.0f);
        if (npc->spd_throttle < 0.5f)
        {
            nextgoal = 1;
            *trantype = GOAL_TRAN_POP;
        }
        return nextgoal;
    }
    else
    {
        xVec3Normalize(&dir_dest, &dir_dest);
        npc->TurnToFace(dt, &dir_dest, -1.0f);
        npc->ThrottleAdjust(dt, 5.0f, -1.0f);
        npc->ThrottleApply(dt, &dir_dest, 0);
        if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, 2))
        {
            if ((S32)(psyche->TimerGet(XPSY_TYMR_CURGOAL) * 5.0f) & 1)
            {
                xDrawSetColor(g_RED);
                xDrawLine(xEntGetCenter(npc), npc->arena.Pos());
            }
        }
        return xGoal::Process(trantype, dt, updCtxt, NULL);
    }
    return nextgoal;
}

NPCBattle g_GlobalBattleData;

S32 zNPCGoalAlert::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = ((zNPCRobot*)(this->psyche->clt_owner));
    npc->SelfType();
    if (npc->npcset.allowDetect && npc->arena.IsReady())
    {
        if (npc->arena.IncludesPlayer(0.0f, NULL))
        {
            npc->ISeePlayer();
        }
    }
    npc->SndPlayRandom(NPC_STYP_ALERT);
    npc->inf_battle = &g_GlobalBattleData;
    npc->inf_battle->JoinBattle(npc);
    this->flg_user = 1;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlert::Exit(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = ((zNPCRobot*)(this->psyche->clt_owner));
    if (npc->inf_battle != NULL)
    {
        npc->inf_battle->LeaveBattle(npc);
    }
    npc->inf_battle = NULL;
    this->flg_user = 1;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalAlert::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 dir_dest;
    if (globals.player.Health < 1)
    {
        zNPCGoalLoopAnim* taunt = (zNPCGoalLoopAnim*)psyche->FindGoal(NPC_GOAL_TAUNT);
        if (taunt != NULL)
        {
            taunt->LoopCountSet(1000);
            *trantype = GOAL_TRAN_PUSH;
            nextgoal = NPC_GOAL_TAUNT;
        }
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        zNPCGoalLoopAnim* taunt = (zNPCGoalLoopAnim*)psyche->FindGoal(NPC_GOAL_TAUNT);
        if (taunt != NULL)
        {
            taunt->LoopCountSet(1);
            *trantype = GOAL_TRAN_PUSH;
            nextgoal = NPC_GOAL_TAUNT;
        }
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->arena.IsReady())
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    if (flg_user != 0)
    {
        DoAutoAnim(NPC_GSPOT_START, 0);
        flg_user = 0;
    }
    xVec3Sub(&dir_dest, xEntGetPos(&globals.player.ent), npc->Pos());
    dir_dest.y = 0.0f;
    if (xVec3Length2(&dir_dest) > 1.0f)
    {
        xVec3Normalize(&dir_dest, &dir_dest);
        npc->TurnToFace(dt, &dir_dest, -1.0f);
    }
    npc->ThrottleAdjust(dt, 0.0f, -1.0f);
    npc->ThrottleApply(dt, NPCC_faceDir(npc), 0);
    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalAlertFodder::Enter(F32 dt, void* updCtxt)
{
    this->flg_attack = 0;
    this->tmr_alertfod = 0.0f;
    this->alertfod = FODDER_ALERT_NOTICE;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertFodder::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    S32 nextgoal = 0;

    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)psyche->FindGoal(NPC_GOAL_TAUNT);
        taunt->LoopCountSet(1000);
        nextgoal = NPC_GOAL_TAUNT;
        *trantype = GOAL_TRAN_PUSH;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        nextgoal = NPC_GOAL_TAUNT;
        *trantype = GOAL_TRAN_PUSH;
    }
    else if (npc->SomethingWonderful())
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->arena.IncludesPlayer(0.0f, NULL)) // I don't understand this check
    {
        nextgoal = NPC_GOAL_IDLE;
        *trantype = GOAL_TRAN_SET;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    en_alertfod old_alertfod = this->alertfod;

    S32 flag2 = this->flg_info & 2;
    this->flg_info = this->flg_info & ~0x6;

    F32 pct = npc->arena.PctFromHome(npc->Pos());

    if (pct > 1.0f)
    {
        flag2 = 1;
        this->alertfod = FODDER_ALERT_ARENA;
    }
    else if (this->alertfod == FODDER_ALERT_ARENA && pct < 0.5f)
    {
        this->alertfod = FODDER_ALERT_BEGIN;
        flag2 = 0x1;
    }

    switch (this->alertfod)
    {
    case FODDER_ALERT_NOTICE: // 0x194
    {
        this->alertfod = FODDER_ALERT_BEGIN;
        nextgoal = NPC_GOAL_NOTICE;
        *trantype = GOAL_TRAN_PUSH;
        break;
    }
    case FODDER_ALERT_ARENA: // 0x1ac
    {
        if (flag2)
        {
            this->DoAutoAnim(NPC_GSPOT_STARTALT, 0);
        }

        this->GetInArena(dt);
        break;
    }
    case FODDER_ALERT_BEGIN: // 0x1d4
    {
        this->alertfod = FODDER_ALERT_CHASE;
        break;
    }
    case FODDER_ALERT_CHASE: // 0x1e0
    {
        if (zNPCGoalAlertFodder::CheckSpot(dt) && !(globals.player.DamageTimer > 0.0f))
        {
            this->alertfod = FODDER_ALERT_STABDONE;
            nextgoal = 'NGRE';
            *trantype = GOAL_TRAN_PUSH;
            break;
        }

        if (flag2)
        {
            this->DoAutoAnim(NPC_GSPOT_STARTALT, 0);
        }

        this->FlankPlayer(dt);

        break;
    }
    case FODDER_ALERT_STABDONE: // 0x250
    {
        this->alertfod = FODDER_ALERT_CHASE;
        break;
    }
    case FODDER_ALERT_EVADE: // 0x25c
    {
        if (flag2)
        {
            this->DoAutoAnim(NPC_GSPOT_STARTALT, 0);
            this->tmr_alertfod = 0.25f;
        }

        if (this->tmr_alertfod < 0.0f)
        {
            this->alertfod = FODDER_ALERT_BEGIN;
            break;
        }

        this->MoveEvade(dt);
        this->tmr_alertfod = MAX(-1.0f, this->tmr_alertfod - dt);

        break;
    }
    }

    if (this->alertfod != old_alertfod)
    {
        this->flg_info |= 2;
    }

    if (*trantype)
    {
        return nextgoal;
    }

    return this->xGoal::Process(trantype, dt, updCtxt, NULL);
}

S32 zNPCGoalAlertFodder::CheckSpot(F32 dt)
{
    S32 plyrInSpot;
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    xVec3 pos_plyr;
    xVec3 dir_plyr;
    F32 dy;
    F32 dst_plyr;
    F32 dot_plyr;

    zEntPlayer_PredictPos(&pos_plyr, 0.5f, 1.0f, 1);
    dst_plyr = npc->XZDstSqToPos(&pos_plyr, NULL, NULL);
    if (npc->XZDstSqToPlayer(NULL, NULL) < dst_plyr)
    {
        pos_plyr = *xEntGetPos(&globals.player.ent);
    }
    dst_plyr = npc->XZDstSqToPos(&pos_plyr, &dir_plyr, &dy);
    dst_plyr = xsqrt(dst_plyr);
    if (xabs(dy) > 3.4f)
    {
        plyrInSpot = 0;
    }
    else if (dst_plyr < 0.35f)
    {
        plyrInSpot = 1;
    }
    else if (dst_plyr > 1.5f)
    {
        plyrInSpot = 0;
    }
    else
    {
        dir_plyr *= 1.0f / dst_plyr;
        dot_plyr = xVec3Dot(&dir_plyr, NPCC_faceDir(npc));
        plyrInSpot = (dot_plyr < 0.86f) ? 0 : 1;
    }
    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, 2) != 0)
    {
        xDrawSetColor(g_GRAY50);
        xDrawSphere2(&pos_plyr, 0.1f, 12);
        xDrawSetColor(g_PINK);
        xDrawCyl(npc->Pos(), 1.5f, 3.4f, 0x2020C);
        xVec3 pos_head = *npc->Pos();
        pos_head.y += 1.0f;
        xDrawCyl(&pos_head, 0.35f, 0.2f, 0x2020C);
    }
    return plyrInSpot;
}

void zNPCGoalAlertFodder::FlankPlayer(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    xVec3 pos_plyr;
    xVec3 dir_arenaToPlyr;
    xVec3 dir_tan;
    xVec3 dir_plyr;
    xVec3 dir_arena;
    xVec3 dir_dest;
    xVec3 dir;
    F32 length;

    zEntPlayer_PredictPos(&pos_plyr, 0.5f, 1.0f, 1);

    if (npc->XZDstSqToPlayer(NULL, NULL) < npc->XZDstSqToPos(&pos_plyr, NULL, NULL))
    {
        xVec3Copy(&pos_plyr, xEntGetPos(&globals.player.ent));
    }

    xVec3Sub(&dir_arenaToPlyr, &pos_plyr, npc->arena.Pos());
    length = xVec3Length(&dir_arenaToPlyr);
    if (length < 1.0f)
    {
        xVec3Copy(&dir_arenaToPlyr, NPCC_faceDir(&globals.player.ent));
    }
    else
    {
        xVec3SMulBy(&dir_arenaToPlyr, 1.0f / length);
    }

    xVec3Cross(&dir_tan, &g_Y3, &dir_arenaToPlyr);

    xVec3Sub(&dir_plyr, xEntGetPos(&globals.player.ent), npc->Pos());
    length = xVec3Length(&dir_plyr);
    if (length < 1.0f)
    {
        xVec3Copy(&dir_plyr, NPCC_rightDir(&globals.player.ent));
    }
    else
    {
        xVec3SMulBy(&dir_plyr, 1.0f / length);
    }

    xVec3Sub(&dir_arena, npc->arena.Pos(), npc->zNPCCommon::Pos());
    length = xVec3Length(&dir_arena);
    if (length < 1.0f)
    {
        xVec3Copy(&dir_arena, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&dir_arena, 1.0f / length);
    }

    xVec3Dot(&dir_arena, &dir_plyr);
    xVec3Copy(&dir_dest, &dir_plyr);

    npc->ThrottleAdjust(dt, 6.0f, -1.0f);
    NPCC_ang_toXZDir(npc->frame->rot.angle + npc->TurnToFace(dt, &dir_dest, 4 * PI), &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

void zNPCGoalAlertFodder::GetInArena(F32 dt)
{
    xVec3 vec1;
    xVec3 dir_want;
    xVec3 dir;

    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3Sub(&vec1, npc->arena.Pos(), npc->zNPCCommon::Pos());

    F32 rot = xVec3Length(&vec1);

    if (rot < 1.0f)
    {
        xVec3Copy(&vec1, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&vec1, 1.0f / rot);
    }

    xVec3Copy(&dir_want, &vec1);

    npc->ThrottleAdjust(dt, 6.0f, -1.0f);
    rot = npc->TurnToFace(dt, &dir_want, -1.0f);
    NPCC_ang_toXZDir(npc->frame->rot.angle + rot, &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

void zNPCGoalAlertFodder::MoveEvade(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    xVec3 dir_arena;
    xVec3 dir_plyr;
    xVec3 dir_dest;
    xVec3 dir;
    F32 length;

    xVec3Sub(&dir_arena, npc->arena.Pos(), npc->Pos());
    length = xVec3Length(&dir_arena);
    if (length < 1.0f)
    {
        xVec3Copy(&dir_arena, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&dir_arena, 1.0f / length);
    }

    xVec3Sub(&dir_plyr, xEntGetPos(&globals.player.ent), npc->Pos());
    length = xVec3Length(&dir_plyr);
    if (length < 1.0f)
    {
        xVec3Copy(&dir_plyr, NPCC_rightDir(&globals.player.ent));
    }
    else
    {
        xVec3SMulBy(&dir_plyr, 1.0f / length);
    }

    xVec3SMul(&dir_dest, &dir_plyr, -1.0f);
    npc->ThrottleAdjust(dt, 6.0f, -1.0f);
    NPCC_ang_toXZDir(npc->frame->rot.angle + (npc->TurnToFace(dt, &dir_dest, PI * 4)), &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

S32 zNPCGoalAlertFodBomb::Enter(F32 dt, void* updCtxt)
{
    zNPCFodBomb* npc = (zNPCFodBomb*)(psyche->clt_owner);
    flg_attack = 0;
    tmr_nextping = 0.0f;
    alertbomb = FODBOMB_ALERT_NOTICE;
    npc->BlinkerReset();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertFodBomb::Resume(F32 dt, void* updCtxt)
{
    flg_info |= 2;
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertFodBomb::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCFodBomb* npc = (zNPCFodBomb*)(psyche->clt_owner);
    en_alertbomb old_alertbomb;
    S32 subenter;
    F32 tym_countdown;
    F32 pct_remain;

    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->arena.IncludesPlayer(0, 0))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    old_alertbomb = alertbomb;
    subenter = flg_info & 2;
    flg_info &= 0xFFFFFFF9;
    tym_countdown = 6.0f;
    if (zGameExtras_CheatFlags() & 0x800)
    {
        tym_countdown = 4.0f;
    }
    switch (alertbomb)
    {
    case FODBOMB_ALERT_NOTICE:
        alertbomb = FODBOMB_ALERT_SONAR;
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_NOTICE;
        break;
    case FODBOMB_ALERT_SONAR:
        if (subenter)
        {
            DoAutoAnim(NPC_GSPOT_STARTALT, 0);
            tmr_countdown = tym_countdown;
            tmr_nextping = 2.0f;
            npc->BlinkerReset();
        }
        tym_countdown = (tmr_countdown / tym_countdown);
        pct_remain = CLAMP(tym_countdown, 0.0f, 1.0f);
        npc->BlinkerUpdate(dt, pct_remain);
        if (npc->arena.IncludesNPC(npc, 0, 0) == 0)
        {
            alertbomb = FODBOMB_ALERT_TERMINAL;
        }
        else
        {
            SonarHoming(dt);
            if ((tmr_nextping < 0.0f) ? 1 : 0)
            {
                npc->SndPlayRandom(NPC_STYP_WARNBANG);
                if (tmr_countdown > 2.0f)
                {
                    tmr_nextping = 2.0f;
                }
                else
                {
                    tmr_nextping = 1.0f;
                }
            }
            else
            {
                tmr_nextping = MAX(-1.0f, (tmr_nextping - dt));
            }
            if (tmr_countdown < 0.0f)
            {
                alertbomb = FODBOMB_ALERT_TERMINAL;
            }
            tmr_countdown = MAX(-1.0f, (tmr_countdown - dt));
        }
        break;
    case FODBOMB_ALERT_TERMINAL:
        Detonate();
        zNPCGoalAfterlife* wanna = (zNPCGoalAfterlife*)(psyche->FindGoal(NPC_GOAL_AFTERLIFE));
        wanna->DieWithAWhimper();
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_AFTERLIFE;
        break;
    }
    if (alertbomb != old_alertbomb)
    {
        flg_info |= 2;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        return xGoal::Process(trantype, dt, updCtxt, NULL);
    }
    return nextgoal;
}

void zNPCGoalAlertFodBomb::Detonate()
{
    zNPCFodBomb* npc = *(zNPCFodBomb**)(&psyche->clt_owner);
    npc->SndPlayRandom(NPC_STYP_ATTACK);
    NPCHazard* haz = HAZ_Acquire();
    if (haz != NULL)
    {
        haz->ConfigHelper(NPC_HAZ_FODBOMB);
        haz->SetNPCOwner(npc);
        xVec3* center = xEntGetCenter(npc);
        haz->Start(center, -1.0f);
    }
}

// dwarf/SB/Game/zNPCGoalRobo.cpp names only npc, pos_plyr, dir_dest and dir here:
// the PS2 build dropped the intermediates whose results this one computes and
// throws away (dir_tan, and the dot of dir_arena with dir_plyr). Those four keep
// role-derived names. fVar4 is left alone deliberately - it is a shared scratch
// float that holds a vector length, then the pursuit speed, then the turn delta,
// so no single name is honest.
void zNPCGoalAlertFodBomb::SonarHoming(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(this->psyche->clt_owner);
    xVec3 pos_plyr;
    xVec3 dir_arenaToPlyr;
    xVec3 dir_tan;
    xVec3 dir_plyr;
    xVec3 dir_arena;
    xVec3 dir_dest;
    xVec3 dir;

    zEntPlayer_PredictPos(&pos_plyr, 0.5f, 1.0f, 0);

    if (npc->XZDstSqToPlayer(0, 0) < npc->XZDstSqToPos(&pos_plyr, 0, 0))
    {
        xVec3Copy(&pos_plyr, xEntGetPos(&globals.player.ent));
    }

    xVec3Sub(&dir_arenaToPlyr, &pos_plyr, npc->arena.Pos());

    F32 fVar4 = xVec3Length(&dir_arenaToPlyr);
    if (fVar4 < 1.0f)
    {
        xVec3Copy(&dir_arenaToPlyr, NPCC_faceDir(&globals.player.ent));
    }
    else
    {
        xVec3SMulBy(&dir_arenaToPlyr, 1.0f / fVar4);
    }

    xVec3Cross(&dir_tan, &g_Y3, &dir_arenaToPlyr);
    xVec3Sub(&dir_plyr, xEntGetPos(&globals.player.ent), npc->Pos());

    fVar4 = xVec3Length(&dir_plyr);
    if (fVar4 < 1.0f)
    {
        xVec3Copy(&dir_plyr, NPCC_rightDir(&globals.player.ent));
    }
    else
    {
        xVec3SMulBy(&dir_plyr, 1.0f / fVar4);
    }

    xVec3Sub(&dir_arena, npc->arena.Pos(), npc->zNPCCommon::Pos());

    fVar4 = xVec3Length(&dir_arena);
    if (fVar4 < 1.0f)
    {
        xVec3Copy(&dir_arena, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&dir_arena, 1.0f / fVar4);
    }

    xVec3Dot(&dir_arena, &dir_plyr);
    xVec3Copy(&dir_dest, &dir_plyr);

    fVar4 = 4.0f;
    F32 acc_pursuit = 1.0f;
    F32 spd_turnrate = DEG2RAD(135);
    if (zGameExtras_CheatFlags() & 0x800)
    {
        fVar4 = 6.0f;
        spd_turnrate = 2 * PI;
        acc_pursuit = 6.0f;
    }

    npc->ThrottleAdjust(dt, fVar4, acc_pursuit);
    fVar4 = (npc->TurnToFace(dt, &dir_dest, spd_turnrate));
    NPCC_ang_toXZDir(npc->frame->rot.angle + fVar4, &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

S32 zNPCGoalAlertFodBzzt::Enter(F32 dt, void* updCtxt)
{
    zNPCFodBzzt::cnt_alerthokey++;
    this->flg_alert = 0;
    this->flg_alert |= -(S32)(xrand() >> 0x17 & 1) + 2;
    this->alertbzzt = FODBZZT_ALERT_NOTICE;
    this->tmr_warmup = 1.25f;
    this->len_laser = 50.0f;
    this->cnt_nextlos = 0;
    this->cnt_inContact = 0;
    xVec3Copy(&this->pos_laserSource, &g_O3);
    xVec3Copy(&this->pos_laserTarget, &g_O3);

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertFodBzzt::Exit(F32 dt, void* updCtxt)
{
    zNPCFodBzzt::cnt_alerthokey--;
    S32 cnt = zNPCFodBzzt::cnt_alerthokey;
    zNPCFodBzzt::cnt_alerthokey = cnt & ~(cnt >> 31);

    zNPC_SNDStop(eNPCSnd_FodBzztAttack);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalAlertFodBzzt::Suspend(F32 dt, void* updCtxt)
{
    zNPC_SNDStop(eNPCSnd_FodBzztAttack);
    return xGoal::Suspend(dt, updCtxt);
}

S32 zNPCGoalAlertFodBzzt::Resume(F32 dt, void* updCtxt)
{
    zNPCFodBzzt* npc = (zNPCFodBzzt*)(psyche->clt_owner);
    flg_alert &= 0xFFFFFFFC;
    flg_alert = (-((xrand() >> 0x17) & 1) + 2) | flg_alert;
    tmr_warmup = 1.25;
    len_laser = 50.0;
    cnt_nextlos = 0;
    cnt_inContact = 0;
    xVec3Copy(&pos_laserSource, &g_O3);
    xVec3Copy(&pos_laserTarget, &g_O3);
    npc->flg_xtrarend &= 0xFFFFFFFE;
    zNPC_SNDPlay3D(eNPCSnd_FodBzztAttack, npc);
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertFodBzzt::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    en_alertbzzt old_alertbzzt;
    S32 nextgoal = 0;
    zNPCFodBzzt* npc = (zNPCFodBzzt*)(psyche->clt_owner);
    S32 subenter;
    zNPCGoalAfterlife* wanna;
    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
        zNPC_SNDStop(eNPCSnd_FodBzztAttack);
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!(npc->tmr_hokeypokey < 0))
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_HOKEYPOKEY;
    }
    else if (!npc->arena.IncludesPlayer(0, 0))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    old_alertbzzt = alertbzzt;
    subenter = flg_info & 2;
    flg_info &= 0xFFFFFFF9;
    if (alertbzzt == FODBZZT_ALERT_ARENA)
    {
        alertbzzt = FODBZZT_ALERT_CHASE;
        old_alertbzzt = FODBZZT_ALERT_ARENA;
        subenter = 1;
    }
    switch (alertbzzt)
    {
    case FODBZZT_ALERT_NOTICE:
        alertbzzt = FODBZZT_ALERT_CHASE;
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_NOTICE;
        zNPC_SNDPlay3D(eNPCSnd_FodBzztAttack, npc);
        break;
    case FODBZZT_ALERT_ARENA:
        if (subenter)
        {
            DoAutoAnim(NPC_GSPOT_STARTALT, 0);
            ToggleOrbit();
        }
        npc->FacePlayer(dt, 3 * PI);
        GetInArena(dt);
        DeathRayUpdate(dt);
        break;
    case FODBZZT_ALERT_CHASE:
        if (subenter)
        {
            alertbzzt = FODBZZT_ALERT_CHASE;
            DoAutoAnim(NPC_GSPOT_STARTALT, 0);
        }
        OrbitPlayer(dt);
        DeathRayUpdate(dt);
        break;
    }
    if (alertbzzt != old_alertbzzt)
    {
        flg_info |= 2;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        return xGoal::Process(trantype, dt, updCtxt, NULL);
    }
    return nextgoal;
}

void zNPCGoalAlertFodBzzt::ToggleOrbit()
{
    if (flg_alert & 1)
    {
        flg_alert &= -2;
        flg_alert |= 2;
    }
    else
    {
        flg_alert |= 1;
        flg_alert &= -3;
    }
}

void zNPCGoalAlertFodBzzt::OrbitPlayer(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3 dir_plyr;
    npc->XZVecToPlayer(&dir_plyr, NULL);

    F32 dst_plyr = xVec3Length(&dir_plyr);

    if (dst_plyr > 0.25f)
    {
        xVec3SMulBy(&dir_plyr, 1.0f / dst_plyr);
    }
    else
    {
        xVec3Copy(&dir_plyr, NPCC_faceDir(&globals.player.ent));
    }

    xVec3 dir_mimic = dir_plyr;
    // The double literal is load-bearing: DEG2RAD(63) folds in float and gives
    // 1.0995575, one ULP above retail's 1.0995574.
    npc->TurnToFace(dt, &dir_mimic, DEG2RAD(63.0));

    xVec3 pos_plyr;
    zEntPlayer_PredictPos(&pos_plyr, 0.3f, 1.0f, 1);

    if (SQ(dst_plyr) < npc->XZDstSqToPos(&pos_plyr, NULL, NULL))
    {
        xVec3Copy(&pos_plyr, xEntGetPos(&globals.player.ent));
    }

    S32 go_away = 0;

    if (flg_alert & 1)
    {
        if (dst_plyr < 3.0f)
        {
            go_away = 1;
        }
        else if (dst_plyr > 4.0f)
        {
            go_away = -1;
        }
        else
        {
            go_away = 0;
        }
    }
    else if (flg_alert & 2)
    {
        if (dst_plyr < 5.0f)
        {
            go_away = 1;
        }
        else if (dst_plyr > 6.0f)
        {
            go_away = -1;
        }
        else
        {
            go_away = 0;
        }
    }

    xVec3 dir_move;

    if (go_away < 0)
    {
        dir_move = dir_plyr;
    }
    else if (go_away > 0)
    {
        dir_move = dir_plyr * -1.0f;
    }
    else
    {
        dir_move = g_O3;
    }

    if (dst_plyr < 7.0f)
    {
        xVec3 dir_tan;
        xVec3Cross(&dir_tan, &g_Y3, &dir_plyr);

        if (flg_alert & 1)
        {
            dir_move += dir_tan * -1.0f;
        }
        else
        {
            dir_move += dir_tan;
        }
    }

    dir_move.normalize();

    npc->ThrottleAdjust(dt, 2.5f, -1.0f);
    npc->ThrottleApply(dt, &dir_move, 0);
}

void zNPCGoalAlertFodBzzt::GetInArena(F32 dt)
{
    xVec3 dir;

    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3Sub(&dir, npc->arena.Pos(), npc->zNPCCommon::Pos());

    F32 rot = xVec3Length(&dir);

    if (rot < 1.0f)
    {
        xVec3Copy(&dir, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&dir, 1.0f / rot);
    }

    npc->ThrottleAdjust(dt, 2.5f, -1.0f);
    npc->ThrottleApply(dt, &dir, 0);
}

void zNPCGoalAlertFodBzzt::DeathRayUpdate(F32 dt)
{
    static const RwRGBA rgba_benign = { 0, 255, 0, 128 };
    static const RwRGBA rgba_warmup = { 255, 255, 0, 176 };
    static const RwRGBA rgba_danger = { 255, 255, 255, 255 };

    zNPCFodBzzt* npc = (zNPCFodBzzt*)this->psyche->clt_owner;
    S32 canDoDamage = 1;

    if (!(tmr_warmup < 0.0f))
    {
        canDoDamage = 0;

        F32 pam = tmr_warmup / 1.25f;
        S32 doFX = 1;
        F32 pam_segments[8] = { 0.9f, 0.95f, 0.7f, 0.8f, 0.45f, 0.6f, 0.1f, 0.3f };
        S32 i;

        for (i = 0; i < 8; i += 2)
        {
            if (pam < pam_segments[i])
            {
                continue;
            }

            if (pam > pam_segments[i + 1])
            {
                continue;
            }

            doFX = 0;
            break;
        }

        F32 pam_inv = 1.0f - pam;

        RwRGBA rgba;
        rgba.red = LERP(pam_inv, rgba_benign.red, rgba_warmup.red);
        rgba.green = LERP(pam_inv, rgba_benign.green, rgba_warmup.green);
        rgba.blue = LERP(pam_inv, rgba_benign.blue, rgba_warmup.blue);
        rgba.alpha = LERP(pam_inv, rgba_benign.alpha, rgba_warmup.alpha);
        rgba_deathRay = rgba;

        tmr_warmup = MAX(-1.0f, tmr_warmup - dt);

        if (!doFX)
        {
            cnt_nextlos = 0;
            cnt_inContact = 0;
            return;
        }
    }
    else
    {
        rgba_deathRay = rgba_danger;
    }

    F32 dst_range = (flg_alert & 1) ? 5.0f : 6.0f;

    static const xVec3 vec_emitOffset = { 0.0f, 0.2f, 0.0f };

    xVec3 pos_src = *(const xVec3*)npc->BonePos(3);
    pos_src += vec_emitOffset;
    xMat3x3RMulVec(&pos_src, (const xMat3x3*)npc->BoneMat(0), &pos_src);
    pos_src += *(const xVec3*)npc->BonePos(0);

    xVec3 pos_tgt = *(const xVec3*)NPCC_faceDir(npc) * dst_range;
    pos_tgt += *npc->Pos();

    xVec3 dir_laser = pos_tgt - pos_src;
    dir_laser.normalize();

    cnt_nextlos--;

    if (cnt_nextlos < 0)
    {
        memset(&g_SharedCollisRecord, 0, sizeof(g_SharedCollisRecord));
        g_SharedCollisRecord.flags = k_HIT_0xF00 | k_HIT_CALC_HDNG;

        xCollis* colrec = &g_SharedCollisRecord;

        S32 rc = npc->HaveLOSToPos(&pos_tgt, 10.0f, globals.sceneCur, NULL, colrec);

        if (!rc && (colrec->flags & k_HIT_IT))
        {
            len_laser = colrec->dist;
        }
        else
        {
            len_laser = 10.0f;
        }

        if (NPCC_LineHitsBound(&pos_src, &pos_tgt, &globals.player.ent.bound, colrec))
        {
            F32 dst_plyr = colrec->dist + globals.player.ent.bound.sph.r;

            if (dst_plyr < len_laser)
            {
                len_laser = dst_plyr;
                cnt_inContact++;

                if (canDoDamage && cnt_inContact > 3)
                {
                    zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
                }
            }
            else if (dst_plyr > 10.0f)
            {
                cnt_inContact = (cnt_inContact - 1) & ~((cnt_inContact - 1) >> 31);
            }
            else
            {
                cnt_inContact++;
            }
        }
        else
        {
            cnt_inContact = (cnt_inContact - 1) & ~((cnt_inContact - 1) >> 31);
        }

        cnt_nextlos = 5;
        cnt_nextlos += (S32)(2.0f * xurand());
    }

    len_laser = MAX(0.25f, MIN(len_laser, 10.0f));

    pos_tgt = pos_src + dir_laser * len_laser;

    xVec3Copy(&pos_laserSource, &pos_src);
    xVec3Copy(&pos_laserTarget, &pos_tgt);

    if (xrand() & 0x800000)
    {
        zFX_SpawnBubbleTrail(&pos_tgt, 1);
    }

    npc->flg_xtrarend |= 1;
}

void zNPCGoalAlertFodBzzt::DeathRayRender()
{
    RwRGBA unkColor = this->rgba_deathRay;
    unkColor.alpha = 0x20;

    zNPCFodBzzt::laser.ColorSet(&this->rgba_deathRay, &unkColor);
    zNPCFodBzzt::laser.Render(&this->pos_laserSource, &this->pos_laserTarget);
}

S32 zNPCGoalAlertChomper::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = (zNPCCommon*)(psyche->clt_owner);
    alertchomp = CHOMPER_ALERT_NOTICE;
    npc->VelStop();
    pos_evade = g_O3;
    tmr_evade = -1.0f;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertChomper::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 nextgoal = 0;
    S32 subenter;
    en_alertchomp old_alertchomp;
    F32 pct;
    if (globals.player.Health < 1)
    {
        zNPCGoalLoopAnim* taunt = (zNPCGoalLoopAnim*)psyche->FindGoal(NPC_GOAL_TAUNT);
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->arena.IncludesPlayer(0, 0))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    old_alertchomp = alertchomp;
    subenter = flg_info & 2;
    flg_info &= 0xFFFFFFF9;
    pct = npc->arena.PctFromHome(npc->Pos());
    if (pct > 1.0f)
    {
        alertchomp = CHOMPER_ALERT_ARENA;
        DoAutoAnim(NPC_GSPOT_STARTALT, 0);
    }
    else if ((alertchomp == 1) && (pct < 0.5f))
    {
        alertchomp = CHOMPER_ALERT_CHASE;
    }
    switch (alertchomp)
    {
    case CHOMPER_ALERT_NOTICE:
        alertchomp = CHOMPER_ALERT_CHASE;
        nextgoal = NPC_GOAL_NOTICE;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case CHOMPER_ALERT_ARENA:
        GetInArena(dt);
        break;
    case CHOMPER_ALERT_CHASE:
        if (subenter != 0)
        {
            DoAutoAnim(NPC_GSPOT_STARTALT, 0);
        }
        CirclePlayer(dt);
        if ((CheckSpot(dt) != 0) && !(globals.player.DamageTimer > 0.0f))
        {
            alertchomp = CHOMPER_ALERT_EVADE;
            nextgoal = NPC_GOAL_ATTACKCHOMPER;
            *trantype = GOAL_TRAN_PUSH;
        }
        break;
    case CHOMPER_ALERT_EVADE:
        if (subenter != 0)
        {
            if (CalcEvadePos(&pos_evade) == 0)
            {
                alertchomp = CHOMPER_ALERT_CHASE;
                break;
            }
            F32 fVar3 = xsqrt(npc->XZDstSqToPos(&pos_evade, NULL, NULL)) / 2.5f;
            tmr_evade = CLAMP(fVar3, 1.0f, 2.0f);
            DoAutoAnim(NPC_GSPOT_STARTALT, 0);
        }
        if (tmr_evade < 0.0f)
        {
            alertchomp = CHOMPER_ALERT_CHASE;
        }
        tmr_evade = MAX(-1.0f, (tmr_evade - dt));
        if (MoveEvadePos(&pos_evade, dt))
        {
            alertchomp = CHOMPER_ALERT_CHASE;
        }
        break;
    }
    if (alertchomp != old_alertchomp)
    {
        flg_info |= 2;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    return nextgoal = xGoal::Process(trantype, dt, updCtxt, NULL);
}

void zNPCGoalAlertChomper::CirclePlayer(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    xVec3 pos_plyr;
    xVec3 dir_plyr;

    zEntPlayer_PredictPos(&pos_plyr, 1.3f, 1.0f, 0);

    if (npc->XZDstSqToPlayer(0, 0) < npc->XZDstSqToPos(&pos_plyr, 0, 0))
    {
        xVec3Copy(&pos_plyr, xEntGetPos(&globals.player.ent));
    }

    npc->XZVecToPlayer(&dir_plyr, NULL);

    F32 length = xVec3Length(&dir_plyr);
    if (length < 1.0f)
    {
        xVec3Copy(&dir_plyr, NPCC_rightDir(&globals.player.ent));
    }
    else
    {
        xVec3SMulBy(&dir_plyr, 1.0f / length);
    }

    xVec3 dir_dest = dir_plyr;
    F32 rot;
    xVec3 dir;

    npc->ThrottleAdjust(dt, 5.0f, 10.0f);
    NPCC_ang_toXZDir(npc->frame->rot.angle + (npc->TurnToFace(dt, &dir_dest, DEG2RAD(270))), &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

void zNPCGoalAlertChomper::GetInArena(F32 dt)
{
    xVec3 vec1;
    xVec3 dir_want;
    xVec3 dir;

    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3Sub(&vec1, npc->arena.Pos(), npc->zNPCCommon::Pos());

    F32 rot = xVec3Length(&vec1);

    if (rot < 1.0f)
    {
        xVec3Copy(&vec1, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&vec1, 1.0f / rot);
    }

    xVec3Copy(&dir_want, &vec1);

    npc->ThrottleAdjust(dt, 2.5f, -1.0f);
    rot = npc->TurnToFace(dt, &dir_want, -1.0f);
    NPCC_ang_toXZDir(npc->frame->rot.angle + rot, &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

S32 zNPCGoalAlertChomper::CalcEvadePos(xVec3* pos)
{
    S32 canEvade;
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    F32 rad_arena = npc->arena.Radius(1.0f);
    const xVec3 pos_home = *npc->arena.Pos();
    F32 rad_evade = MIN(10.0f, rad_arena - 2.0f);

    if (rad_evade < 2.0f)
    {
        canEvade = 0;
    }
    else
    {
        F32 ang = DEG2RAD(90) * xurand();
        F32 rad_ca = icos(ang);
        F32 rad_sa = isin(ang);

        rad_ca *= rad_evade;
        rad_sa *= rad_evade;

        xVec3 pos_loca[4];
        F32 ds2_best;
        S32 idx_best;

        for (S32 i = 0; i < 4; i++)
        {
            pos_loca[i] = pos_home;
        }

        pos_loca[0].x += rad_ca;
        pos_loca[0].z += rad_sa;
        pos_loca[1].x -= rad_sa;
        pos_loca[1].z += rad_ca;
        pos_loca[2].x -= rad_ca;
        pos_loca[2].z -= rad_sa;
        pos_loca[3].x += rad_sa;
        pos_loca[3].z -= rad_ca;

        ds2_best = -1.0f;
        idx_best = -1;

        for (S32 i = 0; i < 4; i++)
        {
            if (npc->XZDstSqToPos(&pos_loca[i], NULL, NULL) < 2.0f)
            {
                continue;
            }

            F32 ds2 = NPCC_DstSq(xEntGetPos(&globals.player.ent), &pos_loca[i], NULL);

            if (ds2 > ds2_best)
            {
                ds2_best = ds2;
                idx_best = i;
            }
        }

        if (idx_best < 0 || ds2_best < 0.0f)
        {
            *pos = *npc->Pos();
            canEvade = 0;
        }
        else
        {
            *pos = pos_loca[idx_best];
            canEvade = 1;
        }
    }

    return canEvade;
}

S32 zNPCGoalAlertChomper::MoveEvadePos(const xVec3* pos, F32 dt)
{
    S32 arrived = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 dir_evade;
    xVec3 dir;
    F32 dst = npc->XZDstSqToPos(pos, &dir_evade, 0);
    if (dst < SQ(1.0f))
    {
        arrived = 1;
    }
    else
    {
        dir_evade /= xsqrt(dst);
        npc->ThrottleAdjust(dt, 2.5f, -1.0f);
        NPCC_ang_toXZDir(npc->frame->rot.angle + npc->TurnToFace(dt, &dir_evade, -1.0f), &dir);
        npc->ThrottleApply(dt, &dir, 0);
    }
    return arrived;
}

S32 zNPCGoalAlertChomper::CheckSpot(F32 dt)
{
    S32 plyrInSpot;
    zNPCRobot* npc = (zNPCRobot*)(this->psyche->clt_owner);
    xVec3 pos_plyr;
    xVec3 dir_plyr;
    F32 dy;
    F32 dst_plyr;
    F32 dot_plyr;

    zEntPlayer_PredictPos(&pos_plyr, 1.3f, 1.0f, 1);
    dst_plyr = npc->XZDstSqToPos(&pos_plyr, NULL, NULL);
    if (npc->XZDstSqToPlayer(NULL, NULL) < dst_plyr)
    {
        pos_plyr = *xEntGetPos(&globals.player.ent);
    }
    dst_plyr = npc->XZDstSqToPos(&pos_plyr, &dir_plyr, &dy);
    dst_plyr = xsqrt(dst_plyr);
    if (xabs(dy) > 3.0f)
    {
        plyrInSpot = 0;
    }
    else if (dst_plyr < 0.3f)
    {
        plyrInSpot = 1;
    }
    else if (dst_plyr > 2.5f)
    {
        plyrInSpot = 0;
    }
    else
    {
        dir_plyr *= 1.0f / dst_plyr;
        dot_plyr = xVec3Dot(&dir_plyr, NPCC_faceDir(npc));
        plyrInSpot = (dot_plyr < 0.86f) ? 0 : 1;
    }
    return plyrInSpot;
}

S32 zNPCGoalAlertHammer::Enter(F32 dt, void* updCtxt)
{
    flg_attack = 0;
    alertham = HAMMER_ALERT_NOTICE;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertHammer::Exit(F32 dt, void* updCtxt)
{
    zNPCHammer* npc = ((zNPCHammer*)(psyche->clt_owner));
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalAlertHammer::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 nextgoal = 0;
    S32 subenter;
    en_alertham old_alertham;
    if (globals.player.Health < 1)
    {
        zNPCGoalLoopAnim* taunt = (zNPCGoalLoopAnim*)psyche->FindGoal(NPC_GOAL_TAUNT);
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.0f)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->arena.IncludesPlayer(0, 0))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    old_alertham = alertham;
    subenter = flg_info & 2;
    flg_info &= 0xFFFFFFF9;
    switch (alertham)
    {
    case HAMMER_ALERT_NOTICE:
        if (subenter)
        {
            DoAutoAnim(NPC_GSPOT_RESUME, 0);
        }
        alertham = HAMMER_ALERT_CHASE;
        break;
    case HAMMER_ALERT_BEGIN:
        alertham = HAMMER_ALERT_CHASE;
        break;
    case HAMMER_ALERT_CHASE:
        if ((PlayerInSpot(dt) != 0) && !(globals.player.DamageTimer > 0.0f))
        {
            alertham = HAMMER_ALERT_WHAM;
            break;
        }
        if (subenter)
        {
            DoAutoAnim(NPC_GSPOT_STARTALT, 0);
            tmr_alertham = 0.0f;
        }
        MoveChase(dt);
        NPCC_TmrCycle(&tmr_alertham, dt, 1.0);
        break;
    case HAMMER_ALERT_WHAM:
        alertham = HAMMER_ALERT_EVADE;
        nextgoal = NPC_GOAL_ATTACKHAMMER;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case HAMMER_ALERT_EVADE:
        if (subenter)
        {
            tmr_alertham = 0.25f;
        }
        if (tmr_alertham < 0.0f)
        {
            alertham = HAMMER_ALERT_CHASE;
        }
        else
        {
            MoveEvade(dt);
            npc->FacePlayer(dt, 3 * PI);
            tmr_alertham = MAX(-1.0f, (tmr_alertham - dt));
            if (subenter)
            {
                DoAutoAnim(NPC_GSPOT_STARTALT, 0);
            }
        }
        break;
    }
    if (alertham != old_alertham)
    {
        flg_info |= 2;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

S32 zNPCGoalAlertHammer::PlayerInSpot(F32 dt)
{
    S32 plyrInSpot;

    xVec3 dir_plyr;
    xVec3 pos_zone;
    xVec3 pos_plyr;
    xVec3 vec;

    F32 dst_guess;
    F32 dst_plyr;
    F32 dy;
    F32 spd_mover;

    plyrInSpot = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    dst_plyr = xsqrt(npc->XZDstSqToPlayer(&dir_plyr, &dy));
    dy = __fabs(dy);
    if (dst_plyr < 2.25f)
    {
        return 1;
    }
    else if (dy > 3.75f)
    {
        return 0;
    }
    else if (dst_plyr < 0.4f)
    {
        return 1;
    }
    else
    {
        dir_plyr *= (1.0f / dst_plyr);
        if (xVec3Dot(&dir_plyr, (xVec3*)NPCC_faceDir(npc)) < 0.5f)
        {
            return 0;
        }
        else
        {
            xVec3SMul(&pos_zone, (xVec3*)NPCC_faceDir(npc), 3.5f);
            xVec3AddTo(&pos_zone, (xVec3*)npc->Pos());
            xVec3Copy(&pos_plyr, (xVec3*)xEntGetPos(&globals.player.ent));
            dst_guess = xsqrt(NPCC_DstSq(&pos_zone, (xVec3*)xEntGetPos(&globals.player.ent), NULL));
            spd_mover = MAX(npc->spd_throttle, 12.0f);
            zEntPlayer_PredictPos(&pos_plyr, MIN((dst_guess / spd_mover) + 0.5f, 2.0f), 1.0f, 0);
            xVec3Sub(&vec, &pos_plyr, &pos_zone);
            if ((F32)__fabs(vec.y) < 2.5f)
            {
                vec.y = 0.0f;
                if (xVec3Length2(&vec) < 1.25f)
                {
                    plyrInSpot = 1;
                }
            }
        }
    }
    return plyrInSpot;
}

void zNPCGoalAlertHammer::MoveChase(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    F32 dst_plyrXYZ = xsqrt(npc->XYZDstSqToPlayer(NULL));
    F32 tym_predict = dst_plyrXYZ / MAX(npc->spd_throttle, 6.0f);
    tym_predict = MIN(tym_predict, 2.0f);

    xVec3 pos_pred;
    zEntPlayer_PredictPos(&pos_pred, tym_predict, 1.0f, 0);

    xVec3 dir_pred;
    F32 dst_pred = xsqrt(npc->XZDstSqToPos(&pos_pred, &dir_pred, NULL));

    if (dst_pred < 0.2f)
    {
        xVec3Copy(&dir_pred, NPCC_rightDir(npc));
        dst_pred = 1.0f;
    }
    else
    {
        xVec3SMulBy(&dir_pred, 1.0f / dst_pred);
    }

    xVec3 dir_NtoP;
    F32 dst_NtoP = xsqrt(npc->XZDstSqToPos(&pos_pred, &dir_NtoP, NULL));

    xVec3 dir_plyr;
    F32 dst_plyr = xsqrt(npc->XZDstSqToPlayer(&dir_plyr, NULL));

    xVec3 dir_PtoP;
    F32 dst_PtoP = xsqrt(NPCC_DstSq(xEntGetPos(&globals.player.ent), &pos_pred, &dir_PtoP));

    if (!(dst_PtoP < 0.1f))
    {
        dir_PtoP *= 1.0f / dst_PtoP;

        if (!(dst_NtoP < 0.1f))
        {
            dir_NtoP *= 1.0f / dst_NtoP;

            if (!(dst_plyr < 0.1f))
            {
                dir_plyr *= 1.0f / dst_plyr;

                if (!(dir_plyr.dot(dir_NtoP) > -0.3f))
                {
                    xVec3 dir_inv;
                    xVec3Inv(&dir_inv, &dir_plyr);

                    F32 dot_back = xVec3Dot(&dir_inv, &dir_PtoP);

                    xVec3 pos_repred = dir_PtoP * (0.75f * (dot_back * dst_plyr));
                    pos_repred += *xEntGetPos(&globals.player.ent);

                    xVec3 dir_revised;
                    F32 ds2_revised = npc->XZDstSqToPos(&pos_repred, &dir_revised, NULL);

                    if (!(ds2_revised < 0.00001f))
                    {
                        xVec3SMul(&dir_pred, &dir_revised, 1.0f / xsqrt(ds2_revised));
                        dst_pred = ds2_revised;
                    }
                }
            }
        }
    }

    npc->TurnToFace(dt, &dir_pred, DEG2RAD(360));

    if (dst_pred < 5.0f)
    {
        npc->ThrottleAdjust(dt, 4.0f, -1.0f);
    }
    else
    {
        npc->ThrottleAdjust(dt, 7.0f, 14.0f);
    }

    npc->ThrottleApply(dt, &dir_pred, 0);
}

void zNPCGoalAlertHammer::MoveEvade(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    xVec3 dir_arena;
    xVec3 dir_dest;
    F32 length;

    xVec3Sub(&dir_arena, npc->arena.Pos(), npc->Pos());
    length = xVec3Length(&dir_arena);
    if (length < 1.0f)
    {
        xVec3Copy(&dir_arena, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&dir_arena, 1.0f / length);
    }

    xVec3Sub(&dir_dest, xEntGetPos(&globals.player.ent), npc->Pos());
    length = xVec3Length(&dir_dest);
    if (length < 0.5f)
    {
        xVec3Copy(&dir_dest, NPCC_rightDir(&globals.player.ent));
    }
    else
    {
        xVec3SMulBy(&dir_dest, 1.0f / length);
    }

    xVec3SMulBy(&dir_dest, -1.0f);
    npc->ThrottleAdjust(dt, 7.0f, 14.0f);
    npc->ThrottleApply(dt, &dir_dest, 0);
}

S32 zNPCGoalAlertTarTar::Enter(F32 dt, void* updCtxt)
{
    zNPCTarTar* npc = ((zNPCTarTar*)(psyche->clt_owner));
    flg_attack = 0;
    alerttart = TARTAR_ALERT_BEGIN;
    hoppy = HOPPY_PATTERN_START;
    tmr_reload = 0.0f;
    npc->VelStop();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertTarTar::Resume(F32 dt, void* updCtxt)
{
    zNPCTarTar* npc = ((zNPCTarTar*)(psyche->clt_owner));
    npc->VelStop();
    flg_info |= 2;
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertTarTar::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal;
    zNPCTarTar* npc;
    en_alerttart old_alerttart;
    F32 tym_reload;
    xVec3 dir_HtoP;
    F32 dsq;
    S32 subenter;
    F32 rad;

    npc = (zNPCTarTar*)(psyche->clt_owner);
    subenter = flg_info & 2;
    old_alerttart = alerttart;
    flg_info &= ~6;
    nextgoal = 0;
    tym_reload = 5.0f;
    dsq = npc->arena.DstSqFromHome(xEntGetPos(&globals.player.ent), &dir_HtoP);
    if ((npc->arena.Radius(1.0f) * 1.5f) > npc->cfg_npc->rad_attack)
    {
        rad = npc->arena.Radius(1.0f) * 1.5f;
    }
    else
    {
        rad = npc->cfg_npc->rad_attack;
    }
    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->npcset.allowDetect)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (dsq > SQ(rad))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    if (alerttart != TARTAR_ALERT_ARENA && (npc->arena.PctFromHome(npc->Pos()) > 1.0f))
    {
        alerttart = TARTAR_ALERT_ARENA;
        DoAutoAnim(NPC_GSPOT_STARTALT, 0);
    }
    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, -1))
    {
        xDrawSetColor(g_YELLOW);
        NPCC_DrawPlayerPredict(5, 1.0f, 4.0f);
    }
    switch (alerttart)
    {
    case TARTAR_ALERT_NOTICE:
        npc->VelStop();
        alerttart = TARTAR_ALERT_BEGIN;
        nextgoal = NPC_GOAL_NOTICE;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case TARTAR_ALERT_ARENA:
        GetInArena(dt);
        if (npc->arena.PctFromHome(npc->Pos()) < 0.5f)
        {
            alerttart = TARTAR_ALERT_BEGIN;
        }
        break;
    case TARTAR_ALERT_BEGIN:
        npc->VelStop();
        alerttart = (en_alerttart)3;
        hoppy = (en_hoppy)0;
        break;
    case TARTAR_ALERT_READY:
        npc->VelStop();
        if (subenter)
        {
            DoAutoAnim(NPC_GSPOT_RESUME, 0);
        }
        nextgoal = HoppyUpdate(trantype, dt);
        if (*trantype != GOAL_TRAN_NONE)
        {
            return nextgoal;
        }
        break;
    }
    if (alerttart != old_alerttart)
    {
        flg_info |= 2;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

S32 zNPCGoalAlertTarTar::NPCMessage(NPCMsg* mail)
{
    S32 snarfed = 1;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xPsyche* psy = GetPsyche();
    switch (mail->msgid)
    {
    case NPC_MID_DAMAGE:
        if ((psy->GIDInStack(NPC_GOAL_WOUND) != NULL) || (psy->GIDOfPending() == NPC_GOAL_WOUND))
        {
            break;
        }

        if ((npc->hitpoints > 1) && (mail->infotype == NPC_MDAT_DAMAGE))
        {
            if ((mail->dmgdata.dmg_type == DMGTYP_SIDE) ||
                (mail->dmgdata.dmg_type == DMGTYP_HITBYTOSS))
            {
                alerttart = TARTAR_ALERT_READY;
                flg_info |= 2;
                psy->GoalPush(NPC_GOAL_WOUND, 0);
                break;
            }
        }

        snarfed = 0;
        break;
    default:
        snarfed = 0;
        break;
    }
    return snarfed;
}

S32 zNPCGoalAlertTarTar::HoppyUpdate(en_trantype* trantype, F32 dt)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    npc->FacePlayer(dt, DEG2RAD(540));

    tmr_reload = MAX(-1.0f, tmr_reload - dt);

    if (!(tmr_reload < 0.0f))
    {
        return 0;
    }

    F32 tym_reload = 2.0f;

    if (zGameExtras_CheatFlags() & 0x800)
    {
        tym_reload = 0.25f;
    }

    xVec3 dir_plyr;
    F32 ds2_plyr = npc->XYZDstSqToPlayer(&dir_plyr);

    if (xabs(dir_plyr.y) > 12.0f)
    {
        return 0;
    }

    if (ds2_plyr < 1.0f)
    {
        return 0;
    }

    xVec3SMulBy(&dir_plyr, 1.0f / xsqrt(ds2_plyr));

    F32 dot = xVec3Dot(NPCC_faceDir(npc), &dir_plyr);

    if (dot < 0.86f)
    {
        return 0;
    }

    switch (hoppy)
    {
    case HOPPY_PATTERN_START:
        if (!npc->npcset.allowChase)
        {
            hoppy = HOPPY_PATTERN_SHOOT;
        }
        else if (xrand() & 0x800000)
        {
            hoppy = HOPPY_PATTERN_SHOOT;
        }
        else
        {
            hoppy = HOPPY_PATTERN_SHOOT;
        }
        break;
    case HOPPY_PATTERN_SHOOT:
        tmr_reload = tym_reload;
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_ATTACKTARTAR;
        break;
    case HOPPY_PATTERN_HOPLEFT:
        hoppy = HOPPY_PATTERN_HOPRIGHT;
        break;
    case HOPPY_PATTERN_HOPRIGHT:
        hoppy = HOPPY_PATTERN_SHOOT;
        break;
    case HOPPY_PATTERN_HOPSHOOT:
        hoppy = HOPPY_PATTERN_HOPSHOOT;
        tmr_reload = tym_reload * (0.25f * (xurand() - 0.5f)) + tym_reload;
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_ATTACKTARTAR;
        break;
    }

    return nextgoal;
}

void zNPCGoalAlertTarTar::GetInArena(F32 dt)
{
    xVec3 vec1;
    xVec3 dir_want;
    xVec3 dir;

    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3Sub(&vec1, npc->arena.Pos(), npc->zNPCCommon::Pos());

    F32 rot = xVec3Length(&vec1);

    if (rot < 1.0f)
    {
        xVec3Copy(&vec1, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&vec1, 1.0f / rot);
    }

    xVec3Copy(&dir_want, &vec1);

    npc->ThrottleAdjust(dt, 2.0f, -1.0f);
    rot = npc->TurnToFace(dt, &dir_want, -1.0f);
    NPCC_ang_toXZDir(npc->frame->rot.angle + rot, &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

S32 zNPCGoalAlertGlove::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    CalcAttackVector();
    npc->flg_vuln &= 0xffefffff;
    npc->flg_vuln &= 0x7dfeffff;
    tmr_minAttack = 1.0f;
    zNPC_SNDPlay3D(eNPCSnd_GloveAttack, npc);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertGlove::Exit(F32 dt, void* updCtxt)
{
    zNPCGlove* npc = ((zNPCGlove*)(psyche->clt_owner));
    npc->flg_vuln |= 0x00100000;
    npc->flg_vuln |= 0x82010000;
    zNPC_SNDStop(eNPCSnd_GloveAttack);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalAlertGlove::Suspend(F32 dt, void* updCtxt)
{
    zNPCGlove* npc = ((zNPCGlove*)(psyche->clt_owner));
    npc->flg_vuln |= 0x00100000;
    npc->flg_vuln |= 0x82010000;
    zNPC_SNDStop(eNPCSnd_GloveAttack);
    return xGoal::Suspend(dt, updCtxt);
}

S32 zNPCGoalAlertGlove::Resume(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = (zNPCCommon*)(psyche->clt_owner);
    npc->flg_vuln &= 0xffefffff;
    npc->flg_vuln &= 0x7dfeffff;
    tmr_minAttack = 1.0;
    zNPC_SNDPlay3D(eNPCSnd_GloveAttack, npc);
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertGlove::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 rc;
    xVec3 path = { 0.0f, 0.0f, 0.0f };

    if (tmr_minAttack < 0.0f)
    {
        if (globals.player.Health < 1)
        {
            zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
            taunt->LoopCountSet(1000);
            *trantype = GOAL_TRAN_PUSH;
            nextgoal = NPC_GOAL_TAUNT;
        }
        else if (globals.player.DamageTimer > 0.5f)
        {
            *trantype = GOAL_TRAN_PUSH;
            nextgoal = NPC_GOAL_TAUNT;
        }
        else if (npc->SomethingWonderful() != 0)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_IDLE;
        }
        else if (!npc->arena.IncludesNPC(npc, 0.0f, NULL))
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_IDLE;
        }
    }

    if (psyche->TimerGet(XPSY_TYMR_CURGOAL) > 0.25f)
    {
        if (!npc->arena.IncludesPlayer(0.0f, NULL))
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_IDLE;
        }
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    if (goback != 0 && tmr_attack < 0.0f)
    {
        CalcAttackVector();
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    if (goback != 0)
    {
        xVec3Sub(&path, &pos_began, npc->Pos());
    }
    else
    {
        xVec3Sub(&path, &pos_end, npc->Pos());
    }

    path.y = 0.0f;
    xVec3Normalize(&path, &path);

    npc->ThrottleAccel(dt, 1, 0.85f);
    npc->TurnToFace(dt, &path, -1.0f);
    npc->ThrottleApply(dt, &path, 0);

    rc = (tmr_minAttack < 0.75f) ? CheckHandBones() : 0;

    if (rc != 0 && goback == 0)
    {
        F32 dst;

        goback = 1;
        xVec3Sub(&path, &pos_began, npc->Pos());
        dst = xVec3Normalize(&path, &path);
        tmr_attack = 2.0f * dst / npc->cfg_npc->spd_moveMax;
    }
    else if (goback == 0 && tmr_attack < 0.0f)
    {
        F32 dst;

        goback = 1;
        xVec3Sub(&path, &pos_began, npc->Pos());
        dst = xVec3Normalize(&path, &path);
        tmr_attack = 2.0f * dst / npc->cfg_npc->spd_moveMax;
    }

    if (--cnt_nextemit < 0)
    {
        U32 astid;

        cnt_nextemit = 3;
        astid = npc->AnimCurStateID();

        if (astid != g_hash_roboanim[17])
        {
            FXWhirlwind();
            FXTurbulence();
        }
    }

    tmr_attack = MAX(-1.0f, tmr_attack - dt);
    tmr_minAttack = MAX(-1.0f, tmr_minAttack - dt);

    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

static S32 g_idx_handbone[6] = { 10, 15, 25, 35, 40, -1 };

void zNPCGoalAlertGlove::FXTurbulence()
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);

    if (xrand() & 0x800000)
    {
        return;
    }

    const xVec3 pos_npc = *npc->Pos();
    S32 idx_bone;
    S32 i = 0;

    while ((idx_bone = g_idx_handbone[i]) >= 0)
    {
        i++;

        if (xrand() & 0x800000)
        {
            continue;
        }

        xVec3 pos_base = *(const xVec3*)npc->BonePos(idx_bone);
        xMat3x3RMulVec(&pos_base, (const xMat3x3*)npc->BoneMat(0), &pos_base);
        pos_base += pos_npc;

        xVec3 dir_out;
        dir_out = *(const xVec3*)npc->BonePos(idx_bone);
        dir_out.y = 0.0f;
        dir_out.normalize();
        xMat3x3RMulVec(&dir_out, (const xMat3x3*)npc->BoneMat(0), &dir_out);

        xVec3 dir_travel;
        xVec3Cross(&dir_travel, &dir_out, &g_Y3);

        xVec3 pos_disperse;
        xVec3 vel_disperse;

        pos_disperse = g_Y3 * (0.1f * (2.0f * (xurand() - 0.5f)));
        pos_disperse += dir_out * (0.1f * xurand());

        vel_disperse = g_Y3 * -0.5f;
        vel_disperse += dir_out * -1.0f;
        vel_disperse += dir_travel * 4.0f;

        zFX_SpawnBubbleTrailNoNegRandVel(&pos_base, 16, &pos_disperse, &vel_disperse);
    }
}

void zNPCGoalAlertGlove::FXWhirlwind()
{
    zNPCCommon* npc = (zNPCCommon*)(psyche->clt_owner);
    const xVec3 pos_npc = *npc->Pos();
    xVec3 pos;
    S32 i;

    pos.x = pos_npc.x;
    pos.y = 0.0f;
    pos.z = pos_npc.z;

    for (i = 0; i < 4.0f; i++)
    {
        F32 pct = xurand();
        F32 inv = 1.0f - pct;

        pos.y = pos_npc.y + 0.55f * pct;

        F32 spd = LERP(inv, 0.4f, 3.4f);

        xVec3 dir = { 0.0f, 0.0f, 0.0f };
        dir.x = 2.0f * (2.0f * (xurand() - 0.5f));
        dir.y = 0.4f * inv;
        dir.z = 2.0f * (2.0f * (xurand() - 0.5f));
        dir.normalize();
        dir *= spd;

        NPAR_EmitGloveDust(&pos, &dir);
    }
}

void zNPCGoalAlertGlove::CalcAttackVector()
{
    zNPCCommon* npc = (zNPCCommon*)(psyche->clt_owner);

    xVec3Sub(&dir_axis, xEntGetPos(&globals.player.ent), npc->Pos());
    dir_axis.y = 0.0f;
    dst_extend = xVec3Normalize(&dir_axis, &dir_axis);

    xVec3Copy(&pos_began, npc->Pos());
    xVec3SMul(&pos_end, &dir_axis, dst_extend);
    xVec3AddTo(&pos_end, npc->Pos());

    goback = 0;
    tmr_attack = 2.0f * dst_extend / npc->cfg_npc->spd_moveMax;
}

S32 zNPCGoalAlertGlove::CheckHandBones()
{
    S32 yeppers_hitplayer = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);

    xBound bnd;
    memset(&bnd, 0, sizeof(xBound));
    bnd.type = 1;

    memset(&g_SharedCollisRecord, 0, sizeof(g_SharedCollisRecord));
    g_SharedCollisRecord.flags = k_HIT_0xF00 | k_HIT_CALC_HDNG;

    xCollis* colrec = &g_SharedCollisRecord;

    static S32 skipballchecks = 1;

    xEntBoulder* bowl = globals.player.bubblebowl;
    S32 doball = 0;
    xVec3 thatway;
    xVec3 dir_smack;

    if (bowl != NULL && xEntIsVisible(bowl) && (--skipballchecks < 0))
    {
        skipballchecks = 1;

        if (!(bowl->timeToLive <= 0.0f))
        {
            npc->XYZVecToPos(&dir_smack, xEntGetPos(bowl));

            if (!(dir_smack.y < -2.0f || dir_smack.y > 3.0f))
            {
                dir_smack.y = 0.0f;

                F32 ds2_bowl = xVec3Length2(&dir_smack);

                if (!(ds2_bowl < 0.001f || ds2_bowl > 16.0f))
                {
                    xVec3SMulBy(&dir_smack, 1.0f / xsqrt(ds2_bowl));
                    xVec3Cross(&thatway, &dir_smack, &g_Y3);
                    xVec3AddScaled(&thatway, &dir_smack, 0.5f);
                    xVec3Normalize(&thatway, &thatway);
                    doball = 1;
                }
            }
        }
    }

    bnd.sph.r = npc->cfg_npc->rad_dmgSize;

    S32 idx_bone;
    S32 i = 0;

    while ((idx_bone = g_idx_handbone[i]) >= 0)
    {
        i++;

        xVec3 pos = *(const xVec3*)npc->BonePos(idx_bone);
        pos *= 0.75f;
        xMat3x3RMulVec(&pos, (const xMat3x3*)npc->BoneMat(0), &pos);
        pos += *npc->Pos();
        bnd.sph.center = pos;

        if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, 2) != 0)
        {
            xDrawSetColor(g_PINK);
            xBoundDraw(&bnd);
        }

        S32 rc = NPCC_chk_hitPlyr(&bnd, colrec);

        if (rc != 0)
        {
            zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
            yeppers_hitplayer = 1;
        }

        if (doball != 0)
        {
            memset(colrec, 0, sizeof(xCollis));

            rc = NPCC_chk_hitEnt(bowl, &bnd, colrec);

            if (rc != 0)
            {
                xVec3 vel_smack;

                xVec3Copy(&vel_smack, &bowl->vel);

                if (xVec3Length(&vel_smack) < 3.0f)
                {
                    xVec3SMul(&vel_smack, &thatway, 3.0f);
                }

                thatway.y = 1e-05f;
                NPCC_Bounce(&vel_smack, &thatway, 1.25f);
                xVec3Copy(&bowl->vel, &vel_smack);
            }
        }
    }

    return yeppers_hitplayer;
}

S32 zNPCGoalAlertGlove::CollReview(void*)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xEntCollis* npccol = npc->collis;
    xCollis* colrec = NULL;
    xVec3 vec_depen = { 0.0f, 0.0f, 0.0f };
    S32 hitstuff = 0;
    S32 i;
    xVec3 pump = { 0.0f, 0.0f, 0.0f };
    // Retail's object carries a third anonymous zero-filled xVec3 template here
    // that nothing references - a local whose every use was optimised away.
    // Reproduced so the .rodata pool keeps its retail layout.
    xVec3 vec_deadstripped = { 0.0f, 0.0f, 0.0f };
    F32 spd = 0.0f;
    xSurface* surf;
    S32 badsurf = 0;
    F32 goodep = 0.0f;

    for (i = npccol->env_sidx; i < npccol->env_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    for (i = npccol->stat_sidx; i < npccol->stat_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    for (i = npccol->dyn_sidx; i < npccol->dyn_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    if (npccol->npc_sidx < npccol->npc_eidx)
    {
        spd = xVec3Normalize(&pump, &npc->frame->vel);
    }

    for (i = npccol->npc_sidx; i < npccol->npc_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        zNPCCommon* tgt = (zNPCCommon*)(xEnt*)(colrec->optr);

        xVec3Normalize(&pump, &colrec->tohit);
        xVec3SMulBy(&pump, spd);
        tgt->Damage(DMGTYP_HITBYTOSS, npc, &pump);
    }

    if ((psyche->TimerGet(XPSY_TYMR_CURGOAL) > 0.1f) && hitstuff &&
        (xVec3Length2(&vec_depen) > 0.0f))
    {
        CalcAttackVector();
    }

    if (badsurf)
    {
        npc->Damage(DMGTYP_DAMAGE_SURFACE, NULL, NULL);
    }

    return 0;
}

S32 zNPCGoalAlertMonsoon::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    flg_attack = 0;
    alertmony = MONSOON_ALERT_NOTICE;
    tmr_reload = 0.0;
    xVec3Copy(&pos_corner, npc->Pos());
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertMonsoon::Exit(F32 dt, void* updCtxt)
{
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalAlertMonsoon::Resume(F32 dt, void* updCtxt)
{
    alertmony = MONSOON_ALERT_BEGIN;
    flg_attack = 0;
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertMonsoon::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal;
    zNPCRobot* npc;
    en_alertmony old_alertmony;
    F32 tym_reload;
    xVec3 dir_HtoP;
    F32 dsq;
    S32 subenter;
    F32 rad;
    npc = (zNPCRobot*)(psyche->clt_owner);
    nextgoal = 0;
    tym_reload = 2.5f;
    dsq = npc->arena.DstSqFromHome(xEntGetPos(&globals.player.ent), &dir_HtoP);
    if ((npc->arena.Radius(1.0f) * 1.5f) > npc->cfg_npc->rad_attack)
    {
        rad = npc->arena.Radius(1.0f) * 1.5f;
    }
    else
    {
        rad = npc->cfg_npc->rad_attack;
    }
    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->npcset.allowDetect)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->arena.IsReady() || dsq > SQ(rad))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    subenter = flg_info & 2;
    old_alertmony = alertmony;
    flg_info &= ~6;
    if (alertmony != (en_alertmony)SLICK_ALERT_ARENA && (npc->arena.PctFromHome(npc->Pos()) > 1.1f))
    {
        alertmony = (en_alertmony)SLICK_ALERT_ARENA;
        subenter = 1;
    }
    switch (alertmony)
    {
    case MONSOON_ALERT_NOTICE:
        alertmony = MONSOON_ALERT_BEGIN;
        npc->FacePlayer(dt, 3 * PI);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_NOTICE;
        break;
    case MONSOON_ALERT_ARENA:
        npc->FacePlayer(dt, 3 * PI);
        npc->MoveTowardsArena(dt, 4.0f);
        if (npc->arena.PctFromHome(npc->Pos()) < 0.5f)
        {
            alertmony = MONSOON_ALERT_READY;
        }
        if (subenter)
        {
            DoAutoAnim(NPC_GSPOT_STARTALT, 0);
        }
        break;
    case MONSOON_ALERT_BEGIN:
        alertmony = MONSOON_ALERT_READY;
        break;
    case MONSOON_ALERT_READY:
        if (((tmr_reload < 0.0f) ? 1 : 0) && !(globals.player.DamageTimer > 0.0f))
        {
            alertmony = MONSOON_ALERT_SPITCLOUD;
            break;
        }
        else
        {
            tmr_reload = MAX(-1.0f, (tmr_reload - dt));
            if (subenter != 0)
            {
                DoAutoAnim(NPC_GSPOT_RESUME, 0);
                npc->VelStop();
                if (!npc->arena.IncludesPos(&pos_corner, NULL, NULL))
                {
                    xVec3Copy(&pos_corner, npc->Pos());
                }
            }
            npc->FacePlayer(dt, 3 * PI);
            MoveCorner(dt);
        }
        break;
    case MONSOON_ALERT_SPITCLOUD:
        tmr_reload = tym_reload + (tym_reload * (0.25f * (xurand() - 0.5f)));
        alertmony = MONSOON_ALERT_READY;
        nextgoal = NPC_GOAL_ATTACKMONSOON;
        *trantype = GOAL_TRAN_PUSH;
        break;
    }
    if (alertmony != old_alertmony)
    {
        flg_info |= 2;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

void zNPCGoalAlertMonsoon::MoveCorner(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    F32 ds2_corn;
    xVec3 dir_corn;
    if (npc->npcset.allowChase && npc->arena.IsReady() && (npc->arena.Radius(1.0f) > 2.0f))
    {
        ds2_corn = npc->XYZDstSqToPos(&pos_corner, 0);
        if (ds2_corn < SQ(0.5f))
        {
            npc->CornerOfArena(&pos_corner, -1.0f);
            ds2_corn = npc->XYZDstSqToPos(&pos_corner, NULL);
        }
        if (npc->DBG_IsNormLog(eNPCDCAT_Ten, 2))
        {
            xDrawSetColor(g_BLUE);
            xDrawSphere2(&pos_corner, 0.1f, 12);
        }
        if ((tmr_reload >= 0.0f) && (tmr_reload < 1.0f))
        {
            npc->ThrottleAdjust(dt, 1.5, -1.0);
        }
        else if (ds2_corn < 2.0f)
        {
            npc->ThrottleAdjust(dt, 1.5f, -1.0f);
        }
        else
        {
            npc->ThrottleAdjust(dt, 3.5f, -1.0f);
        }
        npc->XYZVecToPos(&dir_corn, &pos_corner);
        xVec3Normalize(&dir_corn, &dir_corn);
        npc->ThrottleApply(dt, &dir_corn, 0);
    }
}

S32 zNPCGoalAlertSleepy::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    flg_attack = 0;
    sleepattack = SLEEP_ATAK_REACT;
    npc->VelStop();
    npc->ModelAtomicShow(1, NULL);
    zNPC_SNDPlay3D(eNPCSnd_SleepyAttack, npc);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertSleepy::Exit(F32 dt, void* updCtxt)
{
    zNPCSleepy* npc = ((zNPCSleepy*)(psyche->clt_owner));
    npc->ModelAtomicHide(1, NULL);
    zNPC_SNDStop(eNPCSnd_SleepyAttack);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalAlertSleepy::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCSleepy* npc = (zNPCSleepy*)(psyche->clt_owner);
    en_slepatak old_sleepattack = sleepattack;
    S32 subenter = flg_info & 2;
    xVec3 dir_plyr;

    flg_info &= ~6;

    F32 dsq = npc->XZDstSqToPlayer(&dir_plyr, NULL);

    if (zEntTeleportBox_playerIn())
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if ((tmr_minAttack < 0.0f) && (dsq > SQ(npc->cfg_npc->rad_detect)))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    switch (sleepattack)
    {
    case SLEEP_ATAK_REACT:
        sleepattack = SLEEP_ATAK_ZAP;
        nextgoal = NPC_GOAL_NOTICE;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case SLEEP_ATAK_ZAP:
        if (subenter)
        {
            tmr_minAttack = 0.3f;
        }
        zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
        npc->SndPlayRandom(NPC_STYP_ATTACK);
        npc->FacePlayer(dt, 3.0f * PI);
        tmr_minAttack = MAX(-1.0f, (tmr_minAttack - dt));
        zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
        if ((tmr_minAttack < 0.0f) && !(npc->AnimTimeRemain(NULL) > dt) &&
            (globals.player.Health == 0))
        {
            sleepattack = SLEEP_ATAK_LAUGH;
            flg_attack |= 3;
            if (globals.player.Health < 1)
            {
                sleepattack = SLEEP_ATAK_LAUGH;
            }
        }
        break;
    case SLEEP_ATAK_LAUGH:
        sleepattack = SLEEP_ATAK_REACT;
        nextgoal = NPC_GOAL_TAUNT;
        *trantype = GOAL_TRAN_PUSH;
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        break;
    }

    if (sleepattack != old_sleepattack)
    {
        flg_info |= 2;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

S32 zNPCGoalAlertSleepy::NPCMessage(NPCMsg* mail)
{
    S32 snarfed = 1;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xPsyche* psy = xGoal::GetPsyche();

    switch (mail->msgid)
    {
    case NPC_MID_DAMAGE:
        if ((npc->hitpoints > 1) && (mail->infotype == NPC_MDAT_DAMAGE) &&
            (mail->dmgdata.dmg_type == DMGTYP_SIDE || (mail->dmgdata.dmg_type == DMGTYP_HITBYTOSS)))
        {
            this->sleepattack = SLEEP_ATAK_REACT;
            this->flg_info |= 2;
            psy->GoalPush(NPC_GOAL_WOUND, 0);
        }
        else
        {
            snarfed = 0;
        }
        break;
    default:
        snarfed = 0;
        break;
    }

    return snarfed;
}

S32 zNPCGoalAlertArf::Enter(F32 dt, void* updCtxt)
{
    alertarf = ARF_ALERT_READY;
    tmr_reload = -1.0f;
    flg_user = 1;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertArf::Exit(F32 dt, void* updCtxt)
{
    zNPCArfArf* npc = ((zNPCArfArf*)(psyche->clt_owner));
    flg_info = 0;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalAlertArf::Resume(F32 dt, void* updCtxt)
{
    zNPCArfArf* npc = ((zNPCArfArf*)(psyche->clt_owner));
    flg_info |= 2;
    flg_user = 1;
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertArf::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCArfArf* npc = (zNPCArfArf*)(psyche->clt_owner);
    xVec3 dir_HtoP;
    zNPCGoalTaunt* taunt;
    en_alertarf old_alertarf;
    en_arfdoes rc;

    F32 dsq = npc->arena.DstSqFromHome(xEntGetPos(&globals.player.ent), &dir_HtoP);
    F32 rad = MAX(npc->arena.Radius(1.0f) * 1.5f, npc->cfg_npc->rad_attack);

    if (globals.player.Health < 1)
    {
        taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->npcset.allowDetect)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (dsq > SQ(rad))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    if (flg_user != 0)
    {
        DoAutoAnim(NPC_GSPOT_START, 0);
        flg_user = 0;
    }

    old_alertarf = alertarf;
    flg_info &= ~6;

    npc->VelStop();
    npc->FacePlayer(dt, 3.0f * PI);

    switch (alertarf)
    {
    case ARF_ALERT_REACT:
        alertarf = ARF_ALERT_READY;
        nextgoal = NPC_GOAL_NOTICE;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case ARF_ALERT_READY:
        if (((tmr_reload < 0.0f) ? 1 : 0) && !(globals.player.DamageTimer > 0.0f))
        {
            tmr_reload = 0.5f + (0.5f * (0.25f * (xurand() - 0.5f)));
            rc = DecideAttack();
            if (rc == ARF_DOES_MELEE)
            {
                *trantype = GOAL_TRAN_PUSH;
                nextgoal = NPC_GOAL_ATTACKARFMELEE;
            }
            else if (rc == ARF_DOES_LOB)
            {
                *trantype = GOAL_TRAN_PUSH;
                nextgoal = NPC_GOAL_ATTACKARF;
            }
        }
        else
        {
            tmr_reload = MAX(-1.0f, (tmr_reload - dt));
        }
        break;
    case ARF_ALERT_TELEPORT:
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TELEPORT;
        alertarf = ARF_ALERT_READY;
        break;
    }

    if (alertarf != old_alertarf)
    {
        flg_info |= 2;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

S32 zNPCGoalAlertArf::NPCMessage(NPCMsg* mail)
{
    zNPCGoalRespawn* respgoal;
    S32 snarfed = 1;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xPsyche* psy = GetPsyche();

    switch (mail->msgid)
    {
    case NPC_MID_DAMAGE:
        if (npc->hitpoints <= 1)
        {
            snarfed = 0;
            break;
        }

        if (((mail->infotype != NPC_MDAT_DAMAGE)) || (psy->GIDInStack(NPC_GOAL_WOUND) != NULL) ||
            (psy->GIDOfPending() == NPC_GOAL_WOUND))
        {
            break;
        }

        alertarf = (en_alertarf)2;
        flg_info |= 2;

        if (psy->GIDOfActive() != 0x4E47523E)
        {
            psy->GoalSwap(0x4E475269, 0);
        }
        else
        {
            psy->GoalPush(0x4E475269, 0);
        }
        break;
    default:
        snarfed = 0;
        break;
    }
    return snarfed;
}

en_arfdoes zNPCGoalAlertArf::DecideAttack()
{
    en_arfdoes do_attack = ARF_DOES_NOT;
    zNPCArfArf* npc = ((zNPCArfArf*)(psyche->clt_owner));

    if (npc->XYZDstSqToPlayer(NULL) < SQ(3.0f))
    {
        do_attack = ARF_DOES_MELEE;
    }
    else
    {
        zNPCGoalAttackArf* atak = (zNPCGoalAttackArf*)(psyche->FindGoal(NPC_GOAL_ATTACKARF));
        if (npc->AdoptADoggie() != NULL)
        {
            do_attack = ARF_DOES_LOB;
            atak->SetAttackMode(1, 0);
        }
    }

    return do_attack;
}

S32 zNPCGoalAlertPuppy::Enter(F32 dt, void* updCtxt)
{
    alertpup = PUPPY_ALERT_YAPPY;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertPuppy::Resume(F32 dt, void* updCtxt)
{
    zNPCChomper* npc = ((zNPCChomper*)(psyche->clt_owner));
    flg_info |= 2;
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertPuppy::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    en_alertpuppy old_alertpup;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    old_alertpup = alertpup;
    flg_info &= 0xFFFFFFF9;
    switch (alertpup)
    {
    case PUPPY_ALERT_YAPPY:
        alertpup = PUPPY_ALERT_CHASE;
        nextgoal = NPC_GOAL_DOGBARK;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case PUPPY_ALERT_CHASE:
        alertpup = PUPPY_ALERT_ATTAAAAACK;
        nextgoal = NPC_GOAL_DOGDASH;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case PUPPY_ALERT_ATTAAAAACK:
        alertpup = PUPPY_ALERT_DISAPPEAR;
        nextgoal = NPC_GOAL_DOGPOUNCE;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case PUPPY_ALERT_DISAPPEAR:
        zNPCGoalAfterlife* wanna = (zNPCGoalAfterlife*)(psyche->FindGoal(NPC_GOAL_AFTERLIFE));
        wanna->DieWithAWhimper();
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_AFTERLIFE;
        break;
    }
    if (alertpup != old_alertpup)
    {
        flg_info |= 2;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalAlertChuck::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    alertchuk = CHUCK_ALERT_NOTICE;
    npc->VelStop();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertChuck::Resume(F32 dt, void* updCtxt)
{
    zNPCChuck* npc = ((zNPCChuck*)(psyche->clt_owner));
    flg_info |= 2;
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertChuck::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 dir_HtoP;
    F32 tym_reload;
    en_alertchuk old_alertchuk;
    S32 subenter;
    xVec3 vec_home;
    F32 dst_edge;
    xVec3 dir_plyr;
    F32 dst_farside;
    xVec3 dir_zoomer;
    xVec3 pos_far;
    xVec3 pos_bak;

    F32 dsq = npc->arena.DstSqFromHome(xEntGetPos(&globals.player.ent), &dir_HtoP);
    F32 rad = MAX(npc->arena.Radius(1.0f) * 1.5f, npc->cfg_npc->rad_attack);

    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->npcset.allowDetect)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (dsq > SQ(rad))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    tym_reload = 4.0f;
    if (zGameExtras_CheatFlags() & 0x800)
    {
        tym_reload = 2.0f;
    }

    old_alertchuk = alertchuk;
    subenter = flg_info & 2;
    flg_info &= ~6;

    switch (alertchuk)
    {
    case CHUCK_ALERT_NOTICE:
        alertchuk = CHUCK_ALERT_BEGIN;
        nextgoal = NPC_GOAL_NOTICE;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case CHUCK_ALERT_ARENA:
        GetInArena(dt);
        if (npc->arena.PctFromHome(npc->Pos()) < 0.5f)
        {
            alertchuk = CHUCK_ALERT_BEGIN;
            if (subenter)
            {
                DoAutoAnim(NPC_GSPOT_STARTALT, 0);
            }
        }
        break;
    case CHUCK_ALERT_BEGIN:
        alertchuk = CHUCK_ALERT_READY;
        npc->FacePlayer(dt, 3.0f * PI);
        npc->VelStop();
        tmr_hover = 0.0f;
        break;
    case CHUCK_ALERT_READY:
        if (subenter)
        {
            DoAutoAnim(NPC_GSPOT_RESUME, 0);
        }
        npc->FacePlayer(dt, 3.0f * PI);
        if (((tmr_reload < 0.0f) ? 1 : 0) && !(globals.player.DamageTimer > 0.0f))
        {
            alertchuk = CHUCK_ALERT_DIDTHROW;
            nextgoal = NPC_GOAL_ATTACKCHUCK;
            *trantype = GOAL_TRAN_PUSH;
        }
        else
        {
            tmr_reload = MAX(-1.0f, (tmr_reload - dt));
            if (npc->npcset.allowChase)
            {
                F32 dst_home = xsqrt(npc->XZDstSqToPos(npc->arena.Pos(), &vec_home, NULL));
                dst_edge = npc->arena.Radius(1.0f) - dst_home;

                if (dst_edge < 0.0f)
                {
                    npc->XZVecToPos(&dir_zoom, npc->arena.Pos(), NULL);
                    dst_zoom = xVec3Normalize(&dir_zoom, &dir_zoom);
                    dst_zoom += 0.5f * npc->arena.Radius(1.0f);
                    alertchuk = CHUCK_ALERT_ZOOMPAST;
                }
                else
                {
                    F32 ds2_plyr = npc->XZDstSqToPlayer(&dir_plyr, NULL);

                    if (!(ds2_plyr > SQ(MAX(5.0f, 0.25f * npc->arena.Radius(1.0f)))))
                    {
                        dst_farside = 0.8f * npc->arena.Radius(1.0f) + dst_home;

                        if (!(dst_edge < 8.0f) || !(dst_farside < 8.0f))
                        {
                            if (dst_home > 0.001f)
                            {
                                xVec3SMul(&dir_zoomer, &vec_home, 1.0f / dst_home);
                            }
                            else
                            {
                                F32 sidely = (xrand() & 0x800000) ? 1.0f : -1.0f;
                                xVec3SMul(&dir_zoomer, NPCC_rightDir(npc), sidely);
                            }

                            xVec3SMul(&pos_far, &dir_zoomer, dst_farside);
                            xVec3AddTo(&pos_far, npc->arena.Pos());
                            xVec3SMul(&pos_bak, &dir_zoomer, -10.0f * dst_edge);
                            xVec3AddTo(&pos_bak, npc->arena.Pos());

                            F32 ds2_PtoFar = NPCC_DstSqPlyrToPos(&pos_far);
                            F32 ds2_PtoBak = NPCC_DstSqPlyrToPos(&pos_bak);

                            if (ds2_PtoFar > ds2_PtoBak)
                            {
                                xVec3Copy(&dir_zoom, &dir_zoomer);
                                dst_zoom = dst_farside;
                                alertchuk = CHUCK_ALERT_ZOOMPAST;
                            }
                            else
                            {
                                xVec3SMul(&dir_zoom, &dir_zoomer, -1.0f);
                                dst_zoom = dst_edge;
                                alertchuk = CHUCK_ALERT_BACKAWAY;
                            }
                        }
                    }
                }
            }
        }
        break;
    case CHUCK_ALERT_BACKAWAY:
        npc->FacePlayer(dt, 3.0f * PI);
        if (ZoomMove(dt))
        {
            alertchuk = CHUCK_ALERT_READY;
            tmr_reload = -1.0f;
            tmr_hover = 0.0f;
        }
        break;
    case CHUCK_ALERT_ZOOMPAST:
        npc->FacePlayer(dt, 3.0f * PI);
        if (ZoomMove(dt))
        {
            alertchuk = CHUCK_ALERT_READY;
            tmr_reload = -1.0f;
            tmr_hover = 0.0f;
        }
        break;
    case CHUCK_ALERT_DIDTHROW:
        npc->FacePlayer(dt, 3.0f * PI);
        npc->VelStop();
        tmr_reload = tym_reload + (tym_reload * (0.25f * (xurand() - 0.5f)));
        tmr_hover = 0.0f;
        alertchuk = CHUCK_ALERT_READY;
        break;
    }

    if (alertchuk != old_alertchuk)
    {
        flg_info |= 2;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

void zNPCGoalAlertChuck::GetInArena(F32 dt)
{
    xVec3 vec1;
    xVec3 dir_want;
    xVec3 dir;

    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3Sub(&vec1, npc->arena.Pos(), npc->zNPCCommon::Pos());

    F32 rot = xVec3Length(&vec1);

    if (rot < 1.0f)
    {
        xVec3Copy(&vec1, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&vec1, 1.0f / rot);
    }

    xVec3Copy(&dir_want, &vec1);

    npc->ThrottleAdjust(dt, 4.0f, -1.0f);
    rot = npc->TurnToFace(dt, &dir_want, -1.0f);
    NPCC_ang_toXZDir(npc->frame->rot.angle + rot, &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

S32 zNPCGoalAlertChuck::ZoomMove(F32 dt)
{
    S32 donemoving = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    F32 dist;
    xVec3 dir;
    if (dst_zoom < 0.5f)
    {
        npc->ThrottleAdjust(dt, 0.5f, -1.0f);
    }
    else
    {
        npc->ThrottleAdjust(dt, 6.0f, -1.0f);
    }
    npc->XYZVecToPos(&dir, npc->arena.Pos());
    dir.x = dir_zoom.x;
    dir.z = dir_zoom.z;
    dist = xVec3Length(&dir);
    if (dist > dst_zoom)
    {
        dst_zoom = -1.0f;
    }
    else if (dist < 1e-5f)
    {
        dst_zoom = -1.0f;
    }
    else
    {
        xVec3SMulBy(&dir, (1.0f / dist));
        npc->ThrottleApply(dt, &dir, 0);
        dst_zoom =
            -((dt * npc->spd_throttle) * ((F32)__fabs(dir.x) + (F32)__fabs(dir.z)) - dst_zoom);
    }
    if (dst_zoom < 0.0f)
    {
        donemoving = 1;
    }
    return donemoving;
}

S32 zNPCGoalAlertTubelet::Enter(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = ((zNPCTubeSlave*)(psyche->clt_owner));
    npc->tubespot = ROBO_TUBE_MARY;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertTubelet::Exit(F32 dt, void* updCtxt)
{
    zNPCTubelet* npc = ((zNPCTubelet*)(psyche->clt_owner));
    zNPC_SNDStop(eNPCSnd_TubeAttack);
    npc->pete_attack_last = 0;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalAlertTubelet::Resume(F32 dt, void* updCtxt)
{
    zNPCTubelet* npc = ((zNPCTubelet*)(psyche->clt_owner));
    npc->tubestat = TUBE_STAT_ATTACK;
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertTubelet::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCTubelet* npc = (zNPCTubelet*)(psyche->clt_owner);
    S32 atHome;

    ChkPrelimTran(trantype, &nextgoal);

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    npc->Chk_IsBonked();

    if (npc->hitpoints > 0)
    {
        npc->ModelAtomicShow(0, NULL);
        npc->ModelAtomicHide(1, NULL);
        npc->ModelAtomicHide(4, NULL);
    }

    atHome = MoveToHome(dt);

    if (npc->bonkSpinRate > 0.0f)
    {
        npc->frame->drot.angle += dt * -npc->bonkSpinRate;
        npc->frame->mode |= 0x20;
        npc->bonkSpinRate -= 2.0f * PI * dt;
    }
    else if (npc->hitpoints < 1)
    {
        npc->frame->drot.angle *= 0.97f;
        npc->frame->mode |= 0x20;
    }

    if ((npc->hitpoints > 0) && (PeteAttackParSys(dt, atHome), atHome != 0) &&
        (atHome != npc->pete_attack_last))
    {
        PeteAttackBegin();
    }

    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

void zNPCGoalAlertTubelet::ChkPrelimTran(en_trantype* trantype, S32* nextgoal)
{
    zNPCTubelet* npc = (zNPCTubelet*)(psyche->clt_owner);

    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = ((zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT)));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        *nextgoal = NPC_GOAL_TAUNT;
        npc->tubestat = TUBE_STAT_DUCKLING;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        *nextgoal = NPC_GOAL_IDLE;
        npc->tubestat = TUBE_STAT_DUCKLING;
    }
    else if (npc->arena.IncludesNPC(npc, 0, 0) == 0)
    {
        *trantype = GOAL_TRAN_SET;
        *nextgoal = NPC_GOAL_IDLE;
        npc->tubestat = TUBE_STAT_DUCKLING;
    }
    else if ((psyche->TimerGet(XPSY_TYMR_CURGOAL) > 0.25f) &&
             (npc->arena.IncludesPlayer(0, 0) == 0))
    {
        *trantype = GOAL_TRAN_SET;
        *nextgoal = NPC_GOAL_IDLE;
        npc->tubestat = TUBE_STAT_DUCKLING;
    }
}

S32 zNPCGoalAlertTubelet::MoveToHome(F32 dt)
{
    S32 arrived;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    NPCArena* arena = &npc->arena;
    F32 rad;
    F32 dst_surplus;
    xVec3 dir_home;

    rad = arena->Radius(1.0f) - 15.0f;
    if (rad < 0.0f)
    {
        rad = 0.0f;
    }

    dst_surplus = xsqrt(npc->XZDstSqToPos(arena->Pos(), &dir_home, 0));
    if (dst_surplus < (rad + 1.0f))
    {
        arrived = 1;
        npc->VelStop();
    }
    else
    {
        if (dst_surplus > (rad + 2.0f))
        {
            npc->ThrottleAdjust(dt, 7.2f, 5.5f);
        }
        else
        {
            npc->ThrottleAdjust(dt, 0.25f, 0.5f);
        }
        dir_home *= (1.0f / dst_surplus);
        npc->ThrottleApply(dt, &dir_home, 0);
        arrived = 0;
    }
    return arrived;
}

void zNPCGoalAlertTubelet::PeteAttackBegin()
{
    zNPCTubelet* npc = (zNPCTubelet*)(psyche->clt_owner);
    npc->pete_attack_last = 1;
    zNPC_SNDPlay3D(eNPCSnd_TubeAttack, npc);
}

void zNPCGoalAlertTubelet::PeteAttackParSys(F32 dt, S32 param_2)
{
    xEntFrame* iVar1;

    zNPCTubelet* iVar2 = (zNPCTubelet*)(psyche->clt_owner);
    iVar1 = (iVar2->frame);
    F32 dVar3 = (iVar1->drot.angle);
    if ((F32)__fabs(dVar3) > 0.09599312f)
    {
        iVar1->drot.angle *= 0.8f;
        iVar2->frame->mode |= 0x20;
    }
    else
    {
        if ((F32)__fabs(dVar3) < 0.08726647f)
        {
            iVar1->drot.angle = -(dt * 0.0872664675116539f - dVar3);
            iVar2->frame->mode |= 0x20;
        }
        else
        {
            iVar1->mode |= 0x20;
            if (param_2 != 0)
            {
                EmitSteam(dt);
            }
        }
    }
}

void zNPCGoalAlertTubelet::EmitSteam(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3 pos_emit;

    if (!npc->GetVertPos(NPC_MDLVERT_ATTACK, &pos_emit))
    {
        xVec3Copy(&pos_emit, xEntGetCenter(npc));
    }

    xVec3 pos_end;
    xVec3SMul(&pos_end, NPCC_faceDir(npc), 10.0f);
    xVec3AddTo(&pos_end, &pos_emit);

    xVec3 dir_steam;
    xVec3Sub(&dir_steam, &pos_end, &pos_emit);
    xVec3Normalize(&dir_steam, &dir_steam);

    if (--cnt_nextlos < 0)
    {
        memset(&g_SharedCollisRecord, 0, sizeof(g_SharedCollisRecord));
        g_SharedCollisRecord.flags = k_HIT_0xF00 | k_HIT_CALC_HDNG;

        xCollis* colrec = &g_SharedCollisRecord;

        S32 rc = npc->HaveLOSToPos(&pos_end, 10.0f, globals.sceneCur, NULL, colrec);

        if (!rc && (colrec->flags & k_HIT_IT))
        {
            len_laser = colrec->dist;
        }
        else
        {
            len_laser = 10.0f;
        }

        cnt_nextlos = (xrand() & 1) + 5;

        len_laser = MAX(0.25f, MIN(len_laser, 10.0f));
    }

    xVec3SMul(&pos_end, &dir_steam, len_laser);
    xVec3AddTo(&pos_end, &pos_emit);

    F32 tym_life = len_laser / 5.0f;

    xVec3 vel_emit;
    xVec3SMul(&vel_emit, &dir_steam, 5.0f);

    NPAR_EmitTubeSpiral(&pos_emit, &vel_emit, tym_life);
}

S32 zNPCGoalAlertSlick::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    alertslik = SLICK_ALERT_BEGIN;
    tmr_reload = -1.0f;
    npc->VelStop();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalAlertSlick::Resume(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->VelStop();
    flg_info |= 2;
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalAlertSlick::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal;
    zNPCSlick* npc;
    en_alertslik old_alertslik;
    F32 tym_reload;
    xVec3 dir_HtoP;
    F32 dsq;
    S32 subenter;
    F32 rad;
    npc = (zNPCSlick*)(psyche->clt_owner);
    subenter = flg_info & 2;
    old_alertslik = alertslik;
    flg_info &= ~6;
    nextgoal = 0;
    tym_reload = 5.0f;
    if (zGameExtras_CheatFlags() & 0x800)
    {
        tym_reload = 3.0f;
    }
    dsq = npc->arena.DstSqFromHome(xEntGetPos(&globals.player.ent), &dir_HtoP);
    if ((npc->arena.Radius(1.0f) * 1.5f) > npc->cfg_npc->rad_attack)
    {
        rad = npc->arena.Radius(1.0f) * 1.5f;
    }
    else
    {
        rad = npc->cfg_npc->rad_attack;
    }
    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (!npc->npcset.allowDetect)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (dsq > SQ(rad))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    if (alertslik != SLICK_ALERT_ARENA && (npc->arena.PctFromHome(npc->Pos()) > 1.0f))
    {
        alertslik = SLICK_ALERT_ARENA;
        DoAutoAnim(NPC_GSPOT_STARTALT, 0);
    }
    switch (alertslik)
    {
    case SLICK_ALERT_NOTICE:
        alertslik = SLICK_ALERT_BEGIN;
        nextgoal = NPC_GOAL_NOTICE;
        *trantype = GOAL_TRAN_PUSH;
        break;
    case SLICK_ALERT_ARENA:
        GetInArena(dt);
        if (npc->arena.PctFromHome(npc->Pos()) < 0.5f)
        {
            alertslik = SLICK_ALERT_BEGIN;
        }
        break;
    case SLICK_ALERT_BEGIN:
        alertslik = SLICK_ALERT_READY;
        DoAutoAnim(NPC_GSPOT_RESUME, 0);
        npc->FacePlayer(dt, 3 * PI);
        npc->VelStop();
        break;
    case SLICK_ALERT_READY:
        if (((tmr_reload < 0.0f) ? 1 : 0) && !(globals.player.DamageTimer > 0.0f))
        {
            tmr_reload = tym_reload + (tym_reload * (0.25f * (xurand() - 0.5f)));
            nextgoal = NPC_GOAL_ATTACKSLICK;
            *trantype = GOAL_TRAN_PUSH;
        }
        else
        {
            tmr_reload = MAX(-1.0f, (tmr_reload - dt));
            if (subenter)
            {
                DoAutoAnim(NPC_GSPOT_RESUME, 0);
                npc->VelStop();
                if (!npc->arena.IncludesPos(&pos_corner, 0, 0))
                {
                    xVec3Copy(&pos_corner, npc->Pos());
                }
            }
            npc->FacePlayer(dt, 3 * PI);
            MoveCorner(dt);
        }
        break;
    }
    if (alertslik != old_alertslik)
    {
        flg_info |= 2;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

S32 zNPCGoalAlertSlick::NPCMessage(NPCMsg* mail)
{
    zNPCGoalRespawn* respgoal;
    S32 snarfed;

    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    snarfed = 1;
    xPsyche* psy = GetPsyche();
    switch (mail->msgid)
    {
    case NPC_MID_DAMAGE:
        if ((psy->GIDInStack(NPC_GOAL_WOUND) != NULL) || (psy->GIDOfPending() == NPC_GOAL_WOUND))
        {
            break;
        }

        if ((npc->hitpoints > 1) && (mail->infotype == NPC_MDAT_DAMAGE))
        {
            if ((mail->dmgdata.dmg_type == DMGTYP_SIDE) ||
                (mail->dmgdata.dmg_type == DMGTYP_HITBYTOSS))
            {
                alertslik = SLICK_ALERT_READY;
                flg_info |= 2;
                psy->GoalPush(NPC_GOAL_WOUND, 0);
                break;
            }
        }

        snarfed = 0;
        break;
    default:
        snarfed = 0;
        break;
    }
    return snarfed;
}

void zNPCGoalAlertSlick::GetInArena(F32 dt)
{
    xVec3 vec1;
    xVec3 dir_want;
    xVec3 dir;

    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3Sub(&vec1, npc->arena.Pos(), npc->zNPCCommon::Pos());

    F32 rot = xVec3Length(&vec1);

    if (rot < 1.0f)
    {
        xVec3Copy(&vec1, NPCC_rightDir(npc));
    }
    else
    {
        xVec3SMulBy(&vec1, 1.0f / rot);
    }

    xVec3Copy(&dir_want, &vec1);

    npc->ThrottleAdjust(dt, 2.0f, -1.0f);
    rot = npc->TurnToFace(dt, &dir_want, -1.0f);
    NPCC_ang_toXZDir(npc->frame->rot.angle + rot, &dir);
    npc->ThrottleApply(dt, &dir, 0);
}

void zNPCGoalAlertSlick::MoveCorner(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    F32 ds2_corn;
    xVec3 dir_corn;
    if (npc->npcset.allowChase && npc->arena.IsReady() && (npc->arena.Radius(1.0f) > 2.0f))
    {
        ds2_corn = npc->XYZDstSqToPos(&pos_corner, 0);
        if (ds2_corn < SQ(0.5f))
        {
            npc->CornerOfArena(&pos_corner, -1.0f);
            ds2_corn = npc->XYZDstSqToPos(&pos_corner, NULL);
        }
        if (npc->DBG_IsNormLog(eNPCDCAT_Ten, 2))
        {
            xDrawSetColor(g_BLUE);
            xDrawSphere2(&pos_corner, 0.1f, 12);
        }
        if ((tmr_reload >= 0.0f) && (tmr_reload < 1.0f))
        {
            npc->ThrottleAdjust(dt, 1.5, -1.0);
        }
        else if (ds2_corn < 2.0f)
        {
            npc->ThrottleAdjust(dt, 1.5f, -1.0f);
        }
        else
        {
            npc->ThrottleAdjust(dt, 3.5f, -1.0f);
        }
        npc->XYZVecToPos(&dir_corn, &pos_corner);
        xVec3Normalize(&dir_corn, &dir_corn);
        npc->ThrottleApply(dt, &dir_corn, 0);
    }
}

S32 zNPCGoalChase::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    xVec3 dir_dest = { 0.0f, 0.0f, 0.0f };

    xVec3Sub(&dir_dest, npc->Pos(), npc->arena.Pos());

    if (npc->flg_move & 2)
    {
        dir_dest.y = 0.0f;
    }

    F32 dst_home = xVec3Length2(&dir_dest);

    if (flg_chase & 1)
    {
        if (dst_home > 3.0f)
        {
            flg_chase &= ~1;
        }

        dir_dest *= -1.0f;
    }
    else
    {
        if (dst_home < 1.0f)
        {
            flg_chase |= 1;
        }
    }

    npc->TurnToFace(dt, &dir_dest, -1.0f);
    npc->ThrottleAdjust(dt, 2.0f * npc->cfg_npc->spd_moveMax, -1.0f);
    npc->ThrottleApply(dt, &dir_dest, 0);

    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, 2))
    {
        if ((S32)(psyche->TimerGet(XPSY_TYMR_CURGOAL) * 5.0f) & 1)
        {
            xDrawSetColor(g_RED);
            xDrawLine(xEntGetCenter(npc), xEntGetPos(&globals.player.ent));
        }
    }

    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

S32 zNPCGoalAttackCQC::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = (zNPCCommon*)this->psyche->clt_owner;
    flg_attack = 0;
    return this->zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackCQC::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCCommon* npc = (zNPCCommon*)(psyche->clt_owner);

    npc->ThrottleAdjust(dt, 0.0f, 10.0f);
    npc->ThrottleApply(dt, NPCC_faceDir(npc), 0);

    U32 aid_punch = g_hash_roboanim[14];

    U32 aid_now = npc->AnimCurState()->ID;

    if (aid_now == aid_punch && npc->IsAttackFrame(-1.0f, 0) == 1 && !(flg_attack & 3))
    {
        xBound bnd;

        memset(&bnd, 0, sizeof(xBound));
        bnd.type = 1;
        bnd.sph.r = npc->cfg_npc->rad_dmgSize;

        if (npc->GetVertPos(NPC_MDLVERT_ATTACK, &bnd.sph.center))
        {
            if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, 2) != 0)
            {
                xDrawSetColor(g_NEON_RED);
                xBoundDraw(&bnd);
            }
            if (NPCC_chk_hitPlyr(&bnd, NULL))
            {
                zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
                flg_attack |= 3;
            }
        }
    }

    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

S32 zNPCGoalAttackFodder::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    this->haz_cattle = HAZ_Acquire();

    if (this->haz_cattle)
    {
        if (this->haz_cattle->ConfigHelper(NPC_HAZ_CATTLEPROD))
        {
            this->cbNotify.goal = this;
            this->haz_cattle->SetNPCOwner(npc);
            this->haz_cattle->NotifyCBSet(&this->cbNotify);
            this->haz_cattle->Start(NULL, -1.0f);
        }
        else
        {
            this->haz_cattle->Discard();
            this->haz_cattle = NULL;
        }
    }

    npc->VelStop();

    return this->zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackFodder::Exit(F32 dt, void* updCtxt)
{
    if (this->haz_cattle)
    {
        this->haz_cattle->Discard();
    }

    this->haz_cattle = NULL;
    return this->zNPCGoalPushAnim::Exit(dt, updCtxt);
}

S32 zNPCGoalAttackFodder::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene)
{
    if (!this->haz_cattle)
    {
        *trantype = GOAL_TRAN_POP;
        return 1;
    }
    else
    {
        this->SyncCattleProd();
    }

    return this->zNPCGoalPushAnim::Process(trantype, dt, updCtxt, scene);
}

S32 zNPCGoalAttackFodder::CattleNotify::Notify(en_haznote hazNote, NPCHazard* haz)
{
    switch (hazNote)
    {
    case HAZ_NOTE_DISCARD:
    case HAZ_NOTE_ABORT:
        goal->haz_cattle = NULL;
        break;
    case HAZ_NOTE_HITPLAYER:
        goal->flg_attack |= 3;
        break;
    }
    return 0;
}

S32 zNPCGoalAttackFodder::SyncCattleProd()
{
    xVec3 vec1;
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    S32 var1 = this->flg_attack & 0x3;

    if (!this->haz_cattle)
    {
        return var1;
    }

    if (this->haz_cattle->tmr_remain < 0.35f)
    {
        this->haz_cattle->tym_lifespan = npc->AnimDuration(NULL);
        this->haz_cattle->tmr_remain = npc->AnimTimeRemain(NULL);
    }

    if (!npc->GetVertPos(NPC_MDLVERT_ATTACK, &vec1))
    {
        return this->flg_attack & 0x3;
    }

    this->haz_cattle->PosSet(&vec1);

    if (this->haz_cattle->flg_hazard & 0x40000000)
    {
        this->flg_attack |= 0x3;
    }

    return this->flg_attack & 0x3;
}

S32 zNPCGoalAttackChomper::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->VelStop();
    npc->SndPlayRandom(NPC_STYP_ATTACK);

    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackChomper::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene)
{
    static const F32 tym_ofAttack[2] = { 0.5f, 0.83f };
    F32 tym = ((zNPCRobot*)(psyche->clt_owner))->AnimTimeCurrent();
    if ((tym > tym_ofAttack[0]) && (tym < tym_ofAttack[1]))
    {
        this->BreathAttack();
    }
    return zNPCGoalPushAnim::Process(trantype, dt, updCtxt, scene);
}

void zNPCGoalAttackChomper::BreathAttack()
{
    static const xVec3 vec_boneOffset = { 0.0f, -0.75f, 0.75f };
    static const xVec3 vec_coneWeights = { 0.5f, 0.35f, 1.0f };

    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3 pos_emit = *(const xVec3*)npc->BonePos(4);
    pos_emit += vec_boneOffset;
    xMat3x3RMulVec(&pos_emit, (const xMat3x3*)npc->BoneMat(0), &pos_emit);
    pos_emit += *(const xVec3*)npc->BonePos(0);

    S32 i;
    xVec3 vel_emit;
    F32 wgt_up;
    F32 wgt_right;

    wgt_right = vec_coneWeights.x;
    wgt_up = vec_coneWeights.y;

    for (i = 0; i < 8; i++)
    {
        vel_emit = *(const xVec3*)NPCC_faceDir(npc) * vec_coneWeights.z;
        vel_emit += *(const xVec3*)NPCC_rightDir(npc) * (wgt_right * (2.0f * (xurand() - 0.5f)));
        vel_emit += *(const xVec3*)NPCC_upDir(npc) * (wgt_up * (2.0f * (xurand() - 0.5f)));
        vel_emit.normalize();
        vel_emit *= 12.0f * xurand() + 5.0f;

        NPAR_EmitDoggyAttack(&pos_emit, &vel_emit);
    }
}

S32 zNPCGoalAttackHammer::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    flg_attack = 0;
    FXStreakPrep();

    npc->GetVertPos(NPC_MDLVERT_ATTACK, &pos_lastVert);
    pos_oldVert = pos_lastVert;

    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackHammer::Exit(F32 dt, void* updCtxt)
{
    FXStreakDone();
    return this->zNPCGoalPushAnim::Exit(dt, updCtxt);
}

S32 zNPCGoalAttackHammer::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    static const F32 tym_animDamage[2] = { 0.5f, 1.8f };
    static const F32 tym_streakBetween[2] = { 0.75f, 1.6f };

    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 pos_vert;

    ChkPrelimTran(trantype, &nextgoal);

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    ModifyAnimSpeed();
    npc->ThrottleAdjust(dt, 0.0f, 10.0f);
    npc->ThrottleApply(dt, NPCC_faceDir(npc), 0);

    pos_vert = *(const xVec3*)npc->BonePos(24);
    xMat3x3RMulVec(&pos_vert, (const xMat3x3*)npc->BoneMat(0), &pos_vert);
    pos_vert += *(const xVec3*)npc->BonePos(0);

    F32 tym_animCurr = npc->AnimTimeCurrent();
    S32 doPLYRTests = (tym_animCurr > 1.0f);
    S32 doCHCKTests = (tym_animCurr > 1.15f);
    S32 zapidx =
        (bool)((tym_animCurr > tym_animDamage[0]) && (tym_animCurr < tym_animDamage[1]));

    if (doPLYRTests && zapidx && !(flg_attack & 3))
    {
        S32 rc = PlayerTests(&pos_vert, dt);

        if (rc)
        {
            flg_attack |= 3;
        }
    }

    if (doCHCKTests && zapidx && !(flg_attack & 4))
    {
        S32 rc = ShockwaveTests(&pos_vert, dt);

        if (rc)
        {
            flg_attack |= 3;
            TellBunnies();
        }
    }

    if ((tym_animCurr > tym_streakBetween[0]) && (tym_animCurr < tym_streakBetween[1]))
    {
        xVec3 diff = pos_vert - pos_lastVert;

        if (diff.length() > 0.25f)
        {
            xVec3 pos_fake =
                (pos_vert + *(const xVec3*)NPCC_faceDir(npc)) - (pos_lastVert - pos_oldVert);
            xVec3 pos_mid[4];
            xVec3* pos_midref[4] = {};
            xVec3* pos_ref[4] = {};
            S32 i;

            pos_midref[0] = &pos_mid[0];
            pos_midref[1] = &pos_mid[1];
            pos_midref[2] = &pos_mid[2];
            pos_midref[3] = &pos_mid[3];

            pos_ref[0] = &pos_oldVert;
            pos_ref[1] = &pos_lastVert;
            pos_ref[2] = &pos_vert;
            pos_ref[3] = &pos_fake;

            NPCC_GenSmooth(pos_ref, pos_midref);

            for (i = 0; i < 3; i++)
            {
                FXStreakUpdate(&pos_mid[i]);
            }
        }

        FXStreakUpdate(&pos_vert);
    }

    pos_oldVert = pos_lastVert;
    pos_lastVert = pos_vert;

    return zNPCGoalPushAnim::Process(trantype, dt, updCtxt, xscn);
}

void zNPCGoalAttackHammer::ChkPrelimTran(en_trantype* trantype, S32* nextgoal)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    F32 tym_rem = npc->AnimTimeRemain(0);

    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt =
            (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT)); // Not needed but cleaner.
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_SWAP;
        *nextgoal = NPC_GOAL_TAUNT;
    }
    else if ((tym_rem < 0.15f) && (globals.player.DamageTimer > 0.5f))
    {
        *trantype = GOAL_TRAN_SWAP;
        *nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        *nextgoal = NPC_GOAL_IDLE;
    }
}

S32 zNPCGoalAttackHammer::PlayerTests(xVec3* pos_vert, F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 hithim;

    xBound bnd;
    memset(&bnd, 0, sizeof(xBound));
    bnd.type = 1;
    bnd.sph.r = 0.55f;
    bnd.sph.center = *pos_vert;

    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, 2) != 0)
    {
        xDrawSetColor(g_NEON_RED);
        xBoundDraw(&bnd);
    }
    hithim = NPCC_chk_hitPlyr(&bnd, 0);
    if (hithim != 0)
    {
        if (zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos()) != 0)
        {
            npc->Vibrate(NPC_VIBE_HARD, -1.0f);
        }
    }
    return hithim;
}

S32 zNPCGoalAttackHammer::ShockwaveTests(xVec3* pos_vert, F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 hithim = 0;

    xBound bnd;
    memset(&bnd, 0, sizeof(xBound));
    bnd.type = 1;
    bnd.sph.r = 0.55f;
    bnd.sph.center = *pos_vert;

    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, 2) != 0)
    {
        xDrawSetColor(g_NEON_RED);
        xBoundDraw(&bnd);
    }

    xCollis* collist = g_SharedCollisRecordList;
    U8 numrec = 6;

    for (S32 i = 0; i < numrec; i++)
    {
        memset(&collist[i], 0, sizeof(xCollis));
        collist[i].flags = k_HIT_0xF00 | k_HIT_CALC_HDNG;
    }

    S32 num_hit =
        iSphereHitsEnv3(&bnd.sph, globals.sceneCur->env, collist, numrec, 0.78539819f);

    for (S32 i = 0; i < num_hit; i++)
    {
        xCollis* colrec = &collist[i];

        if (!(colrec->flags & k_HIT_IT))
        {
            continue;
        }

        xVec3 pos = bnd.sph.center + colrec->tohit;
        zFXHammer(&pos);

        flg_attack |= 4;
        if (flg_attack & 3)
        {
            break;
        }

        xVec3 diff;
        F32 ds2_plyr = NPCC_DstSq(xEntGetPos(&globals.player.ent), &bnd.sph.center, &diff);

        if ((ds2_plyr < SQ(2.25f)) && (xabs(diff.y) < 1.0f))
        {
            zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
            npc->Vibrate(NPC_VIBE_HARD, -1.0f);
            hithim = 1;
            flg_attack |= 8;
        }
        else if (!(flg_attack & 8) && (ds2_plyr < SQ(5.0f)))
        {
            npc->Vibrate(ds2_plyr, SQ(5.0f));
            flg_attack |= 8;
        }
    }

    return hithim;
}

void zNPCGoalAttackHammer::TellBunnies()
{
    static en_NPCTYPES totypes[4] = { NPC_TYPE_FODDER, NPC_TYPE_FODBOMB, NPC_TYPE_CHOMPER,
                                      NPC_TYPE_UNKNOWN };

    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));

    zNPCMsg_AreaNotify(npc, NPC_MID_BUNNYHOP, 8.0f, 18, totypes);
}

void zNPCGoalAttackHammer::ModifyAnimSpeed()
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    F32 speed = 1.5f;
    U32 cheats = zGameExtras_CheatFlags();
    if ((cheats & 0x800))
    {
        speed = 2.5f;
    }
    xAnimSingle* anim = npc->AnimCurSingle();
    anim->CurrentSpeed = speed;
}

void zNPCGoalAttackHammer::FXStreakPrep()
{
    streakID[0] = NPCC_StreakCreate(NPC_STRK_HAMMERSMASH_HORZ);
    streakID[1] = NPCC_StreakCreate(NPC_STRK_HAMMERSMASH_VERT);
}

void zNPCGoalAttackHammer::FXStreakDone()
{
    for (S32 i = 0; i < (S32)(sizeof(this->streakID) / sizeof(U32)); i++)
    {
        xFXStreakStop(streakID[i]);
        streakID[i] = 0xDEAD;
    }
}

void zNPCGoalAttackHammer::FXStreakUpdate(xVec3* pos_streak)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);

    xVec3 width = *(const xVec3*)NPCC_rightDir(npc) * 0.75f;
    xVec3 height;
    xVec3 a;
    xVec3 b;

    height = *(const xVec3*)NPCC_upDir(npc) + *(const xVec3*)NPCC_faceDir(npc);
    height.normalize();
    height *= 0.75f;

    a = *pos_streak - width;
    b = *pos_streak + width;
    xFXStreakUpdate(streakID[0], &a, &b);

    a = *pos_streak - height;
    b = *pos_streak + height;
    xFXStreakUpdate(streakID[1], &a, &b);
}

S32 zNPCGoalAttackTarTar::Enter(F32 dt, void* updCtxt)
{
    ((zNPCCommon*)(psyche->clt_owner))->VelStop();
    flg_pushanim |= 2;
    idx_launch = 1;
    flg_attack = 0;
    xVec3Copy(&pos_aimbase, &g_O3);
    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackTarTar::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 zapidx = -1;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    HAZ_AvailablePool();
    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, -1) != 0)
    {
        NPCC_DrawPlayerPredict(5, 1.0, 4.0);
        xDrawSetColor(g_YELLOW);
        xDrawSphere2(&pos_aimbase, 0.1, 0xc);
    }
    if (g_hash_roboanim[14] == npc->AnimCurStateID())
    {
        zapidx = npc->IsAttackFrame(-1.0f, 0);
        if (zapidx > idx_launch)
        {
            idx_launch = zapidx;
        }
        if (zapidx == idx_launch)
        {
            if (!(flg_attack & 1))
            {
                flg_attack |= 1;
                CacheAimPoint();
            }
            if (ShootBlob(dt, zapidx) != 0)
            {
                idx_launch++;
            }
        }
    }
    return zNPCGoalPushAnim::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalAttackTarTar::NPCMessage(NPCMsg* msg)
{
    S32 snarfed = 1;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xPsyche* psyche = (xPsyche*)xGoal::GetPsyche();
    switch (msg->msgid)
    {
    case NPC_MID_DAMAGE:
        if ((npc->hitpoints > 1) &&
            ((msg->infotype == NPC_MDAT_DAMAGE) && ((msg->dmgdata.dmg_type == DMGTYP_SIDE) ||
                                                    (msg->dmgdata.dmg_type == DMGTYP_HITBYTOSS))))
        {
            psyche->GoalSwap(NPC_GOAL_WOUND, 0);
        }
        else
        {
            snarfed = 0;
        }
        break;
    default:
        snarfed = 0;
        break;
    }
    return snarfed;
}

void zNPCGoalAttackTarTar::CacheAimPoint()
{
    F32 dist = xsqrt(((zNPCCommon*)(psyche->clt_owner))->XYZDstSqToPlayer(NULL));
    F32 tym = 8.0f;
    S32 cheats = zGameExtras_CheatFlags();
    if (cheats & 0x800)
    {
        tym = 22.5f;
    }
    F32 spd = 0.25f;
    if (tym > 0.25f)
    {
        spd = tym;
    }
    xVec3* pos = &pos_aimbase;
    if ((dist / spd) < 4.0f)
    {
        tym = (dist / spd);
    }
    else
    {
        tym = 4.0f;
    }
    zEntPlayer_PredictPos(pos, tym, 1.0f, 1);
}

S32 zNPCGoalAttackTarTar::ShootBlob(F32, S32 zapidx)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    NPCHazard* haz = HAZ_Acquire();

    if (haz == NULL)
    {
        return 0;
    }

    xVec3 pos_launch;
    S32 rc;
    F32 dst_miss;
    xVec3 dir_tgt;
    xVec3 pos_tgt;
    F32 dist;
    F32 spd_lob;

    rc = npc->GetVertPos(NPC_MDLVERT_ATTACK, &pos_launch);

    if (rc == 0)
    {
        xVec3SMul(&pos_launch, NPCC_faceDir(npc), 0.5f);
        pos_launch.y = 0.5f;
        xVec3AddTo(&pos_launch, xEntGetCenter(npc));
    }

    dst_miss = 2.0f;

    if (zapidx == 1)
    {
    }
    else if (zapidx == 3)
    {
        dst_miss *= -1.0f;
    }
    else
    {
        dst_miss = 0.0f;
    }

    dist = NPCC_aimMiss(&dir_tgt, &pos_launch, &pos_aimbase, dst_miss, &pos_tgt);

    if (dist < 0.001f || xVec3Dot(&dir_tgt, NPCC_faceDir(npc)) < 0.4f)
    {
        xVec3SMul(&pos_tgt, NPCC_faceDir(npc), 5.0f);
        xVec3AddTo(&pos_tgt, npc->Pos());
    }
    else if (dist < 5.0f)
    {
        xVec3SMul(&pos_tgt, &dir_tgt, 5.0f);
        xVec3AddTo(&pos_tgt, npc->Pos());
    }

    pos_tgt.y += 0.3f;

    haz->ConfigHelper(NPC_HAZ_TARTARPROJ);
    haz->SetNPCOwner(npc);

    spd_lob = 8.0f;

    if (zGameExtras_CheatFlags() & 0x800)
    {
        spd_lob = 22.5f;
    }

    xVec3Copy(&haz->custdata.tartar.pos_tgt, &pos_tgt);
    haz->Start(&pos_launch, MAX(1.0f, dist / spd_lob));

    return 1;
}

S32 zNPCGoalAttackMonsoon::Enter(F32 dt, void* updCtxt)
{
    idx_launch = 0;
    flg_pushanim |= 2;
    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackMonsoon::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);

    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_SWAP;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        *trantype = GOAL_TRAN_SWAP;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    npc->FacePlayer(dt, 3.0f * PI);
    npc->VelStop();

    if (!(flg_pushanim & 2) && (idx_launch == 0) && npc->IsAttackFrame(-1.0f, 0))
    {
        idx_launch += SpitCloud(dt);
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    return zNPCGoalPushAnim::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalAttackMonsoon::SpitCloud(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);

    if (!xEntIsVisible(npc) || globals.cmgr != NULL)
    {
        return 0;
    }

    NPCHazard* haz = HAZ_Acquire();
    if (haz == NULL)
    {
        return 0;
    }

    haz->ConfigHelper(NPC_HAZ_MONCLOUD);
    haz->SetNPCOwner(npc);

    xVec3 pos_emit;
    F32 off_side;

    if (xrand() & 0x800000)
    {
        off_side = 0.2f;
    }
    else
    {
        off_side = -0.2f;
    }

    xVec3Copy(&pos_emit, npc->Pos());
    pos_emit.y -= 1.0f;
    xVec3AddScaled(&pos_emit, NPCC_rightDir(npc), off_side);

    HAZCloud* cloud = &haz->custdata.cloud;
    NPCArena* arena = &npc->arena;

    if (arena->IsReady())
    {
        xVec3Copy(&cloud->pos_home, arena->Pos());
        cloud->rad_maxRange = arena->Radius(1.0f);
    }
    else
    {
        xVec3Copy(&cloud->pos_home, npc->Pos());
        cloud->rad_maxRange = npc->cfg_npc->rad_attack;
    }

    haz->Start(&pos_emit, -1.0f);

    return 1;
}

S32 zNPCGoalAttackArfMelee::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    FXStreakPrep();
    npc->SndPlayRandom(NPC_STYP_PUNCH);
    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackArfMelee::Exit(F32 dt, void* updCtxt)
{
    FXStreakDone();
    return zNPCGoalPushAnim::Exit(dt, updCtxt);
}

S32 zNPCGoalAttackArfMelee::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    ((zNPCCommon*)(psyche->clt_owner))->VelStop();
    if ((globals.player.Health != 0) && !(globals.player.DamageTimer > 0.0f))
    {
        PlayerTests();
    }
    FXStreakUpdate();
    return zNPCGoalPushAnim::Process(trantype, dt, updCtxt, xscn);
}

void zNPCGoalAttackArfMelee::PlayerTests()
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 idxlist[2] = { 0x25, 0x26 };
    xBound bnd;

    memset(&bnd, 0, sizeof(xBound));
    bnd.type = 1;
    bnd.sph.r = 0.5f;

    for (S32 i = 0; i < 2; i++)
    {
        xVec3 pos = *(const xVec3*)npc->BonePos(idxlist[i]);
        xMat3x3RMulVec(&pos, (const xMat3x3*)npc->BoneMat(0), &pos);
        pos += *(const xVec3*)npc->BonePos(0);
        bnd.sph.center = pos;

        if (NPCC_chk_hitPlyr(&bnd, NULL))
        {
            if (zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos()) != 0)
            {
                npc->Vibrate(NPC_VIBE_HARD, -1.0f);
            }
            return;
        }

        if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, 2) != 0)
        {
            xDrawSetColor(g_NEON_RED);
            xBoundDraw(&bnd);
        }
    }
}

void zNPCGoalAttackArfMelee::FXStreakPrep()
{
    for (int i = 0; i < 4; i++)
    {
        streakID[i] = NPCC_StreakCreate(NPC_STRK_ARFMELEE);
    }
}

void zNPCGoalAttackArfMelee::FXStreakDone()
{
    for (S32 i = 0; i < (S32)(sizeof(this->streakID) / sizeof(U32)); i++)
    {
        xFXStreakStop(streakID[i]);
        streakID[i] = 0xDEAD;
    }
}

void zNPCGoalAttackArfMelee::FXStreakUpdate()
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 idxlist[2] = { 0x25, 0x26 };

    for (S32 i = 0; i < 2; i++)
    {
        const xMat4x3* mat = (const xMat4x3*)npc->BoneMat(idxlist[i]);
        S32 idx = 2 * i;

        const xVec3 wide = mat->right * 0.7f;
        xVec3 high = mat->up * 0.7f;
        xVec3 a = mat->pos + wide;
        xVec3 b = mat->pos - wide;

        xMat3x3RMulVec(&a, (const xMat3x3*)npc->BoneMat(0), &a);
        xMat3x3RMulVec(&b, (const xMat3x3*)npc->BoneMat(0), &b);
        a += *(const xVec3*)npc->BonePos(0);
        b += *(const xVec3*)npc->BonePos(0);

        xFXStreakUpdate(streakID[idx], &a, &b);

        xVec3 a2 = mat->pos - high;
        xVec3 b2 = mat->pos + high;

        xMat3x3RMulVec(&a2, (const xMat3x3*)npc->BoneMat(0), &a2);
        xMat3x3RMulVec(&b2, (const xMat3x3*)npc->BoneMat(0), &b2);
        a2 += *(const xVec3*)npc->BonePos(0);
        b2 += *(const xVec3*)npc->BonePos(0);
        xFXStreakUpdate(streakID[idx + 1], &a2, &b2);
    }
}

S32 zNPCGoalAttackArf::Enter(F32 dt, void* updCtxt)
{
    static const U32 keepflags = 7;
    if ((this->flg_info & 0x10) == 0)
    {
        this->flg_attack = 0;
    }
    else
    {
        this->flg_attack &= keepflags;
    }
    if ((this->flg_attack & keepflags) == 0)
    {
        this->SetAttackMode(0, 0);
    }
    this->flg_info = 0;
    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackArf::Exit(F32 dt, void* updCtxt)
{
    flg_attack = 0;
    return zNPCGoalPushAnim::Exit(dt, updCtxt);
}

S32 zNPCGoalAttackArf::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 nextgoal = 0;
    npc->VelStop();
    npc->FacePlayer(dt, 3 * PI);
    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_SWAP;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (globals.player.DamageTimer > 0.5f)
    {
        *trantype = GOAL_TRAN_SWAP;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != 0)
    {
        return nextgoal;
    }
    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, -1))
    {
        NPCC_DrawPlayerPredict(5, 1.0f, 4.0f);
    }
    if (flg_attack & 1)
    {
        if (npc->IsAttackFrame(-1.0f, 1) && (LaunchBone(dt, 1) != 0))
        {
            flg_attack &= 0xFFFFFFFC;
        }
    }
    else if ((flg_attack & 2) && npc->IsAttackFrame(-1.0f, 2) && (LaunchDoggie(dt) != 0))
    {
        flg_attack &= 0xFFFFFFFC;
    }
    return zNPCGoalPushAnim::Process(trantype, dt, updCtxt, xscn);
}

void zNPCGoalAttackArf::SetAttackMode(S32 a, S32 b)
{
    flg_attack &= 0xfffffff8;
    if (a != 0)
    {
        flg_attack |= 2;
        if (b != 0)
        {
            flg_attack |= 4;
        }
    }
    else
    {
        flg_attack |= 1;
    }
    flg_info |= 0x10;
}

S32 zNPCGoalAttackArf::LaunchBone(F32 dt, S32 param_2)
{
    zNPCArfArf* npc = ((zNPCArfArf*)(psyche->clt_owner));
    return npc->LaunchProjectile(NPC_HAZ_ARFBONE, 8.0, 3.5, NPC_MDLVERT_ATTACK, 4.0f, 0.35f);
}

S32 zNPCGoalAttackArf::LaunchDoggie(F32 dt)
{
    zNPCArfArf* npc = (zNPCArfArf*)this->psyche->clt_owner;
    zNPCArfDog* pup = npc->AdoptADoggie();

    xVec3 pos_launch = *(const xVec3*)npc->BonePos(37);
    xMat3x3RMulVec(&pos_launch, (const xMat3x3*)npc->BoneMat(0), &pos_launch);
    pos_launch += *(const xVec3*)npc->BonePos(0);

    xVec3 pos_land = *npc->Pos();
    pos_land += *(const xVec3*)NPCC_faceDir(npc) * 3.0f;
    pos_land += *(const xVec3*)NPCC_rightDir(npc) * (1.5f * ((xrand() & 0x800000) ? 1.0f : -1.0f));

    xPsyche* psy_pup = pup->psy_instinct;
    zNPCGoalDogLaunch* godog = (zNPCGoalDogLaunch*)psy_pup->FindGoal(NPC_GOAL_DOGLAUNCH);

    if (flg_attack & 4)
    {
        godog->SilentSwimout(&pos_launch, &pos_land, npc->nav_curr);
    }
    else
    {
        godog->ViciousAttack(&pos_launch, &pos_land, npc->nav_curr, 0);
    }

    psy_pup->GoalSet(NPC_GOAL_DOGLAUNCH, 0);

    return 1;
}

S32 zNPCGoalAttackChuck::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    idx_launch = 1;
    npc->ModelAtomicHide(1, NULL);
    npc->SndPlayRandom(NPC_STYP_WEPLAUNCH);
    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackChuck::Exit(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->ModelAtomicHide(1, NULL);
    return zNPCGoalPushAnim::Exit(dt, updCtxt);
}

S32 zNPCGoalAttackChuck::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 nextgoal = 0;
    HAZ_AvailablePool();
    if (*trantype != 0)
    {
        return nextgoal;
    }
    npc->VelStop();
    npc->FacePlayer(dt, 3 * PI);
    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, -1))
    {
        NPCC_DrawPlayerPredict(5, 1.0f, 4.0f);
    }
    if ((npc->AnimTimeCurrent() > 0.2f) && (idx_launch == 1))
    {
        npc->ModelAtomicShow(1, 0);
    }
    S32 zapidx = npc->IsAttackFrame(-1.0f, 0);

    if (zapidx == idx_launch)
    {
        if (BombzAway(dt))
        {
            idx_launch++;
            npc->ModelAtomicHide(1, NULL);
        }
    }
    return zNPCGoalPushAnim::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalAttackChuck::BombzAway(F32 param_1)
{
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));
    npc->SndPlayRandom(NPC_STYP_ATTACK);
    return npc->LaunchProjectile(NPC_HAZ_CHUCKBOMB, 15.0f, 3.0f, NPC_MDLVERT_ATTACK, 4.0f, 0.0f);
}

S32 zNPCGoalAttackSlick::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    idx_launch = 1;
    zNPCGoalLoopAnim::LoopCountSet(1);
    npc->SndPlayRandom(NPC_STYP_ATTACK);
    return zNPCGoalLoopAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalAttackSlick::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCSlick* npc = (zNPCSlick*)(psyche->clt_owner);
    S32 zapidx;

    npc->FacePlayer(dt, 3 * PI);
    npc->ThrottleAdjust(dt, 0.0f, -1.0f);
    npc->ThrottleApply(dt, NPCC_faceDir(npc), 0);
    if (npc->DBG_IsNormLog(eNPCDCAT_Thirteen, -1) != 0)
    {
        NPCC_DrawPlayerPredict(5, 1.0, 4.0);
    }
    zapidx = 0;
    if (anid_played == anid_stage[1])
    {
        zapidx = npc->IsAttackFrame(-1.0f, 0);
    }
    if (cnt_loop < idx_launch)
    {
        idx_launch = cnt_loop;
    }
    if ((zapidx != 0) && (idx_launch != 0) && (cnt_loop == idx_launch))
    {
        HAZ_AvailablePool();
        if (FireOne(idx_launch) != 0)
        {
            idx_launch--;
        }
    }
    return zNPCGoalLoopAnim::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalAttackSlick::FireOne(S32)
{
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));
    return npc->LaunchProjectile(NPC_HAZ_OILBUBBLE, 9.0f, 4.0f, NPC_MDLVERT_ATTACK, 4.0f, 0.1f);
}

S32 zNPCGoalDogLaunch::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    zMovePoint* nav_preserveCurr = npc->nav_curr;
    zNPCGoalAfterlife* wanna = (zNPCGoalAfterlife*)(psyche->FindGoal(NPC_GOAL_AFTERLIFE));
    if (wanna != 0)
    {
        wanna->DieWithABang();
    }
    npc->MatPosSet(&pos_src);
    xVec3Copy(&npc->frame->mat.pos, &pos_src);
    npc->frame->mode = 1;
    npc->MvptReset(nav_preserveCurr);
    npc->arena.Cycle(npc, 0);
    tmr_remain = -1.0;
    PreCollide();
    zNPCCommon* duper = npc->npc_duplodude;
    if (duper != 0)
    {
        duper->DuploNotice((en_SM_NOTICES)2, npc);
    }
    flg_info = 0;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalDogLaunch::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);

    if (BallisticUpdate(dt) != 0)
    {
        if (!(flg_launch & 2))
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_DAMAGE;
        }
        else if (flg_launch & 1)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_IDLE;
        }
        else
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_ALERT;
        }
    }
    if (*trantype != 0)
    {
        return nextgoal;
    }
    else
    {
        FurryFlurry();
        npc->colFreq = 1;
        xDrawSetColor(g_BLUE);
        xDrawSphere(&pos_tgt, 0.15f, 0xC0006);
        return xGoal::Process(trantype, dt, updCtxt, xscn);
    }
}

void zNPCGoalDogLaunch::ViciousAttack(xVec3* pos_src, xVec3* pos_tgt, zMovePoint* mvpt, S32 unk)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    this->flg_launch = 0;
    if (unk != 0)
    {
        this->flg_launch |= 1;
    }
    this->pos_src = *pos_src;
    this->pos_tgt = *pos_tgt;
    npc->MvptReset(mvpt);
    this->flg_info |= 0x10;
}

void zNPCGoalDogLaunch::PreCollide()
{
    static xCollis colrec;

    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    xVec3Sub(&npc->frame->vel, &pos_tgt, &pos_src);

    F32 dst = xVec3Length(&npc->frame->vel);

    if (dst < 1.0f)
    {
        dst = 1.0f;
    }

    F32 tym_ETALand = dst / 15.0f;

    xVec3SMulBy(&npc->frame->vel, 1.0f / tym_ETALand);

    npc->frame->vel.y += 5.0f * (0.5f * tym_ETALand);
    npc->frame->mode |= 4;

    xParabola* parab = &parabinfo;

    xVec3Copy(&parab->initPos, &pos_src);
    xVec3Copy(&parab->initVel, &npc->frame->vel);

    parab->gravity = 5.0f;
    parab->minTime = 0.0f;
    parab->maxTime = 5.0f + tym_ETALand;

    memset(&colrec, 0, sizeof(colrec));

    if (xParabolaHitsEnv(parab, globals.sceneCur->env, &colrec))
    {
        flg_launch |= 2;
        tmr_remain = colrec.dist;
    }
    else
    {
        flg_launch &= ~2;
        tmr_remain = 5.0f + tym_ETALand;
    }
}

S32 zNPCGoalDogLaunch::BallisticUpdate(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 arrived = 0;
    F32 tym = psyche->TimerGet(XPSY_TYMR_CURGOAL);
    xVec3 pos_want;
    xVec3 vel;

    if (((tmr_remain < 0.0f) ? 1 : 0))
    {
        arrived = 1;
        xVec3Copy(&npc->frame->vel, &g_O3);
        npc->frame->mode |= 4;
    }
    else
    {
        tmr_remain = MAX(-1.0f, (tmr_remain - dt));
        xParabola* parab = &parabinfo;

        tym = psyche->TimerGet(XPSY_TYMR_CURGOAL);
        tym = MIN(tym, parab->maxTime);

        xParabolaEvalPos(parab, &pos_want, tym);
        npc->frame->dpos = pos_want - npc->frame->mat.pos;
        npc->frame->mode |= 2;

        xParabolaEvalVel(parab, &vel, tym);
        vel.y = 0.0f;

        F32 spd = xsqrt(SQ(vel.x) + SQ(vel.z));

        if (spd > 1e-05f)
        {
            xVec3 dir = vel / spd;
            *NPCC_faceDir(npc) = dir;
            *NPCC_upDir(npc) = g_Y3;
            xVec3Cross(NPCC_rightDir(npc), &dir, &g_Y3);
        }
    }

    return arrived;
}

void zNPCGoalDogLaunch::BubTrailCone(const xVec3* pos, S32 num, const xVec3* pos_rand,
                                     const xVec3* vel_rand, const xMat3x3* mat)
{
    if (num < 1)
    {
        return;
    }

    xVec3* posbuf = (xVec3*)xMemPushTemp(2 * num * sizeof(xVec3));
    xVec3* velbuf = posbuf + num;

    if (posbuf == NULL)
    {
        return;
    }

    F32 zrnd_vel;
    F32 zrnd_pos;
    F32 ang_perseg;

    zrnd_pos = pos_rand->z;
    zrnd_vel = vel_rand->z;
    ang_perseg = 2.0f * PI / num;

    xVec3* pos_cur = posbuf;
    xVec3* vel_cur = velbuf;

    for (S32 i = 0; i < num; i++)
    {
        F32 ang = ang_perseg * xurand() + ang_perseg * i;
        F32 cosang = icos(ang);
        F32 sinang = isin(ang);

        *pos_cur = *pos;
        *vel_cur = mat->at * (zrnd_pos * xurand());
        *pos_cur += mat->right * cosang * pos_rand->x;
        *pos_cur += mat->up * sinang * pos_rand->y;

        *vel_cur = g_O3;
        *vel_cur = mat->at * (zrnd_vel * xurand());
        *vel_cur += mat->right * cosang * vel_rand->x;
        *vel_cur += mat->up * sinang * vel_rand->y;

        pos_cur++;
        vel_cur++;
    }

    zParPTankSpawnBubbles(posbuf, velbuf, num, 1.0f);
    xMemPopTemp(posbuf);
}

void zNPCGoalDogLaunch::FurryFlurry()
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);

    static S32 moreorless = 0;
    static const xVec3 pos_disperse = { 0.01f, 0.01f, 0.01f };
    static const xVec3 vel_disperse = { 3.0f, 2.5f, -2.0f };

    if ((--moreorless < 0) && (psyche->TimerGet(XPSY_TYMR_CURGOAL) > 0.04f))
    {
        moreorless = -1;
        BubTrailCone(npc->Center(), 15, &pos_disperse, &vel_disperse, (xMat3x3*)npc->BoneMat(0));
    }
}

S32 zNPCGoalDogBark::Enter(F32 dt, void* updCtxt)
{
    zNPCGoalLoopAnim::LoopCountSet(1);
    return zNPCGoalLoopAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalDogBark::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (npc->npcset.allowDetect == 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        npc->FacePlayer(dt, 4 * PI);
        npc->VelStop();
        return zNPCGoalLoopAnim::Process(trantype, dt, updCtxt, xscn);
    }
}

S32 zNPCGoalDogDash::Enter(F32 dt, void* updCtxt)
{
    zNPCGoalLoopAnim::LoopCountSet(1);
    return zNPCGoalLoopAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalDogDash::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (npc->npcset.allowDetect == 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        this->HoundPlayer(dt);
        return zNPCGoalLoopAnim::Process(trantype, dt, updCtxt, xscn);
    }
}

void zNPCGoalDogDash::HoundPlayer(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    npc->ThrottleAdjust(dt, 4.0f, 20.0f);
    xVec3* dir = NPCC_faceDir(npc);
    npc->ThrottleApply(dt, dir, 0);
}

S32 zNPCGoalDogPounce::Enter(F32 dt, void* updCtxt)
{
    flg_user = 0;
    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalDogPounce::Exit(F32 dt, void* updCtxt)
{
    if (flg_user == 0)
    {
        Detonate();
    }
    return zNPCGoalPushAnim::Exit(dt, updCtxt);
}

S32 zNPCGoalDogPounce::NPCMessage(NPCMsg* mail)
{
    S32 ret = 0;
    switch (mail->msgid)
    {
    case NPC_MID_DAMAGE:
        flg_user = 1;
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

void zNPCGoalDogPounce::Detonate()
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    npc->SndPlayRandom(NPC_STYP_WARNBANG);
    NPCHazard* haz = HAZ_Acquire();

    if (haz != NULL)
    {
        haz->ConfigHelper(NPC_HAZ_PUPPYNUKE);
        haz->SetNPCOwner(npc);
        haz->Start(xEntGetCenter(npc), -1.0f);
    }
}

S32 zNPCGoalTeleport::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->flg_move &= 0xFFFFFFFD;
    npc->flg_move |= 4;
    if (npc->nav_dest == NULL || npc->nav_dest == npc->nav_curr)
    {
        npc->MvptCycle();
    }
    npc->chkby = 0;
    npc->penby = 0;
    npc->flags2.flg_colCheck = 0;
    npc->flags2.flg_penCheck = 0;
    npc->pflags &= 0xFB;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalTeleport::Exit(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));

    npc->flg_move |= 2;
    npc->flg_move &= 0xFFFFFFFB;

    npc->RestoreColFlags();

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalTeleport::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCArfArf* npc = (zNPCArfArf*)(psyche->clt_owner);

    if (*trantype != GOAL_TRAN_NONE)
    {
        return 0;
    }

    zMovePoint* telept = npc->GetTelepoint(npc->cfg_npc->pts_damage - npc->hitpoints);
    S32 arrived;
    xVec3 dir_move;

    npc->FacePlayer(dt, 3.0f * PI);

    arrived = 0;

    if (telept != NULL)
    {
        npc->XYZVecToPos(&dir_move, telept->PosGet());

        F32 dst = xVec3Length(&dir_move);
        F32 spd = npc->ThrottleAdjust(dt, 12.0f, 15.0f);

        if ((dst < 1e-05f) || (spd * dt > dst))
        {
            arrived = 1;
            xVec3Copy(&npc->frame->mat.pos, telept->PosGet());
            npc->frame->mode |= 1;
            npc->VelStop();
        }
        else
        {
            xVec3SMulBy(&dir_move, 1.0f / dst);
            npc->ThrottleApply(dt, &dir_move, 1);
        }
    }
    else
    {
        arrived = 1;
        npc->VelStop();
    }

    if (arrived)
    {
        if (npc->nav_dest != NULL)
        {
            zEntEvent((xBase*)npc, (xBase*)npc->nav_dest, 0x1f);
        }
        npc->MvptCycle();
        nextgoal = 1;
        *trantype = GOAL_TRAN_POP;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalTeleport::NPCMessage(NPCMsg* msg)
{
    switch (msg->msgid)
    {
    case NPC_MID_DAMAGE:
        return 1;
    }
    return 0;
}

S32 zNPCGoalHokeyPokey::Enter(F32 dt, void* updCtxt)
{
    zNPCFodBzzt* bzzt = ((zNPCFodBzzt*)(psyche->clt_owner));
    flg_hokey = (xrand() >> 0x17) & 1;
    flg_hokey |= 2;
    ang_spinrate = 0.0f;
    bzzt->DiscoReset();
    return zNPCGoalLoopAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalHokeyPokey::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCFodBzzt* npc = (zNPCFodBzzt*)(psyche->clt_owner);

    if (globals.player.Health < 1)
    {
        zNPCGoalTaunt* taunt = (zNPCGoalTaunt*)(psyche->FindGoal(NPC_GOAL_TAUNT));
        taunt->LoopCountSet(1000);
        *trantype = GOAL_TRAN_SWAP;
        nextgoal = NPC_GOAL_TAUNT;
    }
    else if (npc->SomethingWonderful() != 0)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }
    else if (zNPCFodBzzt::tmr_hokeypokey > 0.4f)
    {
        cnt_loop = MAX(2, cnt_loop);
    }
    else if (!npc->npcset.allowDetect)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    if (g_needMusician)
    {
        npc->SndPlayRandom(NPC_STYP_DANCE);
        g_needMusician = 0;
    }

    if (zNPCFodBzzt::tmr_hokeypokey < 0.5f)
    {
        ang_spinrate *= 0.8f;
    }
    else if (xabs(ang_spinrate) < 1.5707964f)
    {
        F32 sidely = -1.0f;

        if (flg_hokey & 1)
        {
            sidely = 1.0f;
        }

        ang_spinrate += sidely * (18.849556f * dt);
    }

    if ((zNPCFodBzzt::tmr_hokeypokey < 0.32f) && (flg_hokey & 2) &&
        (xabs(ang_spinrate) < 0.31415927f))
    {
        flg_hokey &= ~2;
        TriggerExit();
    }

    if (!(flg_hokey & 2))
    {
        npc->FacePlayer(dt, PI);
    }
    else
    {
        npc->frame->drot.angle = dt * ang_spinrate;
        npc->frame->mode |= 0x20;
    }

    npc->VelStop();
    npc->DiscoUpdate(dt);

    return zNPCGoalLoopAnim::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalEvilPat::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));

    if (npc->SelfType() == NPC_TYPE_GLOVE)
    {
        npc->flg_vuln |= 0x80000000;
    }

    GlyphStart();

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalEvilPat::Exit(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));

    if (npc->SelfType() == NPC_TYPE_GLOVE)
    {
        npc->flg_vuln &= 0x7FFFFFFF;
    }

    npc->tmr_stunned = -1.0f;

    GlyphStop();

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalEvilPat::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene)
{
    U32 nextgoal;
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));

    if (npc->tmr_stunned < 0.0f)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_ALERT;
    }
    else
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_STUNNED;
    }

    return (*trantype != GOAL_TRAN_NONE) ? nextgoal : xGoal::Process(trantype, dt, updCtxt, scene);
}

S32 zNPCGoalEvilPat::NPCMessage(NPCMsg* mail)
{
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));
    switch (mail->msgid)
    {
    case NPC_MID_STUN:
        F32 stuntime = mail->stundata.tym_stuntime;
        F32 blah = (xurand() - 0.5f);
        blah = 0.25f * blah;
        npc->tmr_stunned = stuntime + (stuntime * blah);
        return 1;
    }
    return 0;
}

S32 zNPCGoalEvilPat::InputStun(NPCStunInfo* info)
{
    xPsyche* psyche = xGoal::GetPsyche();
    zNPCGoalStunned* stunned = (zNPCGoalStunned*)psyche->FindGoal(NPC_GOAL_STUNNED);
    return stunned == NULL ? 0 : stunned->InputInfo(info);
}

void zNPCGoalEvilPat::GlyphStart()
{
    static xVec3 ang_delta = { DEG2RAD(2.1000001f), 0.0f, 0.0f };
    static xVec3 scale = { 0.75f, 0.75f, 0.75f };

    zNPCRobot* robot = (zNPCRobot*)psyche->clt_owner;
    robot->glyf_stun = GLYF_Acquire(NPC_GLYPH_DAZED);
    if (robot->glyf_stun != NULL)
    {
        robot->glyf_stun->Enable(1);
        robot->glyf_stun->RotSet(&ang_delta, 0);
        robot->glyf_stun->ScaleSet(&scale);
    }
}

void zNPCGoalEvilPat::GlyphStop()
{
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));
    if (npc->glyf_stun != NULL)
    {
        npc->glyf_stun->Discard();
    }
    npc->glyf_stun = NULL;
}

S32 zNPCGoalStunned::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    if (!(flg_info & 0x10))
    {
        F32 stun = npc->cfg_npc->tym_stun;
        npc->tmr_stunned = (stun * (0.25f * (xurand() - 0.5f)) + stun);
    }
    flg_info = 0;
    npc->VelStop();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalStunned::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    if (npc->tmr_stunned < 0.0f)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_ALERT;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        npc->tmr_stunned = MAX(-1.0f, (npc->tmr_stunned - dt));
        npc->SyncStunGlyph(dt, npc->tmr_stunned, -1.0f);
        return xGoal::Process(trantype, dt, updCtxt, xscn);
    }
}

S32 zNPCGoalStunned::InputInfo(NPCStunInfo* info)
{
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));
    flg_info = 0x10;
    F32 stunTime = npc->tmr_stunned;
    F32 infoStun = info->tym_stuntime;
    stunTime = (stunTime > infoStun) ? stunTime : infoStun;
    npc->tmr_stunned = stunTime;
    return flg_info;
}

S32 zNPCGoalPatCarry::Enter(F32 dt, void* updCtxt)
{
    static xVec3 scale = { 0.3f, 0.3f, 0.3f };

    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    NPCGlyph* glyph = npc->glyf_stun;
    if (glyph != NULL)
    {
        glyph->ScaleSet(&scale);
    }
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalPatCarry::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));
    static F32 offset = 0.5f;
    npc->SyncStunGlyph(dt, 5.0f, offset);
    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalPatThrow::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    NPCGlyph* glyph = npc->glyf_stun;
    if (glyph != NULL)
    {
        glyph->Discard();
    }
    npc->glyf_stun = NULL;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalLassoBase::Enter(F32 dt, void* updCtxt)
{
    zNPCSlick* npc = (zNPCSlick*)(psyche->clt_owner);
    if (npc->SelfType() == NPC_TYPE_SLICK)
    {
        npc->RopePopsShield();
    }
    npc->Vibrate(NPC_VIBE_SOFT, -1.0f);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalLassoBase::Exit(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->LassoNotify(LASS_EVNT_ENDED);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalLassoBase::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCLassoInfo* lass = (zNPCLassoInfo*)((zNPCCommon*)(psyche->clt_owner))->GimmeLassInfo();
    S32 nextgoal;
    if (lass->stage == LASS_STAT_GRABBING)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_LASSOGRAB;
    }
    else if (lass->stage == LASS_STAT_TOSSING)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_LASSOTHROW;
    }
    else
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_ALERT;
    }
    return (*trantype != 0) ? nextgoal : xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalLassoGrab::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    S32 nextgoal = 0;
    zNPCLassoInfo* lass = npc->GimmeLassInfo();
    U32 anid = npc->AnimCurStateID();

    if (npc->AnimTimeRemain(NULL) < (0.001f + dt))
    {
        if (anid == g_hash_roboanim[7])
        {
            npc->LassoNotify(LASS_EVNT_GRABEND);
            zEntPlayer_LassoNotify(LASS_EVNT_GRABEND);
        }
        else if (anid == g_hash_roboanim[8])
        {
            npc->LassoNotify(LASS_EVNT_YANK);
        }
    }

    if (lass->stage == LASS_STAT_TOSSING)
    {
        *trantype = GOAL_TRAN_SWAP;
        nextgoal = NPC_GOAL_LASSOTHROW;
        npc->Vibrate(NPC_VIBE_HARD, -1.0f);
    }
    else if (lass->stage == LASS_STAT_GRABBING)
    {
    }
    else
    {
        if (lass->stage != LASS_STAT_DONE)
        {
            npc->LassoNotify(LASS_EVNT_ENDED);
        }

        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_DAMAGE;
        npc->Vibrate(NPC_VIBE_HARD, -1.0f);
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    npc->LassoSyncAnims(LASS_ANIM_GRAB);

    if (anid == g_hash_roboanim[7])
    {
        npc->Vibrate(NPC_VIBE_BUILD_A, -1.0f);
    }
    else if (anid == g_hash_roboanim[8])
    {
        npc->Vibrate(NPC_VIBE_BUILD_B, -1.0f);
    }

    npc->VelStop();
    DoTurnAway(dt);

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

void zNPCGoalLassoGrab::DoTurnAway(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;

    S32 ntlist_towards[6] = { NPC_TYPE_MONSOON, NPC_TYPE_SLEEPY, NPC_TYPE_TUBELET,
                              NPC_TYPE_CHUCK,   NPC_TYPE_SLICK,  0 };

    S32 ntyp = npc->SelfType();
    S32 faceAway = 1;
    S32 type;
    S32 i = 0;

    while (ntlist_towards[i] != 0)
    {
        type = ntlist_towards[i++];

        if (type == ntyp)
        {
            faceAway = 0;
            break;
        }
    }

    if (faceAway)
    {
        npc->FaceAntiPlayer(dt, DEG2RAD(720));
    }
    else
    {
        npc->FacePlayer(dt, DEG2RAD(720));
    }
}

S32 zNPCGoalLassoThrow::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = (zNPCCommon*)(psyche->clt_owner);

    flg_throw = 0;
    floorBounce = 0;

    if (npc->pflags & 0x4)
    {
        flg_throw |= 0x10;
    }
    else
    {
        flg_throw &= ~0x10;
    }
    npc->pflags |= 0x4;

    if (globals.pad0->analog1.x > globals.player.g.AnalogMin)
    {
        ApplyYank(0);
        flg_throw &= ~0x8;
    }
    else if (globals.pad0->analog1.x < -globals.player.g.AnalogMin)
    {
        ApplyYank(1);
        flg_throw |= 0x8;
    }
    else if (xrand() & 0x800000)
    {
        ApplyYank(0);
        flg_throw &= ~0x8;
    }
    else
    {
        ApplyYank(1);
        flg_throw |= 0x8;
    }

    npc->GimmeLassInfo();
    npc->flg_vuln &= ~0x1000000;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalLassoThrow::Exit(F32 dt, void* updCtxt)
{
    xEnt* ent = (xEnt*)(this->psyche->clt_owner);

    if ((flg_throw & 0x10) == 0)
    {
        ent->pflags &= 0xfb;
    }

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalLassoThrow::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 dir_toss;
    F32 fac;

    npc->flg_vuln |= 0x1000000;
    zNPCLassoInfo* lass = npc->GimmeLassInfo();
    npc->flg_vuln &= ~0x1000000;

    if ((flg_throw & 0x20) || (flg_throw & 0x2) || (lass->stage != LASS_STAT_TOSSING) ||
        (npc->AnimTimeRemain(NULL) < (0.001f + dt)))
    {
        npc->LassoNotify(LASS_EVNT_ENDED);
        zEntPlayer_LassoNotify(LASS_EVNT_ENDED);
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_DAMAGE;
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    tmr_colDelay = MAX(-1.0f, tmr_colDelay - dt);

    dir_toss.x = npc->frame->vel.x;
    dir_toss.y = 0.0f;
    dir_toss.z = npc->frame->vel.z;

    fac = xVec3Length(&dir_toss);

    if (fac > 0.001f)
    {
        S32 ntyp = npc->SelfType();
        F32 sign = 1.0f;

        if (ntyp == NPC_TYPE_TARTAR)
        {
            sign = -1.0f;
        }

        xVec3SMulBy(&dir_toss, sign / fac);
        npc->TurnToFace(dt, &dir_toss, 4 * PI);
    }

    npc->colFreq = 0;

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalLassoThrow::CollReview(void*)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xEntCollis* npccol = npc->collis;
    xCollis* colrec = NULL;
    xVec3 vec_depen = { 0.0f, 0.0f, 0.0f };
    S32 hitstuff = 0;
    S32 i;
    xVec3 pump;
    F32 spd;
    xSurface* surf;
    S32 badsurf = 0;
    F32 goodep = 0.0f;

    if (npccol->colls[0].flags & k_HIT_IT)
    {
        flg_throw |= 0x1;
    }

    for (i = npccol->env_sidx; i < npccol->env_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    for (i = npccol->stat_sidx; i < npccol->stat_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    for (i = npccol->dyn_sidx; i < npccol->dyn_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    spd = 0.0f;

    if (npccol->npc_sidx < npccol->npc_eidx)
    {
        spd = xVec3Length(&npc->frame->vel);
    }

    for (i = npccol->npc_sidx; i < npccol->npc_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        zNPCCommon* tgt = (zNPCCommon*)(xEnt*)(colrec->optr);
        hitstuff++;

        if (tgt != NULL)
        {
            xVec3SMul(&pump, &colrec->hdng, spd);
            tgt->Damage(DMGTYP_HITBYTOSS, npc, &pump);
        }
    }

    if (badsurf)
    {
        flg_throw |= 0x20;
    }
    else if ((tmr_colDelay < 0.0f) && hitstuff && (xVec3Length2(&vec_depen) > 0.0f))
    {
        NPCConfig* cfg = npc->cfg_npc;
        flg_throw |= 0x2;

        xVec3Copy(&npc->frame->vel, &npc->frame->oldvel);
        NPCC_Bounce(&npc->frame->vel, &vec_depen, cfg->fac_elastic);
    }

    return 1;
}

F32 g_ang_yankDir = DEG2RAD(60);

void zNPCGoalLassoThrow::ApplyYank(S32 left)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 dir_aim = { 0.0f, 0.0f, 0.0f };
    xMat3x3 mat_rot = { { 0.0f, 0.0f, 0.0f }, 0, { 0.0f, 0.0f, 0.0f }, 0, { 0.0f, 0.0f, 0.0f },
                        0 };
    F32 goleft;
    F32 ang_ref;

    npc->GimmeLassInfo();

    goleft = (left != 0) ? -1.0f : 1.0f;

    tmr_colDelay = 0.07f;

    npc->XZVecToPos(&dir_aim, &globals.camera.mat.pos, NULL);

    F32 ds2_aim = xVec3Length2(&dir_aim);

    if (ds2_aim < 0.001f)
    {
        xVec3Copy(&dir_aim, NPCC_faceDir(&globals.player.ent));
    }
    else
    {
        dir_aim *= 1.0f / xsqrt(ds2_aim);
    }

    ang_ref = NPCC_dir_toXZAng(&dir_aim);

    NPCC_rotHPB(&mat_rot, goleft * g_ang_yankDir + ang_ref, DEG2RAD(30), 0.0f);
    xMat3x3RMulVec(&dir_aim, &mat_rot, &g_Z3);
    xVec3SMulBy(&dir_aim, 75.0f * npc->cfg_npc->npcMassInv);
    xVec3Add(&npc->frame->vel, &npc->frame->oldvel, &dir_aim);

    npc->frame->mode |= 0x4;
}

S32 zNPCGoalDamage::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);

    static F32 ds2_viberange = SQ(30.0f);

    if (npc->SelfType() == NPC_TYPE_TUBELET)
    {
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_DEFLATE;
    }
    else if (flg_info & 0x10)
    {
        if (flg_howtodie & 0x1)
        {
            npc->Vibrate(npc->XYZDstSqToPlayer(NULL), ds2_viberange);
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_AFTERLIFE;
        }
        else if (flg_howtodie & 0x8)
        {
            npc->Vibrate(npc->XYZDstSqToPlayer(NULL), ds2_viberange);
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_AFTERLIFE;
        }
        else if (flg_howtodie & 0x4)
        {
            npc->Vibrate(NPC_VIBE_NORM, -1.0f);
            *trantype = GOAL_TRAN_PUSH;
            nextgoal = NPC_GOAL_BASHED;
        }
        else if (flg_howtodie & 0x2)
        {
            npc->Vibrate(NPC_VIBE_NORM, -1.0f);
            *trantype = GOAL_TRAN_PUSH;
            nextgoal = NPC_GOAL_KNOCK;
        }
        else
        {
            npc->Vibrate(npc->XYZDstSqToPlayer(NULL), ds2_viberange);
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_AFTERLIFE;
        }
    }
    else
    {
        npc->Vibrate(npc->XYZDstSqToPlayer(NULL), ds2_viberange);
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_AFTERLIFE;
    }

    flg_info = 0;
    flg_howtodie = 0;

    return nextgoal;
}

S32 zNPCGoalDamage::NPCMessage(NPCMsg* msg)
{
    U32 ret = 1;
    xPsyche* psyche = (xPsyche*)xGoal::GetPsyche();
    switch (msg->msgid)
    {
    case NPC_MID_DAMAGE:
        if ((msg->infotype == NPC_MDAT_DAMAGE) &&
            ((msg->dmgdata.dmg_type == DMGTYP_SURFACE) ||
             (msg->dmgdata.dmg_type == DMGTYP_DAMAGE_SURFACE)))
        {
            psyche->GoalSet(NPC_GOAL_AFTERLIFE, 0);
        }
        break;
    default:
        ret = 0;
        break;
    }

    return ret;
}

S32 zNPCGoalDamage::InputInfo(NPCDamageInfo* info)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    flg_info = 0x10;
    switch (info->dmg_type)
    {
    case DMGTYP_BELOW:
        npc->InflictPain(-1, 0);
        flg_howtodie = 1;
        break;
    case DMGTYP_INSTAKILL:
        flg_howtodie = 4;
        npc->InflictPain(-1, 0);
        break;
    case DMGTYP_SIDE:
    case DMGTYP_HITBYTOSS:
    case DMGTYP_BOULDER:
    case DMGTYP_BUBBOWL:
    {
        flg_howtodie = 2;
        npc->InflictPain(-1, 0);
        zNPCGoalKnock* knock = (zNPCGoalKnock*)(psyche->FindGoal(NPC_GOAL_KNOCK));
        knock->InputInfo(info);
        break;
    }
    default:
        flg_howtodie = 1;
        npc->InflictPain(-1, 0);
        break;
    }
    return flg_info;
}

S32 zNPCGoalBashed::Enter(F32 dt, void* updCtxt)
{
    F32 rand;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    npc->VelStop();
    static F32 grav = globals.player.g.Gravity;
    static F32 tym_bashrise = globals.player.g.BBashTime - globals.player.g.BBashCVTime;
    static F32 player_bash_fromgrav = grav * 0.5f * tym_bashrise;
    static F32 player_bash_speed =
        (globals.player.g.BBashHeight + player_bash_fromgrav) / globals.player.g.BBashTime;
    npc->frame->vel.y = 10.0f;
    npc->frame->vel.x =
        (globals.camera.mat.right.x * (2.0f * (xurand() - 0.5f)) + globals.camera.mat.at.x) * 5.0f;
    npc->frame->vel.z =
        (globals.camera.mat.right.z * (2.0f * (xurand() - 0.5f)) + globals.camera.mat.at.z) * 5.0f;
    npc->frame->mode |= 4;
    zNPCGoalLoopAnim::LoopCountSet(1);
    npc->InflictPain(1, 0);
    npc->SndPlayRandom(NPC_STYP_OUCH);
    return zNPCGoalLoopAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalBashed::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene)
{
    xEnt* ent = ((xEnt*)(psyche->clt_owner));
    ent->frame->vel.y = -(dt * 30.0f - ent->frame->vel.y);
    ent->frame->mode |= 4;
    return this->zNPCGoalLoopAnim::Process(trantype, dt, updCtxt, scene);
}

S32 zNPCGoalWound::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    this->flg_knock = 0;
    F32 dst = npc->XZDstSqToPlayer(&this->dir_fling, NULL);
    if (dst < 0.001f)
    {
        xVec3Copy(&this->dir_fling, NPCC_faceDir(&globals.player.ent));
    }
    else
    {
        xVec3SMulBy(&this->dir_fling, (-1.0f / xsqrt(dst)));
    }
    S32 iVar2 = npc->SelfType();
    if (iVar2 == NPC_TYPE_TARTAR)
    {
        npc->spd_throttle = 18.0f;
    }
    else if (iVar2 == NPC_TYPE_SLEEPY)
    {
        npc->spd_throttle = 8.0f;
    }
    else
    {
        npc->spd_throttle = 7.0f;
    }
    if (npc->flg_move & 2)
    {
        npc->frame->vel.y = 5.0;
    }
    npc->InflictPain(1, 0);
    npc->SndPlayRandom(NPC_STYP_DIZZY);
    if (psyche->GIDInStack(NPC_GOAL_ALERT) == NULL)
    {
        this->flg_pushanim |= 0x10000;
    }
    else
    {
        this->flg_pushanim &= 0xfffeffff;
    }
    return zNPCGoalPushAnim::Enter(dt, updCtxt);
}

S32 zNPCGoalWound::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    npc->ThrottleApply(dt, &dir_fling, 0);
    npc->ThrottleAdjust(dt, 0.0f, 7.0f);
    if (flg_knock & 8)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = 0x4e475264;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    if (flg_pushanim & 0x10000)
    {
        if ((anid_played != npc->AnimCurStateID()) || (npc->AnimTimeRemain(0) <= (dt + 0.001f)) ||
            (flg_pushanim & 1))
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 0x4e475234;
        }
    }
    else
    {
        nextgoal = zNPCGoalPushAnim::Process(trantype, dt, updCtxt, xscn);
    }
    return nextgoal;
}

S32 zNPCGoalWound::CollReview(void*)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    NPCConfig* cfg = npc->cfg_npc;
    xEntCollis* npccol = npc->collis;
    xCollis* colrec = NULL;
    xVec3 vec_depen = { 0.0f, 0.0f, 0.0f };
    S32 hitstuff = 0;
    S32 i;
    xVec3 pump = { 0.0f, 0.0f, 0.0f };
    F32 spd = 0.0f;
    xSurface* surf;
    S32 badsurf = 0;
    F32 goodep = 0.0f;

    if (npccol->colls[0].flags & k_HIT_IT)
    {
        flg_knock |= 0x1;
    }

    for (i = npccol->env_sidx; i < npccol->env_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    for (i = npccol->stat_sidx; i < npccol->stat_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    for (i = npccol->dyn_sidx; i < npccol->dyn_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    if (npccol->npc_sidx < npccol->npc_eidx)
    {
        spd = xVec3Normalize(&pump, &npc->frame->vel);
    }

    for (i = npccol->npc_sidx; i < npccol->npc_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        zNPCCommon* tgt = (zNPCCommon*)(xEnt*)(colrec->optr);

        xVec3Normalize(&pump, &colrec->tohit);
        xVec3SMulBy(&pump, spd);
        tgt->Damage(DMGTYP_HITBYTOSS, npc, &pump);
    }

    if (badsurf)
    {
        flg_knock |= 0x8;
    }
    else if ((psyche->TimerGet(XPSY_TYMR_CURGOAL) > 0.1f) && hitstuff &&
             (xVec3Length2(&vec_depen) > 0.0f))
    {
        F32 spd_bounce;

        flg_knock |= 0x2;

        xVec3 dir = { 0.0f, 0.0f, 0.0f };

        xVec3Copy(&dir, &npc->frame->oldvel);
        NPCC_Bounce(&dir, &vec_depen, cfg->fac_elastic);

        spd_bounce = xVec3Length(&dir);

        if (spd_bounce > 1e-05f)
        {
            npc->spd_throttle = spd_bounce;
            xVec3SMul(&dir_fling, &dir, 1.0f / spd_bounce);
        }
    }

    return 1;
}

S32 zNPCGoalWound::NPCMessage(NPCMsg* msg)
{
    switch (msg->msgid)
    {
    case NPC_MID_DAMAGE:
        return 1;
    }
    return 0;
}

S32 zNPCGoalKnock::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    NPCConfig* cfg = npc->cfg_npc;
    xVec3 dir_aim = { 0.0f, 0.0f, 0.0f };

    flg_knock = 0;
    floorBounce = 0;

    if (npc->pflags & 0x4)
    {
        flg_knock |= 0x4;
    }
    else
    {
        flg_knock &= ~0x4;
    }

    npc->pflags |= 0x4;
    npc->InflictPain(1, 0);

    if (flg_info & 0x10)
    {
        npc->XZVecToPos(&dir_aim, &pos_bumper, NULL);
    }
    else
    {
        npc->XZVecToPlayer(&dir_aim, NULL);
    }

    flg_info = 0;

    xVec3Inv(&dir_aim, &dir_aim);
    xVec3Normalize(&dir_aim, &dir_aim);
    dir_aim.y = isin(DEG2RAD(30));
    xVec3Normalize(&dir_aim, &dir_aim);
    xVec3SMulBy(&dir_aim, 50.0f * cfg->npcMassInv);
    xVec3Add(&npc->frame->vel, &npc->frame->oldvel, &dir_aim);

    npc->frame->mode |= 0x4;
    npc->SndPlayRandom(NPC_STYP_OUCH);

    StreakPrep();

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalKnock::Exit(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->VelStop();
    StreakDone();

    if (!(flg_knock & 4))
    {
        npc->pflags &= 0xfb;
    }

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalKnock::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    if (this->flg_knock & 1)
    {
        this->floorBounce++;
    }
    if ((this->floorBounce > 3) || (this->flg_knock & 2) && !(this->flg_knock & 1) ||
        (npc->AnimTimeRemain(0) < (dt + 0.001f)))
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_AFTERLIFE;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        npc->FacePlayer(dt, 3 * PI);
        npc->colFreq = 0;
        this->flg_knock &= 0xFFFFFFFC;
        StreakUpdate();
        return xGoal::Process(trantype, dt, updCtxt, xscn);
    }
}

S32 zNPCGoalKnock::InputInfo(NPCDamageInfo* info)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    if (info->dmg_from != NULL)
    {
        NPCC_pos_ofBase(info->dmg_from, &pos_bumper);
        flg_info = 0x10;
    }
    else if (xVec3Length2(&info->vec_dmghit) > 0.001f)
    {
        xVec3Normalize(&pos_bumper, &info->vec_dmghit);
        xVec3Inv(&pos_bumper, &pos_bumper);
        xVec3AddTo(&pos_bumper, npc->Pos());
        flg_info = 0x10;
    }
    else
    {
        xVec3Copy(&pos_bumper, xEntGetPos(&globals.player.ent));
        flg_info = 0x10;
    }
    return flg_info;
}

S32 zNPCGoalKnock::CollReview(void*)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    NPCConfig* cfg = npc->cfg_npc;
    xEntCollis* npccol = npc->collis;
    xCollis* colrec = NULL;
    xVec3 vec_depen = { 0.0f, 0.0f, 0.0f };
    S32 hitstuff = 0;
    S32 i;
    xVec3 dir = { 0.0f, 0.0f, 0.0f };
    xSurface* surf;
    S32 badsurf = 0;
    F32 spd;
    xVec3 pump;
    F32 goodep = 0.0f;

    if (npccol->colls[0].flags & k_HIT_IT)
    {
        flg_knock |= 0x1;
    }

    for (i = npccol->env_sidx; i < npccol->env_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    for (i = npccol->stat_sidx; i < npccol->stat_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    for (i = npccol->dyn_sidx; i < npccol->dyn_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        hitstuff++;
        surf = zSurfaceGetSurface(colrec);

        if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
        {
            badsurf++;
        }
        else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
        {
            badsurf++;
        }
    }

    spd = 0.0f;

    if (npccol->npc_sidx < npccol->npc_eidx)
    {
        spd = xVec3Length(&npc->frame->vel);
    }

    for (i = npccol->npc_sidx; i < npccol->npc_eidx; i++)
    {
        colrec = &npccol->colls[i];

        xVec3AddTo(&vec_depen, &colrec->depen);
        zNPCCommon* tgt = (zNPCCommon*)(xEnt*)(colrec->optr);
        hitstuff++;

        if (tgt != NULL)
        {
            xVec3SMul(&pump, &colrec->hdng, spd);
            tgt->Damage(DMGTYP_HITBYTOSS, npc, &pump);
        }
    }

    if (badsurf)
    {
        flg_knock |= 0x8;
    }
    else if ((psyche->TimerGet(XPSY_TYMR_CURGOAL) > 0.1f) && hitstuff &&
             (xVec3Length2(&vec_depen) > 0.0f))
    {
        flg_knock |= 0x2;

        xVec3Copy(&dir, &npc->frame->oldvel);
        NPCC_Bounce(&dir, &vec_depen, cfg->fac_elastic);
        xVec3Copy(&npc->frame->vel, &dir);
    }

    return 1;
}

void zNPCGoalKnock::StreakPrep()
{
    streakID = NPCC_StreakCreate(NPC_STRK_TOSSEDROBOT);
}

void zNPCGoalKnock::StreakDone()
{
    xFXStreakStop(streakID);
    streakID = 0xDEAD;
}

void zNPCGoalKnock::StreakUpdate()
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    xVec3 a, b, c;

    xVec3Copy(&c, (xVec3*)NPCC_rightDir(npc));
    xVec3SMul(&c, &c, 0.1f);
    xVec3Copy(&a, (xVec3*)npc->Pos());
    a.y += 0.5f;
    b = a;
    xVec3Add(&a, &a, &c);
    xVec3Sub(&b, &b, &c);
    xFXStreakUpdate(this->streakID, &a, &b);
}

S32 zNPCGoalAfterlife::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    zNPCCommon* duper = npc->npc_duplodude;
    if (!(this->flg_deadinfo & 3))
    {
        DieTheGoodDeath();
    }
    npc->MatPosSet(&npc->entass->pos);
    xVec3Copy(&npc->frame->mat.pos, &npc->entass->pos);
    npc->frame->mode |= 1;
    if (duper)
    {
        if (this->flg_deadinfo & 2)
        {
            duper->DuploNotice(SM_NOTE_NPCDIED, npc);
        }
        duper->DuploNotice(SM_NOTE_NPCSTANDBY, npc);
    }

    return zNPCGoalDead::Enter(dt, updCtxt);
}

S32 zNPCGoalAfterlife::NPCMessage(NPCMsg* mail)
{
    S32 snarfed = 1;
    xPsyche* psy = GetPsyche();
    switch (mail->msgid)
    {
    case NPC_MID_RESPAWN:
    {
        if ((psy->GIDInStack(NPC_GOAL_RESPAWN) != NULL) ||
            (psy->GIDOfPending() == NPC_GOAL_RESPAWN))
        {
            break;
        }
        zNPCGoalRespawn* respgoal = ((zNPCGoalRespawn*)(psy->FindGoal(NPC_GOAL_RESPAWN)));
        if (respgoal == NULL)
        {
            break;
        }
        respgoal->InputInfo(&mail->spawning);
        psy->GoalPush(NPC_GOAL_RESPAWN, 0);
        mail->spawning.spawnSuccess = 1;
        break;
    }
    default:
        snarfed = 1;
        break;
    }
    return snarfed;
}

void CollectBountyOnRobot(S32 robotId);

void zNPCGoalAfterlife::DieTheGoodDeath()
{
    zNPCRobot* npc = (zNPCRobot*)this->psyche->clt_owner;
    zNPCCommon* duper = npc->npc_duplodude;
    zNPCMsg_AreaNotify(npc, NPC_MID_NPCDIED, 20.0f, 0x106, NPC_TYPE_UNKNOWN);
    npc->InflictPain(-1, TRUE);
    SetPlayerKillsVillainTimer(4.0f);
    CollectBountyOnRobot(npc->SelfType());
    xScrFXGlareAdd(npc->Pos(), 0.5, 0.2, 5.0, 1.0, 1.0, 1.0, 1.0, NULL);
    npc->SndPlayRandom(NPC_STYP_DEATH);
    if (npc->explosion && npc->explosion->initCB)
    {
        npc->explosion->initCB(npc->explosion, npc->model, NULL, NULL);

        static S32 cnt_nextfunfrag = 60;
        static S32 num_funFrag = 3;
        if (--cnt_nextfunfrag < 0)
        {
            cnt_nextfunfrag = 60;
            cnt_nextfunfrag += (S32)(60.0f * xurand());
            for (S32 i = 0; i < num_funFrag; i++)
            {
                NPCHazard* haz = HAZ_Acquire();
                if (haz != NULL)
                {
                    haz->ConfigHelper(NPC_HAZ_FUNFRAG);
                    haz->SetNPCOwner(npc);
                    haz->Start(xEntGetCenter(npc), -1.0f);
                }
            }
        }
    }
    if (duper)
    {
        duper->DuploNotice(SM_NOTE_NPCDIED, npc);
    }
}

S32 zNPCGoalRespawn::Enter(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    tmr_respawn = 0.35f;
    cnt_ring = 0;
    if (!(flg_info & 0x10))
    {
        xVec3Copy(&pos_poofHere, &npc->entass->pos);
        npc->zNPCCommon::GetParm(NPC_PARM_FIRSTMVPT, &npc->nav_curr);
    }
    xVec3Copy(npc->Pos(), &pos_poofHere);
    xVec3Copy(&npc->frame->mat.pos, &pos_poofHere);
    npc->frame->mode = 1;
    flg_info = 0;
    if (npc->npc_duplodude != 0)
    {
        tmr_robobits = LaunchRoboBits();
        xEntHide(npc);
    }
    else
    {
        tmr_robobits = -1.0f;
    }
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalRespawn::Exit(F32 dt, void* updCtxt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    KickFromTheNest();
    xEntShow(npc);
    zNPCCommon* duper = npc->npc_duplodude;
    if (duper != NULL)
    {
        duper->DuploNotice(SM_NOTE_NPCALIVE, npc);
    }
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalRespawn::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    if (tmr_respawn < 0.0f)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_ALERT;
    }
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    if ((tmr_robobits < 0.0f) ? 1 : 0)
    {
        if (!xEntIsVisible(npc))
        {
            npc->SndPlayRandom(NPC_STYP_RESPAWN);
            xEntShow(npc);
            npc->model->Flags |= 4;
            npc->model->Flags |= 2;
        }
        DoAppearFX(dt);
        tmr_respawn = MAX(-1.0f, (tmr_respawn - dt));
    }
    else
    {
        tmr_robobits = MAX(-1.0f, (tmr_robobits - dt));
    }
    npc->VelStop();
    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

F32 zNPCGoalRespawn::LaunchRoboBits()
{
    static const xVec3 vec_boneOffset = { 0.0f, 0.0f, 0.0f };

    NPCHazard* haz = HAZ_Acquire();
    if (haz == NULL)
    {
        return -1.0f;
    }

    if (!haz->ConfigHelper(NPC_HAZ_ROBOBITS))
    {
        haz->Discard();
        return -1.0f;
    }

    zNPCCommon* npc = (zNPCCommon*)(psyche->clt_owner);
    haz->SetNPCOwner(npc);

    zNPCCommon* duplo = npc->npc_duplodude;

    xVec3 pos_bone;
    pos_bone = *(const xVec3*)duplo->BonePos(11);
    pos_bone += vec_boneOffset;
    xMat3x3RMulVec(&pos_bone, (const xMat3x3*)duplo->BoneMat(0), &pos_bone);
    pos_bone += *(const xVec3*)duplo->BonePos(0);

    const xVec3 pos_tgt = pos_poofHere;
    xVec3 vec_toss = pos_tgt - pos_bone;

    F32 dst_toss = xVec3Length(&vec_toss);
    F32 tym_toss = dst_toss / 2.0f;

    if (dst_toss < 0.25f || tym_toss < 0.25f)
    {
        haz->Discard();
        return -1.0f;
    }

    F32 spd_toss = dst_toss / tym_toss;

    haz->custdata.tartar.pos_tgt = pos_tgt;
    haz->Start(&pos_bone, spd_toss);

    return spd_toss;
}

void zNPCGoalRespawn::DoAppearFX(F32 dt)
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    NPCConfig* cfg = npc->cfg_npc;
    F32 hyt;
    xVec3 pos_poof;
    xBound bnd;
    S32 rc;
    F32 fv;
    xVec3 vec_push;

    fv = MAX(0.0f, MIN(tmr_respawn / 0.35f, 1.0f));

    F32 scl = SMOOTH(1.0f - fv, 0.1f, 1.0f);

    npc->ModelScaleSet(scl, scl, scl);

    if (cfg->useBoxBound)
    {
        hyt = 0.5f * cfg->dim_bound.y + cfg->off_bound.y;
    }
    else
    {
        hyt = cfg->off_bound.y + cfg->off_bound.x;
    }

    xVec3Copy(&pos_poof, &pos_poofHere);

    memset(&bnd, 0, sizeof(xBound));
    bnd.type = 1;

    memset(&g_SharedCollisRecord, 0, sizeof(g_SharedCollisRecord));

    xCollis* colrec = &g_SharedCollisRecord;

    g_SharedCollisRecord.flags = k_HIT_0xF00 | k_HIT_CALC_HDNG;

    bnd.sph.r = 0.5f * hyt * (1.0f - fv);
    xVec3Copy(&bnd.sph.center, &pos_poof);

    rc = NPCC_chk_hitPlyr(&bnd, &g_SharedCollisRecord);

    if (rc != 0)
    {
        zEntPlayer_DamageNPCKnockBack(npc, 1, npc->Pos());
        colrec->depen.y = 0.0f;
        xVec3SMul(&vec_push, &colrec->depen, -1.0f);
        xVec3AddTo(xEntGetPos(&globals.player.ent), &vec_push);
    }

    if (npc->DBG_IsNormLog(eNPCDCAT_Eight, 2) != 0)
    {
        if (rc != 0)
        {
            xDrawSetColor(g_NEON_RED);
        }
        else
        {
            xDrawSetColor(g_NEON_BLUE);
        }

        xBoundDraw(&bnd);
    }

    pos_poof.y += (1.0f - fv) * hyt;

    F32 seg_ring[8] = { 0.875f, 0.75f, 0.625f, 0.5f, 0.375f, 0.25f, 12.5f, 0.0f };

    if (!(fv > seg_ring[cnt_ring]))
    {
        zFXPorterWave(&pos_poof);
        cnt_ring++;
    }

    fv = isin(PI * (4.0f * fv));

    xVec3 vel_poof = { 0.0f, 2.5f, 0.0f };

    npc->VFXStarTrek(dt, &pos_poof, &vel_poof);

    vel_poof.y -= 1.0f;
    pos_poof.x += fv;
    pos_poof.z += fv;

    npc->VFXStarTrek(dt, &pos_poof, &vel_poof);

    fv *= 2.0f;
    pos_poof.x -= fv;
    pos_poof.z -= fv;

    npc->VFXStarTrek(dt, &pos_poof, &vel_poof);
}

void zNPCGoalRespawn::KickFromTheNest()
{
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    zMovePoint* nav_preserveCurr = npc->nav_curr;
    zMovePoint* nav_preserveDest = npc->nav_dest;

    npc->Reset();
    npc->ModelScaleSet(0.0f);
    zNPCGoalAfterlife* wanna = (zNPCGoalAfterlife*)(psyche->FindGoal(NPC_GOAL_AFTERLIFE));
    if (wanna != NULL)
    {
        wanna->DieWithABang();
    }

    xVec3Copy(npc->Pos(), &pos_poofHere);
    xVec3Copy(&npc->frame->mat.pos, &pos_poofHere);

    npc->frame->mode = 1;

    npc->frame->dvel.x = 0.0f;
    npc->frame->dvel.y = 0.0f;
    npc->frame->dvel.z = 0.0f;

    npc->frame->vel.x = 0.0f;
    npc->frame->vel.y = 0.0f;
    npc->frame->vel.z = 0.0f;

    npc->frame->mode |= 0xc;

    npc->frame->dpos.x = 0.0f;
    npc->frame->dpos.y = 0.0f;
    npc->frame->dpos.z = 0.0f;

    npc->frame->mode |= 2;

    npc->nav_past = nav_preserveCurr;
    npc->nav_curr = nav_preserveCurr;
    npc->nav_dest = nav_preserveDest;
    npc->nav_lead = nav_preserveDest;

    npc->arena.SetHome(npc, nav_preserveCurr);
    npc->psy_instinct->GoalSet(NPC_GOAL_IDLE, 1);
}

S32 zNPCGoalRespawn::InputInfo(NPCSpawnInfo* info)
{
    zNPCRobot* npc = ((zNPCRobot*)(psyche->clt_owner));

    flg_info = 0x10;
    xVec3Copy(&pos_poofHere, &info->pos_spawn);

    if (info->nav_spawnReference != NULL)
    {
        npc->nav_curr = info->nav_spawnReference;
    }
    else
    {
        npc->nav_curr = info->nav_firstMovepoint;
    }

    npc->nav_dest = info->nav_firstMovepoint;
    return flg_info;
}

S32 zNPCGoalTubePal::Enter(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    zNPCTubelet* pete = npc->tub_pete;
    npc->ModelAtomicShow(0, NULL);
    npc->ModelAtomicHide(1, NULL);
    npc->ModelAtomicHide(4, NULL);
    npc->VelStop();
    if (pete != NULL)
    {
        xVec3Copy(npc->Pos(), pete->Pos());
    }
    if (npc->tubespot == ROBO_TUBE_MARY)
    {
        npc->model->Mat->pos.y += 3.0f;
    }
    else
    {
        npc->model->Mat->pos.y += 1.5f;
    }
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalTubePal::EvalRules(en_trantype* trantype, F32 dt, void* updCtxt)
{
    S32 nextgoal = 0;
    ChkPrelimTran(trantype, &nextgoal);
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        return xGoal::EvalRules(trantype, dt, updCtxt);
    }
}

void zNPCGoalTubePal::ChkPrelimTran(en_trantype* trantype, int* nextgoal)
{
    zNPCTubeSlave* npc = ((zNPCTubeSlave*)(psyche->clt_owner));

    switch (npc->tub_pete->tubestat)
    {
    case TUBE_STAT_BORN:
        *nextgoal = NPC_GOAL_TUBEBIRTH;
        *trantype = GOAL_TRAN_SET;
        return;
    case TUBE_STAT_DUCKLING:
        *nextgoal = NPC_GOAL_TUBEDUCKLING;
        *trantype = GOAL_TRAN_SET;
        return;
    case TUBE_STAT_ATTACK:
        *nextgoal = NPC_GOAL_TUBEATTACK;
        *trantype = GOAL_TRAN_SET;
        return;
    case TUBE_STAT_LASSO:
        *nextgoal = NPC_GOAL_TUBELASSO;
        *trantype = GOAL_TRAN_SET;
        return;
    case TUBE_STAT_DYING:
        *nextgoal = NPC_GOAL_TUBEDYING;
        *trantype = GOAL_TRAN_SET;
        return;
    case TUBE_STAT_DEAD:
        *nextgoal = NPC_GOAL_TUBEDEAD;
        *trantype = GOAL_TRAN_SET;
        return;
    }
}

S32 zNPCGoalTubeDuckling::Enter(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(this->psyche->clt_owner);
    this->tmr_running = 0.0f;
    this->tmr_hoverCycle = 0.0f;
    this->flg_duckling |= 3;
    this->tmr_outward = 1.0f;
    this->dst_preOrbit = npc->XZDstSqToPos(npc->tub_pete->Pos(), NULL, NULL);
    this->dst_preOrbit = xsqrt(this->dst_preOrbit);
    npc->VelStop();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalTubeDuckling::Resume(F32 dt, void* updCtxt)
{
    zNPCTubelet* npc = (zNPCTubelet*)(psyche->clt_owner);
    tmr_running = 0.0;
    tmr_hoverCycle = 0.0;
    flg_duckling |= 3;
    tmr_outward = 1.0;
    dst_preOrbit = npc->XZDstSqToPos(npc->tub_paul->Pos(), 0, 0);
    dst_preOrbit = xsqrt(dst_preOrbit);
    npc->VelStop();
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalTubeDuckling::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCTubelet* npc = (zNPCTubelet*)(psyche->clt_owner);
    ChkPrelimTran(trantype, &nextgoal);
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    else
    {
        if (npc->tub_paul->tubespot == ROBO_TUBE_MARY)
        {
            if (flg_duckling & 1)
            {
                DuckStackInterpInit();
                flg_duckling &= 0xFFFFFFFE;
            }
            if (DuckStackInterp(dt) == 0)
            {
                flg_duckling &= 0xFFFFFFFD;
            }
            tmr_outward = 1.0f;
            xVec3 vec = *npc->tub_paul->Pos() - npc->frame->mat.pos;
            vec.y = 0.0f;
            dst_preOrbit = xVec3Length(&vec);
        }
        else
        {
            flg_duckling |= 3;
            MoveFrolic(dt);
        }
        return xGoal::Process(trantype, dt, updCtxt, NULL);
    }
}

void zNPCGoalTubeDuckling::ChkPrelimTran(en_trantype* trantype, int* nextgoal)
{
    zNPCTubeSlave* npc = ((zNPCTubeSlave*)(psyche->clt_owner));

    switch (npc->tub_pete->tubestat)
    {
    case TUBE_STAT_BORN:
        *nextgoal = NPC_GOAL_TUBEBIRTH;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_ATTACK:
        if (flg_duckling & 2)
        {
            break;
        }
        *nextgoal = NPC_GOAL_TUBEATTACK;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_LASSO:
        *nextgoal = NPC_GOAL_TUBELASSO;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_DYING:
    case TUBE_STAT_DEAD:
        *nextgoal = NPC_GOAL_TUBEDYING;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_DUCKLING:
        break;
    }

    if (npc->hitpoints == 0)
    {
        *nextgoal = NPC_GOAL_TUBEBONKED;
        *trantype = GOAL_TRAN_PUSH;
    }
}

void zNPCGoalTubeDuckling::MoveFrolic(F32 dt)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    F32 ang_turnrate = DEG2RAD(60);
    xVec3 vec_offset;
    F32 dst_vert;
    F32 dst_horz;
    F32 factor;
    F32 vertDampen;
    F32 interval;
    F32 rat_inv;
    F32 amt;

    if (npc->tubespot == ROBO_TUBE_MARY)
    {
        ang_turnrate *= dt;
    }
    else
    {
        ang_turnrate *= -dt;
    }

    npc->frame->drot.angle = ang_turnrate;
    npc->frame->mode |= 0x20;

    dst_vert = 0.0f;
    dst_horz = dst_vert;
    interval = 4.0f;
    vertDampen = dst_vert;

    switch (npc->tubespot)
    {
    case ROBO_TUBE_PETE:
        break;
    case ROBO_TUBE_MARY:
        dst_vert = 3.0f;
        dst_horz = MIN(2.5f, npc->cfg_npc->spd_moveMax);
        interval = 5.6666665f;
        vertDampen = 1.0f;
        break;
    case ROBO_TUBE_PAUL:
        dst_vert = 1.5f;
        dst_horz = MIN(2.5f, npc->cfg_npc->spd_moveMax);
        interval = 5.0f;
        vertDampen = -1.0f;
        break;
    }

    rat_inv = 1.0f;

    if (!((tmr_outward < 0.0f) ? 1 : 0))
    {
        rat_inv = 1.0f - MAX(0.0f, MIN(tmr_outward, 1.0f));
        dst_horz = dst_preOrbit + SMOOTH(rat_inv, 0.0f, dst_horz - dst_preOrbit);
    }

    tmr_outward = MAX(-1.0f, tmr_outward - dt);

    NPCC_TmrCycle(&tmr_running, dt, interval);
    NPCC_TmrCycle(&tmr_hoverCycle, dt, 1.0f);

    amt = 2 * PI * (vertDampen * (tmr_running / interval));

    factor = isin(amt);
    F32 factor2 = isin(2.0f * amt);

    vec_offset = g_Z3 * (dst_horz * factor);
    vec_offset += g_X3 * (dst_horz * factor2);

    factor = isin(PI * tmr_hoverCycle);

    vec_offset.y = rat_inv * (factor * (0.35f * vertDampen)) + dst_vert;

    if (rat_inv < 1.0f)
    {
        F32 delta = npc->model->Mat->pos.y - (npc->tub_pete->model->Mat->pos.y + vec_offset.y);

        delta *= rat_inv;
        vec_offset.y -= delta;
    }

    npc->frame->mat.pos = *npc->tub_pete->Pos() + vec_offset;
    npc->frame->mode |= 0x1;
}

void zNPCGoalTubeDuckling::DuckStackInterpInit()
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    xVec3 dist;
    xVec3 pos_stacked;

    npc->PosStacked(&pos_stacked);
    xVec3Sub(&dist, npc->Pos(), &pos_stacked);
    dst_visacard = xVec3Length(&dist);
    if (dst_visacard < 0.001f)
    {
        dst_visacard = -1.0f;
        xVec3Copy(&dir_visacard, &g_O3);
    }
    else
    {
        xVec3Normalize(&dir_visacard, &dist);
    }
}

S32 zNPCGoalTubeDuckling::DuckStackInterp(F32 dt)
{
    S32 stillbusy = 0;
    zNPCTubeSlave* npc = (zNPCTubeSlave*)this->psyche->clt_owner;
    zNPCTubelet* pete = npc->tub_pete;

    F32 turnrate = MAX(npc->cfg_npc->spd_turnMax, pete->cfg_npc->spd_turnMax);

    turnrate *= 1.1f;

    npc->TurnToFace(dt, NPCC_faceDir(pete), turnrate);

    if (xVec3Dot(NPCC_faceDir(pete), NPCC_faceDir(npc)) < 0.9f)
    {
        stillbusy = 1;
    }

    xVec3 pos_desire;
    npc->PosStacked(&pos_desire);

    S32 snapped = (dst_visacard < 0.0f) ? 1 : 0;

    if (snapped)
    {
        xVec3Copy(&npc->frame->mat.pos, &pos_desire);
        stillbusy = 0;
        npc->frame->mode |= 1;
    }
    else
    {
        xVec3 pos_interp;

        xVec3SMul(&pos_interp, &dir_visacard, dst_visacard);
        xVec3AddTo(&pos_interp, &pos_desire);
        xVec3Copy(&npc->frame->mat.pos, &pos_interp);

        stillbusy++;

        npc->frame->mode |= 1;

        dst_visacard = MAX(-1.0f, dst_visacard - npc->cfg_npc->spd_moveMax * dt);
    }

    return stillbusy;
}

S32 zNPCGoalTubeAttack::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->VelStop();
    AttackDataReset();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalTubeAttack::Resume(F32 dt, void* updCtxt)
{
    zNPCTubelet* npc = ((zNPCTubelet*)(psyche->clt_owner));
    npc->VelStop();
    AttackDataReset();
    return zNPCGoalCommon::Resume(dt, updCtxt);
}

S32 zNPCGoalTubeAttack::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    ChkPrelimTran(trantype, &nextgoal);
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    npc->PosStacked(&npc->frame->mat.pos);
    npc->frame->mode |= 1;

    if (npc->tubespot == ROBO_TUBE_MARY)
    {
        MaryAttack(dt, xscn);
    }
    else
    {
        npc->FacePlayer(dt, 3 * PI);
    }
    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

void zNPCGoalTubeAttack::ChkPrelimTran(en_trantype* trantype, int* nextgoal)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);

    switch (npc->tub_pete->tubestat)
    {
    case TUBE_STAT_BORN:
        *nextgoal = NPC_GOAL_TUBEBIRTH;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_DUCKLING:
        *nextgoal = NPC_GOAL_TUBEDUCKLING;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_LASSO:
        *nextgoal = NPC_GOAL_TUBELASSO;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_DYING:
    case TUBE_STAT_DEAD:
        *nextgoal = NPC_GOAL_TUBEDYING;
        *trantype = GOAL_TRAN_SET;
        break;
    }

    if (*trantype != 0)
    {
        return;
    }

    if (npc->hitpoints != 0)
    {
        return;
    }

    *nextgoal = NPC_GOAL_TUBEBONKED;
    *trantype = GOAL_TRAN_PUSH;
}

void zNPCGoalTubeAttack::LaserRender()
{
    zNPCTubeSlave::laser.Render(&paul.pos_laserSource, &paul.pos_laserTarget);
}

void zNPCGoalTubeAttack::MaryAttack(F32 dt, xScene* xscn)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    zNPCTubelet* pete = npc->tub_pete;
    zNPCTubeSlave* paul = pete->tub_paul;

    switch (mary.marystat)
    {
    case TUBE_MARY_WAIT:
        if ((pete->hitpoints == 0) || (paul->hitpoints == 0))
        {
            mary.marystat = TUBE_MARY_ANGRY;
            npc->SndPlayRandom(NPC_STYP_WARNBANG);
        }
        else
        {
            npc->FacePlayer(dt, 2 * PI);
        }
        break;
    case TUBE_MARY_ANGRY:
        if (MarySpinUp(dt) != 0)
        {
            MaryzFury();
            MaryzBlessing();
            mary.marystat = TUBE_MARY_COOLOFF;
        }
        break;
    case TUBE_MARY_COOLOFF:
        if (MarySpinDown(dt) != 0)
        {
            mary.marystat = TUBE_MARY_WAIT;
        }
        break;
    }
}

S32 zNPCGoalTubeAttack::MarySpinUp(F32 dt)
{
    S32 retval = 0;
    zNPCRobot* npc = (zNPCRobot*)(psyche->clt_owner);
    npc->DBG_IsNormLog(eNPCDCAT_Thirteen, -1);
    if (mary.ang_spinrate > (8 * PI))
    {
        retval = 1;
    }
    else
    {
        mary.ang_spinrate += (2 * PI) * dt;
    }
    npc->frame->drot.angle = (dt * mary.ang_spinrate);
    npc->frame->mode |= 0x20;
    return retval;
}

S32 zNPCGoalTubeAttack::MarySpinDown(F32 dt)
{
    zNPCTubelet* npc = ((zNPCTubelet*)(psyche->clt_owner));

    S32 ret = 0;

    if (mary.ang_spinrate < npc->cfg_npc->spd_turnMax)
    {
        ret = 1;
    }
    else
    {
        mary.ang_spinrate = -((2 * PI) * dt - mary.ang_spinrate);
    }

    npc->frame->drot.angle = dt * mary.ang_spinrate;
    npc->frame->mode |= 0x20;
    return ret;
}

void zNPCGoalTubeAttack::MaryzFury()
{
    zNPCTubelet* npc = *(zNPCTubelet**)(&psyche->clt_owner);
    NPCHazard* haz = HAZ_Acquire();
    if (haz != NULL)
    {
        haz->ConfigHelper(NPC_HAZ_TUBELETBLAST);
        haz->SetNPCOwner(npc);
        xVec3* center = xEntGetCenter(npc);
        haz->Start(center, -1.0f);
        U32 sndID = xStrHash("Tube_pop");
        xSndPlay3D(sndID, 0.77f, 0.0f, 0x80, 0, &haz->pos_hazard, 5.0f, 15.0f, SND_CAT_GAME, 0.0f);
    }
}

void zNPCGoalTubeAttack::MaryzBlessing()
{
    zNPCTubelet* npc = ((zNPCTubelet*)(psyche->clt_owner));
    npc->hitpoints = 1;
    npc->tub_paul->hitpoints = 1;
    npc->tub_paul->tub_pete->hitpoints = 1;
}

void zNPCGoalTubeAttack::AttackDataReset()
{
    zNPCTubelet* npc = ((zNPCTubelet*)(psyche->clt_owner));
    flg_attack = 0;
    if (npc->tubestat == TUBE_STAT_ATTACK)
    {
        mary.marystat = TUBE_MARY_WAIT;
        mary.ang_spinrate = 0.0;
    }
    else
    {
        paul.flg_paul = 1;
        paul.cnt_nextlos = -1;
        paul.len_laser = 10.0f;
    }
}

S32 zNPCGoalTubeLasso::Enter(F32 dt, void* updCtxt)
{
    zNPCCommon* npc = ((zNPCCommon*)(psyche->clt_owner));
    npc->VelStop();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalTubeLasso::Exit(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = ((zNPCTubeSlave*)(psyche->clt_owner));

    zNPCGoalTubeDying* tubedie = (zNPCGoalTubeDying*)psyche->FindGoal(NPC_GOAL_TUBEDYING);
    tubedie->DeathByLasso(npc->Pos());

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalTubeLasso::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene)
{
    S32 nextgoal = 0;
    ChkPrelimTran(trantype, &nextgoal);
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    MoveTryToEscape(dt);
    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

void zNPCGoalTubeLasso::ChkPrelimTran(en_trantype* trantype, int* nextgoal)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);

    switch (npc->tub_pete->tubestat)
    {
    case TUBE_STAT_BORN:
        *nextgoal = NPC_GOAL_TUBEBIRTH;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_DUCKLING:
        *nextgoal = NPC_GOAL_TUBEDUCKLING;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_ATTACK:
        *nextgoal = NPC_GOAL_TUBEATTACK;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_DYING:
    case TUBE_STAT_DEAD:
        *nextgoal = NPC_GOAL_TUBEDYING;
        *trantype = GOAL_TRAN_SET;
        break;
    }

    // ?????
    if (*trantype == GOAL_TRAN_NONE)
    {
        return;
    }
}

void zNPCGoalTubeLasso::MoveTryToEscape(F32 dt)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    zNPCTubelet* pete = npc->tub_pete;
    F32 side;

    if (npc->tubespot == ROBO_TUBE_MARY)
    {
        side = 1.0f;
    }
    else
    {
        side = -1.0f;
    }

    static F32 dst_tetherMax = xsqrt(SQ(2.5f) + SQ(2.5f));

    xVec3 pos_tether = *pete->Pos();

    pos_tether += *NPCC_rightDir(pete) * 2.5f * side;
    pos_tether += *NPCC_upDir(pete) * 2.5f;

    xVec3 dir_tether;

    npc->XYZVecToPos(&dir_tether, &pos_tether);

    F32 dst = xVec3Normalize(&dir_tether, &dir_tether);
    F32 spd_step = 3.5f * dt;

    if (spd_step > dst)
    {
        xVec3Copy(&npc->frame->mat.pos, &pos_tether);
        npc->frame->mode |= 0x1;
    }
    else if (dst > dst_tetherMax)
    {
        xVec3SMul(&npc->frame->dpos, &dir_tether, spd_step);

        xVec3 diff = pos_tether - dir_tether * dst_tetherMax;
        xVec3 push = diff - *npc->Pos();

        xVec3AddTo(&npc->frame->dpos, &push);
        npc->frame->mode |= 0x2;
    }
    else
    {
        xVec3SMul(&npc->frame->dpos, &dir_tether, spd_step);
        npc->frame->mode |= 0x2;
    }

    npc->TurnToFace(dt, &dir_tether, -1.0f);
}

S32 zNPCGoalTubeBirth::Enter(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = ((zNPCTubeSlave*)(psyche->clt_owner));
    npc->hitpoints = npc->cfg_npc->pts_damage;
    npc->VelStop();
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalTubeBirth::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene)
{
    S32 nextgoal = 0;
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    zNPCTubelet* pete = npc->tub_pete;
    ChkPrelimTran(trantype, &nextgoal);
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    npc->PosStacked(&npc->frame->mat.pos);
    npc->frame->mode |= 1;
    xMat3x3Copy(&npc->frame->mat, (xMat3x3*)&pete->model->Mat);
    npc->frame->mode |= 0x40;
    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

void zNPCGoalTubeBirth::ChkPrelimTran(en_trantype* trantype, int* nextgoal)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);

    switch (npc->tub_pete->tubestat)
    {
    case TUBE_STAT_DUCKLING:
        *nextgoal = NPC_GOAL_TUBEDUCKLING;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_ATTACK:
        *nextgoal = NPC_GOAL_TUBEATTACK;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_LASSO:
        *nextgoal = NPC_GOAL_TUBELASSO;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_DYING:
    case TUBE_STAT_DEAD:
        *nextgoal = NPC_GOAL_TUBEDYING;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_BORN:
        break;
    }

    if (npc->hitpoints != 0)
    {
        return;
    }

    *nextgoal = NPC_GOAL_TUBEBONKED;
    *trantype = GOAL_TRAN_PUSH;
}

S32 zNPCGoalTubeBonked::Enter(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(this->psyche->clt_owner);
    npc->ModelAtomicHide(0, NULL);
    npc->ModelAtomicShow(1, NULL);
    npc->ModelAtomicHide(4, NULL);
    npc->hitpoints = 0;
    npc->VelStop();
    npc->tub_pete->PainInTheBand();
    this->tmr_recover = 3.0f;
    F32 spinrate = ((xurand() - 0.5f) * 2.0f * (3 * PI) + (5 * PI));
    this->ang_spinrate = -spinrate;
    npc->XYZVecToPos(&this->vec_offsetPete, npc->tub_pete->Pos());
    npc->SndPlayRandom(NPC_STYP_BONKED);
    npc->ShowerConfetti(NULL);
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalTubeBonked::Exit(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = ((zNPCTubeSlave*)(psyche->clt_owner));
    npc->ModelAtomicShow(0, NULL);
    npc->ModelAtomicHide(1, NULL);
    npc->ModelAtomicHide(4, NULL);
    npc->ShowerConfetti(NULL);
    npc->SndPlayRandom(NPC_STYP_UNBONKED);
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalTubeBonked::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);

    if (npc->tubespot == ROBO_TUBE_MARY)
    {
        if (tmr_recover < 0.0f)
        {
            npc->tub_pete->Unbonk();
            npc->tub_pete->tub_paul->hitpoints = 1;
            npc->tub_pete->tub_mary->hitpoints = 1;
        }

        tmr_recover = MAX(-1.0f, tmr_recover - dt);
    }

    npc->frame->drot.angle = dt * ang_spinrate;
    npc->frame->mode |= 0x20;

    xVec3 pos_tobe = *npc->tub_pete->Pos() - vec_offsetPete;

    xVec3Copy(&npc->frame->mat.pos, &pos_tobe);
    npc->frame->mode |= 0x1;

    xVec3 dir_want = { 0.0f, 0.0f, 0.0f };

    dir_want.x = vec_offsetPete.x;
    dir_want.z = vec_offsetPete.z;

    F32 dst = dir_want.length();

    if (dst < dt)
    {
        vec_offsetPete.x = 0.0f;
        vec_offsetPete.z = 0.0f;
    }
    else
    {
        dir_want *= -dt / dst;
        vec_offsetPete += dir_want;
    }

    ang_spinrate *= 0.99f;

    CheckForTran(trantype, &nextgoal);

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

void zNPCGoalTubeBonked::CheckForTran(en_trantype* trantype, S32* nextgoal)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);

    switch (npc->tub_pete->tubestat)
    {
    case TUBE_STAT_BORN:
        if (npc->hitpoints != 0)
        {
            *nextgoal = NPC_GOAL_TUBEBIRTH;
            *trantype = GOAL_TRAN_SET;
        }
        break;
    case TUBE_STAT_DUCKLING:
        if (npc->hitpoints != 0)
        {
            *nextgoal = NPC_GOAL_TUBEDUCKLING;
            *trantype = GOAL_TRAN_SET;
        }
        break;
    case TUBE_STAT_ATTACK:
        if (npc->hitpoints != 0)
        {
            *nextgoal = NPC_GOAL_TUBEATTACK;
            *trantype = GOAL_TRAN_SET;
        }
        break;
    case TUBE_STAT_LASSO:
        *nextgoal = NPC_GOAL_TUBELASSO;
        *trantype = GOAL_TRAN_SET;
        break;
    case TUBE_STAT_DYING:
    case TUBE_STAT_DEAD:
        *nextgoal = NPC_GOAL_TUBEDYING;
        *trantype = GOAL_TRAN_SET;
        break;
    }
}

S32 zNPCGoalTubeDead::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    zNPCTubelet* pete = npc->tub_pete;
    ChkPrelimTran(trantype, &nextgoal);
    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }
    npc->PosStacked(&npc->frame->mat.pos);
    npc->frame->mode |= 1;
    xMat3x3Copy(&npc->frame->mat, (xMat3x3*)&pete->model->Mat);
    npc->frame->mode |= 0x40;
    return xGoal::Process(trantype, dt, updCtxt, NULL);
}

void zNPCGoalTubeDead::ChkPrelimTran(en_trantype* trantype, int* nextgoal)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);

    switch (npc->tub_pete->tubestat)
    {
    case TUBE_STAT_BORN:
        *nextgoal = NPC_GOAL_TUBEBIRTH;
        *trantype = GOAL_TRAN_SET;
        return;
    case TUBE_STAT_DUCKLING:
        *nextgoal = NPC_GOAL_TUBEDUCKLING;
        *trantype = GOAL_TRAN_SET;
        return;
    case TUBE_STAT_ATTACK:
        *nextgoal = NPC_GOAL_TUBEATTACK;
        *trantype = GOAL_TRAN_SET;
        return;
    case TUBE_STAT_LASSO:
        *nextgoal = NPC_GOAL_TUBELASSO;
        *trantype = GOAL_TRAN_SET;
        return;
    }
    return;
}

S32 zNPCGoalTubeDying::Enter(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    xModelInstance* mdl_body = npc->ModelAtomicHide(0, NULL);
    npc->ModelAtomicHide(1, NULL);
    xModelInstance* mdl_wig = npc->ModelAtomicShow(4, NULL);
    if (flg_tubedying & 1)
    {
        xVec3Copy((xVec3*)(&mdl_wig->Mat->pos), &pos_lassoDeath);
    }
    else
    {
        mdl_wig->Mat->pos = mdl_body->Mat->pos;
    }
    flg_tubedying &= 0xFFFFFFFE;
    xVec3Copy(&npc->frame->mat.pos, (xVec3*)(&mdl_wig->Mat->pos));
    npc->frame->mode |= 1;
    mdl_wig->Mat->pos.y += 0.5f;
    spd_gothatway = 0.0f;
    cnt_loop = 2;
    hyt_was = mdl_wig->Mat->pos.y;
    scl_shrink = 1.0f;
    npc->flags2.flg_colCheck = 0;
    npc->flags2.flg_penCheck = 0;
    npc->chkby = 0;
    npc->penby = 0;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalTubeDying::Exit(F32 dt, void* updCtxt)
{
    zNPCTubeSlave* npc = ((zNPCTubeSlave*)(psyche->clt_owner));
    xModelInstance* mdl_wig = npc->ModelAtomicFind(4, -1, NULL);

    mdl_wig->Scale.assign(1.0f);
    npc->RestoreColFlags();

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalTubeDying::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCTubeSlave* npc = (zNPCTubeSlave*)(psyche->clt_owner);
    F32 tymr_ingoal;
    S32 goleft;
    xVec3 pos_emit;

    if (npc->AnimTimeRemain(NULL) < (0.001f + dt))
    {
        cnt_loop--;

        if (cnt_loop < 1)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_TUBEDEAD;
        }
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    npc->frame->vel = *NPCC_faceDir(npc) * spd_gothatway;
    npc->frame->vel.y = 1.5f;
    npc->frame->mode |= 0x4;

    tymr_ingoal = xfmod(psyche->TimerGet(XPSY_TYMR_CURGOAL), 1.0f);

    if (npc->tubespot == ROBO_TUBE_MARY)
    {
        if (tymr_ingoal > 0.46f)
        {
            goleft = 0;
        }
        else if (tymr_ingoal > 0.23f)
        {
            goleft = 1;
        }
        else
        {
            goleft = 2;
        }
    }
    else
    {
        if (tymr_ingoal > 0.51f)
        {
            goleft = 1;
        }
        else
        {
            goleft = 0;
        }
    }

    if (goleft)
    {
        npc->frame->drot.angle = 4 * PI * -dt;
    }
    else
    {
        npc->frame->drot.angle = 4 * PI * dt;
    }

    npc->frame->mode |= 0x20;

    xModelInstance* mdl_wig = npc->ModelAtomicFind(4, -1, NULL);
    mdl_wig->Scale.assign(scl_shrink);

    scl_shrink -= 0.5f * dt;
    scl_shrink = MAX(0.15f, scl_shrink);

    spd_gothatway += 10.0f * dt;
    spd_gothatway = MIN(spd_gothatway, 10.0f);

    npc->GetVertPos(NPC_MDLVERT_PROPEL, &pos_emit);
    zFX_SpawnBubbleTrail(&pos_emit, 1);

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

void zNPCGoalTubeDying::DeathByLasso(const xVec3* vec)
{
    flg_tubedying |= 1;
    pos_lassoDeath = *vec;
}

S32 zNPCGoalDeflate::Enter(F32 dt, void* updCtxt)
{
    zNPCTubelet* npc = ((zNPCTubelet*)(this->psyche->clt_owner));

    npc->ModelAtomicHide(0, NULL);
    xModelInstance* mdl_flot = npc->ModelAtomicHide(1, NULL);
    xModelInstance* mdl_wig = npc->ModelAtomicShow(4, NULL);
    mdl_wig->Mat->pos = mdl_flot->Mat->pos;
    mdl_wig->Mat->pos.y += 0.5f;

    this->spd_gothatway = 0.0f;
    this->cnt_loop = 2;
    this->scl_shrink = 1.0f;
    npc->flags2.flg_colCheck = 0;
    npc->flags2.flg_penCheck = 0;
    npc->chkby = 0;
    npc->penby = 0;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalDeflate::Exit(F32 dt, void* updCtxt)
{
    zNPCTubelet* npc = ((zNPCTubelet*)(this->psyche->clt_owner));
    xModelInstance* mdl_wig = npc->ModelAtomicShow(4, NULL);

    mdl_wig->Scale.assign(1.0f);
    npc->RestoreColFlags();

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalDeflate::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    S32 nextgoal = 0;
    zNPCTubelet* npc = (zNPCTubelet*)(psyche->clt_owner);
    F32 tymr_ingoal;
    xVec3 pos_emit;

    if (npc->AnimTimeRemain(NULL) < (0.001f + dt))
    {
        cnt_loop--;

        if (cnt_loop < 1)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = NPC_GOAL_AFTERLIFE;
        }
    }

    if (*trantype != GOAL_TRAN_NONE)
    {
        return nextgoal;
    }

    xVec3SMul(&npc->frame->vel, NPCC_faceDir(npc), spd_gothatway);
    npc->frame->vel.y = 1.5f;
    npc->frame->mode |= 0x4;

    tymr_ingoal = xfmod(psyche->TimerGet(XPSY_TYMR_CURGOAL), 1.0f);

    S32 goleft;

    if (tymr_ingoal > 0.47f)
    {
        goleft = 1;
    }
    else
    {
        goleft = 1;
    }

    if (goleft)
    {
        npc->frame->drot.angle = 3 * PI * -dt;
    }
    else
    {
        npc->frame->drot.angle = 3 * PI * dt;
    }

    npc->frame->mode |= 0x20;

    xModelInstance* mdl_wig = npc->ModelAtomicShow(4, NULL);
    mdl_wig->Scale.assign(scl_shrink);

    scl_shrink -= 0.5f * dt;
    scl_shrink = MAX(0.15f, scl_shrink);

    spd_gothatway += 10.0f * dt;
    spd_gothatway = MIN(spd_gothatway, 10.0f);

    npc->GetVertPos(NPC_MDLVERT_PROPEL, &pos_emit);
    zFX_SpawnBubbleTrail(&pos_emit, 1);

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

static RoboCopMap g_map_policeCounter[17] = {
    // clang-format off
    { NPC_TYPE_HAMMER, ROBOCOP_CNTR_HAMMER },
    { NPC_TYPE_HAMSPIN, ROBOCOP_CNTR_HAMMER },
    { NPC_TYPE_TARTAR, ROBOCOP_CNTR_TARTAR },
    { NPC_TYPE_GLOVE, ROBOCOP_CNTR_GLOVE },
    { NPC_TYPE_MONSOON, ROBOCOP_CNTR_MONSOON },
    { NPC_TYPE_SLEEPY, ROBOCOP_CNTR_SLEEPY },
    { NPC_TYPE_ARFDOG, ROBOCOP_CNTR_ARFDOG },
    { NPC_TYPE_ARFARF, ROBOCOP_CNTR_ARFARF },
    { NPC_TYPE_CHUCK, ROBOCOP_CNTR_CHUCK },
    { NPC_TYPE_TUBELET, ROBOCOP_CNTR_TUBELET },
    { NPC_TYPE_TUBESLAVE, ROBOCOP_CNTR_TUBELET },
    { NPC_TYPE_SLICK, ROBOCOP_CNTR_SLICK },
    { NPC_TYPE_FODDER, ROBOCOP_CNTR_FODDER },
    { NPC_TYPE_FODBOMB, ROBOCOP_CNTR_FODBOMB },
    { NPC_TYPE_FODBZZT, ROBOCOP_CNTR_FODBZZT },
    { NPC_TYPE_CHOMPER, ROBOCOP_CNTR_CHOMPER },
    // clang-format on
};

S32 RoboToCntrIdx(S32 robotId)
{
    S32 res = ROBOCOP_CNTR_FORCE;

    // NOTE: 'cur' starts at the head of the table while 'i' starts at zero, so
    // entry zero is examined twice before the scan moves on. Harmless, but it
    // is what retail does.
    S32 i = 0;
    RoboCopMap* cur = g_map_policeCounter;

    while (cur->ntyp_robotype != 0)
    {
        RoboCopMap* here = cur;

        cur = &g_map_policeCounter[i++];

        if (here->ntyp_robotype == robotId)
        {
            res = here->idx_copCounter;
            break;
        }
    }

    return res;
}

static _xCounter* g_cntr_policeLineup[15] = {};

void CollectBountyOnRobot(S32 robotId)
{
    S32 cntrIdx = RoboToCntrIdx(robotId);

    if (cntrIdx >= 0 && cntrIdx < 15)
    {
        _xCounter* counter = g_cntr_policeLineup[cntrIdx];
        if (counter && counter->count == 0)
        {
            counter->count = 1;
        }
    }
}

void ROBO_PrepRoboCop()
{
    char name[40];
    U8 tag;
    S32 i;

    strcpy(name, "HB09 ROBOT COUNTER 01");

    tag = '1';
    for (i = 0; i < 15; i++)
    {
        if (tag > '9')
        {
            tag = '0';
            name[19]++;
        }
        name[20] = tag;
        tag++;
        g_cntr_policeLineup[i] = (_xCounter*)zSceneFindObject(xStrHash(name));
    }
}

// .text (113c)

void zNPCGoalDogLaunch::SilentSwimout(xVec3* unk1, xVec3* unk2, zMovePoint* unk3)
{
    this->ViciousAttack(unk1, unk2, unk3, 1);
}

S32 zNPCGoalPatThrow::CollReview(void*)
{
    return 0;
}

S32 zNPCGoalDogLaunch::CollReview(void*)
{
    return 0;
}

// .text (38)

void zNPCGoalDead::DieWithAWhimper()
{
    flg_deadinfo &= ~1;
    flg_deadinfo |= 2;
}

void zNPCGoalDead::DieWithABang()
{
    flg_deadinfo &= ~1;
    flg_deadinfo &= ~2;
}

// .text (18)

void xGoal::AddFlags(S32 flags)
{
    this->flg_able |= flags;
}

xPsyche* xGoal::GetPsyche() const
{
    return psyche;
}

// .text (130)

void zNPCCommon::XZVecToPlayer(xVec3* unk1, F32* unk2)
{
    XZVecToPos(unk1, xEntGetPos(&globals.player.ent), unk2);
}

RwMatrix* zNPCCommon::BoneMat(S32 unk) const
{
    return &this->model->Mat[unk];
}

RwV3d* zNPCCommon::BonePos(S32 unk) const
{
    return &this->model->Mat[unk].pos;
}

F32 zNPCCommon::XYZDstSqToPlayer(xVec3* unk)
{
    return XYZDstSqToPos(xEntGetPos(&globals.player.ent), unk);
}

void zNPCCommon::DuploNotice(en_SM_NOTICES, void*)
{
}

xVec3* zNPCCommon::Center()
{
    return xEntGetCenter(this);
}

void zNPCCommon::ModelScaleSet(F32 unk)
{
    ModelScaleSet(unk, unk, unk);
}

// .text (4c)

void NPCC_DrawPlayerPredict(S32, F32, F32)
{
}

void NPCLaser::ColorSet(const RwRGBA* unk1, const RwRGBA* unk2)
{
    rgba[0] = *unk1;
    rgba[1] = *unk2;
}

// .text (4)

void xDrawCyl(const xVec3*, F32, F32, U32)
{
}

// .text (178)

F32 NPCArena::Radius(F32 unk)
{
    return unk * rad_arena;
}

xVec3* NPCArena::Pos()
{
    return &pos_arena;
}

void NPCBattle::JoinBattle(zNPCRobot*)
{
}

S32 NPCArena::IncludesPlayer(F32 rad_thresh, xVec3* vec)
{
    if (NPCC_LampStatus())
    {
        xVec3* pos = xEntGetPos(&globals.player.ent);
        return NPCArena::IncludesPos(pos, rad_thresh, vec);
    }

    return 0;
}

NPCGlyph* GLYF_Acquire(en_npcglyph);

S32 NPCArena::IsReady()
{
    return rad_arena > 0.0f;
}

void NPCBattle::LeaveBattle(zNPCRobot*)
{
}

S32 NPCArena::IncludesNPC(zNPCCommon* npc, float dt, xVec3* vec)
{
    xVec3* pos = npc->Pos();
    return IncludesPos(pos, dt, vec);
}

F32 zNPCRobot::FacePlayer(F32 dt, F32 spd_turn)
{
    xVec3* pos = xEntGetPos(&globals.player.ent);
    return FacePos(pos, dt, spd_turn);
}

void NPCArena::DBG_Draw(zNPCCommon*)
{
}

zMovePoint* zNPCArfArf::GetTelepoint(S32 unk)
{
    return nav_dest;
}

// .text (28)

xVec3& xVec3::assign(float dt)
{
    return assign(dt, dt, dt);
}

// .text (18)

inline void NPCHazard::SetNPCOwner(zNPCCommon* owner)
{
    this->npc_owner = owner;
}

inline void NPCHazard::NotifyCBSet(HAZNotify* noter)
{
    this->cb_notify = noter;
}

inline S32 HAZNotify::Notify(en_haznote note, NPCHazard* haz)
{
    return 0;
}
