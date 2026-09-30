#include "iMemMgr.h"
#include "iSystem.h"
#include "xMemMgr.h"

#include <types.h>
#include <PowerPC_EABI_Support/MSL_C/MSL_Common/stdlib.h>
#include <dolphin.h>

U32 mem_top_alloc;
U32 mem_base_alloc;
volatile OSHeapHandle the_heap;
OSHeapHandle hs;
OSHeapHandle he;
U32 HeapSize;
extern xMemInfo_tag gMemInfo;
extern unsigned char _stack_end[];

void iMemInit()
{
    OSHeapHandle hi = (OSHeapHandle)OSGetArenaHi();
    he = hi & 0xffffffe0;
    OSHeapHandle lo = (OSHeapHandle)OSGetArenaLo() + 0x1f & 0xffffffe0;
    hs = lo;
    the_heap = OSCreateHeap(OSInitAlloc((void*)lo, (void*)(hi & 0xffffffe0), 1), (void*)he);
    OSHeapHandle currHeap = the_heap;
    if (currHeap >= 0)
    {
        OSSetCurrentHeap(currHeap);
    }
    else
    {
        exit(-5);
    }
    gMemInfo.system.addr = 0;
    gMemInfo.system.size = 0x100000;
    gMemInfo.system.flags = 0x20;
    // The original writes the stack base as a plain constant -- there is no
    // relocation on it -- but reaches _stack_end through one, so only the size
    // is expressed in terms of a linker symbol.
    gMemInfo.stack.addr = 0x803d8a50;
    gMemInfo.stack.size = (U32)_stack_end - gMemInfo.stack.addr;
    gMemInfo.stack.flags = 0x820;
    HeapSize = 0x384000;
    U32 base = (U32)OSAllocFromHeap(__OSCurrHeap, HeapSize);
    mem_base_alloc = base;
    mem_top_alloc = base + HeapSize;
    gMemInfo.DRAM.addr = base;
    gMemInfo.DRAM.size = HeapSize;
    gMemInfo.DRAM.flags = 0x820;
    gMemInfo.SRAM.addr = 0;
    gMemInfo.SRAM.size = 0x200000;
    gMemInfo.SRAM.flags = 0x660;
}

void iMemExit()
{
    free((void*)gMemInfo.DRAM.addr);
    gMemInfo.DRAM.addr = 0;
}
