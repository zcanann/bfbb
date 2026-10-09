# PS2 streak and lightning source lifetimes

With the restored PS2 vertex-color setter, two remaining renderer mismatches
come from source-local lifetimes rather than a compiler-version difference.

`xFXStreakRender` now groups its two element pointers and declares the remaining
loop count before the ring-buffer index. No initializer or executable statement
changes. This restores the original pointer and saved-register allocation and
makes the 940-byte function exact in US, Europe and Germany.

`RenderLightning` tests `i & 1` at each texture-coordinate choice instead of
keeping an extra `odd` local alive across the vertex writes. The index does not
change between these tests, and the original DWARF lists no `odd` local. The
compiler still shares the calculation where appropriate, with the original
register lifetimes. The complete 4256-byte function becomes exact in the same
three regions.

Complete source-unit comparisons use the SDK restoration commit `2c5b52301` as
their baseline. Each debug region gains two exact functions and 5196 bytes:

| Unit | Exact bytes before | Exact bytes after |
| --- | ---: | ---: |
| `xFX.cpp` | 11924 | 12864 |
| `zLightning.cpp` | 2952 | 7208 |

Every other function score remains unchanged, as do function counts, target
sizes and data measures. The available French `xFX` profile remains unchanged
at 1820 exact bytes; these two functions are not identified in that profile,
so no French matching gain is claimed.

The shared source changes also preserve every exact GameCube function:
`xFX` retains 148 exact functions / 23256 bytes and improves the streak renderer
from 95.118515% to 95.525925%; `zLightning` retains all 17 exact functions /
12448 bytes with every score unchanged. No platform conditional was needed.

Private evidence is in `build/renderers-final-comparison.json` (seven complete
before/after unit-region pairs), `build/renderers-final-changes.json`, and
`build/{streak,lightning}-gc-{before,after}.json`. The regional rows record retained
compiled objects and complete function scores. These are ordinary source-code
comparison results; unresolved RenderWare call identities and data references
retain their existing limits, and no full-link claim is made. Profiles, identities,
relocation rules and compiler patches remain unchanged.
