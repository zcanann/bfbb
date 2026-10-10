# PS2 Sandy leap center lifetime

`zNPCGoalBossSandyLeap::Enter` retains the ring-center X coordinate while
projecting the leap endpoint outside the inner circle. Loading that coordinate
before the endpoint and reusing it after scaling restores the original load
order and several floating-point register assignments. The change is PS2-only;
the existing GameCube expression already matches its originals.

The complete 70-function Sandy unit was rebuilt against current SDK headers
for SLUS-20680, SLES-51968 and SLES-51970. In each version the 484-byte Enter
function improves from 99.40496% to 99.62810%, and unit similarity improves from
99.61193% to 99.61436%. The other 69 function records are unchanged. Exact
coverage remains 28,676 bytes / 62 functions out of 44,456 bytes / 70 functions.
Baselines agree with the full source reports at `25f7b6853`.

All 77 Sandy functions in each of GQPE78, GQPP78 and GU4Y78 remain exactly
matched, with unchanged report records and measures. The current 21-function
French selection also produces an identical report; Leap Enter is not yet a
selected French identity.

The remaining Enter difference exchanges two floating-point registers while
preserving the recovered instruction order and 484-byte extent. Moving the
center declaration before the vector declaration does not change the score.
Alternative subtraction and center-copy formulations were worse and discarded.
No compiler patch or forced register constraint is introduced.

Private validation artifacts are `build/sandy-leap-region-summary.json`,
`build/sandy-leap-regions/<version>/{before,after}.json`,
`build/sandy-leap-gc-verify/<version>/report.json`, and
`build/sandy-control-callbacks-france-pilot/leap-source.json`.
