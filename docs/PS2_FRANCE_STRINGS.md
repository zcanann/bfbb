# French string boundaries and source comparison

The complete original `SB/Core/x/xString.cpp` sequence has twelve functions / 3,948 code bytes in a 3,992-byte span. Eight previously confirmed French members remain unchanged. Four recovered functions add 2,476 known CPU bytes: `find_char` (1,648), `atox` (200), `imemcmp` (104), and `xStrParseFloatList` (524).

`tools/platforms/france_string_sequence.py` verifies the full sequence against all three authenticated debug-bearing originals, including original canonical linkage, complete function bytes, zero alignment gaps, unique placement, and the existing strict CFG/frame checks.

`find_char` has one twelve-entry jump table. The established dispatch recognizer is reused with explicit offset 64 / count 12; its opcode, register-dataflow, unsigned range guard, incoming-edge, delay-slot, and complete-table checks are unchanged. All twelve original and French relative destinations must agree. No generic indirect-jump exception is added, and existing motion callers retain their previous default specifications.

The sole external string call, from `xStrParseFloatList`, is corroborated by the complete six-member original math tail: `xurand`, `xrand`, `xsrand`, `xatof`, `xMathExit`, and `xMathInit`. Three independent established neighbors and unique whole-sequence placement establish context. The eight-byte `xatof` context is the identical literal J/NOP tail in all four originals; its transfer instruction stays **unmasked**, and an identical 64-byte runtime prefix adds context only. No new runtime or xatof anchor, source ownership, or progress extent is promoted. All other context bodies pass strict CFG checks.

With the independently verified counter-scope and staged hexadecimal-conversion source changes, the complete actual French string TU reports four exact functions / 2,192 bytes. Newly covered `find_char` and `atox` contribute 1,848 bytes / two functions; the eight previous function scores remain unchanged. Independent raw reconstruction verifies all 2,192 exact bytes, both table-address HI/LO fields, and all twelve actual R_MIPS_32 table relocations to the same source function.

The complete existing string TU remains the source input, including all holdouts. Original boundaries and tables are recovered without consulting compiler output. The source counter-scope correction is documented separately in `PS2_STRING_SCANS.md`; original/platform behavior and validation of subsequent source changes belong to their source handoffs. The full French gate adds exactly 1,848 matched bytes / two functions (40,056 / 225 on the tested baseline), with 622 known boundaries. Every unrelated function/unit record remains unchanged; old string records only shift their synthetic target-section offsets as the missing bodies are restored. CPU/data denominators remain unchanged as known functions replace previously unclassified CPU bytes.

Reproduce original metadata verification with:

```sh
python tools/platforms/france_tu_sequences.py --orig-dir orig --check
```

Private proof and comparison artifacts are under `build/string254`: `original-proof.json`, `profile.json`, actual whole-source `command.json` / `xString.o`, `report.json`, `raw-proof.json`, and `full-report-proof.json`. The final French report uses a fresh complete string object and authenticated unchanged objects for the other sixty existing source TUs.
