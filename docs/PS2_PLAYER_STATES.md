# PS2 Bungee and CruiseBubble source comparisons

Both complete existing gameplay units compile with the established PS2 toolchain.
Their full original function sets remain selected, including all holdouts:

| Unit | Original functions / bytes | Normal matched functions / bytes | Fuzzy score |
| --- | ---: | ---: | ---: |
| zEntPlayerBungeeState | 57 / 30,508 | 8 / 2,740 | 48.836895% |
| zEntCruiseBubble | 91 / 48,436 | 25 / 2,160 | 56.973904% |

The independently generated USA, Europe and German comparisons have these same
measures. Each profile records call and global addresses from its authenticated
original. Duplicate header-function names use the existing address-qualified
original identity instead of guessing which copy owns a call. Unknown references
remain unresolved (44 in Bungee and 54 in CruiseBubble); no backend normalization
or compiler setting changes are introduced. France is not enabled here.

## Source and platform dependencies

Bungee selects the ordinary PS2 string header and replaces one PowerPC-specific
`__fabs` expression with the established portable `xabs`. CruiseBubble selects
ordinary PS2 math, the existing complete RenderWare skin API, and the PS2
immediate-mode interface. Its gameplay bodies are unchanged. These are complete
source compilations, without extracted probe bodies or fabricated SDK layouts.

The PS2 string-header dependency also needs the standard `strncmp` and `stricmp`
declarations supplied by the platform-header integration from the existing
standard library interfaces.

## Validation

Bungee has 128 shared named aggregate layouts and CruiseBubble has 267. Every
original variant matches the emitted whole-source debug information in all three
regions: complete sizes, direct member offsets, and recorded direct bitfield
fundamental types, offsets and widths. The existing original-backed NPC nested
flag comparison is retained where applicable. Normal and debug compilations have
identical ordered allocated sections. The independent CruiseBubble rebuild also
matches all 465 allocated sections of the integration object; Bungee's actual
whole-source normal/debug pair preserves all 145 sections.

Both actual GameCube objects rebuild with unchanged ordered allocated sections.
The integration's prior Bungee validation additionally preserved its complete GC
report and retail executable hash. No complete PS2 TU or executable link is claimed.

Independent application of actual source relocations to independently named
original addresses proves six Bungee functions / 384 bytes and 23 CruiseBubble
functions / 1,196 bytes exactly in each region. Normal objdiff code matching can
still include unresolved calls or anonymous constants; those functions are not
reported as raw linked-byte matches.

Private reproducible evidence is under `build/player224`: per-original profiles,
reports, raw relocation proofs, layout inventories, compiler commands, the original
object control, and GC allocated-section comparisons.
