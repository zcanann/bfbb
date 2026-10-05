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
