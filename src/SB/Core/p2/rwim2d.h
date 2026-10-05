#ifndef PS2_RWIM2D_H
#define PS2_RWIM2D_H

#include <rwplcore.h>

// RenderWare 3.5 Sky2 immediate-mode vertex; original PS2 DWARF agrees.

struct RwSky2DVertexFields
{
    RwV3d          scrVertex;
    RwReal         camVertex_z;
    RwReal         u;
    RwReal         v;
    RwReal         recipZ;
    RwReal         pad1;
    RwRGBAReal     color;
    RwV3d          objNormal;
    RwReal         pad2;
};

union RwSky2DVertexAlignmentOverlay
{
    RwSky2DVertexFields els;
    unsigned __int128 qWords[4];
};

struct RwSky2DVertex
{
    RwSky2DVertexAlignmentOverlay u;
};

#define RwIm2DVertexSetCameraX(vert, camx)              \
     /* Nothing */
#define RwIm2DVertexSetCameraY(vert, camy)              \
     /* Nothing */
#define RwIm2DVertexSetCameraZ(vert, camz)              \
     ((vert)->u.els.camVertex_z = (camz))
#define RwIm2DVertexSetRecipCameraZ(vert, recipz)       \
    ((vert)->u.els.recipZ = (recipz))

#define RwIm2DVertexGetCameraZ(vert)                    \
    ((vert)->u.els.camVertex_z)
#define RwIm2DVertexGetRecipCameraZ(vert)               \
    ((vert)->u.els.recipZ)

/* Set screen space coordinates in a device vertex */
#define RwIm2DVertexSetScreenX(vert, scrnx)             \
    ((vert)->u.els.scrVertex.x = (scrnx))
#define RwIm2DVertexSetScreenY(vert, scrny)             \
    ((vert)->u.els.scrVertex.y = (scrny))
#define RwIm2DVertexSetScreenZ(vert, scrnz)             \
    ((vert)->u.els.scrVertex.z = (scrnz))
#define RwIm2DVertexGetScreenX(vert)                    \
    ((vert)->u.els.scrVertex.x)
#define RwIm2DVertexGetScreenY(vert)                    \
    ((vert)->u.els.scrVertex.y)
#define RwIm2DVertexGetScreenZ(vert)                    \
    ((vert)->u.els.scrVertex.z)

/* Set texture coordinates in a device vertex */
#define RwIm2DVertexSetU(vert, texu, recipz)            \
    ((vert)->u.els.u = (texu))
#define RwIm2DVertexSetV(vert, texv, recipz)            \
    ((vert)->u.els.v = (texv))
#define RwIm2DVertexGetU(vert)                          \
    ((vert)->u.els.u)
#define RwIm2DVertexGetV(vert)                          \
    ((vert)->u.els.v)

/* Modify the luminance stuff */
#define RwIm2DVertexSetRealRGBA(vert, r, g, b, a)       \
MACRO_START                                             \
{                                                       \
    ((vert)->u.els.color.red = (r));                    \
    ((vert)->u.els.color.green = (g));                  \
    ((vert)->u.els.color.blue = (b));                   \
    ((vert)->u.els.color.alpha = (a));                  \
}                                                       \
MACRO_STOP

#define RwIm2DVertexSetIntRGBA(vert, r, g, b, a)        \
MACRO_START                                             \
{                                                       \
    ((vert)->u.els.color.red = (RwReal)(r));            \
    ((vert)->u.els.color.green = (RwReal)(g));          \
    ((vert)->u.els.color.blue = (RwReal)(b));           \
    ((vert)->u.els.color.alpha = (RwReal)(a));          \
}                                                       \
MACRO_STOP

#define RwIm2DVertexGetRed(vert)       \
    ((RwUInt32)((vert)->u.els.color.red))

#define RwIm2DVertexGetGreen(vert)     \
    ((RwUInt32)((vert)->u.els.color.green))

#define RwIm2DVertexGetBlue(vert)      \
    ((RwUInt32)((vert)->u.els.color.blue))

#define RwIm2DVertexGetAlpha(vert)     \
    ((RwUInt32)((vert)->u.els.color.alpha))

#define RwIm2DVertexCopyRGBA(dst, src) \
    ((dst)->u.els.color = (src)->u.els.color)

extern "C" {
RwReal RwIm2DGetNearScreenZ(void);
RwBool RwIm2DRenderPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices, RwInt32 numVertices);
RwBool RwIm2DRenderIndexedPrimitive(RwPrimitiveType primType, RwIm2DVertex* vertices,
                                   RwInt32 numVertices, RwImVertexIndex* indices,
                                   RwInt32 numIndices);
}

#endif
