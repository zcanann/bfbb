#include "bink.h"
#include "bitplane.h"
#include "dct.h"
#include "varbits.h"

#define BP_BITS_PER_WORD BPBITSTYPELEN
#define BP_WORD_TOP_BIT (BP_BITS_PER_WORD - 1)
#define BP_S32_SIGN_SHIFT BP_WORD_TOP_BIT
#define BP_S16_SIGN_SHIFT 15
#define BP_S8_SIGN_SHIFT 7
#define BP_BLOCK_COEFFS 64
#define BP_DC_COEFF 0
#define BP_FIRST_AC_COEFF (BP_DC_COEFF + 1)
#define BP_AC_COEFFS (BP_BLOCK_COEFFS - BP_FIRST_AC_COEFF)
#define BP_LOSSY_OUTPUT_COEFFS 32
#define BP_TREE_NODES 68
#define BP_TREE_GROUPS 16
#define BP_TREE_LAST_GROUP (BP_TREE_GROUPS - 1)
#define BP_TREE_HIGH_GROUPS 3
#define BP_TREE_CHILD_COUNT 4
#define BP_TREE_ADDED_CHILD_COUNT (BP_TREE_CHILD_COUNT - 1)
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
#define BP_TREE_GROUP6_INDEX 6
#define BP_TREE_GROUP11_INDEX 11
#define BP_LOSSLESS_TREE_GROUP1_INDEX 0
#define BP_LOSSLESS_TREE_GROUP6_INDEX 5
#define BP_LOSSLESS_TREE_GROUP11_INDEX 10
#define BP_TREE_HIGH_GROUP0_INDEX 2
#define BP_TREE_HIGH_GROUP1_INDEX 7
#define BP_TREE_HIGH_GROUP2_INDEX 12
#define BP_TREE_HIGH_GROUP0_CHILD0_INDEX 3
#define BP_TREE_HIGH_GROUP1_CHILD0_INDEX 8
#define BP_TREE_HIGH_GROUP2_CHILD0_INDEX 13
#define BP_LOSSLESS_TREE_HIGH_GROUP0_INDEX 1
#define BP_LOSSLESS_TREE_HIGH_GROUP1_INDEX 6
#define BP_LOSSLESS_TREE_HIGH_GROUP2_INDEX 11
#define BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX 2
#define BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX 7
#define BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX 12
#define BP_TREE_HIGH_GROUP0_SLOT 0
#define BP_TREE_HIGH_GROUP1_SLOT 1
#define BP_TREE_HIGH_GROUP2_SLOT 2
#define BP_BYTE_MASK 0xff
#define BP_U16_MASK 0xffff
#define BP_SIGN_BIT 0x80
#define BP_NEGATIVE_COEFF_SIGN 0xffff
#define BP_POSITIVE_COEFF_SIGN 1
#define BP_ABS_COEFF(value, sign) (((sign) ^ (value)) - (sign))
#define BP_TREE_EMPTY_ENTRY 0
#define BP_TREE_KIND_MASK 0x300
/* Write-side tree nodes pack kind, coefficient/group index, and bit depth. */
#define BP_TREE_INDEX_SHIFT 10
#define BP_TREE_GROUP_SHIFT 12
#define BP_TREE_HIGH_GROUP_SHIFT 14
#define BP_TREE_BASE_MASK 0xfc00
#define BP_TREE_INDEX_STRIDE 0x400
#define BP_COEFF1_INDEX 1
#define BP_COEFF2_INDEX 2
#define BP_COEFF3_INDEX 3
#define BP_GROUP1_NODE_BASE (1 << BP_TREE_GROUP_SHIFT)
#define BP_GROUP6_NODE_BASE (6 << BP_TREE_GROUP_SHIFT)
#define BP_GROUP11_NODE_BASE (11 << BP_TREE_GROUP_SHIFT)
#define BP_COEFF1_LEAF_BASE (7 << 8)
#define BP_COEFF2_LEAF_BASE (11 << 8)
#define BP_COEFF3_LEAF_BASE (15 << 8)
#define BP_TREE_BRANCH_ENTRY(level, index) ((u16)(level) + (index) * BP_TREE_INDEX_STRIDE + BP_TREE_BRANCH_NODE)
#define BP_TREE_CHILD_BRANCH_ENTRY(level, base, child_base) BP_TREE_BRANCH_ENTRY((level), (base) + (child_base))
#define BP_TREE_COEFF_ENTRY(level, index) ((u16)(level) | (index) * BP_TREE_INDEX_STRIDE + BP_TREE_COEFF_NODE)
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
#define BP_READ_TREE_GROUP_FROM_INDEX(index) BP_READ_TREE_NODE((index) + BP_READ_TREE_CHILD_COUNT, BP_READ_TREE_GROUP_NODE)
#define BP_LOSSLESS_ROOT_NODES 6
#define BP_LOSSY_ROOT_NODES 4
#define BP_ROOT_GROUP1_SLOT 0
#define BP_ROOT_GROUP6_SLOT 1
#define BP_ROOT_GROUP11_SLOT 2
#define BP_ROOT_LOSSLESS_COEFF1_SLOT 3
#define BP_ROOT_LOSSLESS_COEFF2_SLOT 4
#define BP_ROOT_LOSSLESS_COEFF3_SLOT 5
#define BP_ROOT_LOSSY_DC_SLOT 3
#define BP_LOSSLESS_LEVEL_BITS 4
#define BP_LOSSLESS_LEVEL_MASK 0xf
#define BP_LOSSY_LEVEL_BITS 3
#define BP_LOSSY_LEVEL_MASK 7
#define BP_ZIGZAG_COEFF(vals, index) ((vals)[zigzag[index]])

typedef enum BPWriteTreeKind
{
    BP_TREE_HIGH_NODE = 0,
    BP_TREE_GROUP_NODE = 0x100,
    BP_TREE_AFTER_GROUP_NODE = BP_TREE_GROUP_NODE + 1,
    BP_TREE_BRANCH_NODE = 0x200,
    BP_TREE_COEFF_NODE = 0x300
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

#define BP_COEFF_PAIR_AT(values, index) (((BPCOEFFPAIR PTR4*)(values))[(index) / 2])
#define BP_SCATTER_LOSSLESS_PAIR(out, out_index, coeffs, coeff_index) \
    (BP_COEFF_PAIR_AT((out), (out_index)) = (coeffs).pairs[(coeff_index) / 2])

typedef union BPLOSSYBLOCK
{
    s8 bytes[BP_BLOCK_COEFFS];
    s16 words[BP_LOSSY_OUTPUT_COEFFS];
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
        BP_STREAM_BITLEN(bits) = _bitcount;                                                                            \
        BP_STREAM_BITS(bits) = _bitbuf;                                                                                \
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
    u16 kind;
    s32 sign;
    u32 bits;
    u32 maxbits;
    s32 i;
    s32 group;
    s32 len;
    s32 count;
    u8 PTR4* group_end_ptr;
    u8 PTR4* child_len_ptr;
    s32 coeff;
    u16 PTR4* cur;
    u16 PTR4* restart;
    u16 PTR4* end;
    u16 PTR4* roots;
    s32 have_bits;
    s32 total;
    BPLOSSLESSWRITETREE tree;
    u8 lens[BP_BLOCK_COEFFS];
    u8 groups[BP_TREE_GROUPS];
    u8 hi_groups[BP_TREE_HIGH_GROUPS];

    /* Lossless bitplanes code AC coefficient magnitudes by zigzag bit depth. */
    count = BP_AC_COEFFS;
    maxbits = 0;
    i = BP_FIRST_AC_COEFF;
    do {
        coeff = BP_ZIGZAG_COEFF(vals, i);
        sign = coeff >> BP_S32_SIGN_SHIFT;
        bits = getbitlevelvar(BP_ABS_COEFF(coeff, sign) & BP_U16_MASK) & BP_BYTE_MASK;
        if (bits > maxbits) {
            maxbits = bits;
        }
        lens[i] = (u8)bits;
        i++;
        count--;
    } while (count != 0);
    have_bits = maxbits;

    /* Each four-coefficient subtree inherits the deepest child bit depth. */
    group = BP_FIRST_LOSSLESS_TREE_GROUP_INDEX;
    count = BP_TREE_LAST_GROUP;
    group_end_ptr = lens + BP_FIRST_LOSSLESS_TREE_GROUP_END_INDEX;
    child_len_ptr = lens + BP_FIRST_LOSSLESS_TREE_GROUP_INDEX;
    do {
        bits = *child_len_ptr;
        if (*child_len_ptr < child_len_ptr[1]) {
            bits = child_len_ptr[1];
        }
        if (bits < group_end_ptr[-1]) {
            bits = group_end_ptr[-1];
        }
        if (bits < *group_end_ptr) {
            bits = *group_end_ptr;
        }
        groups[BP_LOSSLESS_TREE_GROUP_INDEX(group)] = (u8)bits;
        group_end_ptr = BP_NEXT_TREE_GROUP(group_end_ptr);
        child_len_ptr = BP_NEXT_TREE_GROUP(child_len_ptr);
        group += BP_TREE_CHILD_COUNT;
        count--;
    } while (count != 0);

    bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP0_INDEX];
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX];
    }
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP0_SLOT] = (u8)bits;
    bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP1_INDEX];
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX];
    }
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP1_SLOT] = (u8)bits;
    bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP2_INDEX];
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX];
    }
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (bits < groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        bits = groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP2_SLOT] = (u8)bits;

    len = BP_LOSSLESS_LEVEL_BITS;
    roots = tree.roots;
    bits = hi_groups[BP_TREE_HIGH_GROUP0_SLOT];
    if (bits > groups[BP_LOSSLESS_TREE_GROUP1_INDEX]) {
        entry = bits | BP_GROUP1_NODE_BASE;
    } else {
        entry = groups[BP_LOSSLESS_TREE_GROUP1_INDEX] | BP_GROUP1_NODE_BASE;
    }
    roots[BP_ROOT_GROUP1_SLOT] = entry;
    bits = hi_groups[BP_TREE_HIGH_GROUP1_SLOT];
    if (bits > groups[BP_LOSSLESS_TREE_GROUP6_INDEX]) {
        entry = bits | BP_GROUP6_NODE_BASE;
    } else {
        entry = groups[BP_LOSSLESS_TREE_GROUP6_INDEX] | BP_GROUP6_NODE_BASE;
    }
    roots[BP_ROOT_GROUP6_SLOT] = entry;
    bits = hi_groups[BP_TREE_HIGH_GROUP2_SLOT];
    if (bits > groups[BP_LOSSLESS_TREE_GROUP11_INDEX]) {
        entry = bits | BP_GROUP11_NODE_BASE;
    } else {
        entry = groups[BP_LOSSLESS_TREE_GROUP11_INDEX] | BP_GROUP11_NODE_BASE;
    }
    roots[BP_ROOT_GROUP11_SLOT] = entry;
    roots[BP_ROOT_LOSSLESS_COEFF1_SLOT] = lens[BP_COEFF1_INDEX] + BP_COEFF1_LEAF_BASE;
    roots[BP_ROOT_LOSSLESS_COEFF2_SLOT] = lens[BP_COEFF2_INDEX] + BP_COEFF2_LEAF_BASE;
    roots[BP_ROOT_LOSSLESS_COEFF3_SLOT] = lens[BP_COEFF3_INDEX] + BP_COEFF3_LEAF_BASE;
    cur = roots;
    end = tree.nodes;

    /* Expand pending group/branch/coeff nodes one bitplane level at a time. */
    for (; 1 < maxbits; maxbits = (maxbits - 1) & BP_BYTE_MASK) {
        total = len;
        restart = cur;
        if (cur < end) {
            do {
                entry = *cur;
                len = total;
                if ((entry == BP_TREE_EMPTY_ENTRY) ||
                    (len = total + BP_TREE_NODE_PRESENT_BITS, (entry & BP_BYTE_MASK) != maxbits)) {
                    cur++;
                } else {
                    kind = entry & BP_TREE_KIND_MASK;
                    if (kind == BP_TREE_GROUP_NODE) {
                        kind = entry >> BP_TREE_INDEX_SHIFT;
                        bits = (u32)(entry >> BP_TREE_GROUP_SHIFT);
                        *cur = (u16)groups[bits] + (entry & BP_TREE_BASE_MASK) + BP_TREE_BRANCH_NODE;
                        *end = BP_TREE_CHILD_BRANCH_ENTRY(groups[bits + BP_TREE_CHILD1_INDEX], kind, BP_TREE_CHILD1_BASE);
                        end[1] = BP_TREE_CHILD_BRANCH_ENTRY(groups[bits + BP_TREE_CHILD2_INDEX], kind, BP_TREE_CHILD2_BASE);
                        end[2] = BP_TREE_CHILD_BRANCH_ENTRY(groups[bits + BP_TREE_CHILD3_INDEX], kind, BP_TREE_CHILD3_BASE);
                        end += BP_TREE_ADDED_CHILD_COUNT;
                    } else if (kind == BP_TREE_HIGH_NODE) {
                        *cur = (u16)hi_groups[entry >> BP_TREE_HIGH_GROUP_SHIFT] + ((entry >> BP_TREE_INDEX_SHIFT) + BP_TREE_CHILD1_BASE) * BP_TREE_INDEX_STRIDE + BP_TREE_GROUP_NODE;
handle_children:
                        kind = entry >> BP_TREE_INDEX_SHIFT;
                        len = total + BP_TREE_NODE_SIGNAL_BITS;
                        if (lens[kind] == maxbits) {
                            len += maxbits;
                        } else {
                            *--restart = BP_TREE_BASE_COEFF_ENTRY(lens[kind], entry & BP_TREE_BASE_MASK);
                        }
                        if (lens[kind + BP_TREE_CHILD1_INDEX] == maxbits) {
                            len += maxbits;
                        } else {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[kind + BP_TREE_CHILD1_INDEX], kind + BP_TREE_CHILD1_INDEX);
                        }
                        if (lens[kind + BP_TREE_CHILD2_INDEX] == maxbits) {
                            len += maxbits;
                        } else {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[kind + BP_TREE_CHILD2_INDEX], kind + BP_TREE_CHILD2_INDEX);
                        }
                        if (lens[kind + BP_TREE_CHILD3_INDEX] == maxbits) {
                            len += maxbits;
                        } else {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[kind + BP_TREE_CHILD3_INDEX], kind + BP_TREE_CHILD3_INDEX);
                        }
                    } else if (kind == BP_TREE_BRANCH_NODE) {
                        *cur = BP_TREE_EMPTY_ENTRY;
                        cur++;
                        goto handle_children;
                    } else {
                        if (kind == BP_TREE_COEFF_NODE) {
                            *cur = BP_TREE_EMPTY_ENTRY;
                            len += maxbits;
                        }
                        cur++;
                    }
                }
                total = len;
            } while (cur < end);
        }
        cur = restart;
    }

    if (have_bits && (total = len, restart = cur, cur < end)) {
        do {
            entry = *cur;
            len = total;
            if ((entry == BP_TREE_EMPTY_ENTRY) ||
                (len = total + BP_TREE_NODE_PRESENT_BITS, (entry & BP_BYTE_MASK) != 1)) {
                cur++;
            } else {
                kind = entry & BP_TREE_KIND_MASK;
                if (kind == BP_TREE_GROUP_NODE) {
                    kind = entry >> BP_TREE_INDEX_SHIFT;
                    maxbits = (u32)(entry >> BP_TREE_GROUP_SHIFT);
                    *cur = (u16)groups[maxbits] + (entry & BP_TREE_BASE_MASK) + BP_TREE_BRANCH_NODE;
                    *end = BP_TREE_CHILD_BRANCH_ENTRY(groups[maxbits + BP_TREE_CHILD1_INDEX], kind, BP_TREE_CHILD1_BASE);
                    end[1] = BP_TREE_CHILD_BRANCH_ENTRY(groups[maxbits + BP_TREE_CHILD2_INDEX], kind, BP_TREE_CHILD2_BASE);
                    end[2] = BP_TREE_CHILD_BRANCH_ENTRY(groups[maxbits + BP_TREE_CHILD3_INDEX], kind, BP_TREE_CHILD3_BASE);
                    end += BP_TREE_ADDED_CHILD_COUNT;
                } else if (kind == BP_TREE_HIGH_NODE) {
                    *cur = (u16)hi_groups[entry >> BP_TREE_HIGH_GROUP_SHIFT] + ((entry >> BP_TREE_INDEX_SHIFT) + BP_TREE_CHILD1_BASE) * BP_TREE_INDEX_STRIDE + BP_TREE_GROUP_NODE;
handle_final_children:
                    kind = entry >> BP_TREE_INDEX_SHIFT;
                    len = total + BP_TREE_NODE_SIGNAL_BITS;
                    if (lens[kind] == 1) {
                        len = total + BP_TREE_NODE_SIGNAL_BITS + 1;
                    } else {
                        *--restart = BP_TREE_BASE_COEFF_ENTRY(lens[kind], entry & BP_TREE_BASE_MASK);
                    }
                    if (lens[kind + BP_TREE_CHILD1_INDEX] == 1) {
                        len++;
                    } else {
                        *--restart = BP_TREE_COEFF_ENTRY(lens[kind + BP_TREE_CHILD1_INDEX], kind + BP_TREE_CHILD1_INDEX);
                    }
                    if (lens[kind + BP_TREE_CHILD2_INDEX] == 1) {
                        len++;
                    } else {
                        *--restart = BP_TREE_COEFF_ENTRY(lens[kind + BP_TREE_CHILD2_INDEX], kind + BP_TREE_CHILD2_INDEX);
                    }
                    if (lens[kind + BP_TREE_CHILD3_INDEX] == 1) {
                        len++;
                    } else {
                        *--restart = BP_TREE_COEFF_ENTRY(lens[kind + BP_TREE_CHILD3_INDEX], kind + BP_TREE_CHILD3_INDEX);
                    }
                } else if (kind == BP_TREE_BRANCH_NODE) {
                    *cur = BP_TREE_EMPTY_ENTRY;
                    cur++;
                    goto handle_final_children;
                } else {
                    if (kind == BP_TREE_COEFF_NODE) {
                        *cur = BP_TREE_EMPTY_ENTRY;
                        len = total + BP_FINAL_COEFF_SIGNAL_BITS;
                    }
                    cur++;
                }
            }
            total = len;
        } while (cur < end);
    }

    return len;
}

void WriteBPLossless(BPBITSTREAM PTR4* bits, s16 PTR4* vals)
{
    u16 entry;
    s32 coeff;
    s32 sign;
    u16 kind;
    s32 i;
    s32 count;
    u8 PTR4* group_end_ptr;
    u8 PTR4* child_len_ptr;
    u32 maxbits;
    u32 lenbits;
    u32 bit_count;
    BPBITSTYPE bit_buf;
    u16 PTR4* cur;
    s16 PTR4* ordered_cur;
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
    ordered_cur = ordered;
    do {
        *ordered_cur = BP_ZIGZAG_COEFF(vals, i);
        i++;
        ordered_cur++;
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
    cur = absvals;
    do {
        cur++;
        lenbits = getbitlevelvar((u32)*cur) & BP_BYTE_MASK;
        if (lenbits > maxbits) {
            maxbits = lenbits;
        }
        lens[i] = (u8)lenbits;
        i++;
        count--;
    } while (count != 0);

    i = BP_FIRST_LOSSLESS_TREE_GROUP_INDEX;
    count = BP_TREE_LAST_GROUP;
    group_end_ptr = lens + BP_FIRST_LOSSLESS_TREE_GROUP_END_INDEX;
    child_len_ptr = lens + BP_FIRST_LOSSLESS_TREE_GROUP_INDEX;
    do {
        lenbits = *child_len_ptr;
        if (*child_len_ptr < child_len_ptr[1]) {
            lenbits = child_len_ptr[1];
        }
        if (lenbits < group_end_ptr[-1]) {
            lenbits = group_end_ptr[-1];
        }
        if (lenbits < *group_end_ptr) {
            lenbits = *group_end_ptr;
        }
        groups[BP_LOSSLESS_TREE_GROUP_INDEX(i)] = (u8)lenbits;
        group_end_ptr = BP_NEXT_TREE_GROUP(group_end_ptr);
        child_len_ptr = BP_NEXT_TREE_GROUP(child_len_ptr);
        i += BP_TREE_CHILD_COUNT;
        count--;
    } while (count != 0);

    lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP0_INDEX];
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX];
    }
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP0_SLOT] = (u8)lenbits;
    lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP1_INDEX];
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX];
    }
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP1_SLOT] = (u8)lenbits;
    lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP2_INDEX];
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX];
    }
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (lenbits < groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        lenbits = groups[BP_LOSSLESS_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP2_SLOT] = (u8)lenbits;

    bit_count = BP_STREAM_BITLEN(bits) + BP_LOSSLESS_LEVEL_BITS;
    lenbits = maxbits & VarBitsLens[BP_LOSSLESS_LEVEL_BITS];
    bit_buf = BP_STREAM_BITS(bits) | (lenbits << BP_STREAM_BITLEN(bits));
    BP_STREAM_BITLEN(bits) = bit_count;
    BP_STREAM_BITS(bits) = bit_buf;
    if (bit_count >= BP_BITS_PER_WORD) {
        *BP_STREAM_CUR(bits) = bit_buf;
        bit_buf = BP_STREAM_BITLEN(bits) - BP_BITS_PER_WORD;
        BP_STREAM_CUR(bits) = BP_STREAM_CUR(bits) + 1;
        BP_STREAM_BITLEN(bits) = bit_buf;
        BP_STREAM_BITS(bits) = 0;
        if (bit_buf != 0) {
            BP_STREAM_BITS(bits) = lenbits >> (BP_LOSSLESS_LEVEL_BITS - bit_buf);
        }
    }

    roots = tree.roots;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP0_SLOT];
    if (lenbits > groups[BP_LOSSLESS_TREE_GROUP1_INDEX]) {
        entry = lenbits | BP_GROUP1_NODE_BASE;
    } else {
        entry = groups[BP_LOSSLESS_TREE_GROUP1_INDEX] | BP_GROUP1_NODE_BASE;
    }
    roots[BP_ROOT_GROUP1_SLOT] = entry;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP1_SLOT];
    if (lenbits > groups[BP_LOSSLESS_TREE_GROUP6_INDEX]) {
        entry = lenbits | BP_GROUP6_NODE_BASE;
    } else {
        entry = groups[BP_LOSSLESS_TREE_GROUP6_INDEX] | BP_GROUP6_NODE_BASE;
    }
    roots[BP_ROOT_GROUP6_SLOT] = entry;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP2_SLOT];
    if (lenbits > groups[BP_LOSSLESS_TREE_GROUP11_INDEX]) {
        entry = lenbits | BP_GROUP11_NODE_BASE;
    } else {
        entry = groups[BP_LOSSLESS_TREE_GROUP11_INDEX] | BP_GROUP11_NODE_BASE;
    }
    roots[BP_ROOT_GROUP11_SLOT] = entry;
    roots[BP_ROOT_LOSSLESS_COEFF1_SLOT] = lens[BP_COEFF1_INDEX] + BP_COEFF1_LEAF_BASE;
    roots[BP_ROOT_LOSSLESS_COEFF2_SLOT] = lens[BP_COEFF2_INDEX] + BP_COEFF2_LEAF_BASE;
    roots[BP_ROOT_LOSSLESS_COEFF3_SLOT] = lens[BP_COEFF3_INDEX] + BP_COEFF3_LEAF_BASE;

    cur = roots;
    end = tree.nodes;
    do {
        if (maxbits == 0) {
            return;
        }
        lenbits = maxbits - 1;
        restart = cur;
        /* Active children at lower bit depths are pushed before the current cursor. */
        if (cur < end) {
            do {
                entry = *cur;
                if (entry == BP_TREE_EMPTY_ENTRY) {
next_lossless_node:
                    cur++;
                } else {
                    sign = (entry & BP_BYTE_MASK) != maxbits;
                    PUT_BP_BIT(bits, !sign);
                    if (sign) {
                        goto next_lossless_node;
                    }
                    kind = entry & BP_TREE_KIND_MASK;
                    if (kind == BP_TREE_GROUP_NODE) {
                        kind = entry >> BP_TREE_INDEX_SHIFT;
                        count = (u32)(entry >> BP_TREE_GROUP_SHIFT);
                        *cur = (u16)groups[count] + (entry & BP_TREE_BASE_MASK) + BP_TREE_BRANCH_NODE;
                        *end = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD1_INDEX], kind, BP_TREE_CHILD1_BASE);
                        end[1] = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD2_INDEX], kind, BP_TREE_CHILD2_BASE);
                        end[2] = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD3_INDEX], kind, BP_TREE_CHILD3_BASE);
                        end += BP_TREE_ADDED_CHILD_COUNT;
                    } else if (kind == BP_TREE_HIGH_NODE) {
                        *cur = (u16)hi_groups[entry >> BP_TREE_HIGH_GROUP_SHIFT] + ((entry >> BP_TREE_INDEX_SHIFT) + BP_TREE_CHILD1_BASE) * BP_TREE_INDEX_STRIDE + BP_TREE_GROUP_NODE;
handle_lossless_children:
                        kind = entry >> BP_TREE_INDEX_SHIFT;
                        PUT_BP_BIT(bits, lens[kind] != maxbits);
                        if (lens[kind] == maxbits) {
                            PUT_BP_BITS(bits, absvals[kind], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[kind] < 0);
                        } else {
                            *--restart = BP_TREE_BASE_COEFF_ENTRY(lens[kind], entry & BP_TREE_BASE_MASK);
                        }

                        i = kind + BP_TREE_CHILD1_INDEX;
                        PUT_BP_BIT(bits, lens[i] != maxbits);
                        if (lens[i] == maxbits) {
                            PUT_BP_BITS(bits, absvals[i], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[i] < 0);
                        } else {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[i], i);
                        }

                        i = kind + BP_TREE_CHILD2_INDEX;
                        PUT_BP_BIT(bits, lens[i] != maxbits);
                        if (lens[i] == maxbits) {
                            PUT_BP_BITS(bits, absvals[i], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[i] < 0);
                        } else {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[i], i);
                        }

                        i = kind + BP_TREE_CHILD3_INDEX;
                        PUT_BP_BIT(bits, lens[i] != maxbits);
                        if (lens[i] == maxbits) {
                            PUT_BP_BITS(bits, absvals[i], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[i] < 0);
                        } else {
                            *--restart = BP_TREE_COEFF_ENTRY(lens[i], i);
                        }
                    } else {
                        if (kind == BP_TREE_BRANCH_NODE) {
                            *cur = BP_TREE_EMPTY_ENTRY;
                            cur++;
                            goto handle_lossless_children;
                        }
                        if (kind == BP_TREE_COEFF_NODE) {
                            kind = entry >> BP_TREE_INDEX_SHIFT;
                            PUT_BP_BITS(bits, absvals[kind], lenbits, VarBitsLens[lenbits]);
                            PUT_BP_BIT(bits, ordered[kind] < 0);
                            *cur = BP_TREE_EMPTY_ENTRY;
                        }
                        goto next_lossless_node;
                    }
                }
            } while (cur < end);
        }
        maxbits = lenbits & BP_BYTE_MASK;
        cur = restart;
    } while (1);
}

void ReadBPLossless(s16 PTR4* out, BPBITSTREAM PTR4* bits)
{
    u32 code;
    s32 shift;
    u8 level;
    u8 maxlevel;
    s16 highbit;
    s16 coeff_value;
    u16 bit_mask;
    u8 PTR4* node_ptr;
    u8 PTR4* next_node_ptr;
    u8 PTR4* tree_end_ptr;
    u8 node;
    u8 kind;
    u8 base;
    BPLOSSLESSREADTREE tree;
    BPLOSSLESSCOEFFS coeffs;
    BPBITSTREAM bitcopy;

    bitcopy = *bits;
#define words bitcopy.cur
#define bitbuf bitcopy.bits
#define bitcount bitcopy.bitlen
    coeffs.values[BP_COEFF1_INDEX] = 0;
    memset(coeffs.values + BP_COEFF2_INDEX, 0, sizeof(coeffs) - BP_COEFF2_INDEX * sizeof(coeffs.values[0]));

    /* The stream starts with the maximum active lossless bitplane level. */
    if (bitcount >= BP_LOSSLESS_LEVEL_BITS) {
        code = bitbuf & BP_BYTE_MASK;
        bitbuf >>= BP_LOSSLESS_LEVEL_BITS;
        bitcount -= BP_LOSSLESS_LEVEL_BITS;
    } else {
        shift = BP_LOSSLESS_LEVEL_BITS - bitcount;
        code = bitbuf & BP_BYTE_MASK;
        bitbuf = *words;
        words++;
        code |= bitbuf << bitcount;
        bitbuf >>= shift;
        bitcount = bitcount + BP_BITS_PER_WORD - BP_LOSSLESS_LEVEL_BITS;
    }

    maxlevel = code & BP_LOSSLESS_LEVEL_MASK;
    highbit = (u16)(1 << (maxlevel - 1));
    /* Root nodes mirror WriteBPLossless: three grouped roots plus coeffs 1..3. */
    tree.roots[BP_ROOT_GROUP1_SLOT] = BP_READ_TREE_GROUP1_ROOT;
    tree.roots[BP_ROOT_GROUP6_SLOT] = BP_READ_TREE_GROUP6_ROOT;
    tree.roots[BP_ROOT_GROUP11_SLOT] = BP_READ_TREE_GROUP11_ROOT;
    tree.roots[BP_ROOT_LOSSLESS_COEFF1_SLOT] = BP_READ_TREE_COEFF1_ROOT;
    tree.roots[BP_ROOT_LOSSLESS_COEFF2_SLOT] = BP_READ_TREE_COEFF2_ROOT;
    tree.roots[BP_ROOT_LOSSLESS_COEFF3_SLOT] = BP_READ_TREE_COEFF3_ROOT;

    node_ptr = tree.roots;
    tree_end_ptr = tree.nodes;

    /* Non-final planes read lower magnitude bits plus a sign for new coeffs. */
    while (1 < maxlevel) {
        level = (maxlevel - 1) & BP_BYTE_MASK;
        next_node_ptr = node_ptr;
        highbit = (s16)highbit >> 1;
        if (node_ptr < tree_end_ptr) {
            bit_mask = (u16)GetBitsLen(level);
            do {
                node = *node_ptr;
                if (node == BP_READ_TREE_EMPTY_ENTRY) {
next_lossless_read_node:
                    node_ptr++;
                } else {
                    if (bitcount == 0) {
                        code = *words;
                        bitcount = BP_WORD_TOP_BIT;
                        words++;
                        bitbuf = code >> 1;
                        if ((code & 1) == 0) {
                            goto next_lossless_read_node;
                        }
                    } else {
                        bitcount = bitcount - 1;
                        code = bitbuf & 1;
                        bitbuf >>= 1;
                        if (code == 0) {
                            goto next_lossless_read_node;
                        }
                    }

                    kind = node & BP_READ_TREE_KIND_MASK;
                    if (kind == BP_READ_TREE_GROUP_NODE) {
                        kind = BP_READ_TREE_INDEX(node);
                        *node_ptr = BP_READ_TREE_BRANCH_FROM_NODE(node);
                        *tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(kind, BP_READ_TREE_CHILD1_BASE);
                        tree_end_ptr[1] = BP_READ_TREE_CHILD_BRANCH(kind, BP_READ_TREE_CHILD2_BASE);
                        tree_end_ptr[2] = BP_READ_TREE_CHILD_BRANCH(kind, BP_READ_TREE_CHILD3_BASE);
                        tree_end_ptr += BP_TREE_ADDED_CHILD_COUNT;
                    } else {
                        if (kind < BP_READ_TREE_BRANCH_NODE) {
                            if ((node & BP_READ_TREE_KIND_MASK) != BP_READ_TREE_HIGH_NODE) {
                                goto next_lossless_read_node;
                            }
                            *node_ptr = BP_READ_TREE_GROUP_FROM_INDEX(BP_READ_TREE_INDEX(node));
                        } else {
                            if (kind != BP_READ_TREE_BRANCH_NODE) {
                                if (kind == BP_READ_TREE_COEFF_NODE) {
                                    coeff_value = bitbuf & bit_mask;
                                    if (bitcount < level) {
                                        code = *words++;
                                        coeff_value |= code << bitcount;
                                        bitbuf = code >> (level - bitcount);
                                        bitcount = bitcount + BP_BITS_PER_WORD - level;
                                    } else {
                                        bitbuf >>= level;
                                        bitcount = bitcount - level;
                                    }
                                    coeff_value = (coeff_value & bit_mask) | highbit;
                                    if (bitcount == 0) {
                                        code = *words;
                                        bitcount = BP_WORD_TOP_BIT;
                                        words++;
                                        bitbuf = code >> 1;
                                    } else {
                                        bitcount = bitcount - 1;
                                        code = bitbuf & 1;
                                        bitbuf >>= 1;
                                    }
                                    if ((code & 1) != 0) {
                                        coeff_value = -coeff_value;
                                    }
                                    coeffs.values[BP_READ_TREE_INDEX(node)] = (u16)coeff_value;
                                    *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                                }
                                goto next_lossless_read_node;
                            }
                            *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                            node_ptr++;
                        }

                        base = BP_READ_TREE_INDEX(node);
#define READ_LOSSLESS_CHILD(slot, label)                                                                               \
                        do {                                                                                           \
                            if (bitcount == 0) {                                                                       \
                                code = *words;                                                                         \
                                bitcount = BP_WORD_TOP_BIT;                                                            \
                                bitbuf = code >> 1;                                                                    \
                                words++;                                                                               \
                                if ((code & 1) != 0) {                                                                 \
                                    next_node_ptr--;                                                                   \
                                    *next_node_ptr = BP_READ_TREE_COEFF(slot);                                          \
                                    goto label;                                                                        \
                                }                                                                                      \
                            } else {                                                                                   \
                                bitcount = bitcount - 1;                                                               \
                                code = bitbuf & 1;                                                                     \
                                bitbuf >>= 1;                                                                          \
                                if (code != 0) {                                                                       \
                                    next_node_ptr--;                                                                   \
                                    *next_node_ptr = BP_READ_TREE_COEFF(slot);                                          \
                                    goto label;                                                                        \
                                }                                                                                      \
                            }                                                                                          \
                            coeff_value = bitbuf & bit_mask;                                                           \
                            if (bitcount < level) {                                                                    \
                                code = *words++;                                                                       \
                                coeff_value |= code << bitcount;                                                       \
                                bitbuf = code >> (level - bitcount);                                                   \
                                bitcount = bitcount + BP_BITS_PER_WORD - level;                                        \
                            } else {                                                                                   \
                                bitbuf >>= level;                                                                      \
                                bitcount = bitcount - level;                                                           \
                            }                                                                                          \
                            coeff_value = (coeff_value & bit_mask) | highbit;                                          \
                            if (bitcount == 0) {                                                                       \
                                code = *words;                                                                         \
                                bitcount = BP_WORD_TOP_BIT;                                                            \
                                words++;                                                                               \
                                bitbuf = code >> 1;                                                                    \
                            } else {                                                                                   \
                                bitcount = bitcount - 1;                                                               \
                                code = bitbuf & 1;                                                                     \
                                bitbuf >>= 1;                                                                          \
                            }                                                                                          \
                            if ((code & 1) != 0) {                                                                     \
                                coeff_value = -coeff_value;                                                            \
                            }                                                                                          \
                            coeffs.values[slot] = (u16)coeff_value;                                                    \
                        } while (0)
                        READ_LOSSLESS_CHILD(base, after_lossless_child0);
after_lossless_child0:
                        READ_LOSSLESS_CHILD(base + BP_TREE_CHILD1_INDEX, after_lossless_child1);
after_lossless_child1:
                        READ_LOSSLESS_CHILD(base + BP_TREE_CHILD2_INDEX, after_lossless_child2);
after_lossless_child2:
                        READ_LOSSLESS_CHILD(base + BP_TREE_CHILD3_INDEX, after_lossless_child3);
after_lossless_child3:
#undef READ_LOSSLESS_CHILD
                        ;
                    }
                }
            } while (node_ptr < tree_end_ptr);
        }
        maxlevel = level;
        node_ptr = next_node_ptr;
    }

    /* Level one coeffs need only a sign bit; their magnitude is implicit. */
    if (maxlevel && node_ptr < tree_end_ptr) {
        next_node_ptr = node_ptr;
        do {
            node = *node_ptr;
            if (node == BP_READ_TREE_EMPTY_ENTRY) {
next_lossless_final_node:
                node_ptr++;
            } else {
                if (bitcount == 0) {
                    code = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words++;
                    bitbuf = code >> 1;
                    if ((code & 1) == 0) {
                        goto next_lossless_final_node;
                    }
                } else {
                    bitcount = bitcount - 1;
                    code = bitbuf & 1;
                    bitbuf >>= 1;
                    if (code == 0) {
                        goto next_lossless_final_node;
                    }
                }
                kind = node & BP_READ_TREE_KIND_MASK;
                if (kind == BP_READ_TREE_GROUP_NODE) {
                    kind = BP_READ_TREE_INDEX(node);
                    *node_ptr = BP_READ_TREE_BRANCH_FROM_NODE(node);
                    *tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(kind, BP_READ_TREE_CHILD1_BASE);
                    tree_end_ptr[1] = BP_READ_TREE_CHILD_BRANCH(kind, BP_READ_TREE_CHILD2_BASE);
                    tree_end_ptr[2] = BP_READ_TREE_CHILD_BRANCH(kind, BP_READ_TREE_CHILD3_BASE);
                    tree_end_ptr += BP_TREE_ADDED_CHILD_COUNT;
                } else {
                    if (kind < BP_READ_TREE_BRANCH_NODE) {
                        if ((node & BP_READ_TREE_KIND_MASK) != BP_READ_TREE_HIGH_NODE) {
                            goto next_lossless_final_node;
                        }
                        *node_ptr = BP_READ_TREE_GROUP_FROM_INDEX(BP_READ_TREE_INDEX(node));
                    } else {
                        if (kind != BP_READ_TREE_BRANCH_NODE) {
                            if (kind == BP_READ_TREE_COEFF_NODE) {
                                if (bitcount == 0) {
                                    code = *words;
                                    bitcount = BP_WORD_TOP_BIT;
                                    words++;
                                    bitbuf = code >> 1;
                                } else {
                                    bitcount = bitcount - 1;
                                    code = bitbuf & 1;
                                    bitbuf >>= 1;
                                }
                                coeffs.values[BP_READ_TREE_INDEX(node)] =
                                    (code & 1) ? BP_NEGATIVE_COEFF_SIGN : BP_POSITIVE_COEFF_SIGN;
                                *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                            }
                            goto next_lossless_final_node;
                        }
                        *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                        node_ptr++;
                    }

                    base = BP_READ_TREE_INDEX(node);
#define READ_LOSSLESS_FINAL_CHILD(slot, label)                                                                          \
                    do {                                                                                                \
                        if (bitcount == 0) {                                                                            \
                            code = *words;                                                                              \
                            bitcount = BP_WORD_TOP_BIT;                                                                 \
                            bitbuf = code >> 1;                                                                         \
                            words++;                                                                                    \
                            if ((code & 1) != 0) {                                                                      \
                                next_node_ptr--;                                                                        \
                                *next_node_ptr = BP_READ_TREE_COEFF(slot);                                               \
                                goto label;                                                                             \
                            }                                                                                           \
                        } else {                                                                                        \
                            bitcount = bitcount - 1;                                                                    \
                            code = bitbuf & 1;                                                                          \
                            bitbuf >>= 1;                                                                               \
                            if (code != 0) {                                                                            \
                                next_node_ptr--;                                                                        \
                                *next_node_ptr = BP_READ_TREE_COEFF(slot);                                               \
                                goto label;                                                                             \
                            }                                                                                           \
                        }                                                                                               \
                        if (bitcount == 0) {                                                                            \
                            code = *words;                                                                              \
                            bitcount = BP_WORD_TOP_BIT;                                                                 \
                            words++;                                                                                    \
                            bitbuf = code >> 1;                                                                         \
                        } else {                                                                                        \
                            bitcount = bitcount - 1;                                                                    \
                            code = bitbuf & 1;                                                                          \
                            bitbuf >>= 1;                                                                               \
                        }                                                                                               \
                        coeffs.values[slot] = (code & 1) ? BP_NEGATIVE_COEFF_SIGN : BP_POSITIVE_COEFF_SIGN;                                                  \
                    } while (0)
                    READ_LOSSLESS_FINAL_CHILD(base, after_lossless_final0);
after_lossless_final0:
                    READ_LOSSLESS_FINAL_CHILD(base + BP_TREE_CHILD1_INDEX, after_lossless_final1);
after_lossless_final1:
                    READ_LOSSLESS_FINAL_CHILD(base + BP_TREE_CHILD2_INDEX, after_lossless_final2);
after_lossless_final2:
                    READ_LOSSLESS_FINAL_CHILD(base + BP_TREE_CHILD3_INDEX, after_lossless_final3);
after_lossless_final3:
#undef READ_LOSSLESS_FINAL_CHILD
                    ;
                }
            }
        } while (node_ptr < tree_end_ptr);
    }

    bitcopy.cur = words;
    bitcopy.bits = bitbuf;
    bitcopy.bitlen = bitcount;
    *bits = bitcopy;
#undef words
#undef bitbuf
#undef bitcount
    (void)tree;

    /* Scatter scan-order coefficients back into the 8x8 block. */
    out[BP_COEFF1_INDEX] = coeffs.values[BP_COEFF1_INDEX];
    BP_SCATTER_LOSSLESS_PAIR(out, 2, coeffs, 4);
    BP_SCATTER_LOSSLESS_PAIR(out, 4, coeffs, 8);
    BP_SCATTER_LOSSLESS_PAIR(out, 6, coeffs, 12);
    BP_SCATTER_LOSSLESS_PAIR(out, 8, coeffs, 2);
    BP_SCATTER_LOSSLESS_PAIR(out, 10, coeffs, 6);
    BP_SCATTER_LOSSLESS_PAIR(out, 12, coeffs, 10);
    BP_SCATTER_LOSSLESS_PAIR(out, 14, coeffs, 14);
    BP_SCATTER_LOSSLESS_PAIR(out, 16, coeffs, 24);
    BP_SCATTER_LOSSLESS_PAIR(out, 18, coeffs, 44);
    BP_SCATTER_LOSSLESS_PAIR(out, 20, coeffs, 16);
    BP_SCATTER_LOSSLESS_PAIR(out, 22, coeffs, 20);
    BP_SCATTER_LOSSLESS_PAIR(out, 24, coeffs, 26);
    BP_SCATTER_LOSSLESS_PAIR(out, 26, coeffs, 46);
    BP_SCATTER_LOSSLESS_PAIR(out, 28, coeffs, 18);
    BP_SCATTER_LOSSLESS_PAIR(out, 30, coeffs, 22);
    BP_SCATTER_LOSSLESS_PAIR(out, 32, coeffs, 28);
    BP_SCATTER_LOSSLESS_PAIR(out, 34, coeffs, 32);
    BP_SCATTER_LOSSLESS_PAIR(out, 36, coeffs, 48);
    BP_SCATTER_LOSSLESS_PAIR(out, 38, coeffs, 52);
    BP_SCATTER_LOSSLESS_PAIR(out, 40, coeffs, 30);
    BP_SCATTER_LOSSLESS_PAIR(out, 42, coeffs, 34);
    BP_SCATTER_LOSSLESS_PAIR(out, 44, coeffs, 50);
    BP_SCATTER_LOSSLESS_PAIR(out, 46, coeffs, 54);
    BP_SCATTER_LOSSLESS_PAIR(out, 48, coeffs, 36);
    BP_SCATTER_LOSSLESS_PAIR(out, 50, coeffs, 40);
    BP_SCATTER_LOSSLESS_PAIR(out, 52, coeffs, 56);
    BP_SCATTER_LOSSLESS_PAIR(out, 54, coeffs, 60);
    BP_SCATTER_LOSSLESS_PAIR(out, 56, coeffs, 38);
    BP_SCATTER_LOSSLESS_PAIR(out, 58, coeffs, 42);
    BP_SCATTER_LOSSLESS_PAIR(out, 60, coeffs, 58);
    BP_SCATTER_LOSSLESS_PAIR(out, 62, coeffs, 62);
}

u32 WriteBPLossy(BPBITSTREAM PTR4* bits, char PTR4* vals)
{
    u16 entry;
    s32 coeff;
    s32 sign;
    s32 i;
    s32 count;
    u8 PTR4* group_end_ptr;
    u8 PTR4* child_len_ptr;
    u32 maxbits;
    u32 lenbits;
    u32 bit_count;
    BPBITSTYPE bit_buf;
    u16 PTR4* cur;
    u16 PTR4* insert;
    u16 PTR4* next_node;
    u16 PTR4* roots;
    u16 bit_mask;
    u16 node_entry;
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
        lenbits = getbitlevelvar((u32)absvals[i]) & BP_BYTE_MASK;
        if (lenbits > maxbits) {
            maxbits = lenbits;
        }
        lens[i] = (u8)lenbits;
        i++;
        count--;
    } while (count != 0);

    if (maxbits == 0) {
        return 0;
    }

    /* Group tables use the deepest bit depth of each four-coefficient branch. */
    count = BP_TREE_GROUPS;
    i = 0;
    group_end_ptr = lens + BP_FIRST_LOSSY_TREE_GROUP_END_INDEX;
    child_len_ptr = lens;
    do {
        lenbits = *child_len_ptr;
        if (*child_len_ptr < child_len_ptr[1]) {
            lenbits = child_len_ptr[1];
        }
        if (lenbits < group_end_ptr[-1]) {
            lenbits = group_end_ptr[-1];
        }
        if (lenbits < *group_end_ptr) {
            lenbits = *group_end_ptr;
        }
        groups[BP_TREE_GROUP_INDEX(i)] = (u8)lenbits;
        group_end_ptr = BP_NEXT_TREE_GROUP(group_end_ptr);
        child_len_ptr = BP_NEXT_TREE_GROUP(child_len_ptr);
        i += BP_TREE_CHILD_COUNT;
        count--;
    } while (count != 0);

    lenbits = groups[BP_TREE_HIGH_GROUP0_INDEX];
    if (lenbits < groups[BP_TREE_HIGH_GROUP0_CHILD0_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP0_CHILD0_INDEX];
    }
    if (lenbits < groups[BP_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (lenbits < groups[BP_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP0_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP0_SLOT] = (u8)lenbits;
    lenbits = groups[BP_TREE_HIGH_GROUP1_INDEX];
    if (lenbits < groups[BP_TREE_HIGH_GROUP1_CHILD0_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP1_CHILD0_INDEX];
    }
    if (lenbits < groups[BP_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (lenbits < groups[BP_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP1_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP1_SLOT] = (u8)lenbits;
    lenbits = groups[BP_TREE_HIGH_GROUP2_INDEX];
    if (lenbits < groups[BP_TREE_HIGH_GROUP2_CHILD0_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP2_CHILD0_INDEX];
    }
    if (lenbits < groups[BP_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD1_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD1_INDEX];
    }
    if (lenbits < groups[BP_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD2_INDEX]) {
        lenbits = groups[BP_TREE_HIGH_GROUP2_CHILD0_INDEX + BP_TREE_CHILD2_INDEX];
    }
    hi_groups[BP_TREE_HIGH_GROUP2_SLOT] = (u8)lenbits;

    bit_count = BP_STREAM_BITLEN(bits) + BP_LOSSY_LEVEL_BITS;
    lenbits = (maxbits - 1) & VarBitsLens[BP_LOSSY_LEVEL_BITS];
    bit_buf = BP_STREAM_BITS(bits) | (lenbits << BP_STREAM_BITLEN(bits));
    BP_STREAM_BITLEN(bits) = bit_count;
    BP_STREAM_BITS(bits) = bit_buf;
    if (bit_count >= BP_BITS_PER_WORD) {
        *BP_STREAM_CUR(bits) = bit_buf;
        bit_buf = BP_STREAM_BITLEN(bits) - BP_BITS_PER_WORD;
        BP_STREAM_CUR(bits) = BP_STREAM_CUR(bits) + 1;
        BP_STREAM_BITLEN(bits) = bit_buf;
        BP_STREAM_BITS(bits) = 0;
        if (bit_buf != 0) {
            BP_STREAM_BITS(bits) = lenbits >> (BP_LOSSY_LEVEL_BITS - bit_buf);
        }
    }

    roots = tree.roots;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP0_SLOT];
    if (lenbits > groups[BP_TREE_GROUP1_INDEX]) {
        entry = lenbits | BP_GROUP1_NODE_BASE;
    } else {
        entry = groups[BP_TREE_GROUP1_INDEX] | BP_GROUP1_NODE_BASE;
    }
    roots[BP_ROOT_GROUP1_SLOT] = entry;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP1_SLOT];
    if (lenbits > groups[BP_TREE_GROUP6_INDEX]) {
        entry = lenbits | BP_GROUP6_NODE_BASE;
    } else {
        entry = groups[BP_TREE_GROUP6_INDEX] | BP_GROUP6_NODE_BASE;
    }
    roots[BP_ROOT_GROUP6_SLOT] = entry;
    lenbits = hi_groups[BP_TREE_HIGH_GROUP2_SLOT];
    if (lenbits > groups[BP_TREE_GROUP11_INDEX]) {
        entry = lenbits | BP_GROUP11_NODE_BASE;
    } else {
        entry = groups[BP_TREE_GROUP11_INDEX] | BP_GROUP11_NODE_BASE;
    }
    roots[BP_ROOT_GROUP11_SLOT] = entry;
    roots[BP_ROOT_LOSSY_DC_SLOT] = groups[BP_TREE_GROUP_INDEX(BP_DC_COEFF)] + BP_TREE_BRANCH_NODE;

    cur = roots;
    next_node = tree.nodes;
    bit_mask = (u16)(1 << (maxbits - 1));
    i = 0;
    for (; maxbits != 0; maxbits = (maxbits - 1) & BP_BYTE_MASK) {
        count = 0;
        /* Coefficients introduced on earlier planes emit one residual bit here. */
        if (0 < i) {
            do {
                PUT_BP_BIT(bits, (active_absvals[count] & bit_mask) != 0);
                count++;
            } while (count < i);
        }

        insert = cur;
        if (cur < next_node) {
            do {
                node_entry = *cur;
                if (node_entry == BP_TREE_EMPTY_ENTRY) {
next_lossy_node:
                    cur++;
                } else {
                    count = (node_entry & BP_BYTE_MASK) == maxbits;
                    PUT_BP_BIT(bits, count);
                    if (!count) {
                        goto next_lossy_node;
                    }

                    lenbits = node_entry & BP_TREE_KIND_MASK;
                    if (lenbits == BP_TREE_GROUP_NODE) {
                        lenbits = node_entry >> BP_TREE_INDEX_SHIFT;
                        count = (u32)(node_entry >> BP_TREE_GROUP_SHIFT);
                        *cur = (u16)groups[count] + (node_entry & BP_TREE_BASE_MASK) + BP_TREE_BRANCH_NODE;
                        next_node[0] = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD1_INDEX], lenbits, BP_TREE_CHILD1_BASE);
                        next_node[1] = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD2_INDEX], lenbits, BP_TREE_CHILD2_BASE);
                        next_node[2] = BP_TREE_CHILD_BRANCH_ENTRY(groups[count + BP_TREE_CHILD3_INDEX], lenbits, BP_TREE_CHILD3_BASE);
                        next_node += BP_TREE_ADDED_CHILD_COUNT;
                    } else if (lenbits == BP_TREE_HIGH_NODE) {
                        *cur = (u16)hi_groups[node_entry >> BP_TREE_HIGH_GROUP_SHIFT] + ((node_entry >> BP_TREE_INDEX_SHIFT) + BP_TREE_CHILD1_BASE) * BP_TREE_INDEX_STRIDE + BP_TREE_GROUP_NODE;
handle_lossy_children:
                        lenbits = node_entry >> BP_TREE_INDEX_SHIFT;
                        PUT_BP_BIT(bits, lens[lenbits] != maxbits);
                        if (lens[lenbits] == maxbits) {
                            active_absvals[i] = absvals[lenbits];
                            i++;
                            PUT_BP_BIT(bits, (ordered[lenbits] & BP_SIGN_BIT) != 0);
                        } else {
                            --insert;
                            *insert = BP_TREE_BASE_COEFF_ENTRY(lens[lenbits], node_entry & BP_TREE_BASE_MASK);
                        }

                        PUT_BP_BIT(bits, lens[lenbits + BP_TREE_CHILD1_INDEX] != maxbits);
                        entry = lens[lenbits + BP_TREE_CHILD1_INDEX];
                        if (entry == maxbits) {
                            active_absvals[i] = absvals[lenbits + BP_TREE_CHILD1_INDEX];
                            i++;
                            PUT_BP_BIT(bits, (ordered[lenbits + BP_TREE_CHILD1_INDEX] & BP_SIGN_BIT) != 0);
                        } else {
                            --insert;
                            *insert = BP_TREE_COEFF_ENTRY(entry, lenbits + BP_TREE_CHILD1_INDEX);
                        }

                        PUT_BP_BIT(bits, lens[lenbits + BP_TREE_CHILD2_INDEX] != maxbits);
                        entry = lens[lenbits + BP_TREE_CHILD2_INDEX];
                        if (entry == maxbits) {
                            active_absvals[i] = absvals[lenbits + BP_TREE_CHILD2_INDEX];
                            i++;
                            PUT_BP_BIT(bits, (ordered[lenbits + BP_TREE_CHILD2_INDEX] & BP_SIGN_BIT) != 0);
                        } else {
                            --insert;
                            *insert = BP_TREE_COEFF_ENTRY(entry, lenbits + BP_TREE_CHILD2_INDEX);
                        }

                        PUT_BP_BIT(bits, lens[lenbits + BP_TREE_CHILD3_INDEX] != maxbits);
                        entry = lens[lenbits + BP_TREE_CHILD3_INDEX];
                        if (entry == maxbits) {
                            active_absvals[i] = absvals[lenbits + BP_TREE_CHILD3_INDEX];
                            i++;
                            PUT_BP_BIT(bits, (ordered[lenbits + BP_TREE_CHILD3_INDEX] & BP_SIGN_BIT) != 0);
                        } else {
                            --insert;
                            *insert = BP_TREE_COEFF_ENTRY(entry, lenbits + BP_TREE_CHILD3_INDEX);
                        }
                    } else {
                        if (lenbits == BP_TREE_BRANCH_NODE) {
                            *cur = BP_TREE_EMPTY_ENTRY;
                            cur++;
                            goto handle_lossy_children;
                        }
                        if (lenbits == BP_TREE_COEFF_NODE) {
                            active_absvals[i] = absvals[node_entry >> BP_TREE_INDEX_SHIFT];
                            i++;
                            PUT_BP_BIT(bits, (ordered[node_entry >> BP_TREE_INDEX_SHIFT] & BP_SIGN_BIT) != 0);
                            *cur = BP_TREE_EMPTY_ENTRY;
                        }
                        goto next_lossy_node;
                    }
                }
            } while (cur < next_node);
        }
        cur = insert;
        bit_mask = (s16)bit_mask >> 1;
    }

    return 1;
}

#pragma dont_inline on
static void readlossy(s8 PTR4* dest, BPBITSTREAM PTR4* bits, s32 masks_count)
{
    s8 sample;
    u32 old_bitcount;
    u32 levels_remaining;
    u8 node_kind;
    u8 PTR4* tree_end_ptr;
    s32 mask;
    s32 nz_coeff_count;
    u8 PTR4* node_ptr;
    u8 node;
    u32 code;
    u32 bit;
    s32 delta;
    s32 scan;
    BPBITSTYPE word;
    u8 PTR4* next_node_ptr;
    BPLOSSYREADTREE tree;
    u8 nz_coeff[BP_BLOCK_COEFFS];
    BPBITSTREAM bitcopy;

    bitcopy = *bits;
#define words bitcopy.cur
#define bitbuf bitcopy.bits
#define bitcount bitcopy.bitlen
    memset(dest, 0, BP_BLOCK_COEFFS);

    /* Lossy blocks store max level minus one in the stream header. */
    old_bitcount = bitcount;
    if (bitcount >= BP_LOSSY_LEVEL_BITS) {
        word = bitbuf;
        bitcount = bitcount - BP_LOSSY_LEVEL_BITS;
        bitbuf = bitbuf >> BP_LOSSY_LEVEL_BITS;
        code = word & BP_BYTE_MASK;
    } else {
        mask = BP_LOSSY_LEVEL_BITS - bitcount;
        code = bitbuf & BP_BYTE_MASK;
        word = *words;
        bitcount = bitcount + BP_BITS_PER_WORD - BP_LOSSY_LEVEL_BITS;
        words = words + 1;
        bitbuf = word >> mask;
        code = code | word << old_bitcount;
    }
    levels_remaining = (code & BP_LOSSY_LEVEL_MASK) + 1;
    tree.roots[BP_ROOT_GROUP1_SLOT] = BP_READ_TREE_GROUP1_ROOT;
    tree.roots[BP_ROOT_GROUP6_SLOT] = BP_READ_TREE_GROUP6_ROOT;
    tree.roots[BP_ROOT_GROUP11_SLOT] = BP_READ_TREE_GROUP11_ROOT;
    mask = (s32)(s8)(1 << (levels_remaining - 1));
    tree.roots[BP_ROOT_LOSSY_DC_SLOT] = BP_READ_TREE_DC_ROOT;
    tree_end_ptr = tree.nodes;
    nz_coeff_count = 0;
    node_ptr = tree.roots;
    do {
        if (levels_remaining == 0) {
            goto done;
        }
        scan = 0;
        /* Active coefficients receive one refinement bit at each lower plane. */
        if (0 < nz_coeff_count) {
            do {
                word = bitbuf;
                if (bitcount == 0) {
                    word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words = words + 1;
                    bitbuf = word >> 1;
                } else {
                    bitcount = bitcount - 1;
                    bitbuf = bitbuf >> 1;
                }
                if ((word & 1) != 0) {
                    sample = dest[(u32)nz_coeff[scan]];
                    delta = mask;
                    if (sample < 0) {
                        delta = -mask;
                    }
                    dest[(u32)nz_coeff[scan]] = sample + (s8)delta;
                    if (masks_count-- == 0) {
                        goto done;
                    }
                }
                scan = scan + 1;
            } while (scan < nz_coeff_count);
        }
        next_node_ptr = node_ptr;
        if (node_ptr < tree_end_ptr) {
            scan = -mask;
read_node:
            node = *node_ptr;
            if (node == BP_READ_TREE_EMPTY_ENTRY) {
next_node:
                node_ptr = node_ptr + 1;
            } else {
                if (bitcount == 0) {
                    word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words = words + 1;
                    bitbuf = word >> 1;
                    if ((word & 1) != 0) {
                        goto decode_node;
                    }
                    goto next_node;
                }
                bitcount = bitcount - 1;
                code = bitbuf & 1;
                bitbuf = bitbuf >> 1;
                if (code == 0) {
                    goto next_node;
                }
decode_node:
                node_kind = node & BP_READ_TREE_KIND_MASK;
                if (node_kind == BP_READ_TREE_GROUP_NODE) {
                    node_kind = BP_READ_TREE_INDEX(node);
                    *node_ptr = BP_READ_TREE_BRANCH_FROM_NODE(node);
                    *tree_end_ptr = BP_READ_TREE_CHILD_BRANCH(node_kind, BP_READ_TREE_CHILD1_BASE);
                    tree_end_ptr[1] = BP_READ_TREE_CHILD_BRANCH(node_kind, BP_READ_TREE_CHILD2_BASE);
                    tree_end_ptr[2] = BP_READ_TREE_CHILD_BRANCH(node_kind, BP_READ_TREE_CHILD3_BASE);
                    tree_end_ptr = tree_end_ptr + BP_TREE_ADDED_CHILD_COUNT;
                    goto node_done;
                }
                if (node_kind == BP_READ_TREE_HIGH_NODE) {
                    *node_ptr = BP_READ_TREE_GROUP_FROM_INDEX(BP_READ_TREE_INDEX(node));
                } else {
                    if (node_kind != BP_READ_TREE_BRANCH_NODE) {
                        if (node_kind != BP_READ_TREE_COEFF_NODE) {
                            goto next_node;
                        }
                        nz_coeff[nz_coeff_count] = BP_READ_TREE_INDEX(node);
                        /* Deferred coeff nodes already carry their scan index. */
                        word = bitbuf;
                        nz_coeff_count = nz_coeff_count + 1;
                        if (bitcount == 0) {
                            word = *words;
                            bitcount = BP_WORD_TOP_BIT;
                            words = words + 1;
                            bitbuf = word >> 1;
                        } else {
                            bitcount = bitcount - 1;
                            bitbuf = bitbuf >> 1;
                        }
                        delta = scan;
                        if ((word & 1) == 0) {
                            delta = mask;
                        }
                        dest[(u32)BP_READ_TREE_INDEX(node)] = (s8)delta;
                        if (masks_count-- == 0) {
                            goto done;
                        }
                        *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                        goto next_node;
                    }
                    *node_ptr = BP_READ_TREE_EMPTY_ENTRY;
                    node_ptr = node_ptr + 1;
                }
                code = (u32)BP_READ_TREE_INDEX(node);
                if (bitcount == 0) {
                    word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    bitbuf = word >> 1;
                    words = words + 1;
                    if ((word & 1) != 0) {
push_0:
                        *--next_node_ptr = BP_READ_TREE_BASE(node) + BP_READ_TREE_COEFF_NODE;
                        goto after_0;
                    }
                } else {
                    bitcount = bitcount - 1;
                    word = bitbuf >> 1;
                    bit = bitbuf & 1;
                    bitbuf = word;
                    if (bit != 0) {
                        goto push_0;
                    }
                }
                nz_coeff[nz_coeff_count] = BP_READ_TREE_INDEX(node);
                /* A zero child-presence bit introduces the coefficient immediately. */
                nz_coeff_count = nz_coeff_count + 1;
                if (bitcount == 0) {
                    bitbuf = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words = words + 1;
                } else {
                    bitcount = bitcount - 1;
                }
                bit = bitbuf & 1;
                bitbuf = bitbuf >> 1;
                delta = scan;
                if (bit == 0) {
                    delta = mask;
                }
                dest[code] = (s8)delta;
                if (masks_count-- == 0) {
                    goto done;
                }
after_0:
                node = (u8)(code + BP_TREE_CHILD1_INDEX);
                if (bitcount == 0) {
                    word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    bitbuf = word >> 1;
                    words = words + 1;
                    if ((word & 1) != 0) {
push_1:
                        *--next_node_ptr = BP_READ_TREE_COEFF(node);
                        goto after_1;
                    }
                } else {
                    bitcount = bitcount - 1;
                    word = bitbuf >> 1;
                    bit = bitbuf & 1;
                    bitbuf = word;
                    if (bit != 0) {
                        goto push_1;
                    }
                }
                nz_coeff[nz_coeff_count] = node;
                /* Nonzero children are pushed for later planes instead. */
                nz_coeff_count = nz_coeff_count + 1;
                if (bitcount == 0) {
                    bitbuf = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words = words + 1;
                } else {
                    bitcount = bitcount - 1;
                }
                bit = bitbuf & 1;
                bitbuf = bitbuf >> 1;
                delta = scan;
                if (bit == 0) {
                    delta = mask;
                }
                dest[code + BP_TREE_CHILD1_INDEX] = (s8)delta;
                if (masks_count-- == 0) {
                    goto done;
                }
after_1:
                node = (u8)(code + BP_TREE_CHILD2_INDEX);
                if (bitcount == 0) {
                    word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    bitbuf = word >> 1;
                    words = words + 1;
                    if ((word & 1) != 0) {
push_2:
                        *--next_node_ptr = BP_READ_TREE_COEFF(node);
                        goto after_2;
                    }
                } else {
                    bitcount = bitcount - 1;
                    word = bitbuf >> 1;
                    bit = bitbuf & 1;
                    bitbuf = word;
                    if (bit != 0) {
                        goto push_2;
                    }
                }
                nz_coeff[nz_coeff_count] = node;
                nz_coeff_count = nz_coeff_count + 1;
                if (bitcount == 0) {
                    bitbuf = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words = words + 1;
                } else {
                    bitcount = bitcount - 1;
                }
                bit = bitbuf & 1;
                bitbuf = bitbuf >> 1;
                delta = scan;
                if (bit == 0) {
                    delta = mask;
                }
                dest[code + BP_TREE_CHILD2_INDEX] = (s8)delta;
                if (masks_count-- == 0) {
                    goto done;
                }
after_2:
                word = bitbuf;
                node = (u8)(code + BP_TREE_CHILD3_INDEX);
                if (bitcount == 0) {
                    word = *words;
                    bitcount = BP_WORD_TOP_BIT;
                    words = words + 1;
                    bitbuf = word >> 1;
                } else {
                    bitcount = bitcount - 1;
                    bitbuf = bitbuf >> 1;
                }
                if ((word & 1) == 0) {
                    nz_coeff[nz_coeff_count] = node;
                    word = bitbuf;
                    /* The sign bit follows the first nonzero magnitude bit. */
                    nz_coeff_count = nz_coeff_count + 1;
                    if (bitcount == 0) {
                        word = *words;
                        bitcount = BP_WORD_TOP_BIT;
                        words = words + 1;
                        bitbuf = word >> 1;
                    } else {
                        bitcount = bitcount - 1;
                        bitbuf = bitbuf >> 1;
                    }
                    delta = scan;
                    if ((word & 1) == 0) {
                        delta = mask;
                    }
                    dest[code + BP_TREE_CHILD3_INDEX] = (s8)delta;
                    if (masks_count-- == 0) {
                        goto done;
                    }
                } else {
                    *--next_node_ptr = BP_READ_TREE_COEFF(node);
                }
            }
node_done:
            if (tree_end_ptr <= node_ptr) {
                goto level_done;
            }
            goto read_node;
        }
level_done:
        mask = mask >> 1;
        levels_remaining = (levels_remaining - 1) & BP_BYTE_MASK;
        node_ptr = next_node_ptr;
    } while (1);

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
    (out[(out_index)] = residuals.words[(residual_index)])
    SCATTER_BP_LOSSY_WORD(0, 0);
    SCATTER_BP_LOSSY_WORD(1, 2);
    SCATTER_BP_LOSSY_WORD(2, 4);
    SCATTER_BP_LOSSY_WORD(3, 6);
    SCATTER_BP_LOSSY_WORD(4, 1);
    SCATTER_BP_LOSSY_WORD(5, 3);
    SCATTER_BP_LOSSY_WORD(6, 5);
    SCATTER_BP_LOSSY_WORD(7, 7);
    SCATTER_BP_LOSSY_WORD(8, 12);
    SCATTER_BP_LOSSY_WORD(9, 22);
    SCATTER_BP_LOSSY_WORD(10, 8);
    SCATTER_BP_LOSSY_WORD(11, 10);
    SCATTER_BP_LOSSY_WORD(12, 13);
    SCATTER_BP_LOSSY_WORD(13, 23);
    SCATTER_BP_LOSSY_WORD(14, 9);
    SCATTER_BP_LOSSY_WORD(15, 11);
    SCATTER_BP_LOSSY_WORD(16, 14);
    SCATTER_BP_LOSSY_WORD(17, 16);
    SCATTER_BP_LOSSY_WORD(18, 24);
    SCATTER_BP_LOSSY_WORD(19, 26);
    SCATTER_BP_LOSSY_WORD(20, 15);
    SCATTER_BP_LOSSY_WORD(21, 17);
    SCATTER_BP_LOSSY_WORD(22, 25);
    SCATTER_BP_LOSSY_WORD(23, 27);
    SCATTER_BP_LOSSY_WORD(24, 18);
    SCATTER_BP_LOSSY_WORD(25, 20);
    SCATTER_BP_LOSSY_WORD(26, 28);
    SCATTER_BP_LOSSY_WORD(27, 30);
    SCATTER_BP_LOSSY_WORD(28, 19);
    SCATTER_BP_LOSSY_WORD(29, 21);
    SCATTER_BP_LOSSY_WORD(30, 29);
    SCATTER_BP_LOSSY_WORD(31, 31);
#undef SCATTER_BP_LOSSY_WORD
}

void ReadBPLossyWithMotion(char PTR4* out, s32 pitch, BPBITSTREAM PTR4* bits, s32 masks_count,
                           char PTR4* prev)
{
    char residuals[BP_BLOCK_COEFFS];
    char PTR4* dst = out;
    char PTR4* src = prev;

    readlossy(residuals, bits, masks_count);
    dst[0] = residuals[0] + src[0];
    dst[1] = residuals[1] + src[1];
    dst[2] = residuals[4] + src[2];
    dst[3] = residuals[5] + src[3];
    dst[4] = residuals[8] + src[4];
    dst[5] = residuals[9] + src[5];
    dst[6] = residuals[12] + src[6];
    dst[7] = residuals[13] + src[7];
    dst += pitch;
    dst[0] = residuals[2] + src[8];
    dst[1] = residuals[3] + src[9];
    dst[2] = residuals[6] + src[10];
    dst[3] = residuals[7] + src[11];
    dst[4] = residuals[10] + src[12];
    dst[5] = residuals[11] + src[13];
    dst[6] = residuals[14] + src[14];
    dst[7] = residuals[15] + src[15];
    dst += pitch;
    dst[0] = residuals[24] + src[16];
    dst[1] = residuals[25] + src[17];
    dst[2] = residuals[44] + src[18];
    dst[3] = residuals[45] + src[19];
    dst[4] = residuals[16] + src[20];
    dst[5] = residuals[17] + src[21];
    dst[6] = residuals[20] + src[22];
    dst[7] = residuals[21] + src[23];
    dst = dst + pitch;
    dst[0] = residuals[26] + src[24];
    dst[1] = residuals[27] + src[25];
    dst[2] = residuals[46] + src[26];
    dst[3] = residuals[47] + src[27];
    dst[4] = residuals[18] + src[28];
    dst[5] = residuals[19] + src[29];
    dst[6] = residuals[22] + src[30];
    dst[7] = residuals[23] + src[31];
    dst = dst + pitch;
    dst[0] = residuals[28] + src[32];
    dst[1] = residuals[29] + src[33];
    dst[2] = residuals[32] + src[34];
    dst[3] = residuals[33] + src[35];
    dst[4] = residuals[48] + src[36];
    dst[5] = residuals[49] + src[37];
    dst[6] = residuals[52] + src[38];
    dst[7] = residuals[53] + src[39];
    dst = dst + pitch;
    dst[0] = residuals[30] + src[40];
    dst[1] = residuals[31] + src[41];
    dst[2] = residuals[34] + src[42];
    dst[3] = residuals[35] + src[43];
    dst[4] = residuals[50] + src[44];
    dst[5] = residuals[51] + src[45];
    dst[6] = residuals[54] + src[46];
    dst[7] = residuals[55] + src[47];
    dst = dst + pitch;
    dst[0] = residuals[36] + src[48];
    dst[1] = residuals[37] + src[49];
    dst[2] = residuals[40] + src[50];
    dst[3] = residuals[41] + src[51];
    dst[4] = residuals[56] + src[52];
    dst[5] = residuals[57] + src[53];
    dst[6] = residuals[60] + src[54];
    dst[7] = residuals[61] + src[55];
    dst = dst + pitch;
    dst[0] = residuals[38] + src[56];
    dst[1] = residuals[39] + src[57];
    dst[2] = residuals[42] + src[58];
    dst[3] = residuals[43] + src[59];
    dst[4] = residuals[58] + src[60];
    dst[5] = residuals[59] + src[61];
    dst[6] = residuals[62] + src[62];
    dst[7] = residuals[63] + src[63];
}
