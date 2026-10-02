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
    RGB_BYTE1_SHIFT = 8,
    RGB_BYTE2_SHIFT = 16,
    RGB_BYTE3_SHIFT = 24,
    RGB_HALFWORD_SHIFT = 16,
    RGB_WORD_LO_MASK = 0x0000ffff,
    RGB_WORD_HI_MASK = 0xffff0000,
    RGB_ALPHA0_MASK = 0xff000000,
    RGB_ALPHA2_MASK = 0x0000ff00
} RGBWordMasks;

#define RGB_WORD_BYTE3(word) (((word) >> RGB_BYTE3_SHIFT) & RGB_BYTE_MASK)
#define RGB_WORD_BYTE2(word) (((word) >> RGB_BYTE2_SHIFT) & RGB_BYTE_MASK)
#define RGB_WORD_BYTE1(word) (((word) >> RGB_BYTE1_SHIFT) & RGB_BYTE_MASK)
#define RGB_WORD_BYTE0(word) ((word) & RGB_BYTE_MASK)
#define RGB32_PAIR_HIGH(pixel) (((pixel) & RGB_WORD_HI_MASK) | ((pixel) >> RGB_HALFWORD_SHIFT))
#define RGB32_PAIR_LOW(pixel) (((pixel) << RGB_HALFWORD_SHIFT) | ((pixel) & RGB_WORD_LO_MASK))
#define RGB32_MONO_X2_HIGH(pixel) RGB32_PAIR_HIGH(pixel)
#define RGB32_MONO_X2_LOW(pixel) RGB32_PAIR_LOW(pixel)
#define RGB32_PAIR_HIGH2(left, right) (((left) & RGB_WORD_HI_MASK) | ((right) >> RGB_HALFWORD_SHIFT))
#define RGB32_PAIR_LOW2(left, right) (((left) << RGB_HALFWORD_SHIFT) | ((right) & RGB_WORD_LO_MASK))
#define RGB32_COLOR_RED_PAIR(left, right, red) (((left)[red] << RGB_HALFWORD_SHIFT) | (right)[red])
#define RGB32_COLOR_GB_PAIR(left, right, gb, blue) \
    (((left)[gb] << RGB_BYTE3_SHIFT) | ((left)[blue] << RGB_BYTE2_SHIFT) | ((right)[gb] << RGB_BYTE1_SHIFT) | (right)[blue])
#define RGB32_COLOR_RED_DUP(row, red) (((row)[red] << RGB_HALFWORD_SHIFT) | (row)[red])
#define RGB32_COLOR_GB_DUP(row, gb, blue) \
    (((row)[gb] << RGB_BYTE3_SHIFT) | ((row)[blue] << RGB_BYTE2_SHIFT) | ((row)[gb] << RGB_BYTE1_SHIFT) | (row)[blue])
#define RGB32_ALPHA_PAIR_HIGH(alpha, left, right) \
    (((alpha) & RGB_ALPHA0_MASK) | ((left) & RGB_WORD_HI_MASK) | ((right) >> RGB_HALFWORD_SHIFT) | \
     (((alpha) >> RGB_BYTE1_SHIFT) & RGB_ALPHA2_MASK))
#define RGB32_ALPHA_PAIR_LOW(alpha, left, right) \
    ((((alpha) & RGB_ALPHA2_MASK) << RGB_HALFWORD_SHIFT) | ((left) & RGB_WORD_HI_MASK) | ((right) >> RGB_HALFWORD_SHIFT) | \
     (RGB_WORD_BYTE0(alpha) << RGB_BYTE1_SHIFT))
#define RGB32_ALPHA_COLOR_PAIR_HIGH(alpha, left, right) \
    (((alpha) & RGB_ALPHA0_MASK) | ((left) << RGB_HALFWORD_SHIFT) | (((alpha) >> RGB_BYTE1_SHIFT) & RGB_ALPHA2_MASK) | (right))
#define RGB32_ALPHA_COLOR_PAIR_LOW(alpha, left, right) \
    ((((alpha) & RGB_ALPHA2_MASK) << RGB_HALFWORD_SHIFT) | ((left) << RGB_HALFWORD_SHIFT) | (RGB_WORD_BYTE0(alpha) << RGB_BYTE1_SHIFT) | (right))
#define RGB32_ALPHA_COLOR_DUP_PAIR(alpha_hi, alpha_lo, value) \
    ((alpha_hi) | ((value) << RGB_HALFWORD_SHIFT) | (alpha_lo) | (value))
#define RGB32_ALPHA_DUP_PAIR(alpha_hi, alpha_lo, pixel) \
    ((alpha_hi) | ((pixel) & RGB_WORD_HI_MASK) | ((pixel) >> RGB_HALFWORD_SHIFT) | (alpha_lo))
#define RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha, pixel) RGB32_ALPHA_DUP_PAIR((alpha) << RGB_BYTE3_SHIFT, (alpha) << RGB_BYTE1_SHIFT, (pixel))
#define RGB32_MONO_DUP_PAIR(pixel) (((pixel) << RGB_HALFWORD_SHIFT) | ((pixel) & RGB_WORD_LO_MASK))

typedef enum RGBClampLayout
{
    RGB_CLAMP_BIAS = 0x100
} RGBClampLayout;

#define RGB565(y, blue, green, red)                                                                                  \
    ((u16)clamp_b[RGB_CLAMP_BIAS + (y) + (blue)] | (u16)clamp_r[RGB_CLAMP_BIAS + (y) + (red)] |              \
     (u16)clamp_g[RGB_CLAMP_BIAS + (y) + (green)])

#define RGB565_BIASED(red_table, green_table, blue_table, y, blue, green, red)                                      \
    ((u16)(blue_table)[(y) + (blue)] | (u16)(red_table)[(y) + (red)] | (u16)(green_table)[(y) + (green)])
#define RGB565_PREBIASED(red_table, green_table, blue_table, y)                                                     \
    ((u16)(blue_table)[(y)] | (u16)(red_table)[(y)] | (u16)(green_table)[(y)])
#define RGB565_PAIR(pixel) (((pixel) << RGB_HALFWORD_SHIFT) | (pixel))
#define RGB565_A4(y, blue, green, red, a) (RGB565((y), (blue), (green), (red)) | (u16)clamp_a4[(a)])
#define RGB565_A4_MONO(y, a) ((u16)mono16[(y)] | (u16)clamp_a4[(a)])
#define RGB565_A4_MONO_BIASED(ytable, atable, y, a) ((u16)(atable)[(a)] | (u16)(ytable)[(y)])
#define RGB565_A4_BIASED(red_table, green_table, blue_table, alpha_table, y, blue, green, red, alpha)              \
    ((u16)(blue_table)[(y) + (blue)] | (u16)(red_table)[(y) + (red)] | (u16)(green_table)[(y) + (green)] |          \
     (u16)(alpha_table)[(alpha)])
#define RGB565_A4_PREBIASED(red_table, green_table, blue_table, alpha_table, y, alpha)                             \
    ((u16)(red_table)[(y)] | (u16)(green_table)[(y)] | (u16)(blue_table)[(y)] | (u16)(alpha_table)[(alpha)])
#define RGB32_M(y) (mono32[(y)])
#define RGB565_PAIR2(left, right) (((left) << RGB_HALFWORD_SHIFT) | (right))

// As in the scalar 16-bit converter, the U-derived contribution biases clamp_r
// and the V-derived contribution biases clamp_b in the colored 16-bit kernels.
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y1;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 PTR4* y0;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    RGBYUVTables PTR4* tables;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);
    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    tables = &YUVTables;

    do {
        u8 vhi;
        u8 uhi;
        u8 vlo;
        u8 ulo;
        u32 PTR4* y00;
        u32 PTR4* y01;
        s32 r;
        s32 b;
        s32 gb;

        vword = *v++;
        uword = *u++;
        yv0 = *y0++;
        vhi = RGB_WORD_BYTE1(vword);
        uhi = RGB_WORD_BYTE1(uword);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv0)];
        r = tables->v_to_r[vhi];
        b = tables->u_to_b[uhi];
        gb = tables->u_to_gb[uhi] + tables->v_to_gb[vhi];
        yv1 = *y1++;
        dest0[RGB_TILE_WORD0] = RGB32_COLOR_RED_PAIR(y00, y01, r);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv1)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB32_COLOR_RED_PAIR(y00, y01, r);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);

        vlo = RGB_WORD_BYTE0(vword);
        ulo = RGB_WORD_BYTE0(uword);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv0)];
        r = tables->v_to_r[vlo];
        b = tables->u_to_b[ulo];
        gb = tables->u_to_gb[ulo] + tables->v_to_gb[vlo];
        dest0[RGB_TILE_WORD1] = RGB32_COLOR_RED_PAIR(y00, y01, r);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);
        dest0 += RGB_TILE_BLOCK_WORDS;

        y00 = clamp_ytable[RGB_WORD_BYTE1(yv1)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_WORD1] = RGB32_COLOR_RED_PAIR(y00, y01, r);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y1;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 PTR4* y0;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    RGBYUVTables PTR4* tables;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);
    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    tables = &YUVTables;

    do {
        u8 vhi;
        u8 uhi;
        u8 vlo;
        u8 ulo;
        u32 PTR4* y00;
        u32 PTR4* y01;
        s32 r;
        s32 b;
        s32 gb;

        vword = *v++;
        uword = *u++;
        yv0 = *y0++;
        vhi = RGB_WORD_BYTE1(vword);
        uhi = RGB_WORD_BYTE1(uword);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv0)];
        r = tables->v_to_r[vhi];
        b = tables->u_to_b[uhi];
        gb = tables->u_to_gb[uhi] + tables->v_to_gb[vhi];
        yv1 = *y1++;

        dest0[RGB_TILE_WORD0] = RGB32_COLOR_RED_DUP(y00, r);
        dest0[RGB_TILE_WORD1] = RGB32_COLOR_RED_DUP(y01, r);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv1)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB32_COLOR_RED_DUP(y00, r);
        dest1[RGB_TILE_WORD1] = RGB32_COLOR_RED_DUP(y01, r);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);

        vlo = RGB_WORD_BYTE0(vword);
        ulo = RGB_WORD_BYTE0(uword);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv0)];
        r = tables->v_to_r[vlo];
        b = tables->u_to_b[ulo];
        gb = tables->u_to_gb[ulo] + tables->v_to_gb[vlo];
        dest0[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_COLOR_RED_DUP(y00, r);
        dest0[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_COLOR_RED_DUP(y01, r);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);
        dest0 += RGB_TILE_X2_BLOCK_WORDS;

        y00 = clamp_ytable[RGB_WORD_BYTE1(yv1)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_COLOR_RED_DUP(y00, r);
        dest1[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_COLOR_RED_DUP(y01, r);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    y0 = S.y0;
    y1 = S.y1;

    do {
        u32 yv0 = *y0++;
        u32 yv1 = *y1++;
        u32 a = RGB32_M(RGB_WORD_BYTE3(yv0));
        u32 b = RGB32_M(RGB_WORD_BYTE2(yv0));

        dest0[RGB_TILE_WORD0] = RGB32_MONO_X2_HIGH(a);
        dest0[RGB_TILE_WORD1] = RGB32_MONO_X2_HIGH(b);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_MONO_X2_LOW(a);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_MONO_X2_LOW(b);

        a = RGB32_M(RGB_WORD_BYTE1(yv0));
        b = RGB32_M(RGB_WORD_BYTE0(yv0));
        dest0[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_MONO_X2_HIGH(a);
        dest0[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_MONO_X2_HIGH(b);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_MONO_X2_LOW(a);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_MONO_X2_LOW(b);

        a = RGB32_M(RGB_WORD_BYTE2(yv1));
        b = RGB32_M(RGB_WORD_BYTE1(yv1));
        dest1[RGB_TILE_WORD0] = RGB32_MONO_X2_HIGH(a);
        dest1[RGB_TILE_WORD1] = RGB32_MONO_X2_HIGH(b);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_MONO_X2_LOW(a);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_MONO_X2_LOW(b);

        a = RGB32_M(RGB_WORD_BYTE1(yv1));
        b = RGB32_M(RGB_WORD_BYTE0(yv1));
        dest1[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_MONO_X2_HIGH(a);
        dest1[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_MONO_X2_HIGH(b);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_MONO_X2_LOW(a);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_MONO_X2_LOW(b);

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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
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
    RGBYUVTables PTR4* tables;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH16(pitch);

    S.dest0 += count * RGB_16_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_4X2_ROW_BYTES;

    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    tables = &YUVTables;
    u_to_b = tables->u_to_b;
    v_to_r = tables->v_to_r;
    u_to_gb = tables->u_to_gb;
    v_to_gb = tables->v_to_gb;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    do {
        u8 uhi;
        u8 vhi;
        u8 ulo;
        u8 vlo;
        s32 b;
        s32 gb;
        s32 r;
        u32 PTR4* b_table;
        u32 PTR4* gb_table;
        u32 PTR4* r_table;
        u32 ya;
        u32 yb;

        uword = *u++;
        vword = *v;
        yv0 = *y0++;
        uhi = RGB_WORD_BYTE1(uword);
        vhi = RGB_WORD_BYTE1(vword);
        b = u_to_b[uhi];
        gb = u_to_gb[uhi] + v_to_gb[vhi];
        r = v_to_r[vhi];
        r_table = (clamp_r + RGB_CLAMP_BIAS) + b;
        gb_table = (clamp_g + RGB_CLAMP_BIAS) + gb;
        b_table = (clamp_b + RGB_CLAMP_BIAS) + r;
        ya = ytable[RGB_WORD_BYTE3(yv0)];
        yb = ytable[RGB_WORD_BYTE2(yv0)];
        yv1 = *y1++;
        dest0[RGB_TILE_WORD0] =
            RGB565_PAIR2(RGB565_PREBIASED(r_table, gb_table, b_table, ya), RGB565_PREBIASED(r_table, gb_table, b_table, yb));

        ya = ytable[RGB_WORD_BYTE3(yv1)];
        yb = ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] =
            RGB565_PAIR2(RGB565_PREBIASED(r_table, gb_table, b_table, ya), RGB565_PREBIASED(r_table, gb_table, b_table, yb));

        ulo = RGB_WORD_BYTE0(uword);
        vlo = RGB_WORD_BYTE0(vword);
        b = u_to_b[ulo];
        gb = u_to_gb[ulo] + v_to_gb[vlo];
        r = v_to_r[vlo];
        r_table = (clamp_r + RGB_CLAMP_BIAS) + b;
        gb_table = (clamp_g + RGB_CLAMP_BIAS) + gb;
        b_table = (clamp_b + RGB_CLAMP_BIAS) + r;
        ya = ytable[RGB_WORD_BYTE1(yv0)];
        yb = ytable[RGB_WORD_BYTE0(yv0)];
        dest0[RGB_TILE_WORD1] =
            RGB565_PAIR2(RGB565_PREBIASED(r_table, gb_table, b_table, ya), RGB565_PREBIASED(r_table, gb_table, b_table, yb));
        dest0 += RGB_TILE_HALF_BLOCK_WORDS;

        ya = ytable[RGB_WORD_BYTE1(yv1)];
        yb = ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_WORD1] =
            RGB565_PAIR2(RGB565_PREBIASED(r_table, gb_table, b_table, ya), RGB565_PREBIASED(r_table, gb_table, b_table, yb));
        dest1 += RGB_TILE_HALF_BLOCK_WORDS;

        ++v;
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u16 PTR4* u;
    u16 PTR4* v;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    u32 PTR4* clamp_r_base;
    u32 PTR4* clamp_g_base;
    u32 PTR4* clamp_b_base;
    RGBYUVTables PTR4* tables;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH16(pitch);

    S.dest0 += count * RGB_16_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_X2_4X2_ROW_BYTES;

    y0 = S.y0;
    u = S.u;
    v = S.v;
    y1 = S.y1;
    tables = &YUVTables;
    clamp_r_base = clamp_r + RGB_CLAMP_BIAS;
    clamp_b_base = clamp_b + RGB_CLAMP_BIAS;
    clamp_g_base = clamp_g + RGB_CLAMP_BIAS;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    do {
        u8 uhi;
        u8 vhi;
        u8 ulo;
        u8 vlo;
        u16 pix0;
        u16 pix1;
        s32 b;
        s32 gb;
        s32 r;
        u32 PTR4* b_table;
        u32 PTR4* gb_table;
        u32 PTR4* r_table;
        u16 yhalf;
        u32 ya;
        u32 yb;

        vword = *v++;
        uword = *u++;
        yv0 = *y0++;
        uhi = RGB_WORD_BYTE1(uword);
        vhi = RGB_WORD_BYTE1(vword);
        gb = tables->u_to_gb[uhi] + tables->v_to_gb[vhi];
        b = tables->u_to_b[uhi];
        r = tables->v_to_r[vhi];
        b_table = clamp_b_base + r;
        gb_table = clamp_g_base + gb;
        r_table = clamp_r_base + b;
        yhalf = yv0 >> RGB_HALFWORD_SHIFT;
        ya = ytable[RGB_WORD_BYTE1(yhalf)];
        yb = ytable[RGB_WORD_BYTE0(yhalf)];
        yv1 = *y1++;
        pix0 = RGB565_PREBIASED(r_table, gb_table, b_table, ya);
        pix1 = RGB565_PREBIASED(r_table, gb_table, b_table, yb);
        dest0[RGB_TILE_WORD0] = RGB565_PAIR(pix0);
        dest0[RGB_TILE_WORD1] = RGB565_PAIR(pix1);

        yhalf = yv1 >> RGB_HALFWORD_SHIFT;
        ya = ytable[RGB_WORD_BYTE1(yhalf)];
        yb = ytable[RGB_WORD_BYTE0(yhalf)];
        pix0 = RGB565_PREBIASED(r_table, gb_table, b_table, ya);
        pix1 = RGB565_PREBIASED(r_table, gb_table, b_table, yb);
        dest1[RGB_TILE_WORD0] = RGB565_PAIR(pix0);
        dest1[RGB_TILE_WORD1] = RGB565_PAIR(pix1);

        ulo = RGB_WORD_BYTE0(uword);
        vlo = RGB_WORD_BYTE0(vword);
        r = tables->v_to_r[vlo];
        b = tables->u_to_b[ulo];
        gb = tables->u_to_gb[ulo] + tables->v_to_gb[vlo];
        b_table = clamp_b_base + r;
        gb_table = clamp_g_base + gb;
        r_table = clamp_r_base + b;
        yv0 = (u16)yv0;
        ya = ytable[RGB_WORD_BYTE1(yv0)];
        yb = ytable[RGB_WORD_BYTE0(yv0)];
        pix0 = RGB565_PREBIASED(r_table, gb_table, b_table, ya);
        pix1 = RGB565_PREBIASED(r_table, gb_table, b_table, yb);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(pix0);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(pix1);
        dest0 += RGB_TILE_BLOCK_WORDS;

        yv1 = (u16)yv1;
        ya = ytable[RGB_WORD_BYTE1(yv1)];
        yb = ytable[RGB_WORD_BYTE0(yv1)];
        pix0 = RGB565_PREBIASED(r_table, gb_table, b_table, ya);
        pix1 = RGB565_PREBIASED(r_table, gb_table, b_table, yb);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(pix0);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(pix1);
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    u32 PTR4* table;

    linear0 = S.dest0;
    linear1 = S.dest1;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    table = mono16;

    S.dest0 += count * RGB_16_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    do {
        u32 yv0 = *y0++;
        u32 yv1 = *y1++;
        u16 y0hi = yv0 >> RGB_HALFWORD_SHIFT;
        u16 y1hi = yv1 >> RGB_HALFWORD_SHIFT;
        u16 y0lo = yv0;
        u16 y1lo = yv1;
        u16 a = (u16)table[RGB_WORD_BYTE1(y0hi)];
        u16 b = (u16)table[RGB_WORD_BYTE0(y0hi)];

        dest0[RGB_TILE_WORD0] = RGB565_PAIR2(a, b);

        a = (u16)table[RGB_WORD_BYTE1(y1hi)];
        b = (u16)table[RGB_WORD_BYTE0(y1hi)];
        dest1[RGB_TILE_WORD0] = RGB565_PAIR2(a, b);

        a = (u16)table[RGB_WORD_BYTE1(y0lo)];
        b = (u16)table[RGB_WORD_BYTE0(y0lo)];
        dest0[RGB_TILE_WORD1] = RGB565_PAIR2(a, b);

        a = (u16)table[RGB_WORD_BYTE1(y1lo)];
        b = (u16)table[RGB_WORD_BYTE0(y1lo)];
        dest1[RGB_TILE_WORD1] = RGB565_PAIR2(a, b);

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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    u32 PTR4* table;

    linear0 = S.dest0;
    linear1 = S.dest1;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    table = mono16;

    S.dest0 += count * RGB_16_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    do {
        u32 yv0 = *y0++;
        u32 yv1 = *y1++;
        u16 y0hi = yv0 >> RGB_HALFWORD_SHIFT;
        u16 y1hi = yv1 >> RGB_HALFWORD_SHIFT;
        u16 y0lo = yv0;
        u16 y1lo = yv1;
        u16 a = (u16)table[RGB_WORD_BYTE1(y0hi)];
        u16 b = (u16)table[RGB_WORD_BYTE0(y0hi)];

        dest0[RGB_TILE_WORD0] = RGB565_PAIR(a);
        dest0[RGB_TILE_WORD1] = RGB565_PAIR(b);

        a = (u16)table[RGB_WORD_BYTE1(y1hi)];
        b = (u16)table[RGB_WORD_BYTE0(y1hi)];
        dest1[RGB_TILE_WORD0] = RGB565_PAIR(a);
        dest1[RGB_TILE_WORD1] = RGB565_PAIR(b);

        a = (u16)table[RGB_WORD_BYTE1(y0lo)];
        b = (u16)table[RGB_WORD_BYTE0(y0lo)];
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(a);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(b);

        a = (u16)table[RGB_WORD_BYTE1(y1lo)];
        b = (u16)table[RGB_WORD_BYTE0(y1lo)];
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(a);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(b);

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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
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
    RGBYUVTables PTR4* tables;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);
    S.dest0 += count * RGB_32_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_4X2_ROW_BYTES;
    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);
    u = S.u;
    a0 = S.a0;
    a1 = S.a1;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    tables = &YUVTables;

    do {
        u8 vhi;
        u8 uhi;
        u8 vlo;
        u8 ulo;
        u32 PTR4* y00;
        u32 PTR4* y01;
        s32 r;
        s32 b;
        s32 gb;

        vword = *v++;
        uword = *u++;
        yv0 = *y0++;
        yv1 = *y1++;
        av0 = *a0++;
        av1 = *a1++;
        vhi = RGB_WORD_BYTE1(vword);
        uhi = RGB_WORD_BYTE1(uword);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv0)];
        r = tables->v_to_r[vhi];
        b = tables->u_to_b[uhi];
        gb = tables->u_to_gb[uhi] + tables->v_to_gb[vhi];
        dest0[RGB_TILE_WORD0] = RGB32_ALPHA_COLOR_PAIR_HIGH(av0, y00[r], y01[r]);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv1)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB32_ALPHA_COLOR_PAIR_HIGH(av1, y00[r], y01[r]);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);

        vlo = RGB_WORD_BYTE0(vword);
        ulo = RGB_WORD_BYTE0(uword);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv0)];
        r = tables->v_to_r[vlo];
        b = tables->u_to_b[ulo];
        gb = tables->u_to_gb[ulo] + tables->v_to_gb[vlo];
        dest0[RGB_TILE_WORD1] = RGB32_ALPHA_COLOR_PAIR_LOW(av0, y00[r], y01[r]);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);
        dest0 += RGB_TILE_BLOCK_WORDS;

        y00 = clamp_ytable[RGB_WORD_BYTE1(yv1)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_WORD1] = RGB32_ALPHA_COLOR_PAIR_LOW(av1, y00[r], y01[r]);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_PAIR(y00, y01, gb, b);
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
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
    RGBYUVTables PTR4* tables;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);
    y1 = S.y1;
    y0 = S.y0;
    u = S.u;
    v = S.v;
    a0 = S.a0;
    a1 = S.a1;
    tables = &YUVTables;

    do {
        u8 vhi;
        u8 uhi;
        u8 vlo;
        u8 ulo;
        u32 PTR4* y00;
        u32 PTR4* y01;
        u32 alpha0;
        u32 alpha1;
        s32 r;
        s32 b;
        s32 gb;

        vword = *v++;
        uword = *u++;
        av0 = *a0++;
        yv0 = *y0++;
        av1 = *a1++;
        yv1 = *y1++;
        vhi = RGB_WORD_BYTE1(vword);
        uhi = RGB_WORD_BYTE1(uword);
        alpha0 = RGB_WORD_BYTE3(av0);
        alpha1 = RGB_WORD_BYTE2(av0);

        y00 = clamp_ytable[RGB_WORD_BYTE3(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv0)];
        r = tables->v_to_r[vhi];
        b = tables->u_to_b[uhi];
        gb = tables->u_to_gb[uhi] + tables->v_to_gb[vhi];

        dest0[RGB_TILE_WORD0] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha0 << RGB_BYTE3_SHIFT, alpha0 << RGB_BYTE1_SHIFT, y00[r]);
        dest0[RGB_TILE_WORD1] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha1 << RGB_BYTE3_SHIFT, alpha1 << RGB_BYTE1_SHIFT, y01[r]);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);

        alpha0 = RGB_WORD_BYTE3(av1);
        alpha1 = RGB_WORD_BYTE2(av1);
        y00 = clamp_ytable[RGB_WORD_BYTE3(yv1)];
        y01 = clamp_ytable[RGB_WORD_BYTE2(yv1)];
        dest1[RGB_TILE_WORD0] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha0 << RGB_BYTE3_SHIFT, alpha0 << RGB_BYTE1_SHIFT, y00[r]);
        dest1[RGB_TILE_WORD1] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha1 << RGB_BYTE3_SHIFT, alpha1 << RGB_BYTE1_SHIFT, y01[r]);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);

        vlo = RGB_WORD_BYTE0(vword);
        ulo = RGB_WORD_BYTE0(uword);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv0)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv0)];
        r = tables->v_to_r[vlo];
        b = tables->u_to_b[ulo];
        gb = tables->u_to_gb[ulo] + tables->v_to_gb[vlo];

        alpha0 = RGB_WORD_BYTE1(av0);
        alpha1 = RGB_WORD_BYTE0(av0);
        dest0[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha0 << RGB_BYTE3_SHIFT, alpha0 << RGB_BYTE1_SHIFT, y00[r]);
        dest0[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha1 << RGB_BYTE3_SHIFT, alpha1 << RGB_BYTE1_SHIFT, y01[r]);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);
        dest0 += RGB_TILE_X2_BLOCK_WORDS;

        alpha0 = RGB_WORD_BYTE1(av1);
        alpha1 = RGB_WORD_BYTE0(av1);
        y00 = clamp_ytable[RGB_WORD_BYTE1(yv1)];
        y01 = clamp_ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha0 << RGB_BYTE3_SHIFT, alpha0 << RGB_BYTE1_SHIFT, y00[r]);
        dest1[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_ALPHA_COLOR_DUP_PAIR(alpha1 << RGB_BYTE3_SHIFT, alpha1 << RGB_BYTE1_SHIFT, y01[r]);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_COLOR_GB_DUP(y00, gb, b);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_COLOR_GB_DUP(y01, gb, b);
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH32(pitch);

    S.dest0 += count * RGB_32_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_32_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    a0 = S.a0;
    a1 = S.a1;
    y0 = S.y0;
    y1 = S.y1;

    do {
        u32 yv0 = *y0++;
        u32 av0 = *a0;
        u32 yv1 = *y1;
        u32 av1 = *a1;
        u32 p0 = RGB32_M(RGB_WORD_BYTE3(yv0));
        u32 p1 = RGB32_M(RGB_WORD_BYTE2(yv0));
        u8 alpha0 = RGB_WORD_BYTE3(av0);
        u8 alpha1 = RGB_WORD_BYTE2(av0);

        dest0[RGB_TILE_WORD0] = RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha0, p0);
        dest0[RGB_TILE_WORD1] = RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha1, p1);
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB32_MONO_DUP_PAIR(p0);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB32_MONO_DUP_PAIR(p1);

        p0 = RGB32_M(RGB_WORD_BYTE1(yv0));
        p1 = RGB32_M(RGB_WORD_BYTE0(yv0));
        dest0[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha0, p0);
        dest0[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha1, p1);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_MONO_DUP_PAIR(p0);
        dest0[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_MONO_DUP_PAIR(p1);

        ++y1;
        ++a0;
        ++a1;
        p0 = RGB32_M(RGB_WORD_BYTE2(yv1));
        p1 = RGB32_M(RGB_WORD_BYTE1(yv1));
        alpha0 = RGB_WORD_BYTE3(av1);
        alpha1 = RGB_WORD_BYTE2(av1);
        dest1[RGB_TILE_WORD0] = RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha0, p0);
        dest1[RGB_TILE_WORD1] = RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha1, p1);
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB32_MONO_DUP_PAIR(p0);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB32_MONO_DUP_PAIR(p1);

        p0 = RGB32_M(RGB_WORD_BYTE1(yv1));
        p1 = RGB32_M(RGB_WORD_BYTE0(yv1));
        dest1[RGB_TILE_SECOND_BLOCK_WORD0] = RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha0, p0);
        dest1[RGB_TILE_SECOND_BLOCK_WORD1] = RGB32_ALPHA_SAMPLE_DUP_PAIR(alpha1, p1);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD0] = RGB32_MONO_DUP_PAIR(p0);
        dest1[RGB_TILE_SECOND_BLOCK_NEXT_ROW_WORD1] = RGB32_MONO_DUP_PAIR(p1);

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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
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
    u32 PTR4* clamp_r_base;
    u32 PTR4* clamp_g_base;
    u32 PTR4* clamp_b_base;
    u32 PTR4* clamp_a4_base;
    RGBYUVTables PTR4* tables;

    linear0 = S.dest0;
    base = S.base;
    linear1 = S.dest1;
    pitch = S.pitch;
    tiledPitch = RGB_TILE_PITCH16(pitch);

    S.dest0 += count * RGB_16_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_4X2_ROW_BYTES;

    a0 = S.a0;
    a1 = S.a1;
    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    tables = &YUVTables;
    clamp_r_base = clamp_r + RGB_CLAMP_BIAS;
    clamp_g_base = clamp_g + RGB_CLAMP_BIAS;
    clamp_b_base = clamp_b + RGB_CLAMP_BIAS;
    clamp_a4_base = clamp_a4;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    do {
        u8 uhi;
        u8 vhi;
        u8 ulo;
        u8 vlo;
        s32 b;
        s32 gb;
        s32 r;
        u32 PTR4* b_table;
        u32 PTR4* gb_table;
        u32 PTR4* r_table;
        u16 yhalf;
        u32 ya;
        u32 yb;

        vword = *v++;
        uword = *u++;
        yv0 = *y0++;
        av0 = *a0++;
        uhi = RGB_WORD_BYTE1(uword);
        vhi = RGB_WORD_BYTE1(vword);
        b = tables->u_to_b[uhi];
        gb = tables->u_to_gb[uhi] + tables->v_to_gb[vhi];
        r = tables->v_to_r[vhi];
        b_table = clamp_b_base + r;
        gb_table = clamp_g_base + gb;
        r_table = clamp_r_base + b;
        yhalf = yv0 >> RGB_HALFWORD_SHIFT;
        ya = ytable[RGB_WORD_BYTE1(yhalf)];
        yb = ytable[RGB_WORD_BYTE0(yhalf)];
        yv1 = *y1++;
        av1 = *a1++;
        dest0[RGB_TILE_WORD0] = RGB565_PAIR2(
            RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, ya, RGB_WORD_BYTE3(av0)),
            RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, yb, RGB_WORD_BYTE2(av0)));

        yhalf = yv1 >> RGB_HALFWORD_SHIFT;
        ya = ytable[RGB_WORD_BYTE1(yhalf)];
        yb = ytable[RGB_WORD_BYTE0(yhalf)];
        dest1[RGB_TILE_WORD0] = RGB565_PAIR2(
            RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, ya, RGB_WORD_BYTE3(av1)),
            RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, yb, RGB_WORD_BYTE2(av1)));

        ulo = RGB_WORD_BYTE0(uword);
        vlo = RGB_WORD_BYTE0(vword);
        gb = tables->u_to_gb[ulo] + tables->v_to_gb[vlo];
        b = tables->u_to_b[ulo];
        r = tables->v_to_r[vlo];
        gb_table = clamp_g_base + gb;
        r_table = clamp_r_base + b;
        b_table = clamp_b_base + r;
        yv0 = (u16)yv0;
        ya = ytable[RGB_WORD_BYTE1(yv0)];
        yb = ytable[RGB_WORD_BYTE0(yv0)];
        dest0[RGB_TILE_WORD1] = RGB565_PAIR2(
            RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, ya, RGB_WORD_BYTE1(av0)),
            RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, yb, RGB_WORD_BYTE0(av0)));
        dest0 += RGB_TILE_HALF_BLOCK_WORDS;

        yv1 = (u16)yv1;
        ya = ytable[RGB_WORD_BYTE1(yv1)];
        yb = ytable[RGB_WORD_BYTE0(yv1)];
        dest1[RGB_TILE_WORD1] = RGB565_PAIR2(
            RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, ya, RGB_WORD_BYTE1(av1)),
            RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, yb, RGB_WORD_BYTE0(av1)));
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
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
    RGBYUVTables PTR4* tables;

    linear0 = S.dest0;
    linear1 = S.dest1;
    u = S.u;
    v = S.v;
    y0 = S.y0;
    y1 = S.y1;
    a0 = S.a0;
    a1 = S.a1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    tables = &YUVTables;
    u_to_b = tables->u_to_b;
    v_to_gb = tables->v_to_gb;
    u_to_gb = tables->u_to_gb;
    v_to_r = tables->v_to_r;
    clamp_r_base = clamp_r + RGB_CLAMP_BIAS;
    clamp_g_base = clamp_g + RGB_CLAMP_BIAS;
    clamp_b_base = clamp_b + RGB_CLAMP_BIAS;
    clamp_a4_base = clamp_a4;

    S.dest0 += count * RGB_16_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    do {
        u8 uhi;
        u8 vhi;
        u8 ulo;
        u8 vlo;
        u16 pix0;
        u16 pix1;
        s32 b;
        s32 gb;
        s32 r;
        u32 PTR4* b_table;
        u32 PTR4* gb_table;
        u32 PTR4* r_table;
        u16 yhalf;
        u16 ahalf;
        u32 ya;
        u32 yb;

        uword = *u++;
        vword = *v++;
        yv0 = *y0++;
        av0 = *a0++;
        uhi = RGB_WORD_BYTE1(uword);
        vhi = RGB_WORD_BYTE1(vword);
        gb = u_to_gb[uhi] + v_to_gb[vhi];
        r = v_to_r[vhi];
        b = u_to_b[uhi];
        gb_table = clamp_g_base + gb;
        r_table = clamp_r_base + b;
        b_table = clamp_b_base + r;
        yhalf = yv0 >> RGB_HALFWORD_SHIFT;
        ya = ytable[RGB_WORD_BYTE1(yhalf)];
        yb = ytable[RGB_WORD_BYTE0(yhalf)];
        yv1 = *y1++;
        av1 = *a1++;

        ahalf = av0 >> RGB_HALFWORD_SHIFT;
        pix0 = RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, ya, RGB_WORD_BYTE1(ahalf));
        pix1 = RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, yb, RGB_WORD_BYTE0(ahalf));
        dest0[RGB_TILE_WORD0] = RGB565_PAIR(pix0);
        dest0[RGB_TILE_WORD1] = RGB565_PAIR(pix1);

        yhalf = yv1 >> RGB_HALFWORD_SHIFT;
        ya = ytable[RGB_WORD_BYTE1(yhalf)];
        yb = ytable[RGB_WORD_BYTE0(yhalf)];
        ahalf = av1 >> RGB_HALFWORD_SHIFT;
        pix0 = RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, ya, RGB_WORD_BYTE1(ahalf));
        pix1 = RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, yb, RGB_WORD_BYTE0(ahalf));
        dest1[RGB_TILE_WORD0] = RGB565_PAIR(pix0);
        dest1[RGB_TILE_WORD1] = RGB565_PAIR(pix1);

        ulo = RGB_WORD_BYTE0(uword);
        vlo = RGB_WORD_BYTE0(vword);
        r = v_to_r[vlo];
        b = u_to_b[ulo];
        gb = u_to_gb[ulo] + v_to_gb[vlo];
        b_table = clamp_b_base + r;
        gb_table = clamp_g_base + gb;
        r_table = clamp_r_base + b;
        av0 = (u16)av0;
        yv0 = (u16)yv0;
        ya = ytable[RGB_WORD_BYTE1(yv0)];
        yb = ytable[RGB_WORD_BYTE0(yv0)];
        pix0 = RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, ya, RGB_WORD_BYTE1(av0));
        pix1 = RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, yb, RGB_WORD_BYTE0(av0));
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(pix0);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(pix1);
        dest0 += RGB_TILE_BLOCK_WORDS;

        av1 = (u16)av1;
        yv1 = (u16)yv1;
        ya = ytable[RGB_WORD_BYTE1(yv1)];
        yb = ytable[RGB_WORD_BYTE0(yv1)];
        pix0 = RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, ya, RGB_WORD_BYTE1(av1));
        pix1 = RGB565_A4_PREBIASED(r_table, gb_table, b_table, clamp_a4_base, yb, RGB_WORD_BYTE0(av1));
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(pix0);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(pix1);
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    u32 PTR4* ytable_base;
    u32 PTR4* atable_base;

    linear0 = S.dest0;
    linear1 = S.dest1;
    a0 = S.a0;
    a1 = S.a1;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    atable_base = clamp_a4;
    ytable_base = mono16;

    S.dest0 += count * RGB_16_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    do {
        u32 av0 = *a0++;
        u32 yv0 = *y0;
        u32 av1 = *a1;
        u32 yv1 = *y1;
        u16 av0hi = av0 >> RGB_HALFWORD_SHIFT;
        u16 yv0hi = yv0 >> RGB_HALFWORD_SHIFT;
        u16 av1hi = av1 >> RGB_HALFWORD_SHIFT;
        u16 yv1hi = yv1 >> RGB_HALFWORD_SHIFT;
        u16 yv0lo = yv0;
        u16 av0lo = av0;
        u16 yv1lo = yv1;
        u16 av1lo = av1;
        u16 p0 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE1(yv0hi), RGB_WORD_BYTE1(av0hi));
        u16 p1 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE0(yv0hi), RGB_WORD_BYTE0(av0hi));

        dest0[RGB_TILE_WORD0] = RGB565_PAIR2(p0, p1);

        p0 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE1(yv1hi), RGB_WORD_BYTE1(av1hi));
        p1 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE0(yv1hi), RGB_WORD_BYTE0(av1hi));
        dest1[RGB_TILE_WORD0] = RGB565_PAIR2(p0, p1);

        p0 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE1(yv0lo), RGB_WORD_BYTE1(av0lo));
        p1 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE0(yv0lo), RGB_WORD_BYTE0(av0lo));
        dest0[RGB_TILE_WORD1] = RGB565_PAIR2(p0, p1);

        p0 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE1(yv1lo), RGB_WORD_BYTE1(av1lo));
        p1 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE0(yv1lo), RGB_WORD_BYTE0(av1lo));
        dest1[RGB_TILE_WORD1] = RGB565_PAIR2(p0, p1);

        ++y0;
        ++y1;
        ++a1;
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
    u8 PTR4* linear0;
    u8 PTR4* linear1;
    u32 PTR4* y0;
    u32 PTR4* y1;
    u32 PTR4* a0;
    u32 PTR4* a1;
    u32 pitch;
    u8 PTR4* base;
    s32 row0;
    s32 row1;
    u32 tiledPitch;
    u32 PTR4* ytable_base;
    u32 PTR4* atable_base;

    linear0 = S.dest0;
    linear1 = S.dest1;
    a0 = S.a0;
    a1 = S.a1;
    y0 = S.y0;
    y1 = S.y1;
    pitch = S.pitch;
    base = S.base;
    tiledPitch = RGB_TILE_PITCH16(pitch);
    atable_base = clamp_a4;
    ytable_base = mono16;

    S.dest0 += count * RGB_16_X2_4X2_ROW_BYTES;
    S.dest1 += count * RGB_16_X2_4X2_ROW_BYTES;

    row0 = RGB_TILE_ROW(linear0, base, pitch);
    row1 = RGB_TILE_ROW(linear1, base, pitch);
    dest0 = (u32 PTR4*)RGB_TILE_LOC(base, linear0, pitch, tiledPitch, row0);
    dest1 = (u32 PTR4*)RGB_TILE_LOC(base, linear1, pitch, tiledPitch, row1);

    do {
        u32 av0 = *a0++;
        u32 yv0 = *y0;
        u32 av1 = *a1;
        u32 yv1 = *y1;
        u16 av0hi = av0 >> RGB_HALFWORD_SHIFT;
        u16 yv0hi = yv0 >> RGB_HALFWORD_SHIFT;
        u16 av1hi = av1 >> RGB_HALFWORD_SHIFT;
        u16 yv1hi = yv1 >> RGB_HALFWORD_SHIFT;
        u16 yv0lo = yv0;
        u16 av0lo = av0;
        u16 yv1lo = yv1;
        u16 av1lo = av1;
        u16 p0 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE1(yv0hi), RGB_WORD_BYTE1(av0hi));
        u16 p1 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE0(yv0hi), RGB_WORD_BYTE0(av0hi));

        dest0[RGB_TILE_WORD0] = RGB565_PAIR(p0);
        dest0[RGB_TILE_WORD1] = RGB565_PAIR(p1);

        p0 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE1(yv1hi), RGB_WORD_BYTE1(av1hi));
        p1 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE0(yv1hi), RGB_WORD_BYTE0(av1hi));
        dest1[RGB_TILE_WORD0] = RGB565_PAIR(p0);
        dest1[RGB_TILE_WORD1] = RGB565_PAIR(p1);

        p0 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE1(yv0lo), RGB_WORD_BYTE1(av0lo));
        p1 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE0(yv0lo), RGB_WORD_BYTE0(av0lo));
        dest0[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(p0);
        dest0[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(p1);

        p0 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE1(yv1lo), RGB_WORD_BYTE1(av1lo));
        p1 = RGB565_A4_MONO_BIASED(ytable_base, atable_base, RGB_WORD_BYTE0(yv1lo), RGB_WORD_BYTE0(av1lo));
        dest1[RGB_TILE_NEXT_ROW_WORD0] = RGB565_PAIR(p0);
        dest1[RGB_TILE_NEXT_ROW_WORD1] = RGB565_PAIR(p1);

        ++y0;
        ++y1;
        ++a1;
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
