// Diagnostic reachability context only; excluded from Xbox source coverage.
#include "xParGroup.h"

extern "C" xPar* __cdecl xbox_source_entry(xParGroup* group, xPar* particle, float dt,
                                           int value)
{
    xParMemInit();
    xParGroupInit(group);
    xParGroupSetAging(group, value);
    xParGroupSetBack2Life(group, value);
    xParGroupSetVisibility(group, value);
    xParGroupSetPriority(group, (U8)value);
    xParGroupRegister(group);
    xParGroupUnregister(group);
    xParGroupSetActive(group, value);
    xParGroupKillAllParticles(group);
    xParGroupAnimate(group, dt);
    xParGroupAddParP(group, particle);
    xParGroupKillPar(group, particle);
    xParGroupAddParToDeadList(group, particle);
    return xParGroupAddPar(group);
}
