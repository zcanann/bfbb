#include "zMenu.h"

#include <string.h>

#include "iColor.h"
#include "iSystem.h"
#include "iTime.h"
#include "iTRC.h"

#include "xCutscene.h"
#include "xDebug.h"
#include "xEvent.h"
#include "xMath.h"
#include "xPad.h"
#include "xsavegame.h"
#include "xScrFx.h"
#include "xSFX.h"
#include "xSnd.h"
#include "xSkyDome.h"
#include "xString.h"
#include "xTRC.h"

#include "zCamera.h"
#include "zFMV.h"
#include "zGame.h"
#include "zGameExtras.h"
#include "zGameState.h"
#include "zGlobals.h"
#include "zMusic.h"
#include "zSaveLoad.h"

#if defined(PS2)
#include <rwim3d.h>
#endif

#if defined(VERSION_GQPP78) || defined(VERSION_GU4Y78)
enum { MENU_SCREEN_HEIGHT = 528, MENU_VBLANKS_PER_SECOND = 50 };
#else
enum { MENU_SCREEN_HEIGHT = 480, MENU_VBLANKS_PER_SECOND = 60 };
#endif

bool menu_fmv_played;

// These three are declared here rather than beside time_elapsed and holdTmr
// because .sbss is laid out in declaration order, and the target's order is
// menu_fmv_played, time_last, time_current, sAttractMode_timer, card, sInMenu,
// corruptFileCount. time_elapsed and holdTmr have non-zero initialisers, so
// they land in .sdata and are not part of that sequence.
static volatile F32 time_last;
static volatile F32 time_current;
static volatile F32 sAttractMode_timer;

static S32 sFirstBoot = 1;

// TODO: This probably wasn't volatile in the original code, but we have 100%
// matching code for all of the functions which use it with it marked as
// volatile, so leave it that way for now.
static volatile S32 card;
static S32 sInMenu;
static S32 corruptFileCount;
static char corruptFiles[3][64];

static volatile F32 time_elapsed = 1.0f / 100.0f;
static volatile F32 holdTmr = 10.0f;

S32 zMenuRunning()
{
    return sInMenu;
}

void zMenuInit(U32 theSceneID)
{
    sInMenu = 1;
    xsrand(iTimeGet());
    iTimeGet();
    xrand();
    zSceneInit(theSceneID, 0);
    xCameraInit(&globals.camera, 640, MENU_SCREEN_HEIGHT);
    zCameraReset(&globals.camera);
    xCameraSetScene(&globals.camera, globals.sceneCur);
    zMusicInit();
}

void zMenuExit()
{
    zMusicKill();
    xSndStopAll(-5);
    xCameraExit(&globals.camera);
    zSceneExit(0);
    sInMenu = 0;
}

void zMenuSetup()
{
    globals.player.MaxHealth = 3;
    zSceneSetup();
    sAttractMode_timer = 48.264f;
    zGameSetupPlayer();
    zEnvStartingCamera(gCurEnv);
    xCameraUpdate(&globals.camera, -1.0f);
    xSkyDome_Setup();
    zEntEventAll(NULL, 0, eEventSceneBegin, NULL);
    zEntEventAll(NULL, 0, eEventRoomBegin, NULL);
    if (globals.updateMgr)
    {
        xUpdateCull_Update(globals.updateMgr, eEventDispatcher_PadVibrateOn);
    }
    zEntEvent(&globals.player.ent, 8);
}

void zMenuFirstBootSet(S32 value);

U32 zMenuLoop()
{
    S32 s = 0;
    if (sFirstBoot)
    {
        zGameModeSwitch(eGameMode_Intro);
        zGameStateSwitch(0);
        zMusicNotify(0);
    }
    else
    {
        zGameModeSwitch(eGameMode_Title);
        zGameStateSwitch(0);
        zMusicNotify(0);
    }

    time_last = 1.0f / float(GET_BUS_FREQUENCY() >> 2) * iTimeGet() - 1.0f / (F32)MENU_VBLANKS_PER_SECOND;

    zGameExtras_NewGameReset();

    iColor_tag black = { 0, 0, 0, 255 }; //BlackColorInitializer;
    iColor_tag clear = { 0 }; //ClearColorInitializer;
    xScrFxFade(&black, &clear, 0.0f, NULL, 1);
    S32 ostrich_delay = 0xa;

    time_last = 1.0f / float(GET_BUS_FREQUENCY() >> 2) * iTimeGet() - 1.0f / (F32)MENU_VBLANKS_PER_SECOND;

    S32 draw_black;

    do
    {
        draw_black = 0;
        if (zGameModeGet() == eGameMode_Intro)
        {
            switch (zGameStateGet())
            {
            case 0:
                draw_black = 1;
                zGameStateSwitch(3);
                xScrFxFade(&clear, &black, 0.0f, NULL, 1);
                break;
            case 3:
                draw_black = 1;
                zFMVPlay(zFMVFileGetName(eFMVFile_LogoNick), 0x10000, 2.0f, 1, 0);
                zFMVPlay(zFMVFileGetName(eFMVFile_LogoTHQ), 0x10000, 2.0f, 1, 0);
                zFMVPlay(zFMVFileGetName(eFMVFile_LogoRW), 0x10000, 2.0f, 1, 0);
                holdTmr = 10.0f;
                zGameModeSwitch(eGameMode_Title);
                zGameStateSwitch(1);
                xScrFxFade(&black, &clear, 0.1f, NULL, 1);

                time_last = 1.0f / float(GET_BUS_FREQUENCY() >> 2) * iTimeGet() - 1.0f / (F32)MENU_VBLANKS_PER_SECOND;
                break;
            }
        }

        time_current = 1.0f / float(GET_BUS_FREQUENCY() >> 2) * iTimeGet();

        F32 current = time_current;

        time_elapsed = current - time_last;

        if (menu_fmv_played && time_elapsed > 1.0f / (F32)MENU_VBLANKS_PER_SECOND)
        {
            time_last = current - 1.0f / (F32)MENU_VBLANKS_PER_SECOND;
            time_elapsed = 1.0f / (F32)MENU_VBLANKS_PER_SECOND;
        }

        menu_fmv_played = 0;
        if (time_elapsed < 0.0f)
        {
            time_elapsed = 1.0f / (F32)MENU_VBLANKS_PER_SECOND;
        }

        if (zGameModeGet() == eGameMode_Title)
        {
            sAttractMode_timer -= time_elapsed;
            if (sAttractMode_timer < 0.0f)
            {
                xEnt* title1 = (xEnt*)zSceneFindObject(xStrHash("MNU3 PRESS START 02 UIF"));
                xEnt* title2 = (xEnt*)zSceneFindObject(xStrHash("MNU3 START NEWGAME UIF"));
                if ((title1 && xEntIsVisible(title1)) || (title2 && xEntIsVisible(title2)))
                {
                    zGameStateSwitch(1);
                }
                else
                {
                    sAttractMode_timer = 48.264;
                }
            }

            if (zGameStateGet() == 1)
            {
                if (sFirstBoot)
                {
                    zMenuFirstBootSet(0);
                }
                else
                {
                    zMenuFMVPlay(zFMVFileGetName(eFMVFile_Demo1), -1, 0.1f, true,
                                 *(bool*)&globals.firstStartPressed);
                }
                zGameStateSwitch(0);
                sAttractMode_timer = 48.264f;
            }
            else
            {
                zMusicUpdate(time_elapsed);
#if defined(VERSION_GQPP78) || defined(VERSION_GU4Y78)
                zEntEvent("mnu3 black card", eEventInvisible);
#endif
            }

            if (mPad[globals.currentActivePad].pressed)
            {
                sAttractMode_timer = 48.264f;
            }
        }

        time_last = time_current;
        xPadUpdate(globals.currentActivePad, time_elapsed);

        if (globals.firstStartPressed == 0)
        {
            xPadUpdate(1, time_elapsed);
            xPadUpdate(2, time_elapsed);
            xPadUpdate(3, time_elapsed);
        }

        xDrawBegin();
        S32 wasPaused = zMenuIsPaused();
        zSceneUpdate(time_elapsed);

        if (wasPaused == 0)
        {
            globals.player.ent.update(&globals.player.ent, globals.sceneCur, time_elapsed);
        }

        xSndUpdate();
#if !defined(PS2)
        iTRCDisk::CheckDVDAndResetState();
#endif
        zCameraUpdate(&globals.camera, time_elapsed);
        xCameraBegin(&globals.camera, 1);
        zSceneRender();
        xDebugUpdate();

        if (draw_black)
        {
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
            RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)1);
            RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)1);
            xScrFxDrawScreenSizeRectangle();
        }

        xDrawEnd();
        xCameraEnd(&globals.camera, time_elapsed, 1);
        xCameraShowRaster(&globals.camera);

        s = zMenuUpdateMode();

        if (globals.sceneCur->pendingPortal)
        {
            char* src = (char*)&globals.sceneCur->pendingPortal->passet->sceneID;
            char* tgt = (char*)&s;
            tgt[0] = src[3];
            tgt[1] = src[2];
            tgt[2] = src[1];
            tgt[3] = src[0];
            if (tgt[3] < '0' || tgt[3] > '9')
            {
                memcpy(tgt, src, sizeof(U32));
            }
            zGameModeSwitch(eGameMode_Game);
            zGameStateSwitch(0);
        }

        ++gFrameCount;

        if (ostrich_delay > 0)
        {
            --ostrich_delay;
        }
        else
        {
            zGameSetOstrich(eGameOstrich_InScene);
            if (gTrcPad[0].state != TRC_PadInserted)
            {
                globals.dontShowPadMessageDuringLoadingOrCutScene = 1;
                xTRCPad(gTrcPad[0].id, TRC_PadMissing);
            }
        }
    } while (zMenuLoopContinue() != 0);

    zGameExtras_NewGameReset();
    return s;
}

U32 zMenuIsPaused()
{
    return 0;
}

S32 zMenuLoopContinue()
{
    return gGameMode != eGameMode_Game;
}

U32 zMenuUpdateMode()
{
    U32 retVal = 0;
    if (gGameMode == eGameMode_Load)
    {
        F32 elapsed1 = float(iTimeGet()) - time_last;
        retVal = zSaveLoad_LoadLoop();
        if (retVal == '0000')
        {
            retVal = 0;
        }
        else if (retVal == 'MNU3')
        {
            retVal = 'HB00';
        }
        time_last = elapsed1 + float(iTimeGet());
    }
    else if (gGameMode == eGameMode_Save)
    {
        zSaveLoad_SaveLoop();
        globals.autoSaveFeature = 1;
        retVal = 'HB00';
    }
    return retVal;
}

U32 zMenuGetCorruptFiles(char name[][64])
{
    for (int i = 0; i < corruptFileCount; ++i)
    {
        strcpy(name[i], corruptFiles[i]);
    }
    return corruptFileCount;
}

#if !defined(PS2)
bool zMenuCardCheckStartup(S32* bytesNeeded, S32* availOnDisk, S32* neededFiles)
{
    st_XSAVEGAME_DATA* ldinst = xSGInit(XSG_MODE_SAVE);
    S32 tgtcnt, tgtmax;
    S32 rc = 0;
    tgtcnt = xSGTgtCount(ldinst, &tgtmax);
    S32 tgtslot;
    switch (tgtcnt)
    {
    case 2:
        card = -1;
        xSGTgtPhysSlotIdx(ldinst, 0);
        xSGTgtSelect(ldinst, 0);
        tgtslot = xSGTgtIsFormat(ldinst, 0, NULL);
        if (tgtslot == 0)
        {
            *bytesNeeded = -1;
            *availOnDisk = -1;
            *neededFiles = -1;
            card = xSGTgtPhysSlotIdx(ldinst, 0);
            rc = 1;
            break;
        }
        else if (tgtslot < 0)
        {
            *bytesNeeded = -5;
            *availOnDisk = -5;
            *neededFiles = -5;
            card = xSGTgtPhysSlotIdx(ldinst, 0);
            rc = 1;
            break;
        }
        else
        {
            if ((corruptFileCount = xSGCheckForCorruptFiles(ldinst, corruptFiles)) != 0)
            {
                *bytesNeeded = -4;
                *availOnDisk = -4;
                *neededFiles = -4;
                card = xSGTgtPhysSlotIdx(ldinst, 0);
                rc = 1;
                break;
            }
            else
            {
                rc = xSGTgtHaveRoomStartup(ldinst, 0, 0x0000CC00, -1, bytesNeeded, availOnDisk,
                                           neededFiles);
                if (rc)
                {
                    break;
                }
                card = xSGTgtPhysSlotIdx(ldinst, 0);
                rc = 0;
                break;
            }
        }

    case 1:
        if (xSGTgtPhysSlotIdx(ldinst, 0) == 1)
        {
            card = iSGCheckForWrongDevice();
            if (card >= 0)
            {
                rc = 1;
                *bytesNeeded = -2;
                *availOnDisk = -2;
                *neededFiles = -2;
                break;
            }
        }

        xSGTgtSelect(ldinst, 0);
        tgtslot = xSGTgtIsFormat(ldinst, 0, NULL);
        if (tgtslot == 0)
        {
            *bytesNeeded = -1;
            *availOnDisk = -1;
            *neededFiles = -1;
            card = xSGTgtPhysSlotIdx(ldinst, 0);
            rc = 1;
            break;
        }
        else if (tgtslot < 0)
        {
            *bytesNeeded = -5;
            *availOnDisk = -5;
            *neededFiles = -5;
            card = xSGTgtPhysSlotIdx(ldinst, 0);
            rc = 1;
            break;
        }
        else
        {
            if ((corruptFileCount = xSGCheckForCorruptFiles(ldinst, corruptFiles)) != 0)
            {
                *bytesNeeded = -4;
                *availOnDisk = -4;
                *neededFiles = -4;
                card = xSGTgtPhysSlotIdx(ldinst, 0);
                rc = 1;
                break;
            }
            else
            {
                rc = xSGTgtHaveRoomStartup(ldinst, 0, 0x0000CC00, -1, bytesNeeded, availOnDisk,
                                           neededFiles);
                if (rc)
                {
                    break;
                }
                card = xSGTgtPhysSlotIdx(ldinst, 0);
                break;
            }
        }

    case 0:
        rc = 1;
        card = iSGCheckForWrongDevice();
        if (card >= 0)
        {
            *bytesNeeded = -2;
            *availOnDisk = -2;
            *neededFiles = -2;
            break;
        }
        else
        {
            *bytesNeeded = -3;
            *availOnDisk = -3;
            *neededFiles = -3;
            break;
        }
    }
    xSGDone(ldinst);
    return !rc;
}
#else
// PS2 memory cards have no wrong-device or corrupt-file startup checks; an
// unformatted card only needs an existing game directory.
bool zMenuCardCheckStartup(S32* bytesNeeded, S32* availOnDisk, S32* neededFiles)
{
    st_XSAVEGAME_DATA* ldinst = xSGInit(XSG_MODE_SAVE);
    S32 tgtcnt, tgtmax;
    S32 rc = 0;
    S32 tgtslot;

    tgtcnt = xSGTgtCount(ldinst, &tgtmax);
    switch (tgtcnt)
    {
    case 2:
        card = -1;
        tgtslot = xSGTgtPhysSlotIdx(ldinst, 0);
        xSGTgtSelect(ldinst, 0);
        if (xSGTgtIsFormat(ldinst, 0, NULL) == 0)
        {
            if (xSGTgtHasGameDir(ldinst, tgtslot))
            {
                rc = 1;
                if (bytesNeeded)
                {
                    *bytesNeeded = 0;
                }
                if (availOnDisk)
                {
                    *availOnDisk = 0;
                }
                if (neededFiles)
                {
                    *neededFiles = 0;
                }
            }
            else
            {
                *bytesNeeded = -3;
                *availOnDisk = -3;
                *neededFiles = -3;
            }
            break;
        }
        if (!xSGTgtHaveRoomStartup(ldinst, 0, 0x0000CC00, -1, bytesNeeded, availOnDisk,
                                   neededFiles))
        {
            card = xSGTgtPhysSlotIdx(ldinst, 0);
            break;
        }
        tgtslot = xSGTgtPhysSlotIdx(ldinst, 0);
        xSGTgtSelect(ldinst, 1);
        if (xSGTgtIsFormat(ldinst, 1, NULL) == 0)
        {
            if (xSGTgtHasGameDir(ldinst, tgtslot))
            {
                rc = 1;
                if (bytesNeeded)
                {
                    *bytesNeeded = 0;
                }
                if (availOnDisk)
                {
                    *availOnDisk = 0;
                }
                if (neededFiles)
                {
                    *neededFiles = 0;
                }
            }
            else
            {
                *bytesNeeded = -3;
                *availOnDisk = -3;
                *neededFiles = -3;
            }
            break;
        }
        rc = xSGTgtHaveRoomStartup(ldinst, 0, 0x0000CC00, -1, bytesNeeded, availOnDisk,
                                   neededFiles);
        if (!rc)
        {
            rc = 1;
        }
        break;
    case 1:
        tgtslot = xSGTgtPhysSlotIdx(ldinst, 0);
        xSGTgtSelect(ldinst, 0);
        if (xSGTgtIsFormat(ldinst, 0, NULL) == 0)
        {
            if (xSGTgtHasGameDir(ldinst, tgtslot))
            {
                rc = 1;
                if (bytesNeeded)
                {
                    *bytesNeeded = 0;
                }
                if (availOnDisk)
                {
                    *availOnDisk = 0;
                }
                if (neededFiles)
                {
                    *neededFiles = 0;
                }
            }
            else
            {
                *bytesNeeded = -3;
                *availOnDisk = -3;
                *neededFiles = -3;
            }
            break;
        }
        rc = xSGTgtHaveRoomStartup(ldinst, 0, 0x0000CC00, -1, bytesNeeded, availOnDisk,
                                   neededFiles);
        if (!rc)
        {
            card = xSGTgtPhysSlotIdx(ldinst, 0);
        }
        break;
    case 0:
        *bytesNeeded = -3;
        *availOnDisk = -3;
        rc = 1;
        *neededFiles = -3;
        break;
    }
    xSGDone(ldinst);
    return !rc;
}
#endif

S32 zMenuGetBadCard()
{
    return card + 1;
}

// Floating point assignments are out of order.
void zMenuFMVPlay(char* filename, U32 buttons, F32 time, bool skippable, bool lockController)
{
    if (filename)
    {
        zFMVPlay(filename, buttons, time, skippable, lockController);
        time_last = (float)iTimeGet();
        time_current = time_last;
        sAttractMode_timer = 48.264f;
    }
}

S32 zMenuIsFirstBoot()
{
    return sFirstBoot;
}

void zMenuFirstBootSet(S32 value)
{
    sFirstBoot = value;
}
