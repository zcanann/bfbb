# PS2 serializer inline and scalar recovery

The serializer's existing deferred-only profile requires explicit inline
boundaries for `prepare`, table initialization, and its empty class-specific
delete. A scoped `auto_inline` region permits the constructor, destructor and
client setter to expand in traversal while retaining their original emitted
functions. Reordering the traversal counter's initialization restores its
register assignment. Using a normalized S32 bit result restores the original
return sequence without changing its zero/one values.

These changes make five functions exact in USA, PAL and Germany:

| Function | Bytes |
| --- | ---: |
| `xSerialStartup` | 296 |
| `xSerialTraverse` | 272 |
| `xSerial::rdbit` | 168 |
| `xSerial::setClient` | 76 |
| `xSerial::~xSerial` | 68 |

Complete source objects increase from 3,088 bytes / 31 exact functions to
3,968 bytes / 36 exact functions, out of 4,212 bytes / 39 functions. All earlier
exact functions and all other scores remain unchanged. Explicitly marking the
constructor/setter inline was rejected because it removed original emitted
members; the retained scoped automatic inlining preserves them. Comparator
rewrites that regressed their existing near-exact results were also restored.

Independent raw-byte comparisons reproduce all 880 new bytes in each debug
original after applying only actual source relocations. Calls and GP operands
use the existing original-backed inventory. HI/LO references to `g_xserdata`,
`g_tbl_onbit`, and `g_tbl_clear` resolve against the original DWARF declarations,
including real addends and carry-adjusted high halves. No target identities,
normalization rules or scoring settings change.

The shared counter-order and Boolean-type changes were measured and regressed
GameCube, so their PS2 guards retain all GU4Y78 scores: 42 exact functions /
4,428 bytes. The inline scopes are also PS2-only. France does not currently
profile this source unit, so no French coverage gain is claimed. These are
function-code results, not a retail executable-link claim.

Private PS2-worktree evidence includes `build/serializer-raw-proof.json`,
`build/serializerproof.py`, `build/serializer-gc-{before,after}.json`, and the
retained complete `ps2solo` source objects and ordinary regional reports.
