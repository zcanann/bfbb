#include <rwsdk/rwcore.h>

typedef struct rwMatrixGlobals rwMatrixGlobals;
struct rwMatrixGlobals
{
    RwFreeList* matrixFreeList;
    RwUInt32 flags;
    rwMatrixMultFn MultMatrix;
    RwMatrixTolerance tolerance;
};

static RwInt32 matrixModule = 0;

#define RWMATRIXGLOBAL(var) (RWPLUGINOFFSET(rwMatrixGlobals, RwEngineInstance, matrixModule)->var)

RwMatrix* RwMatrixUpdate(RwMatrix* matrix)
{
    rwMatrixSetFlags(matrix,
                     rwMatrixGetFlags(matrix) & ~(rwMATRIXINTERNALIDENTITY | rwMATRIXTYPEMASK));

    return matrix;
}

RwBool RwEngineSetMatrixTolerances(const RwMatrixTolerance* const tolerance)
{
    RWMATRIXGLOBAL(tolerance) = *tolerance;

    return TRUE;
}

RwReal _rwMatrixDeterminant(const RwMatrix* matrix)
{
    return (matrix->up.y * matrix->at.z - matrix->up.z * matrix->at.y) * matrix->right.x +
           (matrix->up.z * matrix->at.x - matrix->up.x * matrix->at.z) * matrix->right.y +
           (matrix->up.x * matrix->at.y - matrix->up.y * matrix->at.x) * matrix->right.z;
}

RwReal _rwMatrixOrthogonalError(const RwMatrix* matrix)
{
    RwReal a;
    RwReal b;
    RwReal c;

    a = RwV3dDotProductMacro(&matrix->up, &matrix->at);
    b = RwV3dDotProductMacro(&matrix->at, &matrix->right);
    c = RwV3dDotProductMacro(&matrix->right, &matrix->up);

    return a * a + b * b + c * c;
}

RwReal _rwMatrixNormalError(const RwMatrix* matrix)
{
    RwReal a;
    RwReal b;
    RwReal c;

    a = RwV3dDotProductMacro(&matrix->right, &matrix->right) - ((RwReal)1.0);
    b = RwV3dDotProductMacro(&matrix->up, &matrix->up) - ((RwReal)1.0);
    c = RwV3dDotProductMacro(&matrix->at, &matrix->at) - ((RwReal)1.0);

    return a * a + b * b + c * c;
}
