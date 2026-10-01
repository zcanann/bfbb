#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

void _rwGCNVertexBufferFill(rwVertexDescriptor* vtxDesc, RxGameCubeVertexBuffer* vtxBufHeader,
                            rwGCNVertexBufferData* vtxBufData, RwBool cmpNrm)
{
    RwUInt32 count;
    RwUInt32 i;
    RwUInt32 fmt;
    RwReal scale;
    RwUInt32 type;

    count = 0;

    for (i = rwGCNVA_POS; i < rwGCNVA_TEX7 + 1; i++)
    {
        switch (i)
        {
        case rwGCNVA_POS:
        {
            type = (vtxDesc->VCDRegLO >> 9) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                if ((vtxDesc->VATRegA & 0x1) == rwGCNCC_POS_XYZ)
                {
                    scale = (RwReal)(1 << ((vtxDesc->VATRegA >> 4) & 0x1F));
                    fmt = (vtxDesc->VATRegA >> 1) & 0x7;

                    _rwGCNVtxFmtInstPos3D((RwUInt8*)vtxBufHeader->attr[count].array,
                                          (RwV3d*)vtxBufData->data[i], fmt, scale,
                                          vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                    count++;
                }
            }
            break;
        }
        case rwGCNVA_NRM:
        {
            type = (vtxDesc->VCDRegLO >> 11) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegA >> 10) & 0x7;

                if (((vtxDesc->VATRegA >> 9) & 0x1) == rwGCNCC_NRM_NBT)
                {
                    if (!cmpNrm)
                    {
                        _rwGCNVtxFmtInstNBT((RwUInt8*)vtxBufHeader->attr[count].array,
                                            (RwV3d*)vtxBufData->data[i], fmt, vtxBufData->num[i],
                                            vtxBufHeader->attr[count].stride);
                    }
                    else
                    {
                        _rwGCNVtxFmtInstNBTCmp((RwUInt8*)vtxBufHeader->attr[count].array,
                                               (RpVertexNormal*)vtxBufData->data[i], fmt,
                                               vtxBufData->num[i],
                                               vtxBufHeader->attr[count].stride);
                    }
                }
                else
                {
                    if (!cmpNrm)
                    {
                        _rwGCNVtxFmtInstNrm((RwUInt8*)vtxBufHeader->attr[count].array,
                                            (RwV3d*)vtxBufData->data[i], fmt, vtxBufData->num[i],
                                            vtxBufHeader->attr[count].stride);
                    }
                    else
                    {
                        _rwGCNVtxFmtInstNrmCmp((RwUInt8*)vtxBufHeader->attr[count].array,
                                               (RpVertexNormal*)vtxBufData->data[i], fmt,
                                               vtxBufData->num[i],
                                               vtxBufHeader->attr[count].stride);
                    }
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
                fmt = (vtxDesc->VATRegA >> ((i - rwGCNVA_CLR0) * 4 + 14)) & 0x7;

                _rwGCNVtxFmtInstClr((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwRGBA*)vtxBufData->data[i], fmt, vtxBufData->num[i],
                                    vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        case rwGCNVA_TEX0:
        {
            type = (vtxDesc->VCDRegHI >> 0) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegA >> 22) & 0x7;
                scale = (RwReal)(1 << ((vtxDesc->VATRegA >> 25) & 0x1F));

                _rwGCNVtxFmtInstTex((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwTexCoords*)vtxBufData->data[i], fmt, scale,
                                    vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        case rwGCNVA_TEX1:
        {
            type = (vtxDesc->VCDRegHI >> 2) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegB >> 1) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegB >> 4) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwTexCoords*)vtxBufData->data[i], fmt, scale,
                                    vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        case rwGCNVA_TEX2:
        {
            type = (vtxDesc->VCDRegHI >> 4) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegB >> 10) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegB >> 13) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwTexCoords*)vtxBufData->data[i], fmt, scale,
                                    vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        case rwGCNVA_TEX3:
        {
            type = (vtxDesc->VCDRegHI >> 6) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegB >> 19) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegB >> 22) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwTexCoords*)vtxBufData->data[i], fmt, scale,
                                    vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        case rwGCNVA_TEX4:
        {
            type = (vtxDesc->VCDRegHI >> 8) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegB >> 28) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegC & 0x1F));

                _rwGCNVtxFmtInstTex((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwTexCoords*)vtxBufData->data[i], fmt, scale,
                                    vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        case rwGCNVA_TEX5:
        {
            type = (vtxDesc->VCDRegHI >> 10) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegC >> 6) & 0x7;
                scale = (RwReal)(1 << ((vtxDesc->VATRegC >> 9) & 0x1F));

                _rwGCNVtxFmtInstTex((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwTexCoords*)vtxBufData->data[i], fmt, scale,
                                    vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        case rwGCNVA_TEX6:
        {
            type = (vtxDesc->VCDRegHI >> 12) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegC >> 15) & 0x7;
                scale = (RwReal)(1 << ((vtxDesc->VATRegC >> 18) & 0x1F));

                _rwGCNVtxFmtInstTex((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwTexCoords*)vtxBufData->data[i], fmt, scale,
                                    vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        case rwGCNVA_TEX7:
        {
            type = (vtxDesc->VCDRegHI >> 14) & 0x3;
            if (type == rwGCNAT_INDEX8 || type == rwGCNAT_INDEX16)
            {
                fmt = (vtxDesc->VATRegC >> 24) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegC >> 27));

                _rwGCNVtxFmtInstTex((RwUInt8*)vtxBufHeader->attr[count].array,
                                    (RwTexCoords*)vtxBufData->data[i], fmt, scale,
                                    vtxBufData->num[i], vtxBufHeader->attr[count].stride);
                count++;
            }
            break;
        }
        }
    }
}
