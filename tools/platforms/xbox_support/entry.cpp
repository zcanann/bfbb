// Link reachability context only, not reconstructed Xbox game code.
#include "xString.h"

extern "C" U32 __cdecl xbox_source_entry(char* str, const char* tail, size_t size,
                                        const substr& text, size_t& read, char** next, void* buffer,
                                        const substr& other)
{
    xStrupr(str);
    F32 floats[4];
    xStrParseFloatList(floats, str, 4);
    return (U32)find_char(text, other) ^ xStrHash(str) ^ xStrHash(str, size) ^ xStrHashCat(size, tail) ^
           atox(text, read) ^ imemcmp(str, tail, size) ^ xStricmp(str, tail) ^
           (U32)xStrTok(str, tail, next) ^ (U32)xStrTokBuffer(str, tail, buffer) ^ icompare(text, other);
}
