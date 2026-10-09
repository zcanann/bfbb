#!/usr/bin/env python3
"""Rank France function candidates using instruction seeds and gapped alignment.

This is a discovery tool. Its inferred spans and names are NOT verified function
boundaries, recovered symbols, relocation proofs, or matching-progress inputs.
Only authenticated retail executables are read; no compiled source is consulted.
"""
from __future__ import annotations

import argparse
from array import array
from collections import Counter
from contextlib import closing
import hashlib
import json
from pathlib import Path
import sqlite3
import struct
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
from platforms.france_candidates import REFERENCES, TARGET, _loaded
from platforms.ps2 import inspect_elf
from platforms.ps2_report import _function_ranges, _source_name

CACHE_VERSION = 1
MASK = (1 << 63) - 1
BASE = 1000003


def token(word: int) -> int:
    """Discovery normalization, deliberately weaker than a relocation proof.

    Keep zero/GP/SP/RA roles, instruction selectors, shifts, comparison/logical
    literals and small field offsets. Erase ordinary register allocation, direct
    destinations, branch displacements, LUI values, GP/stack offsets and large
    address-like add/load/store immediates. Unknown encodings stay literal.
    """
    op, rs, rt = word >> 26, (word >> 21) & 31, (word >> 16) & 31
    rd, sa, fn = (word >> 11) & 31, (word >> 6) & 31, word & 63
    role = lambda r: r if r in (0, 28, 29, 31) else 1
    regs = (role(rs) << 21) | (role(rt) << 16)
    if word == 0:
        return 0
    if op == 0:
        # Syscall/break codes and SYNC subtype are not register fields.
        if fn in (12, 13, 15):
            return word
        return regs | (role(rd) << 11) | (sa << 6) | fn
    if op in (2, 3):
        return op << 26
    if op == 1:  # REGIMM: rt is the branch/trap selector.
        imm = 0 if rt in (0, 1, 2, 3, 16, 17, 18, 19) else word & 65535
        return (op << 26) | (role(rs) << 21) | (rt << 16) | imm
    if op in (4, 5, 6, 7, 20, 21, 22, 23):
        return (op << 26) | regs
    if op == 15:
        return (op << 26) | (role(rt) << 16)
    if op in (16, 17, 18):
        if rs == 8:  # COP branch: keep cc/tf/likely selector.
            return (op << 26) | (rs << 21) | (rt << 16)
        if rs in (0, 1, 2, 4, 5, 6):
            # COP0 register indexes are architectural, not allocated FPRs.
            return ((op << 26) | (rs << 21) | (role(rt) << 16) |
                    ((rd << 11) if op == 16 else 0) | (word & 2047))
        if op == 17 and rs in (16, 17, 20, 21):
            return (op << 26) | (rs << 21) | fn
        return word  # VU macro/COP0 operations need a proper separate decoder.
    if op == 28:  # R5900 MMI: sa and fn select the packed operation.
        return (op << 26) | regs | (role(rd) << 11) | (sa << 6) | fn
    if op in (8, 9, 24, 25, 26, 27, 30, 31, 32, 33, 34, 35, 36, 37,
              38, 39, 40, 41, 42, 43, 44, 45, 46, 49, 54, 55, 57, 62, 63):
        signed = (word & 65535) - (65536 if word & 32768 else 0)
        imm = word & 65535 if rs not in (28, 29) and abs(signed) <= 256 else 0
        # COP1/2 loads and stores use FPR/VU registers rather than GPR roles.
        result_regs = (role(rs) << 21) if op in (49, 54, 57, 62) else regs
        return (op << 26) | result_regs | imm
    if op in (10, 11, 12, 13, 14):
        return (op << 26) | regs | (word & 65535)
    return word


def decode(body: bytes) -> list[int]:
    if len(body) % 4:
        raise ValueError("Unaligned instruction body")
    return [token(w[0]) for w in struct.iter_unpack("<I", body)]


def seeds(tokens: list[int], width: int):
    """Deterministic rolling hashes; query hits are checked against real tokens."""
    if len(tokens) < width:
        return
    power, value = pow(BASE, width - 1, 1 << 63), 0
    for t in tokens[:width]:
        value = (value * BASE + t + 1) & MASK
    yield 0, value
    for i in range(width, len(tokens)):
        value = ((value - (tokens[i - width] + 1) * power) * BASE + tokens[i] + 1) & MASK
        yield i - width + 1, value


def atomic_json(path: Path, value: dict) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                     delete=False) as stream:
        tmp = Path(stream.name)
        json.dump(value, stream)
    try:
        tmp.replace(path)
    finally:
        tmp.unlink(missing_ok=True)


def cached_tokens(body: bytes, path: Path) -> tuple[list[int], bool]:
    """Cache only normalized uint32 tokens, keyed by original/region/version."""
    hit = path.is_file()
    if hit:
        data = path.read_bytes()
        if len(data) != len(body):
            raise ValueError("Normalized instruction cache has the wrong length")
        values = array("I")
        values.frombytes(data)
        if sys.byteorder != "little":
            values.byteswap()
        return list(values), True
    values = decode(body)
    encoded = array("I", values)
    if sys.byteorder != "little":
        encoded.byteswap()
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(encoded.tobytes())
    try:
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)
    return values, False


def original(version: str, records: dict, orig_dir: Path, cache: Path):
    record = records[version]
    if record["platform"] != "ps2":
        raise ValueError("Expected a PS2 original")
    path = orig_dir / version / Path(record["executable"]["path"]).name
    data = path.read_bytes()
    digest = hashlib.sha1(data).hexdigest()
    if digest != record["executable"]["sha1"]:
        raise ValueError(f"{version}: original executable hash mismatch")
    cached = cache / f"original-v{CACHE_VERSION}-{digest}.json"
    hit = cached.is_file()
    if hit:
        document = json.loads(cached.read_text(encoding="utf-8"))
    else:
        metadata = inspect_elf(path)
        if metadata["sha1"] != digest:
            raise ValueError("Original changed during inspection")
        functions = []
        if version in REFERENCES:
            debug = [s for s in metadata["sections"] if s["name"] == ".debug" and s["size"]]
            if len(debug) != 1:
                raise ValueError("Expected one original DWARF1 section")
            section = debug[0]
            functions = _function_ranges(data[section["offset"]:section["offset"] + section["size"]])
            for f in functions:
                f["source"] = _source_name(f["source"])
        document = {"sha1": digest, "metadata": metadata, "functions": functions}
        atomic_json(cached, document)
    if document["sha1"] != digest or document["metadata"]["sha1"] != digest:
        raise ValueError("Original metadata cache identity mismatch")
    return data, document, hit


def read_body(data: bytes, metadata: dict, address: int, size: int) -> bytes:
    owners = [s for s in _loaded(metadata) if s["address"] <= address and
              address + size <= s["address"] + s["file_size"]]
    if len(owners) != 1 or size < 0 or address % 4 or size % 4:
        raise ValueError("Instruction span lacks a unique aligned loaded owner")
    offset = owners[0]["offset"] + address - owners[0]["address"]
    return data[offset:offset + size]


class SeedIndex:
    def __init__(self, tokens: list[int], width: int, path: Path):
        self.tokens, self.width = tokens, width
        self.cache_hit = path.is_file()
        path.parent.mkdir(parents=True, exist_ok=True)
        if not self.cache_hit:
            # A private temporary database prevents readers observing partial indexes.
            with tempfile.NamedTemporaryFile(dir=path.parent, suffix=".sqlite", delete=False) as f:
                temporary = Path(f.name)
            try:
                with closing(sqlite3.connect(temporary)) as db, db:
                    db.execute("CREATE TABLE seeds (hash INTEGER, position INTEGER)")
                    db.executemany("INSERT INTO seeds VALUES (?, ?)",
                                   ((h, p) for p, h in seeds(tokens, width)
                                    if len(set(tokens[p:p + width])) >= 3))
                    db.execute("CREATE INDEX seed_hash ON seeds(hash)")
                temporary.replace(path)
            finally:
                temporary.unlink(missing_ok=True)
        self.db = sqlite3.connect(path)

    def close(self):
        self.db.close()

    def windows(self, query: list[int], max_occurrences: int, max_seeds: int,
                max_windows: int, slack: int):
        choices = []
        for position, value in seeds(query, self.width):
            if len(set(query[position:position + self.width])) < 3:
                continue
            hits = [p for (p,) in self.db.execute(
                "SELECT position FROM seeds WHERE hash=? LIMIT ?", (value, max_occurrences + 1))]
            if not hits or len(hits) > max_occurrences:
                continue
            hits = [p for p in hits if self.tokens[p:p + self.width] == query[position:position + self.width]]
            if hits:
                choices.append((len(hits), position, hits))
        # Rare seeds first; non-overlapping reference seeds prevent one common run
        # from overwhelming evidence distributed across the reference function.
        selected = []
        for _, position, hits in sorted(choices):
            if any(abs(position - p) < self.width for p, _ in selected):
                continue
            selected.append((position, hits))
            if len(selected) == max_seeds:
                break
        votes = Counter()
        for position, hits in selected:
            for p in hits:
                votes[p - position] += 1
        # Merge shifts caused by small insertion/deletion runs into a single search.
        ranked = sorted(votes, key=lambda p: (-sum(v for q, v in votes.items()
                                                  if abs(q - p) <= slack), p))
        windows = []
        for estimate in ranked:
            start, end = max(0, estimate - slack), min(len(self.tokens), estimate + len(query) + slack)
            if end <= start or any(abs(estimate - w[2]) <= slack for w in windows):
                continue
            windows.append((start, end, estimate, sum(v for q, v in votes.items() if abs(q - estimate) <= slack)))
            if len(windows) == max_windows:
                break
        return windows, {"usable_seeds": len(choices), "selected_seeds": len(selected),
                         "distinct_seed_diagonals": len(votes), "ranked_windows": len(windows)}


def lcs_length(left: list[int], right: list[int]) -> int:
    """Exact bit-parallel longest-common-subsequence length, not substring."""
    masks = {}
    for i, t in enumerate(left):
        masks[t] = masks.get(t, 0) | (1 << i)
    state = 0
    for t in right:
        x = state | masks.get(t, 0)
        state = x & ~(x - ((state << 1) | 1))
    return state.bit_count()


def align(query: list[int], window: list[int], max_cells: int):
    """Exact semiglobal edit alignment; all reference words, free target flanks.

    Costs are one per substitution/insertion/deletion. The returned start/end
    are inferred spans, not function-boundary assertions. Cell cap is checked
    before allocating traceback storage or running the quadratic refinement.
    """
    m, n = len(query), len(window)
    if not m or not n or m * n > max_cells:
        return None
    trace = bytearray((m + 1) * (n + 1))
    previous = [0] * (n + 1)
    for i, a in enumerate(query, 1):
        row, offset = [i] + [0] * n, i * (n + 1)
        trace[offset] = 1
        for j, b in enumerate(window, 1):
            diagonal, deletion, insertion = previous[j - 1] + (a != b), previous[j] + 1, row[j - 1] + 1
            best = min(diagonal, deletion, insertion)
            row[j] = best
            trace[offset + j] = 0 if diagonal == best else (1 if deletion == best else 2)
        previous = row
    end = min(range(1, n + 1), key=lambda j: previous[j])
    distance, i, j, steps = previous[end], m, end, []
    while i:
        direction = trace[i * (n + 1) + j]
        if direction == 0:
            i, j = i - 1, j - 1
            steps.append(("equal" if query[i] == window[j] else "replace", i, j))
        elif direction == 1:
            i -= 1
            steps.append(("delete", i, j))
        else:
            j -= 1
            steps.append(("insert", i, j))
    start, runs = j, []
    for kind, a, b in reversed(steps):
        if not runs or runs[-1]["operation"] != kind:
            runs.append({"operation": kind, "reference_start": a, "target_start": b,
                         "reference_end": a, "target_end": b})
        runs[-1]["reference_end"] = a + (kind != "insert")
        runs[-1]["target_end"] = b + (kind != "delete")
    length = end - start
    counts = Counter(kind for kind, _, _ in steps)
    common = lcs_length(query, window[start:end])
    return {"start": start, "end": end, "edit_distance": distance,
            "edit_similarity_percent": 100 * max(0, 1 - distance / max(m, length)),
            "lcs_instructions": common, "lcs_dice_percent": 200 * common / (m + length),
            "reference_coverage_percent": 100 * counts["equal"] / m,
            "target_coverage_percent": 100 * counts["equal"] / length if length else 0,
            "instructions": {k: counts[k] for k in ("equal", "replace", "insert", "delete")},
            "alignment": runs}


def search(index: SeedIndex, query: list[int], *, top: int = 5, slack: int = 32,
           max_windows: int = 12, max_cells: int = 1000000,
           max_seeds: int = 24, max_occurrences: int = 64):
    windows, diagnostics = index.windows(query, max_occurrences, max_seeds, max_windows, slack)
    candidates, skipped = [], 0
    for begin, end, estimate, votes in windows:
        result = align(query, index.tokens[begin:end], max_cells)
        if result is None:
            skipped += 1
            continue
        result["start"] += begin
        result["end"] += begin
        for run in result["alignment"]:
            run["target_start"] += begin
            run["target_end"] += begin
        result.update(seed_votes=votes, seed_start_estimate=estimate)
        if not any(c["start"] == result["start"] and c["end"] == result["end"] for c in candidates):
            candidates.append(result)
    candidates.sort(key=lambda c: (-c["edit_similarity_percent"], -c["lcs_dice_percent"], c["start"]))
    diagnostics["cell_budget_skipped_windows"] = skipped
    diagnostics["refined_candidates"] = len(candidates)
    diagnostics["best_runner_up_gap_percent"] = (candidates[0]["edit_similarity_percent"] -
        candidates[1]["edit_similarity_percent"] if len(candidates) > 1 else None)
    return candidates[:top], diagnostics


def run(args):
    started = time.perf_counter()
    records = json.loads(args.manifest.read_text(encoding="utf-8"))["versions"]
    reference, ref, ref_hit = original(args.reference, records, args.orig_dir, args.cache_dir)
    target, dst, dst_hit = original(TARGET, records, args.orig_dir, args.cache_dir)
    layout_path = args.config_dir / TARGET / "region-layout.json"
    layout = json.loads(layout_path.read_text(encoding="utf-8"))
    if layout["executable_sha1"] != dst["sha1"]:
        raise ValueError("CPU-region metadata does not describe this original")
    regions = [r for r in layout["regions"] if r["name"] == "cpu_text" and r["classification"] == "code"]
    if len(regions) != 1:
        raise ValueError("Expected one reviewed CPU text region")
    region = regions[0]
    body = read_body(target, dst["metadata"], region["address"], region["size"])
    key = hashlib.sha256((str(CACHE_VERSION) + dst["sha1"] + str(args.seed_words) +
                          json.dumps(region, sort_keys=True)).encode()).hexdigest()
    tokens, token_hit = cached_tokens(body, args.cache_dir / (key + ".tokens"))
    index = SeedIndex(tokens, args.seed_words, args.cache_dir / (key + ".sqlite"))
    ready = time.perf_counter()
    functions = [f for f in ref["functions"] if
                 (not args.name or args.name.lower() in f["name"].lower()) and
                 (not args.source or args.source.lower() in f["source"].lower()) and
                 (args.address is None or f["low"] == args.address)]
    functions.sort(key=lambda f: (f["source"], f["low"], f["name"]))
    output = []
    try:
        for f in functions[:args.limit]:
            size = f["high"] - f["low"]
            record = {"name": f["name"], "source": f["source"],
                      "reference_address": f["low"], "reference_size": size,
                      "boundary_confirmation": False, "eligible_for_progress": False}
            if size < args.seed_words * 4 or size > args.max_words * 4:
                record.update(status="skipped_reference_size", candidates=[])
            else:
                code = read_body(reference, ref["metadata"], f["low"], size)
                candidates, diagnostics = search(index, decode(code), top=args.top, slack=args.slack,
                    max_windows=args.max_windows, max_cells=args.max_cells,
                    max_seeds=args.max_seeds, max_occurrences=args.max_occurrences)
                for c in candidates:
                    start, end = c.pop("start"), c.pop("end")
                    c["candidate_address"] = region["address"] + start * 4
                    c["candidate_size"] = (end - start) * 4
                    c["candidate_body_sha256"] = hashlib.sha256(body[start * 4:end * 4]).hexdigest()
                    c["raw_identical_body"] = code == body[start * 4:end * 4]
                    c["boundary_confirmation"] = False
                    c["seed_start_estimate"] = region["address"] + c["seed_start_estimate"] * 4
                    for run in c["alignment"]:
                        for k in ("reference_start", "reference_end"):
                            run[k] = f["low"] + run[k] * 4
                        for k in ("target_start", "target_end"):
                            run[k] = region["address"] + run[k] * 4
                record.update(status="ranked" if candidates else "no_candidate_within_search_budgets",
                              candidates=candidates, search=diagnostics,
                              reference_body_sha256=hashlib.sha256(code).hexdigest())
            output.append(record)
    finally:
        index.close()
    return {"schema_version": 1, "status": "unconfirmed-fuzzy-candidates",
            "eligible_for_progress": False, "boundary_confirmation": False,
            "reference_version": args.reference, "reference_sha1": ref["sha1"],
            "target_version": TARGET, "target_sha1": dst["sha1"], "region": region,
            "method": {"normalization_version": CACHE_VERSION, "seed_words": args.seed_words,
                       "alignment": "semiglobal unit-cost edit distance; exact bit-parallel LCS",
                       "special_gpr_roles_preserved": ["zero", "gp", "sp", "ra"],
                       "limits": {k: getattr(args, k) for k in ("limit", "max_words", "max_windows",
                                  "max_cells", "max_seeds", "max_occurrences", "slack", "top")}},
            "limitations": ["Candidates are rankings, not recovered function identities or boundaries.",
                "Normalization intentionally ignores register/address differences and is not a relocation proof.",
                "Scores compare normalized instructions; they are not objdiff matching-progress scores.",
                "Unknown reference members, duplicate functions and seed/window limits may hide a better match.",
                "A runner-up gap is local to searched windows, not confidence in a recovered identity.",
                "Independent original-only ownership, entry and boundary evidence is required before promotion."],
            "cache": {"reference_metadata_hit": ref_hit, "target_metadata_hit": dst_hit,
                      "target_tokens_hit": token_hit,
                      "target_seed_index_hit": index.cache_hit},
            "timing_seconds": {"prepare": ready - started, "search": time.perf_counter() - ready,
                               "total": time.perf_counter() - started},
            "selected_reference_functions": len(functions),
            "omitted_by_limit": max(0, len(functions) - args.limit), "functions": output}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--manifest", type=Path, default=ROOT / "config/platforms/versions.json")
    p.add_argument("--orig-dir", type=Path, default=ROOT / "orig")
    p.add_argument("--config-dir", type=Path, default=ROOT / "config/platforms")
    p.add_argument("--cache-dir", type=Path, default=ROOT / "build/france-fuzzy-cache")
    p.add_argument("--output", type=Path, default=ROOT / "build/france-fuzzy-candidates.json")
    p.add_argument("--reference", choices=REFERENCES, default="SLUS-20680")
    p.add_argument("--name", default="", help="case-insensitive DWARF function-name substring")
    p.add_argument("--source", default="", help="case-insensitive original source-path substring")
    p.add_argument("--address", type=lambda s: int(s, 0), help="exact reference entry address")
    for name, default in (("limit", 20), ("top", 5), ("seed-words", 5), ("max-words", 4096),
                          ("max-windows", 12), ("max-cells", 1000000), ("max-seeds", 24),
                          ("max-occurrences", 64), ("slack", 32)):
        p.add_argument("--" + name, type=int, default=default)
    args = p.parse_args()
    if any(getattr(args, k) <= 0 for k in ("limit", "top", "seed_words", "max_words", "max_windows",
                                          "max_cells", "max_seeds", "max_occurrences", "slack")):
        p.error("Search budgets must be positive")
    if args.seed_words < 3:
        p.error("--seed-words must be at least 3")
    # Keep private executable-derived caches and unconfirmed candidates out of
    # committed registries, even when the caller supplies an explicit output.
    protected = [(ROOT / "config").resolve(), args.config_dir.resolve(), args.orig_dir.resolve()]
    if any(path.resolve().is_relative_to(parent) for path in (args.output, args.cache_dir) for parent in protected):
        p.error("Outputs and caches must be outside original/configuration directories")
    try:
        result = run(args)
        atomic_json(args.output, result)
        print(f"Ranked {len(result['functions'])} references in {result['timing_seconds']['total']:.2f}s; "
              f"diagnostic candidates: {args.output}")
    except (OSError, ValueError, KeyError, sqlite3.DatabaseError) as error:
        p.exit(1, f"error: {error}\n")


if __name__ == "__main__":
    main()
