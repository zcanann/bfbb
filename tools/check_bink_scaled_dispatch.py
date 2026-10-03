#!/usr/bin/env python3
"""Check Bink scaled-block dispatch against the retail control-flow contract.

Compiles production dispatch and common advancement with payload decoders
stubbed. This tests bundle consumption, work marks and cursor advancement,
not decoded pixels or movie playback.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

from check_bink_fill import ROOT, production_code as fill_code

SUPPORT = r'''
#include <stddef.h>
typedef int16_t s16;
#define BINK_DC_BYTES 2
#define BINK_BUNDLE_S16(bundle) (*(s16*)((bundle).cur_ptr))
static u32 decode_calls;
#define expand_pattern_block_pixels(...) (++decode_calls)
#define expand_pattern_block_scaled(...) (++decode_calls)
#define expand_run_block(...) (++decode_calls)
#define scale_block(...) (++decode_calls)
#define ReadBPLossless(...) (++decode_calls)
#define FastIDCT8x8d(...) (++decode_calls)
#define exp_get_bits(...) 0
struct Result {
    u32 col,work_col,blocks,subtypes,colors,calls;
    ptrdiff_t dest,prev;
};
static struct Result dispatch(u32 row,u32 plane,u32 col,u32 work_col,
                             u8* out,u8* prev,u8* work_row,
                             READBUNDLE block_types,READBUNDLE subblock_types,READBUNDLE colors) {
    u32 pitch=32;
    u8* dest=out;
    u8* old=prev;
    u8* types_begin=block_types.cur_ptr;
    u8* subtypes_begin=subblock_types.cur_ptr;
    u8* colors_begin=colors.cur_ptr;
    u8 block_type,subblock_type;
    u8 dummy_dc[4]={0};
    READBUNDLE intra_dc={dummy_dc};
    s16 dct_block[64];
    struct Result result;
    decode_calls=0;
'''
SUFFIX = r'''
    result.col=col;result.work_col=work_col;
    result.blocks=(u32)(block_types.cur_ptr-types_begin);
    result.subtypes=(u32)(subblock_types.cur_ptr-subtypes_begin);
    result.colors=(u32)(colors.cur_ptr-colors_begin);
    result.calls=decode_calls;
    result.dest=dest-out;result.prev=old-prev;
    return result;
}
int main(void) {
    u32 cases=0;
    for(u32 row=0;row<64;row+=8)
    for(u32 plane=1;plane<=2;++plane)
    for(u32 block=0;block<7;++block) {
        u8 output[128],previous[128],expected[128],work[32],expected_work[32];
        u8 types[2]={BINK_BLOCK_SCALED,BINK_BLOCK_SKIP};
        u8 subtypes[2]={BINK_BLOCK_RAW,BINK_BLOCK_FILL},colors[128]={0};
        READBUNDLE bt={types},st={subtypes},cb={colors};
        u32 col=block*8,work_col=block*plane,odd=(row/8)%2;
        memset(output,0x5a,sizeof(output));memset(previous,0x5a,sizeof(previous));
        memset(expected,0x5a,sizeof(expected));
        memset(work,0x6b,sizeof(work));memset(expected_work,0x6b,sizeof(expected_work));
        expected_work[work_col/2]=1;
        expected_work[(work_col+plane)/2]=1;
        struct Result r=dispatch(row,plane,col,work_col,output+col,previous+col,work,bt,st,cb);
        if(r.col!=col+16 || r.work_col!=work_col+2*plane || r.dest!=16 || r.prev!=16 ||
           r.blocks!=1 || r.subtypes!=(odd?0:1) || r.colors!=(odd?0:64) || r.calls!=(odd?0:1) ||
           memcmp(work,expected_work,sizeof(work)) || memcmp(output,expected,sizeof(output)) ||
           memcmp(previous,expected,sizeof(previous))) {
            printf("FAIL row=%u plane=%u block=%u advance=%u subtypes=%u colors=%u calls=%u\n",
                   row,plane,block,r.col-col,r.subtypes,r.colors,r.calls);
            return 1;
        }
        ++cases;
    }
    printf("PASS %u scaled dispatch cases: both row parities, plane scales, marks, bundles and cursors\n",cases);
    return 0;
}
'''

def production_code():
    source=(ROOT/'src/bink/src/sdk/decode/expand.c').read_text()
    enum_start=source.index('enum BINKBLOCKTYPE')
    enum_end=source.index('};',enum_start)+2
    start=source.index('            case BINK_BLOCK_SCALED:')
    end=source.index('            }\n\n            col += BINK_BLOCK_SIDE;',start)
    common_start=end+len('            }\n\n')
    common_end=source.index('        }\n\n        if (plane ==',common_start)
    read_start=source.index('            block_type = BINK_BUNDLE_U8(block_types);')
    read_end=source.index('            switch (block_type) {',read_start)
    return (fill_code()+source[enum_start:enum_end]+SUPPORT+source[read_start:read_end]+
            '    switch (block_type) {\n'+source[start:end]+'    }\n'+
            source[common_start:common_end]+SUFFIX)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',default='clang')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args();code=production_code()
    with tempfile.TemporaryDirectory(prefix='bink_dispatch_') as directory:
        path=Path(directory);cfile=path/'check.c';exe=path/'check.exe'
        def run(body):
            cfile.write_text(body)
            subprocess.run([args.cc,'-std=c99','-O2',str(cfile),'-o',str(exe)],check=True)
            return subprocess.run([str(exe)],text=True,capture_output=True)
        result=run(code);print(result.stdout,end='')
        if result.returncode: raise SystemExit(result.returncode)
        if args.self_test:
            for label,broken in [('old odd-row early exit',code.replace('case BINK_BLOCK_SCALED:',
                                  'case BINK_BLOCK_SCALED: if (BINK_BLOCK_ODD_ROW(row)) break;',1)),
                                 ('subtype consumption',code.replace('BINK_BUNDLE_ADVANCE(subblock_types, BINK_BUNDLE_BYTE_PITCH);',''))]:
                assert broken!=code
                result=run(broken)
                if result.returncode==0: raise SystemExit('Negative control incorrectly passed: '+label)
                print('PASS negative control: '+label+'; '+result.stdout.strip())

if __name__=='__main__': main()
