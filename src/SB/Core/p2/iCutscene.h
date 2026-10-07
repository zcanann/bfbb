#ifndef PS2_ICUTSCENE_H
#define PS2_ICUTSCENE_H

#include "xCutscene.h"

// External PS2 cutscene file interface (SB/Core/p2/iCutscene.cpp DWARF).
unsigned int iCSFileOpen(xCutscene* csn);
void iCSFileAsyncRead(xCutscene* csn, void* dest, unsigned int size);
void iCSFileClose(xCutscene* csn);
signed int iCSLoadStep(xCutscene* csn);

#endif
