#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpmatfx.h>
#include <string.h>

#include "rwsdk/plugin/matfx/matfxprivate.h"

#define rwID_TEXTURE 0x06

#define rwTEXTUREBASENAMELENGTH 32

#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define E_RW_NOERROR 0x80000000
#define E_RW_NOTEXTURE 0x16

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

#define MATFXATOMICGETDATA(atomic)                                                                 \
    ((MatFXAtomicData*)(((RwUInt8*)(atomic)) + MatFXAtomicDataOffset))

#define MATFXATOMICGETCONSTDATA(atomic)                                                            \
    ((const MatFXAtomicData*)(((const RwUInt8*)(atomic)) + MatFXAtomicDataOffset))

#define MATFXWORLDSECTORGETDATA(worldSector)                                                       \
    ((MatFXWorldSectorData*)(((RwUInt8*)(worldSector)) + MatFXWorldSectorDataOffset))

#define MATFXWORLDSECTORGETCONSTDATA(worldSector)                                                  \
    ((const MatFXWorldSectorData*)(((const RwUInt8*)(worldSector)) + MatFXWorldSectorDataOffset))

static RwInt32 MatFXWorldSectorDataOffset;
static RwInt32 MatFXAtomicDataOffset;

RwInt32 MatFXMaterialDataOffset;
RwMatFXInfo MatFXInfo = { { 0, 0 }, NULL };

static RwInt32 _rpMatFXMaterialDataFreeListBlockSize = 128;
static RwInt32 _rpMatFXMaterialDataFreeListPreallocBlocks = 1;
static RwFreeList _rpMatFXMaterialDataFreeList;

static MatFXEffectUnion* MatFXGetData(RpMaterial* material, RpMatFXMaterialFlags flags)
{
    rpMatFXMaterialData* materialData;
    RwUInt8 pass;

    materialData = *MATFXMATERIALGETDATA(material);

    for (pass = 0; pass < rpMAXPASS; pass++)
    {
        if (materialData->data[pass].flag == flags)
        {
            return &materialData->data[pass].data;
        }
    }

    return NULL;
}

static const MatFXEffectUnion* MatFXGetConstData(const RpMaterial* material,
                                                 RpMatFXMaterialFlags flags)
{
    const rpMatFXMaterialData* materialData;
    RwUInt8 pass;

    materialData = *MATFXMATERIALGETCONSTDATA(material);

    for (pass = 0; pass < rpMAXPASS; pass++)
    {
        if (materialData->data[pass].flag == flags)
        {
            return &materialData->data[pass].data;
        }
    }

    return NULL;
}

static void* MatFXClose(void* instance, RwInt32 offset, RwInt32 size)
{
    MatFXInfo.Module.numInstances--;
    if (MatFXInfo.Module.numInstances == 0)
    {
        _rpMatFXPipelinesDestroy();
    }

    if (MatFXInfo.MaterialData != NULL)
    {
        RwFreeListDestroy(MatFXInfo.MaterialData);
        MatFXInfo.MaterialData = NULL;
    }

    return instance;
}

static void* MatFXOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    if (MatFXInfo.Module.numInstances == 0)
    {
        MatFXInfo.MaterialData = RwFreeListCreateAndPreallocateSpace(
            sizeof(rpMatFXMaterialData), _rpMatFXMaterialDataFreeListBlockSize, sizeof(RwUInt32),
            _rpMatFXMaterialDataFreeListPreallocBlocks, &_rpMatFXMaterialDataFreeList);
        if (MatFXInfo.MaterialData == NULL)
        {
            return NULL;
        }

        if (!_rpMatFXPipelinesCreate())
        {
            return NULL;
        }
    }

    MatFXInfo.Module.numInstances++;

    return instance;
}

static void* MatFXMaterialConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    *MATFXMATERIALGETDATA(object) = NULL;

    return object;
}

static rpMatFXMaterialData* MatFXMaterialDataClean(rpMatFXMaterialData* materialData)
{
    RwUInt8 pass;
    MatFXBumpMapData* bumpMapData;
    MatFXEnvMapData* envMapData;
    MatFXDualData* dualData;
    MatFXUVAnimData* uvAnimData;

    for (pass = 0; pass < rpMAXPASS; pass++)
    {
        switch (materialData->data[pass].flag)
        {
        case rpMATFXEFFECTBUMPMAP:
            bumpMapData = &materialData->data[pass].data.bumpMap;

            if (bumpMapData->texture != NULL)
            {
                RwTextureDestroy(bumpMapData->texture);
            }

            if (bumpMapData->bumpTexture != NULL)
            {
                RwTextureDestroy(bumpMapData->bumpTexture);
            }
            break;
        case rpMATFXEFFECTENVMAP:
            envMapData = &materialData->data[pass].data.envMap;

            if (envMapData->texture != NULL)
            {
                RwTextureDestroy(envMapData->texture);
            }
            break;
        case rpMATFXEFFECTDUAL:
            dualData = &materialData->data[pass].data.dual;

            if (dualData->texture != NULL)
            {
                RwTextureDestroy(dualData->texture);
            }
            break;
        case rpMATFXEFFECTUVTRANSFORM:
            uvAnimData = &materialData->data[pass].data.uvAnim;

            uvAnimData->baseTransform = NULL;
            uvAnimData->dualTransform = NULL;
            break;
        default:
            break;
        }
    }

    memset(materialData, 0, sizeof(rpMatFXMaterialData));

    return materialData;
}

static void* MatFXMaterialDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    rpMatFXMaterialData* materialData;

    materialData = *MATFXMATERIALGETDATA(object);
    if (materialData != NULL)
    {
        MatFXMaterialDataClean(materialData);

        RwFreeListFree(MatFXInfo.MaterialData, materialData);
        *MATFXMATERIALGETDATA(object) = NULL;
    }

    return object;
}

static rpMatFXMaterialData* MatFXMaterialGetData(RpMaterial* material)
{
    rpMatFXMaterialData* materialData;

    materialData = *MATFXMATERIALGETDATA(material);
    if (materialData == NULL)
    {
        materialData = (rpMatFXMaterialData*)RwFreeListAlloc(MatFXInfo.MaterialData);
        if (materialData == NULL)
        {
            return NULL;
        }

        memset(materialData, 0, sizeof(rpMatFXMaterialData));
        *MATFXMATERIALGETDATA(material) = materialData;
    }

    return materialData;
}

static void* MatFXMaterialCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    const RpMaterial* srcMaterial;
    const rpMatFXMaterialData* srcMaterialData;
    RpMaterial* dstMaterial;
    rpMatFXMaterialData* dstMaterialData;
    RwUInt8 pass;
    RpMatFXMaterialFlags effect;

    srcMaterial = (const RpMaterial*)srcObject;
    dstMaterial = (RpMaterial*)dstObject;

    srcMaterialData = *MATFXMATERIALGETCONSTDATA(srcMaterial);
    if (srcMaterialData == NULL)
    {
        return NULL;
    }

    dstMaterialData = MatFXMaterialGetData(dstMaterial);
    if (dstMaterialData == NULL)
    {
        return NULL;
    }

    RpMatFXMaterialSetEffects(dstMaterial, srcMaterialData->flags);

    for (pass = 0; pass < rpMAXPASS; pass++)
    {
        effect = srcMaterialData->data[pass].flag;

        switch (effect)
        {
        case rpMATFXEFFECTBUMPMAP:
        {
            const MatFXBumpMapData* srcBumpMapData;
            MatFXBumpMapData* dstBumpMapData;
            RwFrame* frame;
            RwReal coef;

            srcBumpMapData = &srcMaterialData->data[pass].data.bumpMap;
            dstBumpMapData = &dstMaterialData->data[pass].data.bumpMap;

            frame = RpMatFXMaterialGetBumpMapFrame(srcMaterial);
            coef = RpMatFXMaterialGetBumpMapCoefficient(srcMaterial);

            RpMatFXMaterialSetBumpMapFrame(dstMaterial, frame);
            RpMatFXMaterialSetBumpMapCoefficient(dstMaterial, coef);

            dstBumpMapData->texture = srcBumpMapData->texture;
            dstBumpMapData->bumpTexture = srcBumpMapData->bumpTexture;

            if (dstBumpMapData->texture != NULL)
            {
                RwTextureAddRef(dstBumpMapData->texture);
            }

            if (dstBumpMapData->bumpTexture != NULL)
            {
                RwTextureAddRef(dstBumpMapData->bumpTexture);
            }
            break;
        }
        case rpMATFXEFFECTENVMAP:
        {
            RwTexture* texture;
            RwFrame* frame;
            RwReal coef;
            RwBool useFrameBufferAlpha;

            texture = RpMatFXMaterialGetEnvMapTexture(srcMaterial);
            frame = RpMatFXMaterialGetEnvMapFrame(srcMaterial);
            coef = RpMatFXMaterialGetEnvMapCoefficient(srcMaterial);
            useFrameBufferAlpha = RpMatFXMaterialGetEnvMapFrameBufferAlpha(srcMaterial);

            if (texture != NULL)
            {
                RpMatFXMaterialSetEnvMapTexture(dstMaterial, texture);
            }

            RpMatFXMaterialSetEnvMapFrame(dstMaterial, frame);
            RpMatFXMaterialSetEnvMapFrameBufferAlpha(dstMaterial, useFrameBufferAlpha);
            RpMatFXMaterialSetEnvMapCoefficient(dstMaterial, coef);
            break;
        }
        case rpMATFXEFFECTDUAL:
        {
            RwTexture* texture;
            RwBlendFunction srcBlendMode;
            RwBlendFunction dstBlendMode;

            texture = RpMatFXMaterialGetDualTexture(srcMaterial);
            RpMatFXMaterialGetDualBlendModes(srcMaterial, &srcBlendMode, &dstBlendMode);

            if (texture != NULL)
            {
                RpMatFXMaterialSetDualTexture(dstMaterial, texture);
            }

            RpMatFXMaterialSetDualBlendModes(dstMaterial, srcBlendMode, dstBlendMode);
            break;
        }
        case rpMATFXEFFECTUVTRANSFORM:
        {
            RwMatrix* baseTransform;
            RwMatrix* dualTransform;

            srcMaterial =
                RpMatFXMaterialGetUVTransformMatrices(srcMaterial, &baseTransform, &dualTransform);
            dstMaterial =
                RpMatFXMaterialSetUVTransformMatrices(dstMaterial, baseTransform, dualTransform);
            break;
        }
        default:
            break;
        }
    }

    return dstObject;
}

RwStream* _rpMatFXStreamWriteTexture(RwStream* stream, const RwTexture* texture)
{
    RwBool present;

    present = (texture != NULL);

    if (!RwStreamWriteInt32(stream, &present, sizeof(present)))
    {
        return NULL;
    }

    if (present)
    {
        if (!RwTextureStreamWrite(texture, stream))
        {
            return NULL;
        }
    }

    return stream;
}

RwStream* _rpMatFXStreamReadTexture(RwStream* stream, RwTexture** texture)
{
    RwBool present;
    RwError err;

    if (!RwStreamReadInt32(stream, &present, sizeof(present)))
    {
        return NULL;
    }

    if (present)
    {
        if (!RwStreamFindChunk(stream, rwID_TEXTURE, NULL, NULL))
        {
            return NULL;
        }

        RwErrorGet(&err);

        *texture = RwTextureStreamRead(stream);
        if (*texture == NULL)
        {
            RwErrorGet(&err);
            if (err.errorCode != E_RW_NOERROR && err.errorCode != E_RW_NOTEXTURE)
            {
                RwErrorSet(&err);
                return NULL;
            }
        }
    }
    else
    {
        *texture = NULL;
    }

    return stream;
}

RwUInt32 _rpMatFXStreamSizeTexture(const RwTexture* texture)
{
    RwUInt32 size;

    size = sizeof(RwBool);

    if (texture != NULL)
    {
        size += RwTextureStreamGetSize(texture) + rwCHUNKHEADERSIZE;
    }

    return size;
}

static RwStream* MatFXMaterialStreamWrite(RwStream* stream, RwInt32 binaryLength,
                                          const void* object, RwInt32 offset, RwInt32 size)
{
    const RpMaterial* material;
    const rpMatFXMaterialData* materialData;
    RwUInt8 pass;
    RwInt32 temp;
    RpMatFXMaterialFlags effect;

    material = (const RpMaterial*)object;
    materialData = *MATFXMATERIALGETCONSTDATA(material);

    temp = materialData->flags;
    if (!RwStreamWriteInt32(stream, &temp, sizeof(temp)))
    {
        return NULL;
    }

    for (pass = 0; pass < rpMAXPASS; pass++)
    {
        effect = materialData->data[pass].flag;

        if (!RwStreamWriteInt32(stream, (RwInt32*)&effect, sizeof(effect)))
        {
            return NULL;
        }

        switch (effect)
        {
        case rpMATFXEFFECTBUMPMAP:
        {
            const MatFXBumpMapData* bumpData;
            RwReal coef;

            bumpData = &materialData->data[pass].data.bumpMap;

            coef = -bumpData->coef;
            if (!RwStreamWriteReal(stream, &coef, sizeof(coef)))
            {
                return NULL;
            }

            if (!_rpMatFXStreamWriteTexture(stream, bumpData->texture))
            {
                return NULL;
            }

            if (!_rpMatFXStreamWriteTexture(stream, bumpData->bumpTexture))
            {
                return NULL;
            }
            break;
        }
        case rpMATFXEFFECTENVMAP:
        {
            const MatFXEnvMapData* envData;

            envData = &materialData->data[pass].data.envMap;

            if (!RwStreamWriteReal(stream, &envData->coef, sizeof(envData->coef)))
            {
                return NULL;
            }

            temp = envData->useFrameBufferAlpha;
            if (!RwStreamWriteInt32(stream, &temp, sizeof(temp)))
            {
                return NULL;
            }

            if (!_rpMatFXStreamWriteTexture(stream, envData->texture))
            {
                return NULL;
            }
            break;
        }
        case rpMATFXEFFECTDUAL:
        {
            const MatFXDualData* dualData;

            dualData = &materialData->data[pass].data.dual;

            temp = dualData->srcBlendMode;
            if (!RwStreamWriteInt32(stream, &temp, sizeof(temp)))
            {
                return NULL;
            }

            temp = dualData->dstBlendMode;
            if (!RwStreamWriteInt32(stream, &temp, sizeof(temp)))
            {
                return NULL;
            }

            if (!_rpMatFXStreamWriteTexture(stream, dualData->texture))
            {
                return NULL;
            }
            break;
        }
        case rpMATFXEFFECTBUMPENVMAP:
        case rpMATFXEFFECTUVTRANSFORM:
        default:
            break;
        }
    }

    return stream;
}

static RwStream* MatFXMaterialStreamRead(RwStream* stream, RwInt32 binaryLength, void* object,
                                         RwInt32 offset, RwInt32 size)
{
    RpMaterial* material;
    rpMatFXMaterialData* materialData;
    RpMatFXMaterialFlags flags;
    RwUInt8 pass;
    RpMatFXMaterialFlags effect;

    material = (RpMaterial*)object;

    materialData = MatFXMaterialGetData(material);
    if (materialData == NULL)
    {
        return NULL;
    }

    if (!RwStreamReadInt32(stream, (RwInt32*)&flags, sizeof(flags)))
    {
        return NULL;
    }

    RpMatFXMaterialSetEffects(material, flags);

    for (pass = 0; pass < rpMAXPASS; pass++)
    {
        if (!RwStreamReadInt32(stream, (RwInt32*)&effect, sizeof(effect)))
        {
            return NULL;
        }

        switch (effect)
        {
        case rpMATFXEFFECTBUMPMAP:
        {
            RwReal coef;
            RwTexture* texture;
            RwTexture* bumpTexture;
            MatFXBumpMapData* bumpMapData;

            bumpMapData = &materialData->data[pass].data.bumpMap;
            texture = NULL;
            bumpTexture = NULL;

            if (!RwStreamReadReal(stream, &coef, sizeof(coef)))
            {
                return NULL;
            }

            if (!_rpMatFXStreamReadTexture(stream, &texture))
            {
                return NULL;
            }

            if (!_rpMatFXStreamReadTexture(stream, &bumpTexture))
            {
                if (texture != NULL)
                {
                    RwTextureDestroy(texture);
                }

                return NULL;
            }

            if (texture != NULL)
            {
                RwRaster* raster;
                RwInt32 nWidth;
                RwReal width;

                bumpMapData->texture = texture;
                bumpMapData->bumpTexture = bumpTexture;

                raster = RwTextureGetRaster(bumpMapData->texture);
                nWidth = RwRasterGetWidth(raster);
                width = (RwReal)nWidth;
                bumpMapData->invBumpWidth = 1.0f / width;
            }
            else if (bumpTexture != NULL)
            {
                RpMatFXMaterialSetBumpMapTexture(material, bumpTexture);
                RwTextureDestroy(bumpTexture);
            }
            else
            {
                bumpMapData->texture = NULL;
                bumpMapData->bumpTexture = NULL;
            }

            RpMatFXMaterialSetBumpMapCoefficient(material, coef);
            break;
        }
        case rpMATFXEFFECTENVMAP:
        {
            RwReal coef;
            RwBool useFrameBufferAlpha;
            RwTexture* texture;

            texture = NULL;

            if (!RwStreamReadReal(stream, &coef, sizeof(coef)))
            {
                return NULL;
            }

            if (!RwStreamReadInt32(stream, &useFrameBufferAlpha, sizeof(useFrameBufferAlpha)))
            {
                return NULL;
            }

            if (!_rpMatFXStreamReadTexture(stream, &texture))
            {
                return NULL;
            }

            if (texture != NULL)
            {
                RpMatFXMaterialSetEnvMapTexture(material, texture);
                RwTextureDestroy(texture);
            }

            RpMatFXMaterialSetEnvMapCoefficient(material, coef);
            RpMatFXMaterialSetEnvMapFrameBufferAlpha(material, useFrameBufferAlpha);
            break;
        }
        case rpMATFXEFFECTDUAL:
        {
            RwBlendFunction blendFuncs[2];
            RwTexture* texture;

            texture = NULL;

            if (!RwStreamReadInt32(stream, (RwInt32*)blendFuncs, sizeof(blendFuncs)))
            {
                return NULL;
            }

            if (!_rpMatFXStreamReadTexture(stream, &texture))
            {
                return NULL;
            }

            if (texture != NULL)
            {
                RpMatFXMaterialSetDualTexture(material, texture);
                RwTextureDestroy(texture);
            }

            RpMatFXMaterialSetDualBlendModes(material, blendFuncs[0], blendFuncs[1]);
            break;
        }
        case rpMATFXEFFECTBUMPENVMAP:
        case rpMATFXEFFECTUVTRANSFORM:
        default:
            break;
        }
    }

    return stream;
}

static RwInt32 MatFXMaterialStreamGetSize(const void* object, RwInt32 offset, RwInt32 sizeInObj)
{
    const RpMaterial* material;
    const rpMatFXMaterialData* materialData;
    RwInt32 size;
    RwUInt8 pass;
    RpMatFXMaterialFlags effect;

    material = (const RpMaterial*)object;
    materialData = *MATFXMATERIALGETCONSTDATA(material);

    if (materialData == NULL || materialData->flags == rpMATFXEFFECTNULL)
    {
        return 0;
    }

    size = sizeof(RwInt32);

    for (pass = 0; pass < rpMAXPASS; pass++)
    {
        size += sizeof(RwInt32);

        effect = materialData->data[pass].flag;

        switch (effect)
        {
        case rpMATFXEFFECTBUMPMAP:
            size += sizeof(RwReal);
            size += _rpMatFXStreamSizeTexture(materialData->data[pass].data.bumpMap.texture);
            size += _rpMatFXStreamSizeTexture(materialData->data[pass].data.bumpMap.bumpTexture);
            break;
        case rpMATFXEFFECTENVMAP:
            size += sizeof(RwReal) + sizeof(RwBool);
            size += _rpMatFXStreamSizeTexture(materialData->data[pass].data.envMap.texture);
            break;
        case rpMATFXEFFECTDUAL:
            size += sizeof(RwBlendFunction) * 2;
            size += _rpMatFXStreamSizeTexture(materialData->data[pass].data.dual.texture);
            break;
        case rpMATFXEFFECTBUMPENVMAP:
        case rpMATFXEFFECTUVTRANSFORM:
        default:
            break;
        }
    }

    return size;
}

static void* MatFXAtomicConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXATOMICGETDATA(object)->enabled = FALSE;

    return object;
}

static void* MatFXAtomicDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXATOMICGETDATA(object)->enabled = FALSE;

    return object;
}

static void* MatFXAtomicCopy(void* dstObject, const void* srcObject, RwInt32 offset, RwInt32 size)
{
    if (MATFXATOMICGETCONSTDATA(srcObject)->enabled)
    {
        MATFXATOMICGETDATA(dstObject)->enabled = TRUE;
    }

    return dstObject;
}

static RwStream* MatFXAtomicStreamWrite(RwStream* stream, RwInt32 binaryLength, const void* object,
                                        RwInt32 offset, RwInt32 size)
{
    RwStream* streamOut;
    RwInt32 temp;

    temp = MATFXATOMICGETCONSTDATA(object)->enabled;
    streamOut = RwStreamWriteInt32(stream, &temp, sizeof(temp));

    return streamOut;
}

static RwStream* MatFXAtomicStreamRead(RwStream* stream, RwInt32 binaryLength, void* object,
                                       RwInt32 offset, RwInt32 size)
{
    RwBool enabled;

    if (!RwStreamReadInt32(stream, &enabled, sizeof(enabled)))
    {
        return NULL;
    }

    if (enabled)
    {
        RpMatFXAtomicEnableEffects((RpAtomic*)object);
    }

    return stream;
}

static RwInt32 MatFXAtomicStreamGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    return (MATFXATOMICGETCONSTDATA(object)->enabled == FALSE) ? 0 : sizeof(RwInt32);
}

static void* MatFXWorldSectorConstructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXWORLDSECTORGETDATA(object)->enabled = FALSE;

    return object;
}

static void* MatFXWorldSectorDestructor(void* object, RwInt32 offset, RwInt32 size)
{
    MATFXWORLDSECTORGETDATA(object)->enabled = FALSE;

    return object;
}

static void* MatFXWorldSectorCopy(void* dstObject, const void* srcObject, RwInt32 offset,
                                  RwInt32 size)
{
    if (MATFXWORLDSECTORGETCONSTDATA(srcObject)->enabled)
    {
        MATFXWORLDSECTORGETDATA(dstObject)->enabled = TRUE;
    }

    return dstObject;
}

static RwStream* MatFXWorldSectorStreamWrite(RwStream* stream, RwInt32 binaryLength,
                                             const void* object, RwInt32 offset, RwInt32 size)
{
    RwStream* streamOut;
    RwInt32 temp;

    temp = MATFXWORLDSECTORGETCONSTDATA(object)->enabled;
    streamOut = RwStreamWriteInt32(stream, &temp, sizeof(temp));

    return streamOut;
}

static RwStream* MatFXWorldSectorStreamRead(RwStream* stream, RwInt32 binaryLength, void* object,
                                            RwInt32 offset, RwInt32 size)
{
    RwBool enabled;

    if (!RwStreamReadInt32(stream, &enabled, sizeof(enabled)))
    {
        return NULL;
    }

    if (enabled)
    {
        RpMatFXWorldSectorEnableEffects((RpWorldSector*)object);
    }

    return stream;
}

static RwInt32 MatFXWorldSectorStreamGetSize(const void* object, RwInt32 offset, RwInt32 size)
{
    return (MATFXWORLDSECTORGETCONSTDATA(object)->enabled == FALSE) ? 0 : sizeof(RwInt32);
}

static void GenBumpedTextureName(RwChar* bumpedTexName, const RwTexture* baseTexture,
                                 const RwTexture* maskTexture)
{
    RwInt32 i;
    RwInt32 j;
    const RwChar* oldName[2];
    RwChar* newPtr;

    newPtr = bumpedTexName;
    i = 0;

    if (baseTexture != NULL)
    {
        oldName[0] = baseTexture->name;
        oldName[1] = maskTexture->name;
    }
    else
    {
        oldName[0] = oldName[1] = maskTexture->name;
    }

    while (i < rwTEXTUREBASENAMELENGTH - 2 && (*oldName[0] != '\0' || *oldName[1] != '\0'))
    {
        for (j = 0; j < 2; j++)
        {
            if (*oldName[j] != '\0')
            {
                *newPtr++ = *oldName[j]++;
                i++;
            }
        }
    }

    *newPtr = '\0';
}

RwTexture* _rpMatFXTextureMaskCreate(const RwTexture* baseTexture, const RwTexture* maskTexture)
{
    RwRaster* baseRaster;
    RwRaster* maskRaster;
    RwRaster* newRaster;
    RwInt32 maskX;
    RwInt32 maskY;
    RwInt32 baseX;
    RwInt32 baseY;
    RwImage* maskImage;
    RwImage* baseImage;
    RwTexture* result;
    RwTextureFilterMode filterMode;
    RwTextureAddressMode addrsMode;
    RwInt32 x;
    RwInt32 y;
    RwInt32 rasterWidth;
    RwInt32 rasterHeight;
    RwInt32 rasterDepth;
    RwInt32 rasterFlags;
    RwInt32 rasterBaseFlags;

    maskRaster = RwTextureGetRaster(maskTexture);
    baseRaster = NULL;

    maskX = RwRasterGetWidth(maskRaster);
    maskY = RwRasterGetHeight(maskRaster);

    maskImage = RwImageCreate(maskX, maskY, 32);
    RwImageAllocatePixels(maskImage);
    RwImageSetFromRaster(maskImage, maskRaster);

    if (baseTexture != NULL)
    {
        baseRaster = RwTextureGetRaster(baseTexture);

        baseX = RwRasterGetWidth(baseRaster);
        baseY = RwRasterGetHeight(baseRaster);

        baseImage = RwImageCreate(baseX, baseY, 32);
        RwImageAllocatePixels(baseImage);
        RwImageSetFromRaster(baseImage, baseRaster);
    }
    else
    {
        baseX = RwRasterGetWidth(maskRaster);
        baseY = RwRasterGetHeight(maskRaster);

        baseImage = RwImageCreate(baseX, baseY, 32);
        RwImageAllocatePixels(baseImage);

        for (y = 0; y < baseY; y++)
        {
            for (x = 0; x < baseX; x++)
            {
                ((RwUInt32*)(baseImage->cpPixels + baseImage->stride * y))[x] = 0xFFFFFFFF;
            }
        }
    }

    if (baseX != maskX || baseY != maskY)
    {
        RwImage* resampleImage;

        resampleImage = RwImageCreate(baseX, baseY, 32);
        RwImageAllocatePixels(resampleImage);
        RwImageResample(resampleImage, maskImage);
        RwImageDestroy(maskImage);
        maskImage = resampleImage;
    }

    RwImageMakeMask(maskImage);
    RwImageApplyMask(baseImage, maskImage);

    RwImageFindRasterFormat(baseImage, rwRASTERTYPETEXTURE, &rasterWidth, &rasterHeight,
                            &rasterDepth, &rasterFlags);

    if (baseTexture != NULL)
    {
        rasterBaseFlags = RwRasterGetFormat(baseRaster);
    }
    else
    {
        rasterBaseFlags = RwRasterGetFormat(maskRaster);
    }

    if (rasterBaseFlags & rwRASTERFORMATMIPMAP)
    {
        rasterFlags |= rwRASTERFORMATMIPMAP | rwRASTERFORMATAUTOMIPMAP;
    }

    newRaster = RwRasterCreate(rasterWidth, rasterHeight, rasterDepth, rasterFlags);
    RwRasterSetFromImage(newRaster, baseImage);

    result = RwTextureCreate(newRaster);

    if (baseTexture != NULL)
    {
        filterMode = RwTextureGetFilterMode(baseTexture);
        addrsMode = RwTextureGetAddressing(baseTexture);
    }
    else
    {
        filterMode = RwTextureGetFilterMode(maskTexture);
        addrsMode = RwTextureGetAddressing(maskTexture);
    }

    RwTextureSetAddressing(result, addrsMode);
    RwTextureSetFilterMode(result, filterMode);

    RwImageDestroy(baseImage);
    RwImageDestroy(maskImage);

    {
        RwChar newName[rwTEXTUREBASENAMELENGTH] = { 0 };

        GenBumpedTextureName(newName, baseTexture, maskTexture);
        RwTextureSetName(result, newName);
    }

    return result;
}

RwBool RpMatFXPluginAttach(void)
{
    RwInt32 offset;

    offset = RwEngineRegisterPlugin(0, rwID_MATERIALEFFECTSPLUGIN, MatFXOpen, MatFXClose);
    if (offset < 0)
    {
        return FALSE;
    }

    MatFXMaterialDataOffset =
        RpMaterialRegisterPlugin(sizeof(rpMatFXMaterialData*), rwID_MATERIALEFFECTSPLUGIN,
                                 MatFXMaterialConstructor, MatFXMaterialDestructor,
                                 MatFXMaterialCopy);
    if (MatFXMaterialDataOffset < 0)
    {
        return FALSE;
    }

    offset = RpMaterialRegisterPluginStream(rwID_MATERIALEFFECTSPLUGIN, MatFXMaterialStreamRead,
                                            MatFXMaterialStreamWrite, MatFXMaterialStreamGetSize);
    if (offset < 0)
    {
        return FALSE;
    }

    MatFXAtomicDataOffset =
        RpAtomicRegisterPlugin(sizeof(MatFXAtomicData), rwID_MATERIALEFFECTSPLUGIN,
                               MatFXAtomicConstructor, MatFXAtomicDestructor, MatFXAtomicCopy);
    if (MatFXAtomicDataOffset < 0)
    {
        return FALSE;
    }

    offset = RpAtomicRegisterPluginStream(rwID_MATERIALEFFECTSPLUGIN, MatFXAtomicStreamRead,
                                          MatFXAtomicStreamWrite, MatFXAtomicStreamGetSize);
    if (offset < 0)
    {
        return FALSE;
    }

    MatFXWorldSectorDataOffset =
        RpWorldSectorRegisterPlugin(sizeof(MatFXWorldSectorData), rwID_MATERIALEFFECTSPLUGIN,
                                    MatFXWorldSectorConstructor, MatFXWorldSectorDestructor,
                                    MatFXWorldSectorCopy);
    if (MatFXWorldSectorDataOffset < 0)
    {
        return FALSE;
    }

    offset =
        RpWorldSectorRegisterPluginStream(rwID_MATERIALEFFECTSPLUGIN, MatFXWorldSectorStreamRead,
                                          MatFXWorldSectorStreamWrite,
                                          MatFXWorldSectorStreamGetSize);
    if (offset < 0)
    {
        return FALSE;
    }

    return _rpMultiTexturePlatformPluginsAttach() != FALSE;
}

RpAtomic* RpMatFXAtomicEnableEffects(RpAtomic* atomic)
{
    MatFXAtomicData* atomicData;

    atomicData = MATFXATOMICGETDATA(atomic);
    if (!atomicData->enabled)
    {
        if (!_rpMatFXPipelineAtomicSetup(atomic))
        {
            return NULL;
        }

        atomicData->enabled = TRUE;
    }

    return atomic;
}

RpWorldSector* RpMatFXWorldSectorEnableEffects(RpWorldSector* worldSector)
{
    MatFXWorldSectorData* worldSectorData;

    worldSectorData = MATFXWORLDSECTORGETDATA(worldSector);
    if (!worldSectorData->enabled)
    {
        if (!_rpMatFXPipelineWorldSectorSetup(worldSector))
        {
            return NULL;
        }

        worldSectorData->enabled = TRUE;
    }

    return worldSector;
}

RpMaterial* RpMatFXMaterialSetEffects(RpMaterial* material, RpMatFXMaterialFlags flags)
{
    rpMatFXMaterialData* materialData;

    materialData = MatFXMaterialGetData(material);
    if (materialData == NULL)
    {
        return NULL;
    }

    if (flags == rpMATFXEFFECTNULL ||
        (materialData->flags != rpMATFXEFFECTNULL && materialData->flags != flags))
    {
        MatFXMaterialDataClean(materialData);
    }

    materialData->flags = flags;

    switch (materialData->flags)
    {
    case rpMATFXEFFECTNULL:
        break;
    case rpMATFXEFFECTBUMPMAP:
        materialData->data[0].flag = rpMATFXEFFECTBUMPMAP;
        break;
    case rpMATFXEFFECTENVMAP:
        materialData->data[0].flag = rpMATFXEFFECTENVMAP;
        break;
    case rpMATFXEFFECTBUMPENVMAP:
        materialData->data[0].flag = rpMATFXEFFECTBUMPMAP;
        materialData->data[1].flag = rpMATFXEFFECTENVMAP;
        break;
    case rpMATFXEFFECTDUAL:
        materialData->data[0].flag = rpMATFXEFFECTDUAL;
        RpMatFXMaterialSetDualBlendModes(material, rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
        break;
    case rpMATFXEFFECTUVTRANSFORM:
        materialData->data[0].flag = rpMATFXEFFECTUVTRANSFORM;
        break;
    case rpMATFXEFFECTDUALUVTRANSFORM:
        materialData->data[0].flag = rpMATFXEFFECTUVTRANSFORM;
        materialData->data[1].flag = rpMATFXEFFECTDUAL;
        RpMatFXMaterialSetDualBlendModes(material, rwBLENDSRCALPHA, rwBLENDINVSRCALPHA);
        break;
    default:
        break;
    }

    return material;
}

RpMaterial* RpMatFXMaterialSetupBumpMap(RpMaterial* material, RwTexture* texture, RwFrame* frame,
                                        RwReal coef)
{
    if (RpMatFXMaterialSetBumpMapTexture(material, texture) == NULL)
    {
        return NULL;
    }

    if (RpMatFXMaterialSetBumpMapFrame(material, frame) == NULL)
    {
        return NULL;
    }

    if (RpMatFXMaterialSetBumpMapCoefficient(material, coef) == NULL)
    {
        return NULL;
    }

    return material;
}

RpMaterial* RpMatFXMaterialSetupEnvMap(RpMaterial* material, RwTexture* texture, RwFrame* frame,
                                       RwBool useFrameBufferAlpha, RwReal coef)
{
    if (RpMatFXMaterialSetEnvMapTexture(material, texture) == NULL)
    {
        return NULL;
    }

    if (RpMatFXMaterialSetEnvMapFrame(material, frame) == NULL)
    {
        return NULL;
    }

    if (RpMatFXMaterialSetEnvMapFrameBufferAlpha(material, useFrameBufferAlpha) == NULL)
    {
        return NULL;
    }

    if (RpMatFXMaterialSetEnvMapCoefficient(material, coef) == NULL)
    {
        return NULL;
    }

    return material;
}

RpMatFXMaterialFlags RpMatFXMaterialGetEffects(const RpMaterial* material)
{
    const rpMatFXMaterialData* materialData;

    materialData = *MATFXMATERIALGETCONSTDATA(material);
    if (materialData == NULL)
    {
        return rpMATFXEFFECTNULL;
    }

    return materialData->flags;
}

RpMaterial* RpMatFXMaterialSetBumpMapTexture(RpMaterial* material, RwTexture* bumpTexture)
{
    MatFXBumpMapData* bumpMapData;
    RwBool dummyTextures;
    RwRaster* bumpRaster;
    RwTexture* baseTexture;

    bumpMapData = &MatFXGetData(material, rpMATFXEFFECTBUMPMAP)->bumpMap;

    if (bumpMapData->bumpTexture != NULL)
    {
        RwTextureDestroy(bumpMapData->bumpTexture);
        bumpMapData->bumpTexture = NULL;
    }

    if (bumpMapData->texture != NULL)
    {
        RwTextureDestroy(bumpMapData->texture);
        bumpMapData->texture = NULL;
        bumpMapData->invBumpWidth = 0.0f;
    }

    if (bumpTexture != NULL)
    {
        bumpMapData->bumpTexture = bumpTexture;
        RwTextureAddRef(bumpMapData->bumpTexture);

        bumpRaster = RwTextureGetRaster(bumpTexture);
        baseTexture = material->texture;

        dummyTextures = (RwRasterGetWidth(bumpRaster) == 0);
        if (!dummyTextures && baseTexture != NULL)
        {
            dummyTextures = (RwRasterGetWidth(RwTextureGetRaster(baseTexture)) == 0);
        }

        if (!dummyTextures)
        {
            RwChar bumpedName[rwTEXTUREBASENAMELENGTH] = { 0 };
            RwTexDictionary* dict;

            GenBumpedTextureName(bumpedName, baseTexture, bumpTexture);

            dict = RwTexDictionaryGetCurrent();

            bumpMapData->texture = NULL;
            if (dict != NULL)
            {
                bumpMapData->texture = RwTexDictionaryFindNamedTexture(dict, bumpedName);
            }

            if (bumpMapData->texture == NULL)
            {
                bumpMapData->texture = _rpMatFXSetupBumpMapTexture(baseTexture, bumpTexture);
                if (bumpMapData->texture == NULL)
                {
                    return NULL;
                }

                if (dict != NULL)
                {
                    RwTexDictionaryAddTexture(dict, bumpMapData->texture);
                }
            }
            else
            {
                RwTextureAddRef(bumpMapData->texture);
            }

            bumpMapData->invBumpWidth =
                1.0f / (RwReal)RwRasterGetWidth(RwTextureGetRaster(bumpMapData->texture));
        }
    }
    else
    {
        bumpMapData->invBumpWidth =
            1.0f / (RwReal)RwRasterGetWidth(RwTextureGetRaster(material->texture));
    }

    return material;
}

RpMaterial* RpMatFXMaterialSetBumpMapFrame(RpMaterial* material, RwFrame* frame)
{
    MatFXBumpMapData* bumpMapData;

    bumpMapData = &MatFXGetData(material, rpMATFXEFFECTBUMPMAP)->bumpMap;
    bumpMapData->frame = frame;

    return material;
}

RpMaterial* RpMatFXMaterialSetBumpMapCoefficient(RpMaterial* material, RwReal coef)
{
    MatFXBumpMapData* bumpMapData;

    bumpMapData = &MatFXGetData(material, rpMATFXEFFECTBUMPMAP)->bumpMap;
    bumpMapData->coef = -coef;

    return material;
}

RwFrame* RpMatFXMaterialGetBumpMapFrame(const RpMaterial* material)
{
    const MatFXBumpMapData* bumpMapData;

    bumpMapData = &MatFXGetConstData(material, rpMATFXEFFECTBUMPMAP)->bumpMap;

    return bumpMapData->frame;
}

RwReal RpMatFXMaterialGetBumpMapCoefficient(const RpMaterial* material)
{
    const MatFXBumpMapData* bumpMapData;

    bumpMapData = &MatFXGetConstData(material, rpMATFXEFFECTBUMPMAP)->bumpMap;

    return -bumpMapData->coef;
}

RpMaterial* RpMatFXMaterialSetEnvMapTexture(RpMaterial* material, RwTexture* texture)
{
    MatFXEnvMapData* envMapData;

    envMapData = &MatFXGetData(material, rpMATFXEFFECTENVMAP)->envMap;

    RwTextureAddRef(texture);

    if (envMapData->texture != NULL)
    {
        RwTextureDestroy(envMapData->texture);
        envMapData->texture = NULL;
    }

    envMapData->texture = texture;

    return material;
}

RpMaterial* RpMatFXMaterialSetEnvMapFrame(RpMaterial* material, RwFrame* frame)
{
    MatFXEnvMapData* envMapData;

    envMapData = &MatFXGetData(material, rpMATFXEFFECTENVMAP)->envMap;
    envMapData->frame = frame;

    return material;
}

RpMaterial* RpMatFXMaterialSetEnvMapFrameBufferAlpha(RpMaterial* material,
                                                     RwBool useFrameBufferAlpha)
{
    MatFXEnvMapData* envMapData;

    envMapData = &MatFXGetData(material, rpMATFXEFFECTENVMAP)->envMap;
    envMapData->useFrameBufferAlpha = useFrameBufferAlpha;

    return material;
}

RpMaterial* RpMatFXMaterialSetEnvMapCoefficient(RpMaterial* material, RwReal coef)
{
    MatFXEnvMapData* envMapData;

    envMapData = &MatFXGetData(material, rpMATFXEFFECTENVMAP)->envMap;
    envMapData->coef = coef;

    return material;
}

RwTexture* RpMatFXMaterialGetEnvMapTexture(const RpMaterial* material)
{
    const MatFXEnvMapData* envMapData;

    envMapData = &MatFXGetConstData(material, rpMATFXEFFECTENVMAP)->envMap;

    return envMapData->texture;
}

RwFrame* RpMatFXMaterialGetEnvMapFrame(const RpMaterial* material)
{
    const MatFXEnvMapData* envMapData;

    envMapData = &MatFXGetConstData(material, rpMATFXEFFECTENVMAP)->envMap;

    return envMapData->frame;
}

RwBool RpMatFXMaterialGetEnvMapFrameBufferAlpha(const RpMaterial* material)
{
    const MatFXEnvMapData* envMapData;

    envMapData = &MatFXGetConstData(material, rpMATFXEFFECTENVMAP)->envMap;

    return envMapData->useFrameBufferAlpha;
}

RwReal RpMatFXMaterialGetEnvMapCoefficient(const RpMaterial* material)
{
    const MatFXEnvMapData* envMapData;

    envMapData = &MatFXGetConstData(material, rpMATFXEFFECTENVMAP)->envMap;

    return envMapData->coef;
}

RpMaterial* RpMatFXMaterialSetDualTexture(RpMaterial* material, RwTexture* texture)
{
    MatFXDualData* dualData;

    dualData = &MatFXGetData(material, rpMATFXEFFECTDUAL)->dual;

    RwTextureAddRef(texture);

    if (dualData->texture != NULL)
    {
        RwTextureDestroy(dualData->texture);
        dualData->texture = NULL;
    }

    dualData->texture = texture;

    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATETEXTUREADDRESS);
    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATETEXTURERASTER);

    return material;
}

RpMaterial* RpMatFXMaterialSetDualBlendModes(RpMaterial* material, RwBlendFunction srcBlendMode,
                                             RwBlendFunction dstBlendMode)
{
    MatFXDualData* dualData;

    dualData = &MatFXGetData(material, rpMATFXEFFECTDUAL)->dual;

    dualData->srcBlendMode = srcBlendMode;
    dualData->dstBlendMode = dstBlendMode;

    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATESRCBLEND);
    _rpMatFXSetupDualRenderState(dualData, rwRENDERSTATEDESTBLEND);

    return material;
}

RwTexture* RpMatFXMaterialGetDualTexture(const RpMaterial* material)
{
    const MatFXDualData* dualData;

    dualData = &MatFXGetConstData(material, rpMATFXEFFECTDUAL)->dual;

    return dualData->texture;
}

const RpMaterial* RpMatFXMaterialGetDualBlendModes(const RpMaterial* material,
                                                   RwBlendFunction* srcBlendMode,
                                                   RwBlendFunction* dstBlendMode)
{
    const MatFXDualData* dualData;

    dualData = &MatFXGetConstData(material, rpMATFXEFFECTDUAL)->dual;

    *srcBlendMode = dualData->srcBlendMode;
    *dstBlendMode = dualData->dstBlendMode;

    return material;
}

RpMaterial* RpMatFXMaterialSetUVTransformMatrices(RpMaterial* material, RwMatrix* baseTransform,
                                                  RwMatrix* dualTransform)
{
    MatFXUVAnimData* uvAnimData;

    uvAnimData = &MatFXGetData(material, rpMATFXEFFECTUVTRANSFORM)->uvAnim;

    uvAnimData->baseTransform = baseTransform;
    uvAnimData->dualTransform = dualTransform;

    return material;
}

const RpMaterial* RpMatFXMaterialGetUVTransformMatrices(const RpMaterial* material,
                                                        RwMatrix** baseTransform,
                                                        RwMatrix** dualTransform)
{
    const MatFXUVAnimData* uvAnimData;

    uvAnimData = &MatFXGetConstData(material, rpMATFXEFFECTUVTRANSFORM)->uvAnim;

    if (baseTransform != NULL)
    {
        *baseTransform = uvAnimData->baseTransform;
    }

    if (dualTransform != NULL)
    {
        *dualTransform = uvAnimData->dualTransform;
    }

    return material;
}
