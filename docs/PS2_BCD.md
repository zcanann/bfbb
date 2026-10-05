# PS2 decimal-to-BCD conversion

The missing `itoBCD(U16)` and `itoBCD(U8)` overloads are restored under the PS2
platform guard. Original DWARF identifies the unsigned-byte return type,
`dec` parameter types and signed-int `ones` local. The original instructions
compute the ones digit, remove it from the last two decimal digits, then shift
the tens digit into the high nibble.

Compiling the complete xutil translation unit with the registered compiler gives
96 bytes per overload, byte-for-byte identical to every one of the four PS2
originals. These leaf functions have no relocations. The existing profiles
already contain both original targets, including the verified French overloads;
no target identity or boundary changes are needed.

Evidence: build/france158/bcd-proof.json, actual complete source object in
bcd-compile, and the original DWARF parameter/local records. Other missing utility
functions remain unmatched. GameCube and Xbox do not receive these PS2-only
routines or declarations.
