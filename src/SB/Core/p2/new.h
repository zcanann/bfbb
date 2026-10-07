#ifndef PS2_NEW_H
#define PS2_NEW_H

#include <types.h>

// Standard placement allocation form; the platform runtime supplies the rest.
inline void* operator new(size_t, void* ptr)
{
    return ptr;
}

#endif
