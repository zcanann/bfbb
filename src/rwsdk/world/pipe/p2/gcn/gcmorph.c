#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/os.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

static void _rwDlV3dInterp(void* dst, const void* src1, const void* src2, const RwReal* scale,
                           RwInt32 count, RwUInt32 inSize, RwUInt32 outSize, RwInt32 outSkip);

static void _rwDlV3dInterpPosGQRSetup(const RpGameCubeVtxFmt* fmt, RwUInt32* outSize)
{
    static RwUInt8 vtxFmtTypeConvTable[5] = { 4, 6, 5, 7, 0 };
    static RwUInt8 vtxFmtSizeConvTable[5] = { 1, 1, 2, 2, 4 };
    register RwUInt32 gqr;

    gqr = 0;

    if (fmt != NULL)
    {
        gqr = vtxFmtTypeConvTable[fmt->pos] | (fmt->posFrac << 8);
        *outSize = vtxFmtSizeConvTable[fmt->pos];
    }
    else
    {
        *outSize = sizeof(RwReal);
    }

    asm { mtspr GQR6, gqr }
}

static void _rwDlV3dInterpNormGQRSetup(const RpGameCubeVtxFmt* fmt, RwUInt32* outSize,
                                       RwInt32* outSkip)
{
    register RwUInt32 gqr;

    gqr = 0;

    if (fmt != NULL)
    {
        RwUInt8 vtxFmtTypeConvTable[5] = { 4, 6, 5, 7, 0 };
        RwUInt8 vtxFmtSizeConvTable[5] = { 1, 1, 2, 2, 4 };
        RwUInt8 vtxFmtNormConvTable[5] = { 0, 6, 0, 14, 0 };

        gqr = vtxFmtTypeConvTable[fmt->norm] | (vtxFmtNormConvTable[fmt->norm] << 8);
        *outSize = vtxFmtSizeConvTable[fmt->norm];
        *outSkip = fmt->nbt ? (*outSize * 6) : 0;
    }
    else
    {
        *outSize = sizeof(RwReal);
        *outSkip = 0;
    }

    asm { mtspr GQR6, gqr }
}

void _rxGCInstanceMorphUpdate(RpGeometry* geometry, RxGameCubeVertexBuffer* vbHeader,
                              RpInterpolator* interp)
{
    RwUInt32 size;
    RwInt32 skip;
    RwInt32 startMT;
    RwInt32 endMT;
    RwReal scale;
    void* dstPos;
    const RpMorphTarget* morphTarget1;
    const RpMorphTarget* morphTarget2;
    void* dstNormals;

    startMT = interp->startMorphTarget;
    endMT = interp->endMorphTarget;
    dstPos = vbHeader->attr[0].array;
    morphTarget1 = &geometry->morphTarget[startMT];
    morphTarget2 = &geometry->morphTarget[endMT];
    scale = interp->recipTime * interp->position;

    _rwDlV3dInterpPosGQRSetup(GEOMVTXFMT(geometry), &size);
    _rwDlV3dInterp(dstPos, morphTarget1->verts, morphTarget2->verts, &scale,
                   geometry->numVertices, sizeof(RwReal), size, 0);
    DCFlushRange(vbHeader->attr[0].array, geometry->numVertices * (size * 3));

    if (geometry->flags & rpGEOMETRYNORMALS)
    {
        dstNormals = vbHeader->attr[1].array;

        _rwDlV3dInterpNormGQRSetup(GEOMVTXFMT(geometry), &size, &skip);
        _rwDlV3dInterp(dstNormals, morphTarget1->normals, morphTarget2->normals, &scale,
                       geometry->numVertices, sizeof(RwReal), size, skip);
        DCFlushRange(vbHeader->attr[1].array, geometry->numVertices * (skip + size * 3));
    }

    GXInvalidateVtxCache();
}

/* clang-format off */
static asm void _rwDlV3dInterp(register void* dst, register const void* src1,
                               register const void* src2, register const RwReal* scale,
                               register RwInt32 count, register RwUInt32 inSize,
                               register RwUInt32 outSize, register RwInt32 outSkip)
{
    nofralloc
    andi.   r11, count, 1
    srawi   count, count, 1
    lfs     f0, 0(scale)
    rotlwi  r11, inSize, 1
    rotlwi  r12, outSize, 1
    subf    dst, r12, dst
    subf    src1, r11, src1
    subf    src2, r11, src2
    beq     even

    /* odd vertex: x,y then z */
    psq_lux f1, src1, r11, 1, 6
    psq_lux f2, src1, inSize, 0, 6
    psq_lux f3, src2, r11, 1, 6
    psq_lux f4, src2, inSize, 0, 6
    ps_sub  f3, f3, f1
    ps_sub  f4, f4, f2
    ps_madd f1, f3, f0, f1
    ps_madd f2, f4, f0, f2
    psq_stux f1, dst, r12, 1, 6
    psq_stux f2, dst, outSize, 0, 6
    add     dst, dst, outSkip

even:
    cmpwi   count, 0
    mtctr   count
    blelr

    psq_lux f1, src1, r11, 0, 6
    psq_lux f2, src1, r11, 0, 6
    psq_lux f3, src1, r11, 0, 6
    psq_lux f4, src2, r11, 0, 6
    psq_lux f5, src2, r11, 0, 6
    psq_lux f6, src2, r11, 0, 6
    ps_sub  f4, f4, f1
    ps_sub  f5, f5, f2
    ps_sub  f6, f6, f3

loop:
    ps_madd f7, f4, f0, f1
    psq_lux f1, src1, r11, 0, 6
    psq_lux f4, src2, r11, 0, 6
    psq_stux f7, dst, r12, 0, 6
    ps_madd f8, f5, f0, f2
    psq_lux f2, src1, r11, 0, 6
    psq_lux f5, src2, r11, 0, 6
    ps_madd f9, f6, f0, f3
    psq_lux f3, src1, r11, 0, 6
    psq_lux f6, src2, r11, 0, 6
    psq_stux f8, dst, r12, 1, 6
    add     dst, dst, outSkip
    ps_merge10 f8, f8, f8
    psq_stux f8, dst, outSize, 1, 6
    psq_stux f9, dst, outSize, 0, 6
    add     dst, dst, outSkip
    ps_sub  f4, f4, f1
    ps_sub  f5, f5, f2
    ps_sub  f6, f6, f3
    bdnz    loop
    blr
}
/* clang-format on */
