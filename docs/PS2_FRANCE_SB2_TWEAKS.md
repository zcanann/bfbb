# France SB2 parameter body

`platforms/france_sb2_tweaks.py` proves the complete 14,764-byte SB2
`register_tweaks` at `0x354f80`. It preserves every non-address instruction
bit, requires strict closed control-flow/frame checks in all three original
debug versions and France, and requires the complete masked body to occur
uniquely in the French loaded image. It does not claim the entire SB2 TU.

All 123 literal operands are complete byte-identical strings. There are 114
distinct string addresses: the empty-string default occurs ten times. The
123 direct calls retain independently proven parameter helpers and xStrHash;
the verifier regenerates the Dutchman helper proof before using those bodies.

The other twenty data operands use two original-owned BSS arrays. The original
DWARF declarations establish `sound_asset_ids` as `uint32[10][4]` and
`sound_data` as ten 16-byte `sound_data_type` records, with an unsigned id at
offset zero. Each array occupies 160 bytes. Ten loads retain the precise
original ID-array indices, including `[7][1]`; ten stores retain the id field
in each sound record. Every access must imply the same respective French
array base. Both complete arrays must fit zero-fill storage, remain separate
by the original 208 bytes, and retain an injective data-address mapping.
These data checks do not promote BSS extents or infer values from source code.

The record uses `reviewed-complete-caller-callee-cluster`, with the narrower
scope of one complete caller and its independently verified callees. Fuzzy
search output and compiled source objects are never inputs to the proof.

Standalone validation passed all three originals on 2026-10-08 in 29.83
seconds. Authenticated-original tests passed in 66.93 seconds and reject
changed instruction bits, complete strings and inconsistent typed-array
address mappings.

The France-only source profile selects this body and restores all 123 verified
direct calls. Authenticated source compilation compares at 99.56651% fuzzy
matching; the function is not exact. This adds 14,764 independently named and
source-compared bytes, with zero additional exact bytes. It expands the
selected-function denominator; the full CPU-code denominator remains fixed.
All existing profiles and source files are unchanged. Private validation is
`build/sb2-france-pilot/report.json` in the regional worker checkout.
