#include <rwsdk/rwcore.h>

typedef struct rwResHeapHeader rwResHeapHeader;
typedef struct rwResHeapBlockHeader rwResHeapBlockHeader;

struct rwResHeapHeader
{
    rwResHeapBlockHeader* firstBlock;
    rwResHeapBlockHeader* firstFreeBlock;
};

enum rwResHeapBlockFlags
{
    rwRESHEAP_BLOCKUSED = 1,
    rwRESHEAPFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum rwResHeapBlockFlags rwResHeapBlockFlags;

struct rwResHeapBlockHeader
{
    rwResHeapHeader* heap;
    rwResHeapBlockHeader* next;
    rwResHeapBlockHeader* prev;
    RwUInt32 size;
    rwResHeapBlockFlags flags;
    RwUInt32 pad[3];
};

#define rwRESHEAPALIGNMENT 32
#define rwRESHEAPALIGN(x) (((RwUInt32)(x) + (rwRESHEAPALIGNMENT - 1)) & ~(rwRESHEAPALIGNMENT - 1))

static void splitBlock(rwResHeapBlockHeader* block, RwUInt32 size)
{
    rwResHeapBlockHeader* newBlock;

    newBlock = (rwResHeapBlockHeader*)((RwUInt8*)block + size + sizeof(rwResHeapBlockHeader));

    if (block->next && (~block->next->flags & rwRESHEAP_BLOCKUSED))
    {
        /* Absorb the following free block */
        newBlock->next = block->next->next;
        newBlock->size = block->next->size + (block->size - size);
    }
    else
    {
        newBlock->next = block->next;
        newBlock->size = block->size - size - sizeof(rwResHeapBlockHeader);
    }

    block->next = newBlock;
    newBlock->flags = (rwResHeapBlockFlags)0;
    newBlock->prev = block;

    if (newBlock->next)
    {
        newBlock->next->prev = newBlock;
    }

    block->size = size;
    newBlock->heap = block->heap;
}

RwBool _rwResHeapInit(void* resHeap, RwUInt32 size)
{
    rwResHeapHeader* heapInfo = (rwResHeapHeader*)resHeap;
    rwResHeapBlockHeader* firstBlock;
    RwUInt32 start;
    RwUInt32 end;
    RwInt32 blockSize;

    start = rwRESHEAPALIGN((RwUInt8*)resHeap + sizeof(rwResHeapHeader));
    end = ((RwUInt32)resHeap + size) & ~(rwRESHEAPALIGNMENT - 1);
    blockSize = end - start - sizeof(rwResHeapBlockHeader);

    if (blockSize < rwRESHEAPALIGNMENT)
    {
        return FALSE;
    }

    firstBlock = (rwResHeapBlockHeader*)start;
    firstBlock->heap = heapInfo;
    firstBlock->next = NULL;
    firstBlock->prev = NULL;
    firstBlock->flags = (rwResHeapBlockFlags)0;
    firstBlock->size = blockSize;

    heapInfo->firstBlock = firstBlock;
    heapInfo->firstFreeBlock = firstBlock;

    return TRUE;
}

RwBool _rwResHeapClose(void* resHeap)
{
    return TRUE;
}

void _rwResHeapFree(void* memory)
{
    rwResHeapBlockHeader* block;
    rwResHeapBlockHeader* prevBlock;
    rwResHeapBlockHeader* nextBlock;

    block = (rwResHeapBlockHeader*)memory - 1;
    block->flags = (rwResHeapBlockFlags)0;

    prevBlock = block->prev;
    nextBlock = block->next;

    if (!block->heap->firstFreeBlock || block < block->heap->firstFreeBlock)
    {
        block->heap->firstFreeBlock = block;
    }

    if (prevBlock && (~prevBlock->flags & rwRESHEAP_BLOCKUSED))
    {
        prevBlock->next = nextBlock;
        if (nextBlock)
        {
            nextBlock->prev = prevBlock;
        }

        prevBlock->size += block->size + sizeof(rwResHeapBlockHeader);
        block = prevBlock;
    }

    if (nextBlock && (~nextBlock->flags & rwRESHEAP_BLOCKUSED))
    {
        block->next = nextBlock->next;
        if (nextBlock->next)
        {
            nextBlock->next->prev = block;
        }

        block->size += nextBlock->size + sizeof(rwResHeapBlockHeader);
    }
}

void* _rwResHeapAlloc(void* resHeap, RwUInt32 size)
{
    rwResHeapHeader* heapInfo = (rwResHeapHeader*)resHeap;
    rwResHeapBlockHeader* targetBlock;
    rwResHeapBlockHeader* curBlock;

    size = rwRESHEAPALIGN(size);

    targetBlock = NULL;
    curBlock = heapInfo->firstFreeBlock;
    while (curBlock)
    {
        if ((~curBlock->flags & rwRESHEAP_BLOCKUSED) && curBlock->size >= size)
        {
            if (!targetBlock || curBlock->size < targetBlock->size)
            {
                targetBlock = curBlock;
            }
        }
        curBlock = curBlock->next;
    }

    if (!targetBlock)
    {
        return NULL;
    }

    if (targetBlock->size > size + 2 * sizeof(rwResHeapBlockHeader))
    {
        splitBlock(targetBlock, size);
    }

    if (targetBlock == heapInfo->firstFreeBlock)
    {
        do
        {
            heapInfo->firstFreeBlock = heapInfo->firstFreeBlock->next;
        } while (heapInfo->firstFreeBlock &&
                 (heapInfo->firstFreeBlock->flags & rwRESHEAP_BLOCKUSED));
    }

    targetBlock->flags = rwRESHEAP_BLOCKUSED;

    return targetBlock + 1;
}
