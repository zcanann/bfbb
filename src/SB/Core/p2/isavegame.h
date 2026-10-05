#ifndef ISAVEGAME_H
#define ISAVEGAME_H

// Shared savegame users require only the opaque platform session and change
// notification codes. PS2's session layout differs from the GameCube layout.
struct st_ISGSESSION;

enum en_CHGCODE
{
    ISG_CHG_NONE,
    ISG_CHG_TARGET,
    ISG_CHG_GAMELIST
};

#endif
