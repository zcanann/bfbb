#include <rwsdk/rwcore.h>
#include <rwsdk/rpworld.h>

#define RwBBoxInitializeMacro(_bbox, _vertex)                                                      \
    MACRO_START                                                                                    \
    {                                                                                              \
        (_bbox)->inf = *(_vertex);                                                                 \
        (_bbox)->sup = *(_vertex);                                                                 \
    }                                                                                              \
    MACRO_STOP

#define RwBBoxAddPointMacro(_bbox, _vertex)                                                        \
    MACRO_START                                                                                    \
    {                                                                                              \
        if ((_bbox)->inf.x > (_vertex)->x)                                                         \
        {                                                                                          \
            (_bbox)->inf.x = (_vertex)->x;                                                         \
        }                                                                                          \
        if ((_bbox)->inf.y > (_vertex)->y)                                                         \
        {                                                                                          \
            (_bbox)->inf.y = (_vertex)->y;                                                         \
        }                                                                                          \
        if ((_bbox)->inf.z > (_vertex)->z)                                                         \
        {                                                                                          \
            (_bbox)->inf.z = (_vertex)->z;                                                         \
        }                                                                                          \
        if ((_bbox)->sup.x < (_vertex)->x)                                                         \
        {                                                                                          \
            (_bbox)->sup.x = (_vertex)->x;                                                         \
        }                                                                                          \
        if ((_bbox)->sup.y < (_vertex)->y)                                                         \
        {                                                                                          \
            (_bbox)->sup.y = (_vertex)->y;                                                         \
        }                                                                                          \
        if ((_bbox)->sup.z < (_vertex)->z)                                                         \
        {                                                                                          \
            (_bbox)->sup.z = (_vertex)->z;                                                         \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

RwBBox* RwBBoxCalculate(RwBBox* boundBox, const RwV3d* verts, RwInt32 numVerts)
{
    RwBBoxInitializeMacro(boundBox, verts);

    for (numVerts--, verts++; numVerts; numVerts--, verts++)
    {
        RwBBoxAddPointMacro(boundBox, verts);
    }

    return boundBox;
}

RwBBox* RwBBoxInitialize(RwBBox* boundBox, const RwV3d* vertex)
{
    RwBBoxInitializeMacro(boundBox, vertex);

    return boundBox;
}

RwBBox* RwBBoxAddPoint(RwBBox* boundBox, const RwV3d* vertex)
{
    RwBBoxAddPointMacro(boundBox, vertex);

    return boundBox;
}
