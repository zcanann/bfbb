#include "xParCmd.h"

// Diagnostic reachability only; the complete real TU supplies all callbacks.
extern "C" U32 __cdecl xbox_source_entry(U32 type, xParCmd* command, xParGroup* group, F32 dt)
{
    xParCmdInit();
    xParCmdUpdateFunc update = xParCmdGetUpdateFunc(type);
    if (update)
    {
        update(command, group, dt);
    }
    return xParCmdGetSize(type);
}
