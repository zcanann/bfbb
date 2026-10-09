# Additional French NPC animation builders

`france_npc_remaining_animation.py` proves five independent complete bodies:
Tiki (124 bytes at 0x2ea600), Test (368 at 0x324910), Sandy (1,852 at
0x336f70), SB1 (1,028 at 0x35c440), and Patrick (1,744 at 0x3786c0).
Their total is 5,116 code bytes across five source units. Each body's exact
DWARF owner, linkage and extent is required in all three original references.
Each has a unique complete masked-template match, with an unchanged anchor
of 44?100 bytes, plus independent control-flow/frame/return and padding checks.
There is no whole-TU claim or change to generic uniqueness thresholds.

The original-only inventory contains 128 changed data operands and 69 JALs
per reference. Exactly five independently established xAnim/Common identities
are permitted as callbacks/callees. Typed char-pointer arrays are checked from
each owner's original declaration, including every complete string and alias:
`g_strz_tikianim[2]`, `g_strz_testanim[11]`, and `g_strz_bossanim[78]`.
The Tiki table's GP-relative code words remain unchanged and unmasked.
No data extent is promoted.

SB1 and Patrick copy local arrays of 11 and 23 signed integers. Their original
DWARF stack locations, complete payloads and exact load/store coverage pass
the existing straight-line checker. Sandy's 25-integer initializer uses a
separate closed loop proof: the complete prefix requires the literal count
three, two 16-byte copies per iteration, exact decrement and BGTZ target,
32-byte source/destination strides including the branch delay slot, and a
final four-byte copy in the first known call's delay slot. It copies exactly
100 bytes into the original stack object at offset 64. This check applies
only to Sandy's authenticated 1,852-byte builder; the generic checker still
rejects copy-prefix branches. All three initializers total 236 bytes.

Ten original-backed tests pass (129.1 seconds), including changed loop count, branch, decrement, strides,
delay and terminal store, mismatched owner/type/count/stack location, missing
copy coverage, malformed bodies/calls/callbacks/padding, changed tables and
payloads, altered original extents/dependencies, and duplicate complete bodies.
JSON round-trip equality is required. Private evidence is
`build/npc-remaining-animation-proof.json` and
`build/npc-remaining-animation-tests.txt`.
