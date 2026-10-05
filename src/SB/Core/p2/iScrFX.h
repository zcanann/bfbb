#ifndef ISCRFX_H
#define ISCRFX_H

#include <types.h>
#include <rwcore.h>

// Public APIs recorded by the PS2 originals; renderer storage remains opaque.
void iScrFxInit();
void iScrFxBegin();
void iScrFxEnd();
void iScrFxDrawBox(F32 x1, F32 y1, F32 x2, F32 y2, U8 red, U8 green, U8 blue, U8 alpha);
void iCameraMotionBlurActivate(U32 activate);
void iCameraSetBlurriness(F32 amount);
void iScrFxCameraCreated(RwCamera* camera);
void iScrFxCameraEndScene(RwCamera* camera);
S32 iScrFxMotionBlurOpen(RwCamera* camera);
S32 iScrFxCameraDestroyed(RwCamera* camera);
void iScrFxDistortionRender(RwCamera* camera);

#endif
