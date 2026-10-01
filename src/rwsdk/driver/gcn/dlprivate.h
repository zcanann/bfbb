#ifndef DLPRIVATE_H
#define DLPRIVATE_H

#include <rwsdk/rwcore.h>
#include <dolphin/gx.h>
#include <dolphin/os.h>

/* Private declarations shared by the GameCube device driver units. */

#define E_RW_DEVICEERROR 0x00000002
#define E_RW_INVIMAGEDEPTH 0x80000008
#define E_RW_INVRASTERDEPTH 0x8000000c
#define E_RW_INVRASTERFORMAT 0x8000000d
#define E_RW_INVRASTERLOCKREQ 0x8000000e
#define E_RW_INVRASTERUNLOCKREQ 0x80000011
#define E_RW_NOMEM 0x80000013
#define E_RW_INVPRIMTYPE 0x00000025

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwID_COREPLUGIN;                                                   \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define RwStreamWriteChunkHeader(stream, type, size)                                               \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, 0xFFFF)

typedef struct RwGameCubeRasterExtension RwGameCubeRasterExtension;
struct RwGameCubeRasterExtension
{
    GXTlutObj tlutObj; /* 0x00 */
    RwUInt32 format; /* 0x0C */
    RwUInt32 tlutFmt; /* 0x10 */
    RwUInt32 flags; /* 0x14 */
    RwUInt8* memory; /* 0x18 */
    RwUInt8* pixels; /* 0x1C */
    RwUInt8* palette; /* 0x20 */
    RwUInt8* lockedPixels; /* 0x24 */
    RwUInt8* lockedBuffer; /* 0x28 */
    void* region; /* 0x2C */
    RwUInt16 token; /* 0x30 */
    RwUInt8 maxLOD; /* 0x32 */
    RwUInt8 lockedMipLevel; /* 0x33 */
};

typedef struct _rwDlTextureExt _rwDlTextureExt;
struct _rwDlTextureExt
{
    GXTexObj texObj; /* 0x00 */
    RwUInt32 flags; /* 0x20 */
};

/* _rwDlTextureExt flags (the low 16 bits cache the texture's filterAddressing) */
#define rwDLTEXTUREEXTDEFAULTLOD 0x01000000 /* LOD parameters not yet initialised */
#define rwDLTEXTUREEXTPRELOADED 0x02000000 /* texture lives in preloaded TMEM */

/* Criterion internal plugin ID used for the driver raster and texture extensions */
#define rwID_DLDRIVERPLUGIN 0x40C

extern RwInt32 _RwGameCubeRasterExtOffset;
extern RwInt32 _RwGameCubeTextureExtOffset;

#define RASTEREXTFROMRASTER(raster)                                                                \
    ((RwGameCubeRasterExtension*)(((RwUInt8*)(raster)) + _RwGameCubeRasterExtOffset))

#define TEXTUREEXTFROMTEXTURE(texture)                                                             \
    ((_rwDlTextureExt*)(((RwUInt8*)(texture)) + _RwGameCubeTextureExtOffset))

/* Dolphin SDK texture object queries that our GX headers do not declare */
extern GXAnisotropy GXGetTexObjMaxAniso(const GXTexObj* obj);
extern GXBool GXGetTexObjBiasClamp(const GXTexObj* obj);
extern GXBool GXGetTexObjEdgeLOD(const GXTexObj* obj);
extern f32 GXGetTexObjLODBias(const GXTexObj* obj);
extern u32 GXGetTexObjTlut(const GXTexObj* obj);
extern void GXSetDrawSync(u16 token);
extern void GXLoadTexObjPreLoaded(GXTexObj* obj, GXTexRegion* region, GXTexMapID id);

/* RenderWare core functions used by the driver */
extern RwInt32 RwTextureRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                       RwPluginObjectConstructor constructCB,
                                       RwPluginObjectDestructor destructCB,
                                       RwPluginObjectCopy copyCB);

extern void GXSetProjectionv(const f32* ptr);
extern OSThread* GXSetCurrentGXThread(void);
extern void GXSetFieldMode(GXBool field_mode, GXBool half_aspect_ratio);
extern u16 GXReadDrawSync(void);

static inline void GXPosition2s16(const s16 x, const s16 y)
{
    GXWGFifo.s16 = x;
    GXWGFifo.s16 = y;
}

/* Render state cache (dlrendst.c) */
typedef struct _rwDlStateCache _rwDlStateCache;
struct _rwDlStateCache
{
    RwBool zWriteEnable; /* 0x00 */
    RwBool zTestEnable; /* 0x04 */
    GXCompare zCompare; /* 0x08 */
    RwCullMode cullMode; /* 0x0C */
    RwBool fogEnable; /* 0x10 */
    RwFogType fogType; /* 0x14 */
    RwUInt32 packedFogColor; /* 0x18 */
    GXColor fogColor; /* 0x1C */
    RwBool userFarFogPlane; /* 0x20 */
    RwReal nearFogPlane; /* 0x24 */
    RwReal farFogPlane; /* 0x28 */
    RwReal nearPlane; /* 0x2C */
    RwReal farPlane; /* 0x30 */
    RwBlendFunction srcBlend; /* 0x34 */
    RwBlendFunction dstBlend; /* 0x38 */
    GXBlendFactor gxSrcBlend; /* 0x3C */
    GXBlendFactor gxDstBlend; /* 0x40 */
    RwBool zBeforeTex; /* 0x44 */
    GXCompare comp0; /* 0x48 */
    GXCompare comp1; /* 0x4C */
    GXAlphaOp op; /* 0x50 */
    RwUInt8 ref0; /* 0x54 */
    RwUInt8 ref1; /* 0x55 */
    RwUInt8 pad[2]; /* 0x56 */
};

/* dl2drend.c */
extern RwBool _rwDlIm2DRenderTriangle(RwIm2DVertex* verts, RwInt32 numVerts, RwInt32 vert1,
                                      RwInt32 vert2, RwInt32 vert3);
extern RwBool _rwDlIm2DRenderLine(RwIm2DVertex* verts, RwInt32 numVerts, RwInt32 vert1,
                                  RwInt32 vert2);
extern RwBool _rwDlIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* verts,
                                       RwInt32 numVertices);
extern RwBool _rwDlIm2DRenderIndexedPrimitive(RwPrimitiveType primType, RwIm2DVertex* verts,
                                              RwInt32 numVertices, RwImVertexIndex* indices,
                                              RwInt32 numIndices);

/* dlconvrt.c */
extern RwInt32 _rwDlFindMSB(RwInt32 num);
extern RwBool _rwDlRGBToPixel(void* pixelOut, void* colIn, RwInt32 format);
extern RwBool _rwDlPixelToRGB(void* rgbOut, void* pixel, RwInt32 format);
extern RwBool _rwDlImageGetFromRaster(void* imageIn, void* rasterIn, RwInt32 flags);
extern RwBool _rwDlRasterSetFromImage(void* rasterIn, void* imageIn, RwInt32 flags);
extern RwBool _rwDlImageFindRasterFormat(void* rasterIn, void* imageIn, RwInt32 flags);

/* dldevice.c */
extern GXRenderModeObj* _RwDlRenderMode;
extern RwInt32 _RwDlFSAA;
extern RwInt32 _RwDlFSAATop;
extern RwInt32 _RwDlHalfHeight;
extern GXPixelFmt _RwDlPixelFormat;
extern GXPixelFmt _RwDlCurPixelFormat;

/* dlraster.c */
extern void _rwDlRasterPluginAttach(void);
extern RwUInt32 _rwDlRasterGetSize(RwRaster* raster);
extern RwBool _rwDlRasterGetNumMipLevels(void* mipLevels, void* rasterIn, RwInt32 flags);
extern RwBool _rwDlRasterLock(void* pixelsIn, void* rasterIn, RwInt32 accessMode);
extern RwBool _rwDlRasterUnlock(void* unused1, void* rasterIn, RwInt32 unused3);
extern RwBool _rwDlRasterLockPalette(void* paletteIn, void* rasterIn, RwInt32 accessMode);
extern RwBool _rwDlRasterUnlockPalette(void* unused1, void* rasterIn, RwInt32 unused3);
extern RwBool _rwDlTextureRasterCreate(RwRaster* raster, RwUInt8 numLods);
extern RwBool _rwDlRasterCreate(void* unused1, void* rasterIn, RwInt32 flags);
extern RwBool _rwDlRasterDestroy(void* unused1, void* rasterIn, RwInt32 unused3);
extern RwBool _rwDlTextureSetRaster(void* textureIn, void* rasterIn, RwInt32 flags);
extern RwBool _rwDlRasterSubRaster(void* raster, void* pIn, RwInt32 flags);

/* dlrendst.c */
extern void _rwDlRenderStateOpen(void);
extern void _rwDlRenderStateClose(void);
extern _rwDlStateCache _RwDlStateCache;
extern RwRaster* _RwDlRasterWhite;
extern RwTexture* _RwDlTexture;
extern RwBool _rwDlGetRenderState(RwRenderState state, void* param);
extern RwBool _rwDlSetRenderState(RwRenderState state, void* param);
extern RwBool _rwDlRenderStateFogEnable(RwBool fog);
extern void _rwDlTextureRasterFlush(void);

/* dlsprite.c */
extern RwBool _rwDlRasterRender(void* rasterIn, void* rectIn, RwInt32 flags);
extern RwBool _rwDlRasterRenderFast(void* rasterIn, void* rectIn, RwInt32 flags);
extern RwBool _rwDlRasterRenderScaled(void* rasterIn, void* rectIn, RwInt32 flags);
extern void _rwDlRasterCamera_ZClearRect(RwRaster* raster, RwRect* rect, RwRGBA* color,
                                         RwInt32 clearFlags);
extern RwBool _rwDlRasterClearRect(void* unused1, void* rectIn, RwInt32 packedColor);
extern RwBool _rwDlRasterClear(void* unused1, void* unused2, RwInt32 packedColor);
extern RwBool _rwDlSetRasterContext(void* unused1, void* rasIn, RwInt32 unused3);

/* dltexdic.c */
extern RwBool _rwDlNativeTextureGetSize(void* sizeIn, void* textureIn, RwInt32 flags);
extern RwBool _rwDlNativeTextureWrite(void* streamIn, void* textureIn, RwInt32 flags);
extern RwBool _rwDlNativeTextureRead(void* streamIn, void* textureIn, RwInt32 flags);

/* dltextur.c */
extern void _rwDlTexturePluginAttach(void);
extern void _rwDlTextureCacheInit(void);
extern void _rwDlTextureSet(RwTexture* texture, RwInt32 index);
extern void RwGameCubeTextureSetLOD(RwTexture* texture, RwReal lodBias, RwBool biasClamp,
                                    RwBool edgeLod, RwUInt32 maxAniso);

/* dltoken.c */
extern volatile RwUInt16 _RwDlTokenCurrent;
extern volatile RwUInt16 _RwDlTokenLastSeen;
extern RwBool _rwDlTokenQueryDone(RwUInt16 token);

#endif
