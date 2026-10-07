#ifndef PS2_IPARMGR_H
#define PS2_IPARMGR_H

#include <types.h>
#include "xMath3.h"
#include <rwcore.h>

// Original PS2 render arrays shared by the immediate-mode consumers.
struct tagiRenderArrays
{
    U16 m_index[960];
    RxObjSpace3DVertex m_vertex[480];
    F32 m_vertexTZ[480];
};

struct tagiRenderInput
{
    // total size: 0x80
    U16* m_index; // offset 0x0, size 0x4
    RxObjSpace3DVertex* m_vertex; // offset 0x4, size 0x4
    F32* m_vertexTZ; // offset 0x8, size 0x4
    U32 m_mode; // offset 0xC, size 0x4
    S32 m_vertexType; // offset 0x10, size 0x4
    S32 m_vertexTypeSize; // offset 0x14, size 0x4
    S32 m_indexCount; // offset 0x18, size 0x4
    S32 m_vertexCount; // offset 0x1C, size 0x4
    xMat4x3 m_camViewMatrix; // offset 0x20, size 0x40
    xVec4 m_camViewR; // offset 0x60, size 0x10
    xVec4 m_camViewU; // offset 0x70, size 0x10
};

extern tagiRenderArrays gRenderArr;
extern tagiRenderInput gRenderBuffer;

void iParMgrInit();
void iParMgrUpdate(F32 elapsedTime);
void iParMgrRender();

struct xParGroup;
void iParMgrRenderParSys_Streak(void* data, xParGroup* ps);
void iParMgrRenderParSys_QuadStreak(void* data, xParGroup* ps);
void iParMgrRenderParSys_InvStreak(void* data, xParGroup* ps);
void iParMgrRenderParSys_Flat(void* data, xParGroup* ps);
void iParMgrRenderParSys_Static(void* data, xParGroup* ps);
void iParMgrRenderParSys_Ground(void* data, xParGroup* ps);
void iParMgrRenderParSys_Sprite(void* data, xParGroup* ps);

#endif
