#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

/* The optimized instancer always owns the result through the sector itself and ignores the
 * owner arguments, but it is called through the same interface as the fast path. */
extern RwResEntry* _rwDlWorldSectorInstanceOptimized(RpWorld* world, RpWorldSector* sector,
                                                     void* owner, RwResEntry** resEntryOwner);

typedef struct RpLightTie RpLightTie;
struct RpLightTie
{
    RwLLLink lWorldSector;
    RpLight* light;
    RwLLLink lLight;
    RpWorldSector* sect;
};

void* _rxGCWorldSectorDefaultLightingCallback(void* object, RxGameCubePipeData* pipeData)
{
    RwUInt32 flags;
    RwLLLink* curLight;
    RwLLLink* endLight;
    RpWorldSector* sector;
    RpLight* light;
    RpLightTie* lightTie;

    sector = (RpWorldSector*)object;

    pipeData->lightMask = 0;
    pipeData->ambientLightColor.red = 0.0f;
    pipeData->ambientLightColor.green = 0.0f;
    pipeData->ambientLightColor.blue = 0.0f;
    pipeData->ambientLightColor.alpha = 1.0f;
    pipeData->ambientLight = FALSE;
    pipeData->numLights = 0;

    flags = RpWorldGetFlags((RpWorld*)RWSRCGLOBAL(curWorld));
    if (flags & rpWORLDLIGHT)
    {
        _rwGCLightsGlobalEnable(rpLIGHTLIGHTWORLD, pipeData);

        RWSRCGLOBAL(lightFrame)++;

        curLight = rwLinkListGetFirstLLLink(&sector->lightsInWorldSector);
        endLight = rwLinkListGetTerminator(&sector->lightsInWorldSector);
        while (curLight != endLight)
        {
            lightTie = rwLLLinkGetData(curLight, RpLightTie, lWorldSector);
            light = lightTie->light;

            if (light != NULL && rwObjectTestFlags(light, rpLIGHTLIGHTWORLD))
            {
                _rwGCLightsLocalEnable(light, pipeData);
            }

            curLight = rwLLLinkGetNext(curLight);
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

void* _rxGCWorldSectorDefaultInstanceCallback(void* object, RxGameCubePipeData* pipeData)
{
    RpWorldSector* sector;
    RpWorld* world;

    sector = (RpWorldSector*)object;
    world = (RpWorld*)RWSRCGLOBAL(curWorld);

    if (RpWorldGetFlags(world) & rpWORLDNATIVEINSTANCE)
    {
        if (_RwDlPreInstanceOptimize == TRUE)
        {
            pipeData->resEntry =
                _rwDlWorldSectorInstanceOptimized(world, sector, sector, &sector->repEntry);
        }
        else
        {
            pipeData->resEntry =
                _rwDlWorldSectorInstanceFast(world, sector, sector, &sector->repEntry);
        }
    }
    else
    {
        pipeData->resEntry = _rwDlWorldSectorInstanceFast(world, sector, sector, &sector->repEntry);
    }

    return object;
}

static RwBool _rxGCWorldSectorAllInOneNode(RxPipelineNode* self, const RxPipelineNodeParam* params)
{
    RpWorld* world;
    RpWorldSector* sector;
    RpMeshHeader* meshHeader;
    _rxGameCubeAllInOneNodeData* nodeData;
    RxGameCubePipeData pipeData;

    sector = (RpWorldSector*)params->dataParam;
    nodeData = (_rxGameCubeAllInOneNodeData*)self->privateData;
    pipeData.nodeData = nodeData;

    world = (RpWorld*)RWSRCGLOBAL(curWorld);

    if (!(RpWorldGetFlags(world) & rpWORLDNATIVE))
    {
        if (sector->numVertices == 0)
        {
            return TRUE;
        }

        meshHeader = sector->mesh;
        if (meshHeader->numMeshes == 0)
        {
            return TRUE;
        }

        pipeData.resEntry = sector->repEntry;
        pipeData.meshHeader = meshHeader;
        pipeData.flags = RpWorldGetFlags(world);

        if (pipeData.resEntry != NULL &&
            ((RxGameCubeVertexBuffer*)(pipeData.resEntry + 1))->serialNumber !=
                meshHeader->serialNum)
        {
            RwResourcesFreeResEntry(pipeData.resEntry);
            pipeData.resEntry = NULL;
        }

        if (pipeData.resEntry != NULL)
        {
            RwResourcesUseResEntry(pipeData.resEntry);
        }
        else if (nodeData->instanceCallback != NULL)
        {
            if (nodeData->instanceCallback(sector, &pipeData) != sector)
            {
                return FALSE;
            }
        }
    }
    else
    {
        pipeData.resEntry = sector->repEntry;
        pipeData.meshHeader = sector->mesh;
        pipeData.flags = RpWorldGetFlags(world);
    }

    if (nodeData->lightingCallback != NULL)
    {
        if (nodeData->lightingCallback(sector, &pipeData) != sector)
        {
            return FALSE;
        }
    }

    if (nodeData->renderCallback != NULL)
    {
        if (nodeData->renderCallback(sector, &pipeData) != sector)
        {
            return FALSE;
        }
    }

    return TRUE;
}

static RwBool _rxGCWorldSectorAllInOnePipelineInit(RxPipelineNode* node)
{
    _rxGameCubeAllInOneNodeData* nodeData;

    nodeData = (_rxGameCubeAllInOneNodeData*)node->privateData;

    nodeData->instanceCallback = _rxGCWorldSectorDefaultInstanceCallback;
    nodeData->reinstanceCallback = NULL;
    nodeData->lightingCallback = _rxGCWorldSectorDefaultLightingCallback;
    nodeData->renderCallback = _rxGCDefaultRenderCallback;

    return TRUE;
}

RxNodeDefinition* RxNodeDefinitionGetGameCubeWorldSectorAllInOne(void)
{
    static RxNodeDefinition nodeGameCubeWorldSectorAllInOneCSL = {
        "GamerCubeWorldSectorAllInOne.csl",
        { _rxGCWorldSectorAllInOneNode, NULL, NULL, _rxGCWorldSectorAllInOnePipelineInit, NULL,
          NULL, NULL },
        { 0, NULL, NULL, 0, NULL },
        sizeof(_rxGameCubeAllInOneNodeData),
        (RxNodeDefEditable)FALSE,
        0
    };

    return &nodeGameCubeWorldSectorAllInOneCSL;
}
