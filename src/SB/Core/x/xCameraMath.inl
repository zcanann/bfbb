// TU-private definitions preserve the retail xCamera inline section.

inline xVec3& xVec3::invert()
{
    this->x = -this->x;
    this->y = -this->y;
    this->z = -this->z;

    return *this;
}

inline F32 xexp(F32 x)
{
    return std::expf(x);
}

#if !defined(INLINE) && !defined(PS2)
inline float std::expf(float x)
{
    return (float)exp((double)x);
}
#endif

#if !defined(PS2)
inline F32 xrmod(F32 ang)
{
    F32 frac = 0.15915494f * ang;

    if (frac < 0.0f)
    {
        return (frac - std::ceilf(frac) + 1.0f) * 6.2831855f;
    }
    else if (frac >= 1.0f)
    {
        return (frac - std::floorf(frac)) * 6.2831855f;
    }

    return ang;
}
#endif

inline xVec3& xVec3::operator/=(F32 f)
{
    F32 f2 = 1.0f / f;

    this->x *= f2;
    this->y *= f2;
    this->z *= f2;

    return *this;
}

inline xVec3& xVec3::right_normalize()
{
    return this->safe_normalize(xVec3::m_UnitAxisX);
}

inline xVec3& xVec3::safe_normalize(const xVec3& val)
{
    F32 len = this->length2();

    if (len < 0.000099999997f)
    {
        return (*this = val);
    }
    else
    {
        return (*this *= 1.0f / xsqrt(len));
    }
}

#if !defined(XBOX)
template <>
inline F32 range_limit<F32>(F32 v, F32 minv, F32 maxv)
{
    if (v <= minv)
    {
        return minv;
    }

    if (v >= maxv)
    {
        return maxv;
    }

    return v;
}
#endif
