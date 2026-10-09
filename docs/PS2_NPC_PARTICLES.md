# PS2 NPC particle update recovery

The October 9 source pass restores the PS2 culling path in all eleven
`NPAR_Upd_*` functions in `zNPCSupplement.cpp`. The previous source used
`RwCameraFrustumTestSphere`, while the three debug originals use four VU side
planes followed by a scalar near-plane test. All changes are PS2-only.

## Original evidence

USA DWARF places `globals` at `0x52c8f0` and `xCamera::frustplane` at offset
`0x270`. The original updates load its first four vectors into `vf14` through
`vf17`. `NPARData` begins with position followed immediately by `xy_size[0]`,
so a quadword load supplies position.xyz and radius.w. The side test computes
four signed plane distances, compares their integer sign representations and
packs the results before branching. Its final `vmul.w` occupies the branch
delay slot. The subsequent scalar expression is:

```cpp
plane[4].x * pos.x + plane[5].x * pos.y + plane[6].x * pos.z -
    plane[7].x + 0.5f * radius < -0.1f
```

The five VU arithmetic words are identical in all eleven functions and all
three debug regions:

```
4be8883f 4be870fc 4be878fd 4be8804e 4a2a41ea
```

These are full raw encodings, including the four `.xyzw` destination masks
and the final `.w` mask. Objdiff's formatted instruction text omits these
masks, so text-only similarity is insufficient. Raw verification also checks
all five vector loads, allowing only the CPU address-register field to vary,
and checks that `vmul.w` remains in a `bnez` delay slot. The reconstructed
helper lets the compiler allocate its scalar output; retail uses `a0`, while
the current compiler selects `v0`. No hard register binding was added.

Additional source corrections follow the original instructions and DWARF:

- OilBubble declares `rat_rev` before its vector updates.
- The death test materializes the byte boolean into `thisChickIsToast` before
  the tail promotion. A direct floating-point branch omits retail's conversion.
- GloveDust and MonsoonRain declare their loop index before `npparm`.
- GloveDust, MonsoonRain and TubeSpiral materialize the smoothing argument as
  the original `subrat` local. Passing the division expression directly emits
  separate multiplies and subtraction; the named local restores retail's
  fused accumulator operations. A replacement smoothing helper alone did not.
- TubeSpiral uses a mutable `useFixedTimestepForSpiral` flag, initialized to
  `1.0f`, to select the regional fixed timestep or the incoming `dt`. USA
  DWARF places that float at `0x506ec0`, whose original bytes contain `1.0f`.
  The original function loads and tests it before changing `dt`. The previous
  source unconditionally substituted a constant timestep.

## Results

USA `SLUS-20680`, Europe `SLES-51968` and Germany `SLES-51970` produce the
same results. Unit fuzzy matching rises from **80.60068% to 99.24505%**.
All eleven updates improve; the other 54 functions retain their previous
scores. Exact coverage remains **49/65 functions and 14,516/41,380 bytes**.

| Update | Original bytes | Before | After |
| --- | ---: | ---: | ---: |
| OilBubble | 1632 | 29.721% | 99.926% |
| DogBreath | 2176 | 61.877% | 99.945% |
| MonsoonRain, GloveDust | 1444 each | 65.435% | 99.917% |
| TubeSpiral | 1880 | 66.121% | 98.202% |
| ChuckSplash | 1984 | 68.179% | 99.940% |
| VisSplash | 1976 | 68.377% | 99.939% |
| Fireworks, TarTarGunk, SleepyZeez, TubeConfetti | 2524 each | 71.345% | 99.952% |

Ten updates retain only the four scalar output-register uses in their culling
block. TubeSpiral additionally differs in static-data addressing and the
special-day branch/load sequence. No compiler defect is claimed.

Private evidence under `build/particles-oct09/` includes three before/after
report pairs, retained-project paths, `prove.py`, and `proof.json` with all
33 original/source function hashes and raw-kernel checks. Each version was
compiled from both the previous source and the retained candidate. The full
GC USA report remains JSON-identical to `build/string-oct09/GC-before-report.json`;
`ninja -j 8` passes the retail DOL checksum
`306526d90b48e99894c3138f5fc8f2716d9fecf6`.

The current source profile covers these three PS2 debug executables only.
This pass adds no French recovery, registry entries or compiler changes.

## Reciprocal numerator follow-up

The four atlas initializers now initialize `du` and `dv` to `1.0f` before
applying their divisions. This keeps the two numerator values available in
retail's order. The shared source expressions and non-PS2 paths remain as
before. Fahrwerkz, TarTarGunk and SleepyZeez rise from 91.37838% to 92.18919%;
TubeConfetti rises from 94.475525% to 94.72028%. All three debug-region unit
scores rise from 99.24505% to 99.29531%, with unchanged exact coverage and no
other function-score changes. The full GC report and retail checksum also
remain unchanged. Evidence: `build/particles-oct09/*-config.json` and
`config-proof.json`.

Removing the synthetic UV index local and moving `justTheRand` after the
reciprocals did not improve these routines; those probes were discarded.
Remaining differences include UV-count reuse and scheduling. No compiler
cause is asserted.

## Exact atlas count reuse

The four ConfigPar atlas builders now keep `num_uvcell[0]` in an unsigned
local across the particle UV stores and use separate reciprocal expressions.
The original instructions retain this count for the second UV expression;
reloading through the parameter object changed register lifetimes and
scheduling. Explicit count reuse restores all four complete bodies without
assembly, volatile accesses, compiler changes or a generic optimization fence.
The non-PS2 preprocessed expressions remain the same.

Fahrwerkz, TarTarGunk and SleepyZeez (740 bytes each) improve from 92.189186%
to 100%; TubeConfetti (1,144 bytes) improves from 94.72028% to 100%. Each of
USA, PAL and German gains 3,364 exact bytes and four exact functions. The full
65-function Supplement unit rises from 14,516 to 17,880 exact bytes, 49 to 53
exact functions and 99.29531% to 99.86032% fuzzy; every other function record
is unchanged. All three GC full-unit reports retain identical records and
measures. The newly proved French Confetti body is also exact, adding another
1,144 exact bytes and one function to the particle-kernel profile; its full
11-function, two-unit pilot is now 100% across 4,332 bytes. The other three
French duplicate builders still require independent surrounding identity proof.

Private evidence: `build/npc-uv-cache-region-summary.json`, before/after reports
in `build/npc-uv-cache-regions/`, `build/npc-uv-cache-gc.txt`, and
`build/npc-particle-kernels-france-pilot/{before-source,report}.json`. A separate
probe rearranging the synthetic arithmetic locals into the DWARF declaration
order did not improve the score and was discarded.
