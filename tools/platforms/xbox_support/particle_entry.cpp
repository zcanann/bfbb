// Host reachability context only; excluded from source-progress comparison.
#include "xPar.h"
extern "C" xPar* __cdecl xbox_source_entry(xPar* particle)
{
    xParMemInit();
    xParInit(particle);
    xParFree(particle);
    return xParAlloc();
}
