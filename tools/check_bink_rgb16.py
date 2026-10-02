#!/usr/bin/env python3
"""Check the four production tiled RGB16 kernels against a scalar pixel oracle.

The channel pairing follows yuv.cpp and the retail ngcrgb object: U feeds
clamp_r and V feeds clamp_b. Synthetic valid lookup tables exercise the mapping;
packed input words model big-endian loads on a host compiler. This checks
conversion and tile addressing, not game playback.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
#define PTR4
"""
TEST = r"""
u32 ytable[RGB_LUMA_TABLE_SIZE];
u32 clamp_r[RGB_CLAMP_TABLE_SIZE], clamp_g[RGB_CLAMP_TABLE_SIZE];
u32 clamp_b[RGB_CLAMP_TABLE_SIZE], clamp_a4[RGB_LUMA_TABLE_SIZE];
RGBContext S;
RGBYUVTables YUVTables;
static u32 seed = 0x52474231;
static u32 rnd(void) { seed = seed * 1664525u + 1013904223u; return seed; }
static void init_tables(int alpha) {
    for (int i = 0; i < RGB_CLAMP_TABLE_SIZE; ++i) {
        int v = i - 256;
        if (v < 0) v = 0;
        if (v > 255) v = 255;
        clamp_r[i] = alpha ? (v >> 4) << 8 : (v >> 3) << 11;
        clamp_g[i] = alpha ? (v >> 4) << 4 : (v >> 2) << 5;
        clamp_b[i] = alpha ? (v >> 4) : (v >> 3);
    }
    for (int i = 0; i < RGB_LUMA_TABLE_SIZE; ++i) {
        ytable[i] = i;
        clamp_a4[i] = (i >> 4) << 12;
    }
    for (int i = 0; i < 256; ++i) {
        YUVTables.u_to_b[i] = (i - 128) * 2 / 3;
        YUVTables.v_to_r[i] = (i - 128) * 3 / 4;
        YUVTables.u_to_gb[i] = -(i - 128) / 5;
        YUVTables.v_to_gb[i] = -(i - 128) / 7;
    }
}
static u32 byte_at(u32 word, u32 pos) { return (word >> (24 - 8 * pos)) & 255; }
static u16 reference_pixel(u32 y, u32 u, u32 v, u32 a, int alpha) {
    s32 luma = ytable[y];
    s32 first = luma + YUVTables.u_to_b[u];
    s32 middle = luma + YUVTables.u_to_gb[u] + YUVTables.v_to_gb[v];
    s32 last = luma + YUVTables.v_to_r[v];
    u32 pixel = clamp_r[256 + first] | clamp_g[256 + middle] | clamp_b[256 + last];
    if (alpha) pixel |= clamp_a4[a];
    return (u16)pixel;
}
static void write_pixel(u32 *image, u32 pitch, u32 row, u32 col, u16 pixel) {
    u32 tile_stride = (4 * pitch + 31) & ~31u;
    u32 byte = (row / 4) * tile_stride + (col / 4) * 32 + (row % 4) * 8 + (col % 4) * 2;
    u32 shift = (col & 1) ? 0 : 16;
    image[byte / 4] = (image[byte / 4] & ~(0xffffu << shift)) | ((u32)pixel << shift);
}
int main(void) {
    void (*kernels[])(u32) = {YUV_16_4x2_even, YUV_16x2_4x2_even,
                              YUV_16a4_4x2_even, YUV_16a4x2_4x2_even};
    const u32 counts[] = {1, 2, 3, 5, 8}, pitches[] = {64, 96, 160};
    const u32 edges[] = {0, 1, 15, 16, 127, 128, 240, 254, 255};
    u32 cases = 0;
    for (u32 kind = 0; kind < 4; ++kind) {
        u32 scale = (kind & 1) + 1, alpha = kind / 2;
        init_tables(alpha);
        for (u32 ci = 0; ci < 5; ++ci)
        for (u32 pi = 0; pi < 3; ++pi)
        for (u32 row = 0; row < 8; ++row)
        for (u32 col = 0; col <= 4; col += 4)
        for (u32 trial = 0; trial < 16; ++trial) {
            u32 count = counts[ci], pitch = pitches[pi];
            u32 actual[2048], expected[2048], y[2][8], a[2][8];
            u16 u[8], v[8];
            u8 *base = (u8 *)(actual + 16);
            if (col + 4 * count * scale > pitch / 2) continue;
            for (u32 n = 0; n < count; ++n) {
                u[n] = (u16)rnd(); v[n] = (u16)rnd();
                for (u32 r = 0; r < 2; ++r) {
                    y[r][n] = rnd(); a[r][n] = rnd();
                    if (trial < 9) {
                        u32 yy = edges[(trial + r + n) % 9];
                        u32 aa = edges[(trial + 2 * r + n) % 9];
                        y[r][n] = yy * 0x01010101u;
                        a[r][n] = aa * 0x01010101u;
                    }
                }
                if (trial < 9) {
                    u[n] = (u16)((edges[trial] << 8) | edges[(trial + 4) % 9]);
                    v[n] = (u16)((edges[(trial + 2) % 9] << 8) | edges[(trial + 7) % 9]);
                }
            }
            memset(actual, 0xa5, sizeof(actual)); memset(expected, 0xa5, sizeof(expected));
            memset(&S, 0, sizeof(S));
            S.base = base; S.pitch = pitch;
            S.dest0 = base + row * pitch + col * 2;
            S.dest1 = base + (row + 1) * pitch + col * 2;
            S.y0 = y[0]; S.y1 = y[1]; S.u = u; S.v = v; S.a0 = a[0]; S.a1 = a[1];
            for (u32 r = 0; r < 2; ++r)
            for (u32 x = 0; x < count * 4; ++x) {
                u32 n = x / 4, pos = x % 4, chroma_shift = pos < 2 ? 8 : 0;
                u16 pixel = reference_pixel(byte_at(y[r][n], pos), (u[n] >> chroma_shift) & 255,
                                             (v[n] >> chroma_shift) & 255, byte_at(a[r][n], pos), alpha);
                for (u32 dup = 0; dup < scale; ++dup)
                    write_pixel(expected + 16, pitch, row + r, col + x * scale + dup, pixel);
            }
            kernels[kind](count);
            if (memcmp(actual, expected, sizeof(actual))) {
                for (u32 i = 0; i < 2048; ++i) if (actual[i] != expected[i]) {
                    printf("FAIL pixels kind=%u count=%u pitch=%u row=%u col=%u trial=%u word=%u actual=%08x expected=%08x\n",
                           kind, count, pitch, row, col, trial, i, actual[i], expected[i]); break;
                }
                return 1;
            }
            if (S.dest0 != base + row * pitch + col * 2 + count * 8 * scale ||
                S.dest1 != base + (row + 1) * pitch + col * 2 + count * 8 * scale ||
                S.u != u + count || S.v != v + count || S.y0 != y[0] + count || S.y1 != y[1] + count ||
                S.a0 != a[0] + (alpha ? count : 0) || S.a1 != a[1] + (alpha ? count : 0) ||
                S.base != base || S.pitch != (s32)pitch) {
                printf("FAIL context kind=%u count=%u\n", kind, count); return 1;
            }
            ++cases;
        }
    }
    printf("PASS %u RGB16 cases: colors, alpha, scaling, tiles, context, and guards\n", cases);
    return 0;
}
"""

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="clang")
    parser.add_argument("--source", type=Path, default=ROOT / "src/bink/src/sdk/decode/ngc/ngcrgb.c")
    args = parser.parse_args()
    source = args.source.read_text()
    header = (ROOT / "src/bink/src/sdk/decode/ngc/ngcrgb.h").read_text().replace('#include "bink.h"', '')
    prefix = source[:source.index('void YUV_32_4x2_even(')]
    prefix = '\n'.join(line for line in prefix.splitlines() if not line.startswith('#include'))
    funcs = []
    for name in ['YUV_16_4x2_even', 'YUV_16x2_4x2_even', 'YUV_16a4_4x2_even', 'YUV_16a4x2_4x2_even']:
        start = source.index('void ' + name + '(')
        end = source.index('\nvoid ', start + 5)
        funcs.append(source[start:end])
    with tempfile.TemporaryDirectory(prefix='bink_rgb16_') as directory:
        path = Path(directory)
        cfile, exe = path / 'check.c', path / 'check.exe'
        cfile.write_text(HEADER + header + '\n' + prefix + '\n' + '\n'.join(funcs) + TEST)
        subprocess.run([args.cc, '-O2', '-std=c99', str(cfile), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)

if __name__ == '__main__':
    main()
