"""Original-backed occupied ranges and explicitly unconfirmed search overlays.

This module reads the project's established boundary registries. It authenticates
their executable identity and complete body hashes; it does not promote search
results or replace the independent proofs maintained by those registries.
"""
from __future__ import annotations

from bisect import bisect_right
from collections import Counter
import hashlib
import json
from pathlib import Path


REGISTRIES = {
    "reviewed-functions.json": None,
    "corroborated-functions.json": "machine-corroborated-static-cfg",
    "relocation-corroborated-functions.json": "machine-corroborated-explicit-transfers",
    "tu-corroborated-functions.json": "reviewed-whole-tu-sequence",
}


class OccupiedRanges:
    """Union of half-open intervals, in the caller's address or token units."""

    def __init__(self, ranges=()):
        self.ranges = []
        for start, end in sorted(ranges):
            if start < 0 or end <= start:
                raise ValueError("Invalid occupied range")
            if self.ranges and start <= self.ranges[-1][1]:
                self.ranges[-1] = (self.ranges[-1][0], max(end, self.ranges[-1][1]))
            else:
                self.ranges.append((start, end))
        self.starts = [start for start, _ in self.ranges]

    def overlaps(self, start, end):
        if end <= start:
            return False
        i = bisect_right(self.starts, end - 1) - 1
        return i >= 0 and self.ranges[i][1] > start

    def available(self, start, end):
        """Clip a search window to pieces that never cross an occupied boundary."""
        cursor = start
        i = max(0, bisect_right(self.starts, start) - 1)
        for low, high in self.ranges[i:]:
            if low >= end:
                break
            if high <= cursor:
                continue
            if cursor < low:
                yield cursor, low
            cursor = max(cursor, high)
        if cursor < end:
            yield cursor, end


def verified_functions(config: Path, executable_sha1: str, region: dict,
                       region_body: bytes) -> tuple[list[dict], list[dict]]:
    """Only recognized, boundary-confirmed registries can exclude search space.

    Address-only anchors and fuzzy candidate files intentionally are not read.
    Contradictory boundaries fail closed, even when both hashes happen to match.
    """
    found, provenance = {}, []
    for filename, kind in REGISTRIES.items():
        path = config / filename
        if not path.is_file():
            continue
        raw = path.read_bytes()
        document = json.loads(raw)
        if document.get("executable_sha1") != executable_sha1:
            raise ValueError(f"{filename}: occupied-range executable identity differs")
        provenance.append({"registry": filename, "sha256": hashlib.sha256(raw).hexdigest()})
        for entry in document["functions"]:
            start, size = entry["address"], entry["size"]
            if (entry.get("boundary_confirmation") is not True or size <= 0 or
                    start % 4 or size % 4 or
                    (kind is not None and entry.get("confirmation_kind") != kind)):
                raise ValueError(f"{filename}: missing independent boundary confirmation")
            offset = start - region["address"]
            if offset < 0 or offset + size > len(region_body):
                raise ValueError(f"{filename}: occupied function exceeds CPU text")
            if hashlib.sha256(region_body[offset:offset + size]).hexdigest() != entry["sha256"]:
                raise ValueError(f"{filename}: occupied function body hash differs")
            item = {"address": start, "size": size, "name": entry["name"],
                    "source": entry["source"], "sha256": entry["sha256"],
                    "boundary_confirmation": True, "registries": [filename],
                    "reference_entries": [{k: ref[k] for k in
                        ("version", "executable_sha1", "source_address", "source", "name")}
                        for ref in entry.get("provenance", []) if "source_address" in ref]}
            old = found.get(start)
            if old:
                if any(old[k] != item[k] for k in ("size", "name", "source", "sha256")):
                    raise ValueError("Conflicting occupied function identities")
                old["registries"].append(filename)
                for ref in item["reference_entries"]:
                    if ref not in old["reference_entries"]:
                        old["reference_entries"].append(ref)
            else:
                found[start] = item
    ordered = sorted(found.values(), key=lambda f: f["address"])
    if any(a["address"] + a["size"] > b["address"] for a, b in zip(ordered, ordered[1:])):
        raise ValueError("Conflicting occupied function boundaries")
    return ordered, provenance


def reference_queries(functions: list[dict], verified: list[dict], *, blocks=False,
                      include_verified=False, max_gap=16, reference_version=None,
                      reference_sha1=None) -> tuple[list[dict], list[dict]]:
    """Build functions plus physically contiguous TU blocks, never source unions.

    Versioned reference provenance identifies overloads and aliases by original
    entry address. Unversioned callers can use unique source/name identities.
    Known functions are block barriers, so unknown blocks do not swallow them.
    """
    identity = lambda f: (f["source"], f["name"])
    counts = Counter(identity(f) for f in functions)
    known = {identity(f) for f in verified}
    known_entries = {(ref["source"], ref["name"], ref["source_address"])
                     for f in verified for ref in f.get("reference_entries", [])
                     if ref["version"] == reference_version and ref["executable_sha1"] == reference_sha1}
    queries, group, skipped = [], [], []

    def flush():
        if blocks and len(group) > 1:
            queries.append({"name": f"{group[0]['source']} [{len(group)} contiguous functions]",
                            "source": group[0]["source"], "low": group[0]["low"],
                            "high": group[-1]["high"], "reference_kind": "unit_block",
                            "members": [{k: f[k] for k in ("name", "low", "high")} for f in group]})
        group.clear()

    for f in sorted(functions, key=lambda f: (f["low"], f["high"], f["source"], f["name"])):
        confirmed = ((f["source"], f["name"], f["low"]) in known_entries if reference_version else
                     identity(f) in known and counts[identity(f)] == 1)
        if not include_verified and confirmed:
            flush()
            skipped.append(f)
            continue
        queries.append(dict(f, reference_kind="function"))
        if group and (f["source"] != group[-1]["source"] or
                      not 0 <= f["low"] - group[-1]["high"] <= max_gap):
            flush()
        group.append(f)
    flush()
    return queries, skipped


def soft_hypotheses(paths: list[Path], executable_sha1: str, region: dict) -> list[dict]:
    """Import diagnostic overlays without ever treating them as occupied ranges."""
    output = []
    for path in paths:
        document = json.loads(path.read_text(encoding="utf-8"))
        if document.get("target_sha1") != executable_sha1:
            raise ValueError("Hypothesis map identifies another executable")
        hypotheses = document.get("hypotheses")
        if hypotheses is None:
            hypotheses = [{"name": f["name"], "source": f["source"],
                           "reference_size": f["reference_size"],
                           "reference_kind": f.get("reference_kind", "function"),
                           "candidate_rank": rank, **candidate}
                          for f in document.get("functions", [])
                          for rank, candidate in enumerate(f["candidates"], 1)]
        for value in hypotheses:
            start, size = value["candidate_address"], value["candidate_size"]
            if (size <= 0 or start % 4 or size % 4 or start < region["address"] or
                    start + size > region["address"] + region["size"]):
                raise ValueError("Hypothesis span exceeds the CPU region")
            output.append({**value, "imported_from": str(path),
                           "boundary_confirmation": False, "eligible_for_progress": False})
    return output


def apply_soft_penalties(candidates: list[dict], reference: dict, hypotheses: list[dict],
                         penalty: float) -> None:
    """Flag incompatible larger hypotheses; never remove a candidate or range.

    Only each earlier query's best candidate influences later searches. A TU
    hypothesis remains compatible with members from its own source. Equal-size
    alternatives do not suppress one another. The original LCS score is retained
    alongside the transparent ranking penalty and conflicting anchor identities.
    """
    for candidate in candidates:
        low = candidate["candidate_address"]
        high = low + candidate["candidate_size"]
        conflicts = []
        for anchor in hypotheses:
            if (anchor.get("candidate_rank", 1) != 1 or
                    anchor["reference_size"] <= reference["reference_size"]):
                continue
            same_source = anchor["source"] == reference["source"]
            if same_source and (anchor.get("reference_kind") == "unit_block" or
                                anchor["name"] == reference["name"]):
                continue
            overlap = max(0, min(high, anchor["candidate_address"] + anchor["candidate_size"]) -
                          max(low, anchor["candidate_address"]))
            if overlap * 2 < candidate["candidate_size"]:
                continue
            conflicts.append({"name": anchor["name"], "source": anchor["source"],
                              "candidate_address": anchor["candidate_address"],
                              "candidate_size": anchor["candidate_size"], "overlap_bytes": overlap,
                              "boundary_confirmation": False})
        candidate["soft_conflicts"] = conflicts
        candidate["soft_overlap_penalty_percent"] = penalty if conflicts else 0
        candidate["ranking_score_percent"] = max(0, candidate["lcs_dice_percent"] -
                                                  candidate["soft_overlap_penalty_percent"])
    candidates.sort(key=lambda c: (-c["ranking_score_percent"], -c["lcs_dice_percent"],
                                  -(c["edit_similarity_percent"] or 0), c["candidate_address"]))


def region_map(executable_sha1: str, region: dict, verified: list[dict], provenance: list[dict],
               records: list[dict], imported=()) -> dict:
    """Reusable original occupancy plus soft overlays, ranked by remaining size."""
    occupied = OccupiedRanges((f["address"], f["address"] + f["size"]) for f in verified)
    hypotheses = list(imported)
    for record in records:
        for rank, candidate in enumerate(record["candidates"], 1):
            hypotheses.append({"name": record["name"], "source": record["source"],
                               "reference_address": record["reference_address"],
                               "reference_size": record["reference_size"],
                               "reference_kind": record["reference_kind"], "candidate_rank": rank,
                               **{k: v for k, v in candidate.items() if k != "alignment"},
                               "boundary_confirmation": False, "eligible_for_progress": False})
    for value in hypotheses:
        start, end = value["candidate_address"], value["candidate_address"] + value["candidate_size"]
        value["overlaps_verified"] = occupied.overlaps(start, end)
    gaps = []
    for start, end in occupied.available(region["address"], region["address"] + region["size"]):
        related = [h for h in hypotheses if h["candidate_address"] < end and
                   h["candidate_address"] + h["candidate_size"] > start]
        gaps.append({"address": start, "size": end - start, "status": "not_independently_owned",
                     "soft_candidate_count": len(related),
                     "largest_candidate_reference_size": max((h["reference_size"] for h in related), default=0),
                     "candidate_sources": sorted({h["source"] for h in related})})
    return {"schema_version": 1, "status": "verified-occupancy-with-unconfirmed-overlays",
            "target_sha1": executable_sha1, "eligible_for_progress": False, "region": region,
            "verified_registries": provenance, "verified_functions": verified,
            "verified_code_bytes": sum(end - start for start, end in occupied.ranges),
            "unclaimed_regions_largest_first": sorted(gaps, key=lambda g: (-g["size"], g["address"])),
            "hypotheses": hypotheses,
            "limitations": ["Only independently boundary-confirmed original registries occupy ranges.",
                            "Unclaimed ranges can contain padding; they are not function extents.",
                            "Hypotheses never exclude another hypothesis or become verified anchors."]}
