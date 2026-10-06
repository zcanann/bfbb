# PS2 compiler profile: auto-inlining and pooled strings

The PS2 source profile (`config/platforms/ps2-toolchain.json`) now compiles with
`-inline auto,deferred,bottomup` and `-str reuse,pool,readonly` instead of
`-inline deferred` alone.

## Evidence

**Auto-inlining.** Many small game functions are ordinary global, out-of-line
symbols in the GameCube target (`config/GQPE78/symbols.txt`), so the original
source did not declare them `inline` (an `inline` function would be weak there).
In the PS2 originals the same functions are inlined into their callers and have no
DWARF function record of their own. For example `zNPCB_SB2::decompose` disappears
into `zNPCGoalBossSB2Death::Enter` (which becomes a tail jump), `deactivate_hand`
appears inside the chop and swipe `Exit` goals, and `update_move` grows from 0x58
bytes on GameCube to 0x174 on PS2. `-inline deferred` without `auto` only expands
functions declared `inline`, so it cannot produce these bodies. Among the inlining
modes tried, `auto,deferred,bottomup` fits best; `level=4` and above are
indistinguishable from it on the units tested, while `level=2`/`3`, `smart`, or
dropping `bottomup`/`deferred` are worse.

**Pooled strings.** No retail GP-relative relocation in any of the 115 profiled
units targets a string literal, and consecutive literals are packed at `strlen+1`
offsets inside one pooled object (`"BoulderVehicleTable\0"` is followed directly
by `"Move01"`). This is the GameCube build's `-str reuse,pool,readonly` setting;
without it short strings land in small data and every load differs.

## Measured effect (all profiled units, whole-unit objdiff)

| Version | Matched code bytes | Exact functions |
| --- | ---: | ---: |
| SLUS-20680 | 226,136 -> 416,368+ | 1,494 -> 1,894+ |
| SLES-51970 | 226,256 -> 423,052 | 1,497 -> 1,922 |
| SLES-53623 | 47,520 -> 66,868 | 252 -> 299 |

(USA figures are before the per-unit overrides below, which recover more.)

## Per-unit exceptions

`xEntMotion`, `xLaserBolt`, `xbinio`, `xserializer` and `xstransvc` match better
with plain `-inline deferred` (a unit-level `"inline"` key, honoured by
`tools/platforms/ps2_source.py`). In `xbinio`, for example, retail tail-calls
`ReadRaw` where auto-inlining expands it. Whether these files were really built
differently, or our callees are smaller than the originals, is unresolved.

About twenty individual functions in otherwise improved units lose an exact match
because we now inline a callee that retail calls (`zNPCRobot::Process`,
`NPAR_Timestep`, `zNPCGoalPatrol::Enter`, ...). These usually mean our callee body
is smaller than the original (often because a helper it calls is not yet an
inline header function on PS2), which is a source-level lever rather than a reason
to keep the old flags.

`tools/ps2solo.py` measures one unit (`--inline` and `--flag` override the profile
for experiments).
