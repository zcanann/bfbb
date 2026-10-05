#include "xMath3.h"
#include "xParCmd.h"
// Diagnostic reachability only: compile the complete real matrix API and its
// real math/particle dependencies. This entry and runtime code are excluded
// from source coverage; original identities and extents are reviewed separately.
extern "C" F32 __cdecl xbox_source_entry(
    U32 which, xMat4x3* out, const xMat4x3* a, const xMat4x3* b, xVec3* v,
    xQuat* q, const xQuat* qa, const xQuat* qb, xBox* box, const xBox* box2,
    const xBox* box3, const xCapsule* cap, xIsect* hit,
    F32 t, F32 u, F32 w, F32 angle)
{
    xParCmdInit();
    switch (which)
    {
    case 0:
        xMath3Init();
        break;
    case 1:
        xLine3VecDist2(&a->pos,&b->pos,v,hit);
        break;
    case 2:
        return xPointInBox(box,v);
    case 3:
        xBoxInitBoundOBB(box,box2,a);
        break;
    case 4:
        xBoxInitBoundCapsule(box,cap);
        break;
    case 5:
        xBoxFromCone(*box,a->pos,b->pos,t,u,w);
        break;
    case 6:
        xMat3x3Normalize(out,a);
        break;
    case 7:
        xMat3x3GetEuler(a,v);
        break;
    case 8:
        xMat4x3MoveLocalRight(out,t);
        break;
    case 9:
        xMat4x3MoveLocalUp(out,t);
        break;
    case 10:
        xMat4x3MoveLocalAt(out,t);
        break;
    case 11:
        return xMat3x3LookVec(out,v);
    case 12:
        xMat3x3Euler(out,v);
        break;
    case 13:
        xMat3x3Euler(out,t,u,w);
        break;
    case 14:
        xMat3x3RotC(out,t,u,w,angle);
        break;
    case 15:
        xMat3x3RotX(out,t);
        break;
    case 16:
        xMat3x3RotY(out,t);
        break;
    case 17:
        xMat3x3RotZ(out,t);
        break;
    case 18:
        xMat3x3ScaleC(out,t,u,w);
        break;
    case 19:
        xMat3x3RMulRotY(out,a,t);
        break;
    case 20:
        xMat3x3Transpose(out,a);
        break;
    case 21:
        xMat3x3Mul(out,a,b);
        break;
    case 22:
        xMat3x3LMulVec(v,a,&b->pos);
        break;
    case 23:
        xMat3x3Tolocal(v,a,&b->pos);
        break;
    case 24:
        xMat4x3Rot(out,v,t,&a->pos);
        break;
    case 25:
        xMat4x3Mul(out,a,b);
        break;
    case 26:
        xQuatFromMat(q,a);
        break;
    case 27:
        xQuatFromAxisAngle(q,v,t);
        break;
    case 28:
        xQuatToMat(qa,out);
        break;
    case 29:
        xQuatToAxisAngle(qa,v,&out->pos.x);
        break;
    case 30:
        return xQuatNormalize(q,qa);
    case 31:
        xQuatSlerp(q,qa,qb,t);
        break;
    case 32:
        xQuatMul(q,qa,qb);
        break;
    case 33:
        xQuatDiff(q,qa,qb);
        break;
    case 34:
        xBoxUnion(*box,*box2,*box3);
        break;
    case 35:
        xBoxFromCircle(*box,a->pos,b->pos,t);
        break;
    case 36:
        xQuatSMul(q,qa,t);
        break;
    case 37:
        return xQuatLength2(qa);
    case 38:
        xQuatAdd(q,qa,qb);
        break;
    }
    return 0.0f;
}
