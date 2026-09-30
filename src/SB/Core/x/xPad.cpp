#include "xPad.h"

#include "xMathInlines.h"

#include "zMenu.h"
#include "zScene.h"
#include "zGame.h"
#include "zGameExtras.h"
#include "zGlobals.h"

#include <types.h>
#include <string.h>

_tagxPad mPad[4];
// The retail object carries mRumbleList as a file-local .bss symbol and this
// is the only translation unit that touches it, so it is static here.
static _tagxRumble mRumbleList[32];
_tagxPad* gDebugPad;
_tagxPad* gPlayerPad;

// NOTE: xPad.h declares this as xPadAnalogIsDigital(F32, F32), which does not
// match the definition below and resolves the calls in xPadUpdate to a symbol
// that does not exist. Declare the real signature here.
void xPadAnalogIsDigital(S32 idx, S32 enable);

S32 xPadInit()
{
    memset(mPad, 0, sizeof(mPad));
    memset(mRumbleList, 0, sizeof(mRumbleList));
    S32 code = iPadInit();
    if (!code)
    {
        return 0;
    }
    gPlayerPad = mPad;
    return 1;
}

_tagxPad* xPadEnable(S32 idx)
{
    _tagxPad* p = mPad + idx;
    if (p->state != ePad_Disabled)
    {
        return p;
    }
    if (idx != 0)
    {
        return p;
    }
    p = iPadEnable(p, idx);
    xPadRumbleEnable(idx, 1);
    return p;
}

void xPadRumbleEnable(S32 idx, S32 enable)
{
    _tagxPad* p = mPad + idx;
    if (p->state != ePad_Enabled)
    {
        enable = 0;
    }
    if (enable)
    {
        if (p->flags & 4)
        {
            p->flags |= 8;
        }
    }
    else
    {
        if (p->flags & 8)
        {
            p->flags ^= 8;
            xPadDestroyRumbleChain(mPad + idx);
        }
    }
}

S32 xPadUpdate(S32 idx, F32 time_passed)
{
    if (idx != 0)
    {
        return 0;
    }

    // Not initialized: iPadUpdate() below fills it in before any read.
    U32 new_on;

    if (zScene_ScreenAdjustMode())
    {
        xPadAnalogIsDigital(idx, 0);
    }
    else if (zMenuRunning() || zGameIsPaused() || zGame_HackIsGallery())
    {
        xPadAnalogIsDigital(idx, 1);
    }
    else
    {
        xPadAnalogIsDigital(idx, 0);
    }

    _tagxPad* p = &mPad[idx];

    if ((p->flags & 4) && (p->flags & 8))
    {
        _tagxRumble* r = p->rumble_head.next;
        while (r != NULL)
        {
            r->seconds -= time_passed;
            if (r->seconds <= 0.0f)
            {
                r->active = 0;
                if (r->next == NULL)
                {
                    p->rumble_head.next = NULL;
                    r = NULL;
                    iPadStopRumble(p);
                }
                else
                {
                    p->rumble_head.next = r->next;
                    r = p->rumble_head.next;
                    iPadStartRumble(p, r);
                }
            }
            else
            {
                iPadRumbleFx(p, r, time_passed);
                break;
            }
        }
    }

    S32 ret = iPadUpdate(p, &new_on);
    if (!ret)
    {
        return 1;
    }

    if (p->flags & 0x10)
    {
        if (p->flags & 0x1)
        {
            U32 fake_dpad = 0;
            if (p->analog1.x >= 50)
            {
                fake_dpad |= 0x20;
            }
            else if (p->analog1.x <= -50)
            {
                fake_dpad |= 0x80;
            }
            if (p->analog1.y >= 50)
            {
                fake_dpad |= 0x40;
            }
            else if (p->analog1.y <= -50)
            {
                fake_dpad |= 0x10;
            }
            if (fake_dpad == 0)
            {
                p->al2d_timer = 0.0f;
            }
            else
            {
                p->al2d_timer -= time_passed;
                if (p->al2d_timer <= 0.0f)
                {
                    new_on |= fake_dpad;
                    p->al2d_timer = 0.35f;
                }
            }
        }

        if (p->flags & 0x2)
        {
            if (p->analog2.x > -50 && p->analog2.x < 50 && p->analog2.y > -50 &&
                p->analog2.y < 50)
            {
                p->ar2d_timer = 0.0f;
            }
            else
            {
                p->ar2d_timer -= time_passed;
                if (p->ar2d_timer <= 0.0f)
                {
                    p->ar2d_timer = 0.35f;
                    if (p->analog2.x >= 50)
                    {
                        new_on |= 0x20;
                    }
                    else if (p->analog2.x <= -50)
                    {
                        new_on |= 0x80;
                    }
                    if (p->analog2.y >= 50)
                    {
                        new_on |= 0x40;
                    }
                    else if (p->analog2.y <= -50)
                    {
                        new_on |= 0x10;
                    }
                }
            }
        }
    }
#ifdef DEBUGRELEASE
    static S32 submap = 0;
    if (submap)
    {
        SubMapAll((S32*)&new_on, p);
    }
#endif

    p->pressed = new_on & ~p->on;
    p->released = p->on & ~new_on;
    p->on = new_on;

    for (S32 i = 0; i < 22; i++)
    {
        if (p->pressed & (1 << i))
        {
            p->down_tmr[i] = 0.0f;
        }
        else if (p->released & (1 << i))
        {
            p->up_tmr[i] = 0.0f;
        }
        if (p->on & (1 << i))
        {
            p->down_tmr[i] += time_passed;
        }
        else
        {
            p->up_tmr[i] += time_passed;
        }
    }

    if (p->flags & 0x10)
    {
        if (!(p->on & 0x10) && !(p->on & 0x40) && !(p->on & 0x80) && !(p->on & 0x20))
        {
            p->d_timer = 0.0f;
        }
        else
        {
            p->d_timer -= time_passed;
            if (p->d_timer <= 0.0f)
            {
                p->d_timer = 0.35f;
                if (p->on & 0x10)
                {
                    p->pressed |= 0x10;
                }
                else if (p->on & 0x40)
                {
                    p->pressed |= 0x40;
                }
                if (p->on & 0x80)
                {
                    p->pressed |= 0x80;
                }
                else if (p->on & 0x20)
                {
                    p->pressed |= 0x20;
                }
            }
        }
    }

    return 1;
}

void xPadNormalizeAnalog(_tagxPad& pad, S32 inner_zone, S32 outer_zone)
{
    const _tagPadAnalog* src = &pad.analog1;
    for (S32 i = 0; i < 2; i++)
    {
        pad.analog[i].offset.x =
            normalize_analog(src[i].x, -outer_zone, outer_zone, 0, -inner_zone, inner_zone);
        pad.analog[i].offset.y =
            normalize_analog(src[i].y, -outer_zone, outer_zone, 0, -inner_zone, inner_zone);
        if (pad.analog[i].offset.x == 0.0f && pad.analog[i].offset.y == 0.0f)
        {
            pad.analog[i].mag = 0.0f;
            pad.analog[i].dir.assign(1.0f, 0.0f);
            pad.analog[i].ang = 0.0f;
        }
        else
        {
            pad.analog[i].mag = pad.analog[i].offset.length();
            pad.analog[i].dir = pad.analog[i].offset;
            pad.analog[i].dir /= pad.analog[i].mag;
            pad.analog[i].ang = xatan2(pad.analog[i].dir.y, pad.analog[i].dir.x);
        }
    }
}

void xPadKill()
{
    iPadKill();
}

_tagxRumble* xPadGetRumbleSlot()
{
    for (S32 i = 0; i < 32; i++)
    {
        if (mRumbleList[i].active == 0)
        {
            memset(&mRumbleList[i], 0, sizeof(_tagxRumble));
            return &mRumbleList[i];
        }
    }
    return NULL;
}

void xPadDestroyRumbleChain(_tagxPad* pad)
{
    iPadStopRumble(pad);
    _tagxRumble* curr = pad->rumble_head.next;
    while (curr != NULL)
    {
        // Retail bug: memset() clears curr->next before it is read, so the
        // chain walk always stops after the first node and the rest of the
        // chain is leaked. Reproduced faithfully.
        memset(curr, 0, sizeof(_tagxRumble));
        curr = curr->next;
    }
    pad->rumble_head.next = NULL;
}

void xPadDestroyRumbleChain(S32 idx)
{
    xPadDestroyRumbleChain(mPad + idx);
}

S32 xPadAddRumble(S32 idx, _tagRumbleType type, F32 time, S32 replace, U32 fxflags)
{
    S32 appended;
    _tagxPad* pad;
    _tagxRumble* r;
    _tagxRumble* last_r;

    pad = mPad + idx;
    if (!(pad->flags & 4))
    {
        return 0;
    }
    if (!(pad->flags & 8))
    {
        return 0;
    }

    appended = 1;
    if (replace != 0)
    {
        xPadDestroyRumbleChain(pad);
        appended = 0;
    }

    r = &pad->rumble_head;
    last_r = r;
    if (pad->rumble_head.next == NULL)
    {
        appended = 0;
    }

    while (r != NULL)
    {
        last_r = r;
        r = r->next;
    }

    r = xPadGetRumbleSlot();
    if (r == NULL)
    {
        return 0;
    }

    r->type = type;
    r->seconds = time;
    r->active = 1;
    r->fxflags = (U16)fxflags;
    last_r->next = r;
    last_r->next->next = NULL;
    if (appended == 0)
    {
        iPadStartRumble(pad, r);
    }
    return 1;
}

void xPadAnalogIsDigital(S32 idx, S32 enable)
{
    if (idx != 0)
        return;

    _tagxPad* pad = &mPad[idx];

    if (enable)
    {
        pad->flags |= 0x10;
    }
    else
    {
        pad->flags &= ~0x10;
    }

    if (pad->al2d_timer >= 0.35f)
    {
        pad->al2d_timer = 0.35f;
    }

    if (pad->ar2d_timer >= 0.35f)
    {
        pad->ar2d_timer = 0.35f;
    }

    if (pad->d_timer >= 0.35f)
    {
        pad->d_timer = 0.35f;
    }
}

inline F32 xVec2::length() const
{
    return xsqrt(length2());
}

inline F32 normalize_analog(S32 v, S32 v_min, S32 v_max, S32 dead_center, S32 dead_min,
                            S32 dead_max)
{
    if (v <= dead_center)
    {
        if (v >= dead_min)
        {
            return 0.0f;
        }
        if (v <= v_min)
        {
            return -1.0f;
        }
        return (F32)(v - dead_min) / (F32)(dead_min - v_min);
    }
    else
    {
        if (v <= dead_max)
        {
            return 0.0f;
        }
        if (v >= v_max)
        {
            return 1.0f;
        }
        return (F32)(v - dead_max) / (F32)(v_max - dead_max);
    }
}
