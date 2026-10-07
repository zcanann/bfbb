#ifndef XMATH2_H
#define XMATH2_H

#include <types.h>

template <class T> struct basic_rect
{
    T x;
    T y;
    T w;
    T h;

    const static basic_rect m_Null;
    const static basic_rect m_Unit;

    basic_rect& assign(T x, T y, T w, T h);
    basic_rect& contract(T s);
    basic_rect& contract(T x, T y, T w, T h)
    {
        return expand(-x, -y, -w, -h);
    }
    basic_rect& expand(T s);
    basic_rect& expand(T x, T y, T w, T h)
    {
        this->x -= x;
        this->w += x + w;
        this->y -= y;
        this->h += y + h;
        return *this;
    }
    basic_rect& move(T x, T y);
    basic_rect& scale(T s);
    basic_rect& scale(T x, T y);
    basic_rect& scale(T x, T y, T w, T h);
    void clip(basic_rect& a, basic_rect& b) const;
    void set_bounds(T x1, T y1, T x2, T y2);
    void get_bounds(T& x1, T& y1, T& x2, T& y2) const;
    bool empty() const;
    basic_rect& set_size(T w, T h);
    basic_rect& set_size(T s);
    void center(T x, T y);

    basic_rect& operator|=(const basic_rect& other);
};

struct xVec2
{
    F32 x;
    F32 y;

    xVec2& assign(F32 xy)
    {
        return assign(xy, xy);
    }
    xVec2& assign(F32 x, F32 y);
    F32 length() const;
    F32 length2() const;
    xVec2 normal() const
    {
        xVec2 tmp = *this;
        return tmp.normalize();
    }

    xVec2& normalize()
    {
        *this /= length();
        return *this;
    }

    F32 dot(const xVec2& b) const
    {
        return (x * b.x) + (y * b.y);
    }

    static xVec2 create(F32 x, F32 y)
    {
        xVec2 v = { x, y };
        return v;
    }

    xVec2& operator=(F32);

    xVec2 operator*(F32 f) const
    {
        xVec2 tmp = *this;

        tmp *= f;

        return tmp;
    }

    xVec2 operator/(F32 f) const
    {
        xVec2 tmp = *this;

        tmp /= f;

        return tmp;
    }

    // xPad.cpp defines XVEC2_DIVEQ_OUT_OF_LINE and supplies this body after
    // its own functions: retail emits xPad's weak copy at the end of its
    // .text, which only happens when the body is not yet visible at the call.
#ifdef XVEC2_DIVEQ_OUT_OF_LINE
    xVec2& operator/=(F32 f);
#else
    xVec2& operator/=(F32 f)
    {
        F32 inv = 1.0f / f;

        x *= inv;
        y *= inv;

        return *this;
    }
#endif

    xVec2 operator+(const xVec2& v) const
    {
        xVec2 tmp = *this;

        tmp += v;

        return tmp;
    }

    xVec2& operator+=(const xVec2& v)
    {
        x += v.x;
        y += v.y;

        return *this;
    }

    xVec2& operator*=(F32 f)
    {
        x *= f;
        y *= f;

        return *this;
    }

    xVec2& operator-=(const xVec2&);
    xVec2 operator-(const xVec2&) const;
};

F32 xVec2Dist(F32 x1, F32 y1, F32 x2, F32 y2);
F32 xVec2Dot(const xVec2* a, const xVec2* b);
void xVec2Init(xVec2* v, F32 _x, F32 _y);

#if defined(PS2)
#include "xMathInlines.h"

inline xVec2& xVec2::assign(F32 x, F32 y)
{
    this->x = x;
    this->y = y;
    return *this;
}

inline F32 xVec2::length2() const
{
    return x * x + y * y;
}

inline F32 xVec2::length() const
{
    return xsqrt(length2());
}

inline xVec2 xVec2::operator-(const xVec2& v) const
{
    xVec2 vec = *this;
    vec -= v;
    return vec;
}

inline xVec2& xVec2::operator-=(const xVec2& v)
{
    x -= v.x;
    y -= v.y;
    return *this;
}
#endif

template <class T> basic_rect<T>& basic_rect<T>::contract(T s)
{
    return expand(-s);
}

template <class T> basic_rect<T>& basic_rect<T>::expand(T s)
{
    return expand(s, s, s, s);
}

#endif
