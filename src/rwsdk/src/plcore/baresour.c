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

#define E_RW_RESOURCES 0xC
#define E_RW_NOMEM 0x80000013

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rwResources rwResources;
struct rwResources
{
    RwInt32 maxSize;
    RwInt32 currentSize;
    RwInt32 reusageSize;
    void* memHeap;
    RwLinkList entriesA;
    RwLinkList entriesB;
    RwLinkList* freeEntries;
    RwLinkList* usedEntries;
};

#define RWRESOURCESGLOBAL(var)                                                                     \
    (RWPLUGINOFFSET(rwResources, RwEngineInstance, resourcesModule.globalsOffset)->var)

extern RwBool _rwResHeapInit(void* resHeap, RwUInt32 size);
extern RwBool _rwResHeapClose(void* resHeap);
extern void _rwResHeapFree(void* memory);
extern void* _rwResHeapAlloc(void* resHeap, RwUInt32 size);

RwModuleInfo resourcesModule;

static rwResources* ResourcesInit(rwResources* res, RwUInt32 size)
{
    if (size)
    {
        res->memHeap = RwMalloc(size);
        if (!res->memHeap)
        {
            RWERROR((E_RW_NOMEM, size));
            return NULL;
        }

        if (!_rwResHeapInit(res->memHeap, size))
        {
            RwFree(res->memHeap);
            RWERROR((E_RW_RESOURCES, 0));
            return NULL;
        }
    }
    else
    {
        res->memHeap = NULL;
    }

    rwLinkListInitialize(&res->entriesA);
    rwLinkListInitialize(&res->entriesB);
    res->usedEntries = &res->entriesA;
    res->freeEntries = &res->entriesB;

    res->maxSize = size;
    res->currentSize = 0;
    res->reusageSize = 0;

    return res;
}

void* _rwResourcesOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    RwUInt32 arenaSize;

    resourcesModule.globalsOffset = offset;

    arenaSize = RWSRCGLOBAL(resArenaInitSize);
    if (!ResourcesInit(RWPLUGINOFFSET(rwResources, RwEngineInstance, resourcesModule.globalsOffset),
                       arenaSize))
    {
        return NULL;
    }

    resourcesModule.numInstances++;

    return instance;
}

void* _rwResourcesClose(void* instance, RwInt32 offset, RwInt32 size)
{
    RwResourcesEmptyArena();

    _rwResHeapClose(RWRESOURCESGLOBAL(memHeap));
    RwFree(RWRESOURCESGLOBAL(memHeap));
    RWRESOURCESGLOBAL(memHeap) = NULL;

    resourcesModule.numInstances--;

    return instance;
}

RwBool RwResourcesFreeResEntry(RwResEntry* entry)
{
    if (entry->destroyNotify)
    {
        entry->destroyNotify(entry);
    }

    if (entry->ownerRef)
    {
        *entry->ownerRef = NULL;
    }

    if (rwLLLinkAttached(&entry->link))
    {
        /* Lives in the arena */
        rwLinkListRemoveLLLink(&entry->link);
        RWRESOURCESGLOBAL(currentSize) -= entry->size;
        _rwResHeapFree(entry);
    }
    else
    {
        RwFree(entry);
    }

    return TRUE;
}

void _rwResourcesPurge(void)
{
    RwLinkList* usedEntries;
    RwLinkList* freeEntries;
    RwLLLink* first;
    RwLLLink* last;

    freeEntries = RWRESOURCESGLOBAL(freeEntries);
    usedEntries = RWRESOURCESGLOBAL(usedEntries);

    first = rwLinkListGetFirstLLLink(freeEntries);
    if (first != rwLinkListGetTerminator(freeEntries))
    {
        /* Move everything on the free list onto the end of the used list */
        if (rwLinkListEmpty(usedEntries))
        {
            usedEntries->link.next = first;
            usedEntries->link.next->prev = &usedEntries->link;
            usedEntries->link.prev = freeEntries->link.prev;
            usedEntries->link.prev->next = &usedEntries->link;

            rwLinkListInitialize(freeEntries);
        }
        else
        {
            last = rwLinkListGetLastLLLink(usedEntries);
            last->next = first;
            first->prev = last;

            last = rwLinkListGetLastLLLink(freeEntries);
            last->next = &usedEntries->link;
            usedEntries->link.prev = last;

            rwLinkListInitialize(freeEntries);
        }
    }

    /* Last frame's entries become candidates for reuse */
    RWRESOURCESGLOBAL(usedEntries) = freeEntries;
    RWRESOURCESGLOBAL(freeEntries) = usedEntries;
    RWRESOURCESGLOBAL(reusageSize) = 0;
}

RwResEntry* RwResourcesAllocateResEntry(void* owner, RwResEntry** ownerRef, RwInt32 size,
                                        RwResEntryDestroyNotify destroyNotify)
{
    RwResEntry* entry;
    RwBool exhaustedOptions = FALSE;
    RwLLLink* cur;

    while (!exhaustedOptions)
    {
        entry = (RwResEntry*)_rwResHeapAlloc(RWRESOURCESGLOBAL(memHeap), size + sizeof(RwResEntry));
        if (entry)
        {
            rwLinkListAddLLLink(RWRESOURCESGLOBAL(usedEntries), &entry->link);

            entry->owner = owner;
            entry->size = size;
            entry->ownerRef = ownerRef;
            entry->destroyNotify = destroyNotify;

            RWRESOURCESGLOBAL(currentSize) += size;

            if (ownerRef)
            {
                *ownerRef = entry;
            }

            return entry;
        }

        /* Make some room, oldest unused entries first */
        cur = rwLinkListGetLastLLLink(RWRESOURCESGLOBAL(freeEntries));
        if (cur != rwLinkListGetTerminator(RWRESOURCESGLOBAL(freeEntries)))
        {
            entry = rwLLLinkGetData(cur, RwResEntry, link);
            RwResourcesFreeResEntry(entry);
        }
        else
        {
            cur = rwLinkListGetLastLLLink(RWRESOURCESGLOBAL(usedEntries));
            if (cur != rwLinkListGetTerminator(RWRESOURCESGLOBAL(usedEntries)))
            {
                entry = rwLLLinkGetData(cur, RwResEntry, link);

                RWRESOURCESGLOBAL(reusageSize) += entry->size;
                RwResourcesFreeResEntry(entry);
            }
            else
            {
                exhaustedOptions = TRUE;
            }
        }
    }

    if (ownerRef)
    {
        *ownerRef = NULL;
    }

    RWERROR((E_RW_RESOURCES, size));
    return NULL;
}

RwBool RwResourcesSetArenaSize(RwUInt32 size)
{
    rwResources* res;

    if (!resourcesModule.numInstances)
    {
        RWSRCGLOBAL(resArenaInitSize) = size;
        return TRUE;
    }

    res = RWPLUGINOFFSET(rwResources, RwEngineInstance, resourcesModule.globalsOffset);
    res->maxSize = size;

    RwResourcesEmptyArena();

    _rwResHeapClose(res->memHeap);
    RwFree(res->memHeap);

    res->memHeap = RwMalloc(size);
    if (!res->memHeap)
    {
        res->maxSize = 0;
        RWERROR((E_RW_NOMEM, size));
        return FALSE;
    }

    if (!_rwResHeapInit(res->memHeap, size))
    {
        RwFree(res->memHeap);
        RWERROR((E_RW_RESOURCES, 0));
        return FALSE;
    }

    return TRUE;
}

RwBool RwResourcesEmptyArena(void)
{
    RwLLLink* cur;
    RwLLLink* end;
    RwResEntry* entry;

    /* Join the two lists so a single walk visits every entry */
    rwLinkListGetLastLLLink(&RWRESOURCESGLOBAL(entriesA))->next =
        rwLinkListGetFirstLLLink(&RWRESOURCESGLOBAL(entriesB));

    cur = rwLinkListGetFirstLLLink(&RWRESOURCESGLOBAL(entriesA));
    end = rwLinkListGetTerminator(&RWRESOURCESGLOBAL(entriesB));
    while (cur != end)
    {
        entry = rwLLLinkGetData(cur, RwResEntry, link);
        cur = rwLLLinkGetNext(cur);

        RwResourcesFreeResEntry(entry);
    }

    rwLinkListInitialize(&RWRESOURCESGLOBAL(entriesA));
    rwLinkListInitialize(&RWRESOURCESGLOBAL(entriesB));
    RWRESOURCESGLOBAL(reusageSize) = 0;

    return TRUE;
}
