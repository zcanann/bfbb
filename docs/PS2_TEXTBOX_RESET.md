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
