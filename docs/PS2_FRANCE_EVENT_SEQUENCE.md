# French event sequence and group-call closure

`france_event_sequence.generate_unit` proves the complete 912-byte original
`xEvent.cpp` sequence at `0x283840`: ten overloads totaling 880 function bytes.
It starts from the independently proven group callback, re-running that proof
from the older item-accessor anchor. Three original callback JALs at offsets
368, 520, and 632 must all resolve to the same 32-byte event overload at
`0x283a20`. Generated event records are never used as their own anchors.

All three authenticated debug originals must agree on complete membership,
canonical overload linkage names, exact instruction operands, zero alignment,
and a unique complete sequence placement. The main implementation and four
other wrappers pass the existing strict return/stack verifier. Five 32-byte
tail wrappers require exactly seven original DADDU register moves and one J to
the same strict 412-byte main body. Every move is confined to the original
argument/temporary registers; SP and RA remain unchanged. This specialized
check does not relax the general control-flow verifier.

The only external destinations are already independently identified
`zSceneFindObject` (128 bytes), `xSTFindAsset` (444), and `xStrHash` (88).
Every external edge rechecks its complete identity, hash and provenance in
all three originals. No data operands, runtime identities, compiler settings,
or scoring settings are changed. French profile keys include addresses to
distinguish the ten overloads using their proven linkage provenance.

The actual complete source object reports all ten functions / 880 bytes exact.
The group callback now restores all five of its real call relocations,
including the three formerly unresolved event-wrapper calls. Independent raw
comparison against the French ELF reproduces all 1,592 bytes comprising the
ten event functions and the 712-byte group callback, after applying exactly
22 real `R_MIPS_26` relocations. This supersedes the unresolved event-call caveat
in `PS2_FRANCE_GROUP_SEQUENCE.md`; it does not establish a fully linked retail
translation unit or executable.

Private artifacts in the PS2 worktree are `build/event-france-proof.json`,
`build/event-france-diagnostic`, the updated `build/group-france-diagnostic`,
`build/event-group-france-raw-proof.json` and its replay script, and
`build/event-proof-negative-controls.json`. Negative controls reject altered
main arithmetic, a named external callee redirected to another identity,
a tail shuffle that writes SP, and a group caller redirected to another
overload. Python syntax checks pass. The standalone module/profile deliberately
leave shared generated registries and hooks for the combined integration gate.
