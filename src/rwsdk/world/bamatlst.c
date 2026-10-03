#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwPLUGIN_ID 2

#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define rpMATERIALLISTGRANULARITY 20

#define RwStreamWriteChunkHeader(stream, type, size)                                               \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, 0xFFFF)

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

RpMaterialList* _rpMaterialListDeinitialize(RpMaterialList* matList)
{
    RpMaterial** materialArray = matList->materials;

    if (materialArray)
    {
        RwInt32 materialCount = matList->numMaterials;
        RwInt32 nI;

        for (nI = 0; nI < materialCount; nI++)
        {
            RpMaterialDestroy(materialArray[nI]);
            materialArray[nI] = (RpMaterial*)NULL;
        }

        RwFree(materialArray);
        matList->materials = (RpMaterial**)NULL;
    }

    matList->numMaterials = 0;
    matList->space = 0;

    return matList;
}

RpMaterialList* _rpMaterialListInitialize(RpMaterialList* matList)
{
    matList->space = 0;
    matList->materials = (RpMaterial**)NULL;
    matList->numMaterials = 0;

    return matList;
}

RpMaterial* _rpMaterialListGetMaterial(const RpMaterialList* matList, RwInt32 matIndex)
{
    return matList->materials[matIndex];
}

static RpMaterialList* _rpMaterialListSetSize(RpMaterialList* matList, RwInt32 size)
{
    if (matList->space < size)
    {
        RpMaterial** materials;
        RwUInt32 memSize = size * sizeof(RpMaterial*);

        if (matList->materials)
        {
            materials = (RpMaterial**)RwRealloc(matList->materials, size * sizeof(RpMaterial*));
        }
        else
        {
            materials = (RpMaterial**)RwMalloc(size * sizeof(RpMaterial*));
        }

        if (!materials)
        {
            RWERROR((E_RW_NOMEM, memSize));
            return (RpMaterialList*)NULL;
        }

        matList->materials = materials;
        matList->space = size;
    }

    return matList;
}

RwInt32 _rpMaterialListAppendMaterial(RpMaterialList* matList, RpMaterial* material)
{
    RpMaterial** materials;
    RwUInt32 count;
    RwUInt32 memSize;

    if (matList->space > matList->numMaterials)
    {
        matList->materials[matList->numMaterials] = material;
        RpMaterialAddRef(material);
        matList->numMaterials++;

        return matList->numMaterials - 1;
    }

    count = matList->space + rpMATERIALLISTGRANULARITY;
    memSize = count * sizeof(RpMaterial*);

    if (matList->materials)
    {
        materials = (RpMaterial**)RwRealloc(matList->materials, count * sizeof(RpMaterial*));
    }
    else
    {
        materials = (RpMaterial**)RwMalloc(count * sizeof(RpMaterial*));
    }

    if (!materials)
    {
        RWERROR((E_RW_NOMEM, memSize));
        return -1;
    }

    matList->materials = materials;
    matList->space += rpMATERIALLISTGRANULARITY;

    materials[matList->numMaterials] = material;
    RpMaterialAddRef(material);
    matList->numMaterials++;

    return matList->numMaterials - 1;
}

RwInt32 _rpMaterialListFindMaterialIndex(const RpMaterialList* matList, const RpMaterial* material)
{
    RwInt32 numMats = matList->numMaterials;

    while (numMats-- > 0)
    {
        if (matList->materials[numMats] == material)
        {
            break;
        }
    }

    return numMats;
}

static RwUInt32 MaterialListStreamGetSizeActual(const RpMaterialList* matList)
{
    RwUInt32 size;

    size = sizeof(RwInt32) + matList->numMaterials * sizeof(RwInt32);

    return size;
}

RwUInt32 _rpMaterialListStreamGetSize(const RpMaterialList* matList)
{
    RwUInt32 size;
    RwInt32 i;
    RwInt32 j;

    size = MaterialListStreamGetSizeActual(matList) + rwCHUNKHEADERSIZE;

    for (i = 0; i < matList->numMaterials; i++)
    {
        j = i;
        while (j--)
        {
            if (matList->materials[j] == matList->materials[i])
            {
                break;
            }
        }

        if (j < 0)
        {
            size += RpMaterialStreamGetSize(matList->materials[i]) + rwCHUNKHEADERSIZE;
        }
    }

    return size;
}

const RpMaterialList* _rpMaterialListStreamWrite(const RpMaterialList* matList, RwStream* stream)
{
    RwInt32 i;
    RwInt32 j;

    if (!RwStreamWriteChunkHeader(stream, rwID_MATLIST, _rpMaterialListStreamGetSize(matList)))
    {
        return (const RpMaterialList*)NULL;
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, MaterialListStreamGetSizeActual(matList)))
    {
        return (const RpMaterialList*)NULL;
    }

    if (!RwStreamWriteInt32(stream, &matList->numMaterials, sizeof(RwInt32)))
    {
        return (const RpMaterialList*)NULL;
    }

    for (i = 0; i < matList->numMaterials; i++)
    {
        j = i;
        while (j--)
        {
            if (matList->materials[j] == matList->materials[i])
            {
                break;
            }
        }

        if (!RwStreamWriteInt32(stream, &j, sizeof(RwInt32)))
        {
            return (const RpMaterialList*)NULL;
        }
    }

    for (i = 0; i < matList->numMaterials; i++)
    {
        j = i;
        while (j--)
        {
            if (matList->materials[j] == matList->materials[i])
            {
                break;
            }
        }

        if (j < 0)
        {
            if (!RpMaterialStreamWrite(matList->materials[i], stream))
            {
                return (const RpMaterialList*)NULL;
            }
        }
    }

    return matList;
}

RpMaterialList* _rpMaterialListStreamRead(RwStream* stream, RpMaterialList* matList)
{
    RwInt32 i;
    RwInt32 len;
    RwInt32* matindex;
    RwUInt32 size;
    RwUInt32 version;
    RwBool status;
    RpMaterial* material;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return (RpMaterialList*)NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        status = (NULL != RwStreamReadInt32(stream, &len, sizeof(len)));
        if (!status)
        {
            return (RpMaterialList*)NULL;
        }

        _rpMaterialListInitialize(matList);
        {
            const RwInt32 materialCount = len;

            if (materialCount == 0)
            {
                return matList;
            }

            if (!_rpMaterialListSetSize(matList, materialCount))
            {
                _rpMaterialListDeinitialize(matList);
                return (RpMaterialList*)NULL;
            }

        }
        matindex = (RwInt32*)RwMalloc(sizeof(RwInt32) * len);

        status = (NULL != RwStreamReadInt32(stream, matindex, sizeof(RwInt32) * len));
        if (!status)
        {
            RwFree(matindex);
            _rpMaterialListDeinitialize(matList);
            return (RpMaterialList*)NULL;
        }

        for (i = 0; i < len; i++)
        {
            if (matindex[i] < 0)
            {
                if (!RwStreamFindChunk(stream, rwID_MATERIAL, (RwUInt32*)NULL, &version))
                {
                    RwFree(matindex);
                    _rpMaterialListDeinitialize(matList);
                    return (RpMaterialList*)NULL;
                }

                if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
                {
                    material = RpMaterialStreamRead(stream);
                    if (!material)
                    {
                        RwFree(matindex);
                        _rpMaterialListDeinitialize(matList);
                        return (RpMaterialList*)NULL;
                    }
                }
                else
                {
                    RWERROR((E_RW_BADVERSION));
                    RwFree(matindex);
                    _rpMaterialListDeinitialize(matList);
                    return (RpMaterialList*)NULL;
                }
            }
            else
            {
                material = _rpMaterialListGetMaterial(matList, matindex[i]);
                RpMaterialAddRef(material);
            }

            _rpMaterialListAppendMaterial(matList, material);
            RpMaterialDestroy(material);
        }

        RwFree(matindex);

        return matList;
    }

    RWERROR((E_RW_BADVERSION));
    return (RpMaterialList*)NULL;
}
