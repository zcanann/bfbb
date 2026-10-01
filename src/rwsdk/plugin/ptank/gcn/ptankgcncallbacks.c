#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpptank.h>
#include <string.h>
#include <math.h>

#include "rwsdk/world/pipe/p2/gcn/gcpipe.h"

/* Render function selection, stored in the platform flags */
#define rpPTANKGCNPOSNCPPM 0x00000001
#define rpPTANKGCNPOSCCPPM 0x00000002
#define rpPTANKGCNPOSNCCSNR 0x00000004
#define rpPTANKGCNPOSCCCSNR 0x00000008
#define rpPTANKGCNPOSNCPPSNR 0x00000010
#define rpPTANKGCNPOSCCPPSNR 0x00000020
#define rpPTANKGCNPOSNCCSPPR 0x00000040
#define rpPTANKGCNPOSCCCSPPR 0x00000080
#define rpPTANKGCNPOSNCPPSPPR 0x00000100
#define rpPTANKGCNPOSCCPPSPPR 0x00000200

#define rpPTANKGCNCOLORPP 0x00010000
#define rpPTANKGCNCOLORPPV 0x00020000
#define rpPTANKGCNCOLORCSV 0x00040000
#define rpPTANKGCNCOLORCS 0x00080000

#define rpPTANKGCNUVPP2 0x00100000
#define rpPTANKGCNUVPP4 0x00200000
#define rpPTANKGCNUVCS2 0x00400000
#define rpPTANKGCNUVCS4 0x00800000

#define rpPTANKGCNNORMALPP 0x01000000

typedef struct PTankGameCubeBillboard PTankGameCubeBillboard;
struct PTankGameCubeBillboard
{
    RwV3d right;
    RwV3d up;
};

extern RpGameCubeVtxFmt* RpGameCubeVtxFmtCreate(void);
extern RwBool RpGameCubeVtxFmtDestroy(RpGameCubeVtxFmt* vtxFmt);
extern RpGameCubeVtxFmt* RpGameCubeVtxFmtSetPosition(RpGameCubeVtxFmt* vtxFmt,
                                                     RpGameCubeCompType type, RwUInt8 frac);
extern RpGameCubeVtxFmt* RpGameCubeVtxFmtSetNormal(RpGameCubeVtxFmt* vtxFmt,
                                                   RpGameCubeCompType type, RwBool nbt);
extern RpGameCubeVtxFmt* RpGameCubeVtxFmtSetPreLight(RpGameCubeVtxFmt* vtxFmt,
                                                     RpGameCubeColorCompType type);
extern RpGameCubeVtxFmt* RpGameCubeVtxFmtSetTexCoord(RpGameCubeVtxFmt* vtxFmt, RwUInt32 numTexCoords,
                                                     RpGameCubeCompType type, RwUInt8 frac);
extern RpGeometry* RpGameCubeGeometrySetVtxFmt(RpGeometry* geometry, RpGameCubeVtxFmt* vtxFmt);

extern RwInt16 _rpMeshGetNextSerialNumber(void);

extern RwBool RwRenderStateGet(RwRenderState state, void* value);
extern RwBool RwRenderStateSet(RwRenderState state, void* value);

extern RxPipeline* _rxPTankGameCubeRenderPipeline;

extern void _rpPTankGameCubeAsmVtxRender_NC_PPM();
extern void _rpPTankGameCubeAsmVtxRender_CC_PPM();
extern void _rpPTankGameCubeAsmVtxRender_NC_CS_NR();
extern void _rpPTankGameCubeAsmVtxRender_CC_CS_NR();
extern void _rpPTankGameCubeAsmVtxRender_NC_PPS_NR();
extern void _rpPTankGameCubeAsmVtxRender_CC_PPS_NR();
extern void _rpPTankGameCubeAsmVtxRender_NC_CS_PPR();
extern void _rpPTankGameCubeAsmVtxRender_CC_CS_PPR();
extern void _rpPTankGameCubeAsmVtxRender_NC_PPS_PPR();
extern void _rpPTankGameCubeAsmVtxRender_CC_PPS_PPR();

/* Mesh module globals, see bamesh.c */
typedef struct rpMeshGlobals rpMeshGlobals;
struct rpMeshGlobals
{
    RwInt16 nextSerialNum;
    void* reserved;
    RwUInt8 meshFlagsToPrimType[0x1F];
    RwUInt8 primTypeToMeshFlags[0x7];
};

extern RwModuleInfo meshModule;

#define RWMESHGLOBAL(var)                                                                          \
    (RWPLUGINOFFSET(rpMeshGlobals, RwEngineInstance, meshModule.globalsOffset)->var)

#define rpMESHHEADERPRIMMASK 0x00FF

#define _rpMeshHeaderSetPrimType(_mshHdr, _prmTyp)                                                 \
    ((_mshHdr)->flags = ((_mshHdr)->flags & ~rpMESHHEADERPRIMMASK) |                               \
                        (rpMESHHEADERPRIMMASK & RWMESHGLOBAL(primTypeToMeshFlags)[_prmTyp]))

RwBool _rpPTankGameCubeCreateCallBack(RpAtomic* atomic, RpPTankData* ptankGlobal,
                                      RwInt32 maxPCount, RwUInt32 dataFlags, RwUInt32 platFlags);
RwBool _rpPTankGameCubeInstanceCallBack(RpAtomic* atomic, RpPTankData* ptankGlobal,
                                        RwInt32 actPCount, RwUInt32 instFlags);
RwBool _rpPTankGameCubeRenderCallBack(RpAtomic* atomic, RpPTankData* ptankGlobal,
                                      RwInt32 actPCount);

RpPTankCallBacks defaultCB = {
    (RpPTankAllocCallBack)NULL,
    _rpPTankGameCubeCreateCallBack,
    _rpPTankGameCubeInstanceCallBack,
    _rpPTankGameCubeRenderCallBack
};

void PTankGameCubeSetupRenderCallback(RpAtomic* atomic)
{
    RpPTankAtomicExtPrv* ptankPrv = RPATOMICPTANKPLUGINDATA(atomic);
    RwUInt32 platFlags = 0;

    ptankPrv->insSetupCB = (RpPTankGENInstanceSetupCallback)NULL;
    ptankPrv->insPosCB = (RpPTankGENInstancePosCallback)NULL;
    ptankPrv->insUVCB = (RpPTankGENInstanceCallback)NULL;
    ptankPrv->insColorsCB = (RpPTankGENInstanceCallback)NULL;
    ptankPrv->insNormalsCB = (RpPTankGENInstanceCallback)NULL;
    ptankPrv->insEndingCB = (RpPTankGENInstanceEndingCallback)NULL;

    /* Position */
    if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGMATRIX)
    {
        if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGUSECENTER)
        {
            platFlags |= rpPTANKGCNPOSCCPPM;
            ptankPrv->insPosCB = (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_CC_PPM;
        }
        else
        {
            platFlags |= rpPTANKGCNPOSNCPPM;
            ptankPrv->insPosCB = (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_NC_PPM;
        }
    }
    else if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGCNSMATRIX)
    {
        if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGUSECENTER)
        {
            platFlags |= rpPTANKGCNPOSCCCSNR;
            ptankPrv->insPosCB =
                (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_CC_CS_NR;
        }
        else
        {
            platFlags |= rpPTANKGCNPOSNCCSNR;
            ptankPrv->insPosCB =
                (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_NC_CS_NR;
        }
    }
    else if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAG2DROTATE)
    {
        if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGSIZE)
        {
            if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGUSECENTER)
            {
                platFlags |= rpPTANKGCNPOSCCPPSPPR;
                ptankPrv->insPosCB =
                    (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_CC_PPS_PPR;
            }
            else
            {
                platFlags |= rpPTANKGCNPOSNCPPSPPR;
                ptankPrv->insPosCB =
                    (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_NC_PPS_PPR;
            }
        }
        else
        {
            if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGUSECENTER)
            {
                platFlags |= rpPTANKGCNPOSCCCSPPR;
                ptankPrv->insPosCB =
                    (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_CC_CS_PPR;
            }
            else
            {
                platFlags |= rpPTANKGCNPOSNCCSPPR;
                ptankPrv->insPosCB =
                    (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_NC_CS_PPR;
            }
        }
    }
    else
    {
        if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGSIZE)
        {
            if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGUSECENTER)
            {
                platFlags |= rpPTANKGCNPOSCCPPSNR;
                ptankPrv->insPosCB =
                    (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_CC_PPS_NR;
            }
            else
            {
                platFlags |= rpPTANKGCNPOSNCPPSNR;
                ptankPrv->insPosCB =
                    (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_NC_PPS_NR;
            }
        }
        else
        {
            if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGUSECENTER)
            {
                platFlags |= rpPTANKGCNPOSCCCSNR;
                ptankPrv->insPosCB =
                    (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_CC_CS_NR;
            }
            else
            {
                platFlags |= rpPTANKGCNPOSNCCSNR;
                ptankPrv->insPosCB =
                    (RpPTankGENInstancePosCallback)_rpPTankGameCubeAsmVtxRender_NC_CS_NR;
            }
        }
    }

    /* Texture coordinates */
    if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGVTX2TEXCOORDS)
    {
        platFlags |= rpPTANKGCNUVPP2;
    }
    else if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGVTX4TEXCOORDS)
    {
        platFlags |= rpPTANKGCNUVPP4;
    }
    else if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGCNSVTX2TEXCOORDS)
    {
        platFlags |= rpPTANKGCNUVCS2;
    }
    else if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGCNSVTX4TEXCOORDS)
    {
        platFlags |= rpPTANKGCNUVCS4;
    }

    /* Colors */
    if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGCOLOR)
    {
        platFlags |= rpPTANKGCNCOLORPP;
    }
    else if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGVTXCOLOR)
    {
        platFlags |= rpPTANKGCNCOLORPPV;
    }
    else if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGCNSVTXCOLOR)
    {
        platFlags |= rpPTANKGCNCOLORCSV;
    }
    else
    {
        platFlags |= rpPTANKGCNCOLORCS;
    }

    /* Normals */
    if (ptankPrv->publicData.format.dataFlags & rpPTANKDFLAGNORMAL)
    {
        platFlags |= rpPTANKGCNNORMALPP;
    }

    ptankPrv->platFlags = platFlags;
}

RwBool _rpPTankGameCubeCreateCallBack(RpAtomic* atomic, RpPTankData* ptankGlobal,
                                      RwInt32 maxPCount, RwUInt32 dataFlags, RwUInt32 platFlags)
{
    RpGeometry* geometry;
    RwUInt32 geometryFlags;
    RpGameCubeVtxFmt* vtxFmt;
    RpMaterial* material;
    RpMeshHeader* meshHeader;
    RpMesh* mesh;
    RpTriangle triangle;
    RwSurfaceProperties surfaceProps;

    vtxFmt = RpGameCubeVtxFmtCreate();
    geometryFlags = rpGEOMETRYPOSITIONS;

    RpGameCubeVtxFmtSetPosition(vtxFmt, rpF32, 0);

    if ((dataFlags & rpPTANKDFLAGNORMAL) == rpPTANKDFLAGNORMAL)
    {
        geometryFlags |= rpGEOMETRYNORMALS | rpGEOMETRYLIGHT;
        RpGameCubeVtxFmtSetNormal(vtxFmt, rpF32, FALSE);
    }

    if (dataFlags & (rpPTANKDFLAGCOLOR | rpPTANKDFLAGVTXCOLOR | rpPTANKDFLAGCNSVTXCOLOR))
    {
        geometryFlags |= rpGEOMETRYPRELIT;
        RpGameCubeVtxFmtSetPreLight(vtxFmt, rpRGBA8);
    }

    if (dataFlags & (rpPTANKDFLAGVTX2TEXCOORDS | rpPTANKDFLAGVTX4TEXCOORDS |
                     rpPTANKDFLAGCNSVTX2TEXCOORDS | rpPTANKDFLAGCNSVTX4TEXCOORDS))
    {
        geometryFlags |= rpGEOMETRYTEXTURED;
        RpGameCubeVtxFmtSetTexCoord(vtxFmt, 1, rpF32, 0);
    }

    geometry = RpGeometryCreateSpace(1.0f);
    geometry->flags = geometryFlags;
    geometry->numVertices = 1;

    RpGameCubeGeometrySetVtxFmt(geometry, vtxFmt);
    RpGameCubeVtxFmtDestroy(vtxFmt);

    material = RpMaterialCreate();

    surfaceProps.ambient = 0.3f;
    surfaceProps.specular = 1.0f;
    surfaceProps.diffuse = 1.0f;
    RpMaterialSetSurfacePropertiesVoidMacro(material, &surfaceProps);

    RpGeometryTriangleSetMaterial(geometry, &triangle, material);

    /* A single mesh of points */
    meshHeader = _rpMeshHeaderCreate(sizeof(RpMeshHeader) + sizeof(RpMesh));
    memset(meshHeader, 0, sizeof(RpMeshHeader) + sizeof(RpMesh));

    _rpMeshHeaderSetPrimType(meshHeader, rwPRIMTYPEPOINTLIST);

    mesh = (RpMesh*)(meshHeader + 1);
    mesh->indices = (RxVertexIndex*)(meshHeader + 1);
    mesh->material = material;

    meshHeader->numMeshes = 1;
    meshHeader->serialNum = _rpMeshGetNextSerialNumber();
    meshHeader->firstMeshOffset = 0;
    meshHeader->totalIndicesInMesh = 0;

    geometry->mesh = meshHeader;

    atomic = RpAtomicSetGeometry(atomic, geometry, 0);

    RpGeometryDestroy(geometry);
    RpMaterialDestroy(material);

    PTankGameCubeSetupRenderCallback(atomic);

    atomic->pipeline = _rxPTankGameCubeRenderPipeline;

    return TRUE;
}

RwBool _rpPTankGameCubeInstanceCallBack(RpAtomic* atomic, RpPTankData* ptankGlobal,
                                        RwInt32 actPCount, RwUInt32 instFlags)
{
    return TRUE;
}

RwBool _rpPTankGameCubeRenderCallBack(RpAtomic* atomic, RpPTankData* ptankGlobal,
                                      RwInt32 actPCount)
{
    RpPTankAtomicExtPrv* ptankPrv = RPATOMICPTANKPLUGINDATA(atomic);
    RwInt32 dataFlags = ptankPrv->publicData.format.dataFlags;
    PTankGameCubeBillboard billboard;
    RwBlendFunction srcBlend;
    RwBlendFunction dstBlend;

    if ((dataFlags & rpPTANKDFLAGCNSMATRIX) == rpPTANKDFLAGCNSMATRIX)
    {
        billboard.right = ptankPrv->publicData.cMatrix.right;
        billboard.up = ptankPrv->publicData.cMatrix.up;
    }
    else
    {
        RwMatrix* camLTM =
            RwFrameGetLTM((RwFrame*)rwObjectGetParent(RWSRCGLOBAL(curCamera)));

        if ((dataFlags & rpPTANKDFLAGCNS2DROTATE) == rpPTANKDFLAGCNS2DROTATE)
        {
            RwReal sinA = (RwReal)sin(ptankPrv->publicData.cRotate);
            RwReal cosA = (RwReal)cos(ptankPrv->publicData.cRotate);
            RwReal nsinA = -sinA;

            billboard.right.x = cosA * camLTM->right.x + nsinA * camLTM->up.x;
            billboard.right.y = cosA * camLTM->right.y + nsinA * camLTM->up.y;
            billboard.right.z = cosA * camLTM->right.z + nsinA * camLTM->up.z;
            billboard.up.x = sinA * camLTM->right.x + cosA * camLTM->up.x;
            billboard.up.y = sinA * camLTM->right.y + cosA * camLTM->up.y;
            billboard.up.z = sinA * camLTM->right.z + cosA * camLTM->up.z;
        }
        else
        {
            billboard.right = camLTM->right;
            billboard.up = camLTM->up;
        }
    }

    billboard.up.x = -1.0f * billboard.up.x;
    billboard.up.y = -1.0f * billboard.up.y;
    billboard.up.z = -1.0f * billboard.up.z;

    ptankPrv->publicData.userData = &billboard;

    if (ptankPrv->publicData.vertexAlphaBlend)
    {
        RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcBlend);
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)ptankPrv->publicData.srcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dstBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)ptankPrv->publicData.dstBlend);
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
    }

    ptankPrv->defaultRenderCB(atomic);

    if (ptankPrv->publicData.vertexAlphaBlend)
    {
        RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)srcBlend);
        RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)dstBlend);
    }

    return FALSE;
}
