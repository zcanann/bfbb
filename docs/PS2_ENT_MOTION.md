# PS2 move-point motion inlining

The 2452-byte `xEntMPMove` improves from 68.368675% to 99.67374% in
all four PS2 profiles, including France. The unit retains its existing
`-inline deferred` profile. A PS2-only `inline_depth(4)` setting surrounds
this function and is reset immediately afterward, allowing its banking
expression to expand through the nested vector helpers as in retail.
No profile or compiler binary changes are involved.

The baseline emits two calls to `xVec3::operator*=(float)` while computing
`bank * speed * 0.01f + gravity`. The original embeds both operations,
including the intermediate vector copies and component loads/stores.
Depths two and three do not reproduce that sequence; depth four does,
and larger depths produce no additional improvement. The speed operand
is read through `motion->mp.speed`, matching the original base register.
The remaining difference is two extra NOPs in aggregate initialization
loops and the resulting shifted branch destinations. No compiler-version
explanation or patch is claimed.

DWARF identifies the complete original at US `0x1d8fb0`, Europe
`0x1d94a0`, and Germany `0x1d8780`. The French comparison uses the
existing authenticated function profile. These source changes do not
promote any new function or runtime identity.

Normal full-unit `ps2solo.py` builds checked all 20 profiled functions
in each of the four regions: only `xEntMPMove` improves. The complete
GameCube report is identical and the rebuilt DOL passes its SHA-1 check.
Private results are in `build/motion-oct09/after-proof.json`.

`xEntMotionInit` also reaches 100% from 89.39247%, adding 744 exact
bytes and one function in all four PS2 regions. Its mechanical-motion
branch now uses the original DWARF local `mkasst` and accesses `sld_tm`
through that member type, instead of the same-offset `mp.speed` union
member. Marking the existing `xEntMotionDebugAdd` helper inline restores
the debug-registration sequence embedded at the end of retail Init.
The US original starts at `0x1da4f0`. All other profiled functions and
the complete GameCube report remain unchanged, and the GameCube DOL
again passes its SHA-1 check. Private full-unit comparisons are in
`build/motion-oct09/init-proof.json`.
