#!/usr/bin/env python3
"""Check production normal/scaled Bink fill pixels, padding and state updates.

Uses a host C compiler and decoded color bundles; not a movie playback test.
"""
import argparse
from pathlib import Path
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
#define BINK_PLANE_WORD_BYTES sizeof(u32)
#define BINK_BYTE_BITS 8
#define BINK_WORK_BLOCK_MARKED 1
#define BINK_BUNDLE_BYTE_PITCH 1
'''
TEST = r'''
int main(void) {
    u32 pitches[]={8,16,40,64}, cases=0;
    for (u32 mode=0; mode<2; ++mode)
    for (u32 pi=0; pi<4; ++pi)
    for (u32 align=0; align<8; align+=4)
    for (u32 color=0; color<256; ++color)
    for (u32 work_col=0; work_col<8; work_col+=2) {
        union { double align; u8 bytes[1056]; } actual,expected;
        u8 colors[3]={0x39,(u8)color,0x72}, work[8],expected_work[8];
        u32 pitch=pitches[pi], offset=16+align, side=mode ? 16 : 8;
        if (pitch<side) continue;
        memset(actual.bytes,0xa5,sizeof(actual.bytes));
        memset(expected.bytes,0xa5,sizeof(expected.bytes));
        memset(work,0x55,sizeof(work));
        memset(expected_work,0x55,sizeof(expected_work));
        if (!mode) expected_work[work_col/2]=1;
        for (u32 row=0; row<side; ++row)
            for (u32 col=0; col<side; ++col)
                expected.bytes[offset+row*pitch+col]=(u8)color;
        READBUNDLE bundle={colors+1};
        u8* end=mode ? fill_scaled_block(actual.bytes+offset,pitch,bundle,work,work_col) :
                       fill_block(actual.bytes+offset,pitch,bundle,work,work_col);
        if (memcmp(actual.bytes,expected.bytes,sizeof(actual.bytes)) ||
            memcmp(work,expected_work,sizeof(work)) || end!=colors+2) {
            printf("FAIL mode=%u pitch=%u alignment=%u color=%u work_col=%u\n",mode,pitch,align,color,work_col);
            return 1;
        }
        ++cases;
    }
    printf("PASS %u normal/scaled fill cases: pixels, padding, guards, normal work marks and bundle consumption\n",cases);
    return 0;
}
'''

def production_code():
    source=(ROOT/'src/bink/src/sdk/decode/expand.c').read_text()
    start=source.index('typedef enum BINKBlockLayout')
    end=source.index('typedef BITSTYPE EXPBITSTYPE;',start)
    code=HEADER+source[start:end]
    for name in ('BINK_BLOCK_ROW_WORD','BINK_FILL_BLOCK_WORD_ROW','BINK_FILL_WORD',
                 'BINK_BUNDLE_U8','BINK_BUNDLE_ADVANCE','BINK_MARK_WORK_BLOCK'):
        lines=source[source.index('#define '+name+'('):].splitlines(keepends=True)
        definition=lines[0]
        for line in lines[1:]:
            if not definition.rstrip('\n\r').endswith('\\'): break
            definition+=line
        code+=definition+'\n'
    search_from=0
    for function in ('fill_block','fill_scaled_block'):
        start=source.index('case BINK_BLOCK_FILL: {',search_from)
        start=source.index('{',start)+1
        end=source.index('break;',start)
        code+='\nstatic u8* '+function+'(u8* dest, u32 pitch, READBUNDLE colors, u8* work_row, u32 work_col) {\n'
        code+=source[start:end]+'\n    return colors.cur_ptr;\n}\n'
        search_from=end
    return code

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',default='clang')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args();code=production_code()
    with tempfile.TemporaryDirectory(prefix='bink_fill_') as directory:
        path=Path(directory);cfile=path/'check.c';exe=path/'check.exe'
        def run(body):
            cfile.write_text(body+TEST)
            subprocess.run([args.cc,'-std=c99','-O2',str(cfile),'-o',str(exe)],check=True)
            return subprocess.run([str(exe)],text=True,capture_output=True)
        result=run(code);print(result.stdout,end='')
        if result.returncode: raise SystemExit(result.returncode)
        if args.self_test:
            for label,broken in [('row advance',code.replace('copy_dest += pitch;','copy_dest += 0;',1)),
                                 ('scaled row advance',code.replace('fill_row += pitch;','fill_row += 0;',1)),
                                 ('bundle consumption',code.replace('BINK_BUNDLE_ADVANCE(colors, BINK_BUNDLE_BYTE_PITCH);',''))]:
                assert broken!=code
                if run(broken).returncode==0: raise SystemExit('Negative control incorrectly passed: '+label)
                print('PASS negative control: '+label)

if __name__=='__main__': main()
