#ifndef XCOLLIDEFAST_H
#define XCOLLIDEFAST_H

#include "xMath3.h"
#include "xRay3.h"
#if defined(PS2)
struct xScene;
#else
#include "xScene.h"
#endif

void xCollideFastInit(xScene* sc);
U32 xRayHitsSphereFast(const xRay3* r, const xSphere* s);
U32 xRayHitsBoxFast(const xRay3* r, const xBox* b);

#endif
