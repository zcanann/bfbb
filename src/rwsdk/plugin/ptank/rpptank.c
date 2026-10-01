#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>
#include <rwsdk/rpptank.h>
#include <string.h>

#define rwID_PTANKPLUGIN 0x12F

#define PTANKFLAGISSET(_flags, _flag) (((_flags) & (_flag)) != 0)

extern RwBool RwRenderStateGet(RwRenderState state, void* value);

extern void* PTankOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* PTankClose(void* instance, RwInt32 offset, RwInt32 size);

extern RpPTankCallBacks defaultCB;

RwInt32 _rpPTankAtomicDataOffset;
RwInt32 _rpPTankGlobalsOffset;

const RwInt32 datasize[] = {
    sizeof(RwV3d),            /* position */
    sizeof(RwMatrix),         /* matrix */
    sizeof(RwV3d),            /* normal */
    sizeof(RwV2d),            /* size */
    sizeof(RwRGBA),           /* color */
    sizeof(RwRGBA) * 4,       /* vertex color */
    sizeof(RwReal),           /* 2D rotate */
    sizeof(RwTexCoords) * 2,  /* 2 texture coordinates */
    sizeof(RwTexCoords) * 4   /* 4 texture coordinates */
};

static void* PTankAtomicInit(void* object, RwInt32 offset, RwInt32 size)
{
    RPATOMICPTANKPLUGINDATA(object) = (RpPTankAtomicExtPrv*)NULL;

    return object;
}

static void* PTankAtomicDestruct(void* object, RwInt32 offset, RwInt32 size)
{
    RpAtomic* atomic = (RpAtomic*)object;

    if (RPATOMICPTANKPLUGINDATA(atomic))
    {
        RpMaterialSetTexture(atomic->geometry->matList.materials[0], (RwTexture*)NULL);

        if (RPATOMICPTANKPLUGINDATA(atomic)->rawdata)
        {
            RwFree(RPATOMICPTANKPLUGINDATA(atomic)->rawdata);
        }

        RwFree(RPATOMICPTANKPLUGINDATA(atomic));
        RPATOMICPTANKPLUGINDATA(atomic) = (RpPTankAtomicExtPrv*)NULL;
    }

    return object;
}

RwBool RpPTankPluginAttach(void)
{
    _rpPTankGlobalsOffset =
        RwEngineRegisterPlugin(sizeof(void*), rwID_PTANKPLUGIN, PTankOpen, PTankClose);
    if (_rpPTankGlobalsOffset >= 0)
    {
        _rpPTankAtomicDataOffset =
            RpAtomicRegisterPlugin(sizeof(RpPTankAtomicExtPrv*), rwID_PTANKPLUGIN,
                                   PTankAtomicInit, PTankAtomicDestruct, (RwPluginObjectCopy)NULL);

        return _rpPTankAtomicDataOffset >= 0;
    }

    return FALSE;
}

/* Data allocated as an array of structures, one array per cluster */
static RwBool rpPTankAStructAlloc(RpPTankAtomicExtPrv* ptankPrv, RwUInt32 dataFlags,
                                  RwUInt32 platFlags)
{
    RwBool hasPosition = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGPOSITION);
    RwBool success = FALSE;
    RwBool hasColor = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGCOLOR);
    RwBool hasSize = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGSIZE);
    RwBool hasMatrix = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGMATRIX);
    RwBool hasNormal = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGNORMAL);
    RwBool has2DRotate = PTANKFLAGISSET(dataFlags, rpPTANKDFLAG2DROTATE);
    RwBool hasVtxColor = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTXCOLOR);
    RwBool hasVtx2TexCoords = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTX2TEXCOORDS);
    RwBool hasVtx4TexCoords = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTX4TEXCOORDS);
    RwInt32 size = 0;
    RwInt32 offset;
    void* rawdata;
    RwUInt8* data;
    RpPTankData* publicData = &ptankPrv->publicData;

    publicData->format.dataFlags &= ~0x1FF;
    publicData->format.numClusters = 0;
    ptankPrv->lockFlags = 0;

    if (hasPosition == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAGPOSITION;
        size += datasize[RPPTANKSIZEPOSITION] * ptankPrv->maxPCount;
    }

    if (hasMatrix == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAGMATRIX;
        size += datasize[RPPTANKSIZEMATRIX] * ptankPrv->maxPCount;
    }

    if (hasNormal == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAGNORMAL;
        size += datasize[RPPTANKSIZENORMAL] * ptankPrv->maxPCount;
    }

    if (hasSize == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAGSIZE;
        size += datasize[RPPTANKSIZESIZE] * ptankPrv->maxPCount;
    }

    if (hasColor == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAGCOLOR;
        size += datasize[RPPTANKSIZECOLOR] * ptankPrv->maxPCount;
    }

    if (hasVtxColor == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAGVTXCOLOR;
        size += datasize[RPPTANKSIZEVTXCOLOR] * ptankPrv->maxPCount;
    }

    if (has2DRotate == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAG2DROTATE;
        size += datasize[RPPTANKSIZE2DROTATE] * ptankPrv->maxPCount;
    }

    if (hasVtx2TexCoords == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAGVTX2TEXCOORDS;
        size += datasize[RPPTANKSIZEVTX2TEXCOORDS] * ptankPrv->maxPCount;
    }

    if (hasVtx4TexCoords == TRUE)
    {
        publicData->format.numClusters++;
        publicData->format.dataFlags |= rpPTANKDFLAGVTX4TEXCOORDS;
        size += datasize[RPPTANKSIZEVTX4TEXCOORDS] * ptankPrv->maxPCount;
    }

    rawdata = RwMalloc(size + 16);
    if (rawdata)
    {
        data = (RwUInt8*)(((RwUInt32)rawdata + 15) & ~15);
        offset = 0;

        if (hasPosition == TRUE)
        {
            publicData->clusters[RPPTANKSIZEPOSITION].data = data + offset;
            publicData->clusters[RPPTANKSIZEPOSITION].stride = sizeof(RwV3d);
            offset += datasize[RPPTANKSIZEPOSITION] * ptankPrv->maxPCount;
        }

        if (hasMatrix == TRUE)
        {
            publicData->clusters[RPPTANKSIZEMATRIX].data = data + offset;
            publicData->clusters[RPPTANKSIZEMATRIX].stride = sizeof(RwMatrix);
            offset += datasize[RPPTANKSIZEMATRIX] * ptankPrv->maxPCount;
        }

        if (hasNormal == TRUE)
        {
            publicData->clusters[RPPTANKSIZENORMAL].data = data + offset;
            publicData->clusters[RPPTANKSIZENORMAL].stride = sizeof(RwV3d);
            offset += datasize[RPPTANKSIZENORMAL] * ptankPrv->maxPCount;
        }

        if (hasSize == TRUE)
        {
            publicData->clusters[RPPTANKSIZESIZE].data = data + offset;
            publicData->clusters[RPPTANKSIZESIZE].stride = sizeof(RwV2d);
            offset += datasize[RPPTANKSIZESIZE] * ptankPrv->maxPCount;
        }

        if (hasColor == TRUE)
        {
            publicData->clusters[RPPTANKSIZECOLOR].data = data + offset;
            publicData->clusters[RPPTANKSIZECOLOR].stride = sizeof(RwRGBA);
            offset += datasize[RPPTANKSIZECOLOR] * ptankPrv->maxPCount;
        }

        if (hasVtxColor == TRUE)
        {
            publicData->clusters[RPPTANKSIZEVTXCOLOR].data = data + offset;
            publicData->clusters[RPPTANKSIZEVTXCOLOR].stride = sizeof(RwRGBA) * 4;
            offset += datasize[RPPTANKSIZEVTXCOLOR] * ptankPrv->maxPCount;
        }

        if (has2DRotate == TRUE)
        {
            publicData->clusters[RPPTANKSIZE2DROTATE].data = data + offset;
            publicData->clusters[RPPTANKSIZE2DROTATE].stride = sizeof(RwReal);
            offset += datasize[RPPTANKSIZE2DROTATE] * ptankPrv->maxPCount;
        }

        if (hasVtx2TexCoords == TRUE)
        {
            publicData->clusters[RPPTANKSIZEVTX2TEXCOORDS].data = data + offset;
            publicData->clusters[RPPTANKSIZEVTX2TEXCOORDS].stride = sizeof(RwTexCoords) * 2;
            offset += datasize[RPPTANKSIZEVTX2TEXCOORDS] * ptankPrv->maxPCount;
        }

        if (hasVtx4TexCoords == TRUE)
        {
            publicData->clusters[RPPTANKSIZEVTX4TEXCOORDS].data = data + offset;
            publicData->clusters[RPPTANKSIZEVTX4TEXCOORDS].stride = sizeof(RwTexCoords) * 4;
        }

        publicData->format.stride = 0;
        ptankPrv->rawdata = rawdata;

        success = TRUE;
    }

    return success;
}

/* Data allocated as a single array of structures, one structure per particle */
static RwBool rpPTankSStructAlloc(RpPTankAtomicExtPrv* ptankPrv, RwUInt32 dataFlags,
                                  RwUInt32 platFlags)
{
    RwBool hasPosition = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGPOSITION);
    RwBool success = FALSE;
    RwBool hasColor = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGCOLOR);
    RwBool hasSize = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGSIZE);
    RwBool hasMatrix = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGMATRIX);
    RwBool hasNormal = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGNORMAL);
    RwBool has2DRotate = PTANKFLAGISSET(dataFlags, rpPTANKDFLAG2DROTATE);
    RwBool hasVtxColor = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTXCOLOR);
    RwBool hasVtx2TexCoords = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTX2TEXCOORDS);
    RwBool hasVtx4TexCoords = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTX4TEXCOORDS);
    RwInt32 stride = 0;
    RwInt32 offset;
    void* rawdata;
    RwUInt8* data;
    RpPTankData* publicData = &ptankPrv->publicData;

    publicData->format.dataFlags &= ~0x1FF;
    publicData->format.numClusters = 0;

    if (hasPosition == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZEPOSITION];
        publicData->format.dataFlags |= rpPTANKDFLAGPOSITION;
    }

    if (hasMatrix == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZEMATRIX];
        publicData->format.dataFlags |= rpPTANKDFLAGMATRIX;
    }

    if (hasNormal == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZENORMAL];
        publicData->format.dataFlags |= rpPTANKDFLAGNORMAL;
    }

    if (hasSize == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZESIZE];
        publicData->format.dataFlags |= rpPTANKDFLAGSIZE;
    }

    if (hasColor == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZECOLOR];
        publicData->format.dataFlags |= rpPTANKDFLAGCOLOR;
    }

    if (hasVtxColor == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZEVTXCOLOR];
        publicData->format.dataFlags |= rpPTANKDFLAGVTXCOLOR;
    }

    if (has2DRotate == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZE2DROTATE];
        publicData->format.dataFlags |= rpPTANKDFLAG2DROTATE;
    }

    if (hasVtx2TexCoords == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZEVTX2TEXCOORDS];
        publicData->format.dataFlags |= rpPTANKDFLAGVTX2TEXCOORDS;
    }

    if (hasVtx4TexCoords == TRUE)
    {
        publicData->format.numClusters++;
        stride += datasize[RPPTANKSIZEVTX4TEXCOORDS];
        publicData->format.dataFlags |= rpPTANKDFLAGVTX4TEXCOORDS;
    }

    /* Keep the matrices word aligned */
    if (hasMatrix)
    {
        stride = (stride + 3) & ~3;
    }

    publicData->format.stride = stride;

    rawdata = RwMalloc(stride * ptankPrv->maxPCount + 16);
    if (rawdata)
    {
        data = (RwUInt8*)(((RwUInt32)rawdata + 15) & ~15);
        offset = 0;

        if (hasPosition == TRUE)
        {
            publicData->clusters[RPPTANKSIZEPOSITION].data = data + offset;
            publicData->clusters[RPPTANKSIZEPOSITION].stride = publicData->format.stride;
            offset += datasize[RPPTANKSIZEPOSITION];
        }

        if (hasMatrix == TRUE)
        {
            publicData->clusters[RPPTANKSIZEMATRIX].data = data + offset;
            publicData->clusters[RPPTANKSIZEMATRIX].stride = publicData->format.stride;
            offset += datasize[RPPTANKSIZEMATRIX];
        }

        if (hasNormal == TRUE)
        {
            publicData->clusters[RPPTANKSIZENORMAL].data = data + offset;
            publicData->clusters[RPPTANKSIZENORMAL].stride = publicData->format.stride;
            offset += datasize[RPPTANKSIZENORMAL];
        }

        if (hasSize == TRUE)
        {
            publicData->clusters[RPPTANKSIZESIZE].data = data + offset;
            publicData->clusters[RPPTANKSIZESIZE].stride = publicData->format.stride;
            offset += datasize[RPPTANKSIZESIZE];
        }

        if (hasColor == TRUE)
        {
            publicData->clusters[RPPTANKSIZECOLOR].data = data + offset;
            publicData->clusters[RPPTANKSIZECOLOR].stride = publicData->format.stride;
            offset += datasize[RPPTANKSIZECOLOR];
        }

        if (hasVtxColor == TRUE)
        {
            publicData->clusters[RPPTANKSIZEVTXCOLOR].data = data + offset;
            publicData->clusters[RPPTANKSIZEVTXCOLOR].stride = publicData->format.stride;
            offset += datasize[RPPTANKSIZEVTXCOLOR];
        }

        if (has2DRotate == TRUE)
        {
            publicData->clusters[RPPTANKSIZE2DROTATE].data = data + offset;
            publicData->clusters[RPPTANKSIZE2DROTATE].stride = publicData->format.stride;
            offset += datasize[RPPTANKSIZE2DROTATE];
        }

        if (hasVtx2TexCoords == TRUE)
        {
            publicData->clusters[RPPTANKSIZEVTX2TEXCOORDS].data = data + offset;
            publicData->clusters[RPPTANKSIZEVTX2TEXCOORDS].stride = publicData->format.stride;
            offset += datasize[RPPTANKSIZEVTX2TEXCOORDS];
        }

        if (hasVtx4TexCoords == TRUE)
        {
            publicData->clusters[RPPTANKSIZEVTX4TEXCOORDS].data = data + offset;
            publicData->clusters[RPPTANKSIZEVTX4TEXCOORDS].stride = publicData->format.stride;
        }

        ptankPrv->rawdata = rawdata;

        success = TRUE;
    }

    return success;
}

static RwUInt32 rpPTankValidateFlag(RwUInt32 dataFlags)
{
    RwBool hasPosition = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGPOSITION);
    RwBool hasSize = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGSIZE);
    RwBool hasMatrix = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGMATRIX);
    RwBool has2DRotate = PTANKFLAGISSET(dataFlags, rpPTANKDFLAG2DROTATE);
    RwBool hasVtxColor = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTXCOLOR);
    RwBool hasVtx2TexCoords = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTX2TEXCOORDS);
    RwBool hasVtx4TexCoords = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGVTX4TEXCOORDS);
    RwBool hasCnsMatrix = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGCNSMATRIX);
    RwBool hasCns2DRotate = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGCNS2DROTATE);
    RwBool hasCnsVtxColor = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGCNSVTXCOLOR);
    RwBool hasCnsVtx2TexCoords = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGCNSVTX2TEXCOORDS);
    RwBool hasCnsVtx4TexCoords = PTANKFLAGISSET(dataFlags, rpPTANKDFLAGCNSVTX4TEXCOORDS);

    /* A per particle value overrides the constant one */
    if (hasMatrix == TRUE && hasCnsMatrix == TRUE)
    {
        dataFlags &= rpPTANKDFLAGCNSMATRIX;
    }

    if (has2DRotate == TRUE && hasCns2DRotate == TRUE)
    {
        dataFlags &= rpPTANKDFLAGCNS2DROTATE;
    }

    if (hasVtxColor == TRUE && hasCnsVtxColor == TRUE)
    {
        dataFlags &= rpPTANKDFLAGCNSVTXCOLOR;
    }

    if (hasVtx2TexCoords == TRUE && hasCnsVtx2TexCoords == TRUE)
    {
        dataFlags &= rpPTANKDFLAGCNSVTX2TEXCOORDS;
    }

    if (hasVtx4TexCoords == TRUE && hasCnsVtx4TexCoords == TRUE)
    {
        dataFlags &= rpPTANKDFLAGCNSVTX4TEXCOORDS;
    }

    /* A matrix already holds the position, size and rotation */
    if (hasMatrix == TRUE && hasPosition == TRUE)
    {
        dataFlags &= ~rpPTANKDFLAGPOSITION;
    }

    if (hasMatrix == TRUE && hasSize == TRUE)
    {
        dataFlags &= ~rpPTANKDFLAGSIZE;
    }

    if (hasMatrix == TRUE && has2DRotate == TRUE)
    {
        dataFlags &= ~rpPTANKDFLAG2DROTATE;
    }

    /* Particles need at least a position */
    if (hasMatrix == FALSE && hasPosition == FALSE)
    {
        dataFlags |= rpPTANKDFLAGPOSITION;
    }

    if (!(dataFlags & (rpPTANKDFLAGARRAY | rpPTANKDFLAGSTRUCTURE)))
    {
        dataFlags |= rpPTANKDFLAGSTRUCTURE;
    }

    return dataFlags;
}

static RpAtomic* rpPTankAtomicRenderCB(RpAtomic* atomic)
{
    RwBool success = TRUE;
    RpPTankAtomicExtPrv* ptankPrv = RPATOMICPTANKPLUGINDATA(atomic);

    if (ptankPrv->actPCount > 0)
    {
        if (ptankPrv->ptankCallBacks.instance)
        {
            ptankPrv->ptankCallBacks.instance(atomic, &ptankPrv->publicData, ptankPrv->actPCount,
                                              ptankPrv->instFlags);
            ptankPrv->instFlags = 0;
        }

        if (ptankPrv->ptankCallBacks.render)
        {
            success =
                ptankPrv->ptankCallBacks.render(atomic, &ptankPrv->publicData, ptankPrv->actPCount);
        }

        if (success == TRUE)
        {
            ptankPrv->defaultRenderCB(atomic);
        }
    }

    return atomic;
}

RpAtomic* RpPTankAtomicCreate(RwInt32 maxParticleNum, RwUInt32 dataFlags, RwUInt32 platFlags)
{
    return _rpPTankAtomicCreateCustom(maxParticleNum, rpPTankValidateFlag(dataFlags), platFlags,
                                      &defaultCB);
}

RpAtomic* _rpPTankAtomicCreateCustom(RwInt32 maxParticleNum, RwUInt32 dataFlags, RwUInt32 platFlags,
                                     RpPTankCallBacks* callbacks)
{
    RwBool success = FALSE;
    RpAtomic* atomic;
    RpPTankAtomicExtPrv* ptankPrv;

    atomic = RpAtomicCreate();
    if (atomic)
    {
        ptankPrv = (RpPTankAtomicExtPrv*)RwMalloc(sizeof(RpPTankAtomicExtPrv));
        if (ptankPrv)
        {
            ptankPrv->defaultRenderCB = atomic->renderCallBack;
            RpAtomicSetRenderCallBack(atomic, rpPTankAtomicRenderCB);

            ptankPrv->maxPCount = maxParticleNum;
            ptankPrv->actPCount = 0;
            ptankPrv->publicData.format.dataFlags = dataFlags;
            ptankPrv->platFlags = platFlags;
            ptankPrv->instFlags = rpPTANKIFLAGALL;
            ptankPrv->lockFlags = 0;
            ptankPrv->rawdata = NULL;

            RPATOMICPTANKPLUGINDATA(atomic) = ptankPrv;

            memcpy(&ptankPrv->ptankCallBacks, callbacks, sizeof(RpPTankCallBacks));

            if (ptankPrv->ptankCallBacks.alloc)
            {
                ptankPrv->rawdata = ptankPrv->ptankCallBacks.alloc(
                    &ptankPrv->publicData, ptankPrv->maxPCount, dataFlags, platFlags);
            }
            else if ((dataFlags & rpPTANKDFLAGSTRUCTURE) == rpPTANKDFLAGSTRUCTURE)
            {
                success = rpPTankSStructAlloc(ptankPrv, dataFlags, platFlags);
            }
            else
            {
                success = rpPTankAStructAlloc(ptankPrv, dataFlags, platFlags);
            }

            if (success == TRUE || ptankPrv->rawdata)
            {
                if (ptankPrv->ptankCallBacks.create(atomic, &ptankPrv->publicData,
                                                    ptankPrv->maxPCount, dataFlags,
                                                    platFlags) == TRUE)
                {
                    RwRGBA color = { 0, 0, 0, 255 };
                    RwV2d size = { 1.0f, 1.0f };
                    RwV2d center = { 0.0f, 0.0f };
                    RwTexCoords uv[4] = { { 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f } };

                    RpMaterialSetTexture(atomic->geometry->matList.materials[0], (RwTexture*)NULL);

                    ptankPrv->publicData.cSize = size;
                    ptankPrv->publicData.cRotate = 0.0f;
                    ptankPrv->publicData.cCenter = center;

                    RwMatrixSetIdentityMacro(&ptankPrv->publicData.cMatrix);

                    ptankPrv->publicData.cColor = color;
                    ptankPrv->publicData.cVtxColor[0] = color;
                    ptankPrv->publicData.cVtxColor[1] = color;
                    ptankPrv->publicData.cVtxColor[2] = color;
                    ptankPrv->publicData.cVtxColor[3] = color;

                    ptankPrv->publicData.cUV[0] = uv[0];
                    ptankPrv->publicData.cUV[1] = uv[1];
                    ptankPrv->publicData.cUV[2] = uv[2];
                    ptankPrv->publicData.cUV[3] = uv[3];

                    RwRenderStateGet(rwRENDERSTATESRCBLEND, &ptankPrv->publicData.srcBlend);
                    RwRenderStateGet(rwRENDERSTATEDESTBLEND, &ptankPrv->publicData.dstBlend);
                    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE,
                                     &ptankPrv->publicData.vertexAlphaBlend);

                    return atomic;
                }
            }
        }
    }

    RpAtomicDestroy(atomic);

    return (RpAtomic*)NULL;
}

void RpPTankAtomicDestroy(RpAtomic* ptank)
{
    RpAtomicDestroy(ptank);
}

RwBool RpPTankAtomicLock(RpAtomic* atomic, RpPTankLockStruct* dst, RwUInt32 dataFlags,
                         RpPTankLockFlags lockFlag)
{
    RpPTankAtomicExtPrv* ptankPrv = RPATOMICPTANKPLUGINDATA(atomic);

    if (!(ptankPrv->publicData.format.dataFlags & dataFlags))
    {
        dst->data = (RwUInt8*)NULL;
        dst->stride = 0;

        return FALSE;
    }

    switch (dataFlags)
    {
    case rpPTANKLFLAGPOSITION:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZEPOSITION];
        break;
    case rpPTANKLFLAGCOLOR:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZECOLOR];
        break;
    case rpPTANKLFLAGSIZE:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZESIZE];
        break;
    case rpPTANKLFLAGMATRIX:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZEMATRIX];
        break;
    case rpPTANKLFLAGNORMAL:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZENORMAL];
        break;
    case rpPTANKLFLAG2DROTATE:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZE2DROTATE];
        break;
    case rpPTANKLFLAGVTXCOLOR:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZEVTXCOLOR];
        break;
    case rpPTANKLFLAGVTX2TEXCOORDS:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZEVTX2TEXCOORDS];
        break;
    case rpPTANKLFLAGVTX4TEXCOORDS:
        *dst = ptankPrv->publicData.clusters[RPPTANKSIZEVTX4TEXCOORDS];
        break;
    }

    if (ptankPrv->publicData.format.stride != 0)
    {
        dst->stride = ptankPrv->publicData.format.stride;
    }

    if (dst->data && (lockFlag & rpPTANKLOCKWRITE) == rpPTANKLOCKWRITE)
    {
        RPATOMICPTANKPLUGINDATA(atomic)->lockFlags |= lockFlag | dataFlags;
    }

    return TRUE;
}

RpAtomic* RpPTankAtomicUnlock(RpAtomic* atomic)
{
    RpPTankAtomicExtPrv* ptankPrv = RPATOMICPTANKPLUGINDATA(atomic);

    ptankPrv->instFlags |= ptankPrv->lockFlags;
    ptankPrv->lockFlags = 0;

    return atomic;
}
