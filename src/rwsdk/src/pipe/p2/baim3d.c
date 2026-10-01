#include <rwsdk/rwcore.h>
#include <string.h>

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

#define E_RW_IM3DNOTACTIVE 0x23
#define E_RW_INVALIDPRIMTYPE 0x25
#define E_RW_TOOMANYVERTICES 0x32

#define RWIM3DMAXVERTICES 65536

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct RxCamSpace3DVertex RxCamSpace3DVertex;
typedef struct RxMeshStateVector RxMeshStateVector;

typedef struct rwIm3DRenderPipelines rwIm3DRenderPipelines;
struct rwIm3DRenderPipelines
{
    RxPipeline* triList;
    RxPipeline* triFan;
    RxPipeline* triStrip;
    RxPipeline* lineList;
    RxPipeline* polyLine;
    RxPipeline* pointList;
};

typedef struct _rwIm3DPoolStash _rwIm3DPoolStash;
struct _rwIm3DPoolStash
{
    RwUInt32 flags;
    RwMatrix* ltm;
    RwUInt32 numVerts;
    RxObjSpace3DVertex* objVerts;
    RxCamSpace3DVertex* camVerts;
    void* devVerts;
    RxMeshStateVector* meshState;
    RxRenderStateVector* renderState;
    RxPipeline* pipeline;
    RwPrimitiveType primType;
    RwImVertexIndex* indices;
    RwUInt32 numIndices;
};

typedef struct rwIm3DPool rwIm3DPool;
struct rwIm3DPool
{
    RwUInt16 numElements;
    RwUInt16 pad;
    void* elements;
    RwInt32 stride;
    _rwIm3DPoolStash stash;
};

typedef struct rwImmediGlobals rwImmediGlobals;
struct rwImmediGlobals
{
    RxPipeline* im3DTransformPipeline;
    rwIm3DRenderPipelines im3DRenderPipelines;
    RxPipeline* platformIm3DTransformPipeline;
    rwIm3DRenderPipelines platformIm3DRenderPipelines;
    rwIm3DPool curPool;
};

extern RwBool _rwIm3DCreatePlatformTransformPipeline(RxPipeline** globalPipe);
extern void _rwIm3DDestroyPlatformTransformPipeline(RxPipeline** globalPipe);
extern RwBool _rwIm3DCreatePlatformRenderPipelines(rwIm3DRenderPipelines* globalPipes);
extern void _rwIm3DDestroyPlatformRenderPipelines(rwIm3DRenderPipelines* globalPipes);

#define RWIMMEDIGLOBAL(var)                                                                        \
    (RWPLUGINOFFSET(rwImmediGlobals, RwEngineInstance, _rwIm3DModule.globalsOffset)->var)

RwModuleInfo _rwIm3DModule;
rwImmediGlobals* _rwIm3DGlobals;

void* RwIm3DTransform(RwIm3DVertex* pVerts, RwUInt32 numVerts, RwMatrix* ltm, RwUInt32 flags)
{
    RxPipeline* result;

    if (numVerts > RWIM3DMAXVERTICES)
    {
        RWERROR((E_RW_TOOMANYVERTICES, numVerts));
    }
    else
    {
        RWIMMEDIGLOBAL(curPool).numElements = (RwUInt16)numVerts;
        RWIMMEDIGLOBAL(curPool).elements = pVerts;
        RWIMMEDIGLOBAL(curPool).stride = sizeof(RwIm3DVertex);
        RWIMMEDIGLOBAL(curPool).stash.ltm = ltm;
        RWIMMEDIGLOBAL(curPool).stash.flags = flags | rwIM3D_VERTEXXYZ | rwIM3D_VERTEXRGBA;

        result = RxPipelineExecute(RWIMMEDIGLOBAL(im3DTransformPipeline),
                                   (void*)&RWIMMEDIGLOBAL(curPool), TRUE);
        if (result != NULL)
        {
            return pVerts;
        }
    }

    return NULL;
}

RwBool RwIm3DEnd(void)
{
    RwBool im3dactive;

    im3dactive = (RWIMMEDIGLOBAL(curPool).elements != NULL);
    if (!im3dactive)
    {
        return FALSE;
    }

    memset(&RWIMMEDIGLOBAL(curPool), 0, sizeof(rwIm3DPool));

    return TRUE;
}

RwBool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primType, RwImVertexIndex* indices,
                                    RwInt32 numIndices)
{
    RwBool im3dactive;
    _rwIm3DPoolStash* stash;

    im3dactive = (RWIMMEDIGLOBAL(curPool).elements != NULL);
    if (im3dactive)
    {
        stash = &RWIMMEDIGLOBAL(curPool).stash;

        stash->pipeline = NULL;
        stash->primType = primType;
        stash->indices = indices;
        stash->numIndices = numIndices;

        switch (primType)
        {
        case rwPRIMTYPETRILIST:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).triList;
            stash->numIndices = numIndices - numIndices % 3;
            break;
        case rwPRIMTYPETRIFAN:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).triStrip;
            break;
        case rwPRIMTYPELINELIST:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).lineList;
            stash->numIndices = numIndices - numIndices % 2;
            break;
        case rwPRIMTYPEPOLYLINE:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).polyLine;
            break;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE, primType));
            break;
        }

        if (RxPipelineExecute(stash->pipeline, (void*)stash, FALSE) != NULL)
        {
            return TRUE;
        }
    }
    else
    {
        RWERROR((E_RW_IM3DNOTACTIVE));
    }

    return FALSE;
}

RwBool RwIm3DRenderPrimitive(RwPrimitiveType primType)
{
    RwBool im3dactive;
    RxHeap* heap;
    _rwIm3DPoolStash* stash;

    im3dactive = (RWIMMEDIGLOBAL(curPool).elements != NULL);
    heap = RxHeapGetGlobalHeap();

    if (im3dactive)
    {
        stash = &RWIMMEDIGLOBAL(curPool).stash;

        stash->pipeline = NULL;
        stash->primType = primType;
        stash->indices = NULL;
        stash->numIndices = RWIMMEDIGLOBAL(curPool).numElements;

        switch (primType)
        {
        case rwPRIMTYPETRILIST:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).triList;
            break;
        case rwPRIMTYPETRIFAN:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).triFan;
            break;
        case rwPRIMTYPETRISTRIP:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).triStrip;
            break;
        case rwPRIMTYPELINELIST:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).lineList;
            break;
        case rwPRIMTYPEPOLYLINE:
            stash->pipeline = RWIMMEDIGLOBAL(im3DRenderPipelines).polyLine;
            break;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE, primType));
            break;
        }

        if (RxPipelineExecute(stash->pipeline, (void*)stash, FALSE) != NULL)
        {
            return TRUE;
        }
    }
    else
    {
        RWERROR((E_RW_IM3DNOTACTIVE));
    }

    return FALSE;
}

RxPipeline* RwIm3DSetTransformPipeline(RxPipeline* pipeline)
{
    if (pipeline != NULL)
    {
        RWIMMEDIGLOBAL(im3DTransformPipeline) = pipeline;
    }
    else if (RWIMMEDIGLOBAL(platformIm3DTransformPipeline) != NULL)
    {
        RWIMMEDIGLOBAL(im3DTransformPipeline) = RWIMMEDIGLOBAL(platformIm3DTransformPipeline);
    }
    else
    {
        RWIMMEDIGLOBAL(im3DTransformPipeline) = NULL;
    }

    return RWIMMEDIGLOBAL(im3DTransformPipeline);
}

RxPipeline* RwIm3DSetRenderPipeline(RxPipeline* pipeline, RwPrimitiveType primType)
{
    if (pipeline != NULL)
    {
        switch (primType)
        {
        case rwPRIMTYPETRILIST:
            RWIMMEDIGLOBAL(im3DRenderPipelines).triList = pipeline;
            return pipeline;
        case rwPRIMTYPETRIFAN:
            RWIMMEDIGLOBAL(im3DRenderPipelines).triFan = pipeline;
            return pipeline;
        case rwPRIMTYPETRISTRIP:
            RWIMMEDIGLOBAL(im3DRenderPipelines).triStrip = pipeline;
            return pipeline;
        case rwPRIMTYPELINELIST:
            RWIMMEDIGLOBAL(im3DRenderPipelines).lineList = pipeline;
            return pipeline;
        case rwPRIMTYPEPOLYLINE:
            RWIMMEDIGLOBAL(im3DRenderPipelines).polyLine = pipeline;
            return pipeline;
        case rwPRIMTYPEPOINTLIST:
            RWIMMEDIGLOBAL(im3DRenderPipelines).pointList = pipeline;
            return pipeline;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE, primType));
            break;
        }
    }
    else
    {
        switch (primType)
        {
        case rwPRIMTYPETRILIST:
            if (RWIMMEDIGLOBAL(platformIm3DRenderPipelines).triList != NULL)
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).triList =
                    RWIMMEDIGLOBAL(platformIm3DRenderPipelines).triList;
            }
            else
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).triList = NULL;
            }
            return RWIMMEDIGLOBAL(im3DRenderPipelines).triList;
        case rwPRIMTYPETRIFAN:
            if (RWIMMEDIGLOBAL(platformIm3DRenderPipelines).triFan != NULL)
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).triFan =
                    RWIMMEDIGLOBAL(platformIm3DRenderPipelines).triFan;
            }
            else
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).triFan = NULL;
            }
            return RWIMMEDIGLOBAL(im3DRenderPipelines).triFan;
        case rwPRIMTYPETRISTRIP:
            if (RWIMMEDIGLOBAL(platformIm3DRenderPipelines).triStrip != NULL)
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).triStrip =
                    RWIMMEDIGLOBAL(platformIm3DRenderPipelines).triStrip;
            }
            else
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).triStrip = NULL;
            }
            return RWIMMEDIGLOBAL(platformIm3DRenderPipelines).triStrip;
        case rwPRIMTYPELINELIST:
            if (RWIMMEDIGLOBAL(platformIm3DRenderPipelines).lineList != NULL)
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).lineList =
                    RWIMMEDIGLOBAL(platformIm3DRenderPipelines).lineList;
            }
            else
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).lineList = NULL;
            }
            return RWIMMEDIGLOBAL(platformIm3DRenderPipelines).lineList;
        case rwPRIMTYPEPOLYLINE:
            if (RWIMMEDIGLOBAL(platformIm3DRenderPipelines).polyLine != NULL)
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).polyLine =
                    RWIMMEDIGLOBAL(platformIm3DRenderPipelines).polyLine;
            }
            else
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).polyLine = NULL;
            }
            return RWIMMEDIGLOBAL(platformIm3DRenderPipelines).polyLine;
        case rwPRIMTYPEPOINTLIST:
            if (RWIMMEDIGLOBAL(platformIm3DRenderPipelines).pointList != NULL)
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).pointList =
                    RWIMMEDIGLOBAL(platformIm3DRenderPipelines).pointList;
            }
            else
            {
                RWIMMEDIGLOBAL(im3DRenderPipelines).pointList = NULL;
            }
            return RWIMMEDIGLOBAL(platformIm3DRenderPipelines).pointList;
        default:
            RWERROR((E_RW_INVALIDPRIMTYPE, primType));
            break;
        }
    }

    return NULL;
}

void* _rwIm3DClose(void* instance)
{
    _rwIm3DDestroyPlatformRenderPipelines(&RWIMMEDIGLOBAL(platformIm3DRenderPipelines));
    _rwIm3DDestroyPlatformTransformPipeline(&RWIMMEDIGLOBAL(platformIm3DTransformPipeline));

    _rwIm3DModule.numInstances--;

    return instance;
}

void* _rwIm3DOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    RwBool success;

    _rwIm3DModule.globalsOffset = offset;
    _rwIm3DGlobals = RWPLUGINOFFSET(rwImmediGlobals, RwEngineInstance, _rwIm3DModule.globalsOffset);
    _rwIm3DModule.numInstances++;

    memset(&RWIMMEDIGLOBAL(im3DTransformPipeline), 0, sizeof(rwImmediGlobals));

    success =
        _rwIm3DCreatePlatformTransformPipeline(&RWIMMEDIGLOBAL(platformIm3DTransformPipeline));
    if (success)
    {
        success =
            _rwIm3DCreatePlatformRenderPipelines(&RWIMMEDIGLOBAL(platformIm3DRenderPipelines));
    }

    if (success)
    {
        return instance;
    }

    _rwIm3DDestroyPlatformRenderPipelines(&RWIMMEDIGLOBAL(platformIm3DRenderPipelines));
    _rwIm3DDestroyPlatformTransformPipeline(&RWIMMEDIGLOBAL(platformIm3DTransformPipeline));

    _rwIm3DModule.numInstances--;

    return NULL;
}
