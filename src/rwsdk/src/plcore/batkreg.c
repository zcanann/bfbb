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

#define E_RW_PLUGININIT 0x80000017

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

extern void* _rwFreeListAllocReal(RwFreeList* freelist);
extern RwUInt32 _rwGetNumEngineInstances(void);

static RwFreeList toolkitRegEntriesSpace;
static RwFreeList* toolkitRegEntries;
static RwInt32 _rwPluginRegFreeListBlockSize = 64;
static RwInt32 _rwPluginRegListPreallocBlocks = 1;
static RwUInt32 numRegToolkits;
static RwPluginRegistry** toolkitNonFLRegList;

RwBool _rwPluginRegistryOpen(void)
{
    toolkitRegEntries = RwFreeListCreateAndPreallocateSpace(
        sizeof(RwPluginRegEntry), _rwPluginRegFreeListBlockSize, sizeof(RwUInt32),
        _rwPluginRegListPreallocBlocks, &toolkitRegEntriesSpace);

    if (!toolkitRegEntries)
    {
        return FALSE;
    }

    numRegToolkits = 0;

    return TRUE;
}

static void rwDestroyEntry(void* object, void* freelist)
{
    RwPluginRegEntry* entry = (RwPluginRegEntry*)object;

    if (entry->parentRegistry->firstRegEntry)
    {
        /* Reset the registry to its unextended state */
        entry->parentRegistry->sizeOfStruct = entry->parentRegistry->origSizeOfStruct;
        entry->parentRegistry->firstRegEntry = NULL;
        entry->parentRegistry->lastRegEntry = NULL;
    }

    RwFreeListFree((RwFreeList*)freelist, entry);
}

RwBool _rwPluginRegistryClose(void)
{
    RwUInt32 i;
    RwPluginRegEntry* entry;
    RwPluginRegistry* parentReg;
    RwPluginRegEntry* nextEntry;

    if (toolkitRegEntries)
    {
        RwFreeListForAllUsed(toolkitRegEntries, rwDestroyEntry, toolkitRegEntries);

        if (RWSRCGLOBAL(memoryAlloc) != _rwFreeListAllocReal)
        {
            /* Entries were not allocated from the freelist, so free them by hand */
            for (i = 0; i < numRegToolkits; i++)
            {
                parentReg = NULL;
                entry = toolkitNonFLRegList[i]->firstRegEntry;
                if (entry)
                {
                    parentReg = entry->parentRegistry;
                }

                while (entry)
                {
                    nextEntry = entry->nextRegEntry;
                    RwFreeListFree(NULL, entry);
                    entry = nextEntry;
                }

                if (parentReg && parentReg->firstRegEntry)
                {
                    parentReg->sizeOfStruct = parentReg->origSizeOfStruct;
                    parentReg->firstRegEntry = NULL;
                    parentReg->lastRegEntry = NULL;
                }
            }

            if (toolkitNonFLRegList)
            {
                RwFree(toolkitNonFLRegList);
                toolkitNonFLRegList = NULL;
            }
        }

        RwFreeListDestroy(toolkitRegEntries);
        toolkitRegEntries = NULL;
    }

    return TRUE;
}

static void* PluginDefaultConstructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

static void* PluginDefaultDestructor(void* object, RwInt32 offsetInObject, RwInt32 sizeInObject)
{
    return object;
}

static void* PluginDefaultCopy(void* dstObject, const void* srcObject, RwInt32 offsetInObject,
                               RwInt32 sizeInObject)
{
    return dstObject;
}

RwInt32 _rwPluginRegistryGetPluginOffset(const RwPluginRegistry* reg, RwUInt32 pluginID)
{
    const RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry)
    {
        if (entry->pluginID == pluginID)
        {
            return entry->offset;
        }

        entry = entry->nextRegEntry;
    }

    return -1;
}

RwInt32 _rwPluginRegistryAddPlugin(RwPluginRegistry* reg, RwInt32 size, RwUInt32 pluginID,
                                   RwPluginObjectConstructor constructCB,
                                   RwPluginObjectDestructor destructCB,
                                   RwPluginObjectCopy copyCB)
{
    RwPluginRegEntry* entry;
    RwInt32 newStructSize;

    if (!toolkitRegEntries)
    {
        return -1;
    }

    if (_rwGetNumEngineInstances())
    {
        RWERROR((E_RW_PLUGININIT));
        return -1;
    }

    if (RWSRCGLOBAL(memoryAlloc) != _rwFreeListAllocReal)
    {
        RwUInt32 i;

        /* Remember the registry so its entries can be freed on close */
        for (i = 0; i < numRegToolkits; i++)
        {
            if (reg == toolkitNonFLRegList[i])
            {
                break;
            }
        }

        if (numRegToolkits == i)
        {
            RwPluginRegistry** newRegistryList;
            RwUInt32 j;

            newRegistryList = (RwPluginRegistry**)RwMalloc((numRegToolkits + 1) *
                                                           sizeof(RwPluginRegistry*));

            j = 0;
            if (toolkitNonFLRegList)
            {
                for (; j < numRegToolkits; j++)
                {
                    newRegistryList[j] = toolkitNonFLRegList[j];
                }

                RwFree(toolkitNonFLRegList);
                toolkitNonFLRegList = NULL;
            }

            newRegistryList[j] = reg;
            numRegToolkits++;
            toolkitNonFLRegList = newRegistryList;
        }
    }

    entry = reg->firstRegEntry;
    while (entry)
    {
        if (entry->pluginID == pluginID)
        {
            RWERROR((E_RW_PLUGININIT));
            return entry->offset;
        }

        entry = entry->nextRegEntry;
    }

    newStructSize = reg->sizeOfStruct + ((size + 3) & ~3);

    if (reg->maxSizeOfStruct && newStructSize > reg->maxSizeOfStruct)
    {
        return -1;
    }

    entry = (RwPluginRegEntry*)RwFreeListAlloc(toolkitRegEntries);
    if (entry)
    {
        entry->offset = reg->sizeOfStruct;
        reg->sizeOfStruct = newStructSize;
        entry->size = size;
        entry->pluginID = pluginID;

        entry->readCB = NULL;
        entry->writeCB = NULL;
        entry->getSizeCB = NULL;
        entry->alwaysCB = NULL;
        entry->rightsCB = NULL;

        entry->constructCB = constructCB ? constructCB : PluginDefaultConstructor;
        entry->destructCB = destructCB ? destructCB : PluginDefaultDestructor;
        entry->copyCB = copyCB ? copyCB : PluginDefaultCopy;
        entry->errStrCB = NULL;

        entry->nextRegEntry = NULL;
        entry->prevRegEntry = NULL;
        entry->parentRegistry = reg;

        if (!reg->firstRegEntry)
        {
            reg->firstRegEntry = entry;
            reg->lastRegEntry = entry;
        }
        else
        {
            reg->lastRegEntry->nextRegEntry = entry;
            entry->prevRegEntry = reg->lastRegEntry;
            reg->lastRegEntry = entry;
        }

        return entry->offset;
    }

    return -1;
}

const RwPluginRegistry* _rwPluginRegistryInitObject(const RwPluginRegistry* reg, void* object)
{
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry)
    {
        if (!entry->constructCB(object, entry->offset, entry->size))
        {
            /* Unwind the plugins that were already constructed */
            entry = entry->prevRegEntry;
            while (entry)
            {
                entry->destructCB(object, entry->offset, entry->size);
                entry = entry->prevRegEntry;
            }

            return NULL;
        }

        entry = entry->nextRegEntry;
    }

    return reg;
}

const RwPluginRegistry* _rwPluginRegistryDeInitObject(const RwPluginRegistry* reg, void* object)
{
    RwPluginRegEntry* entry = reg->lastRegEntry;

    while (entry)
    {
        entry->destructCB(object, entry->offset, entry->size);
        entry = entry->prevRegEntry;
    }

    return reg;
}
