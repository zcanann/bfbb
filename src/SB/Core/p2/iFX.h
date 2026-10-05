#ifndef IFX_H
#define IFX_H

#include <rwcore.h>

struct xVec3;

RxPipeline* iFXanimUVCreatePipe();
extern RxPipeline* xFXgooPipeline;
void iFXgooSetParams(xVec3* center, unsigned int state, float warb_time, float alpha,
                     float min, float max, float* warbc);

#endif
