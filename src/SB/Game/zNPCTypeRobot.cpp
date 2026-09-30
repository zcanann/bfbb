#include "zNPCTypeRobot.h"
#include "zNPCSupplement.h"
#include "zNPCSupport.h"
#include "zNPCSndLists.h"
#include "zNPCGoalRobo.h"
#include "zNPCTypes.h"
#include "zNPCGoalStd.h"
#include "zGlobals.h"
#include "zNPCGoals.h"

#include "xFactory.h"
#include "xMath.h"
#include "xAnim.h"
#include "xBehaviour.h"
#include "xMathInlines.h"
#include "xGroup.h"
#include "zScene.h"
#include "xLinkAsset.h"
#include "zSurface.h"
#include "zGoo.h"
#include "zFX.h"
#include "zNPCFXCinematic.h"
#include "iModel.h"
#include "zGrid.h"
#include "xutil.h"
#include "xEntBoulder.h"
#include "zRenderState.h"
#include "xDraw.h"
#include "xColor.h"
#include "zNPCGoalStd.h"
#include "zNPCGoalCommon.h"
#include "zParEmitter.h"
#include "xParEmitter.h"

#include <string.h>

// These structs were used in deadstripped functions.
// This function is here to force the symbols to be linked.
//
// The target opens .rodata with the same eleven unreferenced all-zero
// templates as zVar.cpp and xParEmitterType.cpp -- four of 0x0C and seven of
// 0x28, 0x148 in total -- which offsets every later .rodata relocation.
void __deadstripped_zNPCTypeRobot()
{
    const char _405[0x0C] = {};
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
}

U32 g_hash_roboanim[41] = { 0 };

char* g_strz_roboanim[41] = {
    "Unknown",        "Idle01",          "Fidget01",       "Move01",       "Notice01",
    "Taunt01",        "Respawn01",       "LassoGrab01",    "LassoHold01",  "StunBegin01",
    "StunLoop01",     "EndTag_Standard", "AlertIdle01",    "AlertMove01",  "Attack01",
    "Attack02",       "Attack03",        "AttackBegin01",  "AttackLoop01", "AttackEnd01",
    "HurtKnock01",    "HurtSmash01",     "HurtBash01",     "LassoYank01",  "PatPickup01",
    "PatCarry01",     "PatThrowBegin01", "PatThrowLoop01", "Sleep01",      "TeleportBegin01",
    "TeleportLoop01", "TeleportEnd01",   "Launch01",       "LaunchEnd01",  "DanceBegin01",
    "DanceLoop01",    "DanceEnd01",      "Death01",        "Death02",      "DodgeBBowl01",
    "DodgeBCruise01"
};

static U32 g_hash_ttsanim[2] = { 0, 0 };

static char* g_strz_ttsanim[2] = { "Unknown", "TarTar_Slosh01" };

static U32 g_hash_cloudanim[3] = { 0, 0, 0 };

static char* g_strz_cloudanim[3] = { "Unknown", "Cloud_Idle01", "Cloud_Attack01" };

static U32 g_hash_nytlytanim[2] = { 0, 0 };

static char* g_strz_nytlytanim[2] = { "Unknown", "Light_Idle01" };

static U32 g_hash_flotanim[2] = { 0, 0 };

static char* g_strz_flotanim[2] = { "Unknown", "Wiggle01" };

static U32 g_hash_shieldanim[2] = { 0, 0 };

static char* g_strz_shieldanim[2] = { "Unknown", "Shield_Idle01" };

static UVAModelInfo g_uvaShield;

static S32 g_cnt_fodbzzt;

static S32 g_cnt_sleepy;

static S32 g_needuvincr_tube;

static S32 g_needuvincr_bzzt;

static S32 g_needuvincr_nightlight;

static S32 g_needuvincr_slickshield;

RwRaster* zNPCFodBomb::rast_blink;

F32 zNPCFodBzzt::tmr_nexthokey;

F32 zNPCFodBzzt::tmr_hokeypokey;

volatile S32 zNPCFodBzzt::cnt_alerthokey;

NPCLaser zNPCFodBzzt::laser;

RwRaster* zNPCFodBzzt::rast_discoLight;

F32 zNPCFodBzzt::uv_slice_discoLight[2] = { 1.0f, 1.0f };

S32 g_needMusician = 1;

F32 zNPCSleepy::uv_deathcone[2] = { 0.0f, 0.0f };

F32 zNPCSleepy::uv_nightlight[2] = { 0.0f, 0.0f };

F32 zNPCSleepy::uv_slice_nightlight[2] = { 0.25f, 0.25f };

F32 zNPCSleepy::uv_slice_deathcone[2] = { 0.25f, 0.25f };

NPCLaser zNPCTubeSlave::laser;

void zNPCFodBzzt_DoTheHokeyPokey(F32 dt);
void ZNPC_Destroy_Robot(xFactoryInst* inst);
void ZNPC_AnimTable_RobotBase(xAnimTable*);
NPARMgmt* NPAR_PartySetup(en_nparptyp parType, void** userData, NPARXtraData* xtraData);

void PlayTheFiddle();
void zNPCSleepy_Timestep(F32 dt);
S32 DUMY_grul_returnToIdle(xGoal* goal, void*, en_trantype* trantype, F32, void*);
S32 FODR_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 BOMB_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 BZZT_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 CHMP_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 HAMR_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 TART_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 GLOV_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 MOON_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 SLEP_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 ARFY_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 PUPY_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 CHUK_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 TUBE_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
S32 SLCK_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*);
void ROBO_KillEffects();
S32 zSurfaceGetDamageType(const xSurface* surf);
void TellPlayerVillainIsNear(F32 visnear);
xVec3* NPCC_upDir(xEnt* ent);
S32 zEntTeleportBox_playerIn();

void ZNPC_Robot_Startup()
{
    S32 i;

    for (i = 0; i < 41; i++)
    {
        g_hash_roboanim[i] = xStrHash(g_strz_roboanim[i]);
    }

    for (i = 0; i < 2; i++)
    {
        g_hash_ttsanim[i] = xStrHash(g_strz_ttsanim[i]);
    }

    for (i = 0; i < 3; i++)
    {
        g_hash_cloudanim[i] = xStrHash(g_strz_cloudanim[i]);
    }

    for (i = 0; i < 2; i++)
    {
        g_hash_nytlytanim[i] = xStrHash(g_strz_nytlytanim[i]);
    }

    for (i = 0; i < 2; i++)
    {
        g_hash_flotanim[i] = xStrHash(g_strz_flotanim[i]);
    }

    for (i = 0; i < 2; i++)
    {
        g_hash_shieldanim[i] = xStrHash(g_strz_shieldanim[i]);
    }

    PlayTheFiddle();
}
void PlayTheFiddle()
{
}

void ZNPC_Robot_Shutdown()
{
}

void zNPCRobot_ScenePrepare()
{
    g_cnt_fodbzzt = 0;
    g_cnt_sleepy = 0;
    g_needuvincr_tube = 0;
    g_needuvincr_bzzt = 0;
    g_needuvincr_nightlight = 0;
    g_needuvincr_slickshield = 0;
    g_uvaShield.Clear();
}

void zNPCRobot_SceneFinish()
{
    ROBO_KillEffects();
    g_uvaShield.Hemorrage();
}

void zNPCRobot_SceneReset()
{
    zNPCFodBzzt_ResetDanceParty();
}

void zNPCRobot_ScenePostInit()
{
    ROBO_InitEffects();
    ROBO_PrepRoboCop();
}

void zNPCRobot_Timestep(xScene* sc, float dt)
{
    if (g_cnt_fodbzzt)
    {
        zNPCFodBzzt_DoTheHokeyPokey(dt);
    }
    if (g_cnt_sleepy)
    {
        zNPCSleepy_Timestep(dt);
    }

    g_needuvincr_tube = 1;
    g_needuvincr_bzzt = 1;
    g_needuvincr_nightlight = 1;
    g_needuvincr_slickshield = 1;
}

xFactoryInst* ZNPC_Create_Robot(S32 who, RyzMemGrow* grow, void*)
{
    zNPCRobot* robo;

    switch (who)
    {
    case NPC_TYPE_ROBOT:
    {
        robo = new (who, grow) zNPCRobot(who);
        break;
    }
    case NPC_TYPE_FODDER:
    {
        robo = new (who, grow) zNPCFodder(who);
        break;
    }
    case NPC_TYPE_FODBOMB:
    {
        robo = new (who, grow) zNPCFodBomb(who);
        break;
    }
    case NPC_TYPE_FODBZZT:
    {
        robo = new (who, grow) zNPCFodBzzt(who);
        break;
    }
    case NPC_TYPE_CHOMPER:
    {
        robo = new (who, grow) zNPCChomper(who);
        break;
    }
    case NPC_TYPE_CRITTER:
    {
        robo = new (who, grow) zNPCCritter(who);
        break;
    }
    case NPC_TYPE_HAMMER:
    case NPC_TYPE_HAMSPIN:
    {
        robo = new (who, grow) zNPCHammer(who);
        break;
    }
    case NPC_TYPE_TARTAR:
    {
        robo = new (who, grow) zNPCTarTar(who);
        break;
    }
    case NPC_TYPE_GLOVE:
    {
        robo = new (who, grow) zNPCGlove(who);
        break;
    }
    case NPC_TYPE_MONSOON:
    {
        robo = new (who, grow) zNPCMonsoon(who);
        break;
    }
    case NPC_TYPE_SLEEPY:
    {
        robo = new (who, grow) zNPCSleepy(who);
        break;
    }
    case NPC_TYPE_ARFDOG:
    {
        robo = new (who, grow) zNPCArfDog(who);
        break;
    }
    case NPC_TYPE_ARFARF:
    {
        robo = new (who, grow) zNPCArfArf(who);
        break;
    }
    case NPC_TYPE_CHUCK:
    {
        robo = new (who, grow) zNPCChuck(who);
        break;
    }
    case NPC_TYPE_TUBELET:
    {
        robo = new (who, grow) zNPCTubelet(who);
        break;
    }
    case NPC_TYPE_TUBESLAVE:
    {
        robo = new (who, grow) zNPCTubeSlave(who);
        break;
    }
    case NPC_TYPE_SLICK:
    {
        robo = new (who, grow) zNPCSlick(who);
        break;
    }
    default:
    {
        robo = new (who, grow) zNPCRobot(who);
        break;
    }
    }

    return robo;
}

void ZNPC_Destroy_Robot(xFactoryInst* inst)
{
    delete inst;
}

void ZNPC_AnimTable_RobotBase(xAnimTable* table)
{
    int ourAnims[17] = { 0x01, 0x03, 0x02, 0x04, 0x05, 0x07, 0x08, 0x17, 0x09,
                         0x0a, 0x14, 0x16, 0x15, 0x25, 0x26, 0x06, 0x00 };

    xAnimTableNewState(table, g_strz_roboanim[0x01], 0x110, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x03], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x02], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x04], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x05], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewState(table, g_strz_roboanim[0x14], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x16], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x15], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewState(table, g_strz_roboanim[0x25], 0x00, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x26], 0x00, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewState(table, g_strz_roboanim[0x06], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x07], 0x020, 0x2000000, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x08], 0x010, 0x2000000, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewState(table, g_strz_roboanim[0x17], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewState(table, g_strz_roboanim[0x09], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0a], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x07], g_strz_roboanim[0x08], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x09], g_strz_roboanim[0x0a], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x14], g_strz_roboanim[0x25], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x16], g_strz_roboanim[0x25], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x15], g_strz_roboanim[0x25], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
}

xAnimTable* ZNPC_AnimTable_Fodder()
{
    int ourAnims[16] = { 0x01, 0x03, 0x02, 0x04, 0x05, 0x22, 0x23, 0x24,
                         0x0e, 0x14, 0x16, 0x15, 0x25, 0x26, 0x06, 0x00 };

    xAnimTable* table = (xAnimTable*)xAnimTableNew("zNPCFodder", NULL, 0);

    xAnimTableNewState(table, g_strz_roboanim[0x01], 0x110, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x03], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x02], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x04], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x05], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x22], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x23], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x24], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0e], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x14], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x16], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x15], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x25], 0x000, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x26], 0x000, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x06], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x14], g_strz_roboanim[0x25], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x16], g_strz_roboanim[0x25], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x15], g_strz_roboanim[0x25], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x22], g_strz_roboanim[0x23], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_Hammer()
{
    int ourAnims[6] = {
        0x0e, 0x18, 0x19, 0x1a, 0x1b, 0x00,
    };

    xAnimTable* table = xAnimTableNew("zNPCHammer", NULL, 0);
    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x0e], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x18], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x19], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x1a], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x1b], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x18], g_strz_roboanim[0x19], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x1a], g_strz_roboanim[0x1b], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_TarTar()
{
    int ourAnims[4] = { 0x0c, 0x11, 0x0e, 0x00 };

    xAnimTable* table = xAnimTableNew("zNPCTarTar", NULL, 0);
    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x0c], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x11], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0e], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x11], g_strz_roboanim[0x0e], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x0e], g_strz_roboanim[0x0c], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x04], g_strz_roboanim[0x0c], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_TTSauce()
{
    xAnimTable* table = (xAnimTable*)xAnimTableNew("TarTarSauce", NULL, 0);

    xAnimTableNewState(table, g_strz_ttsanim[1], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_GLove()
{
    int ourAnims[8] = {
        0x18, 0x19, 0x1a, 0x1b, 0x11, 0x12, 0x13, 0x00,
    };

    xAnimTable* table = (xAnimTable*)xAnimTableNew("zNPCGlove", NULL, 0);
    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x11], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x12], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x13], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x18], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x19], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x1a], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x1b], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x1a], g_strz_roboanim[0x1b], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x11], g_strz_roboanim[0x12], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x18], g_strz_roboanim[0x19], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_Monsoon()
{
    int ourAnims[5] = { 0x0c, 0x0d, 0x11, 0x12, 0x00 };

    xAnimTable* table = xAnimTableNew("zNPCMonsoon", NULL, 0);
    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x0c], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0d], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x11], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x12], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x11], g_strz_roboanim[0x12], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_ThunderCloud()
{
    int ourAnims[3] = { 0x01, 0x02, 0x00 };

    xAnimTable* table = xAnimTableNew("ThunderCloud", NULL, 0);

    xAnimTableNewState(table, g_strz_cloudanim[0x01], 0x010, 1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_cloudanim[0x02], 0x010, 1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_cloudanim, ourAnims, 1, 0.2f);

    return table;
}

xAnimTable* ZNPC_AnimTable_NightLight()
{
    int ourAnims[2] = { 0x01, 0x00 };

    xAnimTable* table = xAnimTableNew("NightLight", NULL, 0);

    xAnimTableNewState(table, g_strz_nytlytanim[1], 0x010, 1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_nytlytanim, ourAnims, 1, 0.2f);

    return table;
}

// Regalloc
xAnimTable* ZNPC_AnimTable_SleepyTime()
{
    int ourAnims[3] = { 0x0c, 0x0e, 0x00 };

    xAnimTable* table = xAnimTableNew("zNPCSleepy", NULL, 0);
    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x0c], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0e], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    return table;
}

xAnimTable* ZNPC_AnimTable_ArfDog()
{
    int ourAnims[17] = { 0x01, 0x03, 0x02, 0x04, 0x05, 0x20, 0x21, 0x0c, 0x0d,
                         0x12, 0x14, 0x16, 0x15, 0x25, 0x26, 0x06, 0x00 };

    xAnimTable* table = (xAnimTable*)xAnimTableNew("zNPCArfDog", NULL, 0);

    xAnimTableNewState(table, g_strz_roboanim[0x01], 0x110, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x03], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x02], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x04], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x05], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x20], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x21], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0c], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0d], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x12], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x14], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x16], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x15], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x25], 0x000, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x26], 0x000, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x06], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    return table;
}

xAnimTable* ZNPC_AnimTable_ArfArf()
{
    int ourAnims[7] = { 0x0e, 0x0f, 0x10, 0x1d, 0x1e, 0x1f, 0x00 };

    xAnimTable* table = xAnimTableNew("zNPCArfArf", NULL, 0);
    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x0e], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0f], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x10], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x1d], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x1e], 0x00, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x1f], 0x20, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x1d], g_strz_roboanim[0x1e], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_Chuck()
{
    int ourAnims[4] = { 0x0c, 0x0d, 0x0e, 0x00 };

    xAnimTable* table = xAnimTableNew("zNPCChuck", NULL, 0);
    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x0c], 0x110, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0d], 0x110, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x0e], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x0e], g_strz_roboanim[0x0c], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x04], g_strz_roboanim[0x0c], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);
    xAnimTableNewTransition(table, g_strz_roboanim[0x04], g_strz_roboanim[0x0c], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);

    return table;
}

// Regalloc
xAnimTable* ZNPC_AnimTable_Tubelet()
{
    int ourAnims[2] = { 0x0e, 0x00 };

    xAnimTable* table = xAnimTableNew("zNPCTubelet", NULL, 0);
    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x0e], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    return table;
}

xAnimTable* ZNPC_AnimTable_FloatDevice()
{
    xAnimTable* table = xAnimTableNew("TubeletFloatDevice", NULL, 0);
    xAnimTableNewState(table, g_strz_flotanim[0x01], 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_SlickShield()
{
    xAnimTable* table = xAnimTableNew("SlickBubbleShield", NULL, 0);
    xAnimTableNewState(table, g_strz_shieldanim[0x01], 0x10, 1, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_Slick()
{
    int ourAnims[4] = { 0x11, 0x12, 0x13, 0x00 };

    xAnimTable* table = xAnimTableNew("zNPCSlick", NULL, 0);

    ZNPC_AnimTable_RobotBase(table);

    xAnimTableNewState(table, g_strz_roboanim[0x11], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x12], 0x010, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_roboanim[0x13], 0x020, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_roboanim, ourAnims, 1, 0.2f);

    xAnimTableNewTransition(table, g_strz_roboanim[0x11], g_strz_roboanim[0x12], NULL, NULL, 0x10,
                            0, 0.0f, 0.0f, 0, 0, 0.2f, NULL);

    return table;
}

U8 zNPCRobot::ColChkFlags() const
{
    S32 flags = 0x3E;

    if (npcset.reduceCollide)
    {
        flags &= ~0x6;
    }

    return flags;
}

U8 zNPCRobot::ColPenFlags() const
{
    S32 flags = 0x3E;

    if (npcset.reduceCollide)
    {
        flags &= ~0x6;
    }

    return flags;
}

U8 zNPCRobot::PhysicsFlags() const
{
    S32 flags = 0;

    if (flg_move & 0x6)
    {
        flags |= 3;
    }

    if (flg_move & 0x2)
    {
        flags |= 4;
    }

    return flags;
}

void zNPCRobot::Init(xEntAsset* asset)
{
    zNPCCommon::Init(asset);
    this->flg_move = 10;
    this->flg_vuln = -1;
    this->idx_neckBone = -1;
    this->flags1.flg_basenpc |= 8;
}

void zNPCRobot::Reset()
{
    NPCConfig* conf = cfg_npc;

    if (conf->dst_castShadow < 0.0f && !(flg_move & 4))
    {
        conf->dst_castShadow = 1.0f;
    }

    if (conf->rad_shadowCache < 0.0f)
    {
        conf->rad_shadowCache = GenShadCacheRad();
    }

    zNPCCommon::Reset();

    hitpoints = cfg_npc->pts_damage;

    if (PRIV_GetLassoData())
    {
        flg_vuln |= 0x1000000;
    }

    if (xNPCBasic::SelfType() != NPC_TYPE_TUBESLAVE)
    {
        zNPCGoalDead* goal;
        xPsyche* psy = psy_instinct;

        if (npc_duplodude)
        {
            goal = (zNPCGoalDead*)psy->FindGoal('NGRj');
            goal->DieQuietly();
            psy_instinct->GoalSet('NGRj', 1);
        }
        else
        {
            if ((U32)xEntIsEnabled(this) == 0)
            {
                goal = (zNPCGoalDead*)psy->FindGoal('NGRj');
                goal->DieQuietly();
                psy_instinct->GoalSet('NGRj', 1);
            }
            else
            {
                psy_instinct->GoalSet('NGN0', 1);
            }
        }
    }

    S32 rc = arena.NeedToCycle(this);

    if (rc == 2)
    {
        arena.Cycle(this, 1);
    }
    else
    {
        if (rc)
        {
            arena.Cycle(this, 0);
        }
    }
}

F32 zNPCRobot::GenShadCacheRad()
{
    F32 fac_use;
    F32 rad_cache;

    switch (xNPCBasic::SelfType())
    {
    case NPC_TYPE_HAMMER: // 0x4e545230:
    case NPC_TYPE_HAMSPIN: // 0x4e545231:
    case NPC_TYPE_TARTAR: // 0x4e545232:
    case NPC_TYPE_GLOVE: // 0x4e545233:
    case NPC_TYPE_MONSOON: // 0x4e545234:
    case NPC_TYPE_SLEEPY: // 0x4e545235:
    case NPC_TYPE_ARFARF: // 0x4e545237:
    case NPC_TYPE_TUBELET: // 0x4e545239:
    case NPC_TYPE_TUBESLAVE: // 0x4e54523a:
        fac_use = 2.4f;
        break;
    case NPC_TYPE_SLICK:
        fac_use = 2.5f;
        break;
    case NPC_TYPE_ARFDOG: // 0x4e545236:
    case NPC_TYPE_CHUCK: // 0x4e545238:
    case NPC_TYPE_FODDER: // 0x4e54523c:
    case NPC_TYPE_FODBOMB: // 0x4e54523d:
    case NPC_TYPE_FODBZZT: // 0x4e54523e:
    case NPC_TYPE_CHOMPER: // 0x4e54523f:
    case NPC_TYPE_CRITTER: // 0x4e545240:
        fac_use = 1.5f;
        break;
    default:
        fac_use = 2.0f;
        break;
    }
    rad_cache = zNPCCommon::BoundAsRadius(0);
    return (fac_use * rad_cache);
}

// This static was used in a function the retail link deadstripped: the target
// .data holds a second, unreferenced `choices$NNNN` of { 20, 21, 22 } between
// the jump tables of zNPCRobot::GenShadCacheRad and zNPCRobot::RoboHandleMail,
// which offsets every later .data relocation.
void __deadstripped_zNPCTypeRobot_2()
{
    static S32 choices[3] = { 20, 21, 22 };
}

void zNPCRobot::ParseINI()
{
    zNPCCommon::ParseINI();
    cfg_npc->snd_traxShare = g_sndTrax_Robot;
    NPCS_SndTablePrepare(g_sndTrax_Robot);
}

void zNPCRobot::Process(xScene* xscn, F32 dt)
{
    psy_instinct->Timestep(dt, NULL);

    if (IsAlive())
    {
        DoAliveStuff(dt);
    }

    zNPCCommon::Process(xscn, dt);
}

void zNPCRobot::NewTime(xScene* xscn, F32 dt)
{
    if (idx_neckBone >= 0 && !IsDying())
    {
        TurnThemHeads();
    }
    zNPCCommon::NewTime(xscn, dt);
}

void zNPCRobot::SelfSetup()
{
    xBehaveMgr* bmgr = xBehaveMgr_GetSelf();
    psy_instinct = bmgr->Subscribe(this, 0);
    xPsyche* psy = psy_instinct;
    xGoal* goal = NULL;

    psy->BrainBegin();
    goal = psy->AddGoal('NGR4', NULL);
    goal->SetCallbacks(DUMY_grul_returnToIdle, NULL, NULL, NULL);
    AddBaseline(psy, NULL, NULL, NULL, NULL, NULL);
    AddStunThrow(psy, NULL, NULL, NULL, NULL);
    AddLassoing(psy, NULL, NULL, NULL, NULL, NULL);
    AddDamage(psy, NULL, NULL, NULL, NULL, NULL);
    AddSpawning(psy, NULL, NULL);
    AddScripting(psy, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    AddMiscTypical(psy, NULL, NULL, NULL);
    psy->BrainEnd();
    psy->SetSafety('NGN0');
}

U32 zNPCRobot::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    S32 idx = -1;
    U32 da_anim = 0;

    switch (gid)
    {
    case NPC_GOAL_IDLE:
    case NPC_GOAL_WAITING:
    case NPC_GOAL_NOMANLAND:
    case NPC_GOAL_LIMBO:
    case NPC_GOAL_ALERT:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 1;
        }
        else
        {
            idx = 1;
        }
        break;
    case NPC_GOAL_PATROL:
        if (gspot == NPC_GSPOT_PATROLPAUSE)
        {
            idx = 2;
            break;
        }
        idx = 3;
        break;
    case NPC_GOAL_WANDER:
    case NPC_GOAL_EVADE:
    case NPC_GOAL_GOHOME:
    case NPC_GOAL_CHASE:
        idx = 3;
        break;
    case NPC_GOAL_FIDGET:
        idx = 2;
        break;
    case NPC_GOAL_NOTICE:
        idx = 4;
        break;
    case NPC_GOAL_TAUNT:
        idx = 5;
        break;
    case NPC_GOAL_LASSOBASE:
    case NPC_GOAL_LASSOGRAB:
        idx = 7;
        break;
    case NPC_GOAL_LASSOTHROW:
        idx = 0x17;
        break;
    case NPC_GOAL_EVILPAT:
    case NPC_GOAL_STUNNED:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 9;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 10;
        }
        break;
    case NPC_GOAL_PATCARRY:
    case NPC_GOAL_PATTHROW:
        idx = 10;
        break;
    case NPC_GOAL_WOUND:
        idx = 0x14;
        break;
    case NPC_GOAL_KNOCK:
    {
        S32 rnd = xrand();
        S32 pick = 0x26;

        if (rnd & 0x800000)
        {
            pick = 0x25;
        }

        idx = pick;
        break;
    }
    case NPC_GOAL_R56:
        idx = 0x15;
        break;
    case NPC_GOAL_BASHED:
        idx = 0x16;
        break;
    case NPC_GOAL_DAMAGE:
    case NPC_GOAL_R54:
        idx = 0x25;
        break;
    case NPC_GOAL_RESPAWN:
        idx = 6;
        break;
    case NPC_GOAL_AFTERLIFE:
        idx = 1;
        break;
    default:
        xUtil_idtag2string(gid, 0);
        idx = 1;
        break;
    }

    if (idx >= 0)
    {
        da_anim = g_hash_roboanim[idx];
    }

    return da_anim;
}

S32 zNPCRobot::SysEvent(xBase* from, xBase* to, U32 toEvent, const F32* toParam,
                        xBase* toParamWidget, S32* handled)
{
    static NPCMsg npcmsg;

    S32 doother = 1;
    xGoal* curgoal;
    xGoal* recgoal;
    xPsyche* psy;

    memset(&npcmsg, 0, sizeof(npcmsg));

    psy = this->psy_instinct;

    if (psy)
    {
        curgoal = psy->GetCurGoal();

        if (curgoal && (curgoal->GetFlags() & 0x10))
        {
            doother = curgoal->SysEvent(from, to, toEvent, toParam, toParamWidget, handled);

            if (handled)
            {
                return doother;
            }
        }

        recgoal = psy->GetPrevRecovery(0);

        if (recgoal && recgoal != curgoal && (recgoal->GetFlags() & 0x10))
        {
            doother = recgoal->SysEvent(from, to, toEvent, toParam, toParamWidget, handled);

            if (handled)
            {
                return doother;
            }
        }
    }

    return zNPCCommon::SysEvent(from, to, toEvent, toParam, toParamWidget, handled);
}

S32 zNPCRobot::NPCMessage(NPCMsg* mail)
{
    xGoal* curgoal;
    xGoal* recgoal;
    xPsyche* psy = psy_instinct;

    if (psy != NULL)
    {
        curgoal = psy->GetCurGoal();

        if (curgoal != NULL)
        {
            S32 handled = ((zNPCGoalCommon*)curgoal)->NPCMessage(mail);
            if (handled != 0)
            {
                return handled;
            }
        }

        recgoal = psy->GetPrevRecovery(0);

        while (recgoal != NULL)
        {
            if (recgoal != curgoal)
            {
                S32 handled = ((zNPCGoalCommon*)recgoal)->NPCMessage(mail);
                if (handled != 0)
                {
                    return handled;
                }
            }

            recgoal = psy->GetPrevRecovery(recgoal->GetID());
        }
    }

    S32 handled = RoboHandleMail(mail);

    if (handled == 0)
    {
        handled = zNPCCommon::NPCMessage(mail);
    }

    return handled;
}

S32 zNPCRobot::RoboHandleMail(NPCMsg* mail)
{
    xPsyche* psy = this->psy_instinct;
    S32 handled = 1;

    switch (mail->msgid)
    {
    case NPC_MID_BUNNYHOP:
    {
        BunnyHopSet(NULL);
        break;
    }
    case NPC_MID_DAMAGE:
    case NPC_MID_EXPLOSION:
    {
        if (psy->GIDOfPending() == 'NGRd')
        {
            handled = 1;
            break;
        }

        zNPCGoalDamage* dmggoal = (zNPCGoalDamage*)psy->FindGoal('NGRd');

        if (dmggoal)
        {
            dmggoal->InputInfo(&mail->dmgdata);
            psy->GoalSet('NGRd', 0);
            handled = 1;
        }

        break;
    }
    case NPC_MID_RESPAWN:
    {
        mail->spawning.spawnSuccess = 0;
        break;
    }
    case NPC_MID_SCRIPTBEGIN:
    {
        psy_instinct->GoalSet('NGS0', 0);
        break;
    }
    case NPC_MID_STUN:
    {
        if (!(flg_vuln & 0x10000000))
        {
            break;
        }

        if (psy->GIDOfPending() == 'NGRd')
        {
            handled = 1;
            break;
        }

        if (psy->GIDInStack('NGRd'))
        {
            handled = 1;
            break;
        }

        if (IsDead())
        {
            break;
        }

        zNPCGoalEvilPat* evilgoal = (zNPCGoalEvilPat*)psy->FindGoal('NGR`');

        if (evilgoal)
        {
            evilgoal->InputStun(&mail->stundata);
            psy->GoalSet('NGR`', 0);
            handled = 1;
        }

        break;
    }
    case NPC_MID_SYSEVENT:
    {
        switch (mail->sysevent.toEvent)
        {
        case eEventNPCSetActiveOn:
        case eEventNPCSetActiveOff:
        {
            if (npc_duplodude == NULL)
            {
                handled = 0;
            }
            else
            {
                handled = 1;
            }

            break;
        }
        default:
        {
            handled = 0;
            break;
        }
        }

        break;
    }
    case NPC_MID_SCRIPTEND:
    case NPC_MID_SCRIPTHALT:
    {
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

void zNPCRobot::DuploOwner(zNPCCommon* duper)
{
    zNPCCommon::DuploOwner(duper);

    xPsyche* psy = this->psy_instinct;

    if (psy)
    {
        zNPCGoalDead* goal = (zNPCGoalDead*)psy->FindGoal('NGRj');
        goal->DieQuietly();
        psy->GoalSet('NGRj', 1);
    }
}

void zNPCRobot::DoAliveStuff(F32 dt)
{
    xPsyche* psy = this->psy_instinct;

    if (drv_data && drv_data->driver)
    {
        F32 wid;
        xBound* bnd = &drv_data->driver->bound;

        switch (bnd->type)
        {
        case XBOUND_TYPE_SPHERE:
            wid = bnd->sph.r;
            break;
        case XBOUND_TYPE_BOX:
        {
            F32 dx = bnd->box.box.upper.x - bnd->box.box.lower.x;
            F32 dz = bnd->box.box.upper.z - bnd->box.box.lower.z;
            wid = 0.5f * MIN(dx, dz);
            break;
        }
        case XBOUND_TYPE_OBB:
        {
            F32 dx = bnd->box.box.upper.x - bnd->box.box.lower.x;
            F32 dz = bnd->box.box.upper.z - bnd->box.box.lower.z;
            wid = 0.5f * MIN(dx, dz);
            break;
        }
        }

        xVec3 pos_drvArena;
        xVec3Copy(&pos_drvArena, xEntGetCenter(drv_data->driver));
        pos_drvArena.y = model->Mat->pos.y;

        arena.AdjustHome(this, &pos_drvArena, -1.0f);
    }

    if (flg_move & 0x2)
    {
        CheckFalling(dt);
    }

    colFreq = MIN(-1, colFreq);

    DoFX_Motorboat(dt);

    if (DBG_IsNormLog((en_npcdcat)12, 2))
    {
        arena.DBG_Draw(NULL);
    }

    if (globals.player.BadGuyNearTimer < 0.1f && globals.player.VictoryTimer < 0.1f &&
        psy->GIDInStack('NGR4') && arena.IsReady() && arena.IncludesPlayer(0.0f, NULL))
    {
        TellPlayerVillainIsNear(0.5f);
    }
}

void zNPCRobot::CheckFalling(F32 dt)
{
    static const S32 skipstates[] = { 'NGRi', 'NGRd', 'NGRg', 'NGRh', 'NGRf', 'NGRb', 'NGRc',
                                      'NGR]', 'NGR^', 'NGR_', 'NGRj', 'NGRk', 0 };

    if ((flg_move & 0x2) && !(frame->oldvel.y > -2.0f) && arena.IsReady())
    {
        S32 selftype = SelfType();

        if (selftype != 'NTR:' && selftype != 'NTR6')
        {
            S32 i;
            S32 inSkipList = 0;
            S32 gid = psy_instinct->GIDOfActive();

            for (i = 0; skipstates[i];)
            {
                if (gid == skipstates[i++])
                {
                    inSkipList = 1;
                    break;
                }
            }

            if (!inSkipList)
            {
                xVec3 vec;
                XYZVecToPos(&vec, arena.Pos());

                if (!(vec.y < 10.0f))
                {
                    Damage(DMGTYP_INSTAKILL, NULL, NULL);
                }
            }
        }
    }
}

void zNPCRobot::BunnyHopSet(xVec3* vel)
{
    static const xVec3 vel_dflt = { 0.0f, 5.0f, 0.0f };

    if (vel)
    {
        xVec3Copy(&vel_bunnyhop, vel);
    }
    else
    {
        xVec3Copy(&vel_bunnyhop, &vel_dflt);
    }

    frame->vel.y = MAX(0.0f, frame->vel.y);
    frame->dvel.y = MAX(0.0f, frame->dvel.y);
}

void zNPCRobot::AddLassoing(xPsyche* psyche, xGoalProcessCallback cb1, xGoalProcessCallback cb2,
                            xGoalProcessCallback cb3, xGoalProcessCallback cb4,
                            xGoalProcessCallback cb5)
{
    if (!(flg_vuln & 0x1000000))
    {
        return;
    }

    xGoal* goal;
    goal = psyche->AddGoal(NPC_GOAL_LASSOBASE, NULL);
    goal->SetCallbacks(cb1, NULL, NULL, NULL);

    goal = psyche->AddGoal(NPC_GOAL_LASSOGRAB, NULL);
    goal->SetCallbacks(cb2, NULL, NULL, NULL);

    goal = psyche->AddGoal(NPC_GOAL_LASSOTHROW, NULL);
    goal->SetCallbacks(cb3, NULL, NULL, NULL);
}

void zNPCRobot::AddMiscTypical(xPsyche* psyche, xGoalProcessCallback cb1, xGoalProcessCallback cb2,
                               xGoalProcessCallback cb3)
{
    xGoal* goal;

    goal = psyche->AddGoal(NPC_GOAL_NOTICE, NULL);
    goal->SetCallbacks(cb1, NULL, NULL, NULL);

    goal = psyche->AddGoal(NPC_GOAL_TAUNT, NULL);
    goal->SetCallbacks(cb2, NULL, NULL, NULL);

    goal = psyche->AddGoal(NPC_GOAL_NOMANLAND, NULL);
    goal->SetCallbacks(cb3, NULL, NULL, NULL);
}

void zNPCRobot::AddStunThrow(xPsyche* psyche, xGoalProcessCallback cb1, xGoalProcessCallback cb2,
                             xGoalProcessCallback cb3, xGoalProcessCallback cb4)
{
    xGoal* goal;

    goal = psyche->AddGoal(NPC_GOAL_EVILPAT, NULL);
    goal->SetCallbacks(cb1, NULL, NULL, NULL);

    goal = psyche->AddGoal(NPC_GOAL_STUNNED, NULL);
    goal->SetCallbacks(cb2, NULL, NULL, NULL);

    if (cfg_npc->pts_damage < 2)
    {
        goal = psyche->AddGoal(NPC_GOAL_PATCARRY, NULL);
        goal->SetCallbacks(cb3, NULL, NULL, NULL);

        goal = psyche->AddGoal(NPC_GOAL_PATTHROW, NULL);
        goal->SetCallbacks(cb4, NULL, NULL, NULL);
    }
}

void zNPCRobot::AddDamage(xPsyche* psyche, xGoalProcessCallback cb1, xGoalProcessCallback cb2,
                          xGoalProcessCallback cb3, xGoalProcessCallback cb4,
                          xGoalProcessCallback cb5)
{
    xGoal* goal;

    goal = psyche->AddGoal(NPC_GOAL_DAMAGE, NULL);
    goal->SetCallbacks(cb1, NULL, NULL, NULL);

    goal = psyche->AddGoal(NPC_GOAL_KNOCK, NULL);
    goal->SetCallbacks(cb2, NULL, NULL, NULL);

    goal = psyche->AddGoal(NPC_GOAL_BASHED, NULL);
    goal->SetCallbacks(cb3, NULL, NULL, NULL);
}

void zNPCRobot::AddSpawning(xPsyche* psyche, xGoalProcessCallback cb1, xGoalProcessCallback cb2)
{
    xGoal* goal;

    goal = psyche->AddGoal(NPC_GOAL_AFTERLIFE, NULL);
    goal->SetCallbacks(cb1, NULL, NULL, NULL);

    goal = psyche->AddGoal(NPC_GOAL_RESPAWN, NULL);
    goal->SetCallbacks(cb2, NULL, NULL, NULL);
}

S32 zNPCRobot::LassoSetup()
{
    S32 idx_grab = -1;
    S32 idx_hold = -1;

    LassoModelIndex(&idx_grab, &idx_hold);
    if ((idx_grab >= 0) && (idx_hold >= 0))
    {
        LassoUseGuides(idx_grab, idx_hold);
    }
    return zNPCCommon::LassoSetup();
}

S32 zNPCRobot::IsDying()
{
    S32 dying = 0;
    xGoal* goal;

    if (psy_instinct == NULL)
    {
        return 0;
    }

    goal = psy_instinct->GetCurGoal();

    if (goal == NULL)
    {
        return 0;
    }

    switch (goal->GetID())
    {
    case NPC_GOAL_DAMAGE:
    case NPC_GOAL_KNOCK:
    case NPC_GOAL_R54:
    case NPC_GOAL_BASHED:
    case NPC_GOAL_R56:
        dying = 2;
        break;
    case NPC_GOAL_LIMBO:
    case NPC_GOAL_AFTERLIFE:
        dying = 1;
        break;
    }

    return dying;
}

S32 zNPCRobot::IsWounded()
{
    return IsDead() ? 0 : cfg_npc->pts_damage - hitpoints;
}

S32 zNPCRobot::SetCarryState(en_NPC_CARRY_STATE stat)
{
    S32 result = 0;

    if (IsDead())
    {
        return 0;
    }

    xGoal* goal = psy_instinct->GetCurGoal();

    if (goal == NULL)
    {
        return 0;
    }

    switch (stat)
    {
    case zNPCCARRY_NONE:
        if (goal->GetID() == NPC_GOAL_PATTHROW)
        {
            psy_instinct->GoalSet(NPC_GOAL_DAMAGE, 0);
        }
        else
        {
            psy_instinct->GoalSet(NPC_GOAL_DAMAGE, 0);
        }
        break;
    case zNPCCARRY_PICKUP:
        if (goal->GetID() == NPC_GOAL_STUNNED)
        {
            psy_instinct->GoalSwap(NPC_GOAL_PATCARRY, 0);
            result = 1;
        }
        break;
    case zNPCCARRY_THROW:
        if (goal->GetID() == NPC_GOAL_PATCARRY)
        {
            psy_instinct->GoalSwap(NPC_GOAL_PATTHROW, 0);
        }
        break;
    case zNPCCARRY_ATTEMPTPICKUP:
        if ((flg_vuln & 0x20000000) && goal->GetID() == NPC_GOAL_STUNNED)
        {
            result = 1;
        }
        break;
    }

    return result;
}

void zNPCRobot::Stun(F32 stuntime)
{
    static NPCMsg msg;

    if ((flg_vuln & 0x10000000) && !IsDying() && psy_instinct->GIDOfPending() != NPC_GOAL_EVILPAT)
    {
        memset(&msg, 0, sizeof(NPCMsg));

        msg.from = id;
        msg.sendto = id;
        msg.msgid = NPC_MID_STUN;
        msg.infotype = NPC_MDAT_STUN;
        msg.stundata.tym_stuntime = stuntime;

        zNPCMsg_SendMsg(&msg, this);
    }
}

void zNPCRobot::SyncStunGlyph(F32 dt, F32 tmr_remain, F32 height)
{
    NPCConfig* cfg = cfg_npc;

    if (glyf_stun != NULL)
    {
        xVec3 vec;
        S32 trun;

        xVec3Copy(&vec, Pos());

        if (height < 0.0f)
        {
            if (cfg->useBoxBound)
            {
                vec.y += cfg->off_bound.y + cfg->dim_bound.y;
            }
            else
            {
                vec.y += 2.0f * cfg->dim_bound.x + cfg->off_bound.y;
            }
        }
        else
        {
            vec.y += height;
        }

        glyf_stun->PosSet(&vec);
        glyf_stun->RotAddDelta(NULL);

        trun = 0;

        if (tmr_remain < 0.75f)
        {
            trun = (S32)(10.0f * tmr_remain);
        }
        else if (tmr_remain < 2.0f)
        {
            trun = (S32)(5.0f * tmr_remain);
        }

        if (trun & 1)
        {
            glyf_stun->Enable(0);
        }
        else
        {
            glyf_stun->Enable(1);
        }
    }
}

void zNPCRobot::LassoNotify(en_LASSO_EVENT event)
{
    if (!IsDead())
    {
        zNPCCommon::LassoNotify(event);
        switch (event)
        {
        case LASS_EVNT_GRABSTART:
            psy_instinct->GoalSet(0x4e47525d, 0); // NPC_GOAL_LASSOGRAB??
            break;
        }
    }
}

void zNPCRobot::CollideReview()
{
    zNPCGoalCommon* goal;
    S32 goaldidit = 0;
    xEntCollis* npccol = collis;
    xVec3 vec_depen = {};

    goal = (zNPCGoalCommon*)psy_instinct->GetCurGoal();

    if (goal != NULL && (goal->flg_npcgable & 1))
    {
        goal->Name();
        goaldidit = goal->CollReview(NULL);
    }

    if (goaldidit == 0)
    {
        S32 badsurf = 0;
        F32 goodep = 0.0f;
        xCollis* colrec = &npccol->colls[0];
        xSurface* surf;
        S32 i;

        if (colrec->flags & k_HIT_IT)
        {
            surf = zSurfaceGetSurface(colrec);

            if (surf != NULL && !surf->state && zSurfaceGetDamageType(surf))
            {
                badsurf = 1;
            }
            else if (colrec->optr != NULL && zGooIs((xEnt*)colrec->optr, goodep, 0))
            {
                badsurf = 1;
            }
        }

        for (i = npccol->env_sidx; i < npccol->env_eidx; i++)
        {
            colrec = &npccol->colls[i];

            xVec3AddTo(&vec_depen, &colrec->depen);
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

        if (badsurf)
        {
            Damage(DMGTYP_SURFACE, NULL, NULL);
        }
        else
        {
            zNPCCommon::CollideReview();
        }
    }
}

void zNPCRobot::InflictPain(S32 numHitPoints, S32 giveCreditToPlayer)
{
    if (numHitPoints < 0)
    {
        hitpoints = 0;
    }
    else
    {
        hitpoints -= numHitPoints;
    }

    hitpoints = (hitpoints > 0) ? hitpoints : 0;

    if (!hitpoints && giveCreditToPlayer)
    {
        zNPCCommon::GiveReward();
    }
}

void zNPCRobot::TurnThemHeads()
{
    xVec3 dir = {};
    xMat3x3 back = {};
    xMat3x3 mat = {};

    if (idx_neckBone >= 0)
    {
        xVec3Sub(&dir, xEntGetPos(&globals.player.ent), Pos());

        F32 dst_toPlyr = xVec3Normalize(&dir, &dir);

        if (dst_toPlyr > cfg_npc->rad_detect)
        {
            return;
        }

        if (xVec3Dot(&dir, NPCC_faceDir(this)) < 0.0f)
        {
            return;
        }

        xMat4x3* neck = (xMat4x3*)model->Mat + idx_neckBone;

        xVec3Inv(&dir, &dir);
        xMat3x3LookVec(&mat, &dir);
        xMat3x3Transpose(&back, (xMat3x3*)model->Mat);
        xMat3x3Mul(neck, &mat, &back);
    }
}

F32 zNPCRobot::FacePos(xVec3* pos, F32 dt, F32 spd_turn)
{
    xVec3 dir_pos;

    xVec3Sub(&dir_pos, pos, Pos());
    dir_pos.y = 0.0f;

    F32 dst = xVec3Length(&dir_pos);

    if (dst > 0.15f)
    {
        xVec3SMulBy(&dir_pos, 1.0f / dst);
        TurnToFace(dt, &dir_pos, spd_turn);
    }

    return dst;
}

F32 zNPCRobot::FaceAntiPlayer(F32 dt, F32 spd_turn)
{
    xVec3 dir_plyr;

    xVec3Sub(&dir_plyr, xEntGetPos(&globals.player.ent), xEntGetPos(this));

    if (flg_move & 0x2)
    {
        dir_plyr.y = 0.0f;
    }

    F32 dst = xVec3Length(&dir_plyr);

    if (dst > 0.15f)
    {
        xVec3SMulBy(&dir_plyr, -1.0f / dst);
        TurnToFace(dt, &dir_plyr, spd_turn);
    }

    return dst;
}

void zNPCRobot::CornerOfArena(xVec3* pos_corner, F32 dst)
{
    NPCArena* arena = &this->arena;
    xVec3 dir;
    xVec3 pos_a;
    xVec3 pos_b;

    XZVecToPos(&dir, arena->Pos(), NULL);

    F32 len = xVec3Length(&dir);

    if (len < 1.0f)
    {
        xVec3Copy(&dir, NPCC_rightDir(this));
    }
    else
    {
        xVec3SMulBy(&dir, 1.0f / len);
    }

    xVec3Cross(&pos_a, &g_Y3, &dir);
    xVec3SMul(&pos_b, &pos_a, -1.0f);

    if (dst < 2.0f)
    {
        dst = MAX(2.0f, 0.75f * arena->Radius(1.0f));
    }

    xVec3SMulBy(&pos_a, dst);
    xVec3SMulBy(&pos_b, dst);

    xVec3AddTo(&pos_a, arena->Pos());
    xVec3AddTo(&pos_b, arena->Pos());

    F32 dst_a = NPCC_DstSq(&pos_a, xEntGetPos(&globals.player.ent), NULL);
    F32 dst_b = NPCC_DstSq(&pos_b, xEntGetPos(&globals.player.ent), NULL);

    if (dst_a > dst_b && dst_a > SQ(2.0f))
    {
        xVec3Copy(pos_corner, &pos_a);
    }
    else if (dst_b > dst_a && dst_b > SQ(2.0f))
    {
        xVec3Copy(pos_corner, &pos_b);
    }
    else
    {
        xVec3Copy(pos_corner, (xrand() & 0x800000) ? &pos_a : &pos_b);
    }
}

F32 zNPCRobot::MoveTowardsArena(F32 dt, F32 speed)
{
    xVec3 dir_home;

    XYZVecToPos(&dir_home, arena.Pos());

    F32 dst = xVec3Length(&dir_home);

    if (dst < 0.5f)
    {
        return dst;
    }

    xVec3SMulBy(&dir_home, 1.0f / dst);
    ThrottleAdjust(dt, speed, -1.0f);
    ThrottleApply(dt, &dir_home, 0);

    return dst;
}

void zNPCRobot::ShowerConfetti(xVec3* pos)
{
    const xVec3 pos_emit = *(pos ? pos : xEntGetCenter(this));

    zNPCRobot_TubeConfetti(&pos_emit);
}

void zNPCRobot_TubeConfetti(const xVec3* pos_emit)
{
    S32 i;
    xVec3 vel_emit;

    for (i = 0; i < 32; i++)
    {
        vel_emit = g_Y3 * 0.5f + g_X3 * (2.0f * (xurand() - 0.5f)) +
                   g_Z3 * (2.0f * (xurand() - 0.5f));
        vel_emit.normalize();
        vel_emit *= 12.0f;

        NPAR_EmitTubeConfetti(pos_emit, &vel_emit);
    }

    zFX_SpawnBubbleSlam(pos_emit, 64, PI, 2.0f, 2.0f);
}

void zNPCFodder::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_move |= 0x10;
    flg_vuln &= 0x9effffff;
    idx_neckBone = 3;
}

void zNPCFodder::ParseINI()
{
    zNPCRobot::ParseINI();
    cfg_npc->snd_trax = g_sndTrax_Fodder;
    NPCS_SndTablePrepare(g_sndTrax_Fodder);
}

void zNPCFodder::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, FODR_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTFODDER, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKFODDER, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCFodder::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_IDLE:
    case NPC_GOAL_ALERT:
        idx = 1;
        break;
    case NPC_GOAL_NOTICE:
        idx = 4;
        break;
    case NPC_GOAL_ALERTFODDER:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 3;
        }
        else
        {
            idx = 1;
        }
        break;
    case NPC_GOAL_ATTACKFODDER:
        idx = 0xe;
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

/*
void zNPCFodder::Stun(F32 stuntime)
{
    xVec3 dir_dmg;

    if (this->IsWounded())
    {
        return;
    }

    xVec3* pos = xEntGetPos(&globals.player.ent);
    xVec3* robot_pos = xEntGetPos(this);

    xVec3Sub(&dir_dmg, pos, robot_pos);
    F32 out = xVec3Normalize(&dir_dmg, robot_pos);

    this->Respawn(pos, NULL, NULL);
}
*/

void zNPCFodder::Stun(F32 stuntime)
{
    if (!IsWounded())
    {
        xVec3 dir_dmg = { 0.0f, 0.0f, 0.0f };

        xVec3Sub(&dir_dmg, xEntGetPos(this), xEntGetPos(&globals.player.ent));
        xVec3Normalize(&dir_dmg, &dir_dmg);

        Damage(DMGTYP_SIDE, &globals.player.ent, &dir_dmg);
    }
}

void zNPCFodBomb::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_move |= 0x10;
    flg_vuln &= 0x9effffff;
    idx_neckBone = 3;
    rast_blink = 0;
}

void zNPCFodBomb::Setup()
{
    zNPCCommon::Setup();
    if (rast_blink == NULL)
    {
        rast_blink = NPCC_FindRWRaster("fx_fodbomb_blinker");
    }
}

void zNPCFodBomb::ParseINI()
{
    zNPCRobot::ParseINI();
    cfg_npc->snd_trax = g_sndTrax_FodBomb;
    NPCS_SndTablePrepare(g_sndTrax_FodBomb);
}

void zNPCFodBomb::SelfSetup()
{
    flg_vuln &= 0xfeffffff;

    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, BOMB_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTFODBOMB, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

void zNPCFodBomb::Stun(F32 stuntime)
{
    if (!IsWounded())
    {
        xVec3 dir_dmg = { 0.0f, 0.0f, 0.0f };

        xVec3Sub(&dir_dmg, xEntGetPos(this), xEntGetPos(&globals.player.ent));
        xVec3Normalize(&dir_dmg, &dir_dmg);

        Damage(DMGTYP_SIDE, &globals.player.ent, &dir_dmg);
    }
}

U32 zNPCFodBomb::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERT:
    case NPC_GOAL_ALERTFODBOMB:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 3;
        }
        else
        {
            idx = 1;
        }
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCFodBomb::BlinkerReset()
{
    blinker.Reset();
    flg_xtrarend &= ~1;
}

void zNPCFodBomb::BlinkerUpdate(F32 dt, F32 pct_timeRemain)
{
    blinker.Update(dt, 1.0f - pct_timeRemain, 0.1f, 0.03f);
    flg_xtrarend |= 1;
}

void zNPCFodBomb::BlinkerRender()
{
    static const xVec3 vec_boneOffset = { 0.0f, 0.49f, 0.0f };

    xVec3 pos_blink = vec_boneOffset;

    xMat3x3RMulVec(&pos_blink, (const xMat3x3*)BoneMat(3), &pos_blink);
    pos_blink += *(const xVec3*)BonePos(3);

    xMat3x3RMulVec(&pos_blink, (const xMat3x3*)BoneMat(0), &pos_blink);
    pos_blink += *(const xVec3*)BonePos(0);

    blinker.Render(&pos_blink, 0.42f, rast_blink);
}

void zNPCFodBzzt::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);

    flg_move &= 0xfffffffd;
    flg_move |= 4;

    flg_vuln &= 0x9effffff;

    idx_neckBone = -1;

    laser.Prepare();

    g_cnt_fodbzzt++;

    tmr_hokeypokey = -1.0f;
    g_needMusician = 0;
    zNPCFodBzzt::tmr_nexthokey = -1.0f;
    zNPCFodBzzt::rast_discoLight = NULL;
}

void zNPCFodBzzt::ParseINI()
{
    zNPCRobot::ParseINI();
    cfg_npc->snd_trax = g_sndTrax_FodBzzt;
    NPCS_SndTablePrepare(g_sndTrax_FodBzzt);
}

void zNPCFodBzzt::Setup()
{
    zNPCCommon::Setup();

    xAnimState* astate = AnimFindState(g_hash_roboanim[14]);

    if (astate)
    {
        astate->Flags &= ~0x20;
        astate->Flags |= 0x10;
    }

    if (!laser.TextureGet())
    {
        RwRaster* rast = NPCC_FindRWRaster("fx_fodbzzt_deathray");
        RwRGBA rgba_beg = { 0, 255, 0, 255 };
        RwRGBA rgba_end = { 0, 204, 0, 0 };
        F32 uv_scroll[2] = { 0.0f, 10.0f };
        F32 radius[2] = { 0.01f, 0.3f };

        laser.TextureSet(rast);
        laser.ColorSet(&rgba_beg, &rgba_end);
        laser.UVScrollSet(uv_scroll[0], uv_scroll[1]);
        laser.RadiusSet(radius[0], radius[1]);
    }

    if (zNPCFodBzzt::rast_discoLight == NULL)
    {
        zNPCFodBzzt::rast_discoLight = NPCC_FindRWRaster("fx_fodbzzt_disco");
    }
}

void zNPCFodBzzt::Reset()
{
    zNPCRobot::Reset();
}

void zNPCFodBzzt::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, BZZT_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTFODBZZT, NULL);
    psy->AddGoal(NPC_GOAL_HOKEYPOKEY, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

void zNPCFodBzzt::Process(xScene* sc, F32 dt)
{
    zNPCRobot::Process(sc, dt);
    if (g_needuvincr_bzzt != 0)
    {
        g_needuvincr_bzzt = 0;
        laser.UVScrollUpdate(dt);
    }
}

void zNPCFodBzzt::Stun(F32 stuntime)
{
    if (!IsWounded())
    {
        xVec3 dir_dmg = { 0.0f, 0.0f, 0.0f };

        xVec3Sub(&dir_dmg, xEntGetPos(this), xEntGetPos(&globals.player.ent));
        xVec3Normalize(&dir_dmg, &dir_dmg);

        Damage(DMGTYP_SIDE, &globals.player.ent, &dir_dmg);
    }
}

U32 zNPCFodBzzt::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERT:
    case NPC_GOAL_ALERTFODBZZT:
        idx = 0xe;
        break;
    case NPC_GOAL_HOKEYPOKEY:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0x22;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0x23;
        }
        else if (gspot == NPC_GSPOT_FINISH)
        {
            idx = 0x24;
        }
        else
        {
            idx = 0x23;
        }
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCFodBzzt::RenderExtra()
{
    xPsyche* psy = psy_instinct;
    S32 gid = psy->GIDOfActive();

    if (gid == NPC_GOAL_ALERTFODBZZT)
    {
        zNPCGoalAlertFodBzzt* alert = (zNPCGoalAlertFodBzzt*)psy->GetCurGoal();
        alert->DeathRayRender();
    }
    else if (gid == NPC_GOAL_HOKEYPOKEY)
    {
        DiscoRender();
    }

    zNPCCommon::RenderExtra();
}

void zNPCFodBzzt_ResetDanceParty()
{
    zNPCFodBzzt::cnt_alerthokey = 0;
}

void zNPCFodBzzt_DoTheHokeyPokey(F32 dt)
{
    static S32 g_somebodyplay;
    static S8 init;

    if (globals.player.Health < 1 || globals.player.DamageTimer > 0.25f ||
        (globals.player.ControlOff & 0xffffbeff) || zNPCFodBzzt::cnt_alerthokey < 1)
    {
        zNPCFodBzzt::tmr_hokeypokey = -1.0f;
        zNPCFodBzzt::tmr_nexthokey = 16.0f * (0.25f * (xurand() - 0.5f)) + 16.0f;
    }
    else
    {
        if (init == 0)
        {
            g_somebodyplay = 1;
            init = 1;
        }

        if (!((zNPCFodBzzt::tmr_hokeypokey < 0.0f) ? 1 : 0))
        {
            zNPCFodBzzt::tmr_hokeypokey = MAX(-1.0f, zNPCFodBzzt::tmr_hokeypokey - dt);
            zNPCFodBzzt::tmr_nexthokey = 16.0f * (0.25f * (xurand() - 0.5f)) + 16.0f;

            if (zNPCFodBzzt::tmr_hokeypokey < 1.0f && !g_needMusician && !g_somebodyplay)
            {
                g_needMusician = 1;
                g_somebodyplay = 1;
            }
            else if (zNPCFodBzzt::tmr_hokeypokey < 0.5f)
            {
                g_needMusician = 0;
            }
        }
        else if (!((zNPCFodBzzt::tmr_nexthokey < 0.0f) ? 1 : 0))
        {
            zNPCFodBzzt::tmr_nexthokey = MAX(-1.0f, zNPCFodBzzt::tmr_nexthokey - dt);
            g_somebodyplay = 0;
        }
        else
        {
            zNPCFodBzzt::tmr_hokeypokey = 5.0f * (0.25f * (xurand() - 0.5f)) + 5.0f;
            g_somebodyplay = 0;
        }
    }
}

void zNPCFodBzzt::DiscoReset()
{
    tmr_discoLight = -1.0f;
}

void zNPCFodBzzt::DiscoUpdate(F32 dt)
{
    static const F32 uv_scroll_discoLight[2] = { -0.2f, -0.2f };

    S32 needNewLight = (tmr_discoLight < 0.0f) ? 1 : 0;

    if (needNewLight)
    {
        tmr_discoLight = 0.75f;

        rgba_discoLight.red = ((xrand() >> 23) & 1) ? 0xff : 0;
        rgba_discoLight.green = ((xrand() >> 23) & 1) ? 0xff : 0;

        if (rgba_discoLight.red == 0 && rgba_discoLight.green == 0)
        {
            rgba_discoLight.blue = 0xff;
        }
        else
        {
            rgba_discoLight.blue = ((xrand() >> 23) & 1) ? 0xff : 0;
        }

        rgba_discoLight.alpha = 0xff;

        pos_discoLight.x = 0.5f * (2.0f * (xurand() - 0.5f));
        pos_discoLight.y = 1.75f;
        pos_discoLight.z = 0.5f * (2.0f * (xurand() - 0.5f));

        pos_discoLight += *Pos();
    }
    else
    {
        tmr_discoLight = MAX(-1.0f, tmr_discoLight - dt);
    }

    rgba_discoLight.alpha = (U8)(255.0f * (MAX(0.0f, tmr_discoLight) / 0.75f));

    uv_discoLight[0] += dt * uv_scroll_discoLight[0];
    uv_discoLight[1] += dt * uv_scroll_discoLight[1];

    RANGEWRAP(&uv_discoLight[0], 0.0f, 1.0f);
    RANGEWRAP(&uv_discoLight[1], 0.0f, 1.0f);

    flg_xtrarend |= 1;
}

F32 RANGEWRAP(F32* val, F32 lo, F32 hi)
{
    F32 range = xabs(hi - lo);
    F32 v = *val;

    while (v > hi)
    {
        v -= range;
    }
    while (v < lo)
    {
        v += range;
    }

    *val = v;
    return v;
}

void zNPCFodBzzt::DiscoRender()
{
    RwRGBA rgba_disco = rgba_discoLight;
    RwRGBA rgba_top = rgba_discoLight;
    RwRGBA rgba_bot = rgba_discoLight;

    rgba_bot.alpha = 0;
    rgba_top.alpha = rgba_top.alpha ? rgba_top.alpha : 0;

    xVec3 pos_top = pos_discoLight;
    xVec3 pos_bot = *Pos();
    const xVec3 vec_ray = { 0.0f, 0.0f, 1.0f };
    xVec3 pos_vtx;
    xMat3x3 mat_spin;

    pos_bot.y += 1.0f;

    F32 uv_top[2] = { 0.5f, 0.0f };
    F32 uv_bot[2] = { 0.0f, 1.0f };

    uv_top[0] = uv_discoLight[0] + 0.5f * uv_slice_discoLight[0];
    uv_top[1] = uv_discoLight[1];
    uv_bot[0] = uv_discoLight[0] + uv_slice_discoLight[0];
    uv_bot[1] = uv_discoLight[1] + uv_slice_discoLight[1];

    void* mem = xMemPushTemp(10 * sizeof(RwIm3DVertex));

    if (!mem)
    {
        return;
    }

    memset(mem, 0, 10 * sizeof(RwIm3DVertex));

    RwIm3DVertex* vert_list = (RwIm3DVertex*)mem;
    RwIm3DVertex* vtx = vert_list + 1;

    RwIm3DVertexSetPos(&vert_list[0], pos_top.x, pos_top.y, pos_top.z);
    RwIm3DVertexSetRGBA(&vert_list[0], rgba_top.red, rgba_top.green, rgba_top.blue,
                        rgba_top.alpha);
    RwIm3DVertexSetUV(&vert_list[0], uv_top[0], uv_top[1]);

    F32 u_bot = uv_bot[0];
    F32 v_bot = uv_bot[1];

    for (S32 i = 0; i < 8; i++)
    {
        F32 uoff = 0.125f * i;

        xMat3x3Euler(&mat_spin, (PI / 4) * i, 0.0f, 0.0f);
        xMat3x3LMulVec(&pos_vtx, &mat_spin, &vec_ray);
        pos_vtx *= 0.2f;
        pos_vtx += pos_bot;

        RwIm3DVertexSetPos(vtx, pos_vtx.x, pos_vtx.y, pos_vtx.z);
        RwIm3DVertexSetRGBA(vtx, rgba_bot.red, rgba_bot.green, rgba_bot.blue, rgba_bot.alpha);
        RwIm3DVertexSetUV(vtx, u_bot + uoff, v_bot);

        vtx++;
    }

    *vtx = vert_list[1];
    RwIm3DVertexSetUV(vtx, uv_bot[0] + uv_slice_discoLight[0], uv_bot[1]);

    _SDRenderState old_rendstat = zRenderStateCurrent();

    if (old_rendstat == SDRS_Unknown)
    {
        old_rendstat = SDRS_Default;
    }

    zRenderState(SDRS_NPCVisual);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)rast_discoLight);
    RwIm3DTransform(vert_list, 10, NULL, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA | rwIM3D_VERTEXUV);
    RwIm3DRenderPrimitive(rwPRIMTYPETRIFAN);
    RwIm3DEnd();

    zRenderState(old_rendstat);

    xMemPopTemp(mem);
}

void zNPCChomper::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_move |= 0x10;
    flg_vuln &= 0x9effffff;
    idx_neckBone = -1;
    NPAR_PartySetup(NPAR_TYP_DOGBREATH, NULL, NULL);
}

void zNPCChomper::ParseINI()
{
    zNPCRobot::ParseINI();
    cfg_npc->snd_trax = g_sndTrax_Chomper;
    NPCS_SndTablePrepare(g_sndTrax_Chomper);
}

void zNPCChomper::SelfSetup()
{
    flg_vuln &= 0xfeffffff;

    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, CHMP_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTCHOMPER, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKCHOMPER, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

void zNPCChomper::Stun(F32 stuntime)
{
    if (!IsWounded())
    {
        xVec3 dir_dmg = { 0.0f, 0.0f, 0.0f };

        xVec3Sub(&dir_dmg, xEntGetPos(this), xEntGetPos(&globals.player.ent));
        xVec3Normalize(&dir_dmg, &dir_dmg);

        Damage(DMGTYP_SIDE, &globals.player.ent, &dir_dmg);
    }
}

void zNPCChomper::Process(xScene* xscn, F32 dt)
{
    if (IsAlive())
    {
        S32 gid = psy_instinct->GIDOfActive();
        if (gid != NPC_GOAL_ATTACKCHOMPER && gid != NPC_GOAL_RESPAWN)
        {
            BreathTrail();
        }
    }

    zNPCRobot::Process(xscn, dt);
}

U32 zNPCChomper::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERT:
    case NPC_GOAL_ALERTCHOMPER:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 3;
        }
        else
        {
            idx = 1;
        }
        break;
    case NPC_GOAL_ATTACKCHOMPER:
        idx = 0xe;
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCChomper::BreathTrail()
{
    static const xVec3 vec_boneOffsetLeft = { -0.32f, -0.2f, 0.3f };
    static const xVec3 vec_boneOffsetRight = { 0.32f, -0.2f, 0.3f };

    if (--cnt_skipEmit <= 0)
    {
        if (--cnt_spurt < 0)
        {
            cnt_spurt = 10;
            cnt_skipEmit = (S32)(20.0f * (2.0f * (xurand() - 0.5f))) + 4;
        }

        {
            xVec3 pos_emit = *(xVec3*)BonePos(3);
            pos_emit += vec_boneOffsetRight;
            xMat3x3RMulVec(&pos_emit, (const xMat3x3*)BoneMat(0), &pos_emit);
            pos_emit += *(xVec3*)BonePos(0);

            xVec3 vel_emit = *NPCC_rightDir(this);
            vel_emit *= 2.5f * xurand() + 1.75f;

            NPAR_EmitDoggyWisps(&pos_emit, &vel_emit);
        }

        {
            xVec3 pos_emit = *(xVec3*)BonePos(3);
            pos_emit += vec_boneOffsetLeft;
            xMat3x3RMulVec(&pos_emit, (const xMat3x3*)BoneMat(0), &pos_emit);
            pos_emit += *(xVec3*)BonePos(0);

            xVec3 vel_emit = *NPCC_rightDir(this);
            vel_emit *= -(2.5f * xurand() + 1.75f);

            NPAR_EmitDoggyWisps(&pos_emit, &vel_emit);
        }
    }
}

void zNPCCritter::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_vuln = 0;
    idx_neckBone = -1;
}

void zNPCCritter::SelfSetup()
{
    zNPCRobot::SelfSetup();
    psy_instinct->SetSafety(NPC_GOAL_IDLE);
}

void zNPCHammer::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_vuln |= 0x60000000;
    idx_neckBone = -1;
}

void zNPCHammer::ParseINI()
{
    zNPCRobot::ParseINI();
    cfg_npc->snd_trax = g_sndTrax_Hammer;
    NPCS_SndTablePrepare(g_sndTrax_Hammer);
}

void zNPCHammer::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, HAMR_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTHAMMER, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKHAMMER, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCHammer::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERT:
    case NPC_GOAL_ALERTHAMMER:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 3;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 3;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 3;
        }
        else
        {
            idx = 3;
        }
        break;
    case NPC_GOAL_ATTACKHAMMER:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0xe;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0xe;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 0x11;
        }
        break;
    case NPC_GOAL_EVILPAT:
    case NPC_GOAL_STUNNED:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 9;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 10;
        }
        break;
    case NPC_GOAL_PATCARRY:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0x18;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0x19;
        }
        break;
    case NPC_GOAL_PATTHROW:
        idx = 0x1a;
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCTarTar::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_vuln &= 0x9fffffff;
    idx_neckBone = -1;
    NPAR_PartySetup(NPAR_TYP_TARTARGUNK, 0, 0);
}

void zNPCTarTar::ParseINI()
{
    zNPCRobot::ParseINI();
    cfg_npc->snd_trax = g_sndTrax_TarTar;
    NPCS_SndTablePrepare(g_sndTrax_TarTar);
}

void zNPCTarTar::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, TART_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTTARTAR, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKTARTAR, NULL);
    psy->AddGoal(NPC_GOAL_WOUND, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCTarTar::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERT:
    case NPC_GOAL_ALERTTARTAR:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0xc;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0xc;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 3;
        }
        else
        {
            idx = 0xc;
        }
        break;
    case NPC_GOAL_ATTACKTARTAR:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0x11;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0xe;
        }
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCGlove::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_vuln = -1;
    idx_neckBone = -1;
    NPAR_PartySetup(NPAR_TYP_GLOVEDUST, 0, 0);
}

void zNPCGlove::ParseINI()
{
    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_Glove;
    NPCS_SndTablePrepare(g_sndTrax_Glove);
}

void zNPCGlove::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, GLOV_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTGLOVE, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCGlove::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERTGLOVE:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0x11;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0x12;
        }
        else
        {
            idx = 0x12;
        }
        break;
    case NPC_GOAL_EVILPAT:
    case NPC_GOAL_STUNNED:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 9;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 10;
        }
        break;
    case NPC_GOAL_PATCARRY:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0x18;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0x19;
        }
        break;
    case NPC_GOAL_PATTHROW:
        idx = 0x1a;
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCMonsoon::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_move &= ~2;
    flg_move |= 4;
    flg_vuln &= 0x9fffffff;
    idx_neckBone = -1;
    NPAR_PartySetup(NPAR_TYP_MONSOONRAIN, NULL, NULL);
}

void zNPCMonsoon::Reset()
{
    zNPCRobot::Reset();
}

void zNPCMonsoon::ParseINI()
{
    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_Monsoon;
    NPCS_SndTablePrepare(g_sndTrax_Monsoon);
}

void zNPCMonsoon::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, MOON_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTMONSOON, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKMONSOON, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCMonsoon::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERT:
    case NPC_GOAL_ALERTMONSOON:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0xc;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0xc;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 0xd;
        }
        else
        {
            idx = 0xc;
        }
        break;
    case NPC_GOAL_ATTACKMONSOON:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0x11;
        }
        else
        {
            idx = 0x12;
        }
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCMonsoon::Process(xScene* xscn, F32 dt)
{
    zNPCRobot::Process(xscn, dt);

    if (psy_instinct != NULL && psy_instinct->GIDInStack(NPC_GOAL_ALERT))
    {
        FoulWeather(dt);
    }
}

void zNPCMonsoon::NewTime(xScene* sc, F32 dt)
{
    zNPCRobot::NewTime(sc, dt);
}

U8 zNPCMonsoon::FoulWeather(F32)
{
    return 0;
}

RwRaster* zNPCSleepy::rast_killcone;

RwRaster* zNPCSleepy::rast_detectcone;

volatile F32 zNPCSleepy::hyt_NightLightCurrent;

static S32 g_sleepy_NightLightStates[10] = { 'NGN0', 'NGN2', 'NGN1', 'NGN3', 'NGN4',
                                             'NGR4', 'NGR=', 'NGR0', 'NGR1', 0 };

static S32 g_sleepy_angryStates[5] = { 'NGR4', 'NGR0', 'NGRi', 'NGR=', 0 };

// Scheduling
void zNPCSleepy_Timestep(F32 dt)
{
    static F32 tmr_cycle;
    static S8 init;

    if (init == 0)
    {
        tmr_cycle = 0.0f;
        init = 1;
    }

    F32 dVar1 = NPCC_TmrCycle(&tmr_cycle, 0.016666667f, 2.63f);
    zNPCSleepy::hyt_NightLightCurrent = 4.0f;
    zNPCSleepy::hyt_NightLightCurrent += (0.35f * isin(PI * dVar1));
}

void zNPCSleepy::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);

    flg_move |= 2;

    flg_vuln &= 0x8fffffff;

    idx_neckBone = -1;

    rast_detectcone = NULL;
    rast_killcone = NULL;

    // Weirdness related to this line.
    g_cnt_sleepy++;

    NPAR_PartySetup(NPAR_TYP_SLEEPYZEEZ, NULL, NULL);
}

void zNPCSleepy::Reset()
{
    zNPCRobot::Reset();

    ModelAtomicShow(0, NULL);
    ModelAtomicHide(1, NULL);
    ModelAtomicShow(4, NULL);

    if (rast_killcone == NULL)
    {
        rast_killcone = NPCC_FindRWRaster("fx_sleepy_beamodeath");
    }

    if (rast_detectcone == NULL)
    {
        rast_detectcone = NPCC_FindRWRaster("fx_sleepy_nightlight");
    }
}

void zNPCSleepy::ParseINI()
{
    static F32 rad_minimum;
    static S8 init;

    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_Sleepy;
    NPCS_SndTablePrepare(g_sndTrax_Sleepy);

    if (init == 0)
    {
        rad_minimum = 5.0f;
        init = 1;
    }
    cfg_npc->rad_detect = MAX(rad_minimum, cfg_npc->rad_detect);
}

void zNPCSleepy::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(SLEP_grul_goAlert, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(SLEP_grul_goAlert, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(SLEP_grul_goAlert, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(SLEP_grul_goAlert, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(SLEP_grul_goAlert, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, SLEP_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTSLEEPY, NULL);
    psy->AddGoal(NPC_GOAL_WOUND, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCSleepy::AnimPick(int gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 uVar1 = 0;
    int iVar2 = -1;

    switch (gid)
    {
    case 'NGR4':
        iVar2 = 0xc;
        break;
    case 'NGR=':
        iVar2 = 0xe;
        break;
    default:
        uVar1 = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (iVar2 >= 0)
    {
        uVar1 = g_hash_roboanim[iVar2];
    }

    return uVar1;
}

void zNPCSleepy::Process(xScene* xscn, F32 dt)
{
    zNPCRobot::Process(xscn, dt);

    if (!IsDead())
    {
        S32 cnt_awake = 0;

        if (globals.player.bubblebowl)
        {
            cnt_awake = RepelBowlBall(dt);
        }

        cnt_awake += RepelMissile(dt);

        S32 gid = psy_instinct->GIDOfActive();

        for (S32 i = 0; g_sleepy_angryStates[i]; i++)
        {
            if (gid == g_sleepy_angryStates[i])
            {
                cnt_awake++;
                break;
            }
        }

        ConeOfRange(dt, cnt_awake);

        if (cnt_awake == 0)
        {
            SnoreNZeez(dt);
        }

        if (g_needuvincr_nightlight)
        {
            g_needuvincr_nightlight = 0;
            NightLightUVStep(dt);
        }
    }
}

void zNPCSleepy::NewTime(xScene* sc, F32 dt)
{
    if (!IsDead())
    {
        xModelInstance* nightlight = zNPCCommon::ModelAtomicFind(4, -1, NULL);
        if (nightlight == NULL)
        {
            return;
        }
        NightLightPos((xVec3*)(&nightlight->Mat->pos));
    }
    zNPCRobot::NewTime(sc, dt);
}

void zNPCSleepy::NightLightPos(xVec3* vec)
{
    xVec3Copy(vec, Pos());
    vec->y += hyt_NightLightCurrent;
}

void zNPCSleepy::NightLightUVStep(F32 dt)
{
    static const F32 uv_scroll_nightlight[2] = { 0.0f, -0.2f };
    static const F32 uv_scroll_deathcone[2] = { 0.0f, -1.55f };

    zNPCSleepy::uv_nightlight[0] += dt * uv_scroll_nightlight[0];
    zNPCSleepy::uv_nightlight[1] += dt * uv_scroll_nightlight[1];
    RANGEWRAP(&zNPCSleepy::uv_nightlight[0], 0.0f, 1.0f);
    RANGEWRAP(&zNPCSleepy::uv_nightlight[1], 0.0f, 1.0f);

    zNPCSleepy::uv_deathcone[0] += dt * uv_scroll_deathcone[0];
    zNPCSleepy::uv_deathcone[1] += dt * uv_scroll_deathcone[1];
    RANGEWRAP(&zNPCSleepy::uv_deathcone[0], 0.0f, 1.0f);
    RANGEWRAP(&zNPCSleepy::uv_deathcone[1], 0.0f, 1.0f);
}

void zNPCSleepy::SnoreNZeez(F32 dt)
{
    xVec3 pos_zeez;
    xVec3 dir_zeez;

    F32 ds2_toCam = NPCC_ds2_toCam(Pos(), NULL);

    if (ds2_toCam > SQ(30.0f))
    {
        return;
    }

    tmr_emitzeez = MAX(-1.0f, tmr_emitzeez - dt);

    if (tmr_emitzeez < 0.0f)
    {
        cnt_grpzeez -= 1.0f;

        if (cnt_grpzeez < 0.0f)
        {
            cnt_grpzeez = 3.0f;
            tmr_emitzeez = 3.0f;
        }
        else
        {
            tmr_emitzeez = 0.4f;
        }

        xVec3Copy(&pos_zeez, (const xVec3*)BonePos(6));
        xMat3x3RMulVec(&pos_zeez, (const xMat3x3*)BoneMat(0), &pos_zeez);
        xVec3AddTo(&pos_zeez, Pos());
        pos_zeez.y += 0.5f;

        xVec3Inv(&dir_zeez, NPCC_faceDir(this));
        dir_zeez += g_Y3;
        dir_zeez.normalize();
        dir_zeez *= 1.5f;

        NPAR_EmitSleepyZeez(&pos_zeez, &dir_zeez);
    }
}

S32 zNPCSleepy::RepelMissile(F32 dt)
{
    tmr_nextPatriot = MAX(-1.0f, tmr_nextPatriot - dt);
    if (haz_patriot != NULL && !(tmr_nextPatriot < 0.0f))
    {
        return 1;
    }
    haz_patriot = NULL;
    return 0;
}

void xEntBoulder_AddForce(xEntBoulder* ent, xVec3* force);

S32 zNPCSleepy::RepelBowlBall(F32 dt)
{
    xEntBoulder* bowl = globals.player.bubblebowl;

    static S32 next_los_check = 0;
    static volatile S32 los_said_ok = 0;
    static S32 keepgoing = 0;
    static S32 nextfx = 0;

    xVec3 vec_force;
    xVec3 vec_NtoB;
    xVec3 dir_NtoB;
    xVec3 dir_ball;
    xVec3 dir_repel;
    xVec3 pos_side;
    xVec3 vec_tmp;

    if (bowl == NULL)
    {
        return 0;
    }

    if (!xEntIsVisible(bowl))
    {
        return 0;
    }

    xVec3Sub(&vec_NtoB, xEntGetPos(bowl), xEntGetPos(this));

    F32 dst = xVec3Normalize(&dir_NtoB, &vec_NtoB);

    if (dst > 12.0f)
    {
        return 0;
    }

    if (xVec3Dot(NPCC_faceDir(this), &dir_NtoB) < -0.86f)
    {
        return 0;
    }

    F32 spd = xVec3Normalize(&dir_ball, &bowl->vel);

    if (xVec3Dot(&dir_ball, &dir_NtoB) > 0.0f && spd > 4.0f)
    {
        if (keepgoing-- < 0)
        {
            return 0;
        }
    }
    else
    {
        keepgoing = 15;
    }

    if (--next_los_check < 0)
    {
        next_los_check = 10.0f * (0.25f * (xurand() - 0.5f)) + 10.0f;

        los_said_ok =
            HaveLOSToPos(xEntGetPos(bowl), dst, globals.sceneCur, bowl, NULL);

        if (los_said_ok)
        {
            next_los_check /= 2;
        }

        xSceneID2Name(globals.sceneCur, id);
    }

    xVec3Inv(&vec_force, &vec_NtoB);

    F32 rat = xVec3Dot(&vec_force, &dir_ball);

    xVec3SMul(&pos_side, &dir_ball, rat);
    xVec3AddTo(&pos_side, xEntGetPos(bowl));

    XYZVecToPos(&dir_repel, &pos_side);
    xVec3Normalize(&dir_repel, &dir_repel);

    if (DBG_IsNormLog(eNPCDCAT_Thirteen, 2))
    {
        xDrawSetColor(g_CYAN);
        xDrawSphere(&pos_side, 0.25f, 0xC0006);
    }

    rat = MAX(0.25f, 1.0f - dst / 12.0f);

    F32 mag_push = 60.0f * rat;

    xVec3SMul(&vec_force, &dir_NtoB, mag_push);
    xEntBoulder_AddForce(bowl, &vec_force);

    F32 mag_side = 60.0f * (0.25f * rat);

    xVec3SMul(&vec_force, &dir_repel, mag_side);
    xEntBoulder_AddForce(bowl, &vec_force);

    if (DBG_IsNormLog(eNPCDCAT_Thirteen, 2))
    {
        xDrawSetColor(g_RED);
        xVec3SMul(&vec_tmp, &dir_NtoB, mag_push);
        xVec3AddTo(&vec_tmp, xEntGetCenter(bowl));
        xDrawLine(xEntGetCenter(bowl), &vec_tmp);

        xDrawSetColor(g_YELLOW);
        xVec3SMul(&vec_tmp, &dir_repel, mag_side);
        xVec3AddTo(&vec_tmp, xEntGetCenter(bowl));
        xDrawLine(xEntGetCenter(bowl), &vec_tmp);
    }

    flg_sleepy |= 1;

    return 1;
}

void zNPCSleepy::ConeOfRange(F32 dt, S32 which)
{
    static const F32 rgb_peace[3] = { 1.0f, 1.0f, 0.63f };
    static const F32 rgb_anger[3] = { 0.5f, 0.0f, 0.0f };

    F32 rgb_current[3];
    S32 i;

    F32 rad2 = SQ(cfg_npc->rad_detect);
    F32 dst2 = XZDstSqToPlayer(NULL, NULL);
    F32 pct = 0.2f + (1.0f - dst2 / rad2);

    pct = MAX(0.0f, MIN(pct, 1.0f));

    for (i = 0; i < 3; i++)
    {
        rgb_current[i] = SMOOTH(pct, rgb_peace[i], rgb_anger[i]);
    }

    rgba_coneColor.red = (U8)(255.0f * rgb_current[0]);
    rgba_coneColor.green = (U8)(255.0f * rgb_current[1]);
    rgba_coneColor.blue = (U8)(255.0f * rgb_current[2]);
    rgba_coneColor.alpha = 0;

    flg_xtrarend |= 1;
}

void zNPCSleepy::RenderExtra()
{
    xPsyche* psy = psy_instinct;

    if (!(SomethingWonderful() & 1))
    {
        if (psy != NULL && !(model->Flags & 0x400))
        {
            S32 gid = psy->GIDOfActive();

            if (gid == NPC_GOAL_ALERTSLEEPY)
            {
                RendConeOfDeath(0);
            }

            if (gid != 0 && gid != NPC_GOAL_RESPAWN)
            {
                RendConeRange();
            }
        }

        if (flg_sleepy & 1)
        {
            RendConeOfDeath(1);
        }

        flg_sleepy &= ~1;

        zNPCCommon::RenderExtra();
    }
}

static RxObjSpace3DVertex g_vert_list[34];

void zNPCSleepy::RendConeOfDeath(S32 tgt_isBowlingBall)
{
    static RwRGBA rgba_beg = { 0, 200, 240, 0 };
    static RwRGBA rgba_end = { 80, 204, 204, 255 };

    xVec3 pos_top;
    xVec3 pos_bot;

    if (tgt_isBowlingBall)
    {
        GetVertPos(NPC_MDLVERT_ATTACK, &pos_top);

        xEntBoulder* bowl = globals.player.bubblebowl;

        if (bowl == NULL)
        {
            return;
        }

        xVec3Copy(&pos_bot, xEntGetCenter(bowl));
    }
    else
    {
        GetVertPos(NPC_MDLVERT_ATTACK, &pos_top);
        xVec3Copy(&pos_bot, xEntGetPos(&globals.player.ent));
    }

    F32 u_beg = zNPCSleepy::uv_deathcone[0];
    F32 v_beg = zNPCSleepy::uv_deathcone[1];
    F32 u_end = zNPCSleepy::uv_deathcone[0] + zNPCSleepy::uv_slice_deathcone[0];
    F32 v_end = zNPCSleepy::uv_deathcone[1] + zNPCSleepy::uv_slice_deathcone[1];

    RxObjSpace3DVertex* vtx = g_vert_list;

    for (S32 i = 0; i < 16; i++)
    {
        F32 ang_seg = (PI / 8) * i;
        F32 sn = isin(ang_seg);
        F32 cs = icos(ang_seg);

        xVec3 vec_ray = { 0.0f, 0.0f, 0.0f };
        xVec3 pos_vtx;

        vec_ray.z = cs;
        vec_ray.x = sn;

        pos_vtx = vec_ray * 0.1f + pos_top;

        RwIm3DVertexSetPos(&vtx[0], pos_vtx.x, pos_vtx.y, pos_vtx.z);
        RwIm3DVertexSetRGBA(&vtx[0], rgba_beg.red, rgba_beg.green, rgba_beg.blue,
                            rgba_beg.alpha);

        F32 u_off = 0.0625f * ((i & 1) ? i : i + 1);
        F32 u_seg = u_beg + u_off;

        RwIm3DVertexSetUV(&vtx[0], u_seg, v_beg + 0.0f);

        pos_vtx = vec_ray * 0.5f + pos_bot;

        RwIm3DVertexSetPos(&vtx[1], pos_vtx.x, pos_vtx.y, pos_vtx.z);
        RwIm3DVertexSetRGBA(&vtx[1], rgba_end.red, rgba_end.green, rgba_end.blue,
                            rgba_end.alpha);
        RwIm3DVertexSetUV(&vtx[1], u_seg, v_end);

        vtx += 2;
    }

    vtx[0] = g_vert_list[0];
    RwIm3DVertexSetUV(&vtx[0], u_end, v_beg);

    vtx[1] = g_vert_list[1];
    RwIm3DVertexSetUV(&vtx[1], u_end, v_end);

    _SDRenderState old_rendstat = zRenderStateCurrent();

    if (old_rendstat == SDRS_Unknown)
    {
        old_rendstat = SDRS_Default;
    }

    zRenderState(SDRS_NPCVisual);

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, (void*)zNPCSleepy::rast_killcone);
    RwIm3DTransform(g_vert_list, 34, NULL,
                    rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA | rwIM3D_VERTEXUV);
    RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
    RwIm3DEnd();

    zRenderState(old_rendstat);
}

void zNPCSleepy::RendConeRange()
{
    static F32 uv_top[2] = { 0.0f, 0.0f };
    static F32 uv_bot[2] = { 0.0f, 1.0f };

    xMat3x3 mat_spin;
    xVec3 pos_top;
    xVec3 pos_bot;
    xVec3 pos_vtx;
    RwRGBA rgba_top;
    RwRGBA rgba_bot;

    F32 rad_cone = cfg_npc->rad_detect;

    NightLightPos(&pos_top);

    pos_bot = *Pos();

    rgba_top = rgba_coneColor;
    rgba_bot = rgba_coneColor;

    F32 ds2 = XZDstSqToPlayer(NULL, NULL);

    if (ds2 < SQ(20.0f))
    {
        rgba_top.alpha = 128;
        rgba_bot.alpha = 32;
    }
    else if (ds2 > SQ(30.0f))
    {
        rgba_top.alpha = 0;
        rgba_bot.alpha = 0;
    }
    else
    {
        F32 pct = 1.0f - (xsqrt(ds2) - 20.0f) / 10.0f;

        pct = CLAMP(pct, 0.0f, 1.0f);

        rgba_top.alpha = (U8)SMOOTH(pct, 0.0f, 128.0f);
        rgba_bot.alpha = (U8)SMOOTH(pct, 0.0f, 32.0f);
    }

    xVec3 vec_ray = { 0.0f, 0.0f, 1.0f };

    RxObjSpace3DVertex* vtx = g_vert_list;

    uv_top[0] = zNPCSleepy::uv_nightlight[0];
    uv_top[1] = zNPCSleepy::uv_nightlight[1];
    uv_bot[0] = zNPCSleepy::uv_nightlight[0] + zNPCSleepy::uv_slice_nightlight[0];
    uv_bot[1] = zNPCSleepy::uv_nightlight[1] + zNPCSleepy::uv_slice_nightlight[1];

    for (S32 i = 0; i < 16; i++)
    {
        xMat3x3Euler(&mat_spin, (PI / 8) * i, 0.0f, 0.0f);

        vec_ray.z = 0.0f;

        xMat3x3LMulVec(&pos_vtx, &mat_spin, &vec_ray);
        xVec3AddTo(&pos_vtx, &pos_top);

        RwIm3DVertexSetPos(&vtx[0], pos_vtx.x, pos_vtx.y, pos_vtx.z);
        RwIm3DVertexSetRGBA(&vtx[0], rgba_top.red, rgba_top.green, rgba_top.blue,
                            rgba_top.alpha);

        F32 u_ofs = 0.0625f * ((i & 1) ? i : i + 1);

        RwIm3DVertexSetUV(&vtx[0], uv_top[0] + u_ofs, uv_top[1]);

        vec_ray.z = rad_cone;

        xMat3x3LMulVec(&pos_vtx, &mat_spin, &vec_ray);
        xVec3AddTo(&pos_vtx, &pos_bot);

        RwIm3DVertexSetPos(&vtx[1], pos_vtx.x, pos_vtx.y, pos_vtx.z);
        RwIm3DVertexSetRGBA(&vtx[1], rgba_bot.red, rgba_bot.green, rgba_bot.blue,
                            rgba_bot.alpha);
        RwIm3DVertexSetUV(&vtx[1], uv_top[0] + u_ofs, uv_bot[1]);

        vtx += 2;
    }

    vtx[0] = g_vert_list[0];
    RwIm3DVertexSetUV(&vtx[0], uv_bot[0], uv_top[1]);

    vtx[1] = g_vert_list[1];
    RwIm3DVertexSetUV(&vtx[1], uv_bot[0], uv_bot[1]);

    _SDRenderState old_rendstat = zRenderStateCurrent();

    if (old_rendstat == SDRS_Unknown)
    {
        old_rendstat = SDRS_Default;
    }

    zRenderState(SDRS_NPCVisual);

    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwIm3DTransform(g_vert_list, 34, NULL, rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);
    RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
    RwIm3DEnd();

    zRenderState(old_rendstat);
}

void zNPCArfArf::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_move |= 2;
    flg_vuln &= ~0x60000000;
    idx_neckBone = -1;
}

void zNPCArfArf::Reset()
{
    zNPCRobot::Reset();
    for (int i = 0; i < sizeof(flg_puppy) / sizeof(S32); i++)
    {
        flg_puppy[i] = 1;
    }
}

void zNPCArfArf::ParseINI()
{
    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_ArfArf;
    NPCS_SndTablePrepare(g_sndTrax_ArfArf);
}

void zNPCArfArf::ParseLinks()
{
    zNPCCommon::ParseLinks();

    for (S32 i = 0; i < linkCount; i++)
    {
        xLinkAsset* lnk = &link[i];

        if (lnk->dstEvent == eEventConnectToChild)
        {
            xSceneID2Name(globals.sceneCur, id);
            xSceneID2Name(globals.sceneCur, lnk->dstAssetID);

            xBase* mychild = zSceneFindObject(lnk->dstAssetID);

            if (mychild != NULL &&
                (mychild->baseType == eBaseTypeNPC || mychild->baseType == eBaseTypeGroup))
            {
                ParseChild(mychild);
            }
        }
    }
}

void zNPCArfArf::ParseChild(xBase* child)
{
    if (child->baseType == eBaseTypeNPC)
    {
        if (((xNPCBasic*)child)->SelfType() == NPC_TYPE_ARFDOG)
        {
            zNPCArfDog* pup = (zNPCArfDog*)child;
            S32 i;

            for (i = 0; i < 5; i++)
            {
                if (pup_kennel[i] == NULL)
                {
                    pup_kennel[i] = pup;
                    pup->DuploOwner(this);
                    break;
                }
            }

            xSceneID2Name(globals.sceneCur, id);
            xSceneID2Name(globals.sceneCur, pup->id);
        }
    }
    else if (child->baseType == eBaseTypeGroup)
    {
        xSceneID2Name(globals.sceneCur, id);

        U32 cnt = xGroupGetCount((xGroup*)child);

        for (S32 i = 0; i < (S32)cnt; i++)
        {
            xBase* grpitem = xGroupGetItemPtr((xGroup*)child, i);

            if (grpitem != NULL)
            {
                if (grpitem->baseType == eBaseTypeNPC)
                {
                    ParseChild(grpitem);
                }
                else if (grpitem->baseType == eBaseTypeGroup)
                {
                    ParseChild(grpitem);
                }
            }
        }
    }
}

void zNPCArfArf::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, ARFY_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTARF, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKARF, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKARFMELEE, NULL);
    psy->AddGoal(NPC_GOAL_TELEPORT, NULL);
    psy->AddGoal(NPC_GOAL_WOUND, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCArfArf::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 da_anim = 0;
    S32 idx = -1;

    switch (gid)
    {
    case 'NGR>':
        idx = 1;
        break;
    case 'NGRJ':
    {
        zNPCGoalAttackArf* atkgoal = (zNPCGoalAttackArf*)rawgoal;

        if (atkgoal->flg_attack & 0x1)
        {
            idx = 14;
        }
        else if (atkgoal->flg_attack & 0x2)
        {
            idx = 15;
        }
        else
        {
            idx = 14;
        }
        break;
    }
    case 'NGRK':
        idx = 16;
        break;
    case 'NGR[':
        idx = 30;
        break;
    case 'NGRi':
        if (hitpoints >= 2)
        {
            idx = 20;
        }
        else if (hitpoints == 1)
        {
            idx = 22;
        }
        break;
    default:
        da_anim = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        da_anim = g_hash_roboanim[idx];
    }

    return da_anim;
}

void zNPCArfArf::DuploNotice(en_SM_NOTICES note, void* data)
{
    S32 i;

    switch (note)
    {
    case SM_NOTE_NPCALIVE:
        for (i = 0; i < 5; i++)
        {
            if (pup_kennel[i] == (zNPCArfDog*)data)
            {
                flg_puppy[i] &= ~1;
                return;
            }
        }
        break;
    case SM_NOTE_NPCSTANDBY:
        for (i = 0; i < 5; i++)
        {
            if (pup_kennel[i] == (zNPCArfDog*)data)
            {
                flg_puppy[i] |= 1;
                return;
            }
        }
        break;
    }
}

zNPCArfDog* zNPCArfArf::AdoptADoggie()
{
    zNPCArfDog* pup = NULL;

    for (S32 i = 0; i < 5; i++)
    {
        if (pup_kennel[i] != NULL && (flg_puppy[i] & 1))
        {
            pup = pup_kennel[i];
            break;
        }
    }

    return pup;
}

RwRaster* zNPCArfDog::rast_blink;

void zNPCArfDog::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_move |= 2;
    flg_vuln &= 0x9effffff;
    idx_neckBone = -1;
    rast_blink = 0;
}

void zNPCArfDog::Setup()
{
    zNPCCommon::Setup();
    if (rast_blink == NULL)
    {
        rast_blink = NPCC_FindRWRaster("fx_fodbomb_blinker");
    }
}

void zNPCArfDog::Reset()
{
    zNPCRobot::Reset();
    BlinkReset();
}

void zNPCArfDog::ParseINI()
{
    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_ArfDog;
    NPCS_SndTablePrepare(g_sndTrax_ArfDog);
}

void zNPCArfDog::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, PUPY_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTPUPPY, NULL);
    psy->AddGoal(NPC_GOAL_DOGLAUNCH, NULL);
    psy->AddGoal(NPC_GOAL_DOGBARK, NULL);
    psy->AddGoal(NPC_GOAL_DOGDASH, NULL);
    psy->AddGoal(NPC_GOAL_DOGPOUNCE, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCArfDog::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERT:
    case NPC_GOAL_ALERTPUPPY:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0xc;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0xc;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 0xd;
        }
        else
        {
            idx = 0xc;
        }
        break;
    case NPC_GOAL_DOGLAUNCH:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0x20;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0x20;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 0x20;
        }
        else if (gspot == NPC_GSPOT_FINISH)
        {
            idx = 0x21;
        }
        else
        {
            idx = 0x20;
        }
        break;
    case NPC_GOAL_DOGBARK:
        idx = 4;
        break;
    case NPC_GOAL_DOGDASH:
        idx = 0xd;
        break;
    case NPC_GOAL_DOGPOUNCE:
        idx = 0x12;
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCArfDog::Stun(F32 stuntime)
{
    if (!IsWounded())
    {
        xVec3 dir_dmg = { 0.0f, 0.0f, 0.0f };

        xVec3Sub(&dir_dmg, xEntGetPos(this), xEntGetPos(&globals.player.ent));
        xVec3Normalize(&dir_dmg, &dir_dmg);

        Damage(DMGTYP_SIDE, &globals.player.ent, &dir_dmg);

        SndPlayRandom(NPC_STYP_OUCH);
    }
}

void zNPCArfDog::Process(xScene* xscn, F32 dt)
{
    zNPCRobot::Process(xscn, dt);

    if (IsDead())
    {
        return;
    }

    S32 gid = psy_instinct->GIDOfActive();
    F32 ratio = 0.0f;

    switch (gid)
    {
    case 'NGRW':
        ratio += 0.3333f;
    case 'NGRX':
        ratio += 0.3333f;
    case 'NGRY':
        ratio += 0.3333f;
    case 'NGRZ':
        BlinkUpdate(dt, ratio);
        break;
    }
}

void zNPCArfDog::BlinkReset()
{
    blinkHead.Reset();
    blinkTail.Reset();
    flg_xtrarend &= ~1;
}

void zNPCArfDog::BlinkUpdate(F32 dt, F32 ratio)
{
    blinkHead.Update(dt, ratio, 0.03f, 0.03f);
    blinkTail.Update(dt, ratio, 0.03f, 0.03f);
    flg_xtrarend |= 1;
}

void zNPCArfDog::BlinkRender()
{
    static const xVec3 vec_headOffset = { 0.0f, 0.23f, 0.0f };
    static const xVec3 vec_tailOffset = { 0.0f, 0.0f, -0.32f };

    xVec3 pos_head = vec_headOffset;

    xMat3x3RMulVec(&pos_head, (const xMat3x3*)BoneMat(5), &pos_head);
    pos_head += *(const xVec3*)BonePos(5);

    xMat3x3RMulVec(&pos_head, (const xMat3x3*)BoneMat(0), &pos_head);
    pos_head += *(const xVec3*)BonePos(0);

    blinkHead.Render(&pos_head, 0.25f, rast_blink);

    xVec3 pos_tail = vec_tailOffset;

    xMat3x3RMulVec(&pos_tail, (const xMat3x3*)BoneMat(9), &pos_tail);
    pos_tail += *(const xVec3*)BonePos(9);

    xMat3x3RMulVec(&pos_tail, (const xMat3x3*)BoneMat(0), &pos_tail);
    pos_tail += *(const xVec3*)BonePos(0);

    blinkTail.Render(&pos_tail, 0.22f, rast_blink);
}

void zNPCChuck::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_move &= 0xfffffffd;
    flg_move |= 4;
    flg_vuln &= 0x9fffffff;
    idx_neckBone = -1;
    NPAR_PartySetup(NPAR_TYP_CHUCKSPLASH, 0, 0);
}

void zNPCChuck::Reset()
{
    zNPCRobot::Reset();
    xModelInstance* minst = ModelAtomicHide(1, NULL);
    if (minst != NULL)
    {
        minst->Flags &= 0xfffe;
    }
}

void zNPCChuck::ParseINI()
{
    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_Chuck;
    NPCS_SndTablePrepare(g_sndTrax_Chuck);
}

void zNPCChuck::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, CHUK_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTCHUCK, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKCHUCK, NULL);
    psy->BrainEnd();

    ((zNPCGoalIdle*)psy->FindGoal(NPC_GOAL_IDLE))->flg_idle |= 1;
    ((zNPCGoalWander*)psy->FindGoal(NPC_GOAL_WANDER))->flg_wand |= 1;
    ((zNPCGoalWaiting*)psy->FindGoal(NPC_GOAL_WAITING))->flg_waiting |= 1;

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCChuck::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERTCHUCK:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0xc;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0xc;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 0xd;
        }
        else
        {
            idx = 0xc;
        }
        break;
    case NPC_GOAL_ATTACKCHUCK:
        idx = 0xe;
        break;
    case NPC_GOAL_CHASE:
        idx = 0xd;
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCTubelet::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);

    flg_move |= 2;
    flg_vuln &= 0x9FFFFFFF;
    idx_neckBone = -1;

    psynote.npc = this;

    NPAR_PartySetup(NPAR_TYP_TUBESPIRAL, NULL, NULL);
    NPAR_PartySetup(NPAR_TYP_TUBECONFETTI, NULL, NULL);

    xModelInstance* iVar1 = zNPCCommon::ModelAtomicFind(1, -1, NULL);

    iVar1->Flags &= 0xffdf;
    iVar1->Flags |= 8;
}

void zNPCTubelet::Reset()
{
    tubestat = TUBE_STAT_BORN;
    zNPCRobot::Reset();
    NPCConfig* cfg = cfg_npc;
    hitpoints = cfg->pts_damage;
    ModelAtomicShow(0, NULL);
    ModelAtomicHide(1, NULL);
    ModelAtomicHide(4, NULL);
}

void zNPCTubelet::ParseINI()
{
    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_Tubelet;
    NPCS_SndTablePrepare(g_sndTrax_Tubelet);
}

void zNPCTubelet::ParseLinks()
{
    zNPCCommon::ParseLinks();

    for (S32 i = 0; i < linkCount; i++)
    {
        xLinkAsset* lnk = &link[i];

        if (lnk->dstEvent == eEventConnectToChild)
        {
            xSceneID2Name(globals.sceneCur, id);
            xSceneID2Name(globals.sceneCur, lnk->dstAssetID);

            xBase* mychild = zSceneFindObject(lnk->dstAssetID);

            if (mychild->baseType == eBaseTypeNPC || mychild->baseType == eBaseTypeGroup)
            {
                ParseChild(mychild);
            }
        }
    }
}

void zNPCTubelet::ParseChild(xBase* child)
{
    if (child->baseType == eBaseTypeNPC)
    {
        if (((xNPCBasic*)child)->SelfType() == NPC_TYPE_TUBESLAVE)
        {
            S32 spot = ROBO_TUBE_PETE;

            if (tub_paul == NULL)
            {
                tub_paul = (zNPCTubeSlave*)child;
                spot = ROBO_TUBE_PAUL;
            }
            else if (tub_mary == NULL)
            {
                tub_mary = (zNPCTubeSlave*)child;
                spot = ROBO_TUBE_MARY;
            }

            if (spot != ROBO_TUBE_PETE)
            {
                xSceneID2Name(globals.sceneCur, id);
                xSceneID2Name(globals.sceneCur, child->id);
                ((zNPCTubeSlave*)child)->SetMaster(this, (en_tubespot)spot);
            }
        }
        else
        {
            ((xNPCBasic*)child)->SelfType();
        }
    }
    else if (child->baseType == eBaseTypeGroup)
    {
        xGroup* grp = (xGroup*)child;

        xSceneID2Name(globals.sceneCur, id);

        U32 cnt = xGroupGetCount(grp);

        for (S32 i = 0; i < (S32)cnt; i++)
        {
            xBase* grpitem = xGroupGetItemPtr(grp, i);

            if (grpitem != NULL)
            {
                if (grpitem->baseType == eBaseTypeNPC)
                {
                    ParseChild(grpitem);
                }
                else if (grpitem->baseType == eBaseTypeGroup)
                {
                    ParseChild(grpitem);
                }
            }
        }
    }
}

void zNPCTubelet::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertMelee, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, TUBE_grul_alert, NULL, NULL);

    psy->SetNotify(&psynote);

    psy->BrainExtend();
    goal = psy->AddGoal(NPC_GOAL_ALERTTUBELET, NULL);
    ((zNPCGoalCommon*)goal)->flg_npcgauto |= 8;
    goal->AddFlags(0x10000);
    psy->AddGoal(NPC_GOAL_DEFLATE, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCTubelet::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_ALERTTUBELET:
    case NPC_GOAL_DEFLATE:
        idx = 3;
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCTubelet::Process(xScene* xscn, F32 dt)
{
    zNPCRobot::Process(xscn, dt);

    if (!IsDead())
    {
        Chk_NonAlertBonk(dt);
        Chk_IsBonked();
    }

    if (tubestat == TUBE_STAT_DYING && psy_instinct->GIDOfActive() != 'NGRN' &&
        psy_instinct->GIDOfPending() != 'NGRN')
    {
        psy_instinct->GoalSet('NGRN', 0);
    }
    else if (tubestat == TUBE_STAT_DEAD && psy_instinct->GIDOfActive() != 'NGRj' &&
             psy_instinct->GIDOfPending() != 'NGRj')
    {
        psy_instinct->GoalSet('NGRj', 0);
    }
}

S32 zNPCTubelet::Respawn(const xVec3* pos, zMovePoint* mvptFirst, zMovePoint* mvptSpawnRef)
{
    S32 rc = zNPCCommon::Respawn(pos, mvptFirst, mvptSpawnRef);
    if (rc != 0)
    {
        PrepTheBand();
    }
    return rc;
}

void zNPCTubelet::PrepTheBand()
{
    tubestat = TUBE_STAT_BORN;
    hitpoints = cfg_npc->pts_damage;
    ModelAtomicShow(0, NULL);
    ModelAtomicHide(1, NULL);
    ModelAtomicHide(4, NULL);
    tub_paul->WeGotAGig();
    tub_mary->WeGotAGig();
}

S32 zNPCTubelet::RoboHandleMail(NPCMsg* mail)
{
    S32 handled = 0;

    switch (mail->msgid)
    {
    case NPC_MID_SYSEVENT:
    {
        switch (mail->sysevent.toEvent)
        {
        case eEventNPCSetActiveOn:
        case eEventNPCSetActiveOff:
        {
            handled = 0;

            NPCMsg msg = *mail;
            zNPCTubeSlave* slave;

            slave = tub_paul;

            if (slave)
            {
                msg.sendto = slave->id;
                msg.sysevent.from = this;
                msg.sysevent.to = slave;
                slave->NPCMessage(&msg);
            }

            slave = tub_mary;

            if (slave)
            {
                msg.sendto = slave->id;
                msg.sysevent.from = this;
                msg.sysevent.to = slave;
                slave->NPCMessage(&msg);
            }

            break;
        }
        default:
            handled = 0;
            break;
        }

        break;
    }
    case NPC_MID_DAMAGE:
    {
        handled = 1;
        Bonk();
        break;
    }
    }

    return handled;
}

void zNPCTubelet::LassoNotify(en_LASSO_EVENT event)
{
    zNPCLassoInfo* lass = lassdata;

    if (IsDead())
    {
        return;
    }

    zNPCRobot::LassoNotify(event);

    switch (event)
    {
    case LASS_EVNT_BEGIN:
        tubestat = TUBE_STAT_LASSO;
        break;
    case LASS_EVNT_ENDED:
    case LASS_EVNT_ABORT:
        if ((lass->stage == LASS_STAT_PENDING || lass->stage == LASS_STAT_DONE) &&
            tubestat == TUBE_STAT_LASSO && hitpoints >= 1)
        {
            PrepTheBand();

            xPsyche* psy = psy_instinct;

            if (psy->GIDInStack(NPC_GOAL_ALERT))
            {
                tubestat = TUBE_STAT_ATTACK;
            }
            else if (psy->GIDInStack(NPC_GOAL_IDLE))
            {
                tubestat = TUBE_STAT_DUCKLING;
            }
            else
            {
                tubestat = TUBE_STAT_DUCKLING;
            }
        }
        break;
    }
}

void zNPCTubelet::Bonk()
{
    if (hitpoints != 0)
    {
        ModelAtomicHide(0, NULL);
        ModelAtomicShow(1, NULL);
        ModelAtomicHide(4, NULL);

        hitpoints = 0;
        pflags |= 0x20;

        bonkSpinRate = 6.2831855f * (2.0f * (xurand() - 0.5f)) + 12.566371f;

        ShowerConfetti(NULL);
        PainInTheBand();

        SndPlayRandom(NPC_STYP_BONKED);
        zNPC_SNDStop(eNPCSnd_TubeAttack);

        pete_attack_last = 0;
    }
}

void zNPCTubelet::Unbonk()
{
    ModelAtomicShow(0, NULL);
    ModelAtomicHide(1, NULL);
    ModelAtomicHide(4, NULL);
    hitpoints = cfg_npc->pts_damage;
    pflags &= (U8)0xdf;
    bonkSpinRate = -1.0f;
}

S32 zNPCTubelet::Chk_IsBonked()
{
    S32 die;
    S32 cnt_hurt = 0;
    S32 alldead = 0;

    if (tubestat == TUBE_STAT_DYING)
    {
        die = 1;
    }
    else if (tubestat == TUBE_STAT_DEAD)
    {
        die = 1;
    }
    else
    {
        zNPCTubeSlave* mary;

        bonkSpinRate = 0.0f;

        if (hitpoints == 0)
        {
            cnt_hurt = 1;
        }

        if (tub_paul == NULL)
        {
            cnt_hurt++;
        }
        else if (tub_paul->hitpoints == 0)
        {
            cnt_hurt++;
        }

        mary = tub_mary;

        if (mary == NULL)
        {
            cnt_hurt++;
        }
        else if (mary->hitpoints == 0)
        {
            cnt_hurt++;
        }

        if (cnt_hurt == 3)
        {
            alldead = 1;
        }

        if (alldead)
        {
            if (mary != NULL)
            {
                mary->hitpoints = 0;
            }

            if (tub_paul != NULL)
            {
                tub_paul->hitpoints = 0;
            }

            hitpoints = 0;
            tubestat = TUBE_STAT_DYING;
        }
        else if (hitpoints > 0)
        {
            Unbonk();
        }

        die = (hitpoints == 0);
    }

    return die;
}

void zNPCTubelet::PainInTheBand()
{
    tmr_restoreHealth = 2.5f;
}

void zNPCTubelet::Chk_NonAlertBonk(F32 dt)
{
    if (tubestat == TUBE_STAT_DUCKLING)
    {
        tmr_restoreHealth = MAX(-1.0f, tmr_restoreHealth - dt);

        if (tmr_restoreHealth < 0.0f &&
            hitpoints + tub_paul->hitpoints + tub_mary->hitpoints <= 2)
        {
            Unbonk();
            tub_paul->PartyOn();
            tub_mary->PartyOn();
        }
    }
}

void TubeNotice::Notice(en_psynote note, xGoal* goal, void* data)
{
    zNPCTubelet* pete = (zNPCTubelet*)npc;

    switch (note)
    {
    case PSY_NOTE_HASRESUMED:
    case PSY_NOTE_HASENTERED:
        break;
    default:
        return;
    }

    switch (goal->GetID())
    {
    case NPC_GOAL_ALERTTUBELET:
        if (pete->tubestat != TUBE_STAT_LASSO)
        {
            pete->tubestat = TUBE_STAT_ATTACK;
        }
        return;
    case NPC_GOAL_DEFLATE:
    case NPC_GOAL_DAMAGE:
    case NPC_GOAL_KNOCK:
        pete->tubestat = TUBE_STAT_DYING;
        return;
    case NPC_GOAL_LASSOBASE:
    case NPC_GOAL_LASSOGRAB:
    case NPC_GOAL_LASSOTHROW:
        pete->tubestat = TUBE_STAT_LASSO;
        return;
    case NPC_GOAL_AFTERLIFE:
        pete->tubestat = TUBE_STAT_DEAD;
        return;
    case NPC_GOAL_RESPAWN:
        pete->tubestat = TUBE_STAT_BORN;
        return;
    }

    if (pete->tubestat != TUBE_STAT_LASSO)
    {
        pete->tubestat = TUBE_STAT_DUCKLING;
    }
}

S32 zNPCTubelet::IsDying()
{
    return (tubestat == TUBE_STAT_DEAD) ? 1 : 0;
}

void zNPCTubeSlave::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);
    flg_move &= 0xfffffffd;
    flg_move |= 4;

    flg_vuln &= 0x8fffffff;
    flg_vuln &= 0xfeffffff;

    idx_neckBone = -1;
    tubespot = ROBO_TUBE_PAUL;
    tub_pete = NULL;

    laser.Prepare();

    xModelInstance* mdl = ModelAtomicFind(1, -1, NULL);
    mdl->Flags &= 0xffdf;
    mdl->Flags |= 8;
}

void zNPCTubeSlave::Setup()
{
    zNPCCommon::Setup();

    if (!laser.TextureGet())
    {
        RwTexture* txtr = NPCC_FindRWTexture("energy_beam");
        RwRaster* rast = NULL;

        if (txtr != NULL)
        {
            rast = txtr->raster;
        }

        const RwRGBA rgba_beg = { 255, 128, 255, 255 };
        const RwRGBA rgba_end = { 255, 128, 255, 255 };

        laser.TextureSet(rast);
        laser.UVScrollSet(0.1f, 0.0f);
        laser.RadiusSet(0.1f, 0.0f);
        laser.ColorSet(&rgba_beg, &rgba_end);
    }
}

void zNPCTubeSlave::Reset()
{
    zNPCRobot::Reset();
    flags |= 0x40;
    zNPCTubeSlave::WeGotAGig();
}

void zNPCTubeSlave::WeGotAGig()
{
    PartyOn();
    psy_instinct->GoalSet(0x4e47524f, 1);
}

void zNPCTubeSlave::PartyOn()
{
    ModelAtomicShow(0, NULL);
    ModelAtomicHide(1, NULL);
    ModelAtomicHide(4, NULL);
    hitpoints = cfg_npc->pts_damage;
}

void zNPCTubeSlave::ParseINI()
{
    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_Tubelet;
    NPCS_SndTablePrepare(g_sndTrax_Tubelet);
}

void zNPCTubeSlave::SelfSetup()
{
    flg_vuln &= 0xfeffffff;

    xBehaveMgr* bmgr = xBehaveMgr_GetSelf();
    psy_instinct = bmgr->Subscribe(this, 0);

    xPsyche* psy = psy_instinct;

    psy->BrainBegin();
    psy->AddGoal(NPC_GOAL_TAUNT, NULL);
    psy->AddGoal(NPC_GOAL_FIDGET, NULL);
    psy->AddGoal(NPC_GOAL_LIMBO, NULL);
    psy->AddGoal(NPC_GOAL_TUBEPAL, NULL);
    psy->AddGoal(NPC_GOAL_TUBEDUCKLING, NULL);
    psy->AddGoal(NPC_GOAL_TUBEATTACK, NULL);
    psy->AddGoal(NPC_GOAL_TUBELASSO, NULL);
    psy->AddGoal(NPC_GOAL_TUBEBIRTH, NULL);
    psy->AddGoal(NPC_GOAL_TUBEBONKED, NULL);
    psy->AddGoal(NPC_GOAL_TUBEDYING, NULL);
    psy->AddGoal(NPC_GOAL_TUBEDEAD, NULL);
    psy->BrainEnd();
    psy->SetSafety(NPC_GOAL_TUBEPAL);
}

U32 zNPCTubeSlave::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    U32 hashid = 0;
    S32 idx = -1;

    switch (gid)
    {
    case NPC_GOAL_TUBEPAL:
        idx = 1;
        break;
    case NPC_GOAL_TUBEDUCKLING:
        idx = 3;
        break;
    case NPC_GOAL_TUBELASSO:
        idx = 0xe;
        break;
    case NPC_GOAL_TUBEBIRTH:
        idx = 1;
        break;
    case NPC_GOAL_TUBEDEAD:
        idx = 0x25;
        break;
    case NPC_GOAL_TUBEATTACK:
        idx = 3;
        break;
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCTubeSlave::SetMaster(zNPCTubelet* pete, en_tubespot spot)
{
    tub_pete = pete;
    tubespot = spot;

    if (pete != NULL)
    {
        xVec3Copy(xEntGetPos(this), pete->Pos());
    }

    if (spot == ROBO_TUBE_MARY)
    {
        model->Mat->pos.y += 3.0f;
    }
    else
    {
        model->Mat->pos.y += 1.5f;
    }
}

void zNPCTubeSlave::Process(xScene* xscn, F32 dt)
{
    zNPCRobot::Process(xscn, dt);
}

void zNPCTubeSlave::RenderExtra()
{
    DoLaserRendering();
    zNPCCommon::RenderExtra();
}

S32 zNPCTubeSlave::RoboHandleMail(NPCMsg* mail)
{
    S32 handled = 0;
    xPsyche* psy = psy_instinct;

    switch (mail->msgid)
    {
    case NPC_MID_DAMAGE:
        if (hitpoints != 0)
        {
            handled = 1;
            if (!psy->GIDInStack(NPC_GOAL_TUBEBONKED))
            {
                psy->GoalPush(NPC_GOAL_TUBEBONKED, 0);
            }
        }
        break;
    }

    return handled;
}

S32 zNPCTubeSlave::IsDying()
{
    return tub_pete->IsDying();
}

void zNPCTubeSlave::PosStacked(xVec3* pos_stacked)
{
    F32 hyt;

    if (tubespot == ROBO_TUBE_MARY)
    {
        hyt = 3.0f;
    }
    else
    {
        hyt = 1.5f;
    }

    xVec3Copy(pos_stacked, xEntGetPos(tub_pete));
    pos_stacked->y += hyt;
}

void zNPCTubeSlave::DoLaserRendering()
{
    xGoal* goal;
    if (tubespot == ROBO_TUBE_PAUL)
    {
        goal = psy_instinct->GetCurGoal();
        if (goal != NULL && (goal->GetID() == NPC_GOAL_TUBEATTACK))
        {
            ((zNPCGoalTubeAttack*)(goal))->LaserRender();
        }
    }
}

static zNPCSlick* g_slick_slipfx_owner;

void zNPCSlick::Init(xEntAsset* asset)
{
    zNPCRobot::Init(asset);

    flg_move &= 0xfffffffd;
    flg_move |= 4;

    flg_vuln &= 0x9fffffff;
    flg_vuln |= 0x1000000;

    idx_neckBone = -1;

    if (!g_uvaShield.Valid())
    {
        xModelInstance* mdl = zNPCCommon::ModelAtomicFind(1, -1, NULL);
        if ((mdl != NULL) && (mdl->Data != NULL))
        {
            g_uvaShield.Init(mdl->Data, 0);
            g_uvaShield.UVVelSet(0.0f, -0.7f);
        }
    }
    NPAR_PartySetup(NPAR_TYP_OILBUB, NULL, NULL);
}

void zNPCSlick::Reset()
{
    zNPCRobot::Reset();
    tmr_repairShield = -1.0f;
    tmr_invuln = -1.0f;
    alf_shieldDesired = 100.0f / 255.0f;
    alf_shieldCurrent = 0.0f;
    rad_shield = 0.3f;
}

void zNPCSlick::ParseINI()
{
    NPCConfig* cfg = cfg_npc;
    zNPCRobot::ParseINI();
    cfg->snd_trax = g_sndTrax_Slick;
    NPCS_SndTablePrepare(g_sndTrax_Slick);
}

void zNPCSlick::SelfSetup()
{
    zNPCRobot::SelfSetup();

    xPsyche* psy = psy_instinct;
    xGoal* goal;

    goal = psy->FindGoal(NPC_GOAL_IDLE);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_FIDGET);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WAITING);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_PATROL);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_WANDER);
    goal->SetCallbacks(ROBO_grul_goAlertLobber, NULL, NULL, NULL);
    goal = psy->FindGoal(NPC_GOAL_ALERT);
    goal->SetCallbacks(NULL, SLCK_grul_alert, NULL, NULL);

    psy->BrainExtend();
    psy->AddGoal(NPC_GOAL_ALERTSLICK, NULL);
    psy->AddGoal(NPC_GOAL_ATTACKSLICK, NULL);
    psy->AddGoal(NPC_GOAL_WOUND, NULL);
    psy->BrainEnd();

    psy->SetSafety(NPC_GOAL_IDLE);
}

U32 zNPCSlick::AnimPick(S32 gid, en_NPC_GOAL_SPOT gspot, xGoal* rawgoal)
{
    static S32 choices[3] = { 20, 22, 21 };

    S32 idx = -1;
    U32 hashid = 0;

    switch (gid)
    {
    case NPC_GOAL_ALERT:
    case NPC_GOAL_ALERTSLICK:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 1;
        }
        else if (gspot == NPC_GSPOT_STARTALT)
        {
            idx = 3;
        }
        else
        {
            idx = 1;
        }
        break;
    case NPC_GOAL_ATTACKSLICK:
        if (gspot == NPC_GSPOT_START)
        {
            idx = 0x11;
        }
        else if (gspot == NPC_GSPOT_RESUME)
        {
            idx = 0x12;
        }
        else if (gspot == NPC_GSPOT_FINISH)
        {
            idx = 0x13;
        }
        else
        {
            idx = 0x11;
        }
        break;
    case NPC_GOAL_WOUND:
        idx = xUtil_choose(choices, 3, NULL);
        // fallthrough
    default:
        hashid = zNPCRobot::AnimPick(gid, gspot, rawgoal);
        break;
    }

    if (idx >= 0)
    {
        hashid = g_hash_roboanim[idx];
    }

    return hashid;
}

void zNPCSlick::Process(xScene* xscn, F32 dt)
{
    if (!IsDead())
    {
        StuffToDoIfAlive(dt);
    }

    if (globals.player.ForceSlipperyTimer > 0.0f)
    {
        if (this == g_slick_slipfx_owner)
        {
            SlipSlidenAway(dt);
        }
    }
    else
    {
        g_slick_slipfx_owner = NULL;
    }

    zNPCRobot::Process(xscn, dt);
}

void zNPCSlick::StuffToDoIfAlive(F32 dt)
{
    tmr_invuln = MAX(-1.0f, tmr_invuln - dt);

    ShieldUpdate(dt);
    ShieldCollide(dt);
    ShieldFX(dt);

    if (IsShield())
    {
        hitpoints = cfg_npc->pts_damage;
    }
}

void zNPCSlick::Damage(en_NPC_DAMAGE_TYPE dmg_type, xBase* who, const xVec3* vec_hit)
{
    switch (dmg_type)
    {
    case DMGTYP_ABOVE:
    case DMGTYP_BELOW:
    case DMGTYP_SIDE:
        if (IsShield())
        {
            zEntPlayer_DamageNPCKnockBack(this, 1, Pos());
            NPCC_Slick_MakePlayerSlip(this);
        }
        break;
    case DMGTYP_BUBBOWL:
        zEntEvent(who, eEventKill);
        break;
    }

    if (tmr_invuln < 0.0f)
    {
        if (!IsShield())
        {
            zNPCCommon::Damage(dmg_type, who, vec_hit);
        }
        else
        {
            ShieldGeneratorDamaged();
            tmr_invuln = 1.0f;
        }

        tmr_repairShield = 7.0f;
    }
}

void zNPCSlick::ShieldUpdate(F32 dt)
{
    tmr_repairShield = MAX(-1.0f, tmr_repairShield - dt);

    if (tmr_repairShield < 0.0f)
    {
        ShieldShow();
    }

    if (alf_shieldCurrent < alf_shieldDesired)
    {
        alf_shieldCurrent = alf_shieldCurrent + 0.39215687f * dt;
    }
    else if (alf_shieldCurrent > alf_shieldDesired)
    {
        alf_shieldCurrent = alf_shieldCurrent - 0.39215687f * dt;
    }

    alf_shieldCurrent = CLAMP(alf_shieldCurrent, 0.0f, 0.39215687f);

    F32 rat_shield = alf_shieldCurrent;

    rat_shield /= 0.39215687f;

    rad_shield = SMOOTH(rat_shield, 0.3f, 2.5f);

    xModelInstance* model = ModelAtomicFind(1, -1, NULL);

    if (alf_shieldCurrent < 0.01f)
    {
        ModelAtomicHide(-1, model);
    }
    else
    {
        ModelAtomicShow(-1, model);

        model->Scale.x = MAX(0.01f, rat_shield);
        model->Scale.y = 1.0f;
        model->Scale.z = model->Scale.x;
        model->Flags |= 0x4000;
        model->Alpha = alf_shieldCurrent;
        model->PipeFlags &= 0xffff00ff;
        model->PipeFlags |= 0x6521;
    }
}

void zNPCSlick::ShieldCollide(F32 dt)
{
    if (IsShield())
    {
        xCollis coll;

        memset(&coll, 0, sizeof(xCollis));
        xBoundHitsBound(&bound, &globals.player.ent.bound, &coll);

        if (coll.flags & k_HIT_IT)
        {
            zEntPlayer_DamageNPCKnockBack(this, 1, Pos());
            NPCC_Slick_MakePlayerSlip(this);
        }
    }
}

void zNPCSlick::ShieldFX(F32 dt)
{
    if (g_needuvincr_slickshield)
    {
        g_uvaShield.Update(dt, NULL);
        g_needuvincr_slickshield = 0;
    }
}

static S32 WhereTheWildThingsAre(xModelInstance* mdl)
{
    S32 numverts = mdl->Data->geometry->numVertices;

    if (numverts < 10)
    {
        return 0;
    }

    S32 step = numverts / 50;
    S32 i;

    for (i = 0; i < numverts; i += step)
    {
        xVec3 pos;

        iModelVertEval(mdl->Data, i, 1, mdl->Mat, NULL, &pos);
        NPAR_EmitOilShieldPop(&pos);
    }

    return 1;
}

// The target carries an unreferenced `offset$NNNN` of { 0.0f, 1.5f, 0.0f } in
// .rodata between zNPCArfDog::BlinkRender's two bone offsets and the `smidge`
// below, left behind by a function the retail link deadstripped.
void __deadstripped_zNPCTypeRobot_4()
{
    static const xVec3 offset = { 0.0f, 1.5f, 0.0f };
}

void zNPCSlick::ShieldGeneratorDamaged()
{
    xModelInstance* mdl = ModelAtomicFind(1, -1, NULL);

    if (mdl != NULL)
    {
        WhereTheWildThingsAre(mdl);
    }
    else
    {
        static const xVec3 smidge = { 0.0f, -0.5f, 0.0f };
        xVec3 pos;

        xVec3Add(&pos, Pos(), &smidge);
        NPCC_BurstBubble(NPC_BURST_SHIELD, &pos);
    }

    ShieldHide();
    tmr_repairShield = 7.0f;

    SndPlayRandom(NPC_STYP_OUCH);
}

void zNPCSlick::RopePopsShield()
{
    ShieldGeneratorDamaged();
    alf_shieldCurrent = 0.0f;
    tmr_repairShield = 5.0f;
}

void zNPCSlick::BUpdate(xVec3* pos)
{
    if (!IsShield())
    {
        zNPCCommon::BUpdate(pos);
    }
    else
    {
        xSphere& sph = bound.sph;

        bound.type = XBOUND_TYPE_SPHERE;
        xVec3Copy(&sph.center, pos);
        sph.center.y += MAX(rad_shield, 2.5f);
        sph.r = MAX(rad_shield, 1.25f);

        xQuickCullForBound(&bound.qcd, &bound);
        zGridUpdateEnt(this);
    }
}

S32 zNPCSlick::IsShield() const
{
    return alf_shieldDesired == 100.0f / 255.0f;
}

void zNPCSlick::ShieldHide()
{
    alf_shieldDesired = 0.0f;
}

void zNPCSlick::ShieldShow()
{
    alf_shieldDesired = 100.0f / 255.0f;
}

zNPCSlick* zNPCSlick::YouOwnSlipFX()
{
    return g_slick_slipfx_owner = this;
}

void zNPCSlick::SlipSlidenAway(F32 dt)
{
    static S32 moreorless;
    static S8 init;

    if (init == 0)
    {
        moreorless = 0;
        init = 1;
    }

    if (--moreorless < 0)
    {
        moreorless = 8;

        F32 rad = 1.2f * globals.player.ent.bound.sph.r;

        xVec3 pos_base = *xEntGetPos(&globals.player.ent);

        pos_base.y += 0.35f;

        for (U32 i = 0; i < 4; i++)
        {
            xVec3 pos_emit;

            pos_emit = *NPCC_upDir(&globals.player.ent) * (rad * (2.0f * (xurand() - 0.5f)));
            pos_emit += *NPCC_faceDir(&globals.player.ent) * (rad * (2.0f * (xurand() - 0.5f)));
            pos_emit += *NPCC_rightDir(&globals.player.ent) * (rad * (2.0f * (xurand() - 0.5f)));
            pos_emit += pos_base;

            NPAR_EmitOilVapors(&pos_emit);
        }
    }
}

S32 DUMY_grul_returnToIdle(xGoal* goal, void*, en_trantype* trantype, F32, void*)
{
    S32 nextgoal = 0;

    if (goal->GetPsyche()->TimerGet(XPSY_TYMR_CURGOAL) > 10.0f)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = NPC_GOAL_IDLE;
    }

    return nextgoal;
}

S32 ROBO_grul_goAlertMelee(xGoal* rawgoal, void*, en_trantype* trantype, F32, void*)
{
    zNPCRobot* npc = (zNPCRobot*)rawgoal->psyche->clt_owner;
    NPCArena* arena = &npc->arena;

    if (globals.player.Health < 1)
    {
        return 0;
    }

    if (npc->SomethingWonderful())
    {
        return 0;
    }

    if (npc->npcset.allowDetect == 0)
    {
        return 0;
    }

    S32 rc = arena->NeedToCycle(npc);

    if (rc == 2)
    {
        arena->Cycle(npc, 1);
    }
    else if (rc != 0)
    {
        arena->Cycle(npc, 0);
    }

    if (!arena->IsReady())
    {
        return 0;
    }

    if (!arena->IncludesNPC(npc, 0.75f, NULL))
    {
        return 0;
    }

    if (!arena->IncludesPlayer(0.5f, NULL))
    {
        return 0;
    }

    *trantype = GOAL_TRAN_SET;

    return NPC_GOAL_ALERT;
}

S32 ROBO_grul_goAlertLobber(xGoal* rawgoal, void*, en_trantype* trantype, F32, void*)
{
    zNPCRobot* npc = (zNPCRobot*)rawgoal->psyche->clt_owner;
    NPCArena* arena = &npc->arena;

    if (globals.player.Health < 1)
    {
        return 0;
    }

    if (npc->SomethingWonderful())
    {
        return 0;
    }

    if (npc->npcset.allowDetect == 0)
    {
        return 0;
    }

    S32 rc = arena->NeedToCycle(npc);

    if (rc == 2)
    {
        arena->Cycle(npc, 1);
    }
    else if (rc != 0)
    {
        arena->Cycle(npc, 0);
    }

    if (!arena->IsReady())
    {
        return 0;
    }

    if (!arena->IncludesNPC(npc, 0.75f, NULL))
    {
        return 0;
    }

    F32 dst_lob = MAX(1.5f * arena->Radius(1.0f), npc->cfg_npc->rad_attack);

    xVec3 delta;
    F32 dsq = arena->DstSqFromHome(xEntGetPos(&globals.player.ent), &delta);

    if (delta.y > 8.0f || dsq > SQ(0.8f * dst_lob))
    {
        return 0;
    }

    *trantype = GOAL_TRAN_SET;

    return NPC_GOAL_ALERT;
}

S32 FODR_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTFODDER;
}

S32 BOMB_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTFODBOMB;
}

S32 BZZT_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTFODBZZT;
}

S32 CHMP_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTCHOMPER;
}

S32 HAMR_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTHAMMER;
}

S32 TART_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTTARTAR;
}

S32 GLOV_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTGLOVE;
}

S32 MOON_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTMONSOON;
}

S32 SLEP_grul_goAlert(xGoal* rawgoal, void*, en_trantype* trantype, F32, void*)
{
    zNPCSleepy* npc = (zNPCSleepy*)rawgoal->psyche->clt_owner;
    NPCArena* arena = &npc->arena;

    if (globals.player.Health < 1)
    {
        return 0;
    }

    if (npc->npcset.allowDetect == 0)
    {
        return 0;
    }

    if (zEntTeleportBox_playerIn())
    {
        return 0;
    }

    S32 rc = arena->NeedToCycle(npc);

    if (rc == 2)
    {
        arena->Cycle(npc, 1);
    }
    else if (rc != 0)
    {
        arena->Cycle(npc, 0);
    }

    if (!arena->IsReady())
    {
        return 0;
    }

    if (!arena->IncludesNPC(npc, 0.0f, NULL))
    {
        return 0;
    }

    if (!arena->IncludesPlayer(0.0f, NULL))
    {
        return 0;
    }

    xVec3 vec_toPlyr;
    F32 hyt_toPlyr = 0.0f;

    F32 ds2 = npc->XZDstSqToPlayer(&vec_toPlyr, &hyt_toPlyr);

    if (ds2 > SQ(npc->cfg_npc->rad_detect))
    {
        return 0;
    }

    if (hyt_toPlyr < -3.0f)
    {
        return 0;
    }

    NPCConfig* cfg = npc->cfg_npc;
    xVec3 pos_light;

    npc->NightLightPos(&pos_light);

    xVec3 pos_edge = *npc->Pos() + g_X3 * cfg->rad_detect;
    xVec3 dir_edge = pos_edge - pos_light;

    dir_edge.normalize();

    xVec3 dir_plyr = *xEntGetPos(&globals.player.ent) - pos_light;

    dir_plyr.normalize();

    if (xVec3Dot(&dir_plyr, &g_NY3) < xVec3Dot(&dir_edge, &g_NY3))
    {
        return 0;
    }

    S32 moveinfo = zEntPlayer_MoveInfo();

    if (gCurrentPlayer == 0 && (moveinfo & 0x40))
    {
        return 0;
    }

    *trantype = GOAL_TRAN_SET;

    return NPC_GOAL_ALERT;
}

S32 SLEP_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTSLEEPY;
}

S32 ARFY_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTARF;
}

S32 PUPY_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTPUPPY;
}

S32 CHUK_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTCHUCK;
}

S32 TUBE_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTTUBELET;
}

S32 SLCK_grul_alert(xGoal* goal, void*, en_trantype* trantype, float, void*)
{
    *trantype = GOAL_TRAN_PUSH;
    return NPC_GOAL_ALERTSLICK;
}

static zParEmitter* g_pemit_smoke;

static zParEmitter* g_pemit_flame;

static zParEmitter* g_pemit_trek;

static zParEmitter* g_pemit_propwash;

static zParEmitter* g_pemit_exhaust;

static zParEmitter* g_pemit_steam;

static xParEmitterCustomSettings g_parf_smoke;

static xParEmitterCustomSettings g_parf_spark;

static xParEmitterCustomSettings g_parf_trek;

static xParEmitterCustomSettings g_parf_propwash;

static xParEmitterCustomSettings g_parf_exhaust;

static xParEmitterCustomSettings g_parf_steam;

void ROBO_InitEffects()
{
    g_pemit_smoke = zParEmitterFind("PAREMIT_ROBO_DMGSMOKE");
    g_pemit_flame = zParEmitterFind("PAREMIT_ROBO_DMGSPARK");
    g_pemit_trek = zParEmitterFind("PAREMIT_ROBO_TELEPORT");
    g_pemit_propwash = zParEmitterFind("PAREMIT_ROBO_WAKE");
    g_pemit_exhaust = zParEmitterFind("PAREMIT_ROHAM_EXHAUST");
    g_pemit_steam = zParEmitterFind("PAREMIT_ROTUB_STEAM");

    g_parf_smoke.custom_flags = 0x100;
    xVec3Copy(&g_parf_smoke.pos, &g_O3);

    g_parf_spark.custom_flags = 0x100;
    xVec3Copy(&g_parf_spark.pos, &g_O3);

    g_parf_trek.custom_flags = 0x300;
    xVec3Copy(&g_parf_trek.pos, &g_O3);
    g_parf_trek.vel.x = 0.0f;
    g_parf_trek.vel.y = 0.3f;
    g_parf_trek.vel.z = 0.0f;

    g_parf_propwash.custom_flags = 0x300;
    xVec3Copy(&g_parf_propwash.pos, &g_O3);
    xVec3Copy(&g_parf_propwash.vel, &g_O3);

    g_parf_exhaust.custom_flags = 0x300;
    xVec3Copy(&g_parf_exhaust.pos, &g_O3);
    xVec3Copy(&g_parf_exhaust.vel, &g_O3);
    g_parf_exhaust.vel.y = 0.5f;

    g_parf_steam.custom_flags = 0x302;
    xVec3Copy(&g_parf_steam.pos, &g_O3);
    xVec3Copy(&g_parf_steam.vel, &g_O3);

    g_parf_steam.life.set(1.0f, 1.0f, 1.0f, 0);
}



void ROBO_KillEffects()
{
}

// Two more unreferenced all-zero xVec3 templates the target holds between
// zNPCSlick::ShieldGeneratorDamaged's `smidge` and the zeroed pos_emit of
// zNPCRobot::DoFX_Motorboat below.
void __deadstripped_zNPCTypeRobot_5()
{
    const char _4696[0x0C] = {};
    const char _4697[0x0C] = {};
}

void zNPCRobot::DoFX_Motorboat(F32 dt)
{
    xVec3 pos_emit = { 0.0f, 0.0f, 0.0f };

    if (xEntIsVisible(this))
    {
        cnt_nextemit--;

        if (cnt_nextemit < 5)
        {
            if (xVec3Length2(&frame->vel) > SQ(0.2f) && GetVertPos(NPC_MDLVERT_PROPEL, &pos_emit))
            {
                zFX_SpawnBubbleTrail(&pos_emit, 1);
            }
        }

        if (cnt_nextemit < 0)
        {
            cnt_nextemit = 15;
        }
    }
}

// One more unreferenced xVec3 template the target holds after
// zNPCRobot::DoFX_Motorboat's zeroed pos_emit: { 0.0f, 1.0f, 0.0f }.
void __deadstripped_zNPCTypeRobot_6()
{
    const xVec3 _4731 = { 0.0f, 1.0f, 0.0f };
}

void zNPCRobot::VFXStarTrek(F32 dt, xVec3* pos, xVec3* vel)
{
    g_parf_trek.vel.x = vel->x;
    g_parf_trek.vel.y = vel->y;
    g_parf_trek.vel.z = vel->z;
    xVec3Copy(&g_parf_trek.pos, pos);
    xParEmitterEmitCustom(g_pemit_trek, dt, &g_parf_trek);
}

S32 zNPCRobot::LaunchProjectile(en_npchaz haztyp, F32 spd_proj, F32 dst_minRange,
                                en_mdlvert idx_mvtx, F32 tym_predictMax, F32 hyt_offset)
{
    xVec3 pos_launch;
    xVec3 dir_target;
    xVec3 pos_target;

    NPCHazard* haz = HAZ_Acquire();

    if (haz == NULL)
    {
        return 0;
    }

    if (haz->ConfigHelper(haztyp))
    {
        haz->SetNPCOwner(this);
    }
    else
    {
        haz->Discard();
        return 0;
    }

    if (!GetVertPos(idx_mvtx, &pos_launch))
    {
        xVec3SMul(&pos_launch, NPCC_faceDir(this), 0.5f);
        pos_launch.y = 0.5f;
        xVec3AddTo(&pos_launch, xEntGetCenter(this));
    }

    F32 tym_predict = xsqrt(XYZDstSqToPlayer(NULL));

    tym_predict = tym_predict / MAX(spd_proj, 0.25f);
    tym_predict = MIN(tym_predict, tym_predictMax);

    zEntPlayer_PredictPos(&pos_target, tym_predict, 1.0f, 1);

    if (XZDstSqToPlayer(NULL, NULL) < XZDstSqToPos(&pos_target, NULL, NULL))
    {
        xVec3Copy(&pos_target, xEntGetPos(&globals.player.ent));
    }

    pos_target.y += hyt_offset;

    F32 dst_target = xsqrt(XZDstSqToPos(&pos_target, &dir_target, NULL));

    if (dst_target < 0.00001f)
    {
        dst_target = dst_minRange;

        xVec3SMul(&pos_target, NPCC_faceDir(this), dst_minRange);
        xVec3AddTo(&pos_target, Pos());
    }
    else
    {
        xVec3SMulBy(&dir_target, 1.0f / dst_target);

        if (dst_target < dst_minRange ||
            xVec3Dot(&dir_target, NPCC_faceDir(this)) < 0.4f)
        {
            dst_target = dst_minRange;

            xVec3SMul(&pos_target, &dir_target, dst_minRange);
            xVec3AddTo(&pos_target, Pos());
        }
    }

    xVec3Copy(&haz->custdata.tartar.pos_tgt, &pos_target);

    haz->Start(&pos_launch, MAX(0.25f, dst_target / spd_proj));

    return 1;
}

S32 NPCArena::IncludesPos(xVec3* pos, F32 rad_thresh, xVec3* vec)
{
    if (!IsReady())
    {
        return 0;
    }

    xVec3 delt;

    xVec3Sub(&delt, pos, &pos_arena);

    if (vec != NULL)
    {
        xVec3Copy(vec, &delt);
    }

    delt.y = 0.0f;

    if (xVec3Length2(&delt) > SQ(rad_arena - rad_thresh))
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

F32 NPCArena::PctFromHome(xVec3* pos)
{
    xVec3 dif;

    if (!IsReady())
    {
        return -1.0f;
    }

    xVec3Sub(&dif, pos, &pos_arena);
    dif.y = 0.0f;

    return xVec3Length2(&dif) / SQ(rad_arena);
}

F32 NPCArena::DstSqFromHome(xVec3* pos, xVec3* delt)
{
    xVec3 dif;

    if (!IsReady())
    {
        return -1.0f;
    }

    xVec3Sub(&dif, pos, &pos_arena);

    if (delt != NULL)
    {
        *delt = dif;
    }

    dif.y = 0.0f;

    return xVec3Length2(&dif);
}

S32 NPCArena::NeedToCycle(zNPCCommon* npc)
{
    S32 rc;

    if (IsReady() && !IncludesNPC(npc, 0.0f, NULL) && !(flg_arena & 4))
    {
        return 2;
    }

    if (npc->nav_dest != nav_refer_dest || npc->nav_curr != nav_refer_curr)
    {
        return 1;
    }

    if (!IsReady() && !(flg_arena & 4))
    {
        rc = 2;
    }
    else
    {
        rc = 0;
    }

    return rc;
}

S32 NPCArena::Cycle(zNPCCommon* npc, S32 peek)
{
    zMovePoint* nav;

    xVec3Copy(&pos_arena, &g_O3);
    rad_arena = -1.0f;
    flg_arena = 0;
    nav_arena = NULL;
    nav_refer_dest = npc->nav_dest;
    nav_refer_curr = npc->nav_curr;

    if (!peek && npc->nav_curr != NULL && npc->nav_curr->RadiusArena() > 1.0f)
    {
        nav_arena = npc->nav_curr;
        SyncHomeFromNav();
    }
    else if (!peek && npc->nav_curr != NULL &&
             (nav = NextBestNav(npc, npc->nav_curr)) != NULL)
    {
        nav_arena = nav;
        SyncHomeFromNav();
    }
    else if (peek && npc->nav_dest != NULL && npc->nav_dest->RadiusArena() > 1.0f)
    {
        nav_arena = npc->nav_dest;
        SyncHomeFromNav();
    }
    else if (peek && npc->nav_dest != NULL &&
             (nav = NextBestNav(npc, npc->nav_dest)) != NULL)
    {
        nav_arena = nav;
        SyncHomeFromNav();
    }

    if (peek && !IsReady())
    {
        Cycle(npc, 0);
    }

    if (peek)
    {
        flg_arena |= 4;
    }

    return rad_arena > 0.0f;
}

zMovePoint* NPCArena::NextBestNav(zNPCCommon* npc, zMovePoint* nav)
{
    zMovePoint* nav_best = NULL;
    F32 rad_best = -1.0f;
    S32 cnt_node = nav->NumNodes();

    for (S32 i = 0; i < cnt_node; i++)
    {
        zMovePoint* node = nav->NodeByIndex(i);

        if (node != NULL)
        {
            if (npc->DBG_IsNormLog((en_npcdcat)12, -1))
            {
                xDrawSetColor(g_BLUE);
                xDrawSphere2(node->PosGet(), 0.1f, 12);
                xDrawLine(nav->PosGet(), node->PosGet());
            }

            F32 rad_node = node->RadiusArena();

            if (rad_node < 1.0f)
            {
                continue;
            }

            if (npc->DBG_IsNormLog((en_npcdcat)12, -1))
            {
                xDrawSetColor(g_BLUE);
                xDrawCyl(nav->PosGet(), rad_node, 1.0f, 0x2020C);
            }

            xVec3 delt;

            xVec3Sub(&delt, nav->PosGet(), node->PosGet());

            if (xabs(delt.y) > 5.0f)
            {
                continue;
            }

            delt.y = 0.0f;

            F32 dst2 = xVec3Length2(&delt);

            if (dst2 > SQ(rad_node))
            {
                continue;
            }

            if (npc->DBG_IsNormLog((en_npcdcat)12, -1))
            {
                xDrawSetColor(g_GREEN);
                xDrawSphere2(node->PosGet(), 0.12f, 12);
            }

            if (rad_node > rad_best)
            {
                rad_best = rad_node;
                nav_best = node;
            }
            else
            {
                nav_best = node;
                break;
            }
        }
    }

    return nav_best;
}

void NPCArena::SetHome(zNPCCommon* npc, zMovePoint* nav)
{
    flg_arena = 0;

    if (nav == NULL)
    {
        nav_arena = NULL;
    }
    else if (nav->RadiusArena() > 1.0f)
    {
        nav_arena = nav;
    }
    else
    {
        nav_arena = NextBestNav(npc, nav);
    }

    nav_refer_dest = npc->nav_dest;
    nav_refer_curr = npc->nav_curr;

    SyncHomeFromNav();
}

void NPCArena::AdjustHome(zNPCCommon* npc, xVec3* pos, F32 rad)
{
    flg_arena |= 2;

    if (pos != NULL)
    {
        xVec3Copy(&pos_arena, pos);
    }

    if (rad > 0.0f)
    {
        rad_arena = rad;
    }

    rad_arena = MAX(1.0f, rad_arena);
}

void NPCArena::SyncHomeFromNav()
{
    flg_arena = 0;

    if (nav_arena != NULL)
    {
        xVec3Copy(&pos_arena, nav_arena->PosGet());
        rad_arena = nav_arena->RadiusArena();
    }
    else
    {
        xVec3Copy(&pos_arena, &g_O3);
        rad_arena = -1.0f;
    }
}

void UVAModelInfo::Clear()
{
    memset(this, 0, sizeof(UVAModelInfo));
}

void UVAModelInfo::UVVelSet(float x, float y)
{
    offset_vel.x = x;
    offset_vel.y = y;
}

zNPCLassoInfo* zNPCRobot::PRIV_GetLassoData()
{
    return &raw_lassoinfo;
}

S32 zNPCRobot::IsAlive()
{
    return !IsDead();
}

void zNPCRobot::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 0xffffffff;
    *idxhold = 0xffffffff;
}

void zNPCSlick::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 2;
    *idxhold = 3;
}

S32 zNPCRobot::IsHealthy()
{
    return (hitpoints < 0) ? 0 : hitpoints;
}

xEntDrive* zNPCRobot::PRIV_GetDriverData()
{
    return &raw_drvdata;
}

zNPCLassoInfo* zNPCFodder::PRIV_GetLassoData()
{
    return NULL;
}

void zNPCFodder::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = -1;
    *idxhold = -1;
}

S32 xEntIsEnabled(xEnt* ent)
{
    return xBaseIsEnabled(ent);
}

zNPCLassoInfo* zNPCFodBomb::PRIV_GetLassoData()
{
    return NULL;
}

zNPCLassoInfo* zNPCFodBzzt::PRIV_GetLassoData()
{
    return NULL;
}

zNPCLassoInfo* zNPCChomper::PRIV_GetLassoData()
{
    return NULL;
}

zNPCLassoInfo* zNPCCritter::PRIV_GetLassoData()
{
    return NULL;
}

zNPCLassoInfo* zNPCArfDog::PRIV_GetLassoData()
{
    return NULL;
}

U8 zNPCTubeSlave::PhysicsFlags() const
{
    return 3;
}

U8 zNPCTubeSlave::ColPenByFlags() const
{
    return 16;
}

U8 zNPCTubeSlave::ColChkByFlags() const
{
    return 16;
}

U8 zNPCTubeSlave::ColPenFlags() const
{
    return 0;
}

U8 zNPCTubeSlave::ColChkFlags() const
{
    return 0;
}

S32 zNPCTubeSlave::CanRope()
{
    return 0;
}

U8 zNPCRobot::ColPenByFlags() const
{
    return 60;
}

U8 zNPCRobot::ColChkByFlags() const
{
    return 60;
}

void zNPCArfDog::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = -1;
    *idxhold = -1;
}

void zNPCChomper::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = -1;
    *idxhold = -1;
}

void zNPCCritter::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = -1;
    *idxhold = -1;
}

void zNPCFodBomb::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = -1;
    *idxhold = -1;
}

void zNPCFodBzzt::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = -1;
    *idxhold = -1;
}

void zNPCArfArf::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 2;
    *idxhold = 3;
}

void zNPCHammer::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 1;
    *idxhold = 2;
}

void zNPCSleepy::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 2;
    *idxhold = 3;
}

void zNPCTarTar::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 2;
    *idxhold = 3;
}

void zNPCMonsoon::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 1;
    *idxhold = 2;
}

void zNPCTubelet::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 2;
    *idxhold = 3;
}

void zNPCTubeSlave::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 2;
    *idxhold = 3;
}

void zNPCChuck::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 2;
    *idxhold = 3;
}

void zNPCGlove::LassoModelIndex(S32* idxgrab, S32* idxhold)
{
    *idxgrab = 1;
    *idxhold = 2;
}

TubeNotice::TubeNotice()
{
}

S32 zNPCRobot::IsDead()
{
    return IsDying() == 1;
}

void NPCLaser::TextureSet(RwRaster* rast)
{
    rast_laser = rast;
}

RwRaster* NPCLaser::TextureGet()
{
    return rast_laser;
}

void NPCLaser::RadiusSet(F32 rad_start, F32 rad_end)
{
    radius[0] = rad_start;
    radius[1] = rad_end;
}

void NPCLaser::UVScrollSet(F32 u, F32 v)
{
    uv_scroll[0] = u;
    uv_scroll[1] = v;
}

void NPCLaser::Prepare()
{
    rast_laser = NULL;
    uv_base[0] = 0.0f;
    uv_base[1] = 0.0f;
}

void NPCLaser::UVScrollUpdate(F32 dt)
{
    for (S32 i = 0; i < 2; i++)
    {
        uv_base[i] += dt * uv_scroll[i];
        while (uv_base[i] > 1.0f)
        {
            uv_base[i] -= 1.0f;
        }
        while (uv_base[i] < -0.0f)
        {
            uv_base[i] += 1.0f;
        }
    }
}

void zNPCArfDog::RenderExtra()
{
    BlinkRender();
    zNPCCommon::RenderExtra();
}

void zNPCFodBomb::RenderExtra()
{
    BlinkerRender();
    zNPCCommon::RenderExtra();
}
