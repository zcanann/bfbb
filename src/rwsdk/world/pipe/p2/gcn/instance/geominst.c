#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/instance/gcninst.h"

RwUInt32 rwGCNPosGetSize(rwVertexDescriptor* vtxDesc)
{
    RwUInt32 size;
    RwUInt32 cnt;
    RwUInt32 fmt;

    cnt = vtxDesc->VATRegA & 0x1;
    size = cnt;
    if (cnt == rwGCNCC_POS_XYZ)
    {
        size = 3;
    }
    else if (cnt == rwGCNCC_POS_XY)
    {
        size = 2;
    }

    fmt = (vtxDesc->VATRegA >> 1) & 0x7;
    switch (fmt)
    {
    case rwGCNCT_U8:
    case rwGCNCT_S8:
        return size;
    case rwGCNCT_U16:
    case rwGCNCT_S16:
        return size * 2;
    case rwGCNCT_F32:
        return size * 4;
    }

    return 0;
}

RwUInt32 rwGCNNrmGetSize(rwVertexDescriptor* vtxDesc)
{
    RwUInt32 fmt;

    fmt = (vtxDesc->VATRegA >> 10) & 0x7;
    switch (fmt)
    {
    case rwGCNCT_S8:
        return 3;
    case rwGCNCT_S16:
        return 6;
    case rwGCNCT_F32:
        return 12;
    }

    return 0;
}

RwUInt32 rwGCNClrGetSize(rwVertexDescriptor* vtxDesc, RwUInt8 clrNum)
{
    RwUInt32 fmt;

    fmt = (vtxDesc->VATRegA >> (clrNum * 4 + 14)) & 0x7;
    switch (fmt)
    {
    case rwGCNCT_RGB565:
    case rwGCNCT_RGBA4:
        return 2;
    case rwGCNCT_RGB8:
    case rwGCNCT_RGBA6:
        return 3;
    case rwGCNCT_RGBX8:
    case rwGCNCT_RGBA8:
        return 4;
    }

    return 0;
}

RwUInt32 rwGCNTexGetSize(rwVertexDescriptor* vtxDesc, RwUInt8 texNum)
{
    RwUInt32 size;
    RwUInt32 cnt;
    RwUInt32 fmt;

    switch (texNum)
    {
    case 0:
        cnt = (vtxDesc->VATRegA >> 21) & 0x1;
        fmt = (vtxDesc->VATRegA >> 22) & 0x7;
        break;
    case 1:
        cnt = vtxDesc->VATRegB & 0x1;
        fmt = (vtxDesc->VATRegB >> 1) & 0x7;
        break;
    case 2:
        cnt = (vtxDesc->VATRegB >> 9) & 0x1;
        fmt = (vtxDesc->VATRegB >> 10) & 0x7;
        break;
    case 3:
        cnt = (vtxDesc->VATRegB >> 18) & 0x1;
        fmt = (vtxDesc->VATRegB >> 19) & 0x7;
        break;
    case 4:
        cnt = (vtxDesc->VATRegB >> 27) & 0x1;
        fmt = (vtxDesc->VATRegB >> 28) & 0x7;
        break;
    case 5:
        cnt = (vtxDesc->VATRegC >> 5) & 0x1;
        fmt = (vtxDesc->VATRegC >> 6) & 0x7;
        break;
    case 6:
        cnt = (vtxDesc->VATRegC >> 14) & 0x1;
        fmt = (vtxDesc->VATRegC >> 15) & 0x7;
        break;
    case 7:
        cnt = (vtxDesc->VATRegC >> 23) & 0x1;
        fmt = (vtxDesc->VATRegC >> 24) & 0x7;
        break;
    default:
        return 0;
    }

    size = cnt;
    if (cnt == rwGCNCC_TEX_ST)
    {
        size = 2;
    }
    else if (cnt == rwGCNCC_TEX_S)
    {
        size = 1;
    }

    switch (fmt)
    {
    case rwGCNCT_U8:
    case rwGCNCT_S8:
        return size;
    case rwGCNCT_U16:
    case rwGCNCT_S16:
        return size * 2;
    case rwGCNCT_F32:
        return size * 4;
    }

    return 0;
}

RwUInt32 _rwGCNVtxFmtInstPos3D(RwUInt8* mem, RwV3d* srcPosition, RwUInt32 fmt, RwReal scale,
                               RwInt32 numVerts, RwUInt32 stride)
{
    RwUInt32 bytesWritten;

    bytesWritten = 0;

    switch (fmt)
    {
    case rwGCNCT_U8:
    {
        RwInt32 i;
        RwUInt8* dstPosition;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwUInt8);
        }

        dstPosition = (RwUInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstPosition[0] = (RwUInt8)(srcPosition->x * scale);
            dstPosition[1] = (RwUInt8)(srcPosition->y * scale);
            dstPosition[2] = (RwUInt8)(srcPosition->z * scale);

            srcPosition++;
            dstPosition = (RwUInt8*)((RwUInt8*)dstPosition + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwUInt8));
        break;
    }
    case rwGCNCT_S8:
    {
        RwInt32 i;
        RwInt8* dstPosition;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt8);
        }

        dstPosition = (RwInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstPosition[0] = (RwInt8)(srcPosition->x * scale);
            dstPosition[1] = (RwInt8)(srcPosition->y * scale);
            dstPosition[2] = (RwInt8)(srcPosition->z * scale);

            srcPosition++;
            dstPosition = (RwInt8*)((RwUInt8*)dstPosition + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt8));
        break;
    }
    case rwGCNCT_U16:
    {
        RwInt32 i;
        RwUInt16* dstPosition;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwUInt16);
        }

        dstPosition = (RwUInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstPosition[0] = (RwUInt16)(srcPosition->x * scale);
            dstPosition[1] = (RwUInt16)(srcPosition->y * scale);
            dstPosition[2] = (RwUInt16)(srcPosition->z * scale);

            srcPosition++;
            dstPosition = (RwUInt16*)((RwUInt8*)dstPosition + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwUInt16));
        break;
    }
    case rwGCNCT_S16:
    {
        RwInt32 i;
        RwInt16* dstPosition;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt16);
        }

        dstPosition = (RwInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstPosition[0] = (RwInt16)(srcPosition->x * scale);
            dstPosition[1] = (RwInt16)(srcPosition->y * scale);
            dstPosition[2] = (RwInt16)(srcPosition->z * scale);

            srcPosition++;
            dstPosition = (RwInt16*)((RwUInt8*)dstPosition + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt16));
        break;
    }
    case rwGCNCT_F32:
    {
        RwInt32 i;
        RwV3d* dstPosition;

        if (stride == 0)
        {
            stride = sizeof(RwV3d);
        }

        dstPosition = (RwV3d*)mem;

        for (i = 0; i < numVerts; i++)
        {
            *dstPosition = *srcPosition;

            srcPosition++;
            dstPosition = (RwV3d*)((RwUInt8*)dstPosition + stride);
        }

        bytesWritten = numVerts * sizeof(RwV3d);
        break;
    }
    }

    return bytesWritten;
}

RwUInt32 _rwGCNVtxFmtInstNrm(RwUInt8* mem, RwV3d* srcNormal, RwUInt32 fmt, RwInt32 numVerts, RwUInt32 stride)
{
    RwUInt32 bytesWritten;

    bytesWritten = 0;

    switch (fmt)
    {
    case rwGCNCT_U8:
    case rwGCNCT_U16:
    {
        /* Unsigned normals are not supported */
        break;
    }
    case rwGCNCT_S8:
    {
        RwInt32 i;
        RwReal scale;
        RwInt8* dstNormal;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt8);
        }

        scale = 64.0f;
        dstNormal = (RwInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstNormal[0] = (RwInt8)(srcNormal->x * scale);
            dstNormal[1] = (RwInt8)(srcNormal->y * scale);
            dstNormal[2] = (RwInt8)(srcNormal->z * scale);

            srcNormal++;
            dstNormal = (RwInt8*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt8));
        break;
    }
    case rwGCNCT_S16:
    {
        RwInt32 i;
        RwReal scale;
        RwInt16* dstNormal;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt16);
        }

        scale = 16384.0f;
        dstNormal = (RwInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstNormal[0] = (RwInt16)(srcNormal->x * scale);
            dstNormal[1] = (RwInt16)(srcNormal->y * scale);
            dstNormal[2] = (RwInt16)(srcNormal->z * scale);

            srcNormal++;
            dstNormal = (RwInt16*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt16));
        break;
    }
    case rwGCNCT_F32:
    {
        RwInt32 i;
        RwV3d* dstNormal;

        if (stride == 0)
        {
            stride = sizeof(RwV3d);
        }

        dstNormal = (RwV3d*)mem;

        for (i = 0; i < numVerts; i++)
        {
            *dstNormal = *srcNormal;

            srcNormal++;
            dstNormal = (RwV3d*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * sizeof(RwV3d);
        break;
    }
    }

    return bytesWritten;
}

RwUInt32 _rwGCNVtxFmtInstNrmCmp(RwUInt8* mem, RpVertexNormal* srcNormal, RwUInt32 fmt, RwInt32 numVerts, RwUInt32 stride)
{
    RwUInt32 bytesWritten;

    bytesWritten = 0;

    switch (fmt)
    {
    case rwGCNCT_U8:
    case rwGCNCT_U16:
    {
        /* Unsigned normals are not supported */
        break;
    }
    case rwGCNCT_S8:
    {
        RwInt32 i;
        RwReal scale;
        RwInt8* dstNormal;
        RwV3d normal;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt8);
        }

        scale = 64.0f;
        dstNormal = (RwInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            normal.x = (RwReal)srcNormal->x * (1.0f / 128.0f);
            normal.y = (RwReal)srcNormal->y * (1.0f / 128.0f);
            normal.z = (RwReal)srcNormal->z * (1.0f / 128.0f);

            dstNormal[0] = (RwInt8)(normal.x * scale);
            dstNormal[1] = (RwInt8)(normal.y * scale);
            dstNormal[2] = (RwInt8)(normal.z * scale);

            srcNormal++;
            dstNormal = (RwInt8*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt8));
        break;
    }
    case rwGCNCT_S16:
    {
        RwInt32 i;
        RwReal scale;
        RwInt16* dstNormal;
        RwV3d normal;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt16);
        }

        scale = 16384.0f;
        dstNormal = (RwInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            normal.x = (RwReal)srcNormal->x * (1.0f / 128.0f);
            normal.y = (RwReal)srcNormal->y * (1.0f / 128.0f);
            normal.z = (RwReal)srcNormal->z * (1.0f / 128.0f);

            dstNormal[0] = (RwInt16)(normal.x * scale);
            dstNormal[1] = (RwInt16)(normal.y * scale);
            dstNormal[2] = (RwInt16)(normal.z * scale);

            srcNormal++;
            dstNormal = (RwInt16*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt16));
        break;
    }
    case rwGCNCT_F32:
    {
        RwInt32 i;
        RwV3d* dstNormal;
        RwV3d normal;

        if (stride == 0)
        {
            stride = sizeof(RwV3d);
        }

        dstNormal = (RwV3d*)mem;

        for (i = 0; i < numVerts; i++)
        {
            normal.x = (RwReal)srcNormal->x * (1.0f / 128.0f);
            normal.y = (RwReal)srcNormal->y * (1.0f / 128.0f);
            normal.z = (RwReal)srcNormal->z * (1.0f / 128.0f);

            *dstNormal = normal;

            srcNormal++;
            dstNormal = (RwV3d*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * sizeof(RwV3d);
        break;
    }
    }

    return bytesWritten;
}

RwUInt32 _rwGCNVtxFmtInstNBT(RwUInt8* mem, RwV3d* srcNormal, RwUInt32 fmt, RwInt32 numVerts, RwUInt32 stride)
{
    RwUInt32 bytesWritten;

    bytesWritten = 0;

    switch (fmt)
    {
    case rwGCNCT_U8:
    case rwGCNCT_U16:
    {
        /* Unsigned normals are not supported */
        break;
    }
    case rwGCNCT_S8:
    {
        RwInt32 i;
        RwReal scale;
        RwInt8* dstNormal;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt8);
        }

        scale = 64.0f;
        dstNormal = (RwInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstNormal[0] = (RwInt8)(srcNormal->x * scale);
            dstNormal[1] = (RwInt8)(srcNormal->y * scale);
            dstNormal[2] = (RwInt8)(srcNormal->z * scale);

            srcNormal++;
            dstNormal = (RwInt8*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt8));
        break;
    }
    case rwGCNCT_S16:
    {
        RwInt32 i;
        RwReal scale;
        RwInt16* dstNormal;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt16);
        }

        scale = 16384.0f;
        dstNormal = (RwInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstNormal[0] = (RwInt16)(srcNormal->x * scale);
            dstNormal[1] = (RwInt16)(srcNormal->y * scale);
            dstNormal[2] = (RwInt16)(srcNormal->z * scale);

            srcNormal++;
            dstNormal = (RwInt16*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt16));
        break;
    }
    case rwGCNCT_F32:
    {
        RwInt32 i;
        RwV3d* dstNormal;

        if (stride == 0)
        {
            stride = sizeof(RwV3d);
        }

        dstNormal = (RwV3d*)mem;

        for (i = 0; i < numVerts; i++)
        {
            *dstNormal = *srcNormal;

            srcNormal++;
            dstNormal = (RwV3d*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwV3d));
        break;
    }
    }

    return bytesWritten;
}

RwUInt32 _rwGCNVtxFmtInstNBTCmp(RwUInt8* mem, RpVertexNormal* srcNormal, RwUInt32 fmt, RwInt32 numVerts, RwUInt32 stride)
{
    RwUInt32 bytesWritten;

    bytesWritten = 0;

    switch (fmt)
    {
    case rwGCNCT_U8:
    case rwGCNCT_U16:
    {
        /* Unsigned normals are not supported */
        break;
    }
    case rwGCNCT_S8:
    {
        RwInt32 i;
        RwReal scale;
        RwInt8* dstNormal;
        RwV3d normal;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt8);
        }

        scale = 64.0f;
        dstNormal = (RwInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            normal.x = (RwReal)srcNormal->x * (1.0f / 128.0f);
            normal.y = (RwReal)srcNormal->y * (1.0f / 128.0f);
            normal.z = (RwReal)srcNormal->z * (1.0f / 128.0f);

            dstNormal[0] = (RwInt8)(normal.x * scale);
            dstNormal[1] = (RwInt8)(normal.y * scale);
            dstNormal[2] = (RwInt8)(normal.z * scale);

            srcNormal++;
            dstNormal = (RwInt8*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt8));
        break;
    }
    case rwGCNCT_S16:
    {
        RwInt32 i;
        RwReal scale;
        RwInt16* dstNormal;
        RwV3d normal;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwInt16);
        }

        scale = 16384.0f;
        dstNormal = (RwInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            normal.x = (RwReal)srcNormal->x * (1.0f / 128.0f);
            normal.y = (RwReal)srcNormal->y * (1.0f / 128.0f);
            normal.z = (RwReal)srcNormal->z * (1.0f / 128.0f);

            dstNormal[0] = (RwInt16)(normal.x * scale);
            dstNormal[1] = (RwInt16)(normal.y * scale);
            dstNormal[2] = (RwInt16)(normal.z * scale);

            srcNormal++;
            dstNormal = (RwInt16*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwInt16));
        break;
    }
    case rwGCNCT_F32:
    {
        RwInt32 i;
        RwV3d* dstNormal;
        RwV3d normal;

        if (stride == 0)
        {
            stride = sizeof(RwV3d);
        }

        dstNormal = (RwV3d*)mem;

        for (i = 0; i < numVerts; i++)
        {
            normal.x = (RwReal)srcNormal->x * (1.0f / 128.0f);
            normal.y = (RwReal)srcNormal->y * (1.0f / 128.0f);
            normal.z = (RwReal)srcNormal->z * (1.0f / 128.0f);

            *dstNormal = normal;

            srcNormal++;
            dstNormal = (RwV3d*)((RwUInt8*)dstNormal + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwV3d));
        break;
    }
    }

    return bytesWritten;
}

RwUInt32 _rwGCNVtxFmtInstClr(RwUInt8* mem, RwRGBA* srcColor, RwUInt32 fmt, RwInt32 numVerts,
                             RwUInt32 stride)
{
    RwUInt32 bytesWritten;

    bytesWritten = 0;

    switch (fmt)
    {
    case rwGCNCT_RGB565:
    {
        RwInt32 i;
        RwUInt16* dstColor;

        if (stride == 0)
        {
            stride = sizeof(RwUInt16);
        }

        dstColor = (RwUInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            *dstColor = ((srcColor->red << 8) & 0xF800) | ((srcColor->green << 3) & 0x07E0) |
                        ((srcColor->blue >> 3) & 0x001F);

            srcColor++;
            dstColor = (RwUInt16*)((RwUInt8*)dstColor + stride);
        }

        bytesWritten = numVerts * sizeof(RwUInt16);
        break;
    }
    case rwGCNCT_RGB8:
    {
        RwInt32 i;
        RwUInt8* dstColor;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwUInt8);
        }

        dstColor = (RwUInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstColor[0] = srcColor->red;
            dstColor[1] = srcColor->green;
            dstColor[2] = srcColor->blue;

            srcColor++;
            dstColor = (RwUInt8*)((RwUInt8*)dstColor + stride);
        }

        bytesWritten = numVerts * (3 * sizeof(RwUInt8));
        break;
    }
    case rwGCNCT_RGBA4:
    {
        RwInt32 i;
        RwUInt16* dstColor;

        if (stride == 0)
        {
            stride = sizeof(RwUInt16);
        }

        dstColor = (RwUInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            *dstColor = ((srcColor->red << 8) & 0xF000) | ((srcColor->green << 4) & 0x0F00) |
                        (srcColor->blue & 0x00F0) | ((srcColor->alpha >> 4) & 0x000F);

            srcColor++;
            dstColor = (RwUInt16*)((RwUInt8*)dstColor + stride);
        }

        bytesWritten = numVerts * sizeof(RwUInt16);
        break;
    }
    case rwGCNCT_RGBA6:
    {
        RwInt32 i;
        RwUInt8* dstColor;

        if (stride == 0)
        {
            stride = 3 * sizeof(RwUInt8);
        }

        dstColor = (RwUInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstColor[0] = (srcColor->red & 0xFC) | ((srcColor->green >> 6) & 0x03);
            dstColor[1] = ((srcColor->green << 2) & 0xF0) | ((srcColor->blue >> 4) & 0x0F);
            dstColor[2] = ((srcColor->blue << 4) & 0xC0) | ((srcColor->alpha >> 2) & 0x3F);

            srcColor++;
            dstColor += stride;
        }

        bytesWritten = numVerts * (3 * sizeof(RwUInt8));
        break;
    }
    case rwGCNCT_RGBX8:
    case rwGCNCT_RGBA8:
    {
        RwInt32 i;
        RwRGBA* dstColor;

        if (stride == 0)
        {
            stride = sizeof(RwRGBA);
        }

        dstColor = (RwRGBA*)mem;

        for (i = 0; i < numVerts; i++)
        {
            *dstColor = *srcColor;

            srcColor++;
            dstColor = (RwRGBA*)((RwUInt8*)dstColor + stride);
        }

        bytesWritten = numVerts * sizeof(RwRGBA);
        break;
    }
    }

    return bytesWritten;
}

RwUInt32 _rwGCNVtxFmtInstTex(RwUInt8* mem, RwTexCoords* srcTexCoord, RwUInt32 fmt, RwReal scale,
                             RwInt32 numVerts, RwUInt32 stride)
{
    RwUInt32 bytesWritten;

    bytesWritten = 0;

    switch (fmt)
    {
    case rwGCNCT_U8:
    {
        RwInt32 i;
        RwUInt8* dstTexCoord;

        if (stride == 0)
        {
            stride = 2 * sizeof(RwUInt8);
        }

        dstTexCoord = (RwUInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstTexCoord[0] = (RwUInt8)(srcTexCoord->u * scale);
            dstTexCoord[1] = (RwUInt8)(srcTexCoord->v * scale);

            srcTexCoord++;
            dstTexCoord += stride;
        }

        bytesWritten = numVerts * (2 * sizeof(RwUInt8));
        break;
    }
    case rwGCNCT_S8:
    {
        RwInt32 i;
        RwInt8* dstTexCoord;

        if (stride == 0)
        {
            stride = 2 * sizeof(RwInt8);
        }

        dstTexCoord = (RwInt8*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstTexCoord[0] = (RwInt8)(srcTexCoord->u * scale);
            dstTexCoord[1] = (RwInt8)(srcTexCoord->v * scale);

            srcTexCoord++;
            dstTexCoord += stride;
        }

        bytesWritten = numVerts * (2 * sizeof(RwInt8));
        break;
    }
    case rwGCNCT_U16:
    {
        RwInt32 i;
        RwUInt16* dstTexCoord;

        if (stride == 0)
        {
            stride = 2 * sizeof(RwUInt16);
        }

        dstTexCoord = (RwUInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstTexCoord[0] = (RwUInt16)(srcTexCoord->u * scale);
            dstTexCoord[1] = (RwUInt16)(srcTexCoord->v * scale);

            srcTexCoord++;
            dstTexCoord = (RwUInt16*)((RwUInt8*)dstTexCoord + stride);
        }

        bytesWritten = numVerts * (2 * sizeof(RwUInt16));
        break;
    }
    case rwGCNCT_S16:
    {
        RwInt32 i;
        RwInt16* dstTexCoord;

        if (stride == 0)
        {
            stride = 2 * sizeof(RwInt16);
        }

        dstTexCoord = (RwInt16*)mem;

        for (i = 0; i < numVerts; i++)
        {
            dstTexCoord[0] = (RwInt16)(srcTexCoord->u * scale);
            dstTexCoord[1] = (RwInt16)(srcTexCoord->v * scale);

            srcTexCoord++;
            dstTexCoord = (RwInt16*)((RwUInt8*)dstTexCoord + stride);
        }

        bytesWritten = numVerts * (2 * sizeof(RwInt16));
        break;
    }
    case rwGCNCT_F32:
    {
        RwInt32 i;
        RwTexCoords* dstTexCoord;

        if (stride == 0)
        {
            stride = sizeof(RwTexCoords);
        }

        dstTexCoord = (RwTexCoords*)mem;

        for (i = 0; i < numVerts; i++)
        {
            *dstTexCoord = *srcTexCoord;

            srcTexCoord++;
            dstTexCoord = (RwTexCoords*)((RwUInt8*)dstTexCoord + stride);
        }

        bytesWritten = numVerts * sizeof(RwTexCoords);
        break;
    }
    }

    return bytesWritten;
}
