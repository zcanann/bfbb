#!/usr/bin/env python3
"""Fail closed if the packaged combo-storage site contains bad public data."""

from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
from typing import Any


ALLOWED_CLASSIFICATIONS = {
    "verified_reachable_runtime_result",
    "natural_source_room_patched_destination_recon",
}
ALLOWED_EFFECTS = {
    "wrong-warp", "progression", "reward", "gameplay-corruption",
    "visual", "crash", "freeze", "no-effect", "unresolved",
}
SCENE_PREFIXES = {
    "HB", "JF", "BB", "GL", "B1", "RB", "BC", "SM", "B2", "KF",
    "GY", "DB", "B3", "PG", "MN",
}
FORBIDDEN_PUBLIC_WORDING = {
    "synthetic", "artificial", "forced address", "injected head",
    "lab-only", "lab assisted", "lab-assisted", "oracle",
}
REQUIRED_FIELDS = {
    "public_id", "platform", "source_scene", "destination_scene",
    "classification", "effect_category", "effect_label",
    "setup_description", "effect_description", "notes_description",
}


def load(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8-sig"))


def public_classification(value: object) -> bool:
    text = str(value or "")
    return text in ALLOWED_CLASSIFICATIONS or text.startswith("player_replay_verified")


def verify(site: Path) -> dict[str, Any]:
    data = site / "data"
    manifest = load(data / "manifest.json")
    index = load(data / "results-index.json")
    if manifest.get("schema") != "bfbb.combo.published-site/v1":
        raise ValueError("unexpected site manifest schema")
    if manifest.get("published_scope") != "verified_real_producer_results":
        raise ValueError("site is not restricted to verified real producers")

    rows: list[dict[str, Any]] = []
    indexed_rows = 0
    for chunk in index.get("chunks") or []:
        path = data / str(chunk.get("file") or "")
        if path.parent != data or not path.is_file():
            raise ValueError(f"missing or unsafe result chunk: {path}")
        payload = load(path)
        if not isinstance(payload, list):
            raise ValueError(f"result chunk is not a list: {path}")
        expected = int(chunk.get("rows") or 0)
        if len(payload) != expected:
            raise ValueError(f"chunk count mismatch: {path}")
        indexed_rows += expected
        rows.extend(payload)

    declared = int(index.get("total") or 0)
    if len(rows) != indexed_rows or len(rows) != declared:
        raise ValueError("result index total does not match its chunks")
    if int(manifest.get("published_rows") or 0) != declared:
        raise ValueError("manifest total does not match result index")
    if not rows:
        raise ValueError("published catalog is empty")

    ids: set[str] = set()
    for row_number, row in enumerate(rows, 1):
        missing = sorted(field for field in REQUIRED_FIELDS if not row.get(field))
        if missing:
            raise ValueError(f"row {row_number} is missing: {', '.join(missing)}")
        public_id = str(row["public_id"])
        if public_id in ids:
            raise ValueError(f"duplicate public result id: {public_id}")
        ids.add(public_id)
        if not public_classification(row.get("classification")):
            raise ValueError(f"unverified classification published: {row.get('classification')}")
        if str(row.get("effect_category")) not in ALLOWED_EFFECTS:
            raise ValueError(f"unknown effect category: {row.get('effect_category')}")
        if str(row.get("platform")) not in {"gamecube", "ps2", "xbox"}:
            raise ValueError(f"unknown platform: {row.get('platform')}")
        for field in ("source_scene", "destination_scene"):
            scene = str(row.get(field) or "")
            if not any(scene.startswith(prefix) for prefix in SCENE_PREFIXES):
                raise ValueError(f"unmapped scene code in {field}: {scene}")
        player_text = " ".join(str(row.get(field) or "") for field in (
            "setup_description", "effect_description", "notes_description",
        )).lower()
        for phrase in FORBIDDEN_PUBLIC_WORDING:
            if phrase in player_text:
                raise ValueError(f"internal research wording leaked into {public_id}: {phrase}")

    effects = Counter(str(row["effect_category"]) for row in rows)
    if not sum(count for effect, count in effects.items() if effect not in {"no-effect", "unresolved"}):
        raise ValueError("default changed-results view would be empty")
    expected_effects = index.get("summary", {}).get("by_effect") or {}
    if dict(sorted(effects.items())) != dict(sorted(expected_effects.items())):
        raise ValueError("effect summary does not match result rows")
    platforms = Counter(str(row["platform"]) for row in rows)
    if dict(sorted(platforms.items())) != dict(sorted((manifest.get("platforms") or {}).items())):
        raise ValueError("platform summary does not match result rows")
    return {
        "rows": len(rows),
        "platforms": dict(sorted(platforms.items())),
        "effects": dict(sorted(effects.items())),
        "changed_by_default": sum(
            count for effect, count in effects.items()
            if effect not in {"no-effect", "unresolved"}
        ),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("site", type=Path, nargs="?", default=Path("site/combostorage"))
    args = parser.parse_args()
    print(json.dumps(verify(args.site.resolve()), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
