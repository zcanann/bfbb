"""Algorithm tests; set BFBB_FRANCE_TEST_ORIG for authenticated-original checks."""
from pathlib import Path
import os
import random
import sys
import tempfile
from types import SimpleNamespace
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from platforms import france_fuzzy as fuzzy


def slow_lcs(a, b):
    previous = [0] * (len(b) + 1)
    for x in a:
        row = [0]
        for j, y in enumerate(b):
            row.append(previous[j] + 1 if x == y else max(previous[j + 1], row[-1]))
        previous = row
    return previous[-1]


class FuzzyTests(unittest.TestCase):
    def test_lcs_against_independent_dynamic_programming(self):
        rng = random.Random(53623)
        for _ in range(300):
            a = [rng.randrange(7) for _ in range(rng.randrange(35))]
            b = [rng.randrange(7) for _ in range(rng.randrange(35))]
            self.assertEqual(fuzzy.lcs_length(a, b), slow_lcs(a, b))

    def test_normalization_preserves_architectural_roles_and_literals(self):
        def addiu(rs, rt, imm):
            return (9 << 26) | (rs << 21) | (rt << 16) | (imm & 65535)
        self.assertEqual(fuzzy.token(addiu(4, 5, 56)), fuzzy.token(addiu(8, 9, 56)))
        self.assertNotEqual(fuzzy.token(addiu(4, 5, 56)), fuzzy.token(addiu(8, 9, 60)))
        self.assertNotEqual(fuzzy.token(addiu(0, 5, 56)), fuzzy.token(addiu(8, 9, 56)))
        self.assertNotEqual(fuzzy.token(addiu(29, 29, -32)), fuzzy.token(addiu(8, 9, -32)))
        self.assertNotEqual(fuzzy.token((31 << 21) | 8), fuzzy.token((4 << 21) | 8))
        self.assertEqual(fuzzy.token((3 << 26) | 123), fuzzy.token((3 << 26) | 789))
        self.assertNotEqual(fuzzy.token((1 << 26) | (1 << 16)), fuzzy.token(1 << 26))

    def test_alignment_reconstructs_mixed_edits_and_free_flanks(self):
        a = list(range(100, 140))
        b = a[:10] + [900, 901] + a[10:20] + a[21:30] + [902] + a[31:]
        window = [888] * 7 + b + [777] * 9
        result = fuzzy.align(a, window, 10000)
        self.assertEqual((result["start"], result["end"]), (7, 7 + len(b)))
        self.assertEqual(result["edit_distance"], 4)
        self.assertEqual(result["instructions"], {"equal": 38, "replace": 1, "insert": 2, "delete": 1})
        qa, ta, rebuilt = 0, result["start"], []
        for run in result["alignment"]:
            self.assertEqual((run["reference_start"], run["target_start"]), (qa, ta))
            qe, te = run["reference_end"], run["target_end"]
            if run["operation"] == "equal":
                self.assertEqual(a[qa:qe], window[ta:te])
                rebuilt.extend(a[qa:qe])
            elif run["operation"] != "delete":
                rebuilt.extend(window[ta:te])
            qa, ta = qe, te
        self.assertEqual((qa, ta), (len(a), result["end"]))
        self.assertEqual(rebuilt, b)
        self.assertEqual(result["lcs_instructions"], slow_lcs(a, b))

    def test_seed_search_handles_insertions_and_register_changes(self):
        a = [(9 << 26) | (4 << 21) | (5 << 16) | i for i in range(20, 70)]
        b = [(w & ~((31 << 21) | (31 << 16))) | (8 << 21) | (9 << 16) for w in a]
        b[15:15] = [0, 0]
        del b[35]
        q = [fuzzy.token(w) for w in a]
        target = [0] * 100 + [fuzzy.token(w) for w in b] + [0] * 100
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "index.sqlite"
            index = fuzzy.SeedIndex(target, 5, path)
            try:
                candidates, _ = fuzzy.search(index, q, slack=12)
                self.assertEqual(candidates[0]["start"], 100)
                self.assertGreater(candidates[0]["edit_similarity_percent"], 94)
                self.assertEqual(candidates[0]["instructions"]["insert"], 2)
                self.assertEqual(candidates[0]["instructions"]["delete"], 1)
            finally:
                index.close()
            cached = fuzzy.SeedIndex(target, 5, path)
            try:
                self.assertTrue(cached.cache_hit)
                self.assertEqual(fuzzy.search(cached, q, slack=12)[0], candidates)
            finally:
                cached.close()

    def test_ambiguous_copies_are_retained_and_cell_limit_is_explicit(self):
        q = list(range(30, 70))
        target = [0] * 20 + q + [0] * 150 + q + [0] * 20
        with tempfile.TemporaryDirectory() as d:
            index = fuzzy.SeedIndex(target, 5, Path(d) / "index.sqlite")
            try:
                candidates, diagnostics = fuzzy.search(index, q, slack=10)
                self.assertEqual([c["start"] for c in candidates[:2]], [20, 210])
                self.assertEqual(diagnostics["best_runner_up_gap_percent"], 0)
                candidates, diagnostics = fuzzy.search(index, q, slack=10, max_cells=1)
                self.assertFalse(candidates)
                self.assertEqual(diagnostics["cell_budget_skipped_windows"], 2)
            finally:
                index.close()


@unittest.skipUnless(os.environ.get("BFBB_FRANCE_TEST_ORIG"), "private original path not supplied")
class OriginalTests(unittest.TestCase):
    def test_independently_verified_exact_and_changed_originals(self):
        with tempfile.TemporaryDirectory() as directory:
            args = SimpleNamespace(manifest=fuzzy.ROOT / "config/platforms/versions.json",
                orig_dir=Path(os.environ["BFBB_FRANCE_TEST_ORIG"]), cache_dir=Path(directory),
                config_dir=fuzzy.ROOT / "config/platforms", reference="SLUS-20680",
                name="xStrHash", source="xString.cpp", address=None, seed_words=5,
                limit=3, max_words=4096, top=5, slack=32, max_windows=12,
                max_cells=1000000, max_seeds=24, max_occurrences=64)
            cold = fuzzy.run(args)
            expected = {0x20F190, 0x20F1F0, 0x20F260}
            self.assertEqual({f["candidates"][0]["candidate_address"] for f in cold["functions"]}, expected)
            self.assertTrue(all(f["candidates"][0]["raw_identical_body"] for f in cold["functions"]))
            warm = fuzzy.run(args)
            self.assertTrue(all(warm["cache"].values()))
            self.assertEqual(cold["functions"], warm["functions"])
            args.name, args.source = "xGridUpdate", "xGrid.cpp"
            changed = fuzzy.run(args)
            candidate = changed["functions"][0]["candidates"][0]
            self.assertEqual(candidate["candidate_address"], 0x305730)
            self.assertEqual(candidate["candidate_size"], 204)
            self.assertFalse(candidate["raw_identical_body"])
            self.assertEqual(candidate["edit_similarity_percent"], 100)
            self.assertFalse(changed["eligible_for_progress"])
            self.assertFalse(candidate["boundary_confirmation"])


if __name__ == "__main__":
    unittest.main()
