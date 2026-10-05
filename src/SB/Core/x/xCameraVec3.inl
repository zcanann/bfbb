// TU-private definitions preserve the retail xCamera inline section.

inline F32 xVec3Length(const xVec3* v)
{
    return xsqrt(v->x * v->x + v->y * v->y + v->z * v->z);
}

#if !defined(XBOX) && !defined(PS2)
inline void xVec3Sub(xVec3* o, const xVec3* a, const xVec3* b)
{
    o->x = a->x - b->x;
    o->y = a->y - b->y;
    o->z = a->z - b->z;
}
#endif

#if !defined(XBOX)
inline void xVec3Inv(xVec3* o, const xVec3* v)
{
    o->x = -v->x;
    o->y = -v->y;
    o->z = -v->z;
}
#endif

// Reconstruct a stripped reference at the retail implicit-copy boundary.
// The original caller is unknown; this inline caller is not emitted.
inline void __deadstripped_xCamera_quat(xQuat& dest, const xQuat& source)
{
    dest = source;
}

#if !defined(XBOX)
inline F32 xacos(F32 x)
{
    return std::acosf(x);
}
#endif

#if !defined(INLINE) && !defined(XBOX) && !defined(PS2)
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
