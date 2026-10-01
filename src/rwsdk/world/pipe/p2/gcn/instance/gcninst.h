#ifndef GCNINST_H
#define GCNINST_H

#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

/* GameCube instancing private types and declarations */

extern volatile RwUInt16 _RwDlTokenLastSeen;

enum rwDataType
{
    rwNADATATYPE = 0,
    rwDATATYPE_INT8 = 1,
    rwDATATYPE_INT16 = 2,
    rwDATATYPE_INT24 = 3,
    rwDATATYPE_INT32 = 4,
    rwDATATYPE_INT64 = 5,
    rwDATATYPE_REAL = 6,
    rwDATATYPE_V2D = 7,
    rwDATATYPE_V3D = 8,
    rwDATATYPE_V4D = 9,
    rwDATATYPE_RGBA = 10,
    rwDATATYPE_FORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum rwDataType rwDataType;

typedef struct rwGCNVtxData rwGCNVtxData;
struct rwGCNVtxData
{
    void* data;
    rwDataType type;
    RwInt8 dep[16];
};

typedef struct rwGCNVtxDataMap rwGCNVtxDataMap;
struct rwGCNVtxDataMap
{
    RwInt32* map;
    RwUInt32 num;
};

typedef struct rwGCNIndexData rwGCNIndexData;
struct rwGCNIndexData
{
    RwUInt16* indices;
};

typedef struct rwGCNVtxDataRemapped rwGCNVtxDataRemapped;
struct rwGCNVtxDataRemapped
{
    void* data;
    RwUInt32 num;
};

typedef struct rwGCNIndexDataRemapped rwGCNIndexDataRemapped;
struct rwGCNIndexDataRemapped
{
    RwUInt16* indices;
};

typedef struct rwVertexDescriptor rwVertexDescriptor;
struct rwVertexDescriptor
{
    RwUInt32 VATID;
    RwUInt32 VATRegA;
    RwUInt32 VATRegB;
    RwUInt32 VATRegC;
    RwUInt32 VCDRegLO;
    RwUInt32 VCDRegHI;
    RwUInt32 XFRegINVTXSPEC;
    RwUInt8 numAttrArrays;
    RwUInt8 pad[3];
};

#define rwGCNMAXVERTEXATTRIBUTES 26

typedef struct rwGCNVertexBufferData rwGCNVertexBufferData;
struct rwGCNVertexBufferData
{
    RwUInt32 num[rwGCNMAXVERTEXATTRIBUTES];
    void* data[rwGCNMAXVERTEXATTRIBUTES];
};

typedef struct rwGCNDisplayListData rwGCNDisplayListData;
struct rwGCNDisplayListData
{
    void* data[rwGCNMAXVERTEXATTRIBUTES];
};

enum rwGCNVertexAttribute
{
    rwGCNVA_PNMTXIDX = 0,
    rwGCNVA_TEX0MTXIDX = 1,
    rwGCNVA_TEX1MTXIDX = 2,
    rwGCNVA_TEX2MTXIDX = 3,
    rwGCNVA_TEX3MTXIDX = 4,
    rwGCNVA_TEX4MTXIDX = 5,
    rwGCNVA_TEX5MTXIDX = 6,
    rwGCNVA_TEX6MTXIDX = 7,
    rwGCNVA_TEX7MTXIDX = 8,
    rwGCNVA_POS = 9,
    rwGCNVA_NRM = 10,
    rwGCNVA_CLR0 = 11,
    rwGCNVA_CLR1 = 12,
    rwGCNVA_TEX0 = 13,
    rwGCNVA_TEX1 = 14,
    rwGCNVA_TEX2 = 15,
    rwGCNVA_TEX3 = 16,
    rwGCNVA_TEX4 = 17,
    rwGCNVA_TEX5 = 18,
    rwGCNVA_TEX6 = 19,
    rwGCNVA_TEX7 = 20,
    rwGCNVA_NBT = 25,
    rwGCNVA_MAX_ATTR = 26,
    rwGCNVA_NULL = 255,
    rwGCNVA_MAX_ENUM = RWFORCEENUMSIZEINT
};
typedef enum rwGCNVertexAttribute rwGCNVertexAttribute;

enum rwGCNCompCnt
{
    rwGCNCC_POS_XY = 0,
    rwGCNCC_POS_XYZ = 1,
    rwGCNCC_NRM_XYZ = 0,
    rwGCNCC_NRM_NBT = 1,
    rwGCNCC_NRM_NBT3 = 2,
    rwGCNCC_CLR_RGB = 0,
    rwGCNCC_CLR_RGBA = 1,
    rwGCNCC_TEX_S = 0,
    rwGCNCC_TEX_ST = 1,
    rwGCNCC_MAX_ENUM = RWFORCEENUMSIZEINT
};
typedef enum rwGCNCompCnt rwGCNCompCnt;

enum rwGCNCompType
{
    rwGCNCT_U8 = 0,
    rwGCNCT_S8 = 1,
    rwGCNCT_U16 = 2,
    rwGCNCT_S16 = 3,
    rwGCNCT_F32 = 4,
    rwGCNCT_RGB565 = 0,
    rwGCNCT_RGB8 = 1,
    rwGCNCT_RGBX8 = 2,
    rwGCNCT_RGBA4 = 3,
    rwGCNCT_RGBA6 = 4,
    rwGCNCT_RGBA8 = 5,
    rwGCNCT_MAX_ENUM = RWFORCEENUMSIZEINT
};
typedef enum rwGCNCompType rwGCNCompType;

enum rwGCNAttrType
{
    rwGCNAT_NONE = 0,
    rwGCNAT_DIRECT = 1,
    rwGCNAT_INDEX8 = 2,
    rwGCNAT_INDEX16 = 3,
    rwGCNAT_MAX_ENUM = RWFORCEENUMSIZEINT
};
typedef enum rwGCNAttrType rwGCNAttrType;

/* geomcond.c */
extern rwGCNIndexDataRemapped* IndexDataCreateRemapped(rwGCNVtxDataMap* vtxDataMap,
                                                       rwGCNIndexData* indexData,
                                                       RwUInt32 numEntries, RwUInt32 numIndices);
extern rwGCNVtxDataRemapped* VertexDataCreateRemapped(rwGCNVtxDataMap* vtxDataMap,
                                                      rwGCNVtxData* vtxData, RwUInt32 numEntries,
                                                      RwUInt32 numVerts);
extern rwGCNVtxDataMap* VertexDataCreateMaps(rwGCNVtxData* vtxData, RwUInt32 numEntries,
                                             RwUInt32 numVerts);

/* itools.c */
extern void _rwGCNTriStripGetStats(RwUInt16* indices, RwUInt32 numIndices, RwUInt32* numStripsOut,
                                   RwUInt32* numIndicesOut, RwBool preserveWindingOrder);
extern void _rwGCNInstanceIndices(RwUInt16* posIndices, RwUInt16* indices, RwUInt32 numIndices,
                                  RwUInt32 numStrips, RwUInt32 stride, RwUInt32 indexType,
                                  RwBool preserveWindingOrder, void* memory);
extern void _rwGCNDisplayListFill(rwVertexDescriptor* vtxDesc, RxGameCubeDisplayList* displayList,
                                  rwGCNDisplayListData* displayListData, RwUInt32 numIndices,
                                  RwUInt32 numStrips, RwUInt32 stride, RwBool preserveWindingOrder,
                                  RwUInt8 primTypeVAT);

/* geominst.c */
extern RwUInt32 rwGCNPosGetSize(rwVertexDescriptor* vtxDesc);
extern RwUInt32 rwGCNNrmGetSize(rwVertexDescriptor* vtxDesc);
extern RwUInt32 rwGCNClrGetSize(rwVertexDescriptor* vtxDesc, RwUInt8 clrNum);
extern RwUInt32 rwGCNTexGetSize(rwVertexDescriptor* vtxDesc, RwUInt8 texNum);

/* ibuffer.c */
extern RwUInt32 _rwGCNDisplayListGetStride(rwVertexDescriptor* vtxDesc);
extern RwUInt32 _rwGCNDisplayListGetSize(rwVertexDescriptor* vtxDesc, RwUInt32 numStrips,
                                         RwUInt32 numIndices);
extern void _rwGCNDisplayListInitialize(RxGameCubeDisplayList* displayList, RwUInt32 index,
                                        RwUInt32 displayListSize, void* memory);

/* vbuffer.c */
extern RwUInt32 _rwGCNVertexBufferHeaderGetSize(rwVertexDescriptor* vtxDesc);
extern RwUInt32 _rwGCNVertexBufferGetSize(rwVertexDescriptor* vtxDesc,
                                          rwGCNVertexBufferData* vtxBufData);
extern void _rwGCNVertexBufferInitialize(rwVertexDescriptor* vtxDesc,
                                         RxGameCubeVertexBuffer* vtxBufHeader,
                                         rwGCNVertexBufferData* vtxBufData, RwUInt8* memory);

/* vtools.c */
extern void _rwGCNVertexBufferFill(rwVertexDescriptor* vtxDesc, RxGameCubeVertexBuffer* vtxBufHeader,
                                   rwGCNVertexBufferData* vtxBufData, RwBool cmpNrm);

/* vtxdesc.c */
extern rwVertexDescriptor* _rwVertexDescriptorInit(rwVertexDescriptor* vtxDesc);
extern void _rwGCNVertexDescSetVAT(rwVertexDescriptor* vtxDesc, RwUInt32 vat);
extern void _rwGCNVertexDescSetElementAttr(rwVertexDescriptor* vtxDesc, rwGCNVertexAttribute attr,
                                           rwGCNCompCnt cnt, rwGCNCompType fmt, RwUInt8 frac);
extern void _rwGCNVertexDescSetElementDesc(rwVertexDescriptor* vtxDesc, rwGCNVertexAttribute attr,
                                           rwGCNAttrType type);
extern void _rwGCNVertexDescSetNumIndexedAttr(rwVertexDescriptor* vtxDesc, RwUInt8 numIndxAttr);

#endif
