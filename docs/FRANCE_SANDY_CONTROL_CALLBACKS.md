# French Sandy player-control callbacks

`france_sandy_control_callbacks.py` proves four complete original callbacks:
getUpCB (428 bytes), elbowDropCB (384), tauntCB (332) and idleCB (232), totaling
1,376 bytes. All three reference originals must have the same source owner,
canonical linkage and complete DWARF extent. Every body is independently
unique across the loaded French file spans; entry alignment, zero padding
and closed control flow are checked. No whole-unit claim is made.

The eight changed data operands load only `globals.player.ent.model` and
`globals.player.ControlOff`. The original complete globals/player/entity
layouts establish both paths, including ControlOff's unsigned type and
component offsets 1792 + 4344. An independently proved complete CalcNewDir
body anchors the full globals object. Three direct calls reach the complete
104-byte AnimTimeRemain body. Only these two fixed independent identities
are read from registries, so unrelated additions cannot affect the proof.
No data extent is promoted and no generic matching rule changes.

Seven original-backed tests passed in 155.4 seconds. They reject changed
instructions, calls, data operands, padding, duplicated complete templates,
wrong original owners/extents, changed dependency identity or complete callee
bytes, and altered original ControlOff offset/type. JSON round trips and
unrelated registry additions preserve the exact proof.

All four French source functions are exact: +1,376 matching bytes and four
functions. All seventeen previous Sandy records remain unchanged. The
selected unit now has 21 functions and 14,296 bytes, including 12,444 exact
bytes and twenty exact functions, at 99.94404% fuzzy matching. Evidence:
`build/sandy-control-callbacks-proof.json`,
`build/sandy-control-callbacks-tests.txt` and
`build/sandy-control-callbacks-france-pilot/report.json`.
Full canonical regeneration and the production report remain integration gates.
