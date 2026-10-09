# French NPC navigation and collision helpers

`tools/platforms/france_npc_helpers.py` verifies 31 complete original bodies,
adding 6,400 independently bounded French code bytes. Each body has a unique
complete instruction template and passes unchanged boundary/control-flow
checks against USA, PAL and German originals.

| Source | New bodies | New bytes |
| --- | ---: | ---: |
| zNPCGoalRobo | 1 | 484 |
| zNPCTypeCommon | 8 | 1452 |
| zNPCTypeRobot | 13 | 2868 |
| zNPCSupport | 9 | 1596 |

The cluster contains complete navigation, collision, bounds, widget and robot
helpers. There are no changed data operands or scoped decoder exceptions.
The sole masked fields are the destinations of 34 explicitly inventoried JALs.
Internal calls follow a checked acyclic order between complete bodies; external
calls must resolve to nineteen fixed independently confirmed identities.
Future registry additions cannot alter those dependencies or emitted evidence.
No whole translation unit is claimed and no rebuilt source contributes proof.

Full source compilation matches nineteen new functions and 3,156 bytes exactly,
including all eight Common helpers and the 484-byte DuckStackInterp method.
The twelve remaining bodies range from 77.0921% to 98.92857%; their proven
targets support further source work. Existing selected function scores remain
unchanged. Source files, compiler settings and every non-French profile are
unchanged by this recovery.

Validation:

```powershell
$env:BFBB_FRANCE_TEST_ORIG='C:/Projects/bfbb-bink/orig'
$env:BFBB_FRANCE_TEST_REGISTRY='C:/Projects/bfbb/config/platforms/SLES-53623'
python -m unittest discover -s tools/tests -p test_france_npc_helpers.py -v
```

Four original-backed tests verify all references and JSON roundtrip, mutated
helper/caller/direct-call instructions, duplicated complete helper bodies, and
missing or corrupted independent callee evidence. The preceding facing, model
and motion proofs supply some fixed dependencies. Replay takes about six
seconds. Private evidence is `build/npc-helpers-proof.json` and
`build/npc-helpers-france-pilot/report.json`; production integration still
requires canonical aggregate regeneration and the full French report.
