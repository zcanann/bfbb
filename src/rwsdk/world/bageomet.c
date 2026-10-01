#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/world/bageomet.h>

#include <string.h>

#define rpGEOMETRYLOCKPOLYGONS 0x01
#define rpGEOMETRYLOCKALL 0xfff

#define rpMESHHEADERTRISTRIP 0x0001

#define rpGEOMETRY 8

/* Geometry chunks older than this carry their own surface properties */
#define rpGEOMETRYSURFACEPROPSVERSION 0x34001

#define rpGEOMETRYMAXVERTICES 65536

#define rpGEOMETRYTEXCOORDSETS(_n) ((_n & 0xff) << 16)

#define rpGEOMETRYNUMTEXCOORDSETS(_fmt)                                                            \
    (((_fmt) & 0xff0000) ?                                                                         \
         (((_fmt) & 0xff0000) >> 16) :                                                             \
         (((_fmt) & rpGEOMETRYTEXTURED2) ? 2 : (((_fmt) & rpGEOMETRYTEXTURED) ? 1 : 0)))

#define rwCHUNKHEADERSIZE (sizeof(RwUInt32) * 3)

#define RwStreamWriteChunkHeader(stream, type, size)                                               \
    _rwStreamWriteVersionedChunkHeader(stream, type, size, rwLIBRARYCURRENTVERSION, 0xFFFF)

#define rwPLUGIN_ID 2

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
#define E_RP_WORLD_TOOMANYVERTICES 0x00000006

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct RpGeometryChunkInfo RpGeometryChunkInfo;
struct RpGeometryChunkInfo
{
    RwInt32 format;
    RwInt32 numTriangles;
    RwInt32 numVertices;
    RwInt32 numMorphTargets;
};

typedef struct _rpTriangle _rpTriangle;
struct _rpTriangle
{
    RwUInt32 vertex01;
    RwUInt32 vertex2Mat;
};

typedef struct _rpMorphTarget _rpMorphTarget;
struct _rpMorphTarget
{
    RwSphere boundingSphere;
    RwBool pointsPresent;
    RwBool normalsPresent;
};

extern void _rpMaterialSetDefaultSurfaceProperties(const RwSurfaceProperties* surfaceProps);

static RwModuleInfo geometryModule;

static RwPluginRegistry geometryTKList = { sizeof(RpGeometry),      sizeof(RpGeometry),     0, 0,
                                           (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwBool GeometryAnnihilate(RpGeometry* geometry)
{
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

    return TRUE;
}

void* _rpGeometryOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    geometryModule.globalsOffset = offset;
    geometryModule.numInstances++;

    return instance;
}

void* _rpGeometryClose(void* instance, RwInt32 offset, RwInt32 size)
{
    geometryModule.numInstances--;

    return instance;
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

const RpMorphTarget* RpMorphTargetCalcBoundingSphere(const RpMorphTarget* morphTarget,
                                                     RwSphere* boundingSphere)
{
    RpGeometry* geometry = morphTarget->parentGeom;
    RwV3d* vert;
    RwInt32 numVerts = geometry->numVertices;
    RwSphere sphere;
    RwReal sphere_radius = (RwReal)0.0;
    RwBBox boundBox;

    RwBBoxCalculate(&boundBox, morphTarget->verts, numVerts);

    RwV3dAddMacro(&sphere.center, &boundBox.inf, &boundBox.sup);
    RwV3dScaleMacro(&sphere.center, &sphere.center, (RwReal)0.5);

    vert = morphTarget->verts;
    while (numVerts--)
    {
        RwReal nDist;
        RwV3d vTmp;

        RwV3dSubMacro(&vTmp, vert, &sphere.center);
        nDist = RwV3dDotProductMacro(&vTmp, &vTmp);

        if (nDist > sphere_radius)
        {
            sphere_radius = nDist;
        }

        vert++;
    }

    if (sphere_radius > (RwReal)0.0)
    {
        sphere_radius = _rwSqrt(sphere_radius);
    }

    sphere.radius = sphere_radius * (RwReal)1.001;

    *boundingSphere = sphere;

    return morphTarget;
}

RwInt32 RpGeometryAddMorphTargets(RpGeometry* geometry, RwInt32 mtcount)
{
    RwInt32 i;
    RwUInt32 mtsize;
    RwUInt32 bytes;
    RpMorphTarget* morphTarget;
    RwV3d* vertexData;

    if (geometry->flags & rpGEOMETRYNATIVE)
    {
        mtsize = sizeof(RpMorphTarget);
    }
    else
    {
        mtsize = sizeof(RpMorphTarget) + geometry->numVertices * sizeof(RwV3d);

        if (geometry->flags & rpGEOMETRYNORMALS)
        {
            mtsize += geometry->numVertices * sizeof(RwV3d);
        }
    }

    bytes = mtsize * (geometry->numMorphTargets + mtcount);

    if (geometry->morphTarget)
    {
        RwUInt8* src;
        RwUInt8* dst;
        RwInt32 len;

        morphTarget = (RpMorphTarget*)RwRealloc(geometry->morphTarget, bytes);
        if (!morphTarget)
        {
            RWERROR((E_RW_NOMEM, bytes));
            return -1;
        }

        /* Shuffle the existing vertex data up to make room for the new headers */
        src = (RwUInt8*)morphTarget + mtsize * geometry->numMorphTargets - 1;
        dst = src + mtcount * sizeof(RpMorphTarget);
        len =
            mtsize * geometry->numMorphTargets - geometry->numMorphTargets * sizeof(RpMorphTarget);

        while (len--)
        {
            *dst-- = *src--;
        }
    }
    else
    {
        morphTarget = (RpMorphTarget*)RwMalloc(bytes);
        if (!morphTarget)
        {
            RWERROR((E_RW_NOMEM, bytes));
            return -1;
        }
    }

    geometry->numMorphTargets += mtcount;
    geometry->morphTarget = morphTarget;

    vertexData = (RwV3d*)(morphTarget + geometry->numMorphTargets);

    for (i = 0; i < geometry->numMorphTargets; i++)
    {
        RpMorphTarget* aMorph = &geometry->morphTarget[i];

        aMorph->verts = (RwV3d*)NULL;
        aMorph->normals = (RwV3d*)NULL;

        if (!(geometry->flags & rpGEOMETRYNATIVE) && geometry->numVertices)
        {
            aMorph->verts = vertexData;
            vertexData += geometry->numVertices;

            if (geometry->flags & rpGEOMETRYNORMALS)
            {
                aMorph->normals = vertexData;
                vertexData += geometry->numVertices;
            }
        }
    }

    for (i = geometry->numMorphTargets - mtcount; i < geometry->numMorphTargets; i++)
    {
        RpMorphTarget* aMorph = &geometry->morphTarget[i];

        aMorph->boundingSphere.center.x = (RwReal)0.0;
        aMorph->boundingSphere.center.y = (RwReal)0.0;
        aMorph->boundingSphere.center.z = (RwReal)0.0;
        aMorph->boundingSphere.radius = (RwReal)0.0;
        aMorph->parentGeom = geometry;
    }

    return geometry->numMorphTargets - mtcount;
}

const RpGeometry* RpGeometryTriangleSetVertexIndices(const RpGeometry* geometry,
                                                     RpTriangle* triangle, RwUInt16 vert1,
                                                     RwUInt16 vert2, RwUInt16 vert3)
{
    triangle->vertIndex[0] = vert1;
    triangle->vertIndex[1] = vert2;
    triangle->vertIndex[2] = vert3;

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

RpMaterial* RpGeometryTriangleGetMaterial(const RpGeometry* geometry, const RpTriangle* triangle)
{
    if (triangle->matIndex == -1)
    {
        return (RpMaterial*)NULL;
    }

    return geometry->matList.materials[triangle->matIndex];
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

RpGeometry* RpGeometryUnlock(RpGeometry* geometry)
{
    RwTexture** textureArray;
    RwRaster** rasterArray;
    RxPipeline** pipelineArray;
    RwUInt16 numTex = 0;
    RwUInt16 numRas = 0;
    RwUInt16 numPip = 0;

    if (!geometry->mesh)
    {
        RpBuildMesh* buildMesh;
        RpMeshHeader* newMesh;
        RwInt32 i;
        RwInt32 numMaterials;

        buildMesh = _rpBuildMeshCreate(geometry->numTriangles);
        if (buildMesh)
        {
            numMaterials = geometry->matList.numMaterials;

            textureArray = (RwTexture**)RwMalloc(numMaterials * sizeof(RwTexture*));
            rasterArray = (RwRaster**)RwMalloc(numMaterials * sizeof(RwRaster*));
            pipelineArray = (RxPipeline**)RwMalloc(numMaterials * sizeof(RxPipeline*));

            for (i = 0; i < geometry->numTriangles; i++)
            {
                RpTriangle* triangle = &geometry->triangles[i];
                RpMaterial* material;
                RwUInt16 texIndex;
                RwUInt16 rasIndex;
                RwUInt16 pipIndex;
                RxPipeline* pipeline;
                RwTexture* texture;
                RwRaster* raster = (RwRaster*)NULL;

                material = _rpMaterialListGetMaterial(&geometry->matList, triangle->matIndex);

                texture = material->texture;
                for (texIndex = 0; texIndex < numTex; texIndex++)
                {
                    if (textureArray[texIndex] == texture)
                    {
                        break;
                    }
                }

                if (texIndex == numTex)
                {
                    textureArray[texIndex] = texture;
                    numTex++;
                }

                if (texture)
                {
                    raster = texture->raster;
                }

                for (rasIndex = 0; rasIndex < numRas; rasIndex++)
                {
                    if (rasterArray[rasIndex] == raster)
                    {
                        break;
                    }
                }

                if (rasIndex == numRas)
                {
                    rasterArray[rasIndex] = raster;
                    numRas++;
                }

                pipeline = material->pipeline;
                for (pipIndex = 0; pipIndex < numPip; pipIndex++)
                {
                    if (pipelineArray[pipIndex] == pipeline)
                    {
                        break;
                    }
                }

                if (pipIndex == numPip)
                {
                    pipelineArray[pipIndex] = pipeline;
                    numPip++;
                }

                _rpBuildMeshAddTriangle(buildMesh, material, triangle->vertIndex[0],
                                        triangle->vertIndex[1], triangle->vertIndex[2],
                                        triangle->matIndex, texIndex, rasIndex, pipIndex);
            }

            RwFree(textureArray);
            RwFree(rasterArray);
            RwFree(pipelineArray);

            if (geometry->flags & rpGEOMETRYTRISTRIP)
            {
                newMesh = _rpMeshOptimise(buildMesh, rpMESHHEADERTRISTRIP);
            }
            else
            {
                newMesh = _rpMeshOptimise(buildMesh, 0);
            }

            if (newMesh)
            {
                geometry->mesh = newMesh;
                return geometry;
            }

            _rpBuildMeshDestroy(buildMesh);
            return (RpGeometry*)NULL;
        }

        return (RpGeometry*)NULL;
    }

    return geometry;
}

RpGeometry* RpGeometryCreate(RwInt32 numVerts, RwInt32 numTriangles, RwUInt32 format)
{
    RwUInt32 numTexCoordSets;
    RwUInt32 native;
    RwInt32 flags;
    RpGeometry* geometry;
    RwUInt8* goffset;
    RwUInt32 gsize;

    if ((numVerts < 0) || (numVerts >= rpGEOMETRYMAXVERTICES) || (numTriangles < 0))
    {
        if ((numVerts >= 0) && (numVerts >= rpGEOMETRYMAXVERTICES))
        {
            RWERROR((E_RP_WORLD_TOOMANYVERTICES));
        }

        return (RpGeometry*)NULL;
    }

    gsize = geometryTKList.sizeOfStruct;

    flags = format & rpGEOMETRYFLAGSMASK;

    numTexCoordSets = rpGEOMETRYNUMTEXCOORDSETS(format);

    flags = (flags & ~(rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2)) |
            ((numTexCoordSets == 1) ? rpGEOMETRYTEXTURED :
                                      ((numTexCoordSets > 1) ? rpGEOMETRYTEXTURED2 : 0));

    native = format & rpGEOMETRYNATIVE;
    if (!native)
    {
        if (flags & rpGEOMETRYPRELIT)
        {
            gsize += numVerts * sizeof(RwRGBA);
        }

        if (numTexCoordSets)
        {
            gsize += numTexCoordSets * (numVerts * sizeof(RwTexCoords));
        }

        gsize += numTriangles * sizeof(RpTriangle);
    }

    geometry = (RpGeometry*)RwMalloc(gsize);
    if (!geometry)
    {
        return (RpGeometry*)NULL;
    }

    if (!_rpMaterialListInitialize(&geometry->matList))
    {
        return (RpGeometry*)NULL;
    }

    geometry->morphTarget = (RpMorphTarget*)NULL;
    geometry->numMorphTargets = 0;

    rwObjectInitialize(geometry, rpGEOMETRY, 0);

    geometry->repEntry = (RwResEntry*)NULL;
    geometry->lockedSinceLastInst = 0;
    geometry->refCount = 1;
    geometry->mesh = (RpMeshHeader*)NULL;

    geometry->numTexCoordSets = numTexCoordSets;
    memset(geometry->texCoords, 0, sizeof(geometry->texCoords));

    geometry->preLitLum = (RwRGBA*)NULL;
    geometry->triangles = (RpTriangle*)NULL;

    geometry->numTriangles = numTriangles;
    geometry->flags = flags | (format & rpGEOMETRYNATIVEFLAGSMASK);
    geometry->numVertices = numVerts;

    if (!native)
    {
        goffset = (RwUInt8*)geometry + geometryTKList.sizeOfStruct;

        if ((flags & rpGEOMETRYPRELIT) && numVerts)
        {
            geometry->preLitLum = (RwRGBA*)goffset;
            goffset += numVerts * sizeof(RwRGBA);
        }

        if (numTexCoordSets && numVerts)
        {
            RwUInt32 i;

            for (i = 0; i < numTexCoordSets; i++)
            {
                geometry->texCoords[i] = (RwTexCoords*)goffset;
                goffset += numVerts * sizeof(RwTexCoords);
            }
        }

        if (numTriangles)
        {
            RwInt32 i;

            geometry->triangles = (RpTriangle*)goffset;

            for (i = 0; i < numTriangles; i++)
            {
                geometry->triangles[i].matIndex = -1;
            }
        }
    }

    if (RpGeometryAddMorphTargets(geometry, 1) < 0)
    {
        _rpMaterialListDeinitialize(&geometry->matList);
        RwFree(geometry);
        return (RpGeometry*)NULL;
    }

    _rwPluginRegistryInitObject(&geometryTKList, geometry);

    return geometry;
}

RpGeometry* _rpGeometryAddRef(RpGeometry* geometry)
{
    geometry->refCount++;

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

        result = GeometryAnnihilate(geometry);
    }
    else
    {
        geometry->refCount--;
    }

    return result;
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

static RwInt32 GeometryStreamGetSizeActual(const RpGeometry* geometry)
{
    RwInt32 size;
    RwInt32 i;

    size = sizeof(RpGeometryChunkInfo);

    if (!(geometry->flags & rpGEOMETRYNATIVE))
    {
        if (geometry->numVertices)
        {
            if (geometry->flags & rpGEOMETRYPRELIT)
            {
                size += geometry->numVertices * sizeof(RwRGBA);
            }

            size += geometry->numTexCoordSets * (geometry->numVertices * sizeof(RwTexCoords));
            size += geometry->numTriangles * sizeof(_rpTriangle);
        }

        for (i = 0; i < geometry->numMorphTargets; i++)
        {
            size += sizeof(_rpMorphTarget);

            if (geometry->morphTarget[i].verts)
            {
                size += geometry->numVertices * sizeof(RwV3d);
            }

            if (geometry->morphTarget[i].normals)
            {
                size += geometry->numVertices * sizeof(RwV3d);
            }
        }
    }
    else
    {
        size += sizeof(_rpMorphTarget);
    }

    return size;
}

RwUInt32 RpGeometryStreamGetSize(const RpGeometry* geometry)
{
    RwUInt32 size;

    size = GeometryStreamGetSizeActual(geometry) + rwCHUNKHEADERSIZE;
    size += _rpMaterialListStreamGetSize(&geometry->matList) + rwCHUNKHEADERSIZE;
    size += _rwPluginRegistryGetSize(&geometryTKList, geometry) + rwCHUNKHEADERSIZE;

    return size;
}

const RpGeometry* RpGeometryStreamWrite(const RpGeometry* geometry, RwStream* stream)
{
    RpGeometryChunkInfo geom;
    RwInt32 i;
    RwUInt32 flags;

    if (!RwStreamWriteChunkHeader(stream, rwID_GEOMETRY, RpGeometryStreamGetSize(geometry)))
    {
        return (const RpGeometry*)NULL;
    }

    if (!RwStreamWriteChunkHeader(stream, rwID_STRUCT, GeometryStreamGetSizeActual(geometry)))
    {
        return (const RpGeometry*)NULL;
    }

    geom.format = geometry->flags | rpGEOMETRYTEXCOORDSETS(geometry->numTexCoordSets);
    geom.numTriangles = geometry->numTriangles;
    geom.numVertices = geometry->numVertices;
    geom.numMorphTargets = geometry->numMorphTargets;

    RwMemLittleEndian32(&geom, sizeof(geom));

    if (!RwStreamWrite(stream, &geom, sizeof(geom)))
    {
        return (const RpGeometry*)NULL;
    }

    if (!(geometry->flags & rpGEOMETRYNATIVE) && geometry->numVertices)
    {
        RwUInt32 sizeTC;

        if (geometry->flags & rpGEOMETRYPRELIT)
        {
            if (!RwStreamWrite(stream, geometry->preLitLum, geometry->numVertices * sizeof(RwRGBA)))
            {
                return (const RpGeometry*)NULL;
            }
        }

        if (geometry->numTexCoordSets > 0)
        {
            sizeTC = geometry->numVertices * sizeof(RwTexCoords);

            for (i = 0; i < geometry->numTexCoordSets; i++)
            {
                if (!RwStreamWriteReal(stream, (const RwReal*)geometry->texCoords[i], sizeTC))
                {
                    return (const RpGeometry*)NULL;
                }
            }
        }

        if (geometry->numTriangles)
        {
            RwInt32 numTris = geometry->numTriangles;
            const RpTriangle* srceTri = geometry->triangles;

            while (numTris--)
            {
                _rpTriangle tri;

                tri.vertex01 = ((RwUInt32)srceTri->vertIndex[0] << 16) | srceTri->vertIndex[1];
                tri.vertex2Mat =
                    ((RwUInt32)srceTri->vertIndex[2] << 16) | (RwUInt16)srceTri->matIndex;
                srceTri++;

                RwMemLittleEndian32(&tri, sizeof(tri));

                if (!RwStreamWrite(stream, &tri, sizeof(tri)))
                {
                    return (const RpGeometry*)NULL;
                }
            }
        }
    }

    flags = geometry->flags & rpGEOMETRYNATIVE;

    for (i = 0; i < geometry->numMorphTargets; i++)
    {
        _rpMorphTarget kf;

        kf.boundingSphere = geometry->morphTarget[i].boundingSphere;

        if (flags)
        {
            kf.pointsPresent = FALSE;
            kf.normalsPresent = FALSE;
        }
        else
        {
            kf.pointsPresent = (geometry->morphTarget[i].verts != NULL);
            kf.normalsPresent = (geometry->morphTarget[i].normals != NULL);
        }

        RwMemLittleEndian32(&kf, sizeof(kf));

        if (!RwStreamWrite(stream, &kf, sizeof(kf)))
        {
            return (const RpGeometry*)NULL;
        }

        if (kf.pointsPresent)
        {
            if (!RwStreamWriteReal(stream, (const RwReal*)geometry->morphTarget[i].verts,
                                   geometry->numVertices * sizeof(RwV3d)))
            {
                return (const RpGeometry*)NULL;
            }
        }

        if (kf.normalsPresent)
        {
            if (!RwStreamWriteReal(stream, (const RwReal*)geometry->morphTarget[i].normals,
                                   geometry->numVertices * sizeof(RwV3d)))
            {
                return (const RpGeometry*)NULL;
            }
        }
    }

    if (!_rpMaterialListStreamWrite(&geometry->matList, stream))
    {
        return (const RpGeometry*)NULL;
    }

    if (!_rwPluginRegistryWriteDataChunks(&geometryTKList, stream, geometry))
    {
        return (const RpGeometry*)NULL;
    }

    return geometry;
}

RpGeometry* RpGeometryStreamRead(RwStream* stream)
{
    RpGeometry* geometry;
    RpGeometryChunkInfo geom;
    RwUInt32 version;
    RwSurfaceProperties surfaceProps;
    RwInt32 i;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, (RwUInt32*)NULL, &version))
    {
        return (RpGeometry*)NULL;
    }

    if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
    {
        RWERROR((E_RW_BADVERSION));
        return (RpGeometry*)NULL;
    }

    if (version < rpGEOMETRYSURFACEPROPSVERSION)
    {
        if (RwStreamRead(stream, &geom, sizeof(geom)) != sizeof(geom))
        {
            return (RpGeometry*)NULL;
        }

        if (RwStreamRead(stream, &surfaceProps, sizeof(surfaceProps)) != sizeof(surfaceProps))
        {
            return (RpGeometry*)NULL;
        }

        RwMemNative32(&surfaceProps, sizeof(surfaceProps));
    }
    else
    {
        if (RwStreamRead(stream, &geom, sizeof(geom)) != sizeof(geom))
        {
            return (RpGeometry*)NULL;
        }
    }

    RwMemNative32(&geom, sizeof(geom));

    geometry = RpGeometryCreate(geom.numVertices, geom.numTriangles, geom.format);
    if (!geometry)
    {
        return (RpGeometry*)NULL;
    }

    if (geom.numMorphTargets > 1)
    {
        if (RpGeometryAddMorphTargets(geometry, geom.numMorphTargets - 1) < 0)
        {
            RpGeometryDestroy(geometry);
            return (RpGeometry*)NULL;
        }
    }

    if (!(geometry->flags & rpGEOMETRYNATIVE) && geometry->numVertices)
    {
        if (geom.format & rpGEOMETRYPRELIT)
        {
            RwUInt32 sizeLum = geometry->numVertices * sizeof(RwRGBA);

            if (RwStreamRead(stream, geometry->preLitLum, sizeLum) != sizeLum)
            {
                RpGeometryDestroy(geometry);
                return (RpGeometry*)NULL;
            }
        }

        if (geometry->numTexCoordSets > 0)
        {
            RwUInt32 sizeTC = geometry->numVertices * sizeof(RwTexCoords);
            RwInt32 i;

            for (i = 0; i < geometry->numTexCoordSets; i++)
            {
                if (!RwStreamReadReal(stream, (RwReal*)geometry->texCoords[i], sizeTC))
                {
                    RpGeometryDestroy(geometry);
                    return (RpGeometry*)NULL;
                }
            }
        }

        if (geometry->numTriangles)
        {
            RpTriangle* destTri = geometry->triangles;
            RwInt32 numTris = geometry->numTriangles;
            RwUInt32 size = numTris * sizeof(_rpTriangle);
            _rpTriangle* srceTri;

            if (RwStreamRead(stream, destTri, size) != size)
            {
                RpGeometryDestroy(geometry);
                return (RpGeometry*)NULL;
            }

            RwMemNative32(destTri, size);

            /* Unpack the triangles in place */
            while (numTris--)
            {
                RwUInt16 hi;
                RwUInt16 lo;

                srceTri = (_rpTriangle*)destTri;

                hi = (RwUInt16)(srceTri->vertex01 >> 16);
                lo = (RwUInt16)(srceTri->vertex01 & 0xFFFF);
                destTri->vertIndex[0] = hi;
                destTri->vertIndex[1] = lo;

                hi = (RwUInt16)(srceTri->vertex2Mat >> 16);
                lo = (RwUInt16)(srceTri->vertex2Mat & 0xFFFF);
                destTri->vertIndex[2] = hi;
                destTri->matIndex = (RwInt16)lo;

                destTri++;
            }
        }
    }

    for (i = 0; i < geometry->numMorphTargets; i++)
    {
        RpMorphTarget* morphTarget = &geometry->morphTarget[i];
        _rpMorphTarget kf;

        if (RwStreamRead(stream, &kf, sizeof(kf)) != sizeof(kf))
        {
            RpGeometryDestroy(geometry);
            return (RpGeometry*)NULL;
        }

        RwMemNative32(&kf, sizeof(kf));

        morphTarget->boundingSphere = kf.boundingSphere;

        if (kf.pointsPresent && kf.normalsPresent)
        {
            /* Vertices and normals are contiguous, so read them in one go */
            if (!RwStreamReadReal(stream, (RwReal*)morphTarget->verts,
                                  geometry->numVertices * sizeof(RwV3d) * 2))
            {
                RpGeometryDestroy(geometry);
                return (RpGeometry*)NULL;
            }
        }
        else
        {
            if (kf.pointsPresent)
            {
                if (!RwStreamReadReal(stream, (RwReal*)morphTarget->verts,
                                      geometry->numVertices * sizeof(RwV3d)))
                {
                    RpGeometryDestroy(geometry);
                    return (RpGeometry*)NULL;
                }
            }

            if (kf.normalsPresent)
            {
                if (!RwStreamReadReal(stream, (RwReal*)morphTarget->normals,
                                      geometry->numVertices * sizeof(RwV3d)))
                {
                    RpGeometryDestroy(geometry);
                    return (RpGeometry*)NULL;
                }
            }
        }
    }

    if (!RwStreamFindChunk(stream, rwID_MATLIST, (RwUInt32*)NULL, &version))
    {
        return (RpGeometry*)NULL;
    }

    if ((version < rwLIBRARYBASEVERSION) || (version > rwLIBRARYCURRENTVERSION))
    {
        RpGeometryDestroy(geometry);
        RWERROR((E_RW_BADVERSION));
        return (RpGeometry*)NULL;
    }

    if (version < rpGEOMETRYSURFACEPROPSVERSION)
    {
        _rpMaterialSetDefaultSurfaceProperties(&surfaceProps);
    }

    if (!_rpMaterialListStreamRead(stream, &geometry->matList))
    {
        RpGeometryDestroy(geometry);
        return (RpGeometry*)NULL;
    }

    if (version < rpGEOMETRYSURFACEPROPSVERSION)
    {
        _rpMaterialSetDefaultSurfaceProperties((RwSurfaceProperties*)NULL);
    }

    if (!_rwPluginRegistryReadDataChunks(&geometryTKList, stream, geometry))
    {
        RpGeometryDestroy(geometry);
        return (RpGeometry*)NULL;
    }

    if (!RpGeometryUnlock(geometry))
    {
        RpGeometryDestroy(geometry);
        return (RpGeometry*)NULL;
    }

    return geometry;
}
