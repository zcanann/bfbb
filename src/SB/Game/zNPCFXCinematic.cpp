#include "xVec3.h"
#include "xMath3.h"
#include "xMathInlines.h"
#include "xParEmitter.h"

#include "zNPCFXCinematic.h"
#include "zParPTank.h"
#include "zNPCTypeBossSB2.h"
#include "zNPCSupport.h"
#include "xString.h"
#include "xDebug.h"
#include "xstransvc.h"
#include "zScene.h"
#include "xEnt.h"
#include "iModel.h"
#include "rpskin.h"

#include <types.h>
#include <stdio.h>
#include <PowerPC_EABI_Support\MSL_C\MSL_Common\cmath>

// These structs were used in deadstripped functions.
// This function is here to force the symbols to be linked.
//
// The target opens .rodata with 11 unreferenced all-zero
// templates (0x148 total) that offset every later .rodata
// relocation. Same idiom as zVar.cpp.
void __deadstripped_zNPCFXCinematic()
{
    const char _406[0x0C] = {};
    const char _410[0x0C] = {};
    const char _441[0x0C] = {};

    const char _612[0x28] = {};
    const char _613[0x28] = {};
    const char _614[0x28] = {};
    const char _615[0x28] = {};
    const char _616[0x28] = {};
    const char _617[0x28] = {};
    const char _618[0x28] = {};
}

static NCINBeNosey* g_noz_ncin;

void get_bone_matrix(xMat4x3& mat, const NCINEntry* fxrec, const RwMatrixTag* animMat);
void clamp_bone_index(NCINEntry* fxrec, RpAtomic* model);

// belongs in xDebug.h next to the other xDebugAddTweak overloads
void xDebugAddTweak(const char*, xVec3*, const tweak_callback*, void*, U32);

xVec3* LERP(F32 t, xVec3* dst, const xVec3* a, const xVec3* b);
xVec3* SMOOTH(F32 t, xVec3* dst, const xVec3* a, const xVec3* b);

void EmitFreezeBreath(xVec3* pos, xVec3* vel, F32 dt, F32 elapsed, F32 total);
void NPAR_EmitTubeSpiralCin(const xVec3* pos, const xVec3* vel, F32 dt);

static void NCIN_Par_BPLANK_JET_1_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_BPLANK_JET_1");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_BPLANK_JET_2_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_BPLANK_JET_2");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_BPLANK_SBB_FLAMES_1_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_SBB_FLAMES_1");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_BPLANK_SBB_FLAMES_2_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_SBB_FLAMES_2");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_BPLANK_SBB_JET_1_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_SBB_JET_1");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_BPLANK_SBB_JET_2_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_SBB_JET_2");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_BPLANK_SBB_SMOKE_1_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_SBB_SMOKE_1");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_BPLANK_SBB_SMOKE_2_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_SBB_SMOKE_2");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_CIN_BIGDUP_SMOKE_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_CIN_BIGDUP_SMOKE");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_CIN_BIGDUP_SPAWN_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_CIN_BIGDUP_SPAWN");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

static void NCIN_Par_CIN_PLATFORM_JETS_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.pardata.emitter = zParEmitterFind("PAREMIT_CIN_PLATFORM_JETS");

        xParEmitterAsset* a = fxrec->fxdata.pardata.emitter->tasset;

        if (a == NULL || a->emit_type != 15)
        {
            fxrec->fxdata.pardata.emitter = NULL;
        }

        if (fxrec->fxdata.pardata.emitter == NULL)
        {
            fxrec->flg_stat |= 4;
            return;
        }

        F32& float_accum = fxrec->pos_A[1].y;

        if (float_accum < 0.0f)
        {
            float_accum = 0.1f + a->e_entbone.bone;
        }

        fxrec->pos_B[0] = 1e9f;
    }
}

void NCINBeNosey::CanRenderNow()
{
    zCutsceneMgr* csnmgr = (zCutsceneMgr*)this->use_csnmgr;
    NCINEntry* fxtab = this->use_fxtab;

    if (csnmgr == NULL)
    {
        return;
    }

    if (fxtab == NULL)
    {
        return;
    }

    NCINEntry* nextrec = fxtab;

    while (nextrec->typ_ncinfx != NCIN_FXTYP_UNKNOWN)
    {
        NCINEntry* fxrec = nextrec;

        nextrec++;

        if (fxrec->cb_fxrend != NULL && (fxrec->flg_stat & 1))
        {
            fxrec->cb_fxrend(csnmgr, fxrec);
        }
    }
}

void NCINBeNosey::UpdatedAnimated(RpAtomic* model, RwMatrixTag* animMat, U32 animIndex,
                                  U32 dataIndex)
{
    zCutsceneMgr* csnmgr = (zCutsceneMgr*)this->use_csnmgr;
    NCINEntry* fxtab = this->use_fxtab;

    if (csnmgr == NULL)
    {
        return;
    }

    if (fxtab == NULL)
    {
        return;
    }

    NCINEntry* nextrec = fxtab;

    while (nextrec->typ_ncinfx != NCIN_FXTYP_UNKNOWN)
    {
        NCINEntry* fxrec = nextrec;

        nextrec++;

        if (fxrec->cb_fxanim != NULL && (fxrec->flg_stat & 1))
        {
            fxrec->cb_fxanim(csnmgr, fxrec, model, animMat, animIndex, dataIndex);
        }
    }
}

void zNPCFXStartup()
{
    static NCINBeNosey nozey_npc_cinematics;

    g_noz_ncin = &nozey_npc_cinematics;
}

void zNPCFXShutdown()
{
}

struct NCINCutMap
{
    const char* csn_name;
    NCINEntry* fxtab;
    U32 hash_name;
};

typedef void (*NCINUpdCB)(zCutsceneMgr*, NCINEntry*, U32);
typedef void (*NCINAnimCB)(zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);

static void NCIN_ArfDogBoom(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_B101Shockwave_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_B201HideBelt_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_BombTrail_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_BoneTrail_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_BubHit(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_BubSlam(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_BubWipe(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_BubbleTrail_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_FireSpiral_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_FodProdBone_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_FodProd_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_FreezeBreath_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Generic_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_GloveShrapnel_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_GooLever_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_HammerShock(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_HammerStreak_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_HazProjShoot(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_HazTTSteam_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_HookRecoil_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Lightnin2Bones_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_LightninBone_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_MaryBoom(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_MidFish_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_OilHazard(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_BPLANK_JET_1_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_BPLANK_JET_2_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_BPLANK_SBB_FLAMES_1_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_BPLANK_SBB_FLAMES_2_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_BPLANK_SBB_JET_1_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_BPLANK_SBB_JET_2_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_BPLANK_SBB_SMOKE_1_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_BPLANK_SBB_SMOKE_2_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_CIN_BIGDUP_SMOKE_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_CIN_BIGDUP_SPAWN_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Par_CIN_PLATFORM_JETS_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_PatBossShrapnel_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_PeteBonk(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_SBBNode_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_ShieldPop(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_SleepyDRay_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_SleepyLamp_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_SpatGlow_Upd(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_WaterSplash(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_Zapper(const zCutsceneMgr*, NCINEntry*, S32);
static void NCIN_BombTrail_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_BoneTrail_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_BubTrailBone_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_BubbleTrail_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_EntityBonePar_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_FireSpiral_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_FodProdBone_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_FodProd_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_FreezeBreath_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_GloveShrapnel_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_GooLever_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_HammerStreak_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_HazTTSteam_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_HookRecoil_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_Lightnin2Bones_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_LightninBone_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_MidFish_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_PatBossShrapnel_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_SBBNode_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_SleepyDRay_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_SleepyLamp_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_SpatGlow_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);
static void NCIN_TTGunSmoke_AR(const zCutsceneMgr*, NCINEntry*, RpAtomic*, RwMatrixTag*, U32, U32);

static NCINEntry g_JF01_cin_fodder[7] = {
    { NCIN_FXTYP_JELLYLIGHT_01, (NCINUpdCB)NCIN_Zapper,
      NULL, NULL, 15.633334f, 15.966667f,
      { { 21.693f, 2.663f, 74.889f }, { 22.186f, 1.863f, 74.348f } },
      { { 21.693f, 2.663f, 74.889f }, { 22.186f, 1.863f, 74.348f } },
      (S8*)"JELLYLIGHT_01 a", 0, 0, 0 },
    { NCIN_FXTYP_JELLYLIGHT_01, (NCINUpdCB)NCIN_Zapper,
      NULL, NULL, 15.633334f, 15.966667f,
      { { 21.693f, 2.663f, 74.889f }, { 22.039f, 1.863f, 74.628f } },
      { { 21.693f, 2.663f, 74.889f }, { 22.039f, 1.863f, 74.628f } },
      (S8*)"JELLYLIGHT_01 b", 0, 0, 0 },
    { NCIN_FXTYP_JELLYLIGHT_01, (NCINUpdCB)NCIN_Zapper,
      NULL, NULL, 15.633334f, 15.966667f,
      { { 21.693f, 2.663f, 74.889f }, { 22.157f, 1.863f, 74.995f } },
      { { 21.693f, 2.663f, 74.889f }, { 22.157f, 1.863f, 74.995f } },
      (S8*)"JELLYLIGHT_01 c", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_LG, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 17.166668f, 17.2f,
      { { 27.262f, 3.154f, 88.222f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_LG", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProd_Upd,
      (NCINAnimCB)NCIN_FodProd_AR, NULL, 1.8333334f, 6.166667f,
      { { 27.893f, 3.949f, 89.272f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD a", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProd_Upd,
      (NCINAnimCB)NCIN_FodProd_AR, NULL, 18.666668f, 19.666668f,
      { { 27.893f, 3.949f, 89.272f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD b", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_JF01_cin_hammer[8] = {
    { NCIN_FXTYP_HAMSHOCK, (NCINUpdCB)NCIN_HammerShock,
      NULL, NULL, 1.5666667f, 1.6666667f,
      { { 11.586f, 16.481f, -59.963f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"HAMSHOCK", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_LG, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 1.7f, 1.8000001f,
      { { 11.586f, 16.181f, -59.963f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_LG", 0, 0, 0 },
    { NCIN_FXTYP_HAMSTREAK, (NCINUpdCB)NCIN_HammerStreak_Upd,
      (NCINAnimCB)NCIN_HammerStreak_AR, NULL, 1.2333333f, 2.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"HAMSTREAK a", 0, 0, 0 },
    { NCIN_FXTYP_HAMSTREAK, (NCINUpdCB)NCIN_HammerStreak_Upd,
      (NCINAnimCB)NCIN_HammerStreak_AR, NULL, 2.3333335f, 3.3333335f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"HAMSTREAK b", 0, 0, 0 },
    { NCIN_FXTYP_HAMSTREAK, (NCINUpdCB)NCIN_HammerStreak_Upd,
      (NCINAnimCB)NCIN_HammerStreak_AR, NULL, 3.5666668f, 3.9333336f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"HAMSTREAK c", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_SM, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 8.900001f, 9.0f,
      { { 12.604f, 17.0f, -58.185f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_SM a", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_SM, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 9.566668f, 9.666667f,
      { { 12.604f, 16.4f, -58.185f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_SM b", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_JF03_cin_tartar[10] = {
    { NCIN_FXTYP_TARTARSHOOT, (NCINUpdCB)NCIN_HazProjShoot,
      NULL, NULL, 12.6f, 12.833334f,
      { { -9.294f, 2.176f, 73.936f }, { -6.553f, 2.284f, 74.638f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARSHOOT a", 0, 0, 0 },
    { NCIN_FXTYP_TARTARSHOOT, (NCINUpdCB)NCIN_HazProjShoot,
      NULL, NULL, 12.933334f, 13.166667f,
      { { -9.181f, 2.236f, 73.556f }, { -6.357f, 2.25f, 73.469f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARSHOOT b", 0, 0, 0 },
    { NCIN_FXTYP_TARTARSHOOT, (NCINUpdCB)NCIN_HazProjShoot,
      NULL, NULL, 13.200001f, 13.366668f,
      { { -9.286f, 2.214f, 72.854f }, { -6.942f, 2.164f, 70.731f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARSHOOT c", 0, 0, 0 },
    { NCIN_FXTYP_TARTARSHOOT, (NCINUpdCB)NCIN_HazProjShoot,
      NULL, NULL, 13.900001f, 14.066668f,
      { { -14.262f, 1.952f, 75.581f }, { -5.214f, 2.273f, 75.341f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARSHOOT d", 0, 0, 0 },
    { NCIN_FXTYP_TARTARSHOOT, (NCINUpdCB)NCIN_HazProjShoot,
      NULL, NULL, 14.566668f, 14.800001f,
      { { -14.262f, 1.952f, 75.581f }, { -5.044f, 2.272f, 73.945f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARSHOOT e", 0, 0, 0 },
    { NCIN_FXTYP_TARTARSHOOT, (NCINUpdCB)NCIN_HazProjShoot,
      NULL, NULL, 15.1f, 15.300001f,
      { { -14.262f, 2.1952f, 75.581f }, { -5.226f, 2.272f, 72.566f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARSHOOT f", 0, 0, 0 },
    { NCIN_FXTYP_TARTARSTEAM, (NCINUpdCB)NCIN_HazTTSteam_Upd,
      (NCINAnimCB)NCIN_HazTTSteam_AR, NULL, 14.000001f, 20.833334f,
      { { 0.0f, 1.0f, 0.0f }, { 4.1f, 0.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARSTEAM A", 0, 0, 0 },
    { NCIN_FXTYP_TARTARSTEAM, (NCINUpdCB)NCIN_HazTTSteam_Upd,
      (NCINAnimCB)NCIN_HazTTSteam_AR, NULL, 19.333334f, 20.833334f,
      { { 0.0f, 1.0f, 0.0f }, { 3.1f, 0.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARSTEAM B", 0, 0, 0 },
    { NCIN_FXTYP_TTGUNSMOKE, (NCINUpdCB)NCIN_Generic_Upd,
      (NCINAnimCB)NCIN_TTGunSmoke_AR, NULL, 21.333334f, 23.2f,
      { { 0.0f, 0.0f, 0.7f }, { 0.1f, 19.1f, 0.0f } },
      { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"TARTARGUNSMOKE A", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_BB01_cin_glove[6] = {
    { NCIN_FXTYP_BUBSLAM_LG, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 17.0f, 17.166668f,
      { { -36.396f, 0.016f, -34.518f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_LG a", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_LG, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 17.0f, 17.166668f,
      { { -36.396f, 0.266f, -34.518f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_LG b", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_LG, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 17.0f, 17.166668f,
      { { -36.396f, 0.616f, -34.518f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_LG c", 0, 0, 0 },
    { NCIN_FXTYP_GLOVEFRAG, (NCINUpdCB)NCIN_GloveShrapnel_Upd,
      (NCINAnimCB)NCIN_GloveShrapnel_AR, NULL, 17.0f, 17.166668f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, 0.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"Glove Smash", 0, 0, 0 },
    { NCIN_FXTYP_MIDFISH, (NCINUpdCB)NCIN_MidFish_Upd,
      (NCINAnimCB)NCIN_MidFish_AR, NULL, 15.233335f, 17.2f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"MIDFISH", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_GL01_cin_monsoon[2] = {
    { NCIN_FXTYP_MONCLOUD, (NCINUpdCB)NCIN_Zapper,
      NULL, NULL, 16.366667f, 16.7f,
      { { 213.488f, 9.0f, 32.119f }, { 213.488f, 6.619f, 32.119f } },
      { { 213.488f, 9.0f, 32.119f }, { 213.488f, 6.619f, 32.119f } },
      (S8*)"MONCLOUD", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_RB01_cin_sleepytime[3] = {
    { NCIN_FXTYP_SLEEPYLAMP, (NCINUpdCB)NCIN_SleepyLamp_Upd,
      (NCINAnimCB)NCIN_SleepyLamp_AR, NULL, 0.0f, 5.8333335f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"SLEEPYLAMP", 0, 0, 0 },
    { NCIN_FXTYP_SLEEPYDRAY, (NCINUpdCB)NCIN_SleepyDRay_Upd,
      (NCINAnimCB)NCIN_SleepyDRay_AR, NULL, 8.766667f, 9.500001f,
      { { -20.968f, 0.985f, 10.83894f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"SLEEPYDRAY", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_BC01_cin_arfarf[9] = {
    { NCIN_FXTYP_ARFDOGBOOM, (NCINUpdCB)NCIN_ArfDogBoom,
      NULL, NULL, 7.3333335f, 8.333334f,
      { { 22.008f, 2.894f, 8.802f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"ARFDOGBOOM a", 0, 0, 0 },
    { NCIN_FXTYP_ARFDOGBOOM, (NCINUpdCB)NCIN_ArfDogBoom,
      NULL, NULL, 12.6f, 13.1f,
      { { 22.465f, 3.695f, 8.574f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"ARFDOGBOOM b", 0, 0, 0 },
    { NCIN_FXTYP_ARFDOGBOOM, (NCINUpdCB)NCIN_ArfDogBoom,
      NULL, NULL, 12.500001f, 13.333334f,
      { { 22.665f, 3.224f, 9.5f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"ARFDOGBOOM c", 0, 0, 0 },
    { NCIN_FXTYP_BONETRAIL, (NCINUpdCB)NCIN_BoneTrail_Upd,
      (NCINAnimCB)NCIN_BoneTrail_AR, NULL, 13.933334f, 14.400001f,
      { { 32.0f, 2.0f, 5.0f }, { 22.665f, 3.124f, 9.5f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BONETRAIL", 0, 0, 0 },
    { NCIN_FXTYP_HOOKRECOIL, (NCINUpdCB)NCIN_HookRecoil_Upd,
      (NCINAnimCB)NCIN_HookRecoil_AR, NULL, 0.6666667f, 2.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"HOOKRECOIL a", 0, 0, 0 },
    { NCIN_FXTYP_HOOKRECOIL, (NCINUpdCB)NCIN_HookRecoil_Upd,
      (NCINAnimCB)NCIN_HookRecoil_AR, NULL, 14.333334f, 14.6f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"HOOKRECOIL b", 0, 0, 0 },
    { NCIN_FXTYP_HOOKRECOIL, (NCINUpdCB)NCIN_HookRecoil_Upd,
      (NCINAnimCB)NCIN_HookRecoil_AR, NULL, 16.833334f, 17.266668f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"HOOKRECOIL c", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_SM, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 14.266667f, 14.433334f,
      { { 21.7136f, 4.1645f, 9.5129f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_BB01_cin_chuck[3] = {
    { NCIN_FXTYP_WATERSPLASH, (NCINUpdCB)NCIN_WaterSplash,
      NULL, NULL, 10.6f, 10.666667f,
      { { 24.947f, -0.804f, 39.105f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"WATERSPLASH", 0, 0, 0 },
    { NCIN_FXTYP_BOMBTRAIL, (NCINUpdCB)NCIN_BombTrail_Upd,
      (NCINAnimCB)NCIN_BombTrail_AR, NULL, 4.8333335f, 10.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BOMBTRAIL", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_KF01_cin_tubelet[5] = {
    { NCIN_FXTYP_FIRESPIRAL, (NCINUpdCB)NCIN_FireSpiral_Upd,
      (NCINAnimCB)NCIN_FireSpiral_AR, NULL, 0.0f, 6.433334f,
      { { -66.481f, 2.385f, 1.765f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FIRESPIRAL", 0, 0, 0 },
    { NCIN_FXTYP_PETEBONK, (NCINUpdCB)NCIN_PeteBonk,
      NULL, NULL, 6.3f, 6.366667f,
      { { -66.52f, 2.385f, 1.765f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"PETEBONK", 0, 0, 0 },
    { NCIN_FXTYP_MARYBOOM, (NCINUpdCB)NCIN_MaryBoom,
      NULL, NULL, 10.6f, 11.333334f,
      { { -66.481f, 5.584f, 1.458f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"MARYBOOM", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_LG, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 16.1f, 16.166668f,
      { { -68.625f, 1.334f, -1.131f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_LG", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_GY01_cin_slick[5] = {
    { NCIN_FXTYP_SHIELDPOP, (NCINUpdCB)NCIN_ShieldPop,
      NULL, NULL, 3.3333335f, 3.6666667f,
      { { 4.8f, 11.2f, 3.7f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"SHIELDPOP", 0, 0, 0 },
    { NCIN_FXTYP_OILSHOOT, (NCINUpdCB)NCIN_HazProjShoot,
      NULL, NULL, 5.2000003f, 5.6000004f,
      { { 4.8f, 12.5f, 3.7f }, { 8.64f, 9.411f, 1.659f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"OILSHOOT", 0, 0, 0 },
    { NCIN_FXTYP_OILHAZARD, (NCINUpdCB)NCIN_OilHazard,
      NULL, NULL, 5.566667f, 8.133334f,
      { { 8.64f, 9.411f, 1.659f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"OILHAZARD", 0, 0, 0 },
    { NCIN_FXTYP_BUBSLAM_SM, (NCINUpdCB)NCIN_BubSlam,
      NULL, NULL, 9.266667f, 9.400001f,
      { { 4.8f, 14.65f, 3.7f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBSLAM_SM", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_GY01_cin_gy_open[2] = {
    { NCIN_FXTYP_SPATULAGLOW, (NCINUpdCB)NCIN_SpatGlow_Upd,
      (NCINAnimCB)NCIN_SpatGlow_AR, NULL, 6.666667f, 18.333334f,
      { { 0.0f, 0.0f, 0.0f }, { 5.1f, 0.1f, 0.0f } },
      { { 2.4e+02f, 235.0f, 224.0f }, { 128.0f, 2.0f, 0.0f } },
      (S8*)"Spatula Glow", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_GY01_cin_gy_end[2] = {
    { NCIN_FXTYP_SPATULAGLOW, (NCINUpdCB)NCIN_SpatGlow_Upd,
      (NCINAnimCB)NCIN_SpatGlow_AR, NULL, 25.000002f, 28.333334f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, 0.1f, 0.0f } },
      { { 2.4e+02f, 235.0f, 224.0f }, { 128.0f, 3.5f, 0.0f } },
      (S8*)"Spatula Glow", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B101_b1_open[4] = {
    { NCIN_FXTYP_B101OPEN_SBENTER, (NCINUpdCB)NCIN_BubHit,
      NULL, NULL, 1.5000001f, 2.25f,
      { { 3.34f, 0.6f, 3.14f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"OPEN_SBENTER", 0, 0, 0 },
    { NCIN_FXTYP_B101OPEN_PATENTER, (NCINUpdCB)NCIN_BubHit,
      NULL, NULL, 1.8333334f, 2.3333335f,
      { { 4.64f, 0.58f, 3.26f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"OPEN_PATENTER", 0, 0, 0 },
    { NCIN_FXTYP_B101OPEN_SHOCKWAVE, (NCINUpdCB)NCIN_B101Shockwave_Upd,
      NULL, NULL, 35.5f, 38.333336f,
      { { 0.0f, -0.5f, -9.5f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"OPEN_LANDSHOCKWAVE", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B101_b1_round2[4] = {
    { NCIN_FXTYP_B101ROUND1_SHOCK1, (NCINUpdCB)NCIN_LightninBone_Upd,
      (NCINAnimCB)NCIN_LightninBone_AR, NULL, 0.0f, 5.0000005f,
      { { 0.0f, 0.7f, 0.0f }, { 0.1f, 2.1f, 0.0f } },
      { { 5.0f, -2.0f, 5.0f }, { -2.5f, -2.0f, -2.5f } },
      (S8*)"B101ROUND1_SHOCK1", 0, 0, 0 },
    { NCIN_FXTYP_B101ROUND1_SHOCK2, (NCINUpdCB)NCIN_LightninBone_Upd,
      (NCINAnimCB)NCIN_LightninBone_AR, NULL, 0.0f, 5.0000005f,
      { { 0.0f, 2.6f, 3.2f }, { 0.1f, 2.1f, 0.0f } },
      { { 5.0f, 5.0f, 2.0f }, { -2.5f, -2.5f, 2.0f } },
      (S8*)"B101ROUND1_SHOCK2", 0, 0, 0 },
    { NCIN_FXTYP_B101ROUND1_SHOCK3, (NCINUpdCB)NCIN_Lightnin2Bones_Upd,
      (NCINAnimCB)NCIN_Lightnin2Bones_AR, NULL, 0.0f, 5.0000005f,
      { { 0.0f, 0.0f, 0.5f }, { 0.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.5f }, { 0.1f, 8.1f, 0.0f } },
      (S8*)"B101ROUND1_SHOCK3", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B101_b1_ending[10] = {
    { NCIN_FXTYP_B101ENDING_SHOCK1, (NCINUpdCB)NCIN_Lightnin2Bones_Upd,
      (NCINAnimCB)NCIN_Lightnin2Bones_AR, NULL, 0.16666667f, 0.4666667f,
      { { 0.0f, 0.4f, 0.0f }, { 2.1f, 4.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, 5.1f, 0.0f } },
      (S8*)"ENDING_NECK1", 0, 0, 0 },
    { NCIN_FXTYP_B101ENDING_SHOCK1, (NCINUpdCB)NCIN_Lightnin2Bones_Upd,
      (NCINAnimCB)NCIN_Lightnin2Bones_AR, NULL, 0.8000001f, 0.90000004f,
      { { 0.0f, 0.4f, 0.0f }, { 2.1f, 4.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, 5.1f, 0.0f } },
      (S8*)"ENDING_NECK2", 0, 0, 0 },
    { NCIN_FXTYP_B101ENDING_SHOCK2, (NCINUpdCB)NCIN_LightninBone_Upd,
      (NCINAnimCB)NCIN_LightninBone_AR, NULL, 0.5f, 2.0f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, 17.1f, 0.0f } },
      { { 4.0f, 4.0f, 2.0f }, { -0.5f, -0.5f, -1.0f } },
      (S8*)"ENDING_LEFTSHOULDER", 0, 0, 0 },
    { NCIN_FXTYP_B101ENDING_SHOCK3, (NCINUpdCB)NCIN_LightninBone_Upd,
      (NCINAnimCB)NCIN_LightninBone_AR, NULL, 1.5000001f, 3.0000002f,
      { { -0.18f, 0.39f, 0.18f }, { 2.1f, 16.1f, 0.0f } },
      { { 3.0f, 3.0f, 3.0f }, { -1.5f, -1.5f, -1.5f } },
      (S8*)"ENDING_HELMETBALL", 0, 0, 0 },
    { NCIN_FXTYP_B101ENDING_SHOCK4, (NCINUpdCB)NCIN_LightninBone_Upd,
      (NCINAnimCB)NCIN_LightninBone_AR, NULL, 2.5000002f, 4.3333335f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, 24.1f, 0.0f } },
      { { -4.0f, 4.0f, 2.0f }, { 0.5f, -0.5f, -1.0f } },
      (S8*)"ENDING_RIGHTSHOULDER", 0, 0, 0 },
    { NCIN_FXTYP_B101ENDING_SHOCKWAVE, (NCINUpdCB)NCIN_B101Shockwave_Upd,
      NULL, NULL, 4.4333334f, 6.666667f,
      { { 0.0f, -0.5f, -1.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"ENDING_FALLSHOCKWAVE", 0, 0, 0 },
    { NCIN_FXTYP_B101ENDING_SHOCK2, (NCINUpdCB)NCIN_LightninBone_Upd,
      (NCINAnimCB)NCIN_LightninBone_AR, NULL, 25.000002f, 25.833334f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, 17.1f, 0.0f } },
      { { 2.0f, 2.0f, 1.0f }, { -0.5f, -0.5f, -1.0f } },
      (S8*)"ENDING_LEFTSHOULDER b", 0, 0, 0 },
    { NCIN_FXTYP_B101ENDING_SHOCK3, (NCINUpdCB)NCIN_LightninBone_Upd,
      (NCINAnimCB)NCIN_LightninBone_AR, NULL, 26.833334f, 27.666668f,
      { { -0.18f, 0.39f, 0.18f }, { 2.1f, 16.1f, 0.0f } },
      { { 2.0f, 2.0f, 2.0f }, { -1.0f, -1.0f, -1.0f } },
      (S8*)"ENDING_HELMETBALL b", 0, 0, 0 },
    { NCIN_FXTYP_SPATULAGLOW, (NCINUpdCB)NCIN_SpatGlow_Upd,
      (NCINAnimCB)NCIN_SpatGlow_AR, NULL, 20.500002f, 24.333334f,
      { { 0.0f, 0.0f, 0.0f }, { 5.1f, 0.1f, 0.0f } },
      { { 2.4e+02f, 235.0f, 224.0f }, { 128.0f, 1.0f, 0.0f } },
      (S8*)"Spatula Glow", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B201_b2_open[2] = {
    { NCIN_FXTYP_B201OPEN_FREEZE, (NCINUpdCB)NCIN_FreezeBreath_Upd,
      (NCINAnimCB)NCIN_FreezeBreath_AR, NULL, 3.3333335f, 5.0000005f,
      { { 0.0f, -0.2f, 0.0f }, { 0.1f, 29.1f, 0.0f } },
      { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"Openning Freeze Breath", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B201_b2_round2[4] = {
    { NCIN_FXTYP_B201HIDECONVEYOR, (NCINUpdCB)NCIN_B201HideBelt_Upd,
      NULL, NULL, 7.2333336f, 8.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, 0.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"Hide Conveyor a", 0, 0, 0 },
    { NCIN_FXTYP_B201ROUND1_FREEZE, (NCINUpdCB)NCIN_FreezeBreath_Upd,
      (NCINAnimCB)NCIN_FreezeBreath_AR, NULL, 9.333334f, 10.000001f,
      { { 0.0f, -0.2f, 0.0f }, { 0.1f, 29.1f, 0.0f } },
      { { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"Round 1 Freeze Breath", 0, 0, 0 },
    { NCIN_FXTYP_B201HIDECONVEYOR, (NCINUpdCB)NCIN_B201HideBelt_Upd,
      NULL, NULL, 10.966667f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, 0.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"Hide Conveyor b", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B201_b2_round3[2] = {
    { NCIN_FXTYP_B201GOOLEVER, (NCINUpdCB)NCIN_GooLever_Upd,
      (NCINAnimCB)NCIN_GooLever_AR, NULL, 0.8333334f, 3.3666668f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, 2.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"Goo Lever", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B201_b2_ending[2] = {
    { NCIN_FXTYP_B201FRAG, (NCINUpdCB)NCIN_PatBossShrapnel_Upd,
      (NCINAnimCB)NCIN_PatBossShrapnel_AR, NULL, 1.6666667f, 2.0f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, 0.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"Pat Splode", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B302_begin[27] = {
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 3.0000002f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"0-SBB_FLAMES_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 3.0000002f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"0-SBB_FLAMES_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 3.0000002f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"0-SBB_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 3.0000002f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"0-SBB_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 3.0000002f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"0-SBB_SMOKE_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 3.0000002f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"0-SBB_SMOKE_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SMOKE_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 36.666668f, 42.000004f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_BIGDUP_SMOKE", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 66.66667f, 72.0f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"2-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 66.66667f, 72.0f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"2-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 76.66667f, 84.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"3-SBB_FLAMES_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 76.66667f, 84.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"3-SBB_FLAMES_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 76.66667f, 84.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"3-SBB_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 76.66667f, 84.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"3-SBB_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 76.66667f, 84.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"3-SBB_SMOKE_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 76.66667f, 84.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"3-SBB_SMOKE_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 76.66667f, 84.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"4-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 76.66667f, 84.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"4-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 90.00001f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"5-SBB_FLAMES_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 90.00001f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"5-SBB_FLAMES_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 90.00001f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"5-SBB_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 90.00001f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"5-SBB_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 90.00001f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"5-SBB_SMOKE_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 90.00001f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"5-SBB_SMOKE_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 90.00001f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"6-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 90.00001f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"6-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_B303STUFF, (NCINUpdCB)NCIN_SBBNode_Upd,
      (NCINAnimCB)NCIN_SBBNode_AR, NULL, 0.0f, 166.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"SBB Nodes", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B302_end[31] = {
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"0-SBB_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"0-SBB_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 4.04f,
      { { 0.0f, 0.0f, 0.0f }, { 11.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-01", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 4.0733333f,
      { { 0.0f, 0.0f, 0.0f }, { 12.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-02", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 4.0933337f,
      { { 0.0f, 0.0f, 0.0f }, { 17.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-03", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 4.1133337f,
      { { 0.0f, 0.0f, 0.0f }, { 14.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-04", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 4.1333337f,
      { { 0.0f, 0.0f, 0.0f }, { 19.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-05", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 5.503334f,
      { { 0.0f, 0.0f, 0.0f }, { 13.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-07", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 5.54f,
      { { 0.0f, 0.0f, 0.0f }, { 22.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-08", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 5.5533338f,
      { { 0.0f, 0.0f, 0.0f }, { 10.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-09", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 5.596667f,
      { { 0.0f, 0.0f, 0.0f }, { 20.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-10", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 5.616667f,
      { { 0.0f, 0.0f, 0.0f }, { 25.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-11", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 5.7000003f,
      { { 0.0f, 0.0f, 0.0f }, { 18.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-12", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 11.966667f,
      { { 0.0f, 0.0f, 0.0f }, { 24.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-13", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 16.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-14", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 21.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-15", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 23.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-16", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_PLATFORM_JETS_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 15.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"1-CIN_PLATFORM_JETS-06", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"2-SBB_FLAMES_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"2-SBB_FLAMES_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"2-SBB_SMOKE_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 0.0f, 12.666667f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"2-SBB_SMOKE_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 12.900001f, 13.800001f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"3-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 12.900001f, 13.800001f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"3-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 20.333334f, 24.166668f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"4-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 20.333334f, 24.166668f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"4-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 30.666668f, 32.4f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"5-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 30.666668f, 32.4f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"5-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_B303STUFF, (NCINUpdCB)NCIN_BubbleTrail_Upd,
      (NCINAnimCB)NCIN_BubbleTrail_AR, NULL, 22.033335f, 22.6f,
      { { 1e+01f, 5.0f, -8.0f }, { 0.1f, 0.1f, 4e+02f } },
      { { 0.0f, 3e+01f, -3e+01f }, { 3.0f, 2e+01f, 4.0f } },
      (S8*)"SBB Leakage Left", 0, 0, 0 },
    { NCIN_FXTYP_B303STUFF, (NCINUpdCB)NCIN_BubbleTrail_Upd,
      (NCINAnimCB)NCIN_BubbleTrail_AR, NULL, 22.033335f, 22.6f,
      { { 11.0f, 2.0f, 8.0f }, { 0.1f, 0.1f, 4e+02f } },
      { { 0.0f, 3e+01f, 3e+01f }, { 3.0f, 2e+01f, 4.0f } },
      (S8*)"SBB Leakage Right", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_B303_end[28] = {
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 9.166667f, 33.300003f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"00-SBB_FLAMES_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_FLAMES_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 9.166667f, 33.300003f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"00-SBB_FLAMES_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 9.166667f, 33.300003f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"00-SBB_SMOKE_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_SMOKE_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 9.166667f, 33.300003f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"00-SBB_SMOKE_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 9.166667f, 38.166668f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"01-SBB_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_SBB_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 9.166667f, 38.166668f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"01-SBB_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 22.666668f, 25.333334f,
      { { 0.0f, 0.0f, 0.0f }, { 4.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"02-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 22.666668f, 25.333334f,
      { { 0.0f, 0.0f, 0.0f }, { 4.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"02-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 50.000004f, 53.500004f,
      { { 0.0f, 0.0f, 0.0f }, { 4.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"03-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 50.000004f, 53.500004f,
      { { 0.0f, 0.0f, 0.0f }, { 4.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"03-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SMOKE_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 36.666668f, 90.00001f,
      { { 0.0f, 0.0f, 0.0f }, { 16.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"04-CIN_BIGDUP_SMOKE", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SPAWN_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 55.333336f, 56.333336f,
      { { 0.0f, 0.0f, 0.0f }, { 16.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"05-CIN_BIGDUP_SPAWN", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 56.000004f, 58.666668f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"06-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 56.000004f, 58.666668f,
      { { 0.0f, 0.0f, 0.0f }, { 1.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"06-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SPAWN_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 64.600006f, 65.600006f,
      { { 0.0f, 0.0f, 0.0f }, { 16.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"07-CIN_BIGDUP_SPAWN", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 65.26667f, 74.16667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"08-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 65.26667f, 74.16667f,
      { { 0.0f, 0.0f, 0.0f }, { 2.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"08-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SPAWN_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 68.66667f, 69.66667f,
      { { 0.0f, 0.0f, 0.0f }, { 16.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"09-CIN_BIGDUP_SPAWN", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 69.333336f, 74.16667f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"10-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 69.333336f, 74.16667f,
      { { 0.0f, 0.0f, 0.0f }, { 3.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"10-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 77.66667f, 89.36667f,
      { { 0.0f, 0.0f, 0.0f }, { 5.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"11-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 77.66667f, 89.36667f,
      { { 0.0f, 0.0f, 0.0f }, { 5.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"11-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SPAWN_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 83.933334f, 84.933334f,
      { { 0.0f, 0.0f, 0.0f }, { 16.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"12-CIN_BIGDUP_SPAWN", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_1_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 84.600006f, 87.50001f,
      { { 0.0f, 0.0f, 0.0f }, { 7.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"13-BPLANK_JET_1", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_BPLANK_JET_2_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 84.600006f, 87.50001f,
      { { 0.0f, 0.0f, 0.0f }, { 7.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"13-BPLANK_JET_2", 0, 0, 0 },
    { NCIN_FXTYP_B303STUFF, (NCINUpdCB)NCIN_BubbleTrail_Upd,
      (NCINAnimCB)NCIN_BubbleTrail_AR, NULL, 25.666668f, 27.666668f,
      { { 3.0f, 0.0f, 4.8f }, { 13.1f, 24.1f, 1e+03f } },
      { { 0.0f, 0.0f, 12.0f }, { 0.5f, 1e+01f, 4.0f } },
      (S8*)"SBB Leakage Left", 0, 0, 0 },
    { NCIN_FXTYP_B303STUFF, (NCINUpdCB)NCIN_BubbleTrail_Upd,
      (NCINAnimCB)NCIN_BubbleTrail_AR, NULL, 29.000002f, 30.333334f,
      { { -3.0f, 0.0f, 4.8f }, { 14.1f, 16.1f, 1e+03f } },
      { { 0.0f, 0.0f, 12.0f }, { 0.5f, 1e+01f, 4.0f } },
      (S8*)"SBB Leakage Right", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINEntry g_HB00_prologue[24] = {
    { NCIN_FXTYP_BUBTRAILBONE, (NCINUpdCB)NCIN_Generic_Upd,
      (NCINAnimCB)NCIN_BubTrailBone_AR, NULL, 56.26667f, 56.666668f,
      { { 0.0f, 0.0f, 0.0f }, { 11.1f, 0.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLETRAILBONE 0", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SMOKE_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 31.666668f, 42.333336f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"CIN_DUPLO_SMOKE-01", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SPAWN_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 57.333336f, 57.833336f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"CIN_BIGDUP_SPAWN-01", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SPAWN_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 58.500004f, 59.000004f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"CIN_BIGDUP_SPAWN-02", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SPAWN_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 59.166668f, 60.000004f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"CIN_BIGDUP_SPAWN-03", 0, 0, 0 },
    { NCIN_FXTYP_PAR_ENTITY_BONE, (NCINUpdCB)NCIN_Par_CIN_BIGDUP_SPAWN_Upd,
      (NCINAnimCB)NCIN_EntityBonePar_AR, NULL, 59.833336f, 60.333336f,
      { { 0.0f, 0.0f, 0.0f }, { 0.1f, -0.9f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"CIN_BIGDUP_SPAWN-04", 0, 0, 0 },
    { NCIN_FXTYP_BUBTRAILBONE, (NCINUpdCB)NCIN_Generic_Upd,
      (NCINAnimCB)NCIN_BubTrailBone_AR, NULL, 57.000004f, 58.666668f,
      { { 0.0f, 0.0f, 0.0f }, { 7.1f, 6.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLETRAILBONE 1", 0, 0, 0 },
    { NCIN_FXTYP_BUBTRAILBONE, (NCINUpdCB)NCIN_Generic_Upd,
      (NCINAnimCB)NCIN_BubTrailBone_AR, NULL, 58.500004f, 59.333336f,
      { { 0.0f, 0.0f, 0.0f }, { 7.1f, 6.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLETRAILBONE 2", 0, 0, 0 },
    { NCIN_FXTYP_BUBTRAILBONE, (NCINUpdCB)NCIN_Generic_Upd,
      (NCINAnimCB)NCIN_BubTrailBone_AR, NULL, 59.000004f, 63.000004f,
      { { 0.0f, 0.0f, 0.0f }, { 5.1f, 6.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLETRAILBONE 3", 0, 0, 0 },
    { NCIN_FXTYP_BUBTRAILBONE, (NCINUpdCB)NCIN_Generic_Upd,
      (NCINAnimCB)NCIN_BubTrailBone_AR, NULL, 60.000004f, 60.666668f,
      { { 0.0f, 0.0f, 0.0f }, { 5.1f, 6.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLETRAILBONE 4", 0, 0, 0 },
    { NCIN_FXTYP_BUBTRAILBONE, (NCINUpdCB)NCIN_Generic_Upd,
      (NCINAnimCB)NCIN_BubTrailBone_AR, NULL, 60.666668f, 62.93334f,
      { { 0.0f, 0.0f, 0.0f }, { 7.1f, 6.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLETRAILBONE 4", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 65.333336f, 7e+01f,
      { { 0.2f, 0.0f, 0.5f }, { 2.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD a", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 65.833336f, 7e+01f,
      { { 0.2f, 0.0f, 0.5f }, { 7.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD b", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 66.333336f, 7e+01f,
      { { 0.2f, 0.0f, 0.5f }, { 6.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD c", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 66.833336f, 7e+01f,
      { { 0.2f, 0.0f, 0.5f }, { 5.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD d", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 74.833336f, 75.26667f,
      { { 0.2f, 0.0f, 0.5f }, { 6.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD r", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 75.700005f, 76.333336f,
      { { 0.2f, 0.0f, 0.5f }, { 6.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD r", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 75.833336f, 76.66667f,
      { { 0.2f, 0.0f, 0.5f }, { 3.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD s", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 77.16667f, 78.00001f,
      { { 0.2f, 0.0f, 0.5f }, { 6.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD t", 0, 0, 0 },
    { NCIN_FXTYP_FODDERPROD, (NCINUpdCB)NCIN_FodProdBone_Upd,
      (NCINAnimCB)NCIN_FodProdBone_AR, NULL, 86.00001f, 87.66667f,
      { { 0.2f, 0.0f, 0.5f }, { 7.1f, 5.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"FODDERPROD w", 0, 0, 0 },
    { NCIN_FXTYP_BUBWIPE, (NCINUpdCB)NCIN_BubWipe,
      NULL, NULL, 91.333336f, 92.833336f,
      { { -427.8f, 4.75f, 589.0f }, { 0.0f, 0.0f, -1.0f } },
      { { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLEWIPE 1", 0, 0, 0 },
    { NCIN_FXTYP_BUBWIPE, (NCINUpdCB)NCIN_BubWipe,
      NULL, NULL, 91.333336f, 93.333336f,
      { { -327.0f, 17.0f, 6.9e+02f }, { 0.0f, 0.0f, -1.0f } },
      { { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLEWIPE 2", 0, 0, 0 },
    { NCIN_FXTYP_BUBTRAILBONE, (NCINUpdCB)NCIN_Generic_Upd,
      (NCINAnimCB)NCIN_BubTrailBone_AR, NULL, 193.33334f, 200.00002f,
      { { 0.0f, 0.0f, 0.0f }, { 7.1f, 6.1f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      (S8*)"BUBBLETRAILBONE 7", 0, 0, 0 },
    { NCIN_FXTYP_UNKNOWN, NULL,
      NULL, NULL, 0.0f, 0.0f,
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } },
      NULL, 0, 0, 0 },
};

static NCINCutMap g_cutmap[24] = {
    { "cin_fodder", g_JF01_cin_fodder, 0 },
    { "cin_hammer", g_JF01_cin_hammer, 0 },
    { "cin_tartar", g_JF03_cin_tartar, 0 },
    { "cin_glove", g_BB01_cin_glove, 0 },
    { "cin_monsoon", g_GL01_cin_monsoon, 0 },
    { "cin_sleepytime_intro", g_RB01_cin_sleepytime, 0 },
    { "cin_arf_intro", g_BC01_cin_arfarf, 0 },
    { "cin_chuck", g_BB01_cin_chuck, 0 },
    { "cin_tublet_intro", g_KF01_cin_tubelet, 0 },
    { "cin_slick", g_GY01_cin_slick, 0 },
    { "gy_dutchman_open", g_GY01_cin_gy_open, 0 },
    { "gy_dutchman_end", g_GY01_cin_gy_end, 0 },
    { "b1_open", g_B101_b1_open, 0 },
    { "b1_round2", g_B101_b1_round2, 0 },
    { "b1_ending", g_B101_b1_ending, 0 },
    { "b2_open", g_B201_b2_open, 0 },
    { "b2_round2", g_B201_b2_round2, 0 },
    { "b2_round3", g_B201_b2_round3, 0 },
    { "b2_ending", g_B201_b2_ending, 0 },
    { "B3_open", g_B302_begin, 0 },
    { "B3_transition", g_B302_end, 0 },
    { "B3_end_game_win", g_B303_end, 0 },
    { "cin_prolog", g_HB00_prologue, 0 },
    { NULL, NULL, 0 },
};

static NCINEntry* zNPCFXCutscenePickTable(const zCutsceneMgr* csnmgr)
{
    NCINEntry* fxtab = NULL;
    NCINCutMap* cutmap;
    static S32 needinit = 1;

    if (needinit != 0)
    {
        cutmap = g_cutmap;

        while (cutmap->csn_name != NULL)
        {
            cutmap->hash_name = xStrHash(cutmap->csn_name);
            cutmap++;
        }

        needinit = 0;
    }

    cutmap = g_cutmap;
    xCutscene* csn = csnmgr->csn;

    while (cutmap->csn_name != NULL)
    {
        if (cutmap->hash_name == csn->Info->AssetID)
        {
            fxtab = cutmap->fxtab;
            break;
        }

        cutmap++;
    }

    return fxtab;
}

S32 zNPCFXCutscenePrep(const xScene*, F32, const zCutsceneMgr* csnmgr)
{
    char tweakBase[128];
    char tweakName[128];

    NCINEntry* fxtab = zNPCFXCutscenePickTable(csnmgr);

    if (fxtab == NULL)
    {
        return 0;
    }

    if (csnmgr != NULL && csnmgr->csn != NULL)
    {
        csnmgr->csn->NoseyClear();
    }

    g_noz_ncin->Init(csnmgr, fxtab, 3);

    NCINEntry* nextrec = fxtab;

    while (nextrec->typ_ncinfx != NCIN_FXTYP_UNKNOWN)
    {
        NCINEntry* fxrec = nextrec;

        nextrec++;

        fxrec->flg_stat = 0;
        fxrec->flg_stat = 2 | 8;
        memset(&fxrec->fxdata, 0, sizeof(NCINData));
    }

    nextrec = fxtab;

    while (nextrec->typ_ncinfx != NCIN_FXTYP_UNKNOWN)
    {
        NCINEntry* fxrec = nextrec;

        nextrec++;

        sprintf(tweakBase, "CinematicFX|%s", fxrec->twk_name);

        sprintf(tweakName, "%s|posA[0]", tweakBase);
        xDebugAddTweak(tweakName, &fxrec->pos_A[0], NULL, NULL, 2);

        sprintf(tweakName, "%s|posA[1]", tweakBase);
        xDebugAddTweak(tweakName, &fxrec->pos_A[1], NULL, NULL, 2);

        sprintf(tweakName, "%s|posB[0]", tweakBase);
        xDebugAddTweak(tweakName, &fxrec->pos_B[0], NULL, NULL, 2);

        sprintf(tweakName, "%s|posB[1]", tweakBase);
        xDebugAddTweak(tweakName, &fxrec->pos_B[1], NULL, NULL, 2);

        sprintf(tweakName, "%s|tym_beg", tweakBase);
        xDebugAddTweak(tweakName, &fxrec->tym_beg, 0.0f, 1e9f, NULL, NULL, 2);

        sprintf(tweakName, "%s|tym_end", tweakBase);
        xDebugAddTweak(tweakName, &fxrec->tym_end, 0.0f, 1e9f, NULL, NULL, 2);
    }

    return 1;
}

void NCINBeNosey::Init(const zCutsceneMgr* m, NCINEntry* e, S32 i)
{
    this->use_csnmgr = m;
    this->use_fxtab = e;
    this->flg_nosey = i;
}

void zNPCFXCutsceneDone(const xScene*, F32, const zCutsceneMgr* csnmgr)
{
    NCINEntry* fxtab = zNPCFXCutscenePickTable(csnmgr);

    if (fxtab == NULL)
    {
        return;
    }

    if (csnmgr != NULL && csnmgr->csn != NULL)
    {
        csnmgr->csn->NoseyClear();
    }

    g_noz_ncin->Done();

    NCINEntry* nextrec = fxtab;

    while (nextrec->typ_ncinfx != NCIN_FXTYP_UNKNOWN)
    {
        NCINEntry* fxrec = nextrec;
        nextrec++;

        if (fxrec->flg_stat & 4)
        {
            continue;
        }

        if (!(fxrec->flg_stat & 1))
        {
            continue;
        }

        fxrec->cb_fxupd((zCutsceneMgr*)csnmgr, fxrec, 1);
        fxrec->flg_stat &= ~1;
    }
}

void NCINBeNosey::Done()
{
    this->use_csnmgr = 0;
    this->use_fxtab = 0;
    this->flg_nosey = 0;
}

void zNPCFXCutscene(const xScene*, F32, const zCutsceneMgr* csnmgr)
{
    xCutscene* csn;
    S32 need_animated;
    S32 need_render;
    S32 flags;

    csn = csnmgr->csn;

    NCINEntry* fxtab = zNPCFXCutscenePickTable(csnmgr);

    if (fxtab == NULL)
    {
        return;
    }

    NCINEntry* nextrec = fxtab;
    need_animated = 0;
    need_render = 0;

    while (nextrec->typ_ncinfx != NCIN_FXTYP_UNKNOWN)
    {
        NCINEntry* fxrec = nextrec;
        nextrec++;

        if (csn->Time < fxrec->tym_beg)
        {
            continue;
        }

        if (fxrec->flg_stat & 4)
        {
            continue;
        }

        if (csn->Time > fxrec->tym_end && (fxrec->flg_stat & 1))
        {
            fxrec->cb_fxupd((zCutsceneMgr*)csnmgr, fxrec, 1);
            fxrec->flg_stat &= ~1;
            continue;
        }

        fxrec->flg_stat |= 1;
        fxrec->cb_fxupd((zCutsceneMgr*)csnmgr, fxrec, 0);

        if (!(fxrec->flg_stat & 2))
        {
            fxrec->flg_stat &= ~8;
        }

        fxrec->flg_stat &= ~2;

        if (fxrec->flg_stat & 4)
        {
            fxrec->flg_stat &= ~1;
        }

        if (fxrec->flg_stat & 1)
        {
            if (fxrec->cb_fxanim != NULL)
            {
                need_animated++;
            }

            if (fxrec->cb_fxrend != NULL)
            {
                need_render++;
            }
        }
    }

    flags = 0;

    if (need_animated != 0)
    {
        flags |= 2;
    }

    if (need_render != 0)
    {
        flags |= 1;
    }

    if (flags != 0)
    {
        g_noz_ncin->flg_nosey = flags;
        csn->NoseySet(g_noz_ncin);
    }
    else
    {
        csn->NoseyClear();
    }
}

static void NCIN_Generic_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_BubSlam(const zCutsceneMgr*, NCINEntry* fxrec, S32 param)
{
    if (param != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (!(fxrec->flg_stat & 2))
    {
        return;
    }

    switch (fxrec->typ_ncinfx)
    {
    case 3:
        zFX_SpawnBubbleSlam(&fxrec->pos_A[0], 64, PI, 2.0f, 2.0f);
        break;
    case 2:
        zFX_SpawnBubbleSlam(&fxrec->pos_A[0], 128, PI, 5.0f, 5.0f);
        break;
    default:
        break;
    }
}

static void NCIN_BubWipe(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    xVec3* pos;
    xVec3* vel;
    xVec3* pp;
    xVec3* vp;
    S32 i;

    static const xVec3 scl_wall = { 3.0f, 3.0f, 3.0f };
    static const xVec3 vel_wall = { 1.0f, 0.5f, 0.5f };
    static xMat4x3 mat_fake;

    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    xMat4x3* mat = &mat_fake;

    mat->pos = fxrec->pos_A[0];
    mat->at = fxrec->pos_A[1];
    mat->up = fxrec->pos_B[0];
    mat->right = fxrec->pos_B[1];

    pos = (xVec3*)xMemPushTemp(2 * 50 * sizeof(xVec3));
    vel = pos + 50;

    xVec3 vec_infront = mat->at * 1.2f;

    F32 sx = scl_wall.x;
    F32 sy = scl_wall.y;
    F32 sz = scl_wall.z;
    F32 vx = vel_wall.x;
    F32 vy = vel_wall.y;
    F32 vz = vel_wall.z;

    pp = pos;
    vp = vel;

    for (i = 0; i < 50; i++)
    {
        pp->x = mat->pos.x + (xurand() - 0.5f) + sx * (xurand() - 0.5f);
        pp->y = mat->pos.y + (xurand() - 0.5f) + sy * (xurand() - 0.5f);
        pp->z = mat->pos.z + (xurand() - 0.5f) + sz * (xurand() - 0.5f);

        *pp += vec_infront;

        vp->x = vx * (xurand() - 0.5f);
        vp->y = vy * (xurand() - 0.5f);
        vp->z = vz * (xurand() - 0.5f);

        pp++;
        vp++;
    }

    zParPTankSpawnBubbles(pos, vel, 50, 1.0f);
    xMemPopTemp(pos);
}

static void NCIN_BubTrailBone_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                          U32 num_1, U32 num_2)
{
    S32 ifx = fxrec->pos_A[1].x;
    S32 idx_boneInterest = fxrec->pos_A[1].y;

    if (num_1 != ifx)
    {
        return;
    }

    xVec3 pos_emit = *(const xVec3*)&animMat->pos;

    if (idx_boneInterest > 0)
    {
        pos_emit += *(const xVec3*)&animMat[idx_boneInterest].pos;
    }

    zFX_SpawnBubbleTrail(&pos_emit, 1);
}

static void NCIN_BubHit(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
    else
    {
        if (fxrec->flg_stat & 2)
        {
            zFX_SpawnBubbleHit(&fxrec->pos_A[0], 16);
        }
        zFX_SpawnBubbleHit(&fxrec->pos_A[0], 3);
    }
}

static void NCIN_Zapper(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    _tagLightningAdd addInfo;

    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        switch (fxrec->typ_ncinfx)
        {
        case NCIN_FXTYP_JELLYLIGHT_01:
        case NCIN_FXTYP_MONCLOUD:
            if (fxrec->fxdata.lytdata.lyt_zap != NULL)
            {
                zLightningKill(fxrec->fxdata.lytdata.lyt_zap);
            }

            fxrec->fxdata.lytdata.lyt_zap = NULL;
            break;
        }
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        memset(&addInfo, 0, sizeof(_tagLightningAdd));

        switch (fxrec->typ_ncinfx)
        {
        case NCIN_FXTYP_JELLYLIGHT_01:
            NPCC_MakeLightningInfo(NPC_LYT_JELLYFISH, &addInfo);
            break;
        case NCIN_FXTYP_MONCLOUD:
            NPCC_MakeLightningInfo(NPC_LYT_CLOUDZAP, &addInfo);
            break;
        }

        addInfo.start = &fxrec->pos_A[0];
        addInfo.end = &fxrec->pos_A[1];
        addInfo.time = 0.5f + (fxrec->tym_end - fxrec->tym_beg);

        fxrec->fxdata.lytdata.lyt_zap = zLightningAdd(&addInfo);
    }

    if (fxrec->fxdata.lytdata.lyt_zap == NULL)
    {
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_HammerShock(const zCutsceneMgr*, NCINEntry* fxrec, S32 param)
{
    if (param != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }
    else if (fxrec->flg_stat & 2)
    {
        zFXHammer(&fxrec->pos_A[0]);
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_HammerStreak_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        xFXStreakStop(fxrec->fxdata.strkdata.sid_horz);
        xFXStreakStop(fxrec->fxdata.strkdata.sid_vert);

        fxrec->fxdata.strkdata.sid_horz = 57005;
        fxrec->fxdata.strkdata.sid_vert = 57005;
    }
    else if (fxrec->flg_stat & 2)
    {
        en_npcstreak styp_h = NPC_STRK_HAMMERSMASH_HORZ;
        en_npcstreak styp_v = NPC_STRK_HAMMERSMASH_VERT;

        fxrec->fxdata.strkdata.sid_horz = NPCC_StreakCreate(styp_h);
        fxrec->fxdata.strkdata.sid_vert = NPCC_StreakCreate(styp_v);
    }
}

static void NCIN_HammerStreak_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                          U32 animIndex, U32 dataIndex)
{
    S32 i;

    if (animIndex != 2)
    {
        return;
    }

    U32 idx_hamBone[4] = { 23, 26, 25, 22 };
    xMat4x3* mat_root = (xMat4x3*)animMat;
    xMat4x3* mat_hamBone[4];

    for (i = 0; i < 4; i++)
    {
        mat_hamBone[i] = (xMat4x3*)&animMat[idx_hamBone[i]];
    }

    const xVec3 left = mat_hamBone[0]->pos + mat_root->pos;
    const xVec3 right = mat_hamBone[1]->pos + mat_root->pos;
    const xVec3 top = mat_hamBone[2]->pos + mat_root->pos;
    const xVec3 bottom = mat_hamBone[3]->pos + mat_root->pos;

    NCINStrk* strkdat = &fxrec->fxdata.strkdata;

    xFXStreakUpdate(strkdat->sid_horz, &left, &right);
    xFXStreakUpdate(strkdat->sid_vert, &top, &bottom);
}

static void NCIN_WaterSplash(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        NPCC_MakeASplash(&fxrec->pos_A[0], -1.0f);
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_HazProjShoot(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        if (fxrec->fxdata.hazdata.npchaz != NULL)
        {
            fxrec->fxdata.hazdata.npchaz->MarkForRecycle();
        }
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        en_npchaz use_haztyp = NPC_HAZ_TARTARPROJ;

        switch (fxrec->typ_ncinfx)
        {
        case NCIN_FXTYP_TARTARSHOOT:
            use_haztyp = NPC_HAZ_TARTARPROJ;
            break;
        case NCIN_FXTYP_BONESHOOT:
            use_haztyp = NPC_HAZ_ARFBONE;
            break;
        case NCIN_FXTYP_OILSHOOT:
            use_haztyp = NPC_HAZ_OILBUBBLE;
            break;
        }

        NPCHazard* haz = HAZ_Acquire();

        if (haz == NULL)
        {
            return;
        }

        if (!haz->ConfigHelper(use_haztyp))
        {
            return;
        }

        haz->SetNPCOwner(NULL);
        haz->custdata.collide.flg_collide = 0;

        xVec3 pos_beg = fxrec->pos_A[0];
        xVec3 pos_end = fxrec->pos_A[1];
        xVec3 dir_shoot = pos_beg - pos_end;

        F32 dst_shoot = dir_shoot.length();
        F32 tym = fxrec->tym_end - fxrec->tym_beg;

        if (tym < 1e-5f)
        {
            tym = 0.25f;
        }

        haz->custdata.tartar.pos_tgt = pos_end;
        haz->Start(&fxrec->pos_A[0], tym);
        fxrec->fxdata.hazdata.npchaz = haz;
    }

    if (fxrec->fxdata.hazdata.npchaz == NULL)
    {
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->fxdata.hazdata.npchaz->typ_hazard != NPC_HAZ_TARTARSPILL)
    {
        fxrec->fxdata.hazdata.npchaz->flg_hazard &= ~0x60000;
    }
}

static void NCIN_HazTTSteam_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        if (fxrec->fxdata.hazdata.npchaz != NULL)
        {
            fxrec->fxdata.hazdata.npchaz->MarkForRecycle();
        }
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        NPCHazard* haz = HAZ_Acquire();

        if (haz == NULL)
        {
            return;
        }

        if (!haz->ConfigHelper(NPC_HAZ_TARTARSTINK))
        {
            return;
        }

        haz->SetNPCOwner(NULL);

        F32 tym = fxrec->tym_end - fxrec->tym_beg;

        if (tym < 1e-5f)
        {
            tym = 0.25f;
        }

        haz->Start(&g_Y3, tym);
        fxrec->fxdata.hazdata.npchaz = haz;
    }

    if (fxrec->fxdata.hazdata.npchaz == NULL)
    {
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_HazTTSteam_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                        U32 animIndex, U32 dataIndex)
{
    S32 idx_boneSign;

    U32 ifx = fxrec->pos_A[1].x;

    if (animIndex != ifx)
    {
        return;
    }

    xVec3 vec_offset = fxrec->pos_A[0];
    xVec3 pos;

    idx_boneSign = fxrec->pos_A[1].y;

    if (idx_boneSign > 0)
    {
        RwMatrixTag* mat_bone = &animMat[idx_boneSign];

        pos = *(const xVec3*)&mat_bone->pos;
        pos += *(const xVec3*)&mat_bone->right * vec_offset.x;
        pos += *(const xVec3*)&mat_bone->up * vec_offset.y;
        pos += *(const xVec3*)&mat_bone->at * vec_offset.z;
        pos += *(const xVec3*)&animMat->pos;
    }
    else
    {
        pos = *(const xVec3*)&animMat->pos;
        pos += *(const xVec3*)&animMat->right * vec_offset.x;
        pos += *(const xVec3*)&animMat->up * vec_offset.y;
        pos += *(const xVec3*)&animMat->at * vec_offset.z;
    }

    NPCHazard* haz = fxrec->fxdata.hazdata.npchaz;
    haz->pos_hazard = pos;
}

static void NCIN_TTGunSmoke_AR(const zCutsceneMgr* csnmgr, NCINEntry* fxrec, RpAtomic*,
                        RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    S32 idx_boneGun;
    F32 spd_blow;
    F32 rat_blow;

    U32 idx_roboAnim = fxrec->pos_A[1].x;

    if (animIndex != idx_roboAnim)
    {
        return;
    }

    idx_boneGun = fxrec->pos_A[1].y;

    xVec3 vec_offset = fxrec->pos_A[0];
    xVec3 pos_smoke;

    xMat4x3* mat_bone = (xMat4x3*)&animMat[idx_boneGun];

    pos_smoke = mat_bone->pos;
    pos_smoke += mat_bone->right * vec_offset.x;
    pos_smoke += mat_bone->up * vec_offset.y;
    pos_smoke += mat_bone->at * vec_offset.z;
    pos_smoke += *(const xVec3*)&animMat->pos;

    F32 tym_blow[2] = { 22.000002f, 23.166667f };

    xCutscene* csn = csnmgr->csn;

    if (csn->Time < tym_blow[0])
    {
        spd_blow = 0.0f;
    }
    else if (csn->Time > tym_blow[1])
    {
        spd_blow = 0.0f;
    }
    else
    {
        rat_blow = (csn->Time - tym_blow[0]) / (tym_blow[1] - tym_blow[0]);
        spd_blow = LERP(ARCH3(1.0f - CLAMP(rat_blow, 0.0f, 1.0f)), 0.5f, 15.5f);
    }

    spd_blow += spd_blow * (0.25f * (xurand() - 0.5f));

    const xVec3 dir_blow = fxrec->pos_B[0];
    xVec3 vel_smoke;

    vel_smoke = g_Y3 * 2.0f;
    vel_smoke += g_Y3 * (2.0f * (xurand() - 0.5f));
    vel_smoke += dir_blow * spd_blow;

    NPAR_EmitTarTarSmoke(&pos_smoke, &vel_smoke);
}

static void NCIN_ArfDogBoom(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        if (fxrec->fxdata.hazdata.npchaz != NULL)
        {
            fxrec->fxdata.hazdata.npchaz->MarkForRecycle();
        }
        return;
    }

    if (!(fxrec->flg_stat & 2))
    {
        return;
    }

    NPCHazard* haz = HAZ_Acquire();

    if (haz == NULL)
    {
        return;
    }

    if (!haz->ConfigHelper(NPC_HAZ_PUPPYNUKE))
    {
        return;
    }

    haz->SetNPCOwner(NULL);

    F32 tym = fxrec->tym_end - fxrec->tym_beg;

    tym = (0.5f > tym) ? 0.5f : tym;

    haz->custdata.typical.rad_max = 1.24f;
    haz->Start(&fxrec->pos_A[0], tym);
    fxrec->fxdata.hazdata.npchaz = haz;
}

static void NCIN_SleepyLamp_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.lampdata.rast = NPCC_FindRWRaster("fx_sleepy_nightlight");
    }
}

static void NCIN_SleepyLamp_AR(const zCutsceneMgr* csnmgr, NCINEntry* fxrec, RpAtomic*,
                        RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    if (animIndex == 2)
    {
        xVec3* pos_robo = &fxrec->fxdata.lampdata.pos_robo;

        *pos_robo = *(const xVec3*)&animMat->pos;
        pos_robo->y += -2.0f;
        return;
    }

    if (animIndex != 3)
    {
        return;
    }

    xVec3* pos_robo = &fxrec->fxdata.lampdata.pos_robo;

    NPCCone cone;
    xVec3 pos_lamp = *(const xVec3*)&animMat->pos;
    xVec3 rgb_peace = { 1.0f, 1.0f, 0.63f };
    xVec3 rgb_anger = { 0.5f, 0.0f, 0.0f };
    xVec3 rgb_current;
    RwRGBA rgba_top;
    RwRGBA rgba_bot;
    F32 pct;

    static const F32 tym_anger[4] = { 4.8333335f, 5.3333335f, 11.000001f, 11.666667f };

    F32 tym = csnmgr->csn->Time;

    if (tym < tym_anger[0])
    {
        rgb_current = rgb_peace;
    }
    else if (tym > tym_anger[3])
    {
        rgb_current = rgb_peace;
    }
    else if (tym > tym_anger[1] && tym < tym_anger[2])
    {
        rgb_current = rgb_anger;
    }
    else
    {
        if (tym < tym_anger[1])
        {
            pct = (tym - tym_anger[0]) / (tym_anger[1] - tym_anger[0]);
        }
        else
        {
            pct = 1.0f - (tym - tym_anger[2]) / (tym_anger[3] - tym_anger[2]);
        }

        SMOOTH(CLAMP(pct, 0.0f, 1.0f), &rgb_current, &rgb_peace, &rgb_anger);
    }

    U8 red = (U8)(255.0f * rgb_current.x);
    U8 green = (U8)(255.0f * rgb_current.y);
    U8 blue = (U8)(255.0f * rgb_current.z);

    rgba_top.red = red;
    rgba_top.green = green;
    rgba_top.blue = blue;
    rgba_top.alpha = 96;

    rgba_bot.red = red;
    rgba_bot.green = green;
    rgba_bot.blue = blue;
    rgba_bot.alpha = 16;

    memset(&cone, 0, sizeof(NPCCone));

    cone.RadiusSet(3.5f);
    cone.ColorSet(rgba_top, rgba_bot);
    cone.UVBaseSet(0.5f, 0.0f);
    cone.UVSliceSet(1.0f, 1.0f);
    cone.TextureSet(NULL);
    cone.RenderCone(&pos_lamp, pos_robo);
}

static void NCIN_SleepyDRay_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.draydata.rast = NPCC_FindRWRaster("fx_sleepy_beamodeath");
        zFX_SpawnBubbleSlam(&fxrec->pos_A[0], 64, 2.0f * PI, 1.5f, 3.0f);
    }
}

static void NCIN_SleepyDRay_AR(const zCutsceneMgr* csnmgr, NCINEntry* fxrec, RpAtomic*,
                        RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    static const F32 uv_scroll_dray[2] = { 0.0f, -4.55f };
    static const F32 uv_slice_dray[2] = { 0.25f, 0.25f };

    xVec3 pos_trail;

    if (animIndex == 0)
    {
        const xVec3* pos_beam = &fxrec->pos_A[0];

        for (S32 i = 0; i < 6; i++)
        {
            LERP(i / 6.0f, &pos_trail, pos_beam, (const xVec3*)&animMat->pos);
            zFX_SpawnBubbleTrail(&pos_trail, 4);
        }

        zFX_SpawnBubbleTrail((const xVec3*)&animMat->pos, 4);
        return;
    }

    if (animIndex != 2)
    {
        return;
    }

    xVec3 pos_top = *(const xVec3*)&animMat[16].pos + *(const xVec3*)&animMat->pos;

    const RwRGBA rgba_top = { 255, 255, 255, 255 };
    const RwRGBA rgba_bot = { 255, 255, 255, 255 };

    F32 tym = csnmgr->csn->Time - fxrec->tym_beg;
    F32 uv_u = tym * uv_scroll_dray[0];
    F32 uv_v = tym * uv_scroll_dray[1];

    NPCCone cone;

    memset(&cone, 0, sizeof(NPCCone));

    cone.RadiusSet(0.5f);
    cone.ColorSet(rgba_top, rgba_bot);
    cone.UVBaseSet(uv_u, uv_v);
    cone.UVSliceSet(uv_slice_dray[0], uv_slice_dray[1]);
    cone.TextureSet(fxrec->fxdata.draydata.rast);
    cone.RenderCone(&pos_top, &fxrec->pos_A[0]);
}

static void NCIN_MaryBoom(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        if (fxrec->fxdata.hazdata.npchaz != NULL)
        {
            fxrec->fxdata.hazdata.npchaz->MarkForRecycle();
        }
        return;
    }

    if (!(fxrec->flg_stat & 2))
    {
        return;
    }

    NPCHazard* haz = HAZ_Acquire();

    if (haz == NULL)
    {
        return;
    }

    if (!haz->ConfigHelper(NPC_HAZ_TUBELETBLAST))
    {
        return;
    }

    haz->SetNPCOwner(NULL);

    F32 tym = fxrec->tym_end - fxrec->tym_beg;

    haz->Start(&fxrec->pos_A[0], (0.5f > tym) ? 0.5f : tym);
    fxrec->fxdata.hazdata.npchaz = haz;
}

static void NCIN_PeteBonk(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        zNPCRobot_TubeConfetti(&fxrec->pos_A[0]);
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_FireSpiral_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        NPAR_FindParty(NPAR_TYP_TUBESPIRAL)->KillAll();
    }
}

static void NCIN_FireSpiral_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                        U32 animIndex, U32 dataIndex)
{
    F32 tym;

    if (animIndex != 3)
    {
        return;
    }

    xVec3 pos_emit = *(const xVec3*)&animMat[26].pos;

    pos_emit += *(const xVec3*)&animMat->pos;

    xVec3 dir_emit = pos_emit - *(const xVec3*)&animMat->pos;

    dir_emit.y = 0.0f;
    dir_emit.normalize();

    xVec3 vel = dir_emit * 5.0f;

    tym = fxrec->tym_end - fxrec->tym_beg;

    NPAR_EmitTubeSpiralCin(&pos_emit, &vel, MAX(0.25f, MIN(tym, 1.0f)));
}

static void NCIN_ShieldPop(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        NPCC_BurstBubble(NPC_BURST_SHIELD, &fxrec->pos_A[0]);
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_OilHazard(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    S32 i;
    S32 rc;
    F32 tym;

    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        if (fxrec->fxdata.hazdata.npchaz != NULL)
        {
            fxrec->fxdata.hazdata.npchaz->MarkForRecycle();
        }
        return;
    }

    if (!(fxrec->flg_stat & 2))
    {
        return;
    }

    const xVec3 pos_emit = fxrec->pos_A[0];
    xVec3 vel_emit;

    for (i = 0; i < 16; i++)
    {
        vel_emit = g_Y3;
        vel_emit += g_Z3 * ((xrand() & 0x800000) ? 1.0f : -1.0f) *
                    (0.4f * (2.0f * (xurand() - 0.5f)) + 0.25f);
        vel_emit += g_X3 * ((xrand() & 0x800000) ? 1.0f : -1.0f) *
                    (0.4f * (2.0f * (xurand() - 0.5f)) + 0.25f);
        vel_emit.normalize();
        vel_emit *= 15.0f;

        NPAR_EmitOilSplash(&pos_emit, &vel_emit);
    }

    NPCHazard* haz = HAZ_Acquire();

    if (haz == NULL)
    {
        return;
    }

    rc = haz->ConfigHelper(NPC_HAZ_OILSLICK);

    if (rc == 0)
    {
        return;
    }

    haz->SetNPCOwner(NULL);

    xVec3Copy((xVec3*)&haz->mdl_hazard->Mat->up, &g_Y3);
    xVec3Copy((xVec3*)&haz->mdl_hazard->Mat->at, &g_Z3);
    xVec3Copy((xVec3*)&haz->mdl_hazard->Mat->right, &g_NX3);

    tym = fxrec->tym_end - fxrec->tym_beg;

    haz->Start(&fxrec->pos_A[0], (0.5f > tym) ? 0.5f : tym);
    fxrec->fxdata.hazdata.npchaz = haz;
}

static void NCIN_FodProd_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        if (fxrec->fxdata.hazdata.npchaz != NULL)
        {
            fxrec->fxdata.hazdata.npchaz->MarkForRecycle();
        }
        return;
    }

    if (!(fxrec->flg_stat & 2))
    {
        return;
    }

    NPCHazard* haz = HAZ_Acquire();

    if (haz == NULL)
    {
        return;
    }

    if (!haz->ConfigHelper(NPC_HAZ_CATTLEPROD))
    {
        return;
    }

    haz->SetNPCOwner(NULL);

    F32 tym = fxrec->tym_end - fxrec->tym_beg;

    tym = (0.5f > tym) ? 0.5f : tym;

    if (tym < 2.0f)
    {
        haz->custdata.typical.rad_min = 0.01f;
    }
    else
    {
        haz->custdata.typical.rad_min = 0.15f;
    }

    haz->custdata.typical.rad_max = 0.15f;
    haz->custdata.typical.rad_cur = haz->custdata.typical.rad_min;
    haz->Start(&fxrec->pos_A[0], tym);
    fxrec->fxdata.hazdata.npchaz = haz;
}

// This vector was used in a deadstripped function.
// This function is here to force the symbol to be linked.
//
// The target's .rodata carries an unreferenced 12-byte @1756
// = { 0.25f, 0.0f, 0.0f } between tym_anger$1601 and
// vec_offset$1853, so its owner was parsed between
// NCIN_SleepyLamp_AR and NCIN_FodProd_AR.
void __deadstripped2_zNPCFXCinematic()
{
    const xVec3 _1756 = { 0.25f, 0.0f, 0.0f };
}

static void NCIN_FodProd_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                     U32 animIndex, U32 dataIndex)
{
    if (animIndex != 0)
    {
        return;
    }

    static const xVec3 vec_offset = { 0.2f, 0.0f, 0.5f };

    const xMat4x3* mat_bone = (const xMat4x3*)&animMat[5];
    NPCHazard* haz = fxrec->fxdata.hazdata.npchaz;

    if (haz == NULL)
    {
        return;
    }

    xVec3 pos_poke = mat_bone->pos;

    pos_poke += mat_bone->right * vec_offset.x;
    pos_poke += mat_bone->up * vec_offset.y;
    pos_poke += mat_bone->at * vec_offset.z;
    pos_poke += *(const xVec3*)&animMat->pos;

    haz->PosSet(&pos_poke);
}

static void NCIN_FodProdBone_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        if (fxrec->fxdata.hazdata.npchaz != NULL)
        {
            fxrec->fxdata.hazdata.npchaz->MarkForRecycle();
        }
        return;
    }

    if (!(fxrec->flg_stat & 2))
    {
        return;
    }

    NPCHazard* haz = HAZ_Acquire();

    if (haz == NULL)
    {
        return;
    }

    if (!haz->ConfigHelper(NPC_HAZ_CATTLEPROD))
    {
        return;
    }

    haz->SetNPCOwner(NULL);

    F32 tym = fxrec->tym_end - fxrec->tym_beg;

    tym = (0.5f > tym) ? 0.5f : tym;

    if (tym < 2.0f)
    {
        haz->custdata.typical.rad_min = 0.01f;
    }
    else
    {
        haz->custdata.typical.rad_min = 0.15f;
    }

    haz->custdata.typical.rad_max = 0.15f;
    haz->custdata.typical.rad_cur = haz->custdata.typical.rad_min;
    haz->Start(&fxrec->pos_A[0], tym);
    fxrec->fxdata.hazdata.npchaz = haz;
}

static void NCIN_FodProdBone_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                         U32 animIndex, U32 dataIndex)
{
    xMat4x3* mat_bone;

    U32 ifx = fxrec->pos_A[1].x;

    if (animIndex != ifx)
    {
        return;
    }

    static S32 idx_bonePoke = fxrec->pos_A[1].y;

    NPCHazard* haz = fxrec->fxdata.hazdata.npchaz;
    mat_bone = (xMat4x3*)&animMat[idx_bonePoke];

    if (haz == NULL)
    {
        return;
    }

    static const xVec3 vec_offset = fxrec->pos_A[0];

    xVec3 pos_poke = mat_bone->pos;

    pos_poke += mat_bone->right * vec_offset.x;
    pos_poke += mat_bone->up * vec_offset.y;
    pos_poke += mat_bone->at * vec_offset.z;
    pos_poke += *(const xVec3*)&animMat->pos;

    haz->PosSet(&pos_poke);
}

static void NCIN_MidFish_Upd(const zCutsceneMgr* mgr, NCINEntry* e, S32 i)
{
    if (i != 0)
    {
        e->flg_stat |= 4;
    }
}

static void NCIN_MidFish_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                     U32 animIndex, U32 dataIndex)
{
    static S32 g_idx_handbone[] = { 11, 16, 27, 36, 41, -1 };

    S32* idx;

    if (animIndex == 0)
    {
        for (S32 i = 0; i < 24; i += 2)
        {
            xVec3 pos_emit = *(const xVec3*)&animMat[i].pos + *(const xVec3*)&animMat->pos;
            zFX_SpawnBubbleTrail(&pos_emit, 1);
        }
    }
    else if (animIndex == 2 || animIndex == 3)
    {
        for (idx = g_idx_handbone; *idx >= 0; idx++)
        {
            xVec3 pos_emit = *(const xVec3*)&animMat[*idx].pos + *(const xVec3*)&animMat->pos;
            zFX_SpawnBubbleTrail(&pos_emit, 1);
        }
    }
}

static void NCIN_BombTrail_Upd(const zCutsceneMgr* mgr, NCINEntry* e, S32 i)
{
    if (i != 0)
    {
        e->flg_stat |= 4;
    }
}

static void NCIN_BombTrail_AR(const zCutsceneMgr* mgr, NCINEntry* e, RpAtomic* a, RwMatrixTag* t, U32 i1, U32 i2)
{
    if (i1 == 0x4)
    {
        zFX_SpawnBubbleTrail((const xVec3*)&t->pos, 0x4);
    }
}

static void NCIN_BoneTrail_Upd(const zCutsceneMgr* mgr, NCINEntry* e, S32 i)
{
    if (i != 0)
    {
        e->flg_stat |= 4;
    }
}

static void NCIN_BoneTrail_AR(const zCutsceneMgr* mgr, NCINEntry* e, RpAtomic* a, RwMatrixTag* t, U32 i1, U32 i2)
{
    if (i1 == 0x7)
    {
        zFX_SpawnBubbleTrail((const xVec3*)&t->pos, 0x4);
    }
}

static void NCIN_HookRecoil_Upd(const zCutsceneMgr* mgr, NCINEntry* e, S32 i)
{
    if (i != 0)
    {
        e->flg_stat |= 4;
    }
}

static void NCIN_HookRecoil_AR(const zCutsceneMgr* csnmgr, NCINEntry*, RpAtomic* model,
                        RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    F32 tym = csnmgr->csn->Time;
    S32 idx_anim;

    if (tym < 7.3666672f)
    {
        idx_anim = 5;
    }
    else if (tym < 12.800001f)
    {
        idx_anim = 1;
    }
    else
    {
        idx_anim = 2;
    }

    if (animIndex != idx_anim)
    {
        return;
    }

    U32 num_bones = iModelNumBones(model);

    for (U32 i = 1; i < num_bones; i++)
    {
        xVec3 pos_emit = *(const xVec3*)&animMat[i].pos + *(const xVec3*)&animMat->pos;
        zFX_SpawnBubbleTrail(&pos_emit, 1);
    }
}

static void NCIN_Lightnin2Bones_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    _tagLightningAdd addInfo;
    xVec3 pnt;

    if (killit != 0)
    {
        if (fxrec->fxdata.arcdata.lightning != NULL)
        {
            zLightningKill(fxrec->fxdata.arcdata.lightning);
        }

        fxrec->flg_stat |= 4;
        return;
    }

    if (!(fxrec->flg_stat & 2))
    {
        return;
    }

    xVec3Init(&pnt, 0.0f, 0.0f, 0.0f);

    addInfo.type = 3;
    addInfo.thickness = 1.0f;
    addInfo.flags = 0x10;
    addInfo.color.r = 200;
    addInfo.color.g = 200;
    addInfo.color.b = 200;
    addInfo.color.a = 255;
    addInfo.time = 1.0f;
    addInfo.start = &pnt;
    addInfo.end = &pnt;

    fxrec->fxdata.arcdata.lightning = zLightningAdd(&addInfo);
}

static void NCIN_Lightnin2Bones_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                            U32 animIndex, U32 dataIndex)
{
    xVec3 begpos;
    xVec3 endpos;
    zLightning* light = fxrec->fxdata.arcdata.lightning;

    if (light == NULL)
    {
        return;
    }

    S32 ifx = fxrec->pos_A[1].x;

    if (animIndex == ifx)
    {
        S32 boneIndex = fxrec->pos_A[1].y;
        const xMat3x3* bone = (const xMat3x3*)&animMat[boneIndex];

        xMat3x3RMulVec(&begpos, bone, &fxrec->pos_A[0]);
        xVec3AddTo(&begpos, (const xVec3*)&animMat[boneIndex].pos);

        if (boneIndex != 0)
        {
            xVec3AddTo(&begpos, (const xVec3*)&animMat->pos);
        }

        zLightningModifyEndpoints(light, &begpos, &light->func.endPoint[1]);
    }

    S32 ifx2 = fxrec->pos_B[1].x;

    if (animIndex == ifx2)
    {
        S32 boneIndex = fxrec->pos_B[1].y;
        const xMat3x3* bone = (const xMat3x3*)&animMat[boneIndex];

        xMat3x3RMulVec(&endpos, bone, &fxrec->pos_B[0]);
        xVec3AddTo(&endpos, (const xVec3*)&animMat[boneIndex].pos);

        if (boneIndex != 0)
        {
            xVec3AddTo(&endpos, (const xVec3*)&animMat->pos);
        }

        zLightningModifyEndpoints(light, &light->func.endPoint[0], &endpos);
    }
}

static void NCIN_LightninBone_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    _tagLightningAdd addInfo;
    xVec3 pnt;

    if (killit != 0)
    {
        if (fxrec->fxdata.arcdata.lightning != NULL)
        {
            zLightningKill(fxrec->fxdata.arcdata.lightning);
        }

        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        xVec3Init(&pnt, 0.0f, 0.0f, 0.0f);

        addInfo.type = 3;
        addInfo.thickness = 1.0f;
        addInfo.flags = 0x100;
        addInfo.color.r = 200;
        addInfo.color.g = 200;
        addInfo.color.b = 200;
        addInfo.color.a = 255;
        addInfo.time = 0.25f * xurand() + 0.05f;
        addInfo.start = &pnt;
        addInfo.end = &pnt;

        fxrec->fxdata.arcdata.lightning = zLightningAdd(&addInfo);

        fxrec->fxdata.arcdata.endPos.x = fxrec->pos_B[0].x * xurand() + fxrec->pos_B[1].x;
        fxrec->fxdata.arcdata.endPos.y = fxrec->pos_B[0].y * xurand() + fxrec->pos_B[1].y;
        fxrec->fxdata.arcdata.endPos.z = fxrec->pos_B[0].z * xurand() + fxrec->pos_B[1].z;
    }

    zLightning* light = fxrec->fxdata.arcdata.lightning;

    if (light == NULL)
    {
        return;
    }

    if (light->flags & 0x40)
    {
        return;
    }

    fxrec->fxdata.arcdata.endPos.x = fxrec->pos_B[0].x * xurand() + fxrec->pos_B[1].x;
    fxrec->fxdata.arcdata.endPos.y = fxrec->pos_B[0].y * xurand() + fxrec->pos_B[1].y;
    fxrec->fxdata.arcdata.endPos.z = fxrec->pos_B[0].z * xurand() + fxrec->pos_B[1].z;

    light->time_total = 0.25f * xurand() + 0.05f;
    light->time_left = light->time_total;

    zLightningShow(light, 1);
}

static void NCIN_LightninBone_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                          U32 animIndex, U32 dataIndex)
{
    xVec3 pnt1;
    xVec3 pnt2;
    S32 ifx = fxrec->pos_A[1].x;

    if (animIndex != ifx)
    {
        return;
    }

    S32 boneIndex = fxrec->pos_A[1].y;
    const xMat3x3* bone = (const xMat3x3*)&animMat[boneIndex];

    xMat3x3RMulVec(&pnt1, bone, &fxrec->pos_A[0]);
    xVec3AddTo(&pnt1, (const xVec3*)&animMat[boneIndex].pos);

    if (boneIndex != 0)
    {
        xVec3AddTo(&pnt1, (const xVec3*)&animMat->pos);
    }

    xMat3x3RMulVec(&pnt2, bone, &fxrec->fxdata.arcdata.endPos);
    xVec3AddTo(&pnt2, &pnt1);
    zLightningModifyEndpoints(fxrec->fxdata.arcdata.lightning, &pnt1, &pnt2);
}

static void NCIN_B101Shockwave_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    zParEmitter* emitter = fxrec->fxdata.pardata.emitter;

    if (killit != 0)
    {
        if (emitter != NULL)
        {
            emitter->emit_flags &= ~1;
        }

        fxrec->flg_stat |= 4;
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        emitter = zParEmitterFind("PAREMIT_SHOCKWAVE");

        if (emitter != NULL)
        {
            fxrec->fxdata.pardata.emitter = emitter;
            emitter->tasset->e_circle.radius = 1.0f;
            xVec3Copy(&emitter->tasset->pos, &fxrec->pos_A[0]);
        }
        else
        {
            fxrec->flg_stat |= 4;
            return;
        }
    }

    emitter->tasset->e_circle.radius +=
        20.0f * globals.update_dt / xsqrt(emitter->tasset->e_circle.radius);
    emitter->emit_flags |= 1;

    if (emitter->tasset->e_circle.radius > 10.0f)
    {
        fxrec->flg_stat |= 4;
        emitter->emit_flags &= ~1;
    }
}

static void NCIN_FreezeBreath_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        StopFreezeBreath();
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        StartFreezeBreath();
    }
}

static void NCIN_FreezeBreath_AR(const zCutsceneMgr* csnmgr, NCINEntry* fxrec, RpAtomic*,
                          RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    xVec3 pnt1;
    xVec3 pnt2;

    S32 ifx = fxrec->pos_A[1].x;

    if (animIndex != ifx)
    {
        return;
    }

    S32 boneIndex = fxrec->pos_A[1].y;
    const xMat3x3* bone = (const xMat3x3*)&animMat[boneIndex];

    xMat3x3RMulVec(&pnt1, bone, &fxrec->pos_A[0]);
    xVec3AddTo(&pnt1, (const xVec3*)&animMat[boneIndex].pos);

    if (boneIndex != 0)
    {
        xVec3AddTo(&pnt1, (const xVec3*)&animMat->pos);
    }

    xMat3x3RMulVec(&pnt2, bone, &fxrec->pos_B[0]);

    EmitFreezeBreath(&pnt1, &pnt2, globals.update_dt,
                     csnmgr->csn->Time - fxrec->tym_beg, fxrec->tym_end - fxrec->tym_beg);
}

static void NCIN_B201HideBelt_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    S32 i;

    if (killit != 0)
    {
        for (i = 0; i < 4; i++)
        {
            xEntShow(fxrec->fxdata.entdata.ent[i]);
        }

        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.entdata.ent[0] = (xEnt*)zSceneFindObject(xStrHash("CONVEYOR BELT 03"));
        fxrec->fxdata.entdata.ent[1] = (xEnt*)zSceneFindObject(xStrHash("ROLLER_MECH 07"));
        fxrec->fxdata.entdata.ent[2] = (xEnt*)zSceneFindObject(xStrHash("ROLLER_MECH 08"));
        fxrec->fxdata.entdata.ent[3] = (xEnt*)zSceneFindObject(xStrHash("ROLLER_MECH 09"));

        for (i = 0; i < 4; i++)
        {
            xEntHide(fxrec->fxdata.entdata.ent[i]);
        }
    }
}

static void NCIN_GooLever_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    zParEmitter* emitter = fxrec->fxdata.pardata.emitter;

    if (killit != 0)
    {
        if (emitter != NULL)
        {
            emitter->prop->rate.val[0] = 0.0f;
        }

        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        emitter = zParEmitterFind("FUDGE_EMIT");

        if (emitter != NULL)
        {
            fxrec->fxdata.pardata.emitter = emitter;
            emitter->prop->rate.val[0] = 0.0f;
        }
        else
        {
            fxrec->flg_stat |= 4;
        }
    }
}

static void NCIN_GooLever_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic*, RwMatrixTag* animMat,
                      U32 animIndex, U32 dataIndex)
{
    S32 ifx = fxrec->pos_A[1].x;

    if (animIndex != ifx)
    {
        return;
    }

    S32 ify = fxrec->pos_A[1].y;
    F32 fudgeRate = 400.0f * -animMat[ify].at.y;

    if (fudgeRate < 0.0f)
    {
        fudgeRate = 0.0f;
    }

    if (fxrec->fxdata.pardata.emitter != NULL)
    {
        fxrec->fxdata.pardata.emitter->prop->rate.val[0] = fudgeRate;
    }
}

static void NCIN_PatBossShrapnel_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        zShrapnelAsset* sasset = (zShrapnelAsset*)xSTFindAsset(xStrHash("boss_pa_shrapnel"), NULL);

        if (sasset == NULL)
        {
            fxrec->flg_stat |= 4;
        }
        else
        {
            fxrec->fxdata.shrapdata.shrap = sasset;
        }
    }
    else
    {
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_PatBossShrapnel_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model,
                             RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    S32 ifx = fxrec->pos_A[1].x;

    if (animIndex == ifx)
    {
        zShrapnel_CinematicInit(fxrec->fxdata.shrapdata.shrap, model, animMat, NULL, NULL);
    }
}

static void NCIN_SpatGlow_Upd(const zCutsceneMgr* mgr, NCINEntry* e, S32 i)
{
    if (i != 0)
    {
        e->flg_stat |= 4;
    }
}

static void NCIN_SpatGlow_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model, RwMatrixTag* animMat,
                      U32 animIndex, U32 dataIndex)
{
    S32 ifx = fxrec->pos_A[1].x;

    if (animIndex != ifx)
    {
        return;
    }

    S32 boneIndex = fxrec->pos_A[1].y;
    iColor_tag color;

    zShrapnel_CinematicInit(fxrec->fxdata.shrapdata.shrap, model, animMat, NULL, NULL);

    color.r = (U8)fxrec->pos_B[0].x;
    color.g = (U8)fxrec->pos_B[0].y;
    color.b = (U8)fxrec->pos_B[0].z;
    color.a = (U8)fxrec->pos_B[1].x;

    xFXAuraAdd(fxrec, (xVec3*)&animMat[boneIndex].pos, &color, fxrec->pos_B[1].y);
}

static void NCIN_GloveShrapnel_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
    }
    else if (fxrec->flg_stat & 2)
    {
        zShrapnelAsset* sasset = (zShrapnelAsset*)xSTFindAsset(xStrHash("g-love_shrapnel"), NULL);

        if (sasset == NULL)
        {
            fxrec->flg_stat |= 4;
        }
        else
        {
            fxrec->fxdata.shrapdata.shrap = sasset;
        }
    }
    else
    {
        fxrec->flg_stat |= 4;
    }
}

static void NCIN_GloveShrapnel_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model,
                           RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    S32 ifx = fxrec->pos_A[1].x;

    if (animIndex == ifx)
    {
        zShrapnel_CinematicInit(fxrec->fxdata.shrapdata.shrap, model, animMat, NULL, NULL);
        zShrapnel_CinematicInit(fxrec->fxdata.shrapdata.shrap, model, animMat, NULL, NULL);
        zShrapnel_CinematicInit(fxrec->fxdata.shrapdata.shrap, model, animMat, NULL, NULL);
    }
}

static void NCIN_EntityBonePar_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model,
                           RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    F32 dt;
    xParEmitter* pe;
    xParEmitterAsset* a;
    xParEmitterPropsAsset* prop;
    xParSys* ps;
    xParGroup* g;
    S32 rate_has_elapsed;
    F32 rate;
    S32 count;
    F32 life;
    F32 size_birth;
    F32 size_death;
    xMat4x3 bone_mat;
    S32 i;
    xPar* p;
    S32 c;
    F32 fc1;
    F32 fc2;

    S32 ifx = fxrec->pos_A[1].x;

    if (ifx != (S32)animIndex)
    {
        return;
    }

    clamp_bone_index(fxrec, model);

    pe = fxrec->fxdata.pardata.emitter;
    dt = globals.update_dt;

    if (pe == NULL)
    {
        return;
    }

    ps = pe->parSys;
    a = pe->tasset;
    prop = pe->prop;

    if (ps == NULL)
    {
        return;
    }

    g = ps->group;

    if (g == NULL)
    {
        return;
    }

    if (a->emit_type != eParEmitterEntityBone)
    {
        return;
    }

    pe->rate_time = pe->rate_time + dt;
    rate_has_elapsed = pe->rate_time > prop->rate.freq;

    if (prop->rate.freq == 0.0f)
    {
        pe->rate_time = 0.0f;
    }

    while (pe->rate_time > prop->rate.freq)
    {
        pe->rate_time -= prop->rate.freq;
    }

    rate = xParInterpCompute(pe->rate_mode, &prop->rate, pe->rate_time, rate_has_elapsed,
                             pe->rate);
    pe->rate = rate;
    pe->rate_fraction = pe->rate_fraction + rate * dt;
    pe->rate_fraction_cull = pe->rate_fraction_cull + rate * dt;

    count = std::floorf(pe->rate_fraction);

    if (count <= 0)
    {
        return;
    }

    pe->rate_fraction = pe->rate_fraction - count;

    if (ps->tasset->maxPar != 0)
    {
        if (g->m_num_of_particles >= (S32)ps->tasset->maxPar)
        {
            return;
        }

        if (ps->group->m_num_of_particles + count >= ps->tasset->maxPar)
        {
            count = ps->tasset->maxPar - ps->group->m_num_of_particles;
        }
    }

    get_bone_matrix(bone_mat, fxrec, animMat);

    const xVec3 oldloc = fxrec->pos_B[0];
    xVec3 loc;
    xVec3 vel;

    xParEmitterTransformEntBone(loc, vel, *a, bone_mat);

    fxrec->pos_B[0] = loc;
    vel *= dt;

    xVec3 emitvel;

    if (pe->emit_flags & 8)
    {
        emitvel = loc - oldloc;

        if (emitvel.length2() > 25.0f)
        {
            emitvel = 0.0f;
        }
    }
    else
    {
        emitvel = 0.0f;
    }

    for (i = 0; i < count; i++)
    {
        p = xParGroupAddPar(ps->group);

        if (p == NULL)
        {
            return;
        }

        life = xParInterpCompute(prop->life.interp, &prop->life, pe->rate_time, 1, 0.0f);
        size_birth = xParInterpCompute(prop->size_birth.interp, &prop->size_birth, pe->rate_time,
                                       1, 0.0f);
        size_death = xParInterpCompute(prop->size_death.interp, &prop->size_death, pe->rate_time,
                                       1, 0.0f);

        p->m_lifetime = life;
        p->totalLifespan = life;
        p->m_size = size_birth;
        p->m_sizeVel = (size_death - size_birth) / life;
        p->m_flag = 0;
        p->m_rotdeg[0] = pe->rot[0];
        p->m_rotdeg[1] = pe->rot[1];
        p->m_rotdeg[2] = pe->rot[2];

        for (c = 0; c < 4; c++)
        {
            fc1 = xParInterpCompute(prop->color_birth[c].interp, &prop->color_birth[c],
                                    pe->rate_time, 1, 0.0f);
            fc2 = xParInterpCompute(prop->color_death[c].interp, &prop->color_death[c],
                                    pe->rate_time, 1, 0.0f);

            p->m_cfl[c] = fc1;
            p->m_c[c] = (U8)(S32)fc1;
            p->m_cvel[c] = (fc2 - fc1) / life;
        }

        p->m_pos = loc;
        xParEmitterEmitSetTexIdxs(p, ps);
        p->m_vel = vel;
        xParEmitterEmitEntBone(p, a, dt, bone_mat);
        p->m_vel += emitvel;
    }
}

void get_bone_matrix(xMat4x3& mat, const NCINEntry* fxrec, const RwMatrixTag* animMat)
{
    S32 idx = fxrec->pos_A[1].y;

    mat = *(const xMat4x3*)&animMat[idx];

    if (idx != 0)
    {
        mat.pos += *(const xVec3*)&animMat->pos;
    }
}

void clamp_bone_index(NCINEntry*, RpAtomic*)
{
}

static void NCIN_BubbleTrail_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;
        return;
    }

    if (!(fxrec->flg_stat & 2))
    {
        return;
    }

    fxrec->fxdata.customdata.v[0].x = 1e38f;
    fxrec->fxdata.customdata.f[0] = 0.0f;
}

static void NCIN_BubbleTrail_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model,
                         RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    S32 ifx = fxrec->pos_A[1].x;

    if (ifx != (S32)animIndex)
    {
        return;
    }

    clamp_bone_index(fxrec, model);

    xVec3& offset = fxrec->pos_A[0];
    xVec3& root_vel = fxrec->pos_B[0];
    F32 rand_diameter = fxrec->pos_B[1].x;
    F32 rand_vel = fxrec->pos_B[1].y;
    F32 scale = fxrec->pos_B[1].z;
    NCINCustom& data = fxrec->fxdata.customdata;
    xVec3& oldloc = data.v[0];
    xVec3& oldvel = data.v[1];
    S32 total;
    xMat4x3 mat;
    xVec3 vel;
    xVec3 loc;

    data.f[0] = data.f[0] + fxrec->pos_A[1].z * globals.update_dt;

    total = data.f[0];

    if (total <= 0)
    {
        return;
    }

    data.f[0] = data.f[0] - total;

    get_bone_matrix(mat, fxrec, animMat);
    xMat3x3RMulVec(&vel, (const xMat3x3*)&mat, &root_vel);
    xMat4x3Toworld(&loc, &mat, &offset);

    if (oldloc.x >= 1e38f)
    {
        oldloc = loc;
        oldvel = vel;
    }

    xVec3 rloc = { 0.0f, 0.0f, 0.0f };
    xVec3 rvel = { 0.0f, 0.0f, 0.0f };

    rloc.x = rand_diameter;
    rloc.y = rand_diameter;
    rloc.z = rand_diameter;

    rvel.x = rand_vel;
    rvel.y = rand_vel;
    rvel.z = rand_vel;

    zFX_SpawnBubbleTrail(&oldloc, &loc, &oldvel, &vel, total, &rloc, &rvel, scale);

    oldloc = loc;
    oldvel = vel;
}

static void NCIN_SBBNode_Upd(const zCutsceneMgr*, NCINEntry* fxrec, S32 killit)
{
    if (killit != 0)
    {
        fxrec->flg_stat |= 4;

        zNPCB_SB2* sb2 = zNPCB_SB2::singleton();

        if (sb2 != NULL)
        {
            sb2->bind_nodes();
        }

        if (fxrec->fxdata.matdata.mat != NULL)
        {
            xMemPopTemp(fxrec->fxdata.matdata.mat);
        }
        return;
    }

    if (fxrec->flg_stat & 2)
    {
        fxrec->fxdata.matdata.mat = NULL;
    }
}

static void NCIN_SBBNode_AR(const zCutsceneMgr*, NCINEntry* fxrec, RpAtomic* model,
                     RwMatrixTag* animMat, U32 animIndex, U32 dataIndex)
{
    zNPCB_SB2* sb2;
    S32 bones;
    xMat3x3 imat;
    S32 i;

    S32 ifx = fxrec->pos_A[1].x;

    if (ifx != (S32)animIndex)
    {
        return;
    }

    sb2 = zNPCB_SB2::singleton();

    if (sb2 == NULL)
    {
        return;
    }

    bones = 0;

    if (model->geometry != NULL)
    {
        RpSkin* skin = RpSkinGeometryGetSkin(model->geometry);

        if (skin != NULL)
        {
            bones = RpSkinGetNumBones(skin);
        }
    }

    NCINMat& data = fxrec->fxdata.matdata;

    if (fxrec->flg_stat & 8)
    {
        data.mat = (RwMatrixTag*)xMemPushTemp((bones + 1) * sizeof(RwMatrixTag));
    }

    sb2->rebind_nodes(model, data.mat);

    xMat3x3Transpose(&imat, (const xMat3x3*)animMat);

    for (i = 1; i < bones; i++)
    {
        xMat3x3Mul((xMat3x3*)&data.mat[i], (const xMat3x3*)&animMat[i], &imat);
        data.mat[i].pos = animMat[i].pos;
    }

    *data.mat = *animMat;

    sb2->move_nodes();
    sb2->render_nodes();
}

void xCutscene::NoseyClear()
{
    this->NoseySet(0);
}

void xCutscene::NoseySet(XCSNNosey* nosey)
{
    this->cb_nosey = nosey;
}

void NPCCone::TextureSet(RwRaster* raster)
{
    rast_cone = raster;
}

void NPCCone::UVSliceSet(F32 u, F32 v)
{
    this->uv_slice[0] = u;
    this->uv_slice[1] = v;
}

void NPCCone::UVBaseSet(F32 u, F32 v)
{
    this->uv_tip[0] = u;
    this->uv_tip[1] = v;
}

void NPCCone::ColorSet(RwRGBA top, RwRGBA bot)
{
    this->rgba_top = top;
    this->rgba_bot = bot;
}

void NPCCone::RadiusSet(F32 conefloat)
{
    rad_cone = conefloat;
}

void NPARMgmt::KillAll()
{
    this->cnt_active = 0;
}

// This string was used in a deadstripped function.
// This function is here to force the symbol to be linked.
//
// "fx_pickup_emphasis" is the last entry in the target's
// @stringBase0 and nothing references it, so its owner was
// parsed after every other string user in the unit.
const char* __deadstripped3_zNPCFXCinematic()
{
    return "fx_pickup_emphasis";
}
