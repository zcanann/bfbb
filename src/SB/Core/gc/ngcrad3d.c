#include "ngcrad3d.h"

#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <dolphin/os.h>
#include "iFMV.h"
#include <bink/include/rad3d.h>

static int GX_texture_format[RAD3DSURFACECOUNT];
static u32 Surface_info[RAD3DSURFACECOUNT];

static int Built_tables;

#define RAD3D_BYTES_PER_PIXEL_MASK 0xff
#define RAD3D_ALPHA_PIXELS 0x80000000

static void Setup_surface_array()
{
    if (Built_tables)
    {
        return;
    }

    GX_texture_format[RAD3DSURFACE32] = GX_TF_RGBA8;
    GX_texture_format[RAD3DSURFACE32A] = GX_TF_RGBA8;
    GX_texture_format[RAD3DSURFACE565] = GX_TF_RGB565;
    GX_texture_format[RAD3DSURFACE4444] = GX_TF_RGB5A3;
    GX_texture_format[RAD3DSURFACEYUY2] = -1;

    Surface_info[RAD3DSURFACE32] = 4;
    Surface_info[RAD3DSURFACE32A] = RAD3D_ALPHA_PIXELS | 4;
    Surface_info[RAD3DSURFACE565] = 2;
    Surface_info[RAD3DSURFACE4444] = RAD3D_ALPHA_PIXELS | 2;
    Surface_info[RAD3DSURFACEYUY2] = 2;

    Built_tables = 1;
}

struct RAD3DIMAGE
{
    u32 width;
    u32 height;
    s32 alpha_pixels;
    u32 bytes_per_pixel;
    u32 surface_format;
    void* pixels;
    u32 buffer_size;
    GXTexObj texobj;
};

HRAD3DIMAGE Open_RAD_3D_image(HRAD3D rad_3d, u32 width, u32 height, u32 rad3d_surface_format)
{
    RAD3DIMAGE* image;
    u32 bytes_per_pixel;

    Setup_surface_array();

    bytes_per_pixel = Surface_info[rad3d_surface_format] & RAD3D_BYTES_PER_PIXEL_MASK;
    image = (RAD3DIMAGE*)iFMVmalloc(sizeof(RAD3DIMAGE));
    if (image == 0)
    {
        image = 0;
    }
    else
    {
        image->width = width;
        image->height = height;
        image->alpha_pixels = Surface_info[rad3d_surface_format] >> 31;
        image->bytes_per_pixel = bytes_per_pixel;
        image->surface_format = rad3d_surface_format;
        image->buffer_size = GXGetTexBufferSize((u16)width, (u16)height,
                                                (GXTexFmt)GX_texture_format[rad3d_surface_format],
                                                GX_FALSE, 0);
        image->pixels = iFMVmalloc(image->buffer_size);
        GXInitTexObj(&image->texobj, image->pixels, (u16)width, (u16)height,
                     (GXTexFmt)GX_texture_format[rad3d_surface_format],
                     GX_CLAMP, GX_CLAMP, GX_FALSE);
        GXInitTexObjLOD(&image->texobj, GX_LINEAR, GX_NEAR, 0.0f, 0.0f, 0.0f,
                        GX_FALSE, GX_FALSE, GX_ANISO_1);
    }

    return image;
}

void Close_RAD_3D_image(struct RAD3DIMAGE* image)
{
    if (image != 0)
    {
        if (image->pixels != 0)
        {
            iFMVfree(image->pixels);
            image->pixels = 0;
        }
        iFMVfree(image);
    }
}

s32 Lock_RAD_3D_image(HRAD3DIMAGE rad_image, void* out_pixel_buffer, u32* out_buffer_pitch,
                      u32* out_surface_format)
{
    if (rad_image == 0)
    {
        return 0;
    }

    if (out_pixel_buffer != 0)
    {
        *(void**)(out_pixel_buffer) = rad_image->pixels;
    }

    if (out_buffer_pitch != 0)
    {
        *out_buffer_pitch = rad_image->width * rad_image->bytes_per_pixel;
    }

    if (out_surface_format != 0)
    {
        *out_surface_format = rad_image->surface_format;
    }

    return 1;
}

void Unlock_RAD_3D_image(HRAD3DIMAGE rad_image)
{
    if (rad_image != 0)
    {
        DCStoreRange(rad_image->pixels, rad_image->buffer_size);
    }
}

static void Submit_vertices(F32 x_offset, F32 y_offset, F32 x_scale, F32 y_scale, long width,
                            long height, F32 alpha_level)
{
    s16 left;
    s16 top;
    s16 right;
    u8 alpha;
    s16 bottom;

    GXSetCullMode(GX_CULL_NONE);
    GXSetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    GXSetColorUpdate(GX_TRUE);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    left = (long)x_offset;
    bottom = (long)(height * y_scale + y_offset);
    GXPosition3s16(left, bottom, 0);
    alpha = (long)(255.0f * alpha_level);
    GXColor4u8(255, 255, 255, alpha);
    GXTexCoord2f32(0.0f, 1.0f);

    top = (long)y_offset;
    GXPosition3s16(left, top, 0);
    GXColor4u8(255, 255, 255, alpha);
    GXTexCoord2f32(0.0f, 0.0f);

    right = (long)(width * x_scale + x_offset);
    GXPosition3s16(right, top, 0);
    GXColor4u8(255, 255, 255, alpha);
    GXTexCoord2f32(1.0f, 0.0f);

    GXPosition3s16(right, bottom, 0);
    GXColor4u8(255, 255, 255, alpha);
    GXTexCoord2f32(1.0f, 1.0f);

    GXEnd();
}

void Blit_RAD_3D_image(HRAD3DIMAGE rad_image, F32 x_offset, F32 y_offset, F32 x_scale, F32 y_scale,
                       F32 alpha_level)
{
    Mtx tex_mtx;
    F32 screen_width;
    F32 screen_height;

    if (rad_image == 0)
    {
        return;
    }

    if (alpha_level >= 0.998f)
    {
        if (rad_image->alpha_pixels == 0)
        {
            GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
        }
        else
        {
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_KONST, GX_CA_ZERO);
        }
    }
    else
    {
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

        if (rad_image->alpha_pixels == 0)
        {
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_RASA, GX_CA_KONST, GX_CA_ZERO);
        }
        else
        {
            GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
        }
    }

    GXLoadTexObj(&rad_image->texobj, GX_TEXMAP0);

    PSMTXScale(tex_mtx, 1.0f / rad_image->width, 1.0f / rad_image->height, 1.0f);
    PSMTXScale(tex_mtx, 1.0f, 1.0f, 1.0f);
    GXLoadTexMtxImm(tex_mtx, GX_TEXMTX0, GX_MTX2x4);

    GXSetNumTexGens(1);
    GXSetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_TEXMTX0);

    // Retail ignores the caller's offsets and scales and always covers the
    // whole 640x480 screen.
    screen_width = 640.0f;
    screen_height = 480.0f;
    Submit_vertices(0.0f, 0.0f, 1.0f, 1.0f, screen_width, screen_height, alpha_level);

    GXSetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
}
