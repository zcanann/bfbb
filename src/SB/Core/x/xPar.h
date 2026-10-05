#ifndef XPAR_H
#define XPAR_H

#if defined(XBOX)
#include "xVec3.h"
#else
#include "xMath3.h"
#endif

struct xParEmitterAsset;
struct xParEmitter;

struct xPar
{
    xPar* m_next; // 0x0
    xPar* m_prev; // 0x4
    F32 m_lifetime; // 0x8
    U8 m_c[4]; // 0xC
    xVec3 m_pos; // 0x10
    F32 m_size; // 0x1C
    xVec3 m_vel; // 0x20
    F32 m_sizeVel; // 0x2C
    U8 m_flag; // 0x30
    U8 m_mode; // 0x31
    U8 m_texIdx[2]; // 0x32
    U8 m_rotdeg[3]; // 0x34
    U8 pad8; // 0x37
    F32 totalLifespan; //0x38
    xParEmitterAsset* m_asset; // 0x3C
    F32 m_cvel[4]; // 0x40
    F32 m_cfl[4]; // 0x50
};

void xParMemInit();
xPar* xParAlloc();
#if defined(XBOX)
extern xPar* gParDead;
inline void xParFree(xPar* par)
{
    if (par->m_next != NULL)
    {
        par->m_next->m_prev = par->m_prev;
    }
    if (par->m_prev != NULL)
    {
        par->m_prev->m_next = par->m_next;
    }
    xPar* dead = gParDead;
    if (dead != NULL)
    {
        dead->m_prev = par;
    }
    par->m_next = gParDead;
    par->m_prev = NULL;
    gParDead = par;
}
#else
void xParFree(xPar* par);
#endif
void xParInit(xPar* p);

#endif
