# Further French PS2 source comparisons

Eight already-compilable shared translation units now have France-only comparison profiles covering every function currently confirmed for each unit. Complete source files are compiled normally; unknown original members are not invented or assigned ownership. No source, header, compiler setting, boundary registry or scoring rule changes are needed.

| Source unit | Confirmed French functions / bytes | Standard code matches |
| --- | ---: | ---: |
| xFX | 9 / 3,372 | 4 / 424 |
| xCamera | 8 / 2,488 | 2 / 460 |
| zNPCHazard | 1 / 700 | 0 / 0 |
| xPad | 1 / 688 | 0 / 0 |
| zEntPlayer | 1 / 660 | 0 / 0 |
| zNPCTypeTiki | 2 / 512 | 1 / 88 |
| xDecal | 1 / 468 | 0 / 0 |
| zEntButton | 3 / 428 | 2 / 264 |

The standard gain is nine functions / 1,236 bytes. All 26 confirmed members / 9,316 bytes remain represented, including units with no exact functions. Canonical linkage names come exclusively from authenticated reference-original DWARF attached to the existing French proofs; xCamera's overloads use the existing explicit address selector. All 30 distinct member/callee identities were independently checked against the originals. Sixteen direct-call operands reach uniquely identified original callees; the other 33 direct transfers remain unmodified and unresolved.

The actual report pipeline revalidates the existing original-only boundary registries before compiling all eight whole source files with `VERSION_SLES_53623`. Its compiler and standard section exporter use the same private pilot profile. Independent objdiff replay reproduces every unit record and confirms that all nine standard matches pair one real, same-size compiled function with its authenticated target identity. These are standard code matches, not claims that all relocation operands or an executable link reconstruct exactly.

The complete section-report denominator remains 597 known functions and 2,979,968 CPU-region bytes; the known-function subset remains 214,848 bytes. Unknown CPU bytes are retained, and every new unit stays partial. Existing debug-region profiles are unchanged because the additive groups are restricted to the authenticated French executable SHA-1.

Private evidence is in `build/france235`: original-only profile generation, eight additive groups in `profiles.json`, actual whole-source compiler evidence and the final standard report under `pilot/SLES-53623`. Independent checks in `build/france235-review` cover all original identities, exhaustive known/unknown transfer accounting, unchanged registry/denominators and direct objdiff replay. No GC or Xbox source changed, and no rebuild is claimed for those unchanged platforms.
