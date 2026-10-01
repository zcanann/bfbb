#!/usr/bin/env python3
"""Check production byte IDCT against a separable scalar transform.

Uses production quantization tables and checks padded output pitches.
Requires a host C compiler; this does not run the GameCube decoder.
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
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
#define PTR4
#define RAD_ATTRIBUTE_ALIGN(n)
"""
TEST = r"""
static s32 mul(s32 a,s32 b){return (s32)((u32)a*(u32)b)>>11;}
static void reference1d(const s32* x,s32* y){
 s32 es=x[2]+x[6],ed=mul(x[2]-x[6],2896)-es;
 s32 e0=x[0]+x[4],e1=x[0]-x[4];
 s32 o0=x[5]-x[3],o1=x[1]-x[7];
 s32 p0=x[1]+x[7],p1=x[5]+x[3];
 s32 rot=mul(o0+o1,3784),mid=mul(o0,-5352)+rot-(p0+p1);
 s32 cross=mul(p0-p1,2896)-mid,tail=mul(o1,2217)-rot+cross;
 s32 even[4]={e0+es,e1+ed,e1-ed,e0-es};
 s32 odd[4]={p0+p1,mid,cross,tail};
 for(u32 i=0;i<4;i++){u32 pos=i==3?4:i;y[pos]=even[i]+odd[i];y[7-pos]=even[i]-odd[i];}
}
static u32 seed=773;
static u32 rnd(void){seed=seed*1664525u+1013904223u;return seed;}
int main(void){
 u32 pitches[]={8,16,37,64},cases=0;
 for(u32 kind=0;kind<2;kind++)for(u32 level=0;level<16;level++)
 for(u32 p=0;p<4;p++)for(u32 trial=0;trial<256;trial++){
  s16 input[64];s32 work[64],x[8],y[8];
  u8 actual[544],expected[544];u32 pitch=pitches[p],doublepitch=(pitch*2+3)&~3u;
  u32 doubled[520],expected_doubled[520];
  const s32* quant=kind?ifimquantlevels8[level]:ifiquantlevels8[level];
  for(u32 i=0;i<64;i++)input[i]=(s16)((rnd()>>16)%65-32);
  if(trial%4==0)for(u32 i=8;i<64;i++)input[i]=0;
  if(trial%4==1)for(u32 i=0;i<64;i++)if(i!=trial%64)input[i]=0;
  for(u32 col=0;col<8;col++){
   for(u32 row=0;row<8;row++)x[row]=mul(input[row*8+col],quant[row*8+col]);
   reference1d(x,y);for(u32 row=0;row<8;row++)work[row*8+col]=y[row];
  }
  memset(actual,0xa5,sizeof(actual));memset(expected,0xa5,sizeof(expected));
  for(u32 row=0;row<8;row++){
   reference1d(work+row*8,y);
   for(u32 col=0;col<8;col++)expected[16+row*pitch+col]=(u8)((y[col]+127)>>8);
  }
  fastidct8x8(actual+16,pitch,input,quant);
  if(memcmp(actual,expected,sizeof(actual))){printf("FAIL kind=%u level=%u pitch=%u trial=%u\n",kind,level,pitch,trial);return 1;}
  memset(doubled,0xa5,sizeof(doubled));memset(expected_doubled,0xa5,sizeof(expected_doubled));
  for(u32 row=0;row<8;row++)for(u32 pair=0;pair<4;pair++){
   u32 left=expected[16+row*pitch+pair*2],right=expected[16+row*pitch+pair*2+1];
   u32 word=(left<<24)|(left<<16)|(right<<8)|right;
   expected_doubled[4+(row*2)*(doublepitch/4)+pair]=word;
   expected_doubled[4+(row*2+1)*(doublepitch/4)+pair]=word;
  }
  fastidct8x8d(doubled+4,doublepitch,input,quant);
  if(memcmp(doubled,expected_doubled,sizeof(doubled))){printf("FAIL doubled kind=%u level=%u pitch=%u trial=%u\n",kind,level,pitch,trial);return 2;}
  ++cases;
 }
 printf("PASS %u IDCT cases: byte and doubled output, scalar reference and padding\n",cases);return 0;
}
"""

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default="clang")
    args = parser.parse_args()
    source = (ROOT / "src/bink/src/sdk/dct.c").read_text()
    start = source.index("#define DCT_BLOCK_WIDTH")
    end = source.index("\nvoid FastmIDCT8x8(")
    with tempfile.TemporaryDirectory(prefix="bink_idct_") as directory:
        path = Path(directory)
        cfile, exe = path / "check.c", path / "check.exe"
        cfile.write_text(HEADER + source[start:end] + TEST)
        subprocess.run([args.cc, "-O2", "-fwrapv", "-std=c99", str(cfile), "-o", str(exe)], check=True)
        subprocess.run([str(exe)], check=True)

if __name__ == "__main__":
    main()
