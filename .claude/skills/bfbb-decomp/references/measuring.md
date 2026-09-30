# Which number means what

Four measurements exist and they disagree on purpose. Quoting the wrong one is how this project has repeatedly fooled itself.

| Tool | Measures | Use it for |
|---|---|---|
| `tools/solo.py` | per-function % against the target object, one unit, private temp build | all before/after work inside a unit |
| `build/GQPE78/report.json` | project metric: `matched_functions`, `fuzzy_match_percent`, `complete_units` | the headline; produced only by a real `ninja` |
| `build/GQPE78/progress.json` | what the progress site shows: `matched_code` bytes | quoting external progress |
| DOL sha1 | whether `Matching` units still link byte-identically | the only proof for a `Matching` unit |

**Never compare a solo.py number against a report.json number.** They count by different rules.

## Keep solo.py's output small while you iterate

The default listings are big — a unit list is 45–120 rows and a symbol diff is one row per instruction, so a 2 KB function prints ~500. Every one of those rows stays in context for the rest of the run, and you will run the tool dozens of times.

- `--bands` — a six-line histogram of match quality, by count **and by bytes**, with absent functions counted separately. This is the right first command on an unfamiliar unit: it costs six lines instead of a hundred-row listing and it answers the only question that matters at that point — is the remaining work a few broken bodies, or a pool ceiling holding a crowd at 99.x%?

  The byte column is what makes it worth reading. `zEntPlayer` shows 224 exact but **45 functions in the 90–99 band holding 81,840 bytes** — far more than any other band, and invisible in a count-only view. `zEntCruiseBubble` shows the opposite shape: 176 exact and 44 stacked in 99–100, which is the ghost-literal ceiling, not work.

  Absent functions are listed on their own line rather than folded into the 0% band. "Not written" and "written wrong" are different jobs, and conflating them has pointed agents at the wrong target.

- `-q` / `--quiet` — header line only. This is what you want between edits, when all you need is whether the non-matching count moved.
- `--top N` — the N worst rows. Enough to pick the next target.
- Symbol diffs now print **only differing rows plus 3 lines of context** by default, with `... N identical` markers. `-C 1` tightens it further, which is the right setting when a function is at 99.x% and every difference is an isolated relocation. `--full` restores the every-row output when you genuinely need to inspect instruction scheduling or ordering.

Truncation always says what it withheld, so a short listing is never mistaken for a clean one. Take the full listing once at the start and once at the end — that is what you report — and stay in `-q` in between.

## Read one target function, not a whole object

`dtk elf disasm` writes the entire object — 1.2 MB for a unit like `zNPCGoalRobo` — and greping around inside it costs context on every hit. `tools/tasm.py` disassembles once, caches under `build/.tasm-cache/` keyed by the object's mtime, and hands back only what you asked for. It always reads the **target** object, so it is ground truth; use `solo.py` to compare against ours.

```
tasm.py <unit-frag> <symbol-frag>   the function's disassembly
tasm.py <unit-frag> --list          every function, name and size, largest first
tasm.py <unit-frag> --pool          .sdata2 in slot order, with each slot's first user
tasm.py <unit-frag> --order         .text definition order = literal interning order
```

The `/* addr off bytes */` prefixes are stripped unless you pass `--raw`; that alone is 62% of the bytes (a 3852-byte function goes 55,829 → 21,161 chars). Keep the prefixes only when you actually need addresses, e.g. resolving a branch target.

`--pool` and `--order` together are the pool-permutation diagnostic from `codewarrior.md` in two commands. Run `--pool` on the unit and compare the slot order against ours; the first row that disagrees names the function that interned wrongly. `--list` is the right way to start on an unfamiliar unit — the largest absent functions are where the bytes are, and working alphabetically wastes a session.

**Beware stale objects when comparing solo.py against the built object.** `solo.py` always compiles fresh into a temp dir. `build/GQPE78/**.o` is only as current as the last successful `ninja`, and a build interrupted by an unrelated compile error leaves some objects untouched.

This produced a convincing false alarm worth recognising. `check_hide_entities` measured **100% under solo.py and 91.047% in `build/`'s object**, with report.json agreeing with the stale object — which looked exactly like solo.py being unfaithful to the real build, in a unit nothing had touched. After a clean `ninja` all three agree at 100%. Nothing was ever wrong.

**Before concluding that two measurements disagree, re-run `ninja` to completion and re-measure.** A stale `.o` explains far more disagreements than a genuine tooling difference does.

**Run `ninja` twice after a large multi-file change, and trust the second report.** This is reproducible, not superstition: after two agents' work landed together, the first build reported `zCutsceneMgr` at 14/18 with `check_hide_entities` at 91.047%, and an immediately following build — same sources, nothing edited in between — reported 15/18 and 100%. It has now happened three times, always in the same unit, always on unrelated changes. The first report can be assembled against objects the same run is still replacing. The DOL sha1 was correct every time; only the report was wrong.

**The flake can present as `matched_functions` going *down*, which reads exactly like a regression.** On the `zEntPlayer_Update` wave the headline went 7641 → 7640 while `fuzzy_match_percent` went *up*. A controlled rebuild against HEAD showed the real deltas were `zEntPlayer +2` and `zCutsceneMgr -1`, and one further `ninja` on identical sources put `zCutsceneMgr` back. Two checks disambiguate this in about a minute, and both are worth doing before you believe a −1:

- **Ask whether the change could even reach that unit.** Only `zEntPlayer.cpp` had been edited — no shared header, and `zCutsceneMgr` does not include it. A unit that cannot see your change cannot have regressed from it.
- **Re-measure the accused unit with `solo.py`,** which compiles into a private temp dir and is deterministic. It put `check_hide_entities` at 100% while the built object still claimed otherwise.

Do not revert a wave on a −1 in `report.json` alone.

The residual difference is benign: counting non-matching symbols off the built object includes data symbols like `[.sdata2-0]`, which `solo.py` does not, so the built-object count runs a little higher on most units.

## A whole-function percentage is not a verdict on the hunk you changed

The percentage measures alignment across the entire function. A *correct* local fix can lower it, because fixing one region shifts everything after it and misaligns a tail that happened to line up before. Judge a hunk by diffing the instructions in the region you touched, not by the headline number.

This cost a real lead. `get_next_quadrant` needed two independent changes, and the numbers in isolation are actively misleading:

| form | % |
|---|---|
| original source | 85.154 |
| loop fix alone | **79.135** |
| `%` instead of `- row * count` alone | 86.308 |
| **both together** | **99.712** |

An earlier pass applied the loop fix, watched 85 → 79, reverted it, and concluded the function needed a *compiler patch* to reproduce a supposed CSE-across-store bug. The loop body was already byte-exact at 79.135%; only the tail had moved. The premise was then refuted outright by a control experiment — feed the compiler the semantically-correct form and it emits `stw` before `slw`, so it uses the new value when the source asks for the new value. There was never any wrong code to reproduce.

Two habits follow. When a plausible fix makes the number worse, **look at whether the region you changed now matches** before reverting. And before concluding that the compiler is at fault, **compile the alternative and look** — a control experiment is cheap next to a compiler patch, and "the compiler has a bug" should be the last hypothesis standing, not the first.

## report.json credits near-misses

It counts functions at 99.x% as matched. Consequences:

- A wave that moves seven functions from ~0.5% to 72–100% and promotes six neighbours to exact can show **+1** in report.json.
- A change measuring **+7 exact** in solo.py showed **+0 functions and −1 complete unit** in report.json, because every one of the seven was a 99.x%→100% crossing already being counted.

Judge stub-filling and near-miss work by solo.py. Judge project progress by report.json.

## Byte-neutral changes and how to prove them

Renaming identifiers **cannot** change codegen — names never reach a non-debug object. So after a pure rename, every percentage must be identical. Any movement means you changed something other than a name.

Reordering local declarations **does** change codegen, because declaration order drives stack slot assignment. Different rules apply: keep if the target improved and no neighbour regressed, revert otherwise.

Two permanent tools do this now. They replace the `vs_head.py` / `snapshot.py` scratch scripts that used to be rebuilt every session — and rebuilt wrong, which is where the three bugs below came from.

```
tools/blast.py <header-frag>              TUs that transitively compile it
tools/blast.py <header-frag> --why <unit> the shortest include chain
tools/sweep.py snap <out.json> <unit>...  measure many units
tools/sweep.py snap <out.json> --all-affected <header-frag>
tools/sweep.py cmp <before.json> <after.json>
```

Run `blast.py` first, because the answer is usually alarming. `xVec3.h` reaches **188 TUs, 64 of them `Matching`**; `xClumpColl.h` 169/55; `xParEmitterType.h` 131/38; `containers.h` 75/21. A filename grep suggests a small fraction of that. It buckets by `configure.py` status, and the `Matching` count is what decides how careful to be — those units must stay byte-identical or the DOL sha1 breaks.

Then `sweep.py snap before.json --all-affected <header>`, edit the header, snap again, `cmp`. It exits non-zero on any regression so it can gate a commit, and it calls out separately any symbol that fell off 100%.

`sweep.py` records **every symbol including data**, not just functions and not just the ones at 100% — that is what catches the `.sdata2` case below. It refuses to write a snapshot if any unit failed to measure, so a half-populated file cannot be compared against later.

**A snapshot tool that cannot fail loudly is worse than no tool.** Three separate bugs in one session each produced a confident, wrong `CLEAN`:

- `solo.py` reports `COMPILE FAILED` and ambiguous-unit errors through `SystemExit`, which prints to **stderr**. A tool reading only stdout turns every failed compile into an empty result, and an empty result compares clean against anything.
- Unit lists written by Windows Python arrive with trailing `\r`. `find_unit` matches on a substring, so every lookup misses. In practice only the *last* line worked, because it alone had no `\r` — which looks like a mass compile failure rather than a quoting bug.
- **Success is the presence of the summary header line, not the presence of rows.** A fully-matching unit legitimately prints zero rows. Treating "no rows" as failure silently drops exactly the units already at 100% — the ones a shared-header change is most likely to break.

Assert the expected unit count and a non-trivial total row count, and exit non-zero when any unit fails to measure.

**Two agents sharing a scratchpad will overwrite each other.** Fixed filenames like `baseline.txt` get clobbered mid-run, and the resulting comparison is nonsense in a way that looks like a real regression. Give every concurrent agent a unique filename prefix.

**Do not swap a shared header underneath a running agent.** The measurement needs HEAD's header in the tree for one pass, and any agent compiling a TU that includes it during that window gets numbers that silently belong to the wrong tree. Either wait for the agent to finish, or have it compile against a shadow copy of the header directory (prepend `-i <shadow>` to the real cflags) so the real tree is never touched.

## Three ways this project has fooled itself

**1. objdiff is blind to definition order.** It pairs symbols by name. Deleting a function from a `.cpp` and letting a header inline provide it relocated the symbol inside `zEnt.o`; objdiff still reported the unit at 38/38 while the object was no longer byte-identical, and the DOL sha1 failed. **A unit reading 100% in objdiff is not proof its object is byte-exact.**

**2. Comparing only exact-match sets hides regressions.** A comparison that diffs "which functions are at 100%" reported a change as clean while it had dropped two units' `.sdata2` match (65.2%→63.8% and 92.1%→90.6%). No function crossed the 100% boundary, so nothing showed. Always diff the *full percentage distribution*, including data symbols.

**3. Raw `mwcc` output is not comparable to the target object.** A tool that compiled a unit and byte-compared it against `objdiff.json`'s target reported *every* unit as differing, including ones known byte-exact. dtk post-processes the extracted objects. Always run a control on a known-good unit before trusting a new measurement tool.

## Weak inline symbols appear in only one target object

An `inline` function that CW also emits out-of-line produces a **weak** symbol in every TU that uses it. The linker keeps one. dtk extracts target objects from the *linked* DOL, so the survivor shows up in exactly one object and is absent from all the others.

So "our object defines a weak symbol the target object does not" is the expected state, not a bug. `xDrawLine__FPC5xVec3PC5xVec3` is the worked example: `.text:0x8001F430`, `size:0x4`, `scope:weak`, landing in `xEntMotion.o` — the first TU in link order that calls it — while the five other calling TUs show it only on our side. The empty `inline` in `xDraw.h` is correct; the 4 bytes are the `blr`.

Check `config/GQPE78/symbols.txt` for the mangled name before concluding a symbol is spurious. `scope:weak` there means the source shape is already right.

## Before touching a shared header

Find the real blast radius, which is wider than a filename grep suggests — headers reach far more TUs transitively. What matters is whether any of them is `Matching`, because a `Matching` unit must stay byte-identical or the DOL breaks.

```
grep -n '"SB/Game/<unit>.cpp"' configure.py     # Matching / NonMatching
```

A unit containing a 0%-fuzzy function cannot be byte-identical, so it is never `Matching` — those units are safe to fill in freely.

When retail inlined a helper into some callers and not others, a single global choice is wrong either way. Define the inline in the header and let the TUs that must not expand it opt out with a macro (see `XSNDPLAY3D_OUT_OF_LINE`). Defining the body `WEAK` instead is catastrophic: it emits a weak copy into every including TU, measured at −217 exact across 24 units.
