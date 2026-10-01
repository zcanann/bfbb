#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

rwVertexDescriptor* _rwVertexDescriptorInit(rwVertexDescriptor* vtxDesc)
{
    vtxDesc->VATID = 0;
    vtxDesc->VATRegA = 0x40000000; /* ByteDequant */
    vtxDesc->VATRegB = 0x80000000; /* VCacheEnhance */
    vtxDesc->VATRegC = 0;
    vtxDesc->VCDRegLO = 0;
    vtxDesc->VCDRegHI = 0;
    vtxDesc->XFRegINVTXSPEC = 0;

    return vtxDesc;
}

void _rwGCNVertexDescSetVAT(rwVertexDescriptor* vtxDesc, RwUInt32 vat)
{
    vtxDesc->VATID = vat;
}

void _rwGCNVertexDescSetElementAttr(rwVertexDescriptor* vtxDesc, rwGCNVertexAttribute attr,
                                    rwGCNCompCnt cnt, rwGCNCompType fmt, RwUInt8 frac)
{
    switch (attr)
    {
    case rwGCNVA_POS:
    {
        vtxDesc->VATRegA &= ~0x000001FF;
        vtxDesc->VATRegA |= (cnt | (fmt << 1) | (frac << 4)) & 0x000001FF;
        break;
    }
    case rwGCNVA_NRM:
    {
        vtxDesc->VATRegA &= ~0x00001E00;
        vtxDesc->VATRegA |= ((cnt << 9) | (fmt << 10)) & 0x00001E00;
        vtxDesc->VATRegA &= ~0x80000000;
        break;
    }
    case rwGCNVA_NBT:
    {
        vtxDesc->VATRegA &= ~0x00001E00;
        vtxDesc->VATRegA |= ((cnt << 9) | (fmt << 10)) & 0x00001E00;
        if (cnt == rwGCNCC_NRM_NBT3)
        {
            vtxDesc->VATRegA |= 0x80000000;
        }
        else
        {
            vtxDesc->VATRegA &= ~0x80000000;
        }
        break;
    }
    case rwGCNVA_CLR0:
    {
        vtxDesc->VATRegA &= ~0x0001E000;
        vtxDesc->VATRegA |= ((cnt << 13) | (fmt << 14)) & 0x0001E000;
        break;
    }
    case rwGCNVA_CLR1:
    {
        vtxDesc->VATRegA &= ~0x001E0000;
        vtxDesc->VATRegA |= ((cnt << 17) | (fmt << 18)) & 0x001E0000;
        break;
    }
    case rwGCNVA_TEX0:
    {
        vtxDesc->VATRegA &= ~0x3FE00000;
        vtxDesc->VATRegA |= ((cnt << 21) | (fmt << 22) | (frac << 25)) & 0x3FE00000;
        break;
    }
    case rwGCNVA_TEX1:
    {
        vtxDesc->VATRegB &= ~0x000001FF;
        vtxDesc->VATRegB |= (cnt | (fmt << 1) | (frac << 4)) & 0x000001FF;
        break;
    }
    case rwGCNVA_TEX2:
    {
        vtxDesc->VATRegB &= ~0x0003FE00;
        vtxDesc->VATRegB |= ((cnt << 9) | (fmt << 10) | (frac << 13)) & 0x0003FE00;
        break;
    }
    case rwGCNVA_TEX3:
    {
        vtxDesc->VATRegB &= ~0x0FFC0000;
        vtxDesc->VATRegB |= ((cnt << 18) | (fmt << 19) | (frac << 22)) & 0x0FFC0000;
        break;
    }
    case rwGCNVA_TEX4:
    {
        vtxDesc->VATRegB &= ~0xF0000000;
        vtxDesc->VATRegB |= ((cnt << 27) | (fmt << 28)) & 0xF0000000;
        vtxDesc->VATRegC &= ~0x0000001F;
        vtxDesc->VATRegC |= frac & 0x0000001F;
        break;
    }
    case rwGCNVA_TEX5:
    {
        vtxDesc->VATRegC &= ~0x00003FE0;
        vtxDesc->VATRegC |= ((cnt << 5) | (fmt << 6) | (frac << 9)) & 0x00003FE0;
        break;
    }
    case rwGCNVA_TEX6:
    {
        vtxDesc->VATRegC &= ~0x007FC000;
        vtxDesc->VATRegC |= ((cnt << 14) | (fmt << 15) | (frac << 18)) & 0x007FC000;
        break;
    }
    case rwGCNVA_TEX7:
    {
        vtxDesc->VATRegC &= ~0xFF800000;
        vtxDesc->VATRegC |= ((cnt << 23) | (fmt << 24) | (frac << 27)) & 0xFF800000;
        break;
    }
    default:
    {
        break;
    }
    }
}

void _rwGCNVertexDescSetElementDesc(rwVertexDescriptor* vtxDesc, rwGCNVertexAttribute attr,
                                    rwGCNAttrType type)
{
    RwUInt32 numTex;
    RwUInt32 i;
    RwUInt32 numcols;

    switch (attr)
    {
    case rwGCNVA_PNMTXIDX:
    {
        vtxDesc->VCDRegLO &= ~0x1;
        vtxDesc->VCDRegLO |= type & 0x1;
        break;
    }
    case rwGCNVA_TEX0MTXIDX:
    case rwGCNVA_TEX1MTXIDX:
    case rwGCNVA_TEX2MTXIDX:
    case rwGCNVA_TEX3MTXIDX:
    case rwGCNVA_TEX4MTXIDX:
    case rwGCNVA_TEX5MTXIDX:
    case rwGCNVA_TEX6MTXIDX:
    case rwGCNVA_TEX7MTXIDX:
    {
        vtxDesc->VCDRegLO &= ~(0x2U << (attr - rwGCNVA_TEX0MTXIDX));
        vtxDesc->VCDRegLO |= (type << attr) & (0x2U << (attr - rwGCNVA_TEX0MTXIDX));
        break;
    }
    case rwGCNVA_POS:
    {
        vtxDesc->VCDRegLO &= ~0x600;
        vtxDesc->VCDRegLO |= (type & 0x3) << 9;
        break;
    }
    case rwGCNVA_NRM:
    {
        RwInt32 numNrm;

        vtxDesc->VCDRegLO &= ~0x1800;
        vtxDesc->VCDRegLO |= (type & 0x3) << 11;

        numNrm = 0;
        if (type != rwGCNAT_NONE)
        {
            numNrm = 1;
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xC;
        vtxDesc->XFRegINVTXSPEC |= (numNrm & 0x3) << 2;
        break;
    }
    case rwGCNVA_NBT:
    {
        RwInt32 numNrm;

        vtxDesc->VCDRegLO &= ~0x1800;
        vtxDesc->VCDRegLO |= (type & 0x3) << 11;

        numNrm = 0;
        if (type != rwGCNAT_NONE)
        {
            numNrm = 2;
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xC;
        vtxDesc->XFRegINVTXSPEC |= (numNrm & 0x3) << 2;
        break;
    }
    case rwGCNVA_CLR0:
    case rwGCNVA_CLR1:
    {
        vtxDesc->VCDRegLO &= ~(0x6000U << (attr - rwGCNVA_CLR0));
        vtxDesc->VCDRegLO |= (type << (attr + 2)) & (0x6000U << (attr - rwGCNVA_CLR0));

        numcols = 0;
        if (vtxDesc->VCDRegLO & 0x6000)
        {
            numcols = 1;
        }
        if (vtxDesc->VCDRegLO & 0x18000)
        {
            numcols++;
        }

        vtxDesc->XFRegINVTXSPEC &= ~0x3;
        vtxDesc->XFRegINVTXSPEC |= numcols & 0x3;
        break;
    }
    case rwGCNVA_TEX0:
    {
        vtxDesc->VCDRegHI &= ~(0x3 << 0);
        vtxDesc->VCDRegHI |= (type & 0x3) << 0;

        i = 8;
        numTex = 0;
        while (i--)
        {
            if (vtxDesc->VCDRegHI & (0x3 << (i * 2)))
            {
                numTex++;
            }
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xF0;
        vtxDesc->XFRegINVTXSPEC |= (numTex & 0xF) << 4;
        break;
    }
    case rwGCNVA_TEX1:
    {
        vtxDesc->VCDRegHI &= ~(0x3 << 2);
        vtxDesc->VCDRegHI |= (type & 0x3) << 2;

        i = 8;
        numTex = 0;
        while (i--)
        {
            if (vtxDesc->VCDRegHI & (0x3 << (i * 2)))
            {
                numTex++;
            }
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xF0;
        vtxDesc->XFRegINVTXSPEC |= (numTex & 0xF) << 4;
        break;
    }
    case rwGCNVA_TEX2:
    {
        vtxDesc->VCDRegHI &= ~(0x3 << 4);
        vtxDesc->VCDRegHI |= (type & 0x3) << 4;

        i = 8;
        numTex = 0;
        while (i--)
        {
            if (vtxDesc->VCDRegHI & (0x3 << (i * 2)))
            {
                numTex++;
            }
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xF0;
        vtxDesc->XFRegINVTXSPEC |= (numTex & 0xF) << 4;
        break;
    }
    case rwGCNVA_TEX3:
    {
        vtxDesc->VCDRegHI &= ~(0x3 << 6);
        vtxDesc->VCDRegHI |= (type & 0x3) << 6;

        i = 8;
        numTex = 0;
        while (i--)
        {
            if (vtxDesc->VCDRegHI & (0x3 << (i * 2)))
            {
                numTex++;
            }
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xF0;
        vtxDesc->XFRegINVTXSPEC |= (numTex & 0xF) << 4;
        break;
    }
    case rwGCNVA_TEX4:
    {
        vtxDesc->VCDRegHI &= ~(0x3 << 8);
        vtxDesc->VCDRegHI |= (type & 0x3) << 8;

        i = 8;
        numTex = 0;
        while (i--)
        {
            if (vtxDesc->VCDRegHI & (0x3 << (i * 2)))
            {
                numTex++;
            }
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xF0;
        vtxDesc->XFRegINVTXSPEC |= (numTex & 0xF) << 4;
        break;
    }
    case rwGCNVA_TEX5:
    {
        vtxDesc->VCDRegHI &= ~(0x3 << 10);
        vtxDesc->VCDRegHI |= (type & 0x3) << 10;

        i = 8;
        numTex = 0;
        while (i--)
        {
            if (vtxDesc->VCDRegHI & (0x3 << (i * 2)))
            {
                numTex++;
            }
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xF0;
        vtxDesc->XFRegINVTXSPEC |= (numTex & 0xF) << 4;
        break;
    }
    case rwGCNVA_TEX6:
    {
        vtxDesc->VCDRegHI &= ~(0x3 << 12);
        vtxDesc->VCDRegHI |= (type & 0x3) << 12;

        i = 8;
        numTex = 0;
        while (i--)
        {
            if (vtxDesc->VCDRegHI & (0x3 << (i * 2)))
            {
                numTex++;
            }
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xF0;
        vtxDesc->XFRegINVTXSPEC |= (numTex & 0xF) << 4;
        break;
    }
    case rwGCNVA_TEX7:
    {
        vtxDesc->VCDRegHI &= ~(0x3 << 14);
        vtxDesc->VCDRegHI |= (type & 0x3) << 14;

        i = 8;
        numTex = 0;
        while (i--)
        {
            if (vtxDesc->VCDRegHI & (0x3 << (i * 2)))
            {
                numTex++;
            }
        }

        vtxDesc->XFRegINVTXSPEC &= ~0xF0;
        vtxDesc->XFRegINVTXSPEC |= (numTex & 0xF) << 4;
        break;
    }
    default:
    {
        break;
    }
    }
}

void _rwGCNVertexDescSetNumIndexedAttr(rwVertexDescriptor* vtxDesc, RwUInt8 numIndxAttr)
{
    vtxDesc->numAttrArrays = numIndxAttr;
}
