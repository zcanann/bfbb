#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#include <math.h>
#include <string.h>

#define rpLIGHT 3

#define rwPLUGIN_ID 2

#define rpLIGHTPRIVATENOCHROMA 0x01

#define rwObjectHasFrameInitialize(o, t, s, syncFunc)                                              \
    MACRO_START                                                                                    \
    {                                                                                              \
        rwObjectInitialize(o, t, s);                                                               \
        ((RwObjectHasFrame*)(o))->sync = (syncFunc);                                               \
    }                                                                                              \
    MACRO_STOP

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

#define E_RW_BADVERSION 0x80000004

typedef union _rwIEEEFloatShapeType _rwIEEEFloatShapeType;
union _rwIEEEFloatShapeType
{
    RwReal value;
    RwUInt32 word;
};

#define rwIEEEGetFloatWord(i, d)                                                                   \
    MACRO_START                                                                                    \
    {                                                                                              \
        _rwIEEEFloatShapeType gf_u;                                                                \
        gf_u.value = (d);                                                                          \
        (i) = gf_u.word;                                                                           \
    }                                                                                              \
    MACRO_STOP

#define rwIEEESetFloatWord(d, i)                                                                   \
    MACRO_START                                                                                    \
    {                                                                                              \
        _rwIEEEFloatShapeType sf_u;                                                                \
        sf_u.word = (i);                                                                           \
        (d) = sf_u.value;                                                                          \
    }                                                                                              \
    MACRO_STOP

#define rwACOSPI ((RwReal)3.1415925026e+00)
#define rwACOSPIO2HI ((RwReal)1.5707962513e+00)
#define rwACOSPIO2LO ((RwReal)7.5497894159e-08)
#define rwACOSPS0 ((RwReal)1.6666667163e-01)
#define rwACOSPS1 ((RwReal) - 3.2556581497e-01)
#define rwACOSPS2 ((RwReal)2.0121252537e-01)
#define rwACOSPS3 ((RwReal) - 4.0055535734e-02)
#define rwACOSPS4 ((RwReal)7.9153501429e-04)
#define rwACOSPS5 ((RwReal)3.4793309169e-05)
#define rwACOSQS1 ((RwReal) - 2.4033949375e+00)
#define rwACOSQS2 ((RwReal)2.0209457874e+00)
#define rwACOSQS3 ((RwReal) - 6.8828397989e-01)
#define rwACOSQS4 ((RwReal)7.7038154006e-02)

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rpLightGlobals rpLightGlobals;
struct rpLightGlobals
{
    RwFreeList* lightFreeList;
};

typedef struct RpLightChunkInfo RpLightChunkInfo;
struct RpLightChunkInfo
{
    RwReal radius;
    RwReal red;
    RwReal green;
    RwReal blue;
    RwReal minusCosAngle;
    RwUInt32 typeAndFlags;
};

extern void* _rpLightOpen(void* instance, RwInt32 offset, RwInt32 size);
extern void* _rpLightClose(void* instance, RwInt32 offset, RwInt32 size);

#define RWLIGHTGLOBAL(var)                                                                         \
    (RWPLUGINOFFSET(rpLightGlobals, RwEngineInstance, lightModule.globalsOffset)->var)

static RwPluginRegistry lightTKList = { sizeof(RpLight),         sizeof(RpLight),        0, 0,
                                        (RwPluginRegEntry*)NULL, (RwPluginRegEntry*)NULL };

static RwModuleInfo lightModule;

static RwInt32 _rpLightFreeListBlockSize = 32;
static RwInt32 _rpLightFreeListPreallocBlocks = 1;
static RwFreeList _rpLightFreeList;

static void LightTidyDestroyLight(void* pMem, void* pData)
{
    RpLightDestroy((RpLight*)pMem);
}

static RwObjectHasFrame* LightSync(RwObjectHasFrame* type)
{
    return type;
}

RpLight* RpLightSetRadius(RpLight* light, RwReal radius)
{
    RwFrame* frame;

    frame = RpLightGetFrame(light);
    light->radius = radius;

    if (frame)
    {
        RwFrameUpdateObjects(frame);
    }

    return light;
}

RpLight* RpLightSetColor(RpLight* light, const RwRGBAReal* color)
{
    light->color = *color;

    if (light->color.red == light->color.green && light->color.red == light->color.blue)
    {
        rwObjectSetPrivateFlags(light, rpLIGHTPRIVATENOCHROMA);
    }
    else
    {
        rwObjectSetPrivateFlags(light, 0);
    }

    return light;
}

RwReal RpLightGetConeAngle(const RpLight* light)
{
    RwReal result;
    RwReal x = -light->minusCosAngle;
    RwReal z, p, q, r, w, s, c, df;
    RwInt32 hx, ix;

    rwIEEEGetFloatWord(hx, x);
    ix = hx & 0x7fffffff;

    if (ix >= 0x3f800000)
    {
        if (hx > 0)
        {
            result = (RwReal)0.0;
        }
        else
        {
            result = rwACOSPI + (RwReal)2.0 * rwACOSPIO2LO;
        }
    }
    else if (ix < 0x3f000000)
    {
        if (ix <= 0x23000000)
        {
            result = rwACOSPIO2HI + rwACOSPIO2LO;
        }
        else
        {
            z = x * x;
            p = z * (rwACOSPS0 +
                     z * (rwACOSPS1 +
                          z * (rwACOSPS2 + z * (rwACOSPS3 + z * (rwACOSPS4 + z * rwACOSPS5)))));
            q = (RwReal)1.0 + z * (rwACOSQS1 + z * (rwACOSQS2 + z * (rwACOSQS3 + z * rwACOSQS4)));
            r = p / q;
            result = rwACOSPIO2HI - (x - (rwACOSPIO2LO - x * r));
        }
    }
    else if (hx < 0)
    {
        z = ((RwReal)1.0 + x) * (RwReal)0.5;
        p = z *
            (rwACOSPS0 +
             z * (rwACOSPS1 + z * (rwACOSPS2 + z * (rwACOSPS3 + z * (rwACOSPS4 + z * rwACOSPS5)))));
        q = (RwReal)1.0 + z * (rwACOSQS1 + z * (rwACOSQS2 + z * (rwACOSQS3 + z * rwACOSQS4)));
        s = _rwSqrt(z);
        r = p / q;
        w = r * s - rwACOSPIO2LO;
        result = rwACOSPI - (RwReal)2.0 * (s + w);
    }
    else
    {
        RwInt32 idf;

        z = ((RwReal)1.0 - x) * (RwReal)0.5;
        s = _rwSqrt(z);
        df = s;
        rwIEEEGetFloatWord(idf, df);
        rwIEEESetFloatWord(df, idf & 0xfffff000);
        p = z *
            (rwACOSPS0 +
             z * (rwACOSPS1 + z * (rwACOSPS2 + z * (rwACOSPS3 + z * (rwACOSPS4 + z * rwACOSPS5)))));
        q = (RwReal)1.0 + z * (rwACOSQS1 + z * (rwACOSQS2 + z * (rwACOSQS3 + z * rwACOSQS4)));
        c = (z - df * df) / (s + df);
        r = p / q;
        w = r * s + c;
        result = (RwReal)2.0 * (df + w);
    }

    return result;
}

RpLight* RpLightSetConeAngle(RpLight* light, RwReal angle)
{
    RwReal minusCosAngle;

    if (angle < (RwReal)0.0 || angle > rwPIOVER2)
    {
        return (RpLight*)NULL;
    }

    minusCosAngle = -(RwReal)cos(angle);
    light->minusCosAngle = minusCosAngle;

    return light;
}

RwInt32 RpLightRegisterPlugin(RwInt32 size, RwUInt32 pluginID,
                              RwPluginObjectConstructor constructCB,
                              RwPluginObjectDestructor destructCB, RwPluginObjectCopy copyCB)
{
    RwInt32 plug;

    plug =
        _rwPluginRegistryAddPlugin(&lightTKList, size, pluginID, constructCB, destructCB, copyCB);

    return plug;
}

RwInt32 RpLightRegisterPluginStream(RwUInt32 pluginID, RwPluginDataChunkReadCallBack readCB,
                                    RwPluginDataChunkWriteCallBack writeCB,
                                    RwPluginDataChunkGetSizeCallBack getSizeCB)
{
    RwInt32 plug;

    plug = _rwPluginRegistryAddPluginStream(&lightTKList, pluginID, readCB, writeCB, getSizeCB);

    return plug;
}

RpLight* RpLightStreamRead(RwStream* stream)
{
    RwUInt32 size;
    RwUInt32 version;
    RpLight* light;
    RpLightChunkInfo lite;

    if (!RwStreamFindChunk(stream, rwID_STRUCT, &size, &version))
    {
        return (RpLight*)NULL;
    }

    if (version >= rwLIBRARYBASEVERSION && version <= rwLIBRARYCURRENTVERSION)
    {
        memset(&lite, 0, sizeof(lite));

        if (RwStreamRead(stream, &lite, size) != size)
        {
            return (RpLight*)NULL;
        }

        RwMemNative32(&lite, sizeof(lite));

        light = RpLightCreate((lite.typeAndFlags >> 16) & 0xFF);
        if (!light)
        {
            return (RpLight*)NULL;
        }

        light->radius = lite.radius;
        light->color.red = lite.red;
        light->color.green = lite.green;
        light->color.blue = lite.blue;

        if (version < 0x30300)
        {
            RwReal TanAngle2 = lite.minusCosAngle * lite.minusCosAngle;
            RwReal CosAngle2 = (RwReal)1.0 / ((RwReal)1.0 + TanAngle2);
            RwReal CosAngle = _rwSqrt(CosAngle2);

            light->minusCosAngle = -CosAngle;
        }
        else
        {
            light->minusCosAngle = lite.minusCosAngle;
        }

        if (light->color.red == light->color.green && light->color.red == light->color.blue)
        {
            rwObjectSetPrivateFlags(light, rpLIGHTPRIVATENOCHROMA);
        }
        else
        {
            rwObjectSetPrivateFlags(light, 0);
        }

        rwObjectSetFlags(light, lite.typeAndFlags & 0xFF);

        if (!_rwPluginRegistryReadDataChunks(&lightTKList, stream, light))
        {
            return (RpLight*)NULL;
        }

        return light;
    }

    RWERROR((E_RW_BADVERSION));
    return (RpLight*)NULL;
}

RwBool RpLightDestroy(RpLight* light)
{
    _rwPluginRegistryDeInitObject(&lightTKList, light);

    rwObjectHasFrameReleaseFrame(light);

    RwFreeListFree(RWLIGHTGLOBAL(lightFreeList), light);

    return TRUE;
}

RpLight* RpLightCreate(RwInt32 type)
{
    RpLight* light;

    light = (RpLight*)RwFreeListAlloc(RWLIGHTGLOBAL(lightFreeList));
    if (!light)
    {
        return (RpLight*)NULL;
    }

    rwObjectHasFrameInitialize(light, rpLIGHT, type, LightSync);

    light->radius = (RwReal)0.0;
    light->minusCosAngle = (RwReal)1.0;

    light->color.red = (RwReal)1.0;
    light->color.green = (RwReal)1.0;
    light->color.blue = (RwReal)1.0;
    light->color.alpha = (RwReal)1.0;
    rwObjectSetPrivateFlags(light, rpLIGHTPRIVATENOCHROMA);

    rwLinkListInitialize(&light->WorldSectorsInLight);
    rwLLLinkInitialize(&light->inWorld);

    light->lightFrame = RWSRCGLOBAL(lightFrame) - 1;

    rwObjectSetFlags(light, rpLIGHTLIGHTATOMICS | rpLIGHTLIGHTWORLD);

    _rwPluginRegistryInitObject(&lightTKList, light);

    return light;
}

void* _rpLightClose(void* instance, RwInt32 offset, RwInt32 size)
{
    RwFreeListForAllUsed(RWLIGHTGLOBAL(lightFreeList), LightTidyDestroyLight, NULL);
    RwFreeListDestroy(RWLIGHTGLOBAL(lightFreeList));
    RWLIGHTGLOBAL(lightFreeList) = (RwFreeList*)NULL;

    lightModule.numInstances--;

    return instance;
}

void* _rpLightOpen(void* instance, RwInt32 offset, RwInt32 size)
{
    lightModule.globalsOffset = offset;

    RWLIGHTGLOBAL(lightFreeList) =
        RwFreeListCreateAndPreallocateSpace(lightTKList.sizeOfStruct, _rpLightFreeListBlockSize, 4,
                                            _rpLightFreeListPreallocBlocks, &_rpLightFreeList);
    if (RWLIGHTGLOBAL(lightFreeList))
    {
        lightModule.numInstances++;

        return instance;
    }

    return NULL;
}
