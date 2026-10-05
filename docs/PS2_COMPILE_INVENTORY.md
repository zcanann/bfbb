# PS2 core compile inventory and first matches

`tools/platforms/ps2_compile_inventory.py` compiles complete existing source files
selected from authenticated original DWARF ownership. It verifies the compiler and
wibo hashes, uses the production flags, bounds each compiler process, records full
logs and emitted ELF function names/sizes, and groups missing headers in JSON.
Compilation success does not update symbols, profiles, matching counts, or progress.
Failed or timed-out compiles cannot reuse an old object.

Run under Linux/WSL, using the existing compiler package and originals:

```sh
python3 tools/platforms/ps2_compile_inventory.py \
  --version SLUS-20680 --orig-dir orig \
  --compilers build/compilers --wibo build/wibo-i686 \
  --output build/ps2-compile-inventory --max-units 40 --timeout 90 \
  --include src/SB/Game
```

By default, already profiled units are skipped. Repeat `--unit` with exact DWARF
source paths to choose units explicitly, including previously profiled units:
`--unit SB/Core/x/xPar.cpp --unit SB/Core/x/xString.cpp`.
The output contains private objects and full compiler logs; only the metadata is
suitable for sharing. A compiled object still needs original relocation and code
comparison before it can contribute to a source profile.

The initial 38-unit core inventory used the established `-O4,p`,
`-Cpp_exceptions off`, and `-inline deferred` settings. Unchanged `xPar`,
`xFactory`, and `xParGroup` compiled immediately. `xString` compiled after extending
its existing platform include guard to omit the unused `rwplcore.h` on PS2. Its
function bodies are unchanged.

The most common blockers were `rwcore.h` (25 units), `rpworld.h` (23), `iEnv.h`
(19), `iMath3.h` (17), `iColor.h` and `iSnd.h` (12 each), and `iFile.h` (11).
These are counts of failed include observations, not promises that adding one
header fixes the entire unit. For example, an independently recovered PS2
`iColor.h` removes that include error from `xBound`, but the unit still needs real
collision definitions. No GameCube SDK layouts were substituted. `xTRC` timed out
at 90 seconds after reporting header errors; the other 37 attempts completed.
Standard-library dependencies include the explicit PowerPC includes for `stdlib`
and placement `new`, plus `cstring`, `cstdlib`, `cstdio`/`stdio`, and character APIs.

The first complete source profiles preserve every original owned function, even
when its source differs or a static relocation remains unresolved:

| Unit | Original functions | Exact functions | Exact bytes / original bytes |
| --- | ---: | ---: | ---: |
| xPar | 4 | 3 | 276 / 352 |
| xFactory | 11 | 7 | 688 / 1396 |
| xParGroup | 12 | 6 | 220 / 1488 |
| xString | 12 | 2 | 344 / 3948 |

All 18 exact functions (1528 bytes) were checked by applying actual compiled
object relocations to independently named addresses from each of the three debug
originals, then comparing every resulting byte. `memset` uses the existing
separately reviewed runtime anchor. Unknown static literal data and unowned helper
identities remain unresolved. No unit or executable link completion is claimed.
These profiles explicitly permit only the three debug executable hashes; France's
partial diagnostic coverage remains unchanged.

One narrow relocation validator extension recognizes MIPS `LW`: it reads its
base register and writes its destination register. This permits the original
`xFactory::RegItemType(XGOFTypeInfo*)` callback capture without weakening the
existing clobber or control-flow checks. At function offsets 164..180 the original
loads `OrdComp_infotype` into `a2` with `LUI`/`ADDIU`; the intervening instructions
are `LW v1,0(s2)`, `LW v0,0(s0)`, and `ADDIU a0,s2,4`. None reads or writes `a2`.
The separately named callback destination, high-half carry, signed low half, and
inverse reconstruction are checked against each actual original.

Validation artifacts from this pass are under `build/inventory153`: the 38-unit
logs, `blockers.json`, three `raw-screen-<version>.json` proofs, four production
reports, and a real run of the reusable inventory covering both successful and
blocked whole units. All prior profiled unit records remain unchanged. The total
exact PS2 source code rises from 5696 to 7224 bytes in each debug region; France
remains at 1460 bytes. These small exact totals sit within the existing full CPU
text denominator rather than a new selected-function denominator.
