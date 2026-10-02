#!/usr/bin/env python3
"""Check production Huff8 decoding against a bit-at-a-time stateful reader."""
import argparse
from pathlib import Path
import subprocess
import tempfile

from check_bink_huff4 import HEADER, ROOT

TEST = r"""
static u32 seed = 42;
static u32 rnd(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }
static u32 read_bit(EXPBITS* bits) {
    u32 value;
    if (!bits->bitlen) { bits->bits = *bits->cur++; bits->bitlen = 32; }
    value = bits->bits & 1;
    bits->bits >>= 1;
    --bits->bitlen;
    return value;
}
int main(void) {
    u32 width, available, initial_state, trial, cases = 0;
    for (width = 1; width <= 8; ++width)
    for (available = 0; available <= 32; ++available)
    for (initial_state = 0; initial_state < 16; ++initial_state)
    for (trial = 0; trial < 32; ++trial) {
        HUFF8TABLE table;
        u8 codes[16][256];
        u32 words[4], state, i, step, actual_state = initial_state, ref_state = initial_state;
        EXPBITS actual, ref;
        for (i = 0; i < 4; ++i) words[i] = rnd();
        for (state = 0; state < 16; ++state) {
            u32 peek = state == initial_state ? width : 1 + rnd() % 8;
            table.bits_to_peek[state] = peek;
            table.decode[state] = codes[state];
            for (i = 0; i < 16; ++i) table.syms[state][i] = rnd() & 15;
            for (i = 0; i < (1u << peek); ++i)
                codes[state][i] = ((1 + rnd() % peek) << 4) | (rnd() & 15);
        }
        actual.cur = words;
        actual.bitlen = available;
        actual.bits = rnd() & (available == 32 ? 0xffffffffu : ((1u << available) - 1));
        ref = actual;
        for (step = 0; step < 4; ++step) {
            EXPBITS look = ref;
            u32 index = 0, packed, used;
            for (i = 0; i < table.bits_to_peek[ref_state]; ++i)
                index |= read_bit(&look) << i;
            packed = table.decode[ref_state][index];
            used = packed >> 4;
            ref_state = table.syms[ref_state][packed & 15];
            for (i = 0; i < used; ++i) read_bit(&ref);
            actual_state = exp_read_huff8(&actual, actual_state, &table);
            if (actual_state != ref_state || actual.cur != ref.cur ||
                actual.bits != ref.bits || actual.bitlen != ref.bitlen) {
                printf("FAIL width=%u available=%u state=%u trial=%u step=%u\n",
                       width, available, initial_state, trial, step);
                return 1;
            }
        }
        ++cases;
    }
    printf("PASS %u Huff8 cases, four state-dependent reads each\n", cases);
    return 0;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="clang")
    args = parser.parse_args()
    source = (ROOT / "src/bink/src/sdk/decode/expand.c").read_text()
    table_start = source.index("typedef struct HUFF8TABLE")
    table_end = source.index("} HUFF8TABLE;", table_start) + len("} HUFF8TABLE;")
    helper_start = source.index("static inline u32 exp_read_huff8(")
    helper_end = source.index("static void ReadHuffTable", helper_start)
    definitions = "\n#define HUFF8_TABLE_STATES 16\n#define HUFF4_SYMBOLS 16\n#define HUFF4_CODE_SYMBOL(code) ((code)&15)\n"
    with tempfile.TemporaryDirectory(prefix="bink_huff8_") as directory:
        path = Path(directory)
        cfile, exe = path / "check.c", path / "check.exe"
        cfile.write_text(HEADER + definitions + source[table_start:table_end] + "\n" + source[helper_start:helper_end] + TEST)
        subprocess.run([args.cc, "-O2", "-std=c99", str(cfile), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
