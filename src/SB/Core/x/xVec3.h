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

    static xVec3 create(F32 x, F32 y, F32 z)
    {
        xVec3 v;

        v.x = x;
        v.y = y;
        v.z = z;

        return v;
    }

    static xVec3 create(F32 f)
    {
        xVec3 v;

        v.x = f;
        v.y = f;
        v.z = f;

        return v;
    }

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

    xVec3 cross(const xVec3& c) const
    {
        xVec3 v = xVec3::m_Null;

        v.x = y * c.z - c.y * z;
        v.y = z * c.x - c.z * x;
        v.z = x * c.y - c.x * y;

        return v;
    }

    xVec3& normalize();
    xVec3& assign(F32 val);
    xVec3 get_abs() const;
    xVec3& set_abs();
};

F32 xVec3Normalize(xVec3* o, const xVec3* v);
F32 xVec3NormalizeFast(xVec3* o, const xVec3* v);
void xVec3Copy(xVec3* dst, const xVec3* src);
F32 xVec3Dot(const xVec3* a, const xVec3* b);

#endif
