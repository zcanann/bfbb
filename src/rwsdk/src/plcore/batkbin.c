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

#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define RwStreamWriteChunkHeader(stream, type, size)                                               \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, 0xFFFF)

extern RwBool _rwStreamReadChunkHeader(RwStream* stream, RwUInt32* type, RwUInt32* length,
                                       RwUInt32* version, RwUInt32* buildNum);

RwInt32 _rwPluginRegistryAddPluginStream(RwPluginRegistry* reg, RwUInt32 pluginID,
                                         RwPluginDataChunkReadCallBack readCB,
                                         RwPluginDataChunkWriteCallBack writeCB,
                                         RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry)
    {
        if (entry->pluginID == pluginID)
        {
            break;
        }

        entry = entry->nextRegEntry;
    }

    if (entry)
    {
        entry->readCB = readCB;
        entry->writeCB = writeCB;
        entry->getSizeCB = getSizeCB;

        return entry->offset;
    }

    return -1;
}

RwInt32 _rwPluginRegistryAddPlgnStrmlwysCB(RwPluginRegistry* reg, RwUInt32 pluginID,
                                           RwPluginDataChunkAlwaysCallBack alwaysCB)
{
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry)
    {
        if (entry->pluginID == pluginID)
        {
            break;
        }

        entry = entry->nextRegEntry;
    }

    if (entry)
    {
        entry->alwaysCB = alwaysCB;

        return entry->offset;
    }

    return -1;
}

RwInt32 _rwPluginRegistryAddPlgnStrmRightsCB(RwPluginRegistry* reg, RwUInt32 pluginID,
                                             RwPluginDataChunkRightsCallBack rightsCB)
{
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry)
    {
        if (entry->pluginID == pluginID)
        {
            break;
        }

        entry = entry->nextRegEntry;
    }

    if (entry)
    {
        entry->rightsCB = rightsCB;

        return entry->offset;
    }

    return -1;
}

const RwPluginRegistry* _rwPluginRegistryReadDataChunks(const RwPluginRegistry* reg,
                                                        RwStream* stream, void* object)
{
    RwUInt32 length;
    RwUInt32 version;
    RwPluginRegEntry* entry;
    RwUInt32 readType;
    RwUInt32 readLength;

    if (!RwStreamFindChunk(stream, rwID_EXTENSION, &length, &version))
    {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        while (length)
        {
            if (!_rwStreamReadChunkHeader(stream, &readType, &readLength, NULL, NULL))
            {
                return NULL;
            }

            entry = reg->firstRegEntry;
            while (entry)
            {
                if (entry->pluginID == readType)
                {
                    break;
                }

                entry = entry->nextRegEntry;
            }

            if (entry && entry->readCB)
            {
                if (!entry->readCB(stream, readLength, object, entry->offset, entry->size))
                {
                    return NULL;
                }
            }
            else
            {
                /* Nobody claims this chunk, so skip it */
                if (!RwStreamSkip(stream, readLength))
                {
                    return NULL;
                }
            }

            length -= readLength + rwCHUNKHEADERSIZE;
        }

        entry = reg->firstRegEntry;
        while (entry)
        {
            if (entry->alwaysCB)
            {
                if (!entry->alwaysCB(object, entry->offset, entry->size))
                {
                    return NULL;
                }
            }

            entry = entry->nextRegEntry;
        }

        return reg;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

const RwPluginRegistry* _rwPluginRegistryInvokeRights(const RwPluginRegistry* reg, RwUInt32 id,
                                                      void* obj, RwUInt32 extraData)
{
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry)
    {
        if (entry->pluginID == id)
        {
            break;
        }

        entry = entry->nextRegEntry;
    }

    if (entry && entry->rightsCB)
    {
        if (entry->rightsCB(obj, entry->offset, entry->size, extraData))
        {
            return reg;
        }

        return NULL;
    }

    return NULL;
}

RwInt32 _rwPluginRegistryGetSize(const RwPluginRegistry* reg, const void* object)
{
    RwInt32 size = 0;
    RwPluginRegEntry* entry = reg->firstRegEntry;

    while (entry)
    {
        if (entry->getSizeCB)
        {
            RwInt32 thisSize = entry->getSizeCB(object, entry->offset, entry->size);

            if (thisSize > 0)
            {
                size += thisSize + rwCHUNKHEADERSIZE;
            }
        }

        entry = entry->nextRegEntry;
    }

    return size;
}

const RwPluginRegistry* _rwPluginRegistryWriteDataChunks(const RwPluginRegistry* reg,
                                                         RwStream* stream, const void* object)
{
    RwPluginRegEntry* entry;
    RwInt32 size;

    size = _rwPluginRegistryGetSize(reg, object);

    if (!RwStreamWriteChunkHeader(stream, rwID_EXTENSION, size))
    {
        return NULL;
    }

    entry = reg->firstRegEntry;
    while (entry)
    {
        if (entry->getSizeCB && entry->writeCB)
        {
            size = entry->getSizeCB(object, entry->offset, entry->size);

            if (size > 0)
            {
                if (!RwStreamWriteChunkHeader(stream, entry->pluginID, size))
                {
                    return NULL;
                }

                if (!entry->writeCB(stream, size, object, entry->offset, entry->size))
                {
                    return NULL;
                }
            }
        }

        entry = entry->nextRegEntry;
    }

    return reg;
}

const RwPluginRegistry* _rwPluginRegistrySkipDataChunks(const RwPluginRegistry* reg,
                                                        RwStream* stream)
{
    RwUInt32 length;
    RwUInt32 readLength;

    if (!RwStreamFindChunk(stream, rwID_EXTENSION, &length, NULL))
    {
        return NULL;
    }

    while (length)
    {
        if (!_rwStreamReadChunkHeader(stream, NULL, &readLength, NULL, NULL))
        {
            return NULL;
        }

        if (!RwStreamSkip(stream, readLength))
        {
            return NULL;
        }

        length -= readLength + rwCHUNKHEADERSIZE;
    }

    return reg;
}
