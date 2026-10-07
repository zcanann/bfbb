#ifndef PS2_IFMV_H
#define PS2_IFMV_H

#include <types.h>

// PS2 movie playback (SB/Core/p2/iFMV.cpp); retail callers pass all five arguments.
U32 iFMVPlay(char* filename, U32 buttons, F32 time, bool skippable, bool lockController);

#endif
