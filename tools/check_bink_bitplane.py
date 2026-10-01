#!/usr/bin/env python3
"""Round-trip production lossless and lossy bitplane coding.

Covers zero/sparse/dense blocks with magnitudes up to 32767, every initial
word offset, output guards, exact bit lengths, and lossless DC preservation.
Lossy checks also cover early mask cutoffs. Host-only; the PowerPC
leading-zero intrinsic is replaced by its portable equivalent.
"""
import argparse
from pathlib import Path
import re
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
#define RADINLINE inline
"""
TEST = r"""
static u32 seed=813;
static u32 rnd(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void){
 u32 cases=0,roundtrip_failures=0;
 for(u32 level=0;level<16;level++)for(u32 offset=0;offset<32;offset++)
 for(u32 trial=0;trial<32;trial++){
  s16 input[64],output[66];u32 words[128]={0};
  u32 mask=(1u<<level)-1;
  for(u32 i=0;i<64;i++){
   s32 v=(rnd()>>16)&mask;if((rnd()>>16)&1)v=-v;
   if(trial%4==0 && i!=trial+1)v=0;
   if(trial%4==1 && i%8)v=0;
   input[i]=(s16)v;
  }
  input[0]=123;memset(output,0x5a,sizeof(output));output[1]=input[0];
  BPBITSTREAM writer={words,words,0,offset};
  WriteBPLossless(&writer,input);
  u32 written=(u32)(writer.cur-words)*32+writer.bitlen-offset;
  *writer.cur=writer.bits;
  BPBITSTREAM reader={words,words,0,0};
  if(offset){u32 ignored;VarBitsGet(ignored,u32,reader,offset);}
  ReadBPLossless(output+1,&reader);
  u32 consumed=(u32)(reader.cur-words)*32-reader.bitlen-offset;
  if(memcmp(input,output+1,sizeof(input)) || output[0]!=0x5a5a || output[65]!=0x5a5a || consumed!=written || LenBPLossless(input)!=written){
   if(!roundtrip_failures){printf("FIRST level=%u offset=%u trial=%u written=%u consumed=%u len=%u\n",level,offset,trial,written,consumed,LenBPLossless(input));for(u32 k=0;k<64;k++)if(input[k]!=output[k+1])printf("coeff %u: %d != %d\n",k,input[k],output[k+1]);}
   ++roundtrip_failures;
  }
  ++cases;
 }
 u32 lossy_cases=0,cutoff_cases=0;
 for(u32 level=1;level<=7;level++)for(u32 offset=0;offset<32;offset++)
 for(u32 trial=0;trial<32;trial++){
  s8 input[64],output[66];u32 words[128]={0},mask=(1u<<level)-1;
  for(u32 i=0;i<64;i++){
   s32 v=(rnd()>>16)&mask;if((rnd()>>16)&1)v=-v;
   if(trial%4==0 && i!=trial+1)v=0;
   if(trial%4==1 && i%8)v=0;
   input[i]=(s8)v;
  }
  BPBITSTREAM writer={words,words,0,offset};
  if(!WriteBPLossy(&writer,(char*)input)){
   for(u32 k=0;k<64;k++)if(input[k]){printf("FAIL nonzero lossy block omitted\n");return 4;}
   if(writer.cur!=words || writer.bitlen!=offset || writer.bits){printf("FAIL empty lossy stream changed\n");return 5;}
   continue;
  }
  u32 written=(u32)(writer.cur-words)*32+writer.bitlen-offset;
  *writer.cur=writer.bits;
  BPBITSTREAM reader={words,words,0,0};
  if(offset){u32 ignored;VarBitsGet(ignored,u32,reader,offset);}
  memset(output,0x5a,sizeof(output));readlossy(output+1,&reader,65535);
  for(u32 k=0;k<64;k++)if(output[k+1]!=input[zigzag[k]]){
   printf("FAIL lossy level=%u offset=%u trial=%u scan=%u: %d != %d\n",level,offset,trial,k,output[k+1],input[zigzag[k]]);return 2;
  }
  u32 consumed=(u32)(reader.cur-words)*32-reader.bitlen-offset;
  if(consumed!=written || output[0]!=0x5a || output[65]!=0x5a){printf("FAIL lossy bit count or guards\n");return 3;}
  u32 updates=0,last_consumed=0;
  for(u32 k=0;k<64;k++){
   u32 magnitude=input[k]<0?-input[k]:input[k];
   for(;magnitude;magnitude>>=1)updates+=magnitude&1;
  }
  for(u32 checkpoint=0;checkpoint<4;checkpoint++){
   u32 cutoff=checkpoint==0?0:checkpoint==1?updates/2:checkpoint==2?updates-1:updates;
   BPBITSTREAM partial={words,words,0,0};
   if(offset){u32 ignored;VarBitsGet(ignored,u32,partial,offset);}
   memset(output,0x5a,sizeof(output));readlossy(output+1,&partial,cutoff);
   u32 found=0;
   for(u32 k=0;k<64;k++){
    s32 want=input[zigzag[k]],got=output[k+1];
    u32 wm=want<0?-want:want,gm=got<0?-got:got;
    if((gm&~wm) || (got && ((got<0)!=(want<0)))){printf("FAIL partial magnitude/sign\n");return 6;}
    for(;gm;gm>>=1)found+=gm&1;
   }
   u32 used=(u32)(partial.cur-words)*32-partial.bitlen-offset;
   u32 wanted=cutoff<updates?cutoff+1:updates;
   if(found!=wanted || used<last_consumed || used>written ||
      (cutoff>=updates && used!=written) || output[0]!=0x5a || output[65]!=0x5a){
    printf("FAIL cutoff level=%u offset=%u trial=%u limit=%u found=%u wanted=%u\n",level,offset,trial,cutoff,found,wanted);return 7;
   }
   if(partial.bitlen && partial.bits!=(words[partial.cur-words-1]>>(32-partial.bitlen))){printf("FAIL partial reservoir\n");return 8;}
   last_consumed=used;++cutoff_cases;
  }
  ++lossy_cases;
 }
 printf("PASS %u nonempty lossy round trips and %u early-cutoff checks\n",lossy_cases,cutoff_cases);
 if(roundtrip_failures){printf("FAIL %u of %u round trips\n",roundtrip_failures,cases);return 1;}
 printf("PASS %u lossless bitplane round trips, guards, and bit-length checks\n",cases);return 0;
}
"""
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="clang")
    args = parser.parse_args()
    sdk = ROOT / "src/bink/src/sdk"
    vb = (sdk / "varbits.h").read_text().replace('#include "bink.h"', '')
    start = vb.index("static RADINLINE u32 getbitlevelvar(")
    end = vb.index("\n}", start) + 2
    vb = vb[:start] + "static inline u32 getbitlevelvar(u32 v){return v?32-__builtin_clz(v):0;}" + vb[end:]
    bp = (sdk / "bitplane.h").read_text().replace('#include "bink.h"', '')
    source = (sdk / "bitplane.c").read_text()
    source = source[source.index("#define BP_BITS_PER_WORD"):source.index("\nvoid ReadBPLossy(")]
    dct = (sdk / "dct.c").read_text()
    zigzag = re.search(r"const u8 zigzag[^=]+=(.*?);", dct, re.S)[1]
    tables = "const u8 zigzag[64]=" + zigzag + ";\n"
    tables += "const u32 VarBitsLens[33]={" + ",".join(hex((1 << n)-1) for n in range(33)) + "};\n"
    with tempfile.TemporaryDirectory(prefix="bink_bitplane_") as directory:
        path = Path(directory)
        cfile, exe = path / "check.c", path / "check.exe"
        cfile.write_text(HEADER + vb + bp + tables + source + TEST)
        subprocess.run([args.cc,"-O2","-fwrapv","-fno-strict-aliasing","-std=c99",str(cfile),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
if __name__ == "__main__":
    main()
