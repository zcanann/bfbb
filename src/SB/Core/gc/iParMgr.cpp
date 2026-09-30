#include "iParMgr.h"
#include "zGlobals.h"
#include "zParSys.h"

#include <types.h>
#include <xstransvc.h>
#include <rwplcore.h>

tagiRenderArrays gRenderArr;
tagiRenderInput gRenderBuffer;

static S32 gColorTableInit;
static F32 gColorTable[256];

// mwcceppc 2.0p1/2.0p1a ceiling: retail reloads the int-to-float bias constant
// and 255.0f from .sdata2 inside every one of the eight unrolled copies,
// because its CSE treats the stfs to gColorTable as possibly aliasing the
// literal pool. Both the stock and the patched compiler here hoist those two
// loads out of the loop body instead, which is the only difference in the
// function. Not reachable from the source: verified against a pointer walk,
// a while loop, an unsigned induction variable and a volatile table.
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
    xVec3 offset[4];

    iRenderSetCameraViewMatrix(NULL);

    s = (zParSys*)data;

    texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture != NULL)
    {
        raster = texture->raster;
        if (raster != NULL)
        {
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
        }
    }

    tex = ps->m_cmdTex;
    pivot = s->tasset->parFlags;

    indexCount = 0;
    vertexCount = 0;
    i3d = gRenderBuffer.m_index;
    v3d = gRenderBuffer.m_vertex;

    if (pivot & 0x8)
    {
        offset[0] = offset[3] = 0.0f;
        offset[1] = offset[2] = *(xVec3*)&gRenderBuffer.m_camViewR;
    }
    else if (pivot & 0x20)
    {
        offset[0] = offset[3] = -*(xVec3*)&gRenderBuffer.m_camViewR;
        offset[1] = offset[2] = 0.0f;
    }
    else
    {
        offset[1] = offset[2] = *(xVec3*)&gRenderBuffer.m_camViewR * 0.5f;
        offset[0] = offset[3] = -offset[1];
    }

    if (pivot & 0x10)
    {
        offset[0] += *(xVec3*)&gRenderBuffer.m_camViewU;
        offset[2] += *(xVec3*)&gRenderBuffer.m_camViewU;
    }
    else if (pivot & 0x40)
    {
        offset[1] -= *(xVec3*)&gRenderBuffer.m_camViewU;
        offset[3] -= *(xVec3*)&gRenderBuffer.m_camViewU;
    }
    else
    {
        xVec3 temp_offset = *(xVec3*)&gRenderBuffer.m_camViewU * 0.5f;
        offset[1] += temp_offset;
        offset[3] += temp_offset;
        offset[0] -= temp_offset;
        offset[2] -= temp_offset;
    }

    while (idx != NULL)
    {
        xPar* p = idx;
        idx = idx->m_next;

        U8 r = p->m_c[0];
        U8 g = p->m_c[1];
        U8 b = p->m_c[2];
        U8 a = p->m_c[3];
        F32 size = p->m_size;
        F32 px = p->m_pos.x;
        F32 py = p->m_pos.y;
        F32 pz = p->m_pos.z;

        // Written a component at a time across all four corners, not a corner
        // at a time: retail loads offset[0..3].x, then [0..3].y, then [0..3].z,
        // in exactly that order, and this compiler emits loads in source order.
        v3d[0].x = offset[0].x * size + px;
        v3d[1].x = offset[1].x * size + px;
        v3d[2].x = offset[2].x * size + px;
        v3d[3].x = offset[3].x * size + px;
        v3d[0].y = offset[0].y * size + py;
        v3d[1].y = offset[1].y * size + py;
        v3d[2].y = offset[2].y * size + py;
        v3d[3].y = offset[3].y * size + py;
        v3d[0].z = offset[0].z * size + pz;
        v3d[1].z = offset[1].z * size + pz;
        v3d[2].z = offset[2].z * size + pz;
        v3d[3].z = offset[3].z * size + pz;

        v3d[0].r = r;
        v3d[0].g = g;
        v3d[0].b = b;
        v3d[0].a = a;
        v3d[1].r = r;
        v3d[1].g = g;
        v3d[1].b = b;
        v3d[1].a = a;
        v3d[2].r = r;
        v3d[2].g = g;
        v3d[2].b = b;
        v3d[2].a = a;
        v3d[3].r = r;
        v3d[3].g = g;
        v3d[3].b = b;
        v3d[3].a = a;

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
        }
    }

    gRenderBuffer.m_indexCount = indexCount;
    gRenderBuffer.m_vertexCount = vertexCount;
    iRenderFlush();
}

void iRenderInit()
{
    gRenderBuffer.m_mode = 0;
    gRenderBuffer.m_indexCount  = 0;
    gRenderBuffer.m_vertexCount = 0;
    gRenderBuffer.m_vertexTypeSize = 0x24;
    gRenderBuffer.m_index = &gRenderArr.m_index[0];
    gRenderBuffer.m_vertex = &gRenderArr.m_vertex[0];
    gRenderBuffer.m_vertexTZ = gRenderArr.m_vertexTZ;
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

    v3d[0].r = r;
    v3d[0].g = g;
    v3d[0].b = b;
    v3d[0].a = a;
    v3d[1].r = r;
    v3d[1].g = g;
    v3d[1].b = b;
    v3d[1].a = a;
    v3d[2].r = r;
    v3d[2].g = g;
    v3d[2].b = b;
    v3d[2].a = a;
    v3d[3].r = r;
    v3d[3].g = g;
    v3d[3].b = b;
    v3d[3].a = a;

    v3d[0].x = px - dx;
    v3d[0].y = py - dy;
    v3d[0].z = pz - dz;

    tx -= dx;
    ty -= dy;
    tz -= dz;
    ax += dx;
    ay += dy;
    az += dz;

    v3d[1].x = tx;
    v3d[1].y = ty;
    v3d[1].z = tz;

    v3d[2].x = ax;
    v3d[2].y = ay;
    v3d[2].z = az;

    v3d[3].x = px + dx;
    v3d[3].y = py + dy;
    v3d[3].z = pz + dz;

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
    xMat3x3 groundmat;
    F32 yaw = 0.0f;

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

    v3d[0].r = r;
    v3d[0].g = g;
    v3d[0].b = b;
    v3d[0].a = a;
    v3d[1].r = r;
    v3d[1].g = g;
    v3d[1].b = b;
    v3d[1].a = a;
    v3d[2].r = r;
    v3d[2].g = g;
    v3d[2].b = b;
    v3d[2].a = a;
    v3d[3].r = r;
    v3d[3].g = g;
    v3d[3].b = b;
    v3d[3].a = a;

    F32 mx = px - xdx;
    F32 mz = pz - xdz;
    F32 sx = px + xdx;
    F32 sz = pz + xdz;

    v3d[0].x = mx - zdx;
    v3d[0].y = py;
    v3d[0].z = mz - zdz;

    v3d[1].x = sx - zdx;
    v3d[1].y = py;
    v3d[1].z = sz - zdz;

    v3d[2].x = zdx + sx;
    v3d[2].y = py;
    v3d[2].z = zdz + sz;

    v3d[3].x = zdx + mx;
    v3d[3].y = py;
    v3d[3].z = zdz + mz;

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

void iRenderFlush()
{
    if (gRenderBuffer.m_vertexCount > 0)
    {
        iRenderTrianglesImmediate(gRenderBuffer.m_vertexType, gRenderBuffer.m_vertexTypeSize, gRenderBuffer.m_vertex, gRenderBuffer.m_vertexCount, gRenderBuffer.m_index, gRenderBuffer.m_indexCount);
    }

    gRenderBuffer.m_indexCount  = 0;
    gRenderBuffer.m_vertexCount = 0;
}

void iRenderTrianglesImmediate(S32 vertType, S32 vertTypeSize, void* data, S32 dataSize, U16* index, S32 indexSize)
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

void iParMgrRenderParSys_Streak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    zParSys* s;
    RwTexture* texture;
    RwRaster* raster;
    RxObjSpace3DVertex* v3d;

    iRenderSetCameraViewMatrix(NULL);

    s = (zParSys*)data;

    texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
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

    v3d = gRenderBuffer.m_vertex;

    for (; idx != NULL; idx = idx->m_next)
    {
        // See iRenderPushQuadStreak: the tail is walked in place for one edge
        // and added into a copy of itself for the other, which is what retail's
        // three `fmr` copies encode.  Declaration order is the FP colour order.
        F32 dx, dy, dz;
        F32 tx, ty, tz;
        F32 ax, ay, az;
        F32 px, py, pz;
        F32 size;

        px = idx->m_pos.x;
        py = idx->m_pos.y;
        pz = idx->m_pos.z;
        tx = px - 5.0f * idx->m_vel.x;
        ty = py - 5.0f * idx->m_vel.y;
        tz = pz - 5.0f * idx->m_vel.z;
        ax = tx;
        ay = ty;
        az = tz;
        size = idx->m_size;
        dx = size * gRenderBuffer.m_camViewR.x;
        dy = size * gRenderBuffer.m_camViewR.y;
        dz = size * gRenderBuffer.m_camViewR.z;

        tx -= dx;
        ty -= dy;
        tz -= dz;
        ax += dx;
        ay += dy;
        az += dz;

        v3d[0].x = ax;
        v3d[0].y = ay;
        v3d[0].z = az;

        v3d[1].x = tx;
        v3d[1].y = ty;
        v3d[1].z = tz;

        v3d[2].x = px;
        v3d[2].y = py;
        v3d[2].z = pz;

        U8 r = idx->m_c[0];
        U8 g = idx->m_c[1];
        U8 b = idx->m_c[2];
        U8 a = idx->m_c[3];

        v3d[0].r = r;
        v3d[0].g = g;
        v3d[0].b = b;
        v3d[0].a = a;
        v3d[1].r = r;
        v3d[1].g = g;
        v3d[1].b = b;
        v3d[1].a = a;
        v3d[2].r = r;
        v3d[2].g = g;
        v3d[2].b = b;
        v3d[2].a = a;

        v3d[2].u = 0.5f;
        v3d[2].v = 1.0f;
        v3d[1].u = 0.0f;
        v3d[1].v = 0.0f;
        v3d[0].u = 1.0f;
        v3d[0].v = 0.0f;

        v3d += 3;

        gRenderBuffer.m_vertexCount += 3;

        if (gRenderBuffer.m_vertexCount + 3 > 477)
        {
            iRenderFlush();
            v3d = gRenderBuffer.m_vertex;
        }
    }

    iRenderFlush();
}

// Identical to iParMgrRenderParSys_Streak in the retail object: the two
// functions differ only in which FPRs the allocator picked for the y lane.
void iParMgrRenderParSys_InvStreak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    zParSys* s;
    RwTexture* texture;
    RwRaster* raster;
    RxObjSpace3DVertex* v3d;

    iRenderSetCameraViewMatrix(NULL);

    s = (zParSys*)data;

    texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
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

    v3d = gRenderBuffer.m_vertex;

    for (; idx != NULL; idx = idx->m_next)
    {
        // See iRenderPushQuadStreak: the tail is walked in place for one edge
        // and added into a copy of itself for the other, which is what retail's
        // three `fmr` copies encode.  Declaration order is the FP colour order.
        F32 dx, dy, dz;
        F32 tx, ty, tz;
        F32 ax, ay, az;
        F32 px, py, pz;
        F32 size;

        px = idx->m_pos.x;
        py = idx->m_pos.y;
        pz = idx->m_pos.z;
        tx = px - 5.0f * idx->m_vel.x;
        ty = py - 5.0f * idx->m_vel.y;
        tz = pz - 5.0f * idx->m_vel.z;
        ax = tx;
        ay = ty;
        az = tz;
        size = idx->m_size;
        dx = size * gRenderBuffer.m_camViewR.x;
        dy = size * gRenderBuffer.m_camViewR.y;
        dz = size * gRenderBuffer.m_camViewR.z;

        tx -= dx;
        ty -= dy;
        tz -= dz;
        ax += dx;
        ay += dy;
        az += dz;

        v3d[0].x = ax;
        v3d[0].y = ay;
        v3d[0].z = az;

        v3d[1].x = tx;
        v3d[1].y = ty;
        v3d[1].z = tz;

        v3d[2].x = px;
        v3d[2].y = py;
        v3d[2].z = pz;

        U8 r = idx->m_c[0];
        U8 g = idx->m_c[1];
        U8 b = idx->m_c[2];
        U8 a = idx->m_c[3];

        v3d[0].r = r;
        v3d[0].g = g;
        v3d[0].b = b;
        v3d[0].a = a;
        v3d[1].r = r;
        v3d[1].g = g;
        v3d[1].b = b;
        v3d[1].a = a;
        v3d[2].r = r;
        v3d[2].g = g;
        v3d[2].b = b;
        v3d[2].a = a;

        v3d[2].u = 0.5f;
        v3d[2].v = 1.0f;
        v3d[1].u = 0.0f;
        v3d[1].v = 0.0f;
        v3d[0].u = 1.0f;
        v3d[0].v = 0.0f;

        v3d += 3;

        gRenderBuffer.m_vertexCount += 3;

        if (gRenderBuffer.m_vertexCount + 3 > 477)
        {
            iRenderFlush();
            v3d = gRenderBuffer.m_vertex;
        }
    }

    iRenderFlush();
}

void iParMgrRenderParSys_QuadStreak(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    zParSys* s = (zParSys*)data;
    iRenderSetCameraViewMatrix(NULL);
    RwTexture* texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture != NULL)
    {
        RwRaster* raster = texture->raster;
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

void iParMgrRenderParSys_Static(void*, xParGroup*) { }

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

    texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
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
        tex = ps->m_cmdTex;

        if ((gRenderBuffer.m_indexCount + 6 > 960) || (gRenderBuffer.m_vertexCount + 4 > 480))
        {
            iRenderFlush();
        }

        U8 r = idx->m_c[0];
        U8 g = idx->m_c[1];
        U8 b = idx->m_c[2];
        U8 a = idx->m_c[3];
        F32 size = 0.5f * idx->m_size;
        xMat3x3 groundmat;
        F32 angz = 0.0f;
        F32 angy = 0.0f;
        F32 angx = 0.0f;

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

        xMat3x3Euler(&groundmat, angx, angy, angz);

        // Declared bare: mwcc colours FP values in declaration order and retail
        // needs the "at" row's products coloured before the "right" row's.
        // Declared in definition order they spill one extra callee-saved FPR
        // and the frame grows by 0x10.
        F32 zdx, zdy, zdz;
        F32 xdx, xdy, xdz;
        F32 px, py, pz;

        xdx = groundmat.right.x * size;
        xdy = groundmat.right.y * size;
        xdz = groundmat.right.z * size;
        px = idx->m_pos.x;
        py = idx->m_pos.y;
        pz = idx->m_pos.z;
        zdx = groundmat.at.x * size;
        zdy = groundmat.at.y * size;
        zdz = groundmat.at.z * size;

        v3d[0].r = r;
        v3d[0].g = g;
        v3d[0].b = b;
        v3d[0].a = a;
        v3d[1].r = r;
        v3d[1].g = g;
        v3d[1].b = b;
        v3d[1].a = a;
        v3d[2].r = r;
        v3d[2].g = g;
        v3d[2].b = b;
        v3d[2].a = a;
        v3d[3].r = r;
        v3d[3].g = g;
        v3d[3].b = b;
        v3d[3].a = a;

        F32 mx = px - xdx;
        F32 my = py - xdy;
        F32 mz = pz - xdz;
        F32 sx = px + xdx;
        F32 sy = py + xdy;
        F32 sz = pz + xdz;

        v3d[0].x = mx - zdx;
        v3d[0].y = my - zdy;
        v3d[0].z = mz - zdz;

        v3d[1].x = zdx + sx;
        v3d[1].y = zdy + sy;
        v3d[1].z = zdz + sz;

        v3d[2].x = sx - zdx;
        v3d[2].y = sy - zdy;
        v3d[2].z = sz - zdz;

        v3d[3].x = zdx + mx;
        v3d[3].y = zdy + my;
        v3d[3].z = zdz + mz;

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

        U16* indices = &gRenderBuffer.m_index[gRenderBuffer.m_indexCount];
        indices[0] = gRenderBuffer.m_vertexCount + i3d[0];
        indices[1] = gRenderBuffer.m_vertexCount + i3d[1];
        indices[2] = gRenderBuffer.m_vertexCount + i3d[2];
        indices[3] = gRenderBuffer.m_vertexCount + i3d[3];
        indices[4] = gRenderBuffer.m_vertexCount + i3d[4];
        indices[5] = gRenderBuffer.m_vertexCount + i3d[5];

        void* vertices = (U8*)gRenderBuffer.m_vertex +
                         gRenderBuffer.m_vertexTypeSize * gRenderBuffer.m_vertexCount;
        memcpy(vertices, v3d, gRenderBuffer.m_vertexTypeSize * 4);

        gRenderBuffer.m_indexCount += 6;
        gRenderBuffer.m_vertexCount += 4;
    }

    iRenderFlush();
}

void iParMgrRenderParSys_Flat(void* data, xParGroup* ps)
{
    xPar* idx = ps->m_root;
    zParSys* s = (zParSys*)data;
    iRenderSetCameraViewMatrix(NULL);
    RwTexture* texture = (RwTexture*)xSTFindAsset(s->tasset->textureID, NULL);
    if (texture != NULL)
    {
        RwRaster* raster = texture->raster;
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