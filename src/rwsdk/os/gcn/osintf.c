#include <rwsdk/rwcore.h>

RwBool _rwpathisabsolute(const RwChar* path)
{
    /* Drive letter followed by a colon */
    if (path[1] == ':' &&
        ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')))
    {
        return TRUE;
    }

    /* Rooted path */
    return path[0] == '\\';
}
