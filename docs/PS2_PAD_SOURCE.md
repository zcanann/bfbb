# PS2 pad input and rumble source matching

The three debug-bearing PS2 executables compare raw analog stick values against
`100` and `-100` when synthesizing D-pad input. The shared GameCube source used
`50` and `-50`. A platform-selected local constant restores the twelve PS2
comparison immediates while retaining the existing non-PS2 threshold.

The pressed-button expression follows the original operand order, and
`xPadRumbleEnable` passes its existing pad pointer to the rumble-chain destructor.
Reusing that pointer also restores the destructor's inlined body in `xPadEnable`.
The destructor's existing behavior is unchanged.

Whole-TU comparisons for SLUS-20680, SLES-51968 and SLES-51970 improve from
932 to 3,088 matched bytes out of 3,536, and from four to seven exact functions.
The new exact bodies are Update (1,672 bytes), RumbleEnable (212), and Enable
(272). AddRumble remains unmatched with its existing loop NOP differences.

Independent raw reconstruction applies every actual source relocation in the
three new exact bodies: 13 `R_MIPS_26` call relocations and six `R_MIPS_HI16` /
`R_MIPS_LO16` pairs for `mPad`. Call addresses use the existing independently
validated profile records; the pad array address comes from each original's
DWARF declarations. All 2,156 reconstructed bytes agree with each authenticated
boot ELF. Function extents, target metadata and scoring policy are unchanged.

The configured GU4Y78 GameCube compilation retains all 14 exact functions /
2,780 bytes and every function score. The initial French control covered only NormalizeAnalog and remained unchanged;
the subsequent original-only sequence proof below restores the remaining members.
No Xbox pad source profile exists.

Private evidence is under `C:/Projects/bfbb-agent-ps2-oct08/build`:
`pad-raw-proof.json`, `pad-gc-before.json`, `pad-gc-after.json`, and retained
`ps2solo-*/source/xPad.o` objects with whole-unit reports. The reproduction script
`padproof.py` reads genuine object relocations and compares relocated bytes to
original loaded segments directly.

## French original sequence recovery

`france_pad_sequence.py` identifies the complete eight-function, 3,572-byte
sequence at `0x1f3950`, including alignment, from all three authenticated debug
originals. The independently established NormalizeAnalog entry at `0x1f3bb0`
anchors its placement. An unchanged 908-byte instruction run locates the full
sequence uniquely in the French original; every non-address instruction bit,
local branch, arithmetic immediate and alignment byte remains equal.

Six newly identified bodies satisfy the existing strict return/frame/CFG checks.
The eight-byte Kill wrapper has a scoped J/NOP proof: its independently compared
original platform callee is exactly a complete JR RA/NOP leaf. This rule changes
no generic boundary checker and creates no platform-callee identity.

Original DWARF proves four 328-byte pad records and thirty-two 16-byte rumble
records. The observed `mPad + 76` address is independently explained by
`rumble_head` at offset 68 and its `next` member at offset 8. Repeated address
producers, original array adjacency and authenticated runtime-BSS bounds agree
on both French arrays. This is operand evidence, with no new data extents or
data matching credit.

Complete original-sized external call contexts independently corroborate the
sequence but remain unpromoted. The 868-byte platform Update context needs
22 explicitly enumerated LBU/SB address consumers; the local checker retains
LUI definitions, clobber checks, delay-slot rules and rejection of incoming
edges that bypass a definition. Opaque runtime calls retain unchanged literal
transfer words and identical 64-byte entry contexts.

Seven new functions add 2,848 known bytes, raising known French code from
235,236 to 238,084 bytes while preserving the 4,255,232-byte loaded denominator.
The whole-TU diagnostic comparison now covers all 3,536 function bytes and
matches seven functions / 3,088 bytes: 2,400 additional matched bytes and six
additional exact functions. AddRumble remains unmatched. The profile restores
only the four calls whose French targets were independently named already;
unknown platform and runtime calls remain unresolved.

The standalone proof passed for all three originals. Negative controls reject
a changed analog threshold, a changed Kill return context, an inconsistent pad
array address, and a branch bypassing a byte-address definition. Previously
committed proof records are preserved exactly. Canonical complete registry
regeneration remains required when integrating this additive proof with other
French sequence changes.

Private reproduction: `build/pad-france-proof.py`, `pad-france-proof.json`,
`pad-proof-negative-controls.json`, and `pad-france-diagnostic/target.o` in the
same worktree as the source evidence above. The diagnostic target uses the fresh
standalone proof and existing independently confirmed callees; it does not
replace the production registry regeneration/check.
