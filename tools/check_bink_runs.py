#!/usr/bin/env python3
"""Check production run-block expansion with synthetic decoded bundles.

Tests pixel placement and bundle consumption, not Huffman/bitstream decoding.
Requires clang or another host C compiler supplied with --cc.
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
typedef uint32_t u32;
typedef int32_t s32;
typedef u32 EXPBITSTYPE;
typedef struct {u8* cur_ptr;} READBUNDLE;
typedef struct {u8 modes[64]; u32 index, scans;} EXPBITS;
#define PTR4
#define BINK_DCT_PATTERN_BITS 4
#define BINK_RUN_BLOCK_LAST_PIXEL 63
#define BINK_BLOCK_PATTERN_OFFSET(offset,pitch) (((offset)/8)*(pitch)+(offset)%8)
static u8 scan_table[64];
#define BINK_DCT_PATTERN_SCAN(index) ((void)(index), scan_table)
static u32 exp_get_bits(EXPBITS* b,u32 n){++b->scans;return 0;}
#define EXPBITS_GET1(bits,bit) ((bit)=(bits).modes[(bits).index++])
"""
TEST = r"""
static u32 seed=76543;
static u32 rnd(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void){
 u32 pitches[]={8,16,37,64},cases=0;
 for(u32 p=0;p<4;p++)for(u32 first=1;first<=16;first++)
 for(u32 mode=0;mode<2;mode++)for(u32 trial=0;trial<256;trial++){
  u8 actual[544],expected[544],colors[64],runs[64];
  u32 filled=0,nruns=0,ncolors=0,pitch=pitches[p];
  EXPBITS bits={{0},0,0};
  memset(actual,0xa5,sizeof(actual));memset(expected,0xa5,sizeof(expected));
  for(u32 i=0;i<64;i++)scan_table[i]=(u8)i;
  for(u32 i=63;i;i--){u32 j=rnd()%(i+1);u8 t=scan_table[i];scan_table[i]=scan_table[j];scan_table[j]=t;}
  while(filled<63){
   u32 count=nruns ? 1+((rnd()>>16)%16) : first;
   if(count>64-filled)count=64-filled;
   u32 repeated=nruns ? ((rnd()>>16)&1) : mode;
   runs[nruns]=(u8)(count-1);bits.modes[nruns++]=(u8)repeated;
   u8 color=(u8)(rnd()>>16);
   if(repeated)colors[ncolors++]=color;
   for(u32 i=0;i<count;i++){
    if(!repeated){color=(u8)(rnd()>>16);colors[ncolors++]=color;}
    u32 offset=scan_table[filled++];
    expected[16+(offset/8)*pitch+offset%8]=color;
   }
  }
  if(filled==63){u8 color=(u8)(rnd()>>16);colors[ncolors++]=color;
   u32 offset=scan_table[63];expected[16+(offset/8)*pitch+offset%8]=color;}
  READBUNDLE cb={colors},rb={runs};
  expand_run_block(actual+16,pitch,&cb,&rb,&bits);
  if(memcmp(actual,expected,sizeof(actual)) || cb.cur_ptr!=colors+ncolors ||
     rb.cur_ptr!=runs+nruns || bits.index!=nruns || bits.scans!=1){
   printf("FAIL pitch=%u first=%u mode=%u trial=%u\n",pitch,first,mode,trial);return 1;}
  ++cases;
 }
 printf("PASS %u run-block cases: pixels, padding, and bundle consumption\n",cases);
 return 0;
}
"""

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="clang")
    args = parser.parse_args()
    source = (ROOT / "src/bink/src/sdk/decode/expand.c").read_text()
    start = source.index("static inline void expand_run_block(")
    end = source.index("\nstatic inline void expand_pattern_block(", start)
    with tempfile.TemporaryDirectory(prefix="bink_runs_") as directory:
        path = Path(directory)
        cfile, exe = path / "check.c", path / "check.exe"
        cfile.write_text(HEADER + source[start:end] + TEST)
        subprocess.run([args.cc, "-O2", "-std=c99", str(cfile), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)

if __name__ == "__main__":
    main()
