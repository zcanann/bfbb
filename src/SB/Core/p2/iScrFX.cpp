#include "iScrFX.h"

#include "iParMgr.h"
#include "xScrFx.h"
#include "xDebug.h"

#include <rwim2d.h>
#include <rwim3d.h>
#include <stdio.h>

struct RwRect
{
    RwInt32 x;
    RwInt32 y;
    RwInt32 w;
    RwInt32 h;
};

extern "C" RwRaster* RwRasterSubRaster(RwRaster* subRaster, RwRaster* raster, RwRect* rect);

extern "C" RwMatrix* RwMatrixMultiply(RwMatrix* matrix, const RwMatrix* a, const RwMatrix* b);

struct _iMotionBlurData
{
    S32 motionBlurAlpha;
    RwRaster* motionBlurFrontBuffer;
    RwIm2DVertex vertex[4];
    U16 index[6];
    U32 w;
    U32 h;
};

static U32 sMotionBlurEnabled;
static _iMotionBlurData sMBD;
static RwIm3DVertex* Im3DBuffer;
static U32 Im3DBufferPos;

void iScrFxInit()
{
}

void iScrFxBegin()
{
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}

void iScrFxEnd()
{
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
}

void iScrFxDrawBox(F32 x1, F32 y1, F32 x2, F32 y2, U8 red, U8 green, U8 blue, U8 alpha)
{
    static RwIm2DVertex v[4];
    static U16 indices[4] = { 0, 1, 2, 3 };

    RwIm2DVertexSetScreenX(&v[0], x1);
    RwIm2DVertexSetScreenX(&v[2], x1);
    RwIm2DVertexSetScreenY(&v[0], y1);
    RwIm2DVertexSetScreenY(&v[1], y1);
    RwIm2DVertexSetScreenX(&v[1], x2);
    RwIm2DVertexSetScreenX(&v[3], x2);
    RwIm2DVertexSetScreenY(&v[2], y2);
    RwIm2DVertexSetScreenY(&v[3], y2);

    RwIm2DVertexSetIntRGBA(&v[0], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[1], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[2], red, green, blue, alpha);
    RwIm2DVertexSetIntRGBA(&v[3], red, green, blue, alpha);
    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRISTRIP, v, 4, indices, 4);
}

void iCameraMotionBlurActivate(U32 activate)
{
    sMotionBlurEnabled = activate;
}

void iCameraSetBlurriness(F32 amount)
{
    if (amount <= 0.0f)
    {
        sMotionBlurEnabled = FALSE;
    }
    else
    {
        if (amount > 1.0f)
        {
            amount = 1.0f;
        }
        sMotionBlurEnabled = TRUE;
        sMBD.motionBlurAlpha = 254.0f * amount + 0.5f;
    }
}

S32 iScrFxCameraDestroyed(RwCamera*)
{
    if (sMBD.motionBlurFrontBuffer != NULL)
    {
        RwRasterDestroy(sMBD.motionBlurFrontBuffer);
        sMBD.motionBlurFrontBuffer = NULL;
        return 1;
    }
    return 0;
}

static void iCameraOverlayRender(RwCamera* pCamera, RwRaster* ras, RwRGBA col)
{
    RwIm2DVertexSetIntRGBA(&sMBD.vertex[0], col.red, col.green, col.blue, col.alpha);
    RwIm2DVertexSetIntRGBA(&sMBD.vertex[1], col.red, col.green, col.blue, col.alpha);
    RwIm2DVertexSetIntRGBA(&sMBD.vertex[2], col.red, col.green, col.blue, col.alpha);
    RwIm2DVertexSetIntRGBA(&sMBD.vertex[3], col.red, col.green, col.blue, col.alpha);

    RwRect rect = {};
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, ras);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
    RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, sMBD.vertex, 4, sMBD.index, 6);
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
}

inline void iScrFxMotionBlurRender(RwCamera* camera, U32 alpha)
{
    if (sMBD.motionBlurFrontBuffer != NULL)
    {
        RwRGBA col = { 0xff, 0xff, 0xff, (U8)alpha };
        iCameraOverlayRender(camera, sMBD.motionBlurFrontBuffer, col);
    }
}

void iScrFxCameraEndScene(RwCamera* pCamera)
{
    if (sMotionBlurEnabled && sMBD.motionBlurAlpha != 0)
    {
        iScrFxMotionBlurRender(pCamera, sMBD.motionBlurAlpha & 0xff);
    }
}

void iScrFxCameraCreated(RwCamera* pCamera)
{
    sMBD.motionBlurAlpha = 0x90;
    sMBD.motionBlurFrontBuffer = NULL;
    sMBD.index[0] = 0;
    sMBD.index[1] = 1;
    sMBD.index[2] = 2;
    sMBD.index[3] = 0;
    sMBD.index[4] = 2;
    sMBD.index[5] = 3;
    iScrFxMotionBlurOpen(pCamera);
}

static void iScrFxMotionBlurCreateImmediateModeData(RwCamera* camera, RwRect* rect)
{
    F32 w = rect->w;
    F32 h = rect->h;
    F32 xSize = 512.0f;
    if (w > 512.0f)
    {
        xSize = 1024.0f;
    }
    F32 U = 0.5f + w;
    U /= xSize;
    F32 V = 0.5f + h;
    V /= 512.0f;
    F32 u = 0.5f / xSize;
    RwIm2DVertex* ver = sMBD.vertex;
    F32 nearz = RwIm2DGetNearScreenZ();
    F32 oocameraNearClipPlane = 1.0f / camera->nearPlane;
    F32 cameraNearClipPlane = camera->nearPlane;
    F32 wstep = w;
    F32 hstep = h;
    F32 ustep = U - u;
    F32 vstep = V - 0.5f / 512.0f;

    for (S32 i = 0; i < 4; i++)
    {
        RwIm2DVertexSetCameraZ(ver, cameraNearClipPlane);
        RwIm2DVertexSetRecipCameraZ(ver, oocameraNearClipPlane);
        RwIm2DVertexSetScreenZ(ver, nearz);
        RwIm2DVertexSetIntRGBA(ver, 255, 255, 255, 255);
        ver++;
    }

    ver = sMBD.vertex;
    for (S32 x = 1; x <= 1; x++)
    {
        for (S32 y = 1; y <= 1; y++)
        {
            RwIm2DVertexSetScreenX(&ver[0], wstep * x - wstep);
            RwIm2DVertexSetScreenY(&ver[0], hstep * y - hstep);
            RwIm2DVertexSetU(&ver[0], u + ustep * (x - 1), oocameraNearClipPlane);
            RwIm2DVertexSetV(&ver[0], 0.5f / 512.0f + vstep * (y - 1), oocameraNearClipPlane);
            RwIm2DVertexSetScreenX(&ver[1], wstep * x - wstep);
            RwIm2DVertexSetScreenY(&ver[1], hstep * y);
            RwIm2DVertexSetU(&ver[1], u + ustep * (x - 1), oocameraNearClipPlane);
            RwIm2DVertexSetV(&ver[1], 0.5f / 512.0f + vstep * y, oocameraNearClipPlane);
            RwIm2DVertexSetScreenX(&ver[2], wstep * x);
            RwIm2DVertexSetScreenY(&ver[2], hstep * y);
            RwIm2DVertexSetU(&ver[2], u + ustep * x, oocameraNearClipPlane);
            RwIm2DVertexSetV(&ver[2], 0.5f / 512.0f + vstep * y, oocameraNearClipPlane);
            RwIm2DVertexSetScreenX(&ver[3], wstep * x);
            RwIm2DVertexSetScreenY(&ver[3], hstep * y - hstep);
            RwIm2DVertexSetU(&ver[3], u + ustep * x, oocameraNearClipPlane);
            RwIm2DVertexSetV(&ver[3], 0.5f / 512.0f + vstep * (y - 1), oocameraNearClipPlane);
            ver += 4;
        }
    }
}

S32 iScrFxMotionBlurOpen(RwCamera* camera)
{
    RwRect rect = {};
    rect.w = camera->frameBuffer->width;
    rect.h = camera->frameBuffer->height;
    if (rect.w <= 0 || rect.h <= 0)
    {
        sMBD.motionBlurFrontBuffer = NULL;
        return 0;
    }
    sMBD.motionBlurFrontBuffer = RwRasterCreate(0, 0, 0, rwRASTERTYPECAMERATEXTURE | rwRASTERDONTALLOCATE);
    if (sMBD.motionBlurFrontBuffer)
    {
        if (!RwRasterSubRaster(sMBD.motionBlurFrontBuffer, camera->frameBuffer, &rect))
        {
            RwRasterDestroy(sMBD.motionBlurFrontBuffer);
            sMBD.motionBlurFrontBuffer = NULL;
            return 0;
        }
    }
    else
    {
        printf("Error creating raster\n");
        return 0;
    }
    iScrFxMotionBlurCreateImmediateModeData(camera, &rect);
    return 1;
}

void iScrFxDistortionRender(RwCamera* camera)
{
    xVec3 at;
    DistortionParticle* dp = gDistortionParticles;
    RwMatrix* mat = RwFrameGetLTM(RwCameraGetFrame(camera));
    RwMatrix ptmat;
    RwMatrix tmp;
    RwMatrix invMtx;
    RwMatrixSetIdentity(&ptmat);
    RwMatrixInvert(&invMtx, RwFrameGetLTM(RwCameraGetFrame(camera)));
    for (S32 i = 0; i < gNumDistortionParticles; i++, dp++)
    {
        RwIm3DVertex* Im3DBuffer = gRenderBuffer.m_vertex;
        if (Im3DBufferPos > 480 - 6)
        {
            if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                                rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
            {
                RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
                RwIm3DEnd();
            }
            Im3DBufferPos = 0;
        }
        RwIm3DVertex* imv = &Im3DBuffer[Im3DBufferPos];
        xVec3 a, b, c, d;
        xVec3 sa, sb, sc, sd;
        xVec3 mmsa, mmsb, mmsc, mmsd;
        xVec3 right;
        xVec3Sub(&at, (xVec3*)&mat->pos, &dp->pos);
        xVec3Normalize(&at, &at);
        xVec3Cross(&right, &at, &dp->dir);
        a.x = dp->pos.x;
        a.y = dp->pos.y;
        a.z = dp->pos.z;
        xVec3AddScaled(&a, &dp->dir, -0.5f);
        xVec3AddScaled(&a, &right, -0.5f);
        xVec3Add(&b, &a, &right);
        xVec3Add(&c, &a, &dp->dir);
        xVec3Add(&d, &a, &dp->dir);
        xVec3AddTo(&d, &right);
        RwIm3DVertexSetPos(&imv[0], a.x, a.y, a.z);
        RwIm3DVertexSetPos(&imv[1], b.x, b.y, b.z);
        RwIm3DVertexSetPos(&imv[2], c.x, c.y, c.z);
        RwIm3DVertexSetPos(&imv[3], b.x, b.y, b.z);
        RwIm3DVertexSetPos(&imv[4], c.x, c.y, c.z);
        RwIm3DVertexSetPos(&imv[5], d.x, d.y, d.z);

        ptmat.pos.x = a.x;
        ptmat.pos.y = a.y;
        ptmat.pos.z = a.z;
        RwMatrixMultiply(&tmp, &invMtx, &ptmat);
        mmsa.x = tmp.pos.x;
        mmsa.y = tmp.pos.y;
        mmsa.z = tmp.pos.z;
        RwV3dTransformPoints((RwV3d*)&sa, (RwV3d*)&a, 1, &invMtx);
        if (mmsa.z <= 0.0f)
        {
            continue;
        }
        if (i == 0)
        {
            xprintf("% 6.3f % 6.3f % 6.3f\n", mmsa.x, mmsa.y, mmsa.z);
            xprintf("% 6.3f % 6.3f % 6.3f\n", sa.x, sa.y, sa.z);
        }
        RwV3dScaleMacro((RwV3d*)&mmsa, (RwV3d*)&mmsa, 1.0f / mmsa.z);
        RwV3dScaleMacro((RwV3d*)&sa, (RwV3d*)&sa, 1.0f / sa.z);

        ptmat.pos.x = b.x;
        ptmat.pos.y = b.y;
        ptmat.pos.z = b.z;
        RwMatrixMultiply(&tmp, &invMtx, &ptmat);
        mmsb.x = tmp.pos.x;
        mmsb.y = tmp.pos.y;
        mmsb.z = tmp.pos.z;
        RwV3dTransformPoints((RwV3d*)&sb, (RwV3d*)&b, 1, &invMtx);
        if (mmsb.z <= 0.0f)
        {
            continue;
        }
        RwV3dScaleMacro((RwV3d*)&mmsb, (RwV3d*)&mmsb, 1.0f / mmsb.z);
        RwV3dScaleMacro((RwV3d*)&sb, (RwV3d*)&sb, 1.0f / sb.z);

        ptmat.pos.x = c.x;
        ptmat.pos.y = c.y;
        ptmat.pos.z = c.z;
        RwMatrixMultiply(&tmp, &invMtx, &ptmat);
        mmsc.x = tmp.pos.x;
        mmsc.y = tmp.pos.y;
        mmsc.z = tmp.pos.z;
        RwV3dTransformPoints((RwV3d*)&sc, (RwV3d*)&c, 1, &invMtx);
        if (mmsc.z <= 0.0f)
        {
            continue;
        }
        RwV3dScaleMacro((RwV3d*)&mmsc, (RwV3d*)&mmsc, 1.0f / mmsc.z);
        RwV3dScaleMacro((RwV3d*)&sc, (RwV3d*)&sc, 1.0f / sc.z);

        ptmat.pos.x = d.x;
        ptmat.pos.y = d.y;
        ptmat.pos.z = d.z;
        RwMatrixMultiply(&tmp, &invMtx, &ptmat);
        mmsd.x = tmp.pos.x;
        mmsd.y = tmp.pos.y;
        mmsd.z = tmp.pos.z;
        RwV3dTransformPoints((RwV3d*)&sd, (RwV3d*)&d, 1, &invMtx);
        if (mmsd.z <= 0.0f)
        {
            continue;
        }
        RwV3dScaleMacro((RwV3d*)&mmsd, (RwV3d*)&mmsd, 1.0f / mmsd.z);
        RwV3dScaleMacro((RwV3d*)&sd, (RwV3d*)&sd, 1.0f / sd.z);
    }
    if (Im3DBufferPos)
    {
        if (RwIm3DTransform(Im3DBuffer, Im3DBufferPos, NULL,
                            rwIM3D_VERTEXUV | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA))
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
            RwIm3DEnd();
        }
        Im3DBufferPos = 0;
    }
}
