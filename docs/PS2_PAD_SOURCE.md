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
2,780 bytes and every function score. The French profile currently owns only
NormalizeAnalog from this TU, whose report remains unchanged; this change does
not add unverified French function identities. No Xbox pad source profile exists.

Private evidence is under `C:/Projects/bfbb-agent-ps2-oct08/build`:
`pad-raw-proof.json`, `pad-gc-before.json`, `pad-gc-after.json`, and retained
`ps2solo-*/source/xPad.o` objects with whole-unit reports. The reproduction script
`padproof.py` reads genuine object relocations and compares relocated bytes to
original loaded segments directly.
