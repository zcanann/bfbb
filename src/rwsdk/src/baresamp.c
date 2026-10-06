#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwIMAGEGAMMACORRECTED 0x02

/* Image coordinates are 16.16 fixed point */
#define rwRESAMPLEFIXEDONE 0x10000
#define rwRESAMPLEFIXEDTOREAL(_x) (((RwReal)(1.0 / 65536.0)) * (RwReal)(_x))

#define RwRGBARealFromRwRGBAMacro(_o, _i)                                                          \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_o)->red = ((RwReal)(1.0 / 255.0)) * (RwReal)(_i)->red;                                   \
        (_o)->green = ((RwReal)(1.0 / 255.0)) * (RwReal)(_i)->green;                               \
        (_o)->blue = ((RwReal)(1.0 / 255.0)) * (RwReal)(_i)->blue;                                 \
        (_o)->alpha = ((RwReal)(1.0 / 255.0)) * (RwReal)(_i)->alpha;                               \
    }                                                                                              \
    MACRO_STOP

#define RwRGBAFromRwRGBARealMacro(_o, _i)                                                          \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwInt32 quantize;                                                                          \
                                                                                                   \
        quantize = (RwInt32)(((RwReal)255.0) * (_i)->red + ((RwReal)0.5));                         \
        (_o)->red = (RwUInt8)quantize;                                                             \
        quantize = (RwInt32)(((RwReal)255.0) * (_i)->green + ((RwReal)0.5));                       \
        (_o)->green = (RwUInt8)quantize;                                                           \
        quantize = (RwInt32)(((RwReal)255.0) * (_i)->blue + ((RwReal)0.5));                        \
        (_o)->blue = (RwUInt8)quantize;                                                            \
        quantize = (RwInt32)(((RwReal)255.0) * (_i)->alpha + ((RwReal)0.5));                       \
        (_o)->alpha = (RwUInt8)quantize;                                                           \
    }                                                                                              \
    MACRO_STOP

#define RwRGBARealAddMacro(_o, _a, _b)                                                             \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_o)->red = (_a)->red + (_b)->red;                                                         \
        (_o)->green = (_a)->green + (_b)->green;                                                   \
        (_o)->blue = (_a)->blue + (_b)->blue;                                                      \
        (_o)->alpha = (_a)->alpha + (_b)->alpha;                                                   \
    }                                                                                              \
    MACRO_STOP

#define RwRGBARealScaleMacro(_o, _a, _s)                                                           \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_o)->red = (_a)->red * (_s);                                                              \
        (_o)->green = (_a)->green * (_s);                                                          \
        (_o)->blue = (_a)->blue * (_s);                                                            \
        (_o)->alpha = (_a)->alpha * (_s);                                                          \
    }                                                                                              \
    MACRO_STOP

/* Average colour of a horizontal span of one row, weighted by pixel coverage */
static void ImageResampleGetSpan(const RwImage* _image, RwInt32 _nStartX, RwInt32 _nEndX,
                                 RwInt32 _nY, RwRGBAReal* _rrpCol)
{
    RwReal nArea;
    RwReal nScale;
    RwRGBA* rpSpan;
    RwRGBAReal rrAdd;
    RwInt32 nPos;

    nArea = rwRESAMPLEFIXEDTOREAL(_nEndX - _nStartX);

    rpSpan = (RwRGBA*)(_image->cpPixels + (_nY >> 16) * _image->stride) + (_nStartX >> 16);

    if ((_nStartX >> 16) == (_nEndX >> 16))
    {
        RwRGBARealFromRwRGBAMacro(_rrpCol, rpSpan);
        RwRGBARealScaleMacro(_rrpCol, _rrpCol, nArea);
    }
    else
    {
        /* Leading partial pixel */
        RwRGBARealFromRwRGBAMacro(_rrpCol, rpSpan);
        nPos = ((_nStartX >> 16) + 1) << 16;
        nScale = rwRESAMPLEFIXEDTOREAL(nPos - _nStartX);
        RwRGBARealScaleMacro(_rrpCol, _rrpCol, nScale);
        rpSpan++;

        /* Whole pixels */
        while ((nPos >> 16) != (_nEndX >> 16))
        {
            RwRGBARealFromRwRGBAMacro(&rrAdd, rpSpan);
            RwRGBARealAddMacro(_rrpCol, _rrpCol, &rrAdd);
            rpSpan++;
            nPos += rwRESAMPLEFIXEDONE;
        }

        /* Trailing partial pixel */
        RwRGBARealFromRwRGBAMacro(&rrAdd, rpSpan);
        nScale = rwRESAMPLEFIXEDTOREAL(_nEndX - nPos);
        RwRGBARealScaleMacro(&rrAdd, &rrAdd, nScale);
        RwRGBARealAddMacro(_rrpCol, _rrpCol, &rrAdd);
    }

    nScale = (RwReal)1.0 / nArea;
    RwRGBARealScaleMacro(_rrpCol, _rrpCol, nScale);
}

/* Average colour of a rectangle of the image, weighted by pixel coverage */
static void ImageResampleGetAvgPixel(const RwImage* _image, RwInt32 _nXStart, RwInt32 _nXEnd,
                                     RwInt32 _nYStart, RwInt32 _nYEnd, RwRGBAReal* _rrpCol)
{
    RwReal nArea;
    RwReal nScale;
    RwRGBAReal rrAdd;
    RwInt32 nPos;

    nArea = rwRESAMPLEFIXEDTOREAL(_nYEnd - _nYStart);

    if ((_nYStart >> 16) == (_nYEnd >> 16))
    {
        ImageResampleGetSpan(_image, _nXStart, _nXEnd, _nYStart, _rrpCol);
        RwRGBARealScaleMacro(_rrpCol, _rrpCol, nArea);
    }
    else
    {
        /* Leading partial row */
        nPos = ((_nYStart >> 16) + 1) << 16;
        ImageResampleGetSpan(_image, _nXStart, _nXEnd, _nYStart, _rrpCol);
        nScale = rwRESAMPLEFIXEDTOREAL(nPos - _nYStart);
        RwRGBARealScaleMacro(_rrpCol, _rrpCol, nScale);

        /* Whole rows */
        while ((nPos >> 16) != (_nYEnd >> 16))
        {
            ImageResampleGetSpan(_image, _nXStart, _nXEnd, nPos, &rrAdd);
            RwRGBARealAddMacro(_rrpCol, &rrAdd, _rrpCol);
            nPos += rwRESAMPLEFIXEDONE;
        }

        /* Trailing partial row */
        ImageResampleGetSpan(_image, _nXStart, _nXEnd, nPos, &rrAdd);
        nScale = rwRESAMPLEFIXEDTOREAL(_nYEnd - nPos);
        RwRGBARealScaleMacro(&rrAdd, &rrAdd, nScale);
        RwRGBARealAddMacro(_rrpCol, _rrpCol, &rrAdd);
    }

    nScale = (RwReal)1.0 / nArea;
    RwRGBARealScaleMacro(_rrpCol, _rrpCol, nScale);
}

RwImage* RwImageResample(RwImage* dstImage, const RwImage* srcImage)
{
    RwInt32 nX;
    RwInt32 nY;
    RwInt32 nXPos;
    RwInt32 nXDelta;
    RwInt32 nYPos;
    RwInt32 nYDelta;
    RwInt32 dstWidth = dstImage->width;
    RwInt32 dstHeight = dstImage->height;
    RwInt32 srcWidth = srcImage->width;
    RwInt32 srcHeight = srcImage->height;

    dstImage->flags |= (srcImage->flags & rwIMAGEGAMMACORRECTED);

    nXDelta = (RwInt32)(((RwReal)rwRESAMPLEFIXEDONE) * ((RwReal)srcWidth / (RwReal)dstWidth));
    nYPos = 0;
    nYDelta = (RwInt32)(((RwReal)rwRESAMPLEFIXEDONE) * ((RwReal)srcHeight / (RwReal)dstHeight));

    for (nY = 0; nY < dstHeight; nY++)
    {
        RwRGBAReal rrCol;
        RwRGBA* const rpDstSpan = (RwRGBA*)(dstImage->cpPixels + dstImage->stride * nY);

        nXPos = 0;
        nX = 0;
        while (nX < dstWidth)
        {
            ImageResampleGetAvgPixel(srcImage, nXPos, nXPos + nXDelta - 1, nYPos,
                                     nYPos + nYDelta - 1, &rrCol);
            RwRGBAFromRwRGBARealMacro(&rpDstSpan[nX], &rrCol);

            nXPos += nXDelta;
            nX++;
        }

        nYPos += nYDelta;
    }

    return dstImage;
}

RwImage* RwImageCreateResample(const RwImage* srcImage, RwInt32 width, RwInt32 height)
{
    RwImage* dstImage;
    RwImage* ipUse;

    dstImage = RwImageCreate(width, height, 32);
    if (!dstImage)
    {
        return NULL;
    }

    if (!RwImageAllocatePixels(dstImage))
    {
        RwImageDestroy(dstImage);
        return NULL;
    }

    if (srcImage->depth != 32)
    {
        /* Resample from a 32 bit copy of the source */
        ipUse = RwImageCreate(srcImage->width, srcImage->height, 32);
        if (!ipUse)
        {
            RwImageFreePixels(dstImage);
            RwImageDestroy(dstImage);
            return NULL;
        }

        if (!RwImageAllocatePixels(ipUse))
        {
            RwImageDestroy(ipUse);
            RwImageFreePixels(dstImage);
            RwImageDestroy(dstImage);
            return NULL;
        }

        RwImageCopy(ipUse, srcImage);

        if (!RwImageResample(dstImage, ipUse))
        {
            RwImageFreePixels(ipUse);
            RwImageDestroy(ipUse);
            RwImageFreePixels(dstImage);
            RwImageDestroy(dstImage);
            return NULL;
        }

        RwImageFreePixels(ipUse);
        RwImageDestroy(ipUse);
    }
    else
    {
        if (!RwImageResample(dstImage, srcImage))
        {
            RwImageFreePixels(dstImage);
            RwImageDestroy(dstImage);
            return NULL;
        }
    }

    return dstImage;
}
