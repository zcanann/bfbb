// TU-private definitions preserve the retail xCamera inline section.

inline F32 xVec3Length(const xVec3* v)
{
    return xsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

inline void xVec3Sub(xVec3* o, const xVec3* a, const xVec3* b)
{
    o->x = a->x - b->x;
    o->y = a->y - b->y;
    o->z = a->z - b->z;
}

inline void xVec3Inv(xVec3* o, const xVec3* v)
{
    o->x = -v->x;
    o->y = -v->y;
    o->z = -v->z;
}

inline F32 xacos(F32 x)
{
    return std::acosf(x);
}

#ifndef INLINE
inline float std::acosf(float x)
{
    return (float)acos((double)x);
}
#endif

inline void xVec3AddTo(xVec3* o, const xVec3* v)
{
    o->x += v->x;
    o->y += v->y;
    o->z += v->z;
}
