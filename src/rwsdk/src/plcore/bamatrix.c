#include <rwsdk/rwcore.h>

#define rwPLUGIN_ID 1

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_BADPARAM 0x80000003

#define rwMATRIXALIGNMENT sizeof(RwUInt32)

#define RwRealAbs(_a) (((_a) >= (RwReal)0.0) ? (_a) : (-(_a)))

#define rwInvSqrtMacro(_result, _num) ((*(_result)) = _rwInvSqrt(_num))

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rwMatrixGlobals rwMatrixGlobals;
struct rwMatrixGlobals
{
    RwFreeList* matrixFreeList;
    RwUInt32 flags;
    rwMatrixMultFn multMatrix;
    RwMatrixTolerance tolerance;
};

#define RWMATRIXGLOBAL(var)                                                                        \
    (RWPLUGINOFFSET(rwMatrixGlobals, RwEngineInstance, matrixModule.globalsOffset)->var)

static RwModuleInfo matrixModule;

static void MatrixMultiply(RwMatrix* dstMat, const RwMatrix* matA, const RwMatrix* matB)
{
    dstMat->right.x = matA->right.x * matB->right.x + matA->right.y * matB->up.x +
                      matA->right.z * matB->at.x;
    dstMat->right.y = matA->right.x * matB->right.y + matA->right.y * matB->up.y +
                      matA->right.z * matB->at.y;
    dstMat->right.z = matA->right.x * matB->right.z + matA->right.y * matB->up.z +
                      matA->right.z * matB->at.z;

    dstMat->up.x = matA->up.x * matB->right.x + matA->up.y * matB->up.x + matA->up.z * matB->at.x;
    dstMat->up.y = matA->up.x * matB->right.y + matA->up.y * matB->up.y + matA->up.z * matB->at.y;
    dstMat->up.z = matA->up.x * matB->right.z + matA->up.y * matB->up.z + matA->up.z * matB->at.z;

    dstMat->at.x = matA->at.x * matB->right.x + matA->at.y * matB->up.x + matA->at.z * matB->at.x;
    dstMat->at.y = matA->at.x * matB->right.y + matA->at.y * matB->up.y + matA->at.z * matB->at.y;
    dstMat->at.z = matA->at.x * matB->right.z + matA->at.y * matB->up.z + matA->at.z * matB->at.z;

    dstMat->pos.x = matA->pos.x * matB->right.x + matA->pos.y * matB->up.x +
                    matA->pos.z * matB->at.x + matB->pos.x;
    dstMat->pos.y = matA->pos.x * matB->right.y + matA->pos.y * matB->up.y +
                    matA->pos.z * matB->at.y + matB->pos.y;
    dstMat->pos.z = matA->pos.x * matB->right.z + matA->pos.y * matB->up.z +
                    matA->pos.z * matB->at.z + matB->pos.z;
}

static RwMatrix* MatrixOrthoNormalize(RwMatrix* dst, const RwMatrix* src)
{
    RwV3d right;
    RwV3d up;
    RwV3d at;
    RwV3d pos;
    RwV3d* vpU;
    RwV3d* vpV;
    RwV3d* vpW;
    RwReal recipRight;
    RwReal recipUp;
    RwReal recipAt;
    RwReal recip;

    right = src->right;
    up = src->up;
    at = src->at;
    pos = src->pos;

    _rwV3dNormalizeMacro(recipRight, &right, &right);
    _rwV3dNormalizeMacro(recipUp, &up, &up);
    _rwV3dNormalizeMacro(recipAt, &at, &at);

    /* Pick the pair of axes that are closest to orthogonal to build from */
    if (recipRight > 0.0f)
    {
        if (recipUp > 0.0f)
        {
            if (recipAt > 0.0f)
            {
                RwReal dotUpAt;
                RwReal dotAtRight;
                RwReal dotRightUp;

                recipAt = RwV3dDotProductMacro(&up, &at);
                dotUpAt = RwRealAbs(recipAt);
                recipAt = RwV3dDotProductMacro(&at, &right);
                dotAtRight = RwRealAbs(recipAt);
                recipAt = RwV3dDotProductMacro(&right, &up);
                dotRightUp = RwRealAbs(recipAt);

                if (dotUpAt < dotAtRight)
                {
                    if (dotUpAt < dotRightUp)
                    {
                        vpU = &up;
                        vpV = &at;
                        vpW = &right;
                    }
                    else
                    {
                        vpU = &right;
                        vpV = &up;
                        vpW = &at;
                    }
                }
                else
                {
                    if (dotAtRight < dotRightUp)
                    {
                        vpU = &at;
                        vpV = &right;
                        vpW = &up;
                    }
                    else
                    {
                        vpU = &right;
                        vpV = &up;
                        vpW = &at;
                    }
                }
            }
            else
            {
                vpU = &right;
                vpV = &up;
                vpW = &at;
            }
        }
        else
        {
            vpU = &at;
            vpV = &right;
            vpW = &up;
        }
    }
    else
    {
        vpU = &up;
        vpV = &at;
        vpW = &right;
    }

    RwV3dCrossProductMacro(vpW, vpU, vpV);
    _rwV3dNormalizeMacro(recip, vpW, vpW);

    RwV3dCrossProductMacro(vpV, vpW, vpU);
    _rwV3dNormalizeMacro(recip, vpV, vpV);

    dst->right = right;
    dst->up = up;
    dst->at = at;
    dst->pos = pos;

    rwMatrixSetFlags(dst, (rwMatrixGetFlags(dst) | rwMATRIXTYPEORTHONORMAL) &
                              ~rwMATRIXINTERNALIDENTITY);

    return dst;
}

static RwMatrix* MatrixInvertOrthoNormalized(RwMatrix* dst, const RwMatrix* src)
{
    /* The inverse of the rotation is its transpose */
    dst->right.x = src->right.x;
    dst->right.y = src->up.x;
    dst->right.z = src->at.x;

    dst->up.x = src->right.y;
    dst->up.y = src->up.y;
    dst->up.z = src->at.y;

    dst->at.x = src->right.z;
    dst->at.y = src->up.z;
    dst->at.z = src->at.z;

    dst->pos.x = -RwV3dDotProductMacro(&src->pos, &src->right);
    dst->pos.y = -RwV3dDotProductMacro(&src->pos, &src->up);
    dst->pos.z = -RwV3dDotProductMacro(&src->pos, &src->at);

    rwMatrixSetFlags(dst, rwMATRIXTYPEORTHONORMAL);

    return dst;
}

static RwMatrix* MatrixInvertGeneric(RwMatrix* dst, const RwMatrix* src)
{
    RwSplitBits determinant;
    RwReal normalize;

    /* First column of the adjoint */
    dst->right.x = src->up.y * src->at.z - src->up.z * src->at.y;
    dst->right.y = -(src->right.y * src->at.z - src->right.z * src->at.y);
    dst->right.z = src->right.y * src->up.z - src->right.z * src->up.y;

    determinant.nReal = dst->right.x * src->right.x + dst->right.y * src->up.x +
                        dst->right.z * src->at.x;

    normalize = (determinant.nInt) ? ((RwReal)1.0f / determinant.nReal) : (RwReal)1.0f;

    dst->right.x *= normalize;
    dst->right.y *= normalize;
    dst->right.z *= normalize;

    dst->up.x = normalize * -(src->up.x * src->at.z - src->up.z * src->at.x);
    dst->up.y = normalize * (src->right.x * src->at.z - src->right.z * src->at.x);
    dst->up.z = normalize * -(src->right.x * src->up.z - src->right.z * src->up.x);

    dst->at.x = normalize * (src->up.x * src->at.y - src->up.y * src->at.x);
    dst->at.y = normalize * -(src->right.x * src->at.y - src->right.y * src->at.x);
    dst->at.z = normalize * (src->right.x * src->up.y - src->right.y * src->up.x);

    dst->pos.x = -(src->pos.x * dst->right.x + src->pos.y * dst->up.x + src->pos.z * dst->at.x);
    dst->pos.y = -(src->pos.x * dst->right.y + src->pos.y * dst->up.y + src->pos.z * dst->at.y);
    dst->pos.z = -(src->pos.x * dst->right.z + src->pos.y * dst->up.z + src->pos.z * dst->at.z);

    rwMatrixSetFlags(dst, 0);

    return dst;
}

RwReal _rwMatrixDeterminant(const RwMatrix* matrix)
{
    RwReal result;
    const RwV3d* mx = &matrix->right;
    const RwV3d* my = &matrix->up;
    const RwV3d* mz = &matrix->at;
    RwV3d cross;

    RwV3dCrossProductMacro(&cross, my, mz);
    result = RwV3dDotProductMacro(&cross, mx);

    return result;
}

RwReal _rwMatrixOrthogonalError(const RwMatrix* matrix)
{
    RwReal result;
    const RwV3d* mx = &matrix->right;
    const RwV3d* my = &matrix->up;
    const RwV3d* mz = &matrix->at;
    RwV3d dot;

    dot.x = RwV3dDotProductMacro(my, mz);
    dot.y = RwV3dDotProductMacro(mz, mx);
    dot.z = RwV3dDotProductMacro(mx, my);
    result = RwV3dDotProductMacro(&dot, &dot);

    return result;
}

RwReal _rwMatrixNormalError(const RwMatrix* matrix)
{
    RwReal result;
    const RwV3d* x = &matrix->right;
    const RwV3d* y = &matrix->up;
    const RwV3d* z = &matrix->at;
    RwV3d dot;

    dot.x = RwV3dDotProductMacro(x, x) - (RwReal)1.0;
    dot.y = RwV3dDotProductMacro(y, y) - (RwReal)1.0;
    dot.z = RwV3dDotProductMacro(z, z) - (RwReal)1.0;
    result = RwV3dDotProductMacro(&dot, &dot);

    return result;
}

RwReal _rwMatrixIdentityError(const RwMatrix* matrix)
{
    RwReal result;
    const RwV3d* mx = &matrix->right;
    const RwV3d* my = &matrix->up;
    const RwV3d* mz = &matrix->at;
    const RwV3d* mw = &matrix->pos;
    RwReal error_x;
    RwReal error_y;
    RwReal error_z;
    RwReal error_w;

    error_x = mx->x - (RwReal)1.0;
    error_x = error_x * error_x + mx->y * mx->y + mx->z * mx->z;
    error_y = my->y - (RwReal)1.0;
    error_y = my->x * my->x + error_y * error_y + my->z * my->z;
    error_z = mz->z - (RwReal)1.0;
    error_z = mz->x * mz->x + mz->y * mz->y + error_z * error_z;
    error_w = mw->x * mw->x + mw->y * mw->y + mw->z * mw->z;

    result = error_x + error_y + error_z + error_w;

    return result;
}

void* _rwMatrixClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWMATRIXGLOBAL(matrixFreeList))
    {
        RwFreeListDestroy(RWMATRIXGLOBAL(matrixFreeList));
        RWMATRIXGLOBAL(matrixFreeList) = NULL;
    }

    matrixModule.numInstances--;

    return instance;
}

static RwInt32 _rwMatrixFreeListBlockSize = 256;
static RwInt32 _rwMatrixFreeListPreallocBlocks = 1;
static RwFreeList _rwMatrixFreeList;

void* _rwMatrixOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    matrixModule.globalsOffset = offset;

    RWMATRIXGLOBAL(matrixFreeList) =
        RwFreeListCreateAndPreallocateSpace(sizeof(RwMatrix), _rwMatrixFreeListBlockSize,
                                            rwMATRIXALIGNMENT, _rwMatrixFreeListPreallocBlocks,
                                            &_rwMatrixFreeList);
    if (!RWMATRIXGLOBAL(matrixFreeList))
    {
        instance = NULL;
    }
    else
    {
        const RwMatrixTolerance tolerance = { 0.01f, 0.01f, 0.01f };

        RWMATRIXGLOBAL(flags) = rwMATRIXOPTIMIZE_IDENTITY;
        RWMATRIXGLOBAL(multMatrix) = MatrixMultiply;

        RwEngineSetMatrixTolerances(&tolerance);

        matrixModule.numInstances++;
    }

    return instance;
}

RwBool RwEngineSetMatrixTolerances(const RwMatrixTolerance* const tolerance)
{
    RWMATRIXGLOBAL(tolerance) = *tolerance;

    return TRUE;
}

RwMatrix* RwMatrixOptimize(RwMatrix* matrix, const RwMatrixTolerance* tolerance)
{
    RwUInt32 flags;
    RwBool MatrixIsNormal;
    RwBool MatrixIsOrthogonal;
    RwBool MatrixIsIdentity;

    if (!tolerance)
    {
        tolerance = &RWMATRIXGLOBAL(tolerance);
    }

    MatrixIsNormal = (tolerance->Normal >= _rwMatrixNormalError(matrix));
    MatrixIsOrthogonal = (tolerance->Orthogonal >= _rwMatrixOrthogonalError(matrix));
    MatrixIsIdentity = MatrixIsNormal && MatrixIsOrthogonal &&
                       (tolerance->Identity >= _rwMatrixIdentityError(matrix));

    flags = rwMatrixGetFlags(matrix);

    if (MatrixIsNormal)
    {
        flags |= rwMATRIXTYPENORMAL;
    }
    else
    {
        flags &= ~rwMATRIXTYPENORMAL;
    }

    if (MatrixIsOrthogonal)
    {
        flags |= rwMATRIXTYPEORTHOGONAL;
    }
    else
    {
        flags &= ~rwMATRIXTYPEORTHOGONAL;
    }

    if (MatrixIsIdentity)
    {
        flags |= rwMATRIXINTERNALIDENTITY;
    }
    else
    {
        flags &= ~rwMATRIXINTERNALIDENTITY;
    }

    rwMatrixSetFlags(matrix, flags);

    return matrix;
}

RwMatrix* RwMatrixUpdate(RwMatrix* matrix)
{
    rwMatrixSetFlags(matrix,
                     rwMatrixGetFlags(matrix) & ~(rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEMASK));

    return matrix;
}

asm RwMatrix* RwMatrixMultiply(register RwMatrix* dst, register const RwMatrix* src1,
                               register const RwMatrix* src2)
{
    psq_l f8, 0x0(src2), 0, 0
    psq_l f0, 0x0(src1), 0, 0
    psq_l f2, 0x10(src1), 0, 0
    ps_muls0 f12, f8, f0
    psq_l f9, 0x10(src2), 0, 0
    ps_muls0 f13, f8, f2
    psq_l f4, 0x20(src1), 0, 0
    psq_l f6, 0x30(src1), 0, 0
    ps_muls0 f30, f8, f4
    psq_l f11, 0x30(src2), 0, 0
    ps_madds1 f12, f9, f0, f12
    psq_l f10, 0x20(src2), 0, 0
    ps_madds1 f13, f9, f2, f13
    ps_madds0 f31, f8, f6, f11
    psq_l f1, 0x8(src1), 1, 0
    ps_madds1 f30, f9, f4, f30
    psq_l f3, 0x18(src1), 1, 0
    ps_madds0 f12, f10, f1, f12
    psq_l f5, 0x28(src1), 1, 0
    ps_madds1 f31, f9, f6, f31
    psq_l f7, 0x38(src1), 1, 0
    ps_madds0 f13, f10, f3, f13
    psq_l f8, 0x8(src2), 1, 0
    psq_st f12, 0x0(dst), 0, 0
    ps_madds0 f30, f10, f5, f30
    ps_madds0 f31, f10, f7, f31
    psq_l f11, 0x38(src2), 1, 0
    psq_st f13, 0x10(dst), 0, 0
    ps_muls0 f12, f8, f0
    psq_l f9, 0x18(src2), 1, 0
    psq_st f30, 0x20(dst), 0, 0
    ps_muls0 f13, f8, f2
    psq_l f10, 0x28(src2), 1, 0
    ps_madds1 f12, f9, f0, f12
    lwz r0, 0xc(src1)
    lwz r4, 0xc(src2)
    ps_madds0 f11, f8, f6, f11
    ps_muls0 f30, f8, f4
    psq_st f31, 0x30(dst), 0, 0
    ps_madds1 f13, f9, f2, f13
    and r4, r0, r4
    ps_madds1 f11, f9, f6, f11
    stw r4, 0xc(dst)
    ps_madds0 f12, f10, f1, f12
    ps_madds1 f8, f9, f4, f30
    ps_madds0 f13, f10, f3, f13
    psq_st f12, 0x8(dst), 1, 0
    ps_madds0 f11, f10, f7, f11
    ps_madds0 f8, f10, f5, f8
    psq_st f13, 0x18(dst), 1, 0
    psq_st f8, 0x28(dst), 1, 0
    psq_st f11, 0x38(dst), 1, 0
    frfree
    blr
}

RwMatrix* RwMatrixOrthoNormalize(RwMatrix* dst, const RwMatrix* src)
{
    return MatrixOrthoNormalize(dst, src);
}

RwMatrix* RwMatrixRotateOneMinusCosineSine(RwMatrix* matrix, const RwV3d* unitAxis,
                                           RwReal oneMinusCosine, RwReal sine,
                                           RwOpCombineType combineOp)
{
    RwMatrix mRotate;
    RwMatrix mLocal;
    RwV3d vLeading;
    RwV3d vScaled;
    RwV3d vCrossed;

    vLeading.x = (RwReal)1.0 - unitAxis->x * unitAxis->x;
    vLeading.y = (RwReal)1.0 - unitAxis->y * unitAxis->y;
    vLeading.z = (RwReal)1.0 - unitAxis->z * unitAxis->z;
    RwV3dScaleMacro(&vLeading, &vLeading, oneMinusCosine);

    vCrossed.x = unitAxis->y * unitAxis->z;
    vCrossed.y = unitAxis->z * unitAxis->x;
    vCrossed.z = unitAxis->x * unitAxis->y;
    RwV3dScaleMacro(&vCrossed, &vCrossed, oneMinusCosine);

    RwV3dScaleMacro(&vScaled, unitAxis, sine);

    mRotate.right.x = (RwReal)1.0 - vLeading.x;
    mRotate.right.y = vCrossed.z + vScaled.z;
    mRotate.right.z = vCrossed.y - vScaled.y;

    mRotate.up.x = vCrossed.z - vScaled.z;
    mRotate.up.y = (RwReal)1.0 - vLeading.y;
    mRotate.up.z = vCrossed.x + vScaled.x;

    mRotate.at.x = vCrossed.y + vScaled.y;
    mRotate.at.y = vCrossed.x - vScaled.x;
    mRotate.at.z = (RwReal)1.0 - vLeading.z;

    mRotate.pos.x = (RwReal)0.0;
    mRotate.pos.y = (RwReal)0.0;
    mRotate.pos.z = (RwReal)0.0;

    rwMatrixSetFlags(&mRotate, rwMATRIXTYPEORTHONORMAL);

    switch (combineOp)
    {
    case rwCOMBINEREPLACE:
        RwMatrixCopy(matrix, &mRotate);
        break;
    case rwCOMBINEPRECONCAT:
        RwMatrixMultiply(&mLocal, &mRotate, matrix);
        RwMatrixCopy(matrix, &mLocal);
        break;
    case rwCOMBINEPOSTCONCAT:
        RwMatrixMultiply(&mLocal, matrix, &mRotate);
        RwMatrixCopy(matrix, &mLocal);
        break;
    default:
        RWERROR((E_RW_BADPARAM, "Invalid combination type"));
        matrix = NULL;
        break;
    }

    return matrix;
}

RwMatrix* RwMatrixRotate(RwMatrix* matrix, const RwV3d* axis, RwReal angle,
                         RwOpCombineType combineOp)
{
    RwV3d unitAxis;
    RwReal sinVal;
    RwReal oneMinusCosVal;
    RwReal radians;
    RwReal recip;

    radians = ((RwReal)(rwPI / 180.0)) * angle;

    _rwV3dNormalizeMacro(recip, &unitAxis, axis);

    sinVal = (RwReal)sin(radians);
    oneMinusCosVal = (RwReal)1.0 - (RwReal)cos(radians);

    RwMatrixRotateOneMinusCosineSine(matrix, &unitAxis, oneMinusCosVal, sinVal, combineOp);

    return matrix;
}

RwMatrix* RwMatrixInvert(RwMatrix* dst, const RwMatrix* src)
{
    if (rwMatrixGetFlags(src) & (RWMATRIXGLOBAL(flags) & rwMATRIXINTERNALIDENTITY))
    {
        RwMatrixCopy(dst, src);
        return dst;
    }

    if ((rwMatrixGetFlags(src) & rwMATRIXTYPEMASK) == rwMATRIXTYPEORTHONORMAL)
    {
        MatrixInvertOrthoNormalized(dst, src);
    }
    else
    {
        MatrixInvertGeneric(dst, src);
    }

    return dst;
}

RwMatrix* RwMatrixScale(RwMatrix* matrix, const RwV3d* scale, RwOpCombineType combineOp)
{
    switch (combineOp)
    {
    case rwCOMBINEREPLACE:
        RwMatrixSetIdentityMacro(matrix);
        matrix->right.x = scale->x;
        matrix->up.y = scale->y;
        matrix->at.z = scale->z;
        break;
    case rwCOMBINEPRECONCAT:
        RwV3dScaleMacro(&matrix->right, &matrix->right, scale->x);
        RwV3dScaleMacro(&matrix->up, &matrix->up, scale->y);
        RwV3dScaleMacro(&matrix->at, &matrix->at, scale->z);
        break;
    case rwCOMBINEPOSTCONCAT:
        matrix->right.x *= scale->x;
        matrix->right.y *= scale->y;
        matrix->right.z *= scale->z;
        matrix->up.x *= scale->x;
        matrix->up.y *= scale->y;
        matrix->up.z *= scale->z;
        matrix->at.x *= scale->x;
        matrix->at.y *= scale->y;
        matrix->at.z *= scale->z;
        matrix->pos.x *= scale->x;
        matrix->pos.y *= scale->y;
        matrix->pos.z *= scale->z;
        break;
    default:
        RWERROR((E_RW_BADPARAM, "Invalid combination type"));
        matrix = NULL;
        break;
    }

    RwMatrixUpdate(matrix);

    return matrix;
}

RwMatrix* RwMatrixTranslate(RwMatrix* matrix, const RwV3d* translation, RwOpCombineType combineOp)
{
    switch (combineOp)
    {
    case rwCOMBINEREPLACE:
        RwMatrixSetIdentityMacro(matrix);
        matrix->pos.x = translation->x;
        matrix->pos.y = translation->y;
        matrix->pos.z = translation->z;
        break;
    case rwCOMBINEPRECONCAT:
        matrix->pos.x += translation->x * matrix->right.x + translation->y * matrix->up.x +
                         translation->z * matrix->at.x;
        matrix->pos.y += translation->x * matrix->right.y + translation->y * matrix->up.y +
                         translation->z * matrix->at.y;
        matrix->pos.z += translation->x * matrix->right.z + translation->y * matrix->up.z +
                         translation->z * matrix->at.z;
        break;
    case rwCOMBINEPOSTCONCAT:
        RwV3dAddMacro(&matrix->pos, &matrix->pos, translation);
        break;
    default:
        RWERROR((E_RW_BADPARAM, "Invalid combination type"));
        matrix = NULL;
        break;
    }

    rwMatrixSetFlags(matrix, rwMatrixGetFlags(matrix) & ~rwMATRIXINTERNALIDENTITY);

    return matrix;
}

RwMatrix* RwMatrixTransform(RwMatrix* matrix, const RwMatrix* transform, RwOpCombineType combineOp)
{
    switch (combineOp)
    {
    case rwCOMBINEREPLACE:
        RwMatrixCopy(matrix, transform);
        break;
    case rwCOMBINEPRECONCAT:
    {
        RwMatrix mTmp;

        RwMatrixMultiply(&mTmp, transform, matrix);
        RwMatrixCopy(matrix, &mTmp);
        break;
    }
    case rwCOMBINEPOSTCONCAT:
    {
        RwMatrix mTmp;

        RwMatrixMultiply(&mTmp, matrix, transform);
        RwMatrixCopy(matrix, &mTmp);
        break;
    }
    default:
        RWERROR((E_RW_BADPARAM, "Invalid combination type"));
        matrix = NULL;
        break;
    }

    return matrix;
}
