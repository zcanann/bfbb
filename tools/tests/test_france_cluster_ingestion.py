"""Cluster labels retain all existing regenerated-proof ingestion checks."""
from copy import deepcopy
import hashlib
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from platforms.ps2_report import _merge_corroborated
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.france_tu_sequences import KIND


class ClusterIngestionTests(unittest.TestCase):
    def setUp(self):
        self.body = bytes(range(16))
        self.entry = {"name": "f", "source": "SB/f.cpp", "address": 0x100, "size": 8,
                      "sha256": hashlib.sha256(self.body[:8]).hexdigest(),
                      "boundary_confirmation": True, "confirmation_kind": CLUSTER_KIND}
        self.registry = {"executable_sha1": "original", "coverage_complete": False,
                         "functions": [self.entry]}
        self.metadata = {"sha1": "original"}
        self.loaded = [{"address": 0x100, "offset": 0, "file_size": 16}]

    def merge(self, registry=None, expected=None, functions=None, kind=(KIND, CLUSTER_KIND)):
        registry = self.registry if registry is None else registry
        return _merge_corroborated([] if functions is None else functions, registry,
            deepcopy(registry) if expected is None else expected,
            self.metadata, self.body, self.loaded, kind)

    def test_exact_generated_cluster_preserves_its_actual_provenance(self):
        functions = []
        self.assertEqual(self.merge(functions=functions), 1)
        self.assertEqual(functions[0]["provenance"], CLUSTER_KIND)
        with self.assertRaises(ValueError):
            self.merge(kind=KIND)

    def test_new_kind_never_bypasses_original_proof_and_extent_checks(self):
        mutations = [{"confirmation_kind": "fuzzy-candidate"}, {"boundary_confirmation": False},
                     {"sha256": "changed"}, {"address": 0x102}, {"size": 20}, {"size": 0}]
        for changes in mutations:
            with self.subTest(changes=changes):
                registry = deepcopy(self.registry)
                registry["functions"][0].update(changes)
                with self.assertRaises(ValueError):
                    self.merge(registry=registry)
        for changes in ({"executable_sha1": "wrong"}, {"coverage_complete": True}):
            registry = {**self.registry, **changes}
            with self.assertRaises(ValueError):
                self.merge(registry=registry)
        stale = deepcopy(self.registry)
        stale["functions"][0]["name"] = "stale"
        with self.assertRaises(ValueError):
            self.merge(expected=stale)
        with self.assertRaises(ValueError):
            self.merge(functions=[{"name": "other", "source": "SB/other.cpp", "low": 0x104, "high": 0x10c}])


if __name__ == "__main__":
    unittest.main()
