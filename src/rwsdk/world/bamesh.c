#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define rwPLUGIN_ID 2

#define rpWORLD 7
#define rpGEOMETRY 8

#define rpMESHHEADERTRISTRIP 0x0001
#define rpMESHHEADERTRIFAN 0x0002
#define rpMESHHEADERLINELIST 0x0004
#define rpMESHHEADERPOLYLINE 0x0008
#define rpMESHHEADERPOINTLIST 0x0010

#define rpMESHHEADERPRIMTYPEOR                                                                     \
    (rpMESHHEADERTRISTRIP | rpMESHHEADERTRIFAN | rpMESHHEADERLINELIST | rpMESHHEADERPOLYLINE |     \
     rpMESHHEADERPOINTLIST)

#define rwPRIMTYPEOR                                                                               \
    (rwPRIMTYPELINELIST | rwPRIMTYPEPOLYLINE | rwPRIMTYPETRILIST | rwPRIMTYPETRISTRIP |            \
     rwPRIMTYPETRIFAN | rwPRIMTYPEPOINTLIST)

#define rpMESHNATIVEFLAG 0x01000000

/* The object a mesh belongs to is either a geometry or a world; either way its
 * flags word follows the RwObject header. */
#define rpMeshObjectIsNative(_object)                                                              \
    ((RwObjectGetType(_object) == rpGEOMETRY &&                                                    \
      (((const RpGeometry*)(_object))->flags & rpMESHNATIVEFLAG)) ||                               \
     (RwObjectGetType(_object) == rpWORLD &&                                                       \
      (((const RpWorld*)(_object))->flags & rpMESHNATIVEFLAG)))

#define rpMeshObjectIsNotNative(_object)                                                           \
    ((RwObjectGetType(_object) == rpGEOMETRY &&                                                    \
      !(((const RpGeometry*)(_object))->flags & rpMESHNATIVEFLAG)) ||                              \
     (RwObjectGetType(_object) == rpWORLD &&                                                       \
      !(((const RpWorld*)(_object))->flags & rpMESHNATIVEFLAG)))

#define rpMESHINDEXBUFFERSIZE 256

#define RwFreeListAlloc(_fl) ((RWSRCGLOBAL(memoryAlloc))((_fl)))
#define RwFreeListFree(_fl, _p) ((RWSRCGLOBAL(memoryFree))((_fl), (_p)))

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_NOMEM 0x80000013

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rpMeshGlobals rpMeshGlobals;
struct rpMeshGlobals
{
    RwInt16 nextSerialNum;
    RwFreeList* triStripListEntryFreeList;
    RwUInt8 meshFlagsToPrimType[rpMESHHEADERPRIMTYPEOR];
    RwUInt8 primTypeToMeshFlags[rwPRIMTYPEOR];
};

typedef struct RpMeshStatic RpMeshStatic;
struct RpMeshStatic
{
    RwFreeList* BuildMeshFreeList;
};

typedef struct binMeshHeader binMeshHeader;
struct binMeshHeader
{
    RwUInt32 flags;
    RwUInt32 numMeshes;
    RwUInt32 totalIndicesInMesh;
};

typedef struct binMesh binMesh;
struct binMesh
{
    RwUInt32 numIndices;
    RwInt32 matIndex;
};

#define RWMESHGLOBAL(var)                                                                          \
    (RWPLUGINOFFSET(rpMeshGlobals, RwEngineInstance, meshModule.globalsOffset)->var)

RwModuleInfo meshModule;

static RpMeshStatic MeshStatic = { (RwFreeList*)NULL };

static void MeshFreeListsDestroy(void)
{
    if (MeshStatic.BuildMeshFreeList)
    {
        RwFreeListDestroy(MeshStatic.BuildMeshFreeList);
        MeshStatic.BuildMeshFreeList = (RwFreeList*)NULL;
    }
}

static RwBool MeshFreeListsCreate(void)
{
    RwBool result;

    MeshStatic.BuildMeshFreeList = RwFreeListCreate(sizeof(RpBuildMesh), 50, 4);
    result = (MeshStatic.BuildMeshFreeList != NULL);

    return result;
}

RpMeshHeader* _rpMeshHeaderCreate(RwUInt32 size)
{
    RpMeshHeader* meshHeader;

    meshHeader = (RpMeshHeader*)RwMalloc(size);

    return meshHeader;
}

void* _rpMeshClose(void* instance, RwInt32 offset, RwInt32 size)
{
    if (--meshModule.numInstances == 0)
    {
        MeshFreeListsDestroy();
    }

    return instance;
}

void* _rpMeshOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    meshModule.globalsOffset = offset;

    if (meshModule.numInstances == 0)
    {
        if (!MeshFreeListsCreate())
        {
            MeshFreeListsDestroy();
            return NULL;
        }
    }

    RWMESHGLOBAL(nextSerialNum) = 1;

    meshModule.numInstances++;

    RWMESHGLOBAL(meshFlagsToPrimType)[0] = rwPRIMTYPETRILIST;
    RWMESHGLOBAL(meshFlagsToPrimType)[rpMESHHEADERTRISTRIP] = rwPRIMTYPETRISTRIP;
    RWMESHGLOBAL(meshFlagsToPrimType)[rpMESHHEADERTRIFAN] = rwPRIMTYPETRIFAN;
    RWMESHGLOBAL(meshFlagsToPrimType)[rpMESHHEADERLINELIST] = rwPRIMTYPELINELIST;
    RWMESHGLOBAL(meshFlagsToPrimType)[rpMESHHEADERPOLYLINE] = rwPRIMTYPEPOLYLINE;
    RWMESHGLOBAL(meshFlagsToPrimType)[rpMESHHEADERPOINTLIST] = rwPRIMTYPEPOINTLIST;

    RWMESHGLOBAL(primTypeToMeshFlags)[rwPRIMTYPELINELIST] = rpMESHHEADERLINELIST;
    RWMESHGLOBAL(primTypeToMeshFlags)[rwPRIMTYPEPOLYLINE] = rpMESHHEADERPOLYLINE;
    RWMESHGLOBAL(primTypeToMeshFlags)[rwPRIMTYPETRILIST] = 0;
    RWMESHGLOBAL(primTypeToMeshFlags)[rwPRIMTYPETRISTRIP] = rpMESHHEADERTRISTRIP;
    RWMESHGLOBAL(primTypeToMeshFlags)[rwPRIMTYPETRIFAN] = rpMESHHEADERTRIFAN;
    RWMESHGLOBAL(primTypeToMeshFlags)[rwPRIMTYPEPOINTLIST] = rpMESHHEADERPOINTLIST;

    return instance;
}

RpBuildMesh* _rpBuildMeshCreate(RwUInt32 bufferSize)
{
    RpBuildMesh* mesh;
    RwUInt32 size;

    mesh = (RpBuildMesh*)RwFreeListAlloc(MeshStatic.BuildMeshFreeList);
    if (mesh)
    {
        mesh->numTriangles = 0;

        if (bufferSize)
        {
            size = bufferSize * sizeof(RpBuildMeshTriangle);

            mesh->meshTriangles = (RpBuildMeshTriangle*)RwMalloc(size);
            if (!mesh->meshTriangles)
            {
                RwFreeListFree(MeshStatic.BuildMeshFreeList, mesh);
                RWERROR((E_RW_NOMEM, size));
                return (RpBuildMesh*)NULL;
            }

            mesh->triangleBufferSize = bufferSize;
        }
        else
        {
            mesh->meshTriangles = (RpBuildMeshTriangle*)NULL;
            mesh->triangleBufferSize = 0;
        }

        return mesh;
    }

    RWERROR((E_RW_NOMEM, sizeof(RpBuildMesh)));
    return (RpBuildMesh*)NULL;
}

RwBool _rpBuildMeshDestroy(RpBuildMesh* mesh)
{
    if (mesh->meshTriangles)
    {
        RwFree(mesh->meshTriangles);
        mesh->meshTriangles = (RpBuildMeshTriangle*)NULL;
    }

    RwFreeListFree(MeshStatic.BuildMeshFreeList, mesh);

    return TRUE;
}

RwBool _rpMeshDestroy(RpMeshHeader* mesh)
{
    if (mesh->flags || mesh->numMeshes || mesh->serialNum || mesh->totalIndicesInMesh ||
        mesh->firstMeshOffset)
    {
        RwFree(mesh);
    }

    return TRUE;
}

RpBuildMesh* _rpBuildMeshAddTriangle(RpBuildMesh* mesh, RpMaterial* material, RwInt32 vert1,
                                     RwInt32 vert2, RwInt32 vert3, RwUInt16 matIndex,
                                     RwUInt16 textureIndex, RwUInt16 rasterIndex,
                                     RwUInt16 pipelineIndex)
{
    if (mesh->numTriangles >= mesh->triangleBufferSize)
    {
        RpBuildMeshTriangle* newMeshTriangles;
        RwUInt32 size = (mesh->numTriangles + 1) * sizeof(RpBuildMeshTriangle);

        if (mesh->numTriangles)
        {
            newMeshTriangles = (RpBuildMeshTriangle*)RwRealloc(mesh->meshTriangles, size);
        }
        else
        {
            newMeshTriangles = (RpBuildMeshTriangle*)RwMalloc(size);
        }

        if (!newMeshTriangles)
        {
            RWERROR((E_RW_NOMEM, size));
            return (RpBuildMesh*)NULL;
        }

        mesh->meshTriangles = newMeshTriangles;
        mesh->triangleBufferSize = mesh->numTriangles + 1;
    }

    mesh->meshTriangles[mesh->numTriangles].material = material;
    mesh->meshTriangles[mesh->numTriangles].vertIndex[0] = (RwUInt16)vert1;
    mesh->meshTriangles[mesh->numTriangles].vertIndex[1] = (RwUInt16)vert2;
    mesh->meshTriangles[mesh->numTriangles].vertIndex[2] = (RwUInt16)vert3;
    mesh->meshTriangles[mesh->numTriangles].matIndex = matIndex;
    mesh->meshTriangles[mesh->numTriangles].textureIndex = textureIndex;
    mesh->meshTriangles[mesh->numTriangles].rasterIndex = rasterIndex;
    mesh->meshTriangles[mesh->numTriangles].pipelineIndex = pipelineIndex;

    mesh->numTriangles++;

    return mesh;
}

RpMeshHeader* _rpMeshHeaderForAllMeshes(RpMeshHeader* meshHeader, RpMeshCallBack fpCallBack,
                                        void* pData)
{
    RwInt32 numMeshes = meshHeader->numMeshes;
    RpMesh* mesh = (RpMesh*)((RwUInt8*)(meshHeader + 1) + meshHeader->firstMeshOffset);

    while (numMeshes--)
    {
        if (!fpCallBack(mesh, meshHeader, pData))
        {
            return meshHeader;
        }

        mesh++;
    }

    return meshHeader;
}

RwStream* _rpMeshWrite(const RpMeshHeader* meshHeader, const void* object, RwStream* stream,
                       const RpMaterialList* matList)
{
    binMeshHeader bmh;
    RwUInt32 numMeshes;
    const RpMesh* mesh;
    RwUInt32 objectType;

    bmh.flags = meshHeader->flags;
    bmh.numMeshes = meshHeader->numMeshes;
    bmh.totalIndicesInMesh = meshHeader->totalIndicesInMesh;

    if (!RwStreamWriteInt32(stream, (RwInt32*)&bmh, sizeof(bmh)))
    {
        return (RwStream*)NULL;
    }

    numMeshes = meshHeader->numMeshes;
    mesh = (const RpMesh*)(meshHeader + 1);
    objectType = RwObjectGetType(object);

    while (numMeshes--)
    {
        binMesh bm;

        bm.numIndices = mesh->numIndices;
        bm.matIndex = _rpMaterialListFindMaterialIndex(matList, mesh->material);
        if (bm.matIndex < 0)
        {
            bm.matIndex = 0;
        }

        if (!RwStreamWriteInt32(stream, (RwInt32*)&bm, sizeof(bm)))
        {
            return (RwStream*)NULL;
        }

        if ((objectType == rpGEOMETRY &&
             !(((const RpGeometry*)object)->flags & rpMESHNATIVEFLAG)) ||
            (objectType == rpWORLD && !(((const RpWorld*)object)->flags & rpMESHNATIVEFLAG)))
        {
            RwUInt32 numIndices = mesh->numIndices;
            RxVertexIndex* meshIndices = mesh->indices;
            RwUInt32 IndexBuffer[rpMESHINDEXBUFFERSIZE];

            while (numIndices)
            {
                RwUInt32 writeIndices;
                RwUInt32 i;

                writeIndices = rpMESHINDEXBUFFERSIZE;
                if (numIndices < rpMESHINDEXBUFFERSIZE)
                {
                    writeIndices = numIndices;
                }

                for (i = 0; i < writeIndices; i++)
                {
                    IndexBuffer[i] = *meshIndices++;
                }

                if (!RwStreamWriteInt32(stream, (RwInt32*)IndexBuffer,
                                        writeIndices * sizeof(RwUInt32)))
                {
                    return (RwStream*)NULL;
                }

                numIndices -= writeIndices;
            }
        }

        mesh++;
    }

    return stream;
}

RpMeshHeader* _rpMeshRead(RwStream* stream, const void* object, const RpMaterialList* matList)
{
    binMeshHeader bmh;
    RpMeshHeader* meshHeader;
    RwUInt32 size;

    if (!RwStreamReadInt32(stream, (RwInt32*)&bmh, sizeof(bmh)))
    {
        return (RpMeshHeader*)NULL;
    }

    size = sizeof(RpMeshHeader) + bmh.numMeshes * (sizeof(RpMesh) + sizeof(RwUInt32));
    if (rpMeshObjectIsNotNative(object))
    {
        size += bmh.totalIndicesInMesh * sizeof(RxVertexIndex);
    }

    meshHeader = (RpMeshHeader*)RwMalloc(size);
    if (meshHeader)
    {
        RpMesh* mesh = (RpMesh*)(meshHeader + 1);
        RxVertexIndex* meshIndices = (RxVertexIndex*)(mesh + bmh.numMeshes);
        RwUInt32 objectType = RwObjectGetType(object);
        RwUInt32 numMeshes;

        meshHeader->flags = bmh.flags;
        meshHeader->numMeshes = (RwUInt16)bmh.numMeshes;
        meshHeader->serialNum = RWMESHGLOBAL(nextSerialNum);
        meshHeader->totalIndicesInMesh = bmh.totalIndicesInMesh;
        meshHeader->firstMeshOffset = 0;

        RWMESHGLOBAL(nextSerialNum)++;

        numMeshes = meshHeader->numMeshes;
        while (numMeshes--)
        {
            binMesh bm;

            if (!RwStreamReadInt32(stream, (RwInt32*)&bm, sizeof(bm)))
            {
                return (RpMeshHeader*)NULL;
            }

            mesh->numIndices = bm.numIndices;
            mesh->material = _rpMaterialListGetMaterial(matList, bm.matIndex);
            mesh->indices = meshIndices;

            if ((objectType == rpGEOMETRY &&
                 !(((const RpGeometry*)object)->flags & rpMESHNATIVEFLAG)) ||
                (objectType == rpWORLD && !(((const RpWorld*)object)->flags & rpMESHNATIVEFLAG)))
            {
                RwUInt32 remainingIndices = mesh->numIndices;
                RwUInt32 IndexBuffer[rpMESHINDEXBUFFERSIZE];

                while (remainingIndices)
                {
                    RwUInt32 readIndices;
                    RwUInt32* source;

                    source = IndexBuffer;

                    readIndices = rpMESHINDEXBUFFERSIZE;
                    if (remainingIndices < rpMESHINDEXBUFFERSIZE)
                    {
                        readIndices = remainingIndices;
                    }

                    if (!RwStreamReadInt32(stream, (RwInt32*)source,
                                           readIndices * sizeof(RwUInt32)))
                    {
                        return (RpMeshHeader*)NULL;
                    }

                    remainingIndices -= readIndices;

                    while (readIndices--)
                    {
                        *meshIndices++ = (RxVertexIndex)*source++;
                    }
                }
            }

            mesh++;
        }
    }

    return meshHeader;
}

RwInt32 _rpMeshSize(const RpMeshHeader* meshHeader, const void* object)
{
    RwUInt32 size;

    if (rpMeshObjectIsNative(object))
    {
        size = sizeof(binMeshHeader) + meshHeader->numMeshes * sizeof(binMesh);
    }
    else
    {
        size = sizeof(binMeshHeader) + meshHeader->numMeshes * sizeof(binMesh) +
               meshHeader->totalIndicesInMesh * sizeof(RwUInt32);
    }

    return size;
}

RwInt16 _rpMeshGetNextSerialNumber(void)
{
    return RWMESHGLOBAL(nextSerialNum)++;
}
