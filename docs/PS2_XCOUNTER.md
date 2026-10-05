# PS2 xCounter source comparison

The shared `xCounter.cpp` produces six exact retail functions (812 bytes) with
`mwcps2-3.0b38-030307` and the existing PS2 source flags in SLUS-20680,
SLES-51968, and SLES-51970. This is function matching, not a complete executable
link or data-layout claim.

The retail DWARF1 attribute 0x200 explicitly identifies both Init overloads and
the byte/short serializer calls. The 116-byte Init body belongs to
`xCounterInit__FPvPv`; the no-argument overload is eight bytes. PS2 therefore
uses the existing initialization body directly at the void-pointer entry and
inlines the empty tweak helper. A lightweight shared header exposes the existing
empty release-build `xDebugRemoveTweak` without importing GameCube rendering
headers. Other platforms keep the original debug-header inclusion and Init API.

EventCB contains separate signed counter-minus-one and unsigned event-base
addition instructions. Converting the index to the event API's U32 before adding
`eEventCount1` recovers the exact 448-byte body. Parentheses alone and a signed
intermediate still combine the constants into one instruction. No calls, stores,
or event semantics were removed. GameCube baseline and candidate allocated
sections and their relocation references are identical with the configured p1g compiler.

Target relocation reconstruction reads actual original DWARF identities, never
source-object guesses. JAL destinations must identify the named function and
invert to the original instruction. Init's adjacent LUI/ADDIU at offsets 28/32
loads the independently named EventCB address; validation checks both opcodes,
register flow, the signed low half, HI16 carry, and inverse reconstruction.
Separately applying those relocations to the compiled source reproduces every
retail byte in all six functions, in all three debug originals.

The profile opts into canonical linkage selectors and an executable SHA-1
allowlist. Existing profiles keep their human-name selectors and target labels.
Target preparation, source compilation and the standard section exporter all
honor the same allowlist. France currently has only xCounterReset independently
recovered; its diagnostic remains unchanged and this profile does not claim the
other five bounds. All four existing symbol registries remain unchanged.

Validation used the production `prepare_report`, `compile_units`, objdiff report,
and section-report adapter on all three authenticated debug executables. Private
artifacts for this pass are in `build/counter148/`, including complete raw-byte
proofs, per-version production reports and the GameCube section comparison.
