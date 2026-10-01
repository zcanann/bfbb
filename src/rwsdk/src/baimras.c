#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwSTANDARDIMAGEGETRASTER 6
#define rwSTANDARDRASTERSETIMAGE 7
#define rwSTANDARDIMAGEFINDRASTERFORMAT 9

#define rwIMAGEGAMMACORRECTED 0x02

RwImage* RwImageSetFromRaster(RwImage* image, RwRaster* raster)
{
    if (RWSRCGLOBAL(stdFunc[rwSTANDARDIMAGEGETRASTER])(image, raster, 0))
    {
        if (raster->privateFlags & rwRASTERGAMMACORRECTED)
        {
            image->flags |= rwIMAGEGAMMACORRECTED;
        }

        return image;
    }

    return NULL;
}

RwRaster* RwRasterSetFromImage(RwRaster* raster, RwImage* image)
{
    if (RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERSETIMAGE])(raster, image, 0))
    {
        if (image->flags & rwIMAGEGAMMACORRECTED)
        {
            raster->privateFlags |= rwRASTERGAMMACORRECTED;
        }

        return raster;
    }

    return NULL;
}

RwImage* RwImageFindRasterFormat(RwImage* ipImage, RwInt32 nRasterType, RwInt32* npWidth,
                                 RwInt32* npHeight, RwInt32* npDepth, RwInt32* npFormat)
{
    RwRaster rRaster;

    if (!RWSRCGLOBAL(stdFunc[rwSTANDARDIMAGEFINDRASTERFORMAT])(&rRaster, ipImage, nRasterType))
    {
        return NULL;
    }

    *npFormat = (rRaster.cFormat << 8) | rRaster.cType;
    *npWidth = rRaster.width;
    *npHeight = rRaster.height;
    *npDepth = rRaster.depth;

    return ipImage;
}
