#include "zNPCHazard.h"

#include <types.h>
#include <string.h>

#include "zGlobals.h"
#include "zNPCTypeCommon.h"
#include "zNPCTypes.h"
#include "zNPCSupplement.h"
#include "zNPCSupport.h"
#include "xMath.h"
#include "xMathInlines.h"
#include "xUtil.h"
#include "xordarray.h"
#include "zRenderState.h"

extern const xVec3 g_O3;

// These structs were used in deadstripped functions.
// This function is here to force the symbols to be linked.
void __deadstripped_zNPCHazard()
{
    const char _446[0x0C] = {};
    const char _447[0x0C] = {};
    const char _451[0x0C] = {};

    const char _461[0x28] = {};
    const char _462[0x28] = {};
    const char _463[0x28] = {};
    const char _464[0x28] = {};
    const char _465[0x28] = {};
    const char _466[0x28] = {};
    const char _467[0x28] = {};

    const char _499[0x0C] = {};
}

// .data order below is the target's: every one of these is a definition in this
// TU, not an import.
static RpAtomic* g_hazard_rawModel[30] = { NULL };

static char* g_strz_hazModel[30] = {
    "fx_boomball_bubble",       "fx_boomball_smoke",
    "fx_fodbomb.dff",           "fx_tubelet_blast.dff",
    "fx_duplotron_blast.dff",   "fx_cattleprod.dff",
    "fx_proj_tartar.dff",       "fx_tartar_splat.dff",
    "fx_steam.dff",             "fx_proj_missile.dff",
    "fx_chuck_splish.dff",      "fx_chuck_splash.dff",
    "fx_proj_bone.dff",         "fx_proj_slick_bubble.dff",
    "fx_oil_puddle.dff",        "fx_oil_burst.dff",
    "fx_oil_glob.dff",          "fx_proj_cloud.dff",
    "frag_generic_wrench",      "frag_generic_joystick.dff",
    "frag_generic_sink.dff",    "frag_generic_duck.dff",
    "frag_generic_bra.dff",     "frag_generic_headphones.dff",
    "frag_generic_cellphone.dff", "frag_generic_shoe.dff",
    "fx_duplotron_blast.dff",   "fx_robobits.dff",
    "fx_vissplash_wave.dff",    "fx_arfdog_nukem.dff",
};

static U32 g_hash_hazanim[3] = { 0, 0, 0 };
static char* g_strz_hazanim[3] = { "Unknown", "Idle01", "Active01" };

static NPCHazard* g_haz_uvAnimQue[27] = { NULL };

static en_hazmodel g_haz_uvModelTypes[] = {
    NPC_HAZMDL_CATTLEPROD,      NPC_HAZMDL_TARTARSTEAM,     NPC_HAZMDL_SLICKPROJ,
    NPC_HAZMDL_SLICKPUDDLE,     NPC_HAZMDL_SLICKBURST,      NPC_HAZMDL_SLICKGLOB,
    NPC_HAZMDL_TUBEBLAST,       NPC_HAZMDL_CHUCKSPLISH,     NPC_HAZMDL_CHUCKSPLASH,
    NPC_HAZMDL_VISSPLASH,       NPC_HAZMDL_PUPPYNUKE,       NPC_HAZMDL_FODBOMB,
    NPC_HAZMDL_BOOMBALL_BUBBLE, NPC_HAZMDL_BOOMBALL_SMOKE,  NPC_HAZMDL_FORCE,
};

static en_hazmodel g_funfrag_choices[8] = {
    NPC_HAZMDL_FUNFRAG_WRENCH,    NPC_HAZMDL_FUNFRAG_JOYSTICK, NPC_HAZMDL_FUNFRAG_SINK,
    NPC_HAZMDL_FUNFRAG_DUCK,      NPC_HAZMDL_FUNFRAG_BRA,      NPC_HAZMDL_FUNFRAG_HEADPHONES,
    NPC_HAZMDL_FUNFRAG_CELLPHONE, NPC_HAZMDL_FUNFRAG_SHOE,
};

static zShrapnelAsset* g_data_hazshrap[5] = { NULL, NULL, NULL, NULL, NULL };

char* g_strz_hazshrap[5] = {
    "", "tartar_gunshot", "tartar_splatter", "slick_oilspill", "shrapnel_splash_water",
};

static RwRaster* g_rast_hazshad[30] = { NULL };

char* g_strz_hazshad[30] = {
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "shadow_monsoon_cloud", "", "", "", "", "", "", "", "", "", "", "", "",
};

static NPCHazard g_hazards[64] = { NPC_HAZ_UNKNOWN };
static xAnimTable* g_haz_animTable[30];
static UVAModelInfo g_haz_uvAnimInfo[30];

static S32 g_cnt_activehaz;
static xParEmitterCustomSettings g_parf_default;
static xParEmitterCustomSettings g_parf_zapwarn;
static xParEmitterCustomSettings g_parf_zapwave;
static xParEmitterCustomSettings g_parf_zaprain;
static zParEmitter* g_pemit_default;
static zParEmitter* g_pemit_zapwarn;
static zParEmitter* g_pemit_zapwave;
static zParEmitter* g_pemit_zaprain;

// These two are `inline` here rather than in a header: the target emits them
// as weak per-TU copies (scope:weak in this object only), which is what an
// out-of-line copy of an inline function looks like.
inline xVec3* LERP(F32 t, xVec3* dst, const xVec3* a, const xVec3* b)
{
    dst->x = LERP(t, a->x, b->x);
    dst->y = LERP(t, a->y, b->y);
    dst->z = LERP(t, a->z, b->z);
    return dst;
}

inline xVec3* SMOOTH(F32 t, xVec3* dst, const xVec3* a, const xVec3* b)
{
    return LERP(EASE(t), dst, a, b);
}

void zNPCHazard_Startup()
{
    for (S32 i = 0; i < 3; i++)
    {
        g_hash_hazanim[i] = xStrHash(g_strz_hazanim[i]);
    }
}

void zNPCHazard_Shutdown()
{
}

void zNPCHazard_ScenePrepare()
{
    S32 i;

    memset(g_hazards, 0, sizeof(g_hazards));
    g_cnt_activehaz = 0;
    for (i = 0; i < 27; i++)
    {
        g_haz_uvAnimQue[i] = NULL;
    }
    for (i = 0; i < 30; i++)
    {
        g_haz_uvAnimInfo[i].Clear();
    }
    for (i = 0; i < 27; i++)
    {
        g_haz_animTable[i] = NULL;
    }
    for (i = 0; i < 30; i++)
    {
        g_hazard_rawModel[i] = NULL;
    }
}

void zNPCHazard_SceneFinish()
{
    S32 i;

    zNPCHazard_SceneReset();
    zNPCHazard_KillEffects();

    memset(g_hazards, 0, sizeof(g_hazards));
    g_cnt_activehaz = 0;

    for (i = 0; i < 27; i++)
    {
        g_haz_uvAnimQue[i] = NULL;
    }
    for (i = 0; i < 30; i++)
    {
        g_haz_uvAnimInfo[i].Hemorrage();
    }
    for (i = 0; i < 27; i++)
    {
        g_haz_animTable[i] = NULL;
    }
    for (i = 0; i < 30; i++)
    {
        g_hazard_rawModel[i] = NULL;
    }
}

void zNPCHazard_SceneReset()
{
    for (S32 i = 0; i < 64; i++)
    {
        if (g_hazards[i].flg_hazard)
        {
            g_hazards[i].Kill();
        }
    }
    g_cnt_activehaz = 0;
}

void zNPCHazard_ScenePostInit()
{
    zNPCHazard_InitEffects();
}

void zNPCHazard_InitEffects()
{
    S32 i;

    g_pemit_default = zParEmitterFind("PAREMIT_CLOUD");
    g_pemit_zapwarn = zParEmitterFind("PAREMIT_ROMON_ZAPWARN");
    g_pemit_zapwave = zParEmitterFind("PAREMIT_ROMON_ZAPWAVE");
    g_pemit_zaprain = zParEmitterFind("PAREMIT_DUPLO_STEAM");

    g_parf_default.custom_flags = 0x100;
    xVec3Copy(&g_parf_default.pos, &g_O3);
    g_parf_zapwarn.custom_flags = 0x100;
    xVec3Copy(&g_parf_zapwarn.pos, &g_O3);
    g_parf_zapwave.custom_flags = 0x100;
    xVec3Copy(&g_parf_zapwave.pos, &g_O3);
    g_parf_zaprain.custom_flags = 0x100;
    xVec3Copy(&g_parf_zaprain.pos, &g_O3);

    for (i = 0; i < 30; i++)
    {
        char* namez = g_strz_hazModel[i];

        if (namez != NULL && namez[0] != '\0')
        {
            U32 hashy = xStrHash(namez);
            if (hashy != 0)
            {
                g_hazard_rawModel[i] = (RpAtomic*)xSTFindAsset(hashy, NULL);
            }
        }
    }

    i = 0;
    while (g_haz_uvModelTypes[i] != NPC_HAZMDL_FORCE)
    {
        en_hazmodel which = g_haz_uvModelTypes[i++];
        RpAtomic* raw_model = g_hazard_rawModel[which];

        if (raw_model != NULL)
        {
            UVAModelInfo* uva = &g_haz_uvAnimInfo[which];

            uva->Init(raw_model, 0);
            uva->UVVelSet(0.0f, 1.0f);
        }
    }

    for (i = 0; i < 5; i++)
    {
        g_data_hazshrap[i] = NULL;

        char* namez = g_strz_hazshrap[i];
        if (namez != NULL && namez[0] != '\0')
        {
            U32 hashy = xStrHash(namez);
            if (hashy != 0)
            {
                U32 size = 0;
                void* asset = xSTFindAsset(hashy, &size);

                if (asset != NULL && size != 0)
                {
                    g_data_hazshrap[i] = (zShrapnelAsset*)asset;
                }
            }
        }
    }

    for (i = 0; i < 30; i++)
    {
        g_rast_hazshad[i] = NULL;

        const char* namez = g_strz_hazshad[i];
        if (namez != NULL && namez[0] != '\0')
        {
            g_rast_hazshad[i] = NPCC_FindRWRaster(namez);
        }
    }
}

void zNPCHazard_KillEffects()
{
}

S32 HAZ_ord_sorttest(void* vkey, void* vitem)
{
    NPCHazard* key = (NPCHazard*)vkey;
    NPCHazard* item = (NPCHazard*)vitem;
    if (key->typ_hazard < item->typ_hazard)
    {
        return -1;
    }
    else if (key->typ_hazard > item->typ_hazard)
    {
        return 1;
    }
    else if ((S32)vkey < (S32)vitem)
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

// Close, kind of.
void zNPCHazard_Timestep(F32 dt)
{
    NPCHazard* haz;
    S32 i;

    if (g_cnt_activehaz < 1)
    {
        return;
    }

    st_XORDEREDARRAY ord_haz;
    XOrdInit(&ord_haz, 64, 1);

    for (i = 0; i < 64; i++)
    {
        haz = &g_hazards[i];

        if (haz->flg_hazard & 1)
        {
            XOrdAppend(&ord_haz, haz);
        }
    }

    XOrdSort(&ord_haz, HAZ_ord_sorttest);

    for (i = 0; i < ord_haz.cnt; i++)
    {
        NPCHazard* hz = (NPCHazard*)ord_haz.list[i];
        S32 flg = hz->flg_hazard;

        if (flg & 4)
        {
            hz->Discard();
            continue;
        }
        if (!(flg & 2))
        {
            continue;
        }
        if (flg & 0x80)
        {
            hz->Timestep(dt);
        }

        flg = hz->flg_hazard;
        if (!(flg & 0x20))
        {
            hz->flg_hazard = flg & ~0x8;
        }
        hz->flg_hazard &= ~0x60;

        hz->tmr_remain = MAX(-1.0f, hz->tmr_remain - dt);

        if (hz->flg_hazard & 0x1000)
        {
            if (hz->tmr_remain < 0.0f)
            {
                hz->MarkForRecycle();
            }
        }

        flg = hz->flg_hazard;
        if (flg & 4)
        {
            hz->Discard();
            continue;
        }
        if (!(flg & 0x80000))
        {
            continue;
        }

        g_haz_uvAnimQue[hz->typ_hazard] = hz;
    }

    XOrdDone(&ord_haz, 1);

    for (i = 0; i < 27; i++)
    {
        NPCHazard* hz = g_haz_uvAnimQue[i];
        if (hz != NULL)
        {
            UVAModelInfo* uva = hz->uva_uvanim;
            g_haz_uvAnimQue[i] = NULL;
            if (uva != NULL)
            {
                uva->Update(dt, NULL);
            }
        }
    }
}

void zNPCCommon_Hazards_RenderAll(S32 doOpaqueStuff)
{
    S32 i;

    if (g_cnt_activehaz < 1)
    {
        return;
    }

    _SDRenderState rs_old = zRenderStateCurrent();

    if (doOpaqueStuff)
    {
        zRenderState(SDRS_OpaqueModels);
    }
    else
    {
        zRenderState(SDRS_NPCVisual);
    }

    xLightKit_Enable(globals.player.ent.lightKit, globals.currWorld);

    st_XORDEREDARRAY ord_haz;
    XOrdInit(&ord_haz, 64, 1);

    for (i = 0; i < 64; i++)
    {
        NPCHazard* haz = &g_hazards[i];
        S32 flg = haz->flg_hazard;

        if ((flg & 1) && (flg & 0x102) && !(flg & 0xc))
        {
            XOrdAppend(&ord_haz, haz);
        }
    }

    XOrdSort(&ord_haz, HAZ_ord_sorttest);

    for (i = 0; i < ord_haz.cnt; i++)
    {
        NPCHazard* hz = (NPCHazard*)ord_haz.list[i];

        if (doOpaqueStuff && !(hz->flg_hazard & 0x200))
        {
            continue;
        }
        if (!doOpaqueStuff && (hz->flg_hazard & 0x200))
        {
            continue;
        }

        S32 flg = hz->flg_hazard;
        if (flg & 0x800)
        {
            if (flg & 0x400)
            {
                RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
            }
            xLightKit_Enable(NULL, globals.currWorld);
            hz->Render();
            xLightKit_Enable(globals.player.ent.lightKit, globals.currWorld);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
        }
        else
        {
            hz->Render();
        }

        RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)rwCULLMODECULLNONE);
        hz->flg_hazard &= ~0x10;
    }

    XOrdDone(&ord_haz, 1);

    xLightKit_Enable(NULL, globals.currWorld);
    zRenderState(rs_old);
}

NPCHazard* HAZ_Acquire()
{
    NPCHazard* haz_found = NULL;

    for (S32 i = 0; i < 64; i++)
    {
        NPCHazard* da_haz = &g_hazards[i];

        if (!(da_haz->flg_hazard & 1))
        {
            da_haz->WipeIt();
            da_haz->flg_hazard = 1;
            haz_found = da_haz;
            g_cnt_activehaz++;
            break;
        }
    }
    return haz_found;
}

S32 HAZ_AvailablePool()
{
    return 64 - g_cnt_activehaz;
}

void HAZ_Iterate(bool (*fp)(NPCHazard&, void*), void* context, S32 flag_filter)
{
    NPCHazard* haz = g_hazards;
    NPCHazard* end = &g_hazards[64];

    while (haz != end)
    {
        if ((haz->flg_hazard & 0xf) == 3 && (haz->flg_hazard & flag_filter) && !fp(*haz, context))
        {
            return;
        }
        haz++;
    }
}

void NPCHazard::WipeIt()
{
    this->typ_hazard = NPC_HAZ_UNKNOWN;
    this->flg_hazard = 0;
    xVec3Copy(&this->pos_hazard, &g_O3);
    this->tym_lifespan = 1.0f;
    this->tmr_remain = -1.0f;
    this->pam_interp = 0.0f;
    this->cb_notify = NULL;
    this->npc_owner = NULL;
    memset(&this->custdata, 0, sizeof(this->custdata));
}

S32 NPCHazard::ConfigHelper(en_npchaz haztype)
{
    S32 result = 1;

    this->typ_hazard = haztype;

    switch (haztype)
    {
    case NPC_HAZ_UNKNOWN:
        result = 0;
        break;
    case NPC_HAZ_EXPLODE:
    {
        static const xVec2 uv_scroll_eo = { 0.0f, -5.0f };

        this->flg_hazard |= 0x89180;
        if (!this->GrabModel(NPC_HAZMDL_BOOMBALL_SMOKE))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_BOOMBALL_BUBBLE, uv_scroll_eo.x, uv_scroll_eo.y);
        this->tmr_remain = 2.5f;
        this->custdata.typical.rad_min = 0.1f;
        this->custdata.typical.rad_max = 2.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_EXPLODE_INNER:
    {
        static const xVec2 uv_scroll_ei = { 0.0f, -5.0f };

        this->flg_hazard |= 0x89180;
        if (!this->GrabModel(NPC_HAZMDL_BOOMBALL_BUBBLE))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_BOOMBALL_BUBBLE, uv_scroll_ei.x, uv_scroll_ei.y);
        this->tmr_remain = 2.0f;
        this->custdata.typical.rad_min = 0.0f;
        this->custdata.typical.rad_max = 1.5f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_FODBOMB:
    {
        static const xVec2 uv_scroll_fb = { 0.0f, -5.0f };

        this->flg_hazard |= 0x8b180;
        if (!this->GrabModel(NPC_HAZMDL_FODBOMB))
        {
            result = 0;
        }
        this->uva_uvanim = this->GetUVAInfo(NPC_HAZMDL_FODBOMB, uv_scroll_fb.x, uv_scroll_fb.y);
        this->tmr_remain = 0.4f;
        this->custdata.typical.rad_min = 0.3f;
        this->custdata.typical.rad_max = 2.75f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_CATTLEPROD:
    {
        static const xVec2 uv_scroll_cp = { 5.0f, -5.0f };

        this->flg_hazard |= 0x8b180;
        if (!this->GrabModel(NPC_HAZMDL_CATTLEPROD))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_CATTLEPROD, uv_scroll_cp.x, uv_scroll_cp.y);
        this->tmr_remain = 0.2f;
        this->custdata.typical.rad_min = 0.15f;
        this->custdata.typical.rad_max = 0.5f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_TUBELETBLAST:
    {
        static const xVec2 uv_scroll_tb = { 0.0f, -5.0f };

        this->flg_hazard |= 0x8b180;
        if (!this->GrabModel(NPC_HAZMDL_TUBEBLAST))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_TUBEBLAST, uv_scroll_tb.x, uv_scroll_tb.y);
        this->tmr_remain = 1.0f;
        this->custdata.typical.rad_min = 0.25f;
        this->custdata.typical.rad_max = 7.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_PUPPYNUKE:
    {
        static const xVec2 uv_scroll_pn = { 5.0f, -5.0f };

        this->flg_hazard |= 0x8b180;
        if (!this->GrabModel(NPC_HAZMDL_PUPPYNUKE))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_PUPPYNUKE, uv_scroll_pn.x, uv_scroll_pn.y);
        this->tmr_remain = 0.3f;
        this->custdata.typical.rad_min = 0.5f;
        this->custdata.typical.rad_max = 2.5f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_DUPLOBOOM:
    {
        xVec3 ang_spin;

        this->flg_hazard |= 0xb180;
        if (!this->GrabModel(NPC_HAZMDL_DUPLOBOOM))
        {
            result = 0;
        }
        this->tmr_remain = 0.75f;
        this->custdata.typical.rad_min = 1.0f;
        this->custdata.typical.rad_max = 5.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;

        ang_spin.x = 0.016666668f * ((xrand() & 0x800000) ? 3.1415927f : -0.78539819f);
        ang_spin.y = ang_spin.z = 0.0f;
        this->TypData_RotMatStore(&ang_spin);
        break;
    }
    case NPC_HAZ_THUNDER:
    {
        xVec3 ang_spin;

        this->flg_hazard |= 0xb180;
        if (!this->GrabModel(NPC_HAZMDL_THUNDER))
        {
            result = 0;
        }
        this->tmr_remain = 0.25f;
        this->custdata.typical.rad_min = 0.1f;
        this->custdata.typical.rad_max = 5.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;

        ang_spin.x = 0.016666668f * ((xrand() & 0x800000) ? 3.1415927f : -0.78539819f);
        ang_spin.y = ang_spin.z = 0.0f;
        this->TypData_RotMatStore(&ang_spin);
        break;
    }
    case NPC_HAZ_DUPLO_SHROOM:
        this->flg_hazard |= 0x9180;
        this->tmr_remain = 5.0f;
        xVec3Copy(&this->custdata.shroom.vel_rise, &g_Y3);
        xVec3Copy(&this->custdata.shroom.acc_rise, &g_Y3);
        this->TypData_RotMatStore(NULL);
        break;
    case NPC_HAZ_PATRIOT:
        this->flg_hazard |= 0x201180;
        this->tmr_remain = 5.0f;
        xVec3Copy(&this->custdata.patriot.pos_began, &g_O3);
        this->custdata.patriot.spd_peak = 10.0f;
        this->custdata.patriot.spd_curr = 0.0f;
        this->custdata.patriot.acc_rate = 5.0f;
        break;
    case NPC_HAZ_TARTARPROJ:
        this->flg_hazard |= 0x21a380;
        if (!this->GrabModel(NPC_HAZMDL_TARTARPROJ))
        {
            result = 0;
        }
        this->tmr_remain = 0.25f;
        this->custdata.typical.rad_min = 0.2f;
        this->custdata.typical.rad_max = 0.25f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        this->custdata.collide.flg_collide |= 7;
        break;
    case NPC_HAZ_TARTARSPILL:
        this->flg_hazard |= 0xb380;
        if (!this->GrabModel(NPC_HAZMDL_TARTARSPLAT))
        {
            result = 0;
        }
        this->tmr_remain = 4.0f;
        this->custdata.typical.rad_min = 0.2f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_max;
        break;
    case NPC_HAZ_TARTARSTINK:
    {
        static const xVec2 uv_scroll_ts = { 0.0f, 1.0f };

        this->flg_hazard |= 0x89180;
        if (!this->GrabModel(NPC_HAZMDL_TARTARSTEAM))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_TARTARSTEAM, uv_scroll_ts.x, uv_scroll_ts.y);
        this->tmr_remain = 3.75f;
        this->custdata.typical.rad_min = 0.2f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_max;
        break;
    }
    case NPC_HAZ_CHUCKBOMB:
        this->flg_hazard |= 0x21a380;
        this->custdata.collide.flg_collide |= 7;
        if (!this->GrabModel(NPC_HAZMDL_CHUCKBOMB))
        {
            result = 0;
        }
        this->tmr_remain = 0.25f;
        this->custdata.typical.rad_min = 1.0f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    case NPC_HAZ_CHUCKBLAST:
    {
        static const xVec2 uv_scroll_cb = { 0.0f, -1.0f };

        this->flg_hazard |= 0x8b980;
        if (!this->GrabModel(NPC_HAZMDL_CHUCKSPLASH))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_CHUCKSPLASH, uv_scroll_cb.x, uv_scroll_cb.y);
        this->tmr_remain = 1.0f;
        this->custdata.typical.rad_min = 0.0f;
        this->custdata.typical.rad_max = 3.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_CHUCKBLOOSH:
    {
        static const xVec2 uv_scroll_cl = { 0.0f, -1.0f };

        this->flg_hazard |= 0x99180;
        if (!this->GrabModel(NPC_HAZMDL_CHUCKSPLISH))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_CHUCKSPLISH, uv_scroll_cl.x, uv_scroll_cl.y);
        this->tmr_remain = 1.6f + 1.6f * (0.25f * (xurand() - 0.5f));
        this->custdata.typical.rad_min = 0.2f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        this->custdata.tartar.vel = g_Y3;
        break;
    }
    case NPC_HAZ_ARFBONE:
    {
        xVec3 ang_spin;

        this->flg_hazard |= 0x21a380;
        if (!this->GrabModel(NPC_HAZMDL_ARFBONE))
        {
            result = 0;
        }
        this->tmr_remain = 0.25f;
        this->custdata.typical.rad_min = 0.0f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;

        ang_spin.x = 0.016666668f * ((xrand() & 0x800000) ? 12.566371f : -12.566371f);
        ang_spin.y = ang_spin.z = 0.0f;
        this->TypData_RotMatStore(&ang_spin);
        this->custdata.collide.flg_collide |= 7;
        break;
    }
    case NPC_HAZ_ARFBONEBLAST:
        this->flg_hazard |= 0xb180;
        this->tmr_remain = 1.0f;
        this->custdata.typical.rad_min = 0.5f;
        this->custdata.typical.rad_max = 0.5f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    case NPC_HAZ_OILBUBBLE:
    {
        static const xVec2 uv_scroll_ob = { 0.0f, -2.0f };

        this->flg_hazard |= 0x29a180;
        if (!this->GrabModel(NPC_HAZMDL_SLICKPROJ))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_SLICKPROJ, uv_scroll_ob.x, uv_scroll_ob.y);
        this->tmr_remain = 0.25f;
        this->custdata.typical.rad_min = 0.0f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        this->custdata.collide.flg_collide |= 7;
        break;
    }
    case NPC_HAZ_OILSLICK:
    {
        static const xVec2 uv_scroll_os = { 0.0f, -0.25f };

        this->flg_hazard |= 0x8b180;
        if (!this->GrabModel(NPC_HAZMDL_SLICKPUDDLE))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_SLICKPUDDLE, uv_scroll_os.x, uv_scroll_os.y);
        this->tmr_remain = 10.0f;
        this->custdata.typical.rad_min = 0.2f;
        this->custdata.typical.rad_max = 1.5f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_OILBURST:
    {
        static const xVec2 uv_scroll_or = { 0.0f, -0.25f };

        this->flg_hazard |= 0x8b180;
        if (!this->GrabModel(NPC_HAZMDL_SLICKBURST))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_SLICKBURST, uv_scroll_or.x, uv_scroll_or.y);
        this->tmr_remain = 0.75f;
        this->custdata.typical.rad_min = 0.0f;
        this->custdata.typical.rad_max = 1.75f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    case NPC_HAZ_OILGLOB:
    {
        static const xVec2 uv_scroll_og = { -1.0f, -2.0f };

        this->flg_hazard |= 0x8b180;
        if (!this->GrabModel(NPC_HAZMDL_SLICKGLOB))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_SLICKGLOB, uv_scroll_og.x, uv_scroll_og.y);
        this->tmr_remain = 1.2f + 1.2f * (0.25f * (xurand() - 0.5f));
        this->custdata.typical.rad_min = 0.1f;
        this->custdata.typical.rad_max = 0.55f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        xVec3Copy(&this->custdata.shroom.vel_rise, &g_O3);
        xVec3SMul(&this->custdata.shroom.acc_rise, &g_Y3, 1.2f);
        break;
    }
    case NPC_HAZ_MONCLOUD:
    {
        xVec3 ang_spin;

        this->flg_hazard |= 0x9180;
        if (!this->GrabModel(NPC_HAZMDL_MONCLOUD))
        {
            result = 0;
        }
        this->shadowCache = NPCC_ShadowCacheReserve();
        this->tmr_remain = 7.5f;
        this->custdata.cloud.spd_cloud = 3.0f;
        this->custdata.cloud.rad_maxRange = 10.0f;
        this->custdata.typical.rad_min = 0.0f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        xVec3Copy(&this->custdata.cloud.pos_home, &g_O3);

        ang_spin.x = 0.016666668f * ((xrand() & 0x800000) ? 0.78539819f : -0.78539819f);
        ang_spin.y = ang_spin.z = 0.0f;
        this->TypData_RotMatStore(&ang_spin);
        break;
    }
    case NPC_HAZ_FUNFRAG:
        this->flg_hazard |= 0x19380;
        {
            en_hazmodel which = this->PickFunFrag();

            if (!this->GrabModel(which))
            {
                result = 0;
            }
        }
        this->tmr_remain = 2.5f;
        this->custdata.typical.rad_min = 1.0f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        this->TypData_RotMatStore(NULL);
        break;
    case NPC_HAZ_ROBOBITS:
    {
        static const xVec3 vec_tumble = { 6.2831855f, 0.78539819f, 0.78539819f };

        this->flg_hazard |= 0x19380;
        if (!this->GrabModel(NPC_HAZMDL_ROBOBITS))
        {
            result = 0;
        }
        this->tmr_remain = 0.25f;
        this->custdata.typical.rad_min = 1e-05f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;

        xVec3 ang_spin = vec_tumble;
        ang_spin.x *= 2.0f * (xurand() - 0.5f);
        ang_spin.y *= 2.0f * (xurand() - 0.5f);
        ang_spin.z *= 2.0f * (xurand() - 0.5f);
        ang_spin *= 0.016666668f;
        this->TypData_RotMatStore(&ang_spin);
        break;
    }
    case NPC_HAZ_VISSPLASH:
    {
        static const xVec2 uv_scroll_vs = { 0.0f, -1.0f };

        this->flg_hazard |= 0x89980;
        if (!this->GrabModel(NPC_HAZMDL_VISSPLASH))
        {
            result = 0;
        }
        this->uva_uvanim =
            this->GetUVAInfo(NPC_HAZMDL_CHUCKSPLASH, uv_scroll_vs.x, uv_scroll_vs.y);
        this->tmr_remain = 0.25f;
        this->custdata.typical.rad_min = 0.0f;
        this->custdata.typical.rad_max = 1.0f;
        this->custdata.typical.rad_cur = this->custdata.typical.rad_min;
        break;
    }
    default:
        result = 0;
        break;
    }

    if (!result)
    {
        this->MarkForRecycle();
    }

    return result;
}

void NPCHazard::Reconfigure(en_npchaz haztype)
{
    HAZNotify* noter = this->cb_notify;
    zNPCCommon* npc_old = this->npc_owner;
    xVec3 pos_old;
    xVec3Copy(&pos_old, &this->pos_hazard);
    this->Cleanup();
    this->WipeIt();
    this->flg_hazard = 0x21;
    this->ConfigHelper(haztype);
    this->PosSet(&pos_old);
    this->NotifyCBSet(noter);
    this->SetNPCOwner(npc_old);
    if (this->cb_notify != NULL)
    {
        this->cb_notify->Notify(HAZ_NOTE_RECONFIG, this);
    }
}

UVAModelInfo* NPCHazard::GetUVAInfo(en_hazmodel which, F32 uvel, F32 vvel)
{
    UVAModelInfo* uva = &g_haz_uvAnimInfo[which];

    if (!uva->Valid())
    {
        this->flg_hazard &= ~0x80000;
        return NULL;
    }

    uva->UVVelSet(uvel, vvel);
    return uva;
}

S32 NPCHazard::GrabModel(en_hazmodel idx_model)
{
    xVec3 ang_orient = { 0.0f, 0.0f, 0.0f };
    xMat4x3 frame;

    if (this->mdl_hazard != NULL)
    {
        FreeModel();
    }

    RpAtomic* raw_model = g_hazard_rawModel[idx_model];
    if (raw_model != NULL)
    {
        this->mdl_hazard = xModelInstanceAlloc(raw_model, NULL, 0, 0, NULL);
        if (this->mdl_hazard == NULL)
        {
            this->flg_hazard &= ~0x100;
        }
        else
        {
            xMat3x3Euler(&frame, &ang_orient);
            xVec3Copy(&frame.pos, &g_O3);
            frame.flags = 0;
            xModelSetFrame(this->mdl_hazard, &frame);
            this->flg_hazard |= 0x100;

            U32 flg_pipespec = xModelGetPipeFlags(raw_model);
            if (((flg_pipespec >> 12) & 0xf) == 2)
            {
                this->flg_hazard |= 0x400;
            }
            if ((flg_pipespec & 0xc0) == 0x40)
            {
                this->flg_hazard |= 0x800;
            }
        }
    }

    return this->mdl_hazard ? 1 : 0;
}

void NPCHazard::FreeModel()
{
    if (mdl_hazard != NULL)
    {
        xModelInstanceFree(mdl_hazard);
    }
    mdl_hazard = NULL;
}

void NPCHazard::Discard()
{
    if ((flg_hazard & 1) != 0)
    {
        if (cb_notify != NULL)
        {
            cb_notify->Notify((en_haznote)0, this);
        }

        Cleanup();

        // Two statements, per the target: subi / stw / lwz / srawi / andc.
        // The reload of the counter after the store is why `g_cnt_activehaz` is
        // declared volatile; reading it once into `cnt` keeps the clamp down to
        // the single load the target has.
        //
        // Do not collapse this back into one expression. The previous
        // single-statement form `g_cnt_activehaz &= ~((g_cnt_activehaz - 1) >> 31);`
        // scored higher than the naive form but never decremented at all: for any
        // x >= 1 it folds to x &= 0xFFFFFFFF. The active-hazard count only ever grew.
        g_cnt_activehaz--;

        S32 cnt = g_cnt_activehaz;

        g_cnt_activehaz = cnt & ~(cnt >> 31);
    }
}

void NPCHazard::Kill()
{
    if ((flg_hazard & 1) != 0)
    {
        if (cb_notify != NULL)
        {
            cb_notify->Notify(HAZ_NOTE_ABORT, this);
        }
        Discard();
    }
}

void NPCHazard::Start(const xVec3* pos, F32 tym)
{
    if (tym > 0.0f)
    {
        tmr_remain = tym;
    }
    tym_lifespan = tmr_remain;
    if (pos != NULL)
    {
        PosSet(pos);
    }
    flg_hazard |= 0x1a;
}

void NPCHazard::PosSet(const xVec3* pos)
{
    if (pos != NULL)
    {
        xVec3Copy(&this->pos_hazard, pos);
    }
    if (this->mdl_hazard != NULL)
    {
        xVec3Copy((xVec3*)&this->mdl_hazard->Mat->pos, &this->pos_hazard);
    }
}

void NPCHazard::Timestep(F32 dt)
{
    F32 rat = 1.0f - this->tmr_remain / this->tym_lifespan;

    this->pam_interp = CLAMP(rat, 0.0f, 1.0f);

    if (this->mdl_hazard != NULL && (this->flg_hazard & 0x4000) &&
        (this->flg_hazard & 0x8000))
    {
        this->TypData_RotMatApply(&this->custdata.typical.mat_rotDelta);
    }

    switch (this->typ_hazard)
    {
    case NPC_HAZ_EXPLODE:
    case NPC_HAZ_EXPLODE_INNER:
        this->Upd_Explode(dt);
        break;
    case NPC_HAZ_FODBOMB:
        this->Upd_FodBomb(dt);
        break;
    case NPC_HAZ_CATTLEPROD:
        this->Upd_CattleProd(dt);
        break;
    case NPC_HAZ_TUBELETBLAST:
        this->Upd_TubeletBlast(dt);
        break;
    case NPC_HAZ_PUPPYNUKE:
        this->Upd_PuppyNuke(dt);
        break;
    case NPC_HAZ_DUPLOBOOM:
        this->Upd_DuploBoom(dt);
        break;
    case NPC_HAZ_THUNDER:
        this->Upd_TikiThunder(dt);
        break;
    case NPC_HAZ_DUPLO_SHROOM:
        this->Upd_Mushroom(dt);
        break;
    case NPC_HAZ_PATRIOT:
        this->Upd_Patriot(dt);
        break;
    case NPC_HAZ_TARTARPROJ:
        this->Upd_TTFlight(dt);
        break;
    case NPC_HAZ_TARTARSPILL:
        this->Upd_TTSpill(dt);
        break;
    case NPC_HAZ_TARTARSTINK:
        this->Upd_TTStink(dt);
        break;
    case NPC_HAZ_CHUCKBOMB:
        this->Upd_ChuckBomb(dt);
        break;
    case NPC_HAZ_CHUCKBLAST:
        this->Upd_ChuckBlast(dt);
        break;
    case NPC_HAZ_CHUCKBLOOSH:
        this->Upd_ChuckBloosh(dt);
        break;
    case NPC_HAZ_ARFBONE:
        this->Upd_BoneFlight(dt);
        break;
    case NPC_HAZ_ARFBONEBLAST:
        this->Upd_BoneBlast(dt);
        break;
    case NPC_HAZ_OILBUBBLE:
        this->Upd_OilBubble(dt);
        break;
    case NPC_HAZ_OILSLICK:
        this->Upd_OilOoze(dt);
        break;
    case NPC_HAZ_OILBURST:
        this->Upd_OilBurst(dt);
        break;
    case NPC_HAZ_OILGLOB:
        this->Upd_OilGlob(dt);
        break;
    case NPC_HAZ_MONCLOUD:
        this->Upd_MonCloud(dt);
        break;
    case NPC_HAZ_FUNFRAG:
        this->Upd_FunFrag(dt);
        break;
    case NPC_HAZ_ROBOBITS:
        this->Upd_RoboBits(dt);
        break;
    case NPC_HAZ_VISSPLASH:
        this->Upd_VisSplash(dt);
        break;
    default:
        break;
    }

    this->PosSet(NULL);
}

void NPCHazard::Render()
{
    xMat4x3 mat;
    en_npchaz typ = this->typ_hazard;
    xVec3 scale = { 1.0f, 1.0f, 1.0f };

    switch (typ)
    {
    case NPC_HAZ_EXPLODE:
    case NPC_HAZ_EXPLODE_INNER:
        if (this->mdl_hazard != NULL)
        {
            this->SetAlpha(0.25f * ARCH(this->pam_interp));
            xVec3SMul(&this->mdl_hazard->Scale, &scale,
                      2.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_FODBOMB:
        if (this->mdl_hazard != NULL)
        {
            static const xVec3 scale = { 1.0f, 1.5f, 1.0f };

            this->SetAlpha(0.35f * (1.0f - this->pam_interp));
            xVec3SMul(&this->mdl_hazard->Scale, &scale,
                      2.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_CATTLEPROD:
        if (this->mdl_hazard != NULL)
        {
            F32 scaleit = 2.0f * this->custdata.typical.rad_cur;

            this->SetAlpha(0.6f);
            xVec3SMul(&this->mdl_hazard->Scale, &scale, scaleit);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_TUBELETBLAST:
        if (this->mdl_hazard != NULL)
        {
            static const xVec3 scl_tallish = { 1.0f, 2.0f, 1.0f };

            this->SetAlpha(1.0f - this->pam_interp);
            xVec3SMul(&this->mdl_hazard->Scale, &scl_tallish,
                      2.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_PUPPYNUKE:
        if (this->mdl_hazard != NULL)
        {
            static const xVec3 wider = { 1.0f, 0.7f, 1.0f };
            static const xVec3 taller = { 0.5f, 2.0f, 0.5f };
            F32 scaleit;

            this->SetAlpha(0.2f * ARCH(this->pam_interp));
            scaleit = 2.0f * this->custdata.typical.rad_cur;
            xVec3SMul(&this->mdl_hazard->Scale, &wider, scaleit);
            xModelRender(this->mdl_hazard);
            xVec3SMul(&this->mdl_hazard->Scale, &taller, scaleit);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_DUPLOBOOM:
        if (this->mdl_hazard != NULL)
        {
            F32 alpha = 0.2f * (1.0f - this->pam_interp);
            F32 scaleit = 2.0f * this->custdata.typical.rad_cur;
            const xVec3 tallish = { 1.0f, 2.0f, 1.0f };

            this->SetAlpha(alpha);
            xVec3SMul(&this->mdl_hazard->Scale, &tallish, scaleit);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_THUNDER:
        if (this->mdl_hazard != NULL)
        {
            static const xVec3 tallish = { 1.0f, 1.75f, 1.0f };

            this->SetAlpha(0.5f * (1.0f - this->pam_interp));
            xVec3SMul(&this->mdl_hazard->Scale, &tallish,
                      2.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_MONCLOUD:
        if (this->mdl_hazard != NULL)
        {
            F32 alpha = 1.0f;
            F32 ds2 = NPCC_ds2_toCam(&this->pos_hazard, NULL);
            F32 pam;
            RwRaster* rast;

            if (ds2 >= SQ(4.0f))
            {
                xVec3 dir_plyr = *xEntGetPos(&globals.player.ent) - globals.camera.mat.pos;
                dir_plyr.normalize();

                xVec3 dir_haz = this->pos_hazard - globals.camera.mat.pos;
                dir_haz.normalize();

                F32 dotter = xVec3Dot(&dir_haz, &dir_plyr);
                if (dotter > 0.86f)
                {
                    alpha = SMOOTH(CLAMP(1.0f - (dotter - 0.86f) / (1.0f - 0.86f), 0.0f, 1.0f),
                                   0.7f, 1.0f);
                }
            }

            if (ds2 < SQ(4.0f))
            {
                alpha = 0.7f;
            }
            else if (ds2 < SQ(6.0f))
            {
                F32 num = ds2 - SQ(4.0f);

                alpha = SMOOTH(num / (SQ(6.0f) - SQ(4.0f)), 0.7f, alpha);
            }

            if (this->pam_interp < 0.15f)
            {
                pam = this->pam_interp / 0.15f;
            }
            else if (this->pam_interp < 0.85f)
            {
                pam = 1.0f;
            }
            else
            {
                pam = (1.0f - this->pam_interp) / (1.0f - 0.85f);
            }

            this->SetAlpha(SMOOTH(pam, 0.0f, alpha));
            xVec3SMul(&this->mdl_hazard->Scale, &scale,
                      2.0f * this->custdata.typical.rad_cur);

            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)3);
            xModelRender(this->mdl_hazard);
            RwRenderStateSet(rwRENDERSTATECULLMODE, (void*)2);
            xModelRender(this->mdl_hazard);

            rast = g_rast_hazshad[NPC_HAZMDL_MONCLOUD];
            if (rast == NULL)
            {
                break;
            }
            if (this->shadowCache == NULL)
            {
                break;
            }

            F32 ds2_cam = NPCC_ds2_toCam(&this->pos_hazard, NULL);
            if (ds2_cam < SQ(20.0f))
            {
                F32 rat_sq = ds2_cam / SQ(20.0f);
                F32 alf_shadow = CLAMP(0.3f + (1.0f - rat_sq), 0.0f, 1.0f);

                mat.right = *(xVec3*)this->Right();
                mat.at = g_NY3;
                mat.up = *(xVec3*)this->At();
                mat.pos = this->pos_hazard;

                static S32 skipfill = 0;

                NPCC_RenderProjTexture(rast, alf_shadow, &mat, 1.2f, 10.0f, this->shadowCache,
                                       !skipfill, NULL);

                if (skipfill++ > 12)
                {
                    skipfill = 0;
                }
            }
        }
        break;
    case NPC_HAZ_TARTARPROJ:
        if (this->mdl_hazard != NULL)
        {
            if (!(this->flg_hazard & 0x20000) && this->tmr_remain < 0.25f)
            {
                this->flg_hazard &= ~0x200;
                this->SetAlpha(4.0f * this->tmr_remain);
            }
            xVec3SMul(&this->mdl_hazard->Scale, &scale,
                      6.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_TARTARSPILL:
        if (this->mdl_hazard != NULL)
        {
            if (this->tmr_remain > 1.0f)
            {
                this->SetAlpha(1.0f);
            }
            else
            {
                this->flg_hazard &= ~0x200;
                this->SetAlpha(this->tmr_remain);
            }
            xVec3SMul(&this->mdl_hazard->Scale, &scale,
                      2.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_TARTARSTINK:
        if (this->mdl_hazard != NULL)
        {
            F32 scaleit = 2.0f * this->custdata.typical.rad_cur;
            xVec3 squat = { 0.75f, 0.5f, 0.75f };

            xVec3SMul(&this->mdl_hazard->Scale, &squat, scaleit);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_ARFBONE:
        if (this->mdl_hazard != NULL)
        {
            if (!(this->flg_hazard & 0x20000) && this->tmr_remain < 0.2f)
            {
                this->flg_hazard &= ~0x200;
                this->SetAlpha(5.0f * this->tmr_remain);
            }
            xVec3SMul(&this->mdl_hazard->Scale, &scale,
                      3.0f * (2.0f * this->custdata.typical.rad_cur));
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_ARFBONEBLAST:
        break;
    case NPC_HAZ_OILBUBBLE:
        if (this->mdl_hazard != NULL)
        {
            this->SetAlpha(0.75f);
            xVec3SMul(&this->mdl_hazard->Scale, &scale,
                      2.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_OILSLICK:
        if (this->mdl_hazard != NULL)
        {
            if (this->tmr_remain > 0.5f)
            {
                this->SetAlpha(0.75f);
            }
            else
            {
                this->SetAlpha(0.75f * MAX(0.0f, this->tmr_remain / 0.5f));
            }
            xVec3SMul(&this->mdl_hazard->Scale, &scale, this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_OILBURST:
        if (this->mdl_hazard != NULL)
        {
            this->SetAlpha(0.75f * (1.0f - this->pam_interp));

            F32 scaleit = 2.0f * this->custdata.typical.rad_cur;
            const xVec3 uni = { 1.0f, 1.0f, 1.0f };

            xVec3SMul(&this->mdl_hazard->Scale, &uni, scaleit);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_OILGLOB:
        if (this->mdl_hazard != NULL)
        {
            if (this->tmr_remain > 0.35f)
            {
                this->SetAlpha(0.75f);
            }
            else
            {
                this->SetAlpha(0.75f * MAX(0.0f, this->tmr_remain / 0.35f));
            }

            F32 scaleit = 2.0f * this->custdata.typical.rad_cur;
            const xVec3 scl_a = { 0.75f, 0.1f, 0.5f };
            const xVec3 scl_b = { 0.25f, 1.5f, 0.25f };
            const xVec3 scl_c = { 1.5f, 0.75f, 1.5f };
            xVec3 scl_now;

            if (this->pam_interp <= 0.25f)
            {
                SMOOTH(this->pam_interp / 0.25f, &scl_now, &scl_a, &scl_b);
            }
            else
            {
                SMOOTH((this->pam_interp - 0.25f) / 0.75f, &scl_now, &scl_b, &scl_c);
            }

            xVec3SMul(&this->mdl_hazard->Scale, &scl_now, scaleit);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_CHUCKBOMB:
        if (this->mdl_hazard != NULL)
        {
            if (!(this->flg_hazard & 0x20000) && this->tmr_remain < 0.25f)
            {
                this->flg_hazard &= ~0x200;
                this->SetAlpha(4.0f * this->tmr_remain);
            }
            xVec3SMul(&this->mdl_hazard->Scale, &scale, this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_CHUCKBLAST:
        if (this->mdl_hazard != NULL)
        {
            F32 rad = this->custdata.typical.rad_cur;
            F32 fx = CLAMP(xabs(isin(3.1415927f * this->pam_interp)), 0.0f, 1.0f);

            this->SetAlpha(fx);

            {
                F32 dim_flux = 1.2f * fx;
                xVec3 scl_wave = { 2.0f, 0.0f, 2.0f };

                scl_wave.y = dim_flux;
                xVec3SMul(&this->mdl_hazard->Scale, &scl_wave, rad);
                xModelRender(this->mdl_hazard);
            }

            {
                F32 rat_ff;

                if (this->pam_interp <= 0.15f)
                {
                    rat_ff = this->pam_interp / 0.15f;
                }
                else
                {
                    rat_ff = 1.0f - (this->pam_interp - 0.15f) / 0.85f;
                }

                F32 arch = EASE(rat_ff);
                F32 dim_flux = 3.75f * arch;
                xVec3 scl_fount = { 1.0f, 0.0f, 1.0f };

                scl_fount.y = dim_flux;
                xVec3SMul(&this->mdl_hazard->Scale, &scl_fount, rad);
                xModelRender(this->mdl_hazard);
            }
        }
        break;
    case NPC_HAZ_CHUCKBLOOSH:
        if (this->mdl_hazard != NULL)
        {
            static const xVec3 scale_cl = { 2.0f, 3.5f, 2.0f };

            if (this->tmr_remain > 0.8f)
            {
                this->SetAlpha(0.25f);
            }
            else
            {
                this->SetAlpha(0.25f * MAX(0.0f, this->tmr_remain / 0.8f));
            }
            xVec3SMul(&this->mdl_hazard->Scale, &scale_cl,
                      2.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_FUNFRAG:
        if (this->mdl_hazard != NULL)
        {
            xVec3SMul(&this->mdl_hazard->Scale, &scale,
                      2.0f * this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_ROBOBITS:
        if (this->mdl_hazard != NULL)
        {
            static const xVec3 scale_rb = { 1.0f, 1.0f, 1.0f };

            this->SetAlpha(1.0f);
            xVec3SMul(&this->mdl_hazard->Scale, &scale_rb, this->custdata.typical.rad_cur);
            xModelRender(this->mdl_hazard);
        }
        break;
    case NPC_HAZ_VISSPLASH:
        if (this->mdl_hazard != NULL)
        {
            F32 rad = this->custdata.typical.rad_cur;
            F32 fx = CLAMP(xabs(isin(3.1415927f * this->pam_interp)), 0.0f, 1.0f);

            this->SetAlpha(fx);

            {
                F32 dim_flux = 1.2f * fx;
                xVec3 scl_wave = { 2.0f, 0.0f, 2.0f };

                scl_wave.y = dim_flux;
                xVec3SMul(&this->mdl_hazard->Scale, &scl_wave, rad);
                xModelRender(this->mdl_hazard);
            }

            {
                F32 rat_ff;

                if (this->pam_interp <= 0.15f)
                {
                    rat_ff = this->pam_interp / 0.15f;
                }
                else
                {
                    rat_ff = 1.0f - (this->pam_interp - 0.15f) / 0.85f;
                }

                F32 arch = EASE(rat_ff);
                F32 dim_flux = 3.75f * arch;
                xVec3 scl_fount = { 1.0f, 0.0f, 1.0f };

                scl_fount.y = dim_flux;
                xVec3SMul(&this->mdl_hazard->Scale, &scl_fount, rad);
                xModelRender(this->mdl_hazard);
            }
        }
        break;
    case NPC_HAZ_PATRIOT:
        if (this->mdl_hazard != NULL)
        {
            xModelRender(this->mdl_hazard);
        }
        break;
    default:
        if (this->mdl_hazard != NULL)
        {
            xModelRender(this->mdl_hazard);
        }
        break;
    }
}

void NPCHazard::Cleanup()
{
    this->flg_hazard = 0;

    if (this->mdl_hazard != NULL)
    {
        FreeModel();
    }

    if (this->shadowCache != NULL)
    {
        NPCC_ShadowCacheRelease(this->shadowCache);
    }
    this->shadowCache = NULL;

    switch (this->typ_hazard)
    {
    case NPC_HAZ_TARTARPROJ:
    case NPC_HAZ_OILBUBBLE:
        if (this->custdata.tartar.streakID != 0xDEAD)
        {
            xFXStreakStop(this->custdata.tartar.streakID);
        }
        this->custdata.tartar.streakID = 0xDEAD;
        break;
    case NPC_HAZ_CATTLEPROD:
        if (this->custdata.catprod.zap_lyta != NULL)
        {
            zLightningKill(this->custdata.catprod.zap_lyta);
        }
        if (this->custdata.catprod.zap_lytb != NULL)
        {
            zLightningKill(this->custdata.catprod.zap_lytb);
        }
        this->custdata.catprod.zap_lyta = NULL;
        this->custdata.catprod.zap_lytb = NULL;
        break;
    case NPC_HAZ_MONCLOUD:
        if (this->custdata.cloud.zap_lytnin != NULL)
        {
            zLightningKill(this->custdata.cloud.zap_lytnin);
        }
        if (this->custdata.cloud.zap_warnin != NULL)
        {
            zLightningKill(this->custdata.cloud.zap_warnin);
        }
        this->custdata.cloud.zap_lytnin = NULL;
        this->custdata.cloud.zap_warnin = NULL;
        break;
    default:
        break;
    }
}

void NPCHazard::SetAlpha(F32 alpha)
{
    if (this->mdl_hazard == NULL)
    {
        return;
    }
    this->mdl_hazard->Flags |= 0x4000;
    this->mdl_hazard->Alpha = alpha;
}

S32 NPCHazard::ColTestSphere(const xBound* bnd_tgt, F32 rad)
{
    S32 hit = 0;
    xCollis colrec;
    xBound bnd;

    memset(&colrec, 0, sizeof(colrec));
    memset(&bnd, 0, sizeof(bnd));

    bnd.type = XBOUND_TYPE_SPHERE;
    bnd.sph.r = rad;
    xVec3Copy(&bnd.sph.center, &this->pos_hazard);

    xSphereHitsBound(&bnd.sph, bnd_tgt, &colrec);

    if (colrec.flags & 1)
    {
        hit = 1;
        if (!(this->flg_hazard & 0x40000000) && this->cb_notify != NULL)
        {
            this->cb_notify->Notify(HAZ_NOTE_HITPLAYER, this);
        }
        this->flg_hazard |= 0x40000000;
    }

    return hit;
}

S32 NPCHazard::ColTestCyl(const xBound* bnd_tgt, F32 rad, F32 hyt)
{
    S32 hit = 1;
    // `delta` is const because the scheduler otherwise treats the stores that
    // fill it as possibly aliasing the 0.5f literal load, and refuses to hoist
    // that load above them the way the retail code does.
    const xVec3 delta = bnd_tgt->cyl.center - this->pos_hazard;
    F32 hyt_top = 0.5f * hyt;
    F32 rad_sum = rad + bnd_tgt->cyl.r;
    F32 hyt_bot;

    hyt_top = hyt + hyt_top;
    hyt_bot = hyt_top - hyt;

    if (delta.y > hyt_top)
    {
        hit = 0;
    }
    else if (delta.y < hyt_bot)
    {
        hit = 0;
    }
    else if (xVec3Length2(&delta) > SQ(rad_sum))
    {
        hit = 0;
    }

    return hit;
}

S32 NPCHazard::ColPlyrSphere(F32 rad)
{
    return this->ColTestSphere(&globals.player.ent.bound, rad);
}

S32 NPCHazard::ColPlyrCyl(F32 rad, F32 hyt)
{
    return this->ColTestCyl(&globals.player.ent.bound, rad, hyt);
}

void NPCHazard::HurtThePlayer()
{
    if (this->npc_owner == NULL)
    {
        zEntPlayer_Damage(NULL, 1);
    }
    else if (zEntPlayer_DamageNPCKnockBack((xBase*)this->npc_owner, 1, &this->pos_hazard))
    {
        this->npc_owner->Vibrate(NPC_VIBE_NORM, -1.0f);
    }
}

void NPCHazard::TypData_RotMatStore(xVec3* euler)
{
    xMat3x3* mat_spin = &this->custdata.typical.mat_rotDelta;

    if (euler != NULL)
    {
        xMat3x3Euler(mat_spin, euler);
    }
    else
    {
        xVec3 ang_spin;

        ang_spin.x = 12.566371f * (2.0f * (xurand() - 0.5f));
        ang_spin.y = 0.78539819f * (2.0f * (xurand() - 0.5f));
        ang_spin.z = 12.566371f * (2.0f * (xurand() - 0.5f));
        xVec3SMulBy(&ang_spin, 0.016666668f);
        xMat3x3Euler(mat_spin, &ang_spin);
    }

    this->flg_hazard |= 0x4000;
}

void NPCHazard::TypData_RotMatSet(xMat3x3* mat)
{
    xMat3x3* frame = (xMat3x3*)xModelGetFrame(this->mdl_hazard);
    xMat3x3Copy(frame, mat);
    xModelSetFrame(this->mdl_hazard, (xMat4x3*)frame);
}

void NPCHazard::TypData_RotMatApply(xMat3x3* mat)
{
    xMat3x3* frame = (xMat3x3*)xModelGetFrame(this->mdl_hazard);
    xMat3x3Mul(frame, mat, frame);
    xModelSetFrame(this->mdl_hazard, (xMat4x3*)frame);
}

void NPCHazard::OrientToDir(const xVec3* vec_path, S32 doTheTwist)
{
    xVec3 dir_norm;
    xMat3x3 mat_orient;
    xMat3x3 mat_twist;

    F32 len = xVec3Length(vec_path);
    if (len < 1e-05f)
    {
        return;
    }

    xVec3SMul(&dir_norm, vec_path, -1.0f / len);
    xMat3x3LookVec(&mat_orient, &dir_norm);

    if (doTheTwist)
    {
        xMat3x3Rot(&mat_twist, &mat_orient.at,
                   6.2831855f * (this->tym_lifespan - this->tmr_remain));
        xMat3x3Mul(&mat_orient, &mat_orient, &mat_twist);
        this->TypData_RotMatSet(&mat_orient);
    }

    this->TypData_RotMatSet(&mat_orient);
}

en_hazmodel NPCHazard::PickFunFrag()
{
    S32 choices[8] = {};
    S32 cnt_choice = 0;

    for (S32 i = 0; i < 8; i++)
    {
        S32 idx = g_funfrag_choices[i];
        if (g_hazard_rawModel[idx] != NULL)
        {
            choices[cnt_choice++] = idx;
        }
    }

    return (en_hazmodel)xUtil_choose<S32>(choices, cnt_choice, NULL);
}

void NPCHazard::PreCollide()
{
    F32 gravity = 10.0f;

    if (this->typ_hazard == NPC_HAZ_CHUCKBOMB)
    {
        gravity = 5.0f;
    }
    else if (this->typ_hazard == NPC_HAZ_TARTARPROJ)
    {
        gravity = 7.5f;
    }
    else if (this->typ_hazard == NPC_HAZ_ARFBONE)
    {
        gravity = 0.5f;
    }
    else if (this->typ_hazard == NPC_HAZ_OILBUBBLE)
    {
        gravity = 0.2f;
    }
    else if (this->typ_hazard == NPC_HAZ_FUNFRAG)
    {
        gravity = 7.5f;
    }
    else if (this->typ_hazard == NPC_HAZ_CHUCKBLOOSH)
    {
        gravity = 25.2f;
    }
    else if (this->typ_hazard == NPC_HAZ_ROBOBITS)
    {
        // Same value as the initialiser, so CW emits the compare and no store.
        gravity = 10.0f;
    }

    xParabola* parab = &this->custdata.collide.parabinfo;
    static xCollis colrec;

    this->tmr_remain += 5.0f;

    xVec3Copy(&parab->initPos, &this->pos_hazard);
    xVec3Copy(&parab->initVel, &this->custdata.tartar.vel);
    parab->gravity = gravity;
    parab->minTime = 0.0f;
    parab->maxTime = this->tmr_remain;

    memset(&colrec, 0, sizeof(colrec));

    if (xParabolaHitsEnv(parab, globals.sceneCur->env, &colrec))
    {
        this->flg_hazard |= 0x20000;
        xVec3Copy(&this->custdata.collide.dir_normal, &colrec.norm);
        parab->maxTime = colrec.dist;
        this->tmr_remain = colrec.dist;
    }
    else
    {
        this->flg_hazard &= ~0x20000;
        xVec3Copy(&this->custdata.collide.dir_normal, &g_Y3);
    }

    this->tym_lifespan = this->tmr_remain;
}

S32 NPCHazard::StaggeredCollide()
{
    HAZCollide* hazcol = &this->custdata.collide;

    if (--hazcol->cnt_skipcol > 0)
    {
        return 0;
    }

    hazcol->cnt_skipcol = ((xrand() >> 23) & 1) + 5;

    static xCollis colrec;
    memset(&colrec, 0, sizeof(colrec));

    switch (hazcol->idx_rotateCol)
    {
    case HAZ_COLTYP_STAT:
        if (hazcol->flg_collide & 1)
        {
            StagColStat();
        }
        break;
    case HAZ_COLTYP_DYN:
        if (hazcol->flg_collide & 2)
        {
            StagColDyn();
        }
        break;
    case HAZ_COLTYP_NPC:
        if (hazcol->flg_collide & 4)
        {
            StagColNPC();
        }
        break;
    default:
        break;
    }

    hazcol->idx_rotateCol = (en_hazcol)(hazcol->idx_rotateCol + 1);
    if (hazcol->idx_rotateCol >= HAZ_COLTYP_NOMORE)
    {
        hazcol->idx_rotateCol = HAZ_COLTYP_STAT;
    }

    return 0;
}

void NPCHazard::StagColGeneral(S32 who)
{
    xParabola* parab = &this->custdata.collide.parabinfo;
    F32 tym_used = this->tym_lifespan - this->tmr_remain;
    F32 tym_beg = MIN(tym_used, this->tym_lifespan);
    F32 tym_end = MIN(0.5f + tym_beg, this->tym_lifespan);

    tym_end += 0.016666668f;
    xVec3 pos_beg = this->pos_hazard;
    xVec3 pos_end;
    xSweptSphere swdata;
    S32 hit;

    xParabolaEvalPos(parab, &pos_beg, tym_beg);
    xParabolaEvalPos(parab, &pos_end, tym_end);

    hit = 0;
    memset(&swdata, 0, sizeof(swdata));
    xSweptSpherePrepare(&swdata, &pos_beg, &pos_end, 0.05f);

    if (who & 3)
    {
        hit = xSweptSphereToStatDyn(&swdata, globals.sceneCur, NULL, 8);
    }
    else if (who & 4)
    {
        hit = xSweptSphereToNPC(&swdata, globals.sceneCur, NULL, 8);
    }

    if (hit)
    {
        F32 tym_hit;

        xSweptSphereGetResults(&swdata);

        tym_hit = 0.0f;
        if (swdata.dist > 1e-05f)
        {
            tym_hit = (swdata.curdist / swdata.dist) * (tym_end - tym_beg);
        }

        this->CollideResponse(&swdata, tym_hit);
    }
}

void NPCHazard::StagColStat()
{
    this->StagColGeneral(1);
}

void NPCHazard::StagColDyn()
{
    this->StagColGeneral(2);
}

void NPCHazard::StagColNPC()
{
    this->StagColGeneral(4);
}

void NPCHazard::CollideResponse(xSweptSphere* swdata, F32 tym_inFuture)
{
    this->ColResp_Default(swdata, tym_inFuture);
}

void NPCHazard::ColResp_Default(xSweptSphere* swdata, F32 tym_inFuture)
{
    xParabola* parab = &this->custdata.collide.parabinfo;
    F32 tym_all = this->tym_lifespan - this->tmr_remain;
    F32 tym_used = MIN(tym_all, this->tym_lifespan);

    this->tmr_remain = MIN(this->tmr_remain, tym_inFuture);
    parab->maxTime = tym_used + this->tmr_remain;
    this->tym_lifespan = parab->maxTime;

    xParabolaEvalPos(parab, &this->custdata.collide.pos_collide, parab->maxTime);
    this->custdata.collide.dir_normal = swdata->worldNormal;

    if (swdata->optr == NULL || ((xBase*)swdata->optr)->baseType != eBaseTypeStatic)
    {
        this->flg_hazard |= 0x40000;
    }
}

xAnimTable* ZNPC_AnimTable_HazardStd()
{
    S32 ourAnims[] = { 1, 2, 0 };
    xAnimTable* table = (xAnimTable*)xAnimTableNew("HazardStandard", NULL, 0);

    xAnimTableNewState(table, g_strz_hazanim[1], 0x10, 1, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);
    xAnimTableNewState(table, g_strz_hazanim[2], 0x10, 1, 1.0f, 0, 0, 0.0f, 0, 0,
                       xAnimDefaultBeforeEnter, 0x0, 0x0);

    NPCC_BuildStandardAnimTran(table, g_strz_hazanim, ourAnims, 1, 0.2f);

    return table;
}

void NPCHazard::Upd_Explode(F32 dt)
{
    this->custdata.typical.rad_cur = LERP(ARCH(this->pam_interp),
                                          this->custdata.typical.rad_min,
                                          this->custdata.typical.rad_max);

    if ((this->flg_hazard & 0x2000) && !(globals.player.DamageTimer > 0.0f) &&
        this->ColPlyrSphere(this->custdata.typical.rad_cur))
    {
        this->HurtThePlayer();
    }

    this->DeathStar();
}

void NPCHazard::DeathStar()
{
    static const xVec3 pos_offset = { 0.0f, 0.0f, 0.0f };
    xVec3 pos_star = this->pos_hazard + pos_offset;

    if (this->flg_hazard & 0x8)
    {
        xVec3 pos_slam = this->pos_hazard;

        pos_slam += pos_offset;
        zFX_SpawnBubbleSlam(&pos_slam, 30, 6.2831855f, 5.0f, 2.0f);
    }

    if (this->pam_interp < 0.16f)
    {
        // Two statements, per the target: `lfs <member>` issues before
        // `lfs 0.4f`, and the fmuls writes the constant's register
        // (`fmuls f0, f0, f1`). `F32 spd = 0.4f * rad_max;` swaps both, and
        // `spd *= 0.4f;` fixes the loads but not the multiply.
        F32 spd = this->custdata.typical.rad_max;
        spd = 0.4f * spd;
        xVec3 vel_bub = { spd, spd, spd };
        xVec3 vel_rnd = { 2.0f, 2.0f, 2.0f };

        zFX_SpawnBubbleTrail(&pos_star, 20, &vel_bub, &vel_rnd);
    }
}

void NPCHazard::Upd_PuppyNuke(F32 dt)
{
    xVec3 pos_emit;
    HAZBall* ball = &this->custdata.ball;

    ball->rad_cur = SMOOTH(this->pam_interp, ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(ball->rad_cur))
        {
            HurtThePlayer();
        }
    }

    if (this->flg_casthurt == 0 && this->npc_owner != NULL && this->flg_hazard & 0x2000 &&
        this->pam_interp > 0.7f)
    {
        zNPCMsg_AreaNPCExplodeNoRobo(npc_owner, ball->rad_max, &this->pos_hazard);
        this->flg_casthurt = 1;
        this->flg_hazard |= 0x40;
    }

    FodBombBubbles(dt);
}

void NPCHazard::Upd_FodBomb(F32 dt)
{
    xVec3 pos_emit;
    HAZBall* ball = &this->custdata.ball;

    ball->rad_cur = LERP(isin(PI * this->pam_interp), ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(ball->rad_cur))
        {
            HurtThePlayer();
        }
    }

    if (this->flg_casthurt == 0 && this->npc_owner != NULL && this->flg_hazard & 0x2000 &&
        this->pam_interp > 0.7f)
    {
        zNPCMsg_AreaNPCExplodeNoRobo(npc_owner, ball->rad_max, &this->pos_hazard);
        this->flg_casthurt = 1;
        this->flg_hazard |= 0x40;
    }

    FodBombBubbles(dt);
}

// Second block of deadstripped-function structs; see __deadstripped_zNPCHazard.
// These two sit between Upd_FodBomb's literals and FodBombBubbles' statics in
// the target's .rodata, so they have to be declared here and not at the top.
void __deadstripped_zNPCHazard_2()
{
    const char _2401[0x0C] = {};
    const char _2452[0x0C] = {};
}

void NPCHazard::FodBombBubbles(F32 dt)
{
    static const xVec3 pos_spread = { 0.1f, 2.0f, 0.1f };
    static const xVec3 vel_spread = { 2.0f, 12.0f, 2.0 };
    static const xVec3 pos_offsetFirst = { 0.0f, -0.5f, 0.0f };
    static const xVec3 pos_offsetLater = { 0.0f, -1.0f, 0.0f };

    if (this->flg_hazard & 0x8)
    {
        xVec3 pos_emit = this->pos_hazard;
        pos_emit += pos_offsetFirst;

        zFX_SpawnBubbleTrail(&pos_emit, 0x100, &pos_spread, &vel_spread);
    }
    else if (this->tmr_remain < 0.2f)
    {
        F32 tym = LERP(1.0f - this->tmr_remain / 0.2f, 0.75f, 1.0f);
        xVec3 pos_emit = this->pos_hazard;
        pos_emit += pos_offsetLater;

        // Retail passes the hazard's own position here, not the pos_emit it
        // just built -- pos_emit is computed and then thrown away.
        zFX_SpawnBubbleSlam(&this->pos_hazard, 0x18, PI, 4.0f * tym, tym);
    }
}

// Third block of deadstripped-function structs; see __deadstripped_zNPCHazard.
void __deadstripped_zNPCHazard_3()
{
    const char _2499[0x0C] = {};
}

void NPCHazard::Upd_CattleProd(F32 dt)
{
    HAZCatProd* catprod = &this->custdata.catprod;
    catprod->rad_cur = LERP(isin(PI * this->pam_interp), catprod->rad_min, catprod->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f) &&
        this->tym_lifespan - this->tmr_remain > 0.25f)
    {
        if (ColPlyrSphere(catprod->rad_cur))
        {
            HurtThePlayer();
        }
    }

    if (this->flg_hazard & 0x8)
    {
        _tagLightningAdd info;
        NPCC_MakeLightningInfo(NPC_LYT_CATTLEPROD, &info);

        xVec3 pos_lytend = { 0.0f, 0.0f, 0.0f };
        pos_lytend.x = catprod->rad_cur;
        pos_lytend.y = catprod->rad_cur;
        pos_lytend.z = catprod->rad_cur;

        info.time = this->tmr_remain;
        info.start = &this->pos_hazard;

        pos_lytend.x *= xurand();
        pos_lytend.y *= xurand();
        pos_lytend.z *= xurand();

        pos_lytend.x += 0.05f;
        pos_lytend.y += 0.05f;
        pos_lytend.z += 0.05f;

        xVec3AddTo(&pos_lytend, &this->pos_hazard);
        info.end = &pos_lytend;
        catprod->zap_lyta = zLightningAdd(&info);

        xVec3SubFrom(&pos_lytend, &this->pos_hazard);
        pos_lytend.y *= -1.0f;
        xVec3AddTo(&pos_lytend, &this->pos_hazard);
        info.end = &pos_lytend;
        catprod->zap_lytb = zLightningAdd(&info);
    }
    else
    {
        xVec3 pos_lytend = { 0.0f, 0.0f, 0.0f };
        pos_lytend.x = catprod->rad_cur;
        pos_lytend.y = catprod->rad_cur;
        pos_lytend.z = catprod->rad_cur;

        if (this->npc_owner != NULL)
        {
            F32 tym = this->npc_owner->AnimTimeCurrent();
            if (tym > 0.8f && tym < 1.0f)
            {
                xVec3SMul(&pos_lytend, NPCC_faceDir(this->npc_owner), catprod->rad_cur);
                xVec3AddScaled(&pos_lytend, NPCC_rightDir(this->npc_owner),
                               (xrand() & 0x800000 ? 0.2f : 0.2f) * xurand());
            }
            else
            {
                pos_lytend.x *= xurand();
                pos_lytend.y *= xurand();
                pos_lytend.z *= xurand();
            }
        }
        else
        {
            pos_lytend.x *= xurand();
            pos_lytend.y *= xurand();
            pos_lytend.z *= xurand();
        }

        xVec3AddTo(&pos_lytend, &this->pos_hazard);
        if (catprod->zap_lyta != NULL)
        {
            zLightningModifyEndpoints(catprod->zap_lyta, &this->pos_hazard, &pos_lytend);
        }

        if (catprod->zap_lytb != NULL)
        {
            xVec3SubFrom(&pos_lytend, &this->pos_hazard);
            pos_lytend.y *= -1.0f;
            xVec3AddTo(&pos_lytend, &this->pos_hazard);

            zLightningModifyEndpoints(catprod->zap_lytb, &this->pos_hazard, &pos_lytend);
        }
    }
}

void NPCHazard::Upd_TubeletBlast(F32 dt)
{
    HAZBall* ball = &this->custdata.ball;
    ball->rad_cur = LERP(this->pam_interp, ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(ball->rad_cur))
        {
            HurtThePlayer();
        }
    }

    if (this->flg_hazard & 0x8)
    {
        xSndPlay3D(xStrHash(""), 0.77f, 0.0f, 0x80, 0x0, &this->pos_hazard, 1.0f, 20.0f,
                   SND_CAT_GAME, 0.0f);
    }

    static S32 moreorless = 0;

    if (--moreorless < 0)
    {
        moreorless = 3;

        xVec3 vel_emit = { 0.0f, 0.0f, 0.0f };
        vel_emit.x = 2.0f * (xurand() - 0.5f);
        vel_emit.y = xurand();
        vel_emit.z = 2.0f * (xurand() - 0.5f);
        vel_emit *= 8.0f;

        xVec3 pos_emit = this->pos_hazard;
        pos_emit.x += 2.0f * (xurand() - 0.5f) * ball->rad_cur;
        pos_emit.y += 2.0f * (xurand() - 0.5f) * ball->rad_cur;
        pos_emit.z += 2.0f * (xurand() - 0.5f) * ball->rad_cur;

        NPAR_EmitTubeSparklies(&pos_emit, &vel_emit);
    }
}

void NPCHazard::Upd_DuploBoom(F32 dt)
{
    xVec3 pos_emit = { 0.0f, 0.0f, 0.0f };
    HAZBall* ball = &this->custdata.ball;

    ball->rad_cur = LERP(this->pam_interp, ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(ball->rad_cur))
        {
            HurtThePlayer();
        }
    }

    if (this->flg_casthurt == 0 && this->npc_owner != NULL && this->flg_hazard & 0x2000 &&
        this->pam_interp > 0.7f)
    {
        zNPCMsg_AreaNPCExplodeNoRobo(npc_owner, ball->rad_max, &this->pos_hazard);
        this->flg_casthurt = 1;
        this->flg_hazard |= 0x40;
    }
}

void NPCHazard::Upd_TikiThunder(F32 dt)
{
    xVec3 pos_emit = { 0.0f, 0.0f, 0.0f };
    HAZBall* ball = &this->custdata.ball;

    ball->rad_cur = LERP(this->pam_interp, ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(ball->rad_cur))
        {
            HurtThePlayer();
        }
    }

    g_parf_default.custom_flags = 0x100;
    xVec3Copy(&pos_emit, &this->pos_hazard);
    xVec3Copy(&g_parf_default.pos, &pos_emit);
}

void NPCHazard::Upd_Mushroom(F32 dt)
{
    xVec3 pos_emit = { 0.0f, 0.0f, 0.0f };
    HAZShroom* shroom = &this->custdata.shroom;

    xVec3AddScaled(&this->pos_hazard, &shroom->vel_rise, dt);
    xVec3AddScaled(&shroom->vel_rise, &shroom->acc_rise, dt);

    shroom->rad_cur = LERP(this->pam_interp, shroom->rad_min, shroom->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(shroom->rad_cur))
        {
            HurtThePlayer();
        }
    }

    g_parf_default.custom_flags = 0x100;
    xVec3Copy(&pos_emit, &this->pos_hazard);
    xVec3Copy(&g_parf_default.pos, &pos_emit);

    xParEmitterEmitCustom(g_pemit_default, dt, &g_parf_default);
}

void NPCHazard::Upd_Patriot(F32)
{
}

void NPCHazard::Upd_TTFlight(F32 dt)
{
    HAZTarTar* tartar = &this->custdata.tartar;
    xParabola* parab = &tartar->parabinfo;

    if (this->mdl_hazard == NULL)
    {
        MarkForRecycle();
        return;
    }

    if (this->pam_interp < 0.2f)
    {
        tartar->rad_cur = LERP(this->pam_interp / 0.2f, tartar->rad_min, tartar->rad_max);
    }
    else
    {
        tartar->rad_cur = tartar->rad_max;
    }

    if (this->tmr_remain < 0.0f)
    {
        if (this->flg_hazard & 0x20000)
        {
            xParabolaEvalPos(parab, &this->pos_hazard, parab->maxTime);

            zShrapnelAsset* shrp = g_data_hazshrap[2];

            xVec3 vel_push;
            xParabolaEvalVel(parab, &vel_push, parab->maxTime);

            if (xVec3Length2(&tartar->dir_normal) > 0.0f)
            {
                xVec3 vec_tmp = {};

                vec_tmp.y = vel_push.y;
                NPCC_Bounce(&vec_tmp, &tartar->dir_normal, 0.1f);
                vel_push.y = vec_tmp.y;
            }

            shrp->initCB(shrp, this->mdl_hazard, &vel_push, NULL);

            if (xVec3Length2(&tartar->dir_normal) > 0.1f)
            {
                TarTarSplash(&tartar->dir_normal);
            }

            if (!(this->flg_hazard & 0x40000))
            {
                ReconTarTar();
                return;
            }

            MarkForRecycle();
            return;
        }
        else
        {
            xVec3 whence;

            xParabolaEvalVel(parab, &whence, parab->maxTime);
            xVec3Inv(&whence, &whence);

            F32 mag = xVec3Length(&whence);
            if (mag > 0.00001f)
            {
                whence /= mag;
                TarTarSplash(&whence);
            }
            else
            {
                TarTarSplash((xVec3*)Up());
            }

            MarkForRecycle();
            return;
        }
    }
    else
    {
        if (this->flg_hazard & 0x8)
        {
            xVec3Sub(&tartar->vel, &tartar->pos_tgt, &this->pos_hazard);
            xVec3SMulBy(&tartar->vel, 1.0f / this->tym_lifespan);

            tartar->vel.y = 7.5f * (0.5f * this->tym_lifespan) + tartar->vel.y;
        }

        if (this->flg_hazard & 0x8)
        {
            PreCollide();
        }

        F32 tym = this->tym_lifespan < this->tym_lifespan - this->tmr_remain ?
                      this->tym_lifespan :
                      this->tym_lifespan - this->tmr_remain;
        xParabolaEvalPos(parab, &this->pos_hazard, tym);
        xParabolaEvalVel(parab, &tartar->vel, tym);

        F32 mag = xVec3Length(&tartar->vel);
        if (mag > 0.00001f)
        {
            xMat3x3 mat_rot;
            xVec3 dir;

            xVec3SMul(&dir, &tartar->vel, -1.0f / mag);
            xMat3x3LookVec(&mat_rot, &dir);
            TypData_RotMatSet(&mat_rot);
        }

        if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
        {
            if (ColPlyrSphere(tartar->rad_cur))
            {
                HurtThePlayer();
                g_data_hazshrap[2]->initCB(g_data_hazshrap[2], globals.player.ent.model,
                                           &tartar->vel, NULL);

                xVec3 whence;
                xParabolaEvalVel(parab, &whence, parab->maxTime);
                xVec3Inv(&whence, &whence);

                TarTarSplash(&whence);
                MarkForRecycle();
                return;
            }
        }
        StaggeredCollide();

        if (this->flg_hazard & 0x8)
        {
            TarTarFalumpf();
        }
        else
        {
            TarTarGunkTrail();
        }

        static S32 moreorless = 0;

        if (--moreorless < 0)
        {
            moreorless = 4;
            zFX_SpawnBubbleTrail(&this->pos_hazard, 0x6);
        }
    }
}

void NPCHazard::ReconTarTar()
{
    const xVec3 dir_norm = this->custdata.collide.dir_normal;
    Reconfigure(NPC_HAZ_TARTARSPILL);

    if (xVec3Length2(&dir_norm) > 0.0f)
    {
        xVec3Copy((xVec3*)&this->mdl_hazard->Mat->up, &dir_norm);
        NPCC_MakePerp((xVec3*)&this->mdl_hazard->Mat->at, &dir_norm);
        xVec3Cross((xVec3*)&this->mdl_hazard->Mat->right, (xVec3*)&this->mdl_hazard->Mat->up,
                   (xVec3*)&this->mdl_hazard->Mat->at);
    }

    Start(NULL, -1.0f);
}

void NPCHazard::Upd_TTSpill(F32 dt)
{
    HAZBall* ball = &this->custdata.ball;
    if (this->flg_hazard & 0x8 && HAZ_AvailablePool() > 5)
    {
        if (KickSteamyStinky())
        {
            this->flg_hazard |= 0x40;
        }
    }

    ball->rad_cur = LERP(this->pam_interp, ball->rad_max, ball->rad_min);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(ball->rad_cur))
        {
            HurtThePlayer();
            g_data_hazshrap[2]->initCB(g_data_hazshrap[2], globals.player.ent.model, NULL, NULL);
        }
    }

    TarTarLinger();
}

S32 NPCHazard::KickSteamyStinky()
{
    S32 ok;
    NPCHazard* haz = (NPCHazard*)HAZ_Acquire();

    F32 tym_lifeOfChild = 0.25f;

    if (haz == NULL)
    {
        ok = 0;
    }
    else
    {
        if (haz->ConfigHelper(NPC_HAZ_TARTARSTINK) == 0)
        {
            haz->Discard();
            ok = 1;
        }
        else
        {
            haz->Start(&pos_hazard, tmr_remain - tym_lifeOfChild);
            ok = 2;
        }
    }
    return ok;
}

void NPCHazard::Upd_TTStink(F32 dt)
{
    HAZBall* ball = &this->custdata.ball;
    ball->rad_cur = LERP(1.0f - this->pam_interp, ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(ball->rad_cur))
        {
            HurtThePlayer();
            g_data_hazshrap[2]->initCB(g_data_hazshrap[2], globals.player.ent.model, NULL, NULL);
        }
    }

    TarTarLinger();
}

void NPCHazard::TarTarFalumpf()
{
    g_parf_default.custom_flags = 0x100;
    xVec3 pos_emit;

    for (S32 i = 0; i < 16; i++)
    {
        pos_emit.x =
            this->custdata.tartar.rad_cur * (2.0f * (xurand() - 0.5f)) + this->pos_hazard.x;
        pos_emit.y =
            this->custdata.tartar.rad_cur * (2.0f * (xurand() - 0.5f)) + this->pos_hazard.y;
        pos_emit.z =
            this->custdata.tartar.rad_cur * (2.0f * (xurand() - 0.5f)) + this->pos_hazard.z;
        NPAR_EmitTarTarNozzle(&pos_emit, &g_Y3);
    }
}

void NPCHazard::TarTarGunkTrail()
{
    xVec3 pos = pos_hazard;
    NPAR_EmitTarTarTrail(&pos, &g_Y3);
}

void NPCHazard::TarTarSplash(const xVec3* dir_norm)
{
    xVec3 up;
    xVec3 at;
    xVec3 rt;

    if (dir_norm != NULL)
    {
        up = *dir_norm;
        NPCC_MakeArbPlane(dir_norm, &at, &rt);
    }
    else
    {
        up = *(xVec3*)Up();
        at = *(xVec3*)At();
        rt = *(xVec3*)Right();
    }

    const xVec3 pos_emit = pos_hazard;
    for (S32 i = 0; i < 16; i++)
    {
        // Using initialization prevents generation of the operator= for the xVec3,
        // So it's on the next line to generate the right instructions.

        // Scheduling meme here but matches otherwise.
        xVec3 vel_emit;
        vel_emit = up;

        F32 direction;
        if (xrand() & 0x800000)
        {
            direction = 1.0f;
        }
        else
        {
            direction = -1.0f;
        }

        vel_emit += at * direction * (0.4f * (2.0f * (xurand() - 0.5f)) + 0.25f);

        if (xrand() & 0x800000)
        {
            direction = 1.0f;
        }
        else
        {
            direction = -1.0f;
        }

        vel_emit += rt * direction * (0.4f * (2.0f * (xurand() - 0.5f)) + 0.25f);
        vel_emit.normalize();
        vel_emit *= 15.0f;

        NPAR_EmitTarTarSplash(&pos_emit, &vel_emit);
    }
}

void NPCHazard::TarTarLinger()
{
    HAZBall* ball = &this->custdata.ball;
    const xVec3 vel_emit = g_Y3;

    if (--this->cnt_nextemit >= 0)
    {
        return;
    }

    this->cnt_nextemit = 10;

    F32 rad_use = 0.75f * ball->rad_cur;
    xVec3 pos_emit = this->pos_hazard;

    pos_emit += *(xVec3*)At() * (rad_use * (2.0f * (xurand() - 0.5f)));
    pos_emit += *(xVec3*)Right() * (rad_use * (2.0f * (xurand() - 0.5f)));
    pos_emit += *(xVec3*)Up() * 0.1f;

    NPAR_EmitTarTarSpoil(&pos_emit, &vel_emit);
}

void NPCHazard::Upd_ChuckBomb(F32 dt)
{
    HAZTarTar* tartar = &this->custdata.tartar;
    const xParabola* parab = &tartar->parabinfo;

    if (this->tmr_remain < dt)
    {
        if (this->flg_hazard & 0x20000)
        {
            xParabolaEvalPos(parab, &this->pos_hazard, parab->maxTime);
            ReconChuck();
            return;
        }
        else
        {
            ReconChuck();
            return;
        }
    }

    if (this->flg_hazard & 0x8)
    {
        xVec3Sub(&tartar->vel, &tartar->pos_tgt, &this->pos_hazard);
        xVec3SMulBy(&tartar->vel, 1.0f / this->tmr_remain);

        tartar->vel.y = 5.0f * (0.5f * this->tmr_remain) + tartar->vel.y;
    }

    if (this->flg_hazard & 0x8)
    {
        PreCollide();
    }

    F32 tym = this->tym_lifespan - this->tmr_remain;
    xParabolaEvalPos(parab, &this->pos_hazard, tym);
    xParabolaEvalVel(parab, &tartar->vel, tym);

    F32 vel_mag = xVec3Length(&tartar->vel);
    if (vel_mag > 0.00001f)
    {
        xMat3x3 mat_rot;
        xVec3 dir;
        xVec3SMul(&dir, &tartar->vel, -1.0f / vel_mag);
        xMat3x3LookVec(&mat_rot, &dir);

        xMat3x3 mat_spiral;
        xMat3x3Rot(&mat_spiral, &mat_rot.at,
                   2.0f * PI * (this->tym_lifespan - this->tmr_remain));
        xMat3x3Mul(&mat_rot, &mat_rot, &mat_spiral);

        TypData_RotMatSet(&mat_rot);
        TypData_RotMatSet(&mat_rot);
    }

    static S32 moreorless = 0;

    if (--moreorless < 0)
    {
        moreorless = 3;
        DisperseBubWake(tartar->rad_cur, &tartar->vel);
    }

    if (this->flg_hazard & 0x2000)
    {
        if (!(globals.player.DamageTimer > 0.0f) && ColPlyrSphere(tartar->rad_cur))
        {
            HurtThePlayer();
            ReconChuck();
            return;
        }
    }

    StaggeredCollide();
}

void NPCHazard::DisperseBubWake(F32 radius, const xVec3* velocity)
{
    F32 dst_disperse = 0.5f * radius;

    xVec3 pos_disperse;
    pos_disperse = *(xVec3*)Up() * (dst_disperse * (2.0f * (xurand() - 0.5f)));
    pos_disperse += *(xVec3*)Right() * (dst_disperse * (2.0f * (xurand() - 0.5f)));

    xVec3 vel_disperse;
    vel_disperse = *(xVec3*)Up() * (8.0f * (2.0f * (xurand() - 0.5f)));
    vel_disperse += *(xVec3*)Right() * (8.0f * (2.0f * (xurand() - 0.5f)));

    xVec3 dir_backward = *velocity;
    dir_backward.inverse();
    dir_backward.normalize();

    xVec3 pos_emit = this->pos_hazard;
    pos_emit -= dir_backward * (1.5f * radius);

    zFX_SpawnBubbleTrail(&pos_emit, 0x10, &pos_disperse, &vel_disperse);
}

void NPCHazard::ReconChuck()
{
    HAZBall* ball = &this->custdata.ball;

    const xVec3 dir_norm = this->custdata.collide.dir_normal;
    const xVec3 vel_flight = this->custdata.tartar.vel;

    Reconfigure(NPC_HAZ_CHUCKBLAST);

    if (xVec3Length2(&dir_norm) > 0.0f)
    {
        xVec3Copy((xVec3*)&this->mdl_hazard->Mat->up, &dir_norm);
        NPCC_MakePerp((xVec3*)&this->mdl_hazard->Mat->at, &dir_norm);
        xVec3Cross((xVec3*)&this->mdl_hazard->Mat->right, (xVec3*)&this->mdl_hazard->Mat->up,
                   (xVec3*)&this->mdl_hazard->Mat->at);

        xMat3x3 mat;
        xMat3x3Rot(&mat, &dir_norm, 2 * PI * xurand());
        xMat3x3Mul(xModelGetFrame(this->mdl_hazard), xModelGetFrame(this->mdl_hazard), &mat);

        F32 dot = xVec3Dot(&dir_norm, &g_Y3);

        if (FABS(dot) < 0.86f)
        {
            ball->rad_max *= 0.5f;
            ball->rad_min *= 0.5f;
            ball->rad_cur *= 0.5f;
        }
    }

    Start(NULL, -1.0f);

    if (this->flg_hazard & 0x8 && HAZ_AvailablePool() > 5)
    {
        for (S32 i = 0; i < 7; i++)
        {
            if (KickBlooshBlob(&vel_flight))
            {
                this->flg_hazard |= 0x40;
            }
        }
    }
}

void NPCHazard::Upd_ChuckBlast(F32 dt)
{
    HAZBall* ball = &this->custdata.ball;

    ball->rad_cur = LERP(this->pam_interp, ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f) &&
        this->pam_interp < 0.75f)
    {
        if (ColPlyrCyl(ball->rad_cur, 0.5f * ball->rad_cur))
        {
            HurtThePlayer();
        }
    }

    if (this->flg_hazard & 0x8)
    {
        xSndPlay3D(xStrHash("Chu_splash"), 0.77f, 0.0f, 0x80, 0x0, &this->pos_hazard, 5.0f, 15.0f,
                   SND_CAT_GAME, 0.0f);
    }

    if (this->flg_hazard & 0x8)
    {
        WaterSplash(NULL);
    }

    WavesOfEvil();
}

void NPCHazard::WaterSplash(const xVec3* dir_norm)
{
    const xVec3 pos_emit = this->pos_hazard;

    xVec3 up, at, rt;
    if (dir_norm)
    {
        up = *dir_norm;
        NPCC_MakePerp(&at, dir_norm);
        xVec3Cross(&rt, &up, &at);
    }
    else
    {
        up = *(xVec3*)Up();
        at = *(xVec3*)At();
        rt = *(xVec3*)Right();
    }

    for (S32 i = 0; i < 8; i++)
    {
        xVec3 vel_emit;
        vel_emit = up * (0.5f * xurand() + 1.5f);

        F32 direction;
        if (xrand() & 0x800000)
        {
            direction = 1.0f;
        }
        else
        {
            direction = -1.0f;
        }

        vel_emit += at * direction * (0.5f * (2.0f * (xurand() - 0.5f)) + 1.0f);

        if (xrand() & 0x800000)
        {
            direction = 1.0f;
        }
        else
        {
            direction = -1.0f;
        }

        vel_emit += rt * direction * (0.5f * (2.0f * (xurand() - 0.5f)) + 1.0f);
        vel_emit.normalize();
        vel_emit *= 10.0f;

        NPAR_EmitH2ODrips(&pos_emit, &vel_emit);
    }

    for (S32 i = 0; i < 8; i++)
    {
        xVec3 vel_emit;
        xurand();
        vel_emit = up * 1.0f;

        F32 direction;
        if (xrand() & 0x800000)
        {
            direction = 1.0f;
        }
        else
        {
            direction = -1.0f;
        }

        vel_emit += at * direction * (0.75f * (2.0f * (xurand() - 0.5f)) + 0.75f);

        if (xrand() & 0x800000)
        {
            direction = 1.0f;
        }
        else
        {
            direction = -1.0f;
        }

        vel_emit += rt * direction * (0.75f * (2.0f * (xurand() - 0.5f)) + 0.75f);
        vel_emit.normalize();
        vel_emit *= 7.0f;

        NPAR_EmitH2ODrops(&pos_emit, &vel_emit);
    }
}

void NPCHazard::WavesOfEvil()
{
    F32 rad = this->custdata.collide.rad_cur;

    for (S32 i = 0; i < 8; i++)
    {
        xVec3 pos_emit;
        xVec3 vel_emit;

        pos_emit = *(xVec3*)At() * (2.0f * (xurand() - 0.5f));
        pos_emit += *(xVec3*)Right() * (2.0f * (xurand() - 0.5f));
        pos_emit.normalize();
        pos_emit *= rad;

        pos_emit += *(xVec3*)Up() * 0.2f;
        pos_emit += this->pos_hazard;

        vel_emit = *(xVec3*)Up();
        vel_emit *= 5.5f;

        NPAR_EmitH2OSpray(&pos_emit, &vel_emit);
    }
}

S32 NPCHazard::KickBlooshBlob(const xVec3* vel_flight)
{
    NPCHazard* haz = HAZ_Acquire();
    if (!haz)
    {
        return 0;
    }

    if (!haz->ConfigHelper(NPC_HAZ_CHUCKBLOOSH))
    {
        haz->Discard();
        return 1;
    }

    haz->SetNPCOwner(this->npc_owner);

    HAZTarTar* tartar = &haz->custdata.tartar;
    tartar->vel = *(xVec3*)this->Up() * (5.0f * xurand() + 5.0f);

    // One expression, not a `spd_factor` local: mwcc evaluates the RIGHT
    // operand of `*` first, so `3.5f * xurand() + 2.5f` still runs before
    // `xurand() - 0.5f` (same RNG order as the two-statement form) while the
    // final multiply keeps the target's `fmuls f31, f0, f31` operand order.
    tartar->vel += *(xVec3*)this->At() * (2.0f * (xurand() - 0.5f) * (3.5f * xurand() + 2.5f));
    tartar->vel +=
        *(xVec3*)this->Right() * (2.0f * (xurand() - 0.5f) * (3.5f * xurand() + 2.5f));

    if (xVec3Dot(&g_Y3, (xVec3*)this->Up()) > 0.86f)
    {
        xVec3 vel_drift = { 0.0f, 0.0f, 0.0f };
        vel_drift.x = vel_flight->x;
        vel_drift.z = vel_flight->z;

        F32 drift_mag = xVec3Length(&vel_drift);
        if (drift_mag > 0.5f)
        {
            F32 mag_factor = MIN(0.25f * drift_mag, 2.5f) / drift_mag;
            xVec3 vel_push = vel_drift * mag_factor;
            tartar->vel += vel_push;
        }
    }

    xVec3 pos_emit = this->pos_hazard;
    pos_emit += *(xVec3*)this->Up() * 0.35f;

    haz->Start(&pos_emit, -1.0f);

    return 2;
}

void NPCHazard::Upd_ChuckBloosh(F32 dt)
{
    HAZTarTar* tartar = &this->custdata.tartar;
    xParabola* parab = &tartar->parabinfo;

    F32 rat_quad = SQ(this->pam_interp) / SQ(0.15f);
    if (rat_quad <= 0.15f)
    {
        tartar->rad_cur = LERP(rat_quad, tartar->rad_min, tartar->rad_max);
    }

    if (this->flg_hazard & 0x8)
    {
        PreCollide();
    }

    F32 dst_behind =
        tym_lifespan < tym_lifespan - tmr_remain ? tym_lifespan : tym_lifespan - tmr_remain;
    xParabolaEvalPos(parab, &this->pos_hazard, dst_behind);
    xParabolaEvalVel(parab, &tartar->vel, dst_behind);

    OrientToDir(&tartar->vel, 0x0);

    if (--this->cnt_nextemit >= 0)
    {
        return;
    }

    this->cnt_nextemit = 5;

    F32 rad_back = 0.5f * tartar->rad_cur;
    xVec3 pos_emit = this->pos_hazard;

    pos_emit -= tartar->vel * rad_back;
    NPAR_EmitH2OTrail(&pos_emit);
}

void NPCHazard::Upd_BoneFlight(F32 dt)
{
    HAZTarTar* tartar = &this->custdata.tartar;
    xParabola* parab = &tartar->parabinfo;

    if (this->tmr_remain < dt)
    {
        if (this->flg_hazard & 0x20000)
        {
            xParabolaEvalPos(parab, &this->pos_hazard, parab->maxTime);
            if (!(this->flg_hazard & 0x40000))
            {
                ReconArfBone();
                return;
            }
            else
            {
                MarkForRecycle();
                return;
            }
        }
        else
        {
            MarkForRecycle();
            return;
        }
    }

    if (this->flg_hazard & 0x8)
    {
        xVec3Sub(&tartar->vel, &tartar->pos_tgt, &this->pos_hazard);
        xVec3SMulBy(&tartar->vel, 1.0f / this->tmr_remain);

        tartar->vel.y += 0.5f * this->tmr_remain * 0.5f;
    }

    if (this->flg_hazard & 0x8)
    {
        PreCollide();
    }

    F32 tym = tym_lifespan - tmr_remain;
    xParabolaEvalPos(parab, &this->pos_hazard, tym);
    xParabolaEvalVel(parab, &tartar->vel, tym);

    static S32 moreorless = 0;
    if (--moreorless < 0)
    {
        moreorless = 3;
        zFX_SpawnBubbleTrail(&this->pos_hazard, 0x1);
    }

    if (this->flg_hazard & 0x2000)
    {
        if (!(globals.player.DamageTimer > 0.0f) && ColPlyrSphere(tartar->rad_cur))
        {
            HurtThePlayer();
            MarkForRecycle();
            return;
        }
    }

    StaggeredCollide();
}

void NPCHazard::ReconArfBone()
{
    Reconfigure(NPC_HAZ_ARFBONEBLAST);
    Start(NULL, -1.0f);
}

void NPCHazard::Upd_BoneBlast(F32 dt)
{
    xVec3 pos_emit = { 0.0f, 0.0f, 0.0f };
    HAZBall* ball = &this->custdata.ball;

    ball->rad_cur = LERP(this->pam_interp, ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f) &&
        ColPlyrSphere(ball->rad_cur))
    {
        HurtThePlayer();
    }

    g_parf_default.custom_flags = 0x100;
    xVec3Copy(&pos_emit, &this->pos_hazard);
    xVec3Copy(&g_parf_default.pos, &pos_emit);

    xParEmitterEmitCustom(g_pemit_default, dt, &g_parf_default);
}

void NPCHazard::Upd_OilBubble(F32 dt)
{
    HAZTarTar* tartar = &this->custdata.tartar;
    xParabola* parab = &tartar->parabinfo;

    if (this->tmr_remain < 0.0f)
    {
        if (this->flg_hazard & 0x20000)
        {
            xParabolaEvalPos(parab, &this->pos_hazard, parab->maxTime);
            if (xVec3Length2(&tartar->dir_normal) > 0.1f)
            {
                OilSplash(&tartar->dir_normal);
            }

            if (!(flg_hazard & 0x40000))
            {
                ReconSlickOil();
                return;
            }
            else
            {
                MarkForRecycle();
                return;
            }
        }
        else
        {
            xVec3 whence;
            xParabolaEvalVel(parab, &whence, parab->maxTime);
            xVec3Inv(&whence, &whence);

            F32 mag = xVec3Length(&whence);
            if (mag > 0.00001f)
            {
                whence /= mag;
                OilSplash(&whence);
            }
            else
            {
                OilSplash((xVec3*)Up());
            }

            MarkForRecycle();
            return;
        }
    }
    else
    {
        if (this->flg_hazard & 0x8)
        {
            xVec3Sub(&tartar->vel, &tartar->pos_tgt, &this->pos_hazard);
            xVec3SMulBy(&tartar->vel, 1.0f / this->tmr_remain);

            tartar->vel.y += 0.2f * (0.5f * this->tmr_remain);
        }

        if (this->flg_hazard & 0x8)
        {
            PreCollide();
        }

        F32 tym = this->tym_lifespan - this->tmr_remain;
        xParabolaEvalPos(parab, &this->pos_hazard, tym);
        xParabolaEvalVel(parab, &tartar->vel, tym);

        F32 mag = xVec3Length(&tartar->vel);
        if (mag > 0.00001f)
        {
            xMat3x3 mat_rot;
            xVec3 dir;

            xVec3SMul(&dir, &tartar->vel, -1.0f / mag);
            xMat3x3LookVec(&mat_rot, &dir);
            TypData_RotMatSet(&mat_rot);
        }

        if (this->flg_hazard & 0x8)
        {
            tartar->streakID = NPCC_StreakCreate(NPC_STRK_OILBUBBLE);
        }

        if (tartar->streakID != 0xDEAD)
        {
            StreakUpdate(tartar->streakID, 0.35f);
        }

        static S32 moreorless = 0;
        if (--moreorless < 0)
        {
            moreorless = 3;

            F32 rad = tartar->rad_cur;
            xVec3 pos = this->pos_hazard;

            xVec3AddScaled(&pos, (xVec3*)Up(), rad * (2.0f * (xurand() - 0.5f)));
            xVec3AddScaled(&pos, (xVec3*)Right(), rad * (2.0f * (xurand() - 0.5f)));

            NPAR_EmitOilTrailz(&pos);
        }

        if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f) &&
            ColPlyrSphere(0.75f * tartar->rad_cur))
        {
            HurtThePlayer();
            NPCC_Slick_MakePlayerSlip(this->npc_owner);

            xVec3 whence;
            xParabolaEvalVel(parab, &whence, parab->maxTime);
            xVec3Inv(&whence, &whence);
            OilSplash(&whence);

            MarkForRecycle();
            return;
        }

        StaggeredCollide();
    }
}

void NPCHazard::ReconSlickOil()
{
    HAZBall* ball = &this->custdata.ball;
    const xVec3 dir_norm = this->custdata.collide.dir_normal;

    Reconfigure(NPC_HAZ_OILSLICK);

    if (xVec3Length2(&dir_norm) > 0.0f)
    {
        xVec3Copy((xVec3*)&this->mdl_hazard->Mat->up, &dir_norm);
        NPCC_MakePerp((xVec3*)&this->mdl_hazard->Mat->at, &dir_norm);
        xVec3Cross((xVec3*)&this->mdl_hazard->Mat->right, (xVec3*)&this->mdl_hazard->Mat->up,
                   (xVec3*)&this->mdl_hazard->Mat->at);

        xMat3x3 mat;
        xMat3x3Rot(&mat, &dir_norm, 2 * PI * xurand());
        xMat3x3Mul(xModelGetFrame(this->mdl_hazard), xModelGetFrame(this->mdl_hazard), &mat);

        F32 dot = xVec3Dot(&dir_norm, &g_Y3);

        if (FABS(dot) < 0.86f)
        {
            ball->rad_max *= 0.5f;
            ball->rad_min *= 0.5f;
            ball->rad_cur *= 0.5f;
        }
    }

    Start(NULL, -1.0f);
}

void NPCHazard::OilSplash(const xVec3* dir_norm)
{
    xVec3 up;
    xVec3 at;
    xVec3 rt;

    if (dir_norm)
    {
        up = *dir_norm;
        NPCC_MakeArbPlane(dir_norm, &at, &rt);
    }
    else
    {
        up = *(xVec3*)Up();
        at = *(xVec3*)At();
        rt = *(xVec3*)Right();
    }

    const xVec3 pos_emit = this->pos_hazard;
    for (S32 i = 0; i < 16; i++)
    {
        xVec3 vel_emit;
        vel_emit = up;

        F32 direction;
        if (xrand() & 0x800000)
        {
            direction = 1.0f;
        }
        else
        {
            direction = -1.0f;
        }

        vel_emit += at * direction * (0.4f * (2.0f * (xurand() - 0.5f)) + 0.25f);

        if (xrand() & 0x800000)
        {
            direction = 1.0f;
        }
        else
        {
            direction = -1.0f;
        }

        vel_emit += rt * direction * (0.4f * (2.0f * (xurand() - 0.5f)) + 0.25f);
        vel_emit.normalize();
        vel_emit *= 15.0f;

        NPAR_EmitOilSplash(&pos_emit, &vel_emit);
    }
}

void NPCHazard::Upd_OilOoze(F32 dt)
{
    static const F32 seg_pam[2] = { 0.15f, 0.75f };
    HAZBall* ball = &this->custdata.ball;

    if (this->pam_interp <= seg_pam[0])
    {
        ball->rad_cur = (ball->rad_max - ball->rad_min) * EASE(this->pam_interp / seg_pam[0]);
        ball->rad_cur += ball->rad_min;
    }
    else if (this->pam_interp >= seg_pam[1])
    {
        ball->rad_cur = (ball->rad_max - ball->rad_min) *
                        EASE(1.0f - ((this->pam_interp - seg_pam[1]) / (1.0f - seg_pam[1])));
        ball->rad_cur += ball->rad_min;
    }
    else
    {
        ball->rad_cur = ball->rad_max;
    }

    if (this->flg_hazard & 0x8 && HAZ_AvailablePool() > 5)
    {
        if (KickOilBurst())
        {
            this->flg_hazard |= 0x40;
        }
    }

    if (this->flg_hazard & 0x8)
    {
        this->tmr_nextglob = 1.5f * (0.25f * (xurand() - 0.5f)) + 1.5f;
    }

    if (this->tmr_nextglob < 0.0f && this->tmr_remain > 1.0f)
    {
        if (KickOilGlobby())
        {
            this->flg_hazard |= 0x40;
        }

        this->tmr_nextglob = 1.5f * (0.25f * (xurand() - 0.5f)) + 1.5f;
    }

    this->tmr_nextglob = -1.0f > this->tmr_nextglob - dt ? -1.0f : this->tmr_nextglob - dt;

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f) &&
        ColPlyrSphere(ball->rad_cur))
    {
        NPCC_Slick_MakePlayerSlip(this->npc_owner);
    }

    if (--this->cnt_nextemit < 0)
    {
        this->cnt_nextemit = 10;

        F32 rad_use = 0.75f * ball->rad_cur;
        xVec3 pos_emit = this->pos_hazard;
        pos_emit += *(xVec3*)this->At() * (2.0f * (xurand() - 0.5f) * rad_use);
        pos_emit += *(xVec3*)this->Right() * (2.0f * (xurand() - 0.5f) * rad_use);
        pos_emit += *(xVec3*)this->Up() * 0.1f;

        NPAR_EmitOilVapors(&pos_emit);
    }
}

S32 NPCHazard::KickOilBurst()
{
    NPCHazard* haz = HAZ_Acquire();

    S32 ok;
    if (haz == NULL)
    {
        ok = 0;
    }
    else if (haz->ConfigHelper(NPC_HAZ_OILBURST) == 0)
    {
        haz->Discard();
        ok = 1;
    }
    else
    {
        haz->SetNPCOwner(this->npc_owner);

        xVec3 pos = this->pos_hazard;
        pos += *(xVec3*)Up() * 0.4f;

        haz->Start(&pos, -1.0f);

        ok = 2;
    }

    return ok;
}

S32 NPCHazard::KickOilGlobby()
{
    HAZBall* ball = &this->custdata.ball;
    NPCHazard* haz = HAZ_Acquire();

    S32 ok;
    if (haz == NULL)
    {
        ok = 0;
    }
    else if (haz->ConfigHelper(NPC_HAZ_OILGLOB) == 0)
    {
        haz->Discard();
        ok = 1;
    }
    else
    {
        haz->SetNPCOwner(this->npc_owner);

        xVec3 pos = this->pos_hazard;
        F32 rad = 0.5f * ball->rad_cur;

        pos += *(xVec3*)At() * (2.0f * (xurand() - 0.5f) * rad);
        pos += *(xVec3*)Right() * (2.0f * (xurand() - 0.5f) * rad);

        haz->Start(&pos, 2.0f < this->tmr_remain ? 2.0f : this->tmr_remain);

        ok = 2;
    }

    return ok;
}

void NPCHazard::Upd_OilBurst(F32 dt)
{
    HAZBall* ball = &this->custdata.ball;

    ball->rad_cur = LERP(this->pam_interp, ball->rad_min, ball->rad_max);

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f))
    {
        if (ColPlyrSphere(0.75f * ball->rad_cur))
        {
            HurtThePlayer();
        }
    }

    if (this->flg_hazard & 0x8)
    {
        xSndPlay3D(xStrHash("Tar_saucehit"), 0.77f, 0.0f, 0x80, 0x0, &this->pos_hazard, 3.0f, 15.0f,
                   SND_CAT_GAME, 0.0f);
    }
}

void NPCHazard::Upd_OilGlob(F32 dt)
{
    HAZShroom* shroom = &this->custdata.shroom;

    F32 rat_quad = SQ(this->pam_interp) / SQ(0.25f);
    if (rat_quad <= 0.25f)
    {
        shroom->rad_cur = LERP(rat_quad, shroom->rad_min, shroom->rad_max);
    }

    this->pos_hazard += shroom->vel_rise * dt;
    shroom->vel_rise += shroom->acc_rise * dt;

    if (this->flg_hazard & 0x2000 && !(globals.player.DamageTimer > 0.0f) &&
        ColPlyrSphere(shroom->rad_cur))
    {
        NPCC_Slick_MakePlayerSlip(this->npc_owner);
    }

    if (--this->cnt_nextemit < 0)
    {
        this->cnt_nextemit = 10;

        F32 rad_use = 0.75f * shroom->rad_cur;
        xVec3 pos_emit = this->pos_hazard;
        pos_emit += *(xVec3*)this->At() * (2.0f * (xurand() - 0.5f) * rad_use);
        pos_emit += *(xVec3*)this->Right() * (2.0f * (xurand() - 0.5f) * rad_use);
        pos_emit += *(xVec3*)this->Up() * 0.1f;

        NPAR_EmitOilVapors(&pos_emit);
    }
}

void NPCHazard::Upd_MonCloud(F32 dt)
{
    xVec3 dir_flat;
    xVec3 dir_delta;
    F32 ds2_owner;
    zNPCCommon* npc_owner = this->npc_owner;

    if (npc_owner != NULL)
    {
        ds2_owner = npc_owner->XZDstSqToPos(&this->pos_hazard, &dir_flat, NULL);
        if (ds2_owner < 1e-05f)
        {
            xVec3Copy(&dir_flat, NPCC_rightDir(npc_owner));
            ds2_owner = 1.0f;
        }

        F32 ds2_home = NPCC_DstSq(&this->custdata.cloud.pos_home, &this->pos_hazard, NULL);

        if (!npc_owner->IsHealthy() || ds2_home > SQ(this->custdata.cloud.rad_maxRange))
        {
            this->tmr_remain = MIN(this->tmr_remain, 1.0f);
        }
        else if (!xEntIsVisible(npc_owner) || globals.cmgr != NULL)
        {
            this->tmr_remain = -1.0f;
            this->MarkForRecycle();
            return;
        }
    }
    else
    {
        ds2_owner = -1.0f;
    }

    if (globals.player.Health < 1)
    {
        this->tmr_remain = MIN(this->tmr_remain, 1.0f);
    }

    xEnt* plyr = &globals.player.ent;

    xVec3Sub(&dir_delta, xEntGetPos(plyr), &this->pos_hazard);
    dir_delta.y += 4.0f;

    F32 dst_plyr = xVec3Length(&dir_delta);

    if (!(ds2_owner < 0.0f) && ds2_owner < 2.0f)
    {
        xVec3SMul(&dir_delta, &dir_flat,
                  (dt * this->custdata.cloud.spd_cloud) * (1.0f / xsqrt(ds2_owner)));
        xVec3AddTo(&this->pos_hazard, &dir_delta);
    }
    else if (dst_plyr > 0.1f)
    {
        xVec3SMulBy(&dir_delta, (dt * this->custdata.cloud.spd_cloud) / dst_plyr);
        xVec3AddTo(&this->pos_hazard, &dir_delta);
    }
    else
    {
        F32 slowit = 0.75f * dt;

        dir_delta.x = dir_delta.x * slowit;
        dir_delta.z = dir_delta.z * slowit;
        xVec3AddTo(&this->pos_hazard, &dir_delta);
    }

    F32 tym_used = this->tym_lifespan - this->tmr_remain;
    F32 rad_span = this->custdata.cloud.rad_max - this->custdata.cloud.rad_min;

    if (tym_used < 0.25f)
    {
        this->custdata.cloud.rad_cur = 4.0f * (tym_used * rad_span);
    }
    else if (this->tmr_remain > 0.25f)
    {
        this->custdata.cloud.rad_cur = this->custdata.cloud.rad_max;
    }
    else
    {
        this->custdata.cloud.rad_cur = 4.0f * (this->tmr_remain * rad_span);
    }

    if ((this->flg_hazard & 0x2000) && !(globals.player.DamageTimer > 0.0f) &&
        this->ColPlyrSphere(this->custdata.cloud.rad_cur))
    {
        this->HurtThePlayer();
    }

    xVec3Sub(&dir_delta, xEntGetPos(plyr), &this->pos_hazard);

    F32 ds2_plyr = SQ(dir_delta.x) + SQ(dir_delta.z);

    if (ds2_plyr > 1.0f && this->custdata.cloud.zap_lytnin == NULL)
    {
        this->custdata.cloud.tmr_dozap = 0.0f;
    }
    else if (xabs(dir_delta.y) > 10.0f)
    {
        this->custdata.cloud.tmr_dozap = 0.0f;
    }
    else
    {
        this->custdata.cloud.tmr_dozap = this->custdata.cloud.tmr_dozap + dt;
    }

    static S32 showLightning = 1;

    if (this->custdata.cloud.zap_lytnin == NULL && ds2_plyr > 2.0f && ds2_owner > 2.0f)
    {
        F32 rate_zap = 1.0f;

        if ((S32)((this->tym_lifespan - this->tmr_remain) * rate_zap) & 1)
        {
            if (this->custdata.cloud.zap_warnin == NULL)
            {
                static xCollis colrec;
                _tagLightningAdd lyt;

                memset(&colrec, 0, sizeof(colrec));
                this->npc_owner->SndPlayRandom(NPC_STYP_LIGHTNING);

                xVec3Copy(&this->custdata.cloud.pos_warnin, &this->pos_hazard);
                this->custdata.cloud.pos_warnin.y = this->custdata.cloud.pos_warnin.y - 7.0f;
                this->custdata.cloud.pos_warnin.x =
                    this->custdata.cloud.rad_cur * (2.0f * (xurand() - 0.5f)) +
                    this->custdata.cloud.pos_warnin.x;
                this->custdata.cloud.pos_warnin.z =
                    this->custdata.cloud.rad_cur * (2.0f * (xurand() - 0.5f)) +
                    this->custdata.cloud.pos_warnin.z;

                if (!NPCC_HaveLOSToPos(&this->pos_hazard, &this->custdata.cloud.pos_warnin,
                                       10.0f, NULL, &colrec))
                {
                    this->custdata.cloud.pos_warnin.y =
                        this->custdata.cloud.pos_warnin.y + (7.0f - colrec.dist);
                }

                NPCC_MakeLightningInfo(NPC_LYT_CLOUDWARN, &lyt);
                lyt.start = &this->pos_hazard;
                lyt.end = &this->custdata.cloud.pos_warnin;
                lyt.time = 1.5f;

                if (showLightning)
                {
                    this->custdata.cloud.zap_warnin = zLightningAdd(&lyt);
                }
            }
            else
            {
                zLightningModifyEndpoints(this->custdata.cloud.zap_warnin, &this->pos_hazard,
                                          &this->custdata.cloud.pos_warnin);
                xVec3Copy(&g_parf_zapwarn.pos, &this->custdata.cloud.pos_warnin);
                xParEmitterEmitCustom(g_pemit_zapwarn, dt, &g_parf_zapwarn);
            }
        }
        else if (this->custdata.cloud.zap_warnin != NULL)
        {
            zLightningKill(this->custdata.cloud.zap_warnin);
            this->custdata.cloud.zap_warnin = NULL;
            xVec3Copy(&this->custdata.cloud.pos_warnin, &g_O3);
        }
    }
    else if (this->custdata.cloud.zap_lytnin == NULL)
    {
        if (this->custdata.cloud.zap_warnin != NULL)
        {
            zLightningKill(this->custdata.cloud.zap_warnin);
            this->custdata.cloud.zap_warnin = NULL;
            xVec3Copy(&this->custdata.cloud.pos_warnin, &g_O3);
        }
    }

    if (this->custdata.cloud.tmr_dozap > 1.0f && this->custdata.cloud.zap_lytnin != NULL)
    {
        if (this->custdata.cloud.tmr_dozap > 1.25f)
        {
            zLightningKill(this->custdata.cloud.zap_lytnin);
            this->custdata.cloud.zap_lytnin = NULL;
            this->custdata.cloud.tmr_dozap = 0.0f;
        }
    }
    else if (this->custdata.cloud.tmr_dozap > 1.0f && ds2_owner > 2.0f)
    {
        _tagLightningAdd lyt;

        NPCC_MakeLightningInfo(NPC_LYT_CLOUDZAP, &lyt);
        lyt.start = &this->pos_hazard;
        lyt.end = xEntGetPos(plyr);
        lyt.time = 1.5f;

        if (showLightning)
        {
            this->custdata.cloud.zap_lytnin = zLightningAdd(&lyt);
        }

        this->HurtThePlayer();
    }
}

void NPCHazard::Upd_FunFrag(F32 dt)
{
    HAZTarTar* tartar = &this->custdata.tartar;
    xParabola* parab = &tartar->parabinfo;

    if (this->flg_hazard & 0x8)
    {
        tartar->vel.x = 2.5f * (2.0f * (xurand() - 0.5f));
        tartar->vel.y = 5.0f * xurand() + 4.0f;
        tartar->vel.z = 2.5f * (2.0f * (xurand() - 0.5f));
    }

    tartar->rad_cur = LERP(this->pam_interp, tartar->rad_min, tartar->rad_max);

    if (this->flg_hazard & 0x8)
    {
        PreCollide();
    }

    F32 tym = this->tym_lifespan < this->tym_lifespan - this->tmr_remain ?
                  this->tym_lifespan :
                  this->tym_lifespan - this->tmr_remain;
    xParabolaEvalPos(parab, &this->pos_hazard, tym);
    xParabolaEvalVel(parab, &tartar->vel, tym);

    static S32 moreorless = 0;
    if (--moreorless < 0)
    {
        moreorless = 3;
        zFX_SpawnBubbleTrail(&this->pos_hazard, 1);
    }
}

void NPCHazard::StreakUpdate(U32 streakID, F32 rad)
{
    xVec3 pos_left;
    pos_left = *(xVec3*)At() * 0.5f * rad;

    xVec3 pos_right;
    pos_right = *(xVec3*)Up() * rad;
    pos_right += pos_left;

    // TODO: These names aren't from DWARF - maybe they could be better after impl
    xVec3 shifted_pos_left;
    shifted_pos_left = *(xVec3*)&this->mdl_hazard->Mat->pos - pos_right;

    xVec3 shifted_pos_right;
    shifted_pos_right = *(xVec3*)&this->mdl_hazard->Mat->pos + pos_right;

    xFXStreakUpdate(streakID, &shifted_pos_left, &shifted_pos_right);
}

void NPCHazard::Upd_RoboBits(F32 dt)
{
    HAZTarTar* tartar = &this->custdata.tartar;
    xParabola* parab = &tartar->parabinfo;

    if (this->pam_interp > 0.25f)
    {
        tartar->rad_cur = tartar->rad_max;
    }
    else
    {
        tartar->rad_cur = LERP(4.0f * this->pam_interp, tartar->rad_min, tartar->rad_max);
    }

    if (this->flg_hazard & 0x8)
    {
        tartar->vel = tartar->pos_tgt - this->pos_hazard;
        tartar->vel /= this->tmr_remain;
        tartar->vel.y += 10.0f * (0.5f * this->tmr_remain);
    }

    if (this->flg_hazard & 0x8)
    {
        F32 keepFlightTime = this->tmr_remain;
        PreCollide();
        this->tmr_remain = keepFlightTime;
        this->tym_lifespan = keepFlightTime;
    }

    F32 tym = this->tym_lifespan < this->tym_lifespan - this->tmr_remain ?
                  this->tym_lifespan :
                  this->tym_lifespan - this->tmr_remain;
    xParabolaEvalPos(parab, &this->pos_hazard, tym);
    xParabolaEvalVel(parab, &tartar->vel, tym);

    static S32 moreorless = 0;
    if (--moreorless < 0)
    {
        moreorless = 3;
        DisperseBubWake(tartar->rad_cur, &tartar->vel);
    }
}

void NPCHazard::Upd_VisSplash(F32 dt)
{
    this->custdata.collide.rad_cur =
        LERP(this->pam_interp, this->custdata.collide.rad_min, this->custdata.collide.rad_max);

    if (this->flg_hazard & 0x8)
    {
        g_data_hazshrap[4]->initCB(g_data_hazshrap[4], this->mdl_hazard, (xVec3*)Up(), NULL);
    }

    VisSplashSparklies();
}

void NPCHazard::VisSplashSparklies()
{
    F32 rad = this->custdata.collide.rad_cur;
    for (S32 i = 0; i < 8; i++)
    {
        xVec3 pos_emit;
        pos_emit = *(xVec3*)At() * (2.0f * (xurand() - 0.5f));
        pos_emit += *(xVec3*)Right() * (2.0f * (xurand() - 0.5f));
        pos_emit.normalize();
        pos_emit *= rad;
        pos_emit += *(xVec3*)Up() * 0.2f;
        pos_emit += this->pos_hazard;

        xVec3 vel_emit;
        vel_emit = *(xVec3*)Up();
        vel_emit *= 3.5f;

        NPAR_EmitVSSpray(&pos_emit, &vel_emit);
    }
}

S32 UVAModelInfo::Init(RpAtomic* model, U32 flg_uvamCaller)
{
    this->model = model;
    this->flg_uvam = flg_uvamCaller;

    if (model == NULL)
    {
        return 0;
    }

    if (this->flg_uvam & 1)
    {
        RpGeometry* geo = model->geometry;

        if (geo == NULL)
        {
            return 0;
        }

        geo->flags |= 0x40;
        this->SetColor(xColorFromRGBA(0xff, 0xff, 0xff, 0xff));
    }

    if (!this->CloneUV(this->uv, this->uvsize, model))
    {
        return 0;
    }

    this->offset.assign(0.0f, 0.0f);
    this->offset_vel.assign(0.0f, 0.0f);

    return 1;
}

void UVAModelInfo::Hemorrage()
{
    model = 0;
    uv = 0;
}

void UVAModelInfo::Update(F32 dt, const xVec2* use_offset)
{
    if (use_offset != NULL)
    {
        this->offset = *use_offset;
    }
    else if (xVec2Length2(&this->offset_vel) > 0.0f)
    {
        this->offset += this->offset_vel * dt;
    }
    else
    {
        return;
    }

    this->offset.x = xfmod(this->offset.x, 1.0f);
    this->offset.y = xfmod(this->offset.y, 1.0f);

    this->Refresh();
}

void UVAModelInfo::Refresh()
{
    RpGeometry* geo = this->model->geometry;
    RwTexCoords* src = this->uv;
    RwTexCoords* dst = geo->texCoords[0];
    RwTexCoords* end = &this->uv[this->uvsize];

    // 0x10 is rpGEOMETRYLOCKTEXCOORDS1; the enum is not declared in any header here.
    RpGeometryLock(geo, 0x10);

    while (src < end)
    {
        dst->u = src->u + this->offset.x;
        dst->v = src->v + this->offset.y;
        src++;
        dst++;
    }

    RpGeometryUnlock(geo);
}

void UVAModelInfo::SetColor(iColor_tag color)
{
    RpGeometry* geo = model->geometry;

    RwRGBA col;
    col.red = color.r;
    col.green = color.g;
    col.blue = color.b;
    col.alpha = color.a;

    int numMats = model->geometry->matList.numMaterials;

    for (int i = 0; i < numMats; i++)
    {
        geo->matList.materials[i]->color = col;
    }
}

S32 UVAModelInfo::GetUV(RwTexCoords*& coords, S32& numVertices, RpAtomic* model) const
{
    coords = NULL;
    numVertices = 0;
    RpGeometry* geom = model->geometry;
    if (geom == NULL)
    {
        return 0;
    }
    numVertices = geom->numVertices;
    if (numVertices <= 0)
    {
        return 0;
    }

    coords = geom->texCoords[0];

    return coords != NULL;
}

S32 UVAModelInfo::CloneUV(RwTexCoords*& coords, S32& numVertices, RpAtomic* model) const
{
    RwTexCoords* src;

    if (!GetUV(src, numVertices, model))
    {
        return 0;
    }

    coords = (RwTexCoords*)xMemAlloc(gActiveHeap, numVertices * sizeof(RwTexCoords), 0);
    if (coords == NULL)
    {
        return 0;
    }

    memcpy(coords, src, numVertices * sizeof(RwTexCoords));
    return 1;
}

F32 xVec2Length2(const xVec2* v)
{
    return xVec2Dot(v, v);
}
