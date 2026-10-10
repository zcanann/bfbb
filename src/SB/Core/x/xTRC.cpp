#include "xTRC.h"
#include "xSnd.h"

#include "zGame.h"
#include "zGlobals.h"
#include "xMath2.h"
#include "xFont.h"
#include "zVar.h"

#if defined(PS2)
#include <rwplcore.h>
#include "zMenu.h"
#include "zHud.h"
#include "zGameExtras.h"
#include "zEntPlayerOOBState.h"
void zGamePauseIfPossible();
#endif

#include <string.h>
#include <types.h>

_tagTRCPadInfo gTrcPad[4];
_tagTRCState gTrcDisk[2];

static const char* __deadstripped_xTRC()
{
    return "The Controller in Controller Socket 1 has been removed. Reconnect the Controller to continue with the game.\0"
           "The Controller in Controller Socket 1 is an unknown type. Connect a standard Controller to continue with the game.\0"
           "Please wait... Checking for the 'SpongeBob SquarePants: Battle for Bikini Bottom' Game Disc.\0"
           "The Disc Cover is open. If you want to continue the game, please close the Disc Cover.\0"
           "Please insert the 'SpongeBob SquarePants: Battle for Bikini Bottom' Game Disc.\0"
           "This is not the 'SpongeBob SquarePants: Battle for Bikini Bottom' Game Disc. Please insert the 'SpongeBob SquarePants: Battle for Bikini Bottom' Game Disc.\0"
           "The Game Disc could not be read. Please read the Nintendo GameCube\x99 Instruction Booklet for more information.\0"
           "An error has occurred. Turn the power off and refer to the Nintendo GameCube\x99 Instruction Booklet for further instructions.";
}

static const basic_rect<F32> screen_bounds = { 0.0f, 0.0f, 1.0f, 1.0f };
#if defined(PS2)
static const iColor_tag yellow = { 0xFF, 0xE6, 0x00, 0xFF };
#else
static const S32 yellow = 0xFFE600FF;
#endif

void xTRCInit()
{
    memset(gTrcPad, 0, sizeof(gTrcPad));
    gTrcPad[0].id = 0;
#if defined(PS2)
    gTrcPad[3].id = 3;
    gTrcPad[2].id = 2;
    gTrcPad[1].id = 1;
#else
    gTrcPad[1].id = 1;
    gTrcPad[2].id = 2;
    gTrcPad[3].id = 3;
#endif
    memset(gTrcDisk, 0, 8);
}

static void render_message(const char* s)
{
    static xtextbox tb = xtextbox::create(xfont::create(1, NSCREENX(19.0f), NSCREENY(22.0f), 0.0f,
#if defined(PS2)
                                                        yellow, screen_bounds),
#else
                                                        *(iColor_tag*)&yellow, screen_bounds),
#endif
                                          screen_bounds, 0x2, 0.0f, 0.0f, 0.0f, 0.0f);

    tb.set_text(s);
    tb.bounds = screen_bounds;
    tb.bounds.contract(0.1f);

    tb.bounds.h = tb.yextent(true);
    tb.bounds.y = -(0.5f * tb.bounds.h - 0.5f);

    render_fill_rect(tb.font.clip, xColorFromRGBA(0, 0, 0, 0xC8));

    tb.render(true);
}

static const char* message_text;

#if defined(PS2)
static U32 standalone;

static U32 pad_message_valid()
{
    if (gGameMode == eGameMode_Boot || gGameMode == eGameMode_Intro)
    {
        return 0;
    }
    if (zMenuRunning())
    {
        return 0;
    }
    if (zGameModeGet() == eGameMode_Save || zGameModeGet() == eGameMode_Load)
    {
        return 1;
    }
    if (gBusStopIsRunning)
    {
        globals.dontShowPadMessageDuringLoadingOrCutScene = false;
        return 0;
    }
    if (zGameGetOstrich() != eGameOstrich_InScene)
    {
        return 0;
    }
    if (!oob_state::IsPlayerInControl())
    {
        zGameStall();
        return 1;
    }
    if (globals.sceneCur->sceneID == 'PG12')
    {
        zGameStall();
    }
    if (globals.player.ControlOff)
    {
        if (globals.player.ControlOff & 0x10)
        {
            return 1;
        }
        if (globals.sceneCur && globals.sceneCur->sceneID == 'MNU3')
        {
            return 1;
        }
        if (zGame_HackIsGallery())
        {
            return 1;
        }
        if ((globals.player.ControlOff & 0x2) &&
#if defined(VERSION_SLES_51970)
            globals.sceneCur &&
#endif
            globals.sceneCur->sceneID == 'HB10')
        {
            return 1;
        }
#if defined(VERSION_SLES_51970)
        if (globals.player.ControlOff & 0x20)
        {
            zGameStall();
            return 1;
        }
#endif
        if (gGameMode == eGameMode_Stall)
        {
            return 1;
        }
        globals.dontShowPadMessageDuringLoadingOrCutScene = false;
        return 0;
    }
    return 1;
}

S32 DisplayMessage(_tagTRCState state)
{
    switch (state)
    {
    case TRC_Unknown:
        message_text = NULL;
        break;
    case TRC_PadMissing:
        if (!pad_message_valid())
        {
            return 0;
        }
#if defined(VERSION_SLES_51970)
        message_text = "{i:text_controller_missing_pal}";
#elif defined(VERSION_SLES_51968)
        message_text = "The analog controller (DUALSHOCK\xAE" "2) in controller port 1 has been "
                       "removed. Reinsert the analog controller (DUALSHOCK\xAE" "2) to continue "
                       "with the game.";
#else
        message_text = "The DUALSHOCK\xAE" "2 analog controller in controller port 1 has been "
                       "removed. Reinsert the DUALSHOCK\xAE" "2 analog controller to continue "
                       "with the game.";
#endif
        break;
    case TRC_PadInvalidType:
        message_text = "The controller in controller port 1 is an unknown type. Connect a "
                       "standard controller to continue with the game.";
        break;
    case TRC_DiskNotIdentified:
        message_text = "Please wait. Checking Disc.";
        break;
    case TRC_DiskTrayOpen:
        message_text = "The Disc Cover is open. If you want to continue the game, please close "
                       "the Disc Cover.";
        break;
    case TRC_DiskNoDisk:
        message_text = "Please insert the Scooby-Doo Game Disc.";
        break;
    case TRC_DiskInvalid:
        message_text = "This is not the Scooby-Doo Game Disc. Please insert the Scooby-Doo "
                       "Game Disc.";
        break;
    case TRC_DiskRetry:
        message_text = "The Game Disc could not be read. Please read the Nintendo GameCube^ "
                       "Instruction Booklet for more information.";
        break;
    case TRC_DiskFatal:
        message_text = "An error has occurred. Turn the power off and refer to the Nintendo "
                       "GameCube^ Instruction Booklet for further instructions.";
        break;
    default:
        return 0;
    }

    if (globals.cmgr && globals.cmgr->csn->Time > 1.0f)
    {
        globals.cmgr->stop = 1;
    }
    iColor_tag clear = {};
    if (standalone || RwCameraGetCurrentCamera())
    {
        RwCamera* cam;
        if (standalone)
        {
            cam = iCameraCreate(640, 448, 0);
            RwRGBA bg = {};
            RwCameraClear(cam, &bg, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
            RwCameraBeginUpdate(cam);
        }
        render_message(message_text);
        if (standalone)
        {
            RwCameraEndUpdate(cam);
            RwCameraShowRaster(cam, NULL, 1);
            iCameraDestroy(cam);
        }
    }
    return 1;
}
#endif

void xTRCRender()
{
    if (message_text != NULL)
    {
        render_message(message_text);
    }
}

void xTRCReset()
{
    message_text = NULL;
    globals.dontShowPadMessageDuringLoadingOrCutScene = false;

    eGameMode mode = gGameMode;
    bool isStall = mode == eGameMode_Stall;

    if (isStall)
    {
        zGameModeSwitch(eGameMode_Game);
        xSndResume();
    }
}

void xTRCPad(S32 pad_id, _tagTRCState state)
{
#if defined(PS2)
    if (pad_id != 0)
    {
        return;
    }
    if (globals.cmgr && globals.cmgr->csn->Time > 1.0f)
    {
        globals.cmgr->stop = 1;
    }
    gTrcPad[pad_id].state = state;
    iColor_tag clear = {};
    S32 display_message = DisplayMessage(gTrcPad[pad_id].state);
    if (display_message)
    {
        if (!message_text)
        {
            message_text = "INSERT ANALOG CONTROLLER (DUALSHOCK_2) INTO CONTROLLER PORT 1";
        }
        zGamePauseIfPossible();
        zhud::show();
    }
    else
    {
        message_text = NULL;
        if (gGameMode == eGameMode_Save)
        {
            zhud::hide();
        }
        if (gGameMode == eGameMode_Stall)
        {
            const char* autoSaveFailed[] = {
                "MNU4 AUTO SAVE FAILED", "MNU4 AUTO SAVE CHANGED",
                "MNU4 AUTO SAVE FAILED UNFORMATTED", "MNU4 AUTO SAVE FAILED NOSPACE"
            };
            S32 i;
            for (i = 0; i < 4; ++i)
            {
                xEnt* ent = (xEnt*)zSceneFindObject(xStrHash(autoSaveFailed[i]));
                if (!xEntIsVisible(ent))
                {
                    continue;
                }
                return;
            }
            zGameModeSwitch(eGameMode_Game);
            xSndResume();
        }
    }
#endif
}

// SDA relocation shenanigans
void xTRCDisk(_tagTRCState state)
{
    if (state != TRC_DiskNotIdentified)
    {
        gTrcDisk[0] = state;
        gTrcDisk[1] = TRC_DiskIdentified;
    }
    else
    {
        gTrcDisk[1] = TRC_DiskNotIdentified;
    }
}

void render_mem_card_no_space(S32 needed, S32 available, S32 neededFiles, bool enabled)
{
    if (available < 0 && neededFiles != -1 && needed != -1)
    {
        available = 0;
    }

    bad_card_needed = needed;
    bad_card_available = available;

    char* error_text = "{i:text_mem_card_no_space}";
#if !defined(PS2)
    if (neededFiles == 0 && needed > available)
    {
        error_text = "{i:text_mem_card_no_space_overwrite}";
    }
    else if ((neededFiles > 0 && needed > available) || neededFiles == -1 || needed > available)
    {
        error_text = "{i:text_mem_card_no_space_no_save}";
    }
#endif

    RenderText(error_text, enabled);
}

void RenderText(const char* text, bool enabled)
{
    static xtextbox tb =
        xtextbox::create(xfont::create(1, NSCREENX(19.0f), NSCREENY(22.0f), 0.0f,
                                       xColorFromRGBA(0xFF, 0xE6, 0x00, 0xFF), screen_bounds),
                         screen_bounds, 0x2, 0.0f, 0.0f, 0.0f, 0.0f);
    
    tb.set_text(enabled ? text : "");
    tb.bounds = screen_bounds;
    tb.bounds.contract(0.1f);
    tb.bounds.h = tb.yextent(true);
    tb.bounds.y = -(0.5f * tb.bounds.h - 0.5f);        
    tb.render(true);

    if (!enabled)
    {
        render_fill_rect(tb.font.clip, xColorFromRGBA(0, 0, 0, 0x96));
    }
}
