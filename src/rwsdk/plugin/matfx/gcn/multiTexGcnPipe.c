#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"
#include "rwsdk/plugin/matfx/gcn/mtgcnprivate.h"

#define rwID_GCNMULTITEXPLUGIN MAKECHUNKID(rwVENDORID_CRITERIONTK, 0x29)

/* RpGameCubeMTConfig flags */
#define rpGAMECUBEMTCONFIGREGS 0x01
#define rpGAMECUBEMTCONFIGKREGS 0x02
#define rpGAMECUBEMTCONFIGBLEND 0x04
#define rpGAMECUBEMTCONFIGZCOMPAFTERTEX 0x08
#define rpGAMECUBEMTCONFIGCHANNELCOLOR 0x10
#define rpGAMECUBEMTCONFIGENVMTX 0x20

/* per-element flags */
#define rpGAMECUBEMTDISABLE 0x01
#define rpGAMECUBEMTTEXMTX2x4 0x01
#define rpGAMECUBEMTTEXMTXTRANSLATE 0x02
#define rpGAMECUBEMTTEXGENNORMALIZE 0x02
#define rpGAMECUBEMTTEXGENSCALEMANUALLY 0x04

enum _TevRasSrc
{
    NATevRasSrc = 0,
    TEVRAS_NORM = 1,
    TEVRAS_CH0A0 = 2
};
typedef enum _TevRasSrc _TevRasSrc;

typedef RpMaterial* (*RpGameCubeMTCallBack)(RpMaterial* material, void* object,
                                            RxGameCubePipeData* pipeData, void* data);

typedef struct rpMultiTextureGameCubeExt rpMultiTextureGameCubeExt;
struct rpMultiTextureGameCubeExt
{
    RpGameCubeMTCallBack preRenderCallBack;
    RpGameCubeMTCallBack postRenderCallBack;
    void* callBackData;
};

typedef struct rpGameCubeMTGlobals rpGameCubeMTGlobals;
struct rpGameCubeMTGlobals
{
    RwUInt32 numFrames;
    RwFrame** frames;
};

#define RPGAMECUBEMTGLOBAL(var)                                                                    \
    (RWPLUGINOFFSET(rpGameCubeMTGlobals, RwEngineInstance, _rpGameCubeMTEngineOffset)->var)

extern RwMatrix _RwDlInvCamLTM;

RwInt32 _rpGameCubeMTEngineOffset;

static void* GameCubeMTOpen(void* object, RwInt32 offset, RwInt32 size)
{
    memset(RWPLUGINOFFSET(rpGameCubeMTGlobals, RwEngineInstance, _rpGameCubeMTEngineOffset), 0,
           sizeof(rpGameCubeMTGlobals));

    return object;
}

static void* GameCubeMTClose(void* object, RwInt32 offset, RwInt32 size)
{
    return object;
}

/*
 * Pointers to the data of one triangle, used by CalcNBT to calculate the
 * binormal and tangent of its first vertex. The component sizes are in bytes
 * and the quantization of each stream is held in GQR5 (pos), GQR6 (nbt) and
 * GQR7 (uv).
 */
typedef struct NBTCalcData NBTCalcData;
struct NBTCalcData
{
    RwUInt8* pos[3];
    RwUInt8* nbt;
    RwUInt8* uv[3];
    RwUInt8 posSize;
    RwUInt8 nbtSize;
    RwUInt8 uvSize;
};

static void TriStripNBTDataSetup8(NBTCalcData* data, RwUInt8* indices, RwUInt32 stride,
                                  RwUInt8* pos, RwUInt8* nbt, RwUInt8* uv, RwInt32 index)
{
    RwUInt8 i0;
    RwUInt8 i1;
    RwUInt8 i2;
    RwUInt8 next;
    RwUInt8 tmp;

    i0 = indices[stride * index];

    if (index < 2)
    {
        if (index == 0)
        {
            i1 = indices[stride];
            i2 = indices[stride * 2];
        }
        else
        {
            i1 = indices[stride * 2];
            i2 = indices[0];
        }
    }
    else
    {
        i1 = indices[stride * (index - 2)];
        i2 = indices[stride * (index - 1)];

        /* Step over degenerate triangles */
        if (i1 == i2)
        {
            next = indices[stride * (index + 1)];

            if (i0 == next)
            {
                index++;
                i1 = indices[stride * (index + 1)];

                if (i0 == i1)
                {
                    index++;
                    i1 = indices[stride * (index + 1)];
                }

                i2 = indices[stride * (index + 2)];
            }
            else
            {
                i1 = i2;
                i2 = next;
            }
        }

        if (index & 1)
        {
            tmp = i2;
            i2 = i1;
            i1 = tmp;
        }
    }

    data->pos[0] = pos + i0 * (data->posSize * 3);
    data->pos[1] = pos + i1 * (data->posSize * 3);
    data->pos[2] = pos + i2 * (data->posSize * 3);
    data->nbt = nbt + i0 * (data->nbtSize * 9);
    data->uv[0] = uv + i0 * (data->uvSize * 2);
    data->uv[1] = uv + i1 * (data->uvSize * 2);
    data->uv[2] = uv + i2 * (data->uvSize * 2);
}

static void TriStripNBTDataSetup16(NBTCalcData* data, RwUInt8* indices, RwUInt32 stride,
                                   RwUInt8* pos, RwUInt8* nbt, RwUInt8* uv, RwInt32 index)
{
    RwUInt16 i0;
    RwUInt16 i1;
    RwUInt16 i2;
    RwUInt16 next;
    RwUInt16 tmp;

    i0 = *(RwUInt16*)&indices[stride * index];

    if (index < 2)
    {
        if (index == 0)
        {
            i1 = *(RwUInt16*)&indices[stride];
            i2 = *(RwUInt16*)&indices[stride * 2];
        }
        else
        {
            i1 = *(RwUInt16*)&indices[stride * 2];
            i2 = *(RwUInt16*)&indices[0];
        }
    }
    else
    {
        i1 = *(RwUInt16*)&indices[stride * (index - 2)];
        i2 = *(RwUInt16*)&indices[stride * (index - 1)];

        /* Step over degenerate triangles */
        if (i1 == i2)
        {
            next = *(RwUInt16*)&indices[stride * (index + 1)];

            if (i0 == next)
            {
                index++;
                i1 = *(RwUInt16*)&indices[stride * (index + 1)];

                if (i0 == i1)
                {
                    index++;
                    i1 = *(RwUInt16*)&indices[stride * (index + 1)];
                }

                i2 = *(RwUInt16*)&indices[stride * (index + 2)];
            }
            else
            {
                i1 = i2;
                i2 = next;
            }
        }

        if (index & 1)
        {
            tmp = i2;
            i2 = i1;
            i1 = tmp;
        }
    }

    data->pos[0] = pos + i0 * (data->posSize * 3);
    data->pos[1] = pos + i1 * (data->posSize * 3);
    data->pos[2] = pos + i2 * (data->posSize * 3);
    data->nbt = nbt + i0 * (data->nbtSize * 9);
    data->uv[0] = uv + i0 * (data->uvSize * 2);
    data->uv[1] = uv + i1 * (data->uvSize * 2);
    data->uv[2] = uv + i2 * (data->uvSize * 2);
}

/*
 * Paired single calculation of the binormal and tangent of the first vertex
 * of a triangle, from its positions and texture coordinates. The binormal is
 * made orthogonal to the normal and normalized, the tangent is the cross
 * product of the normal and the binormal.
 */
static asm void CalcNBT(register NBTCalcData* data)
{
    nofralloc
    stwu r1, -0x10(r1)
    mfcr r0
    stw r0, 0xc(r1)

    /* positions -> (f0,f1), (f2,f3), (f4,f5) */
    lbz r4, NBTCalcData.posSize(data)
    lwz r5, NBTCalcData.pos[0](data)
    rlwinm r4, r4, 1, 0, 31
    psq_l f0, 0x0(r5), 0, 5
    add r5, r5, r4
    psq_l f1, 0x0(r5), 1, 5
    lwz r5, NBTCalcData.pos[1](data)
    psq_l f2, 0x0(r5), 0, 5
    add r5, r5, r4
    psq_l f3, 0x0(r5), 1, 5
    lwz r5, NBTCalcData.pos[2](data)
    psq_l f4, 0x0(r5), 0, 5
    add r5, r5, r4
    psq_l f5, 0x0(r5), 1, 5

    /* texture coordinates -> f6, f7, f8 */
    lwz r5, NBTCalcData.uv[0](data)
    psq_l f6, 0x0(r5), 0, 7
    lwz r5, NBTCalcData.uv[1](data)
    psq_l f7, 0x0(r5), 0, 7
    lwz r5, NBTCalcData.uv[2](data)
    psq_l f8, 0x0(r5), 0, 7

    /* edges */
    ps_sub f9, f2, f0
    ps_sub f11, f4, f0
    lbz r4, NBTCalcData.nbtSize(data)
    ps_sub f0, f7, f6
    lwz r6, NBTCalcData.nbt(data)
    ps_sub f12, f5, f1
    ps_sub f10, f3, f1
    ps_sub f1, f8, f6
    rlwinm r5, r4, 1, 0, 31

    /* binormal */
    ps_muls1 f4, f11, f0
    ps_muls1 f5, f12, f0
    ps_muls1 f2, f9, f1
    ps_muls1 f3, f10, f1
    ps_merge10 f1, f1, f1
    ps_sub f6, f4, f2
    ps_sub f7, f5, f3

    /* handedness */
    ps_mul f0, f0, f1
    ps_merge10 f1, f0, f0
    ps_cmpu0 cr2, f1, f0

    /* normal -> (f8,f9) */
    psq_l f8, 0x0(r6), 0, 6
    add r6, r6, r5
    psq_l f9, 0x0(r6), 1, 6
    ps_mul f10, f8, f6
    ps_sub f4, f4, f4
    ps_mul f11, f9, f7
    add r6, r6, r4

    /* binormal -= normal * dot(normal, binormal) */
    ps_sum1 f12, f10, f10, f10
    ps_sum0 f12, f11, f11, f12
    ps_merge00 f12, f12, f12
    ps_neg f12, f12
    ps_madd f0, f8, f12, f6
    ps_madd f1, f9, f12, f7

    /* normalize the binormal */
    ps_mul f2, f0, f0
    ps_mul f3, f1, f1
    ps_sum1 f12, f2, f2, f2
    ps_sum0 f12, f3, f3, f12
    ps_cmpu0 cr1, f12, f4
    ble cr1, skip_normalize
    ps_rsqrte f12, f12
    ps_muls0 f0, f0, f12
    ps_muls0 f1, f1, f12

skip_normalize:
    /* tangent = normal x binormal */
    ps_muls0 f3, f8, f1
    ps_merge10 f8, f8, f8
    ps_muls0 f2, f0, f9
    ps_mul f4, f0, f8
    ps_merge10 f5, f2, f3
    ps_merge10 f6, f3, f2
    ps_merge10 f7, f4, f4
    ps_sub f2, f5, f6
    ps_sub f3, f4, f7

    bge cr2, skip_negate
    ps_neg f0, f0
    ps_neg f1, f1

skip_negate:
    /* store the binormal and tangent */
    psq_st f0, 0x0(r6), 0, 6
    add r6, r6, r5
    psq_st f1, 0x0(r6), 1, 6
    add r6, r6, r4
    psq_st f2, 0x0(r6), 0, 6
    add r6, r6, r5
    psq_st f3, 0x0(r6), 1, 6

    lwz r12, 0xc(r1)
    mtcrf 255, r12
    addi r1, r1, 0x10
    blr
}

static void TriListNBTDataSetup8(NBTCalcData* data, RwUInt8* indices, RwUInt32 stride,
                                  RwUInt8* pos, RwUInt8* nbt, RwUInt8* uv, RwInt32 index)
{
    RwUInt8 i0;
    RwUInt8 i1;
    RwUInt8 i2;

    i0 = indices[stride * index];

    switch (index % 3)
    {
        case 0:
            i1 = indices[stride * (index + 1)];
            i2 = indices[stride * (index + 2)];
            break;
        case 1:
            i1 = indices[stride * (index + 1)];
            i2 = indices[stride * (index - 1)];
            break;
        case 2:
            i1 = indices[stride * (index - 2)];
            i2 = indices[stride * (index - 1)];
            break;
        default:
            return;
    }

    data->pos[0] = pos + i0 * (data->posSize * 3);
    data->pos[1] = pos + i1 * (data->posSize * 3);
    data->pos[2] = pos + i2 * (data->posSize * 3);
    data->nbt = nbt + i0 * (data->nbtSize * 9);
    data->uv[0] = uv + i0 * (data->uvSize * 2);
    data->uv[1] = uv + i1 * (data->uvSize * 2);
    data->uv[2] = uv + i2 * (data->uvSize * 2);
}

static void TriListNBTDataSetup16(NBTCalcData* data, RwUInt8* indices, RwUInt32 stride,
                                  RwUInt8* pos, RwUInt8* nbt, RwUInt8* uv, RwInt32 index)
{
    RwUInt16 i0;
    RwUInt16 i1;
    RwUInt16 i2;

    i0 = *(RwUInt16*)&indices[stride * index];

    switch (index % 3)
    {
        case 0:
            i1 = *(RwUInt16*)&indices[stride * (index + 1)];
            i2 = *(RwUInt16*)&indices[stride * (index + 2)];
            break;
        case 1:
            i1 = *(RwUInt16*)&indices[stride * (index + 1)];
            i2 = *(RwUInt16*)&indices[stride * (index - 1)];
            break;
        case 2:
            i1 = *(RwUInt16*)&indices[stride * (index - 2)];
            i2 = *(RwUInt16*)&indices[stride * (index - 1)];
            break;
        default:
            return;
    }

    data->pos[0] = pos + i0 * (data->posSize * 3);
    data->pos[1] = pos + i1 * (data->posSize * 3);
    data->pos[2] = pos + i2 * (data->posSize * 3);
    data->nbt = nbt + i0 * (data->nbtSize * 9);
    data->uv[0] = uv + i0 * (data->uvSize * 2);
    data->uv[1] = uv + i1 * (data->uvSize * 2);
    data->uv[2] = uv + i2 * (data->uvSize * 2);
}

/* Has the binormal of a vertex not been calculated yet? */
#define NBTUNSET(_type, _unset, _nbt, _index) (((_type*)(_nbt))[(_index) * 9 + 3] == (_unset))

/* Vertex index of the i'th vertex of a primitive in the display list */
#define INDEX8(_indices, _stride, _i) ((_indices)[(_stride) * (_i)])
#define INDEX16(_indices, _stride, _i) (*(RwUInt16*)&(_indices)[(_stride) * (_i)])

#define PRIMVERTNBTS(_setup, _index, _type, _unset)                                                \
    for (i = 0; i < numVerts; i++)                                                                 \
    {                                                                                              \
        if (NBTUNSET(_type, _unset, nbt, _index(dl, idxStride, i)))                                \
        {                                                                                          \
            _setup(&data, dl, idxStride, pos, nbt, uv, i);                                         \
            CalcNBT(&data);                                                                        \
        }                                                                                          \
    }

#define PRIMNBTS(_setup, _type, _unset8, _unset16)                                                 \
    while (offset < dList->size)                                                                   \
    {                                                                                              \
        offset += 3;                                                                               \
        numVerts = *(RwUInt16*)(dl + 1);                                                           \
        dl += 3;                                                                                   \
                                                                                                   \
        if (vbHeader->attr[0].indexType == GX_INDEX16)                                             \
        {                                                                                          \
            PRIMVERTNBTS(_setup##16, INDEX16, _type, _unset16)                                     \
        }                                                                                          \
        else                                                                                       \
        {                                                                                          \
            PRIMVERTNBTS(_setup##8, INDEX8, _type, _unset8)                                        \
        }                                                                                          \
                                                                                                   \
        dl += idxStride * numVerts;                                                                \
        offset += idxStride * numVerts;                                                            \
                                                                                                   \
        if (*dl == 0)                                                                              \
        {                                                                                          \
            break;                                                                                 \
        }                                                                                          \
    }

static void CalcMeshNBTs(RxGameCubeVertexBuffer* vbHeader, RxGameCubeDisplayList* dList,
                         RpGameCubeVtxFmt* vtxFmt)
{
    RwUInt32 offset;
    RwUInt32 idxStride;
    RwUInt8* pos;
    RwUInt8* uv;
    RwUInt8* nbt;
    RwUInt8* dl;
    RwInt32 i;
    RwInt32 numVerts;
    register RwUInt32 posGQR;
    register RwUInt32 nbtGQR;
    register RwUInt32 uvGQR;
    volatile RwUInt32 savedGQR;
    register RwUInt32 gqr;
    NBTCalcData data;

    pos = (RwUInt8*)vbHeader->attr[0].array;
    nbt = (RwUInt8*)vbHeader->attr[1].array;

    /* Find the first set of texture coordinates */
    for (i = 2; vbHeader->attr[i].attr != GX_VA_TEX0; i++)
    {
    }
    uv = (RwUInt8*)vbHeader->attr[i].array;

    /* Size of one vertex in the display list */
    idxStride = 0;
    for (i = 0; i < vbHeader->numAttrArrays; i++)
    {
        if (vbHeader->attr[i].indexType == GX_INDEX16)
        {
            idxStride += 2;
        }
        else
        {
            idxStride += 1;
        }
    }

    if (vtxFmt != NULL)
    {
        RwUInt8 vtxFmtNormConvTable[5] = { 0, 6, 0, 14, 0 };
        RwUInt8 vtxFmtSizeConvTable[5] = { 1, 1, 2, 2, 4 };
        RwUInt8 vtxFmtTypeConvTable[5] = { 4, 6, 5, 7, 0 };

        data.posSize = vtxFmtSizeConvTable[vtxFmt->pos];
        data.nbtSize = vtxFmtSizeConvTable[vtxFmt->norm];
        data.uvSize = vtxFmtSizeConvTable[vtxFmt->texCoord[0]];

        posGQR = vtxFmtTypeConvTable[vtxFmt->pos] | (vtxFmt->posFrac << 8);
        posGQR |= posGQR << 16;

        nbtGQR = vtxFmtTypeConvTable[vtxFmt->norm] | (vtxFmtNormConvTable[vtxFmt->norm] << 8);
        nbtGQR |= nbtGQR << 16;

        uvGQR = vtxFmtTypeConvTable[vtxFmt->texCoord[0]] | (vtxFmt->texCoordFrac[0] << 8);
        uvGQR |= uvGQR << 16;
    }
    else
    {
        data.posSize = sizeof(RwReal);
        data.nbtSize = sizeof(RwReal);

        posGQR = 0;
        nbtGQR = 0;
        uvGQR = 0;
    }

    /* GQR5 is restored when done */
    asm { mfspr gqr, GQR5 }
    savedGQR = gqr;
    asm { mtspr GQR5, posGQR }
    asm { mtspr GQR6, nbtGQR }
    asm { mtspr GQR7, uvGQR }

    dl = (RwUInt8*)dList->displayList;
    offset = 0;

    if (*dl == (GX_TRIANGLESTRIP | GX_VTXFMT0))
    {
        if (vtxFmt != NULL)
        {
            if (vtxFmt->norm == GX_S8)
            {
                PRIMNBTS(TriStripNBTDataSetup, RwUInt8, 0xff, 0xff)
            }
            else if (vtxFmt->norm == GX_S16)
            {
                /* NB: retail compares the 16 bit binormals against RwRealMAXVAL for
                 * 8 bit indices, so they are never found to be unset */
                PRIMNBTS(TriStripNBTDataSetup, RwUInt16, RwRealMAXVAL, 0xffff)
            }
            else
            {
                PRIMNBTS(TriStripNBTDataSetup, RwReal, RwRealMAXVAL, RwRealMAXVAL)
            }
        }
        else
        {
            PRIMNBTS(TriStripNBTDataSetup, RwReal, RwRealMAXVAL, RwRealMAXVAL)
        }
    }
    else
    {
        if (vtxFmt != NULL)
        {
            if (vtxFmt->norm == GX_S8)
            {
                PRIMNBTS(TriListNBTDataSetup, RwUInt8, 0xff, 0xff)
            }
            else if (vtxFmt->norm == GX_S16)
            {
                /* NB: as above */
                PRIMNBTS(TriListNBTDataSetup, RwUInt16, RwRealMAXVAL, 0xffff)
            }
            else
            {
                PRIMNBTS(TriListNBTDataSetup, RwReal, RwRealMAXVAL, RwRealMAXVAL)
            }
        }
        else
        {
            PRIMNBTS(TriListNBTDataSetup, RwReal, RwRealMAXVAL, RwRealMAXVAL)
        }
    }

    gqr = savedGQR;
    asm { mtspr GQR5, gqr }
}

void _rpGameCubeMTPipeDataCalcNBTs(RxGameCubePipeData* pipeData, RpGameCubeVtxFmt* vtxFmt,
                                   RwInt32 numVerts)
{
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* dList;
    RpMeshHeader* meshHeader;
    RpMesh* mesh;
    RwInt32 i;
    RpMultiTexture* multiTexture;
    RpGameCubeMTConfig* config;
    RwInt32 j;
    RpGameCubeTexGen* texGen;

    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);
    meshHeader = pipeData->meshHeader;
    dList = (RxGameCubeDisplayList*)((RxGameCubeVertexAttr*)(vbHeader + 1) +
                                     (vbHeader->numAttrArrays - 1));

    /* Mark every binormal as not yet calculated */
    if (vtxFmt != NULL)
    {
        if (vtxFmt->norm == GX_S8)
        {
            RwUInt8* binormal;

            binormal = (RwUInt8*)vbHeader->attr[1].array;
            binormal += 3;

            for (i = 0; i < numVerts; i++)
            {
                *binormal = 0xff;
                binormal += 9;
            }
        }
        else if (vtxFmt->norm == GX_S16)
        {
            RwUInt16* binormal;

            binormal = (RwUInt16*)vbHeader->attr[1].array;
            binormal += 3;

            for (i = 0; i < numVerts; i++)
            {
                *binormal = 0xffff;
                binormal += 9;
            }
        }
        else
        {
            RwReal* binormal;

            binormal = (RwReal*)vbHeader->attr[1].array;
            binormal += 3;

            for (i = 0; i < numVerts; i++)
            {
                *binormal = RwRealMAXVAL;
                binormal += 9;
            }
        }
    }
    else
    {
        RwReal* binormal;

        binormal = (RwReal*)vbHeader->attr[1].array;
        binormal += 3;

        for (i = 0; i < numVerts; i++)
        {
            *binormal = RwRealMAXVAL;
            binormal += 9;
        }
    }

    mesh = (RpMesh*)(meshHeader + 1);

    for (i = 0; i < meshHeader->numMeshes; dList++, mesh++, i++)
    {
        multiTexture = RpMaterialGetMultiTexture(mesh->material, rwID_GAMECUBE);
        if (multiTexture != NULL && multiTexture->effect != NULL &&
            multiTexture->effect->platformID == rwID_GAMECUBE)
        {
            config = RpGameCubeMTEffectGetConfig(multiTexture->effect);

            for (j = 0; j < config->numTexGens; j++)
            {
                texGen = &config->texGens[j];

                if (texGen->srcParam == GX_TG_BINRM || texGen->srcParam == GX_TG_TANGENT ||
                    (texGen->func >= GX_TG_BUMP0 && texGen->func <= GX_TG_BUMP7))
                {
                    CalcMeshNBTs(vbHeader, dList, vtxFmt);
                    break;
                }
            }
        }
    }

    DCFlushRange(vbHeader->attr[1].array, numVerts * vbHeader->attr[1].stride);
    GXInvalidateVtxCache();
}

RwBool _rpGameCubeMTPipeDataQueryNBTs(RxGameCubePipeData* pipeData)
{
    RpMeshHeader* meshHeader;
    RpMesh* mesh;
    RwInt32 i;
    RpMultiTexture* multiTexture;
    RpGameCubeMTConfig* config;
    RwInt32 j;
    RpGameCubeTexGen* texGen;

    meshHeader = pipeData->meshHeader;
    mesh = (RpMesh*)(meshHeader + 1);

    for (i = 0; i < meshHeader->numMeshes; i++, mesh++)
    {
        multiTexture = RpMaterialGetMultiTexture(mesh->material, rwID_GAMECUBE);
        if (multiTexture != NULL && multiTexture->effect != NULL &&
            multiTexture->effect->platformID == rwID_GAMECUBE)
        {
            config = RpGameCubeMTEffectGetConfig(multiTexture->effect);

            for (j = 0; j < config->numTexGens; j++)
            {
                texGen = &config->texGens[j];

                if (texGen->srcParam == GX_TG_BINRM || texGen->srcParam == GX_TG_TANGENT ||
                    (texGen->func >= GX_TG_BUMP0 && texGen->func <= GX_TG_BUMP7))
                {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static RwMatrix* GetTexFrameMatrix(RpGameCubeTexFrameID refFrame, RwMatrix* objectLTM,
                                   RwMatrix* scratch)
{
    RwMatrix* matrix;
    RwUInt32 index;
    RwUInt32 numFrames;
    RwFrame** frames;
    RwMatrix invLTM;

    if (refFrame >= rpGAMECUBETEXFRAME_MISC0)
    {
        index = refFrame - rpGAMECUBETEXFRAME_MISC0;
        numFrames = RPGAMECUBEMTGLOBAL(numFrames);
        frames = RPGAMECUBEMTGLOBAL(frames);

        if (index < numFrames && frames != NULL && frames[index] != NULL)
        {
            if (objectLTM != NULL)
            {
                RwMatrixInvert(&invLTM, RwFrameGetLTM(frames[index]));
                RwMatrixMultiply(scratch, objectLTM, &invLTM);
            }
            else
            {
                RwMatrixInvert(scratch, RwFrameGetLTM(frames[index]));
            }

            return scratch;
        }

        if (objectLTM != NULL)
        {
            return objectLTM;
        }

        matrix = scratch;
        RwMatrixSetIdentity(matrix);

        return matrix;
    }
    else if (refFrame == rpGAMECUBETEXFRAME_CAMERA)
    {
        if (objectLTM != NULL)
        {
            RwMatrixMultiply(scratch, objectLTM, &_RwDlInvCamLTM);
            matrix = scratch;
        }
        else
        {
            matrix = &_RwDlInvCamLTM;
        }

        return matrix;
    }
    else if (refFrame == rpGAMECUBETEXFRAME_WORLD)
    {
        if (objectLTM != NULL)
        {
            return objectLTM;
        }

        return NULL;
    }

    return NULL;
}

RpMTEffect* _rpGameCubeMTEffectSend(RpMTEffect* effect, RwUInt32 numTextures, RwUInt8* coordMap,
                                    RwMatrix* objectLTM)
{
    RwInt32 i;
    _TevRasSrc rasSrc;
    RwInt8 texGenTexture[8];
    RpGameCubeMTConfig* config;

    config = RpGameCubeMTEffectGetConfig(effect);

    if (config->flags & rpGAMECUBEMTCONFIGREGS)
    {
        GXSetTevColorS10(GX_TEVREG0, *(GXColorS10*)&config->reg[0]);
        GXSetTevColorS10(GX_TEVREG1, *(GXColorS10*)&config->reg[1]);
        GXSetTevColorS10(GX_TEVREG2, *(GXColorS10*)&config->reg[2]);
        GXSetTevColorS10(GX_TEVPREV, *(GXColorS10*)&config->reg[3]);
    }

    if (config->flags & rpGAMECUBEMTCONFIGKREGS)
    {
        GXSetTevKColor(GX_KCOLOR0, *(GXColor*)&config->kreg[0]);
        GXSetTevKColor(GX_KCOLOR1, *(GXColor*)&config->kreg[1]);
        GXSetTevKColor(GX_KCOLOR2, *(GXColor*)&config->kreg[2]);
        GXSetTevKColor(GX_KCOLOR3, *(GXColor*)&config->kreg[3]);
    }

    if (config->flags & rpGAMECUBEMTCONFIGENVMTX)
    {
        RwMatrix tmpMatrix;
        RwMatrix* matrix;
        RwReal texMtx[3][4];

        if (objectLTM != NULL)
        {
            matrix = RwMatrixMultiply(&tmpMatrix, objectLTM, &_RwDlInvCamLTM);
        }
        else
        {
            matrix = &_RwDlInvCamLTM;
        }

        texMtx[0][0] = -0.5f * matrix->right.x;
        texMtx[0][1] = -0.5f * matrix->up.x;
        texMtx[0][2] = -0.5f * matrix->at.x;
        texMtx[0][3] = 0.5f;
        texMtx[1][0] = -0.5f * matrix->right.y;
        texMtx[1][1] = -0.5f * matrix->up.y;
        texMtx[1][2] = -0.5f * matrix->at.y;
        texMtx[1][3] = 0.5f;

        GXLoadTexMtxImm(texMtx, GX_TEXMTX0, GX_MTX2x4);
    }

    for (i = 0; i < config->numTexMtx; i++)
    {
        RpGameCubeTexMtx* texMtx;
        RwMatrix temp;
        RwMatrix* matrix;
        RwReal mtx[3][4];
        RwReal mtx2[3][4];

        texMtx = &config->texMtx[i];

        matrix = GetTexFrameMatrix(texMtx->refFrame, objectLTM, &temp);
        if (matrix != NULL)
        {
            mtx[0][0] = matrix->right.x;
            mtx[0][1] = matrix->up.x;
            mtx[0][2] = matrix->at.x;
            mtx[1][0] = matrix->right.y;
            mtx[1][1] = matrix->up.y;
            mtx[1][2] = matrix->at.y;
            mtx[2][0] = matrix->right.z;
            mtx[2][1] = matrix->up.z;
            mtx[2][2] = matrix->at.z;

            if (texMtx->flags & rpGAMECUBEMTTEXMTXTRANSLATE)
            {
                mtx[0][3] = matrix->pos.x;
                mtx[1][3] = matrix->pos.y;
                mtx[2][3] = matrix->pos.z;
            }
            else
            {
                mtx[2][3] = mtx[1][3] = mtx[0][3] = 0.0f;
            }

            PSMTXConcat(texMtx->data, mtx, mtx2);
            GXLoadTexMtxImm(mtx2, texMtx->offset,
                            (GXTexMtxType)(texMtx->flags & rpGAMECUBEMTTEXMTX2x4));
        }
        else
        {
            GXLoadTexMtxImm(texMtx->data, texMtx->offset,
                            (GXTexMtxType)(texMtx->flags & rpGAMECUBEMTTEXMTX2x4));
        }
    }

    for (i = 0; i < config->numIndMtx; i++)
    {
        RpGameCubeIndMtx* indMtx;
        RwMatrix scratch;
        RwMatrix* matrix;
        RwReal(*pMtx)[3];
        RwReal mtx[2][3];

        indMtx = &config->indMtx[i];

        matrix = GetTexFrameMatrix(indMtx->refFrame, objectLTM, &scratch);
        if (matrix != NULL)
        {
            mtx[0][0] = indMtx->data[0][0] * matrix->right.x +
                        indMtx->data[0][1] * matrix->right.y + indMtx->data[0][2] * matrix->right.z;
            mtx[0][1] = indMtx->data[0][0] * matrix->up.x + indMtx->data[0][1] * matrix->up.y +
                        indMtx->data[0][2] * matrix->up.z;
            mtx[0][2] = indMtx->data[0][0] * matrix->at.x + indMtx->data[0][1] * matrix->at.y +
                        indMtx->data[0][2] * matrix->at.z;
            mtx[1][0] = indMtx->data[1][0] * matrix->right.x +
                        indMtx->data[1][1] * matrix->right.y + indMtx->data[1][2] * matrix->right.z;
            mtx[1][1] = indMtx->data[1][0] * matrix->up.x + indMtx->data[1][1] * matrix->up.y +
                        indMtx->data[1][2] * matrix->up.z;
            mtx[1][2] = indMtx->data[1][0] * matrix->at.x + indMtx->data[1][1] * matrix->at.y +
                        indMtx->data[1][2] * matrix->at.z;

            pMtx = mtx;
        }
        else
        {
            pMtx = indMtx->data;
        }

        GXSetIndTexMtx((GXIndTexMtxID)indMtx->id, pMtx, (s8)indMtx->scale);
    }

    rasSrc = TEVRAS_NORM;
    if (config->flags & rpGAMECUBEMTCONFIGCHANNELCOLOR)
    {
        rasSrc = TEVRAS_CH0A0;
    }

    for (i = 0; i < config->numTexGens; i++)
    {
        texGenTexture[i] = -1;
    }

    if (config->numIndStages != 0)
    {
        GXSetNumIndStages(config->numIndStages);

        for (i = 0; i < config->numIndStages; i++)
        {
            RpGameCubeIndStage* stage;

            stage = &config->indStages[i];

            if (!(stage->flags & rpGAMECUBEMTDISABLE))
            {
                GXSetIndTexOrder((GXIndTexStageID)i, (GXTexCoordID)stage->texCoordID,
                                 (GXTexMapID)stage->texMapID);
                GXSetIndTexCoordScale((GXIndTexStageID)i, (GXIndTexScale)stage->scaleS,
                                      (GXIndTexScale)stage->scaleT);

                texGenTexture[stage->texCoordID] = (RwInt8)stage->texMapID;
            }
        }
    }

    if (config->numTevStages != 0)
    {
        GXSetNumTevStages(config->numTevStages);

        for (i = 0; i < config->numTevStages; i++)
        {
            RpGameCubeTevStage* stage;
            RwUInt32 channelID;

            stage = &config->tevStages[i];

            if (!(stage->flags & rpGAMECUBEMTDISABLE))
            {
                if (stage->indirect != 0)
                {
                    RpGameCubeTevInd ind;

                    RpGameCubeTevIndUnpack(&ind, &stage->indirect);
                    GXSetTevIndirect((GXTevStageID)i, (GXIndTexStageID)ind.indStage,
                                     (GXIndTexFormat)ind.format, (GXIndTexBiasSel)ind.biasSel,
                                     (GXIndTexMtxID)ind.matrixSel, (GXIndTexWrap)ind.wrapS,
                                     (GXIndTexWrap)ind.wrapT, ind.addPrev, ind.utcLod,
                                     (GXIndTexAlphaSel)ind.alphaSel);
                }
                else
                {
                    GXSetTevDirect((GXTevStageID)i);
                }

                GXSetTevColorIn((GXTevStageID)i, (GXTevColorArg)stage->op.colorA,
                                (GXTevColorArg)stage->op.colorB, (GXTevColorArg)stage->op.colorC,
                                (GXTevColorArg)stage->op.colorD);
                GXSetTevAlphaIn((GXTevStageID)i, (GXTevAlphaArg)stage->op.alphaA,
                                (GXTevAlphaArg)stage->op.alphaB, (GXTevAlphaArg)stage->op.alphaC,
                                (GXTevAlphaArg)stage->op.alphaD);

                channelID = stage->channelID;
                if ((RwInt32)channelID <= GX_COLOR1A1 && rasSrc == TEVRAS_CH0A0)
                {
                    channelID = GX_COLOR0A0;
                }

                GXSetTevOrder((GXTevStageID)i, (GXTexCoordID)stage->texCoordID,
                              (GXTexMapID)stage->texMapID, (GXChannelID)channelID);
                GXSetTevKColorSel((GXTevStageID)i, (GXTevKColorSel)stage->op.colorSel);
                GXSetTevKAlphaSel((GXTevStageID)i, (GXTevKAlphaSel)stage->op.alphaSel);
                GXSetTevColorOp((GXTevStageID)i, (GXTevOp)stage->op.colorOp,
                                (GXTevBias)stage->op.colorBias, (GXTevScale)stage->op.colorScale,
                                stage->op.colorClamp, (GXTevRegID)stage->op.colorOutReg);
                GXSetTevAlphaOp((GXTevStageID)i, (GXTevOp)stage->op.alphaOp,
                                (GXTevBias)stage->op.alphaBias, (GXTevScale)stage->op.alphaScale,
                                stage->op.alphaClamp, (GXTevRegID)stage->op.alphaOutReg);

                texGenTexture[stage->texCoordID] = (RwInt8)stage->texMapID;
            }
        }
    }

    GXSetNumTexGens(config->numTexGens);

    for (i = 0; i < config->numTexGens; i++)
    {
        RpGameCubeTexGen* texGen;
        GXTexGenSrc src;

        texGen = &config->texGens[i];

        if (!(texGen->flags & rpGAMECUBEMTDISABLE))
        {
            src = (GXTexGenSrc)texGen->srcParam;

            if (coordMap != NULL && src >= GX_TG_TEX0 && src <= GX_TG_TEX7 && texGenTexture[i] >= 0)
            {
                src = (GXTexGenSrc)(coordMap[texGenTexture[i]] + GX_TG_TEX0);
            }

            GXSetTexCoordGen2((GXTexCoordID)i, (GXTexGenType)texGen->func, src, texGen->mtx,
                              (texGen->flags >> 1) & 1, texGen->postMtx);

            if (texGen->flags & rpGAMECUBEMTTEXGENSCALEMANUALLY)
            {
                GXSetTexCoordScaleManually((GXTexCoordID)i, GX_TRUE, texGen->scaleS,
                                           texGen->scaleT);
            }
        }
    }

    if (config->flags & rpGAMECUBEMTCONFIGZCOMPAFTERTEX)
    {
        _rwDlRenderStateSetZCompLoc(FALSE);
    }
    else
    {
        _rwDlRenderStateSetZCompLoc(TRUE);
    }

    if (config->flags & rpGAMECUBEMTCONFIGBLEND)
    {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)config->srcBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)config->destBlend);
    }

    return effect;
}

static void _rpGameCubeMTEffectClean(RpMTEffect* effect)
{
    RpGameCubeMTConfig* config;
    RwInt32 i;

    config = RpGameCubeMTEffectGetConfig(effect);

    for (i = 0; i < config->numTexGens; i++)
    {
        if (config->texGens[i].flags & rpGAMECUBEMTTEXGENSCALEMANUALLY)
        {
            GXSetTexCoordScaleManually((GXTexCoordID)i, GX_FALSE, 0, 0);
        }
    }

    if (config->numIndStages != 0)
    {
        GXSetNumIndStages(0);

        for (i = 0; i < config->numIndStages; i++)
        {
            GXSetIndTexCoordScale((GXIndTexStageID)i, GX_ITS_1, GX_ITS_1);
        }

        for (i = 0; i < config->numTevStages; i++)
        {
            if (config->tevStages[i].indirect != 0)
            {
                GXSetTevDirect((GXTevStageID)i);
            }
        }
    }
}

void _rpGameCubeMTMeshRenderCallBack(RxGameCubeDisplayList* dList, RpMaterial* material,
                                     void* object, RxGameCubePipeData* pipeData,
                                     RwMatrix* objectLTM)
{
    RpMultiTexture* mt;
    rpMultiTextureGameCubeExt* mtExt;
    RwBlendFunction srcBlend;
    RwBlendFunction dstBlend;
    RwUInt32 i;

    mt = RpMaterialGetMultiTexture(material, rwID_GAMECUBE);
    mtExt = (rpMultiTextureGameCubeExt*)mt->platformData;

    RwRenderStateGet(rwRENDERSTATESRCBLEND, (void*)&srcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void*)&dstBlend);

    for (i = 0; i < mt->numTextures; i++)
    {
        _rwDlTextureSet(mt->textures[i], i);
    }

    if (mtExt->preRenderCallBack != NULL)
    {
        mtExt->preRenderCallBack(material, object, pipeData, mtExt->callBackData);
    }

    if (mt->effect != NULL)
    {
        _rpGameCubeMTEffectSend(mt->effect, mt->numTextures, mt->coords, objectLTM);
        GXCallDisplayList(dList->displayList, dList->size);
        _rpGameCubeMTEffectClean(mt->effect);
    }
    else
    {
        GXCallDisplayList(dList->displayList, dList->size);
    }

    if (mtExt->postRenderCallBack != NULL)
    {
        mtExt->postRenderCallBack(material, object, pipeData, mtExt->callBackData);
    }

    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dstBlend);
}

RwBool _rpGameCubeMTPipePluginAttach(void)
{
    _rpGameCubeMTEngineOffset = RwEngineRegisterPlugin(
        sizeof(rpGameCubeMTGlobals), rwID_GCNMULTITEXPLUGIN, GameCubeMTOpen, GameCubeMTClose);

    return _rpGameCubeMTEngineOffset >= 0;
}
