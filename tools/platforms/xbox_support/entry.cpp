// Link reachability context only, not reconstructed Xbox game code.
#include "xString.h"

extern "C" U32 __cdecl xbox_source_entry(const char* str, const char* tail, size_t size,
                                        const substr& text, size_t& read)
{
    return xStrHash(str) ^ xStrHash(str, size) ^ xStrHashCat(size, tail) ^ atox(text, read);
}
