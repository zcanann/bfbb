#ifndef ZSCENELOOKUP_H
#define ZSCENELOOKUP_H

#include <types.h>

struct xBase;

xBase* zSceneFindObject(U32 gameID);
const char* zSceneGetName(U32 gameID);
const char* zSceneGetName(xBase* b);

#endif
