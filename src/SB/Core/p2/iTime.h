#ifndef ITIME_H
#define ITIME_H

#include <types.h>

// PS2 DWARF declares iTimeGet/iTimeDiffSec using signed long (32 bits).
typedef signed long iTime;

void iTimeInit();
void iTimeExit();
iTime iTimeGet();
F32 iTimeDiffSec(iTime t0, iTime t1);
F32 iTimeDiffSec(iTime time);
void iTimeGameAdvance(F32 elapsed);
void iTimeSetGame(F32 time);
void iProfileClear(U32 sceneID);
void iFuncProfileDump();
void iFuncProfileParse(char* elfPath, S32 profile);

#endif
