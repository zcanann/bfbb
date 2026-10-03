#!/usr/bin/env python3
"""Check production YUV_init tables against independent integer formulas.

Tests cold initialization, poisoned same-layout/invalid cache returns, and every
ordered warm format pair for flags -2, -1, and 0..16. All table entries, unused
trailing slots, and surrounding canaries are compared. Surface layouts and
RGBshift indices are literal oracle data, not production enum-derived values.
The alpha table intentionally preserves retail's (i & 0x1ffffe0) << 7 rule.
This is a host table/state test, not a big-endian ABI or playback test.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

from check_bink_yuv_columns import HEADER

ROOT = Path(__file__).resolve().parents[1]
ARRAYS = {
    'ytable': ('luma', 'u32', 'RGB_LUMA_TABLE_SIZE'),
    'ytable_x4': ('luma4', 'u32', 'RGB_LUMA_TABLE_SIZE'),
    'clamp_ytable': ('lookup', 'u32*', 'RGB_LUMA_TABLE_SIZE'),
    'clamptable': ('clamp', 'u32', 'RGB_CLAMP_TABLE_SIZE'),
    'clamp_a4': ('alpha4', 'u32', 'RGB_LUMA_TABLE_SIZE'),
    'mono16': ('mono', 'u32', 'RGB_MONO_TABLE_SIZE'),
    'mono16x2': ('mono2', 'u32', 'RGB_MONO_TABLE_SIZE'),
    'mono32': ('mono_rgb', 'u32', 'RGB_MONO_TABLE_SIZE'),
    'RGBshift': ('shifts', 'u32', 'RGB_SHIFT_TABLE_SIZE'),
}
for suffix, field in (('r', 'red'), ('g', 'green'), ('b', 'blue'),
                      ('rh', 'red_hi'), ('gh', 'green_hi'), ('bh', 'blue_hi'),
                      ('rr', 'red_dup'), ('gg', 'green_dup'), ('bb', 'blue_dup')):
    ARRAYS['clamp_' + suffix] = (field, 'u32', 'RGB_CLAMP_TABLE_SIZE')

TEST = r"""
static u32 expected_done, expected_layout, calls;
static void reset(u32 seed) {
    // Poison values and guards alike. The oracle changes only specified entries.
    u8* bytes=(u8*)&actual;
    for(size_t i=0;i<sizeof(actual);++i) { seed=seed*1664525u+1013904223u; bytes[i]=(u8)(seed>>24); }
    memcpy(&expected,&actual,sizeof(actual));
}
static s32 trunc15(s32 value) {
    // Signed integer division truncates toward zero, independently of shift/bias code.
    return (s32)((int64_t)value/32768);
}
static void oracle(s32 flags) {
    if(!expected_done) {
        for(u32 i=0;i<256;++i) {
            const u32 y=i<=16?0:i>=235?254:(i-16)*0x950au/32768;
            expected.luma.values[i]=y; expected.luma4.values[i]=y*4;
            const s32 chroma=(s32)i-128;
            RGBYUVTables& uv=expected.uv.values[0];
            uv.v_to_gb[i]=-trunc15(chroma*0x680f);
            uv.u_to_gb[i]=-trunc15(chroma*0x3225);
            uv.v_to_r[i]=trunc15(chroma*0xcc4b);
            uv.u_to_b[i]=trunc15(chroma*0x10235);
            expected.clamp.values[i]=0;
            expected.clamp.values[256+i]=i;
            expected.clamp.values[512+i]=255;
            expected.lookup.values[i]=&actual.clamp.values[256+y];
        }
        expected.original.values[0]=expected.uv.values[0];
        expected_done=1;
    }
    if(expected_layout==(u32)flags || flags==-1) return;
    expected_layout=(u32)flags;
    // Format 7=4444, 8/9=5551/555, 10=565, 11=655, 12=664.
    static const u32 layouts[6][4]={{4,4,4,8},{5,5,5,10},{5,5,5,10},
                                   {5,6,5,11},{6,5,5,11},{6,6,4,12}};
    const u32 zero[4]={0,0,0,0};
    const u32* layout=flags>=7 && flags<=12?layouts[flags-7]:zero;
    const u32 rb=layout[0], gb=layout[1], bb=layout[2], bs=layout[3];
    const u32 rd=8-rb, gd=8-gb, bd=8-bb;
    const u32 shifts[12]={0,0,rb,0,bs,0,rd,0,gd,0,bd,0};
    memcpy(expected.shifts.values,shifts,sizeof(shifts));
    u32* channels[]={expected.red.values,expected.green.values,expected.blue.values};
    u32* high[]={expected.red_hi.values,expected.green_hi.values,expected.blue_hi.values};
    u32* doubled[]={expected.red_dup.values,expected.green_dup.values,expected.blue_dup.values};
    const u32 bits[]={rb,gb,bb}, offsets[]={0,rb,bs};
    for(u32 c=0;c<3;++c) for(u32 j=0;j<768;++j) {
        const u32 channel=j<256?0:j>=512?255:j-256;
        const u32 packed=(channel/(1u<<(8-bits[c])))<<offsets[c];
        channels[c][j]=packed; high[c][j]=packed*65536u; doubled[c][j]=packed*65537u;
    }
    for(u32 i=0;i<256;++i) {
        const u32 gray=i<=16?0:i>=235?255:(i-16)*255/219;
        const u32 packed=(gray/(1u<<rd))|((gray/(1u<<gd))<<rb)|((gray/(1u<<bd))<<bs);
        expected.mono.values[i]=packed;
        expected.mono2.values[i]=packed*65537u;
        expected.mono_rgb.values[i]=gray*0x010101u;
        // Retail alpha quantization is deliberately not conventional four-bit alpha.
        expected.alpha4.values[i]=(i&0x1ffffe0u)<<7;
    }
}
static bool check(s32 flags,const char* phase,s32 previous) {
    oracle(flags); YUV_init(flags); ++calls;
    if(donetables!=expected_done || rgb_layout!=expected_layout) {
        printf("FAIL cache state phase=%s previous=%d flags=%d done=%u/%u layout=%08x/%08x\n",
               phase,previous,flags,donetables,expected_done,rgb_layout,expected_layout); return false;
    }
    if(memcmp(&actual,&expected,sizeof(actual))) {
        const u8 *a=(const u8*)&actual,*e=(const u8*)&expected;
        size_t offset=0;while(offset<sizeof(actual)&&a[offset]==e[offset])++offset;
        printf("FAIL table state phase=%s previous=%d flags=%d byte=%zu actual=%02x expected=%02x\n",
               phase,previous,flags,offset,a[offset],e[offset]);return false;
    }
    return true;
}
int main() {
    static_assert(RGB_LUMA_TABLE_SIZE==260 && RGB_CLAMP_TABLE_SIZE==772 &&
                  RGB_MONO_TABLE_SIZE==256 && RGB_SHIFT_TABLE_SIZE==12,"Unexpected production table size");
    for(s32 initial=-2;initial<=16;++initial) {
        reset(0x59555600u+(u32)initial); donetables=expected_done=0;
        rgb_layout=expected_layout=0xffffffffu;
        if(!check(initial,"cold",-1))return 1;
        // Mutated tables distinguish a real early return from an idempotent rebuild.
        reset(0xabc00000u+(u32)initial);
        if(!check(initial,"same layout",initial))return 1;
        if(!check(-1,"invalid layout",initial))return 1;
        State cached; memcpy(&cached,&actual,sizeof(cached));
        const u32 layout=rgb_layout;
        for(s32 next=-2;next<=16;++next) {
            memcpy(&actual,&cached,sizeof(actual)); memcpy(&expected,&cached,sizeof(expected));
            donetables=expected_done=1; rgb_layout=expected_layout=layout;
            if(!check(next,"warm format pair",initial))return 1;
        }
    }
    printf("PASS %u YUV initialization calls: 19 cold, 19 same-layout, 19 invalid, 361 warm format pairs; all tables and guards\n",calls);
    return 0;
}
"""


def function(source, name):
    match = re.search(r'^(?:static inline s32|extern "C" void) ' + re.escape(name) + r'\(', source, re.M)
    if not match:
        raise ValueError('Missing production function: ' + name)
    return source[match.start():source.index('\n}', match.start()) + 2]


def harness(source, header):
    pieces = [HEADER, header.replace('#include "bink.h"', '')]
    for name in ('YUVTableLayout', 'BINKSurfaceLayoutState', 'YUVRoundConstants',
                 'YUVColorRange', 'YUVRgbCoefficients', 'RGBPackConstants', 'RGBSurfaceLayout'):
        start = source.index('enum ' + name + ' {')
        pieces.append(source[start:source.index('};', start)+2])
    for line in (ROOT/'src/bink/include/bink.h').read_text().splitlines():
        if re.match(r'#define BINKSURFACE\w*\s',line): pieces.append(line)
    pieces.append('template<class T, size_t N> struct Guarded { u32 before[4]; T values[N]; u32 after[4]; };\nstruct State {')
    for field, kind, size in ARRAYS.values():
        pieces.append('Guarded<%s,%s> %s;' % (kind,size,field))
    pieces.append('Guarded<RGBYUVTables,1> uv, original; Guarded<RGBContext,1> context; };\nstatic State actual, expected;\nstatic u32 donetables, rgb_layout;')
    for name, (field, _, _) in ARRAYS.items(): pieces.append('#define %s (actual.%s.values)' % (name,field))
    pieces.extend(('#define YUVTables (actual.uv.values[0])', '#define origYUVTables (actual.original.values[0])',
                   '#define S (actual.context.values[0])'))
    pieces.append(function(source,'yuv_round15'))
    pieces.append(function(source,'YUV_init'))
    pieces.append(TEST)
    return '\n'.join(pieces)


def replace_once(text, old, new):
    if text.count(old)!=1: raise RuntimeError('Negative-control site changed: '+old)
    return text.replace(old,new,1)


def controls(source):
    yield 'legacy green/blue shift slots', replace_once(replace_once(source,
        'RGBshift[RGB_SHIFT_GREEN_DOWN]', 'RGBshift[7]'),
        'RGBshift[RGB_SHIFT_BLUE_DOWN]', 'RGBshift[8]')
    yield 'luma coefficient', replace_once(source,'YUV_COEFF_Y_TO_RGB = 0x950a','YUV_COEFF_Y_TO_RGB = 0x9000')
    yield 'luma upper saturation', replace_once(source,'YUV_LUMA_MAX = 0xfe','YUV_LUMA_MAX = 0xff')
    yield 'signed chroma truncation', replace_once(source,'value += YUV_ROUND_BIAS;','value += 0;')
    yield 'conventional alpha mask', replace_once(source,'RGB_A4_SOURCE_MASK = 0x1ffffe0','RGB_A4_SOURCE_MASK = 0xf0')
    yield 'same-layout cache guard', replace_once(source,'if (rgb_layout == flags)', 'if (false)')
    yield 'one-time table cache guard', replace_once(source,'if (donetables == 0)', 'if (true)')
    yield 'invalid-layout cache guard', replace_once(source,'if (flags == BINKSURFACE_INVALID)', 'if (false)')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,default=ROOT/'src/bink/src/sdk/decode/yuv.cpp')
    parser.add_argument('--header',type=Path,default=ROOT/'src/bink/src/sdk/decode/ngc/ngcrgb.h')
    parser.add_argument('--cc',default='clang')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args();source=args.source.read_text();header=args.header.read_text()
    with tempfile.TemporaryDirectory(prefix='bink_yuv_init_') as directory:
        cfile,exe=Path(directory)/'check.cpp',Path(directory)/'check.exe'
        def run(body):
            cfile.write_text(harness(body,header))
            subprocess.run([args.cc,'-O2','-std=c++11',str(cfile),'-o',str(exe)],check=True)
            return subprocess.run([str(exe)],capture_output=True,text=True)
        result=run(source);print(result.stdout,end='');result.check_returncode()
        if args.self_test:
            for name,mutated in controls(source):
                result=run(mutated)
                if result.returncode!=1 or not result.stdout.startswith('FAIL '):
                    raise RuntimeError('Negative control did not detect '+name+': '+result.stdout)
                print('PASS negative control ('+name+'): '+result.stdout.splitlines()[0])


if __name__=='__main__':
    main()
