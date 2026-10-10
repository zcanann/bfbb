# French Ambient and Villager animation clusters

`france_npc_townsfolk_animation.py` proves nine complete functions in two
independent original-order clusters. Ambient contains Neptune, Jelly and
Ambient builders: 1,368 code bytes in 1,376 bytes at 0x2cf230. Villager contains
SuperFriend, BalloonBoy and Villager builders, each followed by its eight-byte
no-argument wrapper: 2,608 code bytes in 2,632 bytes at 0x2ef4d0. Both complete
masked templates must be unique across all loaded French file spans.

All three originals must agree on the exact DWARF source owners, linkage names,
member order, relative entries, full extents and zero padding. The six full
builders pass existing frame/return/control-flow checks. Each short wrapper
is exactly J to its corresponding complete local builder with
`daddu a0, zero, zero` in its delay slot. It preserves SP/RA and passes the
callee's complete boundary check. These wrappers are identified by their
complete cluster offsets and original DWARF order; there is no short-seed
exception or whole-TU claim.

Each reference inventories 74 changed address operands and 62 direct transfers.
The internal graph permits only the three proved wrappers and SuperFriend /
BalloonBoy calling the complete Villager builder. Exactly five independently
established xAnim/Common identities provide external call/callback evidence.
Both complete original char-pointer tables and their strings/aliases are
verified: g_strz_ambianim[12] and g_strz_folkanim[26]. Five local signed-integer
initializers total 164 bytes and must reach their exact original DWARF stack
objects with complete, non-overlapping byte coverage.

Neptune uses the existing straight-line initializer checker. Jelly's separate
scoped prefix permits the initial complete xAnimTableNew call only before any
initializer pointers or values become live; the full copy then finishes by
the next call's delay slot. Villager copies finish in the delay slot of the exact
non-likely `BEQ a0, zero, +3`; both successors therefore see the complete typed
array. No later branch or arbitrary call is accepted by these scoped rules,
and the generic straight-line checker is unchanged.

Twelve original-backed tests pass (91.5 seconds). They reject mutated wrappers/destinations, call and callback bodies,
cluster membership/order, padding, duplicate complete clusters, arrays and
payloads, types/counts/stack locations, copy coverage, the guarded split and
its delay store, and Jelly's preceding call. JSON round-trip identity is
required. Evidence is `build/npc-townsfolk-animation-proof.json` and
`build/npc-townsfolk-animation-tests-final.txt`. No data extents are promoted and no
compiled source supplies function identity evidence.

All nine new French source functions match exactly: +3,976 exact bytes and
+9 functions. Ambient adds a new source profile with 1,368 exact bytes in
three members. Villager adds 2,608 exact bytes in six members; its prior
280-byte SceneTimestep retains every score/metadata field (69.64286%).
The Villager profile uses address-qualified selectors to distinguish each
wrapper from its overloaded full builder, preserving the earlier member's
linkage and call evidence. All non-French profiles remain unchanged.
Source evidence is `build/npc-townsfolk-animation-france-pilot/report.json`.
