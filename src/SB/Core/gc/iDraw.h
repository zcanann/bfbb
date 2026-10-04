#ifndef IDRAW_H
#define IDRAW_H

#include <types.h>

void iDrawSetFBMSK(U32 abgr);
#if defined(VERSION_GQPP78) || defined(VERSION_GU4Y78)
void iDrawSetDisplayOffset(F32 offsetx, F32 offsety);
#endif
void iDrawBegin();
void iDrawEnd();

#endif
