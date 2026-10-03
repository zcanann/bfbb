#!/usr/bin/env python3
"""Check production Bink motion copies against byte-wise pitched copies.

Exercises the three copy sites, word/double alignment branches, source data,
row padding and destination guards on the host. This is not movie playback.
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
#define PTR4
#define BINK_PLANE_WORD_BYTES sizeof(u32)
'''
TEST = r'''
int main(void) {
    u32 pitches[] = {8,16,40,64}, cases=0;
    for (u32 mode=0; mode<3; ++mode)
    for (u32 pi=0; pi<4; ++pi)
    for (u32 sa=0; sa<8; sa+=4)
    for (u32 da=0; da<8; da+=4)
    for (u32 trial=0; trial<256; ++trial) {
        union { double align; u8 bytes[640]; } src, actual, expected;
        u32 pitch=pitches[pi], output_pitch=mode==2 ? pitch : 8;
        u32 dest_offset=16+(mode==2 ? da : 0);
        for (u32 i=0; i<sizeof(src.bytes); ++i)
            src.bytes[i]=(u8)(i*37+trial*71+(i^trial));
        memset(actual.bytes,0xa5,sizeof(actual.bytes));
        memset(expected.bytes,0xa5,sizeof(expected.bytes));
        for (u32 row=0; row<8; ++row)
            for (u32 col=0; col<8; ++col)
                expected.bytes[dest_offset+row*output_pitch+col]=src.bytes[16+sa+row*pitch+col];
        if (mode==0) copy_residue(actual.bytes+dest_offset,src.bytes+16+sa,pitch);
        else if (mode==1) copy_inter(actual.bytes+dest_offset,src.bytes+16+sa,pitch);
        else copy_motion(actual.bytes+dest_offset,src.bytes+16+sa,pitch);
        if (memcmp(actual.bytes,expected.bytes,sizeof(actual.bytes))) {
            printf("FAIL mode=%u pitch=%u source_alignment=%u dest_alignment=%u trial=%u\n",mode,pitch,sa,da,trial);
            return 1;
        }
        ++cases;
    }
    printf("PASS %u motion-copy cases: pixels, alignment branches, padding and guards\n",cases);
    return 0;
}
'''

def production_code():
    source=(ROOT/'src/bink/src/sdk/decode/expand.c').read_text()
    start=source.index('typedef enum BINKBlockLayout')
    end=source.index('typedef BITSTYPE EXPBITSTYPE;',start)
    code=HEADER+source[start:end]
    for name in ('BINK_BLOCK_ROW_WORD','BINK_BLOCK_ROW_DOUBLE','BINK_COPY_MOTION_DOUBLE_ROW','BINK_COPY_MOTION_WORD_ROW'):
        # Collect continuation lines explicitly; regex greediness must not drop them.
        lines=source[source.index('#define '+name+'('):].splitlines(keepends=True)
        definition=lines[0]
        for line in lines[1:]:
            if not definition.rstrip('\n\r').endswith('\\'): break
            definition+=line
        code+=definition+'\n'
    for name in ('RESIDUE','INTER','MOTION'):
        case=source[source.index('            case BINK_BLOCK_'+name+': {'):]
        begin=case.index('                if (')
        depth=0; end=None
        for i in range(begin,len(case)):
            if case[i]=='{': depth+=1
            elif case[i]=='}':
                depth-=1
                if depth==0 and not case[i+1:].lstrip().startswith('else'):
                    end=i+1;break
        assert end is not None
        parameter='dest' if name=='MOTION' else 'motion_block'
        code+=f'\nstatic void copy_{name.lower()}(u8* {parameter}, u8* motion_source, u32 pitch) {{\n'+case[begin:end]+'\n}\n'
    return code

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',default='clang')
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args()
    code=production_code()
    with tempfile.TemporaryDirectory(prefix='bink_motion_') as directory:
        path=Path(directory); cfile=path/'check.c'; exe=path/'check.exe'
        def run(body):
            cfile.write_text(body+TEST)
            subprocess.run([args.cc,'-std=c99','-O2','-fno-strict-aliasing','-Wno-pointer-to-int-cast',str(cfile),'-o',str(exe)],check=True)
            return subprocess.run([str(exe)],text=True,capture_output=True)
        result=run(code)
        print(result.stdout,end='')
        if result.returncode: raise SystemExit(result.returncode)
        if args.self_test:
            broken=code.replace('motion_source += pitch;', 'motion_source += 0;',1)
            assert broken!=code
            result=run(broken)
            if result.returncode==0: raise SystemExit('Negative control incorrectly passed')
            print('PASS negative control: missing source-row advance detected')

if __name__=='__main__': main()
