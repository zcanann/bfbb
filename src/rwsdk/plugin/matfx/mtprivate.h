#ifndef MTPRIVATE_H
#define MTPRIVATE_H

#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

/* Private declarations shared by the multi-texture units of the material effects plugin. */

#define MAKECHUNKID(vendorID, chunkID) (((vendorID & 0xFFFFFF) << 8) | (chunkID & 0xFF))

#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define rwID_STRUCT 0x01
#define rwID_EXTENSION 0x03
#define rwID_MTEFFECTNATIVE MAKECHUNKID(rwVENDORID_CORE, 0x20)
#define rwID_MULTITEXPLUGIN MAKECHUNKID(rwVENDORID_CRITERIONTK, 0x20)
#define rwID_MTEFFECTDICTPLUGIN MAKECHUNKID(rwVENDORID_CRITERIONTK, 0x2C)

#define E_RW_NOMEM 0x80000013

#define rpMULTITEXTUREMAXTEXTURES 8
#define rpMULTITEXTURENUMPLATFORMS (rwID_PCD3D8 + 1)
#define rpMTEFFECTNAMELENGTH 32

#ifndef RWMODULEINFO_DEFINED
#define RWMODULEINFO_DEFINED
typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};
#endif

typedef struct RpMTEffect RpMTEffect;
struct RpMTEffect
{
    RwPlatformID platformID;
    RwUInt32 refCount;
    RwChar name[rpMTEFFECTNAMELENGTH];
    RwLLLink dictLink;
};

typedef struct RpMTEffectDict RpMTEffectDict;
struct RpMTEffectDict
{
    RwLinkList effectList;
    RwLLLink dictListLink;
};

typedef struct rpMultiTextureRegEntry rpMultiTextureRegEntry;
struct rpMultiTextureRegEntry
{
    RwPlatformID platformID;
    RwUInt32 pluginID;
    RwInt32 offset;
    RwUInt32 extensionSize;
};

typedef struct RpMultiTexture RpMultiTexture;
struct RpMultiTexture
{
    rpMultiTextureRegEntry* regEntry;
    RwUInt32 numTextures;
    RwTexture* textures[rpMULTITEXTUREMAXTEXTURES];
    RwUInt8 coords[rpMULTITEXTUREMAXTEXTURES];
    RpMTEffect* effect;
    void* platformData;
};

typedef RpMTEffect* (*RpMTEffectStreamReadCallBack)(RwStream* stream, RwPlatformID platformID,
                                                    RwUInt32 version, RwUInt32 length);
typedef RpMTEffect* (*RpMTEffectStreamWriteCallBack)(RpMTEffect* effect, RwStream* stream);
typedef RwInt32 (*RpMTEffectStreamGetSizeCallBack)(RpMTEffect* effect);
typedef void (*RpMTEffectDestroyCallBack)(RpMTEffect* effect);

extern RwModuleInfo _rpMultiTextureModule;

typedef struct rpMultiTextureGlobals rpMultiTextureGlobals;
struct rpMultiTextureGlobals
{
    RwLinkList dictList;
    RpMTEffectDict* currentDict;
    RwUInt32 pathSize;
    RwChar* path;
    RwChar* scratch;
};

#define RPMULTITEXTUREGLOBAL(var)                                                                  \
    (RWPLUGINOFFSET(rpMultiTextureGlobals, RwEngineInstance, _rpMultiTextureModule.globalsOffset)  \
         ->var)

/* multiTex.c */
extern RwBool _rpMultiTexturePluginAttach(void);
extern RwBool _rpMaterialRegisterMultiTexturePlugin(RwPlatformID platformID, RwUInt32 pluginID,
                                                    RwUInt32 extensionSize);
extern RpMultiTexture* RpMultiTextureSetEffect(RpMultiTexture* multiTexture, RpMTEffect* effect);
extern RpMTEffect* RpMultiTextureGetEffect(const RpMultiTexture* multiTexture);
extern RpMultiTexture* RpMultiTextureSetTexture(RpMultiTexture* multiTexture, RwUInt32 index,
                                                RwTexture* texture);
extern RwTexture* RpMultiTextureGetTexture(const RpMultiTexture* multiTexture, RwUInt32 index);
extern RpMultiTexture* RpMultiTextureSetCoords(RpMultiTexture* multiTexture, RwUInt32 index,
                                               RwUInt32 texCoordIndex);
extern RwUInt32 RpMultiTextureGetCoords(const RpMultiTexture* multiTexture, RwUInt32 index);
extern RpMultiTexture* RpMaterialGetMultiTexture(const RpMaterial* material,
                                                 RwPlatformID platformID);

/* multiTexEffect.c */
extern RwBool _rpMTEffectSystemInit(void);
extern RwBool _rpMTEffectRegisterPlatform(RwPlatformID platformID,
                                          RpMTEffectStreamReadCallBack streamRead,
                                          RpMTEffectStreamWriteCallBack streamWrite,
                                          RpMTEffectStreamGetSizeCallBack streamGetSize,
                                          RpMTEffectDestroyCallBack destroy);
extern RwBool _rpMTEffectOpen(void);
extern RwBool _rpMTEffectClose(void);
extern RpMTEffect* _rpMTEffectInit(RpMTEffect* effect, RwPlatformID platformID);
extern RpMTEffectDict* RpMTEffectDictCreate(void);
extern void RpMTEffectDictDestroy(RpMTEffectDict* dict);
extern RpMTEffectDict* RpMTEffectDictAddEffect(RpMTEffectDict* dict, RpMTEffect* effect);
extern RpMTEffect* RpMTEffectDictRemoveEffect(RpMTEffect* effect);
extern RpMTEffect* RpMTEffectCreateDummy(void);
extern void RpMTEffectDestroy(RpMTEffect* effect);
extern RpMTEffect* RpMTEffectFind(RwChar* name);
extern RpMTEffect* RpMTEffectSetName(RpMTEffect* effect, RwChar* name);
extern RpMTEffect* RpMTEffectAddRef(RpMTEffect* effect);

/* rpmatfx.c */
extern RwStream* _rpMatFXStreamWriteTexture(RwStream* stream, const RwTexture* texture);
extern RwStream* _rpMatFXStreamReadTexture(RwStream* stream, RwTexture** texture);
extern RwUInt32 _rpMatFXStreamSizeTexture(const RwTexture* texture);

/* string streaming (src) */
extern RwUInt32 _rwStringStreamGetSize(const RwChar* string);
extern const RwChar* _rwStringStreamWrite(const RwChar* string, RwStream* stream);
extern RwChar* _rwStringStreamFindAndRead(RwChar* string, RwStream* stream);

#endif
