# French sound playback cluster

`tools/platforms/france_sound_playback.py` proves nine complete original bodies,
4,924 bytes across five source units. USA, PAL and German debug originals must
agree on every complete body, canonical linkage, boundary, call and changed
data operand. No rebuilt source object participates in the proof.

| Function | French entry | Bytes |
| --- | --- | ---: |
| iSndFindFreeVoice | 0x1b8cc0 | 900 |
| iSndPlay | 0x1b92a0 | 1120 |
| iSndCalcVol | 0x1b9af0 | 776 |
| iSndLookup | 0x1ba100 | 96 |
| xSndPlayInternal | 0x209d60 | 1376 |
| xVec3NormalizeFast | 0x210c30 | 224 |
| xVec3Normalize | 0x210d10 | 224 |
| _HISBatchSoundCommandsNoWait | 0x34b050 | 124 |
| HISFlushAsyncRequestsNoWait | 0x34bb00 | 84 |

All bodies close under the existing control-flow checker. Seven explicit
external dependencies use complete independently verified identities: the two
sound helpers from the listener cluster, xSTAssetName, and four HIS operations.
The dependency address set is fixed; later registry additions cannot change
the generated context inventory. Every internal call resolves to its complete
named body in this cluster.

The two vector normalizers have identical 224-byte bodies, so neither passes
individual uniqueness. The original xVec3 unit contains exactly those two
members in that order, without a gap. Their complete literal 448-byte sequence
is unique, preserving both named positions. Other bodies use unique complete
instruction templates. The 84-byte flush uses a scoped 28-byte exact seed and
exactly three checked masks (one internal call and two typed addresses); every
word of the full 84-byte candidate is tested. The generic 32-byte seed minimum
is unchanged.

The 36 changed BSS operands resolve to typed original paths in gSnd, the
512-element iSndFileInfo array, the 4,096-byte asynchronous request buffer and
the 40-byte RPC client object. Original declarations, element types, sizes,
array bounds, storage class, consistent bases and nonoverlap are checked.
Two diagnostic-string operands must retain their complete strings, including
the newline and terminator. No data extents are promoted.

Opaque runtime transfers retain their literal original JAL words. Seven call
sites retain identical 64-byte runtime context. The other two target the same
fixed five-word arithmetic leaf at 0x114b50. Its complete 20-byte body is checked
word for word, with the exact branch to its return, no frame or calls, no
unreachable words, four bytes of zero padding and exhaustive uniqueness. The
next SDK routine starts at 0x114b68, so a 64-byte witness would cross into its
unrelated relocated operands. The proof retains the generic checker's explicit
16-byte-alignment diagnostic and separately records this exact 8-byte-aligned
context. It assigns no runtime symbol, function extent or progress credit.
Neither the generic CFG rules nor the generic opaque-prefix rule changes.

The source pilot compares seven new functions, 4,716 bytes. The two HIS bodies
remain coverage-only because their implementation sources are absent.
xSndPlayInternal (1,376), iSndLookup (96) and iSndCalcVol (776) match exactly,
adding 2,248 exact bytes and three exact functions. Initial fuzzy scores are
85.28% for FindFreeVoice, 76.26071% for Play and 79.16071% for each vector
normalizer. Existing French xSnd members remain exact: 13/13 functions,
3,496/3,496 bytes after adding PlayInternal. This profile change only enables
the French executable; the debug profiles are unchanged.

Validation:

```powershell
$env:BFBB_FRANCE_TEST_ORIG='C:/Projects/bfbb-bink/orig'
python -m unittest discover -s tools/tests -p test_france_sound_playback.py -v
```

The fixture registry must include the independent listener cluster. Tests
reject modified runtime instructions, branch/return/delay words and padding,
duplicate runtime/flush/vector sequences, changed literal caller transfers,
changed complete diagnostic strings and altered original file-array bounds.
The private pilot and standalone proof do not replace the canonical aggregate
regeneration and full French source-report integration gate.

The production gate passes in `build/oct09-france-playback-retry`: all aggregate
registries regenerate identically and 72 enabled source units compile. Together
with the independently raw-exact vector source change, this adds 2,696 exact
bytes / five functions, reaching 163,072 bytes / 464 functions. Recovered
coverage reaches 731 functions / 367,744 bytes; the full CPU denominator stays
2,979,968 bytes. Both HIS bodies retain coverage-only status, and source data
and the full executable link remain pending.

Publishing the vector identities initially replaced three earlier Hangable
entry-context records. Hangable now excludes subsequent vector identities from
its dependency set, preserving its original independent evidence. An original-
backed test removes Event and vector records separately and together; the full
Hangable proof and its three contexts per callee remain identical. The complete
French report retains every earlier function and exact code/data measure.
