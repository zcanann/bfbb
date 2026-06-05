#include "bink.h"
#include "expand.h"
#include "../bitplane.h"
#include "../dct.h"
#include "../varbits.h"
#include "binkngc.h"

typedef enum BINKBlockLayout
{
    BINK_BLOCK_SHIFT = 3,
    BINK_BLOCK_SIDE = 8,
    BINK_BLOCK_PIXELS = BINK_BLOCK_SIDE * BINK_BLOCK_SIDE,
    BINK_BLOCK_ROW_WORD_0 = 0,
    BINK_BLOCK_ROW_WORD_1 = 1,
    BINK_RUN_BLOCK_LAST_PIXEL = BINK_BLOCK_PIXELS - 1,
    BINK_BLOCK_ROUND_MASK = BINK_BLOCK_SIDE - 1
} BINKBlockLayout;

#define BINK_BLOCK_ROUND(value) (((value) + BINK_BLOCK_ROUND_MASK) & ~BINK_BLOCK_ROUND_MASK)
#define BINK_BLOCK_PATTERN_OFFSET(offset, pitch) \
    (((offset) >> BINK_BLOCK_SHIFT) * (pitch) + ((offset) & BINK_BLOCK_ROUND_MASK))
#define BINK_BLOCK_ODD_ROW(row) (((row) & BINK_BLOCK_SIDE) != 0)
typedef enum BINKPlaneLayout
{
    BINK_BUNDLE_WIDTH = 0x200,
    BINK_CHROMA_SHIFT = 1,
    BINK_LUMA_PLANE_SCALE = 1,
    BINK_CHROMA_PLANE_SCALE = 2,
    BINK_COLOR_BLOCK_BYTES = BINK_BLOCK_PIXELS,
    BINK_PATTERN_BLOCK_BYTES = BINK_BLOCK_SIDE,
    BINK_PATTERN_COLOR_0 = 0,
    BINK_PATTERN_COLOR_1 = 1,
    BINK_PATTERN_COLOR_BIT = 1,
    BINK_PATTERN_COLOR_SHIFT = 1,
    BINK_PATTERN_COLOR_COUNT = 2,
    BINK_RUN_BLOCK_BYTES = 0x30
} BINKPlaneLayout;

#define BINK_CHROMA_ROUND(value) (((value) + 1) >> BINK_CHROMA_SHIFT)
typedef enum BINKBundleBitWidths
{
    BINK_BLOCK_TYPE_BITS = 4,
    BINK_COLOR_BITS = 8,
    BINK_PATTERN_BITS = 8,
    BINK_MOTION_BITS = 5,
    BINK_DC_START_BITS = 11,
    BINK_RUN_BITS = 4
} BINKBundleBitWidths;

typedef BITSTYPE EXPBITSTYPE;

typedef enum BINKExpandBitLayout
{
    EXP_BITS_PER_WORD = BITSTYPELEN,
    EXP_LAST_BIT_INDEX = EXP_BITS_PER_WORD - 1,
    EXP_WORD_BYTES = BITSTYPEBYTES,
    EXP_U16_MASK = 0xffff,
    EXP_BIT_MASK = 1
} BINKExpandBitLayout;

typedef enum BINKHuff4Layout
{
    HUFF4_SYMBOLS = 16,
    HUFF4_IDENTITY_CODEBOOK = 0,
    HUFF4_SYMBOL_MASK = 0xf,
    HUFF4_SYMBOL_PRESENT_BIT = 1,
    HUFF4_ALL_SYMBOLS_MASK = 0xffff,
    HUFF4_NIBBLE_BITS = 4,
    HUFF4_SORT_MODE_BITS = 2,
    HUFF4_EXPLICIT_INDEX_BITS = 3,
    HUFF4_PAIR_COUNT = 8,
    HUFF4_PAIR_SYMBOLS = 2,
    HUFF4_QUARTER_SYMBOLS = 4,
    HUFF4_HALF_SYMBOLS = 8,
    HUFF4_LAST_QUARTER_SYMBOL = HUFF4_QUARTER_SYMBOLS * 3,
    HUFF4_LAST_PAIR_SYMBOL = HUFF4_SYMBOLS - HUFF4_PAIR_SYMBOLS,
    HUFF4_DECODE_1BYTE_SIZE = HUFF4_SYMBOLS,
    HUFF4_DECODE_2BYTE_SIZE = HUFF4_SYMBOLS * 2,
    HUFF4_DECODE_4BYTE_SIZE = HUFF4_SYMBOLS * 4,
    HUFF4_DECODE_8BYTE_SIZE = HUFF4_SYMBOLS * 8,
    HUFF4_MASK_4BYTE_SIZE = HUFF4_SYMBOLS * 4,
    HUFF4_MASK_1BYTE_SIZE = HUFF4_SYMBOLS,
    HUFF4_MERGE_PAIR_SIZE = 4,
    HUFF4_RLE_LITERAL_COUNT = 12,
    HUFF4_RLE_FIRST_RUN_SYMBOL = HUFF4_RLE_LITERAL_COUNT
} BINKHuff4Layout;

typedef enum BINKHuff4SortMode
{
    HUFF4_SORT_PAIRS,
    HUFF4_SORT_QUARTERS,
    HUFF4_SORT_HALVES,
    HUFF4_SORT_FULL
} BINKHuff4SortMode;

#define HUFF4_CODE_USED(code) ((code) >> HUFF4_NIBBLE_BITS)
#define HUFF4_CODE_SYMBOL(code) ((code) & HUFF4_SYMBOL_MASK)
#define HUFF4_CODE_SYM(code, syms) ((syms)[HUFF4_CODE_SYMBOL(code)])
typedef enum BINKBundleLayout
{
    HUFF8_TABLE_STATES = 16,
    HUFF8_LAST_TABLE_STATE = HUFF8_TABLE_STATES - 1,
    BUNDLE_REPEAT_EXTRA = 0x14,
    BUNDLE_REPEAT_THRESHOLD = BUNDLE_REPEAT_EXTRA + 2,
    BINK_BUNDLE_BYTE_PITCH = 1,
    BINK_SIGNED_BYTE_BIAS = 0x80,
    BINK_SIGNED_BYTE_MASK = 0x7f,
    BINK_DELTA16_GROUP_MAX = 8,
    BINK_BUNDLE_MIN_BYTE_BITS = 8,
    BINK_BUNDLE_MIN_WORD_BITS = 16,
    BINK_BYTE_BITS = 8,
    BINK_WORD_ALIGN_MASK = 3,
    BINK_PLANE_WORD_BYTES = sizeof(u32),
    BINK_WORK_BLOCK_SPAN = BINK_BLOCK_SIDE * BINK_CHROMA_PLANE_SCALE,
    BINK_WORK_BLOCK_MARKED = 1,
    BINK_DC_BYTES = sizeof(s16),
    BINK_RESIDUE_LIMIT_BITS = 7,
    BINK_DCT_QUANT_BITS = 4,
    BINK_DCT_PATTERN_BITS = 4
} BINKBundleLayout;

#define BINK_BLOCK_ROWS(rows) ((rows) >> BINK_BLOCK_SHIFT)
#define BINK_BUNDLE_COUNT_BASE(rows, pitch) (BINK_BLOCK_ROWS(rows) * (pitch) - 1)
#define BINK_BUNDLE_COUNT_BITS(width, count_base) \
    getbitlevelvar(((width) + (count_base)) & EXP_U16_MASK)
#define BINK_BUNDLE_INITIAL_VALUE(shift) (1 << ((shift) - 1))
typedef enum BINKBUNDLEINITIALVALUEFLAG
{
    BINK_BUNDLE_NO_INITIAL_VALUE,
    BINK_BUNDLE_USE_INITIAL_VALUE
} BINKBUNDLEINITIALVALUEFLAG;
typedef enum BINKBUNDLEINITIALVALUE
{
    BINK_BUNDLE_INITIAL_VALUE_NONE = 0
} BINKBUNDLEINITIALVALUE;
#define BINK_BUNDLE_STORAGE_SIZE(width, rows, bits, pitch) \
    ((((width) * (bits)) >> BINK_BLOCK_SHIFT) + \
     ((BINK_BLOCK_ROWS(rows) * (pitch) * (bits)) >> BINK_BLOCK_SHIFT))
#define BINK_BUNDLE_ALIGN_SIZE(size) (((size) + BINK_WORD_ALIGN_MASK) & ~BINK_WORD_ALIGN_MASK)
#define BINK_BUNDLE_EMPTY_CUR(bundle) ((bundle)->data + EXP_WORD_BYTES)
#define BINK_BUNDLE_U8(bundle) (*(bundle).cur_ptr)
#define BINK_BUNDLE_S8(bundle) (*(s8 PTR4*)((bundle).cur_ptr))
#define BINK_BUNDLE_S16(bundle) (*(s16 PTR4*)((bundle).cur_ptr))
#define BINK_BUNDLE_ADVANCE(bundle, bytes) ((bundle).cur_ptr += (bytes))
#define BINK_BUNDLE_CHUNK_BYTE_SIZE(header) (*(header))
#define BINK_BUNDLE_PAYLOAD_CHUNK_BYTE_SIZE(payload) ((payload)[-1])
#define BINK_BUNDLE_CHUNK_NEXT(header) \
    ((u32 PTR4*)((u8 PTR4*)(header) + BINK_BUNDLE_CHUNK_BYTE_SIZE(header)))
#define BINK_BUNDLE_PAYLOAD_NEXT(payload) \
    ((u32 PTR4*)((u8 PTR4*)(payload) + BINK_BUNDLE_PAYLOAD_CHUNK_BYTE_SIZE(payload) - EXP_WORD_BYTES))
#define BINK_MARK_WORK_BLOCK(work_row, work_col) ((work_row)[(work_col) >> BINK_CHROMA_SHIFT] = BINK_WORK_BLOCK_MARKED)
#define BINK_MOTION_SOURCE(old, pitch, mx, my) ((old) + (my) * (s32)(pitch) + (mx))
#define BINK_DCT_PATTERN_SCAN(pattern) (patterns + (pattern) * BINK_BLOCK_PIXELS)
#define BINK_FILL_WORD(value) \
    ((value) | ((value) << BINK_BYTE_BITS) | ((value) << (BINK_BYTE_BITS * 2)) | ((value) << (BINK_BYTE_BITS * 3)))
#define BINK_BLOCK_ROW_WORD(ptr, pitch, row, word) \
    (*(u32 PTR4*)((ptr) + (pitch) * (row) + (word) * BINK_PLANE_WORD_BYTES))
#define BINK_LINEAR_BLOCK_ROW_WORD(ptr, row, word) \
    (*(u32 PTR4*)((ptr) + (row) * BINK_BLOCK_SIDE + (word) * BINK_PLANE_WORD_BYTES))
#define BINK_HUFF4_RLE_LENGTH(value) \
    ((u8 PTR4*)&BINK_HUFF4_RLE_LENGTHS_PACKED)[(value)]
#define BINK_BUNDLE_REPEAT_COUNT(count) (-(s32)(count) - BUNDLE_REPEAT_EXTRA)
#define BINK_BUNDLE_REPEAT_FILL_COUNT(remaining) (-(remaining + BUNDLE_REPEAT_EXTRA + 1))
#define BINK_SIGNED_BYTE_NEGATIVE(value) (-BINK_SIGNED_BYTE_BIAS - ((value) & BINK_SIGNED_BYTE_MASK))
#define BINK_SIGNED_BYTE_POSITIVE(value) ((value) | BINK_SIGNED_BYTE_BIAS)

enum BINKBLOCKTYPE
{
    BINK_BLOCK_SKIP,    /* Copy the previous-frame 8x8 block. */
    BINK_BLOCK_SCALED,  /* Decode a 16x16 block through the subblock bundle. */
    BINK_BLOCK_MOTION,  /* Copy an 8x8 block from the previous frame with motion offsets. */
    BINK_BLOCK_RUN,     /* Fill in scan-order color runs. */
    BINK_BLOCK_RESIDUE, /* Motion block plus residue bitplane coefficients. */
    BINK_BLOCK_INTRA,   /* Intra DCT block. */
    BINK_BLOCK_FILL,    /* Solid color block. */
    BINK_BLOCK_INTER,   /* Motion-compensated inter DCT block. */
    BINK_BLOCK_PATTERN, /* Two-color pattern block. */
    BINK_BLOCK_RAW      /* Uncoded 8x8 color block. */
};

const double BINK_HUFF4_DECODE0_ALIGN = 0.0;
const double BINK_HUFF4_DECODE_TABLES_ALIGN = 0.0;
static const u8 huff4decode00[HUFF4_DECODE_1BYTE_SIZE] = "@ABCDEFGHIJKLMNO";
static const u8 huff4decode01[HUFF4_DECODE_2BYTE_SIZE] =
    "\20A\20R\20S\20T\20U\20V\20W\20X\20A\20Y\20Z\20[\20\\\20]\20^\20_";
static const u8 huff4decode02[HUFF4_DECODE_2BYTE_SIZE] = " B!X T!\\ C!Z V!^ B!Y U!] C![ W!_";
static const u8 huff4decode03[HUFF4_DECODE_2BYTE_SIZE] = " C1X E2\\ D1Z V2^ C1Y E2] D1[ W2_";
static const u8 huff4decode04[HUFF4_DECODE_2BYTE_SIZE] = "0D2X1F3\\0E2Z1G3^0D2Y1F3]0E2[1G3_";
static const u8 huff4decode05[HUFF4_DECODE_2BYTE_SIZE] = "0FBJ1HD\\0GCK1IE^0FBJ1HD]0GCK1IE_";
static const u8 huff4decode06[HUFF4_DECODE_2BYTE_SIZE] = " EAI GC\\ FBZ HD^ EAI GC] FB[ HD_";
static const u8 huff4decode07[HUFF4_DECODE_4BYTE_SIZE] =
    "1S2h1U2l1T2j1f2n1S2i1U2m1T2k1g2o";
static const u8 huff4decode08[HUFF4_DECODE_4BYTE_SIZE] =
    "\20!\20R\20!\20h\20!\20d\20!\20l\20!\20S\20!\20j\20!\20f\20!\20n\20!\20R\20!\20i\20!\20e\20!\20m\20!\20S\20!\20k\20!\20g\20!\20o";
static const u8 huff4decode09[HUFF4_DECODE_4BYTE_SIZE] =
    "1TBh1VCl1UBj1WCn1TBi1VCm1UBk1WCo";
static const u8 huff4decode10[HUFF4_DECODE_4BYTE_SIZE] =
    " 2!U C!Y 2!W D!l 2!V C!j 2!X D!n 2!U C!Y 2!W D!m 2!V C!k 2!X D!o";
static const u8 huff4decode11[HUFF4_DECODE_4BYTE_SIZE] =
    "\20A\20U\20C\20Y\20B\20W\20D\20l\20A\20V\20C\20j\20B\20X\20D\20n\20A\20U\20C\20Y\20B\20W\20D\20m\20A\20V\20C\20k\20B\20X\20D\20o";
static const u8 huff4decode12[HUFF4_DECODE_4BYTE_SIZE] =
    " \"!S \"!h \"!U \"!l \"!T \"!j \"!f \"!n \"!S \"!i \"!U \"!m \"!T \"!k \"!g \"!o";
static const u8 huff4decode13[HUFF4_DECODE_8BYTE_SIZE] =
    "132d132x132f132|132e132z132g132~132d132y132f132}132e132{132g132";
static const u8 huff4decode14[HUFF4_DECODE_8BYTE_SIZE] =
    "132T132x132e132|132T132z132v132~132T132y132e132}132T132{132w132";
static const u8 huff4decode15[HUFF4_DECODE_8BYTE_SIZE] =
    " 2!4 3!e 2!4 3!i 2!4 3!g 2!4 3!| 2!4 3!f 2!4 3!z 2!4 3!h 2!4 3!~ 2!4 3!e 2!4 3!i 2!4 3!g 2!4 3!} 2!4 3!f 2!4 3!{ 2!4 3!h 2!4 3!";
static const u8 PTR4* huff4decodes[HUFF4_SYMBOLS] = {
    huff4decode00, huff4decode01, huff4decode02, huff4decode03,
    huff4decode04, huff4decode05, huff4decode06, huff4decode07,
    huff4decode08, huff4decode09, huff4decode10, huff4decode11,
    huff4decode12, huff4decode13, huff4decode14, huff4decode15,
};
static const u8 BINK_HUFF4_BITS_TO_PEEK[HUFF4_SYMBOLS] = "\4\5\5\5\5\5\5\6\6\6\6\6\6\7\7\7";
static const u8 mask2[HUFF4_MASK_4BYTE_SIZE] =
    "\0\0\0\0\377\0\0\0\0\377\0\0\377\377\0\0\0\0\377\0\377\0\377\0\0\377\377\0\377\377\377\0\0\0\0\377\377\0\0\377\0\377\0\377\377\377\0\377\0\0\377\377\377\0\377\377\0\377\377\377\377\377\377\377";
static const u8 mask1[HUFF4_MASK_4BYTE_SIZE] =
    "\377\377\377\377\0\377\377\377\377\0\377\377\0\0\377\377\377\377\0\377\0\377\0\377\377\0\0\377\0\0\0\377\377\377\377\0\0\377\377\0\377\0\377\0\0\0\377\0\377\377\0\0\0\377\0\0\377\0\0\0\0\0\0\0";
static const u8 mask4[HUFF4_MASK_1BYTE_SIZE] = "\0\0\0\0\377\377\0\0\0\0\377\377\377\377\377\377";
static const u8 mask3[HUFF4_MASK_1BYTE_SIZE] = "\377\377\377\377\0\0\377\377\377\377\0\0\0\0\0\0";
/* Huff4 symbols 12..15 expand to repeated block-type runs of 4, 8, 12, and 32. */
const u32 BINK_HUFF4_RLE_LENGTHS_PACKED = 0x04080c20;

typedef struct READBUNDLE
{
    u8 PTR4* cur_ptr;    /* Next decoded symbol to be consumed. */
    u8 PTR4* cur_dec;    /* End of the currently decoded symbols. */
    u32 bit_size;      /* Bits per direct bundle element. */
    u32 initial_value;
    u8 syms[HUFF4_SYMBOLS]; /* Huffman leaf-to-symbol translation list. */
    u32 bits_to_peek;
    const u8 PTR4* decode;
    u32 len;         /* Bits used to read the next decoded-element count. */
    u8 PTR4* data;
} READBUNDLE;

typedef struct HUFF8TABLE
{
    u8 syms[HUFF8_TABLE_STATES][HUFF4_SYMBOLS];
    u32 bits_to_peek[HUFF8_TABLE_STATES];
    const u8 PTR4* decode[HUFF8_TABLE_STATES];
    /* Last decoded high nibble selects the next color high-nibble codebook. */
    u32 lastval;
} HUFF8TABLE;

typedef struct HUFF4MERGES
{
    u8 merge01[HUFF4_MERGE_PAIR_SIZE];
    u8 merge23[HUFF4_MERGE_PAIR_SIZE];
    u8 merge45[HUFF4_MERGE_PAIR_SIZE];
    u8 merge67[HUFF4_MERGE_PAIR_SIZE];
    u8 order[HUFF4_SYMBOLS];
} HUFF4MERGES;

typedef VARBITS EXPBITS;

static void ReadHuffTable(EXPBITS PTR4* bits, const u8 PTR4* PTR4* decode,
                          u32 PTR4* bits_to_peek, u8 PTR4* syms);

static void OpenReadBundle(u8 PTR4* bits, READBUNDLE PTR4* bundle, s32 bundle_width,
                           u32 rows, s32 bit_size, s32 element_pitch,
                           BINKBUNDLEINITIALVALUEFLAG use_initial_value)
{
    u32 count_base;
    u32 len;

    bundle->bit_size = bit_size;
    bundle->cur_ptr = 0;
    bundle->cur_dec = 0;
    count_base = BINK_BUNDLE_COUNT_BASE(rows, element_pitch);
    len = BINK_BUNDLE_COUNT_BITS(bundle_width, count_base);
    bundle->len = len;
    if (use_initial_value) {
        bundle->initial_value = BINK_BUNDLE_INITIAL_VALUE(bit_size);
    } else {
        bundle->initial_value = BINK_BUNDLE_INITIAL_VALUE_NONE;
    }
    bundle->data = bits;
}

static inline u32 exp_get_bits(EXPBITS PTR4* bits, u32 count)
{
    u32 bitcount;
    EXPBITSTYPE bitbuf;
    EXPBITSTYPE word;
    u32 mask;

    mask = GetBitsLen(count);
    bitcount = bits->bitlen;
    if (bitcount > count - 1) {
        bitbuf = bits->bits;
        bits->bitlen = bitcount - count;
        bits->bits = bitbuf >> count;
    } else {
        bitbuf = bits->bits;
        word = *bits->cur++;
        bits->bitlen = bitcount + EXP_BITS_PER_WORD - count;
        bits->bits = word >> (count - bitcount);
        bitbuf |= word << bitcount;
    }

    return bitbuf & mask;
}

static inline u32 exp_get_bit(EXPBITS PTR4* bits)
{
    u32 bitcount;
    EXPBITSTYPE bitbuf;

    bitcount = bits->bitlen;
    if (bitcount != 0) {
        bitbuf = bits->bits;
        bits->bitlen = bitcount - 1;
        bits->bits = bitbuf >> 1;
    } else {
        bitbuf = *bits->cur++;
        bits->bitlen = EXP_LAST_BIT_INDEX;
        bits->bits = bitbuf >> 1;
    }

    return bitbuf & EXP_BIT_MASK;
}

static void simpmergesort(EXPBITS PTR4* bits, u8 PTR4* out, u8 PTR4* left,
                          u8 PTR4* right, s32 right_count)
{
    u8 selected;
    s32 left_count;

    left_count = right_count;
    for (;;) {
        if (exp_get_bit(bits)) {
            selected = *right++;
            right_count--;
        } else {
            selected = *left++;
            left_count--;
        }

        *out++ = selected;
        if (left_count == 0) {
            break;
        }
        if (right_count == 0) {
            break;
        }
    }

    if (left_count != 0) {
        while (left_count != 0) {
            *out++ = *left++;
            left_count--;
        }
    } else {
        while (right_count != 0) {
            *out++ = *right++;
            right_count--;
        }
    }
}

static inline u32 exp_read_huff4(EXPBITS PTR4* bits, u32 bits_to_peek,
                                 const u8 PTR4* decode, u8 PTR4* syms)
{
    u32 bitcount;
    EXPBITSTYPE bitbuf;
    EXPBITSTYPE word;
    u32 mask;
    u8 code;
    u32 used;
    u32 symbol;

    bitcount = bits->bitlen;
    mask = GetBitsLen(bits_to_peek);
    if (bitcount >= bits_to_peek) {
        bitbuf = bits->bits & mask;
        code = decode[bitbuf];
        used = HUFF4_CODE_USED(code);
        symbol = HUFF4_CODE_SYM(code, syms);
        bits->bits >>= used;
        bits->bitlen = bitcount - used;
    } else {
        word = *bits->cur;
        bitbuf = (bits->bits | (word << bitcount)) & mask;
        code = decode[bitbuf];
        used = HUFF4_CODE_USED(code);
        symbol = HUFF4_CODE_SYM(code, syms);
        if (bitcount >= used) {
            bits->bits >>= used;
            bits->bitlen = bitcount - used;
        } else {
            bits->bits = word >> (used - bitcount);
            bits->bitlen = bitcount + EXP_BITS_PER_WORD - used;
            bits->cur++;
        }
    }

    return symbol;
}

static inline void exp_read_huff4_store(EXPBITS PTR4* bits, u32 bits_to_peek,
                                        const u8 PTR4* decode, u8 PTR4* syms,
                                        u8 PTR4* dest)
{
    u32 bitcount;
    EXPBITSTYPE bitbuf;
    EXPBITSTYPE word;
    u32 mask;
    u8 code;
    u32 used;

    bitcount = bits->bitlen;
    mask = GetBitsLen(bits_to_peek);
    if (bitcount >= bits_to_peek) {
        bitbuf = bits->bits & mask;
        code = decode[bitbuf];
        used = HUFF4_CODE_USED(code);
        *dest = (u8)HUFF4_CODE_SYM(code, syms);
        bits->bits >>= used;
        bits->bitlen = bitcount - used;
    } else {
        word = *bits->cur;
        bitbuf = (bits->bits | (word << bitcount)) & mask;
        code = decode[bitbuf];
        used = HUFF4_CODE_USED(code);
        *dest = (u8)HUFF4_CODE_SYM(code, syms);
        if (bitcount >= used) {
            bits->bits >>= used;
            bits->bitlen = bitcount - used;
        } else {
            bits->bits = word >> (used - bitcount);
            bits->bitlen = bitcount + EXP_BITS_PER_WORD - used;
            bits->cur++;
        }
    }
}

static inline u32 exp_read_huff4_mask(EXPBITS PTR4* bits, u32 bits_to_peek,
                                      const u8 PTR4* decode, u8 PTR4* syms,
                                      u32 mask)
{
    u32 bitcount;
    EXPBITSTYPE bitbuf;
    EXPBITSTYPE word;
    u8 code;
    u32 used;
    u32 symbol;

    bitcount = bits->bitlen;
    if (bitcount >= bits_to_peek) {
        bitbuf = bits->bits & mask;
        code = decode[bitbuf];
        used = HUFF4_CODE_USED(code);
        symbol = HUFF4_CODE_SYM(code, syms);
        bits->bits >>= used;
        bits->bitlen = bitcount - used;
    } else {
        word = *bits->cur;
        bitbuf = (bits->bits | (word << bitcount)) & mask;
        code = decode[bitbuf];
        used = HUFF4_CODE_USED(code);
        symbol = HUFF4_CODE_SYM(code, syms);
        if (bitcount >= used) {
            bits->bits >>= used;
            bits->bitlen = bitcount - used;
        } else {
            bits->bits = word >> (used - bitcount);
            bits->bitlen = bitcount + EXP_BITS_PER_WORD - used;
            bits->cur++;
        }
    }

    return symbol;
}

static inline u32 exp_read_huff8(EXPBITS PTR4* bits, u32 state, HUFF8TABLE PTR4* table)
{
    return exp_read_huff4(bits, table->bits_to_peek[state], table->decode[state],
                          table->syms[state]);
}

static void ReadHuffTable(EXPBITS PTR4* vb, const u8 PTR4* PTR4* decode,
                          u32 PTR4* bits_to_peek, u8 PTR4* syms)
{
    u32 vlc_num;
    u32 sort_mode;
    u32 last_explicit;
    u32 count;
    u32 unused_symbols;
    u32 fill_symbol;
    u32 symbol;
    u32 j;
    u32 i;
    HUFF4MERGES merges;

    /* Each table stores a 4-bit codebook index plus a 16-entry symbol remap. */
    VarBitsGet(vlc_num, u32, *vb, HUFF4_NIBBLE_BITS);
    *decode = huff4decodes[vlc_num];
    *bits_to_peek = (u8)BINK_HUFF4_BITS_TO_PEEK[vlc_num];
    if (vlc_num == HUFF4_IDENTITY_CODEBOOK) {
        for (j = 0; j < HUFF4_SYMBOLS; ++j) {
            syms[j] = j;
        }
        return;
    }

    if (exp_get_bit(vb) == 0) {
        /* Compact symbol shuffling: merge adjacent pair, quarter, and half lists. */
        VarBitsGet(sort_mode, u32, *vb, HUFF4_SORT_MODE_BITS);
        if (sort_mode == HUFF4_SORT_PAIRS) {
            i = 0;
            count = HUFF4_PAIR_COUNT;
            do {
                u32 swap_pair = exp_get_bit(vb);

                if (swap_pair != 0) {
                    syms[i + 1] = i;
                    syms[i] = i + 1;
                } else {
                    syms[i] = i;
                    syms[i + 1] = i + 1;
                }
                i += HUFF4_PAIR_SYMBOLS;
                count--;
            } while (count != 0);
        } else {
            u32 left;
            u32 right;

            left = 0;
            right = 1;
            for (i = 0; i < HUFF4_PAIR_COUNT; ++i) {
                u32 swap_pair = exp_get_bit(vb);

                if (swap_pair != 0) {
                    merges.order[left] = right;
                    merges.order[right] = left;
                } else {
                    merges.order[left] = left;
                    merges.order[right] = right;
                }
                left += HUFF4_PAIR_SYMBOLS;
                right += HUFF4_PAIR_SYMBOLS;
            }

            if (sort_mode == HUFF4_SORT_QUARTERS) {
                simpmergesort(vb, syms, merges.order,
                              merges.order + HUFF4_PAIR_SYMBOLS, HUFF4_PAIR_SYMBOLS);
                simpmergesort(vb, syms + HUFF4_QUARTER_SYMBOLS,
                              merges.order + HUFF4_QUARTER_SYMBOLS,
                              merges.order + HUFF4_QUARTER_SYMBOLS + HUFF4_PAIR_SYMBOLS,
                              HUFF4_PAIR_SYMBOLS);
                simpmergesort(vb, syms + HUFF4_HALF_SYMBOLS,
                              merges.order + HUFF4_HALF_SYMBOLS,
                              merges.order + HUFF4_HALF_SYMBOLS + HUFF4_PAIR_SYMBOLS,
                              HUFF4_PAIR_SYMBOLS);
                simpmergesort(vb, syms + HUFF4_LAST_QUARTER_SYMBOL,
                              merges.order + HUFF4_LAST_QUARTER_SYMBOL,
                              merges.order + HUFF4_LAST_PAIR_SYMBOL, HUFF4_PAIR_SYMBOLS);
            } else {
                simpmergesort(vb, merges.merge01, merges.order,
                              merges.order + HUFF4_PAIR_SYMBOLS,
                              HUFF4_PAIR_SYMBOLS);
                simpmergesort(vb, merges.merge23,
                              merges.order + HUFF4_QUARTER_SYMBOLS,
                              merges.order + HUFF4_QUARTER_SYMBOLS + HUFF4_PAIR_SYMBOLS,
                              HUFF4_PAIR_SYMBOLS);
                simpmergesort(vb, merges.merge45, merges.order + HUFF4_HALF_SYMBOLS,
                              merges.order + HUFF4_HALF_SYMBOLS + HUFF4_PAIR_SYMBOLS,
                              HUFF4_PAIR_SYMBOLS);
                simpmergesort(vb, merges.merge67,
                              merges.order + HUFF4_LAST_QUARTER_SYMBOL,
                              merges.order + HUFF4_LAST_PAIR_SYMBOL, HUFF4_PAIR_SYMBOLS);
                if (sort_mode == HUFF4_SORT_HALVES) {
                    simpmergesort(vb, syms, merges.merge01, merges.merge23,
                                  HUFF4_QUARTER_SYMBOLS);
                    simpmergesort(vb, syms + HUFF4_HALF_SYMBOLS,
                                  merges.merge45, merges.merge67, HUFF4_QUARTER_SYMBOLS);
                } else {
                    simpmergesort(vb, merges.order, merges.merge01, merges.merge23,
                                  HUFF4_QUARTER_SYMBOLS);
                    simpmergesort(vb, merges.order + HUFF4_HALF_SYMBOLS,
                                  merges.merge45, merges.merge67, HUFF4_QUARTER_SYMBOLS);
                    simpmergesort(vb, syms, merges.order,
                                  merges.order + HUFF4_HALF_SYMBOLS,
                                  HUFF4_HALF_SYMBOLS);
                }
            }
        }
    } else {
        VarBitsGet(last_explicit, u32, *vb, HUFF4_EXPLICIT_INDEX_BITS);
        unused_symbols = HUFF4_ALL_SYMBOLS_MASK;
        for (i = 0; i <= last_explicit; ++i) {
            VarBitsGet(symbol, u32, *vb, HUFF4_NIBBLE_BITS);
            syms[i] = symbol;
            unused_symbols &= ~(HUFF4_SYMBOL_PRESENT_BIT << symbol);
        }

        fill_symbol = 0;
        do {
            if ((unused_symbols & HUFF4_SYMBOL_PRESENT_BIT) != 0) {
                last_explicit++;
                syms[last_explicit] = fill_symbol;
            }
            fill_symbol++;
            unused_symbols >>= 1;
        } while (unused_symbols != 0);
    }
}

static void StartReadHuff4Bundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits)
{
    ReadHuffTable(bits, &bundle->decode, &bundle->bits_to_peek, bundle->syms);
}

static void StartReadHuff8Bundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits,
                                 HUFF8TABLE PTR4* huff8_table)
{
    u32 PTR4* codes;
    const u8 PTR4* PTR4* huff_table;
    u8 PTR4* cur;
    u8 PTR4* end;

    cur = huff8_table->syms[0];
    end = huff8_table->syms[HUFF8_LAST_TABLE_STATE];
    codes = huff8_table->bits_to_peek;
    huff_table = huff8_table->decode;
    do {
        ReadHuffTable(bits, huff_table, codes, cur);
        cur += HUFF4_SYMBOLS;
        ++codes;
        ++huff_table;
    } while (cur <= end);
    ReadHuffTable(bits, &bundle->decode, &bundle->bits_to_peek, bundle->syms);
    huff8_table->lastval = 0;
}

static void CheckReadRLEHuff4Bundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits)
{
    u32 count;
    u8 PTR4* dest;
    u32 symbol;
    u32 last_value;
    u8 run_length;
    u32 fill;
    u8 PTR4* syms;
    const u8 PTR4* decode;
    u32 peek;

    if (bundle->cur_ptr != bundle->cur_dec) {
        return;
    }

    VarBitsGet(count, u32, *bits, bundle->len);
    if (count != 0) {
        bundle->cur_ptr = bundle->data;
        bundle->cur_dec = bundle->data + count;
        if (exp_get_bit(bits) == 0) {
            /* Literal Huff4 symbols above 11 repeat the previous decoded symbol. */
            syms = bundle->syms;
            peek = (u8)bundle->bits_to_peek;
            dest = bundle->data;
            last_value = 0;
            decode = bundle->decode;
            while (count != 0) {
                symbol = exp_read_huff4(bits, peek, decode, syms);
                if (symbol >= HUFF4_RLE_FIRST_RUN_SYMBOL) {
                    /* Packed word stores four copies of the last byte for the run fill. */
                    fill = last_value | (last_value << BINK_BYTE_BITS);
                    symbol -= HUFF4_RLE_LITERAL_COUNT;
                    run_length = BINK_HUFF4_RLE_LENGTH(symbol);
                    count -= run_length;
                    fill |= fill << BINK_BUNDLE_MIN_WORD_BITS;
                    do {
                        *(u32 PTR4*)dest = fill;
                        dest += EXP_WORD_BYTES;
                        run_length -= EXP_WORD_BYTES;
                    } while (run_length != 0);
                } else {
                    *dest++ = (u8)symbol;
                    count--;
                    last_value = symbol;
                }
            }
        } else {
            fill = exp_get_bits(bits, HUFF4_NIBBLE_BITS);
            memset(bundle->data, fill, count);
        }
    } else {
        /* Empty bundles point cur_ptr past data so callers see no decoded elements. */
        bundle->cur_dec = bundle->data;
        bundle->cur_ptr = BINK_BUNDLE_EMPTY_CUR(bundle);
    }
}

static void CheckReadHuff8Bundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits,
                                 HUFF8TABLE PTR4* huff8_table)
{
    u32 count;
    u8 PTR4* dest;
    u8 PTR4* syms;
    const u8 PTR4* decode;
    u32 peek;
    u32 mask;
    u32 lastval;
    u32 high;
    u32 low;
    u32 packed;
    u32 signed_byte;
    s32 remaining;

    if (bundle->cur_ptr != bundle->cur_dec) {
        return;
    }

    VarBitsGet(count, u32, *bits, bundle->len);
    if (count != 0) {
        bundle->cur_ptr = bundle->data;
        bundle->cur_dec = bundle->data + count;
        dest = bundle->data;
        syms = bundle->syms;
        decode = bundle->decode;
        peek = bundle->bits_to_peek;
        lastval = huff8_table->lastval;
        if (exp_get_bit(bits) != 0) {
            /* Negative remaining marks the old-format repeat packet variant. */
            count = BINK_BUNDLE_REPEAT_COUNT(count);
        }
        mask = GetBitsLen(peek);
        remaining = (s32)count;
        do {
            high = exp_read_huff8(bits, lastval, huff8_table);
            lastval = high;
            low = exp_read_huff4_mask(bits, peek, decode, syms, mask);
            packed = ((high & HUFF4_SYMBOL_MASK) << HUFF4_NIBBLE_BITS) | low;
            signed_byte = packed;
            if ((signed_byte & BINK_SIGNED_BYTE_BIAS) != 0) {
                signed_byte = BINK_SIGNED_BYTE_NEGATIVE(signed_byte);
            } else {
                signed_byte = BINK_SIGNED_BYTE_POSITIVE(signed_byte);
            }
            *dest++ = (u8)signed_byte;
            remaining--;
        } while (remaining > 0);

        if (remaining < -BUNDLE_REPEAT_THRESHOLD) {
            /* Repeat packets back-fill the whole bundle with the first decoded byte. */
            memset(bundle->data, *bundle->data, BINK_BUNDLE_REPEAT_FILL_COUNT(remaining));
        }
        huff8_table->lastval = lastval;
    } else {
        bundle->cur_dec = bundle->data;
        bundle->cur_ptr = BINK_BUNDLE_EMPTY_CUR(bundle);
    }
}

static void NewCheckReadHuff8Bundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits,
                                    HUFF8TABLE PTR4* huff8_table)
{
    u32 count;
    u8 PTR4* dest;
    u8 PTR4* syms;
    const u8 PTR4* decode;
    u32 peek;
    u32 mask;
    u32 lastval;
    u32 low;
    u32 packed;
    s32 remaining;

    if (bundle->cur_ptr != bundle->cur_dec) {
        return;
    }

    VarBitsGet(count, u32, *bits, bundle->len);
    if (count != 0) {
        bundle->cur_ptr = bundle->data;
        bundle->cur_dec = bundle->data + count;
        dest = bundle->data;
        syms = bundle->syms;
        decode = bundle->decode;
        peek = bundle->bits_to_peek;
        lastval = huff8_table->lastval;
        if (exp_get_bit(bits) != 0) {
            /* New-format Huff8 repeat packets keep the byte unsigned. */
            count = BINK_BUNDLE_REPEAT_COUNT(count);
        }
        mask = GetBitsLen(peek);
        remaining = (s32)count;
        do {
            lastval = exp_read_huff8(bits, lastval, huff8_table);
            low = exp_read_huff4_mask(bits, peek, decode, syms, mask);
            packed = low | (lastval << HUFF4_NIBBLE_BITS);
            *dest++ = (u8)packed;
            remaining--;
        } while (remaining > 0);

        if (remaining < -BUNDLE_REPEAT_THRESHOLD) {
            /* Match old-format repeat handling after the one-byte payload is decoded. */
            memset(bundle->data, *bundle->data, BINK_BUNDLE_REPEAT_FILL_COUNT(remaining));
        }
        huff8_table->lastval = lastval;
    } else {
        bundle->cur_dec = bundle->data;
        bundle->cur_ptr = BINK_BUNDLE_EMPTY_CUR(bundle);
    }
}

static void CheckReadHuff4Bundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits)
{
    u32 count;
    u8 PTR4* dest;
    u8 PTR4* syms;
    const u8 PTR4* decode;
    u32 peek;
    u32 fill;

    if (bundle->cur_ptr != bundle->cur_dec) {
        return;
    }

    VarBitsGet(count, u32, *bits, bundle->len);
    if (count != 0) {
        bundle->cur_ptr = bundle->data;
        bundle->cur_dec = bundle->data + count;
        if (exp_get_bit(bits) == 0) {
            /* Direct Huff4 bundles decode one nibble-sized symbol per byte. */
            dest = bundle->data;
            syms = bundle->syms;
            decode = bundle->decode;
            peek = bundle->bits_to_peek;
            while (count != 0) {
                count--;
                exp_read_huff4_store(bits, peek, decode, syms, dest);
                ++dest;
            }
        } else {
            fill = exp_get_bits(bits, HUFF4_NIBBLE_BITS);
            memset(bundle->data, fill, count);
        }
    } else {
        bundle->cur_dec = bundle->data;
        bundle->cur_ptr = BINK_BUNDLE_EMPTY_CUR(bundle);
    }
}

static void CheckReadHuff4PairBundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits)
{
    u32 count;
    u8 PTR4* dest;
    u8 PTR4* syms;
    const u8 PTR4* decode;
    u32 peek;
    u32 mask;
    u32 low;
    u32 high;

    if (bundle->cur_ptr != bundle->cur_dec) {
        return;
    }

    VarBitsGet(count, u32, *bits, bundle->len);
    if (count != 0) {
        dest = bundle->data;
        bundle->cur_ptr = dest;
        bundle->cur_dec = dest + count;
        syms = bundle->syms;
        decode = bundle->decode;
        peek = bundle->bits_to_peek;
        mask = GetBitsLen(peek);
        do {
            /* Pair bundles pack two Huff4 symbols into each output byte. */
            count--;
            low = exp_read_huff4_mask(bits, peek, decode, syms, mask);
            high = exp_read_huff4_mask(bits, peek, decode, syms, mask);
            *dest++ = (u8)(low | (high << HUFF4_NIBBLE_BITS));
        } while (count != 0);
    } else {
        bundle->cur_dec = bundle->data;
        bundle->cur_ptr = BINK_BUNDLE_EMPTY_CUR(bundle);
    }
}

static void CheckReadHuff4SBundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits)
{
    u32 count;
    s8 PTR4* dest;
    u8 PTR4* syms;
    const u8 PTR4* decode;
    u32 peek;
    s32 symbol;
    s32 fill;

    if (bundle->cur_ptr != bundle->cur_dec) {
        return;
    }

    VarBitsGet(count, u32, *bits, bundle->len);
    if (count != 0) {
        bundle->cur_ptr = bundle->data;
        bundle->cur_dec = bundle->data + count;
        if (exp_get_bit(bits) == 0) {
            /* Signed Huff4 bundles store a sign bit only for nonzero symbols. */
            dest = bundle->data;
            syms = bundle->syms;
            decode = bundle->decode;
            peek = bundle->bits_to_peek;
            while (count-- != 0) {
                symbol = (s32)exp_read_huff4(bits, peek, decode, syms);
                if (symbol != 0 && exp_get_bit(bits) != 0) {
                    symbol = -symbol;
                }
                *dest++ = (s8)symbol;
            }
        } else {
            fill = (s32)exp_get_bits(bits, HUFF4_NIBBLE_BITS);
            if (fill != 0 && exp_get_bit(bits) != 0) {
                fill = -fill;
            }
            memset(bundle->data, fill, count);
        }
    } else {
        bundle->cur_dec = bundle->data;
        bundle->cur_ptr = BINK_BUNDLE_EMPTY_CUR(bundle);
    }
}

static void CheckReadDelta16Bundle(READBUNDLE PTR4* bundle, EXPBITS PTR4* bits)
{
    u32 count;
    u32 remaining;
    u32 group_size;
    u32 delta_bits;
    u16 predictor;
    u16 magnitude;
    s16 delta;
    s16 PTR4* dest;

    if (bundle->cur_ptr != bundle->cur_dec) {
        return;
    }

    VarBitsGet(count, u32, *bits, bundle->len);
    if (count != 0) {
        dest = (s16 PTR4*)bundle->data;
        if (bundle->initial_value != BINK_BUNDLE_INITIAL_VALUE_NONE) {
            VarBitsGet(predictor, u16, *bits, bundle->bit_size - 1);
            if (predictor != 0 && exp_get_bit(bits) != 0) {
                predictor = -predictor;
            }
        } else {
            VarBitsGet(predictor, u16, *bits, bundle->bit_size);
        }

        *dest++ = (s16)predictor;
        remaining = count - 1;
        bundle->cur_ptr = bundle->data;
        bundle->cur_dec = bundle->data + count * sizeof(*dest);
        while (remaining != 0) {
            group_size = remaining;
            if (group_size > BINK_DELTA16_GROUP_MAX) {
                group_size = BINK_DELTA16_GROUP_MAX;
            }

            VarBitsGet(delta_bits, u32, *bits, HUFF4_NIBBLE_BITS);
            if (delta_bits != 0) {
                remaining -= group_size;
                while (group_size != 0) {
                    group_size--;
                    VarBitsGet(magnitude, u16, *bits, delta_bits);
                    delta = (s16)magnitude;
                    if (delta != 0 && exp_get_bit(bits) != 0) {
                        delta = (s16)-magnitude;
                    }
                    predictor = predictor + delta;
                    *dest++ = (s16)predictor;
                }
            } else {
                radmemset16(dest, (u16)predictor, group_size * sizeof(*dest));
                dest += group_size;
                remaining -= group_size;
            }
        }
    } else {
        bundle->cur_dec = bundle->data;
        bundle->cur_ptr = BINK_BUNDLE_EMPTY_CUR(bundle);
    }
}

static inline void expand_run_block(u8 PTR4* dest,
                                    u32 pitch,
                                    READBUNDLE PTR4* colors,
                                    READBUNDLE PTR4* runs,
                                    EXPBITS PTR4* bits)
{
    const u8 PTR4* scan;
    u32 filled_pixels;

    scan = BINK_DCT_PATTERN_SCAN(exp_get_bits(bits, BINK_DCT_PATTERN_BITS));
    filled_pixels = 0;
    do {
        u32 run_length;

        run_length = *runs->cur_ptr++ + 1;
        filled_pixels += run_length;
        if (exp_get_bit(bits) != 0) {
            u8 color;

            color = *colors->cur_ptr++;
            do {
                u32 scan_offset;

                scan_offset = *scan++;
                dest[BINK_BLOCK_PATTERN_OFFSET(scan_offset, pitch)] = color;
            } while (--run_length != 0);
        } else {
            do {
                u32 scan_offset;

                scan_offset = *scan++;
                dest[BINK_BLOCK_PATTERN_OFFSET(scan_offset, pitch)] = *colors->cur_ptr++;
            } while (--run_length != 0);
        }
    } while (filled_pixels < BINK_RUN_BLOCK_LAST_PIXEL);

    if (filled_pixels == BINK_RUN_BLOCK_LAST_PIXEL) {
        u32 scan_offset;

        scan_offset = *scan++;
        dest[BINK_BLOCK_PATTERN_OFFSET(scan_offset, pitch)] = *colors->cur_ptr++;
    }
}

static inline void expand_pattern_block(u8 PTR4* dest,
                                        u32 pitch,
                                        READBUNDLE PTR4* colors,
                                        READBUNDLE PTR4* patterns)
{
    u8 color0;
    u8 color1;
    u32 i;

    color0 = colors->cur_ptr[BINK_PATTERN_COLOR_0];
    color1 = colors->cur_ptr[BINK_PATTERN_COLOR_1];
    colors->cur_ptr += BINK_PATTERN_COLOR_COUNT;
    for (i = 0; i < BINK_BLOCK_SIDE; ++i) {
        u32 row_bits;
        u32 col;

        row_bits = *patterns->cur_ptr++;
        for (col = 0; col < BINK_BLOCK_SIDE; ++col) {
            dest[i * pitch + col] = (row_bits & BINK_PATTERN_COLOR_BIT) != 0 ? color1 : color0;
            row_bits >>= BINK_PATTERN_COLOR_SHIFT;
        }
    }
}

static u32 getbunsize(s32 width, u32 rows, u32 bits, s32 pitch)
{
    if (bits < (BINK_BUNDLE_MIN_BYTE_BITS + 1)) {
        bits = BINK_BUNDLE_MIN_BYTE_BITS;
    } else if (bits < BINK_BUNDLE_MIN_WORD_BITS) {
        bits = BINK_BUNDLE_MIN_WORD_BITS;
    }
    return BINK_BUNDLE_ALIGN_SIZE(BINK_BUNDLE_STORAGE_SIZE(width, rows, bits, pitch));
}

void ExpandBundleSizes(u32 PTR4* sizes, u32 rows)
{
    /* Order matches the Bink value sources: block/subblock types, colors, patterns,
       motion X/Y, intra/inter DC, then run lengths. */
    sizes[BINK_BUNDLE_BLOCK_TYPES] = getbunsize(BINK_BUNDLE_WIDTH, rows, BINK_BLOCK_TYPE_BITS, BINK_BUNDLE_BYTE_PITCH);
    sizes[BINK_BUNDLE_SUBBLOCK_TYPES] =
        getbunsize(BINK_BUNDLE_WIDTH, rows >> BINK_CHROMA_SHIFT, BINK_BLOCK_TYPE_BITS, BINK_BUNDLE_BYTE_PITCH);
    sizes[BINK_BUNDLE_COLORS] =
        getbunsize(BINK_BUNDLE_WIDTH, rows, BINK_COLOR_BITS, BINK_COLOR_BLOCK_BYTES);
    sizes[BINK_BUNDLE_PATTERN] =
        getbunsize(BINK_BUNDLE_WIDTH, rows, BINK_PATTERN_BITS, BINK_PATTERN_BLOCK_BYTES);
    sizes[BINK_BUNDLE_X_OFF] = getbunsize(BINK_BUNDLE_WIDTH, rows, BINK_MOTION_BITS, BINK_BUNDLE_BYTE_PITCH);
    sizes[BINK_BUNDLE_Y_OFF] = getbunsize(BINK_BUNDLE_WIDTH, rows, BINK_MOTION_BITS, BINK_BUNDLE_BYTE_PITCH);
    sizes[BINK_BUNDLE_INTRA_DC] = getbunsize(BINK_BUNDLE_WIDTH, rows, BINK_DC_START_BITS, BINK_BUNDLE_BYTE_PITCH);
    sizes[BINK_BUNDLE_INTER_DC] = getbunsize(BINK_BUNDLE_WIDTH, rows, BINK_DC_START_BITS, BINK_BUNDLE_BYTE_PITCH);
    sizes[BINK_BUNDLE_RUN] =
        getbunsize(BINK_BUNDLE_WIDTH, rows, BINK_RUN_BITS, BINK_RUN_BLOCK_BYTES);
}

static u32 PTR4* ExpandPlane(u8 PTR4* out,
                        u8 PTR4* prev,
                        u32 width,
                        u32 height,
                        u32 pitch,
                        u32 PTR4* bundles,
                        u32 key_frame,
                        u8 PTR4* work,
                        u32 plane,
                        BUNDLEPOINTERS PTR4* table,
                        u32 flags)
{
    READBUNDLE block_types;
    READBUNDLE subblock_types;
    READBUNDLE colors;
    READBUNDLE patterns;
    READBUNDLE xoff;
    READBUNDLE yoff;
    READBUNDLE intra_dc;
    READBUNDLE inter_dc;
    READBUNDLE runs;
    HUFF8TABLE huff8_table;
    s16 dct_block[BINK_BLOCK_PIXELS];
    u8 motion_block[BINK_BLOCK_PIXELS];
    EXPBITS bitstate;
    void (*read_huff8)(READBUNDLE PTR4*, EXPBITS PTR4*, HUFF8TABLE PTR4*);
    u32 row;
    u32 col;
    u32 work_col;
    u32 work_pitch;
    enum BINKBLOCKTYPE block_type;
    enum BINKBLOCKTYPE subblock_type;
    u8 PTR4* dest;
    u8 PTR4* old;
    u8 PTR4* work_row;

    (void)key_frame;

    read_huff8 =
        (flags & BINKOLDFRAMEFORMAT) != 0 ? CheckReadHuff8Bundle : NewCheckReadHuff8Bundle;
    VarBitsOpen(bitstate, bundles);

    OpenReadBundle(table->typeptr, &block_types,
                   BINK_BUNDLE_WIDTH, width, BINK_BLOCK_TYPE_BITS, BINK_BUNDLE_BYTE_PITCH,
                   BINK_BUNDLE_NO_INITIAL_VALUE);
    OpenReadBundle(table->type16ptr, &subblock_types,
                   BINK_BUNDLE_WIDTH, width >> BINK_CHROMA_SHIFT, BINK_BLOCK_TYPE_BITS, BINK_BUNDLE_BYTE_PITCH,
                   BINK_BUNDLE_NO_INITIAL_VALUE);
    OpenReadBundle(table->colorptr, &colors, BINK_BUNDLE_WIDTH, width,
                   BINK_COLOR_BITS, BINK_COLOR_BLOCK_BYTES, BINK_BUNDLE_NO_INITIAL_VALUE);
    OpenReadBundle(table->bits2ptr, &patterns,
                   BINK_BUNDLE_WIDTH, width, BINK_PATTERN_BITS, BINK_PATTERN_BLOCK_BYTES,
                   BINK_BUNDLE_NO_INITIAL_VALUE);
    OpenReadBundle(table->motionXptr, &xoff, BINK_BUNDLE_WIDTH, width,
                   BINK_MOTION_BITS, BINK_BUNDLE_BYTE_PITCH, BINK_BUNDLE_USE_INITIAL_VALUE);
    OpenReadBundle(table->motionYptr, &yoff, BINK_BUNDLE_WIDTH, width,
                   BINK_MOTION_BITS, BINK_BUNDLE_BYTE_PITCH, BINK_BUNDLE_USE_INITIAL_VALUE);
    OpenReadBundle(table->dctptr, &intra_dc, BINK_BUNDLE_WIDTH,
                   width, BINK_DC_START_BITS, BINK_BUNDLE_BYTE_PITCH, BINK_BUNDLE_NO_INITIAL_VALUE);
    OpenReadBundle(table->mdctptr, &inter_dc, BINK_BUNDLE_WIDTH,
                   width, BINK_DC_START_BITS, BINK_BUNDLE_BYTE_PITCH, BINK_BUNDLE_USE_INITIAL_VALUE);
    OpenReadBundle(table->patptr, &runs, BINK_BUNDLE_WIDTH, width,
                   BINK_RUN_BITS, BINK_RUN_BLOCK_BYTES, BINK_BUNDLE_NO_INITIAL_VALUE);

    StartReadHuff4Bundle(&block_types, &bitstate);
    StartReadHuff4Bundle(&subblock_types, &bitstate);
    StartReadHuff8Bundle(&colors, &bitstate, &huff8_table);
    StartReadHuff4Bundle(&patterns, &bitstate);
    StartReadHuff4Bundle(&xoff, &bitstate);
    StartReadHuff4Bundle(&yoff, &bitstate);
    StartReadHuff4Bundle(&runs, &bitstate);

    dest = out;
    old = prev;
    work_row = work;
    work_pitch = pitch / (BINK_WORK_BLOCK_SPAN / plane);
    row = 0;
    if (height != 0) {
    do {
        CheckReadRLEHuff4Bundle(&block_types, &bitstate);
        CheckReadRLEHuff4Bundle(&subblock_types, &bitstate);
        read_huff8(&colors, &bitstate, &huff8_table);
        CheckReadHuff4PairBundle(&patterns, &bitstate);
        CheckReadHuff4SBundle(&xoff, &bitstate);
        CheckReadHuff4SBundle(&yoff, &bitstate);
        CheckReadDelta16Bundle(&intra_dc, &bitstate);
        CheckReadDelta16Bundle(&inter_dc, &bitstate);
        CheckReadHuff4Bundle(&runs, &bitstate);

        col = 0;
        work_col = 0;
        while (col < width) {
            block_type = BINK_BUNDLE_U8(block_types);
            BINK_BUNDLE_ADVANCE(block_types, BINK_BUNDLE_BYTE_PITCH);

            if (BINK_BLOCK_ODD_ROW(row) && block_type == BINK_BLOCK_SCALED) {
                col += BINK_BLOCK_SIDE;
                dest += BINK_BLOCK_SIDE;
                old += BINK_BLOCK_SIDE;
                work_col += plane;
                continue;
            }

            switch (block_type) {
            case BINK_BLOCK_SKIP: {
                u32 block_row;
                for (block_row = 0; block_row < BINK_BLOCK_SIDE; ++block_row) {
                    BINK_BLOCK_ROW_WORD(dest, pitch, block_row, BINK_BLOCK_ROW_WORD_0) =
                        BINK_BLOCK_ROW_WORD(old, pitch, block_row, BINK_BLOCK_ROW_WORD_0);
                    BINK_BLOCK_ROW_WORD(dest, pitch, block_row, BINK_BLOCK_ROW_WORD_1) =
                        BINK_BLOCK_ROW_WORD(old, pitch, block_row, BINK_BLOCK_ROW_WORD_1);
                }
                break;
            }
            case BINK_BLOCK_MOTION: {
                s32 motion_x = BINK_BUNDLE_S8(xoff);
                s32 motion_y = BINK_BUNDLE_S8(yoff);
                u8 PTR4* motion_source;
                u32 block_row;

                BINK_MARK_WORK_BLOCK(work_row, work_col);
                BINK_BUNDLE_ADVANCE(xoff, BINK_BUNDLE_BYTE_PITCH);
                BINK_BUNDLE_ADVANCE(yoff, BINK_BUNDLE_BYTE_PITCH);
                motion_source = BINK_MOTION_SOURCE(old, pitch, motion_x, motion_y);
                for (block_row = 0; block_row < BINK_BLOCK_SIDE; ++block_row) {
                    BINK_BLOCK_ROW_WORD(dest, pitch, block_row, BINK_BLOCK_ROW_WORD_0) =
                        BINK_BLOCK_ROW_WORD(motion_source, pitch, block_row, BINK_BLOCK_ROW_WORD_0);
                    BINK_BLOCK_ROW_WORD(dest, pitch, block_row, BINK_BLOCK_ROW_WORD_1) =
                        BINK_BLOCK_ROW_WORD(motion_source, pitch, block_row, BINK_BLOCK_ROW_WORD_1);
                }
                break;
            }
            case BINK_BLOCK_RESIDUE: {
                s32 motion_x = BINK_BUNDLE_S8(xoff);
                s32 motion_y = BINK_BUNDLE_S8(yoff);
                u8 PTR4* motion_source;
                u32 residue_limit;
                u32 block_row;

                BINK_MARK_WORK_BLOCK(work_row, work_col);
                BINK_BUNDLE_ADVANCE(xoff, BINK_BUNDLE_BYTE_PITCH);
                BINK_BUNDLE_ADVANCE(yoff, BINK_BUNDLE_BYTE_PITCH);
                motion_source = BINK_MOTION_SOURCE(old, pitch, motion_x, motion_y);
                for (block_row = 0; block_row < BINK_BLOCK_SIDE; ++block_row) {
                    BINK_LINEAR_BLOCK_ROW_WORD(motion_block, block_row, BINK_BLOCK_ROW_WORD_0) =
                        BINK_BLOCK_ROW_WORD(motion_source, pitch, block_row, BINK_BLOCK_ROW_WORD_0);
                    BINK_LINEAR_BLOCK_ROW_WORD(motion_block, block_row, BINK_BLOCK_ROW_WORD_1) =
                        BINK_BLOCK_ROW_WORD(motion_source, pitch, block_row, BINK_BLOCK_ROW_WORD_1);
                }
                residue_limit = exp_get_bits(&bitstate, BINK_RESIDUE_LIMIT_BITS);
                ReadBPLossyWithMotion((char PTR4*)dest, (s32)pitch,
                                      (BPBITSTREAM PTR4*)&bitstate, residue_limit,
                                      (char PTR4*)motion_block);
                break;
            }
            case BINK_BLOCK_INTRA: {
                u32 quant;

                BINK_MARK_WORK_BLOCK(work_row, work_col);
                dct_block[0] = BINK_BUNDLE_S16(intra_dc);
                BINK_BUNDLE_ADVANCE(intra_dc, BINK_DC_BYTES);
                ReadBPLossless(dct_block, (BPBITSTREAM PTR4*)&bitstate);
                quant = exp_get_bits(&bitstate, BINK_DCT_QUANT_BITS);
                FastIDCT8x8(dest, pitch, dct_block, quant);
                break;
            }
            case BINK_BLOCK_INTER: {
                s32 motion_x = BINK_BUNDLE_S8(xoff);
                s32 motion_y = BINK_BUNDLE_S8(yoff);
                u8 PTR4* motion_source;
                u32 quant;
                u32 block_row;

                BINK_MARK_WORK_BLOCK(work_row, work_col);
                dct_block[0] = BINK_BUNDLE_S16(inter_dc);
                BINK_BUNDLE_ADVANCE(inter_dc, BINK_DC_BYTES);
                BINK_BUNDLE_ADVANCE(xoff, BINK_BUNDLE_BYTE_PITCH);
                BINK_BUNDLE_ADVANCE(yoff, BINK_BUNDLE_BYTE_PITCH);
                motion_source = BINK_MOTION_SOURCE(old, pitch, motion_x, motion_y);
                for (block_row = 0; block_row < BINK_BLOCK_SIDE; ++block_row) {
                    BINK_LINEAR_BLOCK_ROW_WORD(motion_block, block_row, BINK_BLOCK_ROW_WORD_0) =
                        BINK_BLOCK_ROW_WORD(motion_source, pitch, block_row, BINK_BLOCK_ROW_WORD_0);
                    BINK_LINEAR_BLOCK_ROW_WORD(motion_block, block_row, BINK_BLOCK_ROW_WORD_1) =
                        BINK_BLOCK_ROW_WORD(motion_source, pitch, block_row, BINK_BLOCK_ROW_WORD_1);
                }
                ReadBPLossless(dct_block, (BPBITSTREAM PTR4*)&bitstate);
                quant = exp_get_bits(&bitstate, BINK_DCT_QUANT_BITS);
                FastmIDCT8x8WithMotion(dest, (s32)pitch, dct_block, quant, motion_block);
                break;
            }
            case BINK_BLOCK_FILL: {
                u8 color = BINK_BUNDLE_U8(colors);
                u32 fill = BINK_FILL_WORD(color);
                u32 block_row;

                BINK_MARK_WORK_BLOCK(work_row, work_col);
                BINK_BUNDLE_ADVANCE(colors, BINK_BUNDLE_BYTE_PITCH);
                for (block_row = 0; block_row < BINK_BLOCK_SIDE; ++block_row) {
                    BINK_BLOCK_ROW_WORD(dest, pitch, block_row, BINK_BLOCK_ROW_WORD_0) = fill;
                    BINK_BLOCK_ROW_WORD(dest, pitch, block_row, BINK_BLOCK_ROW_WORD_1) = fill;
                }
                break;
            }
            case BINK_BLOCK_PATTERN:
                BINK_MARK_WORK_BLOCK(work_row, work_col);
                expand_pattern_block(dest, pitch, &colors, &patterns);
                break;
            case BINK_BLOCK_RAW: {
                u32 block_row;

                BINK_MARK_WORK_BLOCK(work_row, work_col);
                for (block_row = 0; block_row < BINK_BLOCK_SIDE; ++block_row) {
                    BINK_BLOCK_ROW_WORD(dest, pitch, block_row, BINK_BLOCK_ROW_WORD_0) =
                        BINK_LINEAR_BLOCK_ROW_WORD(colors.cur_ptr, block_row, BINK_BLOCK_ROW_WORD_0);
                    BINK_BLOCK_ROW_WORD(dest, pitch, block_row, BINK_BLOCK_ROW_WORD_1) =
                        BINK_LINEAR_BLOCK_ROW_WORD(colors.cur_ptr, block_row, BINK_BLOCK_ROW_WORD_1);
                }
                BINK_BUNDLE_ADVANCE(colors, BINK_COLOR_BLOCK_BYTES);
                break;
            }
            case BINK_BLOCK_RUN:
                BINK_MARK_WORK_BLOCK(work_row, work_col);
                expand_run_block(dest, pitch, &colors, &runs, &bitstate);
                break;
            case BINK_BLOCK_SCALED:
                subblock_type = BINK_BUNDLE_U8(subblock_types);
                BINK_BUNDLE_ADVANCE(subblock_types, BINK_BUNDLE_BYTE_PITCH);
                if (subblock_type == BINK_BLOCK_FILL) {
                    BINK_BUNDLE_ADVANCE(colors, BINK_BUNDLE_BYTE_PITCH);
                } else if (subblock_type == BINK_BLOCK_PATTERN) {
                    BINK_BUNDLE_ADVANCE(colors, BINK_PATTERN_COLOR_COUNT);
                    BINK_BUNDLE_ADVANCE(patterns, BINK_PATTERN_BLOCK_BYTES);
                } else if (subblock_type == BINK_BLOCK_RAW) {
                    BINK_BUNDLE_ADVANCE(colors, BINK_COLOR_BLOCK_BYTES);
                } else if (subblock_type == BINK_BLOCK_INTRA) {
                    u32 quant;

                    dct_block[0] = BINK_BUNDLE_S16(intra_dc);
                    BINK_BUNDLE_ADVANCE(intra_dc, BINK_DC_BYTES);
                    ReadBPLossless(dct_block, (BPBITSTREAM PTR4*)&bitstate);
                    quant = exp_get_bits(&bitstate, BINK_DCT_QUANT_BITS);
                    FastIDCT8x8d(dest, pitch, dct_block, quant);
                }
                BINK_MARK_WORK_BLOCK(work_row, work_col);
                col += BINK_BLOCK_SIDE;
                dest += BINK_BLOCK_SIDE;
                old += BINK_BLOCK_SIDE;
                work_col += plane;
                BINK_MARK_WORK_BLOCK(work_row, work_col);
                break;
            }

            col += BINK_BLOCK_SIDE;
            dest += BINK_BLOCK_SIDE;
            old += BINK_BLOCK_SIDE;
            work_col += plane;
        }

        if (plane == BINK_LUMA_PLANE_SCALE) {
            if (BINK_BLOCK_ODD_ROW(row)) {
                work_row += work_pitch;
            }
        } else {
            work_row += work_pitch;
        }
        row += BINK_BLOCK_SIDE;
        dest = out + row * pitch;
        old = prev + row * pitch;
    } while (row < height);
    }

    if (bitstate.bitlen != 0) {
        bitstate.cur++;
    }
    return bitstate.cur;
}

void ExpandBink(u8 PTR4* yout,
                u8 PTR4* yprev,
                u8 PTR4* aout,
                u8 PTR4* aprev,
                u8 PTR4* work,
                u32 width,
                u32 height,
                u32 pitch,
                u32 uvpitch,
                u32 PTR4* bundles,
                u32 key_frame,
                BUNDLEPOINTERS PTR4* table,
                u32 yflags,
                u32 aflags)
{
    u32 PTR4* next;
    u32 uv_size;

    if ((aflags & BINKALPHA) != 0) {
        if ((yflags & BINKALPHA) != 0) {
            ExpandPlane(aout, aprev, BINK_BLOCK_ROUND(width), BINK_BLOCK_ROUND(height),
                        pitch, bundles + 1, key_frame, work, BINK_LUMA_PLANE_SCALE, table,
                        yflags);
        }
        bundles = BINK_BUNDLE_CHUNK_NEXT(bundles);
    }

    if ((yflags & BINKOLDFRAMEFORMAT) == 0) {
        bundles++;
    }

    next = ExpandPlane(yout, yprev, BINK_BLOCK_ROUND(width), BINK_BLOCK_ROUND(height),
                       pitch, bundles, key_frame, work, BINK_LUMA_PLANE_SCALE, table, yflags);
    if ((yflags & BINKOLDFRAMEFORMAT) == 0) {
        next = BINK_BUNDLE_PAYLOAD_NEXT(bundles);
    }

    if ((yflags & BINKGRAYSCALE) == 0) {
        yout = yout + pitch * uvpitch;
        yprev = yprev + pitch * uvpitch;
        uvpitch >>= BINK_CHROMA_SHIFT;
        width = BINK_BLOCK_ROUND(BINK_CHROMA_ROUND(width));
        height = BINK_BLOCK_ROUND(BINK_CHROMA_ROUND(height));
        pitch >>= BINK_CHROMA_SHIFT;
        next = ExpandPlane(yout, yprev, width, height, pitch, next, key_frame, work,
                           BINK_CHROMA_PLANE_SCALE, table, yflags);
        uv_size = pitch * uvpitch;
        ExpandPlane(yout + uv_size, yprev + uv_size, width, height, pitch,
                    next, key_frame, work, BINK_CHROMA_PLANE_SCALE, table, yflags);
    }
}
