"""Large-body discovery and trusted/soft occupancy separation."""
from pathlib import Path
import hashlib
import json
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from platforms import france_fuzzy as fuzzy
from platforms import france_layout as layout


class LayoutTests(unittest.TestCase):
    def test_occupied_boundaries_are_half_open_and_split_windows(self):
        occupied = layout.OccupiedRanges([(20, 40), (40, 50), (80, 100)])
        self.assertFalse(occupied.overlaps(0, 20))
        self.assertFalse(occupied.overlaps(50, 80))
        self.assertTrue(occupied.overlaps(49, 51))
        self.assertEqual(list(occupied.available(10, 110)), [(10, 20), (50, 80), (100, 110)])
        self.assertEqual(list(occupied.available(25, 45)), [])

    def test_large_gapped_body_survives_quadratic_cell_cap(self):
        q = list(range(100, 2100))
        body = q[:700] + [8001, 8002] + q[700:1400] + q[1401:]
        target = [0] * 40 + body + [0] * 80
        with tempfile.TemporaryDirectory() as directory:
            index = fuzzy.SeedIndex(target, 5, Path(directory) / "index.sqlite")
            try:
                candidates, diagnostics = fuzzy.search(index, q, max_cells=1000000)
            finally:
                index.close()
        self.assertTrue(candidates)
        candidate = candidates[0]
        self.assertEqual(candidate["alignment_kind"], "coarse_seed_window")
        self.assertLessEqual(candidate["start"], 40)
        self.assertGreaterEqual(candidate["end"], 40 + len(body))
        self.assertGreater(candidate["lcs_dice_percent"], 97)
        self.assertIsNone(candidate["edit_distance"])
        self.assertEqual(candidate["alignment"], [])
        self.assertGreater(diagnostics["coarse_scored_windows"], 0)
        self.assertEqual(diagnostics["refined_candidates"], 0)

    def test_verified_copy_is_excluded_without_hiding_unknown_copy(self):
        q = list(range(100, 180))
        target = [0] * 20 + q + [0] * 80 + q + [0] * 20
        occupied = layout.OccupiedRanges([(20, 100)])
        with tempfile.TemporaryDirectory() as directory:
            index = fuzzy.SeedIndex(target, 5, Path(directory) / "index.sqlite")
            try:
                candidates, _ = fuzzy.search(index, q, occupied=occupied)
            finally:
                index.close()
        self.assertEqual(candidates[0]["start"], 180)
        self.assertTrue(all(not occupied.overlaps(c["start"], c["end"]) for c in candidates))

    def test_blocks_follow_original_adjacency_and_known_members_are_barriers(self):
        fs = [{"name": name, "source": source, "low": low, "high": low + 64}
              for name, source, low in [("a", "A", 0), ("b", "A", 64), ("c", "A", 128),
                                        ("d", "B", 192), ("e", "A", 256), ("f", "A", 320)]]
        queries, skipped = layout.reference_queries(fs, [{"name": "c", "source": "A"}], blocks=True)
        blocks = [q for q in queries if q["reference_kind"] == "unit_block"]
        self.assertEqual([f["name"] for f in skipped], ["c"])
        self.assertEqual([(q["low"], q["high"]) for q in blocks], [(0, 128), (256, 384)])

    def test_verified_reference_provenance_distinguishes_overloads(self):
        fs = [{"source": "A", "name": "overload", "low": low, "high": low + 32}
              for low in (100, 200)]
        verified = [{"source": "A", "name": "overload", "reference_entries": [
            {"version": "V", "executable_sha1": "hash", "source": "A", "name": "overload",
             "source_address": 100}]}]
        queries, skipped = layout.reference_queries(fs, verified, reference_version="V", reference_sha1="hash")
        self.assertEqual([f["low"] for f in skipped], [100])
        self.assertEqual([f["low"] for f in queries], [200])

    def test_registry_identity_bytes_and_boundaries_are_required(self):
        region, body = {"address": 100, "size": 16}, bytes(range(16))
        entry = {"name": "a", "source": "A", "address": 100, "size": 8,
                 "sha256": hashlib.sha256(body[:8]).hexdigest(), "boundary_confirmation": True}
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "reviewed-functions.json"
            document = {"executable_sha1": "expected", "functions": [entry]}
            path.write_text(json.dumps(document))
            verified, _ = layout.verified_functions(Path(directory), "expected", region, body)
            self.assertEqual(len(verified), 1)
            with self.assertRaises(ValueError):
                layout.verified_functions(Path(directory), "wrong", region, body)
            with self.assertRaises(ValueError):
                layout.verified_functions(Path(directory), "expected", region, bytes(16))
            entry["boundary_confirmation"] = False
            path.write_text(json.dumps(document))
            with self.assertRaises(ValueError):
                layout.verified_functions(Path(directory), "expected", region, body)

    def test_only_proven_cluster_kind_can_occupy_the_tu_registry(self):
        region, body = {"address": 100, "size": 16}, bytes(range(16))
        entry = {"name": "a", "source": "A", "address": 100, "size": 8,
                 "sha256": hashlib.sha256(body[:8]).hexdigest(), "boundary_confirmation": True,
                 "confirmation_kind": "reviewed-complete-caller-callee-cluster"}
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "tu-corroborated-functions.json"
            document = {"executable_sha1": "expected", "functions": [entry]}
            path.write_text(json.dumps(document))
            verified, _ = layout.verified_functions(Path(directory), "expected", region, body)
            self.assertEqual(len(verified), 1)
            self.assertTrue(layout.OccupiedRanges([(v["address"], v["address"] + v["size"])
                                                 for v in verified]).overlaps(100, 108))
            entry["confirmation_kind"] = "fuzzy-candidate"
            path.write_text(json.dumps(document))
            with self.assertRaises(ValueError):
                layout.verified_functions(Path(directory), "expected", region, body)

    def test_soft_anchors_reorder_but_never_remove_ambiguous_candidates(self):
        candidates = [{"candidate_address": address, "candidate_size": 40,
                       "lcs_dice_percent": score, "edit_similarity_percent": None}
                      for address, score in [(120, 98), (400, 96)]]
        anchor = {"name": "large", "source": "A", "reference_size": 200,
                  "candidate_address": 100, "candidate_size": 200, "reference_kind": "function"}
        reference = {"name": "small", "source": "B", "reference_size": 40}
        layout.apply_soft_penalties(candidates, reference, [anchor], 5)
        self.assertEqual([c["candidate_address"] for c in candidates], [400, 120])
        self.assertEqual(candidates[1]["soft_overlap_penalty_percent"], 5)
        self.assertFalse(candidates[1]["soft_conflicts"][0]["boundary_confirmation"])
        anchor.update(source="B", reference_kind="unit_block")
        layout.apply_soft_penalties(candidates, reference, [anchor], 5)
        self.assertEqual(candidates[0]["candidate_address"], 120)

    def test_imported_claims_never_become_occupied(self):
        region = {"address": 100, "size": 1000}
        value = {"name": "tentative", "source": "A", "reference_size": 200,
                 "candidate_address": 120, "candidate_size": 200, "boundary_confirmation": True}
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "map.json"
            path.write_text(json.dumps({"target_sha1": "expected", "hypotheses": [value]}))
            imported = layout.soft_hypotheses([path], "expected", region)
        self.assertFalse(imported[0]["boundary_confirmation"])
        result = layout.region_map("expected", region, [], [], [], imported)
        self.assertEqual(result["verified_code_bytes"], 0)
        self.assertEqual(result["unclaimed_regions_largest_first"][0]["size"], 1000)

    def test_map_replay_lowers_incompatible_rank_without_claiming_occupancy(self):
        region = {"address": 100, "size": 1000}
        large = {"name": "large", "source": "A", "reference_address": 2000,
                 "reference_size": 200, "reference_kind": "function", "candidates": [
                     {"candidate_address": 100, "candidate_size": 200, "lcs_dice_percent": 99,
                      "alignment_kind": "coarse_seed_window", "alignment": []}]}
        original = layout.region_map("expected", region, [], [], [large])
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "map.json"
            path.write_text(json.dumps(original))
            imported = layout.soft_hypotheses([path], "expected", region)
        candidates = [{"candidate_address": address, "candidate_size": 40,
                       "lcs_dice_percent": score, "edit_similarity_percent": None}
                      for address, score in [(120, 98), (400, 96)]]
        layout.apply_soft_penalties(candidates, {"name": "small", "source": "B", "reference_size": 40},
                                    imported, 5)
        self.assertEqual([c["candidate_address"] for c in candidates], [400, 120])
        self.assertEqual(len(candidates), 2)
        replayed = layout.region_map("expected", region, [], [], [], imported)
        self.assertEqual(replayed["verified_code_bytes"], 0)
        self.assertEqual(replayed["unclaimed_regions_largest_first"][0]["size"], 1000)
        self.assertTrue(all(h["boundary_confirmation"] is False for h in replayed["hypotheses"]))


if __name__ == "__main__":
    unittest.main()
