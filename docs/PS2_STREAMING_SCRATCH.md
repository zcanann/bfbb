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

The declaration is PS2-only. A full GameCube USA build produces an identical
`report.json` and passes the retail DOL SHA-1 check. Private unit reports for all
four PS2 versions and rejected source probes are in `build/stransvc-oct08`.
