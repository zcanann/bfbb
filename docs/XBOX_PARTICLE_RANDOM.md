# Xbox particle random calculations

An Xbox-only local inline helper computes the existing xrand update scaled by
2^-32. Four particle commands use it in place of out-of-line xurand calls.
The original commands embed that same LCG. Other platforms expand the local
helper name to the original xurand call; shared math headers are unchanged.

Both authenticated Xbox releases have the same results:

| Function | Before | After | Original bytes |
| --- | ---: | ---: | ---: |
| xParCmdMoveRandom_Update | 54.266666% | 100% | 263 |
| xParCmdMoveRandomPar_Update | 50.333332% | 87.91111% | 166 |
| xParCmdRandomVelocityPar_Update | 61.210526% | 86.494736% | 335 |
| xParCmdRotPar_Update | 51.988888% | 92.922226% | 360 |

MoveRandom's actual linked source body independently decodes to 263 bytes.
Restoring its eight named address operands reproduces every original byte in
both releases. Source inverse reconstruction is also checked. Original hashes,
boundaries and address expressions are unchanged. The remaining source bodies
decode to 162, 341 and 360 bytes respectively; their differences remain scored.

Inlining exposes actual seed and literal references in the source PE. All 45
source HIGHLOW fields across the four bodies are resolved through named MAP
globals, checked object ownership and literal bytes. The folded 2^-31 literal
is already independently reviewed in both originals; its actual source owner
is xParCmd.obj. Removing obsolete source-only xurand callee declarations lets
the strict source extractor check the resulting call inventory. Register,
opcode, flag and instruction-count differences remain scored. Compiler binaries,
flags and scoring settings are unchanged.

Both complete 13-unit builds preserve earlier exact matches and data measures;
only these four function scores improve. They add 263 exact bytes / one function,
reaching 15,569 bytes / 82 functions.
Recovered boundaries remain 2,556 functions / 635,415 bytes, and the full CPU
denominator remains 1,798,760 bytes. Source data and a complete original
executable build remain pending.

All three GameCube unit comparisons preserve every code/data symbol score.
All four PS2 before/after objects preserve every allocated section byte, size
and alignment, including all 31 section entries with repeated names. Private
evidence in the Xbox worktree: `build/parrand-full-retry`,
`build/parrand-raw-proof.json`, `build/parrand-expressions.json`,
`build/nonxbox-parrand/proof.json` and `build/nonxbox-ps2-parrand/proof.json`.
Root comparison logs are `build/oct09-parrand-full-<Xbox-version>-compare.log`.
