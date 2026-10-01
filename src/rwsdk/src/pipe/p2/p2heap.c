#include <rwsdk/rwcore.h>

#define rwPLUGIN_ID 1

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_NOMEM 0x80000013

#define RxHeapReset(heap) (!(heap)->dirty ? TRUE : _rxHeapReset(heap))

static rxHeapFreeBlock* HeapFreeBlocksNewEntry(RxHeap* heap)
{
    rxHeapFreeBlock* freeBlocks;
    RwUInt32 entriesUsed;
    rxHeapFreeBlock* iter;

    entriesUsed = heap->entriesUsed;
    freeBlocks = heap->freeBlocks;

    if (heap->entriesAlloced <= entriesUsed)
    {
        freeBlocks = (rxHeapFreeBlock*)RwRealloc(heap->freeBlocks, (heap->entriesAlloced += 32) *
                                                                       sizeof(rxHeapFreeBlock));

        if (freeBlocks == NULL)
        {
            RWERROR((E_RW_NOMEM, heap->entriesAlloced * sizeof(rxHeapFreeBlock)));
            heap->entriesAlloced -= 32;
        }
        else
        {
            if (freeBlocks != heap->freeBlocks)
            {
                if (entriesUsed)
                {
                    iter = freeBlocks;
                    do
                    {
                        iter->ptr->freeEntry = iter;
                        iter++;
                    } while (--entriesUsed);
                }
            }

            heap->freeBlocks = freeBlocks;
        }
    }

    if (freeBlocks != NULL)
    {
        freeBlocks += heap->entriesUsed++;
    }

    return freeBlocks;
}

static void HeapFreeBlocksDeleteEntry(RxHeap* heap, rxHeapFreeBlock* freeEntry)
{
    if (&heap->freeBlocks[heap->entriesUsed - 1] != freeEntry)
    {
        *freeEntry = heap->freeBlocks[heap->entriesUsed - 1];
        freeEntry->ptr->freeEntry = freeEntry;
    }

    heap->entriesUsed--;
}

static rxHeapSuperBlockDescriptor* HeapSuperBlockCreate(RwUInt32 size)
{
    rxHeapSuperBlockDescriptor* superBlock;

    superBlock =
        (rxHeapSuperBlockDescriptor*)RwMalloc(size + sizeof(rxHeapSuperBlockDescriptor) + 127);
    if (superBlock != NULL)
    {
        superBlock->start =
            (void*)(((RwUInt32)superBlock + sizeof(rxHeapSuperBlockDescriptor) + 127) & ~127);
        superBlock->size = size;
        superBlock->next = NULL;
    }

    return superBlock;
}

static void HeapSuperBlockDestroy(rxHeapSuperBlockDescriptor* superBlock)
{
    if (superBlock != NULL)
    {
        RwFree(superBlock);
    }
}

static RwBool HeapSuperBlockReset(rxHeapSuperBlockDescriptor* superBlock,
                                  rxHeapSuperBlockDescriptor* attach2SuperBlock, RxHeap* heap)
{
    rxHeapFreeBlock* freeEntry;
    rxHeapBlockHeader* blockHdrBeg;
    rxHeapBlockHeader* blockHdrPrincipal;
    rxHeapBlockHeader* blockHdrEnd;
    rxHeapBlockHeader* blockHdrAttach2;

    freeEntry = HeapFreeBlocksNewEntry(heap);
    if (freeEntry != NULL)
    {
        blockHdrBeg = (rxHeapBlockHeader*)superBlock->start;
        blockHdrPrincipal = blockHdrBeg + 1;
        blockHdrEnd = (rxHeapBlockHeader*)((RwUInt8*)superBlock->start + superBlock->size -
                                           sizeof(rxHeapBlockHeader));

        blockHdrBeg->prev = NULL;
        blockHdrBeg->next = NULL;
        blockHdrBeg->size = 0;
        blockHdrBeg->freeEntry = NULL;
        *blockHdrEnd = *blockHdrBeg;

        blockHdrBeg->next = blockHdrPrincipal;
        blockHdrPrincipal->prev = blockHdrBeg;
        blockHdrPrincipal->next = blockHdrEnd;
        blockHdrEnd->prev = blockHdrPrincipal;
        blockHdrPrincipal->size = (RwUInt8*)blockHdrEnd - (RwUInt8*)(blockHdrPrincipal + 1);
        blockHdrPrincipal->freeEntry = freeEntry;

        freeEntry->ptr = blockHdrPrincipal;
        freeEntry->size = blockHdrPrincipal->size;

        if (attach2SuperBlock != NULL)
        {
            blockHdrAttach2 =
                (rxHeapBlockHeader*)((RwUInt8*)attach2SuperBlock->start + attach2SuperBlock->size -
                                     sizeof(rxHeapBlockHeader));
            blockHdrAttach2->next = blockHdrBeg;
            blockHdrBeg->prev = blockHdrAttach2;
        }

        return TRUE;
    }

    return FALSE;
}

void RxHeapFree(RxHeap* heap, void* block)
{
    rxHeapBlockHeader* blockHdr;
    RwBool mergeBack;
    RwBool mergeFwd;
    rxHeapFreeBlock* freeEntry;

    blockHdr = (rxHeapBlockHeader*)block - 1;

    mergeBack = (blockHdr->prev != NULL && blockHdr->prev->freeEntry != NULL);
    mergeFwd = (blockHdr->next != NULL && blockHdr->next->freeEntry != NULL);

    if (mergeBack)
    {
        if (mergeFwd)
        {
            HeapFreeBlocksDeleteEntry(heap, blockHdr->next->freeEntry);

            blockHdr->prev->size +=
                blockHdr->size + blockHdr->next->size + 2 * sizeof(rxHeapBlockHeader);
            blockHdr->prev->freeEntry->size = blockHdr->prev->size;
            blockHdr->prev->next = blockHdr->next->next;
            if (blockHdr->next->next != NULL)
            {
                blockHdr->next->next->prev = blockHdr->prev;
            }
        }
        else
        {
            blockHdr->prev->size += blockHdr->size + sizeof(rxHeapBlockHeader);
            blockHdr->prev->freeEntry->size = blockHdr->prev->size;
            blockHdr->prev->next = blockHdr->next;
            if (blockHdr->next != NULL)
            {
                blockHdr->next->prev = blockHdr->prev;
            }
        }
    }
    else if (mergeFwd)
    {
        blockHdr->size += blockHdr->next->size + sizeof(rxHeapBlockHeader);
        blockHdr->freeEntry = blockHdr->next->freeEntry;
        blockHdr->next->freeEntry->ptr = blockHdr;
        blockHdr->next->freeEntry->size = blockHdr->size;
        blockHdr->next = blockHdr->next->next;
        if (blockHdr->next != NULL)
        {
            blockHdr->next->prev = blockHdr;
        }
    }
    else
    {
        freeEntry = HeapFreeBlocksNewEntry(heap);
        if (freeEntry != NULL)
        {
            freeEntry->ptr = blockHdr;
            freeEntry->size = blockHdr->size;
            blockHdr->freeEntry = freeEntry;
        }
    }
}

RwBool _rxHeapReset(RxHeap* heap)
{
    rxHeapSuperBlockDescriptor* prev;
    rxHeapSuperBlockDescriptor* iter;

    heap->entriesUsed = 0;

    prev = NULL;
    iter = heap->head->next;
    while (iter != NULL)
    {
        if (!HeapSuperBlockReset(iter, prev, heap))
        {
            return FALSE;
        }

        if (prev == NULL)
        {
            heap->headBlock = (rxHeapBlockHeader*)iter->start;
        }

        prev = iter;
        iter = iter->next;
    }

    iter = heap->head;
    if (!HeapSuperBlockReset(iter, prev, heap))
    {
        return FALSE;
    }

    if (prev == NULL)
    {
        heap->headBlock = (rxHeapBlockHeader*)iter->start;
    }

    heap->dirty = FALSE;

    return TRUE;
}

void RxHeapDestroy(RxHeap* heap)
{
    rxHeapSuperBlockDescriptor* superBlock;

    if (heap != NULL)
    {
        if (heap->freeBlocks != NULL)
        {
            RwFree(heap->freeBlocks);
            heap->freeBlocks = NULL;
        }

        superBlock = heap->head;
        while (superBlock != NULL)
        {
            rxHeapSuperBlockDescriptor* const next = superBlock->next;

            HeapSuperBlockDestroy(superBlock);
            superBlock = next;
        }

        RwFree(heap);
    }
}

RxHeap* RxHeapCreate(RwUInt32 size)
{
    RxHeap* heap;
    rxHeapSuperBlockDescriptor* superBlock;

    if (size < 1024)
    {
        size = 1024;
    }

    heap = (RxHeap*)RwMalloc(sizeof(RxHeap));
    if (heap != NULL)
    {
        size = (size + 31) & ~31;
        if (size < 128)
        {
            size = 128;
        }

        superBlock = HeapSuperBlockCreate(size);
        if (superBlock != NULL)
        {
            heap->superBlockSize = size;
            heap->head = superBlock;
            heap->freeBlocks = NULL;
            heap->entriesAlloced = 0;
            heap->entriesUsed = 0;
            heap->dirty = TRUE;

            if (RxHeapReset(heap))
            {
                return heap;
            }

            HeapSuperBlockDestroy(superBlock);
        }

        RwFree(heap);
    }

    return NULL;
}
