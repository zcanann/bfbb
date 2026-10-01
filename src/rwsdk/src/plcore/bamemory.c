#include <stdlib.h>
#include <string.h>
#include <rwsdk/rwcore.h>

/* Not declared by our MSL stdlib.h */
extern void* malloc(size_t size);
extern void* realloc(void* mem, size_t newSize);

#define rwFREELISTFLAG_STATIC 0x00000001
#define rwFREELISTFLAG_FREEBLOCKS 0x00000002

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

#define rwFREELISTALIGN(ptr, align)                                                                    ((RwUInt8*)(((RwUInt32)(ptr) + ((align) - 1)) & ~((align) - 1)))

static RwBool FreeListsEnabled = TRUE;

static RwFreeList _masterFreeList;
static RwLinkList _freeListList;
static RwFreeList* _masterFreeListPtr;
static RwBool _freeListModuleOpen;

static void _RwFreeListFree(RwFreeList* freeList);

void _rwFreeListEnable(RwBool enabled)
{
    FreeListsEnabled = enabled;
}

static RwFreeList* FreeListCreate(RwUInt32 entrySize, RwUInt32 entriesPerBlock,
                                  RwUInt32 alignment, RwUInt32 blocks, RwFreeList* freeList)
{
    RwUInt32 alignedBlockSize;
    RwUInt32 heapSize;
    void* block;
    RwLLLink* link;

    if (!FreeListsEnabled)
    {
        blocks = 0;
    }

    if (!alignment)
    {
        alignment = 32;
    }

    if (!freeList)
    {
        if (_masterFreeListPtr)
        {
            freeList = (RwFreeList*)RwFreeListAlloc(_masterFreeListPtr);
        }
        else
        {
            freeList = (RwFreeList*)RwMalloc(sizeof(RwFreeList));
        }

        if (!freeList)
        {
            return NULL;
        }

        freeList->flags = rwFREELISTFLAG_FREEBLOCKS;
    }
    else
    {
        freeList->flags = rwFREELISTFLAG_STATIC | rwFREELISTFLAG_FREEBLOCKS;
    }

    entrySize = (entrySize + alignment - 1) & ~(alignment - 1);
    heapSize = (entriesPerBlock + 7) >> 3;

    freeList->entrySize = entrySize;
    freeList->entriesPerBlock = entriesPerBlock;
    freeList->heapSize = heapSize;
    freeList->alignment = alignment;
    rwLinkListInitialize(&freeList->blockList);

    alignedBlockSize = heapSize + entriesPerBlock * entrySize + alignment + sizeof(RwLLLink) - 1;

    while (blocks)
    {
        block = RwMalloc(alignedBlockSize);
        if (!block)
        {
            _RwFreeListFree(freeList);
            return NULL;
        }

        link = (RwLLLink*)block;
        rwLLLinkInitialize(link);
        rwLinkListAddLLLink(&freeList->blockList, link);

        memset(link + 1, 0, heapSize);

        blocks--;
    }

    rwLinkListAddLLLink(&_freeListList, &freeList->link);

    return freeList;
}

RwFreeList* RwFreeListCreate(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment)
{
    return FreeListCreate(entrySize, entriesPerBlock, alignment, 1, NULL);
}

RwFreeList* RwFreeListCreateAndPreallocateSpace(RwInt32 entrySize, RwInt32 entriesPerBlock,
                                                RwInt32 alignment, RwInt32 numBlocksToPreallocate,
                                                RwFreeList* inPlaceSpaceForFreeListStruct)
{
    return FreeListCreate(entrySize, entriesPerBlock, alignment, numBlocksToPreallocate,
                          inPlaceSpaceForFreeListStruct);
}

static void _RwFreeListFree(RwFreeList* freeList)
{
    RwLLLink* link;

    link = rwLinkListGetFirstLLLink(&freeList->blockList);
    while (link != rwLinkListGetTerminator(&freeList->blockList))
    {
        rwLinkListRemoveLLLink(link);
        RwFree(link);

        link = rwLinkListGetFirstLLLink(&freeList->blockList);
    }

    if (!(freeList->flags & rwFREELISTFLAG_STATIC))
    {
        if (_masterFreeListPtr == freeList || !_masterFreeListPtr)
        {
            RwFree(freeList);
        }
        else
        {
            RwFreeListFree(_masterFreeListPtr, freeList);
        }
    }
}

RwBool RwFreeListDestroy(RwFreeList* freeList)
{
    rwLinkListRemoveLLLink(&freeList->link);
    _RwFreeListFree(freeList);

    return TRUE;
}

void* _rwFreeListAllocReal(RwFreeList* freeList)
{
    RwUInt32 heapEntries = freeList->heapSize;
    void* freeEntry = NULL;
    RwLLLink* link;
    RwLLLink* lastLink;

    link = rwLinkListGetFirstLLLink(&freeList->blockList);
    lastLink = rwLinkListGetTerminator(&freeList->blockList);
    while (link != lastLink && !freeEntry)
    {
        RwUInt8* heap = (RwUInt8*)(link + 1);
        RwUInt32 checkEntries = freeList->entriesPerBlock;
        RwUInt32 i;

        for (i = 0; i < heapEntries; i++)
        {
            RwUInt8 heapElement = heap[i];

            if (heapElement != 0xFF)
            {
                RwUInt32 j = 0;

                while (j < 8 && checkEntries)
                {
                    RwUInt8 mask = (RwUInt8)(0x80 >> j);

                    if (!(heapElement & mask))
                    {
                        RwUInt8* aligned;

                        heap[i] = mask | heapElement;

                        aligned = rwFREELISTALIGN(heap + heapEntries, freeList->alignment);
                        freeEntry = aligned + freeList->entrySize * (i * 8 + j);
                        break;
                    }

                    j++;
                    checkEntries--;
                }
            }
            else
            {
                checkEntries -= 8;
            }

            if (freeEntry)
            {
                break;
            }
        }

        link = rwLLLinkGetNext(link);
    }

    if (!freeEntry)
    {
        /* Every block is full, so add another one */
        RwUInt8* heap;

        link = (RwLLLink*)RwMalloc(sizeof(RwLLLink) + heapEntries +
                                   freeList->entriesPerBlock * freeList->entrySize +
                                   freeList->alignment - 1);
        if (!link)
        {
            return NULL;
        }

        heap = (RwUInt8*)(link + 1);
        memset(heap, 0, heapEntries);
        rwLinkListAddLLLink(&freeList->blockList, link);

        heap[0] = 0x80;
        freeEntry = rwFREELISTALIGN(heap + heapEntries, freeList->alignment);
    }

    return freeEntry;
}

static RwBool FreeListBlockIsEmpty(RwUInt8* heap, RwUInt32 heapEntries)
{
    RwUInt32 i;
    RwUInt32 sum = 0;

    for (i = 0; i < heapEntries; i++)
    {
        sum += heap[i];
    }

    return sum == 0;
}

RwFreeList* _rwFreeListFreeReal(RwFreeList* freeList, void* entry)
{
    RwUInt32 heapEntries = freeList->heapSize;
    RwLLLink* link;

    link = rwLinkListGetFirstLLLink(&freeList->blockList);
    while (link != rwLinkListGetTerminator(&freeList->blockList))
    {
        RwUInt8* dataBlock = (RwUInt8*)link + (sizeof(RwLLLink) + heapEntries);

        if ((RwUInt8*)entry >= dataBlock &&
            (RwUInt8*)entry <= dataBlock + freeList->entriesPerBlock * freeList->entrySize)
        {
            RwUInt8* heap = (RwUInt8*)(link + 1);
            RwUInt32 entryIndex = ((RwUInt8*)entry - dataBlock) / freeList->entrySize;
            RwUInt32 heapElement = entryIndex >> 3;
            RwUInt8 mask = (RwUInt8)(0x80 >> (entryIndex - (heapElement << 3)));

            heap[heapElement] &= (RwUInt8)~mask;

            if (freeList->flags & rwFREELISTFLAG_FREEBLOCKS)
            {
                /* Release the block once it is empty, unless it is the only one */
                if (FreeListBlockIsEmpty(heap, heapEntries) &&
                    (link != rwLinkListGetFirstLLLink(&freeList->blockList) ||
                     rwLLLinkGetNext(link) != rwLinkListGetTerminator(&freeList->blockList)))
                {
                    rwLinkListRemoveLLLink(link);
                    RwFree(link);
                }
            }

            return freeList;
        }

        link = rwLLLinkGetNext(link);
    }

    return NULL;
}

RwFreeList* RwFreeListForAllUsed(RwFreeList* freeList, RwFreeListCallBack fpCallBack,
                                 void* pData)
{
    RwUInt32 heapEntries = freeList->heapSize;
    RwLLLink* link;
    RwLLLink* nextLink;

    link = rwLinkListGetFirstLLLink(&freeList->blockList);
    while (link != rwLinkListGetTerminator(&freeList->blockList))
    {
        RwUInt8* heapCopy;
        RwUInt32 i;

        /* Work from a copy so the callback may free entries */
        heapCopy = (RwUInt8*)RwMalloc(heapEntries);
        if (!heapCopy)
        {
            return NULL;
        }

        memcpy(heapCopy, link + 1, heapEntries);
        nextLink = rwLLLinkGetNext(link);

        for (i = 0; i < heapEntries; i++)
        {
            RwUInt8 heapElement = heapCopy[i];

            if (heapElement)
            {
                RwUInt32 j;

                for (j = 0; j < 8; j++)
                {
                    if (heapElement & (RwUInt8)(0x80 >> j))
                    {
                        RwUInt8* aligned = rwFREELISTALIGN((RwUInt8*)(link + 1) + heapEntries,
                                                           freeList->alignment);

                        fpCallBack(aligned + freeList->entrySize * (i * 8 + j), pData);
                    }
                }
            }
        }

        RwFree(heapCopy);

        link = nextLink;
    }

    return freeList;
}

static void* FakeCalloc(size_t numObj, size_t sizeObj)
{
    size_t size = numObj * sizeObj;
    void* mem = malloc(size);

    if (mem)
    {
        memset(mem, 0, size);
    }

    return mem;
}

static RwBool _rwFreeListModuleOpen(void)
{
    rwLinkListInitialize(&_freeListList);
    _freeListModuleOpen = TRUE;

    _masterFreeListPtr = FreeListCreate(sizeof(RwFreeList), 16, 32, 0, &_masterFreeList);
    if (!_masterFreeListPtr)
    {
        _freeListModuleOpen = FALSE;
        return FALSE;
    }

    /* The master freelist is not tracked in the list of freelists */
    rwLinkListRemoveLLLink(&_masterFreeListPtr->link);

    return TRUE;
}

static void _rwFreeListModuleClose(void)
{
    RwLLLink* link;

    link = rwLinkListGetFirstLLLink(&_freeListList);
    while (link != rwLinkListGetTerminator(&_freeListList))
    {
        RwFreeListDestroy(rwLLLinkGetData(link, RwFreeList, link));

        link = rwLinkListGetFirstLLLink(&_freeListList);
    }

    RwFreeListDestroy(_masterFreeListPtr);
    _masterFreeListPtr = NULL;

    _freeListModuleOpen = FALSE;
}

RwBool _rwMemoryOpen(const RwMemoryFunctions* memFuncs)
{
    if (!_rwFreeListModuleOpen())
    {
        return FALSE;
    }

    if (memFuncs)
    {
        RWSRCGLOBAL(memoryFuncs) = *memFuncs;
    }
    else
    {
        RWSRCGLOBAL(memoryFuncs).rwmalloc = malloc;
        RWSRCGLOBAL(memoryFuncs).rwfree = free;
        RWSRCGLOBAL(memoryFuncs).rwrealloc = realloc;
        RWSRCGLOBAL(memoryFuncs).rwcalloc = FakeCalloc;
    }

    return TRUE;
}

void _rwMemoryClose(void)
{
    _rwFreeListModuleClose();
}
