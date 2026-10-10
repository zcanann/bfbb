# French NPC smoothing leaf

`france_npc_smooth.generate_unit` establishes the complete 508-byte
`NPCC_GenSmooth` body at `0x31ac30` from all three authenticated debug originals.
It is a single complete leaf proof, not a whole-translation-unit claim.

Every instruction bit remains literal except three inventoried ADDIU address
fields. They select the original function's typed local `yews[4]` float array
at `0x5004e0` and `prepute[4][4]` float array at `0x5d54e0`. Original lexical
ownership, absolute local linkages, both array dimensions and float element
types are checked. All sixteen initialized sample bytes must equal
`{0.25f, 0.5f, 0.75f, 1.0f}` in each original. The entire 64-byte coefficient
array must lie inside independently established runtime BSS.

The literal-identical GP-relative LW/SW at offsets 0 and 24 select the original
signed 32-bit `init$6723` local. The proof checks its DWARF lexical owner and
complete four-byte BSS extent. Each executable's `.reginfo` GP must agree with
the real startup LUI/ADDIU/MOVE sequence, and the complete intervening instruction
forms must preserve the input register. Actual effective addresses must select
the declared local, including French `0x50fc90` from GP `0x516070`. These words
remain unmasked. No data identity, data extent or GP relocation is added to an
existing registry or source profile.

Exhaustive search across every loaded span finds one complete template, using
a 296-byte unchanged anchor. All three originals retain exact ownership,
linkage, entry alignment, extent, four zero padding bytes and a strictly closed
call-free CFG. The proof has no evolving registry dependencies.

Six original-backed tests pass in 50.19 seconds. They reject complete duplicate
bodies, body/address/GP/padding mutations, altered sample data, truncated BSS,
startup register clobbers, GP metadata mismatches, original identity changes,
and incorrect local owner, signedness or array dimensions. JSON round trips and
unrelated future registry contents preserve the complete proof output.

With the separately validated smoothing source, the seventeen-function French
NPCSupport selection gains 508 exact bytes and one exact function. It reaches
1,808 exact bytes / twelve functions out of 3,912 bytes / seventeen functions,
with 98.49284% similarity. The previous sixteen function records remain
identical apart from target-object layout offsets. Existing comparison behavior
handles the source object's data operands; no new relocation bridge or generic
verifier relaxation is needed. Full canonical production replay remains the
integration gate.

Private evidence: `build/smooth-proof.json`, `build/smooth-tests.txt`,
`build/smooth-data-survey.txt`, `build/smooth-original-survey.txt`, and
`build/smooth-france-pilot/no-gp.json`. The fuzzy candidate output was used only
for discovery and does not contribute proof or matching progress.
