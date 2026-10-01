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

#define rpMTEFFECTNUMPLATFORMS (rwID_PCD3D8 + 1)
#define rpMTEFFECTPATHSIZE 256

typedef struct EffectRegEntry EffectRegEntry;
struct EffectRegEntry
{
    RwPlatformID platformID;
    RpMTEffectDestroyCallBack destroy;
    RpMTEffectStreamReadCallBack streamRead;
    RpMTEffectStreamWriteCallBack streamWrite;
    RpMTEffectStreamGetSizeCallBack streamGetSize;
};

typedef struct Iterator Iterator;
struct Iterator
{
    void* cur;
    void* end;
};

typedef union RpPtrMTEffect RpPtrMTEffect;
union RpPtrMTEffect
{
    RpMTEffect* ptrMTEffect;
    const RpMTEffect* constptrMTEffect;
};

typedef struct BinaryEffect BinaryEffect;
struct BinaryEffect
{
    RwUInt32 platformID;
};

static RpMTEffectDict* DummyDict;
static EffectRegEntry EffectRegEntries[rpMTEFFECTNUMPLATFORMS];

RwBool _rpMTEffectSystemInit(void)
{
    memset(EffectRegEntries, 0, sizeof(EffectRegEntries));

    return TRUE;
}

RwBool _rpMTEffectRegisterPlatform(RwPlatformID platformID, RpMTEffectStreamReadCallBack streamRead,
                                   RpMTEffectStreamWriteCallBack streamWrite,
                                   RpMTEffectStreamGetSizeCallBack streamGetSize,
                                   RpMTEffectDestroyCallBack destroy)
{
    EffectRegEntry* regEntry;

    regEntry = &EffectRegEntries[platformID];

    regEntry->platformID = platformID;
    regEntry->streamRead = streamRead;
    regEntry->streamWrite = streamWrite;
    regEntry->streamGetSize = streamGetSize;
    regEntry->destroy = destroy;

    return TRUE;
}

RwBool _rpMTEffectOpen(void)
{
    RwUInt32 pathSize;
    RwUInt32 totalSize;
    RwChar* path;

    rwLinkListInitialize(&RPMULTITEXTUREGLOBAL(dictList));

    DummyDict = RpMTEffectDictCreate();
    if (DummyDict == NULL)
    {
        return FALSE;
    }

    RPMULTITEXTUREGLOBAL(currentDict) = DummyDict;

    pathSize = rpMTEFFECTPATHSIZE;
    totalSize = 2 * pathSize + rpMTEFFECTNAMELENGTH;

    path = (RwChar*)RwMalloc(totalSize);
    if (path == NULL)
    {
        RpMTEffectDictDestroy(RPMULTITEXTUREGLOBAL(currentDict));
        RWERROR((E_RW_NOMEM, totalSize));
        return FALSE;
    }

    memset(path, 0, totalSize);

    RPMULTITEXTUREGLOBAL(path) = path;
    RPMULTITEXTUREGLOBAL(scratch) = path + pathSize;
    RPMULTITEXTUREGLOBAL(pathSize) = pathSize;

    return TRUE;
}

RwBool _rpMTEffectClose(void)
{
    Iterator iter;
    const RpMTEffectDict* dict;

    if (RPMULTITEXTUREGLOBAL(path) != NULL)
    {
        RwFree(RPMULTITEXTUREGLOBAL(path));
        RPMULTITEXTUREGLOBAL(path) = NULL;
        RPMULTITEXTUREGLOBAL(scratch) = NULL;
        RPMULTITEXTUREGLOBAL(pathSize) = 0;
    }

    iter.cur = rwLinkListGetFirstLLLink(&RPMULTITEXTUREGLOBAL(dictList));
    iter.end = rwLinkListGetTerminator(&RPMULTITEXTUREGLOBAL(dictList));
    while (iter.cur != iter.end)
    {
        dict = rwLLLinkGetConstData((const RwLLLink*)iter.cur, RpMTEffectDict, dictListLink);
        if (dict == DummyDict)
        {
            RpMTEffectDictDestroy(DummyDict);
            DummyDict = NULL;
            break;
        }

        iter.cur = (void*)rwLLLinkGetNext((const RwLLLink*)iter.cur);
    }

    return TRUE;
}

RpMTEffect* _rpMTEffectInit(RpMTEffect* effect, RwPlatformID platformID)
{
    memset(effect, 0, sizeof(RpMTEffect));

    effect->platformID = platformID;
    effect->refCount = 1;
    rwLLLinkInitialize(&effect->dictLink);

    if (platformID != 0 && RPMULTITEXTUREGLOBAL(currentDict) != NULL)
    {
        RpMTEffectDictAddEffect(RPMULTITEXTUREGLOBAL(currentDict), effect);
    }

    return effect;
}

RpMTEffectDict* RpMTEffectDictCreate(void)
{
    RpMTEffectDict* dict;

    dict = (RpMTEffectDict*)RwMalloc(sizeof(RpMTEffectDict));
    if (dict == NULL)
    {
        RWERROR((E_RW_NOMEM, sizeof(RpMTEffectDict)));
        return NULL;
    }

    rwLinkListInitialize(&dict->effectList);
    rwLinkListAddLLLink(&RPMULTITEXTUREGLOBAL(dictList), &dict->dictListLink);

    return dict;
}

void RpMTEffectDictDestroy(RpMTEffectDict* dict)
{
    RwLLLink* cur;
    RpMTEffect* effect;

    if (dict == RPMULTITEXTUREGLOBAL(currentDict))
    {
        RPMULTITEXTUREGLOBAL(currentDict) = NULL;
    }

    cur = rwLinkListGetFirstLLLink(&dict->effectList);
    while (cur != rwLinkListGetTerminator(&dict->effectList))
    {
        effect = rwLLLinkGetData(cur, RpMTEffect, dictLink);
        cur = rwLLLinkGetNext(cur);

        RpMTEffectDictRemoveEffect(effect);
    }

    rwLinkListRemoveLLLink(&dict->dictListLink);

    RwFree(dict);
}

RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dict, RpMTEffect* effect)
{
    if (rwLLLinkAttached(&effect->dictLink))
    {
        rwLinkListRemoveLLLink(&effect->dictLink);
        RpMTEffectDestroy(effect);
    }

    rwLinkListAddLLLink(&dict->effectList, &effect->dictLink);
    RpMTEffectAddRef(effect);

    return dict;
}

RpMTEffect* RpMTEffectDictRemoveEffect(RpMTEffect* effect)
{
    if (rwLLLinkAttached(&effect->dictLink))
    {
        rwLinkListRemoveLLLink(&effect->dictLink);
        RpMTEffectDestroy(effect);
    }

    return effect;
}

static RpMTEffect* RpMTEffectDictFindNamedEffect(const RpMTEffectDict* dict, const RwChar* name)
{
    Iterator iter;
    RpPtrMTEffect PtrMTEffect;

    iter.cur = (void*)rwLinkListGetFirstLLLink(&dict->effectList);
    iter.end = (void*)rwLinkListGetTerminator(&dict->effectList);
    while (iter.cur != iter.end)
    {
        PtrMTEffect.constptrMTEffect =
            rwLLLinkGetConstData((const RwLLLink*)iter.cur, RpMTEffect, dictLink);
        if (!rwstrcmp(PtrMTEffect.constptrMTEffect->name, name))
        {
            return PtrMTEffect.ptrMTEffect;
        }

        iter.cur = (void*)rwLLLinkGetNext((const RwLLLink*)iter.cur);
    }

    return NULL;
}

RpMTEffect* RpMTEffectCreateDummy(void)
{
    RpMTEffect* effect;

    effect = (RpMTEffect*)RwMalloc(sizeof(RpMTEffect));
    if (effect == NULL)
    {
        RWERROR((E_RW_NOMEM, sizeof(RpMTEffect)));
        return NULL;
    }

    _rpMTEffectInit(effect, (RwPlatformID)0);

    return effect;
}

void RpMTEffectDestroy(RpMTEffect* effect)
{
    EffectRegEntry* regEntry;

    effect->refCount--;
    if (effect->refCount == 0)
    {
        RpMTEffectDictRemoveEffect(effect);

        if (effect->platformID != 0)
        {
            regEntry = &EffectRegEntries[effect->platformID];
            if (regEntry->destroy != NULL)
            {
                regEntry->destroy(effect);
                return;
            }
        }

        RwFree(effect);
    }
}

static RpMTEffect* RpMTEffectStreamRead(RwStream* stream)
{
    BinaryEffect binEffect;
    EffectRegEntry* regEntry;
    RpMTEffect* effect;
    RwChar name[rpMTEFFECTNAMELENGTH];
    RwUInt32 version;
    RwUInt32 length;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, NULL, NULL) ||
        !RwStreamRead(stream, &binEffect, sizeof(binEffect)))
    {
        return NULL;
    }

    RwMemNative32(&binEffect, sizeof(binEffect));

    regEntry = &EffectRegEntries[binEffect.platformID];
    if (regEntry->streamRead == NULL)
    {
        return NULL;
    }

    if (!_rwStringStreamFindAndRead(name, stream))
    {
        return NULL;
    }

    if (!RwStreamFindChunk(stream, rwID_EXTENSION, &length, &version))
    {
        return NULL;
    }

    effect = regEntry->streamRead(stream, (RwPlatformID)binEffect.platformID, version, length);
    if (effect == NULL)
    {
        return NULL;
    }

    RpMTEffectSetName(effect, name);

    return effect;
}

RpMTEffect* RpMTEffectFind(RwChar* name)
{
    RpMTEffect* effect;
    RwStream* stream;
    RwChar* scratch;
    Iterator iter;
    const RpMTEffectDict* dict;

    effect = NULL;

    if (RPMULTITEXTUREGLOBAL(currentDict) != NULL)
    {
        effect = RpMTEffectDictFindNamedEffect(RPMULTITEXTUREGLOBAL(currentDict), name);
    }
    else
    {
        iter.cur = rwLinkListGetFirstLLLink(&RPMULTITEXTUREGLOBAL(dictList));
        iter.end = rwLinkListGetTerminator(&RPMULTITEXTUREGLOBAL(dictList));
        while (iter.cur != iter.end)
        {
            dict = rwLLLinkGetConstData((const RwLLLink*)iter.cur, RpMTEffectDict, dictListLink);

            effect = RpMTEffectDictFindNamedEffect(dict, name);
            if (effect != NULL)
            {
                break;
            }

            iter.cur = (void*)rwLLLinkGetNext((const RwLLLink*)iter.cur);
        }
    }

    if (effect != NULL)
    {
        RpMTEffectAddRef(effect);
        return effect;
    }

    scratch = RPMULTITEXTUREGLOBAL(scratch);
    rwstrcpy(scratch, RPMULTITEXTUREGLOBAL(path));
    rwstrncat(scratch, name, rpMTEFFECTNAMELENGTH - 1);

    stream = RwStreamOpen(rwSTREAMFILENAME, rwSTREAMREAD, scratch);
    if (stream == NULL)
    {
        return NULL;
    }

    if (!RwStreamFindChunk(stream, rwID_MTEFFECTNATIVE, NULL, NULL))
    {
        RwStreamClose(stream, NULL);
        return NULL;
    }

    effect = RpMTEffectStreamRead(stream);

    RwStreamClose(stream, NULL);

    return effect;
}

RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, RwChar* name)
{
    rwstrncpy(effect->name, name, rpMTEFFECTNAMELENGTH - 1);

    return effect;
}

RpMTEffect* RpMTEffectAddRef(RpMTEffect* effect)
{
    effect->refCount++;

    return effect;
}
