#ifndef XPARGROUP_H
#define XPARGROUP_H

#include "xPar.h"

struct xParCmdTex;

struct xParGroup
{
    xPar* m_root;
    xPar* m_dead;
    S32 m_num_of_particles;
    U8 m_alive;
    U8 m_killWhenDead;
    U8 m_active;
    U8 m_visible;
    U8 m_culled;
    U8 m_priority;
    U8 m_flags;
    U8 m_regidx;
    xParGroup* m_next;
    xParGroup* m_prev;
    void (*draw)(void*, xParGroup*);
    xParCmdTex* m_cmdTex;
};

#define XPARGROUP_UNK1 0x1
#define XPARGROUP_ALLOCPARS 0x2
#define XPARGROUP_NOAGING 0x4
#define XPARGROUP_NOBACK2LIFE 0x8
#define XPARGROUP_UNK10 0x10

void xParGroupInit(xParGroup* ps);
void xParGroupSetAging(xParGroup* ps, S32 age);
void xParGroupSetBack2Life(xParGroup* ps, S32 b2l);
void xParGroupSetVisibility(xParGroup* ps, S32 vis);
void xParGroupSetPriority(xParGroup* ps, U8 val);
void xParGroupRegister(xParGroup* ps);
void xParGroupUnregister(xParGroup* ps);
void xParGroupSetActive(xParGroup* ps, U32 isActive);
void xParGroupKillAllParticles(xParGroup* ps);
void xParGroupAnimate(xParGroup* ps, F32 dt);
#if defined(XBOX)
inline void xParGroupAddParP(xParGroup* ps, xPar* p)
{
    ps->m_alive = 1;
    ps->m_num_of_particles++;

    p->m_prev = NULL;
    p->m_next = NULL;

    if (ps->m_root)
    {
        p->m_next = ps->m_root;
        ps->m_root->m_prev = p;
        ps->m_root = p;
    }
    else
    {
        ps->m_root = p;
    }
}
#else
void xParGroupAddParP(xParGroup* ps, xPar* p);
#endif
xPar* xParGroupAddPar(xParGroup* ps);
void xParGroupKillPar(xParGroup* ps, xPar* p);
#if defined(XBOX)
inline void xParGroupAddParToDeadList(xParGroup* ps, xPar* p)
{
    if (ps->m_dead)
    {
        ps->m_dead->m_prev = p;
    }

    p->m_next = ps->m_dead;
    p->m_prev = NULL;

    ps->m_dead = p;
}
#else
void xParGroupAddParToDeadList(xParGroup* ps, xPar* p);
#endif

#endif
