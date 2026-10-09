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
text's 2,617 bytes stay identical. Private proof: build/near261/{proof.json,
raw-proof.json,gc/proof.json,xbox/proof.json,phased/<version>/report.json,
french-expanded/report.json}. The French find_char1,648-byte body and complete
ordered12-entry table also independently replayed in
build/near259/french-raw-proof.json.


## Float-list loop truth conversion

The original outer FloatList loop materializes the current character's truth
value with SLTU followed by XORI before its exit branch. Using the character
directly in the ordinary C++ loop condition recovers this conversion boundary;
the explicit comparison with zero had collapsed it to BEQZ. Both conditions
are equivalent for every character value. No parser stores, delimiters, limits,
helper calls, signedness or other source expressions change.

All four complete-source comparisons improve FloatList from 96.45038 to 98.43511
percent. The source grows from 524 to 532 bytes because the independently remaining
whitespace-loop NOPs are still present; the original is 524 bytes. The null-input
guard also still tests a different equivalent register. These differences remain
visible, and no new exact function or byte credit is claimed. String totals stay
4 matched functions / 2,192 bytes, with all other function records unchanged.

Every other allocated section and every relocation record stays identical;
all four source objects are byte-identical. The actual GameCube object's two allocated
sections and the actual Xbox whole-TU linked text's 2,617 bytes are unchanged.
The French comparison uses the independently reviewed 12-member target from
string254. Private evidence: build/near263/{alignment.json,proof.json,
gc/proof.json,xbox/proof.json,truth/<version>/report.json,
french-expanded/report.json}. No additional source variants were retained.

## Hash character lifetimes (2026-10-09)

The PS2 hash loops now test the current byte before creating the original
`char` local inside the loop body. Both hash overloads and HashCat update the
accumulator before advancing the input pointer. This preserves the original
byte narrowing and register lifetimes. Merely changing the old outer U32 local
to char did not match: the character's scope and update order both matter.
The bounded overload retains its unsigned limit check before reading a byte.

All three complete original bodies now match: the unbounded hash (88 bytes),
bounded hash (104), and prefixed hash (88). USA, Europe, Germany and France each
gain 280 exact bytes and three functions. Their complete twelve-function string
units rise from 5/12 functions and 2296/3948 exact bytes to 8/12 and 2576/3948;
fuzzy matching improves from 92.64033% to 95.42148%. Every other function score
is unchanged. Direct source-object/target-object body comparisons also confirm
all twelve regional hash bodies are byte-identical, with no relocations needed.

The new loop forms are PS2-only. The full GameCube USA build retains its entire
progress report and passes the retail DOL SHA-1 check. No shared header, profile,
registry, target boundary or compiler setting changes. Private evidence is
`build/string-oct09`, including all four before/after reports, rejected loop
forms and `raw-hash-proof.json`.

## In-place tokenizer (2026-10-09)

The PS2 tokenizer now selects the restart pointer with the original if/else
shape, tests the current input byte before creating a plain-char local, and
returns the result directly from the empty-token comparison. Its bitmap index
uses the unsigned byte shifted by three without the redundant five-bit mask.
The selected PS2 compiler's unsigned-char mode keeps that index within 0..31.
Token splitting, restart-pointer updates and empty-token behavior are unchanged.

The complete 320-byte `xStrTok` body becomes exact in all four PS2 releases,
raising each string unit to 9/12 functions and 2896/3948 exact bytes. Fuzzy
matching rises from 95.42148% to 97.32827%, with every other function score
unchanged. Raw source/target comparisons confirm all four bodies byte-for-byte.

These forms are confined to PS2. GameCube's full USA build report remains
identical and its retail DOL SHA-1 check passes. The buffer tokenizer remains
unchanged: multiple restart, character, destination and counter-lifetime probes
improved its fuzzy score but did not reproduce its whole body. Private evidence
uses `*-token.json` and `raw-token-proof.json` under `build/string-oct09`.

## Substring comparison join (2026-10-09)

PS2 now shares the existing Xbox conditional expression for the length
tiebreak in `icompare`. This recovers the original join branch and makes its
complete 192-byte body exact in all four PS2 releases. Each string unit rises
to 10/12 functions and 3088/3948 exact bytes, with fuzzy matching improving
from 97.32827% to 97.535965%. Every other function score is unchanged.

Direct source/target comparisons confirm all four complete bodies. The full
GameCube USA report remains identical and its retail DOL SHA-1 check passes.
Private evidence uses `*-compare.json` and `raw-compare-proof.json` under
`build/string-oct09`. No target, registry, header or compiler setting changes.

## Buffer tokenizer recovery (2026-10-09)

The PS2 buffer tokenizer now uses the same explicit restart branch as the
in-place tokenizer, indexes its bitmap directly with the unsigned input byte
shifted by three, and reads that byte directly for its token test and copy.
These equivalent source forms improve its complete 336-byte comparison from
73.4881% to 91.96429% in all four PS2 releases. Remaining register and scheduling
differences prevent an exact match. No exact function or byte gain is claimed.

The string-unit fuzzy score rises from 97.535965% to 99.108406%, retaining
10/12 exact functions and 3088/3948 exact bytes. Every other function score is
unchanged. The PS2-only branches preserve the complete GameCube USA report,
and its retail DOL SHA-1 check passes. Private evidence uses `*-buffer.json`
under `build/string-oct09`.
