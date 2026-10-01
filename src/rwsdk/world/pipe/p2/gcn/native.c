#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/os.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

#define E_RW_BADVERSION 0x80000004

#ifndef rwCHUNKHEADERSIZE
#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)
#endif

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
                                                                                                   \
        _rwErrorCode.pluginID = rwID_WORLDPLUGIN;                                                  \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define RwStreamWriteVersionedChunkHeader(_stream, _type, _size)                                  \
    _rwStreamWriteVersionedChunkHeader(_stream, _type, _size, rwLIBRARYCURRENTVERSION, 0xFFFF)

/* The display list data starts on the first 32 byte boundary after the display list headers */
#define NATIVEBASEADDRESS(_dList, _numMeshes)                                                     \
    ((RwUInt8*)(((RwUInt32)((_dList) + (_numMeshes)) + 31) & ~31))

static void _rpNativeOffset2Pointer(RxGameCubeVertexBuffer* vbHeader, RxGameCubeDisplayList* dList,
                                    RwUInt32 numMeshes)
{
    RwUInt32 i;
    RwUInt8* baseAddress;

    baseAddress = NATIVEBASEADDRESS(dList, numMeshes);

    for (i = 0; i < vbHeader->numAttrArrays; i++)
    {
        vbHeader->attr[i].array = baseAddress + (RwUInt32)vbHeader->attr[i].array;
    }

    for (i = 0; i < numMeshes; i++)
    {
        dList[i].displayList = baseAddress + (RwUInt32)dList[i].displayList;
    }
}

static RwUInt32 _rpNativeSize(RwResEntry* resEntry)
{
    RwUInt32 size;

    /* Chunk header, platform ID, header size, display list size, then the data less the
     * alignment padding */
    size = rwCHUNKHEADERSIZE + sizeof(RwPlatformID) + sizeof(RwUInt32) + sizeof(RwUInt32) +
           resEntry->size - 31;

    return size;
}

static void* _rpNativeRead(RwStream* stream, void* owner, RwResEntry** resEntryPointer,
                           RwUInt32 numMeshes)
{
    RwUInt32 version;
    RwUInt32 size;
    RwUInt32 headerSize;
    RwUInt32 dlistSize;
    RwPlatformID id;
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* dList;
    RwUInt8* displayLists;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return NULL;
    }

    if (!((version >= rwLIBRARYBASEVERSION) && (version <= rwLIBRARYCURRENTVERSION)))
    {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    /* Older native data is not compatible */
    if (version <= 0x34004)
    {
        RWERROR((E_RW_BADVERSION));
        return NULL;
    }

    if (!RwStreamReadInt32(stream, (RwInt32*)&id, sizeof(id)))
    {
        return NULL;
    }

    if (id != rwID_GAMECUBE)
    {
        return NULL;
    }

    if (!RwStreamReadInt32(stream, (RwInt32*)&headerSize, sizeof(headerSize)))
    {
        return NULL;
    }

    if (!RwStreamReadInt32(stream, (RwInt32*)&dlistSize, sizeof(dlistSize)))
    {
        return NULL;
    }

    *resEntryPointer = (RwResEntry*)RwMalloc(sizeof(RwResEntry) + headerSize + dlistSize + 31);

    vbHeader = (RxGameCubeVertexBuffer*)(*resEntryPointer + 1);
    if (RwStreamRead(stream, vbHeader, headerSize) != headerSize)
    {
        return NULL;
    }

    displayLists = (RwUInt8*)(((RwUInt32)vbHeader + headerSize + 31) & ~31);
    if (RwStreamRead(stream, displayLists, dlistSize) != dlistSize)
    {
        return NULL;
    }

    dList = (RxGameCubeDisplayList*)((RxGameCubeVertexAttr*)(vbHeader + 1) +
                                     (vbHeader->numAttrArrays - 1));
    _rpNativeOffset2Pointer(vbHeader, dList, numMeshes);

    (*resEntryPointer)->link.next = NULL;
    (*resEntryPointer)->link.prev = NULL;
    (*resEntryPointer)->owner = owner;
    (*resEntryPointer)->size = size;
    (*resEntryPointer)->ownerRef = resEntryPointer;
    (*resEntryPointer)->destroyNotify = _rxGCResEntryWaitDone;

    vbHeader->token = _RwDlTokenCurrent;

    DCFlushRange(*resEntryPointer + 1, (*resEntryPointer)->size);
    GXInvalidateVtxCache();

    return owner;
}

static RwStream* _rpNativeWrite(RwStream* stream, RwResEntry* resEntry, RwUInt32 numMeshes)
{
    RwPlatformID id;
    RwUInt32 chunkSize;
    RwUInt32 headerSize;
    RwUInt32 dListSize;
    RxGameCubeVertexBuffer* vbHeader;
    RxGameCubeDisplayList* dList;
    RwUInt8* baseAddress;
    RwUInt32 i;

    id = rwID_GAMECUBE;

    vbHeader = (RxGameCubeVertexBuffer*)(resEntry + 1);
    dList = (RxGameCubeDisplayList*)((RxGameCubeVertexAttr*)(vbHeader + 1) +
                                     (vbHeader->numAttrArrays - 1));

    chunkSize = _rpNativeSize(resEntry) - rwCHUNKHEADERSIZE;
    if (!RwStreamWriteVersionedChunkHeader(stream, rwID_STRUCT, chunkSize))
    {
        return NULL;
    }

    if (!RwStreamWriteInt32(stream, (RwInt32*)&id, sizeof(id)))
    {
        return NULL;
    }

    headerSize = sizeof(RxGameCubeVertexBuffer) +
                 (vbHeader->numAttrArrays - 1) * sizeof(RxGameCubeVertexAttr) +
                 numMeshes * sizeof(RxGameCubeDisplayList);
    if (!RwStreamWriteInt32(stream, (RwInt32*)&headerSize, sizeof(headerSize)))
    {
        return NULL;
    }

    dListSize = resEntry->size - headerSize - 31;
    if (!RwStreamWriteInt32(stream, (RwInt32*)&dListSize, sizeof(dListSize)))
    {
        return NULL;
    }

    /* Pointers are stored as offsets from the start of the display list data */
    baseAddress = NATIVEBASEADDRESS(dList, numMeshes);

    for (i = 0; i < vbHeader->numAttrArrays; i++)
    {
        vbHeader->attr[i].array = (void*)((RwUInt8*)vbHeader->attr[i].array - baseAddress);
    }

    for (i = 0; i < numMeshes; i++)
    {
        dList[i].displayList = (void*)((RwUInt8*)dList[i].displayList - baseAddress);
    }

    if (!RwStreamWrite(stream, vbHeader, headerSize))
    {
        return NULL;
    }

    _rpNativeOffset2Pointer(vbHeader, dList, numMeshes);

    if (!RwStreamWrite(stream, dList->displayList, dListSize))
    {
        return NULL;
    }

    DCFlushRange(resEntry + 1, resEntry->size);
    GXInvalidateVtxCache();

    return stream;
}

RwInt32 _rpGeometryNativeSize(const RpGeometry* geometry)
{
    if ((RpGeometryGetFlags(geometry) & rpGEOMETRYNATIVE) && geometry->repEntry != NULL)
    {
        return _rpNativeSize(geometry->repEntry);
    }

    return 0;
}

RwStream* _rpGeometryNativeWrite(RwStream* stream, const RpGeometry* geometry)
{
    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNATIVE)
    {
        _rpNativeWrite(stream, geometry->repEntry, geometry->mesh->numMeshes);
    }

    return stream;
}

RpGeometry* _rpGeometryNativeRead(RwStream* stream, RpGeometry* geometry)
{
    return (RpGeometry*)_rpNativeRead(stream, geometry, &geometry->repEntry,
                                      geometry->mesh->numMeshes);
}

RwInt32 _rpWorldSectorNativeSize(const RpWorldSector* sector)
{
    if ((RpWorldGetFlags(RpWorldSectorGetWorld(sector)) & rpWORLDNATIVE) && sector->repEntry != NULL)
    {
        return _rpNativeSize(sector->repEntry);
    }

    return 0;
}

RwStream* _rpWorldSectorNativeWrite(RwStream* stream, const RpWorldSector* sector)
{
    if (RpWorldGetFlags(RpWorldSectorGetWorld(sector)) & rpWORLDNATIVE)
    {
        _rpNativeWrite(stream, sector->repEntry, sector->mesh->numMeshes);
    }

    return stream;
}

RpWorldSector* _rpWorldSectorNativeRead(RwStream* stream, RpWorldSector* sector)
{
    return (RpWorldSector*)_rpNativeRead(stream, sector, &sector->repEntry,
                                         sector->mesh->numMeshes);
}
