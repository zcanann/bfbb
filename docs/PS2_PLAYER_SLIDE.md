# PS2 player slide interpolation

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
