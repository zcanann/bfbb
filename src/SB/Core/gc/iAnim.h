#ifndef IANIM_H
#define IANIM_H

#include "xMath3.h"

// giAnimScratch holds five poses. A pose is IANIM_MAXBONES quats followed by
// IANIM_MAXBONES translations, the translation array padded to 16 bytes:
// 0x410 + 0x310 = IANIM_POSE_SIZE.
#define IANIM_MAXBONES 65
#define IANIM_POSE_SIZE 0x720

extern U8* giAnimScratch;

void iAnimInit();
F32 iAnimDuration(void* RawData);
U32 iAnimBoneCount(void* RawData);
void iAnimBlend(F32 BlendFactor, F32 BlendRecip, U16* BlendTimeOffset, F32* BoneTable,
                U32 BoneCount, xVec3* Tran1, xQuat* Quat1, xVec3* Tran2, xQuat* Quat2,
                xVec3* TranDest, xQuat* QuatDest);
void iAnimEval(void* RawData, float time, unsigned int flags, class xVec3* tran, class xQuat* quat);

#endif
