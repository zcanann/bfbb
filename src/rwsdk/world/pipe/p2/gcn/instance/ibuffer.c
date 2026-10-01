#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <string.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

RwUInt32 _rwGCNDisplayListGetStride(rwVertexDescriptor* vtxDesc)
{
    RwInt32 i;
    RwUInt32 type;
    RwUInt32 stride;
    RwUInt32 cnt;

    stride = 0;

    for (i = 0; i < rwGCNVA_TEX7 + 1; i++)
    {
        switch (i)
        {
        case rwGCNVA_PNMTXIDX:
        case rwGCNVA_TEX0MTXIDX:
        case rwGCNVA_TEX1MTXIDX:
        case rwGCNVA_TEX2MTXIDX:
        case rwGCNVA_TEX3MTXIDX:
        case rwGCNVA_TEX4MTXIDX:
        case rwGCNVA_TEX5MTXIDX:
        case rwGCNVA_TEX6MTXIDX:
        case rwGCNVA_TEX7MTXIDX:
        {
            if ((1 << i) & vtxDesc->VCDRegLO)
            {
                stride++;
            }
            break;
        }
        case rwGCNVA_POS:
        {
            type = (vtxDesc->VCDRegLO >> 9) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNPosGetSize(vtxDesc);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_NRM:
        {
            type = (vtxDesc->VCDRegLO >> 11) & 0x3;
            cnt = (vtxDesc->VATRegA >> 9) & 0x1;
            if (cnt == rwGCNCC_NRM_NBT)
            {
                cnt = !(vtxDesc->VATRegA >> 31) ? 1 : 3;

                if (type == rwGCNAT_DIRECT)
                {
                    stride += cnt * rwGCNNrmGetSize(vtxDesc);
                }
                else if (type == rwGCNAT_INDEX8)
                {
                    stride += cnt;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    stride += cnt * 2;
                }
            }
            else
            {
                if (type == rwGCNAT_DIRECT)
                {
                    stride += rwGCNNrmGetSize(vtxDesc);
                }
                else if (type == rwGCNAT_INDEX8)
                {
                    stride += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    stride += 2;
                }
            }
            break;
        }
        case rwGCNVA_CLR0:
        case rwGCNVA_CLR1:
        {
            type = (vtxDesc->VCDRegLO >> (13 + (i - rwGCNVA_CLR0) * 2)) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNClrGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_CLR0));
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_TEX0:
        {
            type = (vtxDesc->VCDRegHI >> 0) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNTexGetSize(vtxDesc, 0);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_TEX1:
        {
            type = (vtxDesc->VCDRegHI >> 2) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNTexGetSize(vtxDesc, 1);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_TEX2:
        {
            type = (vtxDesc->VCDRegHI >> 4) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNTexGetSize(vtxDesc, 2);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_TEX3:
        {
            type = (vtxDesc->VCDRegHI >> 6) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNTexGetSize(vtxDesc, 3);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_TEX4:
        {
            type = (vtxDesc->VCDRegHI >> 8) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNTexGetSize(vtxDesc, 4);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_TEX5:
        {
            type = (vtxDesc->VCDRegHI >> 10) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNTexGetSize(vtxDesc, 5);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_TEX6:
        {
            type = (vtxDesc->VCDRegHI >> 12) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNTexGetSize(vtxDesc, 6);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        case rwGCNVA_TEX7:
        {
            type = (vtxDesc->VCDRegHI >> 14) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                stride += rwGCNTexGetSize(vtxDesc, 7);
            }
            else if (type == rwGCNAT_INDEX8)
            {
                stride += 1;
            }
            else if (type == rwGCNAT_INDEX16)
            {
                stride += 2;
            }
            break;
        }
        }
    }

    return stride;
}

RwUInt32 _rwGCNDisplayListGetSize(rwVertexDescriptor* vtxDesc, RwUInt32 numStrips,
                                  RwUInt32 numIndices)
{
    RwUInt32 size;
    RwUInt32 stride;

    stride = _rwGCNDisplayListGetStride(vtxDesc);

    /* One primitive header per strip (GXBegin = 3 bytes) plus the indices */
    size = numStrips * 3 + numIndices * stride;
    size = (size + 31) & ~31;

    return size;
}

void _rwGCNDisplayListInitialize(RxGameCubeDisplayList* displayList, RwUInt32 index,
                                 RwUInt32 displayListSize, void* memory)
{
    displayList->displayList = memory;
    displayList->size = displayListSize;

    memset(memory, 0, displayListSize);
}
