#include "iFMV.h"

#include <rwcore.h>
#include <stdio.h>
#include <types.h>

S32 ps2_mpeg_play(char* fname, char* work_area, S32 work_area_size, U32 buttons, F32 time);

U32 iFMVPlay(char* filename, U32 buttons, F32 time, bool skippable, bool lockController)
{
    RwResEntry* repEntry;
    RwResEntry* repEntryOwner;

    if (filename == NULL)
    {
        return 1;
    }

    // Borrow the MPEG decoder's work buffer from the (emptied) resource arena.
    RwResourcesEmptyArena();
    repEntryOwner = NULL;
    repEntry = RwResourcesAllocateResEntry((void*)0xDEADBEEF, &repEntryOwner, 0x3AAFAC, NULL);
    if (repEntry == NULL)
    {
        printf("### Error allocating MPEG work buffer\n");
        return 1;
    }

    ps2_mpeg_play(filename, (char*)(repEntry + 1), 0x3AAFAC, buttons, time);
    RwResourcesFreeResEntry(repEntry);

    return 0;
}
