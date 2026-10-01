#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <string.h>

#include "rwsdk/plugin/matfx/gcn/mtgcnprivate.h"

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

#define rwLIBRARYCURRENTVERSION 0x35000

#define RwStreamWriteChunkHeader(stream, type, size)                                               \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, 0xFFFF)

/* Streams written before this library version lack the matrix and indirect data */
#define rpGAMECUBEMTCONFIGVERSION 0x33002

#define rpGAMECUBEMTSTREAMTEXDISABLE 0x80

#define rpMTALIGN(size) (((size) + 3) & ~3)

typedef struct _StreamMTConfig _StreamMTConfig;
struct _StreamMTConfig
{
    RwUInt8 numTexGens;
    RwUInt8 numTevStages;
    RwUInt8 flags;
    RwUInt8 blending;
    RpGXColorS10 reg[4];
    RwRGBA kreg[4];
    RwUInt8 numTexMtx;
    RwUInt8 numIndMtx;
    RwUInt8 numIndStages;
    RwUInt8 pad;
};

typedef struct _StreamTexGen _StreamTexGen;
struct _StreamTexGen
{
    RwUInt8 flags;
    RwUInt8 func;
    RwUInt8 srcParam;
    RwUInt8 mtx;
    RwUInt8 postMtx;
    RwUInt8 pad[3];
    RwUInt16 scaleS;
    RwUInt16 scaleT;
};

typedef struct _StreamTevStage _StreamTevStage;
struct _StreamTevStage
{
    RwUInt8 colorA;
    RwUInt8 colorB;
    RwUInt8 colorC;
    RwUInt8 colorD;
    RwUInt8 alphaA;
    RwUInt8 alphaB;
    RwUInt8 alphaC;
    RwUInt8 alphaD;
    RwUInt8 colorOp;
    RwUInt8 colorBias;
    RwUInt8 colorScale;
    RwUInt8 colorClamp;
    RwUInt8 colorOutReg;
    RwUInt8 alphaOp;
    RwUInt8 alphaBias;
    RwUInt8 alphaScale;
    RwUInt8 alphaClamp;
    RwUInt8 alphaOutReg;
    RwUInt8 colorSel;
    RwUInt8 alphaSel;
    RwUInt8 texCoordID;
    RwUInt8 texMapID;
    RwUInt8 channelID;
    RwUInt8 flags;
    RwUInt32 indirect;
};

typedef struct _StreamTexMtx _StreamTexMtx;
struct _StreamTexMtx
{
    RwUInt8 offset;
    RwUInt8 refFrame;
    RwUInt8 flags;
    RwUInt8 pad;
};

typedef struct _StreamIndMtx _StreamIndMtx;
struct _StreamIndMtx
{
    RwUInt8 id;
    RwUInt8 refFrame;
    RwUInt8 flags;
    RwInt8 scale;
};

typedef struct _StreamIndStage _StreamIndStage;
struct _StreamIndStage
{
    RwUInt8 flags;
    RwUInt8 texCoordID;
    RwUInt8 texMapID;
    RwUInt8 scaleS;
    RwUInt8 scaleT;
    RwUInt8 pad[3];
};

#define MTEFFECTGETCONFIG(effect) ((RpGameCubeMTConfig*)((effect) + 1))
#define MTEFFECTGETCONSTCONFIG(effect) ((const RpGameCubeMTConfig*)((effect) + 1))

extern RwStream* _rwStreamWriteVersionedChunkHeader(RwStream* stream, RwInt32 type, RwInt32 size,
                                                    RwUInt32 version, RwUInt32 buildNum);
extern void* RwMemLittleEndian16(void* mem, RwUInt32 size);
extern void* RwMemLittleEndian32(void* mem, RwUInt32 size);

static RwInt32 GameCubeMTEffectStreamGetSize(const RpMTEffect* effect)
{
    RwInt32 size;
    const RpGameCubeMTConfig* config;

    config = MTEFFECTGETCONSTCONFIG(effect);

    size = sizeof(_StreamMTConfig);
    size += config->numTexMtx * (sizeof(_StreamTexMtx) + sizeof(RwReal) * 12);
    size += config->numTexGens * sizeof(_StreamTexGen);
    size += config->numIndMtx * (sizeof(_StreamIndMtx) + sizeof(RwReal) * 6);
    size += config->numIndStages * sizeof(_StreamIndStage);
    size += config->numTevStages * sizeof(_StreamTevStage);

    return size;
}

static RpMTEffect* GameCubeMTEffectStreamWrite(const RpMTEffect* effect, RwStream* stream)
{
    const RpGameCubeMTConfig* config;
    RwUInt32 i;
    _StreamMTConfig binConfig;

    config = MTEFFECTGETCONSTCONFIG(effect);

    if (!RwStreamWriteChunkHeader(stream, rwID_EXTENSION, GameCubeMTEffectStreamGetSize(effect)))
    {
        return NULL;
    }

    binConfig.flags = (RwUInt8)config->flags;
    binConfig.numTevStages = config->numTevStages;
    binConfig.numTexGens = config->numTexGens;
    binConfig.numTexMtx = config->numTexMtx;
    binConfig.numIndMtx = config->numIndMtx;
    binConfig.numIndStages = config->numIndStages;
    binConfig.blending = (RwUInt8)(((config->srcBlend << 4) & 0xF0) | (config->destBlend & 0x0F));

    memcpy(binConfig.kreg, config->kreg, sizeof(binConfig.kreg));
    memcpy(binConfig.reg, config->reg, sizeof(binConfig.reg));
    RwMemLittleEndian16(binConfig.reg, sizeof(binConfig.reg));

    if (!RwStreamWrite(stream, &binConfig, sizeof(binConfig)))
    {
        return NULL;
    }

    for (i = 0; i < config->numTexGens; i++)
    {
        RpGameCubeTexGen* texGen;
        _StreamTexGen binTexGen;

        texGen = &config->texGens[i];

        binTexGen.flags = (RwUInt8)texGen->flags;
        binTexGen.func = (RwUInt8)texGen->func;
        binTexGen.srcParam = (RwUInt8)texGen->srcParam;
        binTexGen.mtx = (RwUInt8)texGen->mtx;
        binTexGen.postMtx = (RwUInt8)texGen->postMtx;
        binTexGen.pad[0] = 0;
        binTexGen.pad[1] = 0;
        binTexGen.pad[2] = 0;
        binTexGen.scaleS = texGen->scaleS;
        binTexGen.scaleT = texGen->scaleT;
        RwMemLittleEndian16(&binTexGen.scaleS, sizeof(RwUInt16) * 2);

        if (!RwStreamWrite(stream, &binTexGen, sizeof(binTexGen)))
        {
            return NULL;
        }
    }

    for (i = 0; i < config->numTevStages; i++)
    {
        RpGameCubeTevStage* tevStage;
        _StreamTevStage binTevStage;

        tevStage = &config->tevStages[i];

        binTevStage.colorA = (RwUInt8)tevStage->op.colorA;
        binTevStage.colorB = (RwUInt8)tevStage->op.colorB;
        binTevStage.colorC = (RwUInt8)tevStage->op.colorC;
        binTevStage.colorD = (RwUInt8)tevStage->op.colorD;
        binTevStage.alphaA = (RwUInt8)tevStage->op.alphaA;
        binTevStage.alphaB = (RwUInt8)tevStage->op.alphaB;
        binTevStage.alphaC = (RwUInt8)tevStage->op.alphaC;
        binTevStage.alphaD = (RwUInt8)tevStage->op.alphaD;
        binTevStage.colorOp = (RwUInt8)tevStage->op.colorOp;
        binTevStage.colorBias = (RwUInt8)tevStage->op.colorBias;
        binTevStage.colorScale = (RwUInt8)tevStage->op.colorScale;
        binTevStage.colorClamp = tevStage->op.colorClamp;
        binTevStage.colorOutReg = (RwUInt8)tevStage->op.colorOutReg;
        binTevStage.alphaOp = (RwUInt8)tevStage->op.alphaOp;
        binTevStage.alphaBias = (RwUInt8)tevStage->op.alphaBias;
        binTevStage.alphaScale = (RwUInt8)tevStage->op.alphaScale;
        binTevStage.alphaClamp = tevStage->op.alphaClamp;
        binTevStage.alphaOutReg = (RwUInt8)tevStage->op.alphaOutReg;
        binTevStage.colorSel = (RwUInt8)tevStage->op.colorSel;
        binTevStage.alphaSel = (RwUInt8)tevStage->op.alphaSel;
        binTevStage.texCoordID = (RwUInt8)tevStage->texCoordID;
        binTevStage.texMapID = (RwUInt8)tevStage->texMapID;
        binTevStage.channelID = (RwUInt8)tevStage->channelID;
        binTevStage.flags = (RwUInt8)tevStage->flags;
        binTevStage.indirect = tevStage->indirect;
        RwMemLittleEndian32(&binTevStage.indirect, sizeof(RwUInt32));

        if (tevStage->texMapID & rpGX_TEX_DISABLE)
        {
            binTevStage.flags |= rpGAMECUBEMTSTREAMTEXDISABLE;
        }

        if (!RwStreamWrite(stream, &binTevStage, sizeof(binTevStage)))
        {
            return NULL;
        }
    }

    for (i = 0; i < config->numTexMtx; i++)
    {
        RpGameCubeTexMtx* texMtx;
        _StreamTexMtx binTexMtx;

        texMtx = &config->texMtx[i];

        binTexMtx.offset = (RwUInt8)texMtx->offset;
        binTexMtx.refFrame = (RwUInt8)texMtx->refFrame;
        binTexMtx.flags = (RwUInt8)texMtx->flags;
        binTexMtx.pad = 0;

        if (!RwStreamWrite(stream, &binTexMtx, sizeof(binTexMtx)) ||
            !RwStreamWriteReal(stream, &texMtx->data[0][0], sizeof(texMtx->data)))
        {
            return NULL;
        }
    }

    for (i = 0; i < config->numIndMtx; i++)
    {
        RpGameCubeIndMtx* indMtx;
        _StreamIndMtx binIndMtx;

        indMtx = &config->indMtx[i];

        binIndMtx.id = (RwUInt8)indMtx->id;
        binIndMtx.refFrame = (RwUInt8)indMtx->refFrame;
        binIndMtx.flags = (RwUInt8)indMtx->flags;
        binIndMtx.scale = (RwInt8)indMtx->scale;

        if (!RwStreamWrite(stream, &binIndMtx, sizeof(binIndMtx)) ||
            !RwStreamWriteReal(stream, &indMtx->data[0][0], sizeof(indMtx->data)))
        {
            return NULL;
        }
    }

    for (i = 0; i < config->numIndStages; i++)
    {
        RpGameCubeIndStage* indStage;
        _StreamIndStage binIndStage;

        indStage = &config->indStages[i];

        binIndStage.flags = (RwUInt8)indStage->flags;
        binIndStage.texCoordID = (RwUInt8)indStage->texCoordID;
        binIndStage.texMapID = (RwUInt8)indStage->texMapID;
        binIndStage.scaleS = (RwUInt8)indStage->scaleS;
        binIndStage.scaleT = (RwUInt8)indStage->scaleT;
        binIndStage.pad[0] = 0;
        binIndStage.pad[1] = 0;
        binIndStage.pad[2] = 0;

        if (!RwStreamWrite(stream, &binIndStage, sizeof(binIndStage)))
        {
            return NULL;
        }
    }

    return (RpMTEffect*)effect;
}

static RpMTEffect* GameCubeMTEffectStreamRead(RwStream* stream, RwPlatformID platformID,
                                              RwUInt32 version, RwUInt32 length)
{
    RpMTEffect* effect;
    _StreamMTConfig binConfig;
    RpGameCubeMTConfig* config;
    RwUInt32 i;
    RwUInt32 size;
    RwBool oldVersion;

    oldVersion = (version < rpGAMECUBEMTCONFIGVERSION);

    if (oldVersion)
    {
        size = sizeof(_StreamMTConfig) - 4;
        binConfig.numTexMtx = 0;
        binConfig.numIndMtx = 0;
        binConfig.numIndStages = 0;
    }
    else
    {
        size = sizeof(_StreamMTConfig);
    }

    if (RwStreamRead(stream, &binConfig, size) != size)
    {
        return NULL;
    }

    effect =
        RpGameCubeMTEffectCreate(binConfig.numTevStages, binConfig.numTexGens, binConfig.numTexMtx,
                                 binConfig.numIndStages, binConfig.numIndMtx);
    if (effect == NULL)
    {
        return NULL;
    }

    config = RpGameCubeMTEffectGetConfig(effect);

    config->flags = binConfig.flags;
    config->srcBlend = (RwBlendFunction)((binConfig.blending >> 4) & 0x0F);
    config->destBlend = (RwBlendFunction)(binConfig.blending & 0x0F);

    memcpy(config->kreg, binConfig.kreg, sizeof(config->kreg));
    RwMemNative32(binConfig.reg, sizeof(binConfig.reg));
    memcpy(config->reg, binConfig.reg, sizeof(config->reg));

    for (i = 0; i < config->numTexGens; i++)
    {
        _StreamTexGen binTexGen;
        RpGameCubeTexGen* texGen;

        texGen = &config->texGens[i];

        if (oldVersion)
        {
            size = 5;
            binTexGen.scaleS = 0;
            binTexGen.scaleT = 0;
        }
        else
        {
            size = sizeof(_StreamTexGen);
        }

        if (RwStreamRead(stream, &binTexGen, size) != size)
        {
            RwFree(effect);
            return NULL;
        }

        texGen->flags = binTexGen.flags;
        texGen->func = binTexGen.func;
        texGen->srcParam = binTexGen.srcParam;
        texGen->mtx = binTexGen.mtx;
        texGen->postMtx = binTexGen.postMtx;
        RwMemNative32(&binTexGen.scaleS, sizeof(RwUInt16) * 2);
        texGen->scaleS = binTexGen.scaleS;
        texGen->scaleT = binTexGen.scaleT;
    }

    for (i = 0; i < config->numTevStages; i++)
    {
        _StreamTevStage binTevStage;
        RpGameCubeTevStage* tevStage;

        tevStage = &config->tevStages[i];

        if (oldVersion)
        {
            size = sizeof(_StreamTevStage) - sizeof(RwUInt32);
            binTevStage.indirect = 0;
        }
        else
        {
            size = sizeof(_StreamTevStage);
        }

        if (RwStreamRead(stream, &binTevStage, size) != size)
        {
            RwFree(effect);
            return NULL;
        }

        tevStage->op.colorA = binTevStage.colorA;
        tevStage->op.colorB = binTevStage.colorB;
        tevStage->op.colorC = binTevStage.colorC;
        tevStage->op.colorD = binTevStage.colorD;
        tevStage->op.alphaA = binTevStage.alphaA;
        tevStage->op.alphaB = binTevStage.alphaB;
        tevStage->op.alphaC = binTevStage.alphaC;
        tevStage->op.alphaD = binTevStage.alphaD;
        tevStage->op.colorOp = binTevStage.colorOp;
        tevStage->op.colorBias = binTevStage.colorBias;
        tevStage->op.colorScale = binTevStage.colorScale;
        tevStage->op.colorClamp = binTevStage.colorClamp;
        tevStage->op.colorOutReg = binTevStage.colorOutReg;
        tevStage->op.alphaOp = binTevStage.alphaOp;
        tevStage->op.alphaBias = binTevStage.alphaBias;
        tevStage->op.alphaScale = binTevStage.alphaScale;
        tevStage->op.alphaClamp = binTevStage.alphaClamp;
        tevStage->op.alphaOutReg = binTevStage.alphaOutReg;
        tevStage->op.colorSel = binTevStage.colorSel;
        tevStage->op.alphaSel = binTevStage.alphaSel;
        tevStage->texCoordID = binTevStage.texCoordID;
        tevStage->texMapID = binTevStage.texMapID;
        tevStage->channelID = binTevStage.channelID;
        tevStage->flags = binTevStage.flags;
        RwMemNative32(&binTevStage.indirect, sizeof(RwUInt32));
        tevStage->indirect = binTevStage.indirect;

        if (tevStage->flags & rpGAMECUBEMTSTREAMTEXDISABLE)
        {
            tevStage->texMapID |= rpGX_TEX_DISABLE;
            tevStage->flags &= ~rpGAMECUBEMTSTREAMTEXDISABLE;
        }
    }

    for (i = 0; i < config->numTexMtx; i++)
    {
        RpGameCubeTexMtx* texMtx;
        _StreamTexMtx binTexMtx;

        texMtx = &config->texMtx[i];

        if (RwStreamRead(stream, &binTexMtx, sizeof(binTexMtx)) != sizeof(binTexMtx) ||
            !RwStreamReadReal(stream, &texMtx->data[0][0], sizeof(texMtx->data)))
        {
            RwFree(effect);
            return NULL;
        }

        texMtx->offset = binTexMtx.offset;
        texMtx->refFrame = (RpGameCubeTexFrameID)binTexMtx.refFrame;
        texMtx->flags = binTexMtx.flags;
    }

    for (i = 0; i < config->numIndMtx; i++)
    {
        RpGameCubeIndMtx* indMtx;
        _StreamIndMtx binIndMtx;

        indMtx = &config->indMtx[i];

        if (RwStreamRead(stream, &binIndMtx, sizeof(binIndMtx)) != sizeof(binIndMtx) ||
            !RwStreamReadReal(stream, &indMtx->data[0][0], sizeof(indMtx->data)))
        {
            RwFree(effect);
            return NULL;
        }

        indMtx->id = binIndMtx.id;
        indMtx->refFrame = (RpGameCubeTexFrameID)binIndMtx.refFrame;
        indMtx->flags = binIndMtx.flags;
        indMtx->scale = binIndMtx.scale;
    }

    for (i = 0; i < config->numIndStages; i++)
    {
        RpGameCubeIndStage* indStage;
        _StreamIndStage binIndStage;

        indStage = &config->indStages[i];

        if (RwStreamRead(stream, &binIndStage, sizeof(binIndStage)) != sizeof(binIndStage))
        {
            RwFree(effect);
            return NULL;
        }

        indStage->flags = binIndStage.flags;
        indStage->texCoordID = binIndStage.texCoordID;
        indStage->texMapID = binIndStage.texMapID;
        indStage->scaleS = binIndStage.scaleS;
        indStage->scaleT = binIndStage.scaleT;
    }

    return effect;
}

RwBool _rpGameCubeMTDataPluginAttach(void)
{
    RwBool result;

    result =
        _rpMTEffectRegisterPlatform(rwID_GAMECUBE, GameCubeMTEffectStreamRead,
                                    (RpMTEffectStreamWriteCallBack)GameCubeMTEffectStreamWrite,
                                    (RpMTEffectStreamGetSizeCallBack)GameCubeMTEffectStreamGetSize,
                                    NULL);

    return result;
}

RpMTEffect* RpGameCubeMTEffectCreate(RwUInt32 numTevStages, RwUInt32 numTexGens, RwUInt32 numTexMtx,
                                     RwUInt32 numIndStages, RwUInt32 numIndMtx)
{
    RpMTEffect* effect;
    RpGameCubeMTConfig* config;
    RwUInt32 size;
    RwUInt8* offsetData;

    size = sizeof(RpMTEffect) + sizeof(RpGameCubeMTConfig) + numTexGens * sizeof(RpGameCubeTexGen);
    size = rpMTALIGN(size) + numTevStages * sizeof(RpGameCubeTevStage);
    size = rpMTALIGN(size) + numIndStages * sizeof(RpGameCubeIndStage);
    size = rpMTALIGN(size) + numTexMtx * sizeof(RpGameCubeTexMtx);
    size = rpMTALIGN(size) + numIndMtx * sizeof(RpGameCubeIndMtx);
    size = rpMTALIGN(size);

    effect = (RpMTEffect*)RwMalloc(size);
    if (effect == NULL)
    {
        RWERROR((E_RW_NOMEM, size));
        return NULL;
    }

    memset(effect, 0, size);
    _rpMTEffectInit(effect, rwID_GAMECUBE);

    config = MTEFFECTGETCONFIG(effect);

    config->maxNumTevStages = (RwUInt8)numTevStages;
    config->maxNumTexGens = (RwUInt8)numTexGens;
    config->maxNumIndStages = (RwUInt8)numIndStages;
    config->maxNumTexMtx = (RwUInt8)numTexMtx;
    config->maxNumIndMtx = (RwUInt8)numIndMtx;

    config->numTevStages = config->maxNumTevStages;
    config->numTexGens = config->maxNumTexGens;
    config->numIndStages = config->maxNumIndStages;
    config->numTexMtx = config->maxNumTexMtx;
    config->numIndMtx = config->maxNumIndMtx;

    offsetData = (RwUInt8*)(config + 1);

    if (numTexMtx != 0)
    {
        config->texMtx = (RpGameCubeTexMtx*)offsetData;
        offsetData += numTexMtx * sizeof(RpGameCubeTexMtx);
    }

    if (numTexGens != 0)
    {
        config->texGens = (RpGameCubeTexGen*)offsetData;
        offsetData += numTexGens * sizeof(RpGameCubeTexGen);
    }

    if (numIndMtx != 0)
    {
        config->indMtx = (RpGameCubeIndMtx*)offsetData;
        offsetData += numIndMtx * sizeof(RpGameCubeIndMtx);
    }

    if (numIndStages != 0)
    {
        config->indStages = (RpGameCubeIndStage*)offsetData;
        offsetData += numIndStages * sizeof(RpGameCubeIndStage);
    }

    if (numTevStages != 0)
    {
        config->tevStages = (RpGameCubeTevStage*)offsetData;
    }

    return effect;
}

RpGameCubeMTConfig* RpGameCubeMTEffectGetConfig(RpMTEffect* effect)
{
    return MTEFFECTGETCONFIG(effect);
}

void RpGameCubeTevIndUnpack(RpGameCubeTevInd* data, RwUInt32* pkData)
{
    RwUInt32 pk;

    pk = *pkData;

    data->indStage = pk & 0x3;
    data->format = (pk >> 2) & 0x3;
    data->biasSel = (pk >> 4) & 0x7;
    data->matrixSel = (pk >> 7) & 0xF;
    data->wrapS = (pk >> 11) & 0x7;
    data->wrapT = (pk >> 14) & 0x7;
    data->addPrev = (RwUInt8)((pk >> 17) & 0x1);
    data->utcLod = (RwUInt8)((pk >> 18) & 0x1);
    data->alphaSel = (pk >> 19) & 0x3;
}
