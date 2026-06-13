#ifndef _BINKTEXTURES_H_
#define _BINKTEXTURES_H_
#include "bink.h"
#include "dolphin/gx.h"

/*
 * RAD's high level API for using 3D hardware to do color conversion.
 *
 * Playback allocates platform textures, registers the resulting
 * BINKFRAMEBUFFERS with Bink, waits for the GPU before decoding, syncs the
 * decoded texture memory after BinkDoFrame, and then draws the frame.
 */

typedef struct BINKFRAMETEXTURES {
    GXTexObj Ytexture;
    GXTexObj cRtexture;
    GXTexObj cBtexture;
    GXTexObj Atexture;
} BINKFRAMETEXTURES;

typedef struct BINKTEXTURESET {
    /* GPU texture resources for each Bink frame buffer. */
    BINKFRAMETEXTURES textures[BINKMAXFRAMEBUFFERS];

    /* Bink's view of the frame buffers backed by those textures. */
    BINKFRAMEBUFFERS bink_buffers;

    /* GameCube texture memory and deswizzle state. */
    void* base_ptr;
    u32 framesize;
    u32 YAdeswizzle_width;
    u32 YAdeswizzle_height;
    u32 cRcBdeswizzle_width;
    u32 cRcBdeswizzle_height;
    GXTexObj YAdeswizzle;
    GXTexObj cRcBdeswizzle;
    s32 drawing[2];
} BINKTEXTURESET;

RADDEFFUNC s32 Create_Bink_textures(BINKTEXTURESET* set_textures);
RADDEFFUNC void Free_Bink_textures(BINKTEXTURESET* set_textures);
RADDEFFUNC void Draw_Bink_textures(BINKTEXTURESET* set_textures, u32 width, u32 height, f32 x_offset,
                                   f32 y_offset, f32 x_scale, f32 y_scale, f32 alpha_level);
RADDEFFUNC void Wait_for_Bink_textures(BINKTEXTURESET* set_textures);
RADDEFFUNC void Sync_Bink_textures(BINKTEXTURESET* set_textures);

#endif
