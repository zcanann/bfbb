# PS2 whole-scene source comparison

The unchanged bodies in `xScene.cpp` now compile on PS2 with direct includes for
its actual JSP and collision SDK dependencies. The GameCube include path remains
unchanged. No header definitions, compiler flags, or function bodies change.

All 24 original functions / 12,384 bytes are compared. SLUS-20680, SLES-51968,
and SLES-51970 each report 70.47351% fuzzy match and 16 code-matched functions /
2,092 bytes. The remaining functions stay partial; no entire-TU or executable-link
completion is claimed. France needs independent original function mapping.

Whole-source debug compilations reproduce the original complete sizes and every
direct member offset for `xScene`, `xEnt`, `xNearFloorPoly`, `xJSPHeader`, and
`xClumpCollBSPTriangle` in all three originals. Normal and debug builds have
identical ordered allocated sections. The actual GameCube `xScene.cpp` object is
byte-identical in every allocated section; all other source inputs are unchanged.

Independent application of actual source relocations proves 15 functions /
1,792 bytes exactly in each original. The normal code-matched `gridNearestFloorCB`
(300 bytes) still has unnamed SDK calls, so it is not claimed as a raw link match.
The standard progress metric is retained without altering its comparison settings.

Private reproduction evidence is under `build/ps2core180/xScene`: candidate
`profile.json`, all three actual reports and authenticated target objects,
`all-region-summary.json`, original/compiled layouts, and `raw-proof.json`.
The surrounding `scene/compile-inventory.json` records the actual compiler command;
`gc/comparison.json` records the GameCube section comparison.
