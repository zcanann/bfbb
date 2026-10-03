#!/usr/bin/env python3
"""Check production Bink pattern rows against an independent pixel oracle.

Mask-table words are interpreted as big endian, matching the target. Checks
all row patterns, generated color pairs, padded pitches and bundle consumption
on a host compiler; it does not decode movies or run GameCube code.
"""
import argparse
import ast
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint32_t u32;
typedef struct {u8* cur_ptr;} READBUNDLE;
#define PTR4
#define BINK_PATTERN_COLOR_0 0
#define BINK_PATTERN_COLOR_1 1
#define BINK_PATTERN_COLOR_COUNT 2
#define BINK_BYTE_BITS 8
#define BINK_BUNDLE_MIN_WORD_BITS 16
#define HUFF4_SYMBOL_MASK 15
#define HUFF4_NIBBLE_BITS 4
#define BINK_BLOCK_ROW_0 0
#define BINK_BLOCK_ROW_WORD_0 0
#define BINK_BLOCK_ROW_WORD_1 1
#define BINK_BLOCK_ROW_WORD(ptr,pitch,row,word) (*(u32*)((ptr)+(pitch)*(row)+(word)*4))
'''
TEST = r'''
int main(void) {
    u32 pitches[]={8,16,40,64}, cases=0;
    for (u32 pi=0; pi<4; ++pi)
    for (u32 pattern=0; pattern<256; ++pattern)
    for (u32 color=0; color<256; ++color) {
        u32 actual[136], expected[136], pitch=pitches[pi];
        u8 colors[2]={(u8)color,(u8)(color^0xa7)}, patterns[8];
        memset(actual,0x5a,sizeof(actual));
        memset(expected,0x5a,sizeof(expected));
        for (u32 row=0; row<8; ++row) {
            patterns[row]=(u8)(pattern+row*31);
            for (u32 word=0; word<2; ++word) {
                u32 value=0;
                for (u32 pixel=0; pixel<4; ++pixel) {
                    u32 bit=(patterns[row]>>(word*4+pixel))&1;
                    value=(value<<8)|colors[bit];
                }
                expected[4+row*(pitch/4)+word]=value;
            }
        }
        READBUNDLE cb={colors}, pb={patterns};
        expand_pattern_block((u8*)(actual+4),pitch,&cb,&pb);
        if (memcmp(actual,expected,sizeof(actual)) || cb.cur_ptr!=colors+2 || pb.cur_ptr!=patterns+8) {
            printf("FAIL pitch=%u pattern=%u color=%u\n",pitch,pattern,color);
            return 1;
        }
        ++cases;
    }
    printf("PASS %u pattern cases: big-endian pixel words, padding, guards and bundle consumption\n",cases);
    return 0;
}
'''

def production_code():
    source=(ROOT/'src/bink/src/sdk/decode/expand.c').read_text()
    start=source.index('static inline void expand_pattern_row(')
    end=source.index('\nstatic inline void expand_pattern_block_pixels(',start)
    tables=''
    for name in ('mask1','mask2'):
        literal=re.search(r'static const u8 '+name+r'\[[^]]+\]\s*=\s*("[^\n]+?");',source)[1]
        data=ast.literal_eval('b'+literal)
        assert len(data)==64
        words=[int.from_bytes(data[i:i+4],'big') for i in range(0,len(data),4)]
        tables+='static const u32 '+name+'[]={'+','.join(hex(word) for word in words)+'};\n'
    return HEADER+tables+source[start:end]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',default='clang')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    code=production_code()
    with tempfile.TemporaryDirectory(prefix='bink_pattern_') as directory:
        path=Path(directory);cfile=path/'check.c';exe=path/'check.exe'
        def run(body):
            cfile.write_text(body+TEST)
            subprocess.run([args.cc,'-std=c99','-O2',str(cfile),'-o',str(exe)],check=True)
            return subprocess.run([str(exe)],text=True,capture_output=True)
        result=run(code)
        print(result.stdout,end='')
        if result.returncode: raise SystemExit(result.returncode)
        if args.self_test:
            for label,broken in [('row advance',code.replace('dest += pitch;','dest += 0;',1)),
                                 ('mask choice',code.replace('*)mask1','*)mask2'))]:
                assert broken!=code
                if run(broken).returncode==0: raise SystemExit('Negative control incorrectly passed: '+label)
                print('PASS negative control: '+label)

if __name__=='__main__': main()
