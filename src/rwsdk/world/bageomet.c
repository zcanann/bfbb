#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/world/bageomet.h>

#define rpGEOMETRYLOCKPOLYGONS 0x01
#define rpGEOMETRYLOCKALL 0xfff

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

static RwModuleInfo geometryModule;

static RwPluginRegistry geometryTKList = { sizeof(RpGeometry),      sizeof(RpGeometry),     0, 0,
                                           (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

const RpGeometry* RpGeometryTriangleSetVertexIndices(const RpGeometry* geometry,
                                                     RpTriangle* triangle, RwUInt16 vert1,
                                                     RwUInt16 vert2, RwUInt16 vert3)
{
    triangle->vertIndex[0] = vert1;
    triangle->vertIndex[1] = vert2;
    triangle->vertIndex[2] = vert3;

    return geometry;
}

RpGeometry* _rpGeometryAddRef(RpGeometry* geometry)
{
    geometry->refCount++;

    return geometry;
}

void* _rpGeometryClose(void* instance, RwInt32 offset, RwInt32 size)
{
    geometryModule.numInstances--;

    return instance;
}

void* _rpGeometryOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    geometryModule.globalsOffset = offset;
    geometryModule.numInstances++;

    return instance;
}

RpMaterial* RpGeometryTriangleGetMaterial(const RpGeometry* geometry, const RpTriangle* triangle)
{
    if (triangle->matIndex == -1)
    {
        return (RpMaterial*)NULL;
    }

    return geometry->matList.materials[triangle->matIndex];
}

const RpGeometry* RpGeometryTriangleGetVertexIndices(const RpGeometry* geometry,
                                                     const RpTriangle* triangle, RwUInt16* vert1,
                                                     RwUInt16* vert2, RwUInt16* vert3)
{
    if (vert1)
    {
        *vert1 = triangle->vertIndex[0];
    }

    if (vert2)
    {
        *vert2 = triangle->vertIndex[1];
    }

    if (vert3)
    {
        *vert3 = triangle->vertIndex[2];
    }

    return geometry;
}

RwInt32 RpGeometryRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                                 RwPluginObjectConstructor constructCB,
                                 RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    return _rwPluginRegistryAddPlugin(&geometryTKList, size, pluginID, constructCB, destructCB,
                                      copyCB);
}

RwInt32 RpGeometryRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                       RwPluginDataChunkWriteCallBack writeCB,
                                       RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    return _rwPluginRegistryAddPluginStream(&geometryTKList, pluginID, readCB, writeCB, getSizeCB);
}

RpGeometry* RpGeometryLock(RpGeometry* geometry, RwInt32 lockMode)
{
    geometry->lockedSinceLastInst |= lockMode;

    if ((lockMode & rpGEOMETRYLOCKPOLYGONS) && geometry->mesh)
    {
        _rpMeshDestroy(geometry->mesh);
        geometry->mesh = (RpMeshHeader*)NULL;
    }

    return geometry;
}

RpGeometry* RpGeometryForAllMaterials(RpGeometry* geometry, RpMaterialCallBack fpCallBack,
                                      void* pData)
{
    RwInt32 numMaterials = geometry->matList.numMaterials;
    RwInt32 i;

    for (i = 0; i < numMaterials; i++)
    {
        if (!fpCallBack(geometry->matList.materials[i], pData))
        {
            return geometry;
        }
    }

    return geometry;
}

RpGeometry* RpGeometryTriangleSetMaterial(RpGeometry* geometry, RpTriangle* triangle,
                                          RpMaterial* material)
{
    if (material)
    {
        RwInt32 matIndex = _rpMaterialListFindMaterialIndex(&geometry->matList, material);

        if (matIndex < 0)
        {
            matIndex = _rpMaterialListAppendMaterial(&geometry->matList, material);

            if (matIndex < 0)
            {
                return (RpGeometry*)NULL;
            }
        }

        triangle->matIndex = (RwInt16)matIndex;
    }
    else
    {
        triangle->matIndex = -1;
    }

    return geometry;
}

RpGeometry* RpGeometryCreateSpace(RwReal radius)
{
    RpGeometry* geometry = RpGeometryCreate(0, 0, 0);

    if (geometry)
    {
        RpMorphTarget* morphTarget = geometry->morphTarget;

        morphTarget->boundingSphere.center.x = (RwReal)0.0;
        morphTarget->boundingSphere.center.y = (RwReal)0.0;
        morphTarget->boundingSphere.center.z = (RwReal)0.0;
        morphTarget->boundingSphere.radius = radius;
    }

    if (!RpGeometryUnlock(geometry))
    {
        RpGeometryDestroy(geometry);

        return (RpGeometry*)NULL;
    }

    return geometry;
}

RwBool RpGeometryDestroy(RpGeometry* geometry)
{
    RwBool result = TRUE;

    if ((geometry->refCount - 1) <= 0)
    {
        if (geometry->repEntry)
        {
            RwResourcesFreeResEntry(geometry->repEntry);
        }

        geometry->refCount--;
        _rpGeometryAddRef(geometry);

        RpGeometryLock(geometry, rpGEOMETRYLOCKALL);

        _rwPluginRegistryDeInitObject(&geometryTKList, geometry);

        if (geometry->morphTarget)
        {
            RwFree(geometry->morphTarget);
            geometry->morphTarget = (RpMorphTarget*)NULL;
        }

        _rpMaterialListDeinitialize(&geometry->matList);

        geometry->refCount--;

        RwFree(geometry);
    }
    else
    {
        geometry->refCount--;
    }

    return result;
}
