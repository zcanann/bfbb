# French NPC model helper cluster

`tools/platforms/france_goalrobo_models.py` proves nine complete bodies and
1,616 French code bytes using all three debug originals. Every complete body
has a unique instruction template and passes the unchanged boundary and
control-flow checks. There are no changed data operands.

ModelAtomicFind is a literal 80-byte leaf. The 100-byte ModelAtomicShow and
84-byte ModelAtomicHide each have one direct call, at offset 24, to that leaf.
Six robotic-goal consumers add 1,352 bytes. Every one of the cluster's 19 direct
JAL destinations resolves either to those complete helpers or one of three
fixed independently confirmed identities: common Enter, standard Exit and
VelStop. The internal helper graph is explicitly restricted to this acyclic
shape. Later registry additions cannot expand the dependency inventory.

Full source compilation matches all nine new bodies exactly. The robotic-goal
profile grows from 22 to 28 selected exact functions (5,720 to 7,072 bytes), and
the Common profile from six to nine exact functions (1,324 to 1,588 bytes).
This adds 1,616 exact bytes and nine exact functions. No source, compiler or
non-French profile change is needed. The proof claims a complete caller/callee
cluster, not either whole translation unit.

Validation:

```powershell
$env:BFBB_FRANCE_TEST_ORIG='C:/Projects/bfbb-bink/orig'
$env:BFBB_FRANCE_TEST_REGISTRY='C:/Projects/bfbb/config/platforms/SLES-53623'
python -m unittest discover -s tools/tests -p test_france_goalrobo_models.py -v
```

The registry must include the preceding motion cluster. Four original-backed
tests cover complete three-reference evidence and JSON roundtrip, changed
helper/caller/call instructions, a duplicate complete helper, and missing or
corrupted independent callee evidence. Replay takes about six seconds. Private
artifacts are `build/goalrobo-models-proof.json` and
`build/goalrobo-models-france-pilot/report.json`; canonical aggregate
regeneration and the full French report remain production integration gates.
