#include "iTime.h"

#include <types.h>
#include <string.h>
#include <eekernel.h>
#include <eeregs.h>

struct prof_info
{
    U32 addr;
    U32 len;
    U32 count;
};

static S32 tim0_handler_id;
volatile iTime tim0_high;
prof_info* pip;
U32 pil;
U32 app_hits;
U32 kernel_hits;
U32 vsync_hits;
U32 scene_id = 'NONE';
U32 prof_enable;
static U32* profSampleBuffer;
static U32* profSampleStatic;
static U32 profSampleSize;
static F32 sGameTime;

static S32 TimerHandler(S32 ca, void*, void*);

void iTimeInit()
{
    tim0_handler_id = AddIntcHandler2(INTC_TIM0, TimerHandler, 0, NULL);
    tim0_high = 0;
    *T0_COUNT = 0;
    *T0_COMP = 0;
    *T0_HOLD = 0;
    *T0_MODE = T_MODE_OVFE_M | T_MODE_CUE_M | 1;
    EnableIntc(INTC_TIM0);
}

void iTimeExit()
{
    DisableIntc(INTC_TIM0);
    RemoveIntcHandler(INTC_TIM0, tim0_handler_id);
}

static S32 TimerHandler(S32 ca, void*, void*)
{
    if (ca == INTC_TIM0 && (*T0_MODE & T_MODE_OVFF_M))
    {
        *T0_MODE |= T_MODE_OVFF_M;
        tim0_high += 0x10000;
    }

    ExitHandler();
    return 0;
}

iTime iTimeGet()
{
    iTime high0, low0, high1, low1;

    high0 = tim0_high;
    low0 = *T0_COUNT;
    high1 = tim0_high;
    low1 = *T0_COUNT;

    if (high0 == high1)
    {
        return high0 | (low0 & 0xFFFF);
    }

    return high1 | (low1 & 0xFFFF);
}

F32 iTimeDiffSec(iTime time)
{
    return (F32)(U32)time * (1.0f / 9216000.0f);
}

F32 iTimeDiffSec(iTime t0, iTime t1)
{
    return iTimeDiffSec(t1 - t0);
}

void iTimeGameAdvance(F32 elapsed)
{
    sGameTime += elapsed;
}

void iTimeSetGame(F32 time)
{
    sGameTime = time;
}

void iProfileClear(U32 sceneID)
{
    S32 i;

    profSampleBuffer = NULL;
    prof_enable = 0;

    if (profSampleStatic)
    {
        memset(profSampleStatic, 0, profSampleSize);
    }

    for (i = 0; i < pil; i++)
    {
        pip[i].count = 0;
    }

    if (pil && sceneID)
    {
        scene_id = sceneID;
    }

    prof_enable = 1;
    app_hits = 0;
    kernel_hits = 0;
    vsync_hits = 0;
    profSampleBuffer = profSampleStatic;
}

void iFuncProfileParse(char* elfPath, S32 profile)
{
}

void iFuncProfileDump()
{
}
