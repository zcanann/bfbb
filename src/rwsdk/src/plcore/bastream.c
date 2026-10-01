#include <string.h>
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

#define E_RW_BADOPEN 0x80000002
#define E_RW_ENDOFSTREAM 0x5
#define E_RW_INVSTREAMACCESSTYPE 0xD
#define E_RW_INVSTREAMTYPE 0xE
#define E_RW_NOMEM 0x80000013
#define E_RW_READ 0x8000001A
#define E_RW_WRITE 0x8000001C

#define rwSTREAMGROWSIZE 512

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

#define RwFopen(_name, _mode) (RWSRCGLOBAL(fileFuncs).rwfopen((_name), (_mode)))
#define RwFclose(_fptr) (RWSRCGLOBAL(fileFuncs).rwfclose((_fptr)))
#define RwFread(_addr, _size, _count, _fptr)                                                       \
    (RWSRCGLOBAL(fileFuncs).rwfread((_addr), (_size), (_count), (_fptr)))
#define RwFwrite(_addr, _size, _count, _fptr)                                                      \
    (RWSRCGLOBAL(fileFuncs).rwfwrite((_addr), (_size), (_count), (_fptr)))
#define RwFeof(_fptr) (RWSRCGLOBAL(fileFuncs).rwfeof((_fptr)))
#define RwFseek(_fptr, _offset, _origin)                                                           \
    (RWSRCGLOBAL(fileFuncs).rwfseek((_fptr), (_offset), (_origin)))
#define RwFtell(_fptr) (RWSRCGLOBAL(fileFuncs).rwftell((_fptr)))

#define RwRealloc(_p, _s) ((RWSRCGLOBAL(memoryFuncs).rwrealloc)((_p), (_s)))

#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rwStreamGlobals rwStreamGlobals;
struct rwStreamGlobals
{
    RwFreeList* streamFreeList;
};

#define RWSTREAMGLOBAL(var)                                                                        \
    (RWPLUGINOFFSET(rwStreamGlobals, RwEngineInstance, streamModule.globalsOffset)->var)

static RwFreeList _rwStreamFreeList;
static RwModuleInfo streamModule;
static RwInt32 _rwStreamFreeListBlockSize = 16;
static RwInt32 _rwStreamFreeListPreallocBlocks = 1;

void* _rwStreamModuleOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    streamModule.globalsOffset = offset;

    RWSTREAMGLOBAL(streamFreeList) =
        RwFreeListCreateAndPreallocateSpace(sizeof(RwStream), _rwStreamFreeListBlockSize,
                                            sizeof(RwUInt32), _rwStreamFreeListPreallocBlocks,
                                            &_rwStreamFreeList);
    if (!RWSTREAMGLOBAL(streamFreeList))
    {
        return NULL;
    }

    streamModule.numInstances++;

    return instance;
}

void* _rwStreamModuleClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (RWSTREAMGLOBAL(streamFreeList))
    {
        RwFreeListDestroy(RWSTREAMGLOBAL(streamFreeList));
    }

    streamModule.numInstances--;

    return instance;
}

static RwStream* StreamFileInitialize(RwStream* stream, void* pData)
{
    if (RwFtell(pData) == -1)
    {
        return NULL;
    }

    stream->Type.file.fpFile = pData;

    return stream;
}

static RwStream* StreamFileNameInitialize(RwStream* stream, RwStreamAccessType accessType,
                                          const void* pData)
{
    RwStream* result = NULL;
    void* fp = NULL;
    const RwChar* cpFile = (const RwChar*)pData;

    switch (accessType)
    {
    case rwSTREAMREAD:
        fp = RwFopen(cpFile, "rb");
        break;
    case rwSTREAMWRITE:
        fp = RwFopen(cpFile, "wb");
        break;
    case rwSTREAMAPPEND:
        fp = RwFopen(cpFile, "ab");
        break;
    default:
        RWERROR((E_RW_INVSTREAMACCESSTYPE));
        break;
    }

    if (fp)
    {
        stream->Type.file.fpFile = fp;
        result = stream;
    }
    else
    {
        RWERROR((E_RW_BADOPEN, cpFile));
    }

    return result;
}

static RwStream* StreamMemoryInitialize(RwStream* stream, RwStreamAccessType accessType,
                                        const void* pData)
{
    RwStream* result = NULL;
    const RwMemory* mepMem = (const RwMemory*)pData;

    switch (accessType)
    {
    case rwSTREAMREAD:
        stream->Type.memory.position = 0;
        stream->Type.memory.nSize = mepMem->length;
        stream->Type.memory.memBlock = mepMem->start;
        result = stream;
        break;
    case rwSTREAMWRITE:
        stream->Type.memory.position = 0;
        stream->Type.memory.nSize = 0;
        stream->Type.memory.memBlock = NULL;
        result = stream;
        break;
    case rwSTREAMAPPEND:
        stream->Type.memory.position = mepMem->length;
        stream->Type.memory.nSize = mepMem->length;
        stream->Type.memory.memBlock = mepMem->start;
        result = stream;
        break;
    default:
        RWERROR((E_RW_INVSTREAMACCESSTYPE));
        break;
    }

    return result;
}

static RwStream* StreamCustomInitialize(RwStream* stream, const void* pData)
{
    memcpy(&stream->Type.custom, pData, sizeof(RwStreamCustom));

    return stream;
}

RwStream* _rwStreamInitialize(RwStream* stream, RwBool rwOwned, RwStreamType type,
                              RwStreamAccessType accessType, const void* pData)
{
    RwStream* result = NULL;

    if (!stream)
    {
        return NULL;
    }

    stream->type = type;
    stream->accessType = accessType;
    stream->rwOwned = rwOwned;

    switch (type)
    {
    case rwSTREAMFILE:
        result = StreamFileInitialize(stream, (void*)pData);
        break;
    case rwSTREAMFILENAME:
        result = StreamFileNameInitialize(stream, accessType, pData);
        break;
    case rwSTREAMMEMORY:
        result = StreamMemoryInitialize(stream, accessType, pData);
        break;
    case rwSTREAMCUSTOM:
        result = StreamCustomInitialize(stream, pData);
        break;
    default:
        RWERROR((E_RW_INVSTREAMTYPE));
        break;
    }

    return result;
}

RwUInt32 RwStreamRead(RwStream* stream, void* buffer, RwUInt32 length)
{
    switch (stream->type)
    {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME:
    {
        void* fp = stream->Type.file.fpFile;
        RwUInt32 nBytesRead;

        nBytesRead = RwFread(buffer, 1, length, fp);
        if (nBytesRead != length)
        {
            if (RwFeof(fp))
            {
                RWERROR((E_RW_ENDOFSTREAM));
            }
            else
            {
                RWERROR((E_RW_READ));
            }
        }

        return nBytesRead;
    }
    case rwSTREAMMEMORY:
    {
        RwStreamMemory* smpMem = &stream->Type.memory;

        if (length > smpMem->nSize - smpMem->position)
        {
            length = smpMem->nSize - smpMem->position;
            RWERROR((E_RW_ENDOFSTREAM));
        }

        memcpy(buffer, smpMem->memBlock + smpMem->position, length);
        smpMem->position += length;

        return length;
    }
    case rwSTREAMCUSTOM:
        return stream->Type.custom.sfnread(stream->Type.custom.data, buffer, length);
    default:
        RWERROR((E_RW_INVSTREAMTYPE));
        return 0;
    }
}

RwStream* RwStreamWrite(RwStream* stream, const void* buffer, RwUInt32 length)
{
    switch (stream->type)
    {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME:
    {
        void* fp = stream->Type.file.fpFile;
        RwUInt32 nBytesWritten;

        nBytesWritten = RwFwrite(buffer, 1, length, fp);
        if (nBytesWritten != length)
        {
            RWERROR((E_RW_WRITE));
            return NULL;
        }

        return stream;
    }
    case rwSTREAMMEMORY:
    {
        RwStreamMemory* smpMem = &stream->Type.memory;

        if (!smpMem->memBlock)
        {
            smpMem->memBlock = (RwUInt8*)RwMalloc(rwSTREAMGROWSIZE);
            if (!smpMem->memBlock)
            {
                RWERROR((E_RW_NOMEM, rwSTREAMGROWSIZE));
                return NULL;
            }

            smpMem->nSize = rwSTREAMGROWSIZE;
        }

        if (smpMem->nSize - smpMem->position < length)
        {
            RwUInt32 nAllocSize;
            void* pData;

            if (length < rwSTREAMGROWSIZE)
            {
                nAllocSize = smpMem->nSize + rwSTREAMGROWSIZE;
            }
            else
            {
                nAllocSize = length + smpMem->nSize;
            }

            pData = RwRealloc(smpMem->memBlock, nAllocSize);
            if (!pData)
            {
                RWERROR((E_RW_NOMEM, nAllocSize - smpMem->nSize));
                return NULL;
            }

            smpMem->memBlock = (RwUInt8*)pData;
            smpMem->nSize = nAllocSize;
        }

        memcpy(smpMem->memBlock + smpMem->position, buffer, length);
        smpMem->position += length;

        return stream;
    }
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnwrite(stream->Type.custom.data, buffer, length))
        {
            return stream;
        }

        return NULL;
    default:
        RWERROR((E_RW_INVSTREAMTYPE));
        return NULL;
    }
}

RwStream* RwStreamSkip(RwStream* stream, RwUInt32 offset)
{
    if (!offset)
    {
        return stream;
    }

    switch (stream->type)
    {
    case rwSTREAMFILE:
    case rwSTREAMFILENAME:
    {
        void* fp = stream->Type.file.fpFile;

        if (RwFseek(fp, offset, SEEK_CUR))
        {
            if (RwFeof(fp))
            {
                RWERROR((E_RW_ENDOFSTREAM));
            }

            return NULL;
        }

        return stream;
    }
    case rwSTREAMMEMORY:
    {
        RwStreamMemory* smpMem = &stream->Type.memory;

        if (smpMem->position + offset > smpMem->nSize)
        {
            smpMem->position = smpMem->nSize;
            RWERROR((E_RW_ENDOFSTREAM));
            return NULL;
        }

        smpMem->position += offset;

        return stream;
    }
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnskip(stream->Type.custom.data, offset))
        {
            return stream;
        }

        return NULL;
    default:
        RWERROR((E_RW_INVSTREAMTYPE));
        return NULL;
    }
}

RwBool RwStreamClose(RwStream* stream, void* pData)
{
    RwBool result;

    switch (stream->type)
    {
    case rwSTREAMFILE:
        result = TRUE;
        break;
    case rwSTREAMFILENAME:
        result = !RwFclose(stream->Type.file.fpFile);
        break;
    case rwSTREAMMEMORY:
        if (stream->accessType != rwSTREAMREAD && pData)
        {
            RwMemory* mepMem = (RwMemory*)pData;

            mepMem->start = stream->Type.memory.memBlock;
            mepMem->length = stream->Type.memory.position;
        }

        result = TRUE;
        break;
    case rwSTREAMCUSTOM:
        if (stream->Type.custom.sfnclose)
        {
            stream->Type.custom.sfnclose(stream->Type.custom.data);
        }

        result = TRUE;
        break;
    default:
        RWERROR((E_RW_INVSTREAMTYPE));
        return FALSE;
    }

    if (stream->rwOwned)
    {
        RwFreeListFree(RWSTREAMGLOBAL(streamFreeList), stream);
    }

    return result;
}

RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType accessType, const void* pData)
{
    RwStream* stream;

    stream = (RwStream*)RwFreeListAlloc(RWSTREAMGLOBAL(streamFreeList));
    if (!_rwStreamInitialize(stream, TRUE, type, accessType, pData))
    {
        RwFreeListFree(RWSTREAMGLOBAL(streamFreeList), stream);
        stream = NULL;
    }

    return stream;
}
