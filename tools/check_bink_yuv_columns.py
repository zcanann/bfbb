#!/usr/bin/env python3
"""Check three production grayscale YUV column routines with a scalar oracle.

Extracts the current source bodies and constants, then checks positive pixel
counts, both source rows, 1x/2x dimensions, alpha, signed pitch, phase wrap,
cursors, untouched context, and input/output guards. Output words are compared
as numeric packed RGB values; this is not a big-endian ABI or playback test.
Use --self-test to also prove detection of pixel and cursor mutations.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
FUNCTIONS = ('dounaligned32colm', 'dounaligned32acolm2h', 'dounaligned32acolm2wh')
HEADER = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
#define PTR4
"""
TEST = r"""
u32 mono32[RGB_MONO_TABLE_SIZE];
RGBContext S;
static u32 seed = 0x59555643;
static u8 random_byte(void) {
    seed = seed * 1664525u + 1013904223u;
    return (u8)(seed >> 24);
}
static u32 gray(u8 y) {
    if (y <= 16) return 0;
    if (y >= 235) return 255;
    return ((u32)y - 16) * 255 / 219;
}
int main(void) {
    u32 (*kernels[])(u32, s32) = {
        dounaligned32colm, dounaligned32acolm2h, dounaligned32acolm2wh
    };
    const u32 counts[] = {1, 2, 3, 7, 16, 31};
    const s32 pitches[] = {64, 144, 288};
    const s32 phases[] = {0, 1, -1, -17, INT32_MAX, INT32_MIN};
    const u8 edges[] = {0, 1, 15, 16, 17, 127, 128, 234, 235, 254, 255};
    u32 cases = 0, table_before[256];
    /* A standard limited-range grayscale palette, independent of the kernels. */
    for (u32 i = 0; i < 256; ++i) {
        u32 value = i < 17 ? 0 : i >= 235 ? 255 : (i - 16) * 255 / 219;
        mono32[i] = value * 0x010101u;
    }
    memcpy(table_before, mono32, sizeof(mono32));
    for (u32 kind = 0; kind < 3; ++kind)
    for (u32 ci = 0; ci < 6; ++ci)
    for (u32 pi = 0; pi < 3; ++pi)
    for (u32 ph = 0; ph < 6; ++ph)
    for (u32 col = 0; col < 3; ++col)
    for (u32 reverse = 0; reverse < 2; ++reverse)
    for (u32 trial = 0; trial < 16; ++trial) {
        const u32 count = counts[ci], scale_x = kind == 2 ? 2 : 1;
        const u32 scale_y = kind ? 2 : 1;
        const s32 pitch = reverse ? -pitches[pi] : pitches[pi];
        const s32 row = reverse ? 6 : 1;
        const s32 first_word = 16 + row * pitches[pi] / 4 + (s32)col;
        const s32 row_words = pitch / 4;
        u32 actual[1024], expected[1024];
        u32 y[2][24], a[2][24], y_before[2][24], a_before[2][24];
        u16 chroma[8], chroma_before[8];
        u8 *ys[2] = {(u8 *)y[0] + 16, (u8 *)y[1] + 16};
        u8 *as[2] = {(u8 *)a[0] + 16, (u8 *)a[1] + 16};
        RGBContext expected_context;
        if ((col + count * scale_x) * 4 > (u32)pitches[pi]) continue;
        memset(y, 0xc7, sizeof(y)); memset(a, 0xd9, sizeof(a));
        memset(chroma, 0x3b, sizeof(chroma));
        for (u32 r = 0; r < 2; ++r)
        for (u32 x = 0; x < count; ++x) {
            ys[r][x] = trial < 11 ? edges[(trial + x + 3 * r) % 11] : random_byte();
            as[r][x] = trial < 11 ? edges[(2 * trial + 3 * x + 5 * r) % 11] : random_byte();
        }
        memcpy(y_before, y, sizeof(y)); memcpy(a_before, a, sizeof(a));
        memcpy(chroma_before, chroma, sizeof(chroma));
        memset(actual, 0xa5, sizeof(actual)); memset(expected, 0xa5, sizeof(expected));
        memset(&S, 0, sizeof(S));
        S.dest0 = (u8 *)(actual + first_word);
        S.dest1 = (u8 *)(actual + first_word + (s32)scale_y * row_words);
        S.y0 = (u32 *)ys[0]; S.y1 = (u32 *)ys[1];
        S.a0 = (u32 *)as[0]; S.a1 = (u32 *)as[1];
        S.u = chroma; S.v = chroma + 4;
        S.pitch = pitch; S.base = (u8 *)(actual + 16);
        S.r = 19; S.gb = -27; S.b = 43;
        memset(S.pad, 0x62, sizeof(S.pad));
        memcpy(&expected_context, &S, sizeof(S));
        expected_context.dest0 += count * scale_x * 4;
        expected_context.dest1 += count * scale_x * 4;
        expected_context.y0 = (u32 *)(ys[0] + count);
        expected_context.y1 = (u32 *)(ys[1] + count);
        if (kind) {
            expected_context.a0 = (u32 *)(as[0] + count);
            expected_context.a1 = (u32 *)(as[1] + count);
        }
        /* Geometry-based oracle: each source sample fills its scaled rectangle. */
        for (u32 r = 0; r < 2; ++r)
        for (u32 x = 0; x < count; ++x) {
            u32 value = gray(ys[r][x]);
            u32 pixel = (value << 16) + (value << 8) + value;
            if (kind) pixel += (u32)as[r][x] << 24;
            for (u32 dy = 0; dy < scale_y; ++dy)
            for (u32 dx = 0; dx < scale_x; ++dx) {
                s32 index = first_word + (s32)(r * scale_y + dy) * row_words;
                expected[index + x * scale_x + dx] = pixel;
            }
        }
        u32 result = kernels[kind](count, phases[ph]);
        const char *failure = NULL;
        if (memcmp(actual, expected, sizeof(actual))) failure = "pixels/guards";
        else if (memcmp(&S, &expected_context, sizeof(S))) failure = "context/cursors";
        else if (result != (u32)phases[ph] + count) failure = "returned phase";
        else if (memcmp(y, y_before, sizeof(y)) || memcmp(a, a_before, sizeof(a)) ||
                 memcmp(chroma, chroma_before, sizeof(chroma)) ||
                 memcmp(mono32, table_before, sizeof(mono32))) failure = "input/table guards";
        if (failure) {
            printf("FAIL %s kind=%u count=%u pitch=%d phase=%d col=%u trial=%u\n",
                   failure, kind, count, pitch, phases[ph], col, trial);
            return 1;
        }
        ++cases;
    }
    printf("PASS %u YUV column cases: grayscale/alpha, dimensions, both rows, signed pitch, phase, cursors, guards\n", cases);
    return 0;
}
"""


def extract(source):
    pieces = []
    for name in ('RGBPackConstants', 'YUVBlitLayout'):
        start = source.index('enum ' + name + ' {')
        pieces.append(source[start:source.index('};', start) + 2])
    pieces.append(next(line for line in source.splitlines() if line.startswith('#define RGB32_M(y)')))
    for name in FUNCTIONS:
        start = source.index('static u32 ' + name + '(u32 count, s32 phase)\n{')
        pieces.append(source[start:source.index('\n}', start) + 2])
    return '\n'.join(pieces)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--source', type=Path, default=ROOT / 'src/bink/src/sdk/decode/yuv.cpp')
    parser.add_argument('--self-test', action='store_true', help='also require pixel and cursor mutations to fail')
    args = parser.parse_args()
    source = extract(args.source.read_text())
    header = (ROOT / 'src/bink/src/sdk/decode/ngc/ngcrgb.h').read_text().replace('#include "bink.h"', '')
    with tempfile.TemporaryDirectory(prefix='bink_yuv_columns_') as directory:
        path = Path(directory)
        cfile, exe = path / 'check.c', path / 'check.exe'

        def run(body):
            cfile.write_text(HEADER + header + '\n' + body + TEST)
            subprocess.run([args.cc, '-O2', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            str(cfile), '-o', str(exe)], check=True)
            return subprocess.run([str(exe)], capture_output=True, text=True)

        result = run(source)
        print(result.stdout, end='')
        result.check_returncode()
        if args.self_test:
            mutations = (
                ('pixel channel', 'pixel0 = RGB32_M(y);',
                 'pixel0 = RGB32_M(y) ^ 1u;', 'FAIL pixels/guards'),
                ('input cursor', 'S.y1 = (u32 PTR4*)yptr1;',
                 'S.y1 = (u32 PTR4*)(yptr1 + 1);', 'FAIL context/cursors'),
            )
            start = source.index('static u32 ' + FUNCTIONS[0] + '(')
            end = source.index('\n}', start) + 2
            control = source[start:end]
            for name, original, replacement, expected in mutations:
                if control.count(original) != 1:
                    raise RuntimeError('Negative-control mutation site changed: ' + name)
                mutated = control.replace(original, replacement)
                result = run(source[:start] + mutated + source[end:])
                if result.returncode != 1 or expected not in result.stdout:
                    raise RuntimeError('Negative control did not detect ' + name + ': ' + result.stdout)
                print('PASS negative control (' + name + '): ' + result.stdout.strip())


if __name__ == '__main__':
    main()
