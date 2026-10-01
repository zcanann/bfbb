#include <rwsdk/rwcore.h>

#define E_RW_NOERROR ((RwInt32)0x80000000L)

typedef struct RwModuleInfo RwModuleInfo;
struct RwModuleInfo
{
    RwInt32 globalsOffset;
    RwInt32 numInstances;
};

typedef struct rwErrorGlobals rwErrorGlobals;
struct rwErrorGlobals
{
    RwError errComp;
};

#define RWERRORGLOBAL(var) \
    (RWPLUGINOFFSET(rwErrorGlobals, RwEngineInstance, errorModule.globalsOffset)->var)

static RwModuleInfo errorModule;

void* _rwErrorOpen(void* object, RwInt32 offset, RwInt32 size)
{
    errorModule.globalsOffset = offset;
    errorModule.numInstances++;

    RWERRORGLOBAL(errComp.pluginID) = 0;
    RWERRORGLOBAL(errComp.errorCode) = E_RW_NOERROR;

    return object;
}

void* _rwErrorClose(void* object, RwInt32 offset, RwInt32 size)
{
    errorModule.numInstances--;

    return object;
}

RwError* RwErrorSet(RwError* code)
{
    if (RWERRORGLOBAL(errComp.pluginID) == 0 && RWERRORGLOBAL(errComp.errorCode) == E_RW_NOERROR)
    {
        if (code->errorCode & 0x80000000)
        {
            RWERRORGLOBAL(errComp.pluginID) = 0;
        }
        else
        {
            RWERRORGLOBAL(errComp.pluginID) = code->pluginID;
        }

        RWERRORGLOBAL(errComp.errorCode) = code->errorCode;
    }

    return code;
}

RwError* RwErrorGet(RwError* code)
{
    *code = RWERRORGLOBAL(errComp);

    RWERRORGLOBAL(errComp.pluginID) = 0;
    RWERRORGLOBAL(errComp.errorCode) = E_RW_NOERROR;

    return code;
}

RwInt32 _rwerror(RwInt32 code, ...)
{
    va_list ap;

    va_start(ap, code);
    va_end(ap);

    return code;
}
