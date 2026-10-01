#ifndef MATFXPRIVATE_H
#define MATFXPRIVATE_H

#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpmatfx.h>

/* Private declarations shared by the material effects plugin units. */

#define rpMAXPASS 2

#define rwID_MATERIALEFFECTSPLUGIN 0x120

#ifndef RWMODULEINFO_DEFINED
#define RWMODULEINFO_DEFINED
typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};
#endif

typedef struct MatFXBumpMapData MatFXBumpMapData;
struct MatFXBumpMapData
{
    RwFrame* frame;
    RwTexture* texture;
    RwTexture* bumpTexture;
    RwReal coef;
    RwReal invBumpWidth;
};

typedef struct MatFXEnvMapData MatFXEnvMapData;
struct MatFXEnvMapData
{
    RwFrame* frame;
    RwTexture* texture;
    RwReal coef;
    RwBool useFrameBufferAlpha;
};

typedef struct MatFXDualData MatFXDualData;
struct MatFXDualData
{
    RwTexture* texture;
    RwBlendFunction srcBlendMode;
    RwBlendFunction dstBlendMode;
};

typedef struct MatFXUVAnimData MatFXUVAnimData;
struct MatFXUVAnimData
{
    RwMatrix* baseTransform;
    RwMatrix* dualTransform;
};

typedef union MatFXEffectUnion MatFXEffectUnion;
union MatFXEffectUnion
{
    MatFXBumpMapData bumpMap;
    MatFXEnvMapData envMap;
    MatFXDualData dual;
    MatFXUVAnimData uvAnim;
};

typedef struct MatFXEffectData MatFXEffectData;
struct MatFXEffectData
{
    MatFXEffectUnion data;
    RpMatFXMaterialFlags flag;
};

typedef struct rpMatFXMaterialData rpMatFXMaterialData;
struct rpMatFXMaterialData
{
    MatFXEffectData data[rpMAXPASS];
    RpMatFXMaterialFlags flags;
};

typedef struct MatFXAtomicData MatFXAtomicData;
struct MatFXAtomicData
{
    RwBool enabled;
};

typedef struct MatFXWorldSectorData MatFXWorldSectorData;
struct MatFXWorldSectorData
{
    RwBool enabled;
};

typedef struct RwMatFXInfo RwMatFXInfo;
struct RwMatFXInfo
{
    RwModuleInfo Module;
    RwFreeList* MaterialData;
};

extern RwInt32 MatFXMaterialDataOffset;
extern RwMatFXInfo MatFXInfo;

#define MATFXMATERIALGETDATA(material)                                                             \
    ((rpMatFXMaterialData**)(((RwUInt8*)(material)) + MatFXMaterialDataOffset))

#define MATFXMATERIALGETCONSTDATA(material)                                                        \
    ((const rpMatFXMaterialData* const*)(((const RwUInt8*)(material)) + MatFXMaterialDataOffset))

/* rpmatfx.c */
extern RwStream* _rpMatFXStreamWriteTexture(RwStream* stream, const RwTexture* texture);
extern RwStream* _rpMatFXStreamReadTexture(RwStream* stream, RwTexture** texture);
extern RwUInt32 _rpMatFXStreamSizeTexture(const RwTexture* texture);
extern RwTexture* _rpMatFXTextureMaskCreate(const RwTexture* baseTexture,
                                            const RwTexture* maskTexture);

/* gcn/effectPipesGcn.c */
extern RwBool _rpMatFXPipelinesCreate(void);
extern RwBool _rpMatFXPipelinesDestroy(void);
extern RpAtomic* _rpMatFXPipelineAtomicSetup(RpAtomic* atomic);
extern RpWorldSector* _rpMatFXPipelineWorldSectorSetup(RpWorldSector* worldSector);
extern RwBool _rpMatFXSetupDualRenderState(MatFXDualData* dualData, RwRenderState nState);
extern RwTexture* _rpMatFXSetupBumpMapTexture(const RwTexture* baseTexture,
                                              const RwTexture* effectTexture);

/* gcn/multiTexGcn.c */
extern RwBool _rpMultiTexturePlatformPluginsAttach(void);

#endif
