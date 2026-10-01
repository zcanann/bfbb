#include <string.h>
#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwSTANDARDRASTERCREATE 4
#define rwSTANDARDRASTERDESTROY 5
#define rwSTANDARDRASTERLOCK 15
#define rwSTANDARDRASTERUNLOCK 16
#define rwSTANDARDRASTERSHOWRASTER 20
#define rwSTANDARDRASTERLOCKPALETTE 23
#define rwSTANDARDRASTERUNLOCKPALETTE 24
#define rwSTANDARDRASTERGETMIPLEVELS 28

#define rwRASTERCONTEXTSTACKSIZE 10

#define rwRASTERALIGNMENT sizeof(RwUInt32)

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rwRasterGlobals rwRasterGlobals;
struct rwRasterGlobals
{
    RwRaster* rasterStack[rwRASTERCONTEXTSTACKSIZE];
    RwInt32 rasterSP;
    RwRaster dummyRaster;
    RwFreeList* rasterFreeList;
};

#define RWRASTERGLOBAL(var)                                                                        \
    (RWPLUGINOFFSET(rwRasterGlobals, RwEngineInstance, rasterModule.globalsOffset)->var)

static RwPluginRegistry rasterTKList = { sizeof(RwRaster),        sizeof(RwRaster),       0, 0,
                                         (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwModuleInfo rasterModule;

RwRaster* RwRasterUnlock(RwRaster* raster)
{
    RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERUNLOCK])(NULL, raster, 0);

    return raster;
}

RwRaster* RwRasterUnlockPalette(RwRaster* raster)
{
    RwStandardFunc RasterUnlockPaletteFunc = RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERUNLOCKPALETTE]);

    RasterUnlockPaletteFunc(NULL, raster, 0);
    raster->privateFlags &= ~rwRASTERPALETTELOCKED;

    return raster;
}

RwBool RwRasterDestroy(RwRaster* raster)
{
    _rwPluginRegistryDeInitObject(&rasterTKList, raster);

    RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERDESTROY])(NULL, raster, 0);

    RwFreeListFree(RWRASTERGLOBAL(rasterFreeList), raster);

    return TRUE;
}

RwInt32 RwRasterRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                               RwPluginObjectConstructor constructCB,
                               RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPlugin(&rasterTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);

    return plug;
}

RwUInt8* RwRasterLockPalette(RwRaster* raster, RwInt32 lockMode)
{
    RwUInt8* palettePtr;

    if (RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERLOCKPALETTE])(&palettePtr, raster, lockMode))
    {
        return palettePtr;
    }

    return NULL;
}

RwInt32 RwRasterGetNumLevels(RwRaster* raster)
{
    RwStandardFunc GetMipLevelsFunc;
    RwInt32 numMipLevels;

    if (!(RwRasterGetFormat(raster) & rwRASTERFORMATMIPMAP))
    {
        return 1;
    }

    GetMipLevelsFunc = RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERGETMIPLEVELS]);
    if (GetMipLevelsFunc(&numMipLevels, raster, 0))
    {
        return numMipLevels;
    }

    return -1;
}

RwRaster* RwRasterShowRaster(RwRaster* raster, void* dev, RwUInt32 flags)
{
    RwStandardFunc RasterShowRasterFunc = RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERSHOWRASTER]);

    /* Anything cached for the previous frame may now be reused */
    _rwResourcesPurge();

    if (RasterShowRasterFunc(raster, dev, flags))
    {
        return raster;
    }

    return NULL;
}

RwRaster* RwRasterCreate(RwInt32 width, RwInt32 height, RwInt32 depth, RwInt32 flags)
{
    RwRaster* raster;

    raster = (RwRaster*)RwFreeListAlloc(RWRASTERGLOBAL(rasterFreeList));
    if (raster)
    {
        RwStandardFunc const RasterCreateFunc = RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERCREATE]);

        raster->privateFlags = 0;
        raster->cFlags = 0;
        raster->width = width;
        raster->height = height;
        raster->nOffsetX = 0;
        raster->nOffsetY = 0;
        raster->depth = depth;
        raster->parent = raster;
        raster->cpPixels = NULL;
        raster->palette = NULL;

        if (!RasterCreateFunc(NULL, raster, flags))
        {
            RwFreeListFree(RWRASTERGLOBAL(rasterFreeList), raster);
            return NULL;
        }

        _rwPluginRegistryInitObject(&rasterTKList, raster);

        return raster;
    }

    return NULL;
}

RwUInt8* RwRasterLock(RwRaster* raster, RwUInt8 level, RwInt32 lockMode)
{
    RwUInt8* pixelPtr;

    if (RWSRCGLOBAL(stdFunc[rwSTANDARDRASTERLOCK])(&pixelPtr, raster,
                                                   lockMode + ((RwInt32)level << 8)))
    {
        return pixelPtr;
    }

    return NULL;
}

void* _rwRasterClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWRASTERGLOBAL(rasterFreeList))
    {
        RwFreeListDestroy(RWRASTERGLOBAL(rasterFreeList));
        RWRASTERGLOBAL(rasterFreeList) = NULL;
    }

    rasterModule.numInstances--;

    return instance;
}

static RwInt32 _rwRasterFreeListBlockSize = 128;
static RwInt32 _rwRasterFreeListPreallocBlocks = 1;
static RwFreeList _rwRasterFreeList;

void* _rwRasterOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    rasterModule.globalsOffset = offset;

    /* The bottom of the context stack is a dummy raster */
    memset(&RWRASTERGLOBAL(dummyRaster), 0, sizeof(RwRaster));
    RWRASTERGLOBAL(dummyRaster).width = 0;
    RWRASTERGLOBAL(dummyRaster).height = 0;
    RWRASTERGLOBAL(dummyRaster).depth = 0;
    RWRASTERGLOBAL(dummyRaster).cFlags = rwRASTERDONTALLOCATE;
    RWRASTERGLOBAL(dummyRaster).cpPixels = NULL;
    RWRASTERGLOBAL(dummyRaster).palette = NULL;
    RWRASTERGLOBAL(dummyRaster).cType = 0;

    RWRASTERGLOBAL(rasterSP) = 0;
    RWRASTERGLOBAL(rasterStack[0]) = &RWRASTERGLOBAL(dummyRaster);

    RWRASTERGLOBAL(rasterFreeList) = RwFreeListCreateAndPreallocateSpace(
        rasterTKList.sizeOfStruct, _rwRasterFreeListBlockSize, rwRASTERALIGNMENT,
        _rwRasterFreeListPreallocBlocks, &_rwRasterFreeList);
    if (!RWRASTERGLOBAL(rasterFreeList))
    {
        return NULL;
    }

    rasterModule.numInstances++;

    return instance;
}
