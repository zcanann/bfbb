# French FFX boundaries and source comparison

The French executable's complete `xFFX.cpp` sequence is corroborated against all three authenticated, debug-bearing PS2 originals. Fourteen original functions occupy 1,588 code bytes in a 1,680-byte span. Seven previously confirmed neighbors remain unchanged; seven additional functions contribute 520 known code bytes.

`tools/platforms/france_ffx_sequence.py` verifies full function bodies, zero alignment gaps, original canonical linkage names, unique whole-sequence placement, and unchanged strict control-flow/frame checks. The new members are `xFFXShakeFree`, `xFFXShakeAlloc`, `xFFXShakeUpdateEnt`, the `xFFX*` overload of `xFFXAddEffect`, `xFFXTurnOff`, `xFFXTurnOn`, and `xFFXAlloc`.

Allocator call identity reuses the previously reviewed, non-progress `xMemAlloc` anchor. The verifier independently decodes the actual JAL instruction in both original and French caller bodies, verifies their full hashes and original DWARF identities, and requires witnesses from at least two other source units for each reference. The current registry supplies thirteen witnesses per reference. This change neither invents an allocator identity nor promotes its extent into progress.

Two other math destinations remain unnamed. Their JAL words are literally identical in all four executables and remain **unmasked** in whole-sequence uniqueness checks. An identical 64-byte entry prefix is additional context only; it is not a function-boundary or source-ownership claim.

The existing unchanged, complete `src/SB/Core/x/xFFX.cpp` compiles with the pinned PS2 profile. Its comparison now covers all fourteen original members: eleven functions / 712 bytes score 100%, adding six functions / 172 bytes. `xFFXShakeUpdateEnt` remains partial at 79.22988%; the seven previously reported function scores are unchanged. Independent application of actual source JAL/GP relocations reproduces all 712 claimed exact bytes. GP identities use original DWARF declarations, original `.reginfo` GP values, and the same actual access instructions in all three debug references; no guessed French data layout is committed.

The full French report moves from 34,384 bytes / 209 functions to 34,556 bytes / 215 functions matched, with 609 known function boundaries. All prior function scores and every unrelated unit remain unchanged; old FFX records only receive shifted synthetic target-object section offsets as the missing members are restored. CPU code and data denominators stay unchanged. New known boundaries replace previously unclassified CPU bytes; source comparisons cover the complete existing TU, including all three holdouts. No full executable-link claim is made. There are no game-source, compiler-flag, generic CFG, or other-platform changes.

Reproduce original metadata validation with:

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```

Private validation artifacts: `build/ffx250/original-proof.json`, `command.json`, `report.json`, `raw-proof.json`, `full-report-proof.json`, and `production/SLES-53623/report.json`. The complete report reuses authenticated objects from the preceding full 60-TU production compile; source and flags are unchanged, the FFX whole object was independently recompiled, and all allocated sections agree. Object hashes and the unchanged build inputs are recorded in `reused-actual-source-build.json`.

## Direct return source recovery

Returning the effect ID directly, after an early allocation-failure return,
removes the redundant signed-short conversion introduced by the intermediate
local. Original DWARF confirms that both overloads return signed short; no ABI
or arithmetic type changed. The actual complete source object now reports all
fourteen functions / 1,588 bytes exact in all four PS2 versions, adding the
remaining 104-byte callback overload.

Every GU4Y78 GameCube function score is unchanged, with sixteen exact functions
and 1,280 bytes. Independent raw comparisons reproduce all 104 new bytes in
each debug original after applying its two real `alist` GP relocations. The
French diagnostic retains its existing unresolved GP operands, so its ordinary
code score does not establish a fully resolved relocation or retail link.
No compiler, profile, identity or scoring settings changed.

Private PS2-worktree evidence is `build/ffx-raw-proof.json`, `build/ffxproof.py`,
`build/ffx-gc-{before,after}.json`, retained whole `ps2solo` objects, and the fresh
limited target in `build/ffx-france-diagnostic`.
