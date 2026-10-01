#include <rwsdk/rwcore.h>

#define rwPLUGIN_ID 1

#define RWERROR(errorArgs)                                                                         \
    MACRO_START                                                                                    \
    {                                                                                              \
        RwError _rwErrorCode;                                                                      \
        _rwErrorCode.pluginID = rwPLUGIN_ID;                                                       \
        _rwErrorCode.errorCode = _rwerror errorArgs;                                               \
        RwErrorSet(&_rwErrorCode);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define E_RW_NULLP 0x80000016
#define E_RW_PLUGINNOTINIT 0x80000018

enum RxRenderStateFlag
{
    rxRENDERSTATEFLAG_TEXTUREPERSPECTIVE = 0x00000001,
    rxRENDERSTATEFLAG_ZTESTENABLE = 0x00000002,
    rxRENDERSTATEFLAG_ZWRITEENABLE = 0x00000004,
    rxRENDERSTATEFLAG_VERTEXALPHAENABLE = 0x00000008,
    rxRENDERSTATEFLAG_FOGENABLE = 0x00000010,
    rxRENDERSTATEFLAG_ALPHAPRIMITIVEBUFFER = 0x00000020,
    rxRENDERSTATEFLAGFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};
typedef enum RxRenderStateFlag RxRenderStateFlag;

typedef struct rwPipeGlobals rwPipeGlobals;
struct rwPipeGlobals
{
    RwFreeList* pipesFreeList;
    RxRenderStateVector defaultRenderState;
    RwLinkList allPipelines;
    RwUInt32 maxNodesPerPipe;
};

extern RwInt32 _rxPipelineGlobalsOffset;

#define RXPIPELINEGLOBAL(var)                                                                      \
    (RWPLUGINOFFSET(rwPipeGlobals, RwEngineInstance, _rxPipelineGlobalsOffset)->var)

RxRenderStateVector* RxRenderStateVectorSetDefaultRenderStateVector(RxRenderStateVector* rsvp)
{
    if (rsvp != NULL)
    {
        if (RWSRCGLOBAL(engineStatus) == rwENGINESTATUSSTARTED)
        {
            *rsvp = RXPIPELINEGLOBAL(defaultRenderState);
        }
        else if (rsvp != &RXPIPELINEGLOBAL(defaultRenderState))
        {
            RWERROR((E_RW_PLUGINNOTINIT));
            return NULL;
        }
        else
        {
            RwRGBA OpaqueWhite = { 255, 255, 255, 255 };

            rsvp->Flags = rxRENDERSTATEFLAG_TEXTUREPERSPECTIVE | rxRENDERSTATEFLAG_ZTESTENABLE |
                          rxRENDERSTATEFLAG_ZWRITEENABLE;
            rsvp->ShadeMode = rwSHADEMODEGOURAUD;
            rsvp->SrcBlend = rwBLENDSRCALPHA;
            rsvp->DestBlend = rwBLENDINVSRCALPHA;
            rsvp->TextureRaster = NULL;
            rsvp->AddressModeU = rwTEXTUREADDRESSWRAP;
            rsvp->AddressModeV = rwTEXTUREADDRESSWRAP;
            rsvp->FilterMode = rwFILTERLINEAR;
            rsvp->BorderColor = OpaqueWhite;
            rsvp->FogType = rwFOGTYPENAFOGTYPE;
            rsvp->FogColor = OpaqueWhite;
        }

        return rsvp;
    }

    RWERROR((E_RW_NULLP));
    return NULL;
}

RxRenderStateVector* RxRenderStateVectorLoadDriverState(RxRenderStateVector* rsvp)
{
    RwInt32 flag;

    if (rsvp != NULL)
    {
        rsvp->Flags = 0;

        RwRenderStateGet(rwRENDERSTATETEXTUREPERSPECTIVE, (void*)&flag);
        if (flag)
        {
            rsvp->Flags |= rxRENDERSTATEFLAG_TEXTUREPERSPECTIVE;
        }

        RwRenderStateGet(rwRENDERSTATEZTESTENABLE, (void*)&flag);
        if (flag)
        {
            rsvp->Flags |= rxRENDERSTATEFLAG_ZTESTENABLE;
        }

        RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, (void*)&flag);
        if (flag)
        {
            rsvp->Flags |= rxRENDERSTATEFLAG_ZWRITEENABLE;
        }

        RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)&flag);
        if (flag)
        {
            rsvp->Flags |= rxRENDERSTATEFLAG_VERTEXALPHAENABLE;
        }

        RwRenderStateGet(rwRENDERSTATESHADEMODE, (void*)&rsvp->ShadeMode);
        RwRenderStateGet(rwRENDERSTATESRCBLEND, (void*)&rsvp->SrcBlend);
        RwRenderStateGet(rwRENDERSTATEDESTBLEND, (void*)&rsvp->DestBlend);
        RwRenderStateGet(rwRENDERSTATETEXTURERASTER, (void*)&rsvp->TextureRaster);

        if (RwRenderStateGet(rwRENDERSTATETEXTUREADDRESS, (void*)&rsvp->AddressModeU))
        {
            rsvp->AddressModeV = rsvp->AddressModeU;
        }
        else
        {
            RwRenderStateGet(rwRENDERSTATETEXTUREADDRESSU, (void*)&rsvp->AddressModeU);
            RwRenderStateGet(rwRENDERSTATETEXTUREADDRESSV, (void*)&rsvp->AddressModeV);
        }

        RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, (void*)&rsvp->FilterMode);
        RwRenderStateGet(rwRENDERSTATEBORDERCOLOR, (void*)&rsvp->BorderColor);
        RwRenderStateGet(rwRENDERSTATEFOGTYPE, (void*)&rsvp->FogType);
        RwRenderStateGet(rwRENDERSTATEFOGCOLOR, (void*)&rsvp->FogColor);

        return rsvp;
    }

    RWERROR((E_RW_NULLP));
    return NULL;
}
