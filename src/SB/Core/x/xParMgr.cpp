#include "iParMgr.h"

#include "xPar.h"
#include "xParMgr.h"

#include <types.h>

volatile static U32 sFrameCount;
static S32 sParMgrInit;

void xParMgrInit()
{
    if (sParMgrInit == 0)
    {
        xParMemInit();
        iParMgrInit();
        sParMgrInit = 1;
    }
    else
    {
        xParMgrKillAllParticles();
        iParMgrInit();
    }
}

#if defined(PS2)
inline
#endif
void xParMgrKillAllParticles()
{
}

void xParMgrUpdate(F32 elapsedTime)
{
    iParMgrUpdate(elapsedTime);

    sFrameCount++;

    if (sFrameCount > 10)
    {
        sFrameCount = 1;
    }

    U32 i;
#if defined(PS2)
    // Load the volatile counter once before counting down.
    i = sFrameCount;
    for (;;)
    {
        if (i == 0)
        {
            break;
        }
        i--;
    }
#else
    for (i = sFrameCount; i >= 1; i--);
#endif
    sFrameCount = i;
}

void xParMgrRender()
{
    iParMgrRender();
}
