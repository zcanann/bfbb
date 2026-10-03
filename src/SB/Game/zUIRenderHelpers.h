#ifndef ZUI_RENDER_HELPERS_H
#define ZUI_RENDER_HELPERS_H

// Private to zUI.cpp: definition ownership determines deferred helper order.
inline void xMat3x3Scale(xMat3x3* m, const xVec3* s)
{
    xMat3x3ScaleC(m, s->x, s->y, s->z);
}

#endif
