#ifndef XSCENELOOKUP_H
#define XSCENELOOKUP_H

#include <types.h>

struct xBase;
struct xScene;

xBase* xSceneResolvID(xScene* sc, U32 id);
const char* xSceneID2Name(xScene* sc, U32 id);

#endif
