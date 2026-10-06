# PS2 xrmod header ownership

The existing `xrmod` body is now available inline from `xMath.h` for PS2.
`xCameraMath.inl` retains the original private definition for other platforms.
This restores the original ownership without changing the arithmetic, compiler
flags, source profiles, or comparison policy.

## Original evidence

All three debug originals identify `xrmod` as a 176-byte float function owned by
`C:\SB\Core\x\xMath.h`. Its addresses are `0x2ab4f0`, `0x2aba00`, and `0x2aad10`
for USA, Europe, and Germany respectively. The body computes the fractional turn
using the exact existing `0.15915494f` constant, branches on negative and
at-least-one inputs, and has the existing ceiling/floor arithmetic shape and
`6.2831855f` constant. Original inline consumers reuse its two runtime
destinations; those runtime identities are not promoted here.

Original consumers provide separate corroboration. The three SB2 predicates and
updates improved here contain that modulo algorithm inline; the old source
instead called `xrmod`. The same inline runtime-call pair appears in the original
Plankton yaw/orbit functions, Prawn beam sweep, and OOB grab initialization. These
are original bytes and independently named function extents, not newly inferred
function boundaries. Runtime addresses remain contextual evidence, without new
runtime-identity or linked-executable claims.

## Measured changes

Actual complete source translation units are compiled for every profiled caller:
camera, Plankton, SB2, Dutchman, Prawn, Bungee, cruise bubble, disco floor, and OOB.
All original functions remain in each report, including incomplete ones. The
baseline is the preceding verified xatan2 restoration, with the unchanged disco
floor unit taken from the actual full baseline report.

All nine complete callers were compiled and compared in each debug version.
The existing French camera comparison was also compiled and remains unchanged.
Every function delta below is identical across all three debug versions:

| Function | Before | After |
| --- | ---: | ---: |
| Plankton idle `get_yaw` | 48.18182% | 91.46753% |
| Plankton `orbit_yaw_offset` | 46.46923% | 75.93077% |
| Plankton `update_move_orbit` | 58.13072% | 74.28432% |
| SB2 chop `can_start` | 30.246666% | 57.76% |
| SB2 `update_nodes` | 61.39423% | 92.25961% |
| SB2 `update_follow` | 74.49123% | 86.37193% |
| Prawn beam `update_sweep` | 60.44371% | 78.78808% |
| OOB grab `start` | 41.36% | 71.416% |
| Bungee hanging `start` | 54.985874% | 49.718925% |

This is a partial-code improvement; it adds no newly exact functions. Every
previous exact function remains exact.

## Explicit Bungee tradeoff

Unlike the improved consumers, the original Bungee hanging `start` retains an
out-of-line `xrmod` call at offset 2020. Its original body is 2,832 bytes, the
previous source body is 2,064 bytes, and the candidate is 2,192 bytes. The candidate
inlines the modulo body where the previous source called it at offset 1652.

The caller already has substantial unrelated inline-boundary differences. Source
calls `show_models` once, `update_hook_loc` twice, and `play_sound` three times;
the original expands them, including twelve directly named player-sound API calls
in place of the three sound-helper calls. The much shorter source caller is
consistent with a different compiler inlining-cost decision, but that specific
cost explanation is not proved. The measured Bungee regression is preserved
explicitly. There is no consumer exclusion or forced no-inline workaround.

## Private reproduction artifacts

`bfbb-agent-math273/build/near275/` contains the authenticated original helper and
consumer proofs, complete original/baseline/candidate Bungee disassemblies, and
actual whole-source compiler records. `shared/<version>/report.json` and
`comparison.json` retain the full function-level deltas. `gc/proof.json` records
unchanged ordered allocated sections for an actual GameCube camera compile;
`xbox/proof.json` records identical nonblank output from the actual pinned MSVC
preprocessor. Public source changes contain no original binary bytes.
