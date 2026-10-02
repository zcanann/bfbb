#!/usr/bin/env python3
"""Check production YUV scaling setup against an independent dispatch oracle.

Covers all mode bit patterns, grayscale/channel inversion, previous table order,
varied row widths/pitches/pixel sizes, and even/odd alignment-step combinations.
This checks setup behavior on a host; it does not exercise renderer playback.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from check_bink_yuv_columns import HEADER

ROOT = Path(__file__).resolve().parents[1]
GLOBALS = r"""
RGBContext S;
RGBYUVTables YUVTables, origYUVTables;
static YUVTableOrder whichyuv;
static u32 testing[2], alignshift, align, alignm1;
static CoreBlitFn EVEN, ODD, EVENx, ODDx;
static RowBlitFn dounalignedrow;
static ColBlitFn dounalignedcol;
static u32 zoom_calls, zoom_bytes;
static volatile u32 sink;
static void checkzoombufs(u32 bytes) { ++zoom_calls; zoom_bytes = bytes; }
static void zoom2heven(s32 x) { sink = (u32)x + 101; }
static void zoom2hodd(s32 x) { sink = (u32)x + 102; }
"""
TEST = r"""
static u32 maximum(u32 a, u32 b) { return a > b ? a : b; }
int main(void) {
    u32 cases = 0;
    for (u32 mode = 0; mode < 8; ++mode)
    for (u32 gray = 0; gray < 2; ++gray)
    for (u32 invert = 0; invert < 2; ++invert)
    for (u32 previous = 0; previous < 2; ++previous)
    for (u32 wi = 0; wi < 2; ++wi)
    for (u32 bytes = 2; bytes <= 4; ++bytes)
    for (u32 pi = 0; pi < 2; ++pi)
    for (u32 even_step = 0; even_step < 7; ++even_step)
    for (u32 odd_step = 0; odd_step < 7; ++odd_step) {
        const u32 width = wi ? 19 : 0, initial_pitch = pi ? 512 : 64;
        const bool zoom = mode == 1 || mode == 4;
        const bool wide = mode == 3 || mode == 4 || mode == 5;
        const u32 row_kind = mode == 1 ? 2 : mode == 4 ? 3 : wide ? 1 : 0;
        const u32 flags = (mode << 28) | (gray << 17) | (invert << 16) | 0x80000000u;
        BLITS blits = {};
        blits.bytes_per_pixel = bytes;
        blits.even_step = even_step; blits.odd_step = odd_step;
        blits.even_x2_step = (even_step + 1) % 7;
        blits.odd_x2_step = (odd_step + 3) % 7;
        blits.masked_step = (even_step + 2) % 7;
        blits.masked_x2_step = (odd_step + 4) % 7;
        blits.even = core0; blits.odd = core1; blits.masked = core2;
        blits.even_x2 = core3; blits.odd_x2 = core4; blits.masked_x2 = core5;
        blits.row = row0; blits.rowm = row1;
        blits.row2w = row2; blits.rowm2w = row3;
        blits.row2h = row4; blits.rowm2h = row5;
        blits.row2wh = row6; blits.rowm2wh = row7;
        blits.col = col0; blits.colm = col1;
        blits.col2w = col2; blits.colm2w = col3;
        blits.col2h = col4; blits.colm2h = col5;
        blits.col2wh = col6; blits.colm2wh = col7;
        const RowBlitFn rows[] = {row0, row1, row2, row3, row4, row5, row6, row7};
        const ColBlitFn cols[] = {col0, col1, col2, col3, col4, col5, col6, col7};
        const CoreBlitFn normal_even = wide ? core3 : core0;
        const CoreBlitFn normal_odd = wide ? core4 : core1;
        const CoreBlitFn masked = wide ? core5 : core2;
        const CoreBlitFn expected_even = zoom ? zoom2heven : gray ? masked : normal_even;
        const CoreBlitFn expected_odd = zoom ? zoom2hodd : gray ? masked : normal_odd;
        const CoreBlitFn expected_evenx = zoom ? (gray ? masked : normal_even) : core6;
        const CoreBlitFn expected_oddx = zoom ? (gray ? masked : normal_odd) : core7;
        const u32 expected_step = gray ? (wide ? blits.masked_x2_step : blits.masked_step) :
            wide ? maximum(blits.even_x2_step, blits.odd_x2_step) : maximum(even_step, odd_step);
        RGBYUVTables expected_tables;
        s32 *original[] = {origYUVTables.u_to_b, origYUVTables.v_to_gb,
                          origYUVTables.u_to_gb, origYUVTables.v_to_r};
        s32 *actual[] = {YUVTables.u_to_b, YUVTables.v_to_gb,
                        YUVTables.u_to_gb, YUVTables.v_to_r};
        s32 *expected[] = {expected_tables.u_to_b, expected_tables.v_to_gb,
                          expected_tables.u_to_gb, expected_tables.v_to_r};
        for (u32 channel = 0; channel < 4; ++channel)
        for (u32 i = 0; i < 256; ++i) {
            original[channel][i] = (s32)(channel * 1000 + i) - 1500;
            actual[channel][i] = (s32)(channel * 2000 + i) - 3000;
        }
        for (u32 channel = 0; channel < 4; ++channel)
        for (u32 i = 0; i < 256; ++i)
            expected[channel][i] = previous == invert ? actual[channel][i] :
                                   original[invert ? 3 - channel : channel][i];
        RGBYUVTables original_before;
        memcpy(&original_before, &origYUVTables, sizeof(original_before));
        memset(&S, 0, sizeof(S)); S.r = 17; S.gb = -23; S.b = 39;
        RGBContext expected_context;
        memcpy(&expected_context, &S, sizeof(S)); expected_context.pitch = (s32)initial_pitch;
        whichyuv = (YUVTableOrder)previous;
        testing[0] = 42; testing[1] = 73;
        EVEN = ODD = core7; EVENx = core6; ODDx = core7;
        alignshift = align = alignm1 = 99;
        zoom_calls = 0; zoom_bytes = 12345;
        u32 pitch = initial_pitch, pitch_delta = 99;
        setup_scaling(flags, &pitch, width, 37, &blits, &pitch_delta);
        const u32 expected_pitch = initial_pitch * (zoom ? 2 : 1);
        if (pitch != expected_pitch || pitch_delta != expected_pitch - width * bytes * (wide ? 2 : 1) ||
            EVEN != expected_even || ODD != expected_odd || EVENx != expected_evenx || ODDx != expected_oddx ||
            dounalignedrow != rows[row_kind * 2 + gray] || dounalignedcol != cols[row_kind * 2 + gray] ||
            alignshift != expected_step || align != (1u << expected_step) || alignm1 != (1u << expected_step) - 1 ||
            zoom_calls != (u32)zoom || zoom_bytes != (zoom ? width * bytes * (wide ? 2 : 1) : 12345) ||
            testing[0] != 0 || testing[1] != 73 || whichyuv != (YUVTableOrder)invert ||
            memcmp(&S, &expected_context, sizeof(S)) || memcmp(&YUVTables, &expected_tables, sizeof(YUVTables)) ||
            memcmp(&origYUVTables, &original_before, sizeof(origYUVTables))) {
            printf("FAIL mode=%u gray=%u invert=%u previous=%u width=%u bytes=%u pitch=%u steps=%u,%u\n",
                   mode, gray, invert, previous, width, bytes, initial_pitch, even_step, odd_step);
            return 1;
        }
        ++cases;
    }
    printf("PASS %u YUV scaling cases: dispatch, pitches, alignments, zoom requests, tables, context\n", cases);
    return 0;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--source', type=Path, default=ROOT / 'src/bink/src/sdk/decode/yuv.cpp')
    parser.add_argument('--self-test', action='store_true', help='require an inverted maximum to fail')
    args = parser.parse_args()
    source = args.source.read_text()
    header = (ROOT / 'src/bink/src/sdk/decode/ngc/ngcrgb.h').read_text().replace('#include "bink.h"', '')
    pieces = [HEADER, header, source[source.index('typedef void (*CoreBlitFn)'):source.index('enum YUVTableLayout')]]
    for name in ('YUVTableLayout', 'YUVBlitLayout', 'YUVTableOrder'):
        start = source.index('enum ' + name + ' {')
        pieces.append(source[start:source.index('};', start) + 2])
    start = source.index('struct BLITS {')
    pieces.append(source[start:source.index('};', start) + 2])
    for line in (ROOT / 'src/bink/include/bink.h').read_text().splitlines():
        if re.match(r'#define BINK(?:COPY\w*|RBINVERT|GRAYSCALE)\s', line): pieces.append(line)
    macros = ('YUV_BLIT_ROW_BYTES', 'YUV_BLIT_ROW_BYTES_X2', 'YUV_SURFACE_MODE', 'YUV_UV_TABLES_INVERTED', 'YUV_BLIT_GRAYSCALE')
    pieces.extend(line for line in source.splitlines() if any(line.startswith('#define ' + name + '(') for name in macros))
    pieces.append(GLOBALS)
    for i in range(8):
        pieces.append('static void core%d(s32 x) { sink = (u32)x + %d; }' % (i, i))
        pieces.append('static void row%d(u32 x, u32 y) { sink = x + y + %d; }' % (i, i))
        pieces.append('static u32 col%d(u32 x, s32 y) { return x + (u32)y + %d; }' % (i, i))
    start = source.index('static void setup_scaling(')
    body = source[start:source.index('\n}', start) + 2]
    with tempfile.TemporaryDirectory(prefix='bink_yuv_scaling_') as directory:
        cfile, exe = Path(directory) / 'check.cpp', Path(directory) / 'check.exe'

        def run(function):
            cfile.write_text('\n'.join(pieces) + '\n' + function + TEST)
            subprocess.run([args.cc, '-O2', '-std=c++11', str(cfile), '-o', str(exe)], check=True)
            return subprocess.run([str(exe)], capture_output=True, text=True)

        result = run(body)
        print(result.stdout, end='')
        result.check_returncode()
        if args.self_test:
            original = 'blits->odd_step < blits->even_step'
            if original not in body: raise RuntimeError('Negative-control mutation site changed')
            result = run(body.replace(original, 'blits->odd_step > blits->even_step'))
            if result.returncode != 1 or not result.stdout.startswith('FAIL '):
                raise RuntimeError('Negative control did not detect inverted maximum: ' + result.stdout)
            print('PASS negative control (inverted maximum): ' + result.stdout.strip())


if __name__ == '__main__':
    main()
