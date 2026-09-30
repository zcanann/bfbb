#ifndef XGRIDCHECKBOUND_H
#define XGRIDCHECKBOUND_H

#include "xGrid.h"
#include "xBound.h"
#include "xMath.h"
#include "xQuickCull.h"

// Kept out of xGrid.h: the aggregate initializer in get_grid_index emits a
// zero-filled .sbss2 constant in every unit that includes it.

struct grid_index
{
    U16 x;
    U16 z;
};

inline grid_index get_grid_index(const xGrid& grid, F32 x, F32 z)
{
    grid_index index = { range_limit<U16>((U16)((x - grid.minx) * grid.inv_csizex), 0, grid.nx - 1),
                         range_limit<U16>((U16)((z - grid.minz) * grid.inv_csizez), 0,
                                          grid.nz - 1) };
    return index;
}

template <class T>
inline void xGridCheckBound(xGrid& grid, const xBound& bound, const xQCData& qcd, T cb)
{
    xGridIterator it;

    xBox box;
    xBoundGetBox(box, bound);

    F32 ex = 0.25f * grid.csizex;
    F32 ez = 0.25f * grid.csizez;
    box.lower.x -= ex;
    box.lower.z -= ez;
    box.upper.x += ex;
    box.upper.z += ez;

    grid_index var_4C, var_50;
    var_50 = get_grid_index(grid, box.lower.x, box.lower.z);
    var_4C = get_grid_index(grid, box.upper.x, box.upper.z);

    xGridBound* cell = xGridIterFirstCell(&grid.other, it);
    while (cell)
    {
        if (xQuickCullIsects(&qcd, &((xBound*)(cell + 1))->qcd))
        {
            if (!cb(*(xEnt*)cell->data, *cell))
            {
                xGridIterClose(it);
                return;
            }
        }
        cell = xGridIterNextCell(it);
    }

    for (U16 gx = var_50.x; gx <= var_4C.x; gx++)
    {
        for (U16 gz = var_50.z; gz <= var_4C.z; gz++)
        {
            xGridBound* cell = xGridIterFirstCell(&grid, gx, gz, it);
            while (cell)
            {
                if (xQuickCullIsects(&qcd, &((xBound*)(cell + 1))->qcd))
                {
                    if (!cb(*(xEnt*)cell->data, *cell))
                    {
                        xGridIterClose(it);
                        return;
                    }
                }
                cell = xGridIterNextCell(it);
            }
        }
    }
}

#endif
