# Remaining confirmed French source comparisons

Twelve complete shared source files compile with the normal PS2 recipe and authenticated `VERSION_SLES_53623` define. France-only profiles compare every currently confirmed member of each unit. Source files are byte-identical to the parent-reviewed implementations; this batch needs no new source, header, boundary, compiler-option or scoring changes.

| Source unit | Confirmed functions / bytes | Standard code matches |
| --- | ---: | ---: |
| zFX | 1 / 708 | 0 / 0 |
| xIni | 2 / 424 | 0 / 0 |
| zCamera | 1 / 360 | 0 / 0 |
| xPtankPool | 1 / 336 | 0 / 0 |
| zNPCTypeVillager | 1 / 280 | 0 / 0 |
| zEntHangable | 1 / 252 | 1 / 252 |
| xGroup | 1 / 200 | 0 / 0 |
| zEntSimpleObj | 1 / 156 | 0 / 0 |
| xSnd | 3 / 132 | 1 / 68 |
| xScrFx | 2 / 116 | 2 / 116 |
| zEntPickup | 1 / 60 | 1 / 60 |
| xEntDrive | 1 / 56 | 1 / 56 |

The standard gain is six functions / 552 bytes. All 16 confirmed members / 3,080 bytes remain represented, including zero-match units. Every group is partial; even the fully code-matched known subsets do not establish whole-unit recovery.

Canonical linkage identities come from authenticated reference-original DWARF and the existing French proof registries. Independent checks cover all 19 distinct member/callee identities, all four executable hashes, and exact per-unit member coverage. Four direct transfers have uniquely confirmed callees; the other 15 remain untouched and unresolved. No new function boundaries are introduced.

The actual full pipeline exited successfully after revalidating the original-only registries and compiling all twelve whole TUs. An independent objdiff invocation reproduced each unit record exactly and confirmed six real, same-name, same-size source/target pairs. Standard code matching is not a claim of complete raw relocation reconstruction or executable linking.

The report retains 597 known functions and the complete 2,979,968-byte CPU-region denominator, with 214,848 bytes in the known-function subset. Unknown CPU bytes and unmatched functions remain included. Debug-region profiles are unaffected by the additive French SHA-1 restrictions. No GC or Xbox source changes or rebuilds are claimed.

Private evidence is under `build/france237`: additive `profiles.json`, the complete compiler recipe and final reports in `pilot/SLES-53623`, and `exit-status.json`. Independent original identity/transfer checks, source identity hashes, report/member checks and objdiff replay are in `build/france237-review`. The eligibility snapshot excludes previously enabled all-version profiles and the previous eight-unit batch. A later parent snapshot introduced zUI as a newly eligible follow-up; it is not included here.
