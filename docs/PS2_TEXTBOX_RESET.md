# PS2 textbox reset call boundary

The original textbox loader and event callback both call the complete
`ztextbox::reset` function. The shared source's automatic inlining had expanded
that body into both callers: the loader grew from the original 164 bytes to
484 and scored zero, while the callback scored about 63.8%.

A PS2-only `dont_inline` scope now preserves `reset` as a call. The loader also
uses the original positive `linkCount > 0` test, and the adjacent SceneEnd and
Reset labels use the original comparison order. The unsigned link-count test
and shared event body retain their behavior. No global inline flags change.

Both complete caller bodies now match in all three debug releases: loader
164 bytes and callback 892 bytes. Each unit gains **1,056 exact bytes and two
functions**, rising from 15/21 functions and 1,480 bytes to 17/21 and 2,536.

| Version | Unit fuzzy before | Unit fuzzy after |
| --- | ---: | ---: |
| SLUS-20680 | 83.25264% | 93.14053% |
| SLES-51968 | 83.894226% | 93.79821% |
| SLES-51970 | 83.894226% | 93.79821% |

Every other function score is unchanged. These are complete-source comparisons
against the existing original-backed profiles; no target identity, boundary,
registry or scoring rule changes. France has no enabled textbox source profile,
so no France matching gain is claimed.

The full GameCube USA report remains identical and the retail DOL SHA-1 check
passes. Private evidence is `build/textbox-oct09/*-{before,after}.json`, with
source snapshots for the separate call-boundary and condition-order probes.

## Text pointer lifetime

Advancing the asset pointer past its header in a separate statement before
calling `set_text` recovers the original text-pointer register lifetime. The
168-byte ID overload becomes exact in all three debug releases, raising each
unit to 18/21 functions and 2,704 exact bytes. USA fuzzy matching reaches
93.17709%; Europe/Germany reach 93.83482%. Every other function score is
unchanged, as is the entire GameCube USA report; its retail DOL SHA-1 passes.
Private evidence uses `*-pointer.json` in the same directory. An explicit text
local and byte-pointer arithmetic did not reproduce the same register lifetime.

## Textured backdrop

Three omitted source details account for the backdrop mismatch. PS2 vertices
need their reciprocal camera depth written explicitly; its UV setters do not
perform that write. Original screen scaling is 640 by 448 in USA and 512 by
512 in Europe/Germany. Finally, the original rectangle scaling overloads inline
into this caller, so the TU now supplies their existing definitions from
`xFont.cpp` under PS2. The original instructions independently show the depth
store, scale constants and four component multiplications.

The complete backdrop body becomes exact: 1,560 bytes in USA and 1,552 in
Europe/Germany. Units reach 19/21 exact functions, with 4,264/4,924 exact bytes
in USA and 4,256/4,916 in Europe/Germany. Fuzzy matching reaches 98.90495% and
98.903175% respectively. Every other function score is unchanged.

The depth setter is a no-op on GameCube; its existing dimensions and external
rectangle helper remain intact. The full GameCube USA report is identical and
its retail DOL SHA-1 passes. Private evidence uses `*-backdrop.json` in
`build/textbox-oct09`. No shared header, profile, target or registry changes.
