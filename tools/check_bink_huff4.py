#!/usr/bin/env python3
"""Check the production Huff4 store macro against a bit-at-a-time reader.

Requires a host C compiler (clang by default). Does not build or run the game.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HEADER = r"""#include <stdint.h>
#include <stdio.h>
typedef uint32_t u32;
typedef uint8_t u8;
typedef u32 EXPBITSTYPE;
typedef struct {u32* cur; u32 bits; u32 bitlen;} EXPBITS;
#define EXP_BITS_PER_WORD 32
#define GetBitsLen(n) (0xffffffffu >> (32-(n)))
#define HUFF4_CODE_SYM(code, syms) ((syms)[(code)&15])
#define HUFF4_CODE_USED(code) ((code)>>4)
"""
TEST = r"""static u32 seed=42;
static u32 rnd(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void){
 u32 width,available,trial,cases=0;
 for(width=1;width<=8; width++) for(available=0;available<=32;available++)
 for(trial=0;trial<256;trial++){
  u32 words[2]={rnd(),rnd()},i,used,index;
  u8 table[256],symbols[16],output=0,*dest=&output,expected;
  EXPBITS actual,ref;
  for(i=0;i<(1u<<width);i++)table[i]=(u8)(((1+rnd()%width)<<4)|(rnd()&15));
  for(i=0;i<16;i++)symbols[i]=(u8)rnd();
  actual.cur=words;actual.bitlen=available;
  actual.bits=rnd() & (available==32 ? 0xffffffffu : ((1u<<available)-1));
  ref=actual;
  index=ref.bits;
  if(available<32)index|=*ref.cur<<available;
  index&=GetBitsLen(width);
  expected=symbols[table[index]&15];used=table[index]>>4;
  for(i=0;i<used;i++){
   if(!ref.bitlen){ref.bits=*ref.cur++;ref.bitlen=32;}
   ref.bits>>=1;--ref.bitlen;
  }
  EXP_READ_HUFF4_STORE(&actual,width,table,symbols,dest);
  if(output!=expected || dest!=&output+1 || actual.cur!=ref.cur || actual.bits!=ref.bits || actual.bitlen!=ref.bitlen){
   printf("FAIL width=%u available=%u trial=%u\n",width,available,trial);return 1;
  }
  ++cases;
 }
 for(trial=0;trial<1000;trial++){
  u32 old=trial,now=trial,nold=0,nnew=0;
  while(old-- !=0)++nold;
  while(--now !=(u32)-1)++nnew;
  if(nold!=nnew || old!=now)return 2;
 }
 printf("PASS %u Huff4 cases across all bit counts and peek widths; 1000 countdown cases\n",cases);
 return 0;
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="clang", help="host C compiler executable")
    args = parser.parse_args()
    source = (ROOT / "src/bink/src/sdk/decode/expand.c").read_text()
    start = source.index("#define EXP_READ_HUFF4_STORE")
    end = source.index("\nstatic inline u32 exp_read_huff4_mask", start)
    with tempfile.TemporaryDirectory(prefix="bink_huff4_") as directory:
        path = Path(directory)
        cfile = path / "check.c"
        exe = path / "check.exe"
        cfile.write_text(HEADER + source[start:end] + TEST)
        subprocess.run([args.cc, "-O2", "-std=c99", str(cfile), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main()
