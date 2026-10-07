// TU-private definitions preserve the retail xCamera inline section.

#if !defined(PS2)
inline void xMat4x3Identity(xMat4x3* m)
{
    xMat4x3Copy(m, &g_I3);
}

inline void xMat4x3Copy(xMat4x3* o, const xMat4x3* m)
{
    memcpy(o, m, sizeof(xMat4x3));
}
#endif

#if !defined(PS2) && !defined(XBOX)
inline void xQuatConj(xQuat* o, const xQuat* q)
{
    o->s = q->s;

    xVec3Inv(&o->v, &q->v);
}
#endif

#if !defined(PS2)
inline void xMat3x3LookAt(xMat3x3* m, const xVec3* pos, const xVec3* at)
{
    xVec3 v;

    xVec3Sub(&v, at, pos);
    xMat3x3LookVec(m, &v);
}
#endif

static inline void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v)
{
    F32 x = m->right.x * v->x + m->up.x * v->y + m->at.x * v->z;
    F32 y = m->right.y * v->x + m->up.y * v->y + m->at.y * v->z;
    F32 z = m->right.z * v->x + m->up.z * v->y + m->at.z * v->z;

    o->x = x;
    o->y = y;
    o->z = z;
}

inline F32 xQuatGetAngle(const xQuat* q)
{
    if (q->s > 0.99998999f)
    {
        return 0.0f;
    }
    else if (q->s < -0.99998999f)
    {
        return 6.2831855f;
    }
    else
    {
        return 2.0f * xacos(q->s);
    }
}
