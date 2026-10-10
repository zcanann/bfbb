# French Sandy Leap Enter

`france_sandy_leap.py` independently identifies the complete 484-byte
`zNPCGoalBossSandyLeap::Enter` at `0x32ea40` from all three debug retail
originals. This is one complete function and its independently authenticated
callee, not a whole translation-unit claim. The registry and aggregate
generator are deliberately separate integration steps.

The original owner, canonical linkage, complete extent, aligned entry and all
twelve bytes of following zero padding agree. The entire instruction template
has exactly one location across all loaded French segments. Only the two
reviewed global loads and the complete terminal direct transfer are masked;
opcodes, registers, arithmetic constants, branches and every other word remain
literal.

The two LW operands at offsets 60 and 80 refer to the independently typed
`globals.player.ent.model` field at `0x52cc14`. The existing complete CalcNewDir
624-byte anchor and original declarations establish that path; no global
extent or inferred object identity is promoted.

The terminal J at offset 476 reaches the already verified, complete 216-byte
`zNPCGoalCommon::Enter` at `0x2cb490`. Each original and the French target have
the same three generic boundary failures expected of a tail: no local return,
no terminal return-delay pair, and one edge outside the extent. A scoped
verifier additionally requires no calls, stack adjustments, RA saves/loads,
uncovered words, external conditional edges or other external jumps. Every
instruction preserves SP and RA; the terminal delay is valid and the complete
callee has closed bounds. The generic boundary checker is unchanged.

The local register-effects helper recognizes only the encountered EE scalar
floating arithmetic/accumulator/compare forms. It checks the scalar format,
the reserved zero fs field of EE SQRT.S, and the zero destination field of
MULA.S/C.LT.S/C.LE.S. COP1 register transfers retain their ordinary GPR effects;
unknown encodings still fail. Reserved-bit and MFC1/CFC1 negative controls cover
this distinction.

Seven tests pass with the original fixtures. They reject modified function,
operand, tail, padding and callee bytes; duplicate complete templates; changed
original owners/extents; absent or altered dependencies; and stack/RA writes,
external branches and wrong tail targets. Adding unrelated registry identities
leaves the generated JSON identical. Only two fixed independent identities are
read, so future discoveries cannot change this proof's context.

Private artifacts: `build/sandy-leap-proof.json`,
`build/sandy-leap-tests.txt`, `build/sandy-leap-originals.txt`, and
`build/sandy-leap-france-pilot/report.json`.

The source profile explicitly restores this verified J transfer with opcode 2.
Leap Enter scores 99.62810%, matching the three debug versions after the scoped
PS2 ring-center lifetime change. All 21 prior French Sandy function records
remain unchanged. The selection now contains 22 functions / 14,780 bytes,
12,444 exact bytes / 20 exact functions, at 99.93369% similarity. This identity
adds known coverage but no newly exact bytes.
