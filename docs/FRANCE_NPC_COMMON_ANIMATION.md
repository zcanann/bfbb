# French Common NPC animation builders

`france_npc_common_animation.py` proves ZNPC_AnimTable_LassoGuide (264 bytes)
and ZNPC_AnimTable_Common (128 bytes) as a complete contiguous cluster at
0x2cfd60. The 400-byte span includes their eight-byte zero alignment gap.
All three original DWARF member orders, extents and relative entry addresses
must agree. Each original and French body independently passes the existing
control-flow and frame/return checks.

The complete cluster has exactly one match across all loaded French file
spans, using a 44-byte unchanged anchor at offset 328. No short-seed exception
or whole-translation-unit claim is used. Ten changed address operands and six
JALs are inventoried per reference, with four fixed complete xAnim identities.
The original g_strz_lassanim declaration must be char*[3], and all three full
strings must agree. Its four scalar pointer loads, three literal strings and
three known callback operands are checked separately. No data extent is
promoted, and no source compilation supplies identity evidence.

Seven original-backed tests reject changed bodies, calls, callback code,
alignment, string tables/literals, original function extents or pointer-array
types, missing/altered dependencies, and a duplicate complete cluster. JSON
round-trip identity is required. Private replay and test evidence is
`build/npc-common-animation-proof.json` and
`build/npc-common-animation-tests.log`.

Both source bodies are exact in the private French pilot, adding 392 exact
bytes and two functions. The Common subset grows from 3,040 to 3,432 exact
bytes and 17 to 19 exact functions; fuzzy matching rises from 99.88757% to
99.89926%. All eighteen prior function scores, sizes and metadata remain
unchanged. Only their offsets within the expanded target object move.
All non-French source profiles are unchanged.

The independent Common-pair proof passes canonical production integration
together with the Robot cluster in `build/oct09-france-curve-animation`.
All seven Common tests and eight Robot tests pass; the full 76-unit source
report retains every earlier score. The two Common bodies contribute their
392 exact bytes / two functions without expanding the Robot proof's scope.
Source data and a full original executable link remain pending.
