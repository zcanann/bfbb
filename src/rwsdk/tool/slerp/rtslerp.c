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


/* The matrix-slerp half of this toolkit (RtSlerp*) is never called by the
 * game, so the linker strips it from the DOL. Its presence ahead of
 * RtQuatSetupSlerpCache is evidenced by retail's .sdata2: the 1.0, 0.0 and
 * 2.0 literals RtQuatSetupSlerpCache shares are numbered @304..@306, far
 * below its own @500.., i.e. they were created first, in that order, by
 * earlier code in this file. The bodies are reconstructions following the
 * public toolkit API and the RtSlerp structure (axis/angle in degrees). */

RtSlerp* RtSlerpCreate(RwInt32 nMatRefMask)
{
    RtSlerp* spNew;

    spNew = (RtSlerp*)RwMalloc(sizeof(RtSlerp));
    if (spNew == NULL)
    {
        return NULL;
    }

    spNew->matRefMask = nMatRefMask;
    spNew->useLerp = FALSE;
    spNew->startMat = NULL;
    spNew->endMat = NULL;

    if (!(nMatRefMask & rtSLERPREFSTARTMAT))
    {
        spNew->startMat = RwMatrixCreate();
        if (spNew->startMat == NULL)
        {
            RwFree(spNew);
            return NULL;
        }
    }

    if (!(nMatRefMask & rtSLERPREFENDMAT))
    {
        spNew->endMat = RwMatrixCreate();
        if (spNew->endMat == NULL)
        {
            if (spNew->startMat != NULL && !(nMatRefMask & rtSLERPREFSTARTMAT))
            {
                RwMatrixDestroy(spNew->startMat);
            }
            RwFree(spNew);
            return NULL;
        }
    }

    return spNew;
}

void RtSlerpDestroy(RtSlerp* spSlerp)
{
    if (!(spSlerp->matRefMask & rtSLERPREFSTARTMAT))
    {
        RwMatrixDestroy(spSlerp->startMat);
    }

    if (!(spSlerp->matRefMask & rtSLERPREFENDMAT))
    {
        RwMatrixDestroy(spSlerp->endMat);
    }

    RwFree(spSlerp);
}

RtSlerp* RtSlerpInitialize(RtSlerp* spSlerp, RwMatrix* mpMat1, RwMatrix* mpMat2)
{
    RwMatrix mInvStart;
    RwMatrix mRelative;
    RtQuat qRelative;

    if (spSlerp->matRefMask & rtSLERPREFSTARTMAT)
    {
        spSlerp->startMat = mpMat1;
    }
    else
    {
        RwMatrixCopy(spSlerp->startMat, mpMat1);
    }

    if (spSlerp->matRefMask & rtSLERPREFENDMAT)
    {
        spSlerp->endMat = mpMat2;
    }
    else
    {
        RwMatrixCopy(spSlerp->endMat, mpMat2);
    }

    /* Rotation taking the start orientation onto the end orientation */
    RwMatrixInvert(&mInvStart, spSlerp->startMat);
    RwMatrixMultiply(&mRelative, &mInvStart, spSlerp->endMat);

    RtQuatConvertFromMatrix(&qRelative, &mRelative);
    RtQuatQueryRotate(&qRelative, &spSlerp->axis, &spSlerp->angle);

    return spSlerp;
}

RwMatrix* RtSlerpGetMatrix(RtSlerp* spSlerp, RwMatrix* mpResultMat, RwReal nDelta)
{
    RwMatrix* mpStart = spSlerp->startMat;
    RwMatrix* mpEnd = spSlerp->endMat;
    RwV3d vDiff;

    /* Keep the interpolant within [0, 1] */
    if (nDelta > (RwReal)1)
    {
        nDelta = (RwReal)1;
    }
    else if (nDelta < (RwReal)0)
    {
        nDelta = (RwReal)0;
    }

    if (spSlerp->useLerp)
    {
        /* Straight linear interpolation of the basis vectors */
        RwV3dSubMacro(&vDiff, &mpEnd->right, &mpStart->right);
        RwV3dScaleMacro(&vDiff, &vDiff, nDelta);
        RwV3dAddMacro(&mpResultMat->right, &mpStart->right, &vDiff);

        RwV3dSubMacro(&vDiff, &mpEnd->up, &mpStart->up);
        RwV3dScaleMacro(&vDiff, &vDiff, nDelta);
        RwV3dAddMacro(&mpResultMat->up, &mpStart->up, &vDiff);

        RwV3dSubMacro(&vDiff, &mpEnd->at, &mpStart->at);
        RwV3dScaleMacro(&vDiff, &vDiff, nDelta);
        RwV3dAddMacro(&mpResultMat->at, &mpStart->at, &vDiff);
    }
    else
    {
        RtQuat qRotate;
        RwMatrix mRotate;

        /* Rotate part of the way about the slerp axis */
        RtQuatRotate(&qRotate, &spSlerp->axis, spSlerp->angle * nDelta, rwCOMBINEREPLACE);
        RtQuatUnitConvertToMatrix(&qRotate, &mRotate);
        RwMatrixMultiply(mpResultMat, &mRotate, mpStart);
    }

    /* The position is always lerped */
    RwV3dSubMacro(&vDiff, &mpEnd->pos, &mpStart->pos);
    RwV3dScaleMacro(&vDiff, &vDiff, nDelta);
    RwV3dAddMacro(&mpResultMat->pos, &mpStart->pos, &vDiff);

    RwMatrixUpdate(mpResultMat);

    return mpResultMat;
}

RtSlerp* RtSlerpSetLerp(RtSlerp* spSlerp, RwBool bUseLerp)
{
    spSlerp->useLerp = bUseLerp;

    return spSlerp;
}

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
        RwReal omega;
        RwReal cosecOm;

        omega = sCache->omega;
        RwSinMinusPiToPiMacro(cosecOm, omega);
        cosecOm = (RwReal)1 / cosecOm;

        RtQuatScale(&sCache->raFrom, &sCache->raFrom, cosecOm);
        RtQuatScale(&sCache->raTo, &sCache->raTo, cosecOm);
    }
}
