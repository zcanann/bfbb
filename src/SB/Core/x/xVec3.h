#ifndef XVEC3_H
#define XVEC3_H

#include <types.h>

struct xVec3
{
    F32 x;
    F32 y;
    F32 z;

    static const xVec3 m_Null;
    static const xVec3 m_UnitAxisX;
    static const xVec3 m_UnitAxisY;

    // These inline aggregate initializers emit 12-byte .rodata templates.
    // Declaration-only users resolve the helpers in another translation unit.
#ifdef XVEC3_DEFER_AGGREGATE_HELPERS
    static xVec3 create(F32 x, F32 y, F32 z);
    static xVec3 create(F32 f);
#else
    static xVec3 create(F32 x, F32 y, F32 z)
    {
        xVec3 v = { x, y, z };
        return v;
    }

    static xVec3 create(F32 f)
    {
        xVec3 v = { f, f, f };
        return v;
    }

#endif

    xVec3& operator=(F32 f)
    {
        x = y = z = f;
        return *this;
    }
    xVec3 operator+(const xVec3&) const;
    xVec3 operator+(F32) const;
    xVec3 operator-(const xVec3&) const;
    xVec3 operator-() const
    {
        xVec3 v = *this;

        v.x = -v.x;
        v.y = -v.y;
        v.z = -v.z;

        return v;
    }
    xVec3 operator*(F32) const;
    xVec3 operator*(const xVec3&) const;
    xVec3 operator/(F32) const;
    xVec3& operator+=(const xVec3&);
    xVec3& operator+=(F32 f)
    {
        this->x += f;
        this->y += f;
        this->z += f;

        return *this;
    }
    xVec3& operator-=(const xVec3&);
    xVec3& operator-=(F32 f)
    {
        this->x -= f;
        this->y -= f;
        this->z -= f;

        return *this;
    }

    xVec3& operator*=(F32);
    xVec3& operator*=(const xVec3&);
    xVec3& operator/=(F32);

    xVec3& right_normalize();
    xVec3& safe_normalize(const xVec3& val);
    xVec3& up_normalize();
    xVec3 safe_normal(const xVec3& val) const
    {
        xVec3 v = *this;

        return v.safe_normalize(val);
    }

    xVec3 up_normal() const
    {
        return safe_normal(xVec3::m_UnitAxisY);
    }
    xVec3 normal() const
    {
        xVec3 tmp = *this;
        return tmp.normalize();
    }
    xVec3& assign(F32 x, F32 y, F32 z);
    F32 length() const;
    F32 length2() const;
    xVec3& invert();

    xVec3 inverse() const
    {
        xVec3 inverse = *this;
        return inverse.invert();
    }

    F32 dot(const xVec3& c) const;

#ifdef XVEC3_DEFER_AGGREGATE_HELPERS
    xVec3 cross(const xVec3& c) const;
#else
    xVec3 cross(const xVec3& c) const
    {
        xVec3 v = { 0.0f, 0.0f, 0.0f };

        v.x = y * c.z - c.y * z;
        v.y = z * c.x - c.z * x;
        v.z = x * c.y - c.x * y;

        return v;
    }

#endif

    xVec3& normalize();
    xVec3& assign(F32 val);
    xVec3 get_abs() const;
    xVec3& set_abs();
};

F32 xVec3Normalize(xVec3* o, const xVec3* v);
F32 xVec3NormalizeFast(xVec3* o, const xVec3* v);
void xVec3Copy(xVec3* dst, const xVec3* src);
#if defined(PS2) || defined(XBOX)
inline F32 xVec3Dot(const xVec3* a, const xVec3* b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}
#else
F32 xVec3Dot(const xVec3* a, const xVec3* b);
#endif

inline xVec3& xVec3::assign(float dt)
{
    return assign(dt, dt, dt);
}

// Retail has these as weak header inlines (xBound/xCollide own the copies).
// zParPTank opts in so its emitted copies form the inline group that the
// snow_particle_data assignment joins, as in the retail object.
#if defined(XVEC3_SCALE_ADD_INLINE) || defined(XBOX)
inline xVec3 xVec3::operator*(F32 f) const
{
    xVec3 temp = *this;
    temp *= f;

    return temp;
}

inline xVec3& xVec3::operator+=(const xVec3& v)
{
    x += v.x;
    y += v.y;
    z += v.z;
    return *this;
}
#endif

// Bungee groups the local matrix helper with the vector helpers. Keep this
// implementation identical to xMath3.h; other units retain their header group.
#ifdef XVEC3_MATMUL_INLINE
struct xMat3x3
{
    xVec3 right;
    S32 flags;
    xVec3 up;
    U32 pad1;
    xVec3 at;
    U32 pad2;
};

static inline void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v)
{
    F32 x = m->right.x * v->x + m->up.x * v->y + m->at.x * v->z;
    F32 y = m->right.y * v->x + m->up.y * v->y + m->at.y * v->z;
    F32 z = m->right.z * v->x + m->up.z * v->y + m->at.z * v->z;

    o->x = x;
    o->y = y;
    o->z = z;
}
#endif

#if defined(XBOX)
inline xVec3 xVec3::operator+(const xVec3& v) const
{
    xVec3 vec = *this;
    vec += v;
    return vec;
}

inline xVec3 xVec3::operator-(const xVec3& v) const
{
    xVec3 temp = *this;
    temp -= v;

    return temp;
}

inline xVec3& xVec3::operator-=(const xVec3& v)
{
    this->x -= v.x;
    this->y -= v.y;
    this->z -= v.z;

    return *this;
}

inline xVec3& xVec3::operator*=(F32 f)
{
    this->x *= f;
    this->y *= f;
    this->z *= f;

    return *this;
}
#endif

#endif
