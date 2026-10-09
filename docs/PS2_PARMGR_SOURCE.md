# PS2 particle-manager countdown

The PS2 countdown uses an explicit zero check before decrementing its local
unsigned counter. This preserves the original update's control and observable
memory operations: call the platform update, increment the volatile frame
counter, wrap values above ten to one, load it once for the countdown, and
store zero after the countdown completes. Entry at zero performs no decrement.
The original DWARF confirms an unsigned frame counter in all three debug
versions; it does not retain a named local-counter DIE.

All four original update bodies have three GP load sites and three store sites,
one store conditional. The previous source object contained a fourth load while
setting up an eight-way unrolled countdown. The revised object has the original
three loads and a retained countdown, with no per-iteration memory access or
call. The loop's final value and volatile accesses agree with the source-level
operation. This is supported by original control-flow inspection and actual
source relocation/disassembly inventories, not by a score alone.

The update improves from 0% to 64.48276% in all four PS2 versions. The three
debug whole-unit reports improve from 39.583332% to 78.541664%; both previously
exact functions / 76 bytes remain exact, out of 192 bytes. France currently
profiles only the 116-byte update, so its percentage equals the function score.
There is no new exact function or linked-executable claim. Branch placement
and scheduling remain different from the original.

The shared rewrite was tested and regressed the GameCube update, so only PS2
uses the explicit break spelling. All GU4Y78 function scores remain unchanged,
with four exact functions / 196 bytes. Earlier and later PS2 compiler probes
retain the countdown but differ in other instructions; they do not establish
a specific compiler patch. No compiler flags, profile, identity, relocation or
scoring settings change.

Private PS2-worktree evidence includes `build/parmgr-full-comparison.json`
and its replay script, `build/parmgr-source-accesses.json`,
`build/parmgr-original-accesses.json` and their scripts, retained complete
source objects, `build/parmgr-gc-{before,after}.json`, and the fresh limited
French target in `build/parmgr-france-diagnostic`.
