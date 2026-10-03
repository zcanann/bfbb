#!/usr/bin/env python3
"""Check production masked YUV dispatch with an independent rectangle oracle.

Compiles setup_scaling, the masked row/block helpers, and YUV_blit_mask from
--source. Mock core callbacks record their entry context and advance four source
pixels per count; zoom callbacks represent one top-level EVEN invocation, not
individual duplicated pixel writes. This tests cursor geometry, mask coverage,
alpha, pitch, context restoration, and edge-fallback arguments, not RGB output,
big-endian ABI behavior, or playback. Complete rectangles include aligned source
origins; partial rectangles use source origin zero. The generic edge renderer is
recorded rather than executed. --self-test requires targeted legacy faults to fail.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from check_bink_yuv_scaling import HEADER

ROOT = Path(__file__).resolve().parents[1]
GLOBALS = r"""
#include <vector>
#include <cstdlib>
#include <cstddef>
RGBContext S;
RGBYUVTables YUVTables, origYUVTables;
static YUVTableOrder whichyuv;
static u32 testing[2], alignshift, align, alignm1;
static CoreBlitFn EVEN, ODD, EVENx, ODDx;
static RowBlitFn dounalignedrow;
static ColBlitFn dounalignedcol;
static u32 pixel_bytes, horizontal_scale, fallback_calls;
static u8 source_pixels[131072], alpha_pixels[131072], destination[1048576], dirty[4096];
struct Trace { u32 count; ptrdiff_t d0,d1,y0,y1,a0,a1,u,v; s32 pitch; };
struct Edge { u32 dx,dy,dp,sx,sy,w,h,sp,sh,flags; };
static std::vector<Trace> actual, expected;
static std::vector<Edge> edges, expected_edges;
static void core(s32 count) {
    Trace t = {(u32)count, S.dest0-destination, S.dest1-destination,
        (u8*)S.y0-source_pixels, (u8*)S.y1-source_pixels,
        (u8*)S.a0-alpha_pixels, (u8*)S.a1-alpha_pixels,
        (u8*)S.u-source_pixels, (u8*)S.v-source_pixels, S.pitch};
    actual.push_back(t);
    // Core kernels may overwrite scratch color fields; block restoration must preserve them.
    S.r=101; S.gb=-55; S.b=91;
    if (actual.size()>4096 || count<0 || count>16) { puts("FAIL runaway callback"); exit(1); }
    const u32 pixels = (u32)count * 4;
    S.dest0 += pixels*pixel_bytes*horizontal_scale;
    S.dest1 += pixels*pixel_bytes*horizontal_scale;
    S.y0 = (u32*)((u8*)S.y0+pixels); S.y1 = (u32*)((u8*)S.y1+pixels);
    S.a0 = (u32*)((u8*)S.a0+pixels); S.a1 = (u32*)((u8*)S.a1+pixels);
    S.u = (u16*)((u8*)S.u+pixels/2); S.v = (u16*)((u8*)S.v+pixels/2);
}
static void zoom2heven(s32 n) { core(n); }
static void zoom2hodd(s32 n) { core(n); }
static void row(u32,u32) { puts("FAIL unexpected unaligned row callback"); exit(1); }
static u32 col(u32,s32) { puts("FAIL unexpected unaligned column callback"); exit(1); }
static void checkzoombufs(u32) {}
static u32 mult64anddiv(u32 a,u32 b,u32 c) { return (u32)((uint64_t)a*b/c); }
static void YUV_blit(void* dst,u32 dx,u32 dy,u32 dp,void* src,u32 sx,u32 sy,
                     u32 w,u32 h,u32 sp,u32 sh,u32 flags,void* alpha,BLITS* blits) {
    if(dst!=destination || src!=source_pixels || alpha!=alpha_pixels || blits->bytes_per_pixel!=pixel_bytes) {
        puts("FAIL fallback base pointer"); exit(1);
    }
    Edge e={dx,dy,dp,sx,sy,w,h,sp,sh,flags}; edges.push_back(e); ++fallback_calls;
}
"""
TEST = r"""
static bool marked(u32 pattern,u32 x,u32 y) {
    if(pattern==0) return false;
    if(pattern==1) return !(x&1);
    if(pattern==2) return x&1;
    if(pattern==3) return true;
    return ((x+2*y)%3)!=1;
}
static bool same(const Trace& a,const Trace& b) {
    return a.count==b.count && a.d0==b.d0 && a.d1==b.d1 && a.y0==b.y0 && a.y1==b.y1 &&
        a.a0==b.a0 && a.a1==b.a1 && a.u==b.u && a.v==b.v && a.pitch==b.pitch;
}
static bool same(const Edge& a,const Edge& b) {
    return a.dx==b.dx && a.dy==b.dy && a.dp==b.dp && a.sx==b.sx && a.sy==b.sy &&
        a.w==b.w && a.h==b.h && a.sp==b.sp && a.sh==b.sh && a.flags==b.flags;
}
static void show(const Trace& t) {
    printf(" count=%u dest=%td,%td y=%td,%td alpha=%td,%td uv=%td,%td pitch=%d\n",
        t.count,t.d0,t.d1,t.y0,t.y1,t.a0,t.a1,t.u,t.v,t.pitch);
}
static bool check_case(u32 mode,u32 bpp,u32 gray,u32 invert,u32 pattern,u32 width,u32 height,u32 origin) {
    const u32 source_step=mode==6?2:1;
    const u32 xscale=(mode==3 || mode==4 || mode==5)?2:1;
    const u32 yscale=(mode==0 || mode==3)?1:2;
    const u32 context_pitch_scale=(mode==2 || mode==5 || mode==6)?2:1;
    const u32 dest_pitch=2048, input_pitch=128, input_height=256, mask_pitch=8;
    const u32 srcx=origin, srcy=origin*source_step, destx=7, desty=3;
    const u32 input_rows=height*source_step, flags=(mode<<28)|(gray<<17)|(invert<<16);
    const u32 full_width=width/16*16, full_height=height/16*16;
    const u32 effective_pitch=input_pitch*source_step;
    const u32 chroma_pitch=effective_pitch/2;
    const u32 first_chroma=input_pitch*input_height;
    const u32 second_chroma=first_chroma+chroma_pitch*(input_height/source_step/2);
    const u32 ubase=invert?first_chroma:second_chroma, vbase=invert?second_chroma:first_chroma;
    const u32 dest_base=desty*dest_pitch+destx*bpp;
    BLITS blits={}; blits.bytes_per_pixel=bpp;
    blits.even=blits.odd=blits.masked=blits.even_x2=blits.odd_x2=blits.masked_x2=core;
    blits.even_step=blits.odd_step=blits.masked_step=2;
    blits.even_x2_step=blits.odd_x2_step=blits.masked_x2_step=2;
    blits.row=blits.rowm=blits.row2w=blits.rowm2w=blits.row2h=blits.rowm2h=blits.row2wh=blits.rowm2wh=row;
    blits.col=blits.colm=blits.col2w=blits.colm2w=blits.col2h=blits.colm2h=blits.col2wh=blits.colm2wh=col;
    memset(&S,0,sizeof(S)); S.r=17; S.gb=-23; S.b=39; memset(S.pad,0x5a,sizeof(S.pad));
    pixel_bytes=bpp; horizontal_scale=xscale; fallback_calls=0;
    actual.clear(); expected.clear(); edges.clear(); expected_edges.clear(); memset(dirty,0,sizeof(dirty));
    for(u32 by=0;by<full_height/16;++by)
    for(u32 bx=0;bx<full_width/16;++bx) {
        const u32 physical_mask_y=(origin/16+by)*source_step;
        // Alternate which interlaced half is dirty: both mask rows must participate.
        dirty[(physical_mask_y+(source_step==2?((bx+by)&1):0))*mask_pitch+origin/16+bx]=
            marked(pattern,bx,by)?(u8)(1+bx+by):0;
    }
    // Enumerate rectangular regions from mask cells, independent of pointer deltas.
    for(u32 by=0;by<full_height/16;++by)
    for(u32 bx=0;bx<full_width/16;bx+=2) {
        const bool left=marked(pattern,bx,by);
        const bool right=bx+1<full_width/16 && marked(pattern,bx+1,by);
        if(!left && !right) continue;
        const u32 column=(bx+(left?0:1))*16;
        const u32 pixels=(left&&right)?32:16;
        for(u32 pair=0;pair<8;++pair) {
            const u32 y=by*16+pair*2;
            Trace t; t.count=pixels/4;
            t.d0=dest_base+y*dest_pitch*yscale+column*bpp*xscale;
            t.d1=t.d0+dest_pitch*yscale;
            t.y0=(origin+y)*effective_pitch+srcx+column; t.y1=t.y0+effective_pitch;
            t.a0=t.y0; t.a1=t.y1;
            t.u=ubase+(origin+y)/2*chroma_pitch+(srcx+column)/2;
            t.v=vbase+(origin+y)/2*chroma_pitch+(srcx+column)/2;
            t.pitch=dest_pitch*context_pitch_scale; expected.push_back(t);
        }
    }
    if(width%16) {
        Edge e={destx+full_width*xscale,desty,dest_pitch,full_width,0,width-full_width,input_rows,
                input_pitch,input_height,flags}; expected_edges.push_back(e);
    }
    if(height%16) {
        // Convert the completed effective rows back to physical source rows.
        const u32 physical_rows=full_height*source_step;
        Edge e={destx,desty+full_height*yscale,dest_pitch,0,physical_rows,width,input_rows-physical_rows,
                input_pitch,input_height,flags}; expected_edges.push_back(e);
    }
    YUV_blit_mask(destination,destx,desty,dest_pitch,dirty,mask_pitch,source_pixels,srcx,srcy,
                  width,input_rows,input_pitch,input_height,flags,alpha_pixels,&blits);
    const auto fail=[&](const char* reason) {
        printf("FAIL %s mode=%u bpp=%u gray=%u invert=%u pattern=%u width=%u effective_height=%u origin=%u\n",
               reason,mode,bpp,gray,invert,pattern,width,height,origin); return false;
    };
    if(actual.size()!=expected.size()) return fail("callback count");
    for(size_t i=0;i<actual.size();++i) if(!same(actual[i],expected[i])) {
        fail("callback geometry"); printf("callback %zu actual",i);show(actual[i]);printf("expected");show(expected[i]);return false;
    }
    if(edges.size()!=expected_edges.size()) return fail("fallback count");
    for(size_t i=0;i<edges.size();++i) if(!same(edges[i],expected_edges[i])) {
        fail("fallback arguments"); const Edge& a=edges[i]; const Edge& e=expected_edges[i];
        printf("edge %zu actual dest=%u,%u source=%u,%u extent=%u,%u; expected dest=%u,%u source=%u,%u extent=%u,%u\n",
               i,a.dx,a.dy,a.sx,a.sy,a.w,a.h,e.dx,e.dy,e.sx,e.sy,e.w,e.h);return false;
    }
    const ptrdiff_t next_y=(origin+full_height)*effective_pitch+srcx;
    if(S.dest0!=destination+dest_base+full_height*dest_pitch*yscale ||
       S.dest1!=S.dest0+dest_pitch*yscale || (u8*)S.y0!=source_pixels+next_y ||
       (u8*)S.y1!=source_pixels+next_y+effective_pitch || (u8*)S.a0!=alpha_pixels+next_y ||
       (u8*)S.a1!=alpha_pixels+next_y+effective_pitch ||
       (u8*)S.u!=source_pixels+ubase+(origin+full_height)/2*chroma_pitch+srcx/2 ||
       (u8*)S.v!=source_pixels+vbase+(origin+full_height)/2*chroma_pitch+srcx/2 ||
       S.pitch!=(s32)(dest_pitch*context_pitch_scale) || S.base!=destination ||
       S.r!=17 || S.gb!=-23 || S.b!=39) return fail("final context");
    for(u32 i=0;i<sizeof(S.pad);++i) if(S.pad[i]!=0x5a) return fail("reserved context");
    return true;
}
int main() {
    u32 cases=0;
    for(u32 mode=0;mode<7;++mode) for(u32 bpp=2;bpp<=4;++bpp)
    for(u32 gray=0;gray<2;++gray) for(u32 invert=0;invert<2;++invert)
    for(u32 pattern=0;pattern<5;++pattern) {
        for(u32 width=16;width<=64;width+=16) for(u32 height=16;height<=48;height+=16)
        for(u32 origin=0;origin<=16;origin+=16) {
            if(!check_case(mode,bpp,gray,invert,pattern,width,height,origin)) return 1; ++cases;
        }
        const u32 widths[]={17,31,48,49}, heights[]={17,31,32,33};
        for(u32 wi=0;wi<4;++wi) for(u32 hi=0;hi<4;++hi) {
            if(!check_case(mode,bpp,gray,invert,pattern,widths[wi],heights[hi],0)) return 1; ++cases;
        }
    }
    printf("PASS %u masked YUV cases: callback geometry, alpha, masks, pitches, context, edge fallbacks\n",cases);
}
"""


def function(source, name):
    match = re.search(r'^static (?:inline )?void ' + re.escape(name) + r'\(', source, re.M)
    if not match:
        raise ValueError('Missing production function: ' + name)
    return source[match.start():source.index('\n}', match.start()) + 2]


def harness(source):
    header = (ROOT / 'src/bink/src/sdk/decode/ngc/ngcrgb.h').read_text().replace('#include "bink.h"', '')
    pieces = [HEADER, header, source[source.index('typedef void (*CoreBlitFn)'):source.index('enum YUVTableLayout')]]
    for name in ('YUVTableLayout', 'YUVBlitLayout', 'YUVTableOrder', 'YUVMaskLayout', 'YUVChromaLayout'):
        start = source.index('enum ' + name + ' {')
        pieces.append(source[start:source.index('};', start) + 2])
    start = source.index('struct BLITS {')
    pieces.append(source[start:source.index('};', start) + 2])
    for line in (ROOT / 'src/bink/include/bink.h').read_text().splitlines():
        if re.match(r'#define BINK(?:COPY\w*|RBINVERT|GRAYSCALE)\s', line):
            pieces.append(line)
    names = ('YUV_BLIT_ROW_BYTES', 'YUV_BLIT_ROW_BYTES_X2', 'YUV_BLIT_SCALED_PIXEL_BYTES',
             'YUV_BLIT_SCALED_ROW_BYTES', 'YUV_SURFACE_MODE', 'YUV_UV_TABLES_INVERTED', 'YUV_BLIT_GRAYSCALE')
    lines = iter(source.splitlines())
    for line in lines:
        if any(line.startswith('#define ' + name + '(') for name in names):
            while line.endswith('\\'):
                line = line[:-1] + next(lines)
            pieces.append(line)
    pieces.append(GLOBALS)
    pieces.append(function(source, 'setup_scaling'))
    if re.search(r'^static inline void blit_mask_rows\(', source, re.M):
        pieces.append(function(source, 'blit_mask_rows'))
    pieces.append(function(source, 'blit_mask_block'))
    pieces.append(function(source, 'YUV_blit_mask'))
    return '\n'.join(pieces) + TEST


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError('Negative-control site changed: ' + old)
    return text.replace(old, new, 1)


def negative_controls(source):
    setup = 'setup_scaling(flags, &pitch16, YUV_MASK_BLOCK_PIXELS, srch, blits, &pitch_delta16);'
    changed = replace_once(source, '    pitch32 = pitch16;\n', '')
    yield 'compounded setup pitch', replace_once(changed, setup,
        setup + '\n    pitch32 = pitch16;')
    yield 'helper uses unscaled context pitch', replace_once(source,
        'S.dest1 = S.dest0 + row_pitch;', 'S.dest1 = S.dest0 + S.pitch;')
    start = source.index('case YUV_MASK_RIGHT_HALF_BIT: {')
    end = source.index('case YUV_MASK_BOTH_HALVES:', start)
    right = source[start:end]
    old = 'YUV_BLIT_SCALED_ROW_BYTES(YUV_MASK_BLOCK_PIXELS, blits, xscale)'
    if right.count(old) != 2: raise RuntimeError('Right-half destination mutation sites changed')
    yield 'unscaled right-half destination', source[:start]+right.replace(old,'YUV_MASK_BLOCK_PIXELS')+source[end:]
    alpha_lines = re.findall(r'^.*S\.a[01] = .*YUV_MASK_BLOCK_PIXELS.*\n', right, re.M)
    if len(alpha_lines) != 2: raise RuntimeError('Right-half alpha mutation sites changed')
    changed = right
    for line in alpha_lines: changed = changed.replace(line, '')
    yield 'missing right-half alpha offset', source[:start]+changed+source[end:]
    old = 'luma_row_skip = srcpitch - srcw + (srcw & YUV_MASK_BLOCK_MASK) +'
    yield 'fixed-width luma row skip', replace_once(source,old,'luma_row_skip = srcpitch - YUV_MASK_BLOCK_PIXELS +')
    old = 'chroma_row_skip = chroma_pitch - (srcw >> YUV_CHROMA_SHIFT) +\n                      ((srcw & YUV_MASK_BLOCK_MASK) >> YUV_CHROMA_SHIFT) + chroma_pitch * 7;'
    yield 'fixed-width chroma row skip', replace_once(source,old,'chroma_row_skip = chroma_pitch - YUV_CHROMA_BLOCK_BYTES + chroma_pitch * 7;')
    # Mutate only the second horizontal block cursor update, not the right-half adjustment.
    old = 'S.a0 = (u32 PTR4*)((u8 PTR4*)S.a0 + YUV_MASK_BLOCK_PAIR_PIXELS);'
    yield 'missing horizontal alpha advance', replace_once(source,old,'/* legacy omitted alpha cursor */')
    yield 'unconverted bottom source rows', replace_once(source,
        'full_h = mult64anddiv(srch & ~YUV_MASK_BLOCK_MASK, old_srch, srch);',
        'full_h = srch & ~YUV_MASK_BLOCK_MASK;')
    yield 'bottom destination uses frame height', replace_once(source,
        'pitch32 * old_srcpitch, destpitch * srcpitch',
        'pitch32 * old_srcheight, destpitch * srcpitch')
    yield 'bottom remaining height uses adjusted rows', replace_once(source,
        'old_srcw, old_srch - full_h, old_srcpitch',
        'old_srcw, srch - full_h, old_srcpitch')



def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default='clang')
    parser.add_argument('--source', type=Path, default=ROOT / 'src/bink/src/sdk/decode/yuv.cpp')
    parser.add_argument('--self-test', action='store_true')
    args = parser.parse_args()
    source = args.source.read_text()
    with tempfile.TemporaryDirectory(prefix='bink_yuv_mask_') as directory:
        cfile, exe = Path(directory)/'check.cpp', Path(directory)/'check.exe'
        def run(body):
            cfile.write_text(harness(body))
            subprocess.run([args.cc,'-O2','-std=c++17',str(cfile),'-o',str(exe)],check=True)
            return subprocess.run([str(exe)],capture_output=True,text=True)
        result = run(source)
        print(result.stdout,end='')
        result.check_returncode()
        if args.self_test:
            for name, mutated in negative_controls(source):
                result = run(mutated)
                if result.returncode!=1 or not result.stdout.startswith('FAIL '):
                    raise RuntimeError('Negative control failed to detect '+name+': '+result.stdout)
                print('PASS negative control ('+name+'): '+result.stdout.splitlines()[0])


if __name__ == '__main__':
    main()
