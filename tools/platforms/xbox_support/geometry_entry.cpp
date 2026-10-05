#include "iMath3.h"

// Diagnostic reachability only; all called routines are real whole-TU code.
// This entry and linked runtime support are excluded from source coverage.
extern "C" F32 __cdecl xbox_source_entry(
    U32 which, xSphere* s, const xSphere* t, const xVec3* v, xIsect* hit,
    const xRay3* ray, const xCylinder* c, xBox* box, const xBox* b2)
{
    switch (which)
    {
    case 0:
        iMath3Init();
        break;
    case 1:
        iSphereIsectVec(s, v, hit);
        break;
    case 2:
        iSphereIsectRay(s, ray, hit);
        break;
    case 3:
        iSphereIsectSphere(s, t, hit);
        break;
    case 4:
        iSphereInitBoundVec(s, v);
        break;
    case 5:
        iSphereBoundVec(s, t, v);
        break;
    case 6:
        iCylinderIsectVec(c, v, hit);
        break;
    case 7:
        iBoxVecDist(box, v, hit);
        break;
    case 8:
        iBoxIsectVec(box, v, hit);
        break;
    case 9:
        iBoxIsectRay(box, ray, hit);
        break;
    case 10:
        iBoxIsectSphere(box, s, hit);
        break;
    case 11:
        iBoxInitBoundVec(box, v);
        break;
    case 12:
        iBoxBoundVec(box, b2, v);
        break;
    }
    return 0.0f;
}
