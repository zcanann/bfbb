# French particle emitter clusters

`france_npc_particle_emitters.py` identifies twelve complete particle emitters
in `zNPCSupplement.cpp`, totaling 2,904 bytes. Four H2O wrappers form the
872-byte span at `0x3b48b0`; eight tube/oil wrappers form the 2,092-byte span at
`0x3b4de0`. Both spans include original inter-function zero padding. These are
complete ordered function clusters, not a whole translation-unit claim.

Several individual wrappers have identical instruction templates after address
masking. Their identities therefore require the complete cluster, all three
original DWARF member lists and canonical linkages, exact relative offsets,
complete function sizes, every aligned entry, and all inter-body and terminal
padding. Each full cluster has exactly one location across all loaded French
segments. Every member independently passes the ordinary closed CFG checker.
No seed-size or boundary rule is weakened.

Changed data operands select original typed array elements:

| Original array | French base | Element count | Element bytes |
| --- | --- | ---: | ---: |
| `g_npar_mgmt` | `0x5e6240` | 12 | 32 |
| `g_parm_chucksplash` | `0x506ca0` | 5 | 32 |
| `g_parm_tubeconfetti` | `0x506be0` | 2 | 40 |
| `g_parm_tubespiral` | `0x506ac0` | 4 | 12 |
| `g_parm_oilbub` | `0x506a50` | 4 | 28 |

Each declaration's complete array descriptor, element type and size are checked
against original DWARF. The management field layout and signed counters agree;
its particle-buffer pointer names the original 80-byte NPARData type. Every
changed address selects the same aligned element index in reference and target.
All initialized parameter-array bytes agree, the management array lies wholly
inside BSS, and the complete arrays do not overlap. One additional operand uses
the independently typed, complete zero vector `g_O3`. No data extent is promoted.

The fixed dependencies are three complete ConfigPar bodies and xurand. All
other transfers are literal, unchanged calls to `0x118c08`, corroborated by
identical 64-byte original runtime contexts. Those words stay unmasked and do
not supply a runtime name, extent, relocation or progress. Only the verified
complete callees and typed address halves may be masked.

The French source pilot matches all sixteen selected Supplement functions and
5,176 bytes exactly: twelve newly exact functions / 2,904 bytes, plus the four
unchanged ConfigPar functions. Source selection expands; the executable's full
code denominator does not change. No source implementation is changed here.

Private artifacts are `build/emitter-proof.json`, `build/emitter-tests.txt`,
`build/emitter-france-pilot/report.json`, and the `build/hazard-emitter-*`
original inventories. Tests cover full cluster duplication, changed function
membership/owner/name/size, array bounds and element types, parameter bytes,
operands, padding, runtime context, complete callees and dependency stability.
All six original-backed tests passed in 236 seconds; the independent three-
reference proof replay completed in 29 seconds.
