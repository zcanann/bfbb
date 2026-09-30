#include "zNPCSupplement.h"
#include "zNPCFXCinematic.h"
#include "zNPCSupport.h"
#include "zNPCTypeRobot.h"
#include "zGame.h"
#include "zGameExtras.h"
#include "zGlobals.h"
#include "xCutsceneMgr.h"

#include "xFX.h"
#include "xMath.h"
#include "xMathInlines.h"
#include "iMath.h"

#include <types.h>
#include <rwplcore.h>

// MSL's <cmath> is not reachable from here; the target calls floorf__3stdFf.
namespace std
{
    float floorf(float x);
}

U32 xShadowReceiveShadowSetup(xEnt* ent);
void xShadowReceiveShadow(xEnt* ent, F32 factor, S32 flags, RwMatrixTag* mat, RwRaster* rast);

static const NPARParmOilBub g_parm_oilbub[4] = {
    {
        1.2f, 
        {255, 255, 255, 160}, 
        {0.0f, 2.0f, 0.0f}, 
        {0.1f, 0.4f}
    },
    {
        0.5f,
        {255, 255, 255, 160},
        {0.0f, -8.0f, 0.0f},
        {0.1f, 0.8f}
    },
    {
        0.8f,
        {255, 255, 255, 160},
        {0.0f, 3.0f, 0.0f},
        {0.05f, 0.25f}
    },
    {
        1.2f,
        {255, 255, 255, 255},
        {0.0f, -10.0f, 0.0f},
        {0.2f, 0.3f}
    }
};

static const NPARParmTubeSpiral g_parm_tubespiral[4] = {
    {
        {204, 96, 204, 255},
        {0.1f, 1.0f},
    },
    {
        {255, 100, 70, 255},
        {0.1f, 1.0f}
    },
    {
        {204, 96, 204, 255},
        {0.1f, 0.25f}
    },
    {
        {255, 100, 70, 255},
        {0.1f, 0.25f}
    }
};

static const NPARParmTubeConfetti g_parm_tubeconfetti[2] = {
    {
        2.5f,
        {255, 255, 255, 255},
        {0.3f, 0.5f},
        {0.0f, -2.5f, 0.0f},
        {0.25f, 0.25f},
        0,
        {4, 2},
        85
    },
    {
        1.0f,
        {255, 255, 255, 255},
        {0.1f, 0.15f},
        {0.0f, -3.5f, 0.0f},
        {0.25f, 0.25f},
        0,
        {4, 2},
        97
    }
};

static const NPARParmGloveDust g_parm_glovedust[1] = {
    {
        0.4f,
        {64, 32, 0, 128},
        {0.1f, 0.5f},
        {0.0f, -2.33f, 0.0f}
    }
};

static const NPARParmMonsoonRain g_parm_monsoonrain = {
    0.2f,
    {160, 128, 64, 255},
    {0.1f, 0.2f},
    {0.0f, -0.33f, 0.0f}
};

static const NPARParmSleepyZeez g_parm_sleepyzeez[1] = {
    {
        2.0f,
        {221, 221, 221, 255},
        {0.1f, 1.2f},
        {0.0f, 0.33f, 0.0f},
        {0.5f, 0.5f},
        0,
        {1, 1},
        100
    }
};

static const NPARParmChuckSplash g_parm_chucksplash[5] = {
    {
        0.1f,
        {0, 0, 255, 255},
        {0.3f, 0.6f},
        {0.0f, -3.0f, 0.0f},
        0
    },
    {
        0.4f,
        {136, 136, 255, 255},
        {0.2f, 0.8f},
        {0.0f, -6.5f, 0.0f},
        90
    },
    {
        1.6f,
        {68, 68, 255, 255},
        {0.2f, 1.2f},
        {0.0f, -4.0f, 0.0f},
        90
    },
    {
        0.5f,
        {136, 136, 255, 255},
        {0.2f, 1.0f},
        {0.0f, -50.0f, 0.0f},
        102
    },
    {
        0.25f,
        {136, 136, 255, 128},
        {0.05f, 0.25f},
        {0.0f, -10.0f, 0.0f},
        100
    }
};

static const NPARParmVisSplash g_parm_vissplash[1] = {
    {
        0.25f,
        {136, 136, 255, 255},
        {0.2f, 0.75f},
        {0.0f, -20.0f},
        92
    }
};

static const NPARParmTarTarGunk g_parm_tartargunk[6] = {
    {
        0.25f,
        {240, 240, 64, 255},
        {0.05f, 0.6f},
        {0.0f, -10.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        100
    },
    {
        0.3f,
        {200, 255, 255, 128},
        {0.5f, 2.1f},
        {0.0f, 10.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        100
    },
    {
        0.75f,
        {200, 255, 255, 128},
        {0.2f, 1.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        100
    },
    {
        0.5f,
        {200, 255, 255, 128},
        {0.5f, 0.5f},
        {0.0f, -60.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        80
    },
    {
        0.5f,
        {200, 255, 255, 128},
        {0.05f, 0.2f},
        {0.0f, 3.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        90
    },
    {
        1.5f,
        {200, 255, 255, 128},
        {0.3f, 0.7f},
        {0.0f, 20.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        50
    },
};

static const NPARParmDogBreath g_parm_dogbreath[3] = {
    {
        5.0f,
        {101, 132, 0, 128},
        {0.05f, 0.6f},
        {0.0f, 5.0f, 0.0f},
        100,
        {}
    },
    {
        0.5f,
        {101, 132, 0, 128},
        {0.06f, 0.2f},
        {0.0f, 30.0f, 0.0f},
        60,
        {}
    },
    {
        1.5f,
        {101, 132, 0, 200},
        {0.05f, 1.4f},
        {0.0f, 10.0f, 0.0f},
        85,
        {}
    }
};
static const NPARParmFahrwerkz g_parm_fahrwerkz[4] = {
    {
        0.25f,
        {240, 240, 64, 255},
        {0.1f, 0.1f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        100
    },
    {
        0.3f,
        {200, 255, 255, 128},
        {0.1f, 0.1f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        100
    },
    {
        0.75f,
        {200, 255, 255, 128},
        {0.1f, 0.1f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        100
    },
    {
        0.5f,
        {200, 255, 255, 128},
        {0.1f, 0.1f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 1.0f},
        0,
        {1, 1},
        80
    },
};

static S32 g_gameExtrasFlags;
static S32 g_isSpecialDay;
static S32 g_mon; // month
static S32 g_day; // day
static S32 g_doNPARCull = 1;

void NPAR_CopyNPARToPTPool(NPARData* param_1, ptank_pool__pos_color_size_uv2* param_2);
static void NPAR_TubeSpiralMagic(RwRGBA* color, int unused, F32 pam);

void NPCSupplement_Startup()
{
    return;
}

void NPCSupplement_Shutdown()
{
    NPCWidget_Shutdown();
}

void NPCSupplement_ScenePrepare()
{
    NPAR_ScenePrepare();
}

void NPCSupplement_SceneFinish()
{
    NPAR_SceneFinish();
}

void NPCSupplement_ScenePostInit()
{
    NPAR_PartySetup(NPAR_TYP_VISSPLASH, 0, 0);
}

void NPCSupplement_SceneReset(void)
{
    NPAR_SceneReset();
    NPCC_ShadowCacheReset();
}

void NPCSupplement_Timestep(float dt)
{
    NPAR_Timestep(dt);
}

void NPCC_MakeLightningInfo(en_npclyt style, _tagLightningAdd* info)
{
    switch (style)
    {
    case NPC_LYT_PLACEHOLDER:
        info->type = 3;
        info->total_points = 4;
        info->thickness = 0.3f;
        info->color = xColorFromRGBA(0xff, 0xff, 0xff, 0xff);
        info->rand_radius = 1.0f;
        info->arc_height = 0.2f;
        info->rot_radius = 0.5f;
        info->time = 1.0f;
        info->flags = 0x2c;
        info->setup_degrees = 20.0f + 90.0f * xurand();
        info->move_degrees = 1280.0f + 12360.0f * xurand();
        if (xrand() & 0x800000)
        {
            info->move_degrees *= -1.0f;
        }
        break;
    case NPC_LYT_JELLYFISH:
        info->type = 3;
        info->total_points = 10;
        info->thickness = 0.15f;
        info->color = xColorFromRGBA(0xff, 0x00, 0xff, 0xff);
        info->rand_radius = 1.0f;
        info->arc_height = 0.2f;
        info->rot_radius = 0.5f;
        info->time = 1.0f;
        info->flags = 0x2c;
        info->setup_degrees = 20.0f + 90.0f * xurand();
        info->move_degrees = 1280.0f + 12360.0f * xurand();
        if (xrand() & 0x800000)
        {
            info->move_degrees *= -1.0f;
        }
        break;
    case NPC_LYT_JELLYFISHBLUE:
        info->type = 3;
        info->total_points = 10;
        info->color = xColorFromRGBA(0x20, 0x20, 0xff, 0xff);
        info->thickness = 0.15f;
        info->rand_radius = 1.0f;
        info->arc_height = 0.2f;
        info->rot_radius = 0.5f;
        info->time = 1.0f;
        info->flags = 0x2c;
        info->setup_degrees = 20.0f + 90.0f * xurand();
        info->move_degrees = 1280.0f + 12360.0f * xurand();
        if (xrand() & 0x800000)
        {
            info->move_degrees *= -1.0f;
        }
        break;
    case NPC_LYT_CATTLEPROD:
        info->type = 3;
        info->total_points = 4;
        info->thickness = 0.1f;
        info->color = xColorFromRGBA(200, 200, 0xff, 0xff);
        info->rand_radius = 0.15f;
        info->arc_height = 0.1f;
        info->rot_radius = 0.1f;
        info->time = 1.0f;
        info->flags = 0xc28;
        info->setup_degrees = 100.0f * (0.25f * (xurand() - 0.5f)) + 100.0f;
        info->move_degrees = 8000.0f * (0.25f * (xurand() - 0.5f)) + 8000.0f;
        if (xrand() & 0x800000)
        {
            info->move_degrees *= -1.0f;
        }
        break;
    case NPC_LYT_TIKITHUNDER:
        info->type = 3;
        info->total_points = 4;
        info->thickness = 1.0f;
        info->color = xColorFromRGBA(0xff, 0xff, 0xff, 0xff);
        info->rand_radius = 1.0f;
        info->arc_height = 0.2f;
        info->rot_radius = 0.5f;
        info->time = 1.0f;
        info->flags = 0x2c;
        info->setup_degrees = 20.0f + 90.0f * xurand();
        info->move_degrees = 1280.0f + 12360.0f * xurand();
        if (xrand() & 0x800000)
        {
            info->move_degrees *= -1.0f;
        }
        break;
    case NPC_LYT_SLEEPYARC:
        info->type = 3;
        info->total_points = 8;
        info->thickness = 0.1f;
        info->color = xColorFromRGBA(0x20, 0x20, 0xff, 0xff);
        info->rand_radius = 0.2f;
        info->arc_height = 0.1f;
        info->rot_radius = 0.25f;
        info->time = 1.0f;
        info->flags = 0x2c;
        info->setup_degrees = 20.0f + 90.0f * xurand();
        info->move_degrees = 1280.0f + 12360.0f * xurand();
        if (xrand() & 0x800000)
        {
            info->move_degrees *= -1.0f;
        }
        break;
    case NPC_LYT_CLOUDWARN:
        info->type = 3;
        info->total_points = 12;
        info->thickness = 0.4f;
        info->color = xColorFromRGBA(0x96, 0x40, 0x80, 0xff);
        info->rand_radius = 1.0f;
        info->arc_height = 0.2f;
        info->rot_radius = 0.5f;
        info->time = 1.0f;
        info->flags = 0x428;
        info->setup_degrees = 20.0f;
        info->move_degrees = 1280.0f;
        if (xrand() & 0x800000)
        {
            info->move_degrees *= -1.0f;
        }
        break;
    case NPC_LYT_CLOUDZAP:
        info->type = 3;
        info->total_points = 8;
        info->thickness = 0.3f;
        info->color = xColorFromRGBA(0x96, 0x40, 0x80, 0xff);
        info->rand_radius = 1.0f;
        info->arc_height = 0.2f;
        info->rot_radius = 0.5f;
        info->time = 1.0f;
        info->flags = 0x428;
        info->setup_degrees = 20.0f;
        info->move_degrees = 1280.0f;
        if (xrand() & 0x800000)
        {
            info->move_degrees *= -1.0f;
        }
        break;
    }
}

void NPCC_MakeStreakInfo(en_npcstreak styp, StreakInfo* info)
{
    info->Defaults();

    switch (styp)
    {
    case NPC_STRK_TARTARBLOB:
        info->freq = 0.0f;
        info->rgba_left.r = 0xf0;
        info->rgba_left.g = 0xf0;
        info->rgba_left.b = 0x40;
        info->rgba_left.a = 0xff;
        info->rgba_right.a = 0xf0;
        info->rgba_right.g = 0xf0;
        info->rgba_right.b = 0x40;
        info->rgba_right.a = 0x40;
        break;
    case NPC_STRK_OILBUBBLE:
        info->freq = 0.0f;
        info->alf_fade = 1.2f;
        info->alf_start = 0.65f;
        info->rgba_right.r = 0x00;
        info->rgba_right.g = 0x00;
        info->rgba_right.b = 0x00;
        info->rgba_right.a = 0xff;
        info->rgba_left.r = 0x40;
        info->rgba_left.g = 0x40;
        info->rgba_left.b = 0x40;
        info->rgba_left.a = 0x80;
        break;
    case NPC_STRK_ARFMELEE:
        info->rgba_right.r = 0xff;
        info->rgba_right.g = 0xff;
        info->rgba_right.b = 0xff;
        info->rgba_right.a = 0xff;
        info->rgba_left.r = 0xf0;
        info->rgba_left.g = 0xf0;
        info->rgba_left.b = 0xf0;
        info->rgba_left.a = 0xf0;
        info->freq = 1.0f / 60.0f;
        break;
    case NPC_STRK_HAMMERSMASH_VERT:
        info->rgba_right.r = 0xb0;
        info->rgba_right.g = 0x30;
        info->rgba_right.b = 0x00;
        info->rgba_right.a = 0xcc;
        info->rgba_left.r = 0x00;
        info->rgba_left.g = 0xff;
        info->rgba_left.b = 0x00;
        info->rgba_left.a = 0xff;
        info->freq = -1.0f;
        info->taper = 1;
        break;
    case NPC_STRK_HAMMERSMASH_HORZ:
        info->rgba_right.r = 0x66;
        info->rgba_right.g = 0x20;
        info->rgba_right.b = 0x40;
        info->rgba_right.a = 0xaa;
        info->rgba_left.r = 0x66;
        info->rgba_left.g = 0x20;
        info->rgba_left.b = 0x40;
        info->rgba_left.a = 0xaa;
        info->freq = -1.0f;
        info->taper = 1;
        break;
    case NPC_STRK_TOSSEDROBOT:
        info->rgba_right.r = 0xff;
        info->rgba_right.g = 0xff;
        info->rgba_right.b = 0xff;
        info->rgba_right.a = 0xff;
        info->rgba_left.r = 0xf0;
        info->rgba_left.g = 0xf0;
        info->rgba_left.b = 0xf0;
        info->rgba_left.a = 0xf0;
        info->freq = 0.1f;
        break;
    case NPC_STRK_TOSSEDJELLY:
        info->rgba_right.r = 0xff;
        info->rgba_right.g = 0x40;
        info->rgba_right.b = 0xff;
        info->rgba_right.a = 0xff;
        info->rgba_left.r = 0xf0;
        info->rgba_left.g = 0x40;
        info->rgba_left.b = 0xf0;
        info->rgba_left.a = 0xf0;
        info->freq = 0.025f;
        break;
    case NPC_STRK_TOSSEDJELLYBLUE:
        info->rgba_right.r = 0x40;
        info->rgba_right.g = 0x40;
        info->rgba_right.b = 0xff;
        info->rgba_right.a = 0xff;
        info->rgba_left.r = 0x40;
        info->rgba_left.g = 0x40;
        info->rgba_left.b = 0xf0;
        info->rgba_left.a = 0xf0;
        info->freq = 0.025f;
        break;
    }
}

void StreakInfo::Defaults()
{
    freq = 0.25f;
    alf_fade = 4.0f;
    alf_start = 1.0f;
    idx_useTxtr = 0;
    rgba_left.r = rgba_left.g = rgba_left.b = rgba_left.a = 0xff;
    rgba_right.r = rgba_right.g = rgba_right.b = rgba_right.a = 0xff;
    taper = 1;
}

U32 xFXStreakStart(StreakInfo* styp)
{
    return xFXStreakStart(styp->freq, styp->alf_fade, styp->alf_start, styp->idx_useTxtr,
                          &styp->rgba_left, &styp->rgba_right, styp->taper);
}

U32 NPCC_StreakCreate(en_npcstreak styp)
{
    static StreakInfo info;
    NPCC_MakeStreakInfo(styp, &info);
    return xFXStreakStart(&info);
}

// .bss and .data ordering put these here in retail: info$NNNN from
// NPCC_StreakCreate is the first .bss object, and g_npar_info follows both
// switch tables in .data.
static NPARMgmt g_npar_mgmt[12];
static xShadowCache g_shadCaches[16];
static S8 g_shadCachesInUseFlags[16];

static NPARInfo g_npar_info[12] = {
    {},
    {
        NPAR_Upd_OilBubble, 128, "fx_oil_bubble", 0x2
    },
    {
        NPAR_Upd_TubeSpiral, 256, "fx_tubelet_flame", 0x2
    },
    {
        NPAR_Upd_TubeConfetti, 64, "fx_tubelet_confetti_sep", 0x2
    },
    {
        NPAR_Upd_GloveDust, 64, "fx_glove_dust", 0x2
    },
    {
        NPAR_Upd_MonsoonRain, 64, "fx_monsoon_rain", 0x2
    },
    {
        NPAR_Upd_SleepyZeez, 32, "fx_sleepy_zeez", 0x2
    },
    {
        NPAR_Upd_ChuckSplash, 128, "fx_chuck_dripdrop", 0x2
    },
    {
        NPAR_Upd_TarTarGunk, 256, "fx_tartar_gunk", 0x2
    },
    {
        NPAR_Upd_DogBreath, 64, "fx_chomper_breath", 0x2
    },
    {
        NPAR_Upd_VisSplash, 32, "fx_vissplash_sep", 0x2
    },
    {
        NPAR_Upd_Fireworks, 128, "fx_firework", 0x0
    }
};

void NPCC_BurstBubble(en_npcburst burst, xVec3* pos_base)
{
    S32 j;
    S32 i;

    for (i = 0; i < 16; i++)
    {
        F32 hyt = 0.375f * i;
        F32 rat_hyt = 0.5f * hyt / 3.0f;
        F32 rad_cur = 3.0f;

        rad_cur *= ARCH3(rat_hyt);

        for (j = 0; j < 20; j++)
        {
            F32 curang = 0.31415927f * xurand() + 0.31415927f * j;
            curang = CLAMP(curang, 0.0f, 6.2831855f);

            F32 fs = isin(curang);
            F32 fc = icos(curang);

            xVec3 pos_emit;
            pos_emit.x = rad_cur * fs + 0.1f * (2.0f * (xurand() - 0.5f));
            pos_emit.y = 0.1f * (2.0f * (xurand() - 0.5f)) + hyt;
            pos_emit.z = rad_cur * fc + 0.1f * (2.0f * (xurand() - 0.5f));

            xVec3AddTo(&pos_emit, pos_base);

            if (burst == NPC_BURST_SHIELD)
            {
                NPAR_EmitOilShieldPop(&pos_emit);
            }
            else
            {
                zFX_SpawnBubbleTrail(&pos_emit, 1);
            }
        }
    }
}

void NPCC_MakeASplash(const xVec3* pos, F32 radius)
{
    F32 rad_splash = radius;
    F32 tym_splash = 0.35f;

    if (rad_splash < 0.0f)
    {
        rad_splash = 1.5f;
    }
    else
    {
        tym_splash = LERP(CLAMP((rad_splash - 1.5f) / 8.5f, 0.0f, 1.0f), 0.25f, 1.15f);
        if (1.5f > rad_splash)
        {
            rad_splash = 1.5f;
        }
    }

    NPCHazard* haz = HAZ_Acquire();
    if (haz != NULL)
    {
        if (!haz->ConfigHelper(NPC_HAZ_VISSPLASH))
        {
            haz->Discard();
        }
        else
        {
            haz->SetNPCOwner(NULL);
            haz->custdata.typical.rad_max = rad_splash;
            haz->Start(pos, tym_splash);
        }
    }
}

void NPCC_Slick_MakePlayerSlip(zNPCCommon* npc)
{
    globals.player.ForceSlipperyTimer = MAX(globals.player.ForceSlipperyTimer, 4.0f);
    globals.player.ForceSlipperyFriction = 0.01f;

    if (npc != NULL)
    {
        ((zNPCSlick*)npc)->YouOwnSlipFX();
    }
}

void NPCC_RenderProjTexture(RwRaster* rast, F32 factor, xMat4x3* mat, F32 radius, F32 height,
                            xShadowCache* cache, S32 fillCache, xEnt* ent)
{
    if (fillCache)
    {
        xShadowVertical_FillCache(cache, &mat->pos, radius, height, 0.0871557f);
    }

    gShadowObjectRadius = radius;
    xShadowVertical_DrawCache(cache, factor, 0.0f, 1, (RwMatrixTag*)mat, rast);

    for (U32 i = 0; i < cache->entCount; i++)
    {
        xEnt* ep = cache->ent[i];

        if (ep != ent && ep->baseType != eBaseTypeNPC && xShadowReceiveShadowSetup(ep))
        {
            xShadowReceiveShadow(ep, factor, 1, (RwMatrixTag*)mat, rast);
        }
    }
}

void NPCC_RenderProjTextureFaceCamera(RwRaster* rast, F32 factor, xVec3* pos, F32 radius,
                                      F32 height, xShadowCache* cache, S32 fillCache, xEnt* ent)
{
    xMat4x3 matrix;

    xVec3Copy(&matrix.pos, pos);
    xVec3Copy(&matrix.right, &globals.camera.mat.right);

    if (matrix.right.y < -0.0001f || matrix.right.y > 0.0001f)
    {
        matrix.right.y = 0.0f;

        F32 len = xVec3Length(&matrix.right);
        if (len < 0.0001f)
        {
            xVec3Init(&matrix.right, 1.0f, 0.0f, 0.0f);
        }
        else
        {
            xVec3SMulBy(&matrix.right, 1.0f / len);
        }
    }

    xVec3Init(&matrix.at, 0.0f, -1.0f, 0.0f);
    xVec3Init(&matrix.up, -matrix.right.z, 0.0f, matrix.right.x);

    RwMatrixUpdate((RwMatrixTag*)&matrix);

    NPCC_RenderProjTexture(rast, factor, &matrix, radius, height, cache, fillCache, ent);
}

void NPAR_Upd_OilBubble(NPARMgmt* mgmt, F32 dt)
{
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 6;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmOilBub* npparm = &g_parm_oilbub[npdata->nparmode];

        npdata->tmr_remain -= dt;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;

        npdata->pos += npdata->vel * dt;
        npdata->vel += npparm->acc_oilBubble * dt;
        npdata->vel *= 0.9f;

        F32 arch = ARCH(1.0f - rat);

        F32 dim;
        if (npdata->nparmode == 1)
        {
            dim = SMOOTH(rat, npparm->siz_base[0], npparm->siz_base[1]);
        }
        else
        {
            dim = LERP(arch, npparm->siz_base[0], npparm->siz_base[1]);
        }

        npdata->xy_size[0] = dim;
        npdata->xy_size[1] = dim;
        npdata->color.alpha = arch * npparm->colr_base.alpha;

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_ScenePrepare()
{
    for (int i = 0; i < 12; i++)
    {
        g_npar_mgmt[i].Clear();
    }
}

void NPAR_SceneFinish()
{
    for (int i = 0; i < 12; i++)
    {
        g_npar_mgmt[i].Done();
    }
    g_gameExtrasFlags = 0;
    g_mon = 0;
    g_day = 0;
}

void NPAR_SceneReset()
{
    for (int i = 0; i < 12; i++)
    {
        g_npar_mgmt[i].Reset();
    }
}

void NPAR_CheckSpecials()
{
    g_gameExtrasFlags = zGameExtras_ExtrasFlags();
    zGameExtras_MoDay(&g_mon, &g_day);
    g_isSpecialDay = g_gameExtrasFlags & 0b111110111;
}

void NPAR_Timestep(F32 dt)
{
    S32 isPawzd = zGameIsPaused();
    S32 isCine = (globals.cmgr != NULL) && (((xCutsceneMgr*)(globals.cmgr))->csn != NULL);
    NPAR_CheckSpecials();
    NPARMgmt* mgr;
    for (S32 i = 0; i < 12; i++, mgr++)
    {
        mgr = &g_npar_mgmt[i];
        if (1 <= mgr->cnt_active)
        {
            if (!isPawzd || ((mgr->flg_npar & 1)))
            {
                if (!isCine || ((mgr->flg_npar & 2)))
                {
                    mgr->UpdateAndRender(dt);
                }
            }
        }
    }
}

NPARMgmt* NPAR_PartySetup(en_nparptyp parType, void** userData, NPARXtraData* xtraData)
{
    NPARMgmt* mgmt = &g_npar_mgmt[parType];
    S32 isReady = mgmt->IsReady();
    if (isReady)
    {
        return mgmt;
    }
    mgmt->Init(parType, userData, xtraData);
    return mgmt;
}

NPARMgmt* NPAR_FindParty(en_nparptyp parType)
{
    NPARMgmt* mgmt = &g_npar_mgmt[parType];
    S32 isReady = mgmt->IsReady();
    if (isReady)
    {
        return mgmt;
    }
    mgmt = NULL;
    return mgmt;
}

void NPARMgmt::Init(en_nparptyp parType, void** userData, NPARXtraData* xtraData)
{
    U32 amt = g_npar_info[parType].num_maxParticles;
    NPARInfo* info = &g_npar_info[parType];

    void* mem = xMemAlloc(gActiveHeap, amt * sizeof(NPARData), 0x10);
    memset(mem, 0, amt * sizeof(NPARData));

    typ_npar = parType;
    flg_npar = info->flg_npar;
    par_buf = (NPARData*)mem;
    cnt_active = 0;
    num_max = info->num_maxParticles;

    UserDataSet(userData);
    XtraDataSet(xtraData);

    if (info->nam_texture == NULL)
    {
        txtr = NULL;
    }
    else
    {
        txtr = NPCC_FindRWTexture(info->nam_texture);
    }
}

void NPARMgmt::Clear()
{
    typ_npar = NPAR_TYP_UNKNOWN;
    flg_npar = 0;
    par_buf = 0;
    cnt_active = 0;
    num_max = 0;
    txtr = 0;
    xtra_data = 0;
    user_data = 0;
}

void NPARMgmt::UpdateAndRender(F32 param_1)
{
    g_npar_info[typ_npar].fun_update(this, param_1);
}

void NPAR_CopyNPARToPTPool(NPARData* param_1, ptank_pool__pos_color_size_uv2* param_2)
{
    *param_2->pos = param_1->pos;
    param_2->color->r = (param_1->color).red;
    param_2->color->g = (param_1->color).green;
    param_2->color->b = (param_1->color).blue;
    param_2->color->a = (param_1->color).alpha;
    param_2->size->x = param_1->xy_size[0];
    param_2->size->y = param_1->xy_size[1];
    param_2->uv->x = param_1->uv_tl[0];
    param_2->uv->y = param_1->uv_tl[1];
    param_2->uv[1].x = param_1->uv_br[0];
    param_2->uv[1].y = param_1->uv_br[1];
}

void NPAR_Upd_TubeSpiral(NPARMgmt* mgmt, F32 dt)
{
    static const F32 useFixedTimestepForSpiral = 1.0f / 60.0f;
    static const F32 seg_allowCollide[2] = { 0.0f, 0.9f };

    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 2;
    pool.rs.flags = 0;
    pool.reset();

    xVec3 pos_plyr = *xEntGetCenter(&globals.player.ent);

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmTubeSpiral* npparm = &g_parm_tubespiral[npdata->nparmode];

        npdata->tmr_remain -= useFixedTimestepForSpiral;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;
        F32 rat_rev = 1.0f - rat;

        npdata->pos += npdata->vel * useFixedTimestepForSpiral;

        F32 arch = ARCH(rat_rev);

        F32 dim;
        if (rat_rev < 0.2f)
        {
            dim = SMOOTH(rat_rev / 0.2f, npparm->siz_base[0], npparm->siz_base[1]);
        }
        else
        {
            dim = npparm->siz_base[1];
        }

        npdata->xy_size[0] = 2.5f * dim;
        npdata->xy_size[1] = dim;

        if (g_isSpecialDay)
        {
            NPAR_TubeSpiralMagic(&npdata->color, g_isSpecialDay, rat_rev);
            npdata->color.alpha = rat * npparm->colr_base.alpha;
        }
        else
        {
            npdata->color.alpha = rat * npparm->colr_base.alpha;
        }

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (!(globals.player.DamageTimer > 0.0f) && globals.player.Health &&
                rat_rev > seg_allowCollide[0] && rat_rev < seg_allowCollide[1] &&
                npdata->color.alpha > 0x20)
            {
                F32 ds2_plyr = NPCC_DstSq(&npdata->pos, &pos_plyr, NULL);
                if (ds2_plyr < 0.5f)
                {
                    zEntPlayer_DamageNPCKnockBack(NULL, 1, &npdata->pos);
                }

                static S32 howfreq = 0;
            }

            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

// The colour statics are declared in the order retail laid them out in .sdata2;
// the anonymous-counter gaps in the target ($1467, $1471-3, $1482, $1484) are
// statics that no longer survive, and only the layout order is reproducible.
static void NPAR_TubeSpiralMagic(RwRGBA* color, int unused, F32 pam)
{
    static const RwRGBA colr_julyred = { 0xdd, 0x00, 0x00, 0xff };
    static const RwRGBA colr_julywhite = { 0xcc, 0xcc, 0xcc, 0xff };
    static const RwRGBA colr_julyblue = { 0x00, 0x00, 0xdd, 0xff };
    static const RwRGBA colr_red = { 0xff, 0x00, 0x00, 0xff };
    static const RwRGBA colr_orange = { 0xff, 0xa5, 0x00, 0xff };
    static const RwRGBA colr_yellow = { 0xff, 0xff, 0x00, 0xff };
    static const RwRGBA colr_green = { 0x00, 0xff, 0x00, 0xff };
    static const RwRGBA colr_blue = { 0x00, 0x00, 0xff, 0xff };
    static const RwRGBA colr_indigo = { 0x19, 0x19, 0x70, 0xff };
    static const RwRGBA colr_lavender = { 0xc6, 0x09, 0xe9, 0xff };
    static const RwRGBA colr_kellygreen = { 0x0a, 0x7f, 0x03, 0xff };
    static const RwRGBA colr_pinkRyanz = { 0xcc, 0x60, 0xcc, 0xff };
    static const RwRGBA colr_fuschia = { 0xbc, 0x40, 0x99, 0xff };
    static const RwRGBA colr_neon_red = { 0xff, 0x20, 0x00, 0xff };
    static const RwRGBA colr_neon_green = { 0x20, 0xff, 0x00, 0xff };
    static const RwRGBA colr_neon_blue = { 0x20, 0x20, 0xff, 0xff };
    static const RwRGBA colr_peach = { 0xf0, 0x80, 0x80, 0xff };
    static const RwRGBA colr_maroon = { 0x80, 0x00, 0x00, 0xff };
    static const RwRGBA colr_seagreen = { 0x80, 0xcc, 0x99, 0xff };
    static const RwRGBA colr_khaki = { 0xf0, 0xe6, 0x8c, 0xff };
    static const RwRGBA colr_cyan = { 0x00, 0xff, 0xff, 0xff };
    static const RwRGBA colr_pimp_gold = { 0xd7, 0xdc, 0x13, 0xff };

    static RwRGBA zanyArray[10] = { colr_neon_red, colr_yellow,   colr_neon_green, colr_neon_blue,
                                    colr_fuschia,  colr_peach,    colr_maroon,     colr_seagreen,
                                    colr_khaki,    colr_cyan };

    // Lots of different dates
    if (g_isSpecialDay & 0b100000001)
    {
        S32 trun = 10.0f * pam;
        if (trun < 0)
        {
            trun = 0;
        }
        if (trun > 9)
        {
            trun = 9;
        }
        *color = zanyArray[trun];
        return;
    }

    // July 4th (Independence Day)
    if (g_isSpecialDay & 0b000000010)
    {
        if (pam < (2.0f / 7.0f))
        {
            *color = colr_julyred;
            return;
        }
        if (pam < (4.0f / 7.0f))
        {
            *color = colr_julywhite;
            return;
        }
        *color = colr_julyblue;
        return;
    }

    // March 17th (St. Patrick's Day)
    if (g_isSpecialDay & 0b000000100)
    {
        *color = colr_kellygreen;
        return;
    }

    // March 15th? I believe this is disabled by the operation in NPAR_CheckSpecials
    if (g_isSpecialDay & 0b000001000)
    {
        *color = colr_pimp_gold;
        return;
    }

    // Also 4th of July? Unused
    if (g_isSpecialDay & 0b000010000)
    {
        if (pam < 0.125f)
        {
            *color = colr_maroon;
            return;
        }
        if (pam < 0.25f)
        {
            *color = colr_julyred;
            return;
        }
        if (pam < 0.375f)
        {
            *color = colr_julywhite;
            return;
        }

        if (pam < 0.5f)
        {
            *color = colr_julyblue;
            return;
        }
        *color = colr_indigo;
        return;
    }

    // October 31st (Halloween)
    if (g_isSpecialDay & 0b000100000)
    {
        *color = colr_orange;
        return;
    }

    // June 6th
    if (g_isSpecialDay & 0b001000000)
    {
        if (pam < 0.25f)
        {
            *color = colr_red;
            return;
        }
        if (pam < 0.375f)
        {
            *color = colr_orange;
            return;
        }
        if (pam < 0.5f)
        {
            *color = colr_green;
            return;
        }
        if (pam < 0.625f)
        {
            *color = colr_blue;
            return;
        }
        *color = colr_lavender;
        return;
    }

    // April 1st (April Fools)
    if (g_isSpecialDay & 0b010000000)
    {
        *color = colr_pinkRyanz;
        return;
    }
}

F32 ARCH3(F32 param_1)
{
    return 1.0f - BOWL3(param_1);
}

F32 BOWL3(F32 param_1)
{
    return QUB((F32)2.0f * (F32)iabs(param_1 - 0.5f));
}

F32 QUB(F32 param_1)
{
    return param_1 * param_1 * param_1;
}

F32 ARCH(F32 param_1)
{
    return 1.0f - BOWL(param_1);
}

F32 BOWL(F32 param_1)
{
    return SQ((F32)2.0f * (param_1 - 0.5f));
}

void NPARMgmt::Done()
{
    Clear();
}

void NPARMgmt::Reset()
{
    cnt_active = 0;
}

S32 NPARMgmt::IsReady()
{
    return num_max != 0 && par_buf != 0;
}

void NPARMgmt::XtraDataSet(NPARXtraData* param_1)
{
    xtra_data = param_1;
}

void NPARMgmt::UserDataSet(void** param_1)
{
    user_data = param_1;
}

void NPARMgmt::PromoteTail(S32 idx)
{
    cnt_active--;
    par_buf[idx] = par_buf[cnt_active];
}

NPARData* NPARMgmt::NextAvail()
{
    if (cnt_active >= num_max)
    {
        return NULL;
    }

    NPARData* par = &par_buf[cnt_active++];
    memset(par, 0, sizeof(NPARData));

    return par;
}

void NPARParmVisSplash::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = xurand() * 0.5f + 0.5f;
    F32 samecalc = tym_lifespan * fac_rand;

    par->fac_abuse = fac_rand;
    par->tmr_remain = samecalc;
    par->tym_exist = samecalc;

    par->pos = *pos;
    par->vel = *vel;

    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];

    par->color = colr_base;

    par->uv_tl[0] = 0.0f;
    par->uv_tl[1] = 0.0f;
    par->uv_br[0] = 1.0f;
    par->uv_br[1] = 1.0f;

    par->flg_popts |= 2;
    par->nparmode = pmod;
}

void NPARParmDogBreath::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = xurand() * 0.5f + 0.5f;
    F32 samecalc = tym_lifespan * fac_rand;

    par->fac_abuse = fac_rand;
    par->tmr_remain = samecalc;
    par->tym_exist = samecalc;

    par->pos = *pos;
    par->vel = *vel;

    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];

    par->color = colr_base;

    par->uv_tl[0] = 0.0f;
    par->uv_tl[1] = 0.0f;
    par->uv_br[0] = 1.0f;
    par->uv_br[1] = 1.0f;

    par->nparmode = pmod;
}

void NPAR_EmitFireworks(en_nparmode pmod, const xVec3* pos, const xVec3* vel)
{
    NPARData *pNVar1;
    NPARMgmt *mgmt = NPAR_FindParty(NPAR_TYP_FIREWORKS);
    if ((mgmt != NULL) && (pNVar1 = mgmt->NextAvail(), pNVar1 != NULL))
    {
        g_parm_fahrwerkz[pmod].ConfigPar(pNVar1, pmod, pos, vel);
    }
}

void NPAR_EmitFWExhaust(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitFireworks(NPAR_MODE_FWEXHAUST, pos, vel);
}


void NPAR_EmitVisSplash(en_nparmode pmod, const xVec3* vel, const xVec3* pos)
{
    NPARData* par;
    NPARMgmt* mgmt = NPAR_FindParty(NPAR_TYP_VISSPLASH);
    if ((mgmt != NULL) && (par = mgmt->NextAvail(), par != NULL))
    {
        g_parm_vissplash[pmod].ConfigPar(par, pmod, vel, pos);
    }
}


void NPAR_EmitOilBubble(en_nparmode pmod, const xVec3* pos, const xVec3* vel)
{
    NPARData* par;
    NPARMgmt* mgmt = NPAR_FindParty(NPAR_TYP_OILBUB);
    if ((mgmt != NULL) && (par = mgmt->NextAvail(), par != NULL))
    {
        g_parm_oilbub[pmod].ConfigPar(par, pmod, pos, vel);
    }
}


// Equivalent: weird unnecessary use of mulli to index into g_parm_tubespiral.
void NPAR_EmitTubeSpiral(const xVec3* pos, const xVec3* vel, F32 lifespan)
{
    NPARData* par;
    NPARMgmt* mgmt = NPAR_FindParty(NPAR_TYP_TUBESPIRAL);
    if ((mgmt != NULL) && (par = mgmt->NextAvail(), par != NULL))
    {
        en_nparmode pmod = NPAR_MODE_SPIRALNORM;
        g_parm_tubespiral[pmod].ConfigPar(par, pmod, pos, vel, lifespan);
    }
}


void NPAR_EmitTubeConfetti(const xVec3* pos, const xVec3* vel)
{
    NPARData* par;
    NPARMgmt* mgmt = NPAR_FindParty(NPAR_TYP_TUBECONFETTI);
    if ((mgmt != NULL) && (par = mgmt->NextAvail(), par != NULL))
    {
        g_parm_tubeconfetti[0].ConfigPar(par, NPAR_MODE_STD, pos, vel);
    }
}

void NPAR_EmitTubeSparklies(const xVec3* pos, const xVec3* vel)
{
    NPARData* par;
    NPARMgmt* mgmt = NPAR_FindParty(NPAR_TYP_TUBECONFETTI);
    if ((mgmt != NULL) && (par = mgmt->NextAvail(), par != NULL))
    {
        g_parm_tubeconfetti[1].ConfigPar(par, NPAR_MODE_FETTI_SPARKLIES, pos, vel);
    }
}

// Equivalent: operands of single fmuls instruction swapped.
void NPARParmTubeConfetti::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = 0.5f * xurand() + 0.5f;

    par->fac_abuse = fac_rand;
    par->tmr_remain = tym_lifespan * fac_rand;
    par->tym_exist  = tym_lifespan * fac_rand;
    par->pos = *pos;
    par->vel = *vel;
    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];
    par->color = colr_base;
    
    if (pmod == 0)
    {
        par->color.red   = xurand() * 95.0f + 160.0f;
        par->color.green = xurand() * 95.0f + 160.0f;
        par->color.blue  = xurand() * 95.0f + 160.0f;
        par->color.alpha = colr_base.alpha;

        par->flg_popts |= 4;
    }
    else
    {
        par->color = colr_base;
    }

    F32 justTheRand = fac_rand;

    F32 du = 1.0f / num_uvcell[0];
    F32 dv = 1.0f / num_uvcell[1];

    if (pmod == 0)
    {
        F32 samecalc = 2.0f * (justTheRand - 0.5f );
        S32 idx_cell = samecalc * num_uvcell[1];
        par->uv_tl[0] = idx_cell * du;
        par->uv_tl[1] = (row_uvstart + (int)(samecalc * num_uvcell[0])) * dv;
        par->uv_br[0] = par->uv_tl[0] + du;
        par->uv_br[1] = par->uv_tl[1] + dv;

        par->flg_popts |= 2;
    }
    else
    {
        par->uv_tl[0] = 0.0f;
        par->uv_tl[1] = row_uvstart * dv;
        par->uv_br[0] = du;
        par->uv_br[1] = dv;

        par->flg_popts &= ~2;
    }

    par->nparmode = pmod;
}

void NPAR_Upd_TubeConfetti(NPARMgmt* mgmt, F32 dt)
{
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 6;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmTubeConfetti* npparm = &g_parm_tubeconfetti[npdata->nparmode];

        npdata->tmr_remain -= dt;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;
        F32 rat_rev = 1.0f - rat;
        F32 beans = dt * npdata->fac_abuse;
        F32 fac_keep = MAX(0.0f, MIN(0.01f * npparm->pct_keep, 100.0f));

        npdata->pos += npdata->vel * dt;
        npdata->vel *= fac_keep;
        npdata->vel += npparm->acc_base * beans;

        if (npdata->nparmode == 0)
        {
            npdata->vel.x += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
            npdata->vel.z += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
        }

        F32 arch = ARCH(rat_rev);
        F32 dim = LERP(arch, npparm->siz_base[0], npparm->siz_base[1]);

        npdata->xy_size[0] = dim;
        npdata->xy_size[1] = dim;
        npdata->color.alpha = rat * npparm->colr_base.alpha;

        F32 uv_du = npparm->uv_scroll[0];
        F32 uv_dv = npparm->uv_scroll[1];

        S32 popts = npdata->flg_popts;

        if (npdata->nparmode && !(popts & 2))
        {
            if (popts & 1)
            {
                S32 r = npparm->num_uvcell[0] * xurand();
                S32 c = npparm->num_uvcell[1] * xurand();

                npdata->uv_tl[0] = c * uv_du;
                npdata->uv_tl[1] = (npparm->row_uvstart + r) * uv_dv;
                npdata->uv_br[0] = npdata->uv_tl[0] + uv_du;
                npdata->uv_br[1] = npdata->uv_tl[1] + uv_dv;
            }
            else
            {
                S32 num_cells = npparm->num_uvcell[0] * npparm->num_uvcell[1];
                S32 cell = std::floorf(rat_rev * num_cells);
                S32 col = cell % npparm->num_uvcell[0];
                S32 row = (cell - col) / npparm->num_uvcell[0];

                npdata->uv_tl[0] = col * uv_du;
                npdata->uv_tl[1] = (npparm->row_uvstart + row) * uv_dv;
                npdata->uv_br[0] = npdata->uv_tl[0] + uv_du;
                npdata->uv_br[1] = npdata->uv_tl[1] + uv_dv;
            }
        }

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_Upd_GloveDust(NPARMgmt* mgmt, F32 dt)
{
    const NPARParmGloveDust* npparm = &g_parm_glovedust[0];
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 6;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        npdata->tmr_remain -= dt;

        F32 rat_rev = 1.0f - MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;

        npdata->pos += npdata->vel * dt;

        F32 arch = ARCH(rat_rev);

        F32 dim;
        if (rat_rev < 0.2f)
        {
            dim = SMOOTH(rat_rev / 0.2f, npparm->siz_base[0], npparm->siz_base[1]);
        }
        else
        {
            dim = npparm->siz_base[1];
        }

        npdata->xy_size[0] = 2.5f * dim;
        npdata->xy_size[1] = dim;
        npdata->color.alpha = (1.0f - rat_rev) * npparm->colr_base.alpha;

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_Upd_MonsoonRain(NPARMgmt* mgmt, F32 dt)
{
    const NPARParmMonsoonRain* npparm = &g_parm_monsoonrain;
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 6;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        npdata->tmr_remain -= dt;

        F32 rat_rev = 1.0f - MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;

        npdata->pos += npdata->vel * dt;

        F32 arch = ARCH(rat_rev);

        F32 dim;
        if (rat_rev < 0.2f)
        {
            dim = SMOOTH(rat_rev / 0.2f, npparm->siz_base[0], npparm->siz_base[1]);
        }
        else
        {
            dim = npparm->siz_base[1];
        }

        npdata->xy_size[0] = 2.5f * dim;
        npdata->xy_size[1] = dim;
        npdata->color.alpha = (1.0f - rat_rev) * npparm->colr_base.alpha;

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_Upd_SleepyZeez(NPARMgmt* mgmt, F32 dt)
{
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 2;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmSleepyZeez* npparm = &g_parm_sleepyzeez[npdata->nparmode];

        npdata->tmr_remain -= dt;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;
        F32 rat_rev = 1.0f - rat;
        F32 beans = dt * npdata->fac_abuse;
        F32 fac_keep = MAX(0.0f, MIN(0.01f * npparm->pct_keep, 100.0f));

        npdata->pos += npdata->vel * dt;
        npdata->vel *= fac_keep;
        npdata->vel += npparm->acc_base * beans;

        if (npdata->nparmode == 0)
        {
            npdata->vel.x += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
            npdata->vel.z += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
        }

        F32 arch = ARCH(rat_rev);
        F32 dim = LERP(arch, npparm->siz_base[0], npparm->siz_base[1]);

        npdata->xy_size[0] = dim;
        npdata->xy_size[1] = dim;
        npdata->color.alpha = rat * npparm->colr_base.alpha;

        F32 uv_du = npparm->uv_scroll[0];
        F32 uv_dv = npparm->uv_scroll[1];

        S32 popts = npdata->flg_popts;

        if (npdata->nparmode && !(popts & 2))
        {
            if (popts & 1)
            {
                S32 r = npparm->num_uvcell[0] * xurand();
                S32 c = npparm->num_uvcell[1] * xurand();

                npdata->uv_tl[0] = c * uv_du;
                npdata->uv_tl[1] = (npparm->row_uvstart + r) * uv_dv;
                npdata->uv_br[0] = npdata->uv_tl[0] + uv_du;
                npdata->uv_br[1] = npdata->uv_tl[1] + uv_dv;
            }
            else
            {
                S32 num_cells = npparm->num_uvcell[0] * npparm->num_uvcell[1];
                S32 cell = std::floorf(rat_rev * num_cells);
                S32 col = cell % npparm->num_uvcell[0];
                S32 row = (cell - col) / npparm->num_uvcell[0];

                npdata->uv_tl[0] = col * uv_du;
                npdata->uv_tl[1] = (npparm->row_uvstart + row) * uv_dv;
                npdata->uv_br[0] = npdata->uv_tl[0] + uv_du;
                npdata->uv_br[1] = npdata->uv_tl[1] + uv_dv;
            }
        }

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_Upd_ChuckSplash(NPARMgmt* mgmt, F32 dt)
{
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 2;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmChuckSplash* npparm = &g_parm_chucksplash[npdata->nparmode];

        npdata->tmr_remain -= dt;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;
        F32 rat_rev = 1.0f - rat;
        F32 beans = dt * npdata->fac_abuse;
        F32 fac_keep = MAX(0.0f, MIN(0.01f * npparm->pct_keep, 100.0f));

        npdata->pos += npdata->vel * dt;
        npdata->vel *= fac_keep;
        npdata->vel += npparm->acc_base * beans;

        if (npdata->nparmode == 0)
        {
            npdata->vel.x += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
            npdata->vel.z += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
        }

        if (npdata->nparmode == 3)
        {
            F32 dim = LERP(xurand(), npparm->siz_base[0], npparm->siz_base[1]);

            npdata->xy_size[0] = dim;
            npdata->xy_size[1] = dim;
        }
        else
        {
            F32 arch = ARCH(rat_rev);
            F32 dim = LERP(arch, npparm->siz_base[0], npparm->siz_base[1]);

            npdata->xy_size[0] = dim;
            npdata->xy_size[1] = dim;
        }

        npdata->color.alpha = rat * npparm->colr_base.alpha;

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_Upd_VisSplash(NPARMgmt* mgmt, F32 dt)
{
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 2;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmVisSplash* npparm = &g_parm_vissplash[npdata->nparmode];

        npdata->tmr_remain -= dt;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;
        F32 rat_rev = 1.0f - rat;
        F32 beans = dt * npdata->fac_abuse;
        F32 fac_keep = MAX(0.0f, MIN(0.01f * npparm->pct_keep, 100.0f));

        npdata->pos += npdata->vel * dt;
        npdata->vel *= fac_keep;
        npdata->vel += npparm->acc_base * beans;

        if (npdata->nparmode == 0)
        {
            npdata->vel.x += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
            npdata->vel.z += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
        }

        if (npdata->nparmode == 0)
        {
            F32 dim = LERP(xurand(), npparm->siz_base[0], npparm->siz_base[1]);

            npdata->xy_size[0] = dim;
            npdata->xy_size[1] = dim;
        }
        else
        {
            F32 arch = ARCH(rat_rev);
            F32 dim = LERP(arch, npparm->siz_base[0], npparm->siz_base[1]);

            npdata->xy_size[0] = dim;
            npdata->xy_size[1] = dim;
        }

        npdata->color.alpha = rat * npparm->colr_base.alpha;

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_Upd_TarTarGunk(NPARMgmt* mgmt, F32 dt)
{
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 6;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmTarTarGunk* npparm = &g_parm_tartargunk[npdata->nparmode];

        npdata->tmr_remain -= dt;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;
        F32 rat_rev = 1.0f - rat;
        F32 beans = dt * npdata->fac_abuse;
        F32 fac_keep = MAX(0.0f, MIN(0.01f * npparm->pct_keep, 100.0f));

        npdata->pos += npdata->vel * dt;
        npdata->vel *= fac_keep;
        npdata->vel += npparm->acc_base * beans;

        if (npdata->nparmode == 0)
        {
            npdata->vel.x += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
            npdata->vel.z += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
        }

        F32 arch = ARCH(rat_rev);
        F32 dim = LERP(arch, npparm->siz_base[0], npparm->siz_base[1]);

        npdata->xy_size[0] = dim;
        npdata->xy_size[1] = dim;
        npdata->color.alpha = rat * npparm->colr_base.alpha;

        F32 uv_du = npparm->uv_scroll[0];
        F32 uv_dv = npparm->uv_scroll[1];

        S32 popts = npdata->flg_popts;

        if (npdata->nparmode && !(popts & 2))
        {
            if (popts & 1)
            {
                S32 r = npparm->num_uvcell[0] * xurand();
                S32 c = npparm->num_uvcell[1] * xurand();

                npdata->uv_tl[0] = c * uv_du;
                npdata->uv_tl[1] = (npparm->row_uvstart + r) * uv_dv;
                npdata->uv_br[0] = npdata->uv_tl[0] + uv_du;
                npdata->uv_br[1] = npdata->uv_tl[1] + uv_dv;
            }
            else
            {
                S32 num_cells = npparm->num_uvcell[0] * npparm->num_uvcell[1];
                S32 cell = std::floorf(rat_rev * num_cells);
                S32 col = cell % npparm->num_uvcell[0];
                S32 row = (cell - col) / npparm->num_uvcell[0];

                npdata->uv_tl[0] = col * uv_du;
                npdata->uv_tl[1] = (npparm->row_uvstart + row) * uv_dv;
                npdata->uv_br[0] = npdata->uv_tl[0] + uv_du;
                npdata->uv_br[1] = npdata->uv_tl[1] + uv_dv;
            }
        }

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_Upd_DogBreath(NPARMgmt* mgmt, F32 dt)
{
    static const F32 seg_allowCollide[2] = { 0.0f, 1.0f };

    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 6;
    pool.rs.flags = 0;
    pool.reset();

    xVec3 pos_plyr = *xEntGetCenter(&globals.player.ent);

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmDogBreath* npparm = &g_parm_dogbreath[npdata->nparmode];

        npdata->tmr_remain -= dt;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;
        F32 rat_rev = 1.0f - rat;
        F32 beans = dt * npdata->fac_abuse;
        F32 fac_keep = MAX(0.0f, MIN(0.01f * npparm->pct_keep, 100.0f));

        npdata->pos += npdata->vel * dt;
        npdata->vel *= fac_keep;
        npdata->vel += npparm->acc_base * beans;

        F32 arch = ARCH(rat_rev);
        F32 dim = LERP(arch, npparm->siz_base[0], npparm->siz_base[1]);

        npdata->xy_size[0] = dim;
        npdata->xy_size[1] = dim;
        npdata->color.alpha = arch * npparm->colr_base.alpha;

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            if (npdata->nparmode != 1 && !(globals.player.DamageTimer > 0.0f) &&
                globals.player.Health && rat_rev > seg_allowCollide[0] &&
                rat_rev < seg_allowCollide[1] && npdata->color.alpha > 0x20)
            {
                F32 ds2_plyr = NPCC_DstSq(&npdata->pos, &pos_plyr, NULL);
                if (ds2_plyr < SQ(0.5f))
                {
                    zEntPlayer_DamageNPCKnockBack(NULL, 1, &npdata->pos);
                }

                static S32 howfreq = 0;
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

void NPAR_Upd_Fireworks(NPARMgmt* mgmt, F32 dt)
{
    ptank_pool__pos_color_size_uv2 pool;

    pool.rs.texture = mgmt->txtr;
    pool.rs.src_blend = 5;
    pool.rs.dst_blend = 2;
    pool.rs.flags = 0;
    pool.reset();

    for (S32 i = 0; i < mgmt->cnt_active; i++)
    {
        NPARData* npdata = &mgmt->par_buf[i];

        const NPARParmFahrwerkz* npparm = &g_parm_fahrwerkz[npdata->nparmode];

        npdata->tmr_remain -= dt;

        F32 rat = MAX(0.0f, npdata->tmr_remain) / npdata->tym_exist;
        F32 rat_rev = 1.0f - rat;
        F32 beans = dt * npdata->fac_abuse;
        F32 fac_keep = MAX(0.0f, MIN(0.01f * npparm->pct_keep, 100.0f));

        npdata->pos += npdata->vel * dt;
        npdata->vel *= fac_keep;
        npdata->vel += npparm->acc_base * beans;

        if (npdata->nparmode == 0)
        {
            npdata->vel.x += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
            npdata->vel.z += beans * (6.0f * ((xrand() & 0x800000) ? 1.0f : -1.0f));
        }

        F32 arch = ARCH(rat_rev);
        F32 dim = LERP(arch, npparm->siz_base[0], npparm->siz_base[1]);

        npdata->xy_size[0] = dim;
        npdata->xy_size[1] = dim;
        npdata->color.alpha = rat * npparm->colr_base.alpha;

        F32 uv_du = npparm->uv_scroll[0];
        F32 uv_dv = npparm->uv_scroll[1];

        S32 popts = npdata->flg_popts;

        if (npdata->nparmode && !(popts & 2))
        {
            if (popts & 1)
            {
                S32 r = npparm->num_uvcell[0] * xurand();
                S32 c = npparm->num_uvcell[1] * xurand();

                npdata->uv_tl[0] = c * uv_du;
                npdata->uv_tl[1] = (npparm->row_uvstart + r) * uv_dv;
                npdata->uv_br[0] = npdata->uv_tl[0] + uv_du;
                npdata->uv_br[1] = npdata->uv_tl[1] + uv_dv;
            }
            else
            {
                S32 num_cells = npparm->num_uvcell[0] * npparm->num_uvcell[1];
                S32 cell = std::floorf(rat_rev * num_cells);
                S32 col = cell % npparm->num_uvcell[0];
                S32 row = (cell - col) / npparm->num_uvcell[0];

                npdata->uv_tl[0] = col * uv_du;
                npdata->uv_tl[1] = (npparm->row_uvstart + row) * uv_dv;
                npdata->uv_br[0] = npdata->uv_tl[0] + uv_du;
                npdata->uv_br[1] = npdata->uv_tl[1] + uv_dv;
            }
        }

        if (npdata->tmr_remain < 0.0f)
        {
            mgmt->PromoteTail(i);
            i--;
        }
        else
        {
            if (g_doNPARCull)
            {
                RwSphere testSphere;
                testSphere.center = *(RwV3d*)&npdata->pos;
                testSphere.radius = npdata->xy_size[0];

                if (!RwCameraFrustumTestSphere(globals.camera.lo_cam, &testSphere))
                {
                    continue;
                }
            }

            pool.next();

            if (pool.valid())
            {
                NPAR_CopyNPARToPTPool(npdata, &pool);
            }
        }
    }

    pool.flush();
}

// Equivalent: operands of single fmuls instruction swapped.
void NPARParmFahrwerkz::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = 0.5f * xurand() + 0.5f;
    
    par->fac_abuse = fac_rand;
    par->tmr_remain = tym_lifespan * fac_rand;
    par->tym_exist  = tym_lifespan * fac_rand;
    par->pos = *pos;
    par->vel = *vel;
    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];
    par->color = colr_base;
    
    F32 justTheRand = fac_rand;

    F32 du = 1.0f / num_uvcell[0];
    F32 dv = 1.0f / num_uvcell[1];

    if (pmod == 0)
    {
        F32 samecalc = 2.0f * (justTheRand - 0.5f );
        S32 idx_cell = samecalc * num_uvcell[1];
        par->uv_tl[0] = idx_cell * du;
        par->uv_tl[1] = (row_uvstart + (int)(samecalc * num_uvcell[0])) * dv;
        par->uv_br[0] = par->uv_tl[0] + du;
        par->uv_br[1] = par->uv_tl[1] + dv;

        par->flg_popts |= 2;
    }
    else
    {
        par->uv_tl[0] = 0.0f;
        par->uv_tl[1] = row_uvstart * dv;
        par->uv_br[0] = du;
        par->uv_br[1] = dv;

        par->flg_popts &= ~2;
    }

    par->nparmode = pmod;
}

// Equivalent: operands of single fmuls instruction swapped.
void NPARParmTarTarGunk::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = 0.5f * xurand() + 0.5f;

    par->fac_abuse = fac_rand;
    par->tmr_remain = tym_lifespan * fac_rand;
    par->tym_exist  = tym_lifespan * fac_rand;
    par->pos = *pos;
    par->vel = *vel;
    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];
    par->color = colr_base;

    F32 justTheRand = fac_rand;

    F32 du = 1.0f / num_uvcell[0];
    F32 dv = 1.0f / num_uvcell[1];

    if (pmod == 0)
    {
        F32 samecalc = 2.0f * (justTheRand - 0.5f );
        S32 idx_cell = samecalc * num_uvcell[1];
        par->uv_tl[0] = idx_cell * du;
        par->uv_tl[1] = (row_uvstart + (int)(samecalc * num_uvcell[0])) * dv;
        par->uv_br[0] = par->uv_tl[0] + du;
        par->uv_br[1] = par->uv_tl[1] + dv;

        par->flg_popts |= 2;
    }
    else
    {
        par->uv_tl[0] = 0.0f;
        par->uv_tl[1] = row_uvstart * dv;
        par->uv_br[0] = du;
        par->uv_br[1] = dv;

        par->flg_popts &= ~2;
    }

    par->nparmode = pmod;
}

// Equivalent: operands of single fmuls instruction swapped.
void NPARParmSleepyZeez::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = 0.5f * xurand() + 0.5f;

    par->fac_abuse = fac_rand;
    par->tmr_remain = tym_lifespan * fac_rand;
    par->tym_exist  = tym_lifespan * fac_rand;
    par->pos = *pos;
    par->vel = *vel;
    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];
    par->color = colr_base;

    F32 justTheRand = fac_rand;

    F32 du = 1.0f / num_uvcell[0];
    F32 dv = 1.0f / num_uvcell[1];

    if (pmod == 0)
    {
        F32 samecalc = 2.0f * (justTheRand - 0.5f );
        S32 idx_cell = samecalc * num_uvcell[1];
        par->uv_tl[0] = idx_cell * du;
        par->uv_tl[1] = (row_uvstart + (int)(samecalc * num_uvcell[0])) * dv;
        par->uv_br[0] = par->uv_tl[0] + du;
        par->uv_br[1] = par->uv_tl[1] + dv;

        par->flg_popts |= 2;
    }
    else
    {
        par->uv_tl[0] = 0.0f;
        par->uv_tl[1] = row_uvstart * dv;
        par->uv_br[0] = du;
        par->uv_br[1] = dv;

        par->flg_popts &= ~2;
    }

    par->nparmode = pmod;
}

void NPARParmChuckSplash::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = xurand() * 0.5f + 0.5f;
    F32 samecalc = tym_lifespan * fac_rand;

    par->fac_abuse = fac_rand;
    par->tmr_remain = samecalc;
    par->tym_exist = samecalc;

    par->pos = *pos;
    par->vel = *vel;

    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];

    par->color = colr_base;

    if (pmod == NPAR_MODE_STD)
    {
        par->uv_tl[0] = 0.0f;
        par->uv_tl[1] = 0.5f;
        par->uv_br[0] = 0.5f;
        par->uv_br[1] = 1.0f;
        par->flg_popts |= 2;
    }
    else if ((pmod == NPAR_MODE_ALT_C) || (pmod == NPAR_MODE_ALT_A) || (pmod == NPAR_MODE_ALT_B))
    {
        par->uv_tl[0] = 0.5f;
        par->uv_tl[1] = 0.5f;
        par->uv_br[0] = 1.0f;
        par->uv_br[1] = 1.0f;
        par->flg_popts |= 2;
    }
    else if (pmod == NPAR_MODE_ALT_D)
    {
        par->uv_tl[0] = 0.5f;
        par->uv_tl[1] = 0.0f;
        par->uv_br[0] = 1.0f;
        par->uv_br[1] = 0.5f;
        par->flg_popts |= 2;
    }
    else
    {
        par->uv_tl[0] = 0.5f;
        par->uv_tl[1] = 0.5f;
        par->uv_br[0] = 1.0f;
        par->uv_br[1] = 1.0f;
        par->flg_popts |= 2;
    }

    par->nparmode = pmod;
}

//todo
void NPAR_EmitDroplets(en_nparmode pmod, const xVec3* pos, const xVec3* vel)
{
    NPARData *pNVar1;
    NPARMgmt *mgmt = (NPARMgmt *)NPAR_FindParty(NPAR_TYP_CHUCKSPLASH);
    if ((mgmt != NULL) && (pNVar1 = mgmt->NextAvail(), pNVar1 != NULL))
    {
        g_parm_chucksplash[pmod].ConfigPar(pNVar1, pmod, pos, vel);
    }
}

void NPAR_EmitOilShieldPop(const xVec3* pos)
{
    NPAR_EmitOilBubble(NPAR_MODE_STD, pos, NULL);
}

void NPAR_EmitOilTrailz(const xVec3* pos)
{
    NPAR_EmitOilBubble(NPAR_MODE_ALT_A, pos, NULL);
}

void NPAR_EmitOilVapors(const xVec3* pos)
{
    NPAR_EmitOilBubble(NPAR_MODE_ALT_B, pos, NULL);
}

void NPAR_EmitOilSplash(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitOilBubble(NPAR_MODE_ALT_C, pos, vel);
}

void NPAR_EmitTarTarGunk(en_nparmode pmod, const xVec3* pos, const xVec3* vel)
{
    NPARData *pNVar1;
    NPARMgmt *mgmt = (NPARMgmt *)NPAR_FindParty(NPAR_TYP_TARTARGUNK);
    if ((mgmt != NULL) && (pNVar1 = mgmt->NextAvail(), pNVar1 != NULL))
    {
        g_parm_tartargunk[pmod].ConfigPar(pNVar1, pmod, pos, vel);
    }
}

void NPAR_EmitGloveDust(const xVec3* pos, const xVec3* vel)
{
    NPARData *pNVar1;
    NPARMgmt *mgmt = (NPARMgmt *)NPAR_FindParty(NPAR_TYP_GLOVEDUST);
    if ((mgmt != NULL) && (pNVar1 = mgmt->NextAvail(), pNVar1 != NULL))
    {
        g_parm_glovedust[0].ConfigPar(pNVar1, NPAR_MODE_STD, pos, vel);
    }
}

void NPAR_EmitSleepyZeez(const xVec3* pos, const xVec3* vel)
{
    NPARData *pNVar1;
    NPARMgmt *mgmt = (NPARMgmt *)NPAR_FindParty(NPAR_TYP_SLEEPYZEEZ);
    if ((mgmt != NULL) && (pNVar1 = mgmt->NextAvail(), pNVar1 != NULL))
    {
        g_parm_sleepyzeez[0].ConfigPar(pNVar1, NPAR_MODE_STD, pos, vel);
    }
}

void NPAR_EmitTarTarNozzle(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitTarTarGunk(NPAR_MODE_ALT_A, pos, vel);
}

void NPAR_EmitTarTarTrail(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitTarTarGunk(NPAR_MODE_ALT_B, pos, vel);
}

void NPAR_EmitTarTarSplash(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitTarTarGunk(NPAR_MODE_ALT_C, pos, vel);
}

void NPAR_EmitTarTarSpoil(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitTarTarGunk(NPAR_MODE_ALT_D, pos, vel);
}

void NPAR_EmitTarTarSmoke(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitTarTarGunk(NPAR_MODE_ALT_E, pos, vel);
}

void NPAR_EmitVSSpray(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitVisSplash(NPAR_MODE_STD, pos, vel);
}

void NPAR_EmitDoggyBreath(en_nparmode pmod, const xVec3* pos, const xVec3* vel)
{
    NPARData *pNVar1;
    NPARMgmt *mgmt = (NPARMgmt *)NPAR_FindParty(NPAR_TYP_DOGBREATH);
    if ((mgmt != NULL) && (pNVar1 = mgmt->NextAvail(), pNVar1 != NULL))
    {
        g_parm_dogbreath[pmod].ConfigPar(pNVar1,pmod,pos,vel);
    }
}

void NPAR_EmitDoggyWisps(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitDoggyBreath(NPAR_MODE_ALT_A, pos, vel);
}

void NPAR_EmitDoggyAttack(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitDoggyBreath(NPAR_MODE_ALT_B, pos, vel);
}

void NPARParmGloveDust::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = xurand() * 0.5f + 0.5f;
    F32 samecalc = tym_lifespan * fac_rand;

    par->fac_abuse = fac_rand;
    par->tmr_remain = samecalc;
    par->tym_exist = samecalc;

    par->pos = *pos;
    par->vel = *vel;

    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];

    par->color = colr_base;

    par->uv_tl[0] = 0.0f;
    par->uv_tl[1] = 0.0f;
    par->uv_br[0] = 1.0f;
    par->uv_br[1] = 1.0f;

    par->nparmode = pmod;
}

void NPARParmTubeSpiral::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel, F32 dt) const
{
    par->fac_abuse = (xurand() * 0.5f + 0.5f);
    par->tmr_remain = dt;
    par->tym_exist = dt;
    par->pos = *pos;
    par->vel = *vel;
    par->xy_size[0] = siz_base[0];
    par->xy_size[1] = siz_base[0];
    par->color = colr_base;
    par->uv_tl[0] = 0.0;
    par->uv_tl[1] = 0.0;
    par->uv_br[0] = 1.0;
    par->uv_br[1] = 1.0;
    par->nparmode = pmod;
}

// Equivalent: weird unnecessary use of mulli to index into g_parm_tubespiral.
void NPAR_EmitTubeSpiralCin(const xVec3* pos, const xVec3* vel, float lifespan)
{
    NPARData* par;
    NPARMgmt* mgmt = NPAR_FindParty(NPAR_TYP_TUBESPIRAL);
    if ((mgmt != NULL) && (par = mgmt->NextAvail(), par != NULL))
    {
        en_nparmode pmod = NPAR_MODE_SPIRALCINE;
        g_parm_tubespiral[pmod].ConfigPar(par, pmod, pos, vel, lifespan);
    }
}

void NPARParmOilBub::ConfigPar(NPARData* par, en_nparmode pmod, const xVec3* pos, const xVec3* vel) const
{
    F32 fac_rand = (xurand() * 0.5f + 0.5f);
    F32 samecalc = tym_lifespan * fac_rand;
    par->fac_abuse  = fac_rand;
    par->tmr_remain = samecalc;
    par->tym_exist  = samecalc;
    par->pos = *pos;
    par->vel = (vel != NULL) ? *vel : g_O3;
    F32 uVar2 = siz_base[0];
    par->xy_size[0] = uVar2;
    par->xy_size[1] = uVar2;
    par->color = colr_base;
    par->uv_tl[0] = 0.0;
    par->uv_tl[1] = 0.0;
    par->uv_br[0] = 1.0;
    par->uv_br[1] = 1.0;
    par->nparmode = pmod;
}

void NPAR_EmitH2ODrips(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitDroplets(NPAR_MODE_DRIP, pos, vel);
}

void NPAR_EmitH2ODrops(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitDroplets(NPAR_MODE_DROP, pos, vel);
}

void NPAR_EmitH2OSpray(const xVec3* pos, const xVec3* vel)
{
    NPAR_EmitDroplets(NPAR_MODE_SPLASH, pos, vel);
}

void NPAR_EmitH2OTrail(const xVec3* pos)
{
    NPAR_EmitDroplets(NPAR_MODE_TRAIL, pos, (xVec3 *)&g_O3);
}

static void NPCC_ShadowCacheReset()
{
    for (int i = 0; i < sizeof(g_shadCachesInUseFlags) / sizeof(S8); i++)
    {
        g_shadCachesInUseFlags[i] = 0;
    }
}

xShadowCache* NPCC_ShadowCacheReserve()
{
    xShadowCache* da_cache = NULL;

    for (S32 i = 0; i < 16; i++)
    {
        if (g_shadCachesInUseFlags[i] == 0)
        {
            g_shadCachesInUseFlags[i] = 1;
            da_cache = &g_shadCaches[i];
            break;
        }
    }

    if (da_cache != NULL)
    {
        da_cache->entCount = 0;
        da_cache->polyCount = 0;
        da_cache->castOnEnt = 0;
        da_cache->castOnPoly = 0;
    }

    return da_cache;
}

void NPCC_ShadowCacheRelease(xShadowCache* shadcache)
{
    for (S32 i = 0; i < 16; i++)
    {
        if (shadcache == &g_shadCaches[i])
        {
            g_shadCachesInUseFlags[i] = 0;
        }
    }
}
