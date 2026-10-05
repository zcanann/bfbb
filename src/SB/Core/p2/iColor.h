#ifndef ICOLOR_H
#define ICOLOR_H

#include <types.h>

// PS2 retail DWARF: four unsigned bytes at offsets 0, 1, 2, 3.
struct iColor_tag
{
    U8 r;
    U8 g;
    U8 b;
    U8 a;
};

#endif
