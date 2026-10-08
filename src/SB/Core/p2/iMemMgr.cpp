#include "iMemMgr.h"

#include "xMemMgr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

// Linker-defined memory map symbols.
extern char __data_start[];
extern char _end[];
extern char _heap_size[];
extern char _stack[];
extern char _stack_size[];
extern char _memtop[];

static U32 StackBase;
static U32 StackSize;
xMemInfo_tag gMemInfo;

void iMemInit()
{
    U32 MemBase;
    U32 MemTop;
    U32 HeapBase;
    U32 malloc_max;
    void* p;

    printf("__data_start: %08X\n", __data_start);
    printf("_end:         %08X\n", _end);
    printf("_heap_size:   %08X\n", _heap_size);
    printf("_stack:       %08X\n", _stack);
    printf("_stack_size:  %08X\n", _stack_size);
    printf("_memtop:      %08X\n", _memtop);

    StackSize = (U32)_stack_size;
    MemBase = (U32)_end;
    MemTop = MemBase + (U32)_heap_size;
    StackBase = ((U32)_stack == -1) ? MemTop : (U32)_stack;

    // Fill the stack with a known pattern so its high-water mark can be seen.
    memset((void*)StackBase, 0x77, StackSize - 0x2000);

    HeapBase = (U32)malloc(0x377800);

    malloc_max = MemTop - MemBase - 0x377800;
    do
    {
        p = malloc(malloc_max);
        if (p == NULL)
        {
            malloc_max -= 0x400;
        }
    } while (p == NULL);
    free(p);

    printf("largest malloc:  %08X bytes\n", malloc_max);
    printf("iMem arena:   %08X - %08X  (%9d / %6d kb)\n", HeapBase, HeapBase + 0x377800 - 1,
           0x377800, 0x377800 / 1024);
    printf("malloc arena: %08X - %08X  (%9d / %6d kb)\n", HeapBase + 0x377800,
           HeapBase + 0x377800 + (malloc_max - 0x377800) - 1,
           (HeapBase + 0x377800 + (malloc_max - 0x377800) - 1) - (HeapBase + 0x377800),
           ((HeapBase + 0x377800 + (malloc_max - 0x377800) - 1) - (HeapBase + 0x377800)) / 1024);

    gMemInfo.system.addr = 0;
    gMemInfo.system.size = 0x100000;
    gMemInfo.system.flags = 0x20;
    gMemInfo.stack.addr = MemTop - StackSize;
    gMemInfo.stack.size = StackSize;
    gMemInfo.stack.flags = 0x820;
    gMemInfo.DRAM.addr = HeapBase;
    gMemInfo.DRAM.size = 0x377800;
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
