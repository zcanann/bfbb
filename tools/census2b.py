#!/usr/bin/env python3
"""2b census v4 -- split by operand kind, with an honest confidence tier each.

History (do not repeat): v1 keyed on objdiff row alignment, 2/9 recall. v2 keyed
on raw operand text, 0/9 -- anonymous pool ids and registers differ between the
two objects. v3 normalised both and hit 9/9 recall but flagged 43% of all
non-matching functions, because collapsing @1171 -> @A merges every distinct
float literal in a function into one key.

v4 keeps 9/9 recall and separates what can actually be counted:

  NAMED   surplus loads of a named global (gActiveHeap, active_ribbons_size,
          Im3DBufferPos, sAuraPulseAng, g_cnt_activehaz). The symbol name is
          identical in both objects, so the comparison is exact. HIGH
          confidence -- this is the countable population.
  MEMBER  surplus loads of a struct member, 0xN(reg). Registers are normalised
          so two different objects at the same displacement can collide.
          MEDIUM confidence.
  ANON    surplus loads of anonymous pool literals. Ids differ across objects
          (@1171 vs @531), so per-literal attribution is impossible without
          resolving pool contents to values. Only the aggregate per function is
          meaningful. LOW confidence -- reported as an upper bound.

Only NAMED should be quoted as the size of the patch target.
"""
import json
import os
import re
import subprocess
from collections import Counter

import cwexec

ROOT = r"C:\Users\joeyj\Documents\Git\bfbb_"
CLI = cwexec.objdiff_cli(ROOT)
SP = r"C:\Users\joeyj\AppData\Local\Temp\claude\C--Users-joeyj-Documents-Git-bfbb-\458c7547-ca83-49e5-8c1d-e42d1a0ce65d\scratchpad"

LOADS = {"lwz", "lwzu", "lfs", "lfsu", "lfd", "lfdu", "lha", "lhau", "lbz",
         "lbzu", "lhz", "lhzu", "lwzx", "lfsx", "lfdx", "lbzx", "lhzx", "lhax",
         "lmw"}
BARRIER = {"stw", "stwu", "stfs", "stfsu", "stfd", "stfdu", "stb", "stbu",
           "sth", "sthu", "stwx", "stfsx", "stfdx", "stbx", "sthx", "stmw",
           "bl", "blrl", "bctrl"}

RE_ANON = re.compile(r"^@\d+")
RE_REG = re.compile(r"\br\d+\b")
RE_FREG = re.compile(r"\bf\d+\b")


def mnem(ins):
    for p in ins.get("parts", []):
        if "opcode" in p:
            return p["opcode"].get("mnemonic")
    return (ins.get("formatted", "").split() or [None])[0]


def classify(ins, symtab=None):
    """(kind, key) for a load. kind in NAMED / ANON / MEMBER / OTHER.

    `relocation.target_symbol` is an INDEX into that side's symbol list, not a
    name, and the indices differ between the two objects -- resolving it is what
    makes the two sides comparable at all.
    """
    rel = ins.get("relocation")
    s = ins.get("formatted", "")
    operand = s.split(",", 1)[-1].strip() if "," in s else s
    if rel:
        ts = rel.get("target_symbol")
        sym = ""
        if isinstance(ts, int) and symtab is not None and 0 <= ts < len(symtab):
            sym = symtab[ts].get("name") or ""
        elif isinstance(ts, dict):
            sym = ts.get("name") or ""
        elif isinstance(ts, str):
            sym = ts
        sym = str(sym)
        if not sym:
            return "OTHER", "%s|?" % mnem(ins)
        if RE_ANON.match(sym) or re.match(r"^[@$]", sym):
            return "ANON", "%s|anon" % mnem(ins)
        # `name$1234` is a function-local static whose compiler-assigned id
        # differs between the two objects; without stripping it every such
        # symbol reads as a phantom surplus.
        sym = re.sub(r"\$\d+$", "$N", sym)
        return "NAMED", "%s|%s" % (mnem(ins), sym)
    if "(" in operand:
        disp = operand.split("(")[0]
        return "MEMBER", "%s|%s(R)" % (mnem(ins), disp)
    return "OTHER", "%s|%s" % (mnem(ins), RE_FREG.sub("F", RE_REG.sub("R", operand)))


def stream(sym):
    return [e["instruction"] for e in (sym.get("instructions") or [])
            if e.get("instruction")]


def repeated_across_barrier(li, k, symtab=None):
    idxs = [i for i, x in enumerate(li)
            if mnem(x) in LOADS and classify(x, symtab)[1] == k]
    if len(idxs) < 2:
        return False
    for a, b in zip(idxs, idxs[1:]):
        if any(mnem(li[j]) in BARRIER for j in range(a + 1, b)):
            return True
    return False


def units():
    cfg = json.load(open(os.path.join(ROOT, "objdiff.json")))
    res = []
    for u in cfg.get("units", []):
        t, b = u.get("target_path"), u.get("base_path")
        if not t or not b:
            continue
        tp, bp = os.path.join(ROOT, t), os.path.join(ROOT, b)
        if os.path.exists(tp) and os.path.exists(bp):
            res.append((u.get("name") or t, tp, bp))
    return res


def analyse(name, tp, bp, outjson):
    if os.path.exists(outjson):
        os.unlink(outjson)
    subprocess.run([CLI, "diff", "-1", tp, "-2", bp, "-o", outjson,
                    "--format", "json"], cwd=ROOT, capture_output=True)
    if not os.path.exists(outjson):
        return []
    try:
        d = json.load(open(outjson))
    except Exception:
        return []
    L = {s["name"]: s for s in (d.get("left") or {}).get("symbols") or []
         if s.get("kind") == "SYMBOL_FUNCTION"}
    R = {s["name"]: s for s in (d.get("right") or {}).get("symbols") or []
         if s.get("kind") == "SYMBOL_FUNCTION"}
    out = []
    for fn, ls in L.items():
        pct = ls.get("match_percent")
        if pct is None or pct == 100.0:
            continue
        rs = R.get(fn)
        if not rs:
            continue
        li, ri = stream(ls), stream(rs)
        if not li or not ri:
            continue
        lsyms = (d.get("left") or {}).get("symbols") or []
        rsyms = (d.get("right") or {}).get("symbols") or []
        lk = Counter(classify(x, lsyms) for x in li if mnem(x) in LOADS)
        rk = Counter(classify(x, rsyms) for x in ri if mnem(x) in LOADS)
        surplus = lk - rk
        deficit = rk - lk
        # If WE load a named symbol the target does not, this is a
        # different-symbol reference (a source bug: e.g. ThunderCloud uses
        # g_strz_roboanim where the target uses g_strz_cloudanim), not the
        # target reloading something we cached. Reject, and record separately.
        wrong_sym = [k for (kind, k), c in deficit.items() if kind == "NAMED"]
        rec = {"unit": name, "function": fn, "match_percent": pct,
               "wrong_symbol": wrong_sym,
               "size": int(ls.get("size") or 0),
               "insn_delta": len(li) - len(ri),
               "NAMED": [], "MEMBER": [], "ANON": 0}
        for (kind, k), c in surplus.items():
            if kind == "ANON":
                rec["ANON"] += c
                continue
            if kind not in ("NAMED", "MEMBER"):
                continue
            if not repeated_across_barrier(li, k, lsyms):
                continue
            rec[kind].append({"key": k, "surplus": c})
        if wrong_sym:
            rec["NAMED"] = []
        if rec["NAMED"] or rec["MEMBER"] or rec["ANON"] or wrong_sym:
            out.append(rec)
    return out


def main():
    us = units()
    print("units with both objects: %d" % len(us), flush=True)
    tmp = os.path.join(SP, "cen_tmp4.json")
    hits = []
    for i, (name, tp, bp) in enumerate(us):
        hits += analyse(name, tp, bp, tmp)
        if (i + 1) % 100 == 0:
            print("  %d/%d, %d rows" % (i + 1, len(us), len(hits)), flush=True)
    json.dump(hits, open(os.path.join(SP, "cen_2b_v7.json"), "w"), indent=1)
    named = [h for h in hits if h["NAMED"]]
    member = [h for h in hits if h["MEMBER"] and not h["NAMED"]]
    anon = [h for h in hits if h["ANON"] and not h["NAMED"] and not h["MEMBER"]]
    print("NAMED  (high confidence): %d functions" % len(named))
    print("MEMBER (medium, no NAMED): %d functions" % len(member))
    print("ANON   (low, neither):     %d functions" % len(anon))


if __name__ == "__main__":
    main()
