#ifndef GCPIPE_H
#define GCPIPE_H

#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/gx.h>

/* GameCube pipeline private types and declarations */

#ifndef rpATOMIC
#define rpATOMIC 1
#endif

typedef struct RxGameCubeVertexAttr RxGameCubeVertexAttr;
struct RxGameCubeVertexAttr
{
    void* array;
    RwUInt8 attr;
    RwUInt8 stride;
    RwUInt8 indexType;
    RwUInt8 pad[1];
};

typedef struct RxGameCubeVertexBuffer RxGameCubeVertexBuffer;
struct RxGameCubeVertexBuffer
{
    RwUInt16 token;
    RwUInt16 serialNumber;
    RwUInt32 flags;
    RwUInt32 numAttrArrays;
    RxGameCubeVertexAttr attr[1];
};

typedef struct RxGameCubeDisplayList RxGameCubeDisplayList;
struct RxGameCubeDisplayList
{
    void* displayList;
    RwUInt32 size;
};

typedef struct RxGameCubePipeData RxGameCubePipeData;
struct RxGameCubePipeData
{
    RwResEntry* resEntry;
    RpMeshHeader* meshHeader;
    RwInt32 flags;
    RwRGBAReal ambientLightColor;
    RwBool ambientLight;
    RwUInt32 lightMask;
    RwInt32 numLights;
    void* nodeData;
};

typedef void* (*RxGameCubeAllInOneCallBack)(void* object, RxGameCubePipeData* pipeData);

typedef struct _rxGameCubeAllInOneNodeData _rxGameCubeAllInOneNodeData;
struct _rxGameCubeAllInOneNodeData
{
    RxGameCubeAllInOneCallBack instanceCallback;
    RxGameCubeAllInOneCallBack reinstanceCallback;
    RxGameCubeAllInOneCallBack lightingCallback;
    RxGameCubeAllInOneCallBack renderCallback;
};

typedef struct RpGameCubeVtxFmt RpGameCubeVtxFmt;
struct RpGameCubeVtxFmt
{
    RwUInt8 pos;
    RwUInt8 norm;
    RwUInt8 texCoord[8];
    RwUInt8 preLight;
    RwUInt8 format;
    RwUInt8 posFrac;
    RwUInt8 nbt;
    RwUInt8 texCoordFrac[8];
    RwUInt16 refCnt;
};

typedef struct RwGameCubeRasterExtension RwGameCubeRasterExtension;
struct RwGameCubeRasterExtension
{
    RwUInt32 tlutObj[3];
    RwUInt32 format;
    RwUInt32 tlutFmt;
    RwUInt32 flags;
    RwUInt8* memory;
    RwUInt8* pixels;
    RwUInt8* palette;
    RwUInt8* lockedPixels;
    RwUInt8* lockedBuffer;
    void* region;
    RwUInt16 token;
    RwUInt8 maxLOD;
    RwUInt8 lockedMipLevel;
};

typedef void (*RwDlMatFunc)(RwRGBAReal* ambient, RwRGBA* color, RwReal ambientCoef);

#ifndef RWMODULEINFO_DEFINED
#define RWMODULEINFO_DEFINED
typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};
#endif

/* Resource arena (baresour.c) */
typedef struct rwResources rwResources;
struct rwResources
{
    RwInt32 maxSize;
    RwInt32 currentSize;
    RwInt32 reusageSize;
    void* memHeap;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLinkList* freeEntries;
    RwLinkList* usedEntries;
};

extern RwModuleInfo resourcesModule;

#define RWRESOURCESGLOBAL(var)                                                                         (RWPLUGINOFFSET(rwResources, RwEngineInstance, resourcesModule.globalsOffset)->var)

#define RwResourcesUseResEntry(_ntry)                                                                  ((((_ntry)->link.next) ? (rwLinkListRemoveLLLink(&((_ntry)->link)),                                                          rwLinkListAddLLLink(RWRESOURCESGLOBAL(usedEntries), &((_ntry)->link)))                            : NULL),                                                                     (_ntry))

extern RwInt32 _RwGameCubeRasterExtOffset;
#define RASTEREXTFROMRASTER(raster)                                                                \
    (RWPLUGINOFFSET(RwGameCubeRasterExtension, (raster), _RwGameCubeRasterExtOffset))

enum RpGameCubeCompType
{
    rpU8 = 0,
    rpS8 = 1,
    rpU16 = 2,
    rpS16 = 3,
    rpF32 = 4,
    rpGAMECUBECOMPFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpGameCubeCompType RpGameCubeCompType;

enum RpGameCubeColorCompType
{
    rpRGB565 = 0,
    rpRGB8 = 1,
    rpRGBX8 = 2,
    rpRGBA4 = 3,
    rpRGBA6 = 4,
    rpRGBA8 = 5,
    rpGAMECUBECOLORCOMPFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpGameCubeColorCompType RpGameCubeColorCompType;

extern RwInt32 _rpDlGeomVtxFmtOffset;
extern RwInt32 _rpDlWorldVtxFmtOffset;

#define GEOMVTXFMT(geom) (*RWPLUGINOFFSET(RpGameCubeVtxFmt*, (geom), _rpDlGeomVtxFmtOffset))
#define WORLDVTXFMT(world) (*RWPLUGINOFFSET(RpGameCubeVtxFmt*, (world), _rpDlWorldVtxFmtOffset))

extern volatile RwUInt16 _RwDlTokenCurrent;
extern RwBool _RwDlPreInstanceOptimize;

/* driver */
extern RwBool _rwDlTokenQueryDone(RwUInt16 token);
extern void _rwDlTransformSetup(RwMatrix* ltm, RwBool normals);
extern RwBool _rwDlTextureSet(RwTexture* texture, RwInt32 stage);
extern void _rwDlRenderStateSetZCompLoc(RwInt32 zBeforeTex);

/* pipe */
extern void _rwDlVtxFmtSetup(RpGameCubeVtxFmt* vtxFmt, RxGameCubePipeData* pipeData);
extern RwDlMatFunc _rwDlObjectRenderSetup(RwUInt32 flags, RwUInt32 lightMask, RwBool ambientLight,
                                          RwBool preLit);

#ifndef RpGeometryGetFlags
#define RpGeometryGetFlags(_geometry) ((_geometry)->flags)
#endif

#ifndef rpGEOMETRYLOCKVERTICES
#define rpGEOMETRYLOCKPOLYGONS 0x01
#define rpGEOMETRYLOCKVERTICES 0x02
#define rpGEOMETRYLOCKNORMALS 0x04
#define rpGEOMETRYLOCKPRELIGHT 0x08
#define rpGEOMETRYLOCKTEXCOORDS 0x10
#define rpGEOMETRYLOCKTEXCOORDS1 0x10
#endif

/* Wait until the GPU has finished with a vertex buffer before touching it */
#define RXGCVERTEXBUFFERWAITDONE(_vbHeader)                                                            MACRO_START                                                                                        {                                                                                                      if ((_vbHeader)->token == _RwDlTokenCurrent)                                                       {                                                                                                      GXSetDrawSync(_RwDlTokenCurrent);                                                                  _RwDlTokenCurrent = (_RwDlTokenCurrent + 1) % 0xE000;                                          }                                                                                                                                                                                                     while (!_rwDlTokenQueryDone((_vbHeader)->token))                                                   {                                                                                                  }                                                                                              }                                                                                                  MACRO_STOP

/* lights (gclights.c) */
extern void _rwGCLightsGlobalEnable(RpLightFlag lightFlags, RxGameCubePipeData* pipeData);
extern void _rwGCLightsLocalEnable(const RpLight* light, RxGameCubePipeData* pipeData);

/* instancing */
extern RwResEntry* _rwDlWorldSectorInstanceFast(RpWorld* world, RpWorldSector* sector, void* owner,
                                                RwResEntry** resEntryOwner);

extern RwResEntry* _rwDlGeometryInstanceOptimized(RpGeometry* geometry, void* owner,
                                                  RwResEntry** resEntryOwner);
extern RwResEntry* _rwDlGeometryInstanceFast(RpGeometry* geometry, void* owner,
                                             RwResEntry** resEntryOwner);

/* geominst.c */
extern RwUInt32 _rwGCNVtxFmtInstPos3D(RwUInt8* mem, RwV3d* srcPosition, RwUInt32 fmt,
                                      RwReal scale, RwInt32 numVerts, RwUInt32 stride);
extern RwUInt32 _rwGCNVtxFmtInstNrm(RwUInt8* mem, RwV3d* srcNormal, RwUInt32 fmt,
                                    RwInt32 numVerts, RwUInt32 stride);
extern RwUInt32 _rwGCNVtxFmtInstNrmCmp(RwUInt8* mem, RpVertexNormal* srcNormal, RwUInt32 fmt,
                                       RwInt32 numVerts, RwUInt32 stride);
extern RwUInt32 _rwGCNVtxFmtInstNBT(RwUInt8* mem, RwV3d* srcNormal, RwUInt32 fmt,
                                    RwInt32 numVerts, RwUInt32 stride);
extern RwUInt32 _rwGCNVtxFmtInstNBTCmp(RwUInt8* mem, RpVertexNormal* srcNormal, RwUInt32 fmt,
                                       RwInt32 numVerts, RwUInt32 stride);
extern RwUInt32 _rwGCNVtxFmtInstClr(RwUInt8* mem, RwRGBA* srcColor, RwUInt32 fmt,
                                    RwInt32 numVerts, RwUInt32 stride);
extern RwUInt32 _rwGCNVtxFmtInstTex(RwUInt8* mem, RwTexCoords* srcTexCoord, RwUInt32 fmt,
                                    RwReal scale, RwInt32 numVerts, RwUInt32 stride);

/* gcmorph.c */
extern void _rxGCInstanceMorphUpdate(RpGeometry* geometry, RxGameCubeVertexBuffer* vbHeader,
                                     RpInterpolator* interp);

/* vtxfmt.c */
extern RpGameCubeVtxFmt* _rpGameCubeVtxFmtGetDefault(void);

/* gcpipe.c */
extern void _rxGCResEntryWaitDone(RwResEntry* resEntry);
extern void* _rxGCDefaultRenderCallback(void* object, RxGameCubePipeData* pipeData);

/* plugins */
extern RwBool _rpDlVtxFmtPluginAttach(void);
extern RwBool _rpDlLightPluginAttach(void);

/* Dolphin GX */
extern void GXSetDrawSync(u16 token);

#endif
