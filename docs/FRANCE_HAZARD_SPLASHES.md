# French hazard splash callers

`france_hazard_splashes.py` identifies three complete functions from all three
debug retail originals: NPCC_MakePerp (56 bytes at `0x31afa0`), WavesOfEvil
(908 bytes at `0x3c4560`) and WaterSplash (2,172 bytes at `0x3c48f0`). All three
compile exactly, adding 3,136 exact bytes and three functions to French coverage.
No source implementation changes are needed.

The two Hazard bodies are independently unique over their complete extents.
Every non-call word is literal across the three originals and France, and
neither body has a changed data operand. Original source ownership, canonical
linkage, extent, alignment, following zero padding and ordinary closed control
flow are checked. All direct calls reach complete independently verified random
or particle-emission bodies, or the helper proved in this same cluster.

The 56-byte helper has no weak individual uniqueness claim. Its complete
264-byte original sequence includes the previously proved 196-byte
NPCC_MakeArbPlane, twelve bytes of inter-body padding, and the complete helper.
All original members, source order and offsets agree, and that complete
sequence has exactly one French location. The anchor's full bytes, calls and
closed bounds are independently replayed.

NPCC_MakePerp ends with a J to the already verified 224-byte xVec3Normalize.
The full fourteen-word inventory contains only the observed scalar loads,
subtractions, stores, argument forwarding and this tail. It preserves SP and
RA; the delay-slot store is required exactly. The scoped boundary proof requires
the three expected generic tail failures, with no calls, branches, stack
adjustments, return-address traffic or uncovered words, plus a closed complete
normalization callee. Generic CFG and instruction-effect rules remain unchanged.
The source profile declares the verified tail relocation explicitly as opcode 2.

Six original-backed tests passed in 35.6 seconds. They reject changed helpers,
anchors, callers, padding and callees; wrong tails and delay slots; duplicate
complete clusters and callers; changed original membership, source, name or
extent; and missing/changed dependencies. Unrelated registry additions preserve
the complete generated JSON. The fixed dependency set includes the newly
authenticated H2O particle emitters, so their proof must be integrated first.

Source validation against the full verified `8831f2efa` baseline
(`build/oct09-france-packer-geometry/SLES-53623/report.json`) preserves all seven
prior Hazard and all fifteen prior NPCSupport function records. Hazard's
selection is now nine functions / 5,140 bytes, all exact. NPCSupport's selection
is sixteen functions / 3,404 bytes, with 1,060 exact bytes / ten functions and
94.83901% similarity. The full executable denominator is unchanged.

Private artifacts: `build/splashes-proof.json`, `build/splashes-tests.txt`,
`build/splashes-france-pilot/report.json`, `build/hazard-perp-effects.py` and
`build/splashes-source-check.py`. Aggregate generator and registry integration
are separate steps; these commits contain only the standalone proof and profile.
