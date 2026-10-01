#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <dolphin/os.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

typedef struct RpTie RpTie;
struct RpTie
{
    RwLLLink lWorldSector;
    RpAtomic* apAtom;
    RwLLLink lAtomic;
    RpWorldSector* worldSector;
};

typedef struct RpLightTie RpLightTie;
struct RpLightTie
{
    RwLLLink lWorldSector;
    RpLight* light;
    RwLLLink lLight;
    RpWorldSector* sect;
};

void* _rxGCAtomicDefaultLightingCallback(void* object, RxGameCubePipeData* pipeData)
{
    RpAtomic* atomic;
    RwInt32 flags;
    RwLLLink* cur;
    RwLLLink* end;
    RpTie* tpTie;
    RwLLLink* curLight;
    RwLLLink* endLight;
    RpLightTie* lightTie;
    RpLight* theLight;
    const RwMatrix* matrixLight;
    const RwSphere* sphere;
    RwV3d distanceVector;
    RwReal distanceSquare;
    RwReal distanceCollision;

    atomic = (RpAtomic*)object;
    flags = RpGeometryGetFlags(atomic->geometry);

    pipeData->lightMask = 0;
    pipeData->ambientLightColor.red = 0.0f;
    pipeData->ambientLightColor.green = 0.0f;
    pipeData->ambientLightColor.blue = 0.0f;
    pipeData->ambientLightColor.alpha = 1.0f;
    pipeData->ambientLight = FALSE;
    pipeData->numLights = 0;

    if ((flags & rpGEOMETRYLIGHT) && RWSRCGLOBAL(curWorld) != NULL)
    {
        _rwGCLightsGlobalEnable(rpLIGHTLIGHTATOMICS, pipeData);

        RWSRCGLOBAL(lightFrame)++;

        cur = rwLinkListGetFirstLLLink(&atomic->llWorldSectorsInAtomic);
        end = rwLinkListGetTerminator(&atomic->llWorldSectorsInAtomic);
        while (cur != end)
        {
            tpTie = rwLLLinkGetData(cur, RpTie, lAtomic);

            curLight = rwLinkListGetFirstLLLink(&tpTie->worldSector->lightsInWorldSector);
            endLight = rwLinkListGetTerminator(&tpTie->worldSector->lightsInWorldSector);
            while (curLight != endLight)
            {
                lightTie = rwLLLinkGetData(curLight, RpLightTie, lWorldSector);
                theLight = lightTie->light;

                if (theLight != NULL && theLight->lightFrame != RWSRCGLOBAL(lightFrame) &&
                    rwObjectTestFlags(theLight, rpLIGHTLIGHTATOMICS))
                {
                    theLight->lightFrame = RWSRCGLOBAL(lightFrame);

                    matrixLight = RwFrameGetLTM(RpLightGetFrame(theLight));
                    sphere = RpAtomicGetWorldBoundingSphere(atomic);

                    distanceVector.x = sphere->center.x - matrixLight->pos.x;
                    distanceVector.y = sphere->center.y - matrixLight->pos.y;
                    distanceVector.z = sphere->center.z - matrixLight->pos.z;

                    distanceSquare = distanceVector.x * distanceVector.x +
                                     distanceVector.y * distanceVector.y +
                                     distanceVector.z * distanceVector.z;
                    distanceCollision = sphere->radius + theLight->radius;

                    if (distanceSquare < distanceCollision * distanceCollision)
                    {
                        _rwGCLightsLocalEnable(lightTie->light, pipeData);
                    }
                }

                curLight = rwLLLinkGetNext(curLight);
            }

            cur = rwLLLinkGetNext(cur);
        }
    }

    if (pipeData->ambientLight)
    {
        if (pipeData->ambientLightColor.red > 1.0f)
        {
            pipeData->ambientLightColor.red = 1.0f;
        }
        if (pipeData->ambientLightColor.green > 1.0f)
        {
            pipeData->ambientLightColor.green = 1.0f;
        }
        if (pipeData->ambientLightColor.blue > 1.0f)
        {
            pipeData->ambientLightColor.blue = 1.0f;
        }
    }

    return object;
}

static void _rxGCDefaultReinstance(RpGeometry* geometry, RxGameCubeVertexBuffer* vbHeader,
                                   _rxGameCubeAllInOneNodeData* nodeData)
{
    RwUInt32 numVerts;
    RwUInt32 size;
    RwUInt32 geomFlags;
    RwUInt32 count;
    RwUInt16 lockedFlags;
    RpGameCubeVtxFmt* vtxFmt;
    RwUInt32 i;

    geomFlags = RpGeometryGetFlags(geometry) | rpGEOMETRYPOSITIONS;
    vtxFmt = GEOMVTXFMT(geometry);
    lockedFlags = geometry->lockedSinceLastInst;
    numVerts = geometry->numVertices;
    count = 0;

    if (vtxFmt == NULL)
    {
        vtxFmt = _rpGameCubeVtxFmtGetDefault();
    }

    if (geomFlags & rpGEOMETRYPOSITIONS)
    {
        if (lockedFlags & rpGEOMETRYLOCKVERTICES)
        {
            size = _rwGCNVtxFmtInstPos3D((RwUInt8*)vbHeader->attr[count].array,
                                         geometry->morphTarget[0].verts, vtxFmt->pos,
                                         (RwReal)(1 << vtxFmt->posFrac), geometry->numVertices,
                                         vbHeader->attr[count].stride);
            DCFlushRange(vbHeader->attr[count].array, size);
        }
        count++;
    }

    if (geomFlags & rpGEOMETRYNORMALS)
    {
        if (lockedFlags & rpGEOMETRYLOCKNORMALS)
        {
            size = _rwGCNVtxFmtInstNrm((RwUInt8*)vbHeader->attr[count].array,
                                       geometry->morphTarget[0].normals, vtxFmt->norm,
                                       geometry->numVertices, vbHeader->attr[count].stride);
            DCFlushRange(vbHeader->attr[count].array, size);
        }
        count++;
    }

    if (geomFlags & rpGEOMETRYPRELIT)
    {
        if (lockedFlags & rpGEOMETRYLOCKPRELIGHT)
        {
            if (GEOMVTXFMT(geometry) == NULL)
            {
                vbHeader->flags &= ~1;

                for (i = 0; i < numVerts; i++)
                {
                    if (geometry->preLitLum[i].alpha < 0xff)
                    {
                        vbHeader->flags |= 1;
                        break;
                    }
                }
            }
            else if (vtxFmt->preLight > rpRGBX8)
            {
                vbHeader->flags |= 1;
            }
            else
            {
                vbHeader->flags &= ~1;
            }

            size = _rwGCNVtxFmtInstClr((RwUInt8*)vbHeader->attr[count].array, geometry->preLitLum,
                                       vtxFmt->preLight, geometry->numVertices,
                                       vbHeader->attr[count].stride);
            DCFlushRange(vbHeader->attr[count].array, size);
        }
        count++;
    }

    if (geomFlags & (rpGEOMETRYTEXTURED | rpGEOMETRYTEXTURED2))
    {
        RwInt32 j;

        for (j = 0; j < geometry->numTexCoordSets; j++)
        {
            if ((rpGEOMETRYLOCKTEXCOORDS1 << j) & lockedFlags)
            {
                size = _rwGCNVtxFmtInstTex((RwUInt8*)vbHeader->attr[count].array,
                                           geometry->texCoords[j], vtxFmt->texCoord[j],
                                           (RwReal)(1 << vtxFmt->texCoordFrac[j]),
                                           geometry->numVertices, vbHeader->attr[count].stride);
                DCFlushRange(vbHeader->attr[count].array, size);
            }
            count++;
        }
    }

    GXInvalidateVtxCache();
}

void* _rxGCAtomicDefaultReinstanceCallback(void* object, RxGameCubePipeData* pipeData)
{
    RpAtomic* atomic;
    RpGeometry* geometry;
    RxGameCubeVertexBuffer* vbHeader;

    atomic = (RpAtomic*)object;
    geometry = atomic->geometry;
    vbHeader = (RxGameCubeVertexBuffer*)(pipeData->resEntry + 1);

    if (geometry->lockedSinceLastInst)
    {
        RXGCVERTEXBUFFERWAITDONE(vbHeader);

        _rxGCDefaultReinstance(geometry, vbHeader,
                               (_rxGameCubeAllInOneNodeData*)pipeData->nodeData);
        geometry->lockedSinceLastInst = 0;
    }

    if (geometry->numMorphTargets != 1 &&
        (atomic->interpolator.flags & rpINTERPOLATORDIRTYINSTANCE))
    {
        RXGCVERTEXBUFFERWAITDONE(vbHeader);

        _rxGCInstanceMorphUpdate(geometry, vbHeader, &atomic->interpolator);
        atomic->interpolator.flags &= ~rpINTERPOLATORDIRTYINSTANCE;
    }

    return object;
}

void* _rxGCAtomicDefaultInstanceCallback(void* object, RxGameCubePipeData* pipeData)
{
    void* owner;
    RpAtomic* atomic;
    RpGeometry* geometry;
    RwResEntry** resEntryOwner;

    atomic = (RpAtomic*)object;
    geometry = atomic->geometry;

    if (geometry->numMorphTargets != 1)
    {
        owner = atomic;
        resEntryOwner = &atomic->repEntry;
    }
    else
    {
        owner = geometry;
        resEntryOwner = &geometry->repEntry;
    }

    if (RpGeometryGetFlags(geometry) & rpGEOMETRYNATIVEINSTANCE)
    {
        if (_RwDlPreInstanceOptimize == TRUE)
        {
            pipeData->resEntry = _rwDlGeometryInstanceOptimized(geometry, owner, resEntryOwner);
        }
        else
        {
            pipeData->resEntry = _rwDlGeometryInstanceFast(geometry, owner, resEntryOwner);
        }
    }
    else
    {
        pipeData->resEntry = _rwDlGeometryInstanceFast(geometry, owner, resEntryOwner);
    }

    geometry->lockedSinceLastInst = 0;

    return object;
}

static RwBool _rxGCAtomicAllInOneNode(RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RpAtomic* atomic;
    RpGeometry* geometry;
    RpMeshHeader* meshHeader;
    RwUInt32 numMeshes;
    RwUInt32 numVerts;
    RxGameCubePipeData pipeData;
    _rxGameCubeAllInOneNodeData* nodeData;

    atomic = (RpAtomic*)params->dataParam;
    nodeData = (_rxGameCubeAllInOneNodeData*)self->privateData;
    geometry = atomic->geometry;
    pipeData.nodeData = nodeData;

    if (!(RpGeometryGetFlags(geometry) & rpGEOMETRYNATIVE))
    {
        numVerts = geometry->numVertices;
        if (numVerts == 0)
        {
            return TRUE;
        }

        meshHeader = geometry->mesh;
        numMeshes = meshHeader->numMeshes;
        if (numMeshes == 0)
        {
            return TRUE;
        }

        if (geometry->numMorphTargets != 1)
        {
            pipeData.resEntry = atomic->repEntry;
        }
        else
        {
            pipeData.resEntry = geometry->repEntry;
        }
        pipeData.meshHeader = geometry->mesh;
        pipeData.flags = RpGeometryGetFlags(geometry);

        if (pipeData.resEntry != NULL &&
            ((RxGameCubeVertexBuffer*)(pipeData.resEntry + 1))->serialNumber !=
                meshHeader->serialNum)
        {
            RwResourcesFreeResEntry(pipeData.resEntry);
            pipeData.resEntry = NULL;
        }

        if (pipeData.resEntry != NULL)
        {
            if (nodeData->reinstanceCallback != NULL)
            {
                if (nodeData->reinstanceCallback(atomic, &pipeData) != atomic)
                {
                    return FALSE;
                }
            }

            RwResourcesUseResEntry(pipeData.resEntry);
        }
        else
        {
            if (nodeData->instanceCallback != NULL)
            {
                if (nodeData->instanceCallback(atomic, &pipeData) != atomic)
                {
                    return FALSE;
                }
            }

            if (nodeData->reinstanceCallback != NULL)
            {
                if (nodeData->reinstanceCallback(atomic, &pipeData) != atomic)
                {
                    return FALSE;
                }
            }
        }
    }
    else
    {
        pipeData.resEntry = geometry->repEntry;
        pipeData.meshHeader = geometry->mesh;
        pipeData.flags = RpGeometryGetFlags(geometry);

        if (nodeData->reinstanceCallback != NULL)
        {
            if (nodeData->reinstanceCallback(atomic, &pipeData) != atomic)
            {
                return FALSE;
            }
        }
    }

    if (nodeData->lightingCallback != NULL)
    {
        if (nodeData->lightingCallback(atomic, &pipeData) != atomic)
        {
            return FALSE;
        }
    }

    if (nodeData->renderCallback != NULL)
    {
        if (nodeData->renderCallback(atomic, &pipeData) != atomic)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static RwBool _rxGCAtomicAllInOnePipelineInit(RxPipelineNode* node)
{
    _rxGameCubeAllInOneNodeData* nodeData;

    nodeData = (_rxGameCubeAllInOneNodeData*)node->privateData;

    nodeData->instanceCallback = _rxGCAtomicDefaultInstanceCallback;
    nodeData->reinstanceCallback = _rxGCAtomicDefaultReinstanceCallback;
    nodeData->lightingCallback = _rxGCAtomicDefaultLightingCallback;
    nodeData->renderCallback = _rxGCDefaultRenderCallback;

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetGameCubeAtomicAllInOne(void)
{
    static RxNodeDefinition nodeGameCubeAtomicAllInOneCSL = {
        "GameCubeAtomicAllInOne.csl",
        { _rxGCAtomicAllInOneNode, NULL, NULL, _rxGCAtomicAllInOnePipelineInit, NULL, NULL, NULL },
        { 0, NULL, NULL, 0, NULL },
        sizeof(_rxGameCubeAllInOneNodeData),
        (RxNodeDefEditable)FALSE,
        0
    };

    return &nodeGameCubeAtomicAllInOneCSL;
}
