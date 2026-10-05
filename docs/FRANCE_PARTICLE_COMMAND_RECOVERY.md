# France particle-command recovery

The stripped French original now compares the complete `xParCmd.cpp` TU:
30 functions / 7,544 bytes. The unchanged shared source compiles normally and
matches 15 functions / 2,780 bytes, with 59.13043% fuzzy matching. All remaining
functions stay in the report. France rises from 9,156 / 63 to **11,936 matched
bytes / 78 matched functions**, without changing its full-game denominator.
This is normal code matching, not a complete-TU or retail-relink claim.

## Independent original identity

The three original debug builds identify the same ordered 30 functions through
DWARF, including their canonical linkage names. At French `0x1f48c0`, the entire
sequence preserves every opcode, register, arithmetic immediate and relative
branch. Only actual direct-transfer fields and validated LUI-derived addresses
vary. Zero alignment gaps agree, and each complete reference template has one
French location. All 30 French bodies have closed local control flow, balanced
frames, terminal returns and valid following alignment.

Most update functions are callbacks, so direct JAL witnesses alone would leave
them unresolved. `france_particle_sequence.py` instead replays the actual
980-byte original `xParCmdInit` initializer. Its instruction set consists only of
LUI, ADDIU, SW and a final JR with a store in its delay slot. Every operand must
have a known constant producer. No source object or reconstructed table supplies
the values.

The 81 actual stores produce 27 records in the independently DWARF-named
`sCmdInfo` table. Original `xCmdInfo` DWARF establishes its 12-byte size and
`type`, `size`, `func` offsets 0, 4, 8. All record indices, IDs and sizes agree
across the four originals. The 26 non-null callbacks form a bijection with the
26 complete update-function entries in the reviewed TU. The remaining null
callback remains null. No guessed RenderWare or callback layout is involved.

The original initializer ends with LUI, JR $ra, SW: the final address consumer
executes before the return. The shared verifier now has an explicit opt-in for
this final store delay slot only, with a non-$ra address register and the existing
clobber/branch-entry checks. All existing callers retain strict defaults. Only
this initializer opts in; other returns and arbitrary control transfers do not.

Other table references retain their original relative offsets. The sole
initialized-data address is the original DWARF-named `cosSinPolynomial`; its
address lies in authenticated initialized data and its four-byte anchor prefix
agrees. No complete array size or match is claimed. All five original direct
callee identities and complete instruction structures agree across references;
these comparisons do not create additional named relocation anchors.

## Actual report verification

The existing TU registry now includes streaming plus particle commands, totaling
49 functions / 13,128 bytes. Every previously checked streaming function record
remains identical. The overall French known-function inventory increases from
350 / 95,624 bytes to 380 / 103,168 bytes.

Actual `platform_progress.py report --version SLES-53623` compilation and stock
objdiff generation pass. All old source-unit report records are unchanged; the
new particle-command unit and the reduced unresolved CPU remainder are the only
unit changes. Total code remains 2,979,968 bytes, total data 2,384,384 bytes, and
completion remains zero. Thirteen call relocations use already independently
confirmed French targets; unproved data and call identities stay unrestored.
Normal `functionRelocDiffs=none` scoring is unchanged.

There are no game source, compiler flag or report-math changes. Effective source
profiles for the three debug versions are identical to published `a3731b1d1`.
Parent CI verifies all versions after integration. Original-only proof replay is:

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```
