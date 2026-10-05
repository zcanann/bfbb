#ifndef PS2_IPAD_H
#define PS2_IPAD_H

#include <types.h>

struct _tagiPad
{
    S32 port;
};

struct _tagxPad;
struct _tagxRumble;

S32 iPadInit();
_tagxPad* iPadEnable(_tagxPad* pad, S16 port);
S32 iPadUpdate(_tagxPad* pad, U32* on);
void iPadRumbleFx(_tagxPad* p, _tagxRumble* r, F32 time_passed);
void iPadStopRumble(_tagxPad* pad);
void iPadStartRumble(_tagxPad* pad, _tagxRumble* rumble);
void iPadKill();

#endif
