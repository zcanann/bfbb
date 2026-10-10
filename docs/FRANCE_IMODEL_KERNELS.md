# French model leaf functions

`france_imodel_kernels.py` proves four complete original functions, totaling
1,272 code bytes in the PS2 model source unit:

| Function | French entry | Bytes |
| --- | --- | ---: |
| iModelMaterialMulCB | 0x1ad240 | 712 |
| iModelSetMaterialAlpha | 0x1ad7f0 | 224 |
| iModelCull | 0x1aee30 | 288 |
| NextAtomicCallback | 0x1af3d0 | 48 |

Each complete body has the same authenticated DWARF owner, canonical linkage
and extent in the USA, PAL and German originals. Each independently passes
control-flow/frame/return checks, preserves its zero alignment padding, and
has a unique complete masked-template match across all French loaded file
spans. There are no direct calls or short-seed exceptions. The surrounding
10,076-byte / 30-function iModel candidate remains discovery evidence only;
its changed, unnamed RenderWare calls are not masked or promoted here.

Exactly three changed address pairs are permitted, all taking the address of
an original typed BSS object. Material color storage is RwRGBA[16], with four
unsigned-byte fields at offsets 0 through 3; saved alpha storage is unsigned char[16].
The frustum pointer follows the original zGlobals base xGlobals at offset zero,
its xCamera member at zero, and xVec4 frustplane[12] at offset 624. Vector
components are four floats at offsets 0, 4, 8 and 12. A fixed, previously proved
complete CalcNewDir body independently anchors the shared globals base through
its typed player-model operand. All declaration, inheritance, aggregate size,
array type/count, component offset and BSS storage checks must agree in every
reference. No data extent is promoted.

The full integer instruction comparison preserves all raw VU encoding bits,
including destination masks. Cull additionally records and checks all 42
COP2/vector-memory words: 26 VU arithmetic words, four COP2 transfers and
twelve vector loads. A decoder's abbreviated display cannot hide mismatched
mask bits in this proof.

Original-backed tests reject altered function extents, bodies, data operands,
padding, VU words, duplicate complete bodies, missing/changed independent
anchors, original material array counts/element types, component types, camera
field offsets and global base declarations. JSON round-trip identity is
required. Private evidence is `build/imodel-kernels-proof.json` and
`build/imodel-kernels-tests.txt`. Source comparison is separate from identity.

Seven original-backed tests passed in 95.9 seconds; standalone complete replay
took 22.7 seconds. The French source pilot adds 984 exact bytes and three exact
functions. MaterialMulCB (712 bytes), SetMaterialAlpha (224 bytes), and
NextAtomicCallback (48 bytes) match exactly; Cull (288 bytes) scores 90.19444%,
the same as all three debug references with the restored PS2 VU source. The
selected four-function unit scores 97.77988% fuzzy overall. Its private report
is `build/imodel-kernels-france-pilot/report.json`. The full canonical registry
replay and production report remain integration gates.
