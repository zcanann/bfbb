# PS2 platform geometry comparison

The complete existing `src/SB/Core/gc/iMath3.cpp` is now compared against the fifteen original `SB/Core/p2/iMath3.cpp` functions in all four PS2 versions. Each original TU contains 5,944 code bytes. The only source changes select the established PS2 header and restore PS2's independently proven empty `iMath3Exit`; its original eight bytes are `jr ra` and a NOP delay slot. No function bodies are extracted or replaced with scaffolding.

All four actual whole-source comparisons report eight matching functions / 3,652 bytes, with 79.00134% fuzzy code match. Independent application of source relocations reproduces every claimed exact byte, including the 2,568-byte `iBoxVecDist`. The remaining seven functions stay in the comparison. Two runtime calls in `iSphereBoundVec` remain unnamed and unnormalized; the TU is not claimed fully matching or linked.

The normal objects compiled with all four version defines have identical allocated sections. A debug compilation has identical allocated sections too. Its six shared aggregate definitions (`xBox`, `xCylinder`, `xIsect`, `xRay3`, `xSphere`, and `xVec3`) match every original declaration variant in all three debug-bearing originals, including sizes and direct member offsets. GameCube's actual whole-TU compilation retains identical contents in all four allocated sections. No shared header or compiler setting changes.

## French original boundaries

`tools/platforms/france_imath3_sequence.py` corroborates the complete fifteen-member, 6,040-byte sequence against all three named originals. Six independently confirmed neighbors remain unchanged; nine additional functions add 4,184 known CPU bytes. Every body passes the existing strict CFG/frame checks, with real following alignment bytes supplied to the verifier. Original linkage names, full byte comparisons, zero alignment gaps, internal call-entry mappings, and unique whole-sequence placement are verified.

The two opaque runtime calls retain their literal JAL words **unmasked** in the whole-sequence comparison. Their shared 64-byte entry context is extra corroboration only: no runtime name, source ownership, or code extent is invented. The already-known quadratic solver's complete original/French hashes and provenance are checked separately. There are no changed data-address producers or generic CFG exceptions.

The complete French report adds exactly 3,652 matched bytes / eight functions (38,208 / 223 on the tested baseline), with 618 known function boundaries. Every other function/unit record is unchanged. CPU and data denominators stay unchanged. The new boundaries replace unclassified CPU bytes; the complete source comparison covers both the existing and recovered members. Other platforms and previously enabled PS2 source units are unaffected.

Reproduce original metadata validation with:

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```

Private validation artifacts under `build/imath3252`: `original-proof.json`, `profiles.json`, all three `iMath3/<version>/report.json` files, `iMath3/raw-proof.json`, `iMath3/type-proof.json`, `france-raw-proof.json`, `gc/proof.json`, `full-report-proof.json`, and `production/SLES-53623/report.json`. The full French gate reuses authenticated objects for the sixty unchanged prior TUs and adds the actual complete new iMath3 object; all source inputs, command, hashes, and report deltas are recorded.
