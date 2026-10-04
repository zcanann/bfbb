#!/usr/bin/env python3
"""Rank non-matching game functions by how far their locals stray from DWARF.

For every non-matching function in the SB units this compares the locals the
PS2 DWARF dump (dwarf/SB/**/<unit>.cpp) lists for it against the declarations
in our function body, and reports:

  D  DWARF-only locals   (strong: the original had a variable we don't)
  S  source-only locals  (we invented a helper the original didn't have)
  R  rename candidates   (a D and an S of the same normalised type)
  T  retyped             (same name, different normalised type)

The PS2 (MIPS) compiler sometimes drops locals it scalarised, so "DWARF lacks
X" is weaker evidence than "DWARF has Y we lack"; the score weights D twice S.
`_col` locals are PS2 RenderWare-macro artifacts and are ignored.

  score = 2*D + 1*S + 1*R + 0.5*T
  rank  = score * unmatched_bytes   (unmatched = size * (100 - pct) / 100)

Functions with no DWARF entry (GC-only code) score 0 and sort last.

Match percentages come from tools/solo.py run over every SB unit (cached in
--cache, default build/dwarflocals_cache.json; --measure refreshes it), with
the SB compiler named in configure.py (or --mw), since build.ninja may be stale.

Usage:
  dwarflocals.py [--measure] [--mw GC/x] [--cache F] [--top N] [--all] [--unit FRAG]
"""
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def opt(flag, default=None):
    if flag in sys.argv:
        i = sys.argv.index(flag)
        if i + 1 < len(sys.argv):
            return sys.argv[i + 1]
    return default


CACHE = opt("--cache", os.path.join(ROOT, "build", "dwarflocals_cache.json"))
LINE = re.compile(r"^\s+([\d.]+)%\s+(\d+)b\s+(\S+)$")


# --------------------------------------------------------------- measurement

def game_compiler():
    """configure.py's SB compiler; build.ninja can be older than it."""
    if opt("--mw"):
        return opt("--mw")
    m = re.search(r'^RW_COMPILER = "([^"]+)"', open(os.path.join(ROOT, "configure.py")).read(), re.M)
    return m.group(1) if m else None


def measure():
    mw = game_compiler()
    cfg = json.load(open(os.path.join(ROOT, "objdiff.json")))
    units = [u["name"] for u in cfg["units"] if "/SB/" in u["name"]]

    def run(name):
        r = subprocess.run([sys.executable, os.path.join("tools", "solo.py"), name] +
                           (["--mw", mw] if mw else []),
                           cwd=ROOT, capture_output=True, text=True)
        fns = []
        for l in r.stdout.splitlines():
            m = LINE.match(l)
            if m:
                fns.append((m.group(3), float(m.group(1)), int(m.group(2))))
        return name, fns

    out = {}
    with ThreadPoolExecutor(6) as ex:
        for name, fns in ex.map(run, units):
            out[name] = fns
    json.dump(out, open(CACHE, "w"), indent=0)
    return out


# ------------------------------------------------------------- name helpers

def demangle(sym):
    """CW mangled name -> (class or None, function name). Rough but enough."""
    i = sym.find("__", 1)
    while i != -1 and not re.match(r"__(?:[0-9]|Q[0-9]|F|C)", sym[i:]):
        i = sym.find("__", i + 1)
    if i == -1:
        return None, sym
    name, rest = sym[:i], sym[i + 2:]
    classes = []
    if rest[:1] == "Q":
        n, rest = int(rest[1]), rest[2:]
        for _ in range(n):
            m = re.match(r"(\d+)", rest)
            ln = int(m.group(1))
            classes.append(rest[len(m.group(1)):len(m.group(1)) + ln])
            rest = rest[len(m.group(1)) + ln:]
    elif rest[:1].isdigit():
        m = re.match(r"(\d+)", rest)
        ln = int(m.group(1))
        classes.append(rest[len(m.group(1)):len(m.group(1)) + ln])
    cls = classes[-1] if classes else None
    if cls and cls.startswith("@"):
        cls = None  # anonymous namespace
    if name == "__ct" and cls:
        name = cls
    elif name == "__dt" and cls:
        name = "~" + cls
    return cls, name


INTS = {
    "signed int": "S32", "int": "S32", "signed long": "S32", "long": "S32", "S32": "S32",
    "unsigned int": "U32", "unsigned long": "U32", "unsigned": "U32", "U32": "U32",
    "signed short": "S16", "short": "S16", "S16": "S16",
    "unsigned short": "U16", "U16": "U16",
    "signed char": "S8", "S8": "S8",
    "unsigned char": "U8", "U8": "U8", "char": "char",
    "float": "F32", "F32": "F32", "double": "F64", "F64": "F64",
    "bool": "bool", "void": "void",
    "long long": "S64", "signed long long": "S64", "S64": "S64",
    "unsigned long long": "U64", "U64": "U64",
}


# GC typedef pairs that are the same type under another name.
ALIASES = {"RwIm3DVertex": "RxObjSpace3DVertex", "RwMatrixTag": "RwMatrix",
           "RwUInt16": "U16", "RwUInt32": "U32", "RwInt32": "S32", "RwReal": "F32",
           "s32": "S32", "u32": "U32", "u16": "U16", "s16": "S16", "u8": "U8",
           "size_t": "U32", "BOOL": "S32"}


def normtype(t):
    t = re.sub(r"\b(const|volatile|class|struct|enum|union|static|register)\b", " ", t)
    stars = t.count("*") + t.count("&")
    arr = "[]" if "[" in t else ""
    t = re.sub(r"[*&]|\[[^\]]*\]", " ", t)
    t = " ".join(t.split())
    t = INTS.get(t, t.split("::")[-1])
    t = ALIASES.get(t, t)
    return t + "*" * stars + arr


# --------------------------------------------------------------------- DWARF

DW_FUNC = re.compile(r"^(?P<sig>[A-Za-z_~].*?)\((?P<params>.*)\)\s*(?:const\s*)?\{\s*$", re.M)
DW_LOCAL = re.compile(r"^\s+(?P<decl>[^/{}()]*?[ *&](?P<name>[A-Za-z_][A-Za-z_0-9]*)"
                      r"(?P<arr>\[[^\]]*\])*);\s*//\s*(?P<loc>.*)$")


def dwarf_functions(path):
    """list of (class, name, params, [(name, type, static?)])"""
    txt = open(path, encoding="utf-8", errors="replace").read()
    out = []
    for m in DW_FUNC.finditer(txt):
        sig = m.group("sig").strip()
        nm = sig.split()[-1].lstrip("*&") if sig.split() else ""
        if nm in ("if", "for", "while", "switch", "return", "else", ""):
            continue
        cls = None
        params = []
        for p in m.group("params").split(","):
            p = re.sub(r"/\*.*?\*/", "", p).strip()
            if not p:
                continue
            pm = re.search(r"([A-Za-z_][A-Za-z_0-9]*)\s*$", p)
            if pm and pm.group(1) == "this":
                cm = re.search(r"([A-Za-z_][A-Za-z_0-9]*)\s*\*\s*this", p)
                cls = cm.group(1) if cm else None
            elif pm:
                params.append(pm.group(1))
        end = txt.find("\n}", m.end())
        body = txt[m.end():end]
        locs = []
        for l in body.splitlines():
            lm = DW_LOCAL.match(l)
            if not lm:
                continue
            decl = lm.group("decl")
            name = lm.group("name")
            typ = decl[:decl.rfind(name)] + (lm.group("arr") or "")
            locs.append((name, normtype(typ), "@" in lm.group("loc")))
        out.append((cls, nm, params, locs))
    return out


# -------------------------------------------------------------------- source

KEYWORDS = {"return", "delete", "goto", "else", "case", "default", "new", "throw",
            "if", "while", "for", "switch", "do", "sizeof", "break", "continue",
            "typedef", "using", "operator", "this"}
DECL = re.compile(
    r"^(?P<type>(?:(?:const|static|volatile|register|struct|class|enum|union|unsigned|signed)\s+)*"
    r"(?P<base>[A-Za-z_][A-Za-z_0-9]*(?:\s*::\s*[A-Za-z_][A-Za-z_0-9]*)*(?:\s*<[^;{}()]*?>)?)"
    r"(?:\s+(?:int|long|short|char|const))*)"
    r"(?P<rest>(?:\s*[*&]\s*|\s+)(?:const\s+)?[*&\s]*[A-Za-z_][A-Za-z_0-9]*\s*(?:\[[^\]]*\]\s*)*(?:[=,(;]|$).*)$",
    re.S)


def strip_code(s):
    s = re.sub(r"//[^\n]*", "", s)
    s = re.sub(r"/\*.*?\*/", "", s, flags=re.S)
    s = re.sub(r'"(?:\\.|[^"\\])*"', '""', s)
    s = re.sub(r"'(?:\\.|[^'\\])*'", "0", s)
    s = re.sub(r"^\s*#.*$", "", s, flags=re.M)
    return s


def split_top(s, sep=","):
    parts, depth, cur = [], 0, ""
    for ch in s:
        if ch in "([{<":
            depth += 1
        elif ch in ")]}>":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    parts.append(cur)
    return parts


def body_locals(body):
    out = []
    # statement chunks between ; { }
    for st in re.split(r"[;{}]", body):
        st = st.strip()
        st = re.sub(r"^(?:for\s*\(|(?:case\s+[^:]+|default|[A-Za-z_]\w*)\s*:(?!:))\s*", "", st)
        st = re.sub(r"^(?:else|do)\s+", "", st)
        m = DECL.match(st)
        if not m:
            continue
        base = m.group("base")
        if base.split("::")[0].strip() in KEYWORDS:
            continue
        static = "static" in m.group("type").split()
        for d in split_top(m.group("rest")):
            d = d.split("=")[0]
            dm = re.match(r"\s*((?:const\s+)?[*&\s]*)([A-Za-z_][A-Za-z_0-9]*)\s*((?:\[[^\]]*\]\s*)*)", d)
            if not dm or dm.group(2) in KEYWORDS:
                break
            # `T name(args)` is a ctor-style decl; `f(x)` alone would not match DECL
            out.append((dm.group(2), normtype(m.group("type") + dm.group(1) + dm.group(3)),
                        static))
    return out


def find_source_fn(src, cls, name):
    """Return (params text, body) for the definition of cls::name, or None."""
    qual = (r"(?:[A-Za-z_]\w*::)*" + re.escape(cls) + r"\s*::\s*") if cls else r"(?<![\w:~])"
    pat = re.compile(qual + re.escape(name) + r"\s*\(", re.M)
    hits = []
    for m in pat.finditer(src):
        # params: match parens
        i, depth = m.end() - 1, 0
        while i < len(src):
            if src[i] == "(":
                depth += 1
            elif src[i] == ")":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        j = i + 1
        tail = src[j:j + 200]
        tm = re.match(r"\s*(?:const\s*)?(?::[^{;]*)?\{", tail, re.S)
        if not tm:
            continue
        # must be at top level: line start of match not deeply nested
        ls = src.rfind("\n", 0, m.start()) + 1
        if src[ls:m.start()].strip().endswith(("=", "return", ".", "->")):
            continue
        k = j + tm.end() - 1
        depth, e = 0, k
        while e < len(src):
            if src[e] == "{":
                depth += 1
            elif src[e] == "}":
                depth -= 1
                if depth == 0:
                    break
            e += 1
        hits.append((src[m.end():i], src[k + 1:e]))
    return hits


# ----------------------------------------------------------------- analysis

def analyse(dlocs, slocs, params):
    pset = set(params)
    dl = {}
    for n, t, st in dlocs:
        if n == "_col":
            continue
        dl.setdefault(n, (t, st))
    sl = {}
    for n, t, st in slocs:
        sl.setdefault(n, (t, st))
    donly = [n for n in dl if n not in sl and n not in pset]
    sonly = [n for n in sl if n not in dl]
    retyped = [(n, dl[n][0], sl[n][0]) for n in dl if n in sl and dl[n][0] != sl[n][0]]
    renames = []
    for d in list(donly):
        for s in sonly:
            if sl[s][0] == dl[d][0]:
                renames.append((d, s, dl[d][0]))
                donly.remove(d)
                sonly.remove(s)
                break
    score = 2 * len(donly) + len(sonly) + len(renames) + 0.5 * len(retyped)
    return {
        "dwarf_only": [(n, dl[n][0]) for n in donly],
        "source_only": [(n, sl[n][0]) for n in sonly],
        "renames": renames, "retyped": retyped, "score": score,
    }


def main():
    if "--measure" in sys.argv or not os.path.exists(CACHE):
        data = measure()
    else:
        data = json.load(open(CACHE))
    ufrag = opt("--unit")
    rows = []
    for unit, fns in data.items():
        if ufrag and ufrag not in unit:
            continue
        rel = unit.split("main/", 1)[1]
        dpath = os.path.join(ROOT, "dwarf", rel + ".cpp")
        spath = None
        for ext in (".cpp", ".c"):
            p = os.path.join(ROOT, "src", rel + ext)
            if os.path.exists(p):
                spath = p
        dfns = dwarf_functions(dpath) if os.path.exists(dpath) else []
        src = strip_code(open(spath, encoding="utf-8", errors="replace").read()) if spath else ""
        for sym, pct, size in fns:
            cls, name = demangle(sym)
            cands = [f for f in dfns if f[1] == name and f[0] == cls] or \
                    [f for f in dfns if f[1] == name]
            hits = find_source_fn(src, cls, name) if src else []
            if not hits and cls:
                hits = find_source_fn(src, None, name)
            note = ""
            if not cands:
                note = "no DWARF"
            if not hits:
                note = (note + "; " if note else "") + "no source"
            best = None
            for c in cands or [(None, name, [], [])]:
                for ptxt, body in hits or [("", "")]:
                    a = analyse(c[3], body_locals(body), c[2] + re.findall(r"(\w+)\s*(?:,|$)", ptxt))
                    if best is None or a["score"] < best["score"]:
                        best = a
            if not cands:
                best["score"] = 0.0  # no DWARF evidence: nothing to rank on
            unm = size * (100.0 - pct) / 100.0
            best.update(unit=unit[8:], sym=sym, pct=pct, size=size, unmatched=unm,
                        rank=best["score"] * unm, note=note)
            rows.append(best)
    rows.sort(key=lambda r: -r["rank"])
    top = int(opt("--top", "0") or 0)
    shown = rows if "--all" in sys.argv or not top else rows[:top]
    for i, r in enumerate(shown, 1):
        print("%3d. rank %7.1f  score %4.1f  %6.2f%% %5db  %s  [%s]%s" % (
            i, r["rank"], r["score"], r["pct"], r["size"], r["sym"], r["unit"],
            ("  (" + r["note"] + ")") if r["note"] else ""))
        if r["dwarf_only"]:
            print("      DWARF-only : " + ", ".join("%s %s" % (t, n) for n, t in r["dwarf_only"]))
        if r["source_only"]:
            print("      source-only: " + ", ".join("%s %s" % (t, n) for n, t in r["source_only"]))
        if r["renames"]:
            print("      rename?    : " + ", ".join("%s<-%s (%s)" % (d, s, t) for d, s, t in r["renames"]))
        if r["retyped"]:
            print("      retyped    : " + ", ".join("%s %s->%s" % (n, a, b) for n, a, b in r["retyped"]))


main()
