# France CruiseBubble parameter body

`platforms/france_cruise_tweaks.py` proves the complete 11,980-byte
`register_tweaks` body at `0x2a1540`, owned by `SB/Game/zEntCruiseBubble.cpp`.
All three authenticated debug originals must preserve its DWARF ownership,
canonical linkage, size, and strict closed control flow and stack frame.
Every non-address instruction bit is preserved. Its complete masked body
must occur uniquely in France; the unchanged discovery anchor is 188 bytes.
This does not claim the whole CruiseBubble translation unit.

Every one of the 94 distinct literal operands preserves a complete nonempty
string. All 94 calls preserve complete original-backed callee identities:
86 float, five integer, and one vector parameter helper, plus two calls to
the bounded 88-byte `xStrHash` overload already verified at `0x20f260`.
The parameter helpers are independently regenerated from their Dutchman
proof. The hash overload is checked against the existing registry and is
not promoted again. The module excludes its own caller record on replay
and emits no optional contexts that could drift as later identities arrive.

The scoped `reviewed-complete-caller-callee-cluster` record uses only
authenticated original evidence. Neither fuzzy search scores nor compiled
source objects are proof inputs. Standalone validation passed all three
originals in 18.33 seconds on 2026-10-09. Private proof output is
`build/cruise-tweaks-proof.json` in the regional checkout.

Authenticated-original mutation tests passed in 43.81 seconds, rejecting
altered instruction bits, strings, and known callee bytes. The France-only
profile restores all 94 calls and compares the newly selected source body
at 99.57195% fuzzy matching, with no new exact bytes. The selected-function
coverage increases by 11,980 bytes; the full CPU-code denominator stays
unchanged. Other version profiles and source files are unchanged. Private
comparison: `build/cruise-france-pilot/report.json`.
