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

#define E_RW_NOMEM 0x80000013
#define E_RW_ZEROLENGTH 0x19

#define rwSQRTTABLESIZE 4096
#define rwSQRTTABLEHALF (rwSQRTTABLESIZE / 2)

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef RwV3d* (*rwVectorMultFn)(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints,
                                 const RwMatrix* matrix);

typedef struct rwVectorGlobals rwVectorGlobals;
struct rwVectorGlobals
{
    RwSplitBits* SqrtTab;
    RwSplitBits* InvSqrtTab;
    rwVectorMultFn multPoint;
    rwVectorMultFn multVector;
};

#define RWVECTORGLOBAL(var)                                                                        \
    (RWPLUGINOFFSET(rwVectorGlobals, RwEngineInstance, vectorModule.globalsOffset)->var)

static RwModuleInfo vectorModule;

/* Single precision square root, refined from the hardware estimate */
static __inline RwReal VectorSqrt(RwReal x)
{
    volatile RwReal y;

    if (x > 0.0f)
    {
        double guess = __frsqrte(x);
        guess = 0.5 * guess * -(guess * guess * x - 3.0);
        guess = 0.5 * guess * -(guess * guess * x - 3.0);
        guess = 0.5 * guess * -(guess * guess * x - 3.0);
        y = x * guess;
        return y;
    }

    return x;
}

static void SqrtTableDestroy(void)
{
    if (RWVECTORGLOBAL(SqrtTab))
    {
        RwFree(RWVECTORGLOBAL(SqrtTab));
        RWVECTORGLOBAL(SqrtTab) = NULL;
    }
}

static RwBool SqrtTableCreate(void)
{
    RwUInt32 i;
    RwSplitBits* SqrtTab;
    RwSplitBits* SqrtTab1to2;
    RwSplitBits* SqrtTab2to4;
    RwSplitBits spIn;
    RwSplitBits spOut;

    SqrtTab = (RwSplitBits*)RwMalloc(sizeof(RwSplitBits) * rwSQRTTABLESIZE);
    if (!SqrtTab)
    {
        RWERROR((E_RW_NOMEM, sizeof(RwSplitBits) * rwSQRTTABLESIZE));
        return FALSE;
    }

    SqrtTab1to2 = SqrtTab + rwSQRTTABLEHALF;
    SqrtTab2to4 = SqrtTab;

    spIn.nReal = 1.0f;

    for (i = 0; i < rwSQRTTABLEHALF; i++)
    {
        spOut.nReal = VectorSqrt(spIn.nReal);
        spOut.nUInt = spOut.nInt - 0x1FC00000;
        SqrtTab1to2[i].nUInt = spOut.nUInt;
        spIn.nInt += 0x1000;
    }

    for (i = 0; i < rwSQRTTABLEHALF; i++)
    {
        spOut.nReal = VectorSqrt(spIn.nReal);
        spOut.nUInt = spOut.nInt - 0x20000000;
        SqrtTab2to4[i].nUInt = spOut.nUInt;
        spIn.nInt += 0x1000;
    }

    RWVECTORGLOBAL(SqrtTab) = SqrtTab;

    return TRUE;
}

static void InvSqrtTableDestroy(void)
{
    if (RWVECTORGLOBAL(InvSqrtTab))
    {
        RwFree(RWVECTORGLOBAL(InvSqrtTab));
        RWVECTORGLOBAL(InvSqrtTab) = NULL;
    }
}

static RwBool InvSqrtTableCreate(void)
{
    RwUInt32 i;
    RwSplitBits* InvSqrtTab;
    RwSplitBits* InvSqrtTab1to2;
    RwSplitBits* InvSqrtTab2to4;
    RwSplitBits spIn;
    RwSplitBits spOut;

    InvSqrtTab = (RwSplitBits*)RwMalloc(sizeof(RwSplitBits) * rwSQRTTABLESIZE);
    if (!InvSqrtTab)
    {
        RWERROR((E_RW_NOMEM, sizeof(RwSplitBits) * rwSQRTTABLESIZE));
        return FALSE;
    }

    InvSqrtTab1to2 = InvSqrtTab + rwSQRTTABLEHALF;
    InvSqrtTab2to4 = InvSqrtTab;

    spIn.nReal = 1.0f;

    for (i = 0; i < rwSQRTTABLEHALF; i++)
    {
        spOut.nReal = 1.0f / VectorSqrt(spIn.nReal);
        spOut.nUInt = spOut.nInt - 0x20000000;
        InvSqrtTab1to2[i].nUInt = spOut.nUInt;
        spIn.nInt += 0x1000;
    }

    for (i = 0; i < rwSQRTTABLEHALF; i++)
    {
        spOut.nReal = 1.0f / VectorSqrt(spIn.nReal);
        spOut.nUInt = spOut.nInt - 0x1FC00000;
        InvSqrtTab2to4[i].nUInt = spOut.nUInt;
        spIn.nInt += 0x1000;
    }

    RWVECTORGLOBAL(InvSqrtTab) = InvSqrtTab;

    return TRUE;
}

static RwV3d* VectorMultPoint(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints,
                              const RwMatrix* matrix)
{
    RwV3d* result = pointsOut;
    RwReal z;
    RwReal y;
    RwReal x;
    RwReal outX;
    RwReal outY;
    RwReal outZ;

    while (--numPoints >= 0)
    {
        x = pointsIn->x;
        y = pointsIn->y;
        z = pointsIn->z;

        pointsIn++;

        outX = x * matrix->right.x;
        outY = x * matrix->right.y;
        outZ = x * matrix->right.z;

        outX += y * matrix->up.x;
        outY += y * matrix->up.y;
        outZ += y * matrix->up.z;

        outX += z * matrix->at.x;
        outY += z * matrix->at.y;
        outZ += z * matrix->at.z;

        pointsOut->x = outX + matrix->pos.x;
        pointsOut->y = outY + matrix->pos.y;
        pointsOut->z = outZ + matrix->pos.z;

        pointsOut++;
    }

    return result;
}

static RwV3d* VectorMultVector(RwV3d* pointsOut, const RwV3d* pointsIn, RwInt32 numPoints,
                               const RwMatrix* matrix)
{
    RwV3d* result = pointsOut;
    RwReal z;
    RwReal y;
    RwReal x;
    RwReal outX;
    RwReal outY;
    RwReal outZ;

    while (--numPoints >= 0)
    {
        x = pointsIn->x;
        y = pointsIn->y;
        z = pointsIn->z;

        pointsIn++;

        outX = x * matrix->right.x;
        outY = x * matrix->right.y;
        outZ = x * matrix->right.z;

        outX += y * matrix->up.x;
        outY += y * matrix->up.y;
        outZ += y * matrix->up.z;

        pointsOut->x = outX + z * matrix->at.x;
        pointsOut->y = outY + z * matrix->at.y;
        pointsOut->z = outZ + z * matrix->at.z;

        pointsOut++;
    }

    return result;
}

RwReal RwV3dNormalize(RwV3d* out, const RwV3d* in)
{
    RwReal length;
    RwReal length2;
    RwReal recip;

    length2 = RwV3dDotProductMacro(in, in);
    length = _rwSqrt(length2);
    recip = _rwInvSqrt(length2);

    RwV3dScaleMacro(out, in, recip);

    if (length <= 0.0f)
    {
        RWERROR((E_RW_ZEROLENGTH));
    }

    return length;
}

RwReal RwV3dLength(const RwV3d* in)
{
    RwReal length;

    length = RwV3dDotProductMacro(in, in);

    return _rwSqrt(length);
}

RwReal _rwSqrt(const RwReal num)
{
    RwSplitBits result;

    result.nReal = num;

    if (result.nUInt)
    {
        const RwSplitBits* const SqrtTab = RWVECTORGLOBAL(SqrtTab);

        result.nInt += 0x800;
        result.nInt = ((result.nInt >> 1) & 0x3FC00000) + SqrtTab[(result.nInt >> 12) & 0xFFF].nInt;
    }

    return result.nReal;
}

RwReal _rwInvSqrt(const RwReal num)
{
    RwSplitBits result;

    result.nReal = num;

    if (result.nUInt)
    {
        const RwSplitBits* const InvSqrtTab = RWVECTORGLOBAL(InvSqrtTab);

        result.nInt += 0x800;
        result.nInt =
            ((~result.nInt >> 1) & 0x3FC00000) + InvSqrtTab[(result.nInt >> 12) & 0xFFF].nInt;
    }

    return result.nReal;
}

asm RwV3d* RwV3dTransformPoints(register RwV3d* pointsOut, register const RwV3d* pointsIn,
                                register RwInt32 numPoints, register const RwMatrix* matrix)
{
    nofralloc
    mtctr numPoints
    psq_l f0, 0x0(matrix), 0, 0
    psq_l f1, 0x8(matrix), 1, 0
    subi r5, pointsOut, 4
    psq_l f6, 0x30(matrix), 0, 0
    psq_l f7, 0x38(matrix), 1, 0
    psq_l f8, 0x0(pointsIn), 0, 0
    psq_lu f9, 0x8(pointsIn), 1, 0
    psq_l f2, 0x10(matrix), 0, 0
    psq_l f3, 0x18(matrix), 1, 0
    psq_l f4, 0x20(matrix), 0, 0
    psq_l f5, 0x28(matrix), 1, 0
loop:
    ps_madds0 f10, f0, f8, f6
    ps_madds0 f11, f1, f8, f7
    ps_madds1 f10, f2, f8, f10
    ps_madds1 f11, f3, f8, f11
    psq_lu f8, 0x4(pointsIn), 0, 0
    ps_madds0 f10, f4, f9, f10
    ps_madds0 f11, f5, f9, f11
    psq_lu f9, 0x8(pointsIn), 1, 0
    psq_stu f10, 0x4(r5), 0, 0
    psq_stu f11, 0x8(r5), 1, 0
    bdnz loop
    blr
}

asm RwV3d* RwV3dTransformVectors(register RwV3d* vectorsOut, register const RwV3d* vectorsIn,
                                 register RwInt32 numPoints, register const RwMatrix* matrix)
{
    nofralloc
    mtctr numPoints
    psq_l f0, 0x0(matrix), 0, 0
    psq_l f1, 0x8(matrix), 1, 0
    subi r5, vectorsOut, 4
    psq_l f6, 0x0(vectorsIn), 0, 0
    psq_lu f7, 0x8(vectorsIn), 1, 0
    psq_l f2, 0x10(matrix), 0, 0
    psq_l f3, 0x18(matrix), 1, 0
    psq_l f4, 0x20(matrix), 0, 0
    psq_l f5, 0x28(matrix), 1, 0
loop:
    ps_muls0 f10, f0, f6
    ps_muls0 f11, f1, f6
    ps_madds1 f10, f2, f6, f10
    ps_madds1 f11, f3, f6, f11
    psq_lu f6, 0x4(vectorsIn), 0, 0
    ps_madds0 f10, f4, f7, f10
    ps_madds0 f11, f5, f7, f11
    psq_lu f7, 0x8(vectorsIn), 1, 0
    psq_stu f10, 0x4(r5), 0, 0
    psq_stu f11, 0x8(r5), 1, 0
    bdnz loop
    blr
}

void* _rwVectorClose(void* instance, RwInt32 offset, RwInt32 size)
{
    InvSqrtTableDestroy();
    SqrtTableDestroy();

    vectorModule.numInstances--;

    return instance;
}

void* _rwVectorOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    vectorModule.globalsOffset = offset;

    RWVECTORGLOBAL(multPoint) = VectorMultPoint;
    RWVECTORGLOBAL(multVector) = VectorMultVector;

    if (!SqrtTableCreate())
    {
        return NULL;
    }

    if (!InvSqrtTableCreate())
    {
        return NULL;
    }

    vectorModule.numInstances++;

    return instance;
}
