# PS2 TalkBox and DiscoFloor source comparisons

Two narrow include selections enable complete source compilation with the established PS2 declarations:

- `zTalkBox.cpp` includes the existing public `xpkrsvc_api.h` on PS2. Its required `st_PKR_ASSET_TOCINFO` is defined there; the private packer header unnecessarily pulled GameCube OS/time types into this unit.
- `zDiscoFloor.cpp` includes standard `stdlib.h` on PS2 for its existing `atoi` call. Other platforms retain the previous MSL include.

No function bodies, SDK layouts, compiler flags or comparison rules change. Both original-owned inventories remain complete and partial:

| Unit | Original functions / bytes | Standard code matches | Independently reconstructed matches | Fuzzy code match |
| --- | ---: | ---: | ---: | ---: |
| zTalkBox | 61 / 19,916 | 31 / 2,368 | 28 / 1,788 | 51.465755% |
| zDiscoFloor | 27 / 16,024 | 5 / 628 | 5 / 628 | 34.734398% |

All three authenticated debug originals have the same results. The standard gain is 36 functions / 2,996 bytes per version; independent source-relocation application reconstructs 33 functions / 2,416 bytes exactly. TalkBox's remaining standard-only matches are `load_settings` (48 bytes, unresolved literal), `parse_tag_teleport` (340 bytes, unresolved asset-type strings) and `read_bool` (192 bytes, unresolved local string tables). These are not claimed as raw or executable-link matches.

The profiles restore only independently identified original references: TalkBox has 199 direct calls and 25 GP operands, with two unmodeled original call/GP references; DiscoFloor has 91 calls and 53 GP operands, with 45 unmodeled references. Every unmatched function remains represented; no original identities or runtime implementations are invented.

Actual normal/debug outputs have identical ordered allocated sections. All common named aggregate variants and direct bitfield attributes agree with each original: 204 variants for TalkBox and 125 for DiscoFloor. Actual GameCube command replays preserve TalkBox's 16 allocated sections and DiscoFloor's six; DiscoFloor's entire object is identical. Other-platform include branches remain unchanged.

Private evidence is in `build/game232`: initial whole-TU compiler diagnostics, final normal/debug commands, complete per-region reports, aggregate inventories, raw reconstruction and GC comparisons. `profiles.json` contains two additive groups, each scoped to the three independently verified original hashes. No French boundary recovery or source eligibility is inferred. The larger xFont renderer/API prerequisite remains a separate unmodified task.
