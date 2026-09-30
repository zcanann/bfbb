---
name: bfbb-recovering-source
description: Use when a bfbb function exists in the target object but not in our source, when solo.py --missing lists functions to write, or when recovering original identifier names, signatures, or file-scope data
---

# Recovering source that does not exist yet

Three sources, and they are complementary rather than alternatives. Using one alone is the common mistake.

| Source | Gives you | Does not give you |
|---|---|---|
| **Ghidra** | control flow, call graph, correct mangled callee names | compilable C++, correct signatures, branch shape |
| **`dwarf/`** | original identifier names, local declaration order, file-scope globals with exact types | anything about the GameCube build's codegen |
| **the asm** (`solo.py`) | branch shape, register allocation, the truth | readability |

Ghidra alone gets to roughly 85–90% of the work. The last ten points come from the other two.

## Ghidra

The project is `C:\Users\joeyj\bfbb.gpr`, program `sbgcM.elf` (symbol-bearing). Dump with `tools/ghidra/DumpFuncs.java` via `analyzeHeadless`:

```
analyzeHeadless.bat C:/Users/joeyj bfbb -process sbgcM.elf -noanalysis -readOnly \
  -scriptPath <dir> -postScript DumpFuncs.java <out.c> <names.txt>
```

`-noanalysis -readOnly` is required or it tries to re-analyse and lock. All 597 missing game functions dump in ~73 seconds with zero decompile failures, so extraction is never the bottleneck.

**Pass names in a FILE, never as command-line arguments.** Mangled C++ names contain `<` and `>` — `xUtil_choose<i>__FPCiiPCf` is real and lives in this project. cmd.exe reads them as redirection and the entire invocation dies **silently, in 0.2 seconds, with no error output**.

What the output is actually like, measured over 100 functions:

- 0% decompile failures, no `halt_baddata`, no unrecovered jumptables
- callee names correct and mangled — this is the most valuable part, it resolves which overload is being called
- **52%** contain `undefined`/`undefined4`/`undefined8` types
- **30%** dereference raw offsets like `*(int *)(param_9 + 0x228)` that you must map onto our structs
- **~5%** invent up to nine `undefined8 param_N` and put the C++ `this` in the last one
- **`extraout_*`, `CONCAT*`, `SUB4*` are artifacts, not operations.** A line like `param_1 = extraout_f1;` is garbage from confused float-return handling. Never transcribe it.
- Some symbols have several copies in the ELF; check the address matches the unit.

Effectively **nothing compiles as-is**. Even a clean case like `xMat3x3RMulVec` comes out as three `float*` with index arithmetic where we need `xMat3x3*` and named members. Treat it as "here is the algorithm", then write real C++.

**The mangled name is authoritative for parameter types.** Ghidra's signature is not.

## dwarf/

231 files, 1:1 with `src/`, DWARF-derived from the **PS2** build. For every function: full signature, every local by name, and each local's register or stack slot. There is a large type preamble — search for the function name rather than reading from the top.

It also carries **file-scope globals with exact names, types and sizes**, which is how to recover data our tree is missing rather than guessing at it.

### Two traps that silently corrupt work

**A dwarf definition omits unnamed and unused parameters.** The same file has both forms:

```
line   12  signed int zGustEventCB(class xBase *, class xBase *, unsigned int, float *, class xBase *);
line 1418  signed int zGustEventCB(class xBase * to, unsigned int toEvent) {
```

Ours is `(from, to, toEvent, toParam, b)` and the **declaration agrees exactly**. Match names positionally against the *definition* and you rename `from`→`to` and `to`→`toEvent`. Nothing catches it: renames are byte-neutral, so every measurement stays identical. **Always resolve a short parameter list against the declaration.** Most apparent PS2-vs-GC "signature divergences" are just this.

**Function-scope statics are listed in DESCENDING ADDRESS order**, not declaration order. They are annotated `// @ 0x005CB880` rather than `// r18` or `// r29+0x90`, and interleaved with compiler `@NNNN` init-guard flags. Sorting them ascending by address gives the source order. Reordering to dwarf's listing cost one function 99.161% → 94.699%.

Registers are MIPS and transfer not at all. Genuine PS2/GC divergence does exist — the asm always overrides dwarf — but check the declaration before concluding you found one.

### What transfers, and how well

- **Identifier names** — reliable, and byte-neutral to adopt. `tools/dwarfaudit.py` finds mismatches.
- **Signatures for functions declared nowhere in our tree** — the highest-value use. A member function cannot be declared from the `.cpp`, so these block work entirely until a header is patched.
- **Local declaration order** — of functions we already match byte-for-byte, 85% already agree with dwarf, so it really is the original order. But reordering a near-miss to match it mostly does **not** help: across 39 attempts, three improved and several regressed hard. A near-100% function is usually wrong for some other reason, and disturbing a working stack layout makes it worse. `tools/dwarforder.py` ranks candidates; treat it as a long shot, not a lever.

## Do not fabricate

If a function needs data or a declaration that does not exist in our tree, the honest outcome is to report it, not to invent a plausible substitute.

Specifically: **never write a placeholder that returns a constant** so neighbouring functions match. One such stub — `zNPCFXCutscenePickTable` returning `NULL` — lifted three neighbours to 100% and would silently disable cutscene effects in a real build. These are invisible to every metric the project tracks.

Adding new `.data`/`.bss` objects is legitimate but shifts relocation offsets for every already-matching function in the unit. Add them where the target's data layout implies, re-measure the whole unit, and revert if it costs more than it gains.
