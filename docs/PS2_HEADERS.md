# PS2 platform headers

The declarations in `src/SB/Core/p2` let shared source compile without importing
GameCube platform layouts. They are incomplete; a successful source compile is
not an executable link or a matching-progress claim.

## Color

`iColor.h` restores the retail `iColor_tag` declaration: four unsigned-byte
members `r`, `g`, `b`, `a`, at offsets 0, 1, 2, 3, with a total size of 4.
The declaration at DWARF offset `0x4c511` agrees in SLUS-20680, SLES-51968,
and SLES-51970. The genuine, unchanged `xColor.cpp` compiles with the pinned
PS2 profile using this header. Its constants are not yet included in matched
data reporting.
