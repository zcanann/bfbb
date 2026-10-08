// Expanding xSndPlay3D here adds a 0.25f .sdata2 literal that retail does not
// have; it shifts this unit's pool and costs fizzicalSlack. See zEnt.h.
#define XSNDPLAY3D_OUT_OF_LINE

#include "zLasso.h"

#include "xMath3.h"
#include "xMathInlines.h"
#include "iAnim.h"
#include "iMath.h"
#include "iModel.h"
#include "xstransvc.h"
#include "iParMgr.h"
#if defined(PS2)
#include <rwim3d.h>
#endif

#include <types.h>

static RwRaster* sLassoRaster;
static U32 sNumGuideLists;
static zLassoGuide* sCurrentGuide;
static RxObjSpace3DVertex* lnverts;

static zLassoGuideList sGuideList[64];

static S32 negativeHondaX = 1;

static void fizzicalRadius(zLasso* lasso, f32 arg1, xVec3* arg2);
static void fizzicalCenter(zLasso* lasso, f32 arg1, xVec3* arg2);
static void fizzicalNormal(zLasso* lasso, f32 arg1, xVec3* arg2);
static void fizzicalHonda(zLasso* lasso, f32 arg1, xVec3* arg2);
static void nonfizzicalHonda(zLasso* lasso, f32 arg1, xVec3* arg2);
static void fizzicalSlack(zLasso* lasso, f32 arg1, xVec3* arg2);
static void initVertMap(zLassoGuide* guide);
static void bakeMorphAnim(RpGeometry* geom, void* anim);
static void vec2vecMat(xMat4x3* m, xVec3* v1, xVec3* v2);

inline void xMat4x3RotC(xMat4x3* m, F32 f1, F32 f2, F32 f3, F32 f4);
inline void xMat4x3Rot(xMat4x3* m, const xVec3* v, F32 f);

void zLasso_Init(zLasso* lasso, xModelInstance* model, F32 x, F32 y, F32 z)
{
    if (sLassoRaster == NULL)
    {
        RwTexture* tempTexture = (RwTexture*)xSTFindAsset(xStrHash("rope"), NULL);
        if (tempTexture != NULL)
        {
            sLassoRaster = tempTexture->raster;
        }
        else
        {
            sLassoRaster = NULL;
        }
    }

    iModelTagSetup(&lasso->tag, model->Data, x, y, z);

    lasso->model = model;
    lnverts = gRenderArr.m_vertex;
}

void zLasso_AddGuide(xEnt* ent, xAnimState* lassoAnim, xModelInstance* lassoModel)
{
    U32 i;
    S32 givenSlot = -1;

    for (i = 0; i < sNumGuideLists; i++)
    {
        if (sGuideList[i].target == ent)
        {
            givenSlot = i;
            break;
        }
    }

    if (givenSlot == -1)
    {
        givenSlot = sNumGuideLists++;
        sGuideList[givenSlot].target = ent;
        sGuideList[givenSlot].numGuides = 0;
    }

    for (i = 0; i < sGuideList[givenSlot].numGuides; i++)
    {
        if (sGuideList[givenSlot].guide[i].lassoAnim == lassoAnim)
        {
            return;
        }
    }

    i = sGuideList[givenSlot].numGuides++;
    sGuideList[givenSlot].guide[i].lassoAnim = lassoAnim;
    sGuideList[givenSlot].guide[i].poly = lassoModel;
    initVertMap(&sGuideList[givenSlot].guide[i]);

    bakeMorphAnim(lassoModel->Data->geometry, *lassoAnim->Data->RawData);
}

void zLasso_SetGuide(xEnt* ent, xAnimState* lassoAnim)
{
    sCurrentGuide = NULL;
    if (ent == NULL || lassoAnim == NULL)
    {
        return;
    }

    U32 i;
    for (i = 0; i < sNumGuideLists; i++)
    {
        if (sGuideList[i].target == ent)
        {
            break;
        }
    }

    if (i < sNumGuideLists)
    {
        U32 j;
        for (j = 0; j < sGuideList[i].numGuides; j++)
        {
            if (sGuideList[i].guide[j].lassoAnim == lassoAnim)
            {
                break;
            }
        }

        if (j < sGuideList[i].numGuides)
        {
            sCurrentGuide = &sGuideList[i].guide[j];
        }
    }

    if (sCurrentGuide == NULL)
    {
        sCurrentGuide = &sGuideList->guide[0];
    }
}

void zLasso_InterpToGuide(zLasso* lasso)
{
    xVec3 rad1;
    xVec3 rad2;

    RpGeometry* geom;
    RwV3d* v;
    S32 numVerts;

    if (sCurrentGuide != NULL)
    {
        geom = sCurrentGuide->poly->Data->geometry;
        v = geom->morphTarget->verts;
        numVerts = geom->numTriangles;
        xVec3Init(&lasso->tgCenter, 0.0f, 0.0f, 0.0f);

        for (S32 i = 0; i < numVerts; i++)
        {
            xVec3AddTo(&lasso->tgCenter, (xVec3*)v + sCurrentGuide->vertMap[i]);
        }

        xVec3SMul(&lasso->tgCenter, &lasso->tgCenter, 1.0f / (f32)numVerts);
        xVec3Sub(&rad1, (xVec3*)v + sCurrentGuide->vertMap[0], &lasso->tgCenter);
        xVec3Sub(&rad2, (xVec3*)v + sCurrentGuide->vertMap[1], &lasso->tgCenter);

        lasso->tgRadius = xVec3Normalize(&rad1, &rad1);

        xVec3Cross(&lasso->tgNormal, &rad1, &rad2);
        xVec3Normalize(&lasso->tgNormal, &lasso->tgNormal);

        if (lasso->tgNormal.y < 0.0f)
        {
            xVec3Inv(&lasso->tgNormal, &lasso->tgNormal);
        }

        xMat4x3Toworld(&lasso->tgCenter, (xMat4x3*)sCurrentGuide->poly->Mat, &lasso->tgCenter);
    }
}

void zLasso_Render(zLasso* lasso)
{
    xVec3 pts[16];
    xVec3 vtx[6];
    xVec3 strand[3];
    xMat4x3 coilMat;
    xMat4x3 tailMat;
    xVec3 delta;
    xVec3 seg;
    xVec3 closest;
    xVec3 best;
    xVec3 target;
    xVec3 perp;
    xVec3 dir;
    xVec3 side;
    xVec3 hondaPos;
    xVec3 step;
    xVec3 cur;
    xVec3 pos;
    xVec3 center;
    xVec3 axis;
    xVec3 tan1;
    xVec3 tan0;

    U8 useGuide;
    S32 i;
    RwV3d* v1;
    RwV3d* v0;
    S32 j;
    S32 k;
    S32 strandIdx;
    S32 numPts;
    RxObjSpace3DVertex* vp;
    S32 curRing;
    S32 prevRing;
    S32 pending;
    U32 numVerts;
    S32 numPrims;
    F32 u;
    F32 interp;
    S32 numMorphs;
    RpGeometry* geom;
    S32 mIndx1;
    F32 bestDist;
    F32 segLen;
    F32 t;
    F32 ropeLen;
    F32 ropeDist;
    F32 du;
    F32 travelled;
    F32 stepLen;
    F32 xDisp;
    F32 yDisp;
    F32 ang;
    F32 third;
    F32 twoThirds;
    F32 c0;
    F32 c1;
    F32 c2;
    F32 s0;
    F32 s1;
    F32 s2;
    F32 v;
    S32 jc;
    S32 j1;
    S32 jm;
    S32 i0;
    S32 i1;
    F32 mu;
    F32 mdu;

    useGuide = ((((lasso->flags & 0x800) != 0) && ((lasso->flags & 0x4000) != 0)) ||
                (((lasso->flags & 0x800) == 0) && ((lasso->flags & 0x2000) != 0)));

    if (useGuide)
    {
        geom = sCurrentGuide->poly->Data->geometry;
        numMorphs = geom->numMorphTargets;
        mIndx1 = (S32)(30.0f * sCurrentGuide->poly->Anim->Single->Time);
        interp = 30.0f * sCurrentGuide->poly->Anim->Single->Time - mIndx1;
        numPts = geom->numTriangles;
        ropeLen = 0.0f;
        mIndx1 = mIndx1 % numMorphs;
        const RpMorphTarget& firstMorph = geom->morphTarget[mIndx1];
        const RpMorphTarget& nextMorph = geom->morphTarget[(mIndx1 + 1) % numMorphs];
        v0 = firstMorph.verts;
        v1 = nextMorph.verts;

        for (i = 0; i < numPts; i++)
        {
            xVec3SMul(&pts[i], (xVec3*)(v0 + sCurrentGuide->vertMap[i]), 1.0f - interp);
            xVec3AddScaled(&pts[i], (xVec3*)(v1 + sCurrentGuide->vertMap[i]), interp);
            xMat4x3Toworld(&pts[i], (xMat4x3*)sCurrentGuide->poly->Mat, &pts[i]);
        }

        xVec3Add(&target, &lasso->lastRefs[lasso->reindex[0]], &lasso->anchor);
        xVec3Sub(&delta, &target, &pts[0]);
        xVec3Copy(&best, &pts[0]);
        bestDist = xVec3Dot(&delta, &delta);

        for (i = 0; i < numPts; i++)
        {
            xVec3Sub(&seg, &pts[(i + 1) % numPts], &pts[i]);
            xVec3Sub(&delta, &target, &pts[i]);
            segLen = xVec3Normalize(&seg, &seg);
            t = xVec3Dot(&delta, &seg);

            if (t < 0.0f)
            {
                xVec3Copy(&closest, &pts[i]);
            }
            else if (t > segLen)
            {
                xVec3Copy(&closest, &pts[(i + 1) % numPts]);
            }
            else
            {
                xVec3SMul(&closest, &seg, t);
                xVec3AddTo(&closest, &pts[i]);
            }

            xVec3Sub(&delta, &target, &closest);
            segLen = xVec3Dot(&delta, &delta);
            if (segLen < bestDist)
            {
                xVec3Copy(&best, &closest);
                bestDist = segLen;
            }

            ropeLen += xVec3Dist(&pts[i], &pts[(i + 1) % numPts]);
        }

        xVec3Sub(&lasso->honda, &best, &lasso->anchor);
    }
    else
    {
        ropeLen = 2.0f * PI * lasso->crRadius;
    }

    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, sLassoRaster);

    curRing = 3;
    prevRing = 0;
    numPrims = 0;
    travelled = 0.0f;

    xVec3Sub(&dir, &lasso->lastRefs[lasso->reindex[0]], &lasso->honda);
    ropeDist = xVec3Normalize(&dir, &dir);
    lasso->lastDist = lasso->currDist;
    lasso->currDist = ropeDist;

    perp.x = dir.y - dir.z;
    perp.y = dir.z - dir.x;
    perp.z = dir.x - dir.y;
    xVec3Normalize(&perp, &perp);
    xVec3Cross(&side, &perp, &dir);

    third = 2.0f * PI / 3.0f;
    twoThirds = 4.0f * PI / 3.0f;
    for (strandIdx = 0; strandIdx < 1; strandIdx++)
    {
        ang = 2.0f * (PI * strandIdx) / 3.0f;
        c0 = icos(ang);
        c1 = icos(ang + third);
        c2 = icos(ang + twoThirds);
        s0 = isin(ang);
        s1 = isin(ang + third);
        s2 = isin(ang + twoThirds);

        xVec3SMul(&strand[strandIdx], &perp, 0.025f * s0);
        xVec3AddScaled(&strand[strandIdx], &side, 0.025f * c0);
        strandIdx++;
        xVec3SMul(&strand[strandIdx], &perp, 0.025f * s1);
        xVec3AddScaled(&strand[strandIdx], &side, 0.025f * c1);
        strandIdx++;
        xVec3SMul(&strand[strandIdx], &perp, 0.025f * s2);
        xVec3AddScaled(&strand[strandIdx], &side, 0.025f * c2);
    }

    xVec3Add(&hondaPos, &lasso->honda, &lasso->anchor);
    xVec3Copy(&cur, &hondaPos);

    for (i = 0; i < 3; i++)
    {
        xVec3Add(&vtx[i], &strand[i], &hondaPos);
    }

    stepLen = ropeDist / 43.0f;
    if (stepLen < 0.2f)
    {
        stepLen = 0.2f;
    }
    xVec3SMul(&step, &dir, stepLen);

    mu = ropeLen;
    vp = lnverts;
    mdu = stepLen;
    v = 0.0f;
    numVerts = 0;

    while (travelled < ropeDist && numVerts + 10 <= 480)
    {
        if (mu > 1.0f)
        {
            mu -= 1.0f;
        }

        travelled += stepLen;

        if (travelled < ropeDist)
        {
            xVec3AddTo(&cur, &step);
            xVec3Copy(&pos, &cur);
            ang = PI * (0.75f * travelled);
            xDisp = (ropeDist - travelled) * (travelled * (lasso->crSlack * isin(ang))) / (ropeDist * ropeDist);
            yDisp = (ropeDist - travelled) * (travelled * (lasso->crSlack * icos(ang))) / (ropeDist * ropeDist);
            xVec3AddScaled(&pos, &perp, xDisp);
            xVec3AddScaled(&pos, &side, yDisp);
        }
        else
        {
            xVec3Add(&pos, &lasso->anchor, &lasso->lastRefs[lasso->reindex[0]]);
            mdu = ropeDist + (stepLen - travelled);
        }

        for (i = 0; i < 3; i++)
        {
            xVec3Add(&vtx[curRing + i], &strand[i], &pos);
        }

        i0 = prevRing + 2;
        RwIm3DVertexSetPos(&vp[0], vtx[i0].x, vtx[i0].y, vtx[i0].z);
        RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
        RwIm3DVertexSetUV(&vp[0], mu, 2.0f / 3.0f);
        i1 = curRing + 2;
        RwIm3DVertexSetPos(&vp[1], vtx[i1].x, vtx[i1].y, vtx[i1].z);
        RwIm3DVertexSetRGBA(&vp[1], 255, 255, 255, 255);
        RwIm3DVertexSetUV(&vp[1], mu + mdu, 2.0f / 3.0f);
        numVerts += 2;
        vp += 2;

        for (k = 0; k < 3; k++)
        {
            v = k;
                v *= (1.0f / 3.0f);
            RwIm3DVertexSetPos(&vp[0], vtx[prevRing + k].x, vtx[prevRing + k].y, vtx[prevRing + k].z);
            RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
            RwIm3DVertexSetUV(&vp[0], mu, v);
            RwIm3DVertexSetPos(&vp[1], vtx[curRing + k].x, vtx[curRing + k].y, vtx[curRing + k].z);
            RwIm3DVertexSetRGBA(&vp[1], 255, 255, 255, 255);
            RwIm3DVertexSetUV(&vp[1], mu + mdu, v);
            numVerts += 2;
            vp += 2;
        }

        prevRing = curRing;
        curRing = curRing ? 0 : 3;
        mu += mdu;
        numPrims++;
    }

    RwIm3DVertexSetPos(&vp[0], vtx[prevRing + 2].x, vtx[prevRing + 2].y, vtx[prevRing + 2].z);
    RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
    RwIm3DVertexSetUV(&vp[0], mu, v);
    numVerts += 1;
    vp += 1;
    pending = 1;

    if (!useGuide)
    {
        xVec3Sub(&perp, &lasso->honda, &lasso->crCenter);
        xVec3Cross(&side, &perp, &lasso->crNormal);
        xVec3Normalize(&axis, &side);
        vec2vecMat(&coilMat, &dir, &axis);

        for (i = 0; i < 3; i++)
        {
            xMat4x3Toworld(&strand[i], &coilMat, &strand[i]);
        }

        xMat4x3Rot(&coilMat, &lasso->crNormal, 2.0f * PI / 15.0f);

        prevRing = 0;
        curRing = 3;
        for (i = 0; i < 3; i++)
        {
            xVec3Add(&vtx[i], &strand[i], &hondaPos);
        }

        xVec3Sub(&cur, &lasso->honda, &lasso->crCenter);
        xVec3Add(&center, &lasso->crCenter, &lasso->anchor);

        du = 2.0f * (PI * lasso->crRadius) / 15.0f;
        u = 0.0f;

        for (j = 0; j < 15 && numVerts + 8 + pending <= 480; j++)
        {
            if (u > 1.0f)
            {
                u -= 1.0f;
            }

            xMat4x3Toworld(&cur, &coilMat, &cur);

            for (i = 0; i < 3; i++)
            {
                xMat4x3Toworld(&strand[i], &coilMat, &strand[i]);
                xVec3Add(&vtx[i + curRing], &strand[i], &cur);
                xVec3AddTo(&vtx[i + curRing], &center);
            }

            if (pending)
            {
                i0 = prevRing + 2;
                RwIm3DVertexSetPos(&vp[0], vtx[i0].x, vtx[i0].y, vtx[i0].z);
                RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
                RwIm3DVertexSetUV(&vp[0], u, 2.0f / 3.0f);
                pending = 0;
                numVerts += 1;
                vp += 1;
            }

            i0 = prevRing + 2;
            RwIm3DVertexSetPos(&vp[0], vtx[i0].x, vtx[i0].y, vtx[i0].z);
            RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
            RwIm3DVertexSetUV(&vp[0], u, 2.0f / 3.0f);
            i1 = curRing + 2;
            RwIm3DVertexSetPos(&vp[1], vtx[i1].x, vtx[i1].y, vtx[i1].z);
            RwIm3DVertexSetRGBA(&vp[1], 255, 255, 255, 255);
            RwIm3DVertexSetUV(&vp[1], u + du, 2.0f / 3.0f);
            numVerts += 2;
            vp += 2;

            for (k = 0; k < 3; k++)
            {
                v = k;
                v *= (1.0f / 3.0f);
                RwIm3DVertexSetPos(&vp[0], vtx[prevRing + k].x, vtx[prevRing + k].y, vtx[prevRing + k].z);
                RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
                RwIm3DVertexSetUV(&vp[0], u, v);
                RwIm3DVertexSetPos(&vp[1], vtx[curRing + k].x, vtx[curRing + k].y, vtx[curRing + k].z);
                RwIm3DVertexSetRGBA(&vp[1], 255, 255, 255, 255);
                RwIm3DVertexSetUV(&vp[1], u + du, v);
                numVerts += 2;
                vp += 2;
            }

            prevRing = curRing;
            curRing = curRing ? 0 : 3;
            u += du;
            numPrims++;
        }
    }
    else
    {
        xVec3Copy(&tan0, &dir);
        xVec3Sub(&tan1, &pts[1], &pts[numPts - 1]);
        xVec3Normalize(&tan1, &tan1);
        vec2vecMat(&tailMat, &tan0, &tan1);
        xVec3Copy(&tan1, &tan0);

        for (i = 0; i < 3; i++)
        {
            xMat4x3Toworld(&strand[i], &tailMat, &strand[i]);
        }

        prevRing = 0;
        curRing = 3;
        for (i = 0; i < 3; i++)
        {
            xVec3Add(&vtx[i], &strand[i], &pts[0]);
        }

        u = 0.0f;

        for (j = 1; j <= numPts && numVerts + 8 + pending <= 480; j++)
        {
            jc = j % numPts;
            j1 = (j + 1) % numPts;
            jm = (j - 1) % numPts;

            if (u > 1.0f)
            {
                u -= 1.0f;
            }

            xVec3Sub(&tan1, &pts[j1], &pts[jm]);
            xVec3Normalize(&tan1, &tan1);
            vec2vecMat(&tailMat, &tan0, &tan1);
            xVec3Copy(&tan0, &tan1);

            for (i = 0; i < 3; i++)
            {
                xMat4x3Toworld(&strand[i], &tailMat, &strand[i]);
                xVec3Add(&vtx[i + curRing], &strand[i], &pts[jc]);
            }

            du = xVec3Dist(&pts[jc], &pts[j1]);

            if (pending)
            {
                i0 = prevRing + 2;
                RwIm3DVertexSetPos(&vp[0], vtx[i0].x, vtx[i0].y, vtx[i0].z);
                RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
                RwIm3DVertexSetUV(&vp[0], u, 2.0f / 3.0f);
                pending = 0;
                numVerts += 1;
                vp += 1;
            }

            i0 = prevRing + 2;
            RwIm3DVertexSetPos(&vp[0], vtx[i0].x, vtx[i0].y, vtx[i0].z);
            RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
            RwIm3DVertexSetUV(&vp[0], u, 2.0f / 3.0f);
            i1 = curRing + 2;
            RwIm3DVertexSetPos(&vp[1], vtx[i1].x, vtx[i1].y, vtx[i1].z);
            RwIm3DVertexSetRGBA(&vp[1], 255, 255, 255, 255);
            RwIm3DVertexSetUV(&vp[1], u + du, 2.0f / 3.0f);
            numVerts += 2;
            vp += 2;

            for (k = 0; k < 3; k++)
            {
                v = k;
                v *= (1.0f / 3.0f);
                RwIm3DVertexSetPos(&vp[0], vtx[prevRing + k].x, vtx[prevRing + k].y, vtx[prevRing + k].z);
                RwIm3DVertexSetRGBA(&vp[0], 255, 255, 255, 255);
                RwIm3DVertexSetUV(&vp[0], u, v);
                RwIm3DVertexSetPos(&vp[1], vtx[curRing + k].x, vtx[curRing + k].y, vtx[curRing + k].z);
                RwIm3DVertexSetRGBA(&vp[1], 255, 255, 255, 255);
                RwIm3DVertexSetUV(&vp[1], u + du, v);
                numVerts += 2;
                vp += 2;
            }

            prevRing = curRing;
            curRing = curRing ? 0 : 3;
            u += du;
            numPrims++;
        }
    }

    if (numPrims > 0)
    {
        RwIm3DTransform(lnverts, numVerts, (RwMatrix*)&g_I3, 0x1b);
        RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP);
        RwIm3DEnd();
    }
}

void zLasso_Update(zLasso* lasso, xEnt* ent, F32 dt)
{
    xVec3 newPoint;
    iModelTagEval(ent->model->Data, &lasso->tag, lasso->model->Mat, &newPoint);
    xVec3Copy(&lasso->anchor, (xVec3*)&ent->model->Mat->pos);
    xVec3SubFrom(&newPoint, &lasso->anchor);

    if (!(lasso->flags & 0x800))
    {
        lasso->secsLeft -= dt;

        if (lasso->secsLeft < 0.0f)
        {
            lasso->secsLeft = 0.0f;
            lasso->flags |= 0x800;
        }

        F32 interp = 1.0f - (lasso->secsLeft / lasso->secsTotal);
        if (!(lasso->flags & 0x2000))
        {
            if (lasso->flags & 0x8)
            {
                fizzicalRadius(lasso, dt, &newPoint);
            }
            else
            {
                lasso->crRadius = lasso->stRadius * (1.0f - interp) + lasso->tgRadius * interp;
            }

            if (lasso->flags & 0x2)
            {
                fizzicalCenter(lasso, dt, &newPoint);
            }
            else
            {
                xVec3SMul(&lasso->crCenter, &lasso->stCenter, 1.0f - interp);
                xVec3AddScaled(&lasso->crCenter, &lasso->tgCenter, interp);
                xVec3SubFrom(&lasso->crCenter, &lasso->anchor);
            }
            
            if (lasso->flags & 0x4)
            {
                fizzicalNormal(lasso, dt, &newPoint);
            }
            else
            {
                xVec3SMul(&lasso->crNormal, &lasso->stNormal, 1.0f - interp);
                xVec3AddScaled(&lasso->crNormal, &lasso->tgNormal, interp);
            }

            xVec3SMulBy(&lasso->crNormal, 1.0f / xVec3Length(&lasso->crNormal));

            if (lasso->flags & 0x10)
            {
                fizzicalHonda(lasso, dt, &newPoint);
            }
            else
            {
                nonfizzicalHonda(lasso, dt, &newPoint);
            }
        }

        if (lasso->flags & 0x20)
        {
            fizzicalSlack(lasso, dt, &newPoint);
        }
        else
        {
            lasso->crSlack = lasso->stSlack * (1.0f - interp) + lasso->tgSlack * interp;
        }
    }
    else
    {
        if (!(lasso->flags & 0x4000))
        {
            if (lasso->flags & 0x100)
            {
                fizzicalRadius(lasso, dt, &newPoint);
            }

            if (lasso->flags & 0x40)
            {
                fizzicalCenter(lasso, dt, &newPoint);
            }

            if (lasso->flags & 0x80)
            {
                fizzicalNormal(lasso, dt, &newPoint);
                xVec3SMulBy(&lasso->crNormal, 1.0f / xVec3Length(&lasso->crNormal));
            }

            if (lasso->flags & 0x200)
            {
                fizzicalHonda(lasso, dt, &newPoint);
            }
            else
            {
                nonfizzicalHonda(lasso, dt, &newPoint);
            }
        }

        if (lasso->flags & 0x400)
        {
            fizzicalSlack(lasso, dt, &newPoint);
        }
    }

    xVec3Copy(&lasso->lastRefs[lasso->reindex[4]], &newPoint);

    lasso->reindex[0] = lasso->reindex[4];

    lasso->reindex[1] = lasso->reindex[0] + 1;
    if (lasso->reindex[1] > 4)
    {
        lasso->reindex[1] -= 5;
    }

    lasso->reindex[2] = lasso->reindex[0] + 2;
    if (lasso->reindex[2] > 4)
    {
        lasso->reindex[2] -= 5;
    }

    lasso->reindex[3] = lasso->reindex[0] + 3;
    if (lasso->reindex[3] > 4)
    {
        lasso->reindex[3] -= 5;
    }

    lasso->reindex[4] = lasso->reindex[0] + 4;
    if (lasso->reindex[4] > 4)
    {
        lasso->reindex[4] -= 5;
    }
}

void zLasso_InitTimer(zLasso* lasso, F32 interpTime)
{
    lasso->secsTotal = interpTime;
    lasso->secsLeft = interpTime;

    lasso->stRadius = lasso->tgRadius = lasso->crRadius = 0.0f;
    lasso->stSlack = lasso->tgSlack = lasso->crSlack = 0.0f;

    iModelTagEval(lasso->model->Data, &lasso->tag, lasso->model->Mat, &lasso->crCenter);

    xVec3Copy(&lasso->anchor, (xVec3*)&lasso->model->Mat->pos);
    xVec3SubFrom(&lasso->crCenter, &lasso->anchor);
    xVec3Copy(&lasso->honda, &lasso->crCenter);

    lasso->currDist = lasso->lastDist = 0.0f;

    xVec3Init(&lasso->stNormal, 0, 1, 0);
    xVec3Init(&lasso->tgNormal, 0, 1, 0);
    xVec3Init(&lasso->crNormal, 0, 1, 0);

    xVec3Copy(&lasso->lastRefs[0], &lasso->crCenter);
    xVec3Copy(&lasso->lastRefs[1], &lasso->crCenter);
    xVec3Copy(&lasso->lastRefs[2], &lasso->crCenter);
    xVec3Copy(&lasso->lastRefs[3], &lasso->crCenter);
    xVec3Copy(&lasso->lastRefs[4], &lasso->crCenter);

    lasso->reindex[0] = 0;
    lasso->reindex[1] = 1;
    lasso->reindex[2] = 2;
    lasso->reindex[3] = 3;
    lasso->reindex[4] = 4;
}

void zLasso_ResetTimer(zLasso* lasso, F32 interpTime)
{
    f32 temp_f0;
    f32 temp_f0_2;

    lasso->secsTotal = interpTime;
    lasso->secsLeft = interpTime;
    temp_f0 = lasso->crRadius;
    lasso->stRadius = temp_f0;
    lasso->tgRadius = temp_f0;
    temp_f0_2 = lasso->crSlack;
    lasso->stSlack = temp_f0_2;
    lasso->tgSlack = temp_f0_2;
    xVec3Copy(&lasso->stNormal, &lasso->crNormal);
    xVec3Copy(&lasso->tgNormal, &lasso->crNormal);
    xVec3Add(&lasso->stCenter, &lasso->crCenter, &lasso->anchor);
}

static void fizzicalRadius(zLasso* lasso, F32 dt, xVec3* newPoint)
{
    lasso->crRadius -= 2.0f * (0.75f - lasso->crSlack) * dt;
    if (lasso->crRadius < 0.0f)
    {
        lasso->crRadius = 0.0f;
    }
}

static void fizzicalCenter(zLasso* lasso, F32 dt, xVec3* newPoint)
{
    xVec3AddScaled(&lasso->crCenter, &lasso->lastRefs[lasso->reindex[4]], -0.2f);
    xVec3AddScaled(&lasso->crCenter, newPoint, 0.2f);
}

static void fizzicalNormal(zLasso* lasso, F32 dt, xVec3* newPoint)
{
    xVec3 sp8;
    f32 temp_f31;

    xVec3Sub(&sp8, &lasso->lastRefs[lasso->reindex[0]], &lasso->lastRefs[lasso->reindex[1]]);
    temp_f31 = xVec3Dot(&lasso->crNormal, &sp8);
    F32 len = xVec3Length(&sp8);
    xVec3AddScaled(&lasso->crNormal, &sp8, 1.1f * (-temp_f31 / len));
    xVec3Normalize(&lasso->crNormal, &lasso->crNormal);
}

static void fizzicalHonda(zLasso* lasso, F32 dt, xVec3* newPoint)
{
    xVec3SMul(&lasso->honda, newPoint, -1.0f);
    xVec3AddScaled(&lasso->honda, &lasso->lastRefs[lasso->reindex[0]], 0.571429f);
    xVec3AddScaled(&lasso->honda, &lasso->lastRefs[lasso->reindex[1]], 0.285714f);
    xVec3AddScaled(&lasso->honda, &lasso->lastRefs[lasso->reindex[2]], 0.142857f);
    xVec3AddScaled(&lasso->honda, &lasso->crNormal, -xVec3Dot(&lasso->crNormal, &lasso->honda));
    xVec3Normalize(&lasso->honda, &lasso->honda);
    xVec3SMulBy(&lasso->honda, lasso->crRadius);
    
    if ((negativeHondaX != 0) && (lasso->honda.x > 0.0f))
    {
        xSndPlay3D(xStrHash("sound_rope_windup"), 0.77f, 0.0f, 0U, 0x10000U, &lasso->anchor, 100.0f,
                   SND_CAT_GAME, 0.0f);
        negativeHondaX = 0;
    }
    
    if (lasso->honda.x < 0.0f)
    {
        negativeHondaX = 1;
    }
    
    xVec3AddTo(&lasso->honda, &lasso->crCenter);
}

static void nonfizzicalHonda(zLasso* lasso, F32 dt, xVec3* newPoint)
{
    xVec3Sub(&lasso->honda, newPoint, &lasso->crCenter);
    xVec3AddScaled(&lasso->honda, &lasso->crNormal, -xVec3Dot(&lasso->honda, &lasso->crNormal));

    F32 hondaLength = xVec3Length2(&lasso->honda);
    if (hondaLength < 0.00001f)
    {
        lasso->honda.x = lasso->crNormal.y;
        lasso->honda.y = -lasso->crNormal.x;
        lasso->honda.z = 0.0f;
        xVec3SMulBy(&lasso->honda, lasso->crRadius / xsqrt(xVec3Length2(&lasso->honda)));
        xVec3AddTo(&lasso->honda, &lasso->crCenter);
        return;
    }

    xVec3SMulBy(&lasso->honda, lasso->crRadius / xsqrt(hondaLength));
    xVec3AddTo(&lasso->honda, &lasso->crCenter);
}

static void fizzicalSlack(zLasso* lasso, F32 dt, xVec3* newPoint)
{
    lasso->crSlack += (2.0f * (lasso->lastDist - lasso->currDist)) - (0.6f * dt);
    if (lasso->crSlack < 0.0f)
    {
        lasso->crSlack = 0.0f;
        return;
    }

    if (lasso->crSlack > 1.0f)
    {
        lasso->crSlack = 1.0f;
    }
}

void zLasso_scenePrepare()
{
    sNumGuideLists = 0;
    sCurrentGuide = NULL;
}

static void initVertMap(zLassoGuide* guide)
{
    S32 center;
    S32 init;
    S32 curr;
    S32 currTri;
    S32 i;
    RpGeometry* geom = guide->poly->Data->geometry;
    S32 numTri = geom->numTriangles;
    RpTriangle* tris = geom->triangles;

    // The hub vertex of the fan: the vertex of tris[0] that tris[1] and
    // tris[2] both share.
    for (i = 0; i < 3; i++)
    {
        center = tris[0].vertIndex[i];
        if ((center == tris[1].vertIndex[0] || center == tris[1].vertIndex[1] ||
             center == tris[1].vertIndex[2]) &&
            (center == tris[2].vertIndex[0] || center == tris[2].vertIndex[1] ||
             center == tris[2].vertIndex[2]))
        {
            break;
        }
    }

    if (center == tris[0].vertIndex[0])
    {
        init = tris[0].vertIndex[1];
    }
    else
    {
        init = tris[0].vertIndex[0];
    }

    // Walk the rim: from the current rim vertex find the next triangle of
    // the fan that contains it, and step to that triangle's other rim vertex,
    // until we are back at the start.
    curr = init;
    currTri = 0;
    i = 0;
    do
    {
        guide->vertMap[i] = curr;

        currTri++;
        if (currTri == numTri)
        {
            currTri = 0;
        }

        while (!(curr == tris[currTri].vertIndex[0] || curr == tris[currTri].vertIndex[1] ||
                 curr == tris[currTri].vertIndex[2]))
        {
            currTri++;
            if (currTri == numTri)
            {
                currTri = 0;
            }
        }

        curr = (curr != tris[currTri].vertIndex[0] && center != tris[currTri].vertIndex[0]) ?
                   tris[currTri].vertIndex[0] :
               (curr != tris[currTri].vertIndex[1] && center != tris[currTri].vertIndex[1]) ?
                   tris[currTri].vertIndex[1] :
                   tris[currTri].vertIndex[2];
        i++;
    } while (curr != init);
}

static void vec2vecMat(xMat4x3* m, xVec3* v1, xVec3* v2)
{
    xVec3 v3;
    xVec3Cross(&v3, v1, v2);
    F32 f1 = xasin(xVec3Normalize(&v3, &v3));
    xMat4x3Rot(m, &v3, f1);
}

static void bakeMorphAnim(RpGeometry* geom, void* anim)
{
    if (*(U32*)anim & 0x80000000)
    {
        return;
    }

    *(U32*)anim |= 0x80000000;

    xMat4x3 mat;
    xVec3 tran[64];
    xQuat quat[64];

    iAnimEval(anim, 0.0f, 0x0, tran, quat);
    xQuatToMat(quat, &mat);
    mat.pos = tran[0];

    S32 i, j;
    for (i = 0; i < geom->numMorphTargets; i++)
    {
        S32 numV = geom->numVertices;
        xVec3* v = (xVec3*)geom->morphTarget[i].verts;

        for (j = 0; j < numV; j++)
        {
            xMat4x3Toworld(&v[j], &mat, &v[j]);
        }
    }
}

inline void xMat4x3Rot(xMat4x3* m, const xVec3* v, F32 f)
{
    xMat4x3RotC(m, v->x, v->y, v->z, f);
}

inline void xMat4x3RotC(xMat4x3* m, F32 f1, F32 f2, F32 f3, F32 f4)
{
    xMat3x3RotC(m, f1, f2, f3, f4);
    xVec3Copy(&m->pos, &g_O3);
}
