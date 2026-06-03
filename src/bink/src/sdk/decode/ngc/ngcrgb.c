#include "bink.h"
#include "ngcrgb.h"

// GameCube texture memory is tiled in four-row groups. These helpers convert
// the linear Bink row pointers in S into the swizzled destination addresses
// used by the RGB and alpha 4x2 core kernels.
typedef enum RGBTileLayout
{
    RGB_TILE_ROWS = 4,
    RGB_TILE_ROW_BITS = 2,
    RGB_TILE_ROW_MASK = RGB_TILE_ROWS - 1,
    RGB_TILE_ROW_SHIFT = 3,
    RGB_16BIT_TILE_ALIGN_MASK = 0x1f,
    RGB_32BIT_TILE_ALIGN_MASK = 0x3f,
    RGB_BYTES_PER_PIXEL32 = 4,
    RGB_16_4X2_ROW_BYTES = 8,
    RGB_16_X2_4X2_ROW_BYTES = 16,
    RGB_32_4X2_ROW_BYTES = 16,
    RGB_32_X2_4X2_ROW_BYTES = 32,
    RGB_TILE_HALF_BLOCK_WORDS = 8,
    RGB_TILE_BLOCK_WORDS = 16,
    RGB_TILE_X2_BLOCK_WORDS = 32,
    RGB_TILE_WORD0 = 0,
    RGB_TILE_WORD1 = 1,
    RGB_TILE_NEXT_ROW_WORD0 = RGB_TILE_HALF_BLOCK_WORDS,
    RGB_TILE_NEXT_ROW_WORD1 = RGB_TILE_HALF_BLOCK_WORDS + 1,
    RGB_TILE_SECOND_BLOCK_WORD0 = RGB_TILE_BLOCK_WORDS,
    RGB_TILE_SECOND_BLOCK_WORD1 = RGB_TILE_BLOCK_WORDS + 1,
    RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0 = RGB_TILE_BLOCK_WORDS + RGB_TILE_HALF_BLOCK_WORDS,
    RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1 = RGB_TILE_BLOCK_WORDS + RGB_TILE_HALF_BLOCK_WORDS + 1
} RGBTileLayout;

#define RGB_TILE_PITCH16(pitch) (((pitch) * RGB_BYTES_PER_PIXEL32 + RGB_16BIT_TILE_ALIGN_MASK) & ~RGB_16BIT_TILE_ALIGN_MASK)
#define RGB_TILE_PITCH32(pitch) (((pitch) * RGB_BYTES_PER_PIXEL32 + RGB_32BIT_TILE_ALIGN_MASK) & ~RGB_32BIT_TILE_ALIGN_MASK)
#define RGB_TILE_ROW(ptr, base, pitch) ((s32)((u8 PTR4*)(ptr) - (base)) / (s32)(pitch))
#define RGB_TILE_ROW_START(base, row, pitch) ((base) + (row) * (pitch))
#define RGB_TILE_LOC(base, ptr, pitch, tilePitch, row)                                                                 \
    ((base) + (tilePitch) * ((u32)(row) >> RGB_TILE_ROW_BITS) +                                                       \
     (((u32)(row) & RGB_TILE_ROW_MASK) << RGB_TILE_ROW_SHIFT) +                                                       \
     (((u8 PTR4*)(ptr) - RGB_TILE_ROW_START((base), (row), (pitch))) << RGB_TILE_ROW_BITS))
typedef enum RGBWordMasks
{
    RGB_BYTE_MASK = 0xff,
    RGB_WORD_LO_MASK = 0x0000ffff,
    RGB_WORD_HI_MASK = 0xffff0000,
    RGB_ALPHA0_MASK = 0xff000000,
    RGB_ALPHA2_MASK = 0x0000ff00
} RGBWordMasks;

#define RGB_WORD_BYTE3(word) (((word) >> 24) & RGB_BYTE_MASK)
#define RGB_WORD_BYTE2(word) (((word) >> 16) & RGB_BYTE_MASK)
#define RGB_WORD_BYTE1(word) (((word) >> 8) & RGB_BYTE_MASK)
#define RGB_WORD_BYTE0(word) ((word) & RGB_BYTE_MASK)
#define RGB32_PAIR_HIGH(pixel) (((pixel) & RGB_WORD_HI_MASK) | ((pixel) >> 16))
#define RGB32_PAIR_LOW(pixel) (((pixel) << 16) | ((pixel) & RGB_WORD_LO_MASK))
#define RGB32_PAIR_HIGH2(left, right) (((left) & RGB_WORD_HI_MASK) | ((right) >> 16))
#define RGB32_PAIR_LOW2(left, right) (((left) << 16) | ((right) & RGB_WORD_LO_MASK))
#define RGB32_COLOR_RED_PAIR(left, right, red) (((left)[red] << 16) | (right)[red])
#define RGB32_COLOR_GB_PAIR(left, right, gb, blue) \
    (((left)[gb] << 24) | ((left)[blue] << 16) | ((right)[gb] << 8) | (right)[blue])
#define RGB32_COLOR_RED_DUP(row, red) (((row)[red] << 16) | (row)[red])
#define RGB32_COLOR_GB_DUP(row, gb, blue) \
    (((row)[gb] << 24) | ((row)[blue] << 16) | ((row)[gb] << 8) | (row)[blue])
#define RGB32_ALPHA_PAIR_HIGH(alpha, left, right) \
    (((alpha) & RGB_ALPHA0_MASK) | ((left) & RGB_WORD_HI_MASK) | ((right) >> 16) | \
     (((alpha) >> 8) & RGB_ALPHA2_MASK))
#define RGB32_ALPHA_PAIR_LOW(alpha, left, right) \
    ((((alpha) & RGB_ALPHA2_MASK) << 16) | ((left) & RGB_WORD_HI_MASK) | ((right) >> 16) | \
     (RGB_WORD_BYTE0(alpha) << 8))
#define RGB32_ALPHA_COLOR_PAIR_HIGH(alpha, left, right) \
    (((alpha) & RGB_ALPHA0_MASK) | ((left) << 16) | (((alpha) >> 8) & RGB_ALPHA2_MASK) | (right))
#define RGB32_ALPHA_COLOR_PAIR_LOW(alpha, left, right) \
    ((((alpha) & RGB_ALPHA2_MASK) | (left)) << 16 | (RGB_WORD_BYTE0(alpha) << 8) | (right))
#define RGB32_ALPHA_COLOR_DUP_PAIR(alpha_hi, alpha_lo, value) \
    ((alpha_hi) | ((value) << 16) | (alpha_lo) | (value))
#define RGB32_ALPHA_DUP_PAIR(alpha_hi, alpha_lo, pixel) \
    ((alpha_hi) | ((pixel) & RGB_WORD_HI_MASK) | ((pixel) >> 16) | (alpha_lo))
#define RGB32_MONO_DUP_PAIR(pixel) (((pixel) << 16) | ((pixel) & RGB_WORD_LO_MASK))

typedef enum RGBClampLayout
{
    RGB_CLAMP_BIAS = 0x100
} RGBClampLayout;

#define RGB565(y, r, g, b)                                                                                             \
    ((u16)clamp_b[RGB_CLAMP_BIAS + (y) + (b)] | (u16)clamp_r[RGB_CLAMP_BIAS + (y) + (r)] |                   \
     (u16)clamp_g[RGB_CLAMP_BIAS + (y) + (g)])

#define RGB565_BIASED(cr, cg, cb, y, r, g, b)                                                                          \
    ((u16)(cb)[(y) + (r)] | (u16)(cr)[(y) + (b)] | (u16)(cg)[(y) + (g)])
#define RGB565_PAIR(pixel) (((pixel) << 16) | (pixel))
#define RGB565_A4(y, r, g, b, a) (RGB565((y), (r), (g), (b)) | (u16)clamp_a4[(a)])
#define RGB565_A4_MONO(y, a) ((u16)clamp_a4[(a)] | (u16)mono16[(y)])
#define RGB565_A4_BIASED(cr, cg, cb, ca, y, r, g, b, a)                                                               \
    ((u16)(cb)[(y) + (r)] | (u16)(cr)[(y) + (b)] | (u16)(cg)[(y) + (g)] | (u16)(ca)[(a)])
#define RGB32_M(y) (mono32[(y)])
#define RGB565_PAIR2(left, right) (((left) << 16) | (right))

// Core kernels consume two luma rows and one chroma row, producing a 4x2 tile
// chunk. The monochrome paths use mono16/mono32 directly, while color paths
// bias through the YUV contribution tables prepared by YUV_init.
void YUV_32_4x2_even(u32 count)
{
    u16 vword;
    u16 uword;
    u32 yv0;
    u32 yv1;
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y1;
    u32 PTR4* y0;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    s32 PTR4* u_to_b;
    s32 PTR4* v_to_gb;
    s32 PTR4* u_to_gb;
    s32 PTR4* v_to_r;

    dest0 = (u32 PTR4*)S.dest0;
    base = S.base;
    dest1 = (u32 PTR4*)S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_4X2_ROW_BYTES;

    row1 = RGB_TILE_ROW(dest1, base, pitch);
    row0 = RGB_TILE_ROW(dest0, base, pitch);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    y1 = S.y1;
    y0 = S.y0;
    u = S.u;
    v = S.v;
    u_to_b = YUVTables.u_to_b;
    v_to_r = YUVTables.v_to_r;
    u_to_gb = YUVTables.u_to_gb;
    v_to_gb = YUVTables.v_to_gb;

    do {
        u32 vhi;
        u32 uhi;
        u32 vlo;
        u32 ulo;
        u32 PTR4* y00;
        u32 PTR4* y01;
        u32 PTR4* y10;
        u32 PTR4* y11;
        s32 r;
        s32 b;
        s32 gb;

        vword = *v++;
        yv0 = *y0++;
        vhi = RGB_WORD_BYTE1(vword);
        uword = *u++;
        uhi = RGB_WORD_BYTE1(uword);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv0)];
        gb = v_to_gb[vhi] + u_to_gb[uhi];
        r = v_to_r[vhi];
        b = u_to_b[uhi];
        yv1 = *y1++;
        dest0[RGB_TILE_WORD0] = RGB32_COLOR_RED_PAIR(y00, y01, r);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);

        y10 = clamp_ytable[RGB_WORD_BYTE3(yv1)];
        y11 = clamp_ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB32_COLOR_RED_PAIR(y10, y11, r);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_PAIR(y10, y11, gb, b);

        vlo = RGB_WORD_BYTE0(vword);
        ulo = RGB_WORD_BYTE0(uword);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv0)];
        gb = v_to_gb[vlo] + u_to_gb[ulo];
        r = v_to_r[vlo];
        b = u_to_b[ulo];
        dest0[RGB_TILE_WORD1] = RGB32_COLOR_RED_PAIR(y00, y01, r);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);
        dest0 += RGB_TILE_BLOCK_WORDS;

        y10 = clamp_ytable[RGB_WORD_BYTE1(yv1)];
        y11 = clamp_ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_WORD1] = RGB32_COLOR_RED_PAIR(y10, y11, r);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_PAIR(y10, y11, gb, b);
        dest1 += RGB_TILE_BLOCK_WORDS;

        count--;
    } while (count != 0);

    S.u = u;
    S.v = v;
    S.y0 = y0;
    S.y1 = y1;
}
void YUV_32x2_4x2_even(u32 count)
{
    u16 vword;
    u16 uword;
    u32 yv0;
    u32 yv1;
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y1;
    u32 PTR4* y0;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    s32 PTR4* u_to_b;
    s32 PTR4* v_to_gb;
    s32 PTR4* u_to_gb;
    s32 PTR4* v_to_r;

    dest0 = (u32 PTR4*)S.dest0;
    base = S.base;
    dest1 = (u32 PTR4*)S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_X2_4X2_ROW_BYTES;

    row1 = RGB_TILE_ROW(dest1, base, pitch);
    row0 = RGB_TILE_ROW(dest0, base, pitch);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    y1 = S.y1;
    y0 = S.y0;
    u = S.u;
    v = S.v;
    u_to_b = YUVTables.u_to_b;
    v_to_r = YUVTables.v_to_r;
    u_to_gb = YUVTables.u_to_gb;
    v_to_gb = YUVTables.v_to_gb;

    do {
        u32 vhi;
        u32 uhi;
        u32 vlo;
        u32 ulo;
        u32 PTR4* y00;
        u32 PTR4* y01;
        u32 PTR4* y10;
        u32 PTR4* y11;
        s32 r;
        s32 b;
        s32 gb;

        vword = *v++;
        yv0 = *y0++;
        uword = *u++;
        vhi = RGB_WORD_BYTE1(vword);
        uhi = RGB_WORD_BYTE1(uword);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv0)];
        r = v_to_r[vhi];
        gb = v_to_gb[vhi] + u_to_gb[uhi];
        b = u_to_b[uhi];
        yv1 = *y1++;

        dest0[RGB_TILE_WORD0] = RGB32_COLOR_RED_DUP(y00, r);
        dest0[RGB_TILE_WORD1] = RGB32_COLOR_RED_DUP(y01, r);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);

        y10 = clamp_ytable[RGB_WORD_BYTE3(yv1)];
        y11 = clamp_ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB32_COLOR_RED_DUP(y10, r);
        dest1[RGB_TILE_WORD1] = RGB32_COLOR_RED_DUP(y11, r);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y10, gb, b);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y11, gb, b);

        vlo = RGB_WORD_BYTE0(vword);
        ulo = RGB_WORD_BYTE0(uword);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv0)];
        r = v_to_r[vlo];
        gb = v_to_gb[vlo] + u_to_gb[ulo];
        b = u_to_b[ulo];
        dest0[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_COLOR_RED_DUP(y00, r);
        dest0[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_COLOR_RED_DUP(y01, r);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);
        dest0 += RGB_TILE_X2_BLOCK_WORDS;

        y10 = clamp_ytable[RGB_WORD_BYTE1(yv1)];
        y11 = clamp_ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_COLOR_RED_DUP(y10, r);
        dest1[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_COLOR_RED_DUP(y11, r);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y10, gb, b);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y11, gb, b);
        dest1 += RGB_TILE_X2_BLOCK_WORDS;

        count--;
    } while (count != 0);

    S.u = u;
    S.v = v;
    S.y0 = y0;
    S.y1 = y1;
}

void YUV_32m_4x2(u32 count)
{
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    dest0 = (u32 PTR4*)S.dest0;
    base = S.base;
    dest1 = (u32 PTR4*)S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    y0 = S.y0;
    y1 = S.y1;

    do {
        u32 yv0 = *y0++;
        u32 yv1 = *y1++;
        u32 a = RGB32_M(RGB_WORD_BYTE3(yv0));
        u32 b = RGB32_M(RGB_WORD_BYTE2(yv0));
        u32 c = RGB32_M(RGB_WORD_BYTE1(yv0));
        u32 d = RGB32_M(RGB_WORD_BYTE0(yv0));

        dest0[RGB_TILE_WORD0] = RGB32_PAIR_HIGH2(a, b);
        dest0[RGB_TILE_WORD1] = RGB32_PAIR_HIGH2(c, d);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_PAIR_LOW2(a, b);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_PAIR_LOW2(c, d);

        a = RGB32_M(RGB_WORD_BYTE3(yv1));
        b = RGB32_M(RGB_WORD_BYTE2(yv1));
        c = RGB32_M(RGB_WORD_BYTE1(yv1));
        d = RGB32_M(RGB_WORD_BYTE0(yv1));
        dest1[RGB_TILE_WORD0] = RGB32_PAIR_HIGH2(a, b);
        dest1[RGB_TILE_WORD1] = RGB32_PAIR_HIGH2(c, d);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_PAIR_LOW2(a, b);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_PAIR_LOW2(c, d);

        dest0 += RGB_TILE_BLOCK_WORDS;
        dest1 += RGB_TILE_BLOCK_WORDS;
        --count;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
}

void YUV_32mx2_4x2(u32 count)
{
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    dest0 = (u32 PTR4*)S.dest0;
    base = S.base;
    dest1 = (u32 PTR4*)S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    y0 = S.y0;
    y1 = S.y1;

    do {
        u32 yv0 = *y0++;
        u32 yv1 = *y1++;
        u32 a = RGB32_M(RGB_WORD_BYTE3(yv0));
        u32 b = RGB32_M(RGB_WORD_BYTE2(yv0));
        u32 c = RGB32_M(RGB_WORD_BYTE1(yv0));
        u32 d = RGB32_M(RGB_WORD_BYTE0(yv0));

        dest0[RGB_TILE_WORD0] = RGB32_PAIR_HIGH(a);
        dest0[RGB_TILE_WORD1] = RGB32_PAIR_HIGH(b);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_PAIR_LOW(a);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_PAIR_LOW(b);
        dest0[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_PAIR_HIGH(c);
        dest0[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_PAIR_HIGH(d);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_PAIR_LOW(c);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_PAIR_LOW(d);

        a = RGB32_M(RGB_WORD_BYTE3(yv1));
        b = RGB32_M(RGB_WORD_BYTE2(yv1));
        c = RGB32_M(RGB_WORD_BYTE1(yv1));
        d = RGB32_M(RGB_WORD_BYTE0(yv1));
        dest1[RGB_TILE_WORD0] = RGB32_PAIR_HIGH(a);
        dest1[RGB_TILE_WORD1] = RGB32_PAIR_HIGH(b);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_PAIR_LOW(a);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_PAIR_LOW(b);
        dest1[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_PAIR_HIGH(c);
        dest1[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_PAIR_HIGH(d);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_PAIR_LOW(c);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_PAIR_LOW(d);

        dest0 += RGB_TILE_X2_BLOCK_WORDS;
        dest1 += RGB_TILE_X2_BLOCK_WORDS;
        --count;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
}

void YUV_16_4x2_even(u32 count)
{
    u16 uword;
    u16 vword;
    u32 yv0;
    u32 yv1;
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    s32 PTR4* u_to_b;
    s32 PTR4* v_to_gb;
    s32 PTR4* u_to_gb;
    s32 PTR4* v_to_r;
    u32 PTR4* clamp_r_base;
    u32 PTR4* clamp_g_base;
    u32 PTR4* clamp_b_base;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    u_to_b = YUVTables.u_to_b;
    v_to_gb = YUVTables.v_to_gb;
    u_to_gb = YUVTables.u_to_gb;
    v_to_r = YUVTables.v_to_r;
    clamp_r_base = clamp_r + RGB_CLAMP_BIAS;
    clamp_g_base = clamp_g + RGB_CLAMP_BIAS;
    clamp_b_base = clamp_b + RGB_CLAMP_BIAS;

    S.dest0 += count * RGB_16_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 uhi;
        u32 vhi;
        u32 ulo;
        u32 vlo;
        s32 rb;
        s32 gb;
        s32 bb;
        s32 ya;
        s32 yb;

        uword = *u++;
        vword = *v++;
        yv0 = *y0++;
        uhi = RGB_WORD_BYTE1(uword);
        vhi = RGB_WORD_BYTE1(vword);
        rb = u_to_b[uhi];
        gb = u_to_gb[uhi] + v_to_gb[vhi];
        bb = v_to_r[vhi];
        ya = ytable[RGB_WORD_BYTE3(yv0)];
        yb = ytable[RGB_WORD_BYTE2(yv0)];
        yv1 = *y1++;
        dest0[RGB_TILE_WORD0] = RGB565_PAIR2(RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, ya, rb, gb, bb),
                                             RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, yb, rb, gb, bb));

        ya = ytable[RGB_WORD_BYTE3(yv1)];
        yb = ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB565_PAIR2(RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, ya, rb, gb, bb),
                                             RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, yb, rb, gb, bb));

        ulo = RGB_WORD_BYTE0(uword);
        vlo = RGB_WORD_BYTE0(vword);
        rb = u_to_b[ulo];
        gb = u_to_gb[ulo] + v_to_gb[vlo];
        bb = v_to_r[vlo];
        ya = ytable[RGB_WORD_BYTE1(yv0)];
        yb = ytable[RGB_WORD_BYTE0(yv0)];
        dest0[RGB_TILE_WORD1] = RGB565_PAIR2(RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, ya, rb, gb, bb),
                                             RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, yb, rb, gb, bb));
        dest0 += RGB_TILE_HALF_BLOCK_WORDS;

        ya = ytable[RGB_WORD_BYTE1(yv1)];
        yb = ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_WORD1] = RGB565_PAIR2(RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, ya, rb, gb, bb),
                                             RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, yb, rb, gb, bb));
        dest1 += RGB_TILE_HALF_BLOCK_WORDS;

        count--;
    } while (count != 0);

    S.u = u;
    S.v = v;
    S.y0 = y0;
    S.y1 = y1;
}

void YUV_16x2_4x2_even(u32 count)
{
    u16 uword;
    u16 vword;
    u32 yv0;
    u32 yv1;
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    s32 PTR4* u_to_b;
    s32 PTR4* v_to_gb;
    s32 PTR4* u_to_gb;
    s32 PTR4* v_to_r;
    u32 PTR4* clamp_r_base;
    u32 PTR4* clamp_g_base;
    u32 PTR4* clamp_b_base;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    u_to_b = YUVTables.u_to_b;
    v_to_gb = YUVTables.v_to_gb;
    u_to_gb = YUVTables.u_to_gb;
    v_to_r = YUVTables.v_to_r;
    clamp_r_base = clamp_r + RGB_CLAMP_BIAS;
    clamp_g_base = clamp_g + RGB_CLAMP_BIAS;
    clamp_b_base = clamp_b + RGB_CLAMP_BIAS;

    S.dest0 += count * RGB_16_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 uhi;
        u32 vhi;
        u32 ulo;
        u32 vlo;
        u32 pix;
        s32 rb;
        s32 gb;
        s32 bb;
        s32 ya;
        s32 yb;

        uword = *u++;
        vword = *v++;
        yv0 = *y0++;
        uhi = RGB_WORD_BYTE1(uword);
        vhi = RGB_WORD_BYTE1(vword);
        rb = u_to_b[uhi];
        gb = u_to_gb[uhi] + v_to_gb[vhi];
        bb = v_to_r[vhi];
        ya = ytable[RGB_WORD_BYTE3(yv0)];
        yb = ytable[RGB_WORD_BYTE2(yv0)];
        yv1 = *y1++;
        pix = RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, ya, rb, gb, bb);
        dest0[RGB_TILE_WORD0] = RGB565_PAIR(pix);
        pix = RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, yb, rb, gb, bb);
        dest0[RGB_TILE_WORD1] = RGB565_PAIR(pix);

        ya = ytable[RGB_WORD_BYTE3(yv1)];
        yb = ytable[RGB_WORD_BYTE2(yv1)];
        pix = RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, ya, rb, gb, bb);
        dest1[RGB_TILE_WORD0] = RGB565_PAIR(pix);
        pix = RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, yb, rb, gb, bb);
        dest1[RGB_TILE_WORD1] = RGB565_PAIR(pix);

        ulo = RGB_WORD_BYTE0(uword);
        vlo = RGB_WORD_BYTE0(vword);
        rb = u_to_b[ulo];
        gb = u_to_gb[ulo] + v_to_gb[vlo];
        bb = v_to_r[vlo];
        ya = ytable[RGB_WORD_BYTE1(yv0)];
        yb = ytable[RGB_WORD_BYTE0(yv0)];
        pix = RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, ya, rb, gb, bb);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(pix);
        pix = RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, yb, rb, gb, bb);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(pix);
        dest0 += RGB_TILE_BLOCK_WORDS;

        ya = ytable[RGB_WORD_BYTE1(yv1)];
        yb = ytable[RGB_WORD_BYTE0(yv1)];
        pix = RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, ya, rb, gb, bb);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(pix);
        pix = RGB565_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, yb, rb, gb, bb);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(pix);
        dest1 += RGB_TILE_BLOCK_WORDS;

        count--;
    } while (count != 0);

    S.u = u;
    S.v = v;
    S.y0 = y0;
    S.y1 = y1;
}

void YUV_16m_4x2(u32 count)
{
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    u32 PTR4* table;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    table = mono16;

    S.dest0 += count * RGB_16_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 yv0 = *y0++;
        u32 yv1 = *y1++;
        u32 y0hi = yv0 >> 16;
        u32 y1hi = yv1 >> 16;
        u32 y0lo = yv0 & RGB_WORD_LO_MASK;
        u32 y1lo = yv1 & RGB_WORD_LO_MASK;
        u16 a = (u16)table[RGB_WORD_BYTE1(y0hi)];
        u16 b = (u16)table[RGB_WORD_BYTE0(y0hi)];
        u16 c = (u16)table[RGB_WORD_BYTE1(y0lo)];
        u16 d = (u16)table[RGB_WORD_BYTE0(y0lo)];

        dest0[RGB_TILE_WORD0] = RGB565_PAIR2(a, b);

        a = (u16)table[RGB_WORD_BYTE1(y1hi)];
        b = (u16)table[RGB_WORD_BYTE0(y1hi)];
        dest1[RGB_TILE_WORD0] = RGB565_PAIR2(a, b);

        dest0[RGB_TILE_WORD1] = RGB565_PAIR2(c, d);

        c = (u16)table[RGB_WORD_BYTE1(y1lo)];
        d = (u16)table[RGB_WORD_BYTE0(y1lo)];
        dest1[RGB_TILE_WORD1] = RGB565_PAIR2(c, d);

        dest0 += RGB_TILE_HALF_BLOCK_WORDS;
        dest1 += RGB_TILE_HALF_BLOCK_WORDS;
        --count;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
}

void YUV_16mx2_4x2(u32 count)
{
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    u32 PTR4* table;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    table = mono16;

    S.dest0 += count * RGB_16_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 yv0 = *y0++;
        u32 yv1 = *y1++;
        u32 y0hi = yv0 >> 16;
        u32 y1hi = yv1 >> 16;
        u32 y0lo = yv0 & RGB_WORD_LO_MASK;
        u32 y1lo = yv1 & RGB_WORD_LO_MASK;
        u16 a = (u16)table[RGB_WORD_BYTE1(y0hi)];
        u16 b = (u16)table[RGB_WORD_BYTE0(y0hi)];
        u16 c = (u16)table[RGB_WORD_BYTE1(y0lo)];
        u16 d = (u16)table[RGB_WORD_BYTE0(y0lo)];

        dest0[RGB_TILE_WORD0] = RGB565_PAIR(a);
        dest0[RGB_TILE_WORD1] = RGB565_PAIR(b);

        a = (u16)table[RGB_WORD_BYTE1(y1hi)];
        b = (u16)table[RGB_WORD_BYTE0(y1hi)];
        dest1[RGB_TILE_WORD0] = RGB565_PAIR(a);
        dest1[RGB_TILE_WORD1] = RGB565_PAIR(b);

        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(c);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(d);

        c = (u16)table[RGB_WORD_BYTE1(y1lo)];
        d = (u16)table[RGB_WORD_BYTE0(y1lo)];
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(c);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(d);

        dest0 += RGB_TILE_BLOCK_WORDS;
        dest1 += RGB_TILE_BLOCK_WORDS;
        --count;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
}

void YUV_32a_4x2_even(u32 count)
{
    u16 vword;
    u16 uword;
    u32 yv0;
    u32 yv1;
    u32 av0;
    u32 av1;
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y1;
    u32 PTR4* y0;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    s32 PTR4* u_to_b;
    s32 PTR4* v_to_gb;
    s32 PTR4* u_to_gb;
    s32 PTR4* v_to_r;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    y1 = S.y1;
    y0 = S.y0;
    u = S.u;
    v = S.v;
    a0 = S.a0;
    a1 = S.a1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH32(pitch);
    u_to_b = YUVTables.u_to_b;
    v_to_gb = YUVTables.v_to_gb;
    u_to_gb = YUVTables.u_to_gb;
    v_to_r = YUVTables.v_to_r;

    S.dest0 += count * RGB_32_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 vhi;
        u32 uhi;
        u32 vlo;
        u32 ulo;
        u32 PTR4* y00;
        u32 PTR4* y01;
        u32 PTR4* y10;
        u32 PTR4* y11;
        s32 r;
        s32 b;
        s32 gb;

        yv0 = *y0++;
        vword = *v++;
        av0 = *a0++;
        uword = *u++;
        av1 = *a1++;
        yv1 = *y1++;
        vhi = RGB_WORD_BYTE1(vword);
        uhi = RGB_WORD_BYTE1(uword);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv0)];
        r = v_to_r[vhi];
        gb = v_to_gb[vhi] + u_to_gb[uhi];
        b = u_to_b[uhi];
        dest0[RGB_TILE_WORD0] = RGB32_ALPHA_COLOR_PAIR_HIGH(av0, y00[r], y01[r]);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);

        y10 = clamp_ytable[RGB_WORD_BYTE3(yv1)];
        y11 = clamp_ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB32_ALPHA_COLOR_PAIR_HIGH(av1, y10[r], y11[r]);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_PAIR(y10, y11, gb, b);

        vlo = RGB_WORD_BYTE0(vword);
        ulo = RGB_WORD_BYTE0(uword);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv0)];
        r = v_to_r[vlo];
        gb = v_to_gb[vlo] + u_to_gb[ulo];
        b = u_to_b[ulo];
        dest0[RGB_TILE_WORD1] = RGB32_ALPHA_COLOR_PAIR_LOW(av0, y00[r], y01[r]);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);
        dest0 += RGB_TILE_BLOCK_WORDS;

        y10 = clamp_ytable[RGB_WORD_BYTE1(yv1)];
        y11 = clamp_ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_WORD1] = RGB32_ALPHA_COLOR_PAIR_LOW(av1, y10[r], y11[r]);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_PAIR(y10, y11, gb, b);
        dest1 += RGB_TILE_BLOCK_WORDS;

        count--;
    } while (count != 0);

    S.v = v;
    S.u = u;
    S.y1 = y1;
    S.y0 = y0;
    S.a0 = a0;
    S.a1 = a1;
}

void YUV_32ax2_4x2_even(u32 count)
{
    u16 vword;
    u16 uword;
    u32 yv0;
    u32 yv1;
    u32 av0;
    u32 av1;
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y1;
    u32 PTR4* y0;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    s32 PTR4* u_to_b;
    s32 PTR4* v_to_gb;
    s32 PTR4* u_to_gb;
    s32 PTR4* v_to_r;

    dest0 = (u32 PTR4*)S.dest0;
    base = S.base;
    dest1 = (u32 PTR4*)S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);
    y1 = S.y1;
    y0 = S.y0;
    u = S.u;
    v = S.v;
    a0 = S.a0;
    a1 = S.a1;
    u_to_b = YUVTables.u_to_b;
    v_to_gb = YUVTables.v_to_gb;
    u_to_gb = YUVTables.u_to_gb;
    v_to_r = YUVTables.v_to_r;

    do {
        u32 vhi;
        u32 uhi;
        u32 vlo;
        u32 ulo;
        u32 PTR4* y00;
        u32 PTR4* y01;
        u32 PTR4* y10;
        u32 PTR4* y11;
        u32 alpha0;
        u32 alpha1;
        s32 r;
        s32 b;
        s32 gb;

        vword = *v++;
        yv0 = *y0++;
        uword = *u++;
        av0 = *a0++;
        av1 = *a1++;
        yv1 = *y1++;
        vhi = RGB_WORD_BYTE1(vword);
        uhi = RGB_WORD_BYTE1(uword);
        alpha0 = av0 & RGB_ALPHA0_MASK;
        alpha1 = RGB_WORD_BYTE2(av0);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv0)];
        r = v_to_r[vhi];
        gb = v_to_gb[vhi] + u_to_gb[uhi];
        b = u_to_b[uhi];

        dest0[RGB_TILE_WORD0] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha0, RGB_WORD_BYTE3(av0) << 8, y00[r]);
        dest0[RGB_TILE_WORD1] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha1 << 24, alpha1 << 8, y01[r]);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);

        alpha0 = av1 & RGB_ALPHA0_MASK;
        alpha1 = RGB_WORD_BYTE2(av1);
        y10 = clamp_ytable[RGB_WORD_BYTE3(yv1)];
        y11 = clamp_ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha0, RGB_WORD_BYTE3(av1) << 8, y10[r]);
        dest1[RGB_TILE_WORD1] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha1 << 24, alpha1 << 8, y11[r]);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y10, gb, b);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y11, gb, b);

        vlo = RGB_WORD_BYTE0(vword);
        ulo = RGB_WORD_BYTE0(uword);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv0)];
        r = v_to_r[vlo];
        gb = v_to_gb[vlo] + u_to_gb[ulo];
        b = u_to_b[ulo];

        alpha0 = RGB_WORD_BYTE1(av0);
        alpha1 = RGB_WORD_BYTE0(av0);
        dest0[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha0 << 24, alpha0 << 8, y00[r]);
        dest0[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha1 << 24, alpha1 << 8, y01[r]);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);
        dest0 += RGB_TILE_X2_BLOCK_WORDS;

        alpha0 = RGB_WORD_BYTE1(av1);
        alpha1 = RGB_WORD_BYTE0(av1);
        y10 = clamp_ytable[RGB_WORD_BYTE1(yv1)];
        y11 = clamp_ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha0 << 24, alpha0 << 8, y10[r]);
        dest1[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha1 << 24, alpha1 << 8, y11[r]);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y10, gb, b);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y11, gb, b);
        dest1 += RGB_TILE_X2_BLOCK_WORDS;

        count--;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
    S.u = u;
    S.v = v;
    S.a0 = a0;
    S.a1 = a1;
}

void YUV_32am_4x2(u32 count)
{
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    dest0 = (u32 PTR4*)S.dest0;
    base = S.base;
    dest1 = (u32 PTR4*)S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    a0 = S.a0;
    a1 = S.a1;
    y0 = S.y0;
    y1 = S.y1;

    do {
        u32 yv0 = *y0++;
        u32 av0 = *a0++;
        u32 yv1 = *y1++;
        u32 av1 = *a1++;
        u32 p0 = RGB32_M(RGB_WORD_BYTE3(yv0));
        u32 p1 = RGB32_M(RGB_WORD_BYTE2(yv0));
        u32 p2 = RGB32_M(RGB_WORD_BYTE1(yv0));
        u32 p3 = RGB32_M(RGB_WORD_BYTE0(yv0));

        dest0[RGB_TILE_WORD0] = RGB32_ALPHA_PAIR_HIGH(av0, p0, p1);
        dest0[RGB_TILE_WORD1] = RGB32_ALPHA_PAIR_LOW(av0, p2, p3);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_PAIR_LOW2(p0, p1);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_PAIR_LOW2(p2, p3);

        p0 = RGB32_M(RGB_WORD_BYTE3(yv1));
        p1 = RGB32_M(RGB_WORD_BYTE2(yv1));
        p2 = RGB32_M(RGB_WORD_BYTE1(yv1));
        p3 = RGB32_M(RGB_WORD_BYTE0(yv1));
        dest1[RGB_TILE_WORD0] = RGB32_ALPHA_PAIR_HIGH(av1, p0, p1);
        dest1[RGB_TILE_WORD1] = RGB32_ALPHA_PAIR_LOW(av1, p2, p3);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_PAIR_LOW2(p0, p1);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_PAIR_LOW2(p2, p3);

        dest0 += RGB_TILE_BLOCK_WORDS;
        dest1 += RGB_TILE_BLOCK_WORDS;
        --count;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
    S.a0 = a0;
    S.a1 = a1;
}

void YUV_32amx2_4x2(u32 count)
{
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    dest0 = (u32 PTR4*)S.dest0;
    base = S.base;
    dest1 = (u32 PTR4*)S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    a0 = S.a0;
    a1 = S.a1;
    y0 = S.y0;
    y1 = S.y1;

    do {
        u32 yv0 = *y0++;
        u32 av0 = *a0++;
        u32 yv1 = *y1++;
        u32 av1 = *a1++;
        u32 p0 = RGB32_M(RGB_WORD_BYTE3(yv0));
        u32 p1 = RGB32_M(RGB_WORD_BYTE2(yv0));
        u32 p2 = RGB32_M(RGB_WORD_BYTE1(yv0));
        u32 p3 = RGB32_M(RGB_WORD_BYTE0(yv0));
        u32 a_hi0 = av0 & RGB_ALPHA0_MASK;
        u32 a_hi1 = RGB_WORD_BYTE2(av0) << 24;
        u32 a_lo0 = RGB_WORD_BYTE3(av0) << 8;
        u32 a_lo1 = RGB_WORD_BYTE2(av0) << 8;

        dest0[RGB_TILE_WORD0] = RGB32_ALPHA_DUP_PAIR(a_hi0, a_lo0, p0);
        dest0[RGB_TILE_WORD1] = RGB32_ALPHA_DUP_PAIR(a_hi1, a_lo1, p1);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_MONO_DUP_PAIR(p0);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_MONO_DUP_PAIR(p1);
        dest0[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_ALPHA_DUP_PAIR(a_hi0, a_lo0, p2);
        dest0[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_ALPHA_DUP_PAIR(a_hi1, a_lo1, p3);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_MONO_DUP_PAIR(p2);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_MONO_DUP_PAIR(p3);

        p0 = RGB32_M(RGB_WORD_BYTE3(yv1));
        p1 = RGB32_M(RGB_WORD_BYTE2(yv1));
        p2 = RGB32_M(RGB_WORD_BYTE1(yv1));
        p3 = RGB32_M(RGB_WORD_BYTE0(yv1));
        a_hi0 = av1 & RGB_ALPHA0_MASK;
        a_hi1 = RGB_WORD_BYTE2(av1) << 24;
        a_lo0 = RGB_WORD_BYTE3(av1) << 8;
        a_lo1 = RGB_WORD_BYTE2(av1) << 8;
        dest1[RGB_TILE_WORD0] = RGB32_ALPHA_DUP_PAIR(a_hi0, a_lo0, p0);
        dest1[RGB_TILE_WORD1] = RGB32_ALPHA_DUP_PAIR(a_hi1, a_lo1, p1);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_MONO_DUP_PAIR(p0);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_MONO_DUP_PAIR(p1);
        dest1[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_ALPHA_DUP_PAIR(a_hi0, a_lo0, p2);
        dest1[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_ALPHA_DUP_PAIR(a_hi1, a_lo1, p3);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_MONO_DUP_PAIR(p2);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_MONO_DUP_PAIR(p3);

        dest0 += RGB_TILE_X2_BLOCK_WORDS;
        dest1 += RGB_TILE_X2_BLOCK_WORDS;
        --count;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
    S.a0 = a0;
    S.a1 = a1;
}

void YUV_16a4_4x2_even(u32 count)
{
    u16 uword;
    u16 vword;
    u32 yv0;
    u32 yv1;
    u32 av0;
    u32 av1;
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    s32 PTR4* u_to_b;
    s32 PTR4* v_to_gb;
    s32 PTR4* u_to_gb;
    s32 PTR4* v_to_r;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    a0 = S.a0;
    a1 = S.a1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    u_to_b = YUVTables.u_to_b;
    v_to_gb = YUVTables.v_to_gb;
    u_to_gb = YUVTables.u_to_gb;
    v_to_r = YUVTables.v_to_r;

    S.dest0 += count * RGB_16_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 uhi;
        u32 vhi;
        u32 ulo;
        u32 vlo;
        s32 rb;
        s32 gb;
        s32 bb;
        s32 ya;
        s32 yb;

        uword = *u++;
        vword = *v++;
        yv0 = *y0++;
        av0 = *a0++;
        uhi = RGB_WORD_BYTE1(uword);
        vhi = RGB_WORD_BYTE1(vword);
        rb = u_to_b[uhi];
        gb = u_to_gb[uhi] + v_to_gb[vhi];
        bb = v_to_r[vhi];
        ya = ytable[RGB_WORD_BYTE3(yv0)];
        yb = ytable[RGB_WORD_BYTE2(yv0)];
        yv1 = *y1++;
        av1 = *a1++;
        dest0[RGB_TILE_WORD0] = RGB565_PAIR2(RGB565_A4(ya, rb, gb, bb, RGB_WORD_BYTE3(av0)),
                                             RGB565_A4(yb, rb, gb, bb, RGB_WORD_BYTE2(av0)));

        ya = ytable[RGB_WORD_BYTE3(yv1)];
        yb = ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB565_PAIR2(RGB565_A4(ya, rb, gb, bb, RGB_WORD_BYTE3(av1)),
                                             RGB565_A4(yb, rb, gb, bb, RGB_WORD_BYTE2(av1)));

        ulo = RGB_WORD_BYTE0(uword);
        vlo = RGB_WORD_BYTE0(vword);
        rb = u_to_b[ulo];
        gb = u_to_gb[ulo] + v_to_gb[vlo];
        bb = v_to_r[vlo];
        ya = ytable[RGB_WORD_BYTE1(yv0)];
        yb = ytable[RGB_WORD_BYTE0(yv0)];
        dest0[RGB_TILE_WORD1] = RGB565_PAIR2(RGB565_A4(ya, rb, gb, bb, RGB_WORD_BYTE1(av0)),
                                             RGB565_A4(yb, rb, gb, bb, RGB_WORD_BYTE0(av0)));
        dest0 += RGB_TILE_HALF_BLOCK_WORDS;

        ya = ytable[RGB_WORD_BYTE1(yv1)];
        yb = ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_WORD1] = RGB565_PAIR2(RGB565_A4(ya, rb, gb, bb, RGB_WORD_BYTE1(av1)),
                                             RGB565_A4(yb, rb, gb, bb, RGB_WORD_BYTE0(av1)));
        dest1 += RGB_TILE_HALF_BLOCK_WORDS;

        count--;
    } while (count != 0);

    S.u = u;
    S.v = v;
    S.y0 = y0;
    S.y1 = y1;
    S.a0 = a0;
    S.a1 = a1;
}

void YUV_16a4x2_4x2_even(u32 count)
{
    u16 uword;
    u16 vword;
    u32 yv0;
    u32 yv1;
    u32 av0;
    u32 av1;
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    s32 PTR4* u_to_b;
    s32 PTR4* v_to_gb;
    s32 PTR4* u_to_gb;
    s32 PTR4* v_to_r;
    u32 PTR4* clamp_r_base;
    u32 PTR4* clamp_g_base;
    u32 PTR4* clamp_b_base;
    u32 PTR4* clamp_a4_base;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    a0 = S.a0;
    a1 = S.a1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    u_to_b = YUVTables.u_to_b;
    v_to_gb = YUVTables.v_to_gb;
    u_to_gb = YUVTables.u_to_gb;
    v_to_r = YUVTables.v_to_r;
    clamp_r_base = clamp_r + RGB_CLAMP_BIAS;
    clamp_g_base = clamp_g + RGB_CLAMP_BIAS;
    clamp_b_base = clamp_b + RGB_CLAMP_BIAS;
    clamp_a4_base = clamp_a4;

    S.dest0 += count * RGB_16_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 uhi;
        u32 vhi;
        u32 ulo;
        u32 vlo;
        u32 pix;
        s32 rb;
        s32 gb;
        s32 bb;
        s32 ya;
        s32 yb;

        uword = *u++;
        vword = *v++;
        yv0 = *y0++;
        av0 = *a0++;
        uhi = RGB_WORD_BYTE1(uword);
        vhi = RGB_WORD_BYTE1(vword);
        rb = u_to_b[uhi];
        gb = u_to_gb[uhi] + v_to_gb[vhi];
        bb = v_to_r[vhi];
        ya = ytable[RGB_WORD_BYTE3(yv0)];
        yb = ytable[RGB_WORD_BYTE2(yv0)];
        yv1 = *y1++;
        av1 = *a1++;

        pix = RGB565_A4_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, clamp_a4_base, ya, rb, gb, bb, RGB_WORD_BYTE3(av0));
        dest0[RGB_TILE_WORD0] = RGB565_PAIR(pix);
        pix = RGB565_A4_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, clamp_a4_base, yb, rb, gb, bb, RGB_WORD_BYTE2(av0));
        dest0[RGB_TILE_WORD1] = RGB565_PAIR(pix);

        ya = ytable[RGB_WORD_BYTE3(yv1)];
        yb = ytable[RGB_WORD_BYTE2(yv1)];
        pix = RGB565_A4_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, clamp_a4_base, ya, rb, gb, bb, RGB_WORD_BYTE3(av1));
        dest1[RGB_TILE_WORD0] = RGB565_PAIR(pix);
        pix = RGB565_A4_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, clamp_a4_base, yb, rb, gb, bb, RGB_WORD_BYTE2(av1));
        dest1[RGB_TILE_WORD1] = RGB565_PAIR(pix);

        ulo = RGB_WORD_BYTE0(uword);
        vlo = RGB_WORD_BYTE0(vword);
        rb = u_to_b[ulo];
        gb = u_to_gb[ulo] + v_to_gb[vlo];
        bb = v_to_r[vlo];
        ya = ytable[RGB_WORD_BYTE1(yv0)];
        yb = ytable[RGB_WORD_BYTE0(yv0)];
        pix = RGB565_A4_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, clamp_a4_base, ya, rb, gb, bb, RGB_WORD_BYTE1(av0));
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(pix);
        pix = RGB565_A4_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, clamp_a4_base, yb, rb, gb, bb, RGB_WORD_BYTE0(av0));
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(pix);
        dest0 += RGB_TILE_BLOCK_WORDS;

        ya = ytable[RGB_WORD_BYTE1(yv1)];
        yb = ytable[RGB_WORD_BYTE0(yv1)];
        pix = RGB565_A4_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, clamp_a4_base, ya, rb, gb, bb, RGB_WORD_BYTE1(av1));
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(pix);
        pix = RGB565_A4_BIASED(clamp_r_base, clamp_g_base, clamp_b_base, clamp_a4_base, yb, rb, gb, bb, RGB_WORD_BYTE0(av1));
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(pix);
        dest1 += RGB_TILE_BLOCK_WORDS;

        count--;
    } while (count != 0);

    S.u = u;
    S.v = v;
    S.y0 = y0;
    S.y1 = y1;
    S.a0 = a0;
    S.a1 = a1;
}

void YUV_16a4m_4x2(u32 count)
{
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    a0 = S.a0;
    a1 = S.a1;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);

    S.dest0 += count * RGB_16_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 av0 = *a0++;
        u32 yv0 = *y0++;
        u32 av1 = *a1++;
        u32 yv1 = *y1++;
        u32 av0hi = av0 >> 16;
        u32 yv0hi = yv0 >> 16;
        u32 av1hi = av1 >> 16;
        u32 yv1hi = yv1 >> 16;
        u32 av0lo = av0 & RGB_WORD_LO_MASK;
        u32 yv0lo = yv0 & RGB_WORD_LO_MASK;
        u32 av1lo = av1 & RGB_WORD_LO_MASK;
        u32 yv1lo = yv1 & RGB_WORD_LO_MASK;
        u32 p0 = RGB565_A4_MONO(RGB_WORD_BYTE1(yv0hi), RGB_WORD_BYTE1(av0hi));
        u32 p1 = RGB565_A4_MONO(RGB_WORD_BYTE0(yv0hi), RGB_WORD_BYTE0(av0hi));
        u32 p2 = RGB565_A4_MONO(RGB_WORD_BYTE1(yv0lo), RGB_WORD_BYTE1(av0lo));
        u32 p3 = RGB565_A4_MONO(RGB_WORD_BYTE0(yv0lo), RGB_WORD_BYTE0(av0lo));

        dest0[RGB_TILE_WORD0] = RGB565_PAIR2(p0, p1);

        p0 = RGB565_A4_MONO(RGB_WORD_BYTE1(yv1hi), RGB_WORD_BYTE1(av1hi));
        p1 = RGB565_A4_MONO(RGB_WORD_BYTE0(yv1hi), RGB_WORD_BYTE0(av1hi));
        dest1[RGB_TILE_WORD0] = RGB565_PAIR2(p0, p1);

        dest0[RGB_TILE_WORD1] = RGB565_PAIR2(p2, p3);

        p2 = RGB565_A4_MONO(RGB_WORD_BYTE1(yv1lo), RGB_WORD_BYTE1(av1lo));
        p3 = RGB565_A4_MONO(RGB_WORD_BYTE0(yv1lo), RGB_WORD_BYTE0(av1lo));
        dest1[RGB_TILE_WORD1] = RGB565_PAIR2(p2, p3);

        dest0 += RGB_TILE_HALF_BLOCK_WORDS;
        dest1 += RGB_TILE_HALF_BLOCK_WORDS;
        --count;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
    S.a0 = a0;
    S.a1 = a1;
}

void YUV_16a4mx2_4x2(u32 count)
{
    u32 PTR4* dest0;
    u32 PTR4* dest1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    dest0 = (u32 PTR4*)S.dest0;
    dest1 = (u32 PTR4*)S.dest1;
    a0 = S.a0;
    a1 = S.a1;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);

    S.dest0 += count * RGB_16_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(dest0, base, pitch);
    row1 = RGB_TILE_ROW(dest1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, dest0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, dest1, pitch, tiledPitch, row1);

    do {
        u32 av0 = *a0++;
        u32 yv0 = *y0++;
        u32 av1 = *a1++;
        u32 yv1 = *y1++;
        u32 av0hi = av0 >> 16;
        u32 yv0hi = yv0 >> 16;
        u32 av1hi = av1 >> 16;
        u32 yv1hi = yv1 >> 16;
        u32 av0lo = av0 & RGB_WORD_LO_MASK;
        u32 yv0lo = yv0 & RGB_WORD_LO_MASK;
        u32 av1lo = av1 & RGB_WORD_LO_MASK;
        u32 yv1lo = yv1 & RGB_WORD_LO_MASK;
        u32 p0 = RGB565_A4_MONO(RGB_WORD_BYTE1(yv0hi), RGB_WORD_BYTE1(av0hi));
        u32 p1 = RGB565_A4_MONO(RGB_WORD_BYTE0(yv0hi), RGB_WORD_BYTE0(av0hi));
        u32 p2 = RGB565_A4_MONO(RGB_WORD_BYTE1(yv0lo), RGB_WORD_BYTE1(av0lo));
        u32 p3 = RGB565_A4_MONO(RGB_WORD_BYTE0(yv0lo), RGB_WORD_BYTE0(av0lo));

        dest0[RGB_TILE_WORD0] = RGB565_PAIR(p0);
        dest0[RGB_TILE_WORD1] = RGB565_PAIR(p1);

        p0 = RGB565_A4_MONO(RGB_WORD_BYTE1(yv1hi), RGB_WORD_BYTE1(av1hi));
        p1 = RGB565_A4_MONO(RGB_WORD_BYTE0(yv1hi), RGB_WORD_BYTE0(av1hi));
        dest1[RGB_TILE_WORD0] = RGB565_PAIR(p0);
        dest1[RGB_TILE_WORD1] = RGB565_PAIR(p1);

        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(p2);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(p3);

        p2 = RGB565_A4_MONO(RGB_WORD_BYTE1(yv1lo), RGB_WORD_BYTE1(av1lo));
        p3 = RGB565_A4_MONO(RGB_WORD_BYTE0(yv1lo), RGB_WORD_BYTE0(av1lo));
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(p2);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(p3);

        dest0 += RGB_TILE_BLOCK_WORDS;
        dest1 += RGB_TILE_BLOCK_WORDS;
        --count;
    } while (count != 0);

    S.y0 = y0;
    S.y1 = y1;
    S.a0 = a0;
    S.a1 = a1;
}

void YUV_16_4x2_odd(void)
{
}

void YUV_32_4x2_odd(void)
{
}

void YUV_16x2_4x2_odd(void)
{
}

void YUV_16a4x2_4x2_odd(void)
{
}

void YUV_32x2_4x2_odd(void)
{
}

void YUV_16a4_4x2_odd(void)
{
}

void YUV_32a_4x2_odd(void)
{
}

void YUV_32ax2_4x2_odd(void)
{
}

void PTR4* GetTiledRgbLoc(void PTR4* ptr, u32 tilePitch)
{
    s32 row;

    row = RGB_TILE_ROW(ptr, S.base, S.pitch);

    return RGB_TILE_LOC(S.base, ptr, S.pitch, tilePitch, row);
}
