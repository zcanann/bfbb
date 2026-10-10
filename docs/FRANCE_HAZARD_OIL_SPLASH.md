# French OilSplash and independently bound calls

`france_hazard_oil_splash.py` identifies the complete 1,084-byte OilSplash at
`0x3c2910` from all three debug retail originals. The source matches exactly,
adding 1,084 exact bytes and one function. All nine prior Hazard selection
records remain unchanged; its ten selected functions / 6,224 bytes are now
entirely exact. The underlying Hazard source object is unchanged from the
immediately preceding splash validation and reused for this selection check.

OilSplash and TarTarSplash have identical arithmetic when every JAL operand is
masked. A generic masked template therefore cannot distinguish them. This
proof requires stronger evidence: every direct callee has an independently
authenticated complete original/target identity. The six calls reach
NPCC_MakeArbPlane, xurand twice, xrand twice and NPAR_EmitOilSplash. The latter
is established by the preceding complete particle-emitter cluster proof.

For each original, the verifier first checks the entire original body against
the proposed target, permitting exactly these six JAL fields and no data
address changes. Each complete callee is checked against its original
declaration, source ownership, extent, full original/target hashes and fixed
provenance. It then constructs a search template from the original body,
replacing only each JAL destination field with the independently established
French callee address. Opcode bits remain literal. No target body bytes are
copied into this template.

The constructed complete template must equal the proposed target and have
exactly one occurrence across all loaded French segments, using no masks.
This rejects any duplicate that reaches the same authenticated callees while
distinguishing the otherwise similar TarTar caller's different emitter.
Original canonical linkage, source, size, aligned entry, four bytes of zero
padding and ordinary closed control flow are independently required. The
generic matching, uniqueness and CFG helpers are unchanged.

Original-backed tests exercise the ambiguous unbound template, exact duplicate
rejection, a distinct different-callee duplicate, mutated function/call/padding
and callee bytes, original owner/name/extent changes, missing or altered
dependencies, and proof stability when unrelated registry entries are added.
All six tests passed in 21.9 seconds; three-original replay took 6.1 seconds.
The registry and aggregate generator remain separate integration steps.

Private artifacts: `build/oil-proof.json`, `build/oil-tests.txt`,
`build/oil-france-pilot/report.json`, `build/oil-source-check.py`, and
`build/oil-splash-bound-calls.py`.
