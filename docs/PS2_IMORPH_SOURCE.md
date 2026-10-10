# PS2 morph source and packed conversion

The complete existing `src/SB/Core/gc/iMorph.cpp` now compiles for PS2. Its profile
covers all six original-owned functions, 3,132 bytes, in USA, Europe, and Germany.
The French profile covers all three already independently identified conversion
kernels, 848 bytes. No new function identity, boundary, or denominator is added;
the larger stripped French caller functions remain outside that proven set.

All three debug-region whole-unit reports reach 86.97318%, including two exact
functions / 192 bytes: `iMorphRender` (52) and `iMorphOptimize` (140).
`MorphCommon` reaches 98.40344%. The three packed kernels remain partial at
51.75%, 59.117645%, and 56.07143% for unpack, two weights, and four weights.
France's three-kernel report reaches 55.82547%, with no exact-function claim.
These are genuine complete-source compilations; none of the six implementations
is extracted, omitted, or replaced with comparison scaffolding.

## Original behavior

PS2's conversion routines load padded groups of eight signed shorts. Weighted
conversion interleaves signed coefficients and samples, uses `phmadh` for paired
integer products, combines four-weight sums with `paddw`, then converts the
packed signed results to floats and applies scale in VU0. Integer wrapping and
the conversion point differ from the existing scalar floating-point blend.
Unpack places each short in a signed word's high half and compensates with a
1/65536 scale factor. The final partial block writes exactly the requested
four-, two-, and one-float pieces. The typed PS2 implementation preserves these
operations with MMI/VU kernels and C block/tail control flow.

Original kernel bodies contain manual integer/VU scheduling and tail control
transfers, with no named compiler locals. Current C control flow leaves branch,
register, and scheduling differences. Original-style assembly control flow
remains a subsequent matching target; no compiler patch is proposed.

`MorphCommon` retains the original `0x70000000` scratchpad destination for its
zero-vertex temporary-buffer branch and computes the normals stride in short
units before advancing the pointer. The conversion helpers stay out of line,
matching the six original call sites. `iMorphOptimize` normalizes its existing
parameter before loading geometry, restoring the original input lifetime.

All seven relevant original record layouts agree across the three debug
releases: `DirtyMorph`, `RpAtomic`, `RpGeometry`, `RpMorphTarget`,
`RpUserDataArray`, `RwObject`, and `RwV3d`. This includes the 32-byte dirty-state
record's four signed-short weights and four source pointers, and every field
offset used by the restored code. Existing platform headers already supply
these layouts. A bounded source-local declaration uses the repository SDK's
`RpGeometryAddUserDataArray` interface; its stripped original destination remains
an unresolved identity, just like the other unknown SDK calls.

## Validation and remaining closure

The new profile records thirteen direct calls per debug region using independent
original DWARF callee identities, and sixty-four GP accesses using the original
ELF GP plus original-owned data declarations. All fields pass inverse byte
reconstruction. Nine original SDK/runtime call sites remain unregistered.
No source object supplies a target alias.

Raw replay reproduces all 52 bytes of `iMorphRender`. It reproduces 124 of 140
bytes of `iMorphOptimize`, including the independently unique loaded
`MORPHSTATE` string; its four remaining JAL words retain unresolved identities.
The ordinary 100% comparison for that function is not a complete-link claim.

The kernels' ordered raw VU instructions, including masks and operands, agree
across all four originals and compiled versions. Their MMI operation and lane
ordering also agree. The four original versions differ only at their internal
absolute jump destinations; every such jump stays within its independently
bounded kernel. Compiled kernel bytes are identical across all four version
macros. A bounded instruction interpreter additionally compares untouched USA
originals against actual compiled functions in 648 cases, covering counts
1-9, 15-17, 23-24, 31-33, and 64, signed samples/weights, several finite scales,
and initialized output-tail canaries. Every output word and untouched canary
agrees. This scoped arithmetic/control-flow check is not PS2 hardware timing or
exception validation; it does not generalize to arbitrary VU programs.

All three GameCube recompilations retain identical complete allocated sections
and their existing six exact functions / 4,848 bytes. There is no current Xbox
production profile for this unit. No shared header or compiler setting changes.

Private reproducible evidence in `build`:

- `imorph-originals.py` / `.txt`, `imorph-layout-proof.py` / `.json`, and
  `imorph-original-profile-proof.json` inventory original instructions, layouts,
  known relocations, and unresolved calls.
- `imorph-prepare-profile.py`, `imorph-prepare-france.py`, and
  `imorph-source-profiles.json` prepare the bounded original-only targets.
- `imorph-final-comparison.py` / `.json` and `imorph-pilot/<version>` contain
  actual complete objects, compiler logs, targets, and reports.
- `imorph-iMorphRender-raw-proof.py` / `.json` and
  `imorph-iMorphOptimize-raw-proof.py` / `.json` replay the newly exact bodies.
- `imorph-kernel-raw-proof.py` / `.json` and
  `imorph-kernel-emulation.py` / `.json` check vector lanes and complete bounded
  kernel execution against original bytes.
- `checkimorph-gc-all.py` and `imorph-gc-allocations.json` record the unchanged
  complete GameCube output against source parent `af07774e8`.

## Original handwritten control flow

The subsequent source restoration uses the original assembly-function form for
all three PS2 kernels. These bodies are independently established handwritten
MMI/VU routines: packed operations, manually allocated ABI registers, software
pipelining, explicit delay slots, and no compiler-generated frame or named
locals. The source preserves readable instructions and local labels, including
the pipelined block stores and partial-block tail. GameCube retains its existing
scalar C implementations.

All 848 kernel bytes reproduce all four originals after resolving each actual
source self-symbol relocation to its independently established original function
base plus its internal label offset. The only ordinary-comparison residual is
that the current target profile retains each literal internal J operand:

| Kernel | J instruction offset | Destination offset within the same function |
| --- | ---: | ---: |
| `FastS16unpack` | 148 | 168 |
| `FastS16weight2` | 180 | 200 |
| `FastS16weight4` | 244 | 264 |

No metadata is changed in this source follow-up. The debug-region whole-unit
reports improve from 86.97318% to 98.91443%; France improves from 55.82547% to
99.929245%. Exact totals remain 192 / two functions in each debug version and
zero for the French subset until those genuine internal relocations are
represented. All other function scores remain unchanged. This residual is a
known relocation representation difference, not a compiler-code-generation
hypothesis.

All three complete GameCube allocated-section comparisons remain byte-identical.
Private `build/imorph-asm-comparison.py` / `.json` preserve before and after
complete-source builds in `imorph-asm-before/<version>` and
`imorph-asm-after/<version>`. `imorph-asm-raw-proof.py` / `.json` independently
checks every source relocation, its original within-function destination, and
all reconstructed bytes. `checkimorph-asm-gc-all.py` and
`imorph-asm-gc-allocations.json` retain the cross-platform control. Earlier typed
pilot objects remain available under `imorph-pilot` and `imorph-asm-before`.

## Proven internal-J relocations

The target profile now represents each of the twelve independently verified
within-function J operands as the genuine function symbol plus its label offset.
An optional `interior_offset` applies only to an unconditional J whose named
original destination is its own function entry, whose symbol is that function's
existing profile symbol, and whose positive aligned label lies strictly within
its complete original byte extent. The recorded `high` bound must agree with
that extent. The retail opcode and decoded destination are checked before
normalization, and inverse reconstruction must reproduce the untouched word.
The ELF REL J field retains `interior_offset / 4`; the existing target writer
references the already-defined function symbol. Ordinary entry J and JAL records
retain their previous behavior and zero-addend representation.

This adds 848 exact bytes / three functions in every PS2 version without changing
source objects, identities, or denominators. The debug unit reaches 1,040 / 3,132
bytes, five / six functions, and 98.933586%. France reaches 848 / 848 bytes and
three / three functions at 100%. `MorphCommon` and all earlier relocation records
are unchanged. Each target unit differs in exactly the three reviewed J fields;
no unknown SDK/runtime operand is altered.

`tools/tests/test_ps2_internal_jumps.py` checks defined-symbol ELF ownership and
REL reconstruction; unchanged default entry J/JAL behavior; invalid, unaligned,
negative, noninteger, zero, and out-of-bounds label claims; changed opcodes,
symbols, cross-function destinations, inconsistent extents, and altered retail
operands. Its original-fixture test independently replays all twelve recorded
kernels and rejects both changed label claims and changed original instructions.
Together with `test_ps2_target_symbols.py`, all eight tests pass using
`BFBB_FRANCE_TEST_ORIG=C:/Projects/bfbb-bink/orig` and
`python -m unittest tools.tests.test_ps2_internal_jumps tools.tests.test_ps2_target_symbols`.

Private `build/imorph-jumps-comparison.py` / `.json` and
`imorph-jumps-{before,after}/<version>` preserve original-only target generation,
relocation records, full-unit reports, unchanged compiled objects, and the
explicit three-field/previous-relocation comparison against `a44ff92f9`.
