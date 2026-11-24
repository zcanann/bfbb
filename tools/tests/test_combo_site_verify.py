import json

import pytest

from tools.bfbb_combo_site_verify import verify


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value), encoding="utf-8")


def row(**changes):
    value = {
        "public_id": "result_1", "platform": "xbox",
        "source_scene": "SM01", "destination_scene": "SM02",
        "classification": "verified_reachable_runtime_result",
        "effect_category": "freeze", "effect_label": "Reset softlock",
        "setup_description": "Build a combo, quit, and load Sand Mountain.",
        "effect_description": "Death never returns to gameplay.",
        "notes_description": "The matched clean reset works normally.",
    }
    value.update(changes)
    return value


def make_site(tmp_path, rows):
    site = tmp_path / "site"
    write_json(site / "data/results-01.json", rows)
    effects = {}
    for item in rows:
        effect = item["effect_category"]
        effects[effect] = effects.get(effect, 0) + 1
    write_json(site / "data/results-index.json", {
        "total": len(rows), "summary": {"by_effect": effects},
        "chunks": [{"file": "results-01.json", "rows": len(rows)}],
    })
    platforms = {}
    for item in rows:
        platform = item["platform"]
        platforms[platform] = platforms.get(platform, 0) + 1
    write_json(site / "data/manifest.json", {
        "schema": "bfbb.combo.published-site/v1",
        "published_scope": "verified_real_producer_results",
        "published_rows": len(rows), "platforms": platforms,
    })
    return site


def test_verified_site_passes(tmp_path):
    assert verify(make_site(tmp_path, [row()]))["changed_by_default"] == 1


def test_internal_or_unverified_rows_fail_closed(tmp_path):
    with pytest.raises(ValueError, match="unverified classification"):
        verify(make_site(tmp_path, [row(classification="lab_oracle")]))
    with pytest.raises(ValueError, match="research wording"):
        verify(make_site(tmp_path, [row(setup_description="Use a synthetic head")]))
