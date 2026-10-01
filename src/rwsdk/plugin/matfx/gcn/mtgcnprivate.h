#ifndef MTGCNPRIVATE_H
#define MTGCNPRIVATE_H

#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/plugin/matfx/mtprivate.h"

/* Private declarations shared by the GameCube multi-texture units. */

typedef struct RpGXColorS10 RpGXColorS10;
struct RpGXColorS10
{
    RwInt16 r;
    RwInt16 g;
    RwInt16 b;
    RwInt16 a;
};

enum RpGameCubeTexFrameID
{
    rpGAMECUBETEXFRAME_NONE = 0,
    rpGAMECUBETEXFRAME_OBJECT = 1,
    rpGAMECUBETEXFRAME_WORLD = 2,
    rpGAMECUBETEXFRAME_CAMERA = 3,
    rpGAMECUBETEXFRAME_MISC0 = 16,
    rpGAMECUBETEXFRAME_MISC1 = 17,
    rpGAMECUBETEXFRAME_MISC2 = 18,
    rpGAMECUBETEXFRAME_MISC3 = 19,
    rpGAMECUBETEXFRAME_MISCMAX = 47,
    rpGAMECUBETEXFRAME_FORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RpGameCubeTexFrameID RpGameCubeTexFrameID;

typedef struct RpGameCubeTexMtx RpGameCubeTexMtx;
struct RpGameCubeTexMtx
{
    RwUInt32 offset;
    RpGameCubeTexFrameID refFrame;
    RwUInt32 flags;
    RwReal data[3][4];
};

typedef struct RpGameCubeTexGen RpGameCubeTexGen;
struct RpGameCubeTexGen
{
    RwUInt32 flags;
    RwInt32 func;
    RwInt32 srcParam;
    RwUInt32 mtx;
    RwUInt32 postMtx;
    RwUInt16 scaleS;
    RwUInt16 scaleT;
};

typedef struct RpGameCubeIndMtx RpGameCubeIndMtx;
struct RpGameCubeIndMtx
{
    RwUInt32 id;
    RpGameCubeTexFrameID refFrame;
    RwUInt32 flags;
    RwReal data[2][3];
    RwInt32 scale;
};

typedef struct RpGameCubeIndStage RpGameCubeIndStage;
struct RpGameCubeIndStage
{
    RwUInt32 flags;
    RwUInt32 texCoordID;
    RwUInt32 texMapID;
    RwUInt32 scaleS;
    RwUInt32 scaleT;
};

typedef struct RpGameCubeTevOp RpGameCubeTevOp;
struct RpGameCubeTevOp
{
    RwUInt32 colorA;
    RwUInt32 colorB;
    RwUInt32 colorC;
    RwUInt32 colorD;
    RwUInt32 alphaA;
    RwUInt32 alphaB;
    RwUInt32 alphaC;
    RwUInt32 alphaD;
    RwUInt32 colorOp;
    RwUInt32 colorBias;
    RwUInt32 colorScale;
    RwUInt8 colorClamp;
    RwUInt32 colorOutReg;
    RwUInt32 alphaOp;
    RwUInt32 alphaBias;
    RwUInt32 alphaScale;
    RwUInt8 alphaClamp;
    RwUInt32 alphaOutReg;
    RwUInt32 colorSel;
    RwUInt32 alphaSel;
};

#define rpGX_TEX_DISABLE 0x100

typedef struct RpGameCubeTevStage RpGameCubeTevStage;
struct RpGameCubeTevStage
{
    RpGameCubeTevOp op;
    RwUInt32 texCoordID;
    RwUInt32 texMapID;
    RwUInt32 channelID;
    RwUInt32 flags;
    RwUInt32 indirect;
};

typedef struct RpGameCubeMTConfig RpGameCubeMTConfig;
struct RpGameCubeMTConfig
{
    RwUInt32 flags;
    RwUInt8 maxNumTexGens;
    RwUInt8 maxNumTevStages;
    RwUInt8 maxNumTexMtx;
    RwUInt8 maxNumIndStages;
    RwUInt8 maxNumIndMtx;
    RwUInt8 numTexGens;
    RwUInt8 numTevStages;
    RwUInt8 numTexMtx;
    RwUInt8 numIndStages;
    RwUInt8 numIndMtx;
    RpGXColorS10 reg[4];
    RwRGBA kreg[4];
    RpGameCubeTexMtx* texMtx;
    RpGameCubeTexGen* texGens;
    RpGameCubeIndMtx* indMtx;
    RpGameCubeIndStage* indStages;
    RpGameCubeTevStage* tevStages;
    RwBlendFunction srcBlend;
    RwBlendFunction destBlend;
};

typedef struct RpGameCubeTevInd RpGameCubeTevInd;
struct RpGameCubeTevInd
{
    RwUInt32 indStage;
    RwUInt32 format;
    RwUInt32 biasSel;
    RwUInt32 matrixSel;
    RwUInt32 wrapS;
    RwUInt32 wrapT;
    RwUInt8 addPrev;
    RwUInt8 utcLod;
    RwUInt32 alphaSel;
};

/* multiTexGcnData.c */
extern RwBool _rpGameCubeMTDataPluginAttach(void);
extern RpMTEffect* RpGameCubeMTEffectCreate(RwUInt32 numTevStages, RwUInt32 numTexGens,
                                            RwUInt32 numTexMtx, RwUInt32 numIndStages,
                                            RwUInt32 numIndMtx);
extern RpGameCubeMTConfig* RpGameCubeMTEffectGetConfig(RpMTEffect* effect);
extern void RpGameCubeTevIndUnpack(RpGameCubeTevInd* data, RwUInt32* pkData);

/* multiTexGcnPipe.c */
extern RwBool _rpGameCubeMTPipePluginAttach(void);

#endif
