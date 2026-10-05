# Entity declarations for PS2 consumers

`xEntTypes.h` separates the existing entity data declaration, callback types and
visibility helpers from model/rendering/collision implementation dependencies.
Pointer members retain their real types through forward declarations. The full
208-byte xEnt definition is moved, not replaced with a shortened prefix or padded
stand-in. GameCube visibility helpers remain at their original definition point
in xEnt.h to preserve emitted helper order.

All three debug-bearing PS2 originals record xEnt as 208 bytes, xGridBound as 20,
xBound as 76, and xEntFrame as 240. Every direct field offset agrees with the
existing declarations. xEntFrame requires 16-byte alignment on PS2: its last
mode word is at 224 and its recorded size is 240. The PS2 alignment attribute
preserves those fields and supplies the original tail extent without padding
members. GameCube keeps its existing alignment.

The nested xEnt::anim_coll_data remains opaque in the lightweight header. Its
complete existing definition stays in xEnt.h. Original PS2 fields start the two
matrices at 16 and 80 and give a 160-byte type; the existing heavy definition has
a different layout and is not claimed to be a restored PS2 implementation.
Consumers needing those fields require a separate original-supported port.

The real xGroup translation unit compiles with the lightweight header. Its debug
output omits expression-only entity types, as does the original xGroup DWARF.
The real xFFX compilation provides the aggregate proof instead: original and
compiled xEnt/xEntFrame layouts agree in all three debug versions, and debug
information leaves allocated source sections identical. Evidence is in the
integration worktree's `build/ffx168/type-proof.json` and this worker's
`build/ps2group168/{ent-layouts,frame-layouts}.json`.

All 224 actual GameCube game/engine translation units compile and retain an
identical ordered list of every allocated ELF section, including repeated .text
sections. Private replay evidence is `build/ps2group168/gc/comparison.json`.
This header extraction introduces no renderer implementation, guessed SDK layout,
or allocator access.
