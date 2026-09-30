#include "xVec3Inlines.h"
#include <types.h>

#include "iMath.h"
#include "iModel.h"
#include "iParMgr.h"
#include "xAnim.h"
#include "xVec3.h"
#include "xEvent.h"
#include "xCamera.h"
#include "xMath3.h"
#include "xMathInlines.h"
#include "xDebug.h"
#include "xJaw.h"

#include "zEnt.h"
#include "zFX.h"
#include "zGlobals.h"
#include "zNPCSndTable.h"
#include "zNPCSndLists.h"
#include "zNPCTypeBossSandy.h"
#include "zThrown.h"
#include "zEntSimpleObj.h"
#include "xMarkerAsset.h"
#include "zCamera.h"
#include "zGrid.h"

extern const char bossSandyStrings[];

// Indices into g_strz_bossanim / g_hash_bossanim (see zNPCTypeBoss.cpp).  These
// were previously all one too high, which made every name in this file refer to
// the *next* animation in the table.
#define Unknown 0
#define Idle01 1
#define Idle02 2
#define Taunt01 3
#define Run01 4
#define Walk01 5
#define Melee01 6
#define Hit01 7
#define Hit02 8
#define GetUp01 9
#define Dizzy01 10
#define ElbowDrop01 11
#define Leap01 12
#define Leap02 13
#define Leap03 14
#define Leap04 15
#define Sit01 16
#define SitShock01 17
#define CLBegin01 18
#define CLLoop01 19
#define CLEnd01 20
#define NoHeadIdle01 21
#define NoHeadWaving01 22
#define NoHeadGetUp01 23
#define NoHeadShotUp01 24
#define NoHeadShock01 25
#define NoHeadReplace01 26
#define NoHeadHit01 27

static F32 sSinTable[16];
static SandyLimbSpring sLeftArmSpring;
static SandyLimbSpring sRightArmSpring;
static SandyLimbSpring sLeftLegSpring;
static SandyLimbSpring sRightLegSpring;
static xVec3 sCamSubTargetFixed;
static U32 sNFSoundValue[30];
static BossDamageEffectRecord BDErecord[4];

static U8 sOthersHaventBeenAdded;
static U8 sPCWasBubbleBouncing;
static F32 sRadiusOfRing;
static F32 sElbowDropTimer;
static F32 sChaseTimer;
static S32 sNumAttacks;
static S32 sDidClothesline;
static volatile F32 sElbowDropThreshold;
static zNPCBSandy* sSandyPtr;
static xVec3* sCamSubTarget;
static F32 sCurrYaw;
static F32 sCurrHeight;
static F32 sCurrRadius;
static F32 sCurrPitch;
static U32 sCurrNFSound;

static U8 sUseBossCam = TRUE;
static U8 sWasUsingBossCam = TRUE;
static F32 sPCHeightDiff = -1.0f;
static F32 sHeadPopOffFactor = -1.0f;
static S32 sLeftFootBones[2] = { 0x2B, 0x2C };
static S32 sRightFootBones[2] = { 0x30, 0x31 };

static S32 sBone[13] = {
    0x3, 0x5, 0x12, 0x14, 0x19, 0x1B, 0x22, 0x24, 0x26, 0x29, 0x2B, 0x2E, 0x30
};
static S32 sLeftHandBones[4] = { 0x14, 0x15, 0x16, 0x17 };
static S32 sRightHandBones[4] = { 0x1B, 0x1C, 0x1D, 0x1E };
static F32 sBoundRadius[13] = { 1.4f,        1.3f, -1.0f, 0.6f,        -1.0f, 0.6f,       0.6f,
                                0.69999999f, 0.8f, -1.0f, 0.89999999f, -1.0f, 0.89999999f };
static xVec3 sBoneOffset[13] = { {},
                                 { 0.0f, 1.1f, 0.0f },
                                 {},
                                 { 0.3f, 0.0f, 0.0f },
                                 {},
                                 { -0.3f, 0.0f, 0.0f },
                                 { 0.0f, 0.2f, 0.0f },
                                 { 0.0f, 0.2f, 0.0f },
                                 {},
                                 {},
                                 { 0.0f, -0.6f, 0.3f },
                                 {},
                                 { 0.0f, -0.6f, 0.3f } };
static char* sNFSoundLabel[30] = {
    "FAB1006", "FAB1007",   "FAB1008",   "FAB1009", "FAB1010", "FAB1011", "FAB1012", "FAB1013",
    "FAB1014", "FAB1015",   "FAB1016",   "FAB1017", "FAB1018", "FAB1019", "FAB1020", "FAB1021",
    "FAB1022", "FAB1023",   "FAB1024",   "FAB1025", "FAB1026", "FAB1027", "FAB1028", "FAB1029",
    "FAB1030", "FAB1031",   "FAB1032",   "FAB1041_a", "FAB1041_b", "FAB1065"
};

static void on_change_newsfish(const tweak_info& tweak);
void on_change_shockwave(const tweak_info& tweak);

// .rodata carries an R_PPC_ADDR32 against on_change_newsfish at +0x0 and one
// against on_change_shockwave at +0x28, i.e. each of these is a create_change
// callback, not an all-NULL struct.
static const tweak_callback newsfish_cb = { (void (*)(tweak_info&))on_change_newsfish };
static const tweak_callback shockwave_cb = { (void (*)(tweak_info&))on_change_shockwave };

extern zGlobals globals;

static S32 idleCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 tauntCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 noHeadCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 elbowDropCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 leapCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 chaseCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 meleeCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 sitCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 getUpCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 runToRopeCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);
static S32 clotheslineCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*);

void zNPCBSandy_BossDamageEffect_Init();

static void GetBonePos(xVec3* result, xMat4x3* matArray, S32 index, xVec3* offset);
static void MakeOBBFor(S32 startBone, S32 endBone, xEnt* ent, xMat4x3* matArray);
static S32 BoundEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*);

static void on_change_newsfish(const tweak_info& tweak)
{
    sSandyPtr->newsfish->SpeakStart(sCurrNFSound, 0, -1);
}

void on_change_shockwave(const tweak_info& tweak)
{
    sSandyPtr->bossFlags |= 0x800;

    xVec3Copy((xVec3*)(&sSandyPtr->shockwaveEmitter->tasset->pos),
              (xVec3*)(&sSandyPtr->model->Mat->pos));

    sSandyPtr->shockwaveEmitter->tasset->pos.y = 0.0f;
    sSandyPtr->shockRadius = 1.0f;
}

xAnimTable* ZNPC_AnimTable_BossSandy()
{
    // clang-format off
    // NPCC_BuildStandardAnimTran walks this until it hits a 0, so the terminator
    // is load bearing.  It must also list NoHeadHit01: that is the round 3 damage
    // reaction, and without a transition to it AnimStart() refuses to play it.
    S32 ourAnims[25] = {
        Idle01,
        Idle02,
        Taunt01,
        Run01,
        Walk01,
        Melee01,
        ElbowDrop01,
        Leap01,
        Leap02,
        Leap03,
        Leap04,
        Sit01,
        SitShock01,
        GetUp01,
        CLBegin01,
        CLLoop01,
        CLEnd01,
        NoHeadIdle01,
        NoHeadWaving01,
        NoHeadGetUp01,
        NoHeadShotUp01,
        NoHeadShock01,
        NoHeadReplace01,
        NoHeadHit01,
        Unknown,
    };
    // clang-format on

    xAnimTable* table = xAnimTableNew("zNPCBSandy", NULL, 0);

    xAnimTableNewState(table, g_strz_bossanim[Idle01], 0x10, 0x40, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Taunt01], 0x10, 0x40, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Run01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Walk01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Melee01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[ElbowDrop01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Leap01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Leap02], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Leap03], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Leap04], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[Sit01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[SitShock01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[GetUp01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[CLBegin01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[CLLoop01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[CLEnd01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[NoHeadIdle01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[NoHeadWaving01], 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[NoHeadGetUp01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[NoHeadShotUp01], 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[NoHeadShock01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[NoHeadReplace01], 0x10, 0, 1.0f, NULL, NULL, 0.0f,
                       NULL, NULL, xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, g_strz_bossanim[NoHeadHit01], 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL,
                       NULL, xAnimDefaultBeforeEnter, NULL, NULL);

    NPCC_BuildStandardAnimTran(table, g_strz_bossanim, ourAnims, 1, 0.2f);

    xAnimTransition* tList = table->TransitionList;
    while (tList != NULL)
    {
        tList->BlendRecip = 3.3333333f;
        tList = tList->Next;
    }

    return table;
}

U32 HeadIsCarried(xAnimTransition*, xAnimSingle*, void*)
{
    return globals.player.carry.grabbed == sSandyPtr->headBoulder;
}

U32 HeadNotCarried(xAnimTransition*, xAnimSingle*, void*)
{
    return !(globals.player.carry.grabbed == sSandyPtr->headBoulder);
}

U32 HeadIsShocked(xAnimTransition*, xAnimSingle*, void*)
{
    return sSandyPtr->bossFlags & 0x100;
}

U32 HeadNotShocked(xAnimTransition*, xAnimSingle*, void*)
{
    return !(sSandyPtr->bossFlags & 0x100);
}

xAnimTable* ZNPC_AnimTable_BossSandyHead()
{
    xAnimTable* table = xAnimTableNew("SandyBossHead", NULL, 0);

    xAnimTableNewState(table, "Idle01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Carried01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Shocked01", 0x10, 0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(table, "Idle01", "Carried01", HeadIsCarried, NULL, 0, 0, 0.0f, 0.0f, 0,
                            0, 0.25f, NULL);
    xAnimTableNewTransition(table, "Carried01", "Idle01", HeadNotCarried, NULL, 0, 0, 0.0f, 0.0f, 0,
                            0, 0.25f, NULL);
    xAnimTableNewTransition(table, "Idle01", "Shocked01", HeadIsShocked, NULL, 0, 0, 0.0f, 0.0f, 0,
                            0, 0.25f, NULL);
    xAnimTableNewTransition(table, "Shocked01", "Idle01", HeadNotShocked, NULL, 0, 0, 0.0f, 0.0f, 0,
                            0, 0.25f, NULL);

    return table;
}

xAnimTable* ZNPC_AnimTable_BossSandyScoreboard()
{
    xAnimTable* table = xAnimTableNew("SandyBossScoreboard", NULL, 0);

    xAnimTableNewState(table, "Idle01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);
    xAnimTableNewState(table, "Shocked01", 0x10, 0x0, 1.0f, NULL, NULL, 0.0f, NULL, NULL,
                       xAnimDefaultBeforeEnter, NULL, NULL);

    xAnimTableNewTransition(table, "Idle01", "Shocked01", HeadIsShocked, NULL, 0x0, 0x0, 0.0f, 0.0f,
                            0, 0, 0.25f, NULL);
    xAnimTableNewTransition(table, "Shocked01", "Idle01", HeadNotShocked, NULL, 0x0, 0x0, 0.0f,
                            0.0f, 0, 0, 0.25f, NULL);

    return table;
}

void zNPCBSandy::Init(xEntAsset* asset)
{
    S32 i;
    char objName[32];
    U32 colorPicker;

    zNPCCommon::Init(asset);
    sSandyPtr = this;

    this->round = 1;
    this->firstTimeR1Csn = 1;
    this->boundFlags = (U32*)xMemAlloc(gActiveHeap, 13 * sizeof(U32), 0x0);
    this->boundList = (xEnt**)xMemAlloc(gActiveHeap, 13 * sizeof(xEnt*), 0x0);
    this->boundEmitTimer = (F32*)xMemAlloc(gActiveHeap, 13 * sizeof(F32), 0x0);

    for (i = 0; i < 13; i++)
    {
        this->boundFlags[i] = 0;
        this->boundList[i] = (xEnt*)xMemAlloc(gActiveHeap, sizeof(xEnt), 0x0);

        xEnt* ent = this->boundList[i];
        ent->id = i;
        ent->eventFunc = BoundEventCB;
        ent->driver = (xEnt*)this;
        ent->baseType = eBaseTypeDynamic;
        ent->collType = XENT_COLLTYPE_STAT;
        ent->chkby = XENT_COLLTYPE_PLYR;
        ent->penby = XENT_COLLTYPE_PLYR;
        ent->baseFlags |= 0x21;
        ent->moreFlags = 0x10;
        ent->flags = this->flags;
        ent->model = this->model;
        ent->update = NULL;
        ent->bupdate = NULL;
        ent->render = NULL;
        ent->transl = NULL;
        ent->subType = 0;
        ent->collModel = NULL;
        ent->lightKit = NULL;
        ent->pflags = 0;
        ent->move = NULL;
        ent->frame = NULL;
        ent->collis = NULL;
        ent->ffx = NULL;
        ent->num_ffx = 0;
        ent->anim_coll = NULL;

        xGridBoundInit(&ent->gridb, ent);

        if (sBoundRadius[i] > 0.0f)
        {
            ent->bound.type = XBOUND_TYPE_SPHERE;
            ent->bound.sph.r = sBoundRadius[i];

            GetBonePos(&this->boundList[i]->bound.sph.center, (xMat4x3*)this->model->Mat, sBone[i],
                       &sBoneOffset[i]);
            xQuickCullForBound(&this->boundList[i]->bound.qcd, &this->boundList[i]->bound);
        }
        else
        {
            ent->bound.type = XBOUND_TYPE_OBB;
            ent->bound.mat = (xMat4x3*)xMemAlloc(gActiveHeap, sizeof(xMat4x3), 0x0);
        }
    }

    MakeOBBFor(2, 3, this->boundList[2], (xMat4x3*)this->model->Mat);
    xQuickCullForBound(&this->boundList[2]->bound.qcd, &this->boundList[2]->bound);

    MakeOBBFor(4, 5, this->boundList[4], (xMat4x3*)this->model->Mat);
    xQuickCullForBound(&this->boundList[4]->bound.qcd, &this->boundList[4]->bound);

    MakeOBBFor(9, 10, this->boundList[9], (xMat4x3*)this->model->Mat);
    xQuickCullForBound(&this->boundList[9]->bound.qcd, &this->boundList[9]->bound);

    MakeOBBFor(11, 12, this->boundList[11], (xMat4x3*)this->model->Mat);
    xQuickCullForBound(&this->boundList[11]->bound.qcd, &this->boundList[11]->bound);

    sOthersHaventBeenAdded = 1;

    for (i = 0; i < 16; i++)
    {
        sSinTable[i] = isin(6.2831855f * i / 16.0f);
    }

    zNPCBSandy_BossDamageEffect_Init();

    this->dustEddieSetting.custom_flags = 0x100;

    strcpy(objName, "LASER_MK_00");
    for (i = 0; i < 16; i++)
    {
        xMarkerAsset* laserMarker = (xMarkerAsset*)xSTFindAsset(xStrHash(objName), NULL);
        xVec3Copy(&this->laserPoint[i], (xVec3*)laserMarker);

        objName[10]++;
        if (objName[10] > '9')
        {
            objName[10] = '0';
            objName[9]++;
        }
    }

    this->laserShow.init("LaserShow", "NPC|zNPCBSandy|LaserShow|");

    colorPicker = (xrand() >> 8) & 1;
    this->curveNodeR = colorPicker ? 1.0f : 0.0f;
    this->curveNodeG = colorPicker ? 1.0f : 0.0f;
    this->curveNodeB = colorPicker ? 1.0f : 0.0f;

    this->curveNode[0].time = 0.0f;
    this->curveNode[0].scale = 1.0f;
    this->curveNode[0].color.r = (U8)(255.0f * this->curveNodeR);
    this->curveNode[0].color.g = (U8)(255.0f * this->curveNodeG);
    this->curveNode[0].color.b = (U8)(255.0f * this->curveNodeB);
    this->curveNode[0].color.a = 0xff;
    this->curveNode[1].time = 1.0f;
    this->curveNode[1].scale = 1.0f;
    this->curveNode[1].color.r = (U8)(255.0f * this->curveNodeR);
    this->curveNode[1].color.g = (U8)(255.0f * this->curveNodeG);
    this->curveNode[1].color.b = (U8)(255.0f * this->curveNodeB);
    this->curveNode[1].color.a = 0xff;
    this->curveNodeAlpha = 1.0f;
    this->laserShow.set_curve(&this->curveNode[0], 0x6);
    this->laserShow.cfg.life_time = 0.5f;
    this->laserShow.cfg.blend_src = 0x5;
    this->laserShow.cfg.blend_dst = 0x2;
    this->laserShow.refresh_config();
    this->laserShow.set_texture(xStrHash("lightning"));

    for (i = 0; i < 30; i++)
    {
        sNFSoundValue[i] = xStrHash(sNFSoundLabel[i]);
    }

    sCurrNFSound = sNFSoundValue[0];

    RwIm3DVertexSetPos(&this->iconVert[0], 1.0f, 0.0f, 1.0f);
    RwIm3DVertexSetPos(&this->iconVert[1], -1.0f, 0.0f, 1.0f);
    RwIm3DVertexSetPos(&this->iconVert[2], 1.0f, 0.0f, -1.0f);
    RwIm3DVertexSetPos(&this->iconVert[3], -1.0f, 0.0f, -1.0f);

    RwIm3DVertexSetRGBA(&this->iconVert[0], 0xff, 0xff, 0xff, 0xff);
    RwIm3DVertexSetRGBA(&this->iconVert[1], 0xff, 0xff, 0xff, 0xff);
    RwIm3DVertexSetRGBA(&this->iconVert[2], 0xff, 0xff, 0xff, 0xff);
    RwIm3DVertexSetRGBA(&this->iconVert[3], 0xff, 0xff, 0xff, 0xff);

    this->iconVert[0].u = 0.0f;
    this->iconVert[1].u = 1.0f;
    this->iconVert[2].u = 0.0f;
    this->iconVert[3].u = 1.0f;

    this->iconVert[0].v = 0.0f;
    this->iconVert[1].v = 0.0f;
    this->iconVert[2].v = 1.0f;
    this->iconVert[3].v = 1.0f;

    this->wireLight[0] = 0;
    this->wireLight[1] = 0;

    xDebugAddTweak("NPC|zNPCBSandy|Newsfish", "Speak", &newsfish_cb, 0, 0);
    xDebugAddSelectTweak("NPC|zNPCBSandy|NewsfishComment", &sCurrNFSound,
                         (const char**)&sNFSoundLabel[0], &sNFSoundValue[0], 0x1e, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|Shockwave|Do It", "Go", &shockwave_cb, 0, 0);

    this->shockwaveGrowthRate = 20.0f;
    this->shockwaveMaxRadius = 10.0f;

    xDebugAddTweak("NPC|zNPCBSandy|Shockwave|GrowthRate", &this->shockwaveGrowthRate, 0.0f, 1000000000.0f, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|Shockwave|MaxRadius", &this->shockwaveMaxRadius, 0.0f, 1000000000.0f, 0, 0, 0);

    this->edropShockwaveTime = 2.25f;
    this->edropTurnMinTime = 1.0f;

    xDebugAddTweak("NPC|zNPCBSandy|ElbowDrop|ShockwaveTime", &this->edropShockwaveTime, 0.0f, 1000000000.0f, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|ElbowDrop|TurnTime", &this->edropTurnMinTime, 0.0f, 1000000000.0f, 0, 0, 0);
}

void zNPCBSandy::Setup()
{
    S32 i;
    char objName[32];

    newsfish = (zNPCNewsFish*)zSceneFindObject(xStrHash("NPC_NEWSCASTER"));
    newsfish->TalkOnScreen(1);

    strcpy(objName, "HEALTH_00");
    for (i = 0; i < 3; i++)
    {
        objName[8] = '1' + (char)i;
        underwear[i] = (zEntPickup*)zSceneFindObject(xStrHash(objName));
    }

    // Configure and initialize bossCam
    bossCam.cfg.zone_rest.distance = 6.0f;
    bossCam.cfg.zone_rest.height = 1.3f;
    bossCam.cfg.zone_rest.height_focus = 2.0f;

    bossCam.cfg.zone_above.distance = 3.0f;
    bossCam.cfg.zone_above.height = 8.2f;
    bossCam.cfg.zone_above.height_focus = 4.0f;

    bossCam.cfg.zone_below.distance = 4.5f;
    bossCam.cfg.zone_below.height = 0.15f;
    bossCam.cfg.zone_below.height_focus = 2.0f;

    bossCam.cfg.move_speed = 10.0f;
    bossCam.cfg.turn_speed = 10.0f;
    bossCam.cfg.stick_speed = 10.0f;
    bossCam.cfg.stick_yaw_vel = 10.0f;
    bossCam.cfg.max_yaw_vel = 10.0f;
    bossCam.cfg.margin_angle = -0.4f;

    bossCam.init();
    bossCam.add_tweaks("NPC|zNPCBSandy|Boss Cam|");

    // Configure and initialize specialBossCam
    specialBossCam.cfg.zone_rest.distance = 9.7f;
    specialBossCam.cfg.zone_rest.height = 4.0f;
    specialBossCam.cfg.zone_rest.height_focus = 4.0f;

    specialBossCam.cfg.zone_above.distance = 10.0f;
    specialBossCam.cfg.zone_above.height = 7.0f;
    specialBossCam.cfg.zone_above.height_focus = 0.15f;

    specialBossCam.cfg.zone_below.distance = 10.0f;
    specialBossCam.cfg.zone_below.height = 0.5f;
    specialBossCam.cfg.zone_below.height_focus = 5.0f;

    specialBossCam.cfg.move_speed = 10.0f;
    specialBossCam.cfg.turn_speed = 10.0f;
    specialBossCam.cfg.stick_speed = 10.0f;
    specialBossCam.cfg.stick_yaw_vel = 10.0f;
    specialBossCam.cfg.max_yaw_vel = 10.0f;
    specialBossCam.cfg.margin_angle = -0.4f;

    specialBossCam.init();
    specialBossCam.add_tweaks("NPC|zNPCBSandy|Mat Smash Cam|");

    zNPCCommon::Setup();
}

void zNPCBSandy::SelfSetup()
{
    xBehaveMgr* bmgr = xBehaveMgr_GetSelf();
    psy_instinct = bmgr->Subscribe(this, 0);

    xPsyche* psy = psy_instinct;
    psy->BrainBegin();

    xGoal* goal = psy->AddGoal('NGB1', NULL);
    goal->SetCallbacks(idleCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB2', NULL);
    goal->SetCallbacks(tauntCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB3', NULL);
    goal->SetCallbacks(chaseCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB4', NULL);
    goal->SetCallbacks(meleeCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB5', NULL);
    goal->SetCallbacks(noHeadCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB6', NULL);
    goal->SetCallbacks(elbowDropCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB7', NULL);
    goal->SetCallbacks(leapCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB8', NULL);
    goal->SetCallbacks(sitCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB9', NULL);
    goal->SetCallbacks(getUpCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB:', NULL);
    goal->SetCallbacks(runToRopeCB, NULL, NULL, NULL);

    goal = psy->AddGoal('NGB;', NULL);
    goal->SetCallbacks(clotheslineCB, NULL, NULL, NULL);

    psy->BrainEnd();
    psy->SetSafety('NGB1');
}

void zNPCBSandy::Reset()
{
    S32 i;
    S32 j;
    RwTexture* tempTex;
    char objName[32];
    U32 size;
    xVec3 endPnt;
    xVec3 zeroPnt;

    zNPCCommon::Reset();

    this->firstUpdate = 0x1;
    this->bossFlags = 0x400;

    for (i = 0; i < 13; i++)
    {
        this->boundFlags[i] = 0;
    }

    sUseBossCam = TRUE;
    sCurrHeight = 0.15f;
    sCurrRadius = 4.5f;
    sCurrPitch = -0.3f;
    sCurrYaw = 0.0f;
    sCamSubTarget = (xVec3*)&this->model->Mat->pos;
    sElbowDropTimer = 0.0f;
    sChaseTimer = 0.0f;
    sElbowDropThreshold = 0.0f;
    sNumAttacks = 0;
    sDidClothesline = 0;

    this->dustEddieEmitter = zParEmitterFind("PAREMIT_DUST_SWIRL");
    this->shockwaveEmitter = zParEmitterFind("PAREMIT_SHOCKWAVE");
    this->headBoulder = (xEntBoulder*)zSceneFindObject(xStrHash("SANDY_HEAD_BOULDER"));
    this->headBoulder->bound.box.box.upper.x = 1.25f;
    this->headBoulder->localCenter.x = 0.0f;
    this->headBoulder->localCenter.y = 0.036f;
    this->headBoulder->localCenter.z = 0.11f;

    xDebugAddTweak("NPC|zNPCBSandy|headBoulder|radius", &this->headBoulder->bound.sph.r, 0.1f, 10.0f, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|headBoulder|center|x", &this->headBoulder->bound.sph.center.x, -10.0f, 10.0f, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|headBoulder|center|y", &this->headBoulder->bound.sph.center.y, -10.0f, 10.0f, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|headBoulder|center|z", &this->headBoulder->bound.sph.center.z, -10.0f, 10.0f, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|headBoulder|localCener|x", &this->headBoulder->localCenter.x, -10.0f, 10.0f, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|headBoulder|localCener|y", &this->headBoulder->localCenter.y, -10.0f, 10.0f, 0, 0, 0);
    xDebugAddTweak("NPC|zNPCBSandy|headBoulder|localCener|z", &this->headBoulder->localCenter.z, -10.0f, 10.0f, 0, 0, 0);

    this->hangingScoreboard = (xEnt*)zSceneFindObject(xStrHash("SO_SCOREBOARD"));
    this->bustedScoreboard = (xEnt*)zSceneFindObject(xStrHash("SCOREBOARD_BUSTED"));
    this->crashedScoreboard = (xEnt*)zSceneFindObject(xStrHash("SCOREBOARD_HAZARD"));

    ((zEntSimpleObj*)this->crashedScoreboard)->sflags |= 0x8;
    this->crashedScoreboard->model->PipeFlags &= ~0xc;
    this->crashedScoreboard->model->PipeFlags |= 0x4;

    this->scoreboardShrap = (zShrapnelAsset*)xSTFindAsset(xStrHash("pdome_scoreboard_shrapnel"), 0);
    this->sboardSecondShrap = (zShrapnelAsset*)xSTFindAsset(xStrHash("pdome_scoreboard_secondary_shrapnel"), 0);
    this->sboardThirdShrap = (zShrapnelAsset*)xSTFindAsset(xStrHash("pdome_scoreboard_tertiary_shrapnel"), 0);
    this->lightRigShrap = (zShrapnelAsset*)xSTFindAsset(xStrHash("pdome_scaffolding_shrapnel"), 0);
    this->lightRig[0] = (xEnt*)zSceneFindObject(xStrHash("SO_LIGHTRIG01"));
    this->lightRig[1] = (xEnt*)zSceneFindObject(xStrHash("SO_LIGHTRIG010"));
    this->lightRig[2] = (xEnt*)zSceneFindObject(xStrHash("SO_LIGHTRIG0100"));
    this->lightRig[3] = (xEnt*)zSceneFindObject(xStrHash("SO_LIGHTRIG01000"));
    this->round1Csn = (zCutsceneMgr*)zSceneFindObject(xStrHash("CSNMGR_ROUND1"));
    this->round2Csn = (zCutsceneMgr*)zSceneFindObject(xStrHash("CSNMGR_ROUND2"));
    this->round3Csn = (zCutsceneMgr*)zSceneFindObject(xStrHash("CSNMGR_ROUND3"));
    this->targetRaster = 0;
    this->helmetRaster = 0;
    this->feetRaster = 0;

    RwRaster** x;

    if (x = (RwRaster**)xSTFindAsset(xStrHash("target"), 0))
    {
        this->helmetRaster = *x;
    }

    if (x = (RwRaster**)xSTFindAsset(xStrHash("target_foot"), 0))
    {
        this->feetRaster = *x;
    }

    crashedScoreboard->chkby &= (U8)~XENT_COLLTYPE_PLYR;
    crashedScoreboard->flags &= (U8)~XENT_IS_VISIBLE;

    strcpy(objName, "CORNER_00");
    for (i = 0; i < 8; i++)
    {
        objName[8] = '0' + (char)i;
        xMarkerAsset* marker = (xMarkerAsset*)xSTFindAsset(xStrHash(objName), &size);
        xVec3Copy(&this->ringCorner[i], (xVec3*)marker);
    }

    for (i = 0; i < 8; i++)
    {
        j = i + 1;
        if (j == 8)
        {
            j = 0;
        }

        xVec3Add(&this->ringEdgeCenter[i], &this->ringCorner[i], &this->ringCorner[j]);
        xVec3SMulBy(&this->ringEdgeCenter[i], 0.5f);
        xVec3Sub(&this->ropeNormal[i], &this->ringCorner[j], &this->ringCorner[i]);

        this->ropeNormal[i].y = this->ropeNormal[i].z;
        this->ropeNormal[i].z = -this->ropeNormal[i].x;
        this->ropeNormal[i].x = this->ropeNormal[i].y;
        this->ropeNormal[i].y = 0.0f;

        xVec3Normalize(&this->ropeNormal[i], &this->ropeNormal[i]);
        xVec3Copy(&this->bouncePoint[i], &this->ringEdgeCenter[i]);
        xVec3AddScaled(&this->bouncePoint[i], &this->ropeNormal[i], 6.042f);
    }

    strcpy(objName, "ROPE_0_0");
    for (i = 0; i < 8; i++)
    {
        objName[5] = '0' + (char)i;

        for (j = 0; j < 4; j++)
        {
            objName[7] = '0' + (char)j;

            this->ropeObject[i][j] = (xEnt*)zSceneFindObject(xStrHash(objName));
            this->ropeObject[i][j]->model->PipeFlags &= ~0xc;
            this->ropeObject[i][j]->model->PipeFlags |= 0x4;

            xAnimState* state = this->ropeObject[i][j]->model->Anim->Table->StateList;
            while (state)
            {
                if (strcmp(state->Name, "Idle01") == 0)
                {
                    state->Flags &= ~0x30;
                    break;
                }

                state = state->Next;
            }
        }
    }

    strcpy(objName, "ROPE_0_LO");
    for (i = 0; i < 8; i++)
    {
        objName[5] = '0' + (char)i;

        this->ropeObjectLo[i] = (xEnt*)zSceneFindObject(xStrHash(objName));
        this->ropeObjectLo[i]->model->PipeFlags &= ~0xc;
        this->ropeObjectLo[i]->model->PipeFlags |= 0x4;
    }

    this->ropeSb = this->ropeObjectLo[4];
    this->ropeSbDamaged = (xEnt*)zSceneFindObject(xStrHash("ROPE_4_LO_DAMAGED"));
    this->ropeSbDamaged->model->PipeFlags &= ~0xc;
    this->ropeSbDamaged->model->PipeFlags |= 0x4;

    strcpy(objName, "TURNBUCKLE_OBJ_00");
    for (i = 0; i < 8; i++)
    {
        objName[16] = '0' + (char)i;

        this->turnbuckle[i] = (xEnt*)zSceneFindObject(xStrHash(objName));
        this->turnbuckle[i]->model->PipeFlags &= ~0xc;
        this->turnbuckle[i]->model->PipeFlags |= 0x4;
    }

    sRadiusOfRing = 0.0f;
    for (i = 0; i < 4; i++)
    {
        xVec3Sub(&endPnt, &this->bouncePoint[i], &this->bouncePoint[i + 4]);
        sRadiusOfRing = 0.125f * xVec3Length(&endPnt) + sRadiusOfRing;
    }

    sLeftLegSpring.bound = NULL;
    sRightLegSpring.bound = NULL;
    sLeftArmSpring.bound = NULL;
    sRightArmSpring.bound = NULL;

    zEntEvent(headBoulder, eEventKill);

    xModelInstance* currModel = this->model;
    while (currModel)
    {
        currModel->Data->boundingSphere.radius = 100.0f;
        currModel = currModel->Next;
    }

    this->jawTime = 0.0f;
    this->jawData = xJaw_FindData(xStrHash("SpongebobPoseidome 44"));

    zNPCBSandy::InitFX();

    if (this->psy_instinct)
    {
        this->psy_instinct->GoalSet(0x4e474231, 0);
    }

    nfFlags = 0;
    xVec3Init(&zeroPnt, 0.0f, 0.0f, 0.0f);

    this->sparks[4].start = &zeroPnt;
    this->sparks[4].end = &zeroPnt;

    if (!this->wireLight[0])
    {
        this->wireLight[0] = zLightningAdd(&this->sparks[4]);
    }

    this->wireLight[0]->flags &= ~0x40;
    this->sparks[4].start = NULL;
    this->sparks[4].end = NULL;

    this->sparks[5].start = &zeroPnt;
    this->sparks[5].end = &zeroPnt;

    if (!this->wireLight[1])
    {
        this->wireLight[1] = zLightningAdd(&this->sparks[5]);
    }

    this->wireLight[1]->flags &= ~0x40;
    this->sparks[5].start = NULL;
    this->sparks[5].end = NULL;
}

void zNPCBSandy::ParseINI()
{
    zNPCCommon::ParseINI();

    this->cfg_npc->snd_traxShare = NULL;
    this->cfg_npc->snd_trax = g_sndTrax_BossSandy;

    NPCS_SndTablePrepare(g_sndTrax_BossSandy);
}

U32 zNPCBSandy::AnimPick(S32 gid, en_NPC_GOAL_SPOT param_2, xGoal* rawgoal)
{
    S32 index = -1;
    U32 animID = 0;

    switch (gid)
    {
    case 'NGB1':
        index = 1;
        break;
    case 'NGB2':
        index = 3;
        break;
    case 'NGB3':
        index = 5;
        break;
    case 'NGB4':
        index = 6;
        break;
    case 'NGB5':
    {
        zNPCGoalBossSandyNoHead* noHeadGoal = (zNPCGoalBossSandyNoHead*)rawgoal;
        U32 anid = noHeadGoal->stage;
        if (anid == 0)
        {
            if (round == 2)
            {
                index = 23;
            }
            else
            {
                index = 24;
            }
        }
        else if (anid == 1)
        {
            if (round == 2)
            {
                index = 21;
            }
            else
            {
                index = 22;
            }
        }
        else if (anid == 2)
        {
            index = 25;
        }
        else if (anid == 3)
        {
            index = 21;
        }
        else if (anid == 4)
        {
            index = 26;
        }
        else if (anid == 5)
        {
            index = 27;
        }
        break;
    }
    case 'NGB6':
        index = 11;
        break;
    case 'NGB7':
        index = 12;
        break;
    case 'NGB8':
    {
        zNPCGoalBossSandySit* sitGoal = (zNPCGoalBossSandySit*)rawgoal;
        if ((sitGoal->sitFlags & 2) != 0x0)
        {
            index = 17;
        }
        else
        {
            index = 16;
        }
        break;
    }
    case 'NGB:':
        index = 4;
        break;
    case 'NGB;':
    {
        zNPCGoalBossSandyClothesline* cl = (zNPCGoalBossSandyClothesline*)rawgoal;
        if (cl->stage == 0)
        {
            index = 18;
        }
        else if (cl->stage == 1 && cl->playedAnimEarly == FALSE)
        {
            index = 19;
        }
        else
        {
            index = 20;
        }
        break;
    }
    case 'NGB9':
        index = 9;
        break;
    default:
        index = 1;
        break;
    }

    if (index > -1)
    {
        animID = g_hash_bossanim[index];
    }

    return animID;
}

static void SpringRender(SandyLimbSpring* spring)
{
    F32 end1 = spring->bound->box.box.lower.x;
    F32 end2 = spring->bound->box.box.upper.x;
    F32 curr;
    F32 percent;
    F32 len = end2 - end1;
    F32 radius = spring->bound->box.box.upper.y;
    F32 minStep = 0.1f * (4.0f * (2.0f * len)) / (478.0f - 0.6f * (2.0f * len) / 0.1f);
    F32 step;
    F32 node1Dist;
    F32 node2Dist;
    RxObjSpace3DVertex* verts = gRenderArr.m_vertex;
    U32 numVerts = 0;
    S32 done = 0;
    S32 currSin = 0;
    S32 currCos = 4;

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);

    curr = end1;
    percent = 0.0f;

    do
    {
        if (!done && curr > end2)
        {
            curr = end2;
            percent = 1.0f;
            done = 1;
        }

        F32 y0 = radius * (0.9f * sSinTable[currSin]);
        F32 z0 = radius * (0.9f * sSinTable[currCos]);

        RwIm3DVertexSetPos(&verts[numVerts], curr, y0, z0);

        F32 y1 = radius * (1.1f * sSinTable[currSin]);
        F32 z1 = radius * (1.1f * sSinTable[currCos]);

        RwIm3DVertexSetPos(&verts[numVerts + 1], curr, y1, z1);
        RwIm3DVertexSetRGBA(&verts[numVerts], 0x80, 0x80, 0x80, 0xff);
        RwIm3DVertexSetRGBA(&verts[numVerts + 1], 0x80, 0x80, 0x80, 0xff);

        numVerts += 2;

        node1Dist = spring->node1 - percent;
        if (node1Dist < 0.0f)
        {
            node1Dist = -node1Dist;
        }
        if (node1Dist > 0.1f)
        {
            step = 1.0f;
        }
        else
        {
            step = 10.0f * node1Dist;
        }

        node2Dist = spring->node2 - percent;
        if (node2Dist < 0.0f)
        {
            node2Dist = -node2Dist;
        }
        if (node2Dist > 0.1f)
        {
            node2Dist = 1.0f;
        }
        else
        {
            node2Dist = 10.0f * node2Dist;
        }

        if (node2Dist < step)
        {
            step = node2Dist;
        }

        step = 0.1f * step;
        if (step < minStep)
        {
            step = minStep;
        }

        curr += step;
        percent = (curr - end1) / len;

        currSin++;
        if (currSin == 16)
        {
            currSin = 0;
        }

        currCos++;
        if (currCos == 16)
        {
            currCos = 0;
        }
    } while (!done);

    RwIm3DTransform(verts, numVerts, (RwMatrix*)spring->bound->mat,
                    rwIM3D_ALLOPAQUE | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);
    RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
    RwIm3DEnd();

    spring->node1 += spring->vel1;
    if ((spring->node1 > 0.9f && spring->vel1 > 0.0f) ||
        (spring->node1 < 0.1f && spring->vel1 < 0.0f))
    {
        spring->vel1 = -spring->vel1;
        spring->node1 += 2.0f * spring->vel1;
    }

    spring->node2 += spring->vel2;
    if ((spring->node2 > 0.9f && spring->vel2 > 0.0f) ||
        (spring->node2 < 0.1f && spring->vel2 < 0.0f))
    {
        spring->vel2 = -spring->vel2;
        spring->node2 += 2.0f * spring->vel2;
    }
}

void zNPCBSandy_BossDamageEffect_Init()
{
    for (S32 i = 0; i < 4; i++)
    {
        BDErecord[i].BDEminst = NULL;
    }
}

static void zNPCBSandy_BossDamageEffect(xModelInstance* minst, U32 turnOff)
{
    S32 j;
    S32 i;

    if (turnOff)
    {
        for (i = 0; i < 4; i++)
        {
            if (BDErecord[i].BDEminst == minst)
            {
                break;
            }
        }

        if (i >= 4)
        {
            return;
        }

        j = 0;
        minst = BDErecord[i].BDEminst;
        BDErecord[i].BDEtimer = 0.0f;

        while (minst != NULL)
        {
            minst->RedMultiplier = BDErecord[i].save_F32[j++];
            minst->GreenMultiplier = BDErecord[i].save_F32[j++];
            minst->BlueMultiplier = BDErecord[i].save_F32[j++];
            minst = minst->Next;
        }

        BDErecord[i].BDEminst = NULL;
    }
    else
    {
        for (i = 0; i < 4; i++)
        {
            if (BDErecord[i].BDEminst == NULL)
            {
                break;
            }
        }

        if (i >= 4)
        {
            return;
        }

        j = 0;
        BDErecord[i].BDEminst = minst;
        BDErecord[i].BDEtimer = 3.0f;

        while (minst != NULL)
        {
            BDErecord[i].save_F32[j++] = minst->RedMultiplier;
            minst->RedMultiplier = 1.0f;
            BDErecord[i].save_F32[j++] = minst->GreenMultiplier;
            minst->GreenMultiplier = 0.0f;
            BDErecord[i].save_F32[j++] = minst->BlueMultiplier;
            minst->BlueMultiplier = 0.0f;
            minst = minst->Next;
        }
    }
}

static void zNPCBSandy_BossDamageEffect_Update(F32 dt)
{
    S32 i;
    F32 frac;
    F32 phase;
    F32 gb;

    for (i = 0; i < 4; i++)
    {
        if (BDErecord[i].BDEminst == NULL)
        {
            continue;
        }

        if (!(BDErecord[i].BDEtimer > 0.0f))
        {
            continue;
        }

        if (BDErecord[i].BDEtimer < dt)
        {
            zNPCBSandy_BossDamageEffect(BDErecord[i].BDEminst, 1);
        }

        if (!(BDErecord[i].BDEtimer > 0.0f))
        {
            continue;
        }

        BDErecord[i].BDEtimer -= dt;

        frac = 1.0f - BDErecord[i].BDEtimer / 3.0f;
        phase = 9.0f * (6.2831855f * frac);
        gb = 0.7f * xpow(icos(3.1415927f * frac), 100.0f) + 0.3f;

        for (xModelInstance* m = BDErecord[i].BDEminst; m != NULL; m = m->Next)
        {
            m->RedMultiplier = 0.25f * icos(phase) + 0.75f;
            m->GreenMultiplier = gb;
            m->BlueMultiplier = gb;
        }
    }
}

void zNPCBSandy::Render()
{
    RwMatrix mat;
    F32 len;

    if (iModelSphereCull(&this->boundList[10]->bound.sph))
    {
        this->model->Next->Next->Next->Next->Flags &= ~0x1;
    }
    else
    {
        this->model->Next->Next->Next->Next->Flags |= 0x1;
    }

    if (iModelSphereCull(&this->boundList[12]->bound.sph))
    {
        this->model->Next->Next->Next->Flags &= ~0x1;
    }
    else
    {
        this->model->Next->Next->Next->Flags |= 0x1;
    }

    if (iModelSphereCull(&this->boundList[3]->bound.sph))
    {
        this->model->Next->Next->Next->Next->Next->Flags &= ~0x1;
    }
    else
    {
        this->model->Next->Next->Next->Next->Next->Flags |= 0x1;
    }

    if (iModelSphereCull(&this->boundList[5]->bound.sph))
    {
        this->model->Next->Next->Next->Next->Next->Next->Flags &= ~0x1;
    }
    else
    {
        this->model->Next->Next->Next->Next->Next->Next->Flags |= 0x1;
    }

    xEntRender(this);

    if (this->bossFlags & 0x80)
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, this->targetRaster);

        xVec3Copy((xVec3*)&mat.pos, &this->targetPos);
        xVec3Copy((xVec3*)&mat.at, (xVec3*)&globals.camera.mat.at);

        mat.at.y = 0.0f;

        len = xVec3Length((xVec3*)&mat.at);
        if (len < 1e-05f)
        {
            xVec3Init((xVec3*)&mat.at, 0.0f, 0.0f, 1.0f);
        }
        else
        {
            xVec3SMulBy((xVec3*)&mat.at, 1.0f / len);
        }

        xVec3Init((xVec3*)&mat.up, 0.0f, 1.0f, 0.0f);
        xVec3Init((xVec3*)&mat.right, mat.at.z, 0.0f, -mat.at.x);

        RwMatrixUpdate(&mat);

        RwIm3DTransform(&this->iconVert[0], 4, &mat,
                        rwIM3D_VERTEXUV | rwIM3D_ALLOPAQUE | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA);
        RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
        RwIm3DEnd();
    }

    if (sLeftLegSpring.bound)
    {
        SpringRender(&sLeftLegSpring);
    }

    if (sRightLegSpring.bound)
    {
        SpringRender(&sRightLegSpring);
    }

    if (sLeftArmSpring.bound)
    {
        SpringRender(&sLeftArmSpring);
    }

    if (sRightArmSpring.bound)
    {
        SpringRender(&sRightArmSpring);
    }
}

void zNPCBSandy::CalcMagnetizeInfo()
{
    xMat4x3 boneMat;
    xQuatFromMat(&this->qBoulder, (xMat3x3*)this->headBoulder->model->Mat);
    xVec3Copy(&this->pBoulder, (xVec3*)&this->headBoulder->model->Mat->pos);
    xMat4x3Mul(&boneMat, (xMat4x3*)&this->model->Mat[sBone[1]], (xMat4x3*)this->model->Mat);
    xQuatFromMat(&this->qHead, &boneMat);
    xMat3x3RMulVec(&this->pHead, &boneMat, &sBoneOffset[1]);
    xVec3AddTo(&this->pHead, &boneMat.pos);
    magnetizeTime = xVec3Dist(&this->pHead, &this->pBoulder) * 0.1f;
}

void zNPCBSandy::InitFX()
{
    this->timeToNextBolt[0] = 0.0f;
    this->timeToNextBolt[1] = 0.0f;
    this->maxLightningWait[0] = 1.0f;
    this->maxLightningWait[1] = 1.0f;

    xMat4x3Copy((xMat4x3*)&this->sparkTransform[0][0], (xMat4x3*)this->hangingScoreboard->model->Mat);
    xMat4x3Copy((xMat4x3*)&this->sparkTransform[0][1], (xMat4x3*)this->hangingScoreboard->model->Mat);
    xMat4x3Copy((xMat4x3*)&this->sparkTransform[1][0], (xMat4x3*)this->crashedScoreboard->model->Mat);
    xVec3SMul((xVec3*)&this->sparkTransform[1][1], (xVec3*)this->crashedScoreboard->model->Mat, 0.8f);
    xVec3SMul((xVec3*)&this->sparkTransform[1][1].up, (xVec3*)&this->crashedScoreboard->model->Mat->at, -0.8f);
    xVec3SMul((xVec3*)&this->sparkTransform[1][1].at, (xVec3*)&this->crashedScoreboard->model->Mat->up, 0.8f);
    xVec3Copy((xVec3*)&this->sparkTransform[1][1].pos, (xVec3*)&this->crashedScoreboard->model->Mat->pos);
    xVec3AddScaled((xVec3*)&this->sparkTransform[1][1].pos, (xVec3*)&this->crashedScoreboard->model->Mat->up, 4.5f);
    xVec3AddScaled((xVec3*)&this->sparkTransform[1][1].pos, (xVec3*)&this->crashedScoreboard->model->Mat->at, 5.0f);
    xVec3Init((xVec3*)&this->endPoints[0][0], -2.0f, 2.0f, 0.0f);
    xVec3Init((xVec3*)&this->endPoints[0][1], 2.0f, 2.0f, 0.0f);

    this->sparks[0].type = 0x3;
    this->sparks[0].setup_degrees = 66.0f;
    this->sparks[0].move_degrees = -2500.0f;
    this->sparks[0].rot_radius = 0.15f;
    this->sparks[0].total_points = 0x10;
    this->sparks[0].end_points = 0;
    this->sparks[0].time = 0.25f;
    this->sparks[0].arc_height = -1.5f;
    this->sparks[0].thickness = 1.0f;
    this->sparks[0].color.r = 0xa0;
    this->sparks[0].color.g = 0xa0;
    this->sparks[0].color.b = 0xff;
    this->sparks[0].color.a = 0xc8;
    this->sparks[0].rand_radius = 13.0f;
    this->sparks[0].flags = 0x1c28;

    xVec3Init((xVec3*)&this->endPoints[1][0], -2.0f, 2.0f, 0.0f);
    xVec3Init((xVec3*)&this->endPoints[1][1], 2.0f, 2.0f, 0.0f);

    this->sparks[1].type = 0x3;
    this->sparks[1].setup_degrees = 66.0f;
    this->sparks[1].move_degrees = -2500.0f;
    this->sparks[1].rot_radius = 0.15f;
    this->sparks[1].total_points = 0x10;
    this->sparks[1].end_points = 0;
    this->sparks[1].time = 0.25f;
    this->sparks[1].arc_height = -1.5f;
    this->sparks[1].thickness = 1.0f;
    this->sparks[1].color.r = 0xc8;
    this->sparks[1].color.g = 0xc8;
    this->sparks[1].color.b = 0x37;
    this->sparks[1].color.a = 0xc8;
    this->sparks[1].rand_radius = 13.0f;
    this->sparks[1].flags = 0x1c28;

    xVec3Init((xVec3*)&this->endPoints[2][0], 0.0f, 2.0f, 0.0f);
    xVec3Init((xVec3*)&this->endPoints[2][1], 3.5f, 2.0f, 0.0f);

    this->sparks[2].type = 0x3;
    this->sparks[2].zeus_normal_offset = 1.0f;
    this->sparks[2].zeus_back_offset = -1.0f;
    this->sparks[2].zeus_side_offset = 0.0f;
    this->sparks[2].total_points = 0x8;
    this->sparks[2].end_points = 0;
    this->sparks[2].time = 0.25f;
    this->sparks[2].arc_height = -2.5f;
    this->sparks[2].thickness = 1.0f;
    this->sparks[2].color.r = 0xa0;
    this->sparks[2].color.g = 0xa0;
    this->sparks[2].color.b = 0xff;
    this->sparks[2].color.a = 0xc8;
    this->sparks[2].rand_radius = 25.0f;
    this->sparks[2].flags = 0x1c20;

    xVec3Init((xVec3*)&this->endPoints[3][0], 0.0f, 2.0f, 0.0f);
    xVec3Init((xVec3*)&this->endPoints[3][1], 3.5f, 2.0f, 0.0f);

    this->sparks[3].type = 0x3;
    this->sparks[3].zeus_normal_offset = 1.0f;
    this->sparks[3].zeus_back_offset = -1.0f;
    this->sparks[3].zeus_side_offset = 0.0f;
    this->sparks[3].total_points = 0xa;
    this->sparks[3].end_points = 0;
    this->sparks[3].time = 0.25f;
    this->sparks[3].arc_height = -2.5f;
    this->sparks[3].thickness = 1.0f;
    this->sparks[3].color.r = 0xc8;
    this->sparks[3].color.g = 0xc8;
    this->sparks[3].color.b = 0x37;
    this->sparks[3].color.a = 0xc8;
    this->sparks[3].rand_radius = 15.0f;
    this->sparks[3].flags = 0x1c20;
    this->sparks[4].type = 0x3;
    this->sparks[4].color.r = 0xc8;
    this->sparks[4].color.g = 0xc8;
    this->sparks[4].color.b = 0xc8;
    this->sparks[4].color.a = 0xff;
    this->sparks[4].time = 1.0f;
    this->sparks[4].thickness = 1.0f;
    this->sparks[4].flags = 0x110;
    this->sparks[5].type = 0x3;
    this->sparks[5].color.r = 0x32;
    this->sparks[5].color.g = 0x32;
    this->sparks[5].color.b = 0xc8;
    this->sparks[5].color.a = 0xff;
    this->sparks[5].time = 1.0f;
    this->sparks[5].thickness = 1.0f;
    this->sparks[5].flags = 0x110;
}

void zNPCBSandy::UpdateFX(F32 dt)
{
    S32 board;
    S32 i;
    S32 boltIdx;
    xVec3 pntB;
    xVec3 pntA;
    xMat3x3 rotMat;

    if (!(this->bustedScoreboard->flags & 0x1) && !(this->crashedScoreboard->flags & 0x1))
    {
        return;
    }

    if (!(this->bustedScoreboard->flags & 0x1))
    {
        board = 1;
    }
    else
    {
        board = 0;
    }

    for (i = 0; i < 2; i++)
    {
        this->maxLightningWait[i] += 0.2f * dt;
        if (this->maxLightningWait[i] > 1.0f)
        {
            this->maxLightningWait[i] = 1.0f;
        }

        if (i == 1 && board == 0)
        {
            return;
        }

        this->timeToNextBolt[i] -= dt;
        if (this->timeToNextBolt[i] < 0.0f)
        {
            this->timeToNextBolt[i] = this->maxLightningWait[i] * xurand();
            boltIdx = (xrand() >> 13) & 0x3;

            xMat3x3RotY(&rotMat, 6.2831855f * xurand());

            xMat3x3RMulVec(&pntB, &rotMat, &this->endPoints[boltIdx][0]);
            xMat3x3RMulVec(&pntB, &this->sparkTransform[board][i], &pntB);
            xVec3AddTo(&pntB, &this->sparkTransform[board][i].pos);

            xMat3x3RMulVec(&pntA, &rotMat, &this->endPoints[boltIdx][1]);
            xMat3x3RMulVec(&pntA, &this->sparkTransform[board][i], &pntA);
            xVec3AddTo(&pntA, &this->sparkTransform[board][i].pos);

            this->sparks[boltIdx].start = &pntB;
            this->sparks[boltIdx].end = &pntA;

            if (board == 1 && i == 1)
            {
                if (this->sparks[boltIdx].arc_height < 0.0f)
                {
                    this->sparks[boltIdx].arc_height = -this->sparks[boltIdx].arc_height;
                }
            }
            else
            {
                if (this->sparks[boltIdx].arc_height > 0.0f)
                {
                    this->sparks[boltIdx].arc_height = -this->sparks[boltIdx].arc_height;
                }
            }

            zLightningAdd(&this->sparks[boltIdx]);
        }
    }
}

static void UpdateSandyBossCam(zNPCBSandy* sandy, F32 dt)
{
    S32 needToCallStart = 0;
    xVec3 tempTarget;

    if ((zCameraIsTrackingDisabled() & 0x8) == 0)
    {
        needToCallStart = 1;
    }

    zCameraDisableTracking(CO_BOSS);

    if (needToCallStart)
    {
        sandy->bossCam.start(globals.camera);
    }

    if (sandy->bossFlags & 0x40)
    {
        if ((sandy->bossFlags & 0x2000) == 0)
        {
            sandy->specialBossCam.start(globals.camera);
        }

        tempTarget.x = globals.player.ent.model->Mat->pos.x;
        tempTarget.y = 0.0f;
        tempTarget.z = globals.player.ent.model->Mat->pos.z;
        sandy->specialBossCam.set_targets(tempTarget, *sCamSubTarget, 2.0f);

        if ((sandy->bossFlags & 0x4000))
        {
            sandy->specialBossCam.update(10.0f);
            sandy->bossFlags &= ~0x4000;
        }
        else
        {
            sandy->specialBossCam.update(dt);
        }
    }
    else
    {
        if (sandy->bossFlags & 0x2000)
        {
            sandy->bossCam.start(globals.camera);
        }

        sandy->bossCam.set_targets(*((xVec3*)&globals.player.ent.model->Mat->pos), *sCamSubTarget,
                                   2.0f);

        if (sandy->bossFlags & 0x4000)
        {
            sandy->bossCam.update(10.0f);
            sandy->bossFlags &= ~0x4000;
        }
        else
        {
            sandy->bossCam.update(dt);
        }
    }

    if (sandy->bossFlags & 0x40)
    {
        sandy->bossFlags |= 0x2000;
    }
    else
    {
        sandy->bossFlags &= ~0x2000;
    }
}

static void GetBonePos(xVec3* result, xMat4x3* matArray, S32 index, xVec3* offset)
{
    xMat4x3 tmpMat;

    if (index == 0)
    {
        xMat3x3RMulVec(result, matArray, offset);
        xVec3AddTo(result, &matArray->pos);
    }
    else
    {
        xMat4x3Mul(&tmpMat, &matArray[index], matArray);

        if (offset)
        {
            xMat3x3RMulVec(result, &tmpMat, offset);
            xVec3AddTo(result, &tmpMat.pos);
        }
        else
        {
            xVec3Copy(result, &tmpMat.pos);
        }
    }
}

static void MakeOBBFor(S32 index1, S32 index2, xEnt* ent, xMat4x3* matArray)
{
    xVec3 startPos;
    xVec3 dir;
    xVec3 axis;
    F32 len;
    F32 angle;

    GetBonePos(&startPos, matArray, sBone[index1], &sBoneOffset[index1]);
    GetBonePos(&dir, matArray, sBone[index2], &sBoneOffset[index2]);

    xVec3SubFrom(&dir, &startPos);
    len = xVec3Normalize(&dir, &dir);

    xVec3Init(&ent->bound.box.box.lower, 0.0f, -0.4f, -0.4f);
    xVec3Init(&ent->bound.box.box.upper, len, 0.4f, 0.4f);
    xVec3Init(&ent->bound.box.center, 0.5f * len, 0.0f, 0.0f);

    xVec3Init(&axis, 0.0f, -dir.z, dir.y);
    angle = xasin(xVec3Normalize(&axis, &axis));

    if (dir.x < 0.0f)
    {
        if (angle < 0.0f)
        {
            angle = -3.1415927f - angle;
        }
        else
        {
            angle = 3.1415927f - angle;
        }
    }

    xMat3x3RotC(ent->bound.mat, axis.x, axis.y, axis.z, angle);
    xVec3Copy(&ent->bound.mat->pos, &startPos);
}

static S32 BoundEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*)
{
    xEnt* ent = (xEnt*)to;
    U32 idx = ent->id;
    zNPCBSandy* sandy = (zNPCBSandy*)ent->driver;

    switch (toEvent)
    {
    case eEventHit:
        sandy->boundFlags[idx] |= 0x2;
        break;
    case eEventHit_BubbleBash:
        sandy->boundFlags[idx] |= 0x2;
        sandy->boundFlags[idx] |= 0x4;
        break;
    }

    return 1;
}

void zNPCBSandy::Process(xScene* xscn, F32 dt)
{
    S32 i;
    S32 j;
    S32 hidden;
    S32 oldRound;
    S32 allDone;
    S32 prev;
    S32 wasBeat;
    S32 idx;
    S32 jawLen;
    S32 whichPoint;
    F32 fadeRate;
    F32 amp;
    F32 base;
    F32 spread;
    F32 distSq;
    F32 radSq;
    F32 lerpAmt;
    xVec3 toEdge;
    F32 color[3];
    xVec3 point;
    xVec3 normal;
    xVec3 toPlayer;
    xVec3 knock;
    xVec3 endA;
    xVec3 endB;
    xVec3 boneOffset;

    if (this->firstUpdate != 0)
    {
        this->firstUpdate = 0;

        switch (this->round)
        {
        case 1:
            if (this->firstTimeR1Csn != 0)
            {
                this->firstTimeR1Csn = 0;
            }
            else
            {
                zEntEvent(this->round1Csn, eEventPreload);
            }
            this->hitPoints = 9;
            break;
        case 2:
            zEntEvent(this->round2Csn, eEventPreload);
            this->hitPoints = 6;
            break;
        case 3:
            zEntEvent(this->round3Csn, eEventPreload);
            this->hitPoints = 3;
            break;
        }
    }

    hidden = 0;

    if (globals.cmgr != NULL)
    {
        if (globals.cmgr->csn->Ready != 0)
        {
            hidden = 1;
        }
    }

    if (hidden)
    {
        if (this->bossFlags & 0x1000)
        {
            this->csnTimer += dt;
        }
        else
        {
            this->csnTimer = 0.0f;
        }

        if (this->bossFlags & 0x400)
        {
            this->bossFlags &= ~0x400;
            this->hiddenByCutscene();
        }
    }
    else if (this->bossFlags & 0x1000)
    {
        this->bossFlags |= 0x4000;
        this->jawTime = 0.0f;
        sElbowDropTimer = 0.0f;
        sChaseTimer = 0.0f;
    }

    if (!(this->baseFlags & 0x1))
    {
        hidden = 1;
    }

    if (hidden)
    {
        this->bossFlags |= 0x1000;
    }
    else
    {
        this->bossFlags &= ~0x1000;
    }

    if (this->psy_instinct)
    {
        this->psy_instinct->Timestep(dt, NULL);
    }

    if (sWasUsingBossCam != sUseBossCam)
    {
        if (sWasUsingBossCam)
        {
            globals.camera.hcur = sCurrHeight;
            globals.camera.dcur = sCurrRadius;
            globals.camera.pcur = 3.1415927f + sCurrYaw;

            if (globals.camera.pcur > 6.2831855f)
            {
                globals.camera.pcur = globals.camera.pcur - 6.2831855f;
            }

            globals.camera.yaw_cur = sCurrYaw;
            globals.camera.pitch_cur = sCurrPitch;
            globals.camera.roll_cur = 0.0f;
        }
        else
        {
            sCurrYaw = globals.camera.yaw_cur;
            sCurrHeight = globals.camera.hcur;
            sCurrRadius = globals.camera.dcur;
            sCurrPitch = globals.camera.pitch_cur;
        }

        sWasUsingBossCam = sUseBossCam;
    }

    if (sUseBossCam)
    {
        UpdateSandyBossCam(this, dt);
    }
    else
    {
        zCameraEnableTracking(CO_BOSS);
    }

    this->scoreboardAlpha = 1.0f;

    if (this->bossFlags & 0x1000)
    {
        for (i = 0; i < 8; i++)
        {
            this->edgeAlpha[i] = 1.0f;
        }
    }
    else
    {
        for (i = 0; i < 8; i++)
        {
            this->edgeAlpha[i] = 1.0f;

            xVec3Sub(&toEdge, (xVec3*)&globals.camera.mat.pos, &this->ringEdgeCenter[i]);

            if (xVec3Dot(&toEdge, &this->ropeNormal[i]) < 0.0f)
            {
                this->edgeAlpha[i] = 0.1f;

                if (i == 4)
                {
                    this->scoreboardAlpha = 0.1f;
                }
            }
        }
    }

    fadeRate = 4.0f * dt;

    for (i = 0; i < 8; i++)
    {
        if (this->edgeAlpha[i] > 0.9f)
        {
            for (j = 0; j < 4; j++)
            {
                this->ropeObject[i][j]->model->Alpha += dt;

                if (this->ropeObject[i][j]->model->Alpha > this->edgeAlpha[i])
                {
                    this->ropeObject[i][j]->model->Alpha = this->edgeAlpha[i];
                }
            }

            this->ropeObjectLo[i]->model->Alpha += dt;

            if (this->ropeObjectLo[i]->model->Alpha > this->edgeAlpha[i])
            {
                this->ropeObjectLo[i]->model->Alpha = this->edgeAlpha[i];
            }
        }
        else
        {
            for (j = 0; j < 4; j++)
            {
                this->ropeObject[i][j]->model->Alpha -= fadeRate;

                if (this->ropeObject[i][j]->model->Alpha < this->edgeAlpha[i])
                {
                    this->ropeObject[i][j]->model->Alpha = this->edgeAlpha[i];
                }
            }

            this->ropeObjectLo[i]->model->Alpha -= fadeRate;

            if (this->ropeObjectLo[i]->model->Alpha < this->edgeAlpha[i])
            {
                this->ropeObjectLo[i]->model->Alpha = this->edgeAlpha[i];
            }
        }

        prev = i - 1;
        if (prev < 0)
        {
            prev += 8;
        }

        if (this->edgeAlpha[i] < 0.9f)
        {
            this->turnbuckle[i]->model->Alpha -= fadeRate;

            if (this->turnbuckle[i]->model->Alpha < this->edgeAlpha[i])
            {
                this->turnbuckle[i]->model->Alpha = this->edgeAlpha[i];
            }
        }
        else if (this->edgeAlpha[prev] < 0.9f)
        {
            this->turnbuckle[i]->model->Alpha -= fadeRate;

            if (this->turnbuckle[i]->model->Alpha < this->edgeAlpha[prev])
            {
                this->turnbuckle[i]->model->Alpha = this->edgeAlpha[prev];
            }
        }
        else
        {
            this->turnbuckle[i]->model->Alpha += dt;

            if (this->turnbuckle[i]->model->Alpha > this->edgeAlpha[i])
            {
                this->turnbuckle[i]->model->Alpha = this->edgeAlpha[i];
            }
        }
    }

    if (this->scoreboardAlpha > 0.9f)
    {
        this->crashedScoreboard->model->Alpha += dt;

        if (this->crashedScoreboard->model->Alpha > this->scoreboardAlpha)
        {
            this->crashedScoreboard->model->Alpha = this->scoreboardAlpha;
        }
    }
    else
    {
        this->crashedScoreboard->model->Alpha -= fadeRate;

        if (this->crashedScoreboard->model->Alpha < this->scoreboardAlpha)
        {
            this->crashedScoreboard->model->Alpha = this->scoreboardAlpha;
        }
    }

    this->UpdateFX(dt);

    sElbowDropTimer += dt;
    sElbowDropThreshold = sElbowDropThreshold - 0.5f;

    if (sElbowDropThreshold < 0.0f)
    {
        sElbowDropThreshold = 0.0f;
    }

    if (strcmp("BbounceAttack01", globals.player.ent.model->Anim->Single->State->Name) != 0 &&
        strcmp("StunFall", globals.player.ent.model->Anim->Single->State->Name) != 0)
    {
        if (sPCWasBubbleBouncing)
        {
            sHeadPopOffFactor = 1.1f;
        }
        else
        {
            sHeadPopOffFactor = -1.0f;
        }

        sPCHeightDiff = globals.player.ent.model->Mat->pos.y;
        sPCWasBubbleBouncing = 0;
    }
    else
    {
        sPCWasBubbleBouncing = 1;
    }

    oldRound = this->round;

    if ((this->bossFlags & 0x20) && globals.player.ControlOff == 0)
    {
        if (oldRound == 1)
        {
            if (this->hitPoints <= 6)
            {
                this->round = 2;
            }
        }
        else if (oldRound == 2)
        {
            if (this->hitPoints <= 3)
            {
                this->round = 3;
            }
        }
    }

    if (oldRound != this->round)
    {
        this->bossFlags |= 0x400;

        if (this->round == 1)
        {
            zEntEvent(this->round1Csn, eEventPreload);
        }
        else if (this->round == 2)
        {
            zEntEvent(this->round2Csn, eEventPreload);
        }
        else if (this->round == 3)
        {
            zEntEvent(this->round3Csn, eEventPreload);
        }

        this->nfFlags = 0;
    }

    xprintf("Round %d\n", this->round);
    xprintf("R1 - Next  R2 - Prev\n");

    for (i = 0; i < 8; i++)
    {
        allDone = 1;

        for (j = 0; j < 4; j++)
        {
            if (this->ropeObject[i][j]->model->Anim->Single->CurrentSpeed > 1e-05f)
            {
                allDone = 0;
            }
        }

        if (allDone)
        {
            for (j = 0; j < 4; j++)
            {
                xEnt* rope = this->ropeObject[i][j];
                rope->model->Flags &= 0xfffd;
                xEntHide(rope);
            }

            xEntShow(this->ropeObjectLo[i]);
        }
    }

    this->jawTime += dt;

    jawLen = *(S32*)this->jawData;

    if (60.0f * this->jawTime > jawLen)
    {
        this->jawTime -= jawLen / 60.0f;
    }

    amp = xJaw_EvalData(this->jawData, this->jawTime);

    this->jawLevel *= 0.9f;
    this->jawLevel = 0.1f * amp + this->jawLevel;
    this->jawThreshold *= 0.99f;
    this->jawThreshold = 0.01f * amp + this->jawThreshold;

    wasBeat = this->isBeat;

    if (amp > 0.5f)
    {
        this->isBeat = 1;
    }
    else
    {
        this->isBeat = 0;
    }

    if (wasBeat == 0 && this->isBeat != 0)
    {
        color[0] = 0.5f * xurand();
        color[1] = 0.5f * xurand();
        color[2] = 0.5f * xurand();

        idx = (S32)(2.95f * xurand());

        color[idx] += 0.5f;

        this->curveNodeAlpha = 1.0f;

        this->curveNodeR = color[0];
        this->curveNodeG = color[1];
        this->curveNodeB = color[2];
    }

    base = 0.25f * this->jawLevel + 0.25f;

    this->curveNodeAlpha -= dt;

    if (this->curveNodeAlpha < 0.5f * this->jawLevel)
    {
        this->curveNodeAlpha = base;
    }

    // Retail inlined LERP(x, y, z) == x * (z - y) + y here. This build compiles the
    // game with -inline off, so calling LERP() would emit an out-of-line call the
    // target object does not have; the arithmetic is written out instead. The
    // blend factor goes through lerpAmt because the target keeps the multiply by
    // 1.0f/0.0f -- as an inlined parameter it was never constant-folded -- and a
    // literal there folds the fmadds away.
    lerpAmt = 1.0f;
    this->curveNode[0].color.a = (U8)(255.0f * (lerpAmt * (this->curveNodeAlpha - base) + base));
    this->curveNode[0].color.r = (U8)(255.0f * this->curveNodeR);
    this->curveNode[0].color.g = (U8)(255.0f * this->curveNodeG);
    this->curveNode[0].color.b = (U8)(255.0f * this->curveNodeB);
    lerpAmt = 0.0f;
    this->curveNode[1].color.a = (U8)(255.0f * (lerpAmt * (this->curveNodeAlpha - base) + base));
    this->curveNode[1].color.r = (U8)(255.0f * this->curveNodeR);
    this->curveNode[1].color.g = (U8)(255.0f * this->curveNodeG);
    this->curveNode[1].color.b = (U8)(255.0f * this->curveNodeB);

    this->laserShow.set_curve(&this->curveNode[0], 2);

    whichPoint = (S32)(15.5f * xurand());

    xVec3Copy(&point, &this->laserPoint[whichPoint]);

    normal.x = -point.z;
    normal.y = 0.0f;
    normal.z = point.x;

    xVec3Normalize(&normal, &normal);

    this->laserShow.insert(point, normal, 1.0f, 1.0f, 0);

    spread = 2.0f * (xurand() - 0.5f);
    spread = spread * (1.5f - (0.5f * spread) * spread);
    point.x = 20.0f * spread;

    spread = 2.0f * (xurand() - 0.5f);
    spread = spread * (1.5f - (0.5f * spread) * spread);
    point.z = 20.0f * spread;
    point.y = 40.0f;

    this->laserShow.insert(point, normal, 1.0f, 1.0f, 0);

    if (this->bossFlags & 0x800)
    {
        this->shockRadius =
            this->shockRadius + dt * this->shockwaveGrowthRate / xsqrt(this->shockRadius);

        if (this->shockRadius > this->shockwaveMaxRadius)
        {
            this->bossFlags &= ~0x800;
            this->shockwaveEmitter->emit_flags &= ~0x1;
        }
        else
        {
            this->shockwaveEmitter->emit_flags |= 0x1;
            this->shockwaveEmitter->tasset->e_circle.radius = this->shockRadius;

            if (globals.player.JumpState == 0)
            {
                xVec3Sub(&toPlayer, (xVec3*)&globals.player.ent.model->Mat->pos,
                         (xVec3*)&this->shockwaveEmitter->tasset->pos);

                distSq = xVec3Length2(&toPlayer);
                radSq = this->shockRadius * this->shockRadius;

                if (distSq < radSq && distSq > 0.81f * radSq)
                {
                    xVec3Sub(&knock, (xVec3*)&globals.player.ent.model->Mat->pos,
                             &this->frame->mat.pos);

                    knock.y = 0.0f;
                    xVec3Normalize(&knock, &knock);

                    xVec3SMul((xVec3*)&globals.player.ent.model->Mat->at, &knock, -1.0f);
                    xVec3Cross((xVec3*)&globals.player.ent.model->Mat->right,
                               (xVec3*)&globals.player.ent.model->Mat->up,
                               (xVec3*)&globals.player.ent.model->Mat->at);

                    xVec3SMulBy(&knock, 10.0f);
                    knock.y = 7.0f;

                    zEntPlayer_Damage(NULL, 1, &knock);

                    this->bossFlags |= 0x2;
                }
            }
        }
    }
    else
    {
        this->shockwaveEmitter->emit_flags &= ~0x1;
    }

    if (this->round != 1)
    {
        xVec3Init(&boneOffset, 0.0f, 0.0f, 0.5f);

        GetBonePos(&endA, (xMat4x3*)this->crashedScoreboard->model->Mat, 5, &boneOffset);
        GetBonePos(&endB, (xMat4x3*)this->crashedScoreboard->model->Mat, 8, &boneOffset);

        zLightningModifyEndpoints(this->wireLight[0], &endA, &endB);
        zLightningModifyEndpoints(this->wireLight[1], &endB, &endA);
    }

    zNPCCommon::Process(xscn, dt);

    zNPCBSandy_BossDamageEffect_Update(dt);
}

void zNPCBSandy::hiddenByCutscene()
{
    for (S32 i = 0; i < 3; i++)
    {
        this->underwear[i]->state = (this->underwear[i]->state & 0xffffffc0) | 1;
        zEntEvent(this->underwear[i], eEventCollision_Visible_On);
        this->underwear[i]->timer = 0.0f; // 0.0
    }

    switch (this->round)
    {
    case 1:
    {
        this->hangingScoreboard->chkby &= 0xef;
        this->hangingScoreboard->flags |= 1;
        this->bustedScoreboard->chkby &= 0xef;
        this->bustedScoreboard->flags &= 0xfe;
        this->crashedScoreboard->chkby &= 0xef;
        this->crashedScoreboard->flags &= 0xfe;

        this->ropeObjectLo[4] = this->ropeSb;

        xEntHide(this->ropeSbDamaged);
        xEntShow(this->ropeSb);

        gCurrentPlayer = eCurrentPlayerSpongeBob;

        zLightningShow(this->wireLight[0], 0);
        zLightningShow(this->wireLight[1], 0);

        break;
    }
    case 2:
    {
        this->crashedScoreboard->chkby |= 0x10;
        this->crashedScoreboard->flags |= 1;
        this->hangingScoreboard->chkby &= 0xef;
        this->hangingScoreboard->flags &= 0xfe;
        this->bustedScoreboard->chkby &= 0xef;
        this->bustedScoreboard->flags &= 0xfe;

        this->ropeObjectLo[4] = this->ropeSbDamaged;

        xEntHide(this->ropeSb);
        xEntShow(this->ropeSbDamaged);

        zLightningShow(this->wireLight[0], 1);
        zLightningShow(this->wireLight[1], 1);

        gCurrentPlayer = eCurrentPlayerPatrick;

        break;
    }
    case 3:
    {
        gCurrentPlayer = eCurrentPlayerSpongeBob;
        break;
    }
    }
}

void zNPCBSandy::Damage(en_NPC_DAMAGE_TYPE damtype, xBase*, const xVec3*)
{
    if (damtype == DMGTYP_INSTAKILL)
    {
        this->bossFlags |= 0x100;
    }
}

void zNPCBSandy_AddBoundEntsToGrid(zScene* scn)
{
    S32 i;
    S32 isLimb;

    if (sOthersHaventBeenAdded)
    {
        sOthersHaventBeenAdded = 0;

        for (i = 0; i < 13; i++)
        {
            xEnt* ent = sSandyPtr->boundList[i];
            if (i == 2 || i == 4 || i == 9 || i == 11)
            {
                isLimb = TRUE;
            }
            else
            {
                isLimb = FALSE;
            }

            if (isLimb || xGridEntIsTooBig(&colls_grid, ent) != FALSE)
            {
                xGridAdd(&colls_oso_grid, ent);

                if (isLimb || xGridEntIsTooBig(&colls_oso_grid, ent) != FALSE)
                {
                    ent->gridb.oversize = 0x2;
                }
                else
                {
                    ent->gridb.oversize = 0x1;
                }
            }
            else
            {
                xGridAdd(&colls_grid, ent);
                ent->gridb.oversize = 0x0;
            }
        }
    }
    else
    {
        sSandyPtr = NULL;
    }
}

void zNPCBSandy_GameIsPaused(zScene*)
{
    if (sSandyPtr)
    {
        if (sSandyPtr->bossFlags & 0x400)
        {
            sSandyPtr->bossFlags &= ~0x400; // clear bit
            sSandyPtr->hiddenByCutscene();
        }
    }
}

void zNPCBSandy::NewTime(xScene* xscn, F32 dt)
{
    S32 i;
    S32 hidden = 0;
    F32 dot;
    F32 dist;
    F32 posDist;
    F32 negDist;
    F32 amount;
    xCollis* coll;
    xCollis* collEnd;
    xVec3 up;
    xVec3 toEdge;
    xVec3 dir;

    if (globals.cmgr != NULL)
    {
        if (globals.cmgr->csn->Ready != 0)
        {
            hidden = 1;
        }
    }

    if (!(this->baseFlags & 0x1))
    {
        hidden = 1;
    }

    xVec3Init(&up, 1.0f, 0.0f, 0.0f);

    posDist = 30.0f;
    negDist = 30.0f;

    for (i = 0; i < 8; i++)
    {
        dot = xVec3Dot(&this->ropeNormal[i], (xVec3*)&this->model->Mat->right);

        if (dot < 1e-05f && dot > -1e-05f)
        {
            continue;
        }

        xVec3Sub(&toEdge, (xVec3*)&this->model->Mat->pos, &this->ringEdgeCenter[i]);

        dist = xVec3Dot(&toEdge, &this->ropeNormal[i]) / -dot;

        if (dist < 0.0f)
        {
            if (-dist < negDist)
            {
                negDist = -dist;
            }
        }
        else
        {
            if (dist < posDist)
            {
                posDist = dist;
            }
        }
    }

    if (this->bossFlags & 0x8)
    {
        amount = -(negDist - 2.0f);
        for (i = 0; i < 2; i++)
        {
            xVec3AddScaled((xVec3*)&this->model->Mat[sLeftFootBones[i]].pos, &up, amount);
        }

        amount = posDist - 2.0f;
        for (i = 0; i < 2; i++)
        {
            xVec3AddScaled((xVec3*)&this->model->Mat[sRightFootBones[i]].pos, &up, amount);
        }
    }

    if (this->bossFlags & 0x10)
    {
        amount = -(negDist - 1.0f);
        for (i = 0; i < 4; i++)
        {
            xVec3AddScaled((xVec3*)&this->model->Mat[sLeftHandBones[i]].pos, &up, amount);
        }

        amount = posDist - 1.0f;
        for (i = 0; i < 4; i++)
        {
            xVec3AddScaled((xVec3*)&this->model->Mat[sRightHandBones[i]].pos, &up, amount);
        }
    }

    for (i = 0; i < 13; i++)
    {
        if (sBoundRadius[i] < 0.0f)
        {
            continue;
        }

        GetBonePos(&this->boundList[i]->bound.sph.center, (xMat4x3*)this->model->Mat, sBone[i],
                   &sBoneOffset[i]);
        xQuickCullForBound(&this->boundList[i]->bound.qcd, &this->boundList[i]->bound);
        zGridUpdateEnt(this->boundList[i]);

        if (this->boundList[i]->bound.sph.center.y - this->boundList[i]->bound.sph.r > 0.0f)
        {
            this->boundFlags[i] |= 0x8;
        }
        else if (this->boundFlags[i] & 0x8)
        {
            this->boundFlags[i] &= ~0x8;

            if (this->boundEmitTimer[i] <= 0.0f && (this->boundFlags[i] & 0x10) && hidden == 0)
            {
                if (xrand() & 0x4)
                {
                    xSndPlay3D(xStrHash("B101_SC_step1"), 2.31f, 0.0f, 0x0, 0x0, this, 30.0f,
                               SND_CAT_GAME, 0.0f);
                }
                else
                {
                    xSndPlay3D(xStrHash("B101_SC_step2"), 2.31f, 0.0f, 0x0, 0x0, this, 30.0f,
                               SND_CAT_GAME, 0.0f);
                }
            }

            this->boundEmitTimer[i] = 0.5f;
        }

        if (this->boundEmitTimer[i] > 0.0f && hidden == 0)
        {
            this->boundEmitTimer[i] = this->boundEmitTimer[i] - dt;
            this->dustEddieSetting.pos.y = 0.0f;
            this->dustEddieSetting.pos.x = this->boundList[i]->bound.sph.center.x;
            this->dustEddieSetting.pos.z = this->boundList[i]->bound.sph.center.z;
            xParEmitterEmitCustom(this->dustEddieEmitter, dt, &this->dustEddieSetting);
        }
    }

    MakeOBBFor(2, 3, this->boundList[2], (xMat4x3*)this->model->Mat);
    xQuickCullForBound(&this->boundList[2]->bound.qcd, &this->boundList[2]->bound);
    zGridUpdateEnt(this->boundList[2]);

    MakeOBBFor(4, 5, this->boundList[4], (xMat4x3*)this->model->Mat);
    xQuickCullForBound(&this->boundList[4]->bound.qcd, &this->boundList[4]->bound);
    zGridUpdateEnt(this->boundList[4]);

    MakeOBBFor(9, 10, this->boundList[9], (xMat4x3*)this->model->Mat);
    xQuickCullForBound(&this->boundList[9]->bound.qcd, &this->boundList[9]->bound);
    zGridUpdateEnt(this->boundList[9]);

    MakeOBBFor(11, 12, this->boundList[11], (xMat4x3*)this->model->Mat);
    xQuickCullForBound(&this->boundList[11]->bound.qcd, &this->boundList[11]->bound);
    zGridUpdateEnt(this->boundList[11]);

    if (this->bossFlags & 0x1)
    {
        coll = globals.player.ent.collis->colls;
        collEnd = coll + globals.player.ent.collis->idx;

        for (; coll < collEnd; coll++)
        {
            if (!(coll->flags & 0x1))
            {
                continue;
            }

            xEnt* hit = (xEnt*)coll->optr;
            if (hit == NULL)
            {
                continue;
            }

            if (hit->baseType != eBaseTypeDynamic)
            {
                continue;
            }

            if (hit->driver != (xEnt*)this)
            {
                continue;
            }

            if (!(this->boundFlags[hit->id] & 0x1))
            {
                continue;
            }

            xVec3Sub(&dir, (xVec3*)&globals.player.ent.model->Mat->pos, &this->frame->mat.pos);
            dir.y = 0.0f;
            xVec3Normalize(&dir, &dir);

            xVec3SMul((xVec3*)&globals.player.ent.model->Mat->at, &dir, -1.0f);
            xVec3Cross((xVec3*)&globals.player.ent.model->Mat->right,
                       (xVec3*)&globals.player.ent.model->Mat->up,
                       (xVec3*)&globals.player.ent.model->Mat->at);

            xVec3SMulBy(&dir, 10.0f);
            dir.y = 7.0f;

            zEntPlayer_Damage(NULL, 1, &dir);

            this->bossFlags |= 0x2;
        }
    }

    for (i = 0; i < 13; i++)
    {
        this->boundFlags[i] &= ~0x7;
    }

    this->bossFlags &= ~0x1;

    zNPCCommon::NewTime(xscn, dt);
}

static S32 idleCB(xGoal* rawgoal, void*, en_trantype* trantype, F32, void*)
{
    zNPCGoalBossSandyIdle* idle = (zNPCGoalBossSandyIdle*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)idle->psyche->clt_owner;
    S32 nextgoal = 0;
    xVec3 tempVector;

    if (sandy->bossFlags & 0x400)
    {
        return 0;
    }

    if (globals.player.ControlOff)
    {
        return 0;
    }

    if (sandy->hitPoints == 0)
    {
        return 0;
    }

    xVec3Sub(&tempVector, (xVec3*)&globals.player.ent.model->Mat->pos,
             (xVec3*)&sandy->model->Mat->pos);

    tempVector.y = 0.0f; // 0.0

    F32 length = xVec3Length2(&tempVector);

    if (idle->timeInGoal > 0.30000001192092896f) // 0.3
    {
        if (length > 12.0f) // 12.0
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB3';
        }
        else
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB4';
        }
    }

    return nextgoal;
}

static S32 tauntCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyTaunt* taunt = (zNPCGoalBossSandyTaunt*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)taunt->psyche->clt_owner;
    S32 nextgoal = 0;
    xVec3 tempVector;

    if (taunt->timeInGoal > 0.30000001192092896f) // 0.3
    {
        if (sandy->bossFlags & 0x400)
        {
            *trantype = GOAL_TRAN_SET;
            return 'NGB1';
        }
    }

    xVec3Sub(&tempVector, (xVec3*)&globals.player.ent.model->Mat->pos,
             (xVec3*)&sandy->model->Mat->pos);

    tempVector.y = 0.0f; // 0.0

    F32 length = xVec3Length2(&tempVector);

    if (sandy->AnimTimeRemain(NULL) < 1.100000023841858f * dt) // 1.1
    {
        if (globals.player.ControlOff)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB1';
        }
        else if (length > 12.0f) // 12.0
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB3';
        }
        else
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB4';
        }
    }

    return nextgoal;
}

static S32 chaseCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyChase* chase = (zNPCGoalBossSandyChase*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)chase->psyche->clt_owner;
    S32 nextgoal = 0;
    xVec3 tempVector;
    xVec3 pcFuturePos;
    F32 length;
    F32 futureDist;
    F32 atDot;
    F32 rightDot;
    F32 ringDistSq;

    if (chase->timeInGoal > 0.30000001192092896f)
    {
        if (sandy->bossFlags & 0x400)
        {
            *trantype = GOAL_TRAN_SET;
            return 'NGB1';
        }
    }

    xVec3Sub(&tempVector, (xVec3*)&globals.player.ent.model->Mat->pos,
             (xVec3*)&sandy->model->Mat->pos);

    tempVector.y = 0.0f;
    length = xVec3Length2(&tempVector);

    zEntPlayer_PredictPos(&pcFuturePos, 1.0f, 1.0f, 1);
    xVec3SubFrom(&pcFuturePos, (xVec3*)&sandy->model->Mat->pos);

    futureDist = xVec3Normalize(&pcFuturePos, &pcFuturePos);
    atDot = xVec3Dot(&pcFuturePos, (xVec3*)&sandy->model->Mat->at);
    rightDot = xVec3Dot(&pcFuturePos, (xVec3*)&sandy->model->Mat->right);

    ringDistSq = globals.player.ent.model->Mat->pos.x * globals.player.ent.model->Mat->pos.x +
                 globals.player.ent.model->Mat->pos.z * globals.player.ent.model->Mat->pos.z;

    if (futureDist > 3.5f && futureDist < 6.0f && atDot > 0.70700002f && rightDot < 0.0f)
    {
        sElbowDropThreshold += 1.0f;
    }

    if (globals.player.ControlOff)
    {
        *trantype = GOAL_TRAN_SET;
        nextgoal = 'NGB1';
    }
    else if (chase->timeInGoal > 0.30000001192092896f)
    {
        if (length < 12.0f)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB4';
        }
        else if (sChaseTimer > 2.5f)
        {
            if (sandy->round != 1 && sNumAttacks == 1)
            {
                *trantype = GOAL_TRAN_SET;
                nextgoal = 'NGB:';
            }
            else if (sNumAttacks == 0)
            {
                *trantype = GOAL_TRAN_SET;
                nextgoal = 'NGB6';
            }
            else if (sandy->round == 1 && sNumAttacks >= 1)
            {
                *trantype = GOAL_TRAN_SET;
                nextgoal = 'NGB7';
            }
            else if (sandy->round != 1 && sNumAttacks >= 2)
            {
                *trantype = GOAL_TRAN_SET;
                nextgoal = 'NGB7';
            }
        }
        else if (sandy->round == 1 && sNumAttacks == 1 && ringDistSq < 9.0f)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB7';
        }
    }

    return nextgoal;
}

static S32 meleeCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyMelee* melee = (zNPCGoalBossSandyMelee*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)melee->psyche->clt_owner;
    S32 nextgoal = 0;
    U32 idx;
    U32 numHints;
    xVec3 tempVector;
    F32 length;

    xVec3Sub(&tempVector, (xVec3*)&globals.player.ent.model->Mat->pos,
             (xVec3*)&sandy->model->Mat->pos);

    tempVector.y = 0.0f;
    length = xVec3Length2(&tempVector);

    if (sandy->AnimTimeRemain(NULL) < 1.7000000476837158f * dt)
    {
        if (globals.player.ControlOff)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB1';
        }
        else if (sandy->bossFlags & 0x2)
        {
            sandy->bossFlags &= ~0x2;
            nextgoal = 'NGB2';
            *trantype = GOAL_TRAN_SET;
        }
        else if (length > 12.0f)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB3';
        }
        else
        {
            melee->timeInGoal = 0.0f;
            xSndPlay3D(xStrHash("B101_SC_chop"), 0.77f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME,
                       0.6f);
        }
    }

    if (nextgoal == 'NGB2')
    {
        numHints = sandy->nfFlags & 0x3;

        if (numHints < 3 || (xrand() & 0x300) == 0)
        {
            idx = 2;
            if (numHints < 3)
            {
                idx = numHints;
            }

            if (sandy->round == 2)
            {
                sandy->newsfish->SpeakStart(sNFSoundValue[idx + 9], 0, 0xFFFFFFFF);
            }
            else
            {
                sandy->newsfish->SpeakStart(sNFSoundValue[idx], 0, 0xFFFFFFFF);
            }

            sandy->nfFlags &= ~0x3;
            sandy->nfFlags |= idx + 1;
        }
    }

    return nextgoal;
}

static S32 noHeadCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyNoHead* noHead = (zNPCGoalBossSandyNoHead*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)noHead->psyche->clt_owner;
    S32 nextgoal = 0;

    if (noHead->stage == 4 || noHead->stage == 5)
    {
        if (sandy->AnimTimeRemain(NULL) < 1.7000000476837158f * dt) // 1.7
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB1';
        }
    }

    return nextgoal;
}

static S32 elbowDropCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyElbowDrop* edrop = (zNPCGoalBossSandyElbowDrop*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)edrop->psyche->clt_owner;
    S32 nextgoal = 0;
    xVec3 tempVector;

    if (edrop->timeInGoal > 0.30000001192092896f) // 0.3
    {
        if (sandy->bossFlags & 0x400)
        {
            *trantype = GOAL_TRAN_SET;
            return 'NGB1';
        }
    }

    xVec3Sub(&tempVector, (xVec3*)&globals.player.ent.model->Mat->pos,
             (xVec3*)&sandy->model->Mat->pos);

    tempVector.y = 0.0f; // 0.0

    F32 length = xVec3Length2(&tempVector);

    if (sandy->AnimTimeRemain(NULL) < 1.7000000476837158f * dt) // 1.7
    {
        if (globals.player.ControlOff)
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB1';
        }
        else if (sandy->bossFlags & 2)
        {
            sandy->bossFlags &= ~2; // clear bit 2
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB2';
        }
        else if (length < 12.0f) // 12.0
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB4';
        }
        else
        {
            *trantype = GOAL_TRAN_SET;
            nextgoal = 'NGB3';
        }
    }

    return nextgoal;
}

static S32 leapCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyLeap* leap = (zNPCGoalBossSandyLeap*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)leap->psyche->clt_owner;
    S32 nextgoal = 0;

    if (leap->stage == 3)
    {
        if (sandy->AnimTimeRemain(NULL) < 1.7000000476837158f * dt) // 1.7
        {
            if (sandy->bossFlags & 2)
            {
                *trantype = GOAL_TRAN_SET;
                nextgoal = 'NGB9';
            }
            else
            {
                *trantype = GOAL_TRAN_SET;
                nextgoal = 'NGB8';
            }
        }
    }

    return nextgoal;
}

static S32 sitCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandySit* sit = (zNPCGoalBossSandySit*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)sit->psyche->clt_owner;
    S32 nextgoal = 0;

    if (sandy->round == 3 && sHeadPopOffFactor > 0.0f)
    {
        xSndPlay3D(xStrHash("B101_SC_headoff1"), 2.31f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME,
                   0.0f);
        *trantype = GOAL_TRAN_SET;
        nextgoal = 'NGB5';
    }
    else if (sit->sitFlags & 0x1)
    {
        if (sit->timeInGoal > sit->totalTime)
        {
            if (sandy->round == 1)
            {
                nextgoal = 'NGB9';
                *trantype = GOAL_TRAN_SET;
            }
            else
            {
                nextgoal = 'NGB5';
                *trantype = GOAL_TRAN_SET;
            }
        }
    }
    else if (sit->timeInGoal > 5.0f)
    {
        nextgoal = 'NGB9';
        *trantype = GOAL_TRAN_SET;
    }

    return nextgoal;
}

static S32 getUpCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyRunToRope* runGoal = (zNPCGoalBossSandyRunToRope*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)runGoal->psyche->clt_owner;
    S32 nextgoal = 0;
    xVec3 pcFuturePos;
    F32 futureDist;

    xVec3Sub(&pcFuturePos, (xVec3*)&globals.player.ent.model->Mat->pos,
             (xVec3*)&sandy->model->Mat->pos);

    pcFuturePos.y = 0.0f;
    futureDist = xVec3Length2(&pcFuturePos);

    if (sandy->AnimTimeRemain(NULL) < 1.7f * dt)
    {
        if ((sandy->round == 1 && !(sandy->hitPoints > 6)) ||
            (sandy->round == 2 && !(sandy->hitPoints > 3)) ||
            (sandy->round == 3 && !(sandy->hitPoints > 0)) || (globals.player.ControlOff))
        {
            nextgoal = 'NGB1';
            *trantype = GOAL_TRAN_SET;
        }
        else if (sandy->bossFlags & 0x2)
        {
            sandy->bossFlags &= ~0x2;
            nextgoal = 'NGB2';
            *trantype = GOAL_TRAN_SET;
        }
        else if (futureDist < 12.0f)
        {
            nextgoal = 'NGB4';
            *trantype = GOAL_TRAN_SET;
        }
        else
        {
            nextgoal = 'NGB3';
            *trantype = GOAL_TRAN_SET;
        }
    }

    return nextgoal;
}

static S32 runToRopeCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyRunToRope* runGoal = (zNPCGoalBossSandyRunToRope*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)runGoal->psyche->clt_owner;
    S32 nextgoal = 0;
    F32 projection;
    xVec3 newPos;

    xVec3Sub(&newPos, &sandy->bouncePoint[sandy->fromRope], (xVec3*)&sandy->model->Mat->pos);

    newPos.y = 0.0f;
    projection = xVec3Dot(&newPos, &sandy->ropeNormal[sandy->fromRope]);

    if (globals.player.ControlOff != FALSE)
    {
        nextgoal = 'NGB1';
        *trantype = GOAL_TRAN_SET;
    }
    else if (projection > 0.0f)
    {
        nextgoal = 'NGB;';
        *trantype = GOAL_TRAN_SET;
        sandy->boundFlags[10] &= ~0x10;
        sandy->boundFlags[12] &= ~0x10;
    }

    return nextgoal;
}

static S32 clotheslineCB(xGoal* rawgoal, void*, en_trantype* trantype, F32 dt, void*)
{
    zNPCGoalBossSandyClothesline* cl = (zNPCGoalBossSandyClothesline*)rawgoal;
    zNPCBSandy* sandy = (zNPCBSandy*)cl->psyche->clt_owner;
    S32 nextgoal = 0;

    if (cl->stage == 2 && sandy->AnimTimeRemain(NULL) < 1.7f * dt)
    {
        sandy->boundFlags[10] &= ~0x10;
        sandy->boundFlags[12] &= ~0x10;

        if (globals.player.ControlOff)
        {
            nextgoal = 'NGB1';
            *trantype = GOAL_TRAN_SET;
        }
        else if (sandy->bossFlags & 0x2)
        {
            sandy->bossFlags &= ~0x2;
            nextgoal = 'NGB2';
            *trantype = GOAL_TRAN_SET;
        }
        else
        {
            nextgoal = 'NGB3';
            *trantype = GOAL_TRAN_SET;
        }
    }

    return nextgoal;
}

S32 zNPCGoalBossSandyIdle::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    timeInGoal = 0.0f;
    sandy->bossFlags |= 0x20;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);

    sandy->boundFlags[10] |= 0x10;
    sandy->boundFlags[12] |= 0x10;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyIdle::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    timeInGoal += dt;

    xVec3 newAt;
    xVec3Sub(&newAt, (xVec3*)&globals.player.ent.model->Mat->pos, (xVec3*)&sandy->model->Mat->pos);

    newAt.y = 0.0f;

    xVec3Normalize(&newAt, &newAt);
    xVec3SMul((xVec3*)&sandy->frame->mat.at, (xVec3*)&sandy->model->Mat->at, 0.98f);

    xVec3AddScaled((xVec3*)&sandy->frame->mat.at, &newAt, 0.02f);

    sandy->frame->mat.at.y = 0.0f;
    xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
    xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up, &sandy->frame->mat.at);

    sandy->frame->mat.pos.y = 0.0f;

    F32 lerp = 1.0f - xVec3Dot(&newAt, (xVec3*)&sandy->model->Mat->right);
    sandy->model->Anim->Single->BilinearLerp[0] = lerp;
    sandy->model->Anim->Single->Blend->BilinearLerp[0] = lerp;

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandyIdle::Exit(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    sandy->bossFlags &= ~0x20;
    sandy->boundFlags[10] &= ~0x10;
    sandy->boundFlags[12] &= ~0x10;

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSandyTaunt::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    timeInGoal = 0.0f;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);
    xSndPlay3D(xStrHash("B101_SC_taunt"), 0.77f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME, 0.6f);

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyTaunt::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    timeInGoal += dt;

    xVec3 newAt;
    xVec3Sub(&newAt, (xVec3*)&globals.player.ent.model->Mat->pos, (xVec3*)&sandy->model->Mat->pos);

    newAt.y = 0.0f;

    xVec3Normalize(&newAt, &newAt);
    xVec3SMul((xVec3*)&sandy->frame->mat.at, (xVec3*)&sandy->model->Mat->at, 0.9f);

    xVec3AddScaled((xVec3*)&sandy->frame->mat.at, &newAt, 0.1f);

    sandy->frame->mat.at.y = 0.0f;
    xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
    xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up, &sandy->frame->mat.at);

    sandy->frame->mat.pos.y = 0.0f;

    return zNPCGoalCommon::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandyChase::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    sandy->bossFlags |= 0x20;
    timeInGoal = 0.0f;

    sandy->boundFlags[10] |= 0x10;
    sandy->boundFlags[12] |= 0x10;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyChase::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    sChaseTimer += dt;
    timeInGoal += dt;

    xVec3 newAt;
    xVec3Sub(&newAt, (xVec3*)&globals.player.ent.model->Mat->pos, (xVec3*)&sandy->model->Mat->pos);

    newAt.y = 0.0f;

    xVec3Normalize(&newAt, &newAt);
    xVec3SMul((xVec3*)&sandy->frame->mat.at, (xVec3*)&sandy->model->Mat->at, 0.9f);

    xVec3AddScaled((xVec3*)&sandy->frame->mat.at, &newAt, 0.1f);

    sandy->frame->mat.at.y = 0.0f;
    xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
    xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up, &sandy->frame->mat.at);

    sandy->frame->mat.pos.y = 0.0f;
    xVec3SMul(&sandy->frame->vel, &sandy->frame->mat.at, sandy->cfg_npc->spd_moveMax);

    return zNPCGoalCommon::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandyChase::Exit(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    sandy->bossFlags &= ~0x20;
    sandy->boundFlags[10] &= ~0x10;
    sandy->boundFlags[12] &= ~0x10;

    return zNPCGoalCommon::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSandyMelee::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    timeInGoal = 0.0f;
    sandy->bossFlags &= ~0x2;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);

    sandy->boundList[2]->penby = 0x0;
    sandy->boundList[3]->penby = 0x0;
    sandy->boundList[4]->penby = 0x0;
    sandy->boundList[5]->penby = 0x0;
    sandy->boundList[6]->penby = 0x0;
    sandy->boundList[7]->penby = 0x0;
    sandy->boundList[8]->penby = 0x0;

    xSndPlay3D(xStrHash("B101_SC_chop"), 0.77f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME, 0.6f);

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyMelee::Exit(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    sandy->boundList[2]->penby = 0x10;
    sandy->boundList[3]->penby = 0x10;
    sandy->boundList[4]->penby = 0x10;
    sandy->boundList[5]->penby = 0x10;
    sandy->boundList[6]->penby = 0x10;
    sandy->boundList[7]->penby = 0x10;
    sandy->boundList[8]->penby = 0x10;

    return zNPCGoalCommon::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSandyMelee::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    timeInGoal += dt;

    if (timeInGoal > 0.1f && timeInGoal < 0.75f)
    {
        xVec3 newAt;
        xVec3Sub(&newAt, (xVec3*)&globals.player.ent.model->Mat->pos,
                 (xVec3*)&sandy->model->Mat->pos);

        newAt.y = 0.0f;
        xVec3Normalize(&newAt, &newAt);
        xVec3SMul((xVec3*)&sandy->frame->mat.at, (xVec3*)&sandy->model->Mat->at, 0.9f);

        xVec3AddScaled((xVec3*)&sandy->frame->mat.at, &newAt, 0.1f);

        sandy->frame->mat.at.y = 0.0f;
        xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
        xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up, &sandy->frame->mat.at);
    }

    sandy->boundFlags[2] |= 0x1;
    sandy->boundFlags[3] |= 0x1;
    sandy->boundFlags[4] |= 0x1;
    sandy->boundFlags[5] |= 0x1;
    sandy->boundFlags[6] |= 0x1;
    sandy->boundFlags[7] |= 0x1;
    sandy->boundFlags[8] |= 0x1;

    sandy->bossFlags |= 0x1;

    return zNPCGoalCommon::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandyNoHead::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    sandy->bossFlags &= ~0x100;
    timeInGoal = 0.0f;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);

    if (sandy->round == 2)
    {
        sandy->headBoulder->collis_chk = 0x26;
        sandy->headBoulder->collis_pen = 0x0;
    }

    stage = 0;
    secsSincePatWasCarryingHead = 1.0f;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyNoHead::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    U32 numHints;
    U32 hintsGiven;
    xVec3 newAt;
    F32 lerpFactor;
    xMat4x3 boneMat;
    xQuat q;

    timeInGoal += dt;
    if (globals.player.carry.grabbed == sandy->headBoulder)
    {
        secsSincePatWasCarryingHead = 0.0f;
    }
    else
    {
        secsSincePatWasCarryingHead += dt;
    }

    if (stage == 0)
    {
        if (sandy->AnimTimeRemain(NULL) < 1.7f * dt)
        {
            stage = 1;
            DoAutoAnim(NPC_GSPOT_START, FALSE);
            timeInGoal = 0.0f;

            if (sandy->round == 3)
            {
                sandy->boundFlags[10] |= 0x10;
                sandy->boundFlags[12] |= 0x10;
            }

            if ((sandy->nfFlags & 0x4) == 0 && (sandy->round == 2 || sandy->round == 3))
            {
                hintsGiven = (sandy->nfFlags >> 3) & 3;

                if (hintsGiven < 3 || (xrand() & 0x300) == 0)
                {
                    numHints = 2;
                    if (hintsGiven < 3)
                    {
                        numHints = hintsGiven;
                    }

                    if (sandy->round == 2)
                    {
                        sandy->newsfish->SpeakStart(sNFSoundValue[numHints + 12], 0, -1);
                    }
                    else
                    {
                        sandy->newsfish->SpeakStart(sNFSoundValue[numHints + 21], 0, -1);
                    }

                    sandy->nfFlags &= ~0x18;
                    sandy->nfFlags |= (numHints + 1) * 8;
                }
            }
        }

        if (sandy->round == 2)
        {
            if (secsSincePatWasCarryingHead < 0.5f)
            {
                sCamSubTarget = &sandy->ringEdgeCenter[4];
            }
            else
            {
                sCamSubTarget = &sandy->headBoulder->bound.sph.center;
            }
        }
    }
    else if (stage == 1)
    {
        if (sandy->round == 2)
        {
            if (sandy->bossFlags & 0x100)
            {
                sandy->headBoulder->collis_chk = 0x0;
                sandy->headBoulder->collis_pen = 0x0;

                xVec3Copy(&sandy->shockPos, (xVec3*)&sandy->headBoulder->model->Mat->pos);

                sandy->headBoulder->angVel = 0.0f;
                stage = 2;
                DoAutoAnim(NPC_GSPOT_START, FALSE);
                timeInGoal = 0.0f;

                numHints = 6 - sandy->hitPoints;
                sandy->newsfish->SpeakStart(sNFSoundValue[numHints + 15], 0, -1);

                sandy->nfFlags |= 0x4;
                sandy->hitPoints--;

                xSndPlay3D(xStrHash("B101_SC_damage3"), 1.155f, 0.0f, 0x0, 0x0, sandy, 80.0f,
                           SND_CAT_GAME, 0.0f);
                zEntEvent(sandy, sandy, eEventNPCHPDecremented);

                sandy->maxLightningWait[1] = 0.1f;

                zNPCBSandy_BossDamageEffect(sandy->model, 0);
                zNPCBSandy_BossDamageEffect(sandy->headBoulder->model, 0);

                sandy->sboardThirdShrap->initCB(sandy->sboardThirdShrap,
                                                sandy->crashedScoreboard->model, NULL, NULL);
            }
            else if ((timeInGoal > 15.0f && sandy->AnimTimeRemain(NULL) < 1.7f * dt) ||
                     globals.player.carry.flyingToTarget == sandy)
            {
                stage = 3;
                sandy->CalcMagnetizeInfo();
                sUseBossCam = TRUE;
                sCamSubTarget = (xVec3*)&sandy->model->Mat->pos;
                DoAutoAnim(NPC_GSPOT_START, FALSE);
                timeInGoal = 0.0f;

                if (globals.player.carry.grabbed == sandy->headBoulder)
                {
                    globals.player.carry.grabbed = NULL;
                }

                if (globals.player.carry.flyingToTarget == sandy)
                {
                    zThrown_Remove(sandy->headBoulder);
                    globals.player.carry.flyingToTarget = NULL;
                }
            }
            else
            {
                xVec3Sub(&newAt, &sandy->headBoulder->bound.sph.center,
                         (xVec3*)&sandy->model->Mat->pos);

                newAt.y = 0.0f;

                xVec3Normalize(&newAt, &newAt);
                xVec3SMul((xVec3*)&sandy->frame->mat.at, (xVec3*)&sandy->model->Mat->at, 0.9f);
                xVec3AddScaled((xVec3*)&sandy->frame->mat.at, &newAt, 0.1f);

                sandy->frame->mat.at.y = 0.0f;
                xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
                xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up,
                           &sandy->frame->mat.at);

                sandy->frame->mat.pos.y = 0.0f;

                if (secsSincePatWasCarryingHead < 0.5f)
                {
                    sCamSubTarget = &sandy->ringEdgeCenter[4];
                }
                else
                {
                    sCamSubTarget = &sandy->headBoulder->bound.sph.center;
                }
            }
        }
        else if (timeInGoal > 15.0f)
        {
            sandy->boundFlags[10] &= ~0x10;
            sandy->boundFlags[12] &= ~0x10;
            stage = 4;

            xSndPlay3D(xStrHash("B101_SC_headback"), 1.155f, 0.0f, 0x0, 0x0, sandy, 30.0f,
                       SND_CAT_GAME, 0.8f);
            DoAutoAnim(NPC_GSPOT_START, FALSE);
            timeInGoal = 0.0f;
            sandy->bossFlags &= ~0x80;
        }
        else if (sandy->boundFlags[1] & 0x4)
        {
            stage = 5;

            numHints = 3 - sandy->hitPoints;
            sandy->newsfish->SpeakStart(sNFSoundValue[numHints + 24], 0, -1);

            sandy->nfFlags |= 0x4;
            sandy->hitPoints--;

            xSndPlay3D(xStrHash("B101_SC_damage3"), 1.155f, 0.0f, 0x0, 0x0, sandy, 80.0f,
                       SND_CAT_GAME, 0.0f);
            zEntEvent(sandy, sandy, eEventNPCHPDecremented);

            if (sandy->hitPoints == 0)
            {
                zEntEvent(sandy, eEventDeath);
            }

            zNPCBSandy_BossDamageEffect(sandy->model, 0);
            DoAutoAnim(NPC_GSPOT_START, FALSE);
            timeInGoal = 0.0f;
            sandy->bossFlags &= ~0x80;
        }
        else
        {
            sandy->bossFlags |= 0x80;
            sandy->targetRaster = sandy->helmetRaster;

            GetBonePos(&sandy->targetPos, (xMat4x3*)sandy->model->Mat, sBone[1], NULL);

            sandy->targetPos.y = 0.1f;
        }
    }
    else if (stage == 2)
    {
        if (timeInGoal > 3.0f)
        {
            stage = 3;
            sUseBossCam = TRUE;
            sCamSubTarget = (xVec3*)&sandy->model->Mat->pos;
            sandy->CalcMagnetizeInfo();
            DoAutoAnim(NPC_GSPOT_START, FALSE);
            timeInGoal = 0.0f;
            sandy->bossFlags &= ~0x100;
        }
        else
        {
            if (timeInGoal > 1.5f)
            {
                sCamSubTarget = (xVec3*)&sandy->model->Mat->pos;
            }

            xVec3Copy((xVec3*)&sandy->headBoulder->model->Mat->pos, &sandy->shockPos);

            sandy->headBoulder->model->Mat->pos.x += 0.1f * icos(31.0f * timeInGoal);
            sandy->headBoulder->model->Mat->pos.z += 0.1f * isin(42.0f * timeInGoal);

            sandy->headBoulder->angVel = 0.0f;
            xVec3Init(&sandy->headBoulder->vel, 0.0f, 0.0f, 0.0f);
        }
    }
    else if (stage == 3)
    {
        lerpFactor = timeInGoal / sandy->magnetizeTime;

        if (lerpFactor > 1.0f)
        {
            stage = 4;

            xSndPlay3D(xStrHash("B101_SC_headback"), 1.155f, 0.0f, 0x0, 0x0, sandy, 30.0f,
                       SND_CAT_GAME, 0.8f);
            DoAutoAnim(NPC_GSPOT_START, FALSE);
            timeInGoal = 0.0f;

            zEntEvent(sandy->headBoulder, eEventKill);

            sandy->model->Flags |= 0x1;
            sandy->model->Next->Flags |= 0x1;
            sandy->boundList[1]->chkby = 0x10;
            sandy->boundList[1]->penby = 0x10;
        }
        else
        {
            xMat4x3Mul(&boneMat, (xMat4x3*)&sandy->model->Mat[sBone[1]],
                       (xMat4x3*)sandy->model->Mat);
            xQuatFromMat(&sandy->qHead, &boneMat);
            xMat3x3RMulVec(&sandy->pHead, &boneMat, &sBoneOffset[1]);
            xVec3AddTo(&sandy->pHead, &boneMat.pos);

            xQuatSlerp(&q, &sandy->qBoulder, &sandy->qHead, lerpFactor);
            xQuatToMat(&q, (xMat3x3*)sandy->headBoulder->model->Mat);

            xVec3SMul((xVec3*)&sandy->headBoulder->model->Mat->pos, &sandy->pBoulder,
                      1.0f - lerpFactor);
            xVec3AddScaled((xVec3*)&sandy->headBoulder->model->Mat->pos, &sandy->pHead,
                           lerpFactor);
        }
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandyElbowDrop::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    timeInGoal = 0;
    sandy->bossFlags &= 0xfffffffd;
    sNumAttacks++;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);

    xSndPlay3D(xStrHash("B101_SC_drop"), 0.77f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME, 0.65f);
    xSndPlay3D(xStrHash("B101_SC_drop"), 0.77f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME, 1.3f);
    xSndPlay3D(xStrHash("B101_SC_hitring"), 0.77f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME,
               2.25f);
    elbowFlags = 0;
    return zNPCGoalCommon::Enter(dt, updCtxt);
}

// This function is certified not cool
S32 zNPCGoalBossSandyElbowDrop::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    xVec3 newAt;

    timeInGoal += dt;

    if (timeInGoal > 0.75f)
    {
        for (S32 i = 0; i < 13; i++)
        {
            sandy->boundFlags[i] |= 0x1;
        }

        sandy->bossFlags |= 0x1;
    }

    if (timeInGoal > sandy->edropShockwaveTime)
    {
        if (!(elbowFlags & 0x1))
        {
            elbowFlags |= 0x1;
            sandy->bossFlags |= 0x800;

            xVec3Copy((xVec3*)&sandy->shockwaveEmitter->tasset->pos,
                      (xVec3*)&sandy->model->Mat->pos);

            sandy->shockwaveEmitter->tasset->pos.y = 0.0f;
            sandy->shockRadius = 1.0f;
        }
    }
    else if (timeInGoal > sandy->edropTurnMinTime)
    {
        xVec3Sub(&newAt, (xVec3*)&globals.player.ent.model->Mat->pos,
                 (xVec3*)&sandy->model->Mat->pos);

        newAt.y = 0.0f;

        xVec3Normalize(&newAt, &newAt);
        xVec3SMul((xVec3*)&sandy->frame->mat.at, (xVec3*)&sandy->model->Mat->at, 0.9f);
        xVec3AddScaled((xVec3*)&sandy->frame->mat.at, &newAt, 0.1f);

        sandy->frame->mat.at.y = 0.0f;
        xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
        xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up, &sandy->frame->mat.at);

        sandy->frame->mat.pos.y = 0.0f;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandyElbowDrop::Exit(F32 dt, void* updCtxt)
{
    sElbowDropTimer = 0.0f;
    sElbowDropThreshold = 0.0f;
    sChaseTimer = 0.0f;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSandyLeap::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    xVec3 toRing;
    F32 dist;
    F32 mag;

    timeInGoal = 0.0f;
    stage = 0;

    sandy->bossFlags &= ~0x2;
    sandy->bossFlags &= ~0x4;

    endX = globals.player.ent.model->Mat->pos.x;
    endZ = globals.player.ent.model->Mat->pos.z;

    if (sandy->round == 2)
    {
        toRing.x = endX - sandy->ringEdgeCenter[4].x;
        toRing.y = 0.0f;
        toRing.z = endZ - sandy->ringEdgeCenter[4].z;

        dist = xVec3Length2(&toRing);
        if (dist < 100.0f)
        {
            xVec3SMulBy(&toRing, 10.0f / xsqrt(dist));
            endX = toRing.x + sandy->ringEdgeCenter[4].x;
            endZ = toRing.z + sandy->ringEdgeCenter[4].z;
        }
    }

    if (endX < 3.0f && endX > -3.0f && endZ < 3.0f && endZ > -3.0f)
    {
        endX = 0.0f;
        endZ = 0.0f;
    }

    mag = endX * endX + endZ * endZ;
    if (mag > 100.0f)
    {
        mag = 1.0f / xsqrt(mag);
        endX = endX * (10.0f * mag);
        endZ = endZ * (10.0f * mag);
    }

    startX = sandy->model->Mat->pos.x - endX;
    startZ = sandy->model->Mat->pos.z - endZ;

    sNumAttacks = 0;
    sDidClothesline = 0;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyLeap::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    S32 i;
    S32 hurtAll = 0;
    S32 animIdx = -1;
    U32 animID;
    F32 timeLeft;
    F32 recip;
    zJumpParam jump;

    timeInGoal += dt;

    timeLeft = sandy->AnimTimeRemain(NULL);
    recip = 1.0f / sandy->model->Anim->Single->State->Data->Duration;

    switch (stage)
    {
    case 0:
        if (timeLeft < 1.7000000476837158f * dt)
        {
            stage = 1;
            xSndPlay3D(xStrHash("B101_SC_jump"), 0.77f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME,
                       0.0f);
            animIdx = 13;
            timeInGoal = 0.0f;
        }
        break;
    case 1:
        if (timeLeft < 1.7000000476837158f * dt)
        {
            stage = 2;
            animIdx = 14;
            timeInGoal = 0.0f;
        }
        else
        {
            F32 accel = (7.0f * recip) * recip;
            sandy->frame->mat.pos.y = 7.0f - timeLeft * (accel * timeLeft);
            sandy->frame->mat.pos.x = startX * (timeLeft * recip) + endX;
            sandy->frame->mat.pos.z = startZ * (timeLeft * recip) + endZ;
        }
        break;
    case 2:
        hurtAll = 1;
        if (timeLeft < 1.7000000476837158f * dt)
        {
            stage = 3;
            sandy->bossFlags |= 0x800;
            xVec3Copy((xVec3*)&sandy->shockwaveEmitter->tasset->pos,
                      (xVec3*)&sandy->model->Mat->pos);
            sandy->shockwaveEmitter->tasset->pos.y = 0.0f;
            sandy->shockRadius = 1.0f;
            animIdx = 15;
            timeInGoal = 0.0f;
            sandy->frame->mat.pos.y = 0.0f;
            xSndPlay(xStrHash("B101_SC_butt"), 1.0f, 0.0f, 0x0, 0x0, 0x0, SND_CAT_GAME, 0.0f);
        }
        else
        {
            F32 accel = (7.0f * recip) * recip;
            sandy->frame->mat.pos.y = 7.0f - timeInGoal * (accel * timeInGoal);
        }
        break;
    case 3:
        if (timeInGoal < 0.2f)
        {
            hurtAll = 1;
            sandy->bossFlags |= 0x40;

            if (!(sandy->bossFlags & 0x2) && !(sandy->bossFlags & 0x4) &&
                globals.player.JumpState == 0)
            {
                sandy->bossFlags |= 0x4;

                jump.PeakHeight = 5.25f;
                jump.TimeHold = 0.0f;
                jump.TimeGravChange = 0.3f;

                CalcJumpImpulse(&jump, NULL);
                zEntPlayerJumpStart(&globals.player.ent, &jump);
                zEntPlayer_SNDPlay(ePlayerSnd_Jump, 0.0f);

                globals.player.Jump_CanDouble = 0;
                globals.player.Jump_CanFloat = 1;
                globals.player.Jump_Springboard = NULL;
                globals.player.Jump_SpringboardStart = 0;
                globals.player.Bounced = 1;
            }
        }
        break;
    }

    if (hurtAll)
    {
        for (i = 0; i < 13; i++)
        {
            sandy->boundFlags[i] |= 0x1;
        }

        sandy->bossFlags |= 0x1;
    }

    if (animIdx > -1)
    {
        animID = g_hash_bossanim[animIdx];
        if (animID)
        {
            if (sandy->AnimStart(animID, 0))
            {
                anid_played = animID;
            }
            else
            {
                Name();
                anid_played = 0;
            }
        }
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandyLeap::Exit(F32 dt, void* updCtxt)
{
    sChaseTimer = 0.0f;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSandySit::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    U32 idx;
    U32 numHints;

    timeInGoal = 0.0f;
    sitFlags = 0;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);

    if ((sandy->nfFlags & 0x4) == 0 && sandy->round == 1)
    {
        numHints = (sandy->nfFlags >> 3) & 3;

        if (numHints < 3 || (xrand() & 0x300) == 0)
        {
            idx = 2;
            if (numHints < 3)
            {
                idx = numHints;
            }

            sandy->newsfish->SpeakStart(sNFSoundValue[idx + 3], 0, 0xFFFFFFFF);

            sandy->nfFlags &= ~0x18;
            sandy->nfFlags |= (idx + 1) * 8;
        }
    }

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandySit::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    RwMatrix* bmat;
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    S32 inRing;
    S32 numSpins;
    S32 numHints;
    F32 popFactor;
    xVec3 boulderCenter;
    xVec3 shrapVel;

    timeInGoal += dt;

    if (sandy->headBoulder == globals.player.carry.grabbed)
    {
        xVec3Init(&sandy->headBoulder->vel, 0.0f, 0.0f, 0.0f);
    }

    if ((sitFlags & 0x1) == 0 && globals.player.ent.model->Mat->pos.y >= 0.25f)
    {
        sandy->bossFlags |= 0x80;
        sandy->targetRaster = sandy->feetRaster;
        sandy->targetPos.x = globals.player.ent.model->Mat->pos.x;
        sandy->targetPos.y = 0.1f;
        sandy->targetPos.z = globals.player.ent.model->Mat->pos.z;
    }
    else
    {
        sandy->bossFlags &= ~0x80;
    }

    if (sHeadPopOffFactor > 0.0f && (sitFlags & 0x1) == 0 && sandy->round != 3)
    {
        sitFlags |= 0x1;

        xSndPlay3D(xStrHash("B101_SC_headoff1"), 2.31f, 0.0f, 0x0, 0x0, sandy, 30.0f, SND_CAT_GAME,
                   0.0f);

        timeInGoal = 0.0f;
        zEntEvent(sandy->headBoulder, eEventReset);

        sandy->headBoulder->moreFlags |= 0x8;
        sandy->headBoulder->collis_chk = 0x20;
        sandy->headBoulder->collis_pen = 0x20;

        xMat4x3Mul((xMat4x3*)sandy->headBoulder->model->Mat,
                   (xMat4x3*)&sandy->model->Mat[sBone[1]], (xMat4x3*)sandy->model->Mat);
        xMat3x3RMulVec(&boulderCenter, (xMat3x3*)sandy->headBoulder->model->Mat,
                       &sandy->headBoulder->localCenter);
        xVec3Copy(&sandy->headBoulder->bound.box.center, &sandy->boundList[1]->bound.box.center);
        xVec3Sub((xVec3*)&sandy->headBoulder->model->Mat->pos,
                 &sandy->headBoulder->bound.box.center, &boulderCenter);

        sandy->model->Flags &= ~0x1;
        sandy->model->Next->Flags &= ~0x1;

        sandy->boundList[1]->chkby = 0;
        sandy->boundList[1]->penby = 0;

        if (sandy->round == 1 && sHeadPopOffFactor >= 1.0f)
        {
            popFactor = 10.0f;
        }
        else
        {
            popFactor = 3.0f * sHeadPopOffFactor;
            if (popFactor < 1.0f)
            {
                popFactor = 1.0f;
            }
        }

        if (sandy->round == 1)
        {
            xVec3Init(&sandy->headBoulder->vel, 0.0f,
                      xsqrt(2.0f * popFactor * sandy->headBoulder->basset->gravity), 0.0f);

            totalTime = 2.0f * sandy->headBoulder->vel.y / sandy->headBoulder->basset->gravity;

            sandy->headBoulder->rotVec.x = xurand();
            sandy->headBoulder->rotVec.y = 0.0f;
            sandy->headBoulder->rotVec.z =
                xsqrt(1.0f - sandy->headBoulder->rotVec.x * sandy->headBoulder->rotVec.x);

            if (sHeadPopOffFactor >= 1.0f)
            {
                numSpins = 3;
            }
            else if (sHeadPopOffFactor > 0.7f)
            {
                numSpins = 2;
            }
            else
            {
                numSpins = 1;
            }

            sandy->headBoulder->angVel = 6.2831855f * numSpins / totalTime;
        }
        else if (sandy->round == 2)
        {
            sCamSubTarget = &sandy->headBoulder->bound.box.center;

            xVec3SMul(&sandy->headBoulder->vel, &sandy->headBoulder->bound.box.center, -1.0f);

            if (xVec3Normalize(&sandy->headBoulder->vel, &sandy->headBoulder->vel) < 0.001f)
            {
                xVec3Init(&sandy->headBoulder->vel, 1.0f, 2.0f, 0.0f);
            }
            else
            {
                sandy->headBoulder->vel.y = 2.0f;
            }

            xVec3SMulBy(&sandy->headBoulder->vel, 5.0f);

            sandy->headBoulder->rotVec.x = xurand();
            sandy->headBoulder->rotVec.y = 0.0f;
            sandy->headBoulder->rotVec.z =
                xsqrt(1.0f - sandy->headBoulder->rotVec.x * sandy->headBoulder->rotVec.x);

            sandy->headBoulder->angVel = 4.712389f;
            totalTime = 0.5f;
        }

        sHeadPopOffFactor = -1.0f;
    }
    else if (sandy->round == 1 && (sitFlags & 0x1) && !(sitFlags & 0x2) &&
             sandy->headBoulder->model->Mat->pos.y > 10.0f)
    {
        sitFlags |= 0x2;

        numHints = 9 - sandy->hitPoints;
        sandy->newsfish->SpeakStart(sNFSoundValue[numHints + 6], 0, 0xFFFFFFFF);

        sandy->nfFlags |= 0x4;
        sandy->hitPoints = sandy->hitPoints - 1;

        xSndPlay3D(xStrHash("B101_SC_damage3"), 1.155f, 0.0f, 0x0, 0x0, sandy, 80.0f, SND_CAT_GAME,
                   0.0f);
        zEntEvent(sandy, sandy, eEventNPCHPDecremented);

        bmat = sandy->headBoulder->model->Mat;

        inRing =
            bmat->pos.x > -3.0f && bmat->pos.x < 3.0f && bmat->pos.z > -3.0f && bmat->pos.z < 3.0f;

        if (inRing || (sandy->hitPoints == 6 && (sandy->hangingScoreboard->flags & 0x1)))
        {
            sandy->maxLightningWait[0] = 0.1f;

            if (sandy->hangingScoreboard->flags & 0x1)
            {
                sandy->bustedScoreboard->flags |= 0x1;
                sandy->hangingScoreboard->flags &= ~0x1;

                xVec3Init(&shrapVel, 0.0f, -2.5f, 0.0f);

                sandy->scoreboardShrap->initCB(sandy->scoreboardShrap,
                                               sandy->hangingScoreboard->model, &shrapVel, NULL);
            }
            else
            {
                sandy->sboardSecondShrap->initCB(sandy->sboardSecondShrap,
                                                 sandy->bustedScoreboard->model, NULL, NULL);
            }
        }

        if (!inRing)
        {
            if (bmat->pos.x > bmat->pos.z)
            {
                if (bmat->pos.x < -bmat->pos.z)
                {
                    sandy->lightRigShrap->initCB(sandy->lightRigShrap, sandy->lightRig[2]->model,
                                                 NULL, NULL);
                }
                else
                {
                    sandy->lightRigShrap->initCB(sandy->lightRigShrap, sandy->lightRig[1]->model,
                                                 NULL, NULL);
                }
            }
            else
            {
                if (bmat->pos.x < -bmat->pos.z)
                {
                    sandy->lightRigShrap->initCB(sandy->lightRigShrap, sandy->lightRig[0]->model,
                                                 NULL, NULL);
                }
                else
                {
                    sandy->lightRigShrap->initCB(sandy->lightRigShrap, sandy->lightRig[3]->model,
                                                 NULL, NULL);
                }
            }
        }

        zNPCBSandy_BossDamageEffect(sandy->model, 0);
        zNPCBSandy_BossDamageEffect(sandy->headBoulder->model, 0);

        DoAutoAnim(NPC_GSPOT_START, FALSE);
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandySit::Exit(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    sandy->bossFlags &= ~0x80;

    if (sandy->bossFlags & 0x40)
    {
        sCurrHeight = sCurrHeight - globals.player.ent.model->Mat->pos.y;
        sandy->bossFlags &= ~0x40;
    }

    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSandyGetUp::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;

    timeInGoal = 0.0f;
    sandy->bossFlags &= ~0x80;

    zEntEvent(sandy->headBoulder, eEventKill);

    if (globals.player.carry.grabbed == sandy->headBoulder)
    {
        globals.player.carry.grabbed = NULL;
    }

    sandy->model->Flags |= 0x1;
    sandy->model->Next->Flags |= 0x1;

    sandy->boundList[1]->chkby = 0x10;
    sandy->boundList[1]->penby = 0x10;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);

    if (sandy->bossFlags & 0x40)
    {
        sCurrHeight = sCurrHeight - globals.player.ent.model->Mat->pos.y;
        sandy->bossFlags &= ~0x40;
    }

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyGetUp::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* scene)
{
    timeInGoal += dt;
    return xGoal::Process(trantype, dt, updCtxt, scene);
}

S32 zNPCGoalBossSandyRunToRope::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    S32 secondRope;
    xVec3 heading;
    F32 side;
    F32 bestDot;
    F32 secondDot;
    F32 bestDist;
    F32 secondDist;
    S32 i;
    F32 dist;
    xVec3 toBounce;

    timeInGoal = 0.0f;

    secondRope = 1;
    sandy->fromRope = 1;
    sandy->toRope = 5;

    bestDot = 30.0f;
    secondDot = bestDot;
    bestDist = 0.0f;
    secondDist = bestDist;

    xVec3Sub(&heading, (xVec3*)&sandy->model->Mat->pos,
             (xVec3*)&globals.player.ent.model->Mat->pos);
    xVec3Normalize(&heading, &heading);

    for (i = 0; i < 8; i++)
    {
        if (i == 4)
        {
            continue;
        }

        if (i == 0)
        {
            continue;
        }

        xVec3Sub(&toBounce, &sandy->bouncePoint[i], (xVec3*)&sandy->model->Mat->pos);

        if (xVec3Dot(&toBounce, &sandy->ropeNormal[i]) > 0.0f)
        {
            continue;
        }

        dist = xVec3Length(&toBounce);
        if (dist < 1e-05f)
        {
            continue;
        }

        side = xVec3Dot(&heading, &toBounce) / dist;

        if (side < bestDot && side > -bestDot)
        {
            secondRope = sandy->fromRope;
            secondDot = bestDot;
            sandy->fromRope = i;

            if (side > 0.0f)
            {
                bestDot = side;
            }
            else
            {
                bestDot = -side;
            }

            secondDist = bestDist;
            bestDist = dist;
        }
        else if (side < secondDot && side > -secondDot)
        {
            secondRope = i;

            if (side > 0.0f)
            {
                secondDot = side;
            }
            else
            {
                secondDot = -side;
            }

            secondDist = dist;
        }
    }

    if (secondDist < bestDist)
    {
        sandy->fromRope = secondRope;
    }

    sandy->toRope = sandy->fromRope + 4;
    if (sandy->toRope >= 8)
    {
        sandy->toRope -= 8;
    }

    sandy->boundFlags[10] |= 0x10;
    sandy->boundFlags[12] |= 0x10;

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyRunToRope::Process(en_trantype* trantype, F32 dt, void* updCtxt, xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    xVec3 newAt;

    timeInGoal += dt;

    xVec3Sub(&newAt, &sandy->bouncePoint[sandy->fromRope], (xVec3*)&sandy->model->Mat->pos);

    newAt.y = 0.0f;
    xVec3Normalize(&newAt, &newAt);

    xVec3SMul(&sandy->frame->mat.at, (xVec3*)&sandy->model->Mat->at, 0.9f);
    xVec3AddScaled(&sandy->frame->mat.at, &newAt, 0.1f);

    sandy->frame->mat.at.y = 0.0f;
    xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
    xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up, &sandy->frame->mat.at);

    sandy->frame->mat.pos.y = 0.0f;
    xVec3SMul(&sandy->frame->vel, &sandy->frame->mat.at, 3.5f * sandy->cfg_npc->spd_moveMax);

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

S32 zNPCGoalBossSandyRunToRope::Exit(F32 dt, void* updCtxt)
{
    sChaseTimer = 0.0f;
    return xGoal::Exit(dt, updCtxt);
}

S32 zNPCGoalBossSandyClothesline::Enter(F32 dt, void* updCtxt)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    S32 i;
    xEnt* rope;
    F32 animParam;

    timeInGoal = 0.0f;
    stage = 0;

    sNumAttacks = sNumAttacks + 1;
    sDidClothesline = 1;
    playedAnimEarly = 0;

    xVec3Init(&sandy->frame->vel, 0.0f, 0.0f, 0.0f);

    xSndPlay3D(xStrHash("B101_ring1"), 0.77f, 0.0f, 0x0, 0x0, sandy, 80.0f, SND_CAT_GAME, 0.0f);
    xSndPlay3D(xStrHash("B101_ring2"), 0.77f, 0.0f, 0x0, 0x0, sandy, 80.0f, SND_CAT_GAME, 1.75f);

    animParam = 2.0f;

    for (i = 0; i < 4; i++)
    {
        rope = sandy->ropeObject[sandy->fromRope][i];
        zEntEvent(rope, eEventAnimPlay, &animParam);
        rope->model->Flags |= 0x2;
        xEntShow(rope);
    }

    xEntHide(sandy->ropeObjectLo[sandy->fromRope]);

    xVec3Copy(&bounceStartPoint, &sandy->frame->mat.pos);

    return zNPCGoalCommon::Enter(dt, updCtxt);
}

S32 zNPCGoalBossSandyClothesline::Process(en_trantype* trantype, F32 dt, void* updCtxt,
                                         xScene* xscn)
{
    zNPCBSandy* sandy = (zNPCBSandy*)psyche->clt_owner;
    U32 flip;
    xVec3 newAt;

    timeInGoal += dt;

    if (stage == 0)
    {
        if (sandy->AnimTimeRemain(NULL) < 1.7000000476837158f * dt)
        {
            stage = 1;

            if ((sandy->nfFlags & 0x20) == 0 || (xrand() & 0x300) == 0)
            {
                sandy->newsfish->SpeakStart(sNFSoundValue[19], 0, 0xFFFFFFFF);
                sandy->nfFlags |= 0x20;
            }

            sandy->springSndID = xSndPlay3D(xStrHash("B101_SC_feet_loop"), 0.77f, 0.0f, 0x0, 0x0,
                                            sandy, 30.0f, SND_CAT_GAME, 0.0f);

            sandy->bossFlags |= 0x8;

            if (sandy->round == 3)
            {
                sandy->bossFlags |= 0x10;
            }

            timeInGoal = 0.0f;
            DoAutoAnim(NPC_GSPOT_START, FALSE);

            xVec3Sub(&sandy->frame->mat.at, &sandy->bouncePoint[sandy->fromRope],
                     &sandy->bouncePoint[sandy->toRope]);

            sandy->frame->mat.at.y = 0.0f;
            xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
            xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up, &sandy->frame->mat.at);

            totalHoverTime = 2.0f * sRadiusOfRing / 10.0f;

            sLeftLegSpring.bound = &sandy->boundList[9]->bound;
            sRightLegSpring.bound = &sandy->boundList[11]->bound;

            sLeftLegSpring.node1 = xurand();
            sLeftLegSpring.node2 = xurand();
            sRightLegSpring.node1 = xurand();
            sRightLegSpring.node2 = xurand();

            flip = xrand();

            if (flip & 0x1)
            {
                sLeftLegSpring.vel1 = 0.05f;
            }
            else
            {
                sLeftLegSpring.vel1 = -0.05f;
            }

            if (flip & 0x2)
            {
                sLeftLegSpring.vel2 = 0.05f;
            }
            else
            {
                sLeftLegSpring.vel2 = -0.05f;
            }

            if (flip & 0x4)
            {
                sRightLegSpring.vel1 = 0.05f;
            }
            else
            {
                sRightLegSpring.vel1 = -0.05f;
            }

            if (flip & 0x8)
            {
                sRightLegSpring.vel2 = 0.05f;
            }
            else
            {
                sRightLegSpring.vel2 = -0.05f;
            }

            if (sandy->round == 3)
            {
                sLeftArmSpring.bound = &sandy->boundList[2]->bound;
                sRightArmSpring.bound = &sandy->boundList[4]->bound;

                sLeftArmSpring.node1 = xurand();
                sLeftArmSpring.node2 = xurand();
                sRightArmSpring.node1 = xurand();
                sRightArmSpring.node2 = xurand();

                if (flip & 0x10)
                {
                    sLeftArmSpring.vel1 = 0.05f;
                }
                else
                {
                    sLeftArmSpring.vel1 = -0.05f;
                }

                if (flip & 0x20)
                {
                    sLeftArmSpring.vel2 = 0.05f;
                }
                else
                {
                    sLeftArmSpring.vel2 = -0.05f;
                }

                if (flip & 0x40)
                {
                    sRightArmSpring.vel1 = 0.05f;
                }
                else
                {
                    sRightArmSpring.vel1 = -0.05f;
                }

                if (flip & 0x80)
                {
                    sRightArmSpring.vel2 = 0.05f;
                }
                else
                {
                    sRightArmSpring.vel2 = -0.05f;
                }
            }
        }
        else
        {
            xVec3Sub(&newAt, &sandy->bouncePoint[sandy->fromRope],
                     &sandy->bouncePoint[sandy->toRope]);

            newAt.y = 0.0f;
            xVec3Normalize(&newAt, &newAt);

            xVec3SMul(&sandy->frame->mat.at, (xVec3*)&sandy->model->Mat->at, 0.9f);
            xVec3AddScaled(&sandy->frame->mat.at, &newAt, 0.1f);

            sandy->frame->mat.at.y = 0.0f;
            xVec3Normalize(&sandy->frame->mat.at, &sandy->frame->mat.at);
            xVec3Cross(&sandy->frame->mat.right, &sandy->frame->mat.up, &sandy->frame->mat.at);

            if (timeInGoal < 1.0f)
            {
                xVec3SMul(&sandy->frame->mat.pos, &bounceStartPoint, 1.0f - timeInGoal);
                xVec3AddScaled(&sandy->frame->mat.pos, &sandy->bouncePoint[sandy->fromRope],
                               timeInGoal);
            }
            else
            {
                xVec3Copy(&sandy->frame->mat.pos, &sandy->bouncePoint[sandy->fromRope]);
            }
        }
    }
    else if (stage == 1)
    {
        if (timeInGoal > totalHoverTime)
        {
            stage = 2;

            xSndPlay3D(xStrHash("B101_ring4"), 0.77f, 0.0f, 0x0, 0x0, sandy, 80.0f, SND_CAT_GAME,
                       0.5f);

            if (sandy->springSndID)
            {
                xSndStop(sandy->springSndID);
                sandy->springSndID = 0;
            }

            sandy->bossFlags &= ~0x18;
            timeInGoal = 0.0f;

            if (playedAnimEarly == 0)
            {
                DoAutoAnim(NPC_GSPOT_START, FALSE);

                F32 animParam;
                S32 i;
                xEnt* rope;

                animParam = 3.0f;

                for (i = 0; i < 4; i++)
                {
                    rope = sandy->ropeObject[sandy->toRope][i];
                    zEntEvent(rope, eEventAnimPlay, &animParam);
                    rope->model->Flags |= 0x2;
                    xEntShow(rope);
                }

                xEntHide(sandy->ropeObjectLo[sandy->toRope]);
            }

            playedAnimEarly = 0;

            sLeftLegSpring.bound = NULL;
            sRightLegSpring.bound = NULL;
            sLeftArmSpring.bound = NULL;
            sRightArmSpring.bound = NULL;

            sandy->bossFlags &= ~0x18;
        }
        else
        {
            if (playedAnimEarly == 0 && timeInGoal > totalHoverTime - 0.3f)
            {
                playedAnimEarly = 1;

                F32 animParam;
                S32 i;
                xEnt* rope;

                animParam = 3.0f;

                for (i = 0; i < 4; i++)
                {
                    rope = sandy->ropeObject[sandy->toRope][i];
                    zEntEvent(rope, eEventAnimPlay, &animParam);
                    rope->model->Flags |= 0x2;
                    xEntShow(rope);
                }

                xEntHide(sandy->ropeObjectLo[sandy->toRope]);

                DoAutoAnim(NPC_GSPOT_START, FALSE);
            }

            F32 frac = timeInGoal / totalHoverTime;

            xVec3SMul(&sandy->frame->mat.pos, &sandy->bouncePoint[sandy->toRope], frac);
            xVec3AddScaled(&sandy->frame->mat.pos, &sandy->bouncePoint[sandy->fromRope],
                           1.0f - frac);
            xVec3Length2(&sandy->frame->mat.pos);

            sandy->boundFlags[9] |= 0x1;
            sandy->boundFlags[10] |= 0x1;
            sandy->boundFlags[11] |= 0x1;
            sandy->boundFlags[12] |= 0x1;

            if (sandy->round == 3)
            {
                sandy->boundFlags[2] |= 0x1;
                sandy->boundFlags[3] |= 0x1;
                sandy->boundFlags[4] |= 0x1;
                sandy->boundFlags[5] |= 0x1;
            }

            sandy->bossFlags |= 0x1;
            sandy->bossFlags |= 0x8;

            if (sandy->round == 3)
            {
                sandy->bossFlags |= 0x10;
            }
        }
    }
    else if (timeInGoal > 0.5f)
    {
        sandy->boundFlags[10] |= 0x10;
        sandy->boundFlags[12] |= 0x10;
    }

    return xGoal::Process(trantype, dt, updCtxt, xscn);
}

void xBinaryCamera::add_tweaks(char const*)
{
}

void xBinaryCamera::set_targets(xVec3 const& par_1, xVec3 const& par_2, F32 par_3)
{
    this->s1 = (xVec3*)(&par_1);
    this->s2 = (xVec3*)(&par_2);
    this->s2_radius = par_3;
}

S32 zNPCGoalBossSandyLeap::Name()
{
    return 0;
}
