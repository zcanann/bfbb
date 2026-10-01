#include <rwsdk/rwcore.h>
#include <rwsdk/rtquat.h>
#include <rwsdk/rtslerp.h>

typedef union rwIEEEFloatShape rwIEEEFloatShape;
union rwIEEEFloatShape
{
    RwReal value;
    RwUInt32 word;
};

#define rwIEEEGetFloatWord(_i, _d)                                                                 \
    MACRO_START                                                                                    \
    {                                                                                              \
        rwIEEEFloatShape _gf;                                                                      \
        _gf.value = (_d);                                                                          \
        (_i) = _gf.word;                                                                           \
    }                                                                                              \
    MACRO_STOP

#define rwIEEESetFloatWord(_d, _i)                                                                 \
    MACRO_START                                                                                    \
    {                                                                                              \
        rwIEEEFloatShape _sf;                                                                      \
        _sf.word = (_i);                                                                           \
        (_d) = _sf.value;                                                                          \
    }                                                                                              \
    MACRO_STOP

/* Single precision arc cosine */
#define RwACosMacro(_result, _x)                                                                   \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwReal _z, _p, _q, _r, _w, _s, _c, _df;                                                    \
        RwInt32 _hx, _ix;                                                                          \
                                                                                                   \
        rwIEEEGetFloatWord(_hx, _x);                                                               \
        _ix = _hx & 0x7fffffff;                                                                    \
                                                                                                   \
        if (_ix >= 0x3f800000)                                                                     \
        {                                                                                          \
            /* |x| >= 1 */                                                                         \
            if (_hx > 0)                                                                           \
            {                                                                                      \
                (_result) = (RwReal)0.0;                                                           \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                (_result) = _RW_pi + (RwReal)2.0 * _RW_pio2_lo;                                    \
            }                                                                                      \
        }                                                                                          \
        else if (_ix < 0x3f000000)                                                                 \
        {                                                                                          \
            /* |x| < 0.5 */                                                                        \
            if (_ix <= 0x23000000)                                                                 \
            {                                                                                      \
                (_result) = _RW_pio2_hi + _RW_pio2_lo;                                             \
            }                                                                                      \
            else                                                                                   \
            {                                                                                      \
                _z = (_x) * (_x);                                                                  \
                _p = _z * (_RW_pS0 +                                                               \
                           _z * (_RW_pS1 +                                                         \
                                 _z * (_RW_pS2 + _z * (_RW_pS3 + _z * (_RW_pS4 + _z * _RW_pS5))))); \
                _q = _RW_one + _z * (_RW_qS1 + _z * (_RW_qS2 + _z * (_RW_qS3 + _z * _RW_qS4)));    \
                _r = _p / _q;                                                                      \
                (_result) = _RW_pio2_hi - ((_x) - (_RW_pio2_lo - (_x) * _r));                      \
            }                                                                                      \
        }                                                                                          \
        else if (_hx < 0)                                                                          \
        {                                                                                          \
            /* x < -0.5 */                                                                         \
            _z = (_RW_one + (_x)) * (RwReal)0.5;                                                   \
            _p = _z * (_RW_pS0 +                                                                   \
                       _z * (_RW_pS1 +                                                             \
                             _z * (_RW_pS2 + _z * (_RW_pS3 + _z * (_RW_pS4 + _z * _RW_pS5)))));    \
            _q = _RW_one + _z * (_RW_qS1 + _z * (_RW_qS2 + _z * (_RW_qS3 + _z * _RW_qS4)));        \
            _s = _rwSqrt(_z);                                                                      \
            _r = _p / _q;                                                                          \
            _w = _r * _s - _RW_pio2_lo;                                                            \
            (_result) = _RW_pi - (RwReal)2.0 * (_s + _w);                                          \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            /* x > 0.5 */                                                                          \
            RwInt32 _idf;                                                                          \
                                                                                                   \
            _z = (_RW_one - (_x)) * (RwReal)0.5;                                                   \
            _s = _rwSqrt(_z);                                                                      \
            _df = _s;                                                                              \
            rwIEEEGetFloatWord(_idf, _df);                                                         \
            rwIEEESetFloatWord(_df, _idf & 0xfffff000);                                            \
            _c = (_z - _df * _df) / (_s + _df);                                                    \
            _p = _z * (_RW_pS0 +                                                                   \
                       _z * (_RW_pS1 +                                                             \
                             _z * (_RW_pS2 + _z * (_RW_pS3 + _z * (_RW_pS4 + _z * _RW_pS5)))));    \
            _q = _RW_one + _z * (_RW_qS1 + _z * (_RW_qS2 + _z * (_RW_qS3 + _z * _RW_qS4)));        \
            _r = _p / _q;                                                                          \
            _w = _r * _s + _c;                                                                     \
            (_result) = (RwReal)2.0 * (_df + _w);                                                  \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP


void RtQuatSetupSlerpCache(RtQuat* qpFrom, RtQuat* qpTo, RtQuatSlerpCache* sCache)
{
    RwReal cosOm;

    RtQuatAssign(&sCache->raFrom, qpFrom);

    cosOm = RwV3dDotProductMacro(&qpFrom->imag, &qpTo->imag) + qpFrom->real * qpTo->real;

    /* Take the shortest path */
    if (cosOm < (RwReal)0)
    {
        cosOm = (cosOm < (RwReal)-1) ? (RwReal)1 : -cosOm;
        RtQuatNegate(&sCache->raTo, qpTo);
    }
    else
    {
        cosOm = (cosOm > (RwReal)1) ? (RwReal)1 : cosOm;
        RtQuatAssign(&sCache->raTo, qpTo);
    }

    RwACosMacro(sCache->omega, cosOm);

    sCache->nearlyZeroOm = (cosOm >= (RwReal)0.99999);

    if (!sCache->nearlyZeroOm)
    {
        RwReal sinOm;
        RwReal cosecOm;

        RwSinMinusPiToPiMacro(sinOm, sCache->omega);
        cosecOm = (RwReal)1 / sinOm;

        RtQuatScale(&sCache->raFrom, &sCache->raFrom, cosecOm);
        RtQuatScale(&sCache->raTo, &sCache->raTo, cosecOm);
    }
}
