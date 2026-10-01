#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rtquat.h>
#include <rwsdk/rtanim.h>
#include <rwsdk/rphanim.h>

typedef struct RpHAnimKeyFrame RpHAnimKeyFrame;
struct RpHAnimKeyFrame
{
    RpHAnimKeyFrame* prevFrame;
    RwReal time;
    RtQuat q;
    RwV3d t;
};

typedef struct RpHAnimInterpFrame RpHAnimInterpFrame;
struct RpHAnimInterpFrame
{
    RpHAnimKeyFrame* keyFrame1;
    RpHAnimKeyFrame* keyFrame2;
    RtQuat q;
    RwV3d t;
};

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

void RpHAnimKeyFrameApply(void* matrix, void* voidIFrame)
{
    RwMatrix* pMatrix = (RwMatrix*)matrix;
    RpHAnimInterpFrame* iFrame = (RpHAnimInterpFrame*)voidIFrame;

    RtQuatUnitConvertToMatrix(&iFrame->q, pMatrix);

    pMatrix->pos.x = iFrame->t.x;
    pMatrix->pos.y = iFrame->t.y;
    pMatrix->pos.z = iFrame->t.z;
}

void RpHAnimKeyFrameInterpolate(void* voidOut, void* voidIn1, void* voidIn2, RwReal time,
                                void* customData)
{
    RpHAnimInterpFrame* out = (RpHAnimInterpFrame*)voidOut;
    RpHAnimKeyFrame* in1 = (RpHAnimKeyFrame*)voidIn1;
    RpHAnimKeyFrame* in2 = (RpHAnimKeyFrame*)voidIn2;
    RwReal fRecipTime;
    RwReal fDot;
    RwReal fScale;

    fRecipTime = (time - in1->time) / (in2->time - in1->time);

    fDot = RwV3dDotProductMacro(&in1->q.imag, &in2->q.imag) + in1->q.real * in2->q.real;

    /* Linearly interpolate the translation */
    out->t.x = in1->t.x + fRecipTime * (in2->t.x - in1->t.x);
    out->t.y = in1->t.y + fRecipTime * (in2->t.y - in1->t.y);
    out->t.z = in1->t.z + fRecipTime * (in2->t.z - in1->t.z);

    /* Spherically interpolate the rotation, taking the shortest path */
    if (fDot < (RwReal)0.0)
    {
        fDot = -fDot;
        in2->q.imag.x = -in2->q.imag.x;
        in2->q.imag.y = -in2->q.imag.y;
        in2->q.imag.z = -in2->q.imag.z;
        in2->q.real = -in2->q.real;
    }

    fScale = (RwReal)1.0 - fRecipTime;

    if (!(fDot >= (RwReal)0.999))
    {
        RwReal theta;
        RwReal sinTheta;
        RwReal recipSinTheta;

        RwACosMacro(theta, fDot);

        fScale *= theta;
        fRecipTime *= theta;

        RwSinMinusPiToPiMacro(sinTheta, theta);
        recipSinTheta = (RwReal)1.0 / sinTheta;

        RwSinMinusPiToPiMacro(fScale, fScale);
        RwSinMinusPiToPiMacro(fRecipTime, fRecipTime);

        fScale *= recipSinTheta;
        fRecipTime *= recipSinTheta;
    }

    out->q.imag.x = fScale * in1->q.imag.x + fRecipTime * in2->q.imag.x;
    out->q.imag.y = fScale * in1->q.imag.y + fRecipTime * in2->q.imag.y;
    out->q.imag.z = fScale * in1->q.imag.z + fRecipTime * in2->q.imag.z;
    out->q.real = fScale * in1->q.real + fRecipTime * in2->q.real;
}

void RpHAnimKeyFrameBlend(void* voidOut, void* voidIn1, void* voidIn2, RwReal alpha)
{
    RpHAnimInterpFrame* out = (RpHAnimInterpFrame*)voidOut;
    RpHAnimInterpFrame* in1 = (RpHAnimInterpFrame*)voidIn1;
    RpHAnimInterpFrame* in2 = (RpHAnimInterpFrame*)voidIn2;
    RwReal fDot;
    RwReal fScale;

    fDot = RwV3dDotProductMacro(&in1->q.imag, &in2->q.imag) + in1->q.real * in2->q.real;

    /* Linearly interpolate the translation */
    out->t.x = in1->t.x + alpha * (in2->t.x - in1->t.x);
    out->t.y = in1->t.y + alpha * (in2->t.y - in1->t.y);
    out->t.z = in1->t.z + alpha * (in2->t.z - in1->t.z);

    /* Spherically interpolate the rotation, taking the shortest path */
    if (fDot < (RwReal)0.0)
    {
        fDot = -fDot;
        in2->q.imag.x = -in2->q.imag.x;
        in2->q.imag.y = -in2->q.imag.y;
        in2->q.imag.z = -in2->q.imag.z;
        in2->q.real = -in2->q.real;
    }

    fScale = (RwReal)1.0 - alpha;

    if (!(fDot >= (RwReal)0.999))
    {
        RwReal theta;
        RwReal sinTheta;
        RwReal recipSinTheta;

        RwACosMacro(theta, fDot);

        fScale *= theta;
        alpha *= theta;

        RwSinMinusPiToPiMacro(sinTheta, theta);
        recipSinTheta = (RwReal)1.0 / sinTheta;

        RwSinMinusPiToPiMacro(fScale, fScale);
        RwSinMinusPiToPiMacro(alpha, alpha);

        fScale *= recipSinTheta;
        alpha *= recipSinTheta;
    }

    out->q.imag.x = fScale * in1->q.imag.x + alpha * in2->q.imag.x;
    out->q.imag.y = fScale * in1->q.imag.y + alpha * in2->q.imag.y;
    out->q.imag.z = fScale * in1->q.imag.z + alpha * in2->q.imag.z;
    out->q.real = fScale * in1->q.real + alpha * in2->q.real;
}

RtAnimAnimation* RpHAnimKeyFrameStreamRead(RwStream* stream, RtAnimAnimation* animation)
{
    RwInt32 i;
    RwUInt32 temp;
    RpHAnimKeyFrame* frames = (RpHAnimKeyFrame*)animation->pFrames;

    for (i = 0; i < animation->numFrames; i++)
    {
        if (!RwStreamReadReal(stream, &frames[i].time, sizeof(RwReal) * 8))
        {
            return (RtAnimAnimation*)NULL;
        }

        if (!RwStreamReadInt32(stream, (RwInt32*)&temp, sizeof(RwInt32)))
        {
            return (RtAnimAnimation*)NULL;
        }

        frames[i].prevFrame = &frames[temp / sizeof(RpHAnimKeyFrame)];
    }

    return animation;
}

RwBool RpHAnimKeyFrameStreamWrite(const RtAnimAnimation* animation, RwStream* stream)
{
    RwInt32 i;
    RwInt32 temp;
    RpHAnimKeyFrame* frames = (RpHAnimKeyFrame*)animation->pFrames;

    for (i = 0; i < animation->numFrames; i++)
    {
        if (!RwStreamWriteReal(stream, &frames[i].time, sizeof(RwReal) * 8))
        {
            return FALSE;
        }

        temp = (RwUInt8*)frames[i].prevFrame - (RwUInt8*)frames;

        if (!RwStreamWriteInt32(stream, &temp, sizeof(RwInt32)))
        {
            return FALSE;
        }
    }

    return TRUE;
}

RwInt32 RpHAnimKeyFrameStreamGetSize(const RtAnimAnimation* animation)
{
    RwInt32 size = sizeof(RpHAnimKeyFrame);

    size *= animation->numFrames;

    return size;
}

void RpHAnimKeyFrameMulRecip(void* voidFrame, void* voidStart)
{
    RpHAnimKeyFrame* frame = (RpHAnimKeyFrame*)voidFrame;
    RpHAnimKeyFrame* start = (RpHAnimKeyFrame*)voidStart;
    RtQuat qRecip;
    RtQuat qTemp;

    RtQuatReciprocal(&qRecip, &start->q);

    RtQuatAssign(&qTemp, &frame->q);
    RtQuatMultiply(&frame->q, &qRecip, &qTemp);

    RwV3dSubMacro(&frame->t, &frame->t, &start->t);
}

void RpHAnimKeyFrameAdd(void* voidOut, void* voidIn1, void* voidIn2)
{
    RpHAnimKeyFrame* out = (RpHAnimKeyFrame*)voidOut;
    RpHAnimKeyFrame* in1 = (RpHAnimKeyFrame*)voidIn1;
    RpHAnimKeyFrame* in2 = (RpHAnimKeyFrame*)voidIn2;

    RtQuatMultiply(&out->q, &in1->q, &in2->q);

    RwV3dAddMacro(&out->t, &in1->t, &in2->t);
}
