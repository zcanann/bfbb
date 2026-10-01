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

#define E_RW_NOCAMERA 0x10

RwReal RwIm2DGetNearScreenZ(void)
{
    return RWSRCGLOBAL(dOpenDevice).zBufferNear;
}

RwReal RwIm2DGetFarScreenZ(void)
{
    return RWSRCGLOBAL(dOpenDevice).zBufferFar;
}

RwBool RwRenderStateSet(RwRenderState state, void* value)
{
    if (!RWSRCGLOBAL(curCamera))
    {
        RWERROR((E_RW_NOCAMERA));
        return FALSE;
    }

    return RWSRCGLOBAL(dOpenDevice).fpRenderStateSet(state, value);
}

RwBool RwRenderStateGet(RwRenderState state, void* value)
{
    return RWSRCGLOBAL(dOpenDevice).fpRenderStateGet(state, value);
}

RwBool RwIm2DRenderLine(RwIm2DVertex* vertices, RwInt32 numVertices, RwInt32 vert1, RwInt32 vert2)
{
    return RWSRCGLOBAL(dOpenDevice).fpIm2DRenderLine(vertices, numVertices, vert1, vert2);
}

RwBool RwIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices)
{
    return RWSRCGLOBAL(dOpenDevice).fpIm2DRenderPrimitive(primType, vertices, numVertices);
}

RwBool RwIm2DRenderIndexedPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices,
                                    RwInt32 numVertices, RwImVertexIndex* indices,
                                    RwInt32 numIndices)
{
    return RWSRCGLOBAL(dOpenDevice).fpIm2DRenderIndexedPrimitive(primType, vertices, numVertices,
                                                                 indices, numIndices);
}
