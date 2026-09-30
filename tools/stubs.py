"""Find STUB functions: a symbol exists on both sides, but ours is a bare `blr`.

This is a distinct bucket from MISSING (no symbol at all) and from
near-100% (real code, wrong details). A stub is an empty `{ }` body that
somebody left as a placeholder, so the target disassembly is fully readable
and the whole function is there to be written. zNPCFXCinematic alone has
about 25 of them.

Heuristic: our size <= 12 bytes (mflr/blr epilogue at most) while the
target's is >= 40. Reports per-unit totals so a run can be aimed at one file.

Usage: stubs.py [min-target-bytes]
"""
import json
import os
import subprocess
import sys
import tempfile

import cwexec

ROOT = os.getcwd()
CLI = os.path.abspath(cwexec.objdiff_cli(ROOT))
CFG = json.load(open("objdiff.json"))
TMP = os.path.join(tempfile.gettempdir(), "stubs.json")
MIN = int(sys.argv[1]) if len(sys.argv) > 1 else 40

rows = []
for u in CFG["units"]:
    md = u.get("metadata") or {}
    sp = (md.get("source_path") or "").replace("\\", "/")
    if not sp.startswith("src/SB/"):
        continue
    t = u.get("target_path", "").replace("/", os.sep)
    b = u.get("base_path", "").replace("/", os.sep)
    if not (os.path.exists(t) and os.path.exists(b)):
        continue
    if subprocess.run([CLI, "diff", "-1", t, "-2", b, "-o", TMP,
                       "--format", "json"], capture_output=True).returncode:
        continue
    try:
        d = json.load(open(TMP))
    except Exception:
        continue
    L = {s["name"]: int(s.get("size") or 0) for s in d["left"].get("symbols", [])
         if s.get("kind") == "SYMBOL_FUNCTION"}
    R = {s["name"]: int(s.get("size") or 0) for s in d["right"].get("symbols", [])
         if s.get("kind") == "SYMBOL_FUNCTION"}
    stub = [(L[n], n) for n in L if n in R and R[n] <= 12 and L[n] >= MIN]
    if stub:
        rows.append((len(stub), sum(s[0] for s in stub), u["name"],
                     sorted(stub, reverse=True)))

rows.sort(reverse=True)
tot = totb = 0
for n, nb, name, stub in rows:
    tot += n
    totb += nb
    print("%-42s %3d stubs, %6d bytes" % (name, n, nb))
    for size, s in stub[:6]:
        print("      %5db  %s" % (size, s))
    if len(stub) > 6:
        print("      ... and %d more" % (len(stub) - 6))
print("\nTOTAL %d stub functions, %d bytes, across %d units"
      % (tot, totb, len(rows)))
