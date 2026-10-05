# PS2 complete Plankton source comparison

The complete zNPCTypeBossPlankton.cpp compares all 107 original functions /
42,904 bytes in USA, Europe and Germany. Each reports 43 normal code matches /
5,420 bytes, with 40.832558% fuzzy matching. All partial functions remain in the
comparison; no complete-TU or executable-link claim is made.

The only source change excludes a redundant local namespace std declaration of
fabsf on PS2. The PS2 math header already declares the standard C function and
imports it into std; the second declaration caused ambiguous overload lookup.
GameCube retains its existing declaration. No function body, compiler flag,
compiler patch or scoring policy changes.

Actual normal and debug whole-source objects have identical ordered allocated
sections. The broad layout audit compares all 247 shared concrete aggregate
names against each original, retaining every distinct layout for repeated
nested names such as config. Original-only and opaque types are not claimed
verified; ambiguous tri_data is excluded from the broad name intersection.
The six xNPCBasic bitfield widths, offsets and signedness agree after expanding
its two source wrappers.

Two representation differences are checked explicitly. effect_data has an
original eight-byte data union at byte 12, with par, decal and callback all at
union offset zero; the source exposes those same members directly at byte 12.
The enclosing size remains 24 and irate stays at byte 20. The destructible asset
fields hitModelId/destroyModelId correspond to original hitModel/destroyModel;
all four declarations use unsigned integer type and offsets 48/52, with the
same complete 56-byte layout. No physical layout adjustment was needed.

Original boundaries, canonical function identities and named relocation
inverses are checked independently per region. Forty-five initially unknown
references remain unassigned. Independent application of actual source
relocations reconstructs 38 functions / 2,288 bytes exactly in every original.
The remaining standard matches retain unresolved addresses and are not claimed
raw linked equality. The original boundaries include small return-only bodies;
these remain in the full original inventory rather than being discarded.

The GameCube all-source build and retail SHA1 check pass, with its complete
progress report unchanged. Evidence is under build/npc212: ordinary/debug
compiler objects and logs, audit_zNPCTypeBossPlankton.py, each regional layout
inventory, all-region-summary.json, raw-proof.json and gc-verification.json.
