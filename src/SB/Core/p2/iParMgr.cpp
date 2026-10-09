#include "iParMgr.h"
#include "zGlobals.h"
#include "zParSys.h"

#include <types.h>
#include <string.h>
#include <xstransvc.h>
#include <rwplcore.h>
#include <rwim3d.h>

tagiRenderArrays gRenderArr;
tagiRenderInput gRenderBuffer;

static S32 gColorTableInit;
static F32 gColorTable[256];

static void iRenderInit()
{
    gRenderBuffer.m_mode = 0;
    gRenderBuffer.m_indexCount  = 0;
    gRenderBuffer.m_vertexCount = 0;
    gRenderBuffer.m_vertexTypeSize = 0x24;
    gRenderBuffer.m_index = &gRenderArr.m_index[0];
    gRenderBuffer.m_vertex = &gRenderArr.m_vertex[0];
    gRenderBuffer.m_vertexTZ = gRenderArr.m_vertexTZ;
}

static void iRenderTrianglesImmediate(S32 vertType, S32 vertTypeSize, void* data, S32 dataSize, U16* index, S32 indexSize)
{
    if (RwIm3DTransform((RwIm3DVertex *)data, dataSize, NULL, 1) != NULL)
    {
        if (indexSize != 0)
        {
            RwIm3DRenderIndexedPrimitive(rwPRIMTYPETRILIST, index, indexSize);
        }
        else
        {
            RwIm3DRenderPrimitive(rwPRIMTYPETRILIST);
        }
        RwIm3DEnd();
    }

}

static void iRenderFlush()
{
    if (gRenderBuffer.m_vertexCount > 0)
    {
        iRenderTrianglesImmediate(gRenderBuffer.m_vertexType, gRenderBuffer.m_vertexTypeSize, gRenderBuffer.m_vertex, gRenderBuffer.m_vertexCount, gRenderBuffer.m_index, gRenderBuffer.m_indexCount);
    }

    gRenderBuffer.m_indexCount  = 0;
    gRenderBuffer.m_vertexCount = 0;
}

void iParMgrInit()
{
    iRenderInit();

    if (!gColorTableInit)
    {
        for (S32 i = 0; i < 256; i++)
        {
            gColorTable[i] = i / 255.0f;
        }

        gColorTableInit = 1;
    }
}

void iParMgrUpdate(F32)
{
}

void iParMgrRender()
{
}

void iRenderSetCameraViewMatrix(xMat4x3* m)
{
    if ((m == NULL) && (globals.camera.lo_cam != NULL))
    {
        gRenderBuffer.m_camViewMatrix =
            *(xMat4x3*)&((RwFrame*)((RwCamera*)RWSRCGLOBAL(curCamera))->object.object.parent)
                 ->modelling;
    }
    else
    {
        gRenderBuffer.m_camViewMatrix = *m;
    }
    xVec3Inv((xVec3*)(&gRenderBuffer.m_camViewR), (xVec3*)(&gRenderBuffer.m_camViewMatrix.right));
    xVec3Inv((xVec3*)(&gRenderBuffer.m_camViewU), (xVec3*)(&gRenderBuffer.m_camViewMatrix.up));
}

void iRenderPushQuadStreak(xPar* p, xParCmdTex* tex)
{
    void* vertices;
    U16* indices;
    static RxObjSpace3DVertex v3d[4];
    static U16 i3d[6] = { 0, 1, 2, 0, 2, 3 };

    // vertices/indices are bound to the two function-scope templates up here,
    // not at the point of use: retail materialises both addresses in the entry
    // block and keeps them in r28/r29 across the two iRenderFlush calls, which
    // is what pushes the function to six callee-saved GPRs (stmw r26).
    vertices = v3d;
    indices = i3d;

    if (gRenderBuffer.m_indexCount + 6 > 960)
    {
        iRenderFlush();
    }

    if (gRenderBuffer.m_vertexCount + 4 > 480)
    {
        iRenderFlush();
    }

    U8 r = p->m_c[0];
    U8 g = p->m_c[1];
    U8 b = p->m_c[2];
    U8 a = p->m_c[3];
    // Declared bare and assigned below: mwcc colours FP values in declaration
    // order, and retail's register map needs d before t before the t copies
    // before p.  The copies are what retail's `fmr f9,f6` trio encodes -- the
    // trailing vertex is computed by subtracting from the tail in place while
    // the leading vertex adds into a copy of it, so the copy cannot be
    // propagated away.
    F32 dx, dy, dz;
    F32 tx, ty, tz;
    F32 ax, ay, az;
    F32 px, py, pz;
    F32 size;

    px = p->m_pos.x;
    py = p->m_pos.y;
    pz = p->m_pos.z;
    tx = px - 5.0f * p->m_vel.x;
    ty = py - 5.0f * p->m_vel.y;
    tz = pz - 5.0f * p->m_vel.z;
    ax = tx;
    ay = ty;
    az = tz;
    size = p->m_size;
    dx = size * gRenderBuffer.m_camViewR.x;
    dy = size * gRenderBuffer.m_camViewR.y;
    dz = size * gRenderBuffer.m_camViewR.z;

    RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[3], r, g, b, a);

    RwIm3DVertexSetPos(&v3d[0], px - dx, py - dy, pz - dz);

    tx -= dx;
    ty -= dy;
    tz -= dz;
    ax += dx;
    ay += dy;
    az += dz;

    RwIm3DVertexSetPos(&v3d[1], tx, ty, tz);

    RwIm3DVertexSetPos(&v3d[2], ax, ay, az);

    RwIm3DVertexSetPos(&v3d[3], px + dx, py + dy, pz + dz);

    if (tex != NULL)
    {
        F32 u1 = tex->x1 + p->m_texIdx[0] * tex->unit_width;
        F32 v1 = tex->y1 + p->m_texIdx[1] * tex->unit_height;
        F32 u2 = tex->x1 + (p->m_texIdx[0] + 1) * tex->unit_width;
        F32 v2 = tex->y1 + (p->m_texIdx[1] + 1) * tex->unit_height;

        v3d[0].u = u1;
        v3d[0].v = v2;
        v3d[1].u = u1;
        v3d[1].v = v1;
        v3d[2].u = u2;
        v3d[2].v = v1;
        v3d[3].u = u2;
        v3d[3].v = v2;
    }
    else
    {
        v3d[0].u = 0.0f;
        v3d[0].v = 1.0f;
        v3d[1].u = 0.0f;
        v3d[1].v = 0.0f;
        v3d[2].u = 1.0f;
        v3d[2].v = 0.0f;
        // Retail bug, faithfully preserved: the last pair writes v3d[3].u = 0.0f
        // (it should be 1.0f) and then re-writes v3d[2].v instead of v3d[3].v,
        // so v3d[3].v keeps whatever the previous particle left in the static.
        v3d[3].u = 0.0f;
        v3d[2].v = 1.0f;
    }

    U16* dst = &gRenderBuffer.m_index[gRenderBuffer.m_indexCount];
    dst[0] = gRenderBuffer.m_vertexCount + indices[0];
    dst[1] = gRenderBuffer.m_vertexCount + indices[1];
    dst[2] = gRenderBuffer.m_vertexCount + indices[2];
    dst[3] = gRenderBuffer.m_vertexCount + indices[3];
    dst[4] = gRenderBuffer.m_vertexCount + indices[4];
    dst[5] = gRenderBuffer.m_vertexCount + indices[5];

    memcpy((U8*)gRenderBuffer.m_vertex +
               gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount,
           vertices, gRenderBuffer.m_vertexTypeSize * 4);

    gRenderBuffer.m_indexCount += 6;
    gRenderBuffer.m_vertexCount += 4;
}

static void iRenderPushFlat(xPar* p, xParCmdTex* tex)
{
    void* vertices;
    U16* indices;
    static RxObjSpace3DVertex v3d[4];
    static U16 i3d[6] = { 0, 1, 2, 0, 2, 3 };

    // vertices/indices are bound to the two function-scope templates up here,
    // not at the point of use: retail materialises both addresses in the entry
    // block and keeps them in r28/r29 across the two iRenderFlush calls, which
    // is what pushes the function to six callee-saved GPRs (stmw r26).
    vertices = v3d;
    indices = i3d;

    if (gRenderBuffer.m_indexCount + 6 > 960)
    {
        iRenderFlush();
    }

    if (gRenderBuffer.m_vertexCount + 4 > 480)
    {
        iRenderFlush();
    }

    U8 r = p->m_c[0];
    U8 g = p->m_c[1];
    U8 b = p->m_c[2];
    U8 a = p->m_c[3];
    F32 size = 0.5f * p->m_size;
    F32 yaw = 0.0f;
    xMat3x3 groundmat;

    if (p->m_rotdeg[0])
    {
        yaw = 6.2831855f * (p->m_rotdeg[0] / 255.0f);
    }

    xMat3x3Euler(&groundmat, yaw, 0.0f, 0.0f);

    // See iParMgrRenderParSys_Ground: declared bare so the "at" row's products
    // colour ahead of the "right" row's, which is retail's FP register order.
    F32 zdx, zdz;
    F32 xdx, xdz;
    F32 px, py, pz;

    xdx = groundmat.right.x * size;
    xdz = groundmat.right.z * size;
    px = p->m_pos.x;
    py = p->m_pos.y;
    pz = p->m_pos.z;
    zdx = groundmat.at.x * size;
    zdz = groundmat.at.z * size;

    RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);
    RwIm3DVertexSetRGBA(&v3d[3], r, g, b, a);

    F32 mz = pz - xdz;
    F32 mx = px - xdx;
    F32 sx = px + xdx;
    F32 sz = pz + xdz;

    RwIm3DVertexSetPos(&v3d[0], mx - zdx, py, mz - zdz);
    RwIm3DVertexSetPos(&v3d[1], sx - zdx, py, sz - zdz);
    RwIm3DVertexSetPos(&v3d[2], zdx + sx, py, zdz + sz);
    RwIm3DVertexSetPos(&v3d[3], zdx + mx, py, zdz + mz);

    if (tex != NULL)
    {
        F32 u1 = tex->x1 + p->m_texIdx[0] * tex->unit_width;
        F32 v1 = tex->y1 + p->m_texIdx[1] * tex->unit_height;
        F32 u2 = tex->x1 + (p->m_texIdx[0] + 1) * tex->unit_width;
        F32 v2 = tex->y1 + (p->m_texIdx[1] + 1) * tex->unit_height;

        v3d[0].u = u1;
        v3d[0].v = v1;
        v3d[1].u = u2;
        v3d[1].v = v1;
        v3d[2].u = u2;
        v3d[2].v = v2;
        v3d[3].u = u1;
        v3d[3].v = v2;
    }
    else
    {
        v3d[0].u = 0.0f;
        v3d[0].v = 0.0f;
        v3d[1].u = 1.0f;
        v3d[1].v = 0.0f;
        v3d[2].u = 1.0f;
        v3d[2].v = 1.0f;
        v3d[3].u = 0.0f;
        v3d[3].v = 1.0f;
    }

    U16* dst = &gRenderBuffer.m_index[gRenderBuffer.m_indexCount];
    dst[0] = gRenderBuffer.m_vertexCount + indices[0];
    dst[1] = gRenderBuffer.m_vertexCount + indices[1];
    dst[2] = gRenderBuffer.m_vertexCount + indices[2];
    dst[3] = gRenderBuffer.m_vertexCount + indices[3];
    dst[4] = gRenderBuffer.m_vertexCount + indices[4];
    dst[5] = gRenderBuffer.m_vertexCount + indices[5];

    memcpy((U8*)gRenderBuffer.m_vertex +
               gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount,
           vertices, gRenderBuffer.m_vertexTypeSize * 4);

    gRenderBuffer.m_indexCount += 6;
    gRenderBuffer.m_vertexCount += 4;
}

void iParMgrRenderParSys_QuadStreak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    RwTexture* texture;
    RwRaster* raster;

    iRenderSetCameraViewMatrix(NULL);

    // PS2 caches the particle texture on the system rather than looking it up.
    texture = ((zParSys*)data)->txtr_particle;
    if (texture != NULL)
    {
        raster = texture->raster;
        if (raster != NULL)
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    }

    for (; idx != NULL; idx = idx->m_next)
    {
        iRenderPushQuadStreak(idx, ps->m_cmdTex);
    }
    iRenderFlush();
}


void iParMgrRenderParSys_Static(void*, xParGroup*)
{
}

void iParMgrRenderParSys_Flat(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    RwTexture* texture;
    RwRaster* raster;

    iRenderSetCameraViewMatrix(NULL);

    // PS2 caches the particle texture on the system rather than looking it up.
    texture = ((zParSys*)data)->txtr_particle;
    if (texture != NULL)
    {
        raster = texture->raster;
        if (raster != NULL)
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    }
    for (; idx != NULL; idx = idx->m_next)
    {
        iRenderPushFlat(idx, ps->m_cmdTex);
    }
    iRenderFlush();
}

namespace
{
    // VU registers are restored after RenderWare calls, which may overwrite them.
    inline void prepare_streak_culling()
    {
        asm volatile("lqc2 vf14, 0(%0)\n"
                     "lqc2 vf15, 16(%0)\n"
                     "lqc2 vf16, 32(%0)\n"
                     "lqc2 vf17, 48(%0)\n"
                     : : "r"(globals.camera.frustplane) : "memory");
        asm volatile("lqc2 vf10, 0(%0)\n"
                     : : "r"(&gRenderBuffer.m_camViewR) : "memory");
    }

    inline S32 prepare_streak_vertices(const xPar* p, F32 length)
    {
        // Form the tail and its two width offsets. Cull only when the head and
        // tail are outside the same packed side plane.
        S32 outside;
        U32 other;
        U32 factor;
        asm volatile("lqc2 vf8, 16(%3)\n"
                     "lqc2 vf9, 32(%3)\n"
                     "mfc1 %2, %4\n"
                     "qmtc2 %2, vf7\n"
                     "vadda.xyz ACC, vf0, vf8\n"
                     "vmsubx.xyz vf9, vf9, vf7x\n"
                     "vaddax.xyzw ACC, vf17, vf0x\n"
                     "vmsubax.xyzw ACC, vf14, vf8x\n"
                     "vmsubay.xyzw ACC, vf15, vf8y\n"
                     "vmsubz.xyzw vf1, vf16, vf8z\n"
                     "vaddaw.xyzw ACC, vf17, vf8w\n"
                     "vmsubax.xyzw ACC, vf14, vf9x\n"
                     "vmsubay.xyzw ACC, vf15, vf9y\n"
                     "qmfc2 %0, vf1\n"
                     "vmsubz.xyzw vf2, vf16, vf9z\n"
                     "vadda.xyz ACC, vf9, vf0\n"
                     "vmsubw.xyz vf6, vf10, vf8w\n"
                     "vmaddw.xyz vf7, vf10, vf8w\n"
                     "qmfc2 %1, vf2\n"
                     "pand %0, %0, %1\n"
                     "pcgtw %0, $0, %0\n"
                     "ppach %0, $0, %0\n"
                     : "=&r"(outside), "=&r"(other), "=&r"(factor)
                     : "r"(p), "f"(length) : "memory");
        return outside;
    }

    inline void write_streak_vertices(RxObjSpace3DVertex* vertices, const xPar* p)
    {
        // The original PS2 layout is head, tail-minus-width, tail-plus-width.
        U32 head;
        U32 tail;
        U32 color;
        asm volatile("qmfc2 %0, vf8\n"
                     "lw %2, 12(%4)\n"
                     "sw %0, 0(%3)\n"
                     "prot3w %0, %0\n"
                     "sw %0, 4(%3)\n"
                     "prot3w %0, %0\n"
                     "sw %0, 8(%3)\n"
                     "qmfc2 %1, vf6\n"
                     "sw %2, 12(%3)\n"
                     "sw %1, 36(%3)\n"
                     "prot3w %1, %1\n"
                     "sw %1, 40(%3)\n"
                     "prot3w %1, %1\n"
                     "sw %1, 44(%3)\n"
                     "qmfc2 %0, vf7\n"
                     "sw %2, 48(%3)\n"
                     "sw %0, 72(%3)\n"
                     "prot3w %0, %0\n"
                     "sw %0, 76(%3)\n"
                     "prot3w %0, %0\n"
                     "sw %0, 80(%3)\n"
                     "sw %2, 84(%3)\n"
                     : "=&r"(head), "=&r"(tail), "=&r"(color)
                     : "r"(vertices), "r"(p) : "memory");
    }
}

void iParMgrRenderParSys_Streak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    zParSys* s;
    RwTexture* texture;
    RwRaster* raster;
    RxObjSpace3DVertex* v3d;

    iRenderSetCameraViewMatrix(NULL);

    s = (zParSys*)data;

    texture = s->txtr_particle;
    if (texture != NULL)
    {
        raster = texture->raster;
        if (raster != NULL)
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    }

    prepare_streak_culling();
    v3d = gRenderBuffer.m_vertex;

    for (; idx != NULL; idx = idx->m_next)
    {
        if (prepare_streak_vertices(idx, 5.0f))
        {
            continue;
        }
        write_streak_vertices(v3d, idx);

        // Write the original IEEE UV words alongside the packed vertex data.
        *(U32*)&v3d[0].u = 0x3f000000;
        *(U32*)&v3d[0].v = 0x3f800000;
        *(U32*)&v3d[1].u = 0;
        *(U32*)&v3d[1].v = 0;
        *(U32*)&v3d[2].u = 0x3f800000;
        *(U32*)&v3d[2].v = 0;

        v3d += 3;

        gRenderBuffer.m_vertexCount += 3;

        if (gRenderBuffer.m_vertexCount + 3 > 477)
        {
            iRenderFlush();
            v3d = gRenderBuffer.m_vertex;
            prepare_streak_culling();
        }
    }

    iRenderFlush();
}

void iParMgrRenderParSys_InvStreak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    zParSys* s;
    RwTexture* texture;
    RwRaster* raster;
    RxObjSpace3DVertex* v3d;

    iRenderSetCameraViewMatrix(NULL);

    s = (zParSys*)data;

    texture = s->txtr_particle;
    if (texture != NULL)
    {
        raster = texture->raster;
        if (raster != NULL)
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    }
    else
    {
        RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
    }

    prepare_streak_culling();
    v3d = gRenderBuffer.m_vertex;

    for (; idx != NULL; idx = idx->m_next)
    {
        if (prepare_streak_vertices(idx, -5.0f))
        {
            continue;
        }
        write_streak_vertices(v3d, idx);

        *(U32*)&v3d[0].u = 0x3f000000;
        *(U32*)&v3d[0].v = 0x3f800000;
        *(U32*)&v3d[1].u = 0;
        *(U32*)&v3d[1].v = 0;
        *(U32*)&v3d[2].u = 0x3f800000;
        *(U32*)&v3d[2].v = 0;

        v3d += 3;

        gRenderBuffer.m_vertexCount += 3;

        if (gRenderBuffer.m_vertexCount + 3 > 477)
        {
            iRenderFlush();
            v3d = gRenderBuffer.m_vertex;
            prepare_streak_culling();
        }
    }

    iRenderFlush();
}
