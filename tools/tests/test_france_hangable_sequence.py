"""Original-backed stability when independently recovered Event records grow."""
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from platforms.verify_reviewed import Original, REFERENCES, TARGET
from platforms.france_hangable_sequence import generate_unit


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"), "Set BFBB_FRANCE_TEST_ORIG for original fixtures")
class HangableDependencyTests(unittest.TestCase):
    def test_recovered_event_records_do_not_replace_original_context(self):
        registry = Path(os.environ.get("BFBB_FRANCE_TEST_REGISTRY", ROOT / "config/platforms/SLES-53623"))
        documents = {path.name: json.loads(path.read_text()) for path in registry.glob("*functions.json")}
        event_records = [f for document in documents.values() for f in document["functions"]
                         if f["source"] == "SB/Core/x/xEvent.cpp"]
        if not event_records:
            self.skipTest("Fixture registry predates Event recovery")
        records = json.loads((ROOT / "config/platforms/versions.json").read_text())["versions"]
        originals = {v: Original(v, records[v], Path(os.environ["BFBB_FRANCE_TEST_ORIG"]))
                     for v in (*REFERENCES, TARGET)}
        for version, original in originals.items():
            layout = json.loads((ROOT / "config/platforms" / version / "region-layout.json").read_text())
            self.assertEqual(layout["executable_sha1"], original.sha1)
            original._stream_regions = {r["name"]: r for r in layout["regions"]}
        with_events = generate_unit(originals, registry)
        with tempfile.TemporaryDirectory() as directory:
            without_events = Path(directory)
            for name, document in documents.items():
                document["functions"] = [f for f in document["functions"]
                                         if f["source"] != "SB/Core/x/xEvent.cpp"]
                (without_events / name).write_text(json.dumps(document))
            earlier = generate_unit(originals, without_events)
        self.assertEqual(with_events, earlier)
        contexts = [c for c in with_events["call_neighbors"] if c["reference_name"] == "zEntEvent"]
        self.assertEqual({c["version"] for c in contexts}, set(REFERENCES))
        self.assertEqual(len(contexts), 3)
        self.assertTrue(all(not c["promoted_as_named_anchor"] for c in contexts))


if __name__ == "__main__":
    unittest.main()
