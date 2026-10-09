# France Prawn parameter body

`platforms/france_prawn_tweaks.py` proves Prawn's complete 2,660-byte
`register_tweaks` body at `0x361500`. All three authenticated debug originals
must agree on its DWARF ownership, canonical linkage, size, every non-address
instruction bit, and strict closed control flow and frame. The complete
masked body must occur uniquely in France, with a 124-byte unchanged anchor.
This is a scoped caller proof, not a whole translation-unit claim.

The 23 complete string operands preserve 20 distinct strings, including
four empty-string defaults. The 23 calls preserve complete original-backed
identities: 18 float-parameter, one integer-parameter and four bounded hash
calls. The parameter helpers are independently regenerated from Dutchman;
the bounded hash overload reuses the existing verified 88-byte function
at `0x20f260`. No callee is promoted again.

Original DWARF proves `sound_asset_ids` as `uint32[4][6]` (96 bytes) and
`sound_data` as four 16-byte records (64 bytes). Four ID loads preserve
indices `[0][0]`, `[1][0]`, `[2][0]`, `[3][0]`; four stores preserve the
unsigned id field at offset zero of each sound record. All eight accesses
must imply consistent French array bases and preserve their original
112-byte separation, with both complete typed arrays inside zero-fill
storage. No data extent or BSS value is promoted.

The scoped `reviewed-complete-caller-callee-cluster` proof excludes its own
caller record and emits no optional context that can drift as later
identities are recovered. Fuzzy scores and compiled source are not proof
inputs. Standalone validation passed all three originals in 29.79 seconds
on 2026-10-09; private evidence is `build/prawn-tweaks-proof.json` in the
regional checkout.

Authenticated-original mutation tests passed in 73.76 seconds, rejecting
changed instruction bits, complete strings, and typed-array mappings.
The France-only source profile restores all 23 calls and matches exactly:
one new exact function and 2,660 exact code bytes. Source and all other
version profiles remain unchanged. The selected-function denominator grows
by 2,660 bytes; the full CPU-code denominator stays fixed. Private report:
`build/prawn-france-pilot/report.json`.

Separate PS2 source work restores `update_turn`'s three original planar
locals, representation-sign comparisons, uncached maximum-velocity reads,
and assignment order. Original DWARF names `player_loc`, `loc`, and
`start_dir`; integer loads and `0x80000000` masks establish the sign tests.
The function improves from 59.321243% to 94.72021% in USA, PAL and German
PS2, with only this function changing across each complete selected-unit
report. The unit rises from 91.77952% to 93.9192%, retaining 29/53 exact
functions and 9,396/25,544 exact bytes. Remaining differences include
initial load scheduling and inline boolean expressions; no compiler-version
explanation is assumed. USA, PAL and German GameCube reports preserve all
function records and unit measures. Private comparisons are under
`build/prawn-source-before`, `build/prawn-source-after` and
`build/prawn-gc-verify`, summarized in `build/prawn-source-summary.json`.
