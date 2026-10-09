# PS2 streaming asset-info scratch record

`xSTGetAssetInfo` retains an initialized `st_PKR_ASSET_TOCINFO tocinfo` local
on PS2 even though its package callback writes directly to the caller's output.
The original DWARF dump records `tocinfo` at stack offset `0x70` in this function
(`dwarf/SB/Core/x/xstransvc.cpp:188`). Retail code independently confirms the
24-byte initialization loop and the larger stack frame.

Restoring that declaration improves the 408-byte function from 82.67647% to
97.54902% in USA, Europe, Germany and France. The complete profiled `xstransvc`
unit improves from 98.1361% to 99.22278% in each version. Its 15 exact functions
and 3824 exact source bytes remain unchanged; every other function score is
unchanged. This is a fuzzy code improvement, not an exact-match claim.

Placing the scratch declaration before the return-value initialization worsened
matching. Explicit aggregate fields, the original `found`/`rc` names, and
predeclaring the loop index did not improve it. Initializing the loop index
before counting scenes also scored lower. The retained form is the smallest
change supported by both the original local-variable metadata and instructions.

The declaration is PS2-only. Full GameCube USA, Europe and Germany builds produce
unchanged function, code and data scores and pass their retail DOL SHA-1 checks.
Private unit reports for all
four PS2 versions and rejected source probes are in `build/stransvc-oct08`.

A follow-up places `xSTGetAssetInfoByType`'s `sum` initialization immediately
after its scratch-record initializer. This restores the lifetime used around
the output-clear call, improving the 516-byte body from 96.04651% to 98.02326%
in all four PS2 releases. The unit reaches 99.40544%, with unchanged exact
totals and all other function scores. Moving the initializer after the clear
or after scene counting scored lower. Six initialization-order probes in
`xSTFindAssetByType` did not justify a further source change.

This ordering change is shared source. GameCube USA's function remains exactly
matched, and the full build report remains byte-for-byte equivalent as JSON.
The four additional PS2 reports use the `-sum-after.json` suffix in the same
private evidence directory.
