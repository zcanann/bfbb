#include <types.h>

#include "xVec3.h"
#include "xMath3.h"

#include "zGlobals.h"
#include "zNPCGoalAmbient.h"
#include "zNPCSndLists.h"
#include "zNPCTypeAmbient.h"
#include "zNPCTypes.h"
#include <xutil.h>
#include "macros.h"
#include "xMathInlines.h"
#include "zNPCGoals.h"
#include "zGrid.h"
#include "zLightning.h"
#include "zNPCSupplement.h"

U32 g_hash_ambianim[12] = { 0 };
char* g_strz_ambianim[12] = {
    "Unknown",  "Idle01", "Idle02",   "Idle03",  "Fidget01", "Fidget02",
    "Fidget03", "Move01", "Bumped01", "Dance01", "Pray01",   "Attack01",
};
extern zGlobals globals;

zNPCAmbient::zNPCAmbient(S32 myType) : zNPCCommon(myType)
{
}

zNPCJelly::zNPCJelly(S32 myType) : zNPCAmbient(myType)
{
}

zNPCNeptune::zNPCNeptune(S32 myType) : zNPCAmbient(myType)
{
}

zNPCMimeFish::zNPCMimeFish(S32 myType) : zNPCAmbient(myType)
{
}

void ZNPC_Ambient_Startup()
{
    S32 i = 0;

    do
    {
        g_hash_ambianim[i] = xStrHash(g_strz_ambianim[i]);
        i++;
    } while (i < 12); // using sizeof makes it not match
}

void ZNPC_Ambient_Shutdown()
{
}

xFactoryInst* ZNPC_Create_Ambient(S32 who, RyzMemGrow* grow, void*)
{
    zNPCAmbient* inst = NULL;

    switch (who)
    {
    case NPC_TYPE_AMBIENT:
    {
        inst = new (who, grow) zNPCAmbient(who);
        break;
    }
    case NPC_TYPE_JELLYPINK:
    case NPC_TYPE_JELLYBLUE:
    {
        inst = new (who, grow) zNPCJelly(who);
        break;
    }
    case NPC_TYPE_KINGNEPTUNE:
    {
        inst = new (who, grow) zNPCNeptune(who);
        break;
    }
    case NPC_TYPE_MIMEFISH:
    {
        inst = new (who, grow) zNPCMimeFish(who);
        break;
    }
    case NPC_TYPE_COW:
    {
        inst = new (who, grow) zNPCMimeFish(who);
        break;
    }
    }

    return inst;
}

void ZNPC_Destroy_Ambient(xFactoryInst* inst)
{
    delete inst;
}

xAnimTable* ZNPC_AnimTable_Ambient()
{
    xAnimTable* table = (xAnimTable*)xAnimTableNew("zNPCAmbient", NULL, 0);
    xAnimTableNewState(table, g_strz_ambianim[1], 0x110, 1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    return table;
}

xAnimTable* ZNPC_AnimTable_Jelly()
{
    xAnimTable* table = (xAnimTable*)xAnimTableNew("zNPCJelly", NULL, 0);

    S32 ourAnims[] = {
        1, 7, 4, 8, 11, 0,
    };

    xAnimTableNewState(table, g_strz_ambianim[1], 0x110, 1, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[7], 0x110, 1, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[4], 0x20, 1, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[8], 0x10, 1, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[11], 0x10, 1, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);

    NPCC_BuildStandardAnimTran(table, g_strz_ambianim, ourAnims, 1, 0.2f);

    return table;
}

xAnimTable* ZNPC_AnimTable_Neptune()
{
    S32 ourAnims[] = {
        1, 2, 3, 4, 5, 6, 0,
    };

    xAnimTable* table = (xAnimTable*)xAnimTableNew("zNPCNeptune", NULL, 0);

    xAnimTableNewState(table, g_strz_ambianim[1], 0x10, 0, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[2], 0x10, 0, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[3], 0x10, 0, 1.0f, 0x0, 0x0, 0.0f, 0x0, 0x0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[4], 0x20, 0, 1.0f, 0x0, 0x0, 0.0f, 0x0, 0x0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[5], 0x20, 0, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_ambianim[6], 0x20, 0, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    NPCC_BuildStandardAnimTran(table, g_strz_ambianim, ourAnims, 1, 0.5f);

    xAnimTableNewTransition(table, g_strz_ambianim[4], g_strz_ambianim[1], 0x0, 0x0, 0x10, 0, 0.0f,
                            0.0f, 0, 0, 0.5f, 0x0);
    xAnimTableNewTransition(table, g_strz_ambianim[5], g_strz_ambianim[2], 0x0, 0x0, 0x10, 0, 0.0f,
                            0.0f, 0, 0, 0.5f, 0x0);
    xAnimTableNewTransition(table, g_strz_ambianim[6], g_strz_ambianim[3], 0x0, 0x0, 0x10, 0, 0.0f,
                            0.0f, 0, 0, 0.5f, 0x0);

    return table;
}

void zNPCAmbient::Init(xEntAsset* asset)
{
    zNPCCommon::Init(asset);
    if (cfg_npc->dst_castShadow < 0.0f)
    {
        cfg_npc->dst_castShadow = 1.0f;
    }
}

void zNPCAmbient::Reset()
{
    zNPCCommon::Reset();
    if (psy_instinct != NULL)
    {
        psy_instinct->GoalSet('NGN0', 1);
    }
}

void zNPCAmbient::Process(xScene* xscn, F32 dt)
{
    if (psy_instinct != NULL)
    {
        psy_instinct->Timestep(dt, NULL);
    }
    zNPCCommon::Process(xscn, dt);
}

void zNPCAmbient::SelfSetup()
{
    xBehaveMgr* bmgr;
    xPsyche* psy;

    bmgr = xBehaveMgr_GetSelf();
    psy_instinct = bmgr->Subscribe(this, 0);
    psy = psy_instinct;
    psy->BrainBegin();
    psy->AddGoal(NPC_GOAL_IDLE, NULL);
    psy->BrainEnd();
    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCAmbient::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    S32 idx;
    U32 da_anim = 0;

    switch (gid)
    {
    case 'NGN0':
    case 'NGN6':
    case 'NGN7':
        idx = 1;
        break;
    default:
        idx = 1;
        break;
    }

    if (idx >= 0)
    {
        da_anim = g_hash_ambianim[idx];
    }

    return da_anim;
}

S32 zNPCAmbient::NPCMessage(NPCMsg* mail)
{
    zNPCGoalCommon* curgoal;
    zNPCGoalCommon* recgoal;
    xPsyche* psy = psy_instinct;
    S32 handled;

    if (psy)
    {
        curgoal = (zNPCGoalCommon*)psy->GetCurGoal();
        if (curgoal)
        {
            handled = curgoal->NPCMessage(mail);
            if (handled)
            {
                return handled;
            }
        }

        recgoal = (zNPCGoalCommon*)psy->GetPrevRecovery(0);
        while (recgoal)
        {
            if (recgoal != curgoal)
            {
                handled = recgoal->NPCMessage(mail);
                if (handled)
                {
                    return handled;
                }
            }

            recgoal = (zNPCGoalCommon*)psy->GetPrevRecovery(recgoal->GetID());
        }
    }

    // Retail dispatches this through the vtable (slot 0xCC): AmbiHandleMail is
    // virtual in zNPCAmbient so zNPCJelly's override handles NPC_MID_DAMAGE.
    handled = AmbiHandleMail(mail);
    if (!handled)
    {
        handled = zNPCCommon::NPCMessage(mail);
    }

    return handled;
}

void zNPCJelly::Init(xEntAsset* asset)
{
    zNPCAmbient::Init(asset);
    flg_move = flg_move | 4;
    flg_vuln = 0xffffffff;
    flg_vuln = flg_vuln & 0x9effffef;
}

void zNPCJelly::ParseINI()
{
    S32 selfType;

    zNPCCommon::ParseINI();
    cfg_npc->snd_trax = g_sndTrax_Jelly;
    NPCS_SndTablePrepare(g_sndTrax_Jelly);
    selfType = xNPCBasic::SelfType();
    if (selfType == NPC_TYPE_JELLYBLUE)
    {
        cfg_npc->spd_moveMax = 3.5f;
        cfg_npc->spd_turnMax = 3.4906585f;
    }
    else if (selfType == NPC_TYPE_JELLYPINK)
    {
        if (globals.sceneCur->sceneID == 'JF04') //DAT_803c2518 is globals.sceneCur->sceneID
        {
            cfg_npc->spd_moveMax = 3.5f;
            cfg_npc->spd_turnMax = 3.4906585f;
        }
        else
        {
            cfg_npc->spd_moveMax = 0.75f;
            cfg_npc->spd_turnMax = 0.43633232f;
        }
    }
}

void zNPCJelly::Reset()
{
    zNPCAmbient::Reset();
    cnt_angerLevel = 0;
    hitpoints = cfg_npc->pts_damage;
    if (npc_daddyJelly != NULL)
    {
        hitpoints = 0;
        psy_instinct->GoalSet(NPC_GOAL_DEAD, 1);
    }
}

void zNPCJelly::SelfSetup()
{
    xBehaveMgr* bmgr;
    xPsyche* psy;

    bmgr = xBehaveMgr_GetSelf();
    psy_instinct = bmgr->Subscribe(this, 0);
    psy = psy_instinct;
    psy->BrainBegin();
    zNPCCommon::AddBaseline(psy, JELY_grul_getAngry, JELY_grul_getAngry, JELY_grul_getAngry,
                            JELY_grul_getAngry, JELY_grul_getAngry);
    psy->AddGoal(NPC_GOAL_JELLYBUMPED, NULL);
    psy->AddGoal(NPC_GOAL_JELLYATTACK, NULL);
    psy->AddGoal(NPC_GOAL_JELLYBIRTH, NULL);
    psy->AddGoal(NPC_GOAL_DEAD, NULL);
    psy->BrainEnd();
    psy->SetSafety(NPC_GOAL_IDLE);
}

void zNPCJelly::JellySpawn(const xVec3* pos_spawn, F32 tym_fall)
{
    xPsyche* psy;
    zNPCGoalJellyBirth* birth;

    psy = psy_instinct;
    birth = (zNPCGoalJellyBirth*)psy->FindGoal('NGJ2');
    birth->BirthInfoSet(pos_spawn, tym_fall);
    psy->GoalSet('NGJ2', 0);
}

void zNPCJelly::JellyKill()
{
    xPsyche* psy = psy_instinct;
    if (psy && !psy->GIDInStack(NPC_GOAL_DEAD))
    {
        hitpoints = 0;
        psy->GoalSet(NPC_GOAL_DEAD, 0);
    }
}

U32 zNPCJelly::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* goal)
{
    U32 da_anim = 0;
    S32 idx = -1;

    switch (gid)
    {
    case 'NGN0': // 8c
    {
        idx = 1;
        break;
    }
    case 'NGN3': // 94
    {
        idx = 4;
        break;
    }
    case 'NGN1': // 9c
    case 'NGN2': // 9c
    {
        idx = 7;
        break;
    }
    case 'NGN5': // a4
    {
        idx = 1;
        break;
    }
    case 'NGJ2': // ac
    {
        idx = 1;
        break;
    }
    case 'NGJ1': // b4
    {
        idx = 8;
        break;
    }
    case 'NGJ0': // bc
    {
        idx = 11;
        break;
    }
    default: // c4
    {
        da_anim = this->zNPCAmbient::AnimPick(gid, gspot, goal);
        break;
    }
    }

    if (idx >= 0)
    {
        da_anim = g_hash_ambianim[idx];
    }

    return da_anim;
}

void zNPCJelly::BUpdate(xVec3*)
{
    static const xVec3 vec_offset = { 0.0f, 0.0f, 0.0f };

    xVec3 pos_bnd = *(const xVec3*)BonePos(2);
    pos_bnd += vec_offset;
    xMat3x3RMulVec(&pos_bnd, (const xMat3x3*)BoneMat(0), &pos_bnd);
    pos_bnd += *(const xVec3*)BonePos(0);

    bound.sph.center = pos_bnd;
    bound.sph.r = 0.5f;
    xQuickCullForBound(&bound.qcd, &bound);

    zGridUpdateEnt(this);
}

void zNPCJelly::ActLikeOctopus()
{
    S32 num_vert;
    S32 i;
    S32 stride;
    xVec3 pos_emit;

    num_vert = model->Data->geometry->numVertices;
    if (num_vert < 1)
    {
        return;
    }

    stride = MAX(1, num_vert / 16);

    for (i = 0; i < num_vert; i += stride)
    {
        iModelVertEval(model->Data, i, 1, model->Mat, NULL, &pos_emit);
        zFX_SpawnBubbleTrail(&pos_emit, 4);
    }
}

void zNPCNeptune::ParseINI()
{
    zNPCAmbient::ParseINI();
    cfg_npc->snd_traxShare = NULL;
    cfg_npc->snd_trax = g_sndTrax_Neptune;
    NPCS_SndTablePrepare(g_sndTrax_Neptune);
}

void zNPCNeptune::Reset()
{
    zNPCAmbient::Reset();
    flags |= XENT_0x40;
}

void zNPCMimeFish::Reset()
{
    zNPCAmbient::Reset();
    flg_move = 1;
}

void zNPCJelly::Process(xScene* xscn, F32 dt)
{
    this->zNPCAmbient::Process(xscn, dt);

    if (this->IsAlive())
    {
        this->PlayWithAlpha(dt);
        this->PlayWithAnimSpd();

        xPsyche* psy = this->psy_instinct;

        S32 flg_wonder = this->SomethingWonderful();

        if (xEntIsVisible(this))
        {
            if (xUtil_yesno(0.05f) && psy->GIDOfActive() != NPC_GOAL_DEAD)
            {
                this->PlayWithLightnin();
            }
        }
    }
}

S32 zNPCJelly::AmbiHandleMail(NPCMsg* mail)
{
    S32 handled = 1;
    xPsyche* psy = this->psy_instinct;

    switch (mail->msgid)
    {
    case NPC_MID_DAMAGE:
    {
        if (psy && hitpoints >= 1)
        {
            if (mail->dmgdata.dmg_type == DMGTYP_CRUISEBUBBLE)
            {
                this->hitpoints = this->hitpoints < 1 ? this->hitpoints : 1;
            }

            if (psy->GIDInStack(NPC_GOAL_JELLYBUMPED))
            {
                break;
            }

            if (psy->GIDInStack(NPC_GOAL_FIDGET))
            {
                psy->GoalSwap(NPC_GOAL_JELLYBUMPED, 0);
            }
            else if (psy->GIDInStack(NPC_GOAL_JELLYATTACK))
            {
                psy->GoalSet(NPC_GOAL_IDLE, 0);
                psy->GoalPush(NPC_GOAL_JELLYBUMPED, 0);
            }
            else
            {
                psy->GoalPush(NPC_GOAL_JELLYBUMPED, 0);
            }
        }

        break;
    }
    default:
    {
        handled = 0;
        break;
    }
    }

    return handled;
}

void zNPCJelly::PlayWithAlpha(F32 dt)
{
    F32 t = NPCC_TmrCycle(&this->tmr_pulseAlpha, dt, 0.5f);
    F32 x = PI * t;

    this->SetAlpha(LERP(MAX(0.0f, MIN((F32)__fabs(isin(x)), 1.0f)), 0.7f, 0.95f));
}

void zNPCJelly::SetAlpha(F32 alf)
{
    xModelInstance* minst = this->model;

    for (minst; minst != NULL; minst = minst->Next)
    {
        minst->Flags |= 0x4000;
        minst->Alpha = alf;
    }
}

void zNPCJelly::PlayWithAnimSpd()
{
    const S32 arr[3] = {
        0x4e474e32, // 'NGN2'
        0x4e474e31, // 'NGN1'
        0,
    };

    S32 gid = this->psy_instinct->GIDOfActive();

    for (const S32* i = arr; *i != 0; i++)
    {
        if (gid == *i)
        {
            this->PumpFaster();
            break;
        }
    }
}

void zNPCJelly::PumpFaster()
{
    F32 spd_max = 4.0f;
    F32 pct_spd = spd_throttle;
    F32 spd_move = cfg_npc->spd_moveMax;

    if (spd_move > spd_max)
    {
        spd_max = spd_move;
    }

    pct_spd /= spd_max;
    pct_spd = CLAMP(pct_spd, 0.0f, 1.0f);
    pct_spd = SMOOTH(pct_spd, 1.0f, 2.5f);

    AnimCurSingle()->CurrentSpeed = pct_spd;
}

void zNPCJelly::JellyBoneWorldPos(xVec3* pos, S32 idx_request) const
{
    S32 idx;

    if (idx_request < 1)
    {
        idx = (S32)(xurand() * this->model->BoneCount);
        if (idx < 2)
        {
            idx += 2;
        }
    }
    else
    {
        idx = idx_request;
        if (idx > this->model->BoneCount)
        {
            idx = 0;
        }
    }

    xVec3 pos_place = *(const xVec3*)this->BonePos(idx);
    xMat3x3RMulVec(&pos_place, (const xMat3x3*)this->BoneMat(0), &pos_place);
    pos_place += *(const xVec3*)this->BonePos(0);

    *pos = pos_place;
}

void zNPCJelly::PlayWithLightnin()
{
    _tagLightningAdd info;
    xVec3 pos_bone;

    this->JellyBoneWorldPos(&pos_bone, -1);

    xVec3 pos_place = pos_bone;

    memset(&info, 0, sizeof(info));

    if (this->SelfType() == NPC_TYPE_JELLYBLUE)
    {
        NPCC_MakeLightningInfo(NPC_LYT_JELLYFISHBLUE, &info);
    }
    else
    {
        NPCC_MakeLightningInfo(NPC_LYT_JELLYFISH, &info);
    }

    info.time = 0.1f;
    info.start = &pos_bone;
    info.end = &pos_place;

    for (S32 i = 0; i < 2; i++)
    {
        zLightningAdd(&info);
    }
}

S32 JELY_grul_getAngry(xGoal* rawgoal, void* p1, en_trantype* trantype, F32 f, void* p2)
{
    S32 nextgoal = 0;
    zNPCJelly* npc = (zNPCJelly*)rawgoal->psyche->clt_owner;
    S32 skipit = 0;
    S32 angerThresh;
    S32 selftype;
    F32 dst_sq;

    if (!npc->npcset.allowDetect)
    {
        skipit = 1;
    }
    else if (globals.player.Health < 1)
    {
        skipit = 1;
    }
    else if (globals.player.DamageTimer > 0.0f)
    {
        skipit = 1;
    }
    else if (npc->SomethingWonderful())
    {
        skipit = 1;
    }

    if (skipit)
    {
        npc->cnt_angerLevel = 0;
        return 0;
    }

    selftype = npc->SelfType();

    if (globals.sceneCur->sceneID == 'JF04')
    {
        angerThresh = 100;
    }
    else if (selftype == NPC_TYPE_JELLYPINK)
    {
        angerThresh = 300;
    }
    else if (selftype == NPC_TYPE_JELLYBLUE)
    {
        angerThresh = 150;
    }
    else
    {
        angerThresh = 300;
    }

    dst_sq = npc->XYZDstSqToPlayer(NULL);

    if (dst_sq < SQ(5.0f))
    {
        npc->cnt_angerLevel++;
    }
    else
    {
        npc->cnt_angerLevel--;
    }

    if (npc->cnt_angerLevel < 0)
    {
        npc->cnt_angerLevel = 0;
    }

    // Retail bug: this clamps the anger level to *above* the threshold, so the
    // test right below always succeeds once it has fired.
    if (npc->cnt_angerLevel > angerThresh)
    {
        npc->cnt_angerLevel = angerThresh + 10;
    }

    if (npc->cnt_angerLevel > angerThresh && dst_sq < SQ(3.0f))
    {
        npc->cnt_angerLevel = 0;
        *trantype = GOAL_TRAN_PUSH;
        nextgoal = NPC_GOAL_JELLYATTACK;
    }

    return nextgoal;
}

U32 zNPCNeptune::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    S32 idx;
    U32 da_anim = 0;

    switch (gid)
    {
    case 'NGN0':
    {
        const S32 choices[3] = { 1, 2, 3 };
        idx = xUtil_choose<S32>(choices, 3, NULL);
        break;
    }
    case 'NGN3':
    {
        const S32 choices[3] = { 4, 5, 6 };
        idx = xUtil_choose<S32>(choices, 3, NULL);
        break;
    }
    default:
        idx = 1;
        break;
    }

    if (idx >= 0)
    {
        da_anim = g_hash_ambianim[idx];
    }

    return da_anim;
}

void zNPCNeptune::Process(xScene* xscn, F32 dt)
{
    U32 anid;
    xVec3 vec = { 0.0f, 0.0f, 0.0f };
    S32 flg_fidget = (tmr_fidget < 0.0f) ? 1 : 0;

    if (flg_fidget)
    {
        tmr_fidget =
            cfg_npc->tym_fidget + cfg_npc->tym_fidget * (0.25f * (xurand() - 0.5f));

        if (xrand() & 0x800000)
        {
            anid = AnimPick('NGN0', NPC_GSPOT_START, NULL);
        }
        else
        {
            anid = AnimPick('NGN3', NPC_GSPOT_START, NULL);
        }

        if (anid != 0)
        {
            AnimStart(anid, 0);
        }
    }
    else
    {
        tmr_fidget = MAX(-1.0f, tmr_fidget - dt);
    }

    xVec3Sub(&vec, xEntGetPos(&globals.player.ent), xEntGetPos(this));
    vec.y = 0.0f;
    xVec3Normalize(&vec, &vec);

    TurnToFace(dt, &vec, -1.0f);
    VelStop();

    zNPCAmbient::Process(xscn, dt);
}

U32 zNPCMimeFish::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    S32 idx;
    U32 da_anim = 0;

    switch (gid)
    {
    case 'NGN0':
        idx = 1;
        break;
    case 'NGN3':
        idx = 1;
        break;
    default:
        idx = 1;
        break;
    }

    if (idx >= 0)
    {
        da_anim = g_hash_ambianim[idx];
    }

    return da_anim;
}

void zNPCMimeFish::Process(xScene* xscn, F32 dt)
{
}

S32 zNPCJelly::IsAlive()
{
    return (-(U32)hitpoints & ~(U32)hitpoints) >> 0x1f;
}

void zNPCMimeFish::SelfSetup()
{
}

U8 zNPCAmbient::ColChkFlags() const
{
    return 0;
}

U8 zNPCAmbient::ColPenFlags() const
{
    return 0;
}

U8 zNPCAmbient::ColChkByFlags() const
{
    return 0x18;
}

U8 zNPCAmbient::ColPenByFlags() const
{
    return 0x18;
}

U8 zNPCAmbient::PhysicsFlags() const
{
    return 3;
}

/* This should be 100% matching but it causes a vtable duplication error for some reason
void zNPCNeptune::SelfSetup()
{
}
*/

U8 zNPCNeptune::ColChkFlags() const
{
    return 0;
}

U8 zNPCNeptune::ColPenFlags() const
{
    return 0;
}

U8 zNPCNeptune::ColChkByFlags() const
{
    return 0;
}

U8 zNPCNeptune::ColPenByFlags() const
{
    return 0;
}

void zNPCNeptune::SelfSetup()
{
}
