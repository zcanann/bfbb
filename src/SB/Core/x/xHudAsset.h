#ifndef XHUDASSET_H
#define XHUDASSET_H

#include "xDynAsset.h"
#include "xVec3.h"

namespace xhud
{
    struct asset : xDynAsset
    {
        xVec3 loc;
        xVec3 size;

        static const char* type_name()
        {
            return "hud";
        }
    };
} // namespace xhud

#endif
