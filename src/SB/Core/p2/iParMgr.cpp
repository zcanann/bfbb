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
    U32 transformFlags = rwIM3D_VERTEXUV;
    if (RwIm3DTransform((RwIm3DVertex *)data, dataSize, NULL, transformFlags) != NULL)
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

    gRenderBuffer.m_vertexCount = 0;
    gRenderBuffer.m_indexCount  = 0;
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


namespace
{
    inline void prepare_ground_culling()
    {
        asm volatile("lqc2 vf14, 0(%0)\n"
                     "lqc2 vf15, 16(%0)\n"
                     "lqc2 vf16, 32(%0)\n"
                     "lqc2 vf17, 48(%0)\n"
                     : : "r"(globals.camera.frustplane) : "memory");
    }

    inline S32 ground_outside_side_planes(const xPar* p)
    {
        S32 outside;
        asm volatile("lqc2 vf8, 16(%1)\n"
                     "vaddaw.xyzw ACC, vf17, vf8w\n"
                     "vmsubax.xyzw ACC, vf14, vf8x\n"
                     "vmsubay.xyzw ACC, vf15, vf8y\n"
                     "vmsubz.xyzw vf1, vf16, vf8z\n"
                     "qmfc2 %0, vf1\n"
                     "pcgtw %0, $0, %0\n"
                     "ppach %0, $0, %0\n"
                     "vmul.w vf7, vf8, vf10\n"
                     : "=&r"(outside) : "r"(p) : "memory");
        return outside;
    }
}

// Original coefficient table owned by the PS2 math implementation.
extern F32 cosSinPolynomial[];

namespace
{
    inline void ground_euler(xMat3x3* m, F32 yaw, F32 pitch, F32 roll)
    {
        // Particle angles begin in [0, 2*pi]; the polynomial takes [-pi, pi].
        if (yaw > PI)
            yaw -= 2.0f * PI;
        if (pitch > PI)
            pitch -= 2.0f * PI;
        if (roll > PI)
            roll -= 2.0f * PI;

        U32 scratch;
        asm volatile("lqc2 vf1, 0(%1)\n"
                     "lqc2 vf2, 16(%1)\n"
                     "lqc2 vf3, 32(%1)\n"
                     "lqc2 vf4, 48(%1)\n"
                     "vaddx.xyz vf20, vf0, vf1x\n"
                     "vaddy.xyz vf21, vf0, vf1y\n"
                     "vaddz.xyz vf22, vf0, vf1z\n"
                     "vaddw.xyz vf23, vf0, vf1w\n"
                     "vaddx.xyz vf24, vf0, vf2x\n"
                     "vaddy.xyz vf25, vf0, vf2y\n"
                     "vaddz.xyz vf26, vf0, vf2z\n"
                     "vaddw.xyz vf27, vf0, vf2w\n"
                     "vaddx.xyz vf28, vf0, vf3x\n"
                     "vaddy.xyz vf29, vf0, vf3y\n"
                     "vaddz.xyz vf30, vf0, vf3z\n"
                     "vaddw.xyz vf31, vf0, vf3w\n"
                     "vaddx.xyz vf18, vf0, vf4x\n"
                     "vaddy.xyz vf19, vf0, vf4y\n"
                     "mfc1 %0, %3\n"
                     "qmtc2 %0, vf1\n"
                     "mfc1 %0, %4\n"
                     "qmtc2 %0, vf2\n"
                     "mfc1 %0, %5\n"
                     "qmtc2 %0, vf3\n"
                     "vaddx.y vf1, vf0, vf2x\n"
                     "vaddx.z vf1, vf0, vf3x\n"
                     "vmul.xyz vf2, vf1, vf1\n"
                     "vmul.xyz vf5, vf2, vf25\n"
                     "vmula.xyz ACC, vf2, vf24\n"
                     "vmul.xyz vf3, vf2, vf2\n"
                     "vmadd.xyz vf5, vf2, vf5\n"
                     "vmula.xyz ACC, vf2, vf22\n"
                     "vmadd.xyz vf6, vf3, vf23\n"
                     "vadda.xyz ACC, vf0, vf19\n"
                     "vmul.xyz vf4, vf3, vf3\n"
                     "vmadda.xyz ACC, vf2, vf20\n"
                     "vmadda.xyz ACC, vf3, vf21\n"
                     "vmadda.xyz ACC, vf3, vf6\n"
                     "vmadd.xyz vf8, vf4, vf5\n"
                     "vmula.xyz ACC, vf3, vf31\n"
                     "vmadd.xyz vf5, vf2, vf30\n"
                     "vmula.xyz ACC, vf3, vf29\n"
                     "vmadd.xyz vf6, vf2, vf28\n"
                     "vadda.xyz ACC, vf0, vf18\n"
                     "vmadd.xyz vf8, vf8, vf2\n"
                     "vmadda.xyz ACC, vf3, vf27\n"
                     "vmadda.xyz ACC, vf2, vf26\n"
                     "vmadda.xyz ACC, vf3, vf6\n"
                     "vmadd.xyz vf9, vf4, vf5\n"
                     "vmul.xyz vf9, vf9, vf1\n"
                     "vaddx.x vf1, vf0, vf8x\n"
                     "vaddx.z vf12, vf0, vf8x\n"
                     "vopmula.xyz ACC, vf8, vf9\n"
                     "vmadd.xyz vf3, vf0, vf0\n"
                     "vmuly.x vf3, vf9, vf9y\n"
                     "vadd.y vf3, vf0, vf8\n"
                     "vsubx.z vf1, vf0, vf9x\n"
                     "vaddx.x vf12, vf0, vf9x\n"
                     "vsuby.y vf12, vf0, vf9y\n"
                     "vmulaz.xyz ACC, vf3, vf9z\n"
                     "vmaddaz.xz ACC, vf1, vf8z\n"
                     "vmadd.xyz vf10, vf0, vf0\n"
                     "vmulaz.xyz ACC, vf3, vf8z\n"
                     "vmsubaz.xz ACC, vf1, vf9z\n"
                     "vmadd.xyz vf11, vf0, vf0\n"
                     "vmuly.xz vf12, vf12, vf8y\n"
                     "sqc2 vf10, 0(%2)\n"
                     "sqc2 vf11, 16(%2)\n"
                     "sqc2 vf12, 32(%2)\n"
                     : "=&r"(scratch)
                     : "r"(cosSinPolynomial), "r"(m), "f"(yaw), "f"(pitch), "f"(roll)
                     : "memory");
        m->flags = 0;
    }
}

void iParMgrRenderParSys_Ground(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    zParSys* s;
    RwTexture* texture;
    RwRaster* raster;
    xParCmdTex* tex;
    static RxObjSpace3DVertex v3d[4];
    static U16 i3d[6] = { 0, 1, 2, 3, 0, 1 };

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

    prepare_ground_culling();

    for (; idx != NULL; idx = idx->m_next)
    {
        tex = ps->m_cmdTex;
        void* vertices = v3d;
        U16* indices = i3d;
        S32 vertexCount = 4;
        S32 indexCount = 6;

        if ((indexCount + gRenderBuffer.m_indexCount > 960) ||
            (vertexCount + gRenderBuffer.m_vertexCount > 480))
        {
            iRenderFlush();
            prepare_ground_culling();
        }

        F32 size = 0.5f * idx->m_size;
        if (ground_outside_side_planes(idx))
        {
            continue;
        }

        U8 r = idx->m_c[0];
        U8 g = idx->m_c[1];
        U8 b = idx->m_c[2];
        U8 a = idx->m_c[3];
        xMat3x3 groundmat;
        F32 angx = 0.0f;
        F32 angy = 0.0f;
        F32 angz = 0.0f;

        if (idx->m_rotdeg[0])
        {
            angx = 6.2831855f * (idx->m_rotdeg[0] / 255.0f);
        }

        if (idx->m_rotdeg[1])
        {
            angy = 6.2831855f * (idx->m_rotdeg[1] / 255.0f);
        }

        if (idx->m_rotdeg[2])
        {
            angz = 6.2831855f * (idx->m_rotdeg[2] / 255.0f);
        }

        ground_euler(&groundmat, angx, angy, angz);

        xVec3 vert[4];
        xVec3 zdir = { groundmat.at.x * size, groundmat.at.y * size, groundmat.at.z * size };
        xVec3 xdir = { groundmat.right.x * size, groundmat.right.y * size,
                       groundmat.right.z * size };
        xVec3 centre = { idx->m_pos.x, idx->m_pos.y, idx->m_pos.z };

        vert[0].x = centre.x - xdir.x - zdir.x;
        vert[0].y = centre.y - xdir.y - zdir.y;
        vert[0].z = centre.z - xdir.z - zdir.z;
        vert[1].x = zdir.x + (centre.x + xdir.x);
        vert[1].y = zdir.y + (centre.y + xdir.y);
        vert[1].z = zdir.z + (centre.z + xdir.z);
        vert[2].x = centre.x + xdir.x - zdir.x;
        vert[2].y = centre.y + xdir.y - zdir.y;
        vert[2].z = centre.z + xdir.z - zdir.z;
        vert[3].x = zdir.x + (centre.x - xdir.x);
        vert[3].y = zdir.y + (centre.y - xdir.y);
        vert[3].z = zdir.z + (centre.z - xdir.z);

        RwIm3DVertexSetRGBA(&v3d[0], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[1], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[2], r, g, b, a);
        RwIm3DVertexSetRGBA(&v3d[3], r, g, b, a);
        RwIm3DVertexSetPos(&v3d[0], vert[0].x, vert[0].y, vert[0].z);
        RwIm3DVertexSetPos(&v3d[1], vert[1].x, vert[1].y, vert[1].z);
        RwIm3DVertexSetPos(&v3d[2], vert[2].x, vert[2].y, vert[2].z);
        RwIm3DVertexSetPos(&v3d[3], vert[3].x, vert[3].y, vert[3].z);

        if (tex != NULL)
        {
            F32 u1 = tex->x1 + idx->m_texIdx[0] * tex->unit_width;
            F32 v1 = tex->y1 + idx->m_texIdx[1] * tex->unit_height;
            F32 u2 = tex->x1 + (idx->m_texIdx[0] + 1) * tex->unit_width;
            F32 v2 = tex->y1 + (idx->m_texIdx[1] + 1) * tex->unit_height;

            v3d[0].u = u1;
            v3d[0].v = v1;
            v3d[1].u = u2;
            v3d[2].u = u2;
            v3d[1].v = v2;
            v3d[2].v = v1;
            v3d[3].u = u1;
            v3d[3].v = v2;
        }
        else
        {
            v3d[0].u = 0.0f;
            v3d[0].v = 0.0f;
            v3d[1].u = 1.0f;
            v3d[1].v = 1.0f;
            v3d[2].u = 1.0f;
            v3d[2].v = 0.0f;
            v3d[3].u = 0.0f;
            v3d[3].v = 1.0f;
        }

        U16* src = indices;
        U16* dst = &gRenderBuffer.m_index[gRenderBuffer.m_indexCount];
        for (S32 i = 0; i < indexCount; i++)
        {
            *dst++ = gRenderBuffer.m_vertexCount + *src++;
        }
        memcpy((U8*)gRenderBuffer.m_vertex +
                   gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount,
               vertices, gRenderBuffer.m_vertexTypeSize * vertexCount);
        gRenderBuffer.m_indexCount += indexCount;
        gRenderBuffer.m_vertexCount += vertexCount;
    }

    iRenderFlush();
}


namespace
{
    inline void prepare_sprite_culling()
    {
        prepare_ground_culling();
        // Keep camera right/up in xyz; vf10.w supplies the half-size scale.
        U32 half = 0x3f000000;
        asm volatile("qmtc2 %0, vf9\n"
                     "lqc2 vf10, 0(%1)\n"
                     "lqc2 vf11, 0(%2)\n"
                     "vmulx.w vf10, vf0, vf9x\n"
                     : : "r"(half), "r"(&gRenderBuffer.m_camViewR),
                         "r"(&gRenderBuffer.m_camViewU) : "memory");
    }

    inline S32 sprite_outside_side_planes(const xPar* p, const xPar* next, U32& color)
    {
        asm volatile("lqc2 vf8, 16(%1)\n"
                     "lw %0, 12(%1)\n"
                     "vaddaw.xyzw ACC, vf17, vf8w\n"
                     "vmsubax.xyzw ACC, vf14, vf8x\n"
                     "vmsubay.xyzw ACC, vf15, vf8y\n"
                     "vmsubz.xyzw vf1, vf16, vf8z\n"
                     "vmul.w vf7, vf8, vf10\n"
                     : "=&r"(color) : "r"(p) : "memory");
        if (next)
        {
            asm volatile("pref 0, 0(%0)\n" : : "r"(next));
        }
        S32 outside;
        asm volatile("qmfc2 %0, vf1\n"
                     "pcgtw %0, $0, %0\n"
                     "ppach %0, $0, %0\n"
                     "vadda.xyz ACC, vf0, vf8\n"
                     : "=&r"(outside) : : "memory");
        return outside;
    }

    inline void write_sprite_vertices(RxObjSpace3DVertex* vertices, U32 color)
    {
        U64 first, second;
        asm volatile("vmsubaw.xyz ACC, vf11, vf7w\n"
                     "vmsubw.xyz vf1, vf10, vf7w\n"
                     "vmaddw.xyz vf3, vf10, vf7w\n"
                     "vadda.xyz ACC, vf0, vf8\n"
                     "vmaddaw.xyz ACC, vf11, vf7w\n"
                     "vmaddw.xyz vf2, vf10, vf7w\n"
                     "vmsubw.xyz vf4, vf10, vf7w\n"
                     "qmfc2 %0, vf1\n"
                     "qmfc2 %1, vf2\n"
                     "sd %0, 0(%2)\n"
                     "pcpyud %0, %0, $0\n"
                     "sw %0, 8(%2)\n"
                     "sw %3, 12(%2)\n"
                     "sw %1, 36(%2)\n"
                     "prot3w %1, %1\n"
                     "sd %1, 40(%2)\n"
                     "sw %3, 48(%2)\n"
                     "qmfc2 %0, vf3\n"
                     "qmfc2 %1, vf4\n"
                     "sd %0, 72(%2)\n"
                     "pcpyud %0, %0, $0\n"
                     "sw %0, 80(%2)\n"
                     "sw %3, 84(%2)\n"
                     "sw %1, 108(%2)\n"
                     "prot3w %1, %1\n"
                     "sd %1, 112(%2)\n"
                     "sw %3, 120(%2)\n"
                     : "=&r"(first), "=&r"(second)
                     : "r"(vertices), "r"(color) : "memory");
    }
}

void iParMgrRenderParSys_Sprite(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    zParSys* s;
    RwTexture* texture;
    RwRaster* raster;
    S32 indexCount;
    S32 vertexCount;
    U16* i3d;
    RxObjSpace3DVertex* v3d;
    xParCmdTex* tex;
    U32 pivot;

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

    tex = ps->m_cmdTex;

    indexCount = 0;
    vertexCount = 0;
    i3d = gRenderBuffer.m_index;
    v3d = gRenderBuffer.m_vertex;
    pivot = s->tasset->parFlags;

    xVec3 offset = {};
    if (pivot & 0x8)
    {
        offset += *(xVec3*)&gRenderBuffer.m_camViewR * 0.5f;
    }
    else if (pivot & 0x20)
    {
        offset -= *(xVec3*)&gRenderBuffer.m_camViewR * 0.5f;
    }
    if (pivot & 0x10)
    {
        offset += *(xVec3*)&gRenderBuffer.m_camViewU * 0.5f;
    }
    else if (pivot & 0x40)
    {
        offset -= *(xVec3*)&gRenderBuffer.m_camViewU * 0.5f;
    }
    prepare_sprite_culling();

    while (idx != NULL)
    {
        xPar* p = idx;
        idx = idx->m_next;

        xVec3 pivotOffset;
        if (pivot & 0x78)
        {
            // Retail applies the pivot to the particle and subtracts it again
            // after rendering or culling, preserving that operation order.
            pivotOffset = offset * p->m_size;
            p->m_pos += pivotOffset;
        }

        U32 color;
        if (!sprite_outside_side_planes(p, idx, color))
        {
            write_sprite_vertices(v3d, color);

            if (tex != NULL)
            {
                F32 u1 = tex->x1 + p->m_texIdx[0] * tex->unit_width;
                F32 v1 = tex->y1 + p->m_texIdx[1] * tex->unit_height;
                F32 u2 = tex->x1 + (p->m_texIdx[0] + 1) * tex->unit_width;
                F32 v2 = tex->y1 + (p->m_texIdx[1] + 1) * tex->unit_height;

                v3d[0].u = u1;
                v3d[0].v = v1;
                v3d[1].u = u2;
                v3d[1].v = v2;
                v3d[2].u = u2;
                v3d[2].v = v1;
                v3d[3].u = u1;
                v3d[3].v = v2;
            }
            else
            {
                v3d[0].u = 0.0f;
                v3d[0].v = 0.0f;
                v3d[1].u = 1.0f;
                v3d[1].v = 1.0f;
                v3d[2].u = 1.0f;
                v3d[2].v = 0.0f;
                v3d[3].u = 0.0f;
                v3d[3].v = 1.0f;
            }

            i3d[0] = vertexCount;
            i3d[1] = vertexCount + 1;
            i3d[2] = vertexCount + 2;
            i3d[3] = vertexCount + 3;
            i3d[4] = vertexCount;
            i3d[5] = vertexCount + 1;

            i3d += 6;
            v3d += 4;
            indexCount += 6;
            vertexCount += 4;

            if ((indexCount > 960 - 6) || (vertexCount > 480 - 4))
            {
                gRenderBuffer.m_indexCount = indexCount;
                gRenderBuffer.m_vertexCount = vertexCount;
                v3d = gRenderBuffer.m_vertex;
                i3d = gRenderBuffer.m_index;
                iRenderFlush();
                indexCount = 0;
                vertexCount = 0;
                prepare_sprite_culling();
            }
        }
        if (pivot & 0x78)
        {
            p->m_pos -= pivotOffset;
        }
    }

    gRenderBuffer.m_indexCount = indexCount;
    gRenderBuffer.m_vertexCount = vertexCount;
    iRenderFlush();
}
