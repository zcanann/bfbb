# French robotic goal streak cluster

`tools/platforms/france_goalrobo_streaks.py` proves four complete functions
across USA, PAL and German originals, adding 2,808 independently bounded French
code bytes. Each body has a unique complete instruction template and passes the
unchanged local control-flow checker. No rebuilt source object or changing
registry inventory contributes proof evidence.

| Function | French entry | Bytes |
| --- | --- | ---: |
| xFXStreakUpdate | 0x1e8170 | 208 |
| xFXStreakStop | 0x1e8240 | 96 |
| zNPCGoalAttackArfMelee::FXStreakUpdate | 0x2b7590 | 1636 |
| zNPCGoalAttackHammer::FXStreakUpdate | 0x2b8a50 | 868 |

Both robotic callers have no changed data operands. Their only direct calls
are two calls each to the complete effects helper. The helper has no calls
and one relocated operand: the base of the original sStreakList array. Every
other instruction bit in all four complete bodies remains literal.

Original DWARF proves ten 1,644-byte xFXStreak records, each containing fifty
32-byte ring elements and two xVec3 points per element. The proof checks array
bounds, element identities, sizes, nested offsets, scalar types and runtime
BSS ownership of the complete 16,440-byte storage object. The independently unique complete Stop body must name the same naturally
aligned array base as Update. The data object is
not promoted as a progress extent.

The two goal methods share their human-readable name, so the French source
profile selects their proven address-qualified identities and records all three
original canonical linkages. The existing effects profile gains the update and stop
helpers. Full source compilations match all four new bodies exactly: +2,808
exact bytes and +4 exact functions. No source or compiler change is involved.

This is a complete caller/callee cluster, not a whole GoalRobo TU claim. A wider
survey found 229 original GoalRobo bodies totaling 96,804 code bytes; 225 pass
strict preliminary instruction comparison at the candidate placement. That
survey does not establish data identity, callee closure or ownership for the
remaining functions. The largest contiguous 188-member region remains a search
hypothesis outside the four independently proved bodies above.

Validation:

```powershell
$env:BFBB_FRANCE_TEST_ORIG='C:/Projects/bfbb-bink/orig'
python -m unittest discover -s tools/tests -p test_france_goalrobo_streaks.py -v
```

Tests cover all original bodies and exact JSON roundtrip, changed instructions,
call destinations and typed operands, a duplicate complete caller, and an altered
original nested ring-array bound. Private evidence is
`build/goalrobo-streaks-proof.json` and
`build/goalrobo-streaks-france-pilot/report.json`. Production integration still
requires canonical aggregate regeneration and the full French source report.
