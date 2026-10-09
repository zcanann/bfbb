# French sound listeners and voice helpers

`tools/platforms/france_sound_listeners.py` proves nine complete French sound
functions (1,988 bytes), using the complete corresponding bodies and DWARF1
identities from USA, PAL and German originals. This is a caller/callee cluster,
not a whole translation unit claim. It consumes no registry identities, so later
recoveries cannot change its context inventory or create a proof dependency cycle.

| Function | French entry | Bytes |
| --- | --- | ---: |
| xSndStreamUnlock | 0x209550 | 72 |
| xSndStreamReady | 0x2095a0 | 88 |
| xSndParentDied | 0x209c80 | 64 |
| xSndIDIsPlaying | 0x209cd0 | 88 |
| xSndSetListenerData | 0x20a380 | 232 |
| xSndInternalUpdateVoicePos | 0x20a530 | 284 |
| xSndProcessSoundPos | 0x20a650 | 616 |
| xSndCalculateListenerPosition | 0x20a8c0 | 416 |
| xSndAddDelayed | 0x20aa60 | 128 |

Each body has independently checked local control flow and a unique complete
instruction template across every original loaded span. All bits remain equal
except the one explicitly resolved internal JAL and 58 typed data address
operands. The JAL at InternalUpdateVoicePos+260 targets the complete, separately
unique ProcessSoundPos body. There are no opaque runtime calls or external
callback assumptions.

The data evidence checks original declarations, component sizes, field types,
the listener mode enumeration, xMat4x3's xMat3x3 base, the 48-element voice array,
two listener matrices, and the 16-element delayed-sound array. Float coordinates
and vector endpoints map through explicit member paths. A single consistent
French base is required per object, with original inter-object separation and
runtime BSS ownership. The data objects themselves are not promoted as extents.

The private source pilot matches all nine bodies exactly: 1,988/1,988 bytes and
9/9 functions, 100% fuzzy. Extending the existing French xSnd profile yields
12 selected functions and 2,120 selected bytes. The separately validated
ProcessSoundPos source change also makes the complete xSnd unit exact in all
three debug PS2 versions; see `PS2_SOUND.md`. GameCube reports are unchanged.

Validation:

```powershell
$env:BFBB_FRANCE_TEST_ORIG='C:/Projects/bfbb-bink/orig'
python -m unittest discover -s tools/tests -p test_france_sound_listeners.py -v
```

The tests replay all originals and reject an altered instruction, typed operand,
internal call target, duplicate complete body, and changed original voice-array
bound. Production integration must regenerate the canonical aggregate registry
and run the full French source report; the private pilot is not that final gate.

Production integration passes that gate in `build/oct09-france-listeners`:
all aggregate registries regenerate identically, all 70 enabled source units
compile, and the section report reaches 160,376 exact bytes / 459 exact
functions. This adds 1,988 bytes / nine functions with no earlier score loss.
Recovered coverage is 722 functions / 362,820 bytes; the full CPU denominator
remains 2,979,968 bytes. Source data and a full executable link remain pending.
The listener proof also survives an exact JSON roundtrip, including typed-path
keys, so serialized evidence equals fresh regeneration.
