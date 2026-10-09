# France Hangable sequence proof

`platforms/france_hangable_sequence.py` independently verifies the complete
Hangable sequence at `0x136ee0..0x138224`: eleven original-owned functions,
4,868 code bytes and 64 alignment bytes. The existing independently confirmed
252-byte `HangableSetup` at `0x1380d0` anchors the sequence. The generator
returns ten newly verified members / 4,616 bytes and preserves that existing
anchor. Fuzzy candidate output is not an input to this proof.

Each of the three authenticated debug originals must supply the same ordered
DWARF names, canonical linkage identities, sizes, alignments and relative
placements. Every non-address instruction bit must match. Changed operands
are limited to explicit J/JAL destinations and independently checked reaching
LUI address pairs. The complete 4,932-byte masked template must occur uniquely
in France's loaded image. Inter-function gaps must be less than 16 bytes and
entirely zero on both sides.

Nine bodies, including the pre-existing Setup anchor, pass the strict original
and French local control-flow/frame checks. Load and Save are exact eight-byte
J/NOP tails. Each continues through an exact eight-byte `zEntLoad` or
`zEntSave` context wrapper into the independently verified, closed `xEntLoad`
or `xEntSave` body. The wrappers neither modify SP/RA nor become new named
anchors through this proof.

The data checks use original DWARF declarations and layouts:

- `globals` is the original 8,272-byte `zGlobals`, with verified inheritance
  and component offsets for update manager, player model, cylinder radius,
  hanging entity, hanging pivot and current scene. Every observed address
  implies the same French base; the complete typed object must fit BSS.
- The local circle is an eight-element `xVec3` array. Six original 16-byte
  loads copy its complete 96-byte initializer, which is byte-identical in
  France. No literal is inferred from a compiled source object.
- All four complete emitter/hash strings remain byte-identical.
- Both culling callback pointers retain their independently verified original
  identities. They are not treated as generic data-address substitutions.

Internal calls preserve full-member relationships. Already verified external
callees are rechecked against their original identity and body hashes. Five
other complete original entry identities provide explicit 32-byte prefix
contexts; these contexts do not become recovered functions, relocations or
progress. Their address/call exceptions and global-field uses are checked,
and their identities must agree across all three reference versions. Distinct
data addresses and callees cannot collapse to a single French destination.

The scoped address decoder recognizes the observed single-precision COP1
arithmetic/comparison instructions as writes to FP/ACC/FCC state. Register
transfers retain the strict shared GPR decoder. No shared verifier is weakened.

Standalone original-only validation passed for all three references on
2026-10-08 in 29.46 seconds. The private result is
`build/hangable-proof.json` in the regional worker checkout. Integration must
append these results to the existing combined TU registry and regenerate its
canonical checks; the generator deliberately excludes its own new entries
when loading independent context. This proof alone does not claim compiled
source matching or a linked PS2 executable.
