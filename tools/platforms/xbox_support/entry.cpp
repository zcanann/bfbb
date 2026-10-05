// Link reachability context only, not reconstructed Xbox game code.
#include "xString.h"

extern "C" U32 __cdecl xbox_source_entry(char* str, const char* tail, size_t size,
                                        const substr& text, size_t& read, char** next, void* buffer)
{
    return xStrHash(str) ^ xStrHash(str, size) ^ xStrHashCat(size, tail) ^ atox(text, read) ^
           imemcmp(str, tail, size) ^ xStricmp(str, tail) ^
           (U32)xStrTok(str, tail, next) ^ (U32)xStrTokBuffer(str, tail, buffer);
}
