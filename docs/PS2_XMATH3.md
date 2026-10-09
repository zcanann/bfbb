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

## Interpolation, Euler and cone source recovery (2026-10-09)

All four current PS2 source profiles now match 34/35 functions and 7712/8600
bytes, up from 32/35 and 7036/8600. Unit fuzzy matching rises from 93.57628%
to 99.906975%. The existing French original sequence and complete 35-function
profile were used unchanged; the target was copied from the independently
verified terminal French cache. No identities or denominator bytes were added.

Slerp's 420-byte body now matches completely. Keeping weighted quaternion
components in scalar locals, then adding them before normalization, reproduces
the original arithmetic and register lifetimes. The reconstructed pair of temporary quaternion objects forced extra
stack stores. All source components are evaluated before output writes, retaining
input/output aliasing behavior. The original debug locals contain b2 but neither
reconstructed qp1 nor qp2. GameCube and Xbox retain their existing helper calls.

The 256-byte Euler overload also matches completely. Computing the two products
inside their consuming expressions, rather than through cached product locals,
recovers the original register lifetimes. This shared change preserves the
GameCube body and the separately established Xbox expression order.

Cone bounds improves from 52.90991% to 99.0991%. Its circle helper is explicitly
inline on PS2, matching both expansions in the original cone body. Using each
1.0f literal in its consuming square-root expression recovers the floating-point
register allocation. Applying that literal spelling to GameCube regressed its
otherwise exact circle helper, so the existing constant local remains there.
No shared header changes or substitute math implementations are introduced.

Raw relocation replay independently agrees on 404/420 Slerp bytes and 232/256
Euler bytes in each debug original. The remaining ten call words target unnamed
trigonometric runtime entries; compiled names do not establish their identities.
Cone remains 896 compiled bytes against 888 original bytes. A diagnostic audit
locates two extra no-ops at source offsets 84 and 468 in the zero-initialization
loops. Removing those words only in the diagnostic buffer and rebasing branches
makes every noncall instruction agree; this normalization is not used by the
comparison or progress reports. The original source and ordinary residual score
are retained. A later 3.0.1b74 diagnostic still emits the same extra loop no-ops;
2.4 uses a different initialization sequence. These probes do not support a
compiler patch or establish that all remaining source causes are exhausted.

All allocated sections from the full GameCube USA, Europe and Germany units are
byte-identical before/after (38 exact functions / 6900 bytes retained). Both Xbox
production compiles retain identical full source-linked PE code/data sections,
comparison function bytes and named relocations, and all function scores. This
is a before/after control, not a new claim of complete retail Xbox linkage. The
compiler binaries, original targets, profiles and comparison settings are unchanged.

Private evidence in the PS2 agent checkout: `build/math3-final-comparison.json`,
`math3-final-changes.json`, `math3-xQuatSlerp-raw-proof.json`,
`math3-xMat3x3Euler-raw-proof.json`, `math3-cone-residual-proof.json`,
`math3-gc-allocations.json`, `math3-xbox-binary-controls.json`, and
`math3-xbox-whole-link-controls.json`. Compiler diagnostics are under
`build/math3-compiler-deferred-*`; those use auto,deferred, supported by the
older compiler, and are not production comparison results.
