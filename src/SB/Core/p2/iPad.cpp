#include "iPad.h"

#include "xPad.h"
#include "xRumble.h"
#include "xTRC.h"

#include "iSystem.h"

#include <libpad.h>
#include <string.h>
#include <types.h>

// xPad::flags
#define IPAD_FLAG_ANALOG1 0x1
#define IPAD_FLAG_ANALOG2 0x2
#define IPAD_FLAG_RUMBLE 0x4

static U8 mReadData[32];
static __int128 mPadDmaBuffer[4][16];
static U8 mActDirect[6];
static U8 mActAlign[6];
static F32 mLargeMotor[4];

S32 iPadInit()
{
    iLoadModule("sio2man.irx", NULL);
    iLoadModule("padman.irx", "thpri=20,30");
    scePadInit(0);
    return 1;
}

_tagxPad* iPadEnableGuts(_tagxPad* pad, _tagPadInit* pad_init)
{
    S32 state;
    S32 id;
    S32 exid;

    switch (*pad_init)
    {
    case ePadInit_Open1:
        scePadPortClose(pad->port, pad->slot);
        scePadPortOpen(pad->port, pad->slot, mPadDmaBuffer[pad->port]);
        *pad_init = ePadInit_WaitStable2;
        break;
    case ePadInit_WaitStable2:
        state = scePadGetState(pad->port, pad->slot);
        if (state == scePadStateDiscon)
        {
            pad->state = ePad_Missing;
            return pad;
        }

        if (state == scePadStateStable)
        {
            if (pad->state == ePad_Missing)
            {
                *pad_init = ePadInit_Open1;
                pad->state = ePad_Enabled;
            }
            else
            {
                *pad_init = ePadInit_EnableAnalog3;
            }
        }
        else if (state == scePadStateFindCTP1)
        {
            pad->state = ePad_Missing;
        }
        break;
    case ePadInit_EnableAnalog3:
        id = scePadInfoMode(pad->port, pad->slot, InfoModeCurID, 0);
        exid = scePadInfoMode(pad->port, pad->slot, InfoModeCurExID, 0);
        if (exid > 0)
        {
            id = exid;
        }

        if (id == 4)
        {
            // Digital controller: switch it to analog mode if it can.
            if (!scePadInfoMode(pad->port, pad->slot, InfoModeCurExID, 0))
            {
                *pad_init = ePadInit_Complete8a;
                pad->flags &= ~(IPAD_FLAG_ANALOG1 | IPAD_FLAG_ANALOG2);
            }
            else
            {
                scePadSetMainMode(pad->port, pad->slot, 1, 3);
                for (;;)
                {
                    if (scePadGetReqState(pad->port, pad->slot) == scePadReqStateBusy)
                    {
                        break;
                    }
                }
                scePadSetMainMode(pad->port, pad->slot, 1, 3);
                *pad_init = ePadInit_EnableAnalog3LetsAllPissOffChris;
            }
        }
        else if (id == 7)
        {
            // DualShock 2.
            *pad_init = ePadInit_EnableRumble4;
            pad->flags |= IPAD_FLAG_ANALOG1 | IPAD_FLAG_ANALOG2;
        }
        else
        {
            *pad_init = ePadInit_Open1;
        }
        break;
    case ePadInit_EnableAnalog3LetsAllPissOffChris:
        state = scePadGetReqState(pad->port, pad->slot);
        if (state == scePadReqStateBusy)
        {
            break;
        }

        if (state == scePadReqStateComplete)
        {
            *pad_init = ePadInit_EnableRumble4;
            pad->flags |= IPAD_FLAG_ANALOG1 | IPAD_FLAG_ANALOG2;
        }
        else
        {
            *pad_init = ePadInit_Open1;
        }
        break;
    case ePadInit_EnableRumble4:
        if (!scePadInfoAct(pad->port, pad->slot, -1, 0))
        {
            if (pad->port == 0 && pad->slot == 0)
            {
                *pad_init = ePadInit_Open1;
            }
            else
            {
                *pad_init = ePadInit_Complete8a;
            }
        }
        else
        {
            mActAlign[0] = 0;
            mActAlign[1] = 1;
            mActAlign[5] = 0xFF;
            mActAlign[4] = 0xFF;
            mActAlign[3] = 0xFF;
            mActAlign[2] = 0xFF;

            if (scePadSetActAlign(pad->port, pad->slot, mActAlign))
            {
                *pad_init = ePadInit_EnableRumbleTest5;
            }
            else if (pad->port == 0 && pad->slot == 0)
            {
                *pad_init = ePadInit_Open1;
            }
            else
            {
                *pad_init = ePadInit_Complete8a;
            }
        }
        break;
    case ePadInit_EnableRumbleTest5:
        state = scePadGetReqState(pad->port, pad->slot);
        if (state == scePadReqStateFaild)
        {
            *pad_init = ePadInit_Complete8a;
        }
        else if (state == scePadReqStateComplete)
        {
            pad->flags |= IPAD_FLAG_RUMBLE;
            *pad_init = ePadInit_Complete8a;
        }
        break;
    case ePadInit_Complete8a:
        pad->state = ePad_Enabled;
        *pad_init = ePadInit_Finished9;
        xTRCPad(pad->port, TRC_PadInserted);
        break;
    case ePadInit_Complete8b:
        break;
    }

    return pad;
}

_tagxPad* iPadEnable(_tagxPad* pad, S16 port)
{
    _tagPadInit pad_init = ePadInit_Open1;

    pad->flags = 0;
    pad->port = port;
    pad->slot = 0;
    pad->state = ePad_Disabled;

    do
    {
        iPadEnableGuts(pad, &pad_init);
    } while (pad_init != ePadInit_Finished9 && pad->state != ePad_Missing);

    return pad;
}

static void iPadTRCUpdate(_tagxPad* pad)
{
    switch (gTrcPad[pad->port].state)
    {
    case TRC_PadInserted:
        break;
    case TRC_PadMissing:
        iPadEnableGuts(pad, &gTrcPad[pad->port].pad_init);
        if (gTrcPad[pad->port].state == TRC_PadInserted)
        {
            gTrcPad[pad->port].state = TRC_PadInserted;
        }
        break;
    case TRC_PadInvalidNoAnalog:
        iPadEnableGuts(pad, &gTrcPad[pad->port].pad_init);
        if (gTrcPad[pad->port].state == TRC_PadInserted)
        {
            gTrcPad[pad->port].state = TRC_PadInserted;
        }
        break;
    case TRC_PadInvalidType:
        iPadEnableGuts(pad, &gTrcPad[pad->port].pad_init);
        break;
    }

    pad->on = 0;
    pad->pressed = 0;
    pad->released = 0;
    pad->analog1.x = 0;
    pad->analog1.y = 0;
    pad->analog2.x = 0;
    pad->analog2.y = 0;
}

S32 iPadUpdate(_tagxPad* pad, U32* on)
{
    S32 temp;
    U32 temp_on;
    S32 result;

    if (pad->port == 0 && gTrcPad[pad->port].state != TRC_PadInserted)
    {
        iPadTRCUpdate(pad);
        return 1;
    }

    if (pad->state != ePad_Enabled)
    {
        return 0;
    }

    memcpy(pad->last_value, pad->value, sizeof(pad->value));

    if (scePadRead(pad->port, pad->slot, mReadData) > 0)
    {
        if (pad->flags & IPAD_FLAG_ANALOG1)
        {
            pad->analog1.x = mReadData[6] - 0x80;
            pad->analog1.y = mReadData[7] - 0x80;
        }
        else
        {
            pad->analog1.x = 0;
            pad->analog1.y = 0;
        }

        if (pad->flags & IPAD_FLAG_ANALOG2)
        {
            pad->analog2.x = mReadData[4] - 0x80;
            pad->analog2.y = mReadData[5] - 0x80;
        }
        else
        {
            pad->analog2.x = 0;
            pad->analog2.y = 0;
        }

        // Pressure-sensitive button values.
        pad->value[0] = 0;
        pad->value[1] = 0;
        pad->value[2] = 0;
        pad->value[3] = 0;
        pad->value[4] = mReadData[10];
        pad->value[5] = mReadData[8];
        pad->value[6] = mReadData[11];
        pad->value[7] = mReadData[9];
        pad->value[8] = mReadData[16];
        pad->value[9] = mReadData[18];
        pad->value[10] = 0;
        pad->value[11] = 0;
        pad->value[12] = mReadData[17];
        pad->value[13] = mReadData[19];
        pad->value[14] = 0;
        pad->value[15] = 0;
        pad->value[16] = mReadData[14];
        pad->value[17] = mReadData[13];
        pad->value[18] = mReadData[12];
        pad->value[19] = mReadData[15];
        pad->value[20] = 0;
        pad->value[21] = 0;

        // Digital buttons are active low.
        mReadData[2] ^= 0xFF;
        mReadData[3] ^= 0xFF;

        temp_on = 0;
        temp = mReadData[2];
        temp_on |= ((temp >> 7) & 1) << 7;
        temp_on |= ((temp >> 6) & 1) << 6;
        temp_on |= ((temp >> 5) & 1) << 5;
        temp_on |= ((temp >> 4) & 1) << 4;
        temp_on |= ((temp >> 3) & 1) << 0;
        temp_on |= ((temp >> 2) & 1) << 14;
        temp_on |= ((temp >> 1) & 1) << 10;
        temp_on |= ((temp >> 0) & 1) << 1;
        temp = mReadData[3];
        temp_on |= ((temp >> 7) & 1) << 19;
        temp_on |= ((temp >> 6) & 1) << 16;
        temp_on |= ((temp >> 5) & 1) << 17;
        temp_on |= ((temp >> 4) & 1) << 18;
        temp_on |= ((temp >> 3) & 1) << 12;
        temp_on |= ((temp >> 2) & 1) << 8;
        temp_on |= ((temp >> 1) & 1) << 13;
        temp_on |= ((temp >> 0) & 1) << 9;

        *on = temp_on;
    }
    else
    {
        pad->on = 0;
        pad->pressed = 0;
        pad->released = 0;
        pad->analog1.x = 0;
        pad->analog1.y = 0;
        pad->analog2.x = 0;
        pad->analog2.y = 0;
        gTrcPad[pad->port].pad_init = ePadInit_Open1;
        xTRCPad(pad->port, TRC_PadMissing);
    }

    return 1;
}

void iPadRumbleFx(_tagxPad* p, _tagxRumble* r, F32 time_passed)
{
    F32 act;
    F32 scale;

    if (r->fxflags != 0 && (r->fxflags & 0x1) != 0)
    {
        // Ramp the large motor down over the rumble's duration.
        act = mLargeMotor[p->port];
        scale = act / r->seconds * time_passed;
        act -= scale;
        mLargeMotor[p->port] = act;
        mActDirect[1] = act;
        scePadSetActDirect(p->port, p->slot, mActDirect);
    }
}

void iPadStopRumble(_tagxPad* pad)
{
    mActDirect[0] = 0;
    mActDirect[1] = 0;
    scePadSetActDirect(pad->port, pad->slot, mActDirect);
}

void iPadStartRumble(_tagxPad* pad, _tagxRumble* rumble)
{
    // mActDirect[0] is the small (on/off) motor, mActDirect[1] the large one.
    mActDirect[0] = 0;

    switch (rumble->type)
    {
    case eRumble_Off:
        mActDirect[0] = 0;
        mActDirect[1] = 0;
        break;
    case eRumble_Hi:
        mActDirect[0] = 1;
        mActDirect[1] = 0;
        break;
    case eRumble_VeryLightHi:
        mActDirect[0] = 1;
    case eRumble_VeryLight:
        mActDirect[1] = 100;
        break;
    case eRumble_LightHi:
        mActDirect[0] = 1;
    case eRumble_Light:
        mActDirect[1] = 126;
        break;
    case eRumble_MediumHi:
        mActDirect[0] = 1;
    case eRumble_Medium:
        mActDirect[1] = 169;
        break;
    case eRumble_HeavyHi:
        mActDirect[0] = 1;
    case eRumble_Heavy:
        mActDirect[1] = 212;
        break;
    case eRumble_VeryHeavyHi:
        mActDirect[0] = 1;
    case eRumble_VeryHeavy:
        mActDirect[1] = 255;
        break;
    }

    mLargeMotor[pad->port] = mActDirect[1];
    scePadSetActDirect(pad->port, pad->slot, mActDirect);
}

void iPadKill()
{
}
