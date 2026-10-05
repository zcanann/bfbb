# PS2 particle-manager API

The complete shared `xParMgr.cpp` now compiles with a minimal PS2 `iParMgr.h`.
All three declarations come from authenticated original DWARF in SLUS-20680,
SLES-51968 and SLES-51970: void iParMgrInit(), void iParMgrUpdate(float), and
void iParMgrRender(). Their canonical names and void return attributes agree
across the three executables. No renderer structure or SDK implementation is
introduced.

The existing empty xParMgrKillAllParticles helper is inline on PS2. It has no
separate original function, and original xParMgrInit's already-initialized path
calls iParMgrInit directly. The helper has only this one source caller. Its body
and the existing GameCube path remain unchanged.

The comparison covers all three original functions / 192 bytes. xParMgrInit
(68 bytes) and xParMgrRender (8 bytes) match in every debug version. Independently
applying the source J/JAL and named GP relocations reproduces all 76 bytes in each
original. xParMgrUpdate remains unmatched: the production compiler unrolls its
countdown loop, while the original keeps a compact delay loop. A private !=0
condition control removed that loop altogether and was rejected; the original
shared source condition is retained. No flags, volatile qualifiers, or assembly
were introduced to fit the delay loop.

The actual GameCube xParMgr object has identical allocated sections. The new
header is on the PS2 include path only. This is partial function-code progress,
not a full translation-unit or source-linked executable completion claim.
France remains excluded until its own function and relocation identities are
reviewed.

Private evidence in `build/ps2parmgr166` includes `original.json` (authenticated
API attributes and original bodies), the real complete-TU compiler command and
object, `profile.json`, all three normal reports, `raw-proof.json`, and the GC
object comparison. No production reporting/backend extension is needed.
