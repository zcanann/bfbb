# PS2 player source differences

## Slide interpolation

`SlideTrackUpdate` is 3144 bytes at `0x14a7f0` in USA, Europe and Germany.
Its DWARF records the following local order: `colltmp`, `collsph`, `qcd`,
`isect`, `center`, `tpoly`, `triIndex`, `i`. It also records cached `frame`
and `triidx` pointers. Restoring these locals and the PS2 determinant
expression improves the full unit's slide score from 35.108143% to
36.631042% without introducing helper calls.

The original determinant arithmetic is:

```cpp
a * e + (d * c + (b * f - e * c - a * f)) - d * b
```

The GameCube expression remains separate. Common `center` and `triidx`
pointer spelling preserves the exact GameCube slide body; the complete
USA GC source report is unchanged.

## Remaining source reconstruction

Symbolic execution of the original scalar instructions establishes all
three interpolation expressions. The three terms are the negated texture
V coordinate times the determinant with that vertex replaced by the
query point. The final grouping is `term2 + (term0 + term1)`, divided by
the initial determinant. The other two queries substitute `1.0f + x`
or `1.0f + z`; each immediately subtracts the center value. Their divisions
are at function offsets `0x974`, `0xab0`, and `0xb8c`.

The initial determinant uses accumulator/FMA instructions. The long
interpolation block uses scalar multiply/add/subtract instructions and
loads triangle indices 45 times. There are repeated unsigned-16-bit
conversions and substantial spills. These are still unexplained source
or optimization boundaries, rather than evidence of a compiler bug.
The original span `[+0x35c,+0xb90)` is byte-identical across the three
debug regions, SHA-1 `8a7981cf0b7f4e1283b6d2f88aca7a6f45a385ef`.

Rejected experiments include general nine-argument determinants with a
top row of ones (the original constants are coordinate offsets), cached
vertex/texture values, scalar accessors, aggregate-return accessors, and
nested interpolation helpers. Some nested helpers appeared to improve
the score to 41–43%, but raw inspection revealed new out-of-line calls
absent from the original. Increasing inline budgets removed those calls
and lost most of the apparent gain. Such helpers are not retained.
`-O3,p` produces the same baseline as `-O4,p`; `-O2,p` regresses it.
No compiler flags, pragmas, patches, or forced register bindings are
part of this change.

## Player movement call boundary

The separate `zEntPlayer_Move` routine calls `TurnToFace` at offset `0x25c`.
The original JAL resolves to `0x172d00` in USA/Europe and `0x172e00` in
Germany. Keeping this existing helper out of line on PS2 improves the
1280-byte caller from 75.106% to 99.375%; all other functions in the unit
retain their scores. A scoped `dont_inline` pragma restores that boundary.
The only remaining differences are two extra NOPs, in the aggregate
zero-initialization loop and following the first dampening zero store.
The complete USA GameCube report remains identical.

## Rotation and German goo-death differences

`PlayerRotMatchUpdateEnt` at `0x143020` passes its dot product directly to
`acos` in all three PS2 debug builds. The call is at offset `0x1b4`, with
the final multiply-add in its delay slot. GameCube's `rang > 1.0f` clamp
is absent. PS2 also resets `HangElapsed` before the matrix-vector writes
in both branches and tests `surf->state == 0` directly. These differences
raise the 1252-byte routine from 80.958466% to 85.722046%. The remaining
register allocation and scheduling differences have not been attributed
to a compiler version.

Germany's 320-byte `GooDeathCB` retains `zEntPlayerControlOff`, including
its carried-object cleanup. USA and Europe omit that path. Restricting
the short-path guard to the appropriate PS2 regions restores the German
function from 17.25% to 100%; the prior comment claiming all PS2 builds
used the short path was incorrect.

## Player update integer absolute values

At offsets `0x1290` and `0x12b0`, the original `zEntPlayer_Update` calls
the same 20-byte signed-absolute-value runtime entry at `0x114b50` on the
two signed analog-pad bytes. The entry's five words are `04810002`,
`0080102d`, `00021023`, `03e00008`, `00000000`, identical in all three
debug regions. Its original runtime name remains unresolved; this does
not establish a new `abs` or `labs` symbol identity or relocation alias.

A PS2 `inline_intrinsics off` pragma scoped to this caller preserves the
existing source's two standard `abs(int)` calls, raising its comparison
from 94.44583% to 94.56484%. Scoping the pragma around just the expressions
and resetting it before the rest of the function has no effect. The
setting is reset immediately after the function, with no other unit
scores changed and no GameCube report change. The unresolved runtime
call identity remains a limitation of the source comparison.
