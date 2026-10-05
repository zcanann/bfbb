# PS2 gameplay object source comparisons

Four complete source units now compare every original-owned function in all three
PS2 debug versions. The normal reports retain every partial function:

| Unit | Functions | Code bytes | Matched functions / bytes | Fuzzy |
| --- | ---: | ---: | ---: | ---: |
| zEntTrigger | 7 | 2,212 | 3 / 24 | 68.19168% |
| zEntButton | 13 | 3,664 | 6 / 640 | 77.909386% |
| zEntDestructObj | 15 | 4,232 | 9 / 684 | 95.511345% |
| zEntSimpleObj | 14 | 4,860 | 7 / 252 | 78.42881% |

Together these add 25 matched functions / 1,600 bytes per debug version, with
49 original functions / 14,968 bytes compared. No full-TU completion is claimed.

The compile fixes restore the existing SDK matrix-copy assignment macro, two
original p2 model API declarations, and the actual allocator header dependency.
Original linkage names and return types independently verify iModelNumBones and
iModelSphereCull in all three executables. No function implementation is supplied.

Simple-object frame allocation also used a hardcoded GameCube size (228 bytes).
Every PS2 original loads 240 into the size argument before xMemAlloc, agreeing
with its full xEntFrame layout. Using sizeof(xEntFrame) restores that platform
behavior while preserving the GameCube build, full score report and retail hash.

Compiler debug objects agree with original complete sizes and direct member
offsets for 11 trigger, 12 button, 13 destructible and 18 simple-object consumer
types, checked in every debug region. The two destructible asset names hitModelId
and destroyModelId correspond to original hitModel and destroyModel: each is a
four-byte unsigned integer at offsets 48 and 52. These explicit naming aliases
are recorded; they do not change layout. Debug and production objects have
identical ordered allocated sections.

Independent application of actual source relocations reconstructs 24 functions /
1,472 original bytes in each region. The remaining stock objdiff code match,
FindFX (128 bytes), has six anonymous string-address pairs and is not claimed
raw-exact. One unknown call at offset 336 in the partial simple-object Init stays
raw. Normal score settings, full denominators and completion policy are unchanged.

Local proof artifacts are in build/gameplay192/<unit>: regional reports,
compiled layouts and raw-proof.json. Additional records include api-proof.json,
frame-allocation-original.json and the explicit destructible field-name aliases.
