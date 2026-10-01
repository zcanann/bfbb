#include <string.h>
#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

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
#define E_RW_NOMEM 0x80000013

#define rwSTANDARDNATIVETEXTUREREAD 26

#define rwTEXTURESTREAMFLAGSUSERMIPMAPS 0x01

#define rwSTRINGSTREAMBUFFERSIZE 64

#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define RwStreamWriteChunkHeader(stream, type, size)                                               \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, 0xFFFF)

typedef struct _rwStreamTexture _rwStreamTexture;
struct _rwStreamTexture
{
    RwUInt32 filterAndAddress;
};

typedef struct _rwStreamTexDictionary _rwStreamTexDictionary;
struct _rwStreamTexDictionary
{
    RwInt32 numTextures;
};

extern RwBool _rwStreamReadChunkHeader(RwStream* stream, RwUInt32* type, RwUInt32* length,
                                       RwUInt32* version, RwUInt32* buildNum);

extern RwPluginRegistry textureTKList;
extern RwPluginRegistry texDictTKList;

static const RwChar nullString[] = "";

RwUInt32 _rwStringStreamGetSize(const RwChar* string)
{
    RwUInt32 stringSize;

    if (!string)
    {
        string = nullString;
    }

    /* Include the terminator and pad to a multiple of 4 bytes */
    stringSize = rwstrlen(string) + sizeof(RwChar);
    stringSize = (stringSize + 3) & ~3;

    return stringSize;
}

const RwChar* _rwStringStreamWrite(const RwChar* string, RwStream* stream)
{
    RwUInt32 stringSize;

    if (!string)
    {
        string = nullString;
    }

    stringSize = _rwStringStreamGetSize(string);

    if (!RwStreamWriteChunkHeader(stream, rwID_STRING, stringSize))
    {
        return NULL;
    }

    if (!RwStreamWrite(stream, string, stringSize))
    {
        return NULL;
    }

    return string;
}

static RwChar* StringStreamRead(RwChar* nativeString, RwStream* stream, RwUInt32 length)
{
    RwChar multiByteString[rwSTRINGSTREAMBUFFERSIZE];
    RwChar* baseString;

    if (!nativeString)
    {
        nativeString = (RwChar*)RwMalloc(length);
        if (!nativeString)
        {
            RWERROR((E_RW_NOMEM, length));
            return NULL;
        }
    }

    baseString = nativeString;
    while (length > 0)
    {
        RwUInt32 bytesToRead;
        RwUInt32 i;

        bytesToRead = (length > rwSTRINGSTREAMBUFFERSIZE) ? rwSTRINGSTREAMBUFFERSIZE : length;

        if (RwStreamRead(stream, multiByteString, bytesToRead) != bytesToRead)
        {
            return NULL;
        }

        length -= bytesToRead;

        for (i = 0; i < bytesToRead; i++)
        {
            baseString[i] = multiByteString[i];
        }

        baseString += bytesToRead;
    }

    return nativeString;
}

static RwChar* UnicodeStringStreamRead(RwChar* nativeString, RwStream* stream, RwUInt32 length)
{
    RwUInt16 uniCodeString[rwSTRINGSTREAMBUFFERSIZE];
    RwChar* baseString;
    RwBool mallocced = FALSE;

    if (!nativeString)
    {
        nativeString = (RwChar*)RwMalloc(length);
        if (!nativeString)
        {
            RWERROR((E_RW_NOMEM, length));
            return NULL;
        }

        mallocced = TRUE;
    }

    baseString = nativeString;
    while (length > 0)
    {
        RwUInt32 bytesToRead;
        RwUInt32 i;
        RwUInt32 CharCount;

        bytesToRead = (length > sizeof(uniCodeString)) ? sizeof(uniCodeString) : length;

        if (RwStreamRead(stream, uniCodeString, bytesToRead) != bytesToRead)
        {
            if (mallocced)
            {
                RwFree(nativeString);
            }

            return NULL;
        }

        CharCount = bytesToRead >> 1;
        length -= bytesToRead;

        /* Only the low byte of each character survives */
        for (i = 0; i < CharCount; i++)
        {
            baseString[i] = (RwChar)uniCodeString[i];
        }

        baseString += CharCount;
    }

    return nativeString;
}

RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream)
{
    RwUInt32 type;
    RwUInt32 length;
    RwUInt32 version;

    while (_rwStreamReadChunkHeader(stream, &type, &length, &version, NULL))
    {
        RwBool valid = (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION);

        if (!valid)
        {
            RWERROR((E_RW_BADVERSION));
            return NULL;
        }

        if (type == rwID_STRING)
        {
            return StringStreamRead(string, stream, length);
        }

        if (type == rwID_UNICODESTRING)
        {
            return UnicodeStringStreamRead(string, stream, length);
        }

        if (!RwStreamSkip(stream, length))
        {
            return NULL;
        }
    }

    return NULL;
}

RwInt32 RwTextureRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                     RwPluginDataChunkWriteCallBack writeCB,
                                     RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPluginStream(&textureTKList, pluginID, readCB, writeCB, getSizeCB);

    return plug;
}

RwUInt32 RwTextureStreamGetSize(const RwTexture* texture)
{
    RwUInt32 size;

    size = rwCHUNKHEADERSIZE + sizeof(_rwStreamTexture) + rwCHUNKHEADERSIZE +
           _rwStringStreamGetSize(texture->name);
    size += rwCHUNKHEADERSIZE + _rwStringStreamGetSize(texture->mask);
    size += rwCHUNKHEADERSIZE + _rwPluginRegistryGetSize(&textureTKList, texture);

    return size;
}

const RwTexture* RwTextureStreamWrite(const RwTexture* texture, RwStream* stream)
{
    RwUInt32 flags;
    _rwStreamTexture texFiltAddr;

    if (!RwStreamWriteChunkHeader(stream, rwID_TEXTURE, RwTextureStreamGetSize(texture)))
    {
        return NULL;
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, sizeof(_rwStreamTexture)))
    {
        return NULL;
    }

    /* Mipmaps that were not generated automatically must be stored */
    if (texture->raster && !(texture->raster->cFormat & (rwRASTERFORMATAUTOMIPMAP >> 8)))
    {
        flags = rwTEXTURESTREAMFLAGSUSERMIPMAPS;
    }
    else
    {
        flags = 0;
    }

    texFiltAddr.filterAndAddress =
        (texture->filterAddressing & 0xFFFF) | ((flags & 0xFF) << 16);

    RwMemLittleEndian32(&texFiltAddr, sizeof(texFiltAddr));

    if (!RwStreamWrite(stream, &texFiltAddr, sizeof(texFiltAddr)))
    {
        return NULL;
    }

    if (!_rwStringStreamWrite(texture->name, stream))
    {
        return NULL;
    }

    if (!_rwStringStreamWrite(texture->mask, stream))
    {
        return NULL;
    }

    if (_rwPluginRegistryWriteDataChunks(&textureTKList, stream, texture))
    {
        return texture;
    }

    return NULL;
}

RwTexture* RwTextureStreamRead(RwStream* stream)
{
    RwUInt32 size;
    RwUInt32 version;
    RwTexture* texture;
    RwChar textureName[128];
    RwChar textureMask[128];
    RwTextureFilterMode filtering;
    RwTextureAddressMode addressingU;
    RwTextureAddressMode addressingV;
    _rwStreamTexture texFiltAddr;
    RwBool mipmapState;
    RwBool autoMipmapState;
    RwUInt32 flags;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        memset(&texFiltAddr, 0, sizeof(texFiltAddr));
        if (RwStreamRead(stream, &texFiltAddr, size) != size)
        {
            return NULL;
        }

        RwMemNative32(&texFiltAddr, sizeof(texFiltAddr));

        filtering = (RwTextureFilterMode)(texFiltAddr.filterAndAddress & 0xFF);
        addressingU = (RwTextureAddressMode)((texFiltAddr.filterAndAddress >> 8) & 0x0F);
        addressingV = (RwTextureAddressMode)((texFiltAddr.filterAndAddress >> 12) & 0x0F);

        /* Older files only stored one addressing mode */
        if (addressingV == rwTEXTUREADDRESSNATEXTUREADDRESS)
        {
            texFiltAddr.filterAndAddress |= (addressingU & 0x0F) << 12;
        }

        flags = (texFiltAddr.filterAndAddress >> 16) & 0xFF;

        mipmapState = RwTextureGetMipmapping();
        autoMipmapState = RwTextureGetAutoMipmapping();

        if (filtering == rwFILTERMIPNEAREST || filtering == rwFILTERMIPLINEAR ||
            filtering == rwFILTERLINEARMIPNEAREST || filtering == rwFILTERLINEARMIPLINEAR)
        {
            RwTextureSetMipmapping(TRUE);

            if (flags & rwTEXTURESTREAMFLAGSUSERMIPMAPS)
            {
                RwTextureSetAutoMipmapping(FALSE);
            }
            else
            {
                RwTextureSetAutoMipmapping(TRUE);
            }
        }
        else
        {
            RwTextureSetMipmapping(FALSE);
            RwTextureSetAutoMipmapping(FALSE);
        }

        if (!_rwStringStreamFindAndRead(textureName, stream))
        {
            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);
            return NULL;
        }

        if (!_rwStringStreamFindAndRead(textureMask, stream))
        {
            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);
            return NULL;
        }

        texture = RwTextureRead(textureName, textureMask);
        if (!texture)
        {
            _rwPluginRegistrySkipDataChunks(&textureTKList, stream);
            RwTextureSetMipmapping(mipmapState);
            RwTextureSetAutoMipmapping(autoMipmapState);
            return NULL;
        }

        /* Only a newly loaded texture takes the streamed settings */
        if (texture->refCount == 1)
        {
            texture->filterAddressing = texFiltAddr.filterAndAddress & 0xFFFF;
        }

        RwTextureSetMipmapping(mipmapState);
        RwTextureSetAutoMipmapping(autoMipmapState);

        if (!_rwPluginRegistryReadDataChunks(&textureTKList, stream, texture))
        {
            return NULL;
        }

        return texture;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}

static RwTexture* destroyTexture(RwTexture* texture, void* data)
{
    RwTextureDestroy(texture);

    return texture;
}

RwTexDictionary* RwTexDictionaryStreamRead(RwStream* stream)
{
    RwUInt32 size;
    RwUInt32 version;
    RwTexDictionary* texDict;
    _rwStreamTexDictionary binTexDict;
    RwTexture* texture;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        memset(&binTexDict, 0, sizeof(binTexDict));
        if (RwStreamRead(stream, &binTexDict, size) != size)
        {
            return NULL;
        }

        RwMemNative32(&binTexDict, sizeof(binTexDict));

        texDict = RwTexDictionaryCreate();
        if (!texDict)
        {
            return NULL;
        }

        while (binTexDict.numTextures--)
        {
            if (!RwStreamFindChunk(stream, rwID_TEXTURENATIVE, &size, &version))
            {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                return NULL;
            }

            if (version < rwLIBRARYBASEVERSION || version > rwLIBRARYCURRENTVERSION)
            {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                RWERROR((E_RW_BADVERSION));
                return NULL;
            }

            if (!RWSRCGLOBAL(stdFunc[rwSTANDARDNATIVETEXTUREREAD])(stream, &texture, size))
            {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                return NULL;
            }

            if (!texture)
            {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                return NULL;
            }

            if (!_rwPluginRegistryReadDataChunks(&textureTKList, stream, texture))
            {
                RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
                RwTexDictionaryDestroy(texDict);
                return NULL;
            }

            RwTexDictionaryAddTexture(texDict, texture);
        }

        if (!_rwPluginRegistryReadDataChunks(&texDictTKList, stream, texDict))
        {
            RwTexDictionaryForAllTextures(texDict, destroyTexture, NULL);
            RwTexDictionaryDestroy(texDict);
            return NULL;
        }

        return texDict;
    }

    RWERROR((E_RW_BADVERSION));
    return NULL;
}
