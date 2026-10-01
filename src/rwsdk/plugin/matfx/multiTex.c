#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <string.h>

#include "rwsdk/plugin/matfx/mtprivate.h"

#define rwPLUGIN_ID rwID_MULTITEXPLUGIN

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define rpMULTITEXTUREEFFECTFLAG 0x01

typedef struct _MultiTextureExt _MultiTextureExt;
struct _MultiTextureExt
{
    RpMultiTexture* multiTexture;
};

typedef struct _MultiTextureStreamHdr _MultiTextureStreamHdr;
struct _MultiTextureStreamHdr
{
    RwUInt8 platformID;
    RwUInt8 numTextures;
    RwUInt8 flags;
    RwUInt8 pad;
};

#define MULTITEXTUREEXTFROMOBJECT(object, offset)                                                  \
    ((_MultiTextureExt*)(((RwUInt8*)(object)) + (offset)))

#define MULTITEXTUREEXTFROMCONSTOBJECT(object, offset)                                             \
    ((const _MultiTextureExt*)(((const RwUInt8*)(object)) + (offset)))

RwModuleInfo _rpMultiTextureModule = { 0, 0 };

rpMultiTextureRegEntry RegEntries[rpMULTITEXTURENUMPLATFORMS];

static void* MultiTextureOpen(void* object, RwInt32 offset, RwInt32 size)
{
    _rpMultiTextureModule.numInstances++;

    _rpMTEffectOpen();

    return object;
}

static void* MultiTextureClose(void* object, RwInt32 offset, RwInt32 size)
{
    _rpMTEffectClose();

    _rpMultiTextureModule.numInstances--;

    return object;
}

static RpMultiTexture* MultiTextureCreate(rpMultiTextureRegEntry* regEntry, RwUInt32 numTextures)
{
    RwUInt32 size;
    RpMultiTexture* multiTexture;

    size = sizeof(RpMultiTexture) + regEntry->extensionSize;

    multiTexture = (RpMultiTexture*)RwMalloc(size);
    if (multiTexture == NULL)
    {
        RWERROR((E_RW_NOMEM, size));
        return NULL;
    }

    memset(multiTexture, 0, size);

    multiTexture->regEntry = regEntry;
    multiTexture->numTextures = numTextures;

    if (regEntry->extensionSize != 0)
    {
        multiTexture->platformData = (void*)(multiTexture + 1);
    }

    return multiTexture;
}

static void MultiTextureDestroy(RpMultiTexture* multiTexture)
{
    RwUInt32 i;

    for (i = 0; i < multiTexture->numTextures; i++)
    {
        if (multiTexture->textures[i] != NULL)
        {
            RwTextureDestroy(multiTexture->textures[i]);
            multiTexture->textures[i] = NULL;
        }
    }

    if (multiTexture->effect != NULL)
    {
        RpMTEffectDestroy(multiTexture->effect);
        multiTexture->effect = NULL;
    }

    RwFree(multiTexture);
}

static void* MultiTextureConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    MULTITEXTUREEXTFROMOBJECT(object, offset)->multiTexture = NULL;

    return object;
}

static void* MultiTextureDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    if (MULTITEXTUREEXTFROMOBJECT(object, offset)->multiTexture != NULL)
    {
        MultiTextureDestroy(MULTITEXTUREEXTFROMOBJECT(object, offset)->multiTexture);
        MULTITEXTUREEXTFROMOBJECT(object, offset)->multiTexture = NULL;
    }

    return object;
}

static void* MultiTextureCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    const RpMultiTexture* srcMT;
    RpMultiTexture* dstMT;
    RwUInt32 i;

    srcMT = MULTITEXTUREEXTFROMCONSTOBJECT(srcObject, offset)->multiTexture;
    if (srcMT == NULL)
    {
        return dstObject;
    }

    dstMT = MultiTextureCreate(srcMT->regEntry, srcMT->numTextures);
    if (dstMT == NULL)
    {
        return NULL;
    }

    MULTITEXTUREEXTFROMOBJECT(dstObject, offset)->multiTexture = dstMT;

    for (i = 0; i < srcMT->numTextures; i++)
    {
        RpMultiTextureSetTexture(dstMT, i, RpMultiTextureGetTexture(srcMT, i));
        RpMultiTextureSetCoords(dstMT, i, RpMultiTextureGetCoords(srcMT, i));
    }

    RpMultiTextureSetEffect(dstMT, RpMultiTextureGetEffect(srcMT));

    return dstObject;
}

static RwInt32 MultiTextureStreamGetSize(const void* object, RwInt32 offset, RwInt32 sizeInObj)
{
    RpMultiTexture* multiTexture;
    RwUInt32 size;
    RwUInt32 i;

    size = 0;
    multiTexture = MULTITEXTUREEXTFROMCONSTOBJECT(object, offset)->multiTexture;

    if (multiTexture != NULL)
    {
        size = sizeof(RwUInt32) + sizeof(_MultiTextureStreamHdr) + multiTexture->numTextures;

        for (i = 0; i < multiTexture->numTextures; i++)
        {
            size += _rpMatFXStreamSizeTexture(multiTexture->textures[i]);
        }

        if (multiTexture->effect != NULL)
        {
            size += _rwStringStreamGetSize(multiTexture->effect->name) + rwCHUNKHEADERSIZE;
        }
    }

    return size;
}

static RwStream* MultiTextureStreamWrite(RwStream* stream, RwInt32 binaryLength, const void* object,
                                         RwInt32 offset, RwInt32 size)
{
    RpMultiTexture* multiTexture;
    RwUInt32 version;
    _MultiTextureStreamHdr header;
    RwUInt32 i;

    multiTexture = MULTITEXTUREEXTFROMCONSTOBJECT(object, offset)->multiTexture;
    if (multiTexture != NULL)
    {
        version = RwEngineGetVersion();
        if (!RwStreamWriteInt32(stream, (RwInt32*)&version, sizeof(version)))
        {
            return NULL;
        }

        header.platformID = (RwUInt8)multiTexture->regEntry->platformID;
        header.numTextures = (RwUInt8)multiTexture->numTextures;
        header.flags = (multiTexture->effect != NULL);
        header.pad = 0;

        if (!RwStreamWrite(stream, &header, sizeof(header)))
        {
            return NULL;
        }

        if (multiTexture->numTextures != 0)
        {
            if (!RwStreamWrite(stream, multiTexture->coords, multiTexture->numTextures))
            {
                return NULL;
            }

            for (i = 0; i < multiTexture->numTextures; i++)
            {
                _rpMatFXStreamWriteTexture(stream, multiTexture->textures[i]);
            }
        }

        if (multiTexture->effect != NULL)
        {
            if (!_rwStringStreamWrite(multiTexture->effect->name, stream))
            {
                return NULL;
            }
        }
    }

    return stream;
}

static RwStream* MultiTextureStreamRead(RwStream* stream, RwInt32 binaryLength, void* object,
                                        RwInt32 offset, RwInt32 size)
{
    RwUInt32 version;
    _MultiTextureStreamHdr header;
    rpMultiTextureRegEntry* regEntry;
    RpMultiTexture* multiTexture;
    RwUInt32 i;
    RwChar name[rpMTEFFECTNAMELENGTH];
    RpMTEffect* effect;

    if (!RwStreamReadInt32(stream, (RwInt32*)&version, sizeof(version)))
    {
        return NULL;
    }

    if (!RwStreamRead(stream, &header, sizeof(header)))
    {
        return NULL;
    }

    regEntry = &RegEntries[header.platformID];

    multiTexture = MultiTextureCreate(regEntry, header.numTextures);
    if (multiTexture == NULL)
    {
        return NULL;
    }

    if (header.numTextures != 0)
    {
        if (!RwStreamRead(stream, multiTexture->coords, multiTexture->numTextures))
        {
            MultiTextureDestroy(multiTexture);
            return NULL;
        }

        for (i = 0; i < multiTexture->numTextures; i++)
        {
            if (!_rpMatFXStreamReadTexture(stream, &multiTexture->textures[i]))
            {
                MultiTextureDestroy(multiTexture);
                return NULL;
            }
        }
    }

    if (header.flags & rpMULTITEXTUREEFFECTFLAG)
    {
        if (!_rwStringStreamFindAndRead(name, stream))
        {
            MultiTextureDestroy(multiTexture);
            return NULL;
        }

        effect = RpMTEffectFind(name);
        if (effect == NULL)
        {
            effect = RpMTEffectCreateDummy();
            if (effect == NULL)
            {
                MultiTextureDestroy(multiTexture);
                return NULL;
            }

            RpMTEffectSetName(effect, name);
        }

        RpMultiTextureSetEffect(multiTexture, effect);
        RpMTEffectDestroy(effect);
    }

    MULTITEXTUREEXTFROMOBJECT(object, offset)->multiTexture = multiTexture;

    return stream;
}

RwBool _rpMultiTexturePluginAttach(void)
{
    if (!_rpMTEffectSystemInit())
    {
        return FALSE;
    }

    memset(RegEntries, 0, sizeof(RegEntries));

    _rpMultiTextureModule.globalsOffset =
        RwEngineRegisterPlugin(sizeof(rpMultiTextureGlobals), rwID_MTEFFECTDICTPLUGIN,
                               MultiTextureOpen, MultiTextureClose);

    return _rpMultiTextureModule.globalsOffset >= 0;
}

RwBool _rpMaterialRegisterMultiTexturePlugin(RwPlatformID platformID, RwUInt32 pluginID,
                                             RwUInt32 extensionSize)
{
    RwInt32 offset;
    rpMultiTextureRegEntry* regEntry;

    offset = RpMaterialRegisterPlugin(sizeof(_MultiTextureExt), pluginID, MultiTextureConstructor,
                                      MultiTextureDestructor, MultiTextureCopy);
    if (offset < 0)
    {
        return FALSE;
    }

    regEntry = &RegEntries[platformID];
    regEntry->offset = offset;

    if (RpMaterialRegisterPluginStream(pluginID, MultiTextureStreamRead, MultiTextureStreamWrite,
                                       MultiTextureStreamGetSize) < 0)
    {
        return FALSE;
    }

    regEntry->platformID = platformID;
    regEntry->pluginID = pluginID;
    regEntry->extensionSize = extensionSize;

    return TRUE;
}

RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture, RpMTEffect* effect)
{
    if (multiTexture->effect != NULL)
    {
        RpMTEffectDestroy(multiTexture->effect);
    }

    multiTexture->effect = effect;

    if (effect != NULL)
    {
        RpMTEffectAddRef(multiTexture->effect);
    }

    return multiTexture;
}

RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture)
{
    return multiTexture->effect;
}

RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture, RwUInt32 index,
                                         RwTexture* texture)
{
    if (multiTexture->textures[index] != NULL)
    {
        RwTextureDestroy(multiTexture->textures[index]);
    }

    multiTexture->textures[index] = texture;

    if (texture != NULL)
    {
        texture->refCount++;
    }

    return multiTexture;
}

RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture, RwUInt32 index)
{
    return multiTexture->textures[index];
}

RpMultiTexture* RpMultiTextureSetCoords(RpMultiTexture* multiTexture, RwUInt32 index,
                                        RwUInt32 texCoordIndex)
{
    multiTexture->coords[index] = (RwUInt8)texCoordIndex;

    return multiTexture;
}

RwUInt32 RpMultiTextureGetCoords(const RpMultiTexture* multiTexture, RwUInt32 index)
{
    return multiTexture->coords[index];
}

RpMultiTexture* RpMaterialGetMultiTexture(const RpMaterial* material, RwPlatformID platformID)
{
    rpMultiTextureRegEntry* regEntry;

    regEntry = &RegEntries[platformID];
    if (regEntry->pluginID != 0)
    {
        return MULTITEXTUREEXTFROMCONSTOBJECT(material, regEntry->offset)->multiTexture;
    }

    return NULL;
}
