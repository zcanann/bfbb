// Reachability for all portable utility APIs. This host receives no source credit.
#include "xutil.h"

extern "C" U32 __cdecl xbox_source_entry(
    U32 which, U32 crc, char* data, S32 size, F32* weights, F32 value)
{
    switch (which)
    {
    case 0: return xUtilStartup();
    case 1: return xUtilShutdown();
    case 2: return (U32)xUtil_idtag2string(crc, size);
    case 3: return xUtil_crc_init();
    case 4: return xUtil_crc_update(crc, data, size);
    case 5: return xUtil_yesno(value);
    case 6: xUtil_wtadjust(weights, size, value); break;
    }
    return 0;
}
