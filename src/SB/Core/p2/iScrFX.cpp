#include "iScrFX.h"

#include <rwim2d.h>
#include <rwim3d.h>

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
