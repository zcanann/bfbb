# PS2 Villager alpha update

The original inlined Bubble Buddy alpha update keeps the raw cycle result
across calls and multiplies it by PI separately in each repeated sine operand.
The previous source cached the multiplied angle, changing the saved floating
register and eliminating repeated constant loads/multiplies. The PS2 branch
now places `PI * angle` inside `isin`, preserving the original evaluation shape.
GameCube retains its existing expression.

`zNPCVillager_SceneTimestep` improves from 69.64286% to exact, adding 280 exact
bytes and one exact function in all four PS2 versions. The full 81-function
Villager unit in each debug region improves from 11,212 to 11,492 exact bytes,
59 to 60 exact functions, and 96.72129% to 97.13003% fuzzy matching. Every other
function record is unchanged. The current seven-member French subset reaches
100%, and its six animation bodies remain exact. All three GameCube regions
retain identical full-unit function records and measures.

Evidence: `build/npc-villager-alpha-region-summary.json`, the three full-unit
reports under `build/npc-villager-alpha-regions`,
`build/npc-villager-alpha-pilot/report.json`, and
`build/npc-villager-alpha-gc.txt`.
