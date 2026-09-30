#include "zNPCGlyph.h"
#include "zNPCSupport.h"
#include "zGlobals.h"
#include "zRenderState.h"
#include "xMath.h"
#include "xMathInlines.h"
#include "xMath3.h"
#include "xstransvc.h"

#include <types.h>

char* g_strz_glyphmodel[10] = {
    "unknown",
    "shiny_obj_purple.dff",
    "shiny_obj_blue.dff",
    "shiny_obj_green.dff",
    "shiny_obj_yellow.dff",
    "shiny_obj_orange.dff",
    "fx_glyph_talk",
    "fx_glyph_talkother",
    "fx_glyph_friend",
    "fx_glyph_daze.dff",
};
S32 g_cnt_activeGlyphs[10] = {};

static NPCGlyph g_glyphs_talk[8] = { NPC_GLYPH_UNKNOWN };
static NPCGlyph g_glyphs_talkOther[8] = { NPC_GLYPH_UNKNOWN };
static NPCGlyph g_glyphs_friend[1] = { NPC_GLYPH_UNKNOWN };
static NPCGlyph g_glyphs_dazed[8] = { NPC_GLYPH_UNKNOWN };
static NPCGlyph g_glyphs_shinyOne[16] = { NPC_GLYPH_UNKNOWN };
static NPCGlyph g_glyphs_shinyFive[16] = { NPC_GLYPH_UNKNOWN };
static NPCGlyph g_glyphs_shinyTen[16] = { NPC_GLYPH_UNKNOWN };
static NPCGlyph g_glyphs_shinyFifty[16] = { NPC_GLYPH_UNKNOWN };
static NPCGlyph g_glyphs_shinyHundred[16] = { NPC_GLYPH_UNKNOWN };

// This function is put here to force the string to be linked in, since it isn't used anywhere.
char* __deadstripped_zNPCGlyph()
{
    return "%s.MINF";
}

void zNPCGlyph_Startup()
{
}

void zNPCGlyph_Shutdown()
{
}

// Nonmatching
void zNPCGlyph_ScenePrepare()
{
    RpAtomic* mdl_raw;
    NPCGlyph* glyph;
    U32 aid;
    S32 i;
    S32 k;
    S32 cnt;
    NPCGlyph* list = NULL;

    for (i = 0; i < 10; i++)
    {
        g_cnt_activeGlyphs[i] = 0;
    }

    for (i = NPC_GLYPH_SHINYONE; i < NPC_GLYPH_NOMORE; i++)
    {
        cnt = zNPCGlyph_TypeToList((en_npcglyph)i, &list);

        if (list == NULL || cnt < 1)
        {
            continue;
        }

        mdl_raw = NULL;
        if (g_strz_glyphmodel[i] != NULL)
        {
            aid = xStrHash(g_strz_glyphmodel[i]);
            if (aid != 0)
            {
                mdl_raw = (RpAtomic*)xSTFindAsset(aid, NULL);
            }
        }

        for (k = 0; k < cnt; k++)
        {
            glyph = &list[k];
            glyph->Init((en_npcglyph)i, mdl_raw);
        }
    }
}

void zNPCGlyph_SceneFinish()
{
    NPCGlyph* glist = NULL;
    S32 i;
    for (en_npcglyph gtyp = NPC_GLYPH_SHINYONE; gtyp < NPC_GLYPH_NOMORE;
         gtyp = (en_npcglyph)(gtyp + 1))
    {
        S32 cnt = zNPCGlyph_TypeToList(gtyp, &glist);

        if (glist == NULL || cnt < 1)
        {
            continue;
        }

        for (i = 0; i < cnt; i++)
        {
            NPCGlyph* glyph = &glist[i];
            glyph->Kill();
        }
    }
}

void zNPCGlyph_SceneReset()
{
}

void zNPCGlyph_ScenePostInit()
{
}

void zNPCGlyph_Timestep(F32 dt)
{
    NPCGlyph* glist = NULL;
    S32* counts = &g_cnt_activeGlyphs[NPC_GLYPH_SHINYONE];
    S32 i;

    for (en_npcglyph gtyp = NPC_GLYPH_SHINYONE; gtyp < NPC_GLYPH_NOMORE;
         gtyp = (en_npcglyph)(gtyp + 1), counts++)
    {
        S32 cnt = zNPCGlyph_TypeToList(gtyp, &glist);
        if (glist == NULL || cnt < 1 || *counts < 1)
        {
            continue;
        }
        for (i = 0; i < cnt; i++)
        {
            NPCGlyph* glyph = &glist[i];
            if (!(glyph->flg_glyph & (1 << 0)))
            {
                continue;
            }

            if (glyph->flg_glyph & (1 << 4) && glyph->tmr_glyph < 0.0f)
            {
                glyph->Discard();
                continue;
            }

            glyph->tmr_glyph = MAX(-1.0f, glyph->tmr_glyph - dt);

            if (glyph->mdl_glyph != NULL && glyph->flg_glyph & (1 << 5))
            {
                glyph->RotAddDelta(NULL);
            }
            if (glyph->flg_glyph & ((1 << 3) | (1 << 2)))
            {
                glyph->Timestep(dt);
            }
        }
    }
}

S32 zNPCGlyph_TypeToList(en_npcglyph gtyp, NPCGlyph** glist)
{
    *glist = NULL;
    S32 cnt = 0;

    switch (gtyp)
    {
    case NPC_GLYPH_SHINYONE:
        cnt = 16;
        *glist = g_glyphs_shinyOne;
        break;
    case NPC_GLYPH_SHINYFIVE:
        cnt = 16;
        *glist = g_glyphs_shinyFive;
        break;
    case NPC_GLYPH_SHINYTEN:
        cnt = 16;
        *glist = g_glyphs_shinyTen;
        break;
    case NPC_GLYPH_SHINYFIFTY:
        cnt = 16;
        *glist = g_glyphs_shinyFifty;
        break;
    case NPC_GLYPH_SHINYHUNDRED:
        cnt = 16;
        *glist = g_glyphs_shinyHundred;
        break;
    case NPC_GLYPH_TALK:
        cnt = 8;
        *glist = g_glyphs_talk;
        break;
    case NPC_GLYPH_TALKOTHER:
        cnt = 8;
        *glist = g_glyphs_talkOther;
        break;
    case NPC_GLYPH_FRIEND:
        cnt = 1;
        *glist = g_glyphs_friend;
        break;
    case NPC_GLYPH_DAZED:
        cnt = 8;
        *glist = g_glyphs_dazed;
        break;
    }

    return cnt;
}

S32 zNPCGlyph_TypeIsOpaque(en_npcglyph gtyp)
{
    switch (gtyp)
    {
    case NPC_GLYPH_SHINYONE:
    case NPC_GLYPH_SHINYFIVE:
    case NPC_GLYPH_SHINYTEN:
    case NPC_GLYPH_SHINYFIFTY:
    case NPC_GLYPH_SHINYHUNDRED:
    case NPC_GLYPH_TALK:
    case NPC_GLYPH_TALKOTHER:
    case NPC_GLYPH_FRIEND:
        return 1;
    case NPC_GLYPH_DAZED:
        return 0;
    default:
        return 1;
    }
}

S32 zNPCGlyph_TypeNeedsLightKit(en_npcglyph gtyp)
{
    switch (gtyp)
    {
    case NPC_GLYPH_SHINYONE:
    case NPC_GLYPH_SHINYFIVE:
    case NPC_GLYPH_SHINYTEN:
    case NPC_GLYPH_SHINYFIFTY:
    case NPC_GLYPH_SHINYHUNDRED:
        return 0;
    case NPC_GLYPH_TALK:
    case NPC_GLYPH_TALKOTHER:
    case NPC_GLYPH_FRIEND:
    case NPC_GLYPH_DAZED:
        return 1;
    default:
        return 1;
    }
}

// Nonmatching
void zNPCCommon_Glyphs_RenderAll(S32 doOpaqueStuff)
{
    S32 k;
    S32 i;
    NPCGlyph* glyph;
    S32 cnt;
    NPCGlyph* list = NULL;
    _SDRenderState old_rendstat = zRenderStateCurrent();

    if (doOpaqueStuff)
    {
        zRenderState(SDRS_OpaqueModels);
    }
    else
    {
        zRenderState(SDRS_NPCVisual);
    }

    for (i = NPC_GLYPH_SHINYONE; i < NPC_GLYPH_NOMORE; i++)
    {
        if ((!doOpaqueStuff || zNPCGlyph_TypeIsOpaque((en_npcglyph)i)) &&
            (doOpaqueStuff || !zNPCGlyph_TypeIsOpaque((en_npcglyph)i)))
        {
            if (zNPCGlyph_TypeNeedsLightKit((en_npcglyph)i))
            {
                xLightKit_Enable(globals.player.ent.lightKit, globals.currWorld);
            }
            else
            {
                xLightKit_Enable(NULL, globals.currWorld);
            }

            cnt = zNPCGlyph_TypeToList((en_npcglyph)i, &list);

            if (list == NULL || cnt < 1 || g_cnt_activeGlyphs[i] < 1)
            {
                continue;
            }

            for (k = 0; k < cnt; k++)
            {
                glyph = &list[k];
                if (glyph->flg_glyph & (1 << 0) && glyph->flg_glyph & (1 << 1))
                {
                    glyph->Render();
                }
            }
        }
    }

    xLightKit_Enable(NULL, globals.currWorld);
    zRenderState(old_rendstat);
}

NPCGlyph* GLYF_Acquire(en_npcglyph type)
{
    NPCGlyph* glist = NULL;
    NPCGlyph* ret = NULL;
    S32 cnt = zNPCGlyph_TypeToList(type, &glist);

    if (glist == NULL || cnt < 1)
    {
        return NULL;
    }

    NPCGlyph* glyph = NULL;
    for (S32 i = 0; cnt > 0; i++, cnt--)
    {
        glyph = &glist[i];
        if (glyph->flg_glyph & (1 << 0))
        {
            continue;
        }

        glyph->Reset();
        glyph->flg_glyph = (1 << 0);
        g_cnt_activeGlyphs[type]++;
        ret = glyph;
        break;
    }

    return ret;
}

void NPCGlyph::Reset()
{
    flg_glyph = 0;
    xVec3Copy(&pos_glyph, &g_O3);
    xVec3Copy(&vel_glyph, &g_O3);
    tmr_glyph = 0.0f;
}

void NPCGlyph::Init(en_npcglyph gtyp, RpAtomic* model_data)
{
    xVec3 vec = {};

    typ_glyph = gtyp;

    if (model_data != NULL)
    {
        mdl_glyph = xModelInstanceAlloc(model_data, NULL, 0, 0, NULL);
        if (mdl_glyph != NULL)
        {
            xMat4x3 frame;
            xMat3x3Euler(&frame, &vec);
            xVec3Copy(&frame.pos, &g_O3);
            frame.flags = 0;
            xModelSetFrame(mdl_glyph, &frame);
        }
    }

    Reset();
}

void NPCGlyph::Kill()
{
    if (mdl_glyph != NULL)
    {
        xModelInstanceFree(mdl_glyph);
    }
    typ_glyph = NPC_GLYPH_UNKNOWN;
    flg_glyph = 0;
    xVec3Copy(&pos_glyph, &g_O3);
    xVec3Copy(&vel_glyph, &g_O3);
    mdl_glyph = NULL;
}

void NPCGlyph::Render()
{
    xModelInstance* model = mdl_glyph;
    if (model == NULL)
    {
        return;
    }

    if (flg_glyph & (1 << 6))
    {
        xVec3Copy(&model->Scale, &scl_glyph);
        xModelRender(mdl_glyph);
        xVec3Copy(&mdl_glyph->Scale, &g_Onez);
    }
    else
    {
        xModelRender(model);
    }
}

void NPCGlyph::Enable(S32 ison)
{
    if (ison)
    {
        flg_glyph |= (1 << 1);
    }
    else
    {
        flg_glyph &= ~(1 << 1);
    }
}

void NPCGlyph::Discard()
{
    en_npcglyph type = typ_glyph;
    if (type > NPC_GLYPH_UNKNOWN && type < NPC_GLYPH_NOMORE)
    {
        g_cnt_activeGlyphs[type]--;
        g_cnt_activeGlyphs[typ_glyph] = MAX(0, g_cnt_activeGlyphs[typ_glyph]);
    }

    flg_glyph = 0;
}

void NPCGlyph::PosSet(xVec3* pos)
{
    if (pos != NULL)
    {
        xVec3Copy(&pos_glyph, pos);
    }
    else
    {
        xVec3Copy(&pos_glyph, &g_O3);
    }

    if (mdl_glyph != NULL)
    {
        xVec3Copy((xVec3*)&mdl_glyph->Mat->pos, &pos_glyph);
    }
}

void NPCGlyph::VelSet(xVec3* vel)
{
    if (vel != NULL)
    {
        xVec3Copy(&vel_glyph, vel);
    }
    else
    {
        xVec3Copy(&vel_glyph, &g_O3);
    }

    if (vel == NULL)
    {
        flg_glyph &= ~(1 << 2);
    }
    else if (xVec3Length2(vel) > 0.0f)
    {
        flg_glyph |= (1 << 2);
    }
    else
    {
        flg_glyph &= ~(1 << 2);
    }
}

void NPCGlyph::ScaleSet(xVec3* scale)
{
    if (scale != NULL)
    {
        xVec3Copy(&scl_glyph, scale);
        flg_glyph |= (1 << 6);
    }
    else
    {
        xVec3 one = { 1.0f, 1.0f, 1.0f };
        xVec3Copy(&scl_glyph, &one);
        flg_glyph &= ~(1 << 6);
    }
}

void NPCGlyph::RotSet(xVec3* ang, S32 doautospin)
{
    xMat3x3 mat_rot = {};
    xMat3x3Euler(&mat_rot, ang);
    RotSet(&mat_rot, doautospin);
}

void NPCGlyph::RotSet(xMat3x3* mat_rot, S32 doautospin)
{
    xMat3x3Copy(&rot_glyph, mat_rot);
    if (doautospin)
    {
        flg_glyph |= (1 << 5);
    }
    else
    {
        flg_glyph &= ~(1 << 5);
    }
}

void NPCGlyph::VelSet_Shiny()
{
    tmr_glyph = -1.0f;
    xVec3 local_18 = {};

    {
        xVec3 temp;
        // Do we use sph here?
        temp.x = globals.player.ent.bound.sph.center.x - pos_glyph.x;
        temp.y = globals.player.ent.bound.sph.center.y - pos_glyph.y;
        temp.z = globals.player.ent.bound.sph.center.z - pos_glyph.z;
        xVec3Normalize(&temp, &temp);
        xVec3Copy(&local_18, &temp);
    }

    F32 local_28 = 1.0f;
    // Do we use sph here?
    F32 dx__ = globals.player.ent.bound.sph.center.x - pos_glyph.x;
    F32 dy__ = globals.player.ent.bound.sph.center.y - pos_glyph.y;
    F32 dz__ = globals.player.ent.bound.sph.center.z - pos_glyph.z;
    xsqrtfast(local_28, SQR(dx__) + SQR(dy__) + SQR(dz__));

    F32 fVar1 = 1.9f * (2.0f * local_28 + globals.player.PredictCurrVel);
    // Can be simplified with `CLAMP(fVar1, 3.5f, 45.0f)`
    if (fVar1 > 45.0f)
    {
        fVar1 = 45.0f;
    }
    if (fVar1 < 3.5f)
    {
        fVar1 = 3.5f;
    }
    xVec3SMul(&local_18, &local_18, fVar1);
    VelSet(&local_18);
}

void NPCGlyph::RotAddDelta(xMat3x3* mat_rot)
{
    if (mat_rot == NULL)
    {
        mat_rot = &rot_glyph;
    }

    xMat4x3* frame = xModelGetFrame(mdl_glyph);
    xMat3x3Mul(frame, mat_rot, frame);
    xModelSetFrame(mdl_glyph, frame);
}

void NPCGlyph::Timestep(F32 dt)
{
    xVec3 vec = {};

    switch (typ_glyph)
    {
    case NPC_GLYPH_SHINYONE:
    case NPC_GLYPH_SHINYFIVE:
    case NPC_GLYPH_SHINYTEN:
    case NPC_GLYPH_SHINYFIFTY:
    case NPC_GLYPH_SHINYHUNDRED:
    {
        VelSet_Shiny();
        RotAddDelta(NULL);

        // Do we use sph here?
        F32 x_delta = globals.player.ent.bound.sph.center.x - pos_glyph.x;
        F32 y_delta = globals.player.ent.bound.sph.center.y - pos_glyph.y;
        F32 z_delta = globals.player.ent.bound.sph.center.z - pos_glyph.z;
        if (SQR(x_delta) + SQR(y_delta) + SQR(z_delta) <= SQR(globals.player.ent.bound.sph.r))
        {
            tmr_glyph = -1.0f;
        }

        break;
    }
    case NPC_GLYPH_TALK:
    case NPC_GLYPH_TALKOTHER:
    {
        if (mdl_glyph == NULL)
        {
            break;
        }

        xVec3 vec2;
        if (NPCC_ds2_toCam(&pos_glyph, &vec2) < 0.1f)
        {
            break;
        }

        vec2.y = 0.0f;
        if (SQ(vec2.x) + SQ(vec2.z) < 0.1f)
        {
            break;
        }

        vec2.normalize();
        xMat4x3* frame = xModelGetFrame(mdl_glyph);
        frame->pos = pos_glyph;
        frame->up = g_Y3;
        frame->right = vec2;
        xVec3Cross(&frame->at, &frame->right, &g_Y3);
        xModelSetFrame(mdl_glyph, frame);
        break;
    }
    case NPC_GLYPH_FRIEND:
    case NPC_GLYPH_DAZED:
        if (flg_glyph & (1 << 2))
        {
            xVec3Copy(&vec, &pos_glyph);
            xVec3AddScaled(&vec, &vel_glyph, dt);
            PosSet(&vec);
        }
        break;
    }
}
