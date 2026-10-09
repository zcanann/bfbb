# French NPC facing cluster

`tools/platforms/france_goalrobo_facing.py` verifies five complete functions
covering 1,496 French bytes: NPCC_dir_toXZAng, TurnToFace, FacePos, and the
Chase and GoHome goal Process methods. All three debug originals establish
each named body and extent. Local boundaries, returns, frame handling and
zero padding pass the unchanged CFG checker.

There are no changed data operands. Sixteen explicit JAL sites follow a fixed
caller/callee inventory and six fixed external identities. The small angle
helper has a literal call at offset 12 to an unchanged 64-byte opaque runtime
context; this context supplies no runtime name or promoted extent. Its other
call, at offset 20, is independently established as xAngleClampFast.

Only this complete 40-byte helper uses a scoped uniqueness check with a
20-byte literal seed. Every aligned seed hit across every loaded file-backed
span receives a complete 40-byte comparison. Exactly the proven JAL destination
at offset 20 is masked; the opaque call and all remaining words stay literal.
The complete match must occur once, at the reviewed entry. Generic minimum
seed lengths and all other body checks are unchanged.

Full source compilation matches NPCC_dir_toXZAng (40 bytes), FacePos (220)
and GoHome::Process (420) exactly: +680 exact bytes and three exact functions.
TurnToFace is 98.882355% over 340 bytes; Chase::Process is 98.31933% over 476.
The profile changes only French source selection and retains prior evidence.
This is a complete caller/callee cluster, not a whole translation unit claim.

Validation:

```powershell
$env:BFBB_FRANCE_TEST_ORIG='C:/Projects/bfbb-bink/orig'
$env:BFBB_FRANCE_TEST_REGISTRY='C:/Projects/bfbb/config/platforms/SLES-53623'
python -m unittest discover -s tools/tests -p test_france_goalrobo_facing.py -v
```

The registry requires the preceding motion and vector proofs. Six original-backed
tests cover all references and JSON roundtrip, altered caller/helper/call words,
both small-helper calls, changed padding and opaque context, duplicate complete
helper bodies, missing or corrupted callee evidence, and invalid shorter-seed
scope/mask inventories. Replay takes about six seconds. Private evidence is
`build/goalrobo-facing-proof.json` and
`build/goalrobo-facing-france-pilot/report.json`. Canonical aggregate regeneration
and the full French source report remain required production gates.
