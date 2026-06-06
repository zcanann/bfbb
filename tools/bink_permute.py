#!/usr/bin/env python3
"""Dependency-aware source-statement permutation search for Bink match scoring.

mwcc/ProDG is sensitive to the *order* of source statements: the position of a
`saveout = out` copy, an `output_scale = root` float move, or two argument-save
`mr`s often follows source order verbatim. When a function is otherwise matched
but diverges only by instruction scheduling / register-save ordering, the fix is
usually to reorder a few independent statements in the source.

This tool automates that search. Given a unit, a symbol, and a contiguous block
of simple statements, it:

  1. Parses the block into statements (leading comments/blank lines stick to the
     statement below them).
  2. Computes per-statement read/write sets and builds a hazard DAG (RAW, WAR,
     WAW on scalars; memory ops serialized by default for safety).
  3. Enumerates linear extensions of that DAG -- every ordering that preserves
     all data hazards, i.e. is guaranteed semantics-preserving.
  4. Builds + scores each ordering via the existing bink_match pipeline, keeping
     the best (higher match%, then fewer diffs).
  5. Restores the original source (or, with --apply, leaves the winner in place).

Because only hazard-preserving permutations are emitted, every candidate is a
legal reordering of the original code -- this is NOT a fakematch generator.

FAITHFULNESS, READ THIS: a higher score from a reordering is only meaningful if
that order is plausibly what the original author wrote. A *large* jump (or a hit
to 100%) is strong evidence the winning order IS the original. A *marginal* gain
(a percent or two) from a scrambled-looking order is usually the compiler being
coaxed, not the real source -- reject it. "Score go up" is not the objective.
The tool never applies anything unless you pass --apply, and --apply still wants
--min-gain to clear, so a noisy +0.3% won't get written by accident.

Example:
  python tools/bink_permute.py --unit dct --symbol FastFDCT8x8 --start 1243 --end 1254
  python tools/bink_permute.py --unit binkacd --symbol Unquant --start 360 --end 364 --apply
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Set, Tuple

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import bink_match as bm  # noqa: E402

IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
C_KEYWORDS = {
    "if", "else", "for", "while", "do", "switch", "case", "default", "break",
    "continue", "return", "goto", "sizeof", "const", "static", "volatile",
    "register", "struct", "union", "enum", "typedef", "void",
}
# Type-ish tokens that appear on a declaration LHS but are never real defs/uses.
TYPE_TOKENS = {
    "void", "char", "short", "int", "long", "float", "double", "signed",
    "unsigned", "const", "volatile", "register", "static", "struct", "union",
    "enum", "PTR4", "u8", "u16", "u32", "u64", "s8", "s16", "s32", "s64",
    "f32", "f64", "BOOL", "size_t",
}
MEM = "@mem"


class Statement:
    """One reorderable source statement plus any trivia that sticks above it."""

    def __init__(self, trivia: List[str], line: str, raw: str):
        self.trivia = trivia          # comment/blank lines that precede the stmt
        self.line = line              # the statement source line (stripped of EOL)
        self.raw = raw                # full reconstructed text incl. trivia + EOL
        self.defs: Set[str] = set()
        self.uses: Set[str] = set()
        self.barrier = False          # unparseable -> pinned in place


def read_source(path: Path) -> str:
    # Binary round-trip so we never translate CRLF<->LF (repo files are LF).
    return path.read_bytes().decode("utf-8", "surrogateescape")


def write_source(path: Path, text: str) -> None:
    path.write_bytes(text.encode("utf-8", "surrogateescape"))


def split_keep_ends(text: str) -> List[str]:
    return text.splitlines(keepends=True)


def is_trivia(stripped: str) -> bool:
    return (
        stripped == ""
        or stripped.startswith("//")
        or stripped.startswith("/*")
        or stripped.startswith("*")
        or stripped.endswith("*/")
    )


def top_level_assign_split(code: str) -> Optional[Tuple[str, str]]:
    """Split `lhs = rhs` on the first top-level '=' that is real assignment."""
    depth = 0
    i = 0
    n = len(code)
    while i < n:
        c = code[i]
        if c in "([":
            depth += 1
        elif c in ")]":
            depth -= 1
        elif c == "=" and depth == 0:
            prev = code[i - 1] if i > 0 else ""
            nxt = code[i + 1] if i + 1 < n else ""
            if prev in "=!<>+-*/%&|^" or nxt == "=":
                i += 1
                continue
            return code[:i], code[i + 1:]
        i += 1
    return None


def lhs_target(lhs: str) -> Tuple[Set[str], Set[str], bool]:
    """Return (defs, extra_uses, is_mem) for an assignment LHS."""
    is_mem = bool(re.search(r"[\[\.]|->", lhs)) or lhs.strip().startswith("*")
    idents = [m.group(0) for m in IDENT.finditer(lhs)]
    real = [t for t in idents if t not in TYPE_TOKENS and t not in C_KEYWORDS]
    if is_mem:
        # Writing through memory: the base/index identifiers are *read*.
        return set(), set(real), True
    # Scalar declaration/assignment: the final identifier is the def.
    if not real:
        return set(), set(), False
    return {real[-1]}, set(), False


def collect_idents(expr: str) -> Set[str]:
    out = set()
    for m in IDENT.finditer(expr):
        tok = m.group(0)
        # Skip identifiers that are immediately followed by '(' -> function call
        # name; the call itself is treated via the memory barrier below.
        if tok in C_KEYWORDS or tok in TYPE_TOKENS:
            continue
        out.add(tok)
    return out


def analyze(stmt: Statement, relax_mem: bool) -> None:
    code = stmt.line.strip()
    body = code[:-1].strip() if code.endswith(";") else code

    # Function calls / unparseable constructs -> hard barrier (pinned).
    # We allow a plain assignment whose RHS contains a call only if it is a pure
    # value (handled via mem barrier); but control flow tokens pin the line.
    first = IDENT.match(body)
    if first and first.group(0) in C_KEYWORDS:
        stmt.barrier = True
        return

    has_call = bool(re.search(r"[A-Za-z_][A-Za-z0-9_]*\s*\(", body))

    incdec = re.fullmatch(r"(\+\+|--)?\s*([A-Za-z_][A-Za-z0-9_]*)\s*(\+\+|--)?", body)
    if incdec and (incdec.group(1) or incdec.group(3)):
        name = incdec.group(2)
        stmt.defs.add(name)
        stmt.uses.add(name)
        return

    split = top_level_assign_split(body)
    if split is None:
        # No assignment and not inc/dec: pin it (e.g. a bare call statement).
        stmt.barrier = True
        return

    lhs, rhs = split
    compound = lhs.rstrip().endswith(tuple("+-*/%&|^")) or lhs.rstrip().endswith(">>") or lhs.rstrip().endswith("<<")
    lhs = re.sub(r"[+\-*/%&|^]?=$", "=", lhs)  # normalize but we already split
    defs, lhs_uses, is_mem = lhs_target(lhs.rstrip(" "))

    stmt.uses |= lhs_uses
    stmt.uses |= collect_idents(rhs)
    if compound:
        stmt.uses |= defs
    stmt.defs |= defs

    if is_mem:
        stmt.defs.add(MEM)
    if has_call:
        # A call may read/write arbitrary memory -> serialize against memory.
        stmt.uses.add(MEM)
        stmt.defs.add(MEM)
    if not relax_mem and re.search(r"[\[\.]|->|\*", rhs):
        stmt.uses.add(MEM)


def hazard(a: Statement, b: Statement) -> bool:
    """True if a must stay before b (RAW / WAR / WAW)."""
    if a.barrier or b.barrier:
        return True
    if a.defs & (b.uses | b.defs):
        return True
    if a.uses & b.defs:
        return True
    return False


def linear_extensions(stmts: Sequence[Statement], cap: int) -> List[Tuple[int, ...]]:
    n = len(stmts)
    preds: List[Set[int]] = [set() for _ in range(n)]
    for j in range(n):
        for i in range(j):
            if hazard(stmts[i], stmts[j]):
                preds[j].add(i)

    results: List[Tuple[int, ...]] = []

    def rec(scheduled: Tuple[int, ...], done: Set[int]) -> None:
        if len(results) >= cap:
            return
        if len(scheduled) == n:
            results.append(scheduled)
            return
        for node in range(n):
            if node in done:
                continue
            if preds[node] <= done:
                rec(scheduled + (node,), done | {node})
                if len(results) >= cap:
                    return

    rec((), set())
    return results


def parse_block(lines: List[str]) -> List[Statement]:
    """lines are raw (with EOL). Group trivia onto the following statement."""
    stmts: List[Statement] = []
    trivia: List[str] = []
    for raw in lines:
        stripped = raw.strip()
        if is_trivia(stripped):
            trivia.append(raw)
            continue
        stmt = Statement(trivia, raw.rstrip("\r\n"), "")
        stmt.raw = "".join(trivia) + raw
        stmts.append(stmt)
        trivia = []
    if trivia:
        # Trailing trivia: pin to the last statement so it never floats up.
        if stmts:
            stmts[-1].raw = stmts[-1].raw + "".join(trivia)
        else:
            # Block is all trivia; nothing to permute.
            pass
    return stmts


def render(stmts: Sequence[Statement], order: Sequence[int]) -> str:
    return "".join(stmts[i].raw for i in order)


def score_unit(unit: str, symbol: str) -> Tuple[Optional[float], Optional[int]]:
    try:
        path = bm.current_json(unit, True, True)
    except subprocess.CalledProcessError:
        # Permutation did not compile/link (e.g. C89 decl-after-statement).
        return None, None
    data = bm.load_json(path)
    if data is None:
        return None, None
    pct = bm.symbol_score(data, symbol)
    diffs = None
    try:
        left = bm.left_symbol(data, symbol)
        diffs = bm.diff_count(left)
    except SystemExit:
        diffs = None
    return pct, diffs


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", required=True, choices=list(bm.UNITS))
    ap.add_argument("--symbol", required=True, help="function name to score")
    ap.add_argument("--start", type=int, required=True, help="first line of block (1-based, inclusive)")
    ap.add_argument("--end", type=int, required=True, help="last line of block (1-based, inclusive)")
    ap.add_argument("--max", type=int, default=200, help="max permutations to build (default 200)")
    ap.add_argument("--relax-mem", action="store_true",
                    help="do not serialize memory reads against memory writes "
                         "(more permutations; only safe when addresses are independent)")
    ap.add_argument("--apply", action="store_true", help="leave the best permutation written to disk")
    ap.add_argument("--min-gain", type=float, default=1.0,
                    help="with --apply, only keep the winner if it beats baseline "
                         "match%% by at least this much (default 1.0; guards against "
                         "coaxed marginal gains that are not faithful)")
    ap.add_argument("--file", help="override source file (defaults to the unit's .c/.cpp)")
    args = ap.parse_args()

    if args.file:
        src = Path(args.file)
    else:
        cands = [p for p in bm.unit_source_candidates(args.unit) if p.exists()]
        if not cands:
            print(f"no source file found for unit {args.unit}", file=sys.stderr)
            return 2
        src = cands[0]

    original_text = read_source(src)
    all_lines = split_keep_ends(original_text)
    if args.start < 1 or args.end > len(all_lines) or args.start > args.end:
        print(f"bad range: file has {len(all_lines)} lines", file=sys.stderr)
        return 2

    block = all_lines[args.start - 1: args.end]
    head = all_lines[: args.start - 1]
    tail = all_lines[args.end:]

    stmts = parse_block(block)
    movable = [s for s in stmts if not s.barrier]
    print(f"block: {len(stmts)} statements ({len(movable)} movable, "
          f"{len(stmts) - len(movable)} pinned)")
    for idx, s in enumerate(stmts):
        analyze(s, args.relax_mem)
    # Re-derive defs/uses (analyze sets them); recompute movable after analysis.
    movable = [s for s in stmts if not s.barrier]
    print(f"after dataflow: {len(movable)} movable")

    orders = linear_extensions(stmts, args.max)
    identity = tuple(range(len(stmts)))
    # Put identity first (baseline), then the rest, capped.
    ordered = [identity] + [o for o in orders if o != identity]
    ordered = ordered[: args.max]
    print(f"enumerated {len(orders)} hazard-preserving orderings; "
          f"will build {len(ordered)} (incl. baseline)")
    if len(orders) >= args.max:
        print(f"  (capped at --max {args.max}; raise it to explore more)")

    best: Optional[Tuple[float, int, Tuple[int, ...]]] = None
    baseline: Optional[Tuple[Optional[float], Optional[int]]] = None

    def write_order(order: Sequence[int]) -> None:
        new_block = render(stmts, order)
        write_source(src, "".join(head) + new_block + "".join(tail))

    try:
        for n, order in enumerate(ordered):
            write_order(order)
            pct, diffs = score_unit(args.unit, args.symbol)
            tag = "baseline" if order == identity else f"perm {n}"
            pct_s = f"{pct:.4f}" if pct is not None else "?"
            diff_s = str(diffs) if diffs is not None else "?"
            note = ""
            if order == identity:
                baseline = (pct, diffs)
            if pct is not None:
                key_diffs = diffs if diffs is not None else 10 ** 9
                if best is None or (pct, -key_diffs) > (best[0], -best[1]):
                    best = (pct, key_diffs, tuple(order))
                    if order != identity:
                        note = "  <== new best"
            print(f"[{n + 1}/{len(ordered)}] {tag}: match={pct_s} diffs={diff_s}{note}")
            sys.stdout.flush()
    finally:
        base_pct = baseline[0] if baseline and baseline[0] is not None else None
        gain = (best[0] - base_pct) if (best is not None and base_pct is not None) else 0.0
        keep = (
            args.apply
            and best is not None
            and best[2] != identity
            and gain >= args.min_gain
        )
        if keep:
            write_order(best[2])
            print(f"\napplied best permutation to {src.relative_to(ROOT)} (+{gain:.4f}%)")
            print("REMINDER: confirm this order is faithful to the original source, "
                  "not just a higher score.")
        else:
            write_source(src, original_text)
            if args.apply and best is not None and best[2] != identity:
                print(f"\nbest gain was +{gain:.4f}% (< --min-gain {args.min_gain}); "
                      "left file unchanged -- treat small gains as coaxing, not a fix")
            elif args.apply:
                print("\nno improvement over baseline; left file unchanged")
            else:
                print("\nrestored original source (use --apply to keep the winner)")

    if best is not None:
        bpct, bdiffs, border = best
        print(f"\nbest: match={bpct:.4f} diffs={bdiffs} "
              f"({'baseline' if border == identity else 'permutation'})")
        if baseline and baseline[0] is not None and border != identity:
            print(f"baseline was: match={baseline[0]:.4f} diffs={baseline[1]}")
            if border != identity:
                print("winning order (block-relative statement indices):", list(border))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
