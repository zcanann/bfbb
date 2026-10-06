# PS2 xatan2 header ownership

The PS2 build now uses the existing ordinary `xatan2` definition from
`xMathInlines.h`: `xAngleClampFast(std::atan2f(y, x))`. Its duplicate out-of-line
owner in `xAnim.cpp` remains enabled only on platforms that do not use that inline
definition. GameCube and Xbox preprocessing is preserved. No compiler flags,
source profiles, original metadata, or scoring rules change.

## Original evidence

All three debug executables name a 36-byte `xatan2` function owned by
`C:\SB\Core\x\xMathInlines.h`, returning the DWARF float fundamental type. Its
body calls the same runtime address, `0x114ac0`, then the independently named
`xAngleClampFast`; the latter call's delay slot moves the runtime result to its
argument register. The function addresses are `0x2ab5a0` (USA), `0x2abab0` (Europe),
and `0x2aadc0` (Germany). The previous PS2 header exposed only a declaration,
leaving consumers with an extra call to this wrapper.

The original camera Reset and Rotate bodies contain the two-call sequence
inline. Restoring the existing header body reproduces those instruction
sequences and also completes matrix-to-Euler and NPC direction-to-angle helpers.
The runtime address is corroborating context, not a newly promoted runtime
identity.

## Actual compiler results

The complete 115-unit USA source-profile set was compiled with the existing authenticated
MW PS2 b38 compiler, deferred inlining, exception setting, and version define.
The 17 direct consumer translation units were checked against both other debug
executables; seven of those units have already verified French members and were
compiled and compared there too. All original members, including partials, remain
in the reports. Existing completed camera objects were reused for the regional
checks. The independently expanded 18-member French Math target was also checked.

New standard code matches in every debug version:

| Function | Bytes | Before | After |
| --- | ---: | ---: | ---: |
| `xCameraReset` | 624 | 98.71795% | 100% |
| vector-direction `xCameraRotate` | 408 | 98.039215% | 100% |
| `xMat3x3GetEuler` | 264 | 87.80303% | 100% |
| `NPCC_dir_toXZAng` | 40 | 24% | 100% |

This adds 1,336 matched bytes and four matched functions per debug executable.
France already has verified Rotate and GetEuler extents, so it gains 672 bytes
and two functions. No unverified French boundaries are inferred. The expanded
French Math comparison retains its nine exact functions / 748 bytes and improves
only the still-partial cubic solver.

One partial SB2 predicate decreases from 31.333334% to 30.246666%; it is retained
and reported, not excluded from the helper or comparison. The actual unchanged
baseline body is 252 bytes and the candidate is 268 bytes, against a 600-byte
original. Their complete instruction difference is two `xatan2` call relocations
becoming `atan2f`, insertion of two `jal xAngleClampFast; mov.s f12,f0` pairs, and
one adjusted branch displacement. All other instructions are identical. The
original has exactly both clamp/delay pairs. Its much larger remaining difference
is unrelated missing inlining of platform lookup, vector subtraction, and angle
modulo; the lower fuzzy alignment score does not represent removed original
instructions. No exact function regresses.

After independently named call and static-data relocations are applied, the four
newly exact bodies differ from the debug originals only at existing unresolved
runtime-call words (`atan2f`, `xasin`, and Reset's `memcpy`). Consequently these are
standard code-match claims, not complete linked-byte reconstruction claims.

## Reproduction evidence

Private original/compiler artifacts remain under `build/near273/` in the isolated
`bfbb-agent-math273` worktree:

- `all/SLUS-20680/report.json`: complete actual USA consumer build/report;
  `production-compile.json` records all compiler outputs.
- `regional/<version>/{report,comparison}.json`: complete source-unit comparisons
  against authenticated original targets and verified CI baselines.
- `original-xatan2-body.json` and `original-camera-proof.json`: original ownership,
  function bytes, and call-chain evidence.
- `sb2-regression.json`: full original/baseline/candidate disassemblies and calls.
- `raw/proof.json`: independently restored relocations and every remaining unknown
  runtime word, across all three debug executables.
- `math-expanded/comparison.json`: independently confirmed full French Math target.
- `gc/proof.json` and `gc-xAnim/proof.json`: actual GameCube compilation retains all
  16 and seven ordered allocated sections respectively.
- `xbox/proof.json`: actual pinned MSVC preprocessing of the complete Math source
  has identical nonblank lines before and after the header change.

The baseline is the actual verified platform266 CI report. USA comparisons also
cross-check platform264's actual CI functions plus the independently verified
later volume callback result; those baseline function names, sizes, and scores
agree. Denominators and completeness flags remain unchanged.
