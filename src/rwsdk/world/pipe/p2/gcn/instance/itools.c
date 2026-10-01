#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

void _rwGCNTriStripGetStats(RwUInt16* indices, RwUInt32 numIndices, RwUInt32* numStripsOut,
                            RwUInt32* numIndicesOut, RwBool preserveWindingOrder)
{
    RwUInt32 length;
    RwUInt32 i;

    *numStripsOut = 0;
    *numIndicesOut = 0;

    length = 0;

    for (i = 0; i < numIndices - 2; i++)
    {
        if (indices[0] != indices[1] && indices[0] != indices[2] && indices[1] != indices[2])
        {
            /* A real triangle - extend the current strip */
            if (length == 0 && (i & 1) && preserveWindingOrder)
            {
                length = 2;
            }
            else
            {
                length++;
            }
        }
        else if (length != 0)
        {
            /* A degenerate triangle - the current strip may end here */
            if (preserveWindingOrder && i < numIndices - 3)
            {
                if (indices[1] != indices[2] && indices[1] != indices[3] && indices[2] != indices[3])
                {
                    if (((i + 1) & 1) == 0)
                    {
                        *numIndicesOut += length + 2;
                        length = 0;
                        (*numStripsOut)++;
                    }
                    else
                    {
                        length++;
                    }
                }
                else
                {
                    *numIndicesOut += length + 2;
                    length = 0;
                    (*numStripsOut)++;
                }
            }
            else
            {
                *numIndicesOut += length + 2;
                length = 0;
                (*numStripsOut)++;
            }
        }

        indices++;
    }

    if (length != 0)
    {
        *numIndicesOut += length + 2;
        (*numStripsOut)++;
    }
}

static void _rwGCNInstanceIndicesCopy(RwUInt16* indices, RwUInt32 numIndices, RwUInt32 stride,
                                      RwUInt32 indexType, void* memory)
{
    if (indexType == rwGCNAT_INDEX8)
    {
        RwUInt32 i;

        for (i = 0; i < numIndices; i++)
        {
            *(RwUInt8*)memory = (RwUInt8)*indices;
            indices++;
            memory = (RwUInt8*)memory + stride;
        }
    }
    else if (indexType == rwGCNAT_INDEX16)
    {
        RwUInt32 i;

        for (i = 0; i < numIndices; i++)
        {
            *(RwUInt16*)memory = *indices;
            indices++;
            memory = (RwUInt8*)memory + stride;
        }
    }
}

void _rwGCNInstanceIndices(RwUInt16* posIndices, RwUInt16* indices, RwUInt32 numIndices,
                           RwUInt32 numStrips, RwUInt32 stride, RwUInt32 indexType,
                           RwBool preserveWindingOrder, void* memory)
{
    RwUInt32 numStripsWritten;
    RwUInt32 numIndicesWritten;
    RwUInt32 length;
    RwUInt32 i;

    numStripsWritten = 0;
    numIndicesWritten = 0;
    length = 0;

    if (numStrips > 1)
    {
        for (i = 0; i < numIndices - 2; i++)
        {
            if (posIndices[0] != posIndices[1] && posIndices[0] != posIndices[2] &&
                posIndices[1] != posIndices[2])
            {
                if (length == 0 && (i & 1) && preserveWindingOrder)
                {
                    length = 2;
                }
                else
                {
                    length++;
                }
            }
            else if (length != 0)
            {
                if (preserveWindingOrder && i < numIndices - 3)
                {
                    if (posIndices[1] != posIndices[2] && posIndices[1] != posIndices[3] &&
                        posIndices[2] != posIndices[3])
                    {
                        if (((i + 1) & 1) == 0)
                        {
                            _rwGCNInstanceIndicesCopy(&indices[i - length], length + 2, stride,
                                                      indexType,
                                                      (RwUInt8*)memory + numStripsWritten * 3 +
                                                          numIndicesWritten * stride);
                            numIndicesWritten += length + 2;
                            length = 0;
                            numStripsWritten++;
                        }
                        else
                        {
                            length++;
                        }
                    }
                    else
                    {
                        _rwGCNInstanceIndicesCopy(&indices[i - length], length + 2, stride,
                                                  indexType,
                                                  (RwUInt8*)memory + numStripsWritten * 3 +
                                                      numIndicesWritten * stride);
                        numIndicesWritten += length + 2;
                        length = 0;
                        numStripsWritten++;
                    }
                }
                else
                {
                    _rwGCNInstanceIndicesCopy(&indices[i - length], length + 2, stride, indexType,
                                              (RwUInt8*)memory + numStripsWritten * 3 +
                                                  numIndicesWritten * stride);
                    numIndicesWritten += length + 2;
                    length = 0;
                    numStripsWritten++;
                }
            }

            posIndices++;
        }

        if (length != 0)
        {
            _rwGCNInstanceIndicesCopy(&indices[i - length], length + 2, stride, indexType,
                                      (RwUInt8*)memory + numStripsWritten * 3 +
                                          numIndicesWritten * stride);
        }
    }
    else
    {
        _rwGCNInstanceIndicesCopy(indices, numIndices, stride, indexType, memory);
    }
}

/* Writes a GXBegin style primitive header (command|vat, vertex count) */
#define WRITESTRIPHEADER(_count)                                                                  \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwUInt8* header;                                                                           \
                                                                                                   \
        header = (RwUInt8*)memory + numStripsWritten * 3 + numIndicesWritten * stride;             \
        header[0] = primTypeVAT;                                                                   \
        *(RwUInt16*)(header + 1) = (RwUInt16)(_count);                                             \
    }                                                                                              \
    MACRO_STOP

static void WriteHeaders(RwUInt16* posIndices, RwUInt32 numIndices, RwUInt32 numStrips,
                         RwUInt32 stride, RwUInt8 primTypeVAT, RwBool preserveWindingOrder,
                         void* memory)
{
    RwUInt32 i;
    RwUInt32 numStripsWritten;
    RwUInt32 numIndicesWritten;
    RwUInt32 length;

    numStripsWritten = 0;
    numIndicesWritten = 0;
    length = 0;

    if (numStrips > 1)
    {
        for (i = 0; i < numIndices - 2; i++)
        {
            if (posIndices[0] != posIndices[1] && posIndices[0] != posIndices[2] &&
                posIndices[1] != posIndices[2])
            {
                if (length == 0 && (i & 1) && preserveWindingOrder)
                {
                    length = 2;
                }
                else
                {
                    length++;
                }
            }
            else if (length != 0)
            {
                if (preserveWindingOrder && i < numIndices - 3)
                {
                    if (posIndices[1] != posIndices[2] && posIndices[1] != posIndices[3] &&
                        posIndices[2] != posIndices[3])
                    {
                        if (((i + 1) & 1) == 0)
                        {
                            WRITESTRIPHEADER(length + 2);
                            numIndicesWritten += length + 2;
                            length = 0;
                            numStripsWritten++;
                        }
                        else
                        {
                            length++;
                        }
                    }
                    else
                    {
                        WRITESTRIPHEADER(length + 2);
                        numIndicesWritten += length + 2;
                        length = 0;
                        numStripsWritten++;
                    }
                }
                else
                {
                    WRITESTRIPHEADER(length + 2);
                    numIndicesWritten += length + 2;
                    length = 0;
                    numStripsWritten++;
                }
            }

            posIndices++;
        }

        if (length != 0)
        {
            WRITESTRIPHEADER(length + 2);
        }
    }
    else
    {
        WRITESTRIPHEADER(numIndices);
    }
}

void _rwGCNDisplayListFill(rwVertexDescriptor* vtxDesc, RxGameCubeDisplayList* displayList,
                           rwGCNDisplayListData* displayListData, RwUInt32 numIndices,
                           RwUInt32 numStrips, RwUInt32 stride, RwBool preserveWindingOrder,
                           RwUInt8 primTypeVAT)
{
    RwUInt32 offset;
    RwUInt32 type;
    RwUInt32 fmt;
    RwReal scale;
    RwUInt32 i;
    void* memory;
    RwUInt32 cnt;

    /* The primitive headers go in first, the per attribute indices are interleaved after them */
    WriteHeaders((RwUInt16*)displayListData->data[rwGCNVA_POS], numIndices, numStrips, stride,
                 primTypeVAT, preserveWindingOrder, displayList->displayList);

    for (i = 0, offset = 3; i < rwGCNVA_TEX7 + 1; i++)
    {
        memory = (RwUInt8*)displayList->displayList + offset;

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
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, rwGCNAT_INDEX8, preserveWindingOrder, memory);
                offset += 1;
            }
            break;
        }
        case rwGCNVA_POS:
        {
            type = (vtxDesc->VCDRegLO >> 9) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                if ((vtxDesc->VATRegA & 0x1) == rwGCNCC_POS_XYZ)
                {
                    scale = (RwReal)(1 << ((vtxDesc->VATRegA >> 4) & 0x1F));
                    fmt = (vtxDesc->VATRegA >> 1) & 0x7;

                    _rwGCNVtxFmtInstPos3D((RwUInt8*)memory, (RwV3d*)displayListData->data[i], fmt,
                                          scale, numIndices, stride);
                    offset += rwGCNPosGetSize(vtxDesc);
                }
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_NRM:
        {
            type = (vtxDesc->VCDRegLO >> 11) & 0x3;
            if (((vtxDesc->VATRegA >> 9) & 0x1) == rwGCNCC_NRM_NBT)
            {
                if (type == rwGCNAT_DIRECT)
                {
                    offset += rwGCNNrmGetSize(vtxDesc) * 3;
                }
                else if (type != rwGCNAT_NONE)
                {
                    if (!(vtxDesc->VATRegA >> 31))
                    {
                        cnt = 1;
                        _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                              (RwUInt16*)displayListData->data[i], numIndices,
                                              numStrips, stride, type, preserveWindingOrder,
                                              memory);
                    }
                    else
                    {
                        cnt = 3;
                    }

                    if (type == rwGCNAT_INDEX8)
                    {
                        offset += cnt;
                    }
                    else if (type == rwGCNAT_INDEX16)
                    {
                        offset += cnt * 2;
                    }
                }
            }
            else
            {
                if (type == rwGCNAT_DIRECT)
                {
                    fmt = (vtxDesc->VATRegA >> 10) & 0x7;

                    _rwGCNVtxFmtInstNrm((RwUInt8*)memory, (RwV3d*)displayListData->data[i], fmt,
                                        numIndices, stride);
                    offset += rwGCNNrmGetSize(vtxDesc);
                }
                else if (type != rwGCNAT_NONE)
                {
                    _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                          (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                          stride, type, preserveWindingOrder, memory);

                    if (type == rwGCNAT_INDEX8)
                    {
                        offset += 1;
                    }
                    else if (type == rwGCNAT_INDEX16)
                    {
                        offset += 2;
                    }
                }
            }
            break;
        }
        case rwGCNVA_CLR0:
        case rwGCNVA_CLR1:
        {
            type = (vtxDesc->VCDRegLO >> ((i - rwGCNVA_CLR0) * 2 + 13)) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                fmt = (vtxDesc->VATRegA >> ((i - rwGCNVA_CLR0) * 4 + 14)) & 0x7;

                _rwGCNVtxFmtInstClr((RwUInt8*)memory, (RwRGBA*)displayListData->data[i], fmt,
                                    numIndices, stride);
                offset += rwGCNClrGetSize(vtxDesc, (RwUInt8)(i - rwGCNVA_CLR0));
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_TEX0:
        {
            type = (vtxDesc->VCDRegHI >> 0) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                scale = (RwReal)(1 << ((vtxDesc->VATRegA >> 25) & 0x1F));
                fmt = (vtxDesc->VATRegA >> 22) & 0x7;

                _rwGCNVtxFmtInstTex((RwUInt8*)memory, (RwTexCoords*)displayListData->data[i], fmt,
                                    scale, numIndices, stride);
                offset += rwGCNTexGetSize(vtxDesc, 0);
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_TEX1:
        {
            type = (vtxDesc->VCDRegHI >> 2) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                fmt = (vtxDesc->VATRegB >> 1) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegB >> 4) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)memory, (RwTexCoords*)displayListData->data[i], fmt,
                                    scale, numIndices, stride);
                offset += rwGCNTexGetSize(vtxDesc, 1);
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_TEX2:
        {
            type = (vtxDesc->VCDRegHI >> 4) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                fmt = (vtxDesc->VATRegB >> 10) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegB >> 13) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)memory, (RwTexCoords*)displayListData->data[i], fmt,
                                    scale, numIndices, stride);
                offset += rwGCNTexGetSize(vtxDesc, 2);
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_TEX3:
        {
            type = (vtxDesc->VCDRegHI >> 6) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                fmt = (vtxDesc->VATRegB >> 19) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegB >> 22) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)memory, (RwTexCoords*)displayListData->data[i], fmt,
                                    scale, numIndices, stride);
                offset += rwGCNTexGetSize(vtxDesc, 3);
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_TEX4:
        {
            type = (vtxDesc->VCDRegHI >> 8) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                fmt = (vtxDesc->VATRegB >> 28) & 0x7;
                scale = (RwReal)(1 << vtxDesc->VATRegC & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)memory, (RwTexCoords*)displayListData->data[i], fmt,
                                    scale, numIndices, stride);
                offset += rwGCNTexGetSize(vtxDesc, 4);
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_TEX5:
        {
            type = (vtxDesc->VCDRegHI >> 10) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                fmt = (vtxDesc->VATRegC >> 6) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegC >> 9) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)memory, (RwTexCoords*)displayListData->data[i], fmt,
                                    scale, numIndices, stride);
                offset += rwGCNTexGetSize(vtxDesc, 5);
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_TEX6:
        {
            type = (vtxDesc->VCDRegHI >> 12) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                fmt = (vtxDesc->VATRegC >> 15) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegC >> 18) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)memory, (RwTexCoords*)displayListData->data[i], fmt,
                                    scale, numIndices, stride);
                offset += rwGCNTexGetSize(vtxDesc, 6);
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        case rwGCNVA_TEX7:
        {
            type = (vtxDesc->VCDRegHI >> 14) & 0x3;
            if (type == rwGCNAT_DIRECT)
            {
                fmt = (vtxDesc->VATRegC >> 24) & 0x7;
                scale = (RwReal)(1 << (vtxDesc->VATRegC >> 27) & 0x1F);

                _rwGCNVtxFmtInstTex((RwUInt8*)memory, (RwTexCoords*)displayListData->data[i], fmt,
                                    scale, numIndices, stride);
                offset += rwGCNTexGetSize(vtxDesc, 7);
            }
            else if (type != rwGCNAT_NONE)
            {
                _rwGCNInstanceIndices((RwUInt16*)displayListData->data[rwGCNVA_POS],
                                      (RwUInt16*)displayListData->data[i], numIndices, numStrips,
                                      stride, type, preserveWindingOrder, memory);

                if (type == rwGCNAT_INDEX8)
                {
                    offset += 1;
                }
                else if (type == rwGCNAT_INDEX16)
                {
                    offset += 2;
                }
            }
            break;
        }
        }
    }
}
