# PS2 collision header foundation

`xCollideGeometry.h` separates the existing collision result and five primitive
collision declarations from model, environment and swept-sphere dependencies.
The structure and declarations are moved unchanged. The complete PS2 `xBound.cpp`
can include this header and the authenticated platform geometry API without a
RenderWare header or an invented environment/model layout.

All three debug-bearing originals record `xCollis` as 80 bytes: flags/oid at
0/4, object/model pointers at 8/12, distance at 16, vectors at 20/32/44/56,
and both union alternatives at 68. Original linkage names and unsigned-int
return types confirm all five collision declarations.

The actual whole source unit has nine bounded original functions / 3,248 bytes.
`xRayHitsBound` matches all 208 bytes in each debug-bearing original after
applying the actual source object's call relocations to independently named
original callees. Remaining functions stay nonmatching; no complete TU or
retail executable source link is claimed. All 224 GameCube game/engine objects
compile with byte-identical allocated sections after the header extraction.

## Further type recovery

A read-only DWARF inventory finds 99 distinct Rw/Rp/Rx named concrete types in
each debug-bearing original, with consistent byte sizes across declarations.
This is a starting point for a reproducible type registry, not proof that all
members, calling conventions, macros or alignment attributes are interchangeable
with the GameCube SDK headers.

For example, all three originals record `RwV3d` as 12 bytes and `RwMatrixTag`
as 64 bytes, with vectors at 0/16/32/48 and flags/padding at 12/28/44/60.
`xiMat4x3Union` is 64 bytes, with both members at zero. However, PS2
`xSweptSphere` is 336 bytes while the existing GameCube structure is 324 bytes,
even though the final member ends at byte 324 and the named field offsets
agree. The extra tail size needs authentic alignment evidence before sharing
that definition; it must not be modeled as invented padding.

Private evidence is in `build/ps2headers155`: authenticated `type-layouts.json`,
`prototype-proof.json`, actual compiler command/object, `raw-proof.json`, and
`gc/comparison.json`. The type reader follows original DIE child/sibling links
and accepts member displacement expressions only in the recorded constant-plus
form; it does not infer missing fields or sizes.
