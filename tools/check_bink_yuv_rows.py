#!/usr/bin/env python3
"""Check the production doubly scaled RGB32 row against a scalar color oracle.

Uses synthetic YUV conversion tables and independently calculated saturated
channels. Covers zero/nonzero counts, phase parity and wrap, doubled geometry,
signed pitch, cursor/state changes, and guards. This is not a playback test.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from check_bink_yuv_columns import HEADER

ROOT = Path(__file__).resolve().parents[1]
TEST = r"""
RGBContext S;
RGBYUVTables YUVTables;
u32 *clamp_ytable[RGB_LUMA_TABLE_SIZE];
u32 clamptable[RGB_CLAMP_TABLE_SIZE];
static u32 seed = 0x59555652;
static u8 random_byte(void) {
    seed = seed * 1664525u + 1013904223u;
    return (u8)(seed >> 24);
}
static u32 clamp(s32 x) { return x < 0 ? 0 : x > 255 ? 255 : (u32)x; }
int main(void) {
    const u32 counts[] = {0, 1, 2, 3, 7, 16, 31};
    const s32 pitches[] = {64, 144, 288};
    const u32 phases[] = {0, 1, 2, 3, 0xfffffffeu, 0xffffffffu};
    const u8 edges[] = {0, 1, 15, 16, 17, 127, 128, 234, 235, 254, 255};
    RGBYUVTables tables_before;
    u32 clamp_before[RGB_CLAMP_TABLE_SIZE];
    u32 *pointers_before[RGB_LUMA_TABLE_SIZE];
    for (s32 i = 0; i < RGB_CLAMP_TABLE_SIZE; ++i) clamptable[i] = clamp(i - 256);
    for (u32 i = 0; i < RGB_LUMA_TABLE_SIZE; ++i) clamp_ytable[i] = clamptable + 256 + i;
    for (s32 i = 0; i < 256; ++i) {
        YUVTables.u_to_b[i] = (i - 128) * 2 / 3;
        YUVTables.v_to_r[i] = (i - 128) * 3 / 4;
        YUVTables.u_to_gb[i] = -(i - 128) / 5;
        YUVTables.v_to_gb[i] = -(i - 128) / 7;
    }
    memcpy(&tables_before, &YUVTables, sizeof(YUVTables));
    memcpy(clamp_before, clamptable, sizeof(clamptable));
    memcpy(pointers_before, clamp_ytable, sizeof(clamp_ytable));
    u32 cases = 0;
    for (u32 ci = 0; ci < 7; ++ci)
    for (u32 pi = 0; pi < 3; ++pi)
    for (u32 ph = 0; ph < 6; ++ph)
    for (u32 col = 0; col < 3; ++col)
    for (u32 reverse = 0; reverse < 2; ++reverse)
    for (u32 trial = 0; trial < 16; ++trial) {
        const u32 count = counts[ci], phase = phases[ph];
        const s32 pitch = reverse ? -pitches[pi] : pitches[pi];
        const s32 first = 16 + (reverse ? 4 : 1) * pitches[pi] / 4 + (s32)col;
        u32 actual[1024], expected[1024];
        u32 y[24], y_before[24], unused[8], unused_before[8];
        u16 u[40], v[40], u_before[40], v_before[40];
        u8 *ys = (u8 *)y + 16, *us = (u8 *)u + 16, *vs = (u8 *)v + 16;
        RGBContext wanted;
        if ((col + count * 2) * 4 > (u32)pitches[pi]) continue;
        memset(y, 0xc7, sizeof(y)); memset(u, 0xd3, sizeof(u)); memset(v, 0xe9, sizeof(v));
        memset(unused, 0x61, sizeof(unused));
        for (u32 x = 0; x < count; ++x)
            ys[x] = trial < 11 ? edges[(trial + x) % 11] : random_byte();
        for (u32 x = 0; x < (count + 2) / 2; ++x) {
            us[x] = trial < 11 ? edges[(trial + 3 * x) % 11] : random_byte();
            vs[x] = trial < 11 ? edges[(trial + 5 * x + 3) % 11] : random_byte();
        }
        memcpy(y_before, y, sizeof(y)); memcpy(u_before, u, sizeof(u));
        memcpy(v_before, v, sizeof(v)); memcpy(unused_before, unused, sizeof(unused));
        memset(actual, 0xa5, sizeof(actual)); memset(expected, 0xa5, sizeof(expected));
        memset(&S, 0, sizeof(S));
        S.base = (u8 *)(actual + 16); S.pitch = pitch;
        S.dest0 = (u8 *)(actual + first); S.dest1 = (u8 *)(actual + 800);
        S.y0 = (u32 *)ys; S.y1 = unused; S.a0 = unused + 2; S.a1 = unused + 4;
        S.u = (u16 *)us; S.v = (u16 *)vs;
        S.r = 17; S.gb = -23; S.b = 39; memset(S.pad, 0x72, sizeof(S.pad));
        memcpy(&wanted, &S, sizeof(S));
        wanted.dest0 += count * 8; wanted.y0 = (u32 *)(ys + count);
        const u32 chroma_used = (count + (phase & 1)) / 2;
        wanted.u = (u16 *)(us + chroma_used); wanted.v = (u16 *)(vs + chroma_used);
        for (u32 x = 0; x < count; ++x) {
            const u32 chroma = (x + (phase & 1)) / 2;
            const s32 uu = (s32)us[chroma] - 128, vv = (s32)vs[chroma] - 128;
            wanted.r = vv * 3 / 4; wanted.b = uu * 2 / 3;
            wanted.gb = -uu / 5 - vv / 7;
            u32 pixel = (clamp(ys[x] + wanted.r) << 16) |
                        (clamp(ys[x] + wanted.gb) << 8) | clamp(ys[x] + wanted.b);
            for (u32 dy = 0; dy < 2; ++dy)
            for (u32 dx = 0; dx < 2; ++dx)
                expected[first + (s32)dy * (pitch / 4) + x * 2 + dx] = pixel;
        }
        dounaligned32row2wh(phase, count);
        const char *failure = NULL;
        if (memcmp(actual, expected, sizeof(actual))) failure = "pixels/guards";
        else if (memcmp(&S, &wanted, sizeof(S))) failure = "context/cursors";
        else if (memcmp(y, y_before, sizeof(y)) || memcmp(u, u_before, sizeof(u)) ||
                 memcmp(v, v_before, sizeof(v)) || memcmp(unused, unused_before, sizeof(unused)) ||
                 memcmp(&YUVTables, &tables_before, sizeof(YUVTables)) ||
                 memcmp(clamptable, clamp_before, sizeof(clamptable)) ||
                 memcmp(clamp_ytable, pointers_before, sizeof(clamp_ytable))) failure = "inputs/tables";
        if (failure) {
            printf("FAIL %s count=%u phase=%08x pitch=%d col=%u trial=%u\n",
                   failure, count, phase, pitch, col, trial);
            return 1;
        }
        ++cases;
    }
    printf("PASS %u YUV row cases: colors, zero count, phase, doubling, pitch, cursors, guards\n", cases);
    return 0;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--source', type=Path, default=ROOT / 'src/bink/src/sdk/decode/yuv.cpp')
    parser.add_argument('--self-test', action='store_true', help='require pixel and chroma-phase mutations to fail')
    args = parser.parse_args()
    source = args.source.read_text()
    header = (ROOT / 'src/bink/src/sdk/decode/ngc/ngcrgb.h').read_text().replace('#include "bink.h"', '')
    pieces = [HEADER, header]
    for name in ('RGBPackConstants', 'YUVBlitLayout', 'YUVChromaLayout'):
        start = source.index('enum ' + name + ' {')
        pieces.append(source[start:source.index('};', start) + 2])
    for macro in ('RGB32_COLOR(', 'YUV_PHASE_ADVANCES_CHROMA(', 'YUV_REMAINING_PIXELS_AFTER_FIRST('):
        pieces.append(next(line for line in source.splitlines() if line.startswith('#define ' + macro)))
    start = source.index('static void dounaligned32row2wh(u32 phase, u32 count)\n{')
    body = source[start:source.index('\n}', start) + 2]
    with tempfile.TemporaryDirectory(prefix='bink_yuv_rows_') as directory:
        cfile, exe = Path(directory) / 'check.c', Path(directory) / 'check.exe'

        def run(function):
            cfile.write_text('\n'.join(pieces) + '\n' + function + TEST)
            subprocess.run([args.cc, '-O2', '-std=c99', '-Wall', '-Wextra', '-Werror',
                            str(cfile), '-o', str(exe)], check=True)
            return subprocess.run([str(exe)], capture_output=True, text=True)

        result = run(body)
        print(result.stdout, end='')
        result.check_returncode()
        if args.self_test:
            for name, old, new in (
                ('pixel channel', 'pixel = RGB32_COLOR(', 'pixel = 1u ^ RGB32_COLOR('),
                ('chroma phase', 'if (YUV_PHASE_ADVANCES_CHROMA(phase))',
                 'if (!YUV_PHASE_ADVANCES_CHROMA(phase))'),
            ):
                if body.count(old) != 1: raise RuntimeError('Negative-control mutation site changed: ' + name)
                result = run(body.replace(old, new))
                if result.returncode != 1 or not result.stdout.startswith('FAIL '):
                    raise RuntimeError('Negative control did not detect ' + name + ': ' + result.stdout)
                print('PASS negative control (' + name + '): ' + result.stdout.strip())


if __name__ == '__main__':
    main()
