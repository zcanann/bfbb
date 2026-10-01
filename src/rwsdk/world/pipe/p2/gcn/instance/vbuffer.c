#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

RwUInt32 _rwGCNVertexBufferHeaderGetSize(rwVertexDescriptor* vtxDesc)
{
    RwUInt32 size;

    size = sizeof(RxGameCubeVertexBuffer) +
           (vtxDesc->numAttrArrays - 1) * sizeof(RxGameCubeVertexAttr);

    return size;
}

RwUInt32 _rwGCNVertexBufferGetSize(rwVertexDescriptor* vtxDesc, rwGCNVertexBufferData* vtxBufData)
{
    RwUInt32 i;
    RwUInt32 type;
    RwUInt32 size;

    size = 0;

    for (i = rwGCNVA_POS; i < rwGCNVA_TEX7 + 1; i++)
    {
        switch (i)
        {
        case rwGCNVA_POS:
        {
            type = (vtxDesc->VCDRegLO >> 9) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNPosGetSize(vtxDesc) + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_NRM:
        {
            type = (vtxDesc->VCDRegLO >> 11) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                if (((vtxDesc->VATRegA >> 9) & 0x1) == rwGCNCC_NRM_NBT)
                {
                    size += (vtxBufData->num[rwGCNVA_NRM] * rwGCNNrmGetSize(vtxDesc) * 3 + 31) &
                            ~31;
                }
                else
                {
                    size += (vtxBufData->num[rwGCNVA_NRM] * rwGCNNrmGetSize(vtxDesc) + 31) & ~31;
                }
            }
            break;
        }
        case rwGCNVA_CLR0:
        case rwGCNVA_CLR1:
        {
            type = (vtxDesc->VCDRegLO >> ((i - rwGCNVA_CLR0) * 2 + 13)) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNClrGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_CLR0)) +
                         31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX0:
        {
            type = (vtxDesc->VCDRegHI >> 0) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0)) +
                         31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX1:
        {
            type = (vtxDesc->VCDRegHI >> 2) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0)) +
                         31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX2:
        {
            type = (vtxDesc->VCDRegHI >> 4) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0)) +
                         31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX3:
        {
            type = (vtxDesc->VCDRegHI >> 6) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0)) +
                         31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX4:
        {
            type = (vtxDesc->VCDRegHI >> 8) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0)) +
                         31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX5:
        {
            type = (vtxDesc->VCDRegHI >> 10) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0)) +
                         31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX6:
        {
            type = (vtxDesc->VCDRegHI >> 12) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0)) +
                         31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX7:
        {
            type = (vtxDesc->VCDRegHI >> 14) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size += (vtxBufData->num[i] * rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0)) +
                         31) & ~31;
            }
            break;
        }
        }
    }

    return size;
}

void _rwGCNVertexBufferInitialize(rwVertexDescriptor* vtxDesc, RxGameCubeVertexBuffer* vtxBufHeader,
                                  rwGCNVertexBufferData* vtxBufData, RwUInt8* memory)
{
    RwUInt32 count;
    RwUInt32 offset;
    RwUInt32 i;
    RwUInt32 type;
    RwUInt32 size;
    RwUInt8* mem;

    count = 0;
    offset = 0;

    for (i = rwGCNVA_POS; i < rwGCNVA_TEX7 + 1; i++)
    {
        mem = memory + offset;

        switch (i)
        {
        case rwGCNVA_POS:
        {
            type = (vtxDesc->VCDRegLO >> 9) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNPosGetSize(vtxDesc);

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_NRM:
        {
            type = (vtxDesc->VCDRegLO >> 11) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                if (((vtxDesc->VATRegA >> 9) & 0x1) == rwGCNCC_NRM_NBT)
                {
                    size = rwGCNNrmGetSize(vtxDesc) * 3;

                    vtxBufHeader->attr[count].array = mem;
                    vtxBufHeader->attr[count].attr = i;
                    vtxBufHeader->attr[count].stride = size;
                    vtxBufHeader->attr[count].indexType = type;

                    offset += (size * vtxBufData->num[rwGCNVA_NRM] + 31) & ~31;
                }
                else
                {
                    size = rwGCNNrmGetSize(vtxDesc);

                    vtxBufHeader->attr[count].array = mem;
                    vtxBufHeader->attr[count].attr = i;
                    vtxBufHeader->attr[count].stride = size;
                    vtxBufHeader->attr[count].indexType = type;

                    offset += (size * vtxBufData->num[rwGCNVA_NRM] + 31) & ~31;
                }
                count++;
            }
            break;
        }
        case rwGCNVA_CLR0:
        case rwGCNVA_CLR1:
        {
            type = (vtxDesc->VCDRegLO >> ((i - rwGCNVA_CLR0) * 2 + 13)) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNClrGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_CLR0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX0:
        {
            type = (vtxDesc->VCDRegHI >> 0) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX1:
        {
            type = (vtxDesc->VCDRegHI >> 2) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX2:
        {
            type = (vtxDesc->VCDRegHI >> 4) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX3:
        {
            type = (vtxDesc->VCDRegHI >> 6) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX4:
        {
            type = (vtxDesc->VCDRegHI >> 8) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX5:
        {
            type = (vtxDesc->VCDRegHI >> 10) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX6:
        {
            type = (vtxDesc->VCDRegHI >> 12) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        case rwGCNVA_TEX7:
        {
            type = (vtxDesc->VCDRegHI >> 14) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                size = rwGCNTexGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_TEX0));

                vtxBufHeader->attr[count].array = mem;
                vtxBufHeader->attr[count].attr = i;
                vtxBufHeader->attr[count].stride = size;
                vtxBufHeader->attr[count].indexType = type;
                count++;

                offset += (size * vtxBufData->num[i] + 31) & ~31;
            }
            break;
        }
        }
    }

    vtxBufHeader->numAttrArrays = count;
}
