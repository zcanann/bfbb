#!/usr/bin/env python3
"""Compare the lossless reader with its pre-reordering baseline.

Uses the production writer, checks reader equivalence, and reports round-trip
failures separately (these also occur in the baseline). Host-only;
the PowerPC leading-zero intrinsic is replaced by its portable equivalent.
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
  s16 input[64],output[66],reference[66];u32 words[128]={0};
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
  memcpy(reference,output,sizeof(output));
  BPBITSTREAM baseline=reader;
  ReadBPLossless_reference(reference+1,&baseline);
  ReadBPLossless(output+1,&reader);
  u32 consumed=(u32)(reader.cur-words)*32-reader.bitlen-offset;
  if(memcmp(reference,output,sizeof(output)) || reader.cur!=baseline.cur || reader.bits!=baseline.bits || reader.bitlen!=baseline.bitlen || reader.init!=baseline.init){
   printf("FAIL equivalence level=%u offset=%u trial=%u\n",level,offset,trial);return 1;
  }
  if(memcmp(input,output+1,sizeof(input)) || output[0]!=0x5a5a || output[65]!=0x5a5a || consumed!=written || LenBPLossless(input)!=written)++roundtrip_failures;
  ++cases;
 }
 printf("PASS %u reader-equivalence cases; %u existing writer/reader round-trip failures\n",cases,roundtrip_failures);return 0;
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
    source = source[source.index("#define BP_BITS_PER_WORD"):source.index("\nu32 WriteBPLossy(")]
    original = subprocess.check_output(["git", "show", "c9b08d77e0bb1c0a9b0e08d772c503fa68f29fdc:src/bink/src/sdk/bitplane.c"], cwd=ROOT, text=True)
    original = original[original.index("void ReadBPLossless("):original.index("\nu32 WriteBPLossy(")]
    original = original.replace("void ReadBPLossless(", "void ReadBPLossless_reference(", 1)
    dct = (sdk / "dct.c").read_text()
    zigzag = re.search(r"const u8 zigzag[^=]+=(.*?);", dct, re.S)[1]
    tables = "const u8 zigzag[64]=" + zigzag + ";\n"
    tables += "const u32 VarBitsLens[33]={" + ",".join(hex((1 << n)-1) for n in range(33)) + "};\n"
    with tempfile.TemporaryDirectory(prefix="bink_bitplane_") as directory:
        path = Path(directory)
        cfile, exe = path / "check.c", path / "check.exe"
        cfile.write_text(HEADER + vb + bp + tables + source + original + TEST)
        subprocess.run([args.cc,"-O2","-fwrapv","-fno-strict-aliasing","-std=c99",str(cfile),"-o",str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
if __name__ == "__main__":
    main()
