// TU-private definitions preserve the retail xCamera inline section.

inline void xCameraSetFOV(xCamera* cam, F32 fov)
{
    cam->fov = fov;

    iCameraSetFOV(cam->lo_cam, fov);
}

inline F32 xCameraGetFOV(const xCamera* cam)
{
    return cam->fov;
}

inline void xBinaryCamera::render_debug()
{
}
