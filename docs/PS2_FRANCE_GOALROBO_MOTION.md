# French NPC motion and robotic goal cluster

`tools/platforms/france_goalrobo_motion.py` proves 31 complete French bodies
covering 5,292 bytes across five source units. Each body is independently
unique and passes the unchanged control-flow and boundary checks against USA,
PAL and German original executables. No rebuilt source contributes evidence.

| Source | New bodies | New bytes |
| --- | ---: | ---: |
| zNPCGoalRobo | 20 | 3216 |
| zNPCTypeCommon | 6 | 1324 |
| zNPCTypeRobot | 3 | 440 |
| zNPCSupport | 1 | 184 |
| zNPCHazard | 1 | 128 |

The eleven motion helpers have no direct calls and no changed instruction
bits. Their twenty consumers have no changed data operands; the only masked
bits are the destinations of 34 explicitly checked JAL instructions. Every
destination is either a complete literal helper in this proof or one of eight
fixed, independently verified identities. The dependency address inventory is
fenced so later registry additions cannot change emitted evidence. This is a
complete caller/callee cluster, not a claim to any whole translation unit.

The source profiles preserve existing robotic-goal and hazard members. Selecting
the Support unit also exposes the already verified 104-byte NPCC_TmrCycle and
72-byte IsVisible bodies, retaining their original corroborated registry
provenance. These two do not add known-code coverage.

Full source compilations match 30 of the 31 newly proven functions exactly:
4,828 new exact bytes. ZoomMove is 98.18965% over 464 bytes. Both existing Support
members match exactly, so the profile change adds 5,004 exact bytes and 32 exact
functions in total. Source and compiler files are unchanged, as are every
non-French profile.

Validation:

```powershell
$env:BFBB_FRANCE_TEST_ORIG='C:/Projects/bfbb-bink/orig'
$env:BFBB_FRANCE_TEST_REGISTRY='C:/Projects/bfbb/config/platforms/SLES-53623'
python -m unittest discover -s tools/tests -p test_france_goalrobo_motion.py -v
```

The registry must include the preceding streak cluster. Original-backed tests
check all three references, exact JSON roundtrip, mutated helpers, callers and
call destinations, a duplicated complete helper, and missing or corrupted
independent identity evidence. The original replay takes about six seconds.
Private pilot artifacts are `build/goalrobo-motion-proof.json` and
`build/goalrobo-motion-france-pilot/report.json`. Production integration requires
canonical aggregate regeneration and the full French source report.
