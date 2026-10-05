# PS2 character-set scan scopes

The authenticated debug releases describe twelve distinct signed32 `i` locals
inside `find_char(const substr&, const substr&)`: one for each specialized
character-set scan and one for the default scan. The source previously shared
one outer signed32 counter across all cases. Each scan now owns its counter
inside a block. Character handling, loop conditions, matches, pointer updates,
switch cases, returns and default behavior are unchanged.

This restores the original counter/character register lifetimes without new
temporaries, qualifiers, compiler settings or artificial execution paths. Only
the 1,648-byte function's allocated code changes. Every other allocated section,
including its 48-byte jump table, remains unchanged. All twelve original string
functions remain compared; the unit now matches three functions /1,992 of3,948
bytes in each debug region, a gain of1,648 bytes and one function.

Raw reconstruction independently proves all1,648 function bytes against each
authenticated debug original. The function has exactly two compiler relocations,
HI16 at+44 and LO16 at+52, both referring to its local switch table. The proof
checks their instruction fields and signed-low reconstruction, then checks all
twelve actual R_MIPS32 table relocations against the original ordered destination
list. Every destination lies inside the function. This is a raw function/table
proof, without whole-executable linking or completed-unit credit.

All four complete PS2 source objects are byte-identical. The existing French
8-function/1,472-byte subset is unchanged; new French function recovery and its
expanded comparison are a separate original-only metadata task. This source
commit adds no names, extents, profiles, denominators or scoring rules.

Actual GameCube compilation preserves both ordered allocated sections. Actual
pinned Xbox whole-TU compilation and LTCG linking preserve all2,617 `.text` bytes,
including all source calls and the existing switch helper. Platform-specific
character signedness and every previous exact match are retained.

Private reproducible evidence: `build/near259/{audit.py,alignment.json,proof.json,
raw-proof.json,gc/proof.json,xbox/proof.json,scoped/<version>/report.json}`.


## Hexadecimal digit conversion

Original atox stores an unsigned32 converted digit. For alphabetic characters,
its 200-byte body subtracts the corresponding letter base and then adds10 in
a separate instruction. Splitting the existing source assignment into those
two ordinary statements recovers the original sequence. Bounds, character
signedness, accumulation and read_size updates are unchanged. No new variable,
cast, artificial branch or platform macro is introduced.

Exactly four instruction words change, at offsets112/120/144/148. Every other
allocated section and function record is identical to the preceding scoped-scan
source. The actual whole-unit report gains200 bytes/one function in each debug
region. Both atox and find_char now match in the independently expanded French
12-member comparison as well: all four regions report4 matched functions and
2,192 of3,948 code bytes. The French metadata recovery is a separate commit;
this source change does not supply or alter original identities or boundaries.

All200 atox source bytes equal each of the four authenticated originals directly,
with zero source relocations. All four complete source objects are identical.
GameCube's two ordered allocated sections and the actual Xbox whole-TU linked
text's2,617 bytes stay identical. Private proof: build/near261/{proof.json,
raw-proof.json,gc/proof.json,xbox/proof.json,phased/<version>/report.json,
french-expanded/report.json}. The French find_char1,648-byte body and complete
ordered12-entry table also independently replayed in
build/near259/french-raw-proof.json.
