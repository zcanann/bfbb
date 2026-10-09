# French NPC player-model consumers

`tools/platforms/france_npc_player_model.py` proves eight complete functions
and 2,948 French code bytes. Every body has a unique complete instruction
template and passes the unchanged local CFG/boundary checks against USA,
PAL and German originals.

Nine changed data operands name exactly `globals.player.ent.model`. Original
DWARF establishes the complete 8,272-byte zGlobals object, the player member at
1,792, its zEnt member at zero, the xEnt base at zero, and the model pointer at
36. The already verified 624-byte CalcNewDir body independently anchors the
same target field. Its full original and French identity is rechecked before
any new consumer is accepted. Both complete global objects must remain within
their original runtime BSS ranges; no data extent is promoted.

The cluster has nineteen explicitly inventoried JALs. Seventeen resolve to
fixed complete callee identities. Two preserve literal runtime destinations
and unique unchanged 64-byte opaque prefixes, without assigning runtime names
or extents. Only proved changed data-immediate fields and named callee JAL
destinations are masked; unchanged LUI words and opaque calls stay literal.
The existing reaching-LUI checker used by the Hangable proof handles the
original floating-point address windows. No decoder or generic rule changes.

Full source compilation matches six new bodies and 1,668 bytes exactly:
Knock::Process, CalcAttackVector, MoveEvade, Notice::Process,
NPCC_DstSqPlyrToPos and FaceAntiPlayer. CalcEvadePos is 95.19139% over 836 bytes;
TurnThemHeads is 96.39639% over 444 bytes. The profile change selects only
French bodies and preserves prior source members. No source or compiler
change is included, and no whole translation unit is claimed.

Validation:

```powershell
$env:BFBB_FRANCE_TEST_ORIG='C:/Projects/bfbb-bink/orig'
$env:BFBB_FRANCE_TEST_REGISTRY='C:/Projects/bfbb/config/platforms/SLES-53623'
python -m unittest discover -s tools/tests -p test_france_npc_player_model.py -v
```

Six original-backed tests cover JSON roundtrip and all references, changed
caller/helper/call words, duplicate complete bodies, missing or corrupted
independent identities, all nine changed field operands, the independent
anchor, opaque context bytes, and an altered original xEnt model-member
offset. Replay takes about fourteen seconds. Private artifacts are
`build/npc-player-model-proof.json` and
`build/npc-player-model-france-pilot/report.json`. Production integration still
requires canonical aggregate regeneration and the full French report.

The combined production gate passes in `build/oct09-france-save-npc`:
all original-backed aggregate registries regenerate identically and all 76
enabled source units compile. Facing, navigation/collision and player-model
clusters add 44 complete identities / 10,844 known bytes. With the independently
validated DuploNotice source fix, exact matching rises by 5,672 bytes / 29
functions, reaching 178,636 bytes / 539 functions. Coverage reaches 819
functions / 388,304 bytes; every earlier score survives and the CPU denominator
stays 2,979,968 bytes. All sixteen original-backed tests pass. Source data,
unnamed runtime identity and a full original executable link remain pending.
