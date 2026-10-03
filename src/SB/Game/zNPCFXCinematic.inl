// Helpers owned by the cinematic translation unit, in retail emission order.
// The local matrix helper keeps the arithmetic from xMath3.h.

static void xMat3x3RMulVec(xVec3* o, const xMat3x3* m, const xVec3* v)
{
    F32 x = m->right.x * v->x + m->up.x * v->y + m->at.x * v->z;
    F32 y = m->right.y * v->x + m->up.y * v->y + m->at.y * v->z;
    F32 z = m->right.z * v->x + m->up.z * v->y + m->at.z * v->z;

    o->x = x;
    o->y = y;
    o->z = z;
}

inline void NPCCone::TextureSet(RwRaster* raster)
{
    rast_cone = raster;
}

inline void NPCCone::UVSliceSet(F32 u, F32 v)
{
    this->uv_slice[0] = u;
    this->uv_slice[1] = v;
}

inline void NPCCone::UVBaseSet(F32 u, F32 v)
{
    this->uv_tip[0] = u;
    this->uv_tip[1] = v;
}

inline void NPCCone::ColorSet(RwRGBA top, RwRGBA bot)
{
    this->rgba_top = top;
    this->rgba_bot = bot;
}

inline void NPCCone::RadiusSet(F32 conefloat)
{
    rad_cone = conefloat;
}

inline void NPARMgmt::KillAll()
{
    this->cnt_active = 0;
}

inline zNPCB_SB2* zNPCB_SB2::singleton()
{
    return _singleton;
}
