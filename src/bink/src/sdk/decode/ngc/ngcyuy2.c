#include "bink.h"
#include "ngcrgb.h"

// GameCube YUY2 stores four luma samples as a big-endian word and splices U/V
// bytes between adjacent luma values. The x2 paths duplicate each luma sample
// into two output pixels while reusing the same chroma byte.
typedef enum YUY2PackingConstants
{
    YUY2_LUMA16_DUP = 0x10001,
    YUY2_NEUTRAL_CHROMA = 0x800080,
    YUY2_CHROMA16_MASK = 0x00ff0000,
    YUY2_CHROMA8_MASK = 0x000000ff,
    YUY2_CHROMA_LOW16_MASK = 0x0000ff00
} YUY2PackingConstants;

typedef enum YUY2BlockLayout
{
    YUY2_WORDS_PER_BLOCK = 2,
    YUY2_X2_WORDS_PER_BLOCK = 4,
    YUY2_WORD_BYTES = 4,
    YUY2_PAIR_STRIDE = 2
} YUY2BlockLayout;

typedef enum YUY2PairLumaWord
{
    YUY2_PAIR_LUMA_WORD_0,
    YUY2_PAIR_LUMA_WORD_1
} YUY2PairLumaWord;
#define YUY2_BLOCK_PAIRS(count) ((s32)(count) >> 1)
#define YUY2_HAS_TAIL_BLOCK(count) (((count) & 1) != 0)
#define YUY2_ROW_BYTES(count) ((count) * YUY2_WORDS_PER_BLOCK * YUY2_WORD_BYTES)
#define YUY2_X2_ROW_BYTES(count) ((count) * YUY2_X2_WORDS_PER_BLOCK * YUY2_WORD_BYTES)
typedef enum YUY2LumaByteMask
{
    YUY2_Y0_MASK = 0xff000000,
    YUY2_Y1_MASK = 0x00ff0000,
    YUY2_Y2_MASK = 0x0000ff00,
    YUY2_Y3_MASK = 0x000000ff
} YUY2LumaByteMask;

#define YUY2_PACK_4Y01(y, chroma0, chroma1) ((y) & YUY2_Y0_MASK) + (chroma0) + (((y) >> 8) & YUY2_Y2_MASK) + (chroma1)
#define YUY2_PACK_4Y23(y, chroma0, chroma1) (((y) & YUY2_Y2_MASK) << 16) + (chroma0) + (((y) & YUY2_Y3_MASK) << 8) + (chroma1)
#define YUY2_PACK_M4Y01(y) ((y) & YUY2_Y0_MASK) + (((y) >> 8) & YUY2_Y2_MASK) + YUY2_NEUTRAL_CHROMA
#define YUY2_PACK_M4Y23(y) (((y) & YUY2_Y2_MASK) << 16) + (((y) & YUY2_Y3_MASK) << 8) + YUY2_NEUTRAL_CHROMA
#define YUY2_PACK_X2Y0(y, chroma) ((y) & YUY2_Y0_MASK) + (((y) & YUY2_Y0_MASK) >> 16) + (chroma)
#define YUY2_PACK_X2Y1(y, chroma) (((y) & YUY2_Y1_MASK) << 8) + (((y) & YUY2_Y1_MASK) >> 8) + (chroma)
#define YUY2_PACK_X2Y2(y, chroma) (((y) & YUY2_Y2_MASK) * YUY2_LUMA16_DUP) + (chroma)
#define YUY2_PACK_X2Y3(y, chroma) (((y) & YUY2_Y3_MASK) << 24) + (((y) & YUY2_Y3_MASK) << 8) + (chroma)
#define YUY2_CHROMA0_U(u) (((u) >> 8) & YUY2_CHROMA16_MASK)
#define YUY2_CHROMA0_V(v) ((v) >> 24)
#define YUY2_CHROMA1_U(u) ((u) & YUY2_CHROMA16_MASK)
#define YUY2_CHROMA1_V(v) (((v) >> 16) & YUY2_CHROMA8_MASK)
#define YUY2_CHROMA2_U(u) (((u) & YUY2_CHROMA_LOW16_MASK) << 8)
#define YUY2_CHROMA2_V(v) (((v) >> 8) & YUY2_CHROMA8_MASK)
#define YUY2_CHROMA3_U(u) (((u) & YUY2_CHROMA8_MASK) << 16)
#define YUY2_CHROMA3_V(v) ((v) & YUY2_CHROMA8_MASK)
#define YUY2_TAIL_CHROMA0_U(u) (((u) & YUY2_CHROMA_LOW16_MASK) << 8)
#define YUY2_TAIL_CHROMA0_V(v) ((v) >> 8)
#define YUY2_TAIL_CHROMA1_U(u) (((u) & YUY2_CHROMA8_MASK) << 16)
#define YUY2_TAIL_CHROMA1_V(v) ((v) & YUY2_CHROMA8_MASK)
#define YUY2_CHROMA0(u, v) (((u) >> 8) & YUY2_CHROMA16_MASK) + ((v) >> 24)
#define YUY2_CHROMA1(u, v) ((u) & YUY2_CHROMA16_MASK) + (((v) >> 16) & YUY2_CHROMA8_MASK)
#define YUY2_CHROMA2(u, v) (((u) & YUY2_CHROMA_LOW16_MASK) << 8) + (((v) >> 8) & YUY2_CHROMA8_MASK)
#define YUY2_CHROMA3(u, v) (((u) & YUY2_CHROMA8_MASK) << 16) + ((v) & YUY2_CHROMA8_MASK)
#define YUY2_TAIL_CHROMA0(u, v) (((u) & YUY2_CHROMA_LOW16_MASK) << 8) + ((v) >> 8)
#define YUY2_TAIL_CHROMA1(u, v) (((u) & YUY2_CHROMA8_MASK) << 16) + ((v) & YUY2_CHROMA8_MASK)
typedef enum YUY2RemainingState
{
    YUY2_NO_REMAINING = 0xffffffffU
} YUY2RemainingState;

// Helpers operate on one destination row; the public 4x2 entry points call the
// same packing logic for S.dest0 and S.dest1, then advance the shared context.
static void YUY2_4x2Helper(u32 count, u32 PTR4* dest, const u32 PTR4* y, const u32 PTR4* u,
                           const u32 PTR4* v);
static void YUY2_x2_4x2Helper(u32 count, u32 PTR4* dest, const u32 PTR4* y, const u32 PTR4* u,
                              const u32 PTR4* v);
static void YUY2_m_4x2Helper(u32 count, u32 PTR4* dest, const u32 PTR4* y);
static void YUY2_mx2_4x2Helper(u32 count, u32 PTR4* dest, const u32 PTR4* y);

void YUY2_4x2(u32 count)
{
    s32 pairs;
    u32 PTR4* dest;
    const u32 PTR4* y;
    const u32 PTR4* u;
    const u32 PTR4* v;

    pairs = YUY2_BLOCK_PAIRS(count);
    dest = (u32 PTR4*)S.dest0;
    y = S.y0;
    u = (const u32 PTR4*)S.u;
    v = (const u32 PTR4*)S.v;
    while (pairs-- != 0) {
        u32 y0 = *y++;
        u32 u0 = *u;
        u32 v0 = *v;

        *dest++ = YUY2_PACK_4Y01(y0, YUY2_CHROMA0_U(u0), YUY2_CHROMA0_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_CHROMA1_U(u0), YUY2_CHROMA1_V(v0));

        y0 = *y++;
        u++;
        v++;
        *dest++ = YUY2_PACK_4Y01(y0, YUY2_CHROMA2_U(u0), YUY2_CHROMA2_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_CHROMA3_U(u0), YUY2_CHROMA3_V(v0));

    }

    if (YUY2_HAS_TAIL_BLOCK(count)) {
        u32 y0 = y[YUY2_PAIR_LUMA_WORD_0];
        u16 u0 = *(const u16 PTR4*)u;
        u16 v0 = *(const u16 PTR4*)v;

        *dest++ = YUY2_PACK_4Y01(y0, YUY2_TAIL_CHROMA0_U(u0), YUY2_TAIL_CHROMA0_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_TAIL_CHROMA1_U(u0), YUY2_TAIL_CHROMA1_V(v0));
    }

    pairs = YUY2_BLOCK_PAIRS(count);
    dest = (u32 PTR4*)S.dest1;
    y = S.y1;
    u = (const u32 PTR4*)S.u;
    v = (const u32 PTR4*)S.v;
    while (pairs-- != 0) {
        u32 y0 = *y++;
        u32 u0 = *u;
        u32 v0 = *v;

        *dest++ = YUY2_PACK_4Y01(y0, YUY2_CHROMA0_U(u0), YUY2_CHROMA0_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_CHROMA1_U(u0), YUY2_CHROMA1_V(v0));

        y0 = *y++;
        u++;
        v++;
        *dest++ = YUY2_PACK_4Y01(y0, YUY2_CHROMA2_U(u0), YUY2_CHROMA2_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_CHROMA3_U(u0), YUY2_CHROMA3_V(v0));

    }

    if (YUY2_HAS_TAIL_BLOCK(count)) {
        u32 y0 = y[YUY2_PAIR_LUMA_WORD_0];
        u16 u0 = *(const u16 PTR4*)u;
        u16 v0 = *(const u16 PTR4*)v;

        *dest++ = YUY2_PACK_4Y01(y0, YUY2_TAIL_CHROMA0_U(u0), YUY2_TAIL_CHROMA0_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_TAIL_CHROMA1_U(u0), YUY2_TAIL_CHROMA1_V(v0));
    }

    S.dest0 += YUY2_ROW_BYTES(count);
    S.dest1 += YUY2_ROW_BYTES(count);
    S.y0 += count;
    S.y1 += count;
    S.u += count;
    S.v += count;
}

void YUY2_x2_4x2(u32 count)
{
    s32 pairs;
    u32 PTR4* dest;
    const u32 PTR4* y;
    const u32 PTR4* u;
    const u32 PTR4* v;

    dest = (u32 PTR4*)S.dest0;
    y = S.y0;
    u = (const u32 PTR4*)S.u;
    v = (const u32 PTR4*)S.v;
    pairs = YUY2_BLOCK_PAIRS(count);
    while (pairs-- != 0) {
        u32 y0 = *y++;
        u32 u0 = *u;
        u32 v0 = *v;
        u32 chroma;

        u++;
        chroma = YUY2_CHROMA0(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_CHROMA1(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);

        y0 = *y++;
        v++;
        chroma = YUY2_CHROMA2(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_CHROMA3(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);
    }

    if (YUY2_HAS_TAIL_BLOCK(count)) {
        u32 y0 = y[YUY2_PAIR_LUMA_WORD_0];
        u16 u0 = *(const u16 PTR4*)u;
        u16 v0 = *(const u16 PTR4*)v;
        u32 chroma;

        chroma = YUY2_TAIL_CHROMA0(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_TAIL_CHROMA1(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);
    }

    dest = (u32 PTR4*)S.dest1;
    y = S.y1;
    u = (const u32 PTR4*)S.u;
    v = (const u32 PTR4*)S.v;
    pairs = YUY2_BLOCK_PAIRS(count);
    while (pairs-- != 0) {
        u32 y0 = *y++;
        u32 u0 = *u;
        u32 v0 = *v;
        u32 chroma;

        u++;
        chroma = YUY2_CHROMA0(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_CHROMA1(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);

        y0 = *y++;
        v++;
        chroma = YUY2_CHROMA2(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_CHROMA3(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);
    }

    if (YUY2_HAS_TAIL_BLOCK(count)) {
        u32 y0 = y[YUY2_PAIR_LUMA_WORD_0];
        u16 u0 = *(const u16 PTR4*)u;
        u16 v0 = *(const u16 PTR4*)v;
        u32 chroma;

        chroma = YUY2_TAIL_CHROMA0(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_TAIL_CHROMA1(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);
    }

    S.dest0 += YUY2_X2_ROW_BYTES(count);
    S.dest1 += YUY2_X2_ROW_BYTES(count);
    S.y0 += count;
    S.y1 += count;
    S.u += count;
    S.v += count;
}

void YUY2_m_4x2(u32 count)
{
    u32 remaining;
    u32 PTR4* dest;
    const u32 PTR4* y;

    remaining = count - 1;
    dest = (u32 PTR4*)S.dest0;
    y = S.y0;
    if (remaining != YUY2_NO_REMAINING) {
        do {
            u32 y0 = *y++;

            --remaining;
            *dest++ = YUY2_PACK_M4Y01(y0);
            *dest++ = YUY2_PACK_M4Y23(y0);
        } while (remaining != YUY2_NO_REMAINING);
    }

    remaining = count - 1;
    dest = (u32 PTR4*)S.dest1;
    y = S.y1;
    if (remaining != YUY2_NO_REMAINING) {
        do {
            u32 y0 = *y++;

            --remaining;
            *dest++ = YUY2_PACK_M4Y01(y0);
            *dest++ = YUY2_PACK_M4Y23(y0);
        } while (remaining != YUY2_NO_REMAINING);
    }

    S.dest0 += YUY2_ROW_BYTES(count);
    S.dest1 += YUY2_ROW_BYTES(count);
    S.y0 += count;
    S.y1 += count;
}

void YUY2_mx2_4x2(u32 count)
{
    u32 remaining;
    u32 PTR4* dest;
    const u32 PTR4* y;

    remaining = count - 1;
    dest = (u32 PTR4*)S.dest0;
    y = S.y0;
    if (remaining != YUY2_NO_REMAINING) {
        do {
            u32 y0 = *y++;

            --remaining;
            *dest++ = YUY2_PACK_X2Y0(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y1(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y2(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y3(y0, YUY2_NEUTRAL_CHROMA);
        } while (remaining != YUY2_NO_REMAINING);
    }

    remaining = count - 1;
    dest = (u32 PTR4*)S.dest1;
    y = S.y1;
    if (remaining != YUY2_NO_REMAINING) {
        do {
            u32 y0 = *y++;

            --remaining;
            *dest++ = YUY2_PACK_X2Y0(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y1(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y2(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y3(y0, YUY2_NEUTRAL_CHROMA);
        } while (remaining != YUY2_NO_REMAINING);
    }

    S.dest0 += YUY2_X2_ROW_BYTES(count);
    S.dest1 += YUY2_X2_ROW_BYTES(count);
    S.y0 += count;
    S.y1 += count;
}

static void YUY2_4x2Helper(u32 count, u32 PTR4* dest, const u32 PTR4* y, const u32 PTR4* u,
                           const u32 PTR4* v)
{
    s32 pairs;

    pairs = YUY2_BLOCK_PAIRS(count);
    while (pairs-- != 0) {
        u32 y0 = *y++;
        u32 u0 = *u;
        u32 v0 = *v;

        *dest++ = YUY2_PACK_4Y01(y0, YUY2_CHROMA0_U(u0), YUY2_CHROMA0_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_CHROMA1_U(u0), YUY2_CHROMA1_V(v0));

        y0 = *y++;
        u++;
        v++;
        *dest++ = YUY2_PACK_4Y01(y0, YUY2_CHROMA2_U(u0), YUY2_CHROMA2_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_CHROMA3_U(u0), YUY2_CHROMA3_V(v0));

    }

    if (YUY2_HAS_TAIL_BLOCK(count)) {
        u32 y0 = y[YUY2_PAIR_LUMA_WORD_0];
        u16 u0 = *(const u16 PTR4*)u;
        u16 v0 = *(const u16 PTR4*)v;

        *dest++ = YUY2_PACK_4Y01(y0, YUY2_TAIL_CHROMA0_U(u0), YUY2_TAIL_CHROMA0_V(v0));
        *dest++ = YUY2_PACK_4Y23(y0, YUY2_TAIL_CHROMA1_U(u0), YUY2_TAIL_CHROMA1_V(v0));
    }
}

static void YUY2_x2_4x2Helper(u32 count, u32 PTR4* dest, const u32 PTR4* y, const u32 PTR4* u,
                              const u32 PTR4* v)
{
    s32 pairs;

    for (pairs = YUY2_BLOCK_PAIRS(count); pairs != 0; --pairs) {
        u32 u0 = *u;
        u32 v0 = *v;
        u32 y0 = *y++;
        u32 chroma;

        u++;
        chroma = YUY2_CHROMA0(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_CHROMA1(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);

        y0 = *y++;
        v++;
        chroma = YUY2_CHROMA2(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_CHROMA3(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);
    }

    if (YUY2_HAS_TAIL_BLOCK(count)) {
        u32 y0 = y[YUY2_PAIR_LUMA_WORD_0];
        u16 u0 = *(const u16 PTR4*)u;
        u16 v0 = *(const u16 PTR4*)v;
        u32 chroma;

        chroma = YUY2_TAIL_CHROMA0(u0, v0);
        *dest++ = YUY2_PACK_X2Y0(y0, chroma);
        *dest++ = YUY2_PACK_X2Y1(y0, chroma);
        chroma = YUY2_TAIL_CHROMA1(u0, v0);
        *dest++ = YUY2_PACK_X2Y2(y0, chroma);
        *dest++ = YUY2_PACK_X2Y3(y0, chroma);
    }
}

static void YUY2_m_4x2Helper(u32 count, u32 PTR4* dest, const u32 PTR4* y)
{
    u32 remaining = count - 1;

    if (remaining != YUY2_NO_REMAINING) {
        do {
            u32 y0 = *y++;

            --remaining;
            *dest++ = YUY2_PACK_M4Y01(y0);
            *dest++ = YUY2_PACK_M4Y23(y0);
        } while (remaining != YUY2_NO_REMAINING);
    }
}

static void YUY2_mx2_4x2Helper(u32 count, u32 PTR4* dest, const u32 PTR4* y)
{
    u32 remaining = count - 1;

    if (remaining != YUY2_NO_REMAINING) {
        do {
            u32 y0 = *y++;

            --remaining;
            *dest++ = YUY2_PACK_X2Y0(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y1(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y2(y0, YUY2_NEUTRAL_CHROMA);
            *dest++ = YUY2_PACK_X2Y3(y0, YUY2_NEUTRAL_CHROMA);
        } while (remaining != YUY2_NO_REMAINING);
    }
}
