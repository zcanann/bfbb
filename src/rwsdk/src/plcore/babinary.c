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

#define E_RW_BADVERSION 0x80000004
#define E_RW_READ 0x8000001A

/* Library ID packing: version in the top 16 bits, build number in the bottom 16 */
#define RWLIBRARYIDPACK(_version, _buildNum)                                                       \
    ((((((_version) - 0x30000) & 0x3FF00) << 14) | (((_version) & 0x3F) << 16) |                  \
      ((_buildNum) & 0xFFFF)))

#define RWLIBRARYIDUNPACKVERSION(_libraryID)                                                       \
    (((((_libraryID) >> 14) & 0x3FF00) + 0x30000) | (((_libraryID) >> 16) & 0x3F))

#define RWLIBRARYIDUNPACKBUILDNUM(_libraryID) ((_libraryID) & 0xFFFF)

#define rwSTREAMCONVERTBUFFERSIZE 256

typedef struct _rwMark _rwMark;
struct _rwMark
{
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 libraryID;
};

static RwBool ChunkIsComplex(RwChunkHeaderInfo* chunkHeaderInfo)
{
    RwBool result;

    switch (chunkHeaderInfo->type)
    {
    case rwID_STRUCT:
        result = FALSE;
        break;
    case rwID_STRING:
        result = FALSE;
        break;
    case rwID_EXTENSION:
        result = FALSE;
        break;
    case rwID_CAMERA:
        result = TRUE;
        break;
    case rwID_TEXTURE:
        result = TRUE;
        break;
    case rwID_MATERIAL:
        result = TRUE;
        break;
    case rwID_MATLIST:
        result = TRUE;
        break;
    case rwID_ATOMICSECT:
        result = TRUE;
        break;
    case rwID_PLANESECT:
        result = TRUE;
        break;
    case rwID_WORLD:
        result = TRUE;
        break;
    case rwID_MATRIX:
        result = FALSE;
        break;
    case rwID_FRAMELIST:
        result = TRUE;
        break;
    case rwID_GEOMETRY:
        result = TRUE;
        break;
    case rwID_CLUMP:
        result = TRUE;
        break;
    case rwID_LIGHT:
        result = TRUE;
        break;
    case rwID_UNICODESTRING:
        result = FALSE;
        break;
    case rwID_ATOMIC:
        result = TRUE;
        break;
    case rwID_GEOMETRYLIST:
        result = TRUE;
        break;
    default:
        result = FALSE;
        break;
    }

    return result;
}

RwBool _rwStreamReadChunkHeader(RwStream* stream, RwUInt32* type, RwUInt32* length,
                                RwUInt32* version, RwUInt32* buildNum)
{
    RwBool status;
    _rwMark mark;

    status = (RwStreamRead(stream, &mark, sizeof(mark)) == sizeof(mark));
    if (!status)
    {
        RWERROR((E_RW_READ));
        return FALSE;
    }

    RwMemNative32(&mark, sizeof(mark));

    if (type)
    {
        *type = mark.type;
    }

    if (length)
    {
        *length = mark.length;
    }

    if (!(mark.libraryID & 0xFFFF0000))
    {
        /* Old style chunk: no build number */
        if (version)
        {
            *version = mark.libraryID << 8;
        }

        if (buildNum)
        {
            *buildNum = 0;
        }
    }
    else
    {
        if (version)
        {
            *version = RWLIBRARYIDUNPACKVERSION(mark.libraryID);
        }

        if (buildNum)
        {
            *buildNum = RWLIBRARYIDUNPACKBUILDNUM(mark.libraryID);
        }
    }

    return TRUE;
}

RwStream* _rwStreamWriteVersionedChunkHeader(RwStream* stream, RwInt32 type, RwInt32 size,
                                             RwUInt32 version, RwUInt32 buildNum)
{
    _rwMark mark;

    mark.type = type;
    mark.length = size;
    mark.libraryID = RWLIBRARYIDPACK(version, buildNum);

    RwMemLittleEndian32(&mark, sizeof(mark));

    return RwStreamWrite(stream, &mark, sizeof(mark));
}

RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* lengthOut,
                         RwUInt32* versionOut)
{
    RwUInt32 readType;
    RwUInt32 readLength;
    RwUInt32 readVersion;

    while (_rwStreamReadChunkHeader(stream, &readType, &readLength, &readVersion, NULL))
    {
        if (readType == type)
        {
            if (readVersion < rwLIBRARYBASEVERSION)
            {
                RWERROR((E_RW_BADVERSION));
                return FALSE;
            }

            if (readVersion > rwLIBRARYCURRENTVERSION)
            {
                RWERROR((E_RW_BADVERSION));
                return FALSE;
            }

            if (readVersion <= rwLIBRARYWARNVERSION)
            {
                RWERROR((E_RW_BADVERSION));
            }

            if (lengthOut)
            {
                *lengthOut = readLength;
            }

            if (versionOut)
            {
                *versionOut = readVersion;
            }

            return TRUE;
        }

        if (!RwStreamSkip(stream, readLength))
        {
            return FALSE;
        }
    }

    return FALSE;
}

void* RwMemLittleEndian32(void* mem, RwUInt32 size)
{
    RwUInt32* memInt = (RwUInt32*)mem;

    size >>= 2;
    while (size--)
    {
        *memInt = ((*memInt >> 24) & 0xFF) | ((*memInt >> 8) & 0xFF00) | ((*memInt << 8) & 0xFF0000) |
                  ((*memInt << 24) & 0xFF000000);
        memInt++;
    }

    return mem;
}

void* RwMemLittleEndian16(void* mem, RwUInt32 size)
{
    RwUInt16* memInt = (RwUInt16*)mem;

    size >>= 1;
    while (size--)
    {
        *memInt = (RwUInt16)((*memInt >> 8) | (*memInt << 8));
        memInt++;
    }

    return mem;
}

void* RwMemNative32(void* mem, RwUInt32 size)
{
    RwUInt32* memInt = (RwUInt32*)mem;

    size >>= 2;
    while (size--)
    {
        *memInt = ((*memInt >> 24) & 0xFF) | ((*memInt >> 8) & 0xFF00) | ((*memInt << 8) & 0xFF0000) |
                  ((*memInt << 24) & 0xFF000000);
        memInt++;
    }

    return mem;
}

void* RwMemNative16(void* mem, RwUInt32 size)
{
    RwUInt16* memInt = (RwUInt16*)mem;

    size >>= 1;
    while (size--)
    {
        *memInt = (RwUInt16)((*memInt >> 8) | (*memInt << 8));
        memInt++;
    }

    return mem;
}

RwStream* RwStreamWriteReal(RwStream* stream, const RwReal* reals, RwUInt32 numBytes)
{
    RwUInt8 convertBuffer[rwSTREAMCONVERTBUFFERSIZE];

    while (numBytes)
    {
        RwUInt32 bytesToWrite =
            (numBytes < rwSTREAMCONVERTBUFFERSIZE) ? numBytes : rwSTREAMCONVERTBUFFERSIZE;

        memcpy(convertBuffer, reals, bytesToWrite);
        RwMemLittleEndian32(convertBuffer, bytesToWrite);

        if (!RwStreamWrite(stream, convertBuffer, bytesToWrite))
        {
            return NULL;
        }

        numBytes -= bytesToWrite;
        reals = (const RwReal*)((const RwUInt8*)reals + bytesToWrite);
    }

    return stream;
}

RwStream* RwStreamWriteInt32(RwStream* stream, const RwInt32* ints, RwUInt32 numBytes)
{
    RwUInt8 convertBuffer[rwSTREAMCONVERTBUFFERSIZE];

    while (numBytes)
    {
        RwUInt32 bytesToWrite =
            (numBytes < rwSTREAMCONVERTBUFFERSIZE) ? numBytes : rwSTREAMCONVERTBUFFERSIZE;

        memcpy(convertBuffer, ints, bytesToWrite);
        RwMemLittleEndian32(convertBuffer, bytesToWrite);

        if (!RwStreamWrite(stream, convertBuffer, bytesToWrite))
        {
            return NULL;
        }

        numBytes -= bytesToWrite;
        ints = (const RwInt32*)((const RwUInt8*)ints + bytesToWrite);
    }

    return stream;
}

RwStream* RwStreamReadReal(RwStream* stream, RwReal* reals, RwUInt32 numBytes)
{
    if (!RwStreamRead(stream, reals, numBytes))
    {
        RWERROR((E_RW_READ));
        return NULL;
    }

    RwMemNative32(reals, numBytes);

    return stream;
}

RwStream* RwStreamReadInt32(RwStream* stream, RwInt32* ints, RwUInt32 numBytes)
{
    if (!RwStreamRead(stream, ints, numBytes))
    {
        RWERROR((E_RW_READ));
        return NULL;
    }

    RwMemNative32(ints, numBytes);

    return stream;
}

RwStream* RwStreamReadChunkHeaderInfo(RwStream* stream, RwChunkHeaderInfo* chunkHeaderInfo)
{
    RwUInt32 readType;
    RwUInt32 readLength;
    RwUInt32 readVersion;
    RwUInt32 readBuildNum;

    if (!_rwStreamReadChunkHeader(stream, &readType, &readLength, &readVersion, &readBuildNum))
    {
        return NULL;
    }

    chunkHeaderInfo->type = readType;
    chunkHeaderInfo->length = readLength;
    chunkHeaderInfo->version = readVersion;
    chunkHeaderInfo->buildNum = readBuildNum;
    chunkHeaderInfo->isComplex = ChunkIsComplex(chunkHeaderInfo);

    return stream;
}
