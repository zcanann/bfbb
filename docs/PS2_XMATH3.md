# PS2 xMath3 source compilation

The complete shared `xMath3.cpp` compiles with the existing MW PS2 b38
profile and native math headers. A PS2 `iMath3.h` declares the geometry APIs
whose signatures are recorded by the retail DWARF linkage names; it leaves
`xiMat4x3Union` opaque instead of importing an unverified RenderWare layout.
The unused `intrin.h` and `xClimate.h` includes remain on GameCube, where the
latter still owns a 12-byte zero-data section. Four absolute-value expressions
use the shared `iabs` platform API. All GameCube allocated sections remain
byte-identical under the production compiler and flags.

Independent application of actual source-object relocations verifies 10
functions / 1,124 bytes against all three debug-bearing PS2 releases. In
addition, `xMath3Init` reproduces its complete 220-byte original body when its
real call and 24 adjacent data-address pairs are linked to the original DWARF
anchors. These are `LUI` followed by `LWC1` or `SWC1`, loading/storing components
of `g_X3`, `g_Y3`, `g_Z3`, `g_O3`, and `g_I3`. Component offsets stay as implicit
REL addends; the check reconstructs every original instruction and compares
the entire source body to the original bytes independently.

The standard objdiff instruction score is 13 functions / 1,772 bytes. It uses
the same relocation-comparison policy as GameCube and does not establish raw
linked equality: `xQuatToAxisAngle` calls an unnamed runtime entry through
`xacos`, and `xMat3x3RMulRotY` through `icos` and `isin`. The independently
verified raw subset is 11 functions / 1,344 bytes including initialization.
No complete translation-unit or executable source-link claim is made.

The target relocation backend accepts these adjacent floating load/store
pairs only with explicit `lo_opcode` 49 or 57 and a named original data anchor.
It checks the high-half register, low-half base, component displacement,
original memory range, and control-flow entry; unknown shapes are rejected.
The implicit addend is preserved in the reconstructed REL instruction fields.
Existing profile target bytes and relocation records remain identical in all
three debug-bearing releases.

Private evidence: `build/ps2math3_154/{raw-proof,init-raw-proof,gc-sections}.json`,
`raw_verify.py`, `verify_init.py`, the actual full source object and compiler
command. All three original executable SHA-1 values are authenticated by the
verification scripts before byte comparisons.
