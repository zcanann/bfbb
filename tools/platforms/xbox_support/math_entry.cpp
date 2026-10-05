#include "xMath.h"
// Diagnostic reachability for the complete real math API. Host/runtime excluded.
extern "C" F32 __cdecl xbox_source_entry(
    U32 which, const char* text, U32 seed, F32 a, F32 b, F32 c, F32 d,
    F32* x1, F32* x2, F32* x3, xFuncPiece* piece, xFuncPiece* shift,
    xFuncPiece** iterator)
{
    switch (which)
    {
    case 0:
        xMathInit();
        break;
    case 1:
        xMathExit();
        break;
    case 2:
        return xatof(text);
    case 3:
        xsrand(seed);
        break;
    case 4:
        return xrand();
    case 5:
        return xurand();
    case 6:
        return xMathSolveQuadratic(a, b, c, x1, x2);
    case 7:
        return xMathSolveCubic(a, b, c, d, x1, x2, x3);
    case 8:
        return xAngleClamp(a);
    case 9:
        return xAngleClampFast(a);
    case 10:
        return xDangleClamp(a);
    case 11:
        xAccelMove(*x1, *x2, a, b, c, d);
        break;
    case 12:
        return xAccelMoveTime(a, b, c, d);
    case 13:
        xAccelMove(*x1, *x2, a, b, c);
        break;
    case 14:
        xAccelStop(*x1, *x2, a, b);
        break;
    case 15:
        return xFuncPiece_Eval(piece, a, iterator);
    case 16:
        xFuncPiece_EndPoints(piece, a, b, c, d);
        break;
    case 17:
        xFuncPiece_ShiftPiece(shift, piece, a);
        break;
    }
    return 0;
}
