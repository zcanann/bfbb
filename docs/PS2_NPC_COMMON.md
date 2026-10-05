# PS2 NPC common source comparison

The complete `zNPCTypeCommon.cpp` now compares all 86 original functions /
26,272 bytes. USA matches 37 functions / 5,212 bytes (80.66413% fuzzy), while
Europe and Germany match 36 / 4,904 (80.663216%). All partials remain included.

The only regional score difference is the 308-byte `Vibrate(en_npcvibe,float)`
function. Its three build-rumble cases contain original USA float bits
0x3d4cccce versus PAL 0x3d75c28f. The shared source computes three times
NPC_FRAME_TIME, currently the USA value. PAL therefore retains a 99.92208%
partial; it is not assigned the USA exact score. Regional code/data denominators
and completion remain unchanged.

## Minimal platform declarations

The source's indirect camera header selects ordinary PS2 math.h instead of the
GC-specific cmath path. The PS2 iModel header declares the actual
`unsigned int iModelTagSetup(xModelTag*,RpAtomic*,float,float,float)` API.
All three original canonical linkages confirm parameters; original function
DIEs confirm unsigned-integer return type. No substitute implementation,
function-body change, compiler patch or flag change is introduced.

## Layout and raw evidence

Actual normal/debug whole-source compilations have identical ordered allocated
sections. All 23 checked original aggregate layouts agree with the compiler in
all three regions. For xNPCBasic, the source places six bitfields into two
four-byte nested structs, while the original declares them directly. The private
proof expands only those two wrappers and compares names, byte displacement,
bit offset, width and signed/unsigned type. Every field agrees, and full size
remains 444 bytes; no source layout change is needed.

The TU was also rebuilt after the complete camera/render-state header additions;
its ordered allocated sections are identical to the preceding measured object.
Original function boundaries, linkages and named relocation inverses are checked
independently per region. Sixteen initial unknown references remain unassigned.

Independent actual-source relocation application reconstructs 32 functions /
3,836 bytes exactly in all three originals. Other normal matches retain unresolved
static data or callees and are not claimed raw linked equality. No complete-TU
or executable-link claim is made. The normal GC build passes its retail SHA1
check and its entire objdiff report remains unchanged.

Evidence: `build/npc204/` contains compile inventory, API/layout work, normal/debug
objects, all-region reports, explicit per-region bitfield records, raw proof and
GC verification. Original API DIE evidence is in the renderer worktree's
`build/ps2render198/tagsetup-original-{api,dies}.json`.
