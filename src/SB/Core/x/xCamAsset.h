#ifndef XCAMASSET_H
#define XCAMASSET_H

#include "xBase.h"
#include "xVec3.h"

enum _tagTransType
{
    eTransType_None,
    eTransType_Interp1,
    eTransType_Interp2,
    eTransType_Interp3,
    eTransType_Interp4,
    eTransType_Linear,
    eTransType_Interp1Rev,
    eTransType_Interp2Rev,
    eTransType_Interp3Rev,
    eTransType_Interp4Rev,
    eTransType_Total
};

struct _tagxCamFollowAsset
{
    F32 rotation;
    F32 distance;
    F32 height;
    F32 rubber_band;
    F32 start_speed;
    F32 end_speed;
};

struct _tagxCamShoulderAsset
{
    F32 distance;
    F32 height;
    F32 realign_speed;
    F32 realign_delay;
};

struct _tagp2CamStaticAsset
{
    U32 unused;
};

struct _tagxCamPathAsset
{
    U32 assetID;
    F32 time_end;
    F32 time_delay;
};

struct _tagp2CamStaticFollowAsset
{
    F32 rubber_band;
};

struct xCamAsset : xBaseAsset
{
    xVec3 pos;
    xVec3 at;
    xVec3 up;
    xVec3 right;
    xVec3 view_offset;
    S16 offset_start_frames;
    S16 offset_end_frames;
    F32 fov;
    F32 trans_time;
    _tagTransType trans_type;
    U32 flags;
    F32 fade_up;
    F32 fade_down;
    union
    {
        _tagxCamFollowAsset cam_follow;
        _tagxCamShoulderAsset cam_shoulder;
        _tagp2CamStaticAsset cam_static;
        _tagxCamPathAsset cam_path;
        _tagp2CamStaticFollowAsset cam_staticFollow;
    };
    U32 valid_flags;
    U32 markerid[2];
    U8 cam_type;
    U8 pad[3];
};

#endif
