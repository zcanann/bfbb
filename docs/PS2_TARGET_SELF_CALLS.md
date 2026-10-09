# PS2 target self-call symbols

The reconstructed xParGroupUpdateR body is raw-identical to the complete
220-byte original in USA, PAL and Germany after resolving its two real JALs.
The published comparison still scored 99.90909% because the target retained
its absolute self-call operand, while the compiled source referenced its own
defined function symbol. This is a comparison-metadata issue, with no compiler
patch or source instruction change.

The source profile now explicitly names JAL+52 to xParGroupUpdateR and JAL+132
to xParCmdGetUpdateFunc. Both destinations and canonical linkage identities are
independently read from each authenticated original's DWARF. The normal existing
target preparation validates the decoded opcode/destination and inverse
reconstruction before restoring each relocation; no source object proves a
target identity.

The target ELF writer now reuses an existing function definition when a named
relocation refers to a function in that same object. It retains undefined
symbols for external callees. Creating a second undefined symbol for a defined
callee lost the internal/self relationship. Duplicate definitions fail closed
instead of silently selecting one.

Three independent ELF-parser tests check self/internal definition references,
external undefined references and duplicate rejection. Original-backed replay
verifies both call operands in all three debug versions. The USA comparison
against the already compiled source then reaches 100% for the same 220 bytes.
Private evidence is `build/self-reloc/proof.json` and the target/diff under its
regional directories; the prior raw source audit is
`build/particle-recursion-raw-proof.json` in the PS2 worktree.

Production integration must rebuild target objects and compare full reports
against earlier snapshots before claiming the gain. This does not recover new
function boundaries, change source code/data, identify unnamed runtime callees
or prove a complete executable link.

Production integration passes full target/source regeneration in USA, PAL and
Germany, with all 214 enabled units compiled per region and no earlier
function/code/data score lost. The same updater reaches 100% in every report,
adding 220 exact bytes / one function per debug region through proven metadata.
Evidence: `build/oct09-full-render-save/<version>/report.json` and the three
full comparison logs. PAL/German baselines are independently built from frozen
commit `555924f5e` in `bfbb-verify-oct09-555/build/before-555`.
