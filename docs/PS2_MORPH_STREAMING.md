# PS2 morph streaming and partition comparisons

The complete existing xMorph.cpp now compiles using a minimal PS2 iMorph header,
the original iModelRender API, and the standard strlen declaration. RpAtomic
remains opaque: this consumer passes pointers without inspecting RenderWare
atomic layouts. The unused debug include is excluded only for PS2; GameCube
keeps its previous include chain and source behavior.

Authenticated DWARF in all three debug originals identifies the platform APIs
as void iMorphOptimize(RpAtomic*, int), void iMorphRender(RpAtomic*, RwMatrix*,
short**, short*, unsigned int, float), and void iModelRender(RpAtomic*, RwMatrix*).
The original morph sequence, frame, and target records are 16, 48, and 32 bytes,
with member offsets agreeing across the three versions and the existing source.

The serialized missing-target index is 0xFFFFFFFF. PS2 retail compares that value
directly; the existing GameCube spelling adds 0x10000 and compares against 0xFFFF.
These conditions are equivalent modulo 32 bits. Keeping the direct comparison
on PS2 removes extra constant construction and addition and matches all 592
bytes of xMorphSeqSetup. GameCube retains its existing expression.

Normal objdiff code results agree across all three debug releases:

| Unit | Original functions / bytes | Matched functions / bytes |
| --- | ---: | ---: |
| xMorph | 3 / 1120 | 2 / 612 |
| xPartition | 7 / 1912 | 2 / 16 |

All original-owned functions remain in each profile. xPartition compiles without
source changes; only its two eight-byte empty routines currently match exactly.
xMorphRender remains at 97.637794 percent. These are standard code-match scores,
not raw whole-unit or executable link claims. The morph runtime string-length
call lacks a reviewed runtime identity, so it stays unresolved rather than
receiving a guessed named relocation. Completed-unit status remains false.

All 224 actual GameCube game/engine objects were recompiled with identical
allocated sections. Private evidence in build/ps2cluster158 contains the bounded
seven-unit inventory, original morph layout/API records, whole source objects,
per-region target objects and normal reports, and the GameCube comparison.
