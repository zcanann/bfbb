#include "zEntPlayer.h"
#include "zGlobals.h"
#include "zMain.h"

// NOTE: MSL_Common/printf.h declares sprintf() without an `extern "C"` guard, so
// including it here would emit a call to the C++-mangled sprintf__FPcPCce. Retail
// calls the C-linkage symbol.
extern "C" int sprintf(char*, const char*, ...);

#include <types.h>

#include "iSystem.h"
#include "xMemMgr.h"
#include "zVar.h"
// NOTE: zAssetTypes.h is deliberately NOT included, and its two declarations are
// repeated below instead. It drags in zNPCTypeBossPlankton.h and zNPCTypeDutchman.h,
// whose inline bodies contain brace-initialised aggregates; CodeWarrior materialises
// those templates at *parse* time, so merely including them emits a 12-byte .rodata
// and an 8-byte .sbss2 anonymous object into this TU. Retail's zMain.o has neither,
// and their presence displaces screen_bounds/@1170 and the three RwRGBA .sbss2 zeros.
#include "xUtil.h"
#include "zEntPickup.h"
#include "xDebug.h"
#include "xsavegame.h"
#include "xBehaveMgr.h"
#include "xModel.h"
#include "xShadowSimple.h"
#include "iMath.h"
#include "xString.h"
#include "xFont.h"
#include "iCamera.h"
#include "zUI.h"
#include "iFile.h"
#include "zScene.h"
#include "zGameState.h"
#include <dolphin/card.h>
#include <dolphin/os.h>
// from zAssetTypes.h - see the note above
void zAssetStartup();
void zAssetShutdown();

#include "xTRC.h"
#include "iTime.h"
#include "zDispatcher.h"
#include "xserializer.h"
#include "xScrFX.h"
#include "xFX.h"
#include "xParMgr.h"
#include "zParCmd.h"
#include "zCameraTweak.h"
#include "zShrapnel.h"
#include "zNPCMgr.h"
#include "xCamera.h"
#include "zCamera.h"
#include "zCutsceneMgr.h"
#include "zEntPlayerBungeeState.h"
#include "zEntPlayerOOBState.h"
#include "iTime.h"
#include "xstransvc.h"

static const basic_rect<F32> screen_bounds = { 0.0f, 0.0f, 1.0f, 1.0f };

zGlobals globals;
xGlobals* xglobals = &globals;

S32 percentageDone;
extern _tagxPad* gDebugPad;
static S32 sShowMenuOnBoot = 1;
F32 gSkipTimeCutscene = 1.0f;
F32 gSkipTimeFlythrough = 1.0f;
extern S32 gGameSfxReport;
static st_SERIAL_PERCID_SIZE g_xser_sizeinfo[] = { { 'PLYR', 0x148 }, { 'CNTR', 0x1e0 }, { 0, 0 } };

static void zLedgeAdjust(zLedgeGrabParams* params);
static void zMainMemCardQueryPost(S32 needed, S32 available, S32 neededFiles, S32 unk0);
void zMainMemCardRenderText(const char*, bool);
void RenderText(const char*, bool);
void zMainMemCardSpaceQuery();
extern U32 gSoak;

// These are all defined in other translation units but declared in no header.
void zPickupTableInit();
void zMenuInit(U32 theSceneID);
void zMenuSetup();
U32 zMenuLoop();
void zMenuExit();
// zMenu.cpp defines this returning bool; zMain sees it as S32. The mangled name
// carries no return type, so the two declarations name the same symbol - and the
// retail zMain.o assigns the result with a plain `mr`, which only an int-typed
// declaration produces.
S32 zMenuCardCheckStartup(S32* bytesNeeded, S32* availOnDisk, S32* neededFiles);
S32 zMenuGetBadCard();
U32 zMenuGetCorruptFiles(char name[][64]);
S32 zMenuIsFirstBoot();
void zGameInit(U32 theSceneID);
void zGameSetup();
void zGameLoop();
void zGameExit();

void main(S32 argc, char** argv)
{
    U32 options;
    S32 i;
    char* tmpStr;

    memset(&globals, 0, sizeof(globals));
    globals.firstStartPressed = TRUE;
    iSystemInit(FALSE); // 0x6d2
    zMainOutputMgrSetup();
    xMemRegisterBaseNotifyFunc(zMainMemLvlChkCB);
    zMainInitGlobals();
    var_init();
    zAssetStartup();
    zMainLoadFontHIP();
    xfont::init();
    zMainFirstScreen(1);
    zMainShowProgressBar();
    xTRCInit();
    zMainReadINI();
    iFuncProfileParse(tmpStr = "scooby.elf", globals.profile);
    xUtilStartup();
    xSerialStartup(0x80, g_xser_sizeinfo);
    zDispatcher_Startup();
    xScrFxInit();
    xFXStartup();
    zMainShowProgressBar();
    xParMgrInit();
    zParCmdInit();
    iEnvStartup();
    zEntPickup_Startup();
    zCameraTweakGlobal_Init();
    globals.option_vibration = 1; // 0x6c0
    globals.pad0 = xPadEnable(globals.currentActivePad); // 0x6d1
    globals.pad1 = 0;
    gDebugPad = 0;
    xPadRumbleEnable(globals.currentActivePad, globals.option_vibration); //  0x6d1, 0x6c0
    xSGStartup();
    xDebugTimestampScreen();
    zShrapnel_GameInit();
    zMainShowProgressBar();
    xBehaveMgr_Startup();
    zNPCMgr_Startup();
    zMainLoop();
    zNPCMgr_Shutdown();
    xBehaveMgr_Shutdown();
    zAssetShutdown();
    xFXShutdown();
    zDispatcher_Shutdown();
    xSGShutdown();
    xSerialShutdown();
    xUtilShutdown();
    iSystemExit();
}

void zMainOutputMgrSetup()
{
    iTime tim = iTimeGet();
    __FUNCTION__; // stands in for the (NDEBUG'd) load-timing report; retail's object
                  // carries the __FUNCTION__ string object this evaluation emits
    iTimeDiffSec(tim);
    iTimeGet();
}

void zMainInitGlobals()
{
    memset(&globals, 0, sizeof(zGlobals));
    globals.sceneFirst = 1;
    iTime tim = iTimeGet();
    __FUNCTION__; // stands in for the (NDEBUG'd) load-timing report; retail's object
                  // carries the __FUNCTION__ string object this evaluation emits
    iTimeDiffSec(tim);
    iTimeGet();
}

static void ParseFloatList(F32* dest, char* strbuf, S32 max)
{
    xStrParseFloatList(dest, strbuf, max);
}

void zLedgeAdjust(zLedgeGrabParams* params)
{
    params->animGrab *= (1.0f / 30);
}

void zMainParseINIGlobals(xIniFile* ini)
{
    U32 use_degrees;

    globals.player.g.AnalogMin = xIniGetInt(ini, "g.AnalogMin", 0x20);
    globals.player.g.AnalogMax = xIniGetInt(ini, "g.AnalogMax", 0x6e);
    xScrFxLetterBoxSetSize(xIniGetFloat(ini, "ScrFxLetterBoxSize", 0.0f));
    xScrFxLetterBoxSetAlpha(xIniGetInt(ini, "ScrFxLetterBoxAlpha", 0xff));
    globals.player.g.InitialShinyCount = xIniGetInt(ini, "g.InitialShinyCount", 0);
    globals.player.g.InitialSpatulaCount = xIniGetInt(ini, "g.InitialSpatulaCount", 0);
    globals.player.g.ShinyValuePurple = xIniGetInt(ini, "g.ShinyValuePurple", 100);
    globals.player.g.ShinyValueBlue = xIniGetInt(ini, "g.ShinyValueBlue", 0x19);
    globals.player.g.ShinyValueGreen = xIniGetInt(ini, "g.ShinyValueGreen", 10);
    globals.player.g.ShinyValueYellow = xIniGetInt(ini, "g.ShinyValueYellow", 5);
    globals.player.g.ShinyValueRed = xIniGetInt(ini, "g.ShinyValueRed", 1);
    globals.player.g.ShinyValueCombo0 = xIniGetInt(ini, "g.ShinyValueCombo0", 0);
    globals.player.g.ShinyValueCombo1 = xIniGetInt(ini, "g.ShinyValueCombo1", 0);
    globals.player.g.ShinyValueCombo2 = xIniGetInt(ini, "g.ShinyValueCombo2", 2);
    globals.player.g.ShinyValueCombo3 = xIniGetInt(ini, "g.ShinyValueCombo3", 3);
    globals.player.g.ShinyValueCombo4 = xIniGetInt(ini, "g.ShinyValueCombo4", 3);
    globals.player.g.ShinyValueCombo5 = xIniGetInt(ini, "g.ShinyValueCombo5", 5);
    globals.player.g.ShinyValueCombo6 = xIniGetInt(ini, "g.ShinyValueCombo6", 10);
    globals.player.g.ShinyValueCombo7 = xIniGetInt(ini, "g.ShinyValueCombo7", 0xf);
    globals.player.g.ShinyValueCombo8 = xIniGetInt(ini, "g.ShinyValueCombo8", 0x14);
    globals.player.g.ShinyValueCombo9 = xIniGetInt(ini, "g.ShinyValueCombo9", 0x19);
    globals.player.g.ShinyValueCombo10 = xIniGetInt(ini, "g.ShinyValueCombo10", 0x1e);
    globals.player.g.ShinyValueCombo11 = xIniGetInt(ini, "g.ShinyValueCombo11", 0x28);
    globals.player.g.ShinyValueCombo12 = xIniGetInt(ini, "g.ShinyValueCombo12", 0x32);
    globals.player.g.ShinyValueCombo13 = xIniGetInt(ini, "g.ShinyValueCombo13", 0x3c);
    globals.player.g.ShinyValueCombo14 = xIniGetInt(ini, "g.ShinyValueCombo14", 0x4b);
    globals.player.g.ShinyValueCombo15 = xIniGetInt(ini, "g.ShinyValueCombo15", 100);

    globals.player.g.ComboTimer = xIniGetFloat(ini, "g.ComboTimer", 1.0f); // @1002;
    globals.player.g.SundaeTime = xIniGetFloat(ini, "g.SundaeTime", 6.0f); // @1003;
    globals.player.g.SundaeMult = xIniGetFloat(ini, "g.SundaeMult", 1.5f); // @1004;
    globals.player.g.BBashTime = xIniGetFloat(ini, "g.BBashTime", 0.3f); // @1005;
    globals.player.g.BBashDelay = xIniGetFloat(ini, "g.BBashDelay", 0.15f); // @1006;
    globals.player.g.BBashCVTime = xIniGetFloat(ini, "g.BBashCVTime", 0.15f); // @1006;
    globals.player.g.BBashHeight = xIniGetFloat(ini, "g.BBashHeight", 3.0f); // @1007;
    globals.player.g.BBounceSpeed = xIniGetFloat(ini, "g.BBounceSpeed", 10.0f); // @1008;
    globals.player.g.BSpinMinFrame = xIniGetFloat(ini, "g.BSpinMinFrame", 2.0f); // @1009;
    globals.player.g.BSpinMaxFrame = xIniGetFloat(ini, "g.BSpinMaxFrame", 20.0f); // @1010;
    globals.player.g.BSpinRadius = xIniGetFloat(ini, "g.BSpinRadius", 0.3f); // @1005;
    globals.player.g.BSpinMinFrame *= 0.033333335f; // @909
    globals.player.g.BSpinMaxFrame *= 0.033333335f; // @909
    globals.player.g.SandyMeleeMinFrame = xIniGetFloat(ini, "g.SandyMeleeMinFrame", 1.0f); // @1002;
    globals.player.g.SandyMeleeMaxFrame = xIniGetFloat(ini, "g.SandyMeleeMaxFrame", 9.0f); // @1011;
    globals.player.g.SandyMeleeRadius = xIniGetFloat(ini, "g.SandyMeleeRadius", 0.3f); // @1005;
    globals.player.g.SandyMeleeMinFrame *= 0.033333335f; // @909
    globals.player.g.SandyMeleeMaxFrame *= 0.033333335f; // @909
    globals.player.g.DamageTimeHit = xIniGetFloat(ini, "g.DamageTimeHit", 0.5f); // @1012;
    globals.player.g.DamageTimeSurface = xIniGetFloat(ini, "g.DamageTimeSurface", 1.0f); // @1002;
    globals.player.g.DamageTimeEGen = xIniGetFloat(ini, "g.DamageTimeEGen", 1.0f); // @1002;
    globals.player.g.DamageSurfKnock = xIniGetFloat(ini, "g.DamageSurfKnock", 1.75f); // @1013;
    globals.player.g.DamageGiveHealthKnock =
        xIniGetFloat(ini, "g.DamageGiveHealthKnock", 1.75f); // @1013;
    globals.player.g.BubbleBowlTimeDelay =
        xIniGetFloat(ini, "g.BubbleBowlTimeDelay", 1.0f); // @1002;
    globals.player.g.BubbleBowlLaunchPosLeft =
        xIniGetFloat(ini, "g.BubbleBowlLaunchPosLeft", 0.0f); // @ 1001;
    globals.player.g.BubbleBowlLaunchPosUp =
        xIniGetFloat(ini, "g.BubbleBowlLaunchPosUp", 1.0f); // @ 1002;
    globals.player.g.BubbleBowlLaunchPosAt =
        xIniGetFloat(ini, "g.BubbleBowlLaunchPosAt", 1.5f); // @ 1004;
    globals.player.g.BubbleBowlLaunchVelLeft =
        xIniGetFloat(ini, "g.BubbleBowlLaunchVelLeft", 0.0f); // @ 1001;
    globals.player.g.BubbleBowlLaunchVelUp =
        xIniGetFloat(ini, "g.BubbleBowlLaunchVelUp", 0.0f); // @ 1001;
    globals.player.g.BubbleBowlLaunchVelAt =
        xIniGetFloat(ini, "g.BubbleBowlLaunchVelAt", 10.0f); // @ 1008;
    globals.player.g.BubbleBowlPercentIncrease =
        xIniGetFloat(ini, "g.BubbleBowlPercentIncrease", 0.85f); // @ 1014;
    globals.player.g.BubbleBowlMinSpeed =
        xIniGetFloat(ini, "g.BubbleBowlMinSpeed", 0.1f); // @ 1015;
    globals.player.g.BubbleBowlMinRecoverTime =
        xIniGetFloat(ini, "g.BubbleBowlMinRecoverTime", 0.15f); // @ 1006;
    globals.player.g.SlideAccelVelMin = xIniGetFloat(ini, "g.SlideAccelVelMin", 6.0f); // @ 1003;
    globals.player.g.SlideAccelVelMax = xIniGetFloat(ini, "g.SlideAccelVelMax", 9.0f); // @ 1011;
    globals.player.g.SlideAccelStart = xIniGetFloat(ini, "g.SlideAccelStart", 9.0f); // @ 1011;
    globals.player.g.SlideAccelEnd = xIniGetFloat(ini, "g.SlideAccelEnd", 4.5f); // @ 1016;
    globals.player.g.SlideAccelPlayerFwd =
        xIniGetFloat(ini, "g.SlideAccelPlayerFwd", 5.0f); // @ 1017;
    globals.player.g.SlideAccelPlayerBack =
        xIniGetFloat(ini, "g.SlideAccelPlayerBack", 5.0f); // @ 1017;
    globals.player.g.SlideAccelPlayerSide =
        xIniGetFloat(ini, "g.SlideAccelPlayerSide", 15.0f); // @ 1018;
    globals.player.g.SlideVelMaxStart = xIniGetFloat(ini, "g.SlideVelMaxStart", 12.0f); // @ 1019;
    globals.player.g.SlideVelMaxEnd = xIniGetFloat(ini, "g.SlideVelMaxEnd", 17.0f); // @ 1020;
    globals.player.g.SlideVelMaxIncTime =
        xIniGetFloat(ini, "g.SlideVelMaxIncTime", 6.0f); // @ 1003;
    globals.player.g.SlideVelMaxIncAccel =
        xIniGetFloat(ini, "g.SlideVelMaxIncAccel", 1.0f); // @ 1002;
    globals.player.g.SlideAirHoldTime = xIniGetFloat(ini, "g.SlideAirHoldTime", 1.0f); // @ 1002;
    globals.player.g.SlideAirSlowTime = xIniGetFloat(ini, "g.SlideAirSlowTime", 1.5f); // @ 1004;
    globals.player.g.SlideAirDblHoldTime =
        xIniGetFloat(ini, "g.SlideAirDblHoldTime", 1.0f); // @ 1002;
    globals.player.g.SlideAirDblSlowTime =
        xIniGetFloat(ini, "g.SlideAirDblSlowTime", 1.5f); // @ 1004;
    globals.player.g.SlideVelDblBoost = xIniGetFloat(ini, "g.SlideVelDblBoost", 6.0f); // @ 1003;

    globals.player.g.TakeDamage = xIniGetInt(ini, "g.TakeDamage", 1);
    globals.player.g.CheatSpongeball = xIniGetInt(ini, "g.CheatSpongeball", 0);
    globals.player.g.CheatPlayerSwitch = xIniGetInt(ini, "g.CheatPlayerSwitch", 0);
    globals.player.g.CheatAlwaysPortal = xIniGetInt(ini, "g.CheatAlwaysPortal", 0);
    globals.player.g.CheatFlyToggle = xIniGetInt(ini, "g.CheatFlyToggle", 0);
    globals.player.g.DisableForceConversation = xIniGetInt(ini, "g.DisableForceConversation", 0);

    globals.player.g.CheatFlyToggle = 0;
    globals.player.g.CheatAlwaysPortal = 0;

    globals.player.g.StartSlideAngle = xIniGetFloat(ini, "g.StartSlideAngle", 30.0f); // @ 1021;
    globals.player.g.StopSlideAngle = xIniGetFloat(ini, "g.StopSlideAngle", 10.0f); // @ 1008;
    globals.player.g.RotMatchMaxAngle = xIniGetFloat(ini, "g.RotMatchMaxAngle", 30.0f); // @ 1021;
    globals.player.g.RotMatchMatchTime = xIniGetFloat(ini, "g.RotMatchMatchTime", 0.1f); // @ 1015;
    globals.player.g.RotMatchRelaxTime = xIniGetFloat(ini, "g.RotMatchRelaxTime", 0.3f); // @ 1005
    globals.player.g.Gravity = xIniGetFloat(ini, "g.Gravity", 30.0f); // @ 1021

    globals.player.g.StartSlideAngle = DEG2RAD(globals.player.g.StartSlideAngle);
    globals.player.g.StopSlideAngle = DEG2RAD(globals.player.g.StopSlideAngle);
    globals.player.g.RotMatchMaxAngle = DEG2RAD(globals.player.g.RotMatchMaxAngle);

    use_degrees = xIniGetFloat(ini, "zcam_highbounce_pitch", 180.0f) != 180.0f;
    xcam_collis_radius = xIniGetFloat(ini, "xcam_collis_radius", xcam_collis_radius);
    xcam_collis_stiffness = xIniGetFloat(ini, "xcam_collis_stiffness", xcam_collis_stiffness);
    zcam_pad_pyaw_scale = xIniGetFloat(ini, "zcam_pad_pyaw_scale", zcam_pad_pyaw_scale);
    zcam_pad_pitch_scale = xIniGetFloat(ini, "zcam_pad_pitch_scale", zcam_pad_pitch_scale);
    zcam_near_d = xIniGetFloat(ini, "zcam_near_d", zcam_near_d);
    zcam_near_h = xIniGetFloat(ini, "zcam_near_h", zcam_near_h);
    zcam_near_pitch = xIniGetFloat(ini, "zcam_near_pitch",
                                   use_degrees ? RAD2DEG(zcam_near_pitch) : zcam_near_pitch);

    zcam_far_d = xIniGetFloat(ini, "zcam_far_d", zcam_far_d);
    zcam_far_h = xIniGetFloat(ini, "zcam_far_h", zcam_far_h);
    zcam_far_pitch =
        xIniGetFloat(ini, "zcam_far_pitch", use_degrees ? RAD2DEG(zcam_far_pitch) : zcam_far_pitch);

    zcam_above_d = xIniGetFloat(ini, "zcam_above_d", zcam_above_d);
    zcam_above_h = xIniGetFloat(ini, "zcam_above_h", zcam_above_h);
    zcam_above_pitch = xIniGetFloat(ini, "zcam_above_pitch",
                                    use_degrees ? RAD2DEG(zcam_above_pitch) : zcam_above_pitch);

    zcam_below_d = xIniGetFloat(ini, "zcam_below_d", zcam_below_d);
    zcam_below_h = xIniGetFloat(ini, "zcam_below_h", zcam_below_h);
    zcam_below_pitch = xIniGetFloat(ini, "zcam_below_pitch",
                                    use_degrees ? RAD2DEG(zcam_below_pitch) : zcam_below_pitch);

    zcam_highbounce_d = xIniGetFloat(ini, "zcam_highbounce_d", zcam_highbounce_d);
    zcam_highbounce_h = xIniGetFloat(ini, "zcam_highbounce_h", zcam_highbounce_h);
    zcam_highbounce_pitch =
        xIniGetFloat(ini, "zcam_highbounce_pitch",
                     use_degrees ? RAD2DEG(zcam_highbounce_pitch) : zcam_highbounce_pitch);

    zcam_wall_d = xIniGetFloat(ini, "zcam_wall_d", zcam_wall_d);
    zcam_wall_h = xIniGetFloat(ini, "zcam_wall_h", zcam_wall_h);
    zcam_wall_pitch = xIniGetFloat(ini, "zcam_wall_pitch",
                                   use_degrees ? RAD2DEG(zcam_wall_pitch) : zcam_wall_pitch);

    zcam_overrot_min = xIniGetFloat(ini, "zcam_overrot_min", RAD2DEG(zcam_overrot_min));
    zcam_overrot_mid = xIniGetFloat(ini, "zcam_overrot_mid", RAD2DEG(zcam_overrot_mid));
    zcam_overrot_max = xIniGetFloat(ini, "zcam_overrot_max", RAD2DEG(zcam_overrot_max));

    zcam_overrot_rate = xIniGetFloat(ini, "zcam_overrot_rate", zcam_overrot_rate);
    zcam_overrot_tstart = xIniGetFloat(ini, "zcam_overrot_tstart", zcam_overrot_tstart);
    zcam_overrot_tend = xIniGetFloat(ini, "zcam_overrot_tend", zcam_overrot_tend);
    zcam_overrot_velmin = xIniGetFloat(ini, "zcam_overrot_velmin", zcam_overrot_velmin);
    zcam_overrot_velmax = xIniGetFloat(ini, "zcam_overrot_velmax", zcam_overrot_velmax);
    zcam_overrot_tmanual = xIniGetFloat(ini, "zcam_overrot_tmanual", zcam_overrot_tmanual);

    // non-matching, and the only residue in this function (41 rows of 2245; our
    // object is exactly 14 instructions short).  Retail re-loads BOTH DEG2RAD
    // literals (@1022 = PI, @1023 = 180.0f) for every one of these nine
    // statements; our compiler value-numbers them once per basic block and keeps
    // them live.  That is the known "reload after an aliasing store" defect: in
    // retail a `stfs` to a small-data global kills a cached `.sdata2` literal
    // load, and this branch's mwcc only has that rule in the instruction
    // scheduler (patch clause C), not in the redundancy pass.  Reduced repro:
    //     extern F32 a1, a2, a3;
    //     void f() { a1 = DEG2RAD(a1); a2 = DEG2RAD(a2); a3 = DEG2RAD(a3); }
    // The same three statements against members of a *large* global (see the
    // DEG2RAD block above, on globals.player.g.*) are CSE'd by retail too, which
    // is what pins the rule to small-data objects.  Measured and rejected as
    // source shapes, all bit-identical to the plain form: `*(volatile F32*)&x =`,
    // `extern volatile F32`, a volatile read, a `F32*` local, `*(F32*)&x`,
    // `*&x`, a char* cast, a union member, file-scope statics, and a struct or
    // array of the three.  Only a store through an address the compiler cannot
    // fold defeats it, which this source does not have.
    if (use_degrees)
    {
        zcam_near_pitch = DEG2RAD(zcam_near_pitch);
        zcam_far_pitch = DEG2RAD(zcam_far_pitch);
        zcam_above_pitch = DEG2RAD(zcam_above_pitch);
        zcam_below_pitch = DEG2RAD(zcam_below_pitch);
        zcam_highbounce_pitch = DEG2RAD(zcam_highbounce_pitch);
        zcam_wall_pitch = DEG2RAD(zcam_wall_pitch);
    }

    zcam_overrot_min = DEG2RAD(zcam_overrot_min);
    zcam_overrot_mid = DEG2RAD(zcam_overrot_mid);
    zcam_overrot_max = DEG2RAD(zcam_overrot_max);

    gSkipTimeCutscene = xIniGetFloat(ini, "gSkipTimeCutscene", gSkipTimeCutscene);
    gSkipTimeFlythrough = xIniGetFloat(ini, "gSkipTimeFlythrough", gSkipTimeFlythrough);
    gSkipTimeCutscene = (gSkipTimeCutscene > 1.0f) ? gSkipTimeCutscene : 1.0f; // @ 1002
    gSkipTimeFlythrough = (gSkipTimeFlythrough > 0.0f) ? gSkipTimeFlythrough : 0.0f; // @ 1001

    globals.player.carry.minDist = xIniGetFloat(ini, "carry.minDist", 0.675f); // @1024;
    globals.player.carry.maxDist = xIniGetFloat(ini, "carry.maxDist", 1.9f); // @1025;
    globals.player.carry.minHeight = xIniGetFloat(ini, "carry.minHeight", -0.2f); // @1026;
    globals.player.carry.maxHeight = xIniGetFloat(ini, "carry.maxHeight", 0.4f); // @1027;
    globals.player.carry.maxCosAngle = xIniGetFloat(ini, "carry.maxCosAngle", 45.0f); // @1028;
    globals.player.carry.throwMinDist = xIniGetFloat(ini, "carry.throwMinDist", 1.5f); // @1004;
    globals.player.carry.throwMaxDist = xIniGetFloat(ini, "carry.throwMaxDist", 12.0f); // @1019;
    globals.player.carry.throwMinHeight =
        xIniGetFloat(ini, "carry.throwMinHeight", -3.0f); // @1029;
    globals.player.carry.throwMaxHeight = xIniGetFloat(ini, "carry.throwMaxHeight", 5.0f); // @1017;
    globals.player.carry.throwMaxStack = xIniGetFloat(ini, "carry.throwMaxStack", 2.75f); // @1030;
    globals.player.carry.throwMaxCosAngle =
        xIniGetFloat(ini, "carry.throwMaxCosAngle", 25.0f); // @1031;
    globals.player.carry.grabLerpMin = xIniGetFloat(ini, "carry.grabLerpMin", 0.0f); // @1001;
    globals.player.carry.grabLerpMax = xIniGetFloat(ini, "carry.grabLerpMax", 0.2f); // @1032;
    globals.player.carry.throwGravity = xIniGetFloat(ini, "carry.throwGravity", 30.0f); // @1021;
    globals.player.carry.throwHeight = xIniGetFloat(ini, "carry.throwHeight", 3.75f); // @1033;
    globals.player.carry.throwDistance = xIniGetFloat(ini, "carry.throwDistance", 10.0f); // @1008;
    globals.player.carry.fruitFloorDecayMin =
        xIniGetFloat(ini, "carry.fruitFloorDecayMin", 0.3f); // @1005;
    globals.player.carry.fruitFloorDecayMax =
        xIniGetFloat(ini, "carry.fruitFloorDecayMax", 6.0f); // @1003;
    globals.player.carry.fruitFloorBounce =
        xIniGetFloat(ini, "carry.fruitFloorBounce", 0.15f); // @1006;
    globals.player.carry.fruitFloorFriction =
        xIniGetFloat(ini, "carry.fruitFloorFriction", 0.4f); // @1027;
    globals.player.carry.fruitCeilingBounce =
        xIniGetFloat(ini, "carry.fruitCeilingBounce", 0.1f); // @1015;
    globals.player.carry.fruitWallBounce =
        xIniGetFloat(ini, "carry.fruitWallBounce", 0.5f); // @1012;
    globals.player.carry.fruitLifetime = xIniGetFloat(ini, "carry.fruitLifetime", 15.0f); // @1018;

    globals.player.carry.maxCosAngle = icos(DEG2RAD(globals.player.carry.maxCosAngle));
    globals.player.carry.throwMaxCosAngle = icos(DEG2RAD(globals.player.carry.throwMaxCosAngle));

    globals.player.g.PowerUp[0] = xIniGetInt(ini, "g.BubbleBowl", 0); // Not sure about this
    globals.player.g.PowerUp[1] = xIniGetInt(ini, "g.CruiseBubble", 0); // Not sure about this

    memcpy(globals.player.g.InitialPowerUp, globals.player.g.PowerUp, 2);

    {
        static F32 fbuf[] = { 0.6f, 3.0f, 7.0f, 0.1f, 0.5f, 1.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.MoveSpeed, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.MoveSpeed", 0), 6);
    }
    {
        static F32 fbuf[] = { 1.5f, 0.5f, 1.5f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.AnimSneak, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.AnimSneak", 0), 3);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.5f, 2.5f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.AnimWalk, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.AnimWalk", 0), 3);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.5f, 2.5f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.AnimRun, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.AnimRun", 0), 3);
    }

    globals.player.sb.JumpGravity = xIniGetFloat(ini, "sb.JumpGravity", 7.0f); // @1034;
    globals.player.sb.GravSmooth = xIniGetFloat(ini, "sb.GravSmooth", 0.25f); // @1035;
    globals.player.sb.FloatSpeed = xIniGetFloat(ini, "sb.FloatSpeed", 1.0f); // @1002;
    globals.player.sb.ButtsmashSpeed = xIniGetFloat(ini, "sb.ButtsmashSpeed", 5.0f); // @1017;
    globals.player.sb.ledge.animGrab = xIniGetFloat(ini, "sb.ledge.animGrab", 3.0f); // @1007;

    zLedgeAdjust(&globals.player.sb.ledge);
    bungee_state::load_settings(*ini);
    oob_state::load_settings(*ini);
    ztalkbox::load_settings(*ini);

    {
        static F32 fbuf[] = { 2.0f, 0.7f, 0.35f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.Jump, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.Jump", 0), 4);
    }
    {
        static F32 fbuf[] = { 1.0f, 0.7f, 0.35f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.Double, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.Double", 0), 4);
    }
    {
        static F32 fbuf[] = { 1.5f, 0.2f, 0.2f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.Bounce, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.Bounce", 0), 4);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.2f, 0.2f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.Spring, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.Spring", 0), 4);
    }
    {
        static F32 fbuf[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sb.Wall, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sb.Wall", 0), 4);
    }

    globals.player.sb.WallJumpVelocity = xIniGetFloat(ini, "sb.WallJumpVelocity", 0.0f); // @1001;
    globals.player.sb.spin_damp_xz = xIniGetFloat(ini, "sb.spin_damp_xz", 15.0f); // @1018;
    globals.player.sb.spin_damp_y = xIniGetFloat(ini, "sb.spin_damp_y", 15.0f); // @1018;

    globals.player.s = &globals.player.sb;

    CalcJumpImpulse(&globals.player.sb.Jump, NULL);
    CalcJumpImpulse(&globals.player.sb.Double, NULL);
    CalcJumpImpulse(&globals.player.sb.Bounce, NULL);
    CalcJumpImpulse(&globals.player.sb.Spring, NULL);
    CalcJumpImpulse(&globals.player.sb.Wall, NULL);

    globals.player.sb_model_indices[0] =
        xIniGetInt(ini, "SB.model_index.body",
                   0); // These next 13 are either "sb_models" or "sb_model_indices"
    globals.player.sb_model_indices[1] = xIniGetInt(ini, "SB.model_index.arm_l", 1);
    globals.player.sb_model_indices[2] = xIniGetInt(ini, "SB.model_index.arm_r", 2);
    globals.player.sb_model_indices[3] = xIniGetInt(ini, "SB.model_index.ass", 3);
    globals.player.sb_model_indices[4] = xIniGetInt(ini, "SB.model_index.underwear", 4);
    globals.player.sb_model_indices[5] = xIniGetInt(ini, "SB.model_index.wand", 5);
    globals.player.sb_model_indices[6] = xIniGetInt(ini, "SB.model_index.tongue", 6);
    globals.player.sb_model_indices[7] = xIniGetInt(ini, "SB.model_index.bubble_helmet", 7);
    globals.player.sb_model_indices[8] = xIniGetInt(ini, "SB.model_index.bubble_shoe_l", 8);
    globals.player.sb_model_indices[9] = xIniGetInt(ini, "SB.model_index.bubble_shoe_r", 9);
    globals.player.sb_model_indices[13] = xIniGetInt(ini, "SB.model_index.shadow_wand", 10);
    globals.player.sb_model_indices[11] = xIniGetInt(ini, "SB.model_index.shadow_arm_l", 11);
    globals.player.sb_model_indices[12] = xIniGetInt(ini, "SB.model_index.shadow_arm_r", 12);
    globals.player.sb_model_indices[10] = xIniGetInt(ini, "SB.model_index.shadow_body", 13);

    {
        static F32 fbuf[] = { 0.6f, 3.0f, 7.0f, 0.1f, 0.5f, 1.0f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.MoveSpeed, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.MoveSpeed", 0), 6);
    }
    {
        static F32 fbuf[] = { 1.5f, 0.5f, 1.5f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.AnimSneak, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.AnimSneak", 0), 3);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.5f, 2.5f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.AnimWalk, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.AnimWalk", 0), 3);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.5f, 2.5f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.AnimRun, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.AnimRun", 0), 3);
    }

    globals.player.patrick.JumpGravity = xIniGetFloat(ini, "patrick.JumpGravity", 7.0f); // @1034;
    globals.player.patrick.GravSmooth = xIniGetFloat(ini, "patrick.GravSmooth", 0.25f); // @1035;
    globals.player.patrick.FloatSpeed = xIniGetFloat(ini, "patrick.FloatSpeed", 1.0f); // @1002;
    globals.player.patrick.ButtsmashSpeed =
        xIniGetFloat(ini, "patrick.ButtsmashSpeed", 5.0f); // @1017;
    globals.player.patrick.ledge.animGrab =
        xIniGetFloat(ini, "patrick.ledge.animGrab", 3.0f); // @1007;

    zLedgeAdjust(&globals.player.patrick.ledge);

    {
        static F32 fbuf[] = { 2.0f, 0.7f, 0.35f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.Jump, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.Jump", 0), 4);
    }
    {
        static F32 fbuf[] = { 1.0f, 0.7f, 0.35f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.Double, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.Double", 0), 4);
    }
    {
        static F32 fbuf[] = { 1.5f, 0.2f, 0.2f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.Bounce, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.Bounce", 0), 4);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.2f, 0.2f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.Spring, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.Spring", 0), 4);
    }
    {
        static F32 fbuf[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.patrick.Wall, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "patrick.Wall", 0), 4);
    }

    globals.player.s = &globals.player.sb;

    CalcJumpImpulse(&globals.player.patrick.Jump, NULL);
    CalcJumpImpulse(&globals.player.patrick.Double, NULL);
    CalcJumpImpulse(&globals.player.patrick.Bounce, NULL);
    CalcJumpImpulse(&globals.player.patrick.Spring, NULL);
    CalcJumpImpulse(&globals.player.patrick.Wall, NULL);

    globals.player.patrick.WallJumpVelocity =
        xIniGetFloat(ini, "patrick.WallJumpVelocity", 0.0f); // @1001;

    {
        static F32 fbuf[] = { 0.6f, 3.0f, 7.0f, 0.1f, 0.5f, 1.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.MoveSpeed, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.MoveSpeed", 0), 6);
    }
    {
        static F32 fbuf[] = { 1.5f, 0.5f, 1.5f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.AnimSneak, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.AnimSneak", 0), 3);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.5f, 2.5f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.AnimWalk, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.AnimWalk", 0), 3);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.5f, 2.5f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.AnimRun, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.AnimRun", 0), 3);
    }

    globals.player.sandy.JumpGravity = xIniGetFloat(ini, "sandy.JumpGravity", 7.0f); // @1034;
    globals.player.sandy.GravSmooth = xIniGetFloat(ini, "sandy.GravSmooth", 0.25f); // @1035;
    globals.player.sandy.FloatSpeed = xIniGetFloat(ini, "sandy.FloatSpeed", 1.0f); // @1002;
    globals.player.sandy.ButtsmashSpeed = xIniGetFloat(ini, "sandy.ButtsmashSpeed", 5.0f); // @1017;
    globals.player.sandy.ledge.animGrab = xIniGetFloat(ini, "sandy.ledge.animGrab", 3.0f); // @1007;

    zLedgeAdjust(&globals.player.sandy.ledge);

    {
        static F32 fbuf[] = { 2.0f, 0.7f, 0.35f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.Jump, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.Jump", 0), 4);
    }
    {
        static F32 fbuf[] = { 1.0f, 0.7f, 0.35f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.Double, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.Double", 0), 4);
    }
    {
        static F32 fbuf[] = { 1.5f, 0.2f, 0.2f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.Bounce, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.Bounce", 0), 4);
    }
    {
        static F32 fbuf[] = { 3.0f, 0.2f, 0.2f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.Spring, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.Spring", 0), 4);
    }
    {
        static F32 fbuf[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        ParseFloatList((F32*)memcpy(&globals.player.sandy.Wall, fbuf, sizeof(fbuf)),
                       xIniGetString(ini, "sandy.Wall", 0), 4);
    }

    globals.player.s = &globals.player.sb;

    CalcJumpImpulse(&globals.player.sandy.Jump, NULL);
    CalcJumpImpulse(&globals.player.sandy.Double, NULL);
    CalcJumpImpulse(&globals.player.sandy.Bounce, NULL);
    CalcJumpImpulse(&globals.player.sandy.Spring, NULL);
    CalcJumpImpulse(&globals.player.sandy.Wall, NULL);

    globals.player.sandy.WallJumpVelocity =
        xIniGetFloat(ini, "sandy.WallJumpVelocity", 0.0f); // @1001;

    sShowMenuOnBoot = xIniGetInt(ini, "ShowMenuOnBoot", 1);
    gGameSfxReport = xIniGetInt(ini, "SFXReport", 0);

    globals.player.s = &globals.player.sb;
    globals.player.sb.pcType = ePlayer_SB;
    globals.player.patrick.pcType = ePlayer_Patrick;
    globals.player.sandy.pcType = ePlayer_Sandy;
}

void zMainMemLvlChkCB()
{
    zSceneMemLvlChkCB();
}

void zMainShowProgressBar()
{
    S32 progBar;
    char loadingText[12];
    char auStack_cc[64];
    char acStack_8c[64];
    char formattedStr[64];

    if (zMenuIsFirstBoot())
    {
        if (percentageDone > 100)
        {
            percentageDone = 100;
        }
        progBar = percentageDone / 10;
        strcpy(loadingText, "Loading...");
        memset(auStack_cc, 0, 64);
        memset(formattedStr, 0, 64);
        strcpy(acStack_8c, loadingText);
        acStack_8c[progBar] = '\0';
        memcpy(formattedStr, &loadingText[progBar], strlen(loadingText) - progBar);
        sprintf(auStack_cc, "{font=0}{h*2}{w*2}%s{color=FFFFFFFF}%s{~:c}", acStack_8c,
                formattedStr);
        zMainMemCardRenderText(auStack_cc, '\x01');
        percentageDone += 10;
    }
}

void zMainLoop()
{
    iTime time;
    U32* preinit;
    RpAtomic* modl;
    U32 newGameSceneID;

    static U32 preinit_bubble_matfx[] = { 0xdbd033bc, 0x452279a2, 0xc17f4bcc,
                                          0x0cf9267a, 0x5c009d14, 0x00000000 };

    zMainShowProgressBar();
    xMemPushBase();
    time = iTimeGet();
    xUtil_idtag2string('BOOT', 0);
    iTimeDiffSec(time);
    xSTPreLoadScene('BOOT', NULL, 0x1);
    time = iTimeGet();
    xUtil_idtag2string('BOOT', 0);
    iTimeDiffSec(time);
    xSTQueueSceneAssets('BOOT', 1);
    time = iTimeGet();
    xUtil_idtag2string('BOOT', 0);
    iTimeDiffSec(time);

    do
    {
    } while (xSTLoadStep('BOOT') < 1.0f);

    xSTDisconnect('BOOT', 1);
    time = iTimeGet();
    xUtil_idtag2string('BOOT', 0);
    iTimeDiffSec(time);
    zMainShowProgressBar();
    xSTPreLoadScene('PLAT', 0, 1);
    xSTQueueSceneAssets('PLAT', 1);

    do
    {
    } while (xSTLoadStep('PLAT') < 1.0f);

    xSTDisconnect('PLAT', 1);
    zMainShowProgressBar();
    iTimeGet();
    xShadowSimple_Init();
    globals.pickupTable = (zAssetPickupTable*)xSTFindAssetByType('PICK', 0, 0);
    zPickupTableInit();
    xMemPushBase();
    time = iTimeGet();
    xUtil_idtag2string('MNU4', 0);
    iTimeDiffSec(time);
    xSTPreLoadScene('MNU4', 0, 1);
    time = iTimeGet();
    xUtil_idtag2string('MNU4', 0);
    iTimeDiffSec(time);
    xSTQueueSceneAssets('MNU4', 1);
    time = iTimeGet();
    xUtil_idtag2string('MNU4', 0);
    iTimeDiffSec(time);
    do
    {
    } while (xSTLoadStep('MNU4') < 1.0f);
    xSTDisconnect('MNU4', 1);
    zMainShowProgressBar();
    time = iTimeGet();
    xUtil_idtag2string('MNU4', 0);
    iTimeDiffSec(time);
    xMemPushBase();
    time = iTimeGet();
    xUtil_idtag2string('MNU5', 0);
    iTimeDiffSec(time);
    xSTPreLoadScene('MNU5', 0, 1);
    time = iTimeGet();
    xUtil_idtag2string('MNU5', 0);
    iTimeDiffSec(time);
    xSTQueueSceneAssets('MNU5', 1);
    time = iTimeGet();
    xUtil_idtag2string('MNU5', 0);
    iTimeDiffSec(time);
    do
    {
    } while (xSTLoadStep('MNU5') < 1.0f);
    xSTDisconnect('MNU5', 1);
    zMainShowProgressBar();
    time = iTimeGet();
    xUtil_idtag2string('MNU5', 0);
    iTimeDiffSec(time);
    xModelInit();
    xModelPoolInit(0x20, 0x40);
    xModelPoolInit(0x28, 8);
    xModelPoolInit(0x38, 1);

    for (preinit = preinit_bubble_matfx; *preinit != 0; preinit++)
    {
        modl = (RpAtomic*)xSTFindAsset(*preinit, NULL);
        if (modl)
        {
            xFXPreAllocMatFX(modl->clump);
        }
    }

    {
        static U32 menuModeID = 'MNU3';
        static U32 gameSceneID = (globals.sceneStart[0] << 24) |
                                 (globals.sceneStart[1] << 16) |
                                 (globals.sceneStart[2] << 8) | globals.sceneStart[3];

        newGameSceneID = gameSceneID;
        xUtil_idtag2string(gameSceneID, 0);
        xMemPushBase();
        zMainShowProgressBar();
        zMainMemCardSpaceQuery();

        while (1)
        {
            zMainShowProgressBar();
            xSerialWipeMainBuffer();
            zCutSceneNamesTable_clearAll();
            zMainShowProgressBar();

            if (sShowMenuOnBoot)
            {
                zMenuInit(menuModeID);
                zMainShowProgressBar();
                zMainShowProgressBar();
                zMenuSetup();
                xFX_SceneEnter(globals.sceneCur->env->geom->world);
                newGameSceneID = zMenuLoop();
                zMenuExit();
            }
            else
            {
                sShowMenuOnBoot = 1;
                globals.firstStartPressed = 1;
                zGameModeSwitch(eGameMode_Game);
                zGameStateSwitch(0);
            }

            if (newGameSceneID == 0)
            {
                zVarNewGame();
                iTimeSetGame(0.0f);
                gameSceneID = (globals.sceneStart[0] << 24) | (globals.sceneStart[1] << 16) |
                              (globals.sceneStart[2] << 8) | globals.sceneStart[3];
            }
            else
            {
                gameSceneID = newGameSceneID;
                iTimeSetGame(0.0f);
            }

            zGameInit(gameSceneID);
            zGameSetup();
            iProfileClear(gameSceneID);
            zGameLoop();
            zGameExit();
        }
    }
}

void zMainReadINI()
{
    char* str;
    void* buf;
    U32 size;
    xIniFile* ini;

    buf = iFileLoad("SB.INI", NULL, &size);
    if (buf)
    {
        ini = xIniParse((char*)buf, size);

        str = xIniGetString(ini, "PATH", NULL);
        if (str)
        {
            iFileSetPath(str);
        }

        str = xIniGetString(ini, "BOOT", NULL);
        if (str)
        {
            strcpy(globals.sceneStart, "BOOT");

            if (strlen(str) == 4)
            {
                strcpy(globals.sceneStart, str);

                if (xStricmp(str, "soak") == 0)
                {
                    gSoak = 1;
                    strcpy(globals.sceneStart, "mnu3");
                }
            }
        }

        globals.profile = xIniGetInt(ini, "Profile", 0);

        strncpy(globals.profFunc[0], xIniGetString(ini, "ProfFuncTriangle", ""), 128);
        globals.profFunc[0][127] = '\0';
        strncpy(globals.profFunc[1], xIniGetString(ini, "ProfFuncSquare", ""), 128);
        globals.profFunc[1][127] = '\0';
        strncpy(globals.profFunc[2], xIniGetString(ini, "ProfFuncLeft", ""), 128);
        globals.profFunc[2][127] = '\0';
        strncpy(globals.profFunc[3], xIniGetString(ini, "ProfFuncRight", ""), 128);
        globals.profFunc[3][127] = '\0';
        strncpy(globals.profFunc[4], xIniGetString(ini, "ProfFuncUp", ""), 128);
        globals.profFunc[4][127] = '\0';
        strncpy(globals.profFunc[5], xIniGetString(ini, "ProfFuncDown", ""), 128);
        globals.profFunc[5][127] = '\0';

        globals.useHIPHOP = xIniGetInt(ini, "EnableHipHopLoading", 0);
        globals.NoMusic = xIniGetInt(ini, "NoMusic", 0) != 0;

        zMainParseINIGlobals(ini);
        zUI_ParseINI(ini);
        xIniDestroy(ini);
    }

    iTime tim = iTimeGet();
    __FUNCTION__; // stands in for the (NDEBUG'd) load-timing report; retail's object
                  // carries the __FUNCTION__ string object this evaluation emits
    iTimeDiffSec(tim);
    iTimeGet();

    RwFree(buf);
}

void zMainFirstScreen(S32 mode)
{
    RwCamera* cam = iCameraCreate(640, 480, 0);
    RwRGBA bg = {};
    S32 i;
    S32 vbl;

    for (i = 0; i < 2; i++)
    {
        RwCameraClear(cam, &bg, 3);
        RwCameraBeginUpdate(cam);

        if (mode)
        {
            char text[617] =
                "Game and Software \xa9 2003 THQ Inc. \xa9 2003 Viacom International Inc. All "
                "rights reserved.\n"
                "\n"
                "Nickelodeon, SpongeBob SquarePants and all related titles, logos, and characters "
                "are trademarks of Viacom International Inc. Created by Stephen Hillenburg.\n"
                "\n"
                "Exclusively published by THQ Inc. Developed by Heavy Iron. Portions of this "
                "software are Copyright 1998 - 2003 Criterion Software Ltd. and its Licensors. "
                "THQ, Heavy Iron and the THQ logo are trademarks and/or registered trademarks of "
                "THQ Inc. All rights reserved.\n"
                "\n"
                "All other trademarks, logos and copyrights are property of their respective "
                "owners.\n"
                "\n"
                "Licensed by Nintendo";
            iColor_tag color = { 255, 230, 0, 200 };
            xtextbox tb = xtextbox::create(
                xfont::create(1, NSCREENX(19.0f), NSCREENY(22.0f), 0.0f, color, screen_bounds),
                screen_bounds, 2, 0.0f, 0.0f, 0.0f, 0.0f);

            tb.set_text(text);
            tb.bounds = screen_bounds;
            tb.bounds.contract(0.1f);
            tb.bounds.h = tb.yextent(1);
            tb.bounds.y = 0.5f - 0.5f * tb.bounds.h;
            tb.render(1);
        }

        RwCameraEndUpdate(cam);
        RwCameraShowRaster(cam, NULL, 1);
    }

    vbl = 180;
    while (--vbl)
    {
        iTRCDisk::CheckDVDAndResetState();
        iVSync();
    }

    iCameraDestroy(cam);
}

// non-matching at 97.051% (121 differing rows of 416; our object carries three
// extra `mr` copies).  Pure REGS: the callee-saved set is r20-r31 in both, the
// instruction structure matches, and only the colouring differs.  Splitting the
// declarations from the initialisations and reordering them permutes the
// colours - the best of ~120 orders measured, `startBytes, formatFailed,
// formatInProgress, do_chk, fullCard, workArea, status, startupError`, reaches
// 97.888% / 53 rows with five of the nine registers exactly right - but it can
// never close, because our allocator pins the function's two anonymous temps
// (the hoisted `globals` base and the `result` of CARDGetResultCode) to r30 and
// r31 in every one of the 15 declaration orders sampled, while retail puts them
// in r28/r29 and gives r30/r31 to startBytes and workArea.  Reverted to the
// readable form; hoisting `result` to function scope (9 positions) and binding
// `zGlobals& g = globals;` (9 positions) were also measured - the first changes
// nothing at all, the second costs 1.4 points.
void zMainMemCardSpaceQuery()
{
    S32 bytesNeeded = 0;
    S32 availOnDisk = 0;
    S32 neededFiles = 0;
    S32 do_chk = 1;
    S32 status = 1;
    U8 formatInProgress = 0;
    U8 formatFailed = 0;
    eStartupErrors startupError = eNoError;
    S32 fullCard = -1;
    S32 startBytes = 0;
    void* workArea = RwMalloc(CARD_WORKAREA_SIZE);

    while (1)
    {
        iTRCDisk::CheckDVDAndResetState();

        if (do_chk)
        {
            status = zMenuCardCheckStartup(&bytesNeeded, &availOnDisk, &neededFiles);
        }

        if (bytesNeeded == -1 && availOnDisk == -1 && neededFiles == -1)
        {
            startupError = eNoFormat;
            fullCard = zMenuGetBadCard() - 1;
        }

        if (bytesNeeded == -5 && availOnDisk == -5 && neededFiles == -5)
        {
            startupError = eDamagedCard;
            fullCard = zMenuGetBadCard() - 1;
        }

        if (bytesNeeded == -2 && availOnDisk == -2 && neededFiles == -2)
        {
            startupError = eWrongDevice;
            fullCard = zMenuGetBadCard() - 1;
        }

        if (bytesNeeded == -3 && availOnDisk == -3 && neededFiles == -3)
        {
            startupError = eNoCards;
            fullCard = -1;
        }

        if (bytesNeeded == -4 && availOnDisk == -4 && neededFiles == -4)
        {
            startupError = eCorruptFile;
            fullCard = zMenuGetBadCard() - 1;
        }

        if (fullCard >= 0 && formatInProgress)
        {
            char bar[16];
            char msg[256];
            S32 result = CARDGetResultCode(fullCard);
            F32 pct;

            memset(bar, 0, sizeof(bar));
            strcpy(bar, "oooooooo");
            pct = (F32)(CARDGetXferredBytes(fullCard) - startBytes) / CARD_WORKAREA_SIZE;
            bar[(S32)(pct * strlen(bar))] = 'O';
            sprintf(msg, "{i:text_mem_card_formatting}{n}[%s]", bar);
            zMainMemCardRenderText(msg, 1);

            switch (result)
            {
            case CARD_RESULT_BUSY:
                break;
            case CARD_RESULT_READY:
                CARDUnmount(fullCard);
                do_chk = 1;
                formatInProgress = 0;
                formatFailed = 0;
                startupError = eNoError;
                fullCard = -1;
                continue;
            case CARD_RESULT_NOCARD:
                CARDUnmount(fullCard);
                formatInProgress = 0;
                formatFailed = 1;
                fullCard = -1;
                zMainMemCardRenderText("{i:text_mem_card_format_failed_yanked}", 1);
                break;
            default:
                CARDUnmount(fullCard);
                formatInProgress = 0;
                formatFailed = 1;
                fullCard = -1;
                zMainMemCardRenderText("{i:text_mem_card_format_failed}", 1);
                break;
            }
        }

        if (status == 0 && startupError == eNoError)
        {
            break;
        }

        do_chk = 0;

        if (!formatInProgress && !formatFailed && startupError == eNoError)
        {
            zSceneCardCheckStartup_set(bytesNeeded, availOnDisk, neededFiles);
            zMainMemCardQueryPost(bytesNeeded, availOnDisk, neededFiles, 1);
        }
        else if (!formatInProgress)
        {
            switch (startupError)
            {
            case eNoFormat:
                if (!formatFailed)
                {
                    zMainMemCardRenderText("{i:text_mem_card_no_format}", 1);
                }
                break;
            case eDamagedCard:
                zMainMemCardRenderText("{i:text_mem_card_damaged_card}", 1);
                break;
            case eWrongDevice:
                zMainMemCardRenderText("{i:text_mem_card_wrong_device}", 1);
                break;
            case eNoCards:
                zMainMemCardRenderText("{i:text_mem_card_no_card}", 1);
                break;
            case eCorruptFile:
                zMainMemCardRenderText("{i:text_mem_card_corrupt_file}", 1);
                break;
            case eNoController:
                break;
            }
        }

        if (globals.pad0)
        {
            xPadUpdate(globals.currentActivePad, 1.0f / 60);
        }

        if (globals.pad0 && (globals.pad0->pressed & XPAD_BUTTON_TRIANGLE))
        {
            do_chk = 1;
            startupError = eNoError;
            formatInProgress = 0;
            formatFailed = 0;
            fullCard = -1;
            zMainMemCardRenderText("{i:text_retry}", 1);
        }
        else if (globals.pad0 && (globals.pad0->pressed & XPAD_BUTTON_X))
        {
            if (startupError != eNoController)
            {
                break;
            }

            do_chk = 1;
            startupError = eNoError;
        }
        else if (globals.pad0 && (globals.pad0->pressed & XPAD_BUTTON_O))
        {
            switch (startupError)
            {
            case eNoFormat:
                if (!formatInProgress)
                {
                    if (fullCard >= 0)
                    {
                        CARDMount(fullCard, workArea, NULL);
                        CARDCheck(fullCard);
                        CARDFormatAsync(fullCard, NULL);
                        startBytes = CARDGetXferredBytes(fullCard);
                        formatInProgress = 1;
                    }
                }
                else if (formatFailed)
                {
                    do_chk = 1;
                    startupError = eNoError;
                    formatInProgress = 0;
                    formatFailed = 0;
                    fullCard = -1;
                }
                break;
            case eDamagedCard:
            case eWrongDevice:
            case eNoCards:
                break;
            case eCorruptFile:
            {
                char files[3][64];
                S32 fileCount;
                S32 i;

                CARDMount(fullCard, workArea, NULL);
                CARDCheck(fullCard);
                fileCount = zMenuGetCorruptFiles(files);

                for (i = 0; i < fileCount; i++)
                {
                    S32 result;

                    zMainMemCardRenderText("{i:text_mem_card_deleting_file}", 1);
                    result = CARDDelete(fullCard, files[i]);

                    if (result != CARD_RESULT_READY)
                    {
                        zMainMemCardRenderText("{i:text_mem_card_delete_failed}", 1);
                    }

                    while (result == CARD_RESULT_BUSY)
                    {
                        result = CARDGetResultCode(fullCard);
                        zMainMemCardRenderText("{i:text_mem_card_deleting_file}", 1);
                    }

                    if (result != CARD_RESULT_READY)
                    {
                        zMainMemCardRenderText("{i:text_mem_card_delete_failed}", 1);
                    }
                }

                CARDUnmount(fullCard);
                do_chk = 1;
                startupError = eNoError;
                formatInProgress = 0;
                formatFailed = 0;
                fullCard = -1;
                break;
            }
            default:
                if (startupError != eWrongDevice && status == 0 && workArea)
                {
                    RwFree(workArea);
                    workArea = NULL;
                }

                OSResetSystem(1, 0, 1);
                break;
            }
        }
    }

    zMainMemCardQueryPost(0, 0, 0, 0);

    if (workArea)
    {
        RwFree(workArea);
    }
}

static void zMainMemCardQueryPost(S32 needed, S32 available, S32 neededFiles, S32 unk0)
{
    RwCamera* cam = NULL;
    RwRGBA colour = {};
    RwInt32 clearMode = 3;

    cam = iCameraCreate(640, 480, 0);
    RwCameraClear(cam, &colour, clearMode);
    RwCameraBeginUpdate(cam);
    render_mem_card_no_space(needed, available, neededFiles, unk0 != 0);
    RwCameraEndUpdate(cam);
    RwCameraShowRaster(cam, NULL, 1);
    iCameraDestroy(cam);
}

void zMainMemCardRenderText(const char* a, bool enabled)
{
    RwCamera* cam = NULL;
    RwRGBA colour = {};
    RwInt32 clearMode = 3;

    cam = iCameraCreate(640, 480, 0);
    RwCameraClear(cam, &colour, clearMode);
    RwCameraBeginUpdate(cam);
    RenderText(a, enabled);
    RwCameraEndUpdate(cam);
    RwCameraShowRaster(cam, NULL, 1);
    iCameraDestroy(cam);
}

void zMainLoadFontHIP()
{
    iTime time;

    xMemPushBase();
    time = iTimeGet();
    xUtil_idtag2string('FONT', 0);
    iTimeDiffSec(time);
    xSTPreLoadScene('FONT', NULL, 1);
    time = iTimeGet();
    xUtil_idtag2string('FONT', 0);
    iTimeDiffSec(time);
    xSTQueueSceneAssets('FONT', 1);
    time = iTimeGet();
    xUtil_idtag2string('FONT', 0);
    iTimeDiffSec(time);

    do
    {
    } while (xSTLoadStep('FONT') < 1.0f);

    xSTDisconnect('FONT', 1);
    time = iTimeGet();
    xUtil_idtag2string('FONT', 0);
    iTimeDiffSec(time);
}

void iEnvStartup()
{
}
