# French NPC vector consumers

`tools/platforms/france_npc_vector_constants.py` identifies ten complete
functions, 6,492 code bytes, from authenticated USA, PAL and German originals.
This is a caller/callee cluster proof; it does not claim a complete translation
unit or promote any data extent.

The larger members are NPCC_aimVary (1,248 bytes), MoveFrolic (1,240),
MoveTryToEscape (1,132), CornerOfArena (896), and both 608-byte MoveCorner
implementations. SetHome and three goal Enter functions complete the cluster.

Every original declaration of g_O3, g_X3, g_Y3 and g_Z3 must reference the
original 12-byte xVec3 type with three single-precision members at offsets
0, 4 and 8. All twelve bytes of each vector must match its reviewed payload
in both the original and French initialized storage. Address consumers and
individual scalar loads are checked against that same object and member
offset. The local dst_tetherMax declaration must be the reviewed four-byte
float, and its complete zero initializer and all three load/store operands
must agree. The existing complete CalcNewDir identity independently anchors
the one player-model operand through the original globals/player/ent/model
type path.

The proof checks all 33 changed data operands and 33 JAL instructions per
reference, a fixed set of twelve independent complete dependencies, and an
acyclic order for internal calls. Three unchanged runtime calls retain their
literal instruction words and use a unique 64-byte opaque prefix; that prefix
does not name or bound a runtime function. Every complete body must uniquely
match across the loaded French file spans and pass the existing original and
target control-flow, return/frame, entry-alignment and padding checks. Masks
cover only the reviewed address operands and named call destinations.

Run the original-backed tests with `BFBB_FRANCE_TEST_ORIG` pointing at the
originals directory and `BFBB_FRANCE_TEST_REGISTRY` pointing at the current
French registry. The tests reject changed bodies, transfers, data operands,
constant payloads, original vector member layout and local float type, missing or altered
dependencies, opaque prefixes and a duplicate complete body. JSON round-trip
identity is also required. Private replay output is
`build/npc-vectors-proof.json`.

The private source pilot compiles all three affected units. Five newly
identified bodies are exact: SetHome (244 bytes), CornerOfArena (896), and
three Enter bodies (144, 132 and 240), totaling 1,656 bytes. MoveTryToEscape
is 96.41343%, MoveFrolic 85.83871%, both MoveCorner bodies 95.10526%, and
NPCC_aimVary 91.926285%. These comparisons measure source matching after the
original-only identity proof; they do not supply identity evidence.

The French Robot profile uses address-qualified selectors to retain both
SetHome overloads. Existing symbols, call offsets and provenance are preserved.
The change does not alter debug-region or GameCube source selections.
