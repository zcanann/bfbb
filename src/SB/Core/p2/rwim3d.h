#ifndef PS2_RWIM3D_H
#define PS2_RWIM3D_H

#include <rwplcore.h>

enum RwCullMode
{
    rwCULLMODENACULLMODE = 0,
    rwCULLMODECULLNONE,
    rwCULLMODECULLBACK,
    rwCULLMODECULLFRONT,
    rwCULLMODEFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

enum RwIm3DTransformFlags
{
    rwIM3D_VERTEXUV = 1,
    rwIM3D_ALLOPAQUE = 2,
    rwIM3D_NOCLIP = 4,
    rwIM3D_VERTEXXYZ = 8,
    rwIM3D_VERTEXRGBA = 16,
    rwIM3DTRANSFORMFLAGSFORCEENUMSIZEINT = RWFORCEENUMSIZEINT
};

// SDK vertex setters using the original PS2 aggregate member layout.
#define RxObjSpace3DVertexSetPos(_vert, _pos) ((_vert)->objVertex = *(_pos))

#define RwIm3DVertexSetPos(_vert, _imx, _imy, _imz)                                               \
MACRO_START                                                                                       \
{                                                                                                 \
    RwV3d tmp;                                                                                    \
    tmp.x = (_imx);                                                                               \
    tmp.y = (_imy);                                                                               \
    tmp.z = (_imz);                                                                               \
    RxObjSpace3DVertexSetPos(_vert, &tmp);                                                        \
}                                                                                                 \
MACRO_STOP

#define RxObjSpace3DVertexSetNormal(_vert, _normal) ((_vert)->objNormal = *(_normal))

#define RwIm3DVertexSetNormal(_vert, _imx, _imy, _imz)                                            \
MACRO_START                                                                                       \
{                                                                                                 \
    RwV3d tmp;                                                                                    \
    tmp.x = (_imx);                                                                               \
    tmp.y = (_imy);                                                                               \
    tmp.z = (_imz);                                                                               \
    RxObjSpace3DVertexSetNormal(_vert, &tmp);                                                     \
}                                                                                                 \
MACRO_STOP

#define RxObjSpace3DVertexSetPreLitColor(_vert, _col) ((_vert)->c.preLitColor = *(_col))

#define RwIm3DVertexSetRGBA(_vert, _r, _g, _b, _a)                                              \
MACRO_START                                                                                     \
{                                                                                               \
    RwRGBA* const _col = &(_vert)->c.preLitColor;                                               \
    _col->red = (_r);                                                                           \
    _col->green = (_g);                                                                         \
    _col->blue = (_b);                                                                          \
    _col->alpha = (_a);                                                                         \
}                                                                                               \
MACRO_STOP

#define RwIm3DVertexSetUV(_vert, _u, _v)                                                          \
MACRO_START                                                                                       \
{                                                                                                 \
    (_vert)->u = _u;                                                                              \
    (_vert)->v = _v;                                                                              \
}                                                                                                 \
MACRO_STOP

extern "C" {
RwBool RwRenderStateGet(RwRenderState state, void* value);
RwBool RwRenderStateSet(RwRenderState state, void* value);
void* RwIm3DTransform(RwIm3DVertex* vertices, RwUInt32 count, RwMatrix* matrix, RwUInt32 flags);
RwBool RwIm3DEnd(void);
RwBool RwIm3DRenderPrimitive(RwPrimitiveType primitive);
RwBool RwIm3DRenderIndexedPrimitive(RwPrimitiveType primType, RwImVertexIndex* indices, RwInt32 numIndices);
}

#endif
