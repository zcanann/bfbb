#include "bink.h"
#include "bitplane.h"
#include "dct.h"
#include "varbits.h"

#define BP_BITS_PER_WORD BPBITSTYPELEN
#define BP_WORD_TOP_BIT (BP_BITS_PER_WORD - 1)
#define BP_S32_SIGN_SHIFT BP_WORD_TOP_BIT
#define BP_S16_SIGN_SHIFT 15
#define BP_S8_SIGN_SHIFT 7
#define BP_BLOCK_SIDE 8
#define BP_BLOCK_COEFFS (BP_BLOCK_SIDE * BP_BLOCK_SIDE)
#define BP_LOSSY_BLOCK_BYTES BP_BLOCK_COEFFS
#define BP_DC_COEFF 0
#define BP_FIRST_AC_COEFF (BP_DC_COEFF + 1)
#define BP_AC_COEFFS (BP_BLOCK_COEFFS - BP_FIRST_AC_COEFF)
#define BP_COEFFS_PER_PAIR 2
#define BP_LOSSY_OUTPUT_COEFFS (BP_BLOCK_COEFFS / BP_COEFFS_PER_PAIR)
#define BP_TREE_CHILD_COUNT 4
#define BP_TREE_NODES (BP_BLOCK_COEFFS + BP_TREE_CHILD_COUNT)
#define BP_TREE_GROUPS (BP_BLOCK_COEFFS / BP_TREE_CHILD_COUNT)
#define BP_TREE_LAST_GROUP (BP_TREE_GROUPS - 1)
#define BP_TREE_HIGH_GROUPS 3
#define BP_TREE_ADDED_CHILD_COUNT (BP_TREE_CHILD_COUNT - 1)
#define BP_TREE_HIGH_GROUP_STRIDE (BP_TREE_CHILD_COUNT + 1)
#define BP_TREE_NODE_PRESENT_BITS 1
#define BP_TREE_NODE_SIGNAL_BITS (BP_TREE_CHILD_COUNT + 1)
#define BP_FINAL_COEFF_SIGNAL_BITS 2
#define BP_TREE_CHILD1_INDEX 1
#define BP_TREE_CHILD2_INDEX 2
#define BP_TREE_CHILD3_INDEX 3
#define BP_TREE_CHILD1_BASE (BP_TREE_CHILD_COUNT * 1)
#define BP_TREE_CHILD2_BASE (BP_TREE_CHILD_COUNT * 2)
#define BP_TREE_CHILD3_BASE (BP_TREE_CHILD_COUNT * 3)
#define BP_TREE_GROUP_INDEX(index) ((index) / BP_TREE_CHILD_COUNT)
#define BP_LOSSLESS_TREE_GROUP_INDEX(index) (BP_TREE_GROUP_INDEX(index) - 1)
#define BP_NEXT_TREE_GROUP(ptr) ((ptr) + BP_TREE_CHILD_COUNT)
#define BP_FIRST_LOSSLESS_TREE_GROUP_INDEX BP_TREE_CHILD_COUNT
#define BP_FIRST_LOSSLESS_TREE_GROUP_END_INDEX (BP_FIRST_LOSSLESS_TREE_GROUP_INDEX + BP_TREE_CHILD3_INDEX)
#define BP_FIRST_LOSSY_TREE_GROUP_END_INDEX BP_TREE_CHILD3_INDEX
#define BP_TREE_GROUP1_INDEX 1
#define BP_TREE_GROUP6_INDEX (BP_TREE_GROUP1_INDEX + BP_TREE_HIGH_GROUP_STRIDE)
#define BP_TREE_GROUP11_INDEX (BP_TREE_GROUP6_INDEX + BP_TREE_HIGH_GROUP_STRIDE)
#define BP_LOSSLESS_TREE_GROUP1_INDEX 0
#define BP_LOSSLESS_TREE_GROUP6_INDEX (BP_LOSSLESS_TREE_GROUP1_INDEX + BP_TREE_HIGH_GROUP_STRIDE)
#define BP_LOSSLESS_TREE_GROUP11_INDEX (BP_LOSSLESS_TREE_GROUP6_INDEX + BP_TREE_HIGH_GROUP_STRIDE)
#define BP_TREE_HIGH_GROUP0_INDEX (BP_TREE_GROUP1_INDEX + 1)
#define BP_TREE_HIGH_GROUP1_INDEX (BP_TREE_HIGH_GROUP0_INDEX + BP_TREE_HIGH_GROUP_STRIDE)
#define BP_TREE_HIGH_GROUP2_INDEX (BP_TREE_HIGH_GROUP1_INDEX + BP_TREE_HIGH_GROUP_STRIDE)
#define BP_TREE_HIGH_GROUP0_CHILD0_INDEX (BP_TREE_HIGH_GROUP0_INDEX + 1)
#define BP_TREE_HIGH_GROUP1_CHILD0_INDEX (BP_TREE_HIGH_GROUP1_INDEX + 1)
#define BP_TREE_HIGH_GROUP2_CHILD0_INDEX (BP_TREE_HIGH_GROUP2_INDEX + 1)
#define BP_LOSSLESS_TREE_HIGH_GROUP0_INDEX (BP_LOSSLESS_TREE_GROUP1_INDEX + 1)
#define BP_LOSSLESS_TREE_HIGH_GROUP1_INDEX (BP_LOSSLESS_TREE_HIGH_GROUP0_INDEX + BP_TREE_HIGH_GROUP_STRIDE)
#define BP_LOSSLESS_TREE_HIGH_GROUP2_INDEX (BP_LOSSLESS_TREE_HIGH_GROUP1_INDEX + BP_TREE_HIGH_GROUP_STRIDE)
#define BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX (BP_LOSSLESS_TREE_HIGH_GROUP0_INDEX + 1)
#define BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX (BP_LOSSLESS_TREE_HIGH_GROUP1_INDEX + 1)
#define BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX (BP_LOSSLESS_TREE_HIGH_GROUP2_INDEX + 1)
#define BP_TREE_HIGH_GROUP0_SLOT 0
#define BP_TREE_HIGH_GROUP1_SLOT 1
#define BP_TREE_HIGH_GROUP2_SLOT 2
#define BP_TREE_HIGH_GROUP_MAX(groups, max_level, group_index, child0_index)                                             \
    do {                                                                                                                \
        (max_level) = (groups)[group_index];                                                                            \
        if ((max_level) < (groups)[child0_index]) {                                                                     \
            (max_level) = (groups)[child0_index];                                                                       \
        }                                                                                                               \
        if ((max_level) < (groups)[(child0_index) + BP_TREE_CHILD1_INDEX]) {                                            \
            (max_level) = (groups)[(child0_index) + BP_TREE_CHILD1_INDEX];                                              \
        }                                                                                                               \
        if ((max_level) < (groups)[(child0_index) + BP_TREE_CHILD2_INDEX]) {                                            \
            (max_level) = (groups)[(child0_index) + BP_TREE_CHILD2_INDEX];                                              \
        }                                                                                                               \
    } while (0)
#define BP_BYTE_MASK 0xff
#define BP_U16_MASK 0xffff
#define BP_BIT_MASK 1
#define BP_SIGN_BIT 0x80
#define BP_NEGATIVE_COEFF_SIGN 0xffff
#define BP_POSITIVE_COEFF_SIGN 1
#define BP_ABS_COEFF(value, sign) (((sign) ^ (value)) - (sign))
#define BP_TREE_EMPTY_ENTRY 0
/* Write-side tree nodes pack kind, coefficient/group index, and bit depth. */
#define BP_TREE_KIND_SHIFT 8
#define BP_TREE_KIND_MASK (3 << BP_TREE_KIND_SHIFT)
#define BP_TREE_INDEX_SHIFT 10
#define BP_TREE_GROUP_SHIFT 12
#define BP_TREE_HIGH_GROUP_SHIFT 14
#define BP_TREE_BASE_MASK 0xfc00
#define BP_TREE_INDEX_STRIDE 0x400
#define BP_TREE_PACKED_KIND(kind) ((kind) << BP_TREE_KIND_SHIFT)
#define BP_TREE_PACKED_INDEX(index) ((index) * BP_TREE_INDEX_STRIDE)
#define BP_TREE_PACKED_GROUP(group) ((group) << BP_TREE_GROUP_SHIFT)
#define BP_TREE_ENTRY_KIND(entry) ((entry) & BP_TREE_KIND_MASK)
#define BP_TREE_ENTRY_INDEX(entry) ((entry) >> BP_TREE_INDEX_SHIFT)
#define BP_TREE_ENTRY_GROUP(entry) ((entry) >> BP_TREE_GROUP_SHIFT)
#define BP_TREE_ENTRY_HIGH_GROUP(entry) ((entry) >> BP_TREE_HIGH_GROUP_SHIFT)
#define BP_TREE_ENTRY_BASE(entry) ((entry) & BP_TREE_BASE_MASK)
#define BP_TREE_ENTRY_LEVEL(entry) ((entry) & BP_BYTE_MASK)
#define BP_COEFF1_INDEX 1
#define BP_COEFF2_INDEX 2
#define BP_COEFF3_INDEX 3
#define BP_GROUP1_NODE_BASE BP_TREE_PACKED_GROUP(1)
#define BP_GROUP6_NODE_BASE BP_TREE_PACKED_GROUP(6)
#define BP_GROUP11_NODE_BASE BP_TREE_PACKED_GROUP(11)
#define BP_COEFF1_LEAF_BASE BP_TREE_PACKED_KIND(7)
#define BP_COEFF2_LEAF_BASE BP_TREE_PACKED_KIND(11)
#define BP_COEFF3_LEAF_BASE BP_TREE_PACKED_KIND(15)
#define BP_TREE_GROUP_ENTRY(level, base) ((level) | (base))
#define BP_TREE_COEFF_LEAF_ENTRY(level, base) ((level) + (base))
#define BP_TREE_HIGH_GROUP_ENTRY(level, index) \
    ((u16)(level) + (BP_TREE_PACKED_INDEX((index) + BP_TREE_CHILD1_BASE) + BP_TREE_GROUP_NODE))
#define BP_TREE_BRANCH_ENTRY(level, index) \
    ((u16)(level) + (BP_TREE_PACKED_INDEX(index) + BP_TREE_BRANCH_NODE))
#define BP_TREE_CHILD_BRANCH_ENTRY(level, base, child_base) BP_TREE_BRANCH_ENTRY((level), (base) + (child_base))
#define BP_TREE_BASE_BRANCH_ENTRY(level, base) ((u16)(level) + (base) + BP_TREE_BRANCH_NODE)
#define BP_TREE_COEFF_ENTRY(level, index) ((u16)(level) | BP_TREE_PACKED_INDEX(index) + BP_TREE_COEFF_NODE)
#define BP_TREE_BASE_COEFF_ENTRY(level, base) ((u16)(level) | (base) + BP_TREE_COEFF_NODE)
#define BP_READ_TREE_KIND_MASK 3
/* Read-side nodes pack the same logical tree into byte-sized entries. */
#define BP_READ_TREE_EMPTY_ENTRY 0
#define BP_READ_TREE_INDEX_SHIFT 2
#define BP_READ_TREE_BASE_MASK 0xfc
#define BP_READ_TREE_CHILD_COUNT BP_TREE_CHILD_COUNT
#define BP_READ_TREE_CHILD1_BASE (BP_READ_TREE_CHILD_COUNT * 1)
#define BP_READ_TREE_CHILD2_BASE (BP_READ_TREE_CHILD_COUNT * 2)
#define BP_READ_TREE_CHILD3_BASE (BP_READ_TREE_CHILD_COUNT * 3)
#define BP_READ_TREE_NODE(index, kind) (((index) << BP_READ_TREE_INDEX_SHIFT) + (kind))
#define BP_READ_TREE_KIND(node) ((node) & BP_READ_TREE_KIND_MASK)
#define BP_READ_TREE_INDEX(node) ((node) >> BP_READ_TREE_INDEX_SHIFT)
#define BP_READ_TREE_BASE(node) ((node) & BP_READ_TREE_BASE_MASK)
#define BP_READ_TREE_GROUP_BASE(group) ((group) * BP_READ_TREE_CHILD_COUNT)
#define BP_READ_TREE_GROUP(group) BP_READ_TREE_NODE(BP_READ_TREE_GROUP_BASE(group), BP_READ_TREE_HIGH_NODE)
#define BP_READ_TREE_GROUP1_ROOT BP_READ_TREE_GROUP(BP_TREE_GROUP1_INDEX)
#define BP_READ_TREE_GROUP6_ROOT BP_READ_TREE_GROUP(BP_TREE_GROUP6_INDEX)
#define BP_READ_TREE_GROUP11_ROOT BP_READ_TREE_GROUP(BP_TREE_GROUP11_INDEX)
#define BP_READ_TREE_CHILD_BRANCH(base, child_base) BP_READ_TREE_NODE((base) + (child_base), BP_READ_TREE_BRANCH_NODE)
#define BP_READ_TREE_BRANCH(index) BP_READ_TREE_NODE(index, BP_READ_TREE_BRANCH_NODE)
#define BP_READ_TREE_COEFF(index) BP_READ_TREE_NODE(index, BP_READ_TREE_COEFF_NODE)
#define BP_READ_TREE_DC_ROOT BP_READ_TREE_BRANCH(BP_DC_COEFF)
#define BP_READ_TREE_COEFF1_ROOT BP_READ_TREE_COEFF(BP_COEFF1_INDEX)
#define BP_READ_TREE_COEFF2_ROOT BP_READ_TREE_COEFF(BP_COEFF2_INDEX)
#define BP_READ_TREE_COEFF3_ROOT BP_READ_TREE_COEFF(BP_COEFF3_INDEX)
#define BP_READ_TREE_BRANCH_FROM_NODE(node) (BP_READ_TREE_BASE(node) + BP_READ_TREE_BRANCH_NODE)
#define BP_READ_TREE_COEFF_FROM_NODE(node) (BP_READ_TREE_BASE(node) + BP_READ_TREE_COEFF_NODE)
#define BP_READ_TREE_GROUP_FROM_INDEX(index) BP_READ_TREE_NODE((index) + BP_READ_TREE_CHILD_COUNT, BP_READ_TREE_GROUP_NODE)
#define BP_ROOT_GROUP1_SLOT 0
#define BP_ROOT_GROUP6_SLOT (BP_ROOT_GROUP1_SLOT + 1)
#define BP_ROOT_GROUP11_SLOT (BP_ROOT_GROUP6_SLOT + 1)
#define BP_ROOT_LOSSLESS_COEFF1_SLOT (BP_ROOT_GROUP11_SLOT + 1)
#define BP_ROOT_LOSSLESS_COEFF2_SLOT (BP_ROOT_LOSSLESS_COEFF1_SLOT + 1)
#define BP_ROOT_LOSSLESS_COEFF3_SLOT (BP_ROOT_LOSSLESS_COEFF2_SLOT + 1)
#define BP_ROOT_LOSSY_DC_SLOT (BP_ROOT_GROUP11_SLOT + 1)
#define BP_LOSSLESS_ROOT_NODES (BP_ROOT_LOSSLESS_COEFF3_SLOT + 1)
#define BP_LOSSY_ROOT_NODES (BP_ROOT_LOSSY_DC_SLOT + 1)
#define BP_LOSSLESS_LEVEL_BITS 4
#define BP_LOSSLESS_LEVEL_MASK 0xf
#define BP_LOSSY_LEVEL_BITS 3
#define BP_LOSSY_LEVEL_MASK 7
#define BP_ZIGZAG_COEFF(vals, index) ((vals)[zigzag[index]])
#define BP_COEFF_BIT_LEVEL(value) (getbitlevelvar(value) & BP_BYTE_MASK)
#define BP_NEXT_LEVEL(level) (((level) - 1) & BP_BYTE_MASK)
#define BP_LEVEL_MASK(level) (1 << ((level) - 1))
#define BP_LOSSLESS_LEVEL_CODE(level) ((level) & VarBitsLens[BP_LOSSLESS_LEVEL_BITS])
#define BP_LOSSY_LEVEL_CODE(level) (((level) - 1) & VarBitsLens[BP_LOSSY_LEVEL_BITS])
#define BP_LOSSY_LEVEL_COUNT(encoded) ((u8)((encoded) + 1))

typedef enum BPWriteTreeKind
{
    BP_TREE_HIGH_NODE = 0,
    BP_TREE_GROUP_NODE = 1 << BP_TREE_KIND_SHIFT,
    BP_TREE_AFTER_GROUP_NODE = BP_TREE_GROUP_NODE + 1,
    BP_TREE_BRANCH_NODE = 2 << BP_TREE_KIND_SHIFT,
    BP_TREE_COEFF_NODE = 3 << BP_TREE_KIND_SHIFT
} BPWriteTreeKind;

typedef enum BPReadTreeKind
{
    BP_READ_TREE_HIGH_NODE = 0,
    BP_READ_TREE_GROUP_NODE = 1,
    BP_READ_TREE_BRANCH_NODE = 2,
    BP_READ_TREE_COEFF_NODE = 3
} BPReadTreeKind;

typedef struct BPCOEFFPAIR
{
    s16 first;
    s16 second;
} BPCOEFFPAIR;

typedef union BPLOSSLESSCOEFFS
{
    u16 values[BP_BLOCK_COEFFS];
    BPCOEFFPAIR pairs[BP_LOSSY_OUTPUT_COEFFS];
    u32 words[BP_LOSSY_OUTPUT_COEFFS];
} BPLOSSLESSCOEFFS;

#define BP_COEFF_PAIR_AT(values, index) (((BPCOEFFPAIR PTR4*)(values))[(index) / BP_COEFFS_PER_PAIR])
#define BP_LOSSLESS_OUT_PAIR(pair) ((pair) * BP_COEFFS_PER_PAIR)
#define BP_LOSSLESS_SCAN_PAIR(pair) ((pair) * BP_COEFFS_PER_PAIR)
#define BP_LOSSLESS_CLEAR_START BP_COEFF2_INDEX
#define BP_LOSSLESS_CLEAR_BYTES(coeffs) \
    (sizeof((coeffs).values) - BP_LOSSLESS_CLEAR_START * sizeof((coeffs).values[0]))
#define BP_LOSSY_OUT_PAIR(pair) (pair)
#define BP_LOSSY_SCAN_PAIR(pair) (pair)
#define BP_LOSSY_SCAN_SAMPLE(sample) (sample)
#define BP_MOTION_OUT_COLUMN(col) (col)
#define BP_BLOCK_SAMPLE(row, col) ((row) * BP_BLOCK_SIDE + (col))
#define BP_SCATTER_LOSSLESS_PAIR(out, out_index, coeffs, coeff_index) \
    (BP_COEFF_PAIR_AT((out), (out_index)) = (coeffs).pairs[(coeff_index) / BP_COEFFS_PER_PAIR])

typedef union BPLOSSYBLOCK
{
    s8 bytes[BP_BLOCK_COEFFS];
    s16 pairs[BP_LOSSY_OUTPUT_COEFFS];
} BPLOSSYBLOCK;

typedef struct BPLOSSYREADTREE
{
    u8 pending[BP_TREE_NODES];
    u8 roots[BP_LOSSY_ROOT_NODES];
    u8 nodes[BP_BLOCK_COEFFS];
} BPLOSSYREADTREE;

typedef struct BPLOSSLESSREADTREE
{
    u8 pending[BP_TREE_NODES];
    u8 roots[BP_LOSSLESS_ROOT_NODES];
    u8 nodes[BP_TREE_NODES - BP_LOSSLESS_ROOT_NODES];
} BPLOSSLESSREADTREE;

typedef struct BPLOSSLESSWRITETREE
{
    u16 pending[BP_TREE_NODES];
    u16 roots[BP_LOSSLESS_ROOT_NODES];
    u16 nodes[BP_BLOCK_COEFFS - BP_LOSSLESS_ROOT_NODES];
} BPLOSSLESSWRITETREE;

typedef struct BPLOSSYWRITETREE
{
    u16 pending[BP_TREE_NODES];
    u16 roots[BP_LOSSY_ROOT_NODES];
    u16 nodes[BP_BLOCK_COEFFS - BP_LOSSY_ROOT_NODES];
} BPLOSSYWRITETREE;

static void readlossy(s8 PTR4* dest, BPBITSTREAM PTR4* bits, s32 masks_count);

#define BP_STREAM(bits) ((BPBITSTREAM*)(bits))
#define BP_STREAM_CUR(bits) (BP_STREAM(bits)->cur)
#define BP_STREAM_BITS(bits) (BP_STREAM(bits)->bits)
#define BP_STREAM_BITLEN(bits) (BP_STREAM(bits)->bitlen)

#define PUT_BP_BIT(bits, bit)                                                                                          \
    do {                                                                                                               \
        u32 _bitcount;                                                                                                 \
        if (bit) {                                                                                                     \
            BP_STREAM_BITS(bits) |= 1 << BP_STREAM_BITLEN(bits);                                                       \
        }                                                                                                              \
        _bitcount = BP_STREAM_BITLEN(bits);                                                                            \
        BP_STREAM_BITLEN(bits) = _bitcount + 1;                                                                        \
        if (_bitcount + 1 == BP_BITS_PER_WORD) {                                                                       \
            *BP_STREAM_CUR(bits) = BP_STREAM_BITS(bits);                                                               \
            BP_STREAM_BITLEN(bits) = 0;                                                                                \
            BP_STREAM_BITS(bits) = 0;                                                                                  \
            BP_STREAM_CUR(bits) = BP_STREAM_CUR(bits) + 1;                                                            \
        }                                                                                                              \
    } while (0)

#define PUT_BP_BITS(bits, value, size, mask)                                                                            \
    do {                                                                                                               \
        BPBITSTYPE _value = (value) & (mask);                                                                            \
        u32 _bitcount = BP_STREAM_BITLEN(bits) + (size);                                                               \
        BPBITSTYPE _bitbuf = BP_STREAM_BITS(bits) | (_value << BP_STREAM_BITLEN(bits));                                \
        BP_STREAM_BITS(bits) = _bitbuf;                                                                                \
        BP_STREAM_BITLEN(bits) = _bitcount;                                                                            \
        if (_bitcount >= BP_BITS_PER_WORD) {                                                                           \
            *BP_STREAM_CUR(bits) = _bitbuf;                                                                            \
            _bitcount = BP_STREAM_BITLEN(bits) - BP_BITS_PER_WORD;                                                     \
            BP_STREAM_CUR(bits) = BP_STREAM_CUR(bits) + 1;                                                            \
            BP_STREAM_BITLEN(bits) = _bitcount;                                                                        \
            BP_STREAM_BITS(bits) = 0;                                                                                  \
            if (_bitcount != 0) {                                                                                      \
                BP_STREAM_BITS(bits) = _value >> ((size) - _bitcount);                                                 \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

u32 LenBPLossless(s16 PTR4* vals)
{
    u16 entry;
    u8 PTR4* child_lens;
    s32 sign;
    u32 bits;
    u32 maxbits;
    u32 had_level;
    s32 i;
    s32 group;
    s32 len;
    s32 count;
    s32 coeff;
    u16 PTR4* cur;
    u16 PTR4* restart;
    u16 PTR4* end;
    u16 PTR4* roots;
    BPLOSSLESSWRITETREE tree;
    u8 lens[BP_BLOCK_COEFFS];
    u8 groups[BP_TREE_GROUPS];
    u8 hi_groups[BP_TREE_HIGH_GROUPS];

    /* Lossless bitplanes code AC coefficient magnitudes by zigzag bit depth. */
    count = BP_AC_COEFFS;
    maxbits = 0;
    i = BP_FIRST_AC_COEFF;
    do {
        u32 coeff_bits;
        coeff = BP_ZIGZAG_COEFF(vals, i);
        sign = coeff >> BP_S32_SIGN_SHIFT;
        coeff_bits = BP_COEFF_BIT_LEVEL(BP_ABS_COEFF(coeff, sign) & BP_U16_MASK);
        if (coeff_bits > maxbits) {
            maxbits = coeff_bits;
        }
        lens[i] = (u8)coeff_bits;
        i++;
        count--;
    } while (count != 0);
    had_level = maxbits;
    /* Each four-coefficient subtree inherits the deepest child bit depth. */
    group = BP_FIRST_LOSSLESS_TREE_GROUP_INDEX;
    count = BP_TREE_LAST_GROUP;
    do {
        bits = lens[group];
        if (bits < lens[group + BP_TREE_CHILD1_INDEX]) {
            bits = lens[group + BP_TREE_CHILD1_INDEX];
        }
        if (bits < lens[group + BP_TREE_CHILD2_INDEX]) {
            bits = lens[group + BP_TREE_CHILD2_INDEX];
        }
        if (bits < lens[group + BP_TREE_CHILD3_INDEX]) {
            bits = lens[group + BP_TREE_CHILD3_INDEX];
        }
        groups[BP_LOSSLESS_TREE_GROUP_INDEX(group)] = (u8)bits;
        group += BP_TREE_CHILD_COUNT;
        count--;
    } while (count != 0);

    BP_TREE_HIGH_GROUP_MAX(groups, bits, BP_LOSSLESS_TREE_HIGH_GROUP0_INDEX,
                           BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX);
    hi_groups[BP_TREE_HIGH_GROUP0_SLOT] = (u8)bits;
    BP_TREE_HIGH_GROUP_MAX(groups, bits, BP_LOSSLESS_TREE_HIGH_GROUP1_INDEX,
                           BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX);
    hi_groups[BP_TREE_HIGH_GROUP1_SLOT] = (u8)bits;
    BP_TREE_HIGH_GROUP_MAX(groups, bits, BP_LOSSLESS_TREE_HIGH_GROUP2_INDEX,
                           BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX);
    hi_groups[BP_TREE_HIGH_GROUP2_SLOT] = (u8)bits;

    len = BP_LOSSLESS_LEVEL_BITS;
    roots = tree.roots;
    bits = hi_groups[BP_TREE_HIGH_GROUP0_SLOT];
    if (bits > groups[BP_LOSSLESS_TREE_GROUP1_INDEX]) {
        entry = BP_TREE_GROUP_ENTRY(bits, BP_GROUP1_NODE_BASE);
    } else {
        entry = BP_TREE_GROUP_ENTRY(groups[BP_LOSSLESS_TREE_GROUP1_INDEX], BP_GROUP1_NODE_BASE);
    }
    roots[BP_ROOT_GROUP1_SLOT] = entry;
    bits = hi_groups[BP_TREE_HIGH_GROUP1_SLOT];
    if (bits > groups[BP_LOSSLESS_TREE_GROUP6_INDEX]) {
        entry = BP_TREE_GROUP_ENTRY(bits, BP_GROUP6_NODE_BASE);
    } else {
        entry = BP_TREE_GROUP_ENTRY(groups[BP_LOSSLESS_TREE_GROUP6_INDEX], BP_GROUP6_NODE_BASE);
    }
    roots[BP_ROOT_GROUP6_SLOT] = entry;
    bits = hi_groups[BP_TREE_HIGH_GROUP2_SLOT];
    if (bits > groups[BP_LOSSLESS_TREE_GROUP11_INDEX]) {
        entry = BP_TREE_GROUP_ENTRY(bits, BP_GROUP11_NODE_BASE);
    } else {
        entry = BP_TREE_GROUP_ENTRY(groups[BP_LOSSLESS_TREE_GROUP11_INDEX], BP_GROUP11_NODE_BASE);
    }
    roots[BP_ROOT_GROUP11_SLOT] = entry;
    roots[BP_ROOT_LOSSLESS_COEFF1_SLOT] = BP_TREE_COEFF_LEAF_ENTRY(lens[BP_COEFF1_INDEX], BP_COEFF1_LEAF_BASE);
    roots[BP_ROOT_LOSSLESS_COEFF2_SLOT] = BP_TREE_COEFF_LEAF_ENTRY(lens[BP_COEFF2_INDEX], BP_COEFF2_LEAF_BASE);
    roots[BP_ROOT_LOSSLESS_COEFF3_SLOT] = BP_TREE_COEFF_LEAF_ENTRY(lens[BP_COEFF3_INDEX], BP_COEFF3_LEAF_BASE);
    restart = roots;
    end = tree.nodes;

    /* Expand pending group/branch/coeff nodes one bitplane level at a time. */
    for (; 1 < maxbits; maxbits = BP_NEXT_LEVEL(maxbits)) {
        cur = restart;
        if (cur < end) {
            do {
                entry = *cur;
                if ((entry == BP_TREE_EMPTY_ENTRY) ||
                    (len += BP_TREE_NODE_PRESENT_BITS, BP_TREE_ENTRY_LEVEL(entry) != maxbits)) {
                    cur++;
                } else {
                    switch (BP_TREE_ENTRY_KIND(entry)) {
                    case BP_TREE_HIGH_NODE:
                        entry = BP_TREE_ENTRY_INDEX(entry);
                        *cur = BP_TREE_HIGH_GROUP_ENTRY(hi_groups[entry >> (BP_TREE_HIGH_GROUP_SHIFT - BP_TREE_INDEX_SHIFT)], entry);
                        goto decoded_length_children;
                    case BP_TREE_GROUP_NODE:
                        entry = BP_TREE_ENTRY_INDEX(entry);
                        bits = BP_TREE_GROUP_INDEX(entry);
                        *cur = BP_TREE_BRANCH_ENTRY(groups[bits - 1], entry);
                        *end = BP_TREE_CHILD_BRANCH_ENTRY(groups[bits], entry, BP_TREE_CHILD1_BASE);
                        *++end = BP_TREE_CHILD_BRANCH_ENTRY(groups[bits + BP_TREE_CHILD1_INDEX], entry, BP_TREE_CHILD2_BASE);
                        *++end = BP_TREE_CHILD_BRANCH_ENTRY(groups[bits + BP_TREE_CHILD2_INDEX], entry, BP_TREE_CHILD3_BASE);
                        ++end;
                        break;
                    case BP_TREE_BRANCH_NODE:
                        *cur = BP_TREE_EMPTY_ENTRY;
                        cur++;
                        entry = BP_TREE_ENTRY_INDEX(entry);
decoded_length_children:
                        child_lens = lens + entry;
                        len += BP_TREE_NODE_SIGNAL_BITS - BP_TREE_NODE_PRESENT_BITS;
                        if (*child_lens != maxbits) {
                            *--restart = BP_TREE_COEFF_ENTRY(*child_lens, entry);
                        } else {
                            len += maxbits;
                        }
                        entry++;
                        child_lens++;
                        if (*child_lens != maxbits) {
                            *--restart = BP_TREE_COEFF_ENTRY(*child_lens, entry);
                        } else {
                            len += maxbits;
                        }
                        entry++;
                        child_lens++;
                        if (*child_lens != maxbits) {
                            *--restart = BP_TREE_COEFF_ENTRY(*child_lens, entry);
                        } else {
                            len += maxbits;
                        }
                        entry++;
                        child_lens++;
                        if (*child_lens != maxbits) {
                            *--restart = BP_TREE_COEFF_ENTRY(*child_lens, entry);
                        } else {
                            len += maxbits;
                        }
                        break;
                    case BP_TREE_COEFF_NODE:
                        *cur = BP_TREE_EMPTY_ENTRY;
                        len += maxbits;
                        cur++;
                        break;
                    default:
                        cur++;
                        break;
                    }
                }
        } while (cur < end);
        }
    }

    if (had_level != 0 && (cur = restart, cur < end)) {
        u32 group_index;
        do {
            entry = *cur;
            if ((entry == BP_TREE_EMPTY_ENTRY) ||
                (len += BP_TREE_NODE_PRESENT_BITS, BP_TREE_ENTRY_LEVEL(entry) != 1)) {
                cur++;
            } else {
                switch (BP_TREE_ENTRY_KIND(entry)) {
                case BP_TREE_HIGH_NODE:
                    entry = BP_TREE_ENTRY_INDEX(entry);
                    *cur = BP_TREE_HIGH_GROUP_ENTRY(hi_groups[entry >> (BP_TREE_HIGH_GROUP_SHIFT - BP_TREE_INDEX_SHIFT)], entry);
                    goto decoded_length_final_children;
                case BP_TREE_GROUP_NODE:
                    entry = BP_TREE_ENTRY_INDEX(entry);
                    group_index = BP_TREE_GROUP_INDEX(entry);
                    *cur = BP_TREE_BRANCH_ENTRY(groups[group_index - 1], entry);
                    *end = BP_TREE_CHILD_BRANCH_ENTRY(groups[group_index], entry, BP_TREE_CHILD1_BASE);
                    *++end = BP_TREE_CHILD_BRANCH_ENTRY(groups[group_index + BP_TREE_CHILD1_INDEX], entry, BP_TREE_CHILD2_BASE);
                    *++end = BP_TREE_CHILD_BRANCH_ENTRY(groups[group_index + BP_TREE_CHILD2_INDEX], entry, BP_TREE_CHILD3_BASE);
                    ++end;
                    break;
                case BP_TREE_BRANCH_NODE:
                    *cur = BP_TREE_EMPTY_ENTRY;
                    cur++;
                    entry = BP_TREE_ENTRY_INDEX(entry);
decoded_length_final_children:
                    child_lens = lens + entry;
                    len += BP_TREE_NODE_SIGNAL_BITS - BP_TREE_NODE_PRESENT_BITS;
                    if (*child_lens != 1) {
                        *--restart = BP_TREE_COEFF_ENTRY(*child_lens, entry);
                    } else {
                        len++;
                    }
                    entry++;
                    child_lens++;
                    if (*child_lens != 1) {
                        *--restart = BP_TREE_COEFF_ENTRY(*child_lens, entry);
                    } else {
                        len++;
                    }
                    entry++;
                    child_lens++;
                    if (*child_lens != 1) {
                        *--restart = BP_TREE_COEFF_ENTRY(*child_lens, entry);
                    } else {
                        len++;
                    }
                    entry++;
                    child_lens++;
                    if (*child_lens != 1) {
                        *--restart = BP_TREE_COEFF_ENTRY(*child_lens, entry);
                    } else {
                        len++;
                    }
                    break;
                case BP_TREE_COEFF_NODE:
                    *cur = BP_TREE_EMPTY_ENTRY;
                    len += BP_FINAL_COEFF_SIGNAL_BITS - BP_TREE_NODE_PRESENT_BITS;
                    cur++;
                    break;
                default:
                    cur++;
                    break;
                }
            }
        } while (cur < end);
    }

    return len;
}

void WriteBPLossless(BPBITSTREAM PTR4* bits, s16 PTR4* vals)
{
    u16 entry;
    s32 coeff;
    s32 sign;
    s32 i;
    s32 count;
    u32 maxbits;
    u32 level;
    u32 lenbits;
    u32 bit_count;
    BPBITSTYPE bit_buf;
    u16 PTR4* cur;
    u16 PTR4* restart;
    u16 PTR4* end;
    u16 PTR4* roots;
    BPLOSSLESSWRITETREE tree;
    u8 lens[BP_BLOCK_COEFFS];
    u8 groups[BP_TREE_GROUPS];
    u16 absvals[BP_BLOCK_COEFFS];
    s16 ordered[BP_BLOCK_COEFFS];
    u8 hi_groups[BP_TREE_HIGH_GROUPS];

    /* Put coefficients in scan order before building bit-depth tables. */
    count = BP_BLOCK_COEFFS;
    i = 0;
    do {
        ordered[i] = BP_ZIGZAG_COEFF(vals, i);
        i++;
        count--;
    } while (count != 0);

    count = BP_BLOCK_COEFFS;
    i = 0;
    do {
        coeff = ordered[i];
        sign = coeff >> BP_S32_SIGN_SHIFT;
        absvals[i] = BP_ABS_COEFF(coeff, sign);
        i++;
        count--;
    } while (count != 0);

    /* The writer uses the same grouped bit-depth tree measured by LenBPLossless. */
    maxbits = 0;
    count = BP_AC_COEFFS;
    i = BP_FIRST_AC_COEFF;
    do {
        u32 coeff_bits = BP_COEFF_BIT_LEVEL((u32)absvals[i]);
        if (coeff_bits > maxbits) {
            maxbits = coeff_bits;
        }
        lens[i] = (u8)coeff_bits;
        i++;
        count--;
    } while (count != 0);

    i = BP_FIRST_LOSSLESS_TREE_GROUP_INDEX;
    count = BP_TREE_LAST_GROUP;
    do {
        u32 group_bits = lens[i];
        if (group_bits < lens[i + BP_TREE_CHILD1_INDEX]) {
            group_bits = lens[i + BP_TREE_CHILD1_INDEX];
        }
        if (group_bits < lens[i + BP_TREE_CHILD2_INDEX]) {
            group_bits = lens[i + BP_TREE_CHILD2_INDEX];
        }
        if (group_bits < lens[i + BP_TREE_CHILD3_INDEX]) {
            group_bits = lens[i + BP_TREE_CHILD3_INDEX];
        }
        groups[BP_LOSSLESS_TREE_GROUP_INDEX(i)] = (u8)group_bits;
        i += BP_TREE_CHILD_COUNT;
        count--;
    } while (count != 0);

    {
        u32 group_bits;
        BP_TREE_HIGH_GROUP_MAX(groups, group_bits, BP_LOSSLESS_TREE_HIGH_GROUP0_INDEX,
                               BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX);
        hi_groups[BP_TREE_HIGH_GROUP0_SLOT] = (u8)group_bits;
        BP_TREE_HIGH_GROUP_MAX(groups, group_bits, BP_LOSSLESS_TREE_HIGH_GROUP1_INDEX,
                               BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX);
        hi_groups[BP_TREE_HIGH_GROUP1_SLOT] = (u8)group_bits;
        BP_TREE_HIGH_GROUP_MAX(groups, group_bits, BP_LOSSLESS_TREE_HIGH_GROUP2_INDEX,
                               BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX);
        hi_groups[BP_TREE_HIGH_GROUP2_SLOT] = (u8)group_bits;
    }

    bit_count = BP_STREAM_BITLEN(bits) + BP_LOSSLESS_LEVEL_BITS;
    lenbits = BP_LOSSLESS_LEVEL_CODE(maxbits);
    bit_buf = BP_STREAM_BITS(bits) | (lenbits << BP_STREAM_BITLEN(bits));
    BP_STREAM_BITLEN(bits) = bit_count;
    BP_STREAM_BITS(bits) = bit_buf;
    if (bit_count >= BP_BITS_PER_WORD) {
        *BP_STREAM_CUR(bits) = bit_buf;
        bit_buf = BP_STREAM_BITLEN(bits) - BP_BITS_PER_WORD;
        BP_STREAM_CUR(bits) = BP_STREAM_CUR(bits) + 1;
        BP_STREAM_BITLEN(bits) = bit_buf;
        if (bit_buf != 0) {
            BP_STREAM_BITS(bits) = lenbits >> (BP_LOSSLESS_LEVEL_BITS - bit_buf);
        } else {
            BP_STREAM_BITS(bits) = 0;
        }
    }

    roots = tree.roots;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP0_SLOT];
    if (lenbits > groups[BP_LOSSLESS_TREE_GROUP1_INDEX]) {
        entry = BP_TREE_GROUP_ENTRY(lenbits, BP_GROUP1_NODE_BASE);
    } else {
        entry = BP_TREE_GROUP_ENTRY(groups[BP_LOSSLESS_TREE_GROUP1_INDEX], BP_GROUP1_NODE_BASE);
    }
    roots[BP_ROOT_GROUP1_SLOT] = entry;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP1_SLOT];
    if (lenbits > groups[BP_LOSSLESS_TREE_GROUP6_INDEX]) {
        entry = BP_TREE_GROUP_ENTRY(lenbits, BP_GROUP6_NODE_BASE);
    } else {
        entry = BP_TREE_GROUP_ENTRY(groups[BP_LOSSLESS_TREE_GROUP6_INDEX], BP_GROUP6_NODE_BASE);
    }
    roots[BP_ROOT_GROUP6_SLOT] = entry;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP2_SLOT];
    if (lenbits > groups[BP_LOSSLESS_TREE_GROUP11_INDEX]) {
        entry = BP_TREE_GROUP_ENTRY(lenbits, BP_GROUP11_NODE_BASE);
    } else {
        entry = BP_TREE_GROUP_ENTRY(groups[BP_LOSSLESS_TREE_GROUP11_INDEX], BP_GROUP11_NODE_BASE);
    }
    roots[BP_ROOT_GROUP11_SLOT] = entry;
    roots[BP_ROOT_LOSSLESS_COEFF1_SLOT] = BP_TREE_COEFF_LEAF_ENTRY(lens[BP_COEFF1_INDEX], BP_COEFF1_LEAF_BASE);
    roots[BP_ROOT_LOSSLESS_COEFF2_SLOT] = BP_TREE_COEFF_LEAF_ENTRY(lens[BP_COEFF2_INDEX], BP_COEFF2_LEAF_BASE);
    roots[BP_ROOT_LOSSLESS_COEFF3_SLOT] = BP_TREE_COEFF_LEAF_ENTRY(lens[BP_COEFF3_INDEX], BP_COEFF3_LEAF_BASE);

    restart = roots;
    end = roots + BP_LOSSLESS_ROOT_NODES;
    level = maxbits;
    while (level != 0) {
        lenbits = level - 1;
        cur = restart;
        /* Active children at lower bit depths are pushed before the current cursor. */
        if (cur < end) {
            do {
                entry = *cur;
                if (entry != BP_TREE_EMPTY_ENTRY) {
                    PUT_BP_BIT(bits, BP_TREE_ENTRY_LEVEL(entry) == level);
                    if (BP_TREE_ENTRY_LEVEL(entry) != level) {
                        goto next_lossless_node;
                    }
                    switch (BP_TREE_ENTRY_KIND(entry)) {
                    case BP_TREE_HIGH_NODE:
                        entry = BP_TREE_ENTRY_INDEX(entry);
                        *cur = BP_TREE_HIGH_GROUP_ENTRY(hi_groups[entry >> (BP_TREE_HIGH_GROUP_SHIFT - BP_TREE_INDEX_SHIFT)], entry);
                        goto decoded_write_children;
                    case BP_TREE_GROUP_NODE:
                        entry = BP_TREE_ENTRY_INDEX(entry);
                        count = BP_TREE_GROUP_INDEX(entry);
                        *cur = BP_TREE_BRANCH_ENTRY(groups[count - 1], entry);
                        *end = BP_TREE_CHILD_BRANCH_ENTRY(groups[count], entry, BP_TREE_CHILD1_BASE);
                        *++end = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD1_INDEX], entry, BP_TREE_CHILD2_BASE);
                        *++end = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD2_INDEX], entry, BP_TREE_CHILD3_BASE);
                        ++end;
                        break;
                    case BP_TREE_BRANCH_NODE:
                        *cur = BP_TREE_EMPTY_ENTRY;
                        cur++;
                        entry = BP_TREE_ENTRY_INDEX(entry);
decoded_write_children:
                        PUT_BP_BIT(bits, lens[entry] != level);
                        if (lens[entry] != level) {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[entry], entry);
                        } else {
                            PUT_BP_BITS(bits, absvals[entry], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[entry] < 0);
                        }

                        i = entry + BP_TREE_CHILD1_INDEX;
                        PUT_BP_BIT(bits, lens[i] != level);
                        if (lens[i] != level) {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[i], i);
                        } else {
                            PUT_BP_BITS(bits, absvals[i], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[i] < 0);
                        }

                        i = entry + BP_TREE_CHILD2_INDEX;
                        PUT_BP_BIT(bits, lens[i] != level);
                        if (lens[i] != level) {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[i], i);
                        } else {
                            PUT_BP_BITS(bits, absvals[i], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[i] < 0);
                        }

                        i = entry + BP_TREE_CHILD3_INDEX;
                        PUT_BP_BIT(bits, lens[i] != level);
                        if (lens[i] != level) {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[i], i);
                        } else {
                            PUT_BP_BITS(bits, absvals[i], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[i] < 0);
                        }
                        break;
                    case BP_TREE_COEFF_NODE:
                        entry = BP_TREE_ENTRY_INDEX(entry);
                        PUT_BP_BITS(bits, absvals[entry], lenbits, VarBitsLens[lenbits]);
                        PUT_BP_BIT(bits, ordered[entry] < 0);
                        *cur = BP_TREE_EMPTY_ENTRY;
                        goto next_lossless_node;
                    default:
                        goto next_lossless_node;
                    }
                    continue;
                }
next_lossless_node:
                cur++;
            } while (cur < end);
        }
        level = lenbits & BP_BYTE_MASK;
    }
}

void ReadBPLossless(s16 PTR4* out, BPBITSTREAM PTR4* bits)
{
    u32 code;
    u8 level;
    u8 maxlevel;
    u8 had_level;
    s16 highbit;
    s16 next_highbit;
    s16 coeff_value;
    u32 bit_mask;
    u8 PTR4* roots;
    u8 PTR4* node_ptr;
    u8 PTR4* next_node_ptr;
    u8 PTR4* tree_end_ptr;
    u8 node;
    u8 base;
    u16* deferred_dest;
    struct
    {
        BPLOSSLESSREADTREE tree;
        BPLOSSLESSCOEFFS coeffs;
    } workspace;
    BPBITSTREAM bitcopy;

    bitcopy = *bits;
#define words bitcopy.cur
#define bitbuf bitcopy.bits
#define bitcount bitcopy.bitlen
    workspace.coeffs.values[BP_COEFF1_INDEX] = 0;
    memset(workspace.coeffs.values + BP_LOSSLESS_CLEAR_START, 0, BP_LOSSLESS_CLEAR_BYTES(workspace.coeffs));

    /* The stream starts with the maximum active lossless bitplane level. */
    VarBitsGet(maxlevel, u8, bitcopy, BP_LOSSLESS_LEVEL_BITS);
    had_level = maxlevel;
    highbit = (u16)BP_LEVEL_MASK(maxlevel);
    /* Root nodes mirror WriteBPLossless: three grouped roots plus coeffs 1..3. */
    roots = workspace.tree.roots;
    roots[BP_ROOT_GROUP1_SLOT] = BP_READ_TREE_GROUP1_ROOT;
    roots[BP_ROOT_GROUP6_SLOT] = BP_READ_TREE_GROUP6_ROOT;
    roots[BP_ROOT_GROUP11_SLOT] = BP_READ_TREE_GROUP11_ROOT;
    roots[BP_ROOT_LOSSLESS_COEFF1_SLOT] = BP_READ_TREE_COEFF1_ROOT;
    roots[BP_ROOT_LOSSLESS_COEFF2_SLOT] = BP_READ_TREE_COEFF2_ROOT;
    roots[BP_ROOT_LOSSLESS_COEFF3_SLOT] = BP_READ_TREE_COEFF3_ROOT;

    next_node_ptr = roots;
    tree_end_ptr = workspace.tree.nodes;

    /* Non-final planes read lower magnitude bits plus a sign for new coeffs. */
    while (1 < maxlevel) {
        level = BP_NEXT_LEVEL(maxlevel);
        node_ptr = next_node_ptr;
        next_highbit = (s16)highbit >> 1;
        if (node_ptr < tree_end_ptr) {
            bit_mask = GetBitsLen(level);
            do {
                node = *node_ptr;
                if (node == BP_READ_TREE_EMPTY_ENTRY) {
                    goto next_lossless_read_node;
                } else {
                    if (bitcount != 0) {
                        bitcount = bitcount - 1;
                        code = bitbuf & BP_BIT_MASK;
                        bitbuf >>= 1;
                        if (code != 0) {
                            goto decode_lossless_node;
                        }
                        goto next_lossless_read_node;
                    } else {
                        u32 word = *words;
                        bitcount = BP_WORD_TOP_BIT;
                        words++;
                        bitbuf = word >> 1;
                        if ((word & BP_BIT_MASK) == 0) {
                            goto next_lossless_read_node;
                        }
                    }

decode_lossless_node:
                    switch (BP_READ_TREE_KIND(node)) {
                    case BP_READ_TREE_HIGH_NODE:
                        base = BP_READ_TREE_INDEX(node);
                        *node_ptr = BP_READ_TREE_GROUP_FROM_INDEX(base);
                        goto decoded_lossless_children;
                    case BP_READ_TREE_GROUP_NODE:
                        node = BP_READ_TREE_INDEX(node);
                        *node_ptr = BP_READ_TREE_BRANCH(node);
                        *tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD1_BASE);
                        *++tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD2_BASE);
                        *++tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD3_BASE);
                        ++tree_end_ptr;
                        goto after_lossless_read_children;
                    case BP_READ_TREE_BRANCH_NODE:
                        *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                        node_ptr++;
                        goto handle_lossless_read_children;
                    case BP_READ_TREE_COEFF_NODE:
                        goto deferred_lossless_coeff;
                    default:
                        goto next_lossless_read_node;
                    }

handle_lossless_read_children:
                    base = BP_READ_TREE_INDEX(node);
decoded_lossless_children:
#define READ_LOSSLESS_CHILD(slot, label)                                                                            \
                        do {                                                                                        \
                            u16* coeff_dest;                                                                        \
                            if (bitcount != 0) {                                                                    \
                                bitcount = bitcount - 1;                                                            \
                                code = bitbuf & BP_BIT_MASK;                                                        \
                                bitbuf >>= 1;                                                                       \
                                if (code != 0) {                                                                    \
                                    goto label##_push;                                                              \
                                }                                                                                   \
                            } else {                                                                                \
                                u32 word = *words;                                                                  \
                                bitcount = BP_WORD_TOP_BIT;                                                         \
                                bitbuf = word >> 1;                                                                 \
                                words++;                                                                            \
                                if ((word & BP_BIT_MASK) != 0) {                                                    \
label##_push:                                                                                                       \
                                    next_node_ptr--;                                                                \
                                    *next_node_ptr = BP_READ_TREE_COEFF(slot);                                      \
                                    goto label;                                                                     \
                                }                                                                                   \
                            }                                                                                       \
                            if (bitcount >= level) {                                                                \
                                coeff_value = bitbuf & bit_mask;                                                    \
                                bitbuf >>= level;                                                                   \
                                bitcount = bitcount - level;                                                        \
                            } else {                                                                                \
                                code = *words; \
                                coeff_value = (bitbuf | (code << bitcount)) & bit_mask; \
                                bitbuf = code >> (level - bitcount); \
                                bitcount = bitcount + BP_BITS_PER_WORD - level; \
                                words++; \
                            }                                                                                       \
                            coeff_value = coeff_value | highbit;                                                    \
                            coeff_dest = &workspace.coeffs.values[slot];                                                      \
                            if (bitcount != 0) {                                                                    \
                                bitcount = bitcount - 1;                                                            \
                                code = bitbuf & BP_BIT_MASK;                                                        \
                                bitbuf >>= 1;                                                                       \
                                if (code != 0) {                                                                    \
                                    goto label##_negative;                                                          \
                                }                                                                                   \
                                goto label##_positive;                                                              \
                            } else {                                                                                \
                                u32 word = *words;                                                                  \
                                bitcount = BP_WORD_TOP_BIT;                                                         \
                                words++;                                                                            \
                                bitbuf = word >> 1;                                                                 \
                                if ((word & BP_BIT_MASK) == 0) {                                                    \
                                    goto label##_positive;                                                          \
                                }                                                                                   \
                            }                                                                                       \
label##_negative:                                                                                                   \
                            code = -coeff_value;                                                                    \
                            goto label##_store;                                                                     \
label##_positive:                                                                                                   \
                            code = coeff_value;                                                                     \
label##_store:                                                                                                      \
                            *coeff_dest = (u16)code;                                                                \
                        } while (0)
                        READ_LOSSLESS_CHILD(base, after_lossless_child0);
after_lossless_child0:
                        base++;
                        READ_LOSSLESS_CHILD(base, after_lossless_child1);
after_lossless_child1:
                        base++;
                        READ_LOSSLESS_CHILD(base, after_lossless_child2);
after_lossless_child2:
                        base++;
                        READ_LOSSLESS_CHILD(base, after_lossless_child3);
after_lossless_child3:
#undef READ_LOSSLESS_CHILD
                    ;
after_lossless_read_children:
                    ;
                }
                goto lossless_node_done;
deferred_lossless_coeff:
                node = BP_READ_TREE_INDEX(node);
                if (bitcount >= level) {
                    coeff_value = bitbuf & bit_mask;
                    bitbuf >>= level;
                    bitcount = bitcount - level;
                } else {
                    code = *words;
                    coeff_value = (bitbuf | (code << bitcount)) & bit_mask;
                    bitbuf = code >> (level - bitcount);
                    bitcount = bitcount + BP_BITS_PER_WORD - level;
                    words++;
                }
                coeff_value = coeff_value | highbit;
                deferred_dest = &workspace.coeffs.values[node];
                if (bitcount != 0) {
                    bitcount = bitcount - 1;
                    code = bitbuf & BP_BIT_MASK;
                    bitbuf >>= 1;
                    if (code != 0) {
                        goto lossless_deferred_negative;
                    }
                    goto lossless_deferred_positive;
                } else {
                    u32 word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words++;
                    bitbuf = word >> 1;
                    if ((word & BP_BIT_MASK) == 0) {
                        goto lossless_deferred_positive;
                    }
                }
lossless_deferred_negative:
                code = -coeff_value;
                goto lossless_deferred_store;
lossless_deferred_positive:
                code = coeff_value;
lossless_deferred_store:
                *deferred_dest = (u16)code;
                *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                goto next_lossless_read_node;
next_lossless_read_node:
                node_ptr++;
lossless_node_done:
                ;
            } while (node_ptr < tree_end_ptr);
        }
        highbit = next_highbit;
        maxlevel = level;
    }

    /* Level one coeffs need only a sign bit; their magnitude is implicit. */
    if (had_level && next_node_ptr < tree_end_ptr) {
        node_ptr = next_node_ptr;
        do {
            node = *node_ptr;
            if (node == BP_READ_TREE_EMPTY_ENTRY) {
                goto next_lossless_final_node;
            } else {
                if (bitcount != 0) {
                    bitcount = bitcount - 1;
                    code = bitbuf & BP_BIT_MASK;
                    bitbuf >>= 1;
                    if (code != 0) {
                        goto decode_lossless_final_node;
                    }
                    goto next_lossless_final_node;
                } else {
                    u32 word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words++;
                    bitbuf = word >> 1;
                    if ((word & BP_BIT_MASK) == 0) {
                        goto next_lossless_final_node;
                    }
                }
decode_lossless_final_node:
                switch (BP_READ_TREE_KIND(node)) {
                case BP_READ_TREE_HIGH_NODE:
                    base = BP_READ_TREE_INDEX(node);
                    *node_ptr = BP_READ_TREE_GROUP_FROM_INDEX(base);
                    goto decoded_lossless_final_children;
                case BP_READ_TREE_GROUP_NODE:
                    node = BP_READ_TREE_INDEX(node);
                    *node_ptr = BP_READ_TREE_BRANCH(node);
                    *tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD1_BASE);
                    *++tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD2_BASE);
                    *++tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD3_BASE);
                    ++tree_end_ptr;
                    goto after_lossless_final_children;
                case BP_READ_TREE_BRANCH_NODE:
                    *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                    node_ptr++;
                    goto handle_lossless_final_children;
                case BP_READ_TREE_COEFF_NODE:
                    goto deferred_lossless_final;
                default:
                    goto next_lossless_final_node;
                }

handle_lossless_final_children:
                base = BP_READ_TREE_INDEX(node);
decoded_lossless_final_children:
#define READ_LOSSLESS_FINAL_CHILD(slot, label)                                                                      \
                    do {                                                                                            \
                        if (bitcount != 0) {                                                                        \
                            bitcount = bitcount - 1;                                                                \
                            code = bitbuf & BP_BIT_MASK;                                                            \
                            bitbuf >>= 1;                                                                           \
                            if (code != 0) {                                                                        \
                                goto label##_push;                                                                  \
                            }                                                                                       \
                        } else {                                                                                    \
                            u32 word = *words;                                                                      \
                            bitcount = BP_WORD_TOP_BIT;                                                             \
                            bitbuf = word >> 1;                                                                     \
                            words++;                                                                                \
                            if ((word & BP_BIT_MASK) != 0) {                                                        \
label##_push:                                                                                                       \
                                next_node_ptr--;                                                                    \
                                *next_node_ptr = BP_READ_TREE_COEFF(slot);                                          \
                                goto label;                                                                         \
                            }                                                                                       \
                        }                                                                                           \
                        if (bitcount != 0) {                                                                        \
                            bitcount = bitcount - 1;                                                                \
                            code = bitbuf & BP_BIT_MASK;                                                            \
                            bitbuf >>= 1;                                                                           \
                            if (code != 0) {                                                                        \
                                goto label##_negative;                                                              \
                            }                                                                                       \
                            goto label##_positive;                                                                  \
                        } else {                                                                                    \
                            u32 word = *words;                                                                      \
                            bitcount = BP_WORD_TOP_BIT;                                                             \
                            words++;                                                                                \
                            bitbuf = word >> 1;                                                                     \
                            if ((word & BP_BIT_MASK) == 0) {                                                        \
                                goto label##_positive;                                                              \
                            }                                                                                       \
                        }                                                                                           \
label##_negative:                                                                                                   \
                        code = BP_NEGATIVE_COEFF_SIGN;                                                              \
                        goto label##_store;                                                                         \
label##_positive:                                                                                                   \
                        code = BP_POSITIVE_COEFF_SIGN;                                                              \
label##_store:                                                                                                      \
                        workspace.coeffs.values[slot] = (u16)code;                                                            \
                    } while (0)
                    READ_LOSSLESS_FINAL_CHILD(base, after_lossless_final0);
after_lossless_final0:
                    base++;
                    READ_LOSSLESS_FINAL_CHILD(base, after_lossless_final1);
after_lossless_final1:
                    base++;
                    READ_LOSSLESS_FINAL_CHILD(base, after_lossless_final2);
after_lossless_final2:
                    base++;
                    READ_LOSSLESS_FINAL_CHILD(base, after_lossless_final3);
after_lossless_final3:
#undef READ_LOSSLESS_FINAL_CHILD
                ;
after_lossless_final_children:
                ;
            }
            goto lossless_final_done;
deferred_lossless_final:
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                code = bitbuf & BP_BIT_MASK;
                bitbuf >>= 1;
                if (code != 0) {
                    goto lossless_final_negative;
                }
                goto lossless_final_positive;
            } else {
                u32 word = *words;
                bitcount = BP_WORD_TOP_BIT;
                words++;
                bitbuf = word >> 1;
                if ((word & BP_BIT_MASK) == 0) {
                    goto lossless_final_positive;
                }
            }
lossless_final_negative:
            code = BP_NEGATIVE_COEFF_SIGN;
            goto lossless_final_store;
lossless_final_positive:
            code = BP_POSITIVE_COEFF_SIGN;
lossless_final_store:
            workspace.coeffs.values[BP_READ_TREE_INDEX(node)] = (u16)code;
            *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
            goto next_lossless_final_node;
next_lossless_final_node:
            node_ptr++;
lossless_final_done:
            ;
        } while (node_ptr < tree_end_ptr);
    }

    bitcopy.cur = words;
    bitcopy.bits = bitbuf;
    bitcopy.bitlen = bitcount;
    *bits = bitcopy;
#undef words
#undef bitbuf
#undef bitcount
    (void)workspace.tree;

    /* Scatter scan-order coefficients back into the 8x8 block. */
    out[BP_COEFF1_INDEX] = workspace.coeffs.values[BP_COEFF1_INDEX];
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(1), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(2));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(2), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(4));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(3), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(6));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(4), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(1));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(5), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(3));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(6), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(5));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(7), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(7));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(8), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(12));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(9), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(22));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(10), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(8));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(11), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(10));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(12), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(13));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(13), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(23));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(14), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(9));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(15), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(11));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(16), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(14));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(17), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(16));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(18), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(24));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(19), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(26));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(20), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(15));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(21), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(17));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(22), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(25));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(23), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(27));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(24), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(18));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(25), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(20));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(26), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(28));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(27), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(30));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(28), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(19));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(29), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(21));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(30), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(29));
    BP_SCATTER_LOSSLESS_PAIR(out, BP_LOSSLESS_OUT_PAIR(31), workspace.coeffs, BP_LOSSLESS_SCAN_PAIR(31));
}

u32 WriteBPLossy(BPBITSTREAM PTR4* bits, char PTR4* vals)
{
    u16 entry;
    s32 coeff;
    s32 sign;
    s32 i;
    s32 active_count;
    s32 count;
    u32 maxbits;
    u32 level;
    u32 lenbits;
    u32 bit_count;
    BPBITSTYPE bit_buf;
    u16 PTR4* cur;
    u16 PTR4* insert;
    u16 PTR4* next_node;
    u16 PTR4* roots;
    s16 bit_mask;
    u16 node_entry;
    u8 PTR4* child_lens;
    BPLOSSYWRITETREE tree;
    u8 lens[BP_BLOCK_COEFFS];
    u8 groups[BP_TREE_GROUPS];
    u8 ordered[BP_BLOCK_COEFFS];
    u8 absvals[BP_BLOCK_COEFFS];
    u8 active_absvals[BP_BLOCK_COEFFS];
    u8 hi_groups[BP_TREE_HIGH_GROUPS];

    /* Lossy bitplanes scan all 64 byte coefficients, including DC. */
    count = BP_BLOCK_COEFFS;
    i = 0;
    do {
        ordered[i] = BP_ZIGZAG_COEFF(vals, i);
        i++;
        count--;
    } while (count != 0);

    i = 0;
    count = BP_BLOCK_COEFFS;
    do {
        coeff = (s8)ordered[i];
        sign = coeff >> BP_S32_SIGN_SHIFT;
        absvals[i] = (u8)BP_ABS_COEFF(coeff, sign);
        i++;
        count--;
    } while (count != 0);

    maxbits = 0;
    i = 0;
    count = BP_BLOCK_COEFFS;
    do {
        u32 coeff_bits = BP_COEFF_BIT_LEVEL((u32)absvals[i]);
        if (coeff_bits > maxbits) {
            maxbits = coeff_bits;
        }
        lens[i] = (u8)coeff_bits;
        i++;
        count--;
    } while (count != 0);

    if (maxbits == 0) {
        return 0;
    }

    /* Group tables use the deepest bit depth of each four-coefficient branch. */
    count = BP_TREE_GROUPS;
    i = 0;
    do {
        lenbits = lens[i];
        if (lenbits < lens[i + BP_TREE_CHILD1_INDEX]) {
            lenbits = lens[i + BP_TREE_CHILD1_INDEX];
        }
        if (lenbits < lens[i + BP_TREE_CHILD2_INDEX]) {
            lenbits = lens[i + BP_TREE_CHILD2_INDEX];
        }
        if (lenbits < lens[i + BP_TREE_CHILD3_INDEX]) {
            lenbits = lens[i + BP_TREE_CHILD3_INDEX];
        }
        groups[BP_TREE_GROUP_INDEX(i)] = (u8)lenbits;
        i += BP_TREE_CHILD_COUNT;
        count--;
    } while (count != 0);

    {
        u32 group_bits;
        BP_TREE_HIGH_GROUP_MAX(groups, group_bits, BP_TREE_HIGH_GROUP0_INDEX, BP_TREE_HIGH_GROUP0_CHILD0_INDEX);
        hi_groups[BP_TREE_HIGH_GROUP0_SLOT] = (u8)group_bits;
        BP_TREE_HIGH_GROUP_MAX(groups, group_bits, BP_TREE_HIGH_GROUP1_INDEX, BP_TREE_HIGH_GROUP1_CHILD0_INDEX);
        hi_groups[BP_TREE_HIGH_GROUP1_SLOT] = (u8)group_bits;
        BP_TREE_HIGH_GROUP_MAX(groups, group_bits, BP_TREE_HIGH_GROUP2_INDEX, BP_TREE_HIGH_GROUP2_CHILD0_INDEX);
        hi_groups[BP_TREE_HIGH_GROUP2_SLOT] = (u8)group_bits;

    }

    bit_count = BP_STREAM_BITLEN(bits) + BP_LOSSY_LEVEL_BITS;
    lenbits = BP_LOSSY_LEVEL_CODE(maxbits);
    bit_buf = BP_STREAM_BITS(bits) | (lenbits << BP_STREAM_BITLEN(bits));
    BP_STREAM_BITLEN(bits) = bit_count;
    BP_STREAM_BITS(bits) = bit_buf;
    if (bit_count >= BP_BITS_PER_WORD) {
        *BP_STREAM_CUR(bits) = bit_buf;
        BP_STREAM_CUR(bits) = BP_STREAM_CUR(bits) + 1;
        bit_buf = BP_STREAM_BITLEN(bits) - BP_BITS_PER_WORD;
        BP_STREAM_BITLEN(bits) = bit_buf;
        if (bit_buf != 0) {
            BP_STREAM_BITS(bits) = lenbits >> (BP_LOSSY_LEVEL_BITS - bit_buf);
        } else {
            BP_STREAM_BITS(bits) = 0;
        }
    }

    roots = tree.roots;
    {
        u32 group_bits;
        group_bits = hi_groups[BP_TREE_HIGH_GROUP0_SLOT];
        if (group_bits > groups[BP_TREE_GROUP1_INDEX]) {
            entry = BP_TREE_GROUP_ENTRY(group_bits, BP_GROUP1_NODE_BASE);
        } else {
            entry = BP_TREE_GROUP_ENTRY(groups[BP_TREE_GROUP1_INDEX], BP_GROUP1_NODE_BASE);
        }
        roots[BP_ROOT_GROUP1_SLOT] = entry;
        group_bits = hi_groups[BP_TREE_HIGH_GROUP1_SLOT];
        if (group_bits > groups[BP_TREE_GROUP6_INDEX]) {
            entry = BP_TREE_GROUP_ENTRY(group_bits, BP_GROUP6_NODE_BASE);
        } else {
            entry = BP_TREE_GROUP_ENTRY(groups[BP_TREE_GROUP6_INDEX], BP_GROUP6_NODE_BASE);
        }
        roots[BP_ROOT_GROUP6_SLOT] = entry;
        group_bits = hi_groups[BP_TREE_HIGH_GROUP2_SLOT];
        if (group_bits > groups[BP_TREE_GROUP11_INDEX]) {
            entry = BP_TREE_GROUP_ENTRY(group_bits, BP_GROUP11_NODE_BASE);
        } else {
            entry = BP_TREE_GROUP_ENTRY(groups[BP_TREE_GROUP11_INDEX], BP_GROUP11_NODE_BASE);
        }
        roots[BP_ROOT_GROUP11_SLOT] = entry;
    }
    roots[BP_ROOT_LOSSY_DC_SLOT] = groups[BP_TREE_GROUP_INDEX(BP_DC_COEFF)] + BP_TREE_BRANCH_NODE;

    insert = roots;
    next_node = roots + BP_LOSSY_ROOT_NODES;
    bit_mask = (u16)BP_LEVEL_MASK(maxbits);
    active_count = 0;
    level = maxbits;
    for (; level != 0; level = BP_NEXT_LEVEL(level)) {
        count = 0;
        /* Coefficients introduced on earlier planes emit one residual bit here. */
        if (count < active_count) {
            do {
                u32 active_value = active_absvals[count];
                count++;
                PUT_BP_BIT(bits, (active_value & bit_mask) != 0);
            } while (count < active_count);
        }

        cur = insert;
        if (cur < next_node) {
            do {
                node_entry = *cur;
                if (node_entry != BP_TREE_EMPTY_ENTRY) {
                    PUT_BP_BIT(bits, BP_TREE_ENTRY_LEVEL(node_entry) == level);
                    if (BP_TREE_ENTRY_LEVEL(node_entry) != level) {
                        goto next_lossy_node;
                    }

                    switch (BP_TREE_ENTRY_KIND(node_entry)) {
                    case BP_TREE_HIGH_NODE:
                        node_entry = BP_TREE_ENTRY_INDEX(node_entry);
                        *cur = BP_TREE_HIGH_GROUP_ENTRY(hi_groups[node_entry >> (BP_TREE_HIGH_GROUP_SHIFT - BP_TREE_INDEX_SHIFT)], node_entry);
                        goto decoded_lossy_write_children;
                    case BP_TREE_GROUP_NODE:
                        node_entry = BP_TREE_ENTRY_INDEX(node_entry);
                        count = node_entry >> (BP_TREE_GROUP_SHIFT - BP_TREE_INDEX_SHIFT);
                        *cur = BP_TREE_BRANCH_ENTRY(groups[count], node_entry);
                        *next_node = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD1_INDEX], node_entry, BP_TREE_CHILD1_BASE);
                        *++next_node = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD2_INDEX], node_entry, BP_TREE_CHILD2_BASE);
                        *++next_node = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD3_INDEX], node_entry, BP_TREE_CHILD3_BASE);
                        ++next_node;
                        break;
                    case BP_TREE_BRANCH_NODE:
                        *cur = BP_TREE_EMPTY_ENTRY;
                        cur++;
                        node_entry = BP_TREE_ENTRY_INDEX(node_entry);
decoded_lossy_write_children:
                        child_lens = lens + node_entry;
                        PUT_BP_BIT(bits, *child_lens != level);
                        if (*child_lens != level) {
                            --insert;
                            *insert = BP_TREE_COEFF_ENTRY(*child_lens, node_entry);
                        } else {
                            active_absvals[active_count] = absvals[node_entry];
                            active_count++;
                            PUT_BP_BIT(bits, (ordered[node_entry] & BP_SIGN_BIT) != 0);
                        }

                        node_entry++;
                        child_lens++;
                        PUT_BP_BIT(bits, *child_lens != level);
                        entry = *child_lens;
                        if (entry != level) {
                            --insert;
                            *insert = BP_TREE_COEFF_ENTRY(entry, node_entry);
                        } else {
                            active_absvals[active_count] = absvals[node_entry];
                            active_count++;
                            PUT_BP_BIT(bits, (ordered[node_entry] & BP_SIGN_BIT) != 0);
                        }

                        node_entry++;
                        child_lens++;
                        PUT_BP_BIT(bits, *child_lens != level);
                        entry = *child_lens;
                        if (entry != level) {
                            --insert;
                            *insert = BP_TREE_COEFF_ENTRY(entry, node_entry);
                        } else {
                            active_absvals[active_count] = absvals[node_entry];
                            active_count++;
                            PUT_BP_BIT(bits, (ordered[node_entry] & BP_SIGN_BIT) != 0);
                        }

                        node_entry++;
                        child_lens++;
                        PUT_BP_BIT(bits, *child_lens != level);
                        entry = *child_lens;
                        if (entry != level) {
                            --insert;
                            *insert = BP_TREE_COEFF_ENTRY(entry, node_entry);
                        } else {
                            active_absvals[active_count] = absvals[node_entry];
                            active_count++;
                            PUT_BP_BIT(bits, (ordered[node_entry] & BP_SIGN_BIT) != 0);
                        }
                        break;
                    case BP_TREE_COEFF_NODE:
                        node_entry = BP_TREE_ENTRY_INDEX(node_entry);
                        active_absvals[active_count] = absvals[node_entry];
                        active_count++;
                        PUT_BP_BIT(bits, (ordered[node_entry] & BP_SIGN_BIT) != 0);
                        *cur = BP_TREE_EMPTY_ENTRY;
                        goto next_lossy_node;
                    default:
                        goto next_lossy_node;
                    }
                    continue;
                }
next_lossy_node:
                cur++;
            } while (cur < next_node);
        }
        bit_mask = (s16)bit_mask >> 1;
    }

    return 1;
}

#pragma dont_inline on
static void readlossy(s8 PTR4* dest, BPBITSTREAM PTR4* bits, s32 masks_count)
{
    s8 sample;
    u32 levels_remaining;
    u8 PTR4* tree_end_ptr;
    s8 mask;
    s32 masks_used;
    s32 nz_coeff_count;
    u8 PTR4* node_ptr;
    u8 node;
    s32 delta;
    s32 scan;
    s32 negative_mask;
    BPBITSTYPE word;
    u8 PTR4* next_node_ptr;
    u8 PTR4* roots;
    BPLOSSYREADTREE tree;
    u8 nz_coeff[BP_BLOCK_COEFFS];
    BPBITSTREAM bitcopy;

    bitcopy = *bits;
#define words bitcopy.cur
#define bitbuf bitcopy.bits
#define bitcount bitcopy.bitlen
    masks_used = 0;
    memset(dest, 0, BP_LOSSY_BLOCK_BYTES);

    /* Lossy blocks store max level minus one in the stream header. */
    VarBitsGet(levels_remaining, u8, bitcopy, BP_LOSSY_LEVEL_BITS);
    levels_remaining = BP_LOSSY_LEVEL_COUNT(levels_remaining);
    roots = tree.roots;
    roots[BP_ROOT_GROUP1_SLOT] = BP_READ_TREE_GROUP1_ROOT;
    roots[BP_ROOT_GROUP6_SLOT] = BP_READ_TREE_GROUP6_ROOT;
    roots[BP_ROOT_GROUP11_SLOT] = BP_READ_TREE_GROUP11_ROOT;
    mask = (s32)(s8)BP_LEVEL_MASK(levels_remaining);
    roots[BP_ROOT_LOSSY_DC_SLOT] = BP_READ_TREE_DC_ROOT;
    tree_end_ptr = tree.nodes;
    nz_coeff_count = 0;
    next_node_ptr = roots;
    while (levels_remaining != 0) {
        scan = 0;
        /* Active coefficients receive one refinement bit at each lower plane. */
        if (scan < nz_coeff_count) {
            do {
                if (bitcount != 0) {
                    bitcount = bitcount - 1;
                    word = bitbuf;
                    bitbuf = bitbuf >> 1;
                    if ((word & BP_BIT_MASK) != 0) {
                        goto refine_coeff;
                    }
                    goto next_refinement;
                } else {
                    word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words = words + 1;
                    bitbuf = word >> 1;
                    if ((word & BP_BIT_MASK) == 0) {
                        goto next_refinement;
                    }
                }
refine_coeff:
                sample = dest[(u32)nz_coeff[scan]];
                if (sample < 0) {
                    dest[(u32)nz_coeff[scan]] = sample - mask;
                } else {
                    dest[(u32)nz_coeff[scan]] = sample + mask;
                }
                if (masks_used++ == masks_count) {
                    goto done;
                }
next_refinement:
                scan = scan + 1;
            } while (scan < nz_coeff_count);
        }
        node_ptr = next_node_ptr;
        if (node_ptr < tree_end_ptr) {
            negative_mask = -mask;
read_node:
            node = *node_ptr;
            if (node == BP_READ_TREE_EMPTY_ENTRY) {
                goto next_node;
            }
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto decode_node;
                }
                goto next_node;
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                words = words + 1;
                bitbuf = word >> 1;
                if ((word & BP_BIT_MASK) == 0) {
                    goto next_node;
                }
            }
decode_node:
            switch (BP_READ_TREE_KIND(node)) {
            case BP_READ_TREE_HIGH_NODE:
                node = BP_READ_TREE_INDEX(node);
                *node_ptr = BP_READ_TREE_GROUP_FROM_INDEX(node);
                goto decode_children;
            case BP_READ_TREE_GROUP_NODE:
                node = BP_READ_TREE_INDEX(node);
                *node_ptr = BP_READ_TREE_BRANCH(node);
                *tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD1_BASE);
                *++tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD2_BASE);
                *++tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node, BP_READ_TREE_CHILD3_BASE);
                ++tree_end_ptr;
                goto node_done;
            case BP_READ_TREE_BRANCH_NODE:
                *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                node_ptr = node_ptr + 1;
                break;
            case BP_READ_TREE_COEFF_NODE:
                goto deferred_coeff;
            default:
                goto next_node;
            }
            node = BP_READ_TREE_INDEX(node);
decode_children:
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto push_0;
                }
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                bitbuf = word >> 1;
                words = words + 1;
                if ((word & BP_BIT_MASK) != 0) {
push_0:
                    *--next_node_ptr = BP_READ_TREE_COEFF(node);
                    goto after_0;
                }
            }
            nz_coeff[nz_coeff_count] = node;
            /* A zero child-presence bit introduces the coefficient immediately. */
            nz_coeff_count = nz_coeff_count + 1;
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto negative_0;
                }
                goto positive_0;
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                words = words + 1;
                bitbuf = word >> 1;
                if ((word & BP_BIT_MASK) == 0) {
                    goto positive_0;
                }
            }
negative_0:
            delta = negative_mask;
            goto store_0;
positive_0:
            delta = mask;
store_0:
            dest[(u32)node] = (s8)delta;
            if (masks_used++ == masks_count) {
                goto done;
            }
after_0:
            node++;
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto push_1;
                }
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                bitbuf = word >> 1;
                words = words + 1;
                if ((word & BP_BIT_MASK) != 0) {
push_1:
                    *--next_node_ptr = BP_READ_TREE_COEFF(node);
                    goto after_1;
                }
            }
            nz_coeff[nz_coeff_count] = node;
            /* Nonzero children are pushed for later planes instead. */
            nz_coeff_count = nz_coeff_count + 1;
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto negative_1;
                }
                goto positive_1;
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                words = words + 1;
                bitbuf = word >> 1;
                if ((word & BP_BIT_MASK) == 0) {
                    goto positive_1;
                }
            }
negative_1:
            delta = negative_mask;
            goto store_1;
positive_1:
            delta = mask;
store_1:
            dest[(u32)node] = (s8)delta;
            if (masks_used++ == masks_count) {
                goto done;
            }
after_1:
            node++;
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto push_2;
                }
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                bitbuf = word >> 1;
                words = words + 1;
                if ((word & BP_BIT_MASK) != 0) {
push_2:
                    *--next_node_ptr = BP_READ_TREE_COEFF(node);
                    goto after_2;
                }
            }
            nz_coeff[nz_coeff_count] = node;
            nz_coeff_count = nz_coeff_count + 1;
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto negative_2;
                }
                goto positive_2;
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                words = words + 1;
                bitbuf = word >> 1;
                if ((word & BP_BIT_MASK) == 0) {
                    goto positive_2;
                }
            }
negative_2:
            delta = negative_mask;
            goto store_2;
positive_2:
            delta = mask;
store_2:
            dest[(u32)node] = (s8)delta;
            if (masks_used++ == masks_count) {
                goto done;
            }
after_2:
            node++;
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto push_3;
                }
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                bitbuf = word >> 1;
                words = words + 1;
                if ((word & BP_BIT_MASK) != 0) {
push_3:
                    *--next_node_ptr = BP_READ_TREE_COEFF(node);
                    goto node_done;
                }
            }
            nz_coeff[nz_coeff_count] = node;
            nz_coeff_count = nz_coeff_count + 1;
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto negative_3;
                }
                goto positive_3;
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                words = words + 1;
                bitbuf = word >> 1;
                if ((word & BP_BIT_MASK) == 0) {
                    goto positive_3;
                }
            }
negative_3:
            delta = negative_mask;
            goto store_3;
positive_3:
            delta = mask;
store_3:
            dest[(u32)node] = (s8)delta;
            if (masks_used++ == masks_count) {
                goto done;
            }
            goto node_done;
deferred_coeff:
            node = BP_READ_TREE_INDEX(node);
            nz_coeff[nz_coeff_count] = node;
            /* Deferred coeff nodes already carry their scan index. */
            nz_coeff_count = nz_coeff_count + 1;
            if (bitcount != 0) {
                bitcount = bitcount - 1;
                word = bitbuf;
                bitbuf = bitbuf >> 1;
                if ((word & BP_BIT_MASK) != 0) {
                    goto negative_4;
                }
                goto positive_4;
            } else {
                word = *words;
                bitcount = BP_WORD_TOP_BIT;
                words = words + 1;
                bitbuf = word >> 1;
                if ((word & BP_BIT_MASK) == 0) {
                    goto positive_4;
                }
            }
negative_4:
            delta = negative_mask;
            goto store_4;
positive_4:
            delta = mask;
store_4:
            dest[(u32)node] = (s8)delta;
            if (masks_used++ == masks_count) {
                goto done;
            }
            *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
            goto next_node;
next_node:
            node_ptr = node_ptr + 1;
node_done:
            if (node_ptr < tree_end_ptr) {
                goto read_node;
            }
        }
level_done:
        mask = mask >> 1;
        levels_remaining = BP_NEXT_LEVEL(levels_remaining);
    }

done:
    bitcopy.bitlen = bitcount;
    bitcopy.cur = words;
    bitcopy.bits = bitbuf;
    *bits = bitcopy;
#undef words
#undef bitbuf
#undef bitcount
}
#pragma dont_inline reset

void ReadBPLossy(s16 PTR4* out, BPBITSTREAM PTR4* bits, s32 masks_count)
{
    BPLOSSYBLOCK residuals;

    readlossy(residuals.bytes, bits, masks_count);
#define SCATTER_BP_LOSSY_WORD(out_index, residual_index)                                                              \
    (out[(out_index)] = residuals.pairs[(residual_index)])
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(0), BP_LOSSY_SCAN_PAIR(0));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(1), BP_LOSSY_SCAN_PAIR(2));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(2), BP_LOSSY_SCAN_PAIR(4));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(3), BP_LOSSY_SCAN_PAIR(6));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(4), BP_LOSSY_SCAN_PAIR(1));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(5), BP_LOSSY_SCAN_PAIR(3));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(6), BP_LOSSY_SCAN_PAIR(5));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(7), BP_LOSSY_SCAN_PAIR(7));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(8), BP_LOSSY_SCAN_PAIR(12));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(9), BP_LOSSY_SCAN_PAIR(22));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(10), BP_LOSSY_SCAN_PAIR(8));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(11), BP_LOSSY_SCAN_PAIR(10));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(12), BP_LOSSY_SCAN_PAIR(13));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(13), BP_LOSSY_SCAN_PAIR(23));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(14), BP_LOSSY_SCAN_PAIR(9));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(15), BP_LOSSY_SCAN_PAIR(11));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(16), BP_LOSSY_SCAN_PAIR(14));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(17), BP_LOSSY_SCAN_PAIR(16));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(18), BP_LOSSY_SCAN_PAIR(24));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(19), BP_LOSSY_SCAN_PAIR(26));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(20), BP_LOSSY_SCAN_PAIR(15));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(21), BP_LOSSY_SCAN_PAIR(17));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(22), BP_LOSSY_SCAN_PAIR(25));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(23), BP_LOSSY_SCAN_PAIR(27));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(24), BP_LOSSY_SCAN_PAIR(18));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(25), BP_LOSSY_SCAN_PAIR(20));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(26), BP_LOSSY_SCAN_PAIR(28));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(27), BP_LOSSY_SCAN_PAIR(30));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(28), BP_LOSSY_SCAN_PAIR(19));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(29), BP_LOSSY_SCAN_PAIR(21));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(30), BP_LOSSY_SCAN_PAIR(29));
    SCATTER_BP_LOSSY_WORD(BP_LOSSY_OUT_PAIR(31), BP_LOSSY_SCAN_PAIR(31));
#undef SCATTER_BP_LOSSY_WORD
}

void ReadBPLossyWithMotion(char PTR4* out, s32 pitch, BPBITSTREAM PTR4* bits, s32 masks_count,
                           char PTR4* prev)
{
    BPLOSSYBLOCK residuals;
    char PTR4* dst = out;
    char PTR4* src = prev;

    readlossy(residuals.bytes, bits, masks_count);
#define SCATTER_BP_LOSSY_MOTION(out_index, residual_index, src_index)                                                  \
    (dst[(out_index)] = residuals.bytes[(residual_index)] + src[(src_index)])
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(0), BP_LOSSY_SCAN_SAMPLE(0), BP_BLOCK_SAMPLE(0, 0));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(1), BP_LOSSY_SCAN_SAMPLE(1), BP_BLOCK_SAMPLE(0, 1));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(2), BP_LOSSY_SCAN_SAMPLE(4), BP_BLOCK_SAMPLE(0, 2));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(3), BP_LOSSY_SCAN_SAMPLE(5), BP_BLOCK_SAMPLE(0, 3));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(4), BP_LOSSY_SCAN_SAMPLE(8), BP_BLOCK_SAMPLE(0, 4));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(5), BP_LOSSY_SCAN_SAMPLE(9), BP_BLOCK_SAMPLE(0, 5));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(6), BP_LOSSY_SCAN_SAMPLE(12), BP_BLOCK_SAMPLE(0, 6));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(7), BP_LOSSY_SCAN_SAMPLE(13), BP_BLOCK_SAMPLE(0, 7));
    dst += pitch;
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(0), BP_LOSSY_SCAN_SAMPLE(2), BP_BLOCK_SAMPLE(1, 0));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(1), BP_LOSSY_SCAN_SAMPLE(3), BP_BLOCK_SAMPLE(1, 1));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(2), BP_LOSSY_SCAN_SAMPLE(6), BP_BLOCK_SAMPLE(1, 2));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(3), BP_LOSSY_SCAN_SAMPLE(7), BP_BLOCK_SAMPLE(1, 3));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(4), BP_LOSSY_SCAN_SAMPLE(10), BP_BLOCK_SAMPLE(1, 4));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(5), BP_LOSSY_SCAN_SAMPLE(11), BP_BLOCK_SAMPLE(1, 5));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(6), BP_LOSSY_SCAN_SAMPLE(14), BP_BLOCK_SAMPLE(1, 6));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(7), BP_LOSSY_SCAN_SAMPLE(15), BP_BLOCK_SAMPLE(1, 7));
    dst += pitch;
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(0), BP_LOSSY_SCAN_SAMPLE(24), BP_BLOCK_SAMPLE(2, 0));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(1), BP_LOSSY_SCAN_SAMPLE(25), BP_BLOCK_SAMPLE(2, 1));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(2), BP_LOSSY_SCAN_SAMPLE(44), BP_BLOCK_SAMPLE(2, 2));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(3), BP_LOSSY_SCAN_SAMPLE(45), BP_BLOCK_SAMPLE(2, 3));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(4), BP_LOSSY_SCAN_SAMPLE(16), BP_BLOCK_SAMPLE(2, 4));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(5), BP_LOSSY_SCAN_SAMPLE(17), BP_BLOCK_SAMPLE(2, 5));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(6), BP_LOSSY_SCAN_SAMPLE(20), BP_BLOCK_SAMPLE(2, 6));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(7), BP_LOSSY_SCAN_SAMPLE(21), BP_BLOCK_SAMPLE(2, 7));
    dst = dst + pitch;
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(0), BP_LOSSY_SCAN_SAMPLE(26), BP_BLOCK_SAMPLE(3, 0));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(1), BP_LOSSY_SCAN_SAMPLE(27), BP_BLOCK_SAMPLE(3, 1));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(2), BP_LOSSY_SCAN_SAMPLE(46), BP_BLOCK_SAMPLE(3, 2));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(3), BP_LOSSY_SCAN_SAMPLE(47), BP_BLOCK_SAMPLE(3, 3));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(4), BP_LOSSY_SCAN_SAMPLE(18), BP_BLOCK_SAMPLE(3, 4));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(5), BP_LOSSY_SCAN_SAMPLE(19), BP_BLOCK_SAMPLE(3, 5));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(6), BP_LOSSY_SCAN_SAMPLE(22), BP_BLOCK_SAMPLE(3, 6));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(7), BP_LOSSY_SCAN_SAMPLE(23), BP_BLOCK_SAMPLE(3, 7));
    dst = dst + pitch;
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(0), BP_LOSSY_SCAN_SAMPLE(28), BP_BLOCK_SAMPLE(4, 0));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(1), BP_LOSSY_SCAN_SAMPLE(29), BP_BLOCK_SAMPLE(4, 1));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(2), BP_LOSSY_SCAN_SAMPLE(32), BP_BLOCK_SAMPLE(4, 2));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(3), BP_LOSSY_SCAN_SAMPLE(33), BP_BLOCK_SAMPLE(4, 3));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(4), BP_LOSSY_SCAN_SAMPLE(48), BP_BLOCK_SAMPLE(4, 4));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(5), BP_LOSSY_SCAN_SAMPLE(49), BP_BLOCK_SAMPLE(4, 5));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(6), BP_LOSSY_SCAN_SAMPLE(52), BP_BLOCK_SAMPLE(4, 6));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(7), BP_LOSSY_SCAN_SAMPLE(53), BP_BLOCK_SAMPLE(4, 7));
    dst = dst + pitch;
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(0), BP_LOSSY_SCAN_SAMPLE(30), BP_BLOCK_SAMPLE(5, 0));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(1), BP_LOSSY_SCAN_SAMPLE(31), BP_BLOCK_SAMPLE(5, 1));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(2), BP_LOSSY_SCAN_SAMPLE(34), BP_BLOCK_SAMPLE(5, 2));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(3), BP_LOSSY_SCAN_SAMPLE(35), BP_BLOCK_SAMPLE(5, 3));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(4), BP_LOSSY_SCAN_SAMPLE(50), BP_BLOCK_SAMPLE(5, 4));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(5), BP_LOSSY_SCAN_SAMPLE(51), BP_BLOCK_SAMPLE(5, 5));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(6), BP_LOSSY_SCAN_SAMPLE(54), BP_BLOCK_SAMPLE(5, 6));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(7), BP_LOSSY_SCAN_SAMPLE(55), BP_BLOCK_SAMPLE(5, 7));
    dst = dst + pitch;
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(0), BP_LOSSY_SCAN_SAMPLE(36), BP_BLOCK_SAMPLE(6, 0));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(1), BP_LOSSY_SCAN_SAMPLE(37), BP_BLOCK_SAMPLE(6, 1));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(2), BP_LOSSY_SCAN_SAMPLE(40), BP_BLOCK_SAMPLE(6, 2));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(3), BP_LOSSY_SCAN_SAMPLE(41), BP_BLOCK_SAMPLE(6, 3));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(4), BP_LOSSY_SCAN_SAMPLE(56), BP_BLOCK_SAMPLE(6, 4));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(5), BP_LOSSY_SCAN_SAMPLE(57), BP_BLOCK_SAMPLE(6, 5));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(6), BP_LOSSY_SCAN_SAMPLE(60), BP_BLOCK_SAMPLE(6, 6));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(7), BP_LOSSY_SCAN_SAMPLE(61), BP_BLOCK_SAMPLE(6, 7));
    dst = dst + pitch;
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(0), BP_LOSSY_SCAN_SAMPLE(38), BP_BLOCK_SAMPLE(7, 0));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(1), BP_LOSSY_SCAN_SAMPLE(39), BP_BLOCK_SAMPLE(7, 1));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(2), BP_LOSSY_SCAN_SAMPLE(42), BP_BLOCK_SAMPLE(7, 2));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(3), BP_LOSSY_SCAN_SAMPLE(43), BP_BLOCK_SAMPLE(7, 3));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(4), BP_LOSSY_SCAN_SAMPLE(58), BP_BLOCK_SAMPLE(7, 4));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(5), BP_LOSSY_SCAN_SAMPLE(59), BP_BLOCK_SAMPLE(7, 5));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(6), BP_LOSSY_SCAN_SAMPLE(62), BP_BLOCK_SAMPLE(7, 6));
    SCATTER_BP_LOSSY_MOTION(BP_MOTION_OUT_COLUMN(7), BP_LOSSY_SCAN_SAMPLE(63), BP_BLOCK_SAMPLE(7, 7));
#undef SCATTER_BP_LOSSY_MOTION
}
