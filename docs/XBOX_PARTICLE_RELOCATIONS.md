# Xbox particle pool and allocator relocations

The complete shared particle TU now compares two more independently reviewed
Xbox functions: `xParMemInit()` at `0x149970` (49 bytes) and `xParAlloc()` at
`0x1499b0` (37 bytes). Both reach 100% in normal objdiff reports, and a separate
inverse-address reconstruction reproduces all 86 original bytes in both
independently authenticated Xbox regions. This is function reconstruction,
not a whole-TU or executable-link claim.

The original pool loop uniquely establishes a 192,000-byte array at `0x312910`:
it starts at that address, advances by 96 bytes, compares against the explicit
one-past end `0x341710`, and therefore visits 2,000 particles. It clears both links,
links the previous head, retains the new head in ECX, and finally stores it to
`gParDead` at `0x373dbc`. The allocator uses that same head, returns null if empty,
otherwise pops a particle and clears its links. Its existing reviewed caller
passes the returned EAX pointer directly to `xParInit`. The initialization call
at `0x6c4c7` is behind a startup flag check. Both leaves have independently decoded
local branches and terminal returns, followed by alignment before the next
function. Reviewed metadata records actual instruction hashes and call witnesses.

`reviewed-data-anchors.json` names only this pool and head. Its validator rechecks
the exact original loop's instruction data flow, stride/count/end equation and
writable mapped ownership. It does not add data progress or claim a complete
original symbol table. The pool initializer was not previously in the conservative
anonymous registry, so this adds one recovered function and 49 known bytes;
the allocator's existing anonymous extent is renamed without double counting.

Xbox excludes the shared source's existing `volatile` matching workaround. The
retail Xbox loop forwards its head value across iterations; the other platforms'
original declarations and local pointer type remain unchanged. All four particle
functions are still compiled from the complete TU with the existing pinned
MSVC/LTCG flags and genuine host reachability context.

The comparison restores six ordinary `IMAGE_REL_I386_DIR32` operands:

| Function | Original operand offset | Expression |
| --- | ---: | --- |
| xParMemInit | 2 | gParDead |
| xParMemInit | 7 | gParPool |
| xParMemInit | 36 | gParPool + 192000 |
| xParMemInit | 44 | gParDead |
| xParAlloc | 1 | gParDead |
| xParAlloc | 27 | gParDead |

Original offsets are fixed by reviewed instruction bytes. **Source offsets are
not copied from these originals.** The source extractor parses the actual linked
PE's HIGHLOW records, independently bounds each leaf, and discovers each operand
uniquely by decoded instruction prefix/kind and actual MAP symbol plus addend.
MAP entries must belong to the real source object and the complete declared
ranges must fit writable PE data without another global inside. Every HIGHLOW
field in each selected function must be covered exactly, with no overlap or
unknown field removed. Missing/ambiguous expressions fail rather than guess.

The pool-end expression is essential: in the linked host image, its numeric
address happens to equal `gParDead`. A numeric address lookup would misidentify
that operand. The explicitly reviewed loop-comparison expression identifies
`gParPool + 192000`; the decoded CMP operand disambiguates it. Production PE
addresses also differ from the initial scratch link because genuine CRT imports
change the section layout; name-based recovery handles this without fixed host
addresses.

COFF containers retain every other code byte. Restored fields contain their
actual addends and reference named undefined symbols. Reapplying each independently
known symbol address reconstructs the input function exactly. The section exporter
reconstructs target addresses and requires equality with authenticated `.text`;
a 100% function also requires identical normalized source/target bytes and named
relocation records. The standard objdiff policy and function scores are unchanged.

Both full production reports pass: matched bytes **48 -> 134**, recovered functions
**2,518 -> 2,519**, known function bytes **627,523 -> 627,572**. The full original
`.text` denominator stays **1,798,760**. The entire prior xString unit remains
identical, xParInit remains **88.030304%**, and no data or link completion is added.
An actual PS2 xPar compile preserves every allocated section and relocation record
against the prior validated production object; GameCube validation is performed
by the integration checkout before publishing.

Repeat with the normal commands documented in `XBOX_PARTICLE_INIT.md` and
`verify_xbox_reviewed.py`. These checks use authenticated originals and actual
compiler output; no synthetic source bodies or patched executable bytes are used.
