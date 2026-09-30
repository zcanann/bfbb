---
name: bfbb-decomp
description: Use when writing or fixing decompiled C++ in the bfbb GameCube decompilation, when a function does not match the target object, or when dispatched to work on a src/SB unit
---

# Working on the bfbb decompilation

The goal is source that compiles to **byte-identical PowerPC** against the retail object. Not "equivalent" — identical.

## Measure everything, claim nothing

```
python tools/solo.py <unit-fragment>            # non-matching list with percentages
python tools/solo.py <unit-fragment> <symbol>   # side-by-side asm diff
python tools/solo.py <unit-fragment> --missing  # target functions absent from our object
```

Compiles one unit into a private temp dir in ~2s. Touches no shared state, safe to run constantly.

**The LEFT column is the TARGET (retail). The RIGHT is OURS.** Getting this backwards inverts every conclusion you draw.

**Never state a percentage you did not personally measure.** A measured 87% is worth more than an unmeasured claim of 100%.

**One change, one measurement, one decision.** Never batch several edits and measure once — a regression hides behind an improvement and you cannot tell which edit did what.

See [references/measuring.md](references/measuring.md) for which number means what, and for the three ways this project has fooled itself.

## Hard rules

**If you are reading this skill, these rules apply to you** — there is no exemption for being the top-level session or for a task being read-only. Each rule below says what it protects, because a rule you understand is one you will not talk yourself out of at 2am.

1. **Never run `ninja` or `configure.py`.** They write into `build/GQPE78/` and overwrite `report.json`, which the coordinator is measuring against and other workers are racing. There is no read-only version of this. To check that your work compiles, run `python tools/solo.py <unit>` — it builds the unit into a private temp dir in ~2s and is the correct answer to "is it broken".

2. **Never run a git command that changes state** — `add`, `commit`, `checkout`, `restore`, `reset`, `stash`, `merge`, `rebase`, `push`. The index and working tree belong to the coordinator, who is staging other work. Read-only `git status`/`log`/`diff`/`show` is fine.

   When you do read the tree, expect it to contain **other workers' in-flight edits**. Never attribute a modified file to your own work unless you edited it, and never conclude the build is broken from a file someone else is mid-way through.

3. **Never spawn subagents.** Do the work sequentially yourself and report partial progress. Nested agents have exhausted the session budget.
4. **Edit only the files you were assigned.** Especially: never edit a shared header. Report the exact diff you would apply instead.
5. **LF line endings, never CRLF.** Verify after editing: `open(p,'rb').read().count(b'\r\n')` must be 0.
6. **Quote every shell argument.** Mangled names and game strings contain `<`, `>` and `!`. Unquoted, `xUtil_choose<i>__FPCiiPCf` redirects — it will create junk files, or kill a command silently with no error and no output.
7. **Finish and verify one file before starting the next**, so an interruption leaves a clean tree rather than a half-edited file that fails to build.
8. **Protect what already matches.** Re-run the plain unit-level `solo.py` after every function and read the *whole* list, not just your function.

## Red flags — stop if you catch yourself here

| Thought | Reality |
|---|---|
| "objdiff says 100%, so the object matches" | objdiff pairs symbols by **name** and is blind to definition order. For a `Matching` unit only the DOL sha1 proves byte-identity. |
| "A stub returning NULL lifts three neighbours to 100%" | That is gaming the diff. It ships a silently broken game. See below. |
| "I'll rename the declaration and fix the uses after" | A half-applied rename breaks the build. Apply every occurrence, then measure. |
| "The percentage went up, so the change is right" | A gain from a change you know to be *less* faithful is a symptom, not a fix — it is compensating for a real error elsewhere. |
| "I'll batch these and measure at the end" | Then you know nothing about any of them. |
| "I'll just run ninja to check nothing broke" | It overwrites report.json under the coordinator. `solo.py` is the answer. |
| "This file is modified, so something is broken" | Other workers are mid-edit in this tree. Not yours, not broken. |
| "I'm the top-level session, so the rules are looser for me" | They are not. Reading this skill is what makes them yours. |

## Correctness outranks the percentage

This work also feeds a planned PC port, where semantic correctness matters and byte-exactness does not.

**Never write a body you believe is wrong to make the diff happier.** No placeholder that returns a constant, no dead code to shift the literal pool, no `#if 0`. If you cannot recover a function, leave it and say so — an honest gap is recoverable, a plausible lie is not.

### The one sanctioned exception: `__deadstripped_<unit>()`

Some target objects open `.rodata` with anonymous all-zero templates that **nothing in the object references** — literals left behind by functions the retail link deadstripped. They are real bytes in the real object, and every later relocation is offset by their size, so they can hold a dozen functions hostage at 99.x%.

This repo reproduces them with a never-called function holding zero-filled locals. **15 files already do this, two of them `Matching` units**; `src/SB/Game/zVar.cpp` is the reference implementation. Follow its shape exactly, including the comment:

```cpp
// These structs were used in deadstripped functions.
// This function is here to force the symbols to be linked.
void __deadstripped_zVar()
{
    const char _449[0x0C] = {};
    ...
}
```

This is **not** the pool-padding hack the rule forbids, and the distinction is the whole point: you are reproducing data the target object demonstrably contains, not inventing code to nudge a number. Three conditions, all required:

1. The objects are **present in the target** and referenced by nothing. Verify it; do not infer it from a percentage.
2. The sizes and anonymous indices come from the target, not from trial and error.
3. You **measure** the before/after and report it. If it does not move the number, remove it.

Name it `__deadstripped_<unit>` and place it where parse order puts the objects at the right point in the pool — usually straight after the includes.

Everything else in this section still stands. A probe function that exists to shift a pool, a stub returning a constant, an unused string — those are still forbidden, and the fact that this one exception exists is not licence to invent others.

## Two things that decide the last 10%

**Branch shape.** Whether the original wrote `if (a < b) {X} else {Y}` or `if (a >= b) {Y} else {X}` is encoded in the emitted branch. Swapping the arms of one `if` has been worth 9–12 points. No decompiler tells you this; only the asm diff does.

**Literal pool layout.** objdiff compares relocations by target **offset**, not symbol name, so `.sdata2` layout is part of matching. A large unwritten function early in a file shifts every offset after it. Write the biggest, earliest function first — filling small ones around it just reshuffles a pool that will move again.

For CodeWarrior source-shape patterns (`x *= k`, inlining rules, switch-vs-if-chain, declaration order), see [references/codewarrior.md](references/codewarrior.md).

When the question is the other direction — **you are staring at target asm and cannot tell what source produced it** — see [references/gekko-asm.md](references/gekko-asm.md). Loop trip counts (the immediate is often not the iteration count), the float/int conversion sequences, which magic constant means which divisor, `/ 8` vs `>> 3`, struct-copy and by-value-argument shapes, the exact `switch` jump-table threshold, and a list of instructions that carry no source information at all so you stop chasing them.

To recover a function that does not exist in our source at all, use the **bfbb-recovering-source** skill.
