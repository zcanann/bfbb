#include "iMemMgr.h"
#include "xMemMgr.h"
#include <stdlib.h>

extern xMemInfo_tag gMemInfo;
U32 mem_base_alloc;
U32 mem_top_alloc;

void iMemInit()
{
    mem_base_alloc = (U32)malloc(0x384000);
    mem_top_alloc = mem_base_alloc + 0x384000;
    gMemInfo.DRAM.addr = mem_base_alloc;
    gMemInfo.DRAM.size = 0x384000;
    gMemInfo.DRAM.flags = 0x820;
    gMemInfo.system.addr = 0;
    gMemInfo.system.size = 0;
    gMemInfo.system.flags = 0;
    gMemInfo.stack.addr = 0;
    gMemInfo.stack.size = 0;
    gMemInfo.stack.flags = 0;
    gMemInfo.SRAM.addr = 0;
    gMemInfo.SRAM.size = 0;
    gMemInfo.SRAM.flags = 0;
}
