#!/usr/bin/env python3
"""rw_permute.py - a random source permuter for RenderWare (C) units.

Like decomp-permuter, but self-contained and aware of this repo's build:

  * The unit's exact compile command is taken from `ninja -t commands <obj>`;
    a private COPY of the source (same basename, own scratch dir) is compiled
    with `-o` redirected and `-i <original dir>` prepended so quoted includes
    still resolve.  The real source file is never written.
  * Each candidate is scored with
        objdiff-cli diff -1 <target.o> -2 <cand.o> --format json
                         -c functionRelocDiffs=none
    (sum of match_percent of the --score symbols).  Every other function in
    the unit must stay at (or above) its baseline score, else the candidate is
    rejected.
  * The functions named with --func are parsed into a small C AST (own
    tokenizer + statement parser + Pratt expression parser; no pycparser) and
    mutated with semantics-preserving rewrites (see MUTATIONS below).  Bodies
    are re-emitted by a pretty printer (comments are dropped, braces added for
    every if/loop body; neither changes codegen).
  * N worker processes hill-climb in parallel (keep improvements, accept equal
    scores with --p-equal, rare worse moves with --p-worse), sharing the global
    best through a file in the output dir.  Each new best is written to
    <out>/best_<score>.c; every 100% hit is minimised (diff hunks reverted
    one by one while it stays 100% and other functions stay put) and written
    to <out>/hit_*.diff as a unified diff against the normalised original.

Mutations (all guarded by a def/use + memory hazard analysis; calls are
treated as reading/writing memory and writing every bare-identifier argument,
since many RW "calls" are macros):
  swap        swap adjacent independent statements / declarations
  move        move a statement several slots past independent statements
  declin      move a declaration into the smallest block that uses it
  declout     move a block-local declaration to the enclosing block
  temp        introduce a temp (__typeof__ or the declared type) for a
              side-effect-free subexpression
  reuse       same, but reuse an existing local that is dead at that point
  inline      inline a single-use temp
  commute     swap operands of + * & | ^ == != ; mirror < > <= >=
  ternary     if(c)x=a;else x=b;  <->  x=c?a:b;  <->  x=b;if(c)x=a;
  compound    a op= b  <->  a = a op b
  cast        add/remove a cast to the expression's own declared type
  wrapreal    wrap/unwrap a float product in (RwReal)(...)
  splitdecl   T x = e;  <->  T x; ... x = e;
  splitmulti  T a, b;  <->  T a; T b;
  forwhile    for(i;c;s){..}  <->  i; while(c){..; s;}
  ptrform     *p <-> p[0], p[i] <-> *(p + i), p++ <-> p += 1 <-> p = p + 1
  walkidx     loop pointer walk <-> loop index (p[i] ... / *p ... p++)
  not         !x <-> x == 0 / NULL;  x != 0 <-> x (in conditions)
  ifswap      if(c)A else B  ->  if(!c)B else A
  incdec      i++ <-> ++i (statement context)

Usage:
  python tools/rw_permute.py --src src/rwsdk/src/plcore/bamatrix.c \
      --func MatrixOrthoNormalize --unit main/rwsdk/src/plcore/bamatrix \
      --time 1800 --workers 6
  python tools/rw_permute.py ... --seed my_expanded_copy.c   # start from a copy
  python tools/rw_permute.py ... --selftest      # parse/print round-trip only

Options of note: --score SYM (repeatable; default = the --func names),
--func can be repeated (all are mutated, e.g. an inlined static helper), and
--weights 'temp=3,reuse=0' tunes the mutation mix.

FAITHFULNESS: a 100% hit is only a candidate.  Judge plausibility by hand.
"""

from __future__ import annotations

import argparse
import copy
import difflib
import json
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OBJDIFF = ROOT / "build" / "tools" / "objdiff-cli.exe"

# --------------------------------------------------------------------------
# Tokenizer
# --------------------------------------------------------------------------

PUNCT = sorted("""
>>= <<= ... -> ++ -- << >> <= >= == != && || *= /= %= += -= &= ^= |= ##
{ } [ ] ( ) ; , < > + - * / % & | ^ ! ~ ? : = . #
""".split(), key=len, reverse=True)

TYPE_KW = {
    "void", "char", "short", "int", "long", "float", "double", "signed",
    "unsigned", "_Bool",
}
QUALS = {"const", "volatile", "register", "static", "auto", "extern", "inline", "__inline"}
TAGS = {"struct", "union", "enum"}
KEYWORDS = TYPE_KW | QUALS | TAGS | {
    "if", "else", "for", "while", "do", "switch", "case", "default", "break",
    "continue", "return", "goto", "sizeof", "typedef",
}
FLOAT_TYPES = {"RwReal", "float", "double", "f32", "f64"}
INT_TYPES = {
    "int", "short", "long", "char", "unsigned", "signed", "RwInt32", "RwUInt32",
    "RwInt16", "RwUInt16", "RwInt8", "RwUInt8", "RwBool", "RwChar", "RwInt64",
    "RwUInt64", "u8", "u16", "u32", "s8", "s16", "s32", "size_t", "BOOL",
}
BARRIER_CALLS = {"RWRETURN", "RWRETURNVOID", "RWERROR", "RWASSERT", "RWFUNCTION",
                 "RWAPIFUNCTION", "RWMESSAGE", "RWCRTCHECKMEMORY", "longjmp", "setjmp"}
PURE_CALLS = {"RwRealAbs", "RwV3dDotProductMacro", "RwV3dDotProduct",
              "RwRealMin2", "RwRealMax2", "RwV2dDotProductMacro"}


class Tok:
    __slots__ = ("k", "t")

    def __init__(self, k, t):
        self.k = k
        self.t = t

    def __repr__(self):
        return f"{self.k}:{self.t}"


def tokenize(src):
    toks = []
    i = 0
    n = len(src)
    line_start = True
    while i < n:
        c = src[i]
        if c == "\n":
            line_start = True
            i += 1
            continue
        if c in " \t\r\f\v":
            i += 1
            continue
        if src.startswith("//", i):
            j = src.find("\n", i)
            i = n if j < 0 else j
            continue
        if src.startswith("/*", i):
            j = src.find("*/", i + 2)
            i = n if j < 0 else j + 2
            continue
        if c == "#" and line_start:
            j = i
            while True:
                k = src.find("\n", j)
                if k < 0:
                    k = n
                if src[j:k].rstrip("\r").endswith("\\"):
                    j = k + 1
                    continue
                break
            toks.append(Tok("pp", src[i:k].rstrip("\r")))
            i = k
            continue
        line_start = False
        m = re.match(r"(?:__asm|asm)(?![A-Za-z0-9_])\s*\{", src[i:i + 64])
        if m:
            j = src.find("}", i)
            toks.append(Tok("raw", src[i:j + 1]))
            i = j + 1
            continue
        if c.isalpha() or c == "_":
            j = i + 1
            while j < n and (src[j].isalnum() or src[j] == "_"):
                j += 1
            toks.append(Tok("id", src[i:j]))
            i = j
            continue
        if c.isdigit() or (c == "." and i + 1 < n and src[i + 1].isdigit()):
            m = re.match(r"0[xX][0-9a-fA-F]+[uUlL]*|(\d+\.?\d*|\.\d+)([eE][+-]?\d+)?[fFlLuU]*", src[i:])
            toks.append(Tok("num", m.group(0)))
            i += len(m.group(0))
            continue
        if c in "\"'":
            j = i + 1
            while j < n and src[j] != c:
                if src[j] == "\\":
                    j += 1
                j += 1
            toks.append(Tok("str" if c == '"' else "chr", src[i:j + 1]))
            i = j + 1
            continue
        for p in PUNCT:
            if src.startswith(p, i):
                toks.append(Tok("op", p))
                i += len(p)
                break
        else:
            raise SyntaxError(f"bad char {c!r} at {i}")
    return toks


# --------------------------------------------------------------------------
# AST
# --------------------------------------------------------------------------

class Node:
    def __repr__(self):
        return f"{type(self).__name__}({self.__dict__})"


# expressions
class Id(Node):
    def __init__(self, name):
        self.name = name


class Lit(Node):
    def __init__(self, text, kind):
        self.text = text
        self.kind = kind


class Paren(Node):
    def __init__(self, e):
        self.e = e


class Unary(Node):
    def __init__(self, op, e):
        self.op = op
        self.e = e


class Postfix(Node):
    def __init__(self, op, e):
        self.op = op
        self.e = e


class Cast(Node):
    def __init__(self, ty, e):
        self.ty = ty  # type text
        self.e = e


class Sizeof(Node):
    def __init__(self, ty=None, e=None):
        self.ty = ty
        self.e = e


class Binary(Node):
    def __init__(self, op, l, r):
        self.op = op
        self.l = l
        self.r = r


class Ternary(Node):
    def __init__(self, c, a, b):
        self.c = c
        self.a = a
        self.b = b


class Assign(Node):
    def __init__(self, op, l, r):
        self.op = op
        self.l = l
        self.r = r


class Comma(Node):
    def __init__(self, l, r):
        self.l = l
        self.r = r


class Call(Node):
    def __init__(self, f, args):
        self.f = f
        self.args = args


class Index(Node):
    def __init__(self, a, i):
        self.a = a
        self.i = i


class Member(Node):
    def __init__(self, e, op, name):
        self.e = e
        self.op = op
        self.name = name


class Opaque(Node):
    def __init__(self, text):
        self.text = text


EXPR_CHILDREN = {
    Id: (), Lit: (), Opaque: (), Paren: ("e",), Unary: ("e",), Postfix: ("e",),
    Cast: ("e",), Sizeof: ("e",), Binary: ("l", "r"), Ternary: ("c", "a", "b"),
    Assign: ("l", "r"), Comma: ("l", "r"), Call: ("f", "args"), Index: ("a", "i"),
    Member: ("e",),
}


# statements
class Compound(Node):
    def __init__(self, items, synthetic=False):
        self.items = items
        self.synthetic = synthetic


class Declarator(Node):
    def __init__(self, stars, name, suffix, init):
        self.stars = stars      # e.g. "*" or "**" or ""
        self.name = name
        self.suffix = suffix    # e.g. "[128]"
        self.init = init        # expr / Opaque / None


class Decl(Node):
    def __init__(self, ty, decls):
        self.ty = ty            # base type text, e.g. "const RwMatrix"
        self.decls = decls


class ExprStmt(Node):
    def __init__(self, e):
        self.e = e


class If(Node):
    def __init__(self, c, then, els):
        self.c = c
        self.then = then
        self.els = els


class For(Node):
    def __init__(self, init, c, step, body):
        self.init = init
        self.c = c
        self.step = step
        self.body = body


class While(Node):
    def __init__(self, c, body):
        self.c = c
        self.body = body


class DoWhile(Node):
    def __init__(self, body, c):
        self.body = body
        self.c = c


class Switch(Node):
    def __init__(self, c, body):
        self.c = c
        self.body = body


class Jump(Node):
    def __init__(self, kind, e=None, label=None):
        self.kind = kind
        self.e = e
        self.label = label


class Label(Node):
    def __init__(self, text):
        self.text = text


class PP(Node):
    def __init__(self, text):
        self.text = text


class Empty(Node):
    pass


class OpaqueStmt(Node):
    def __init__(self, text):
        self.text = text


LOOPS = (For, While, DoWhile)
PLAIN_TEMPS = False  # --plain-temps: never use __typeof__ for new temps


# --------------------------------------------------------------------------
# Parser
# --------------------------------------------------------------------------

class ParseError(Exception):
    pass


ASSIGN_OPS = {"=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="}
BINPREC = {
    "||": 4, "&&": 5, "|": 6, "^": 7, "&": 8, "==": 9, "!=": 9,
    "<": 10, ">": 10, "<=": 10, ">=": 10, "<<": 11, ">>": 11,
    "+": 12, "-": 12, "*": 13, "/": 13, "%": 13,
}


def toks_text(toks):
    out = ""
    prev = None
    for t in toks:
        s = t.t
        if prev is not None:
            a, b = prev[-1], s[0]
            if (a.isalnum() or a == "_") and (b.isalnum() or b == "_"):
                out += " "
            elif prev in (",",) or s in ("*",) and (a.isalnum() or a == "_"):
                out += " "
        out += s
        prev = s
    return out


class Parser:
    def __init__(self, toks, typenames, localnames):
        self.toks = toks
        self.i = 0
        self.types = typenames
        self.locals = localnames  # set, grows as decls are seen

    # -- helpers
    def peek(self, k=0):
        j = self.i + k
        return self.toks[j] if j < len(self.toks) else Tok("eof", "")

    def at(self, text, k=0):
        t = self.peek(k)
        return t.k in ("op", "id") and t.t == text

    def next(self):
        t = self.peek()
        self.i += 1
        return t

    def expect(self, text):
        t = self.next()
        if t.t != text:
            raise ParseError(f"expected {text!r} got {t.t!r} at {self.i}")
        return t

    def is_typename(self, t):
        if t.k != "id":
            return False
        if t.t in self.locals:
            return False
        return t.t in TYPE_KW or t.t in QUALS or t.t in TAGS or t.t in self.types

    # -- statements
    def parse_compound(self):
        self.expect("{")
        items = []
        saved = set(self.locals)
        while not self.at("}"):
            if self.peek().k == "eof":
                raise ParseError("eof in compound")
            items.append(self.parse_stmt())
        self.expect("}")
        self.locals = saved
        return Compound(items)

    def body(self):
        s = self.parse_stmt()
        if isinstance(s, Compound):
            return s
        return Compound([s], synthetic=True)

    def parse_stmt(self):
        t = self.peek()
        if t.k == "pp":
            self.next()
            return PP(t.t)
        if t.k == "raw":
            self.next()
            return OpaqueStmt(t.t)
        if t.k == "op" and t.t == "{":
            return self.parse_compound()
        if t.k == "op" and t.t == ";":
            self.next()
            return Empty()
        if t.k == "id":
            w = t.t
            if w == "if":
                self.next()
                self.expect("(")
                c = self.parse_expr_until(")")
                self.expect(")")
                then = self.body()
                els = None
                if self.at("else"):
                    self.next()
                    els = self.body()
                return If(c, then, els)
            if w == "while":
                self.next()
                self.expect("(")
                c = self.parse_expr_until(")")
                self.expect(")")
                return While(c, self.body())
            if w == "do":
                self.next()
                b = self.body()
                self.expect("while")
                self.expect("(")
                c = self.parse_expr_until(")")
                self.expect(")")
                self.expect(";")
                return DoWhile(b, c)
            if w == "for":
                self.next()
                self.expect("(")
                init = None if self.at(";") else self.parse_expr_until(";")
                self.expect(";")
                c = None if self.at(";") else self.parse_expr_until(";")
                self.expect(";")
                step = None if self.at(")") else self.parse_expr_until(")")
                self.expect(")")
                return For(init, c, step, self.body())
            if w == "switch":
                self.next()
                self.expect("(")
                c = self.parse_expr_until(")")
                self.expect(")")
                return Switch(c, self.body())
            if w in ("return",):
                self.next()
                e = None if self.at(";") else self.parse_expr_until(";")
                self.expect(";")
                return Jump("return", e)
            if w in ("break", "continue"):
                self.next()
                self.expect(";")
                return Jump(w)
            if w == "goto":
                self.next()
                lab = self.next().t
                self.expect(";")
                return Jump("goto", label=lab)
            if w in ("case", "default"):
                j = self.i
                depth = 0
                while True:
                    tt = self.toks[j]
                    if tt.t in "([":
                        depth += 1
                    elif tt.t in ")]":
                        depth -= 1
                    elif tt.t == ":" and depth == 0:
                        break
                    j += 1
                text = toks_text(self.toks[self.i:j + 1])
                self.i = j + 1
                return Label(text)
            if self.peek(1).t == ":" and w not in KEYWORDS:
                self.next()
                self.next()
                return Label(w + ":")
            d = self.try_decl()
            if d is not None:
                return d
        # expression statement
        start = self.i
        try:
            e = self.parse_expr_until(";")
            self.expect(";")
            return ExprStmt(e)
        except ParseError:
            self.i = start
            # opaque up to ';' at depth 0 (or a macro call w/o semicolon)
            depth = 0
            j = self.i
            while j < len(self.toks):
                tt = self.toks[j]
                if tt.t in "([{" and tt.k == "op":
                    depth += 1
                elif tt.t in ")]}" and tt.k == "op":
                    if depth == 0:
                        break
                    depth -= 1
                    if depth == 0 and tt.t == ")" and self.toks[j + 1].t != ";" \
                            and self.toks[j + 1].k in ("id", "pp", "op") and self.toks[j + 1].t not in (";",):
                        j += 1
                        text = toks_text(self.toks[self.i:j])
                        self.i = j
                        return OpaqueStmt(text)
                elif tt.t == ";" and depth == 0:
                    break
                j += 1
            text = toks_text(self.toks[self.i:j + 1])
            self.i = j + 1
            return OpaqueStmt(text)

    def try_decl(self):
        start = self.i
        j = self.i
        tyt = []
        saw_type = False
        while True:
            t = self.toks[j]
            if t.k != "id":
                break
            if t.t == "__typeof__" and not saw_type and self.toks[j + 1].t == "(":
                k = j + 1
                depth = 0
                while True:
                    if self.toks[k].t == "(":
                        depth += 1
                    elif self.toks[k].t == ")":
                        depth -= 1
                        if depth == 0:
                            break
                    k += 1
                tyt.extend(self.toks[j:k + 1])
                j = k + 1
                saw_type = True
                continue
            if t.t in QUALS or t.t in ("signed", "unsigned"):
                tyt.append(t)
                j += 1
                if t.t in ("signed", "unsigned"):
                    saw_type = True
                continue
            if t.t in TAGS:
                tyt.append(t)
                tyt.append(self.toks[j + 1])
                j += 2
                saw_type = True
                continue
            if not saw_type and self.is_typename(t):
                tyt.append(t)
                j += 1
                saw_type = True
                continue
            if saw_type and t.t in TYPE_KW:
                tyt.append(t)
                j += 1
                continue
            break
        if not saw_type:
            return None
        # need: stars* IDENT then one of = ; , [
        k = j
        while self.toks[k].t in ("*",) or self.toks[k].t in ("const", "volatile"):
            k += 1
        if self.toks[k].k != "id" or self.toks[k].t in KEYWORDS:
            return None
        if self.toks[k + 1].t not in ("=", ";", ",", "["):
            return None
        self.i = j
        ty = toks_text(tyt)
        decls = []
        while True:
            stars = ""
            while self.at("*") or self.at("const") or self.at("volatile"):
                tt = self.next().t
                stars += tt if tt == "*" else " " + tt + " "
            name = self.next().t
            suffix = ""
            while self.at("["):
                j2 = self.i
                depth = 0
                while True:
                    tt = self.toks[j2].t
                    if tt == "[":
                        depth += 1
                    elif tt == "]":
                        depth -= 1
                        if depth == 0:
                            break
                    j2 += 1
                suffix += toks_text(self.toks[self.i:j2 + 1])
                self.i = j2 + 1
            init = None
            if self.at("="):
                self.next()
                if self.at("{"):
                    j2 = self.i
                    depth = 0
                    while True:
                        tt = self.toks[j2].t
                        if tt == "{":
                            depth += 1
                        elif tt == "}":
                            depth -= 1
                            if depth == 0:
                                break
                        j2 += 1
                    init = Opaque(toks_text(self.toks[self.i:j2 + 1]))
                    self.i = j2 + 1
                else:
                    init = self.parse_assign_safe()
            self.locals.add(name)
            decls.append(Declarator(stars.strip(), name, suffix, init))
            if self.at(","):
                self.next()
                continue
            self.expect(";")
            break
        return Decl(ty, decls)

    # -- expressions
    def parse_expr_until(self, end):
        """Parse a full expression; on failure produce Opaque up to `end` at depth 0."""
        start = self.i
        try:
            e = self.parse_comma()
            if not self.at(end):
                raise ParseError("trailing")
            return e
        except (ParseError, IndexError):
            self.i = start
            depth = 0
            j = self.i
            while True:
                tt = self.toks[j]
                if tt.k == "op" and tt.t in "([{":
                    depth += 1
                elif tt.k == "op" and tt.t in ")]}":
                    if depth == 0:
                        break
                    depth -= 1
                elif depth == 0 and tt.t == end:
                    break
                j += 1
            e = Opaque(toks_text(self.toks[self.i:j]))
            self.i = j
            return e

    def parse_assign_safe(self):
        start = self.i
        try:
            e = self.parse_assign()
            if not (self.at(",") or self.at(")") or self.at(";")):
                raise ParseError("trailing")
            return e
        except (ParseError, IndexError):
            self.i = start
            depth = 0
            j = self.i
            while True:
                tt = self.toks[j]
                if tt.k == "op" and tt.t in "([{":
                    depth += 1
                elif tt.k == "op" and tt.t in ")]}":
                    if depth == 0:
                        break
                    depth -= 1
                elif depth == 0 and tt.t in (",", ";"):
                    break
                j += 1
            e = Opaque(toks_text(self.toks[self.i:j]))
            self.i = j
            return e

    def parse_comma(self):
        e = self.parse_assign()
        while self.at(","):
            self.next()
            e = Comma(e, self.parse_assign())
        return e

    def parse_assign(self):
        l = self.parse_cond()
        t = self.peek()
        if t.k == "op" and t.t in ASSIGN_OPS:
            self.next()
            return Assign(t.t, l, self.parse_assign())
        return l

    def parse_cond(self):
        c = self.parse_binary(4)
        if self.at("?"):
            self.next()
            a = self.parse_comma()
            self.expect(":")
            b = self.parse_cond()
            return Ternary(c, a, b)
        return c

    def parse_binary(self, minp):
        l = self.parse_unary()
        while True:
            t = self.peek()
            if t.k != "op" or t.t not in BINPREC or BINPREC[t.t] < minp:
                return l
            self.next()
            r = self.parse_binary(BINPREC[t.t] + 1)
            l = Binary(t.t, l, r)

    def cast_ahead(self):
        """At '(' : is this a cast/type-name in parens?  Returns end index or None."""
        j = self.i + 1
        first = self.toks[j]
        if not self.is_typename(first):
            return None
        while True:
            t = self.toks[j]
            if t.t == ")":
                return j
            if t.k == "id" and (self.is_typename(t) or t.t in TYPE_KW or t.t in QUALS or t.t in TAGS
                                or (j > 0 and self.toks[j - 1].t in TAGS)):
                j += 1
                continue
            if t.t == "*":
                j += 1
                continue
            return None

    def parse_unary(self):
        t = self.peek()
        if t.k == "op":
            if t.t in ("!", "~", "-", "+", "*", "&"):
                self.next()
                return Unary(t.t, self.parse_unary())
            if t.t in ("++", "--"):
                self.next()
                return Unary(t.t, self.parse_unary())
            if t.t == "(":
                end = self.cast_ahead()
                if end is not None:
                    ty = toks_text(self.toks[self.i + 1:end])
                    self.i = end + 1
                    return Cast(ty, self.parse_unary())
        if t.k == "id" and t.t == "sizeof":
            self.next()
            if self.at("("):
                end = self.cast_ahead()
                if end is not None:
                    ty = toks_text(self.toks[self.i + 1:end])
                    self.i = end + 1
                    return Sizeof(ty=ty)
            return Sizeof(e=self.parse_unary())
        return self.parse_postfix()

    def parse_postfix(self):
        e = self.parse_primary()
        while True:
            t = self.peek()
            if t.k != "op":
                return e
            if t.t == "(":
                self.next()
                args = []
                if not self.at(")"):
                    while True:
                        args.append(self.parse_assign_safe())
                        if self.at(","):
                            self.next()
                            continue
                        break
                self.expect(")")
                e = Call(e, args)
            elif t.t == "[":
                self.next()
                i = self.parse_comma()
                self.expect("]")
                e = Index(e, i)
            elif t.t in (".", "->"):
                self.next()
                e = Member(e, t.t, self.next().t)
            elif t.t in ("++", "--"):
                self.next()
                e = Postfix(t.t, e)
            else:
                return e

    def parse_primary(self):
        t = self.next()
        if t.k == "id" and t.t not in KEYWORDS:
            return Id(t.t)
        if t.k == "num":
            return Lit(t.t, "num")
        if t.k == "chr":
            return Lit(t.t, "chr")
        if t.k == "str":
            s = t.t
            while self.peek().k == "str":
                s += " " + self.next().t
            return Lit(s, "str")
        if t.k == "op" and t.t == "(":
            e = self.parse_comma()
            self.expect(")")
            return Paren(e)
        raise ParseError(f"unexpected {t.t!r}")


# --------------------------------------------------------------------------
# Printer
# --------------------------------------------------------------------------

def eprec(e):
    if isinstance(e, Comma):
        return 1
    if isinstance(e, Assign):
        return 2
    if isinstance(e, Ternary):
        return 3
    if isinstance(e, Binary):
        return BINPREC[e.op]
    if isinstance(e, (Unary, Cast, Sizeof)):
        return 14
    if isinstance(e, (Postfix, Call, Index, Member)):
        return 15
    return 16


def pe(e, need=0):
    s = _pe(e)
    if eprec(e) < need:
        return "(" + s + ")"
    return s


def _pe(e):
    if isinstance(e, Id):
        return e.name
    if isinstance(e, Lit):
        return e.text
    if isinstance(e, Opaque):
        return e.text
    if isinstance(e, Paren):
        return "(" + pe(e.e, 0) + ")"
    if isinstance(e, Unary):
        s = pe(e.e, 14)
        op = e.op
        if s and op[-1] in "+-&" and s[0] == op[-1]:
            return op + " " + s
        return op + s
    if isinstance(e, Postfix):
        return pe(e.e, 15) + e.op
    if isinstance(e, Cast):
        return "(" + e.ty + ")" + pe(e.e, 14)
    if isinstance(e, Sizeof):
        if e.ty is not None:
            return "sizeof(" + e.ty + ")"
        return "sizeof " + pe(e.e, 14) if not isinstance(e.e, Paren) else "sizeof" + pe(e.e, 14)
    if isinstance(e, Binary):
        p = BINPREC[e.op]
        return pe(e.l, p) + " " + e.op + " " + pe(e.r, p + 1)
    if isinstance(e, Ternary):
        return pe(e.c, 4) + " ? " + pe(e.a, 1) + " : " + pe(e.b, 3)
    if isinstance(e, Assign):
        return pe(e.l, 14) + " " + e.op + " " + pe(e.r, 2)
    if isinstance(e, Comma):
        return pe(e.l, 1) + ", " + pe(e.r, 2)
    if isinstance(e, Call):
        return pe(e.f, 15) + "(" + ", ".join(pe(a, 2) for a in e.args) + ")"
    if isinstance(e, Index):
        return pe(e.a, 15) + "[" + pe(e.i, 0) + "]"
    if isinstance(e, Member):
        return pe(e.e, 15) + e.op + e.name
    raise TypeError(type(e))


def ps(s, ind, out):
    pad = "    " * ind
    if isinstance(s, Compound):
        out.append(pad + "{")
        for it in s.items:
            ps(it, ind + 1, out)
        out.append(pad + "}")
    elif isinstance(s, Decl):
        parts = []
        for d in s.decls:
            t = (d.stars + " " if d.stars and not d.stars.endswith("*") else d.stars) + d.name + d.suffix
            if d.init is not None:
                t += " = " + pe(d.init, 2)
            parts.append(t)
        ty = s.ty
        if s.decls[0].stars:
            out.append(pad + ty + " " + ", ".join(parts) + ";")
        else:
            out.append(pad + ty + " " + ", ".join(parts) + ";")
    elif isinstance(s, ExprStmt):
        out.append(pad + pe(s.e, 0) + ";")
    elif isinstance(s, If):
        out.append(pad + "if (" + pe(s.c, 0) + ")")
        ps_body(s.then, ind, out)
        if s.els is not None:
            e = s.els
            if e.synthetic and len(e.items) == 1 and isinstance(e.items[0], If):
                sub = []
                ps(e.items[0], ind, sub)
                out.append(pad + "else " + sub[0].lstrip())
                out.extend(sub[1:])
            else:
                out.append(pad + "else")
                ps_body(e, ind, out)
    elif isinstance(s, While):
        out.append(pad + "while (" + pe(s.c, 0) + ")")
        ps_body(s.body, ind, out)
    elif isinstance(s, DoWhile):
        out.append(pad + "do")
        ps_body(s.body, ind, out, force=True)
        out.append(pad + "while (" + pe(s.c, 0) + ");")
    elif isinstance(s, For):
        a = pe(s.init, 0) if s.init is not None else ""
        b = pe(s.c, 0) if s.c is not None else ""
        c = pe(s.step, 0) if s.step is not None else ""
        out.append(pad + f"for ({a}; {b}; {c})".replace("; )", ";)").replace("( ;", "(;"))
        ps_body(s.body, ind, out)
    elif isinstance(s, Switch):
        out.append(pad + "switch (" + pe(s.c, 0) + ")")
        ps_body(s.body, ind, out, force=True)
    elif isinstance(s, Jump):
        if s.kind == "return":
            out.append(pad + ("return " + pe(s.e, 0) + ";" if s.e is not None else "return;"))
        elif s.kind == "goto":
            out.append(pad + "goto " + s.label + ";")
        else:
            out.append(pad + s.kind + ";")
    elif isinstance(s, Label):
        out.append("    " * max(ind - 1, 0) + s.text)
    elif isinstance(s, PP):
        out.append(s.text)
    elif isinstance(s, Empty):
        out.append(pad + ";")
    elif isinstance(s, OpaqueStmt):
        out.append(pad + s.text)
    else:
        raise TypeError(type(s))


def ps_body(c, ind, out, force=False):
    if c.synthetic and not force and len(c.items) == 1 and not isinstance(c.items[0], (Decl, Compound)):
        ps(c.items[0], ind + 1, out)
    else:
        ps(Compound(c.items), ind, out)


def print_body(body, nl):
    out = []
    ps(body, 0, out)
    return nl.join(out)


# --------------------------------------------------------------------------
# Source file / function handling
# --------------------------------------------------------------------------

_typedef_cache = None


def scan_typenames(extra_text=""):
    global _typedef_cache
    if _typedef_cache is None:
        names = set()
        pats = [re.compile(r"typedef\s+[^;{}]*?\b(\w+)\s*(\[[^\]]*\])?\s*;"),
                re.compile(r"}\s*(\w+)\s*;"),
                re.compile(r"typedef\s+[^;{}]*\(\s*\*\s*(\w+)\s*\)"),
                re.compile(r"#define\s+(\w+)\s+(?:float|int|unsigned\s+\w+|signed\s+\w+|char|short|long|double|void)\b")]
        dirs = [ROOT / "include", ROOT / "src" / "rwsdk", ROOT / "src" / "dolphin" / "include"]
        for d in dirs:
            for p in d.rglob("*.h"):
                try:
                    txt = p.read_text(errors="replace")
                except OSError:
                    continue
                for pat in pats:
                    for m in pat.finditer(txt):
                        names.add(m.group(1))
        names -= KEYWORDS
        _typedef_cache = names
    names = set(_typedef_cache)
    for pat in (re.compile(r"typedef\s+[^;{}]*?\b(\w+)\s*;"), re.compile(r"}\s*(\w+)\s*;")):
        for m in pat.finditer(extra_text):
            names.add(m.group(1))
    names |= {"u8", "u16", "u32", "s8", "s16", "s32", "f32", "f64", "BOOL", "size_t"}
    return names


def find_function(text, name):
    """Return (sig_start, body_open, body_close, params_text) of a function definition."""
    for m in re.finditer(r"\b" + re.escape(name) + r"\s*\(", text):
        # matching paren
        i = m.end() - 1
        depth = 0
        j = i
        while j < len(text):
            if text[j] == "(":
                depth += 1
            elif text[j] == ")":
                depth -= 1
                if depth == 0:
                    break
            j += 1
        k = j + 1
        while k < len(text) and text[k] in " \t\r\n":
            k += 1
        if k >= len(text) or text[k] != "{":
            continue
        # line start of signature: must be at file top level (line starts w/o indent)
        ls = text.rfind("\n", 0, m.start()) + 1
        if text[ls] in " \t":
            continue
        # body close
        toks_end = scan_braces(text, k)
        return ls, k, toks_end, text[i + 1:j]
    raise SystemExit(f"function {name} not found")


def scan_braces(text, k):
    depth = 0
    i = k
    n = len(text)
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            i = text.find("\n", i)
            continue
        if text.startswith("/*", i):
            i = text.find("*/", i) + 2
            continue
        if c in "\"'":
            j = i + 1
            while text[j] != c:
                if text[j] == "\\":
                    j += 1
                j += 1
            i = j + 1
            continue
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    raise SystemExit("unbalanced braces")


class Func:
    def __init__(self, name, params, body, ptypes):
        self.name = name
        self.params = params  # list of (name, typetext)
        self.body = body
        self.ptypes = ptypes


def parse_params(ptext, types):
    out = []
    ptext = ptext.strip()
    if ptext in ("", "void"):
        return out
    depth = 0
    cur = ""
    parts = []
    for ch in ptext:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    parts.append(cur)
    for p in parts:
        ids = re.findall(r"[A-Za-z_]\w*", p)
        if not ids:
            continue
        name = ids[-1]
        ty = p.strip()[: p.strip().rfind(name)].strip()
        out.append((name, ty))
    return out


def parse_function(text, name, types):
    ls, ob, cb, ptext = find_function(text, name)
    params = parse_params(ptext, types)
    toks = tokenize(text[ob:cb + 1])
    p = Parser(toks, types, {n for n, _ in params})
    body = p.parse_compound()
    if p.i != len(toks):
        raise SystemExit(f"{name}: parse stopped early")
    return Func(name, params, body, dict(params)), (ob, cb)


# --------------------------------------------------------------------------
# Generic AST walking
# --------------------------------------------------------------------------

def expr_children(e):
    out = []
    for a in EXPR_CHILDREN.get(type(e), ()):
        v = getattr(e, a)
        if a == "args":
            for k in range(len(v)):
                out.append((v, k))
        elif v is not None:
            out.append((e, a))
    return out


def sget(slot):
    o, k = slot
    return o[k] if isinstance(o, list) else getattr(o, k)


def sset(slot, v):
    o, k = slot
    if isinstance(o, list):
        o[k] = v
    else:
        setattr(o, k, v)


def walk_expr_slots(slot, ctx, out):
    """ctx: tuple of flags: 'cond' (short-circuit/conditional region), 'lval', 'addr'."""
    e = sget(slot)
    out.append((slot, ctx))
    if isinstance(e, Binary) and e.op in ("&&", "||"):
        walk_expr_slots((e, "l"), ctx, out)
        walk_expr_slots((e, "r"), ctx | {"sc"}, out)
        return
    if isinstance(e, Ternary):
        walk_expr_slots((e, "c"), ctx, out)
        walk_expr_slots((e, "a"), ctx | {"sc"}, out)
        walk_expr_slots((e, "b"), ctx | {"sc"}, out)
        return
    if isinstance(e, Assign):
        walk_expr_slots((e, "l"), ctx | {"lval"}, out)
        walk_expr_slots((e, "r"), ctx - {"lval", "addr"}, out)
        return
    if isinstance(e, (Postfix,)) or (isinstance(e, Unary) and e.op in ("++", "--")):
        walk_expr_slots((e, "e"), ctx | {"lval"}, out)
        return
    if isinstance(e, Unary) and e.op == "&":
        walk_expr_slots((e, "e"), ctx | {"addr"}, out)
        return
    if isinstance(e, Sizeof):
        if e.e is not None:
            walk_expr_slots((e, "e"), ctx | {"sizeof"}, out)
        return
    if isinstance(e, Call):
        walk_expr_slots((e, "f"), ctx | {"callee"}, out)
        for k in range(len(e.args)):
            walk_expr_slots((e.args, k), (ctx - {"lval", "addr"}) | {"arg"}, out)
        return
    if isinstance(e, Member):
        sub = ctx if e.op == "." else ctx - {"lval", "addr"}
        walk_expr_slots((e, "e"), sub, out)
        return
    if isinstance(e, Index):
        walk_expr_slots((e, "a"), ctx - {"lval", "addr"}, out)
        walk_expr_slots((e, "i"), ctx - {"lval", "addr"}, out)
        return
    for ch in expr_children(e):
        walk_expr_slots(ch, ctx - {"lval", "addr"}, out)


def stmt_expr_slots(s):
    """Top-level expression slots of a statement with 'once' flag."""
    if isinstance(s, ExprStmt):
        return [((s, "e"), True)]
    if isinstance(s, Jump) and s.e is not None:
        return [((s, "e"), True)]
    if isinstance(s, If):
        return [((s, "c"), True)]
    if isinstance(s, Switch):
        return [((s, "c"), True)]
    if isinstance(s, While):
        return [((s, "c"), False)]
    if isinstance(s, DoWhile):
        return [((s, "c"), False)]
    if isinstance(s, For):
        r = []
        if s.init is not None:
            r.append(((s, "init"), True))
        if s.c is not None:
            r.append(((s, "c"), False))
        if s.step is not None:
            r.append(((s, "step"), False))
        return r
    if isinstance(s, Decl):
        return [((d, "init"), True) for d in s.decls if d.init is not None]
    return []


def child_bodies(s):
    if isinstance(s, If):
        return [b for b in (s.then, s.els) if b is not None]
    if isinstance(s, (For, While, Switch)):
        return [s.body]
    if isinstance(s, DoWhile):
        return [s.body]
    if isinstance(s, Compound):
        return [s]
    return []


def all_compounds(body):
    out = []

    def rec(c):
        out.append(c)
        for it in c.items:
            for b in child_bodies(it):
                if b is it:
                    rec(b)
                else:
                    rec(b)
    rec(body)
    return out


class StmtInfo:
    __slots__ = ("s", "comp", "idx", "pre", "end", "anc", "loops")


def index_stmts(body):
    """Preorder statement index with parent compound, ancestors and enclosing loops."""
    infos = []
    by_id = {}
    counter = [0]

    def rec(c, anc, loops):
        for idx, it in enumerate(c.items):
            inf = StmtInfo()
            inf.s = it
            inf.comp = c
            inf.idx = idx
            inf.pre = counter[0]
            counter[0] += 1
            inf.anc = anc
            inf.loops = loops
            infos.append(inf)
            by_id[id(it)] = inf
            for b in child_bodies(it):
                if b is it:
                    rec(b, anc + [it], loops)
                else:
                    rec(b, anc + [it, b], loops + ([it] if isinstance(it, LOOPS) else []))
            inf.end = counter[0]
    rec(body, [body], [])
    return infos, by_id


# --------------------------------------------------------------------------
# Effects
# --------------------------------------------------------------------------

class Eff:
    __slots__ = ("r", "w", "mr", "mw", "call", "barrier", "fullw")

    def __init__(self):
        self.r = set()
        self.w = set()
        self.mr = False
        self.mw = False
        self.call = False
        self.barrier = False
        self.fullw = set()

    def merge(self, o):
        self.r |= o.r
        self.w |= o.w
        self.mr |= o.mr
        self.mw |= o.mw
        self.call |= o.call
        self.barrier |= o.barrier


class Ctx:
    """Per-function semantic info."""

    def __init__(self, func, types):
        self.func = func
        self.types = types
        self.vtypes = {}   # name -> (basetype, stars, suffix)
        self.addr = set()
        for n, t in func.params:
            m = re.match(r"(.*?)(\**)\s*$", t.replace(" *", "*"))
            base = re.sub(r"\s*\*+\s*$", "", t).strip()
            self.vtypes[n] = (base, t.count("*") * "*", "")
        self.has_goto = False
        self.refresh()

    def refresh(self):
        self.addr = set()
        self.has_goto = False
        body = self.func.body
        for c in all_compounds(body):
            for it in c.items:
                if isinstance(it, Decl):
                    for d in it.decls:
                        self.vtypes.setdefault(d.name, (it.ty, d.stars, d.suffix))
                        if d.suffix:
                            self.addr.add(d.name)
                if isinstance(it, (Label,)) or (isinstance(it, Jump) and it.kind == "goto"):
                    self.has_goto = True
                for slot, _ in stmt_expr_slots(it):
                    self._scan_addr(sget(slot))
                if isinstance(it, (OpaqueStmt,)):
                    for m in re.finditer(r"&\s*([A-Za-z_]\w*)", it.text):
                        self.addr.add(m.group(1))
                    # a macro may use (and take the address of) locals implicitly
                    for x in text_ids(it.text):
                        if x in self.vtypes:
                            self.addr.add(x)

    def _scan_addr(self, e):
        if e is None:
            return
        if isinstance(e, Unary) and e.op == "&":
            b = base_id(e.e)
            if b:
                self.addr.add(b)
        if isinstance(e, Opaque):
            for m in re.finditer(r"&\s*([A-Za-z_]\w*)", e.text):
                self.addr.add(m.group(1))
            for m in re.finditer(r"[A-Za-z_]\w*", e.text):
                pass
        for ch in expr_children(e):
            self._scan_addr(sget(ch))

    def is_local(self, n):
        return n in self.vtypes

    def scalar_local(self, n):
        return n in self.vtypes and n not in self.addr

    def is_float(self, e):
        e = strip_paren(e)
        if isinstance(e, Lit):
            return e.kind == "num" and ("." in e.text or e.text.lower().endswith("f")) and not e.text.lower().startswith("0x")
        if isinstance(e, Id):
            t = self.vtypes.get(e.name)
            return bool(t) and t[0].split()[-1] in FLOAT_TYPES and not t[1] and not t[2]
        if isinstance(e, Cast):
            return e.ty.split()[-1] in FLOAT_TYPES
        if isinstance(e, Binary) and e.op in "+-*/":
            return self.is_float(e.l) or self.is_float(e.r)
        if isinstance(e, Unary) and e.op == "-":
            return self.is_float(e.e)
        if isinstance(e, Call) and isinstance(e.f, Id) and e.f.name in ("RwRealAbs", "RwV3dDotProductMacro", "RwV3dDotProduct", "RwV3dLength"):
            return True
        if isinstance(e, Ternary):
            return self.is_float(e.a) or self.is_float(e.b)
        return False

    def is_int(self, e):
        e = strip_paren(e)
        if isinstance(e, Lit):
            return e.kind in ("num", "chr") and not self.is_float(e)
        if isinstance(e, Id):
            t = self.vtypes.get(e.name)
            return bool(t) and t[0].split()[-1] in INT_TYPES and not t[1] and not t[2]
        return False

    def decl_type(self, n):
        t = self.vtypes.get(n)
        if not t or t[2]:
            return None
        return (t[0] + (" " + t[1] if t[1] else "")).strip()

    def type_of(self, e):
        """Best-effort type text for a new temp; None -> use __typeof__."""
        e2 = strip_paren(e)
        if isinstance(e2, Id):
            return self.decl_type(e2.name)
        if isinstance(e2, Cast):
            return e2.ty
        if self.is_float(e2):
            return "RwReal"
        if isinstance(e2, Binary) and e2.op in ("+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>"):
            tl = self.type_of(e2.l) if not isinstance(strip_paren(e2.l), Lit) else "lit"
            tr = self.type_of(e2.r) if not isinstance(strip_paren(e2.r), Lit) else "lit"
            ts = {t for t in (tl, tr) if t != "lit"}
            if len(ts) == 1 and None not in ts and "*" not in next(iter(ts)):
                t = next(iter(ts))
                if t.split()[-1] in ("RwInt32", "RwUInt32", "int", "unsigned", "long", "size_t",
                                     "u32", "s32", "RwBool") and "const" not in t:
                    return t
        return None

    # -- effects
    def eff_expr(self, e, eff, write=False, full=False):
        if e is None:
            return
        if isinstance(e, Id):
            n = e.name
            if self.is_local(n):
                if n in self.addr:
                    if write:
                        eff.mw = True
                    else:
                        eff.mr = True
                (eff.w if write else eff.r).add(n)
                if write and full:
                    eff.fullw.add(n)
            else:
                if write:
                    eff.mw = True
                else:
                    eff.mr = True
            return
        if isinstance(e, Lit):
            return
        if isinstance(e, Opaque):
            eff.barrier = True
            return
        if isinstance(e, Paren):
            self.eff_expr(e.e, eff, write, full)
            return
        if isinstance(e, Assign):
            if e.op != "=":
                self.eff_expr(e.l, eff)
            self.eff_expr(e.r, eff)
            self.eff_lval(e.l, eff, full=(e.op == "="))
            return
        if isinstance(e, (Postfix,)) or (isinstance(e, Unary) and e.op in ("++", "--")):
            self.eff_expr(e.e, eff)
            self.eff_lval(e.e, eff)
            return
        if isinstance(e, Unary) and e.op == "&":
            b = base_id(e.e)
            if b and self.is_local(b):
                eff.r.add(b)
            self.eff_addr_sub(e.e, eff)
            return
        if isinstance(e, Unary) and e.op == "*":
            self.eff_expr(e.e, eff)
            if write:
                eff.mw = True
            else:
                eff.mr = True
            return
        if isinstance(e, Index):
            self.eff_expr(e.a, eff)
            self.eff_expr(e.i, eff)
            b = base_id(e.a)
            if write:
                eff.mw = True
                if b and self.is_local(b):
                    eff.w.add(b)
            else:
                eff.mr = True
            return
        if isinstance(e, Member):
            if e.op == ".":
                b = base_id(e)
                if b and self.scalar_local(b) and simple_dot_chain(e):
                    eff.r.add(b)
                    if write:
                        eff.w.add(b)
                    return
                self.eff_expr(e.e, eff, write, False)
                return
            self.eff_expr(e.e, eff)
            if write:
                eff.mw = True
            else:
                eff.mr = True
            return
        if isinstance(e, Call):
            fname = e.f.name if isinstance(e.f, Id) else None
            if fname in BARRIER_CALLS:
                eff.barrier = True
            if not (isinstance(e.f, Id) and not self.is_local(e.f.name)):
                self.eff_expr(e.f, eff)
            for a in e.args:
                self.eff_expr(a, eff)
                b = strip_paren(a)
                if isinstance(b, Id) and self.is_local(b.name) and fname not in PURE_CALLS:
                    eff.w.add(b.name)
            for x in MACRO_IDS.get(fname, ()):
                if self.is_local(x):
                    eff.r.add(x)
                    eff.w.add(x)
                    if x in self.addr:
                        eff.mr = eff.mw = True
            if fname in PURE_CALLS:
                return
            eff.call = True
            eff.mr = eff.mw = True
            return
        if isinstance(e, Sizeof):
            return
        for ch in expr_children(e):
            self.eff_expr(sget(ch), eff)

    def eff_addr_sub(self, e, eff):
        # &a.b[i] : evaluate index expressions only
        if isinstance(e, Index):
            self.eff_addr_sub(e.a, eff)
            self.eff_expr(e.i, eff)
        elif isinstance(e, Member):
            if e.op == "->":
                self.eff_expr(e.e, eff)
            else:
                self.eff_addr_sub(e.e, eff)
        elif isinstance(e, Paren):
            self.eff_addr_sub(e.e, eff)
        elif isinstance(e, Unary) and e.op == "*":
            self.eff_expr(e.e, eff)
        elif isinstance(e, Id):
            pass
        else:
            self.eff_expr(e, eff)

    def eff_lval(self, e, eff, full=False):
        self.eff_expr(e, eff, write=True, full=full)

    def eff_stmt(self, s):
        eff = Eff()
        if isinstance(s, (Jump, Label, PP, OpaqueStmt)):
            eff.barrier = True
            if isinstance(s, Jump) and s.e is not None:
                self.eff_expr(s.e, eff)
            return eff
        if isinstance(s, Empty):
            return eff
        if isinstance(s, Decl):
            for d in s.decls:
                if d.init is not None:
                    self.eff_expr(d.init, eff)
                eff.w.add(d.name)
                eff.fullw.add(d.name)
            return eff
        for slot, _ in stmt_expr_slots(s):
            self.eff_expr(sget(slot), eff)
        for b in child_bodies(s):
            for it in b.items:
                eff.merge(self.eff_stmt(it))
        if isinstance(s, Compound):
            for it in s.items:
                eff.merge(self.eff_stmt(it))
        return eff


def strip_paren(e):
    while isinstance(e, Paren):
        e = e.e
    return e


def base_id(e):
    while True:
        if isinstance(e, Id):
            return e.name
        if isinstance(e, (Paren,)):
            e = e.e
        elif isinstance(e, Member) and e.op == ".":
            e = e.e
        elif isinstance(e, Index):
            e = e.a
        else:
            return None


def simple_dot_chain(e):
    while isinstance(e, Member) and e.op == ".":
        e = e.e
    return isinstance(strip_paren(e), Id)


def independent(a, b):
    if a.barrier or b.barrier:
        return False
    if a.w & (b.r | b.w):
        return False
    if a.r & b.w:
        return False
    if a.mw and (b.mr or b.mw):
        return False
    if a.mr and b.mw:
        return False
    return True


MACRO_IDS = {}  # macro name -> identifiers its (transitive) expansion mentions


def scan_macros(text):
    """Collect #define bodies from the unit and the include tree (cached)."""
    global _macro_raw
    defs = {}
    pat = re.compile(r"^[ \t]*#[ \t]*define[ \t]+(\w+)(\([^)]*\))?((?:[^\n]*\\\r?\n)*[^\n]*)", re.M)
    if "_macro_raw" not in globals():
        raw = {}
        for d in [ROOT / "include", ROOT / "src" / "rwsdk"]:
            for p in d.rglob("*.h"):
                try:
                    txt = p.read_text(errors="replace")
                except OSError:
                    continue
                for m in pat.finditer(txt):
                    raw.setdefault(m.group(1), (m.group(2) or "", m.group(3)))
        _macro_raw = raw
    defs.update(_macro_raw)
    for m in pat.finditer(text):
        defs[m.group(1)] = (m.group(2) or "", m.group(3))
    direct = {}
    for name, (params, body) in defs.items():
        ps = set(re.findall(r"\w+", params))
        direct[name] = set(re.findall(r"[A-Za-z_]\w*", body)) - ps
    out = {}
    for name in direct:
        seen = set()
        stack = [name]
        ids = set()
        while stack:
            n = stack.pop()
            if n in seen:
                continue
            seen.add(n)
            for x in direct.get(n, ()):
                ids.add(x)
                if x in direct:
                    stack.append(x)
        out[name] = ids
    MACRO_IDS.clear()
    MACRO_IDS.update(out)


def text_ids(text):
    ids = set(re.findall(r"[A-Za-z_]\w*", text))
    for x in list(ids):
        ids |= MACRO_IDS.get(x, set())
    return ids


def ids_in(e, acc=None):
    acc = set() if acc is None else acc
    if e is None:
        return acc
    if isinstance(e, Id):
        acc.add(e.name)
        acc |= MACRO_IDS.get(e.name, set())
    elif isinstance(e, Opaque):
        acc |= text_ids(e.text)
    elif isinstance(e, Member):
        ids_in(e.e, acc)
        return acc
    for ch in expr_children(e):
        ids_in(sget(ch), acc)
    return acc


def stmt_ids(s):
    acc = set()
    if isinstance(s, (OpaqueStmt, PP)):
        return text_ids(s.text)
    if isinstance(s, Decl):
        for d in s.decls:
            acc.add(d.name)
            ids_in(d.init, acc)
        return acc
    for slot, _ in stmt_expr_slots(s):
        ids_in(sget(slot), acc)
    for b in child_bodies(s):
        for it in b.items:
            acc |= stmt_ids(it)
    if isinstance(s, Compound):
        for it in s.items:
            acc |= stmt_ids(it)
    return acc


def own_ids(s):
    """Identifiers accessed by the statement's own expressions (not nested bodies)."""
    acc = set()
    if isinstance(s, (OpaqueStmt, PP)):
        return text_ids(s.text)
    if isinstance(s, Decl):
        for d in s.decls:
            ids_in(d.init, acc)
        return acc
    for slot, _ in stmt_expr_slots(s):
        ids_in(sget(slot), acc)
    return acc


def has_side_effects(e):
    if e is None:
        return False
    if isinstance(e, (Assign, Postfix, Opaque)):
        return True
    if isinstance(e, Unary) and e.op in ("++", "--"):
        return True
    if isinstance(e, Call):
        if not (isinstance(e.f, Id) and e.f.name in PURE_CALLS):
            return True
    return any(has_side_effects(sget(ch)) for ch in expr_children(e))


def has_deref(e):
    if e is None:
        return False
    if isinstance(e, (Index,)) or (isinstance(e, Unary) and e.op == "*") or (isinstance(e, Member) and e.op == "->"):
        return True
    return any(has_deref(sget(ch)) for ch in expr_children(e))


def contains(s, pred):
    if pred(s):
        return True
    for b in child_bodies(s):
        if b is not s and contains(b, pred):
            return True
    if isinstance(s, Compound):
        return any(contains(it, pred) for it in s.items)
    return False


def has_jump(s):
    return contains(s, lambda x: isinstance(x, (Jump, Label, PP, OpaqueStmt)) or
                    (isinstance(x, ExprStmt) and calls_barrier(x.e)))


def calls_barrier(e):
    if isinstance(e, Call) and isinstance(e.f, Id) and e.f.name in BARRIER_CALLS:
        return True
    return any(calls_barrier(sget(ch)) for ch in expr_children(e)) if e is not None else False


def stmt_eff(ctx, s):
    eff = ctx.eff_stmt(s)
    if not isinstance(s, (Decl, ExprStmt, Empty)) and has_jump(s):
        eff.barrier = True
    return eff


# --------------------------------------------------------------------------
# Liveness helpers (structured, conservative)
# --------------------------------------------------------------------------

def accesses(ctx, infos, v):
    """List of (info, kind) where kind in {'r','w','fw'} for each statement whose own exprs touch v."""
    out = []
    for inf in infos:
        s = inf.s
        if isinstance(s, (OpaqueStmt, PP)):
            if v in own_ids(s):
                out.append((inf, "r"))
            continue
        if isinstance(s, Decl):
            for d in s.decls:
                if d.name == v:
                    out.append((inf, "decl"))
                if d.init is not None and v in ids_in(d.init):
                    out.append((inf, "r"))
            continue
        if v not in own_ids(s):
            continue
        kind = "r"
        if isinstance(s, ExprStmt) and isinstance(s.e, Assign) and s.e.op == "=" \
                and isinstance(strip_paren(s.e.l), Id) and strip_paren(s.e.l).name == v \
                and v not in ids_in(s.e.r):
            kind = "fw"
        out.append((inf, kind))
    return out


def inside(inf, node):
    return node in inf.anc or inf.s is node


def dead_from(ctx, infos, v, pos_inf, exclude=()):
    """True if v's value is not read at/after the point just before statement pos_inf
    (excluding statements in `exclude`), so v may be clobbered there."""
    if ctx.has_goto or not ctx.scalar_local(v):
        return False
    acc = [(i, k) for (i, k) in accesses(ctx, infos, v) if i.s not in exclude and k != "decl"]
    for lp in pos_inf.loops:
        ins = [(i, k) for (i, k) in acc if inside(i, lp)]
        if not ins:
            continue
        # value may flow round the back edge: the loop header must not touch v and
        # the first access in the body must be a top-level full write (it dominates
        # every other access of the next iteration)
        if v in own_ids(lp):
            return False
        f_inf, f_kind = min(ins, key=lambda x: x[0].pre)
        if f_kind != "fw" or f_inf.comp is not lp.body:
            return False
    after = [(i, k) for (i, k) in acc if i.pre >= pos_inf.pre]
    if not after:
        return True
    first, kind = min(after, key=lambda x: x[0].pre)
    if kind != "fw":
        return False
    # first must dominate all other after-accesses: all in first.comp items after first.idx
    comp = first.comp
    later = comp.items[first.idx + 1:]
    for i, k in after:
        if i is first:
            continue
        if not any(inside(i, it) for it in later):
            return False
    # and pos must not be inside first.comp's later items (it is before first textually)
    return True


# --------------------------------------------------------------------------
# Mutations
# --------------------------------------------------------------------------

class Mut:
    def __init__(self, func, types, rng):
        self.f = func
        self.ctx = Ctx(func, types)
        self.rng = rng
        self.infos, self.by_id = index_stmts(func.body)
        self.tmpn = 0
        names = set(self.ctx.vtypes)
        while f"_t{self.tmpn}" in names:
            self.tmpn += 1

    def newname(self):
        names = set(self.ctx.vtypes)
        n = 0
        while f"_t{n}" in names:
            n += 1
        return f"_t{n}"

    def pick(self, xs):
        return self.rng.choice(xs) if xs else None

    # ---- statement swaps
    def m_swap(self):
        cands = []
        for c in all_compounds(self.f.body):
            for i in range(len(c.items) - 1):
                a, b = c.items[i], c.items[i + 1]
                if isinstance(a, Decl) != isinstance(b, Decl):
                    continue
                cands.append((c, i))
        self.rng.shuffle(cands)
        for c, i in cands:
            a, b = c.items[i], c.items[i + 1]
            if independent(stmt_eff(self.ctx, a), stmt_eff(self.ctx, b)):
                if isinstance(a, Decl) and (stmt_ids(a) & stmt_ids(b)):
                    continue
                c.items[i], c.items[i + 1] = b, a
                return "swap"
        return None

    def m_move(self):
        cands = []
        for c in all_compounds(self.f.body):
            for i in range(len(c.items)):
                cands.append((c, i))
        self.rng.shuffle(cands)
        for c, i in cands[:30]:
            s = c.items[i]
            es = stmt_eff(self.ctx, s)
            if es.barrier:
                continue
            d = self.rng.choice((-1, 1))
            j = i
            moved = 0
            limit = self.rng.randint(2, 6)
            while moved < limit:
                k = j + d
                if k < 0 or k >= len(c.items):
                    break
                o = c.items[k]
                if isinstance(o, Decl) != isinstance(s, Decl):
                    break
                if not independent(es, stmt_eff(self.ctx, o)):
                    break
                if isinstance(s, Decl) and (stmt_ids(s) & stmt_ids(o)):
                    break
                j = k
                moved += 1
            if moved >= 2:
                c.items.pop(i)
                c.items.insert(j, s)
                return "move"
        return None

    # ---- declaration placement
    def decl_region_end(self, c):
        n = 0
        while n < len(c.items) and isinstance(c.items[n], (Decl, PP)):
            n += 1
        return n

    def m_declin(self):
        cands = []
        for c in all_compounds(self.f.body):
            for i, it in enumerate(c.items):
                if isinstance(it, Decl) and len(it.decls) == 1 and it.decls[0].init is None:
                    cands.append((c, i))
        self.rng.shuffle(cands)
        for c, i in cands:
            d = c.items[i]
            v = d.decls[0].name
            users = [it for it in c.items if it is not d and v in stmt_ids(it)]
            if len(users) != 1:
                continue
            u = users[0]
            # descend into the smallest body containing all uses
            target = None
            for b in child_bodies(u):
                if b is u:
                    continue
                if any(v in stmt_ids(x) for x in b.items):
                    if target is not None:
                        target = None
                        break
                    target = b
            if target is None:
                continue
            # u's own expressions must not use v
            if v in own_ids(u):
                continue
            if any(isinstance(x, Decl) and any(dd.name == v for dd in x.decls) for x in target.items):
                continue
            # if target is a loop body, first access must be a top-level full write
            if isinstance(u, LOOPS):
                first = None
                for x in target.items:
                    if v in stmt_ids(x):
                        first = x
                        break
                if not (isinstance(first, ExprStmt) and isinstance(first.e, Assign) and first.e.op == "="
                        and isinstance(strip_paren(first.e.l), Id) and strip_paren(first.e.l).name == v
                        and v not in ids_in(first.e.r)):
                    continue
            c.items.pop(i)
            pos = self.rng.randint(0, self.decl_region_end(target))
            target.items.insert(pos, d)
            target.synthetic = False
            return "declin"
        return None

    def m_declout(self):
        cands = []
        for inf in self.infos:
            if isinstance(inf.s, Decl) and len(inf.s.decls) == 1 and inf.s.decls[0].init is None \
                    and inf.comp is not self.f.body:
                cands.append(inf)
        self.rng.shuffle(cands)
        for inf in cands:
            d = inf.s
            v = d.decls[0].name
            # enclosing compound: nearest Compound ancestor above inf.comp
            outer = None
            for a in reversed(inf.anc[:-1]):
                if isinstance(a, Compound):
                    outer = a
                    break
            if outer is None:
                continue
            # v must not be referenced in outer outside inf.comp
            holder = None
            for it in outer.items:
                if contains(it, lambda x: x is inf.comp):
                    holder = it
            if holder is None:
                continue
            bad = False
            for it in outer.items:
                if it is holder:
                    continue
                if v in stmt_ids(it):
                    bad = True
            if v in own_ids(holder):
                bad = True
            # other bodies of holder
            for b in child_bodies(holder):
                if b is not inf.comp and b is not holder and any(v in stmt_ids(x) for x in b.items):
                    bad = True
            if bad:
                continue
            inf.comp.items.remove(d)
            pos = self.rng.randint(0, self.decl_region_end(outer))
            outer.items.insert(pos, d)
            return "declout"
        return None

    # ---- expression helpers
    def expr_sites(self, once_only=True):
        """(stmt_info, slot, ctxflags) for expression nodes."""
        out = []
        for inf in self.infos:
            for top, once in stmt_expr_slots(inf.s):
                if once_only and not once:
                    continue
                lst = []
                walk_expr_slots(top, frozenset(), lst)
                for slot, fl in lst:
                    out.append((inf, slot, fl, top))
        return out

    def all_expr_sites(self):
        return self.expr_sites(once_only=False)

    def hoistable(self, inf, slot, fl, top):
        e = sget(slot)
        if fl & {"sc", "lval", "addr", "sizeof", "callee"}:
            return False
        if isinstance(e, (Id, Lit, Opaque, Paren)) or isinstance(strip_paren(e), (Id, Lit)):
            return False
        if isinstance(e, Lit) or (isinstance(e, Unary) and e.op == "&"):
            return False
        if has_side_effects(e):
            return False
        if isinstance(inf.s, For) and slot[1] != "init" and slot[0] is inf.s:
            return False
        if isinstance(inf.s, For) and top[1] != "init":
            return False
        if isinstance(inf.s, (While, DoWhile)):
            return False
        # struct-valued things: skip address-y & aggregate exprs
        if isinstance(e, (Member,)) and not self.ctx.is_float(e):
            # could be struct valued; allow only via __typeof__ (fine) - but avoid
            # obviously aggregate copies on assignment of whole structs
            pass
        whole = sget(top)
        if whole is e:
            # the whole top-level expression: only meaningful for If/Return/Decl/args
            if isinstance(inf.s, ExprStmt):
                return False
        eff = Eff()
        self.ctx.eff_expr(e, eff)
        if eff.barrier:
            return False
        weff = Eff()
        self.ctx.eff_expr(whole, weff)
        # anything else in the statement writing what e reads?
        if weff.call or weff.mw:
            if eff.mr:
                return False
            # e's locals written in the statement?
        if isinstance(whole, Assign):
            other = Eff()
            self.ctx.eff_lval(whole.l, other)
            if other.w & eff.r and not (isinstance(strip_paren(whole.l), Id)):
                return False
        # any local write inside the statement (besides top-level assignment target)
        inner_w = set()
        self._inner_writes(whole, inner_w, top_level=True)
        if inner_w & eff.r:
            return False
        if isinstance(e, Ternary) or isinstance(e, Binary) and e.op in ("&&", "||"):
            if has_deref(e):
                return False
        return True

    def _inner_writes(self, e, acc, top_level=False):
        if e is None:
            return
        if isinstance(e, Assign) and not top_level:
            b = base_id(e.l)
            if b:
                acc.add(b)
        if isinstance(e, (Postfix,)) or (isinstance(e, Unary) and e.op in ("++", "--")):
            b = base_id(e.e)
            if b:
                acc.add(b)
        if isinstance(e, Call):
            for a in e.args:
                b = strip_paren(a)
                if isinstance(b, Id):
                    acc.add(b.name)
        if isinstance(e, Assign) and top_level:
            self._inner_writes(e.r, acc)
            self._inner_writes(e.l, acc)
            return
        for ch in expr_children(e):
            self._inner_writes(sget(ch), acc)

    def insert_before(self, inf, stmts):
        c = inf.comp
        idx = c.items.index(inf.s)
        for k, s in enumerate(stmts):
            c.items.insert(idx + k, s)
        c.synthetic = False if len(c.items) > 1 else c.synthetic

    def add_decl(self, comp, ty, name, at=None):
        if ty.startswith("__typeof__") or not ty:
            pass
        m = re.match(r"^(.*?)(\s*\*+)$", ty)
        if m:
            d = Decl(m.group(1).strip(), [Declarator(m.group(2).strip(), name, "", None)])
        else:
            d = Decl(ty, [Declarator("", name, "", None)])
        pos = self.decl_region_end(comp) if at is None else at
        comp.items.insert(pos, d)
        comp.synthetic = False
        self.ctx.vtypes[name] = (d.ty, d.decls[0].stars, "")
        return d

    def m_temp(self, reuse=False):
        sites = self.expr_sites()
        self.rng.shuffle(sites)
        for inf, slot, fl, top in sites[:60]:
            if not self.hoistable(inf, slot, fl, top):
                continue
            e = sget(slot)
            if reuse:
                if isinstance(inf.s, Decl):
                    continue
                cands = []
                for v, (bt, st, sf) in self.ctx.vtypes.items():
                    if not self.ctx.scalar_local(v) or sf:
                        continue
                    if v in stmt_ids(inf.s):
                        continue
                    if v in dict(self.f.params):
                        continue
                    ty = (bt + (" " + st if st else "")).strip()
                    if self.ctx.is_float(e):
                        if bt.split()[-1] not in FLOAT_TYPES or st:
                            continue
                    elif self.ctx.is_int(e):
                        if bt.split()[-1] not in INT_TYPES or st:
                            continue
                    else:
                        et = self.ctx.type_of(e)
                        if et is None or et.replace(" ", "") != ty.replace(" ", ""):
                            continue
                    # v must be visible at inf: declared in an ancestor compound
                    if not self.visible(v, inf):
                        continue
                    if not dead_from(self.ctx, self.infos, v, inf):
                        continue
                    cands.append(v)
                if not cands:
                    continue
                v = self.rng.choice(cands)
                sset(slot, Id(v))
                self.insert_before(inf, [ExprStmt(Assign("=", Id(v), e))])
                return "reuse"
            name = self.newname()
            ty = self.ctx.type_of(e)
            if PLAIN_TEMPS:
                if ty is None:
                    continue
            elif ty is None or self.rng.random() < 0.2:
                ty = "__typeof__(" + pe(e, 0) + ")"
            if isinstance(inf.s, Decl):
                d = self.add_decl(inf.comp, ty, name, at=inf.comp.items.index(inf.s))
                d.decls[0].init = e
                sset(slot, Id(name))
                return "temp"
            sset(slot, Id(name))
            self.add_decl(inf.comp, ty, name)
            self.insert_before(self.by_id.get(id(inf.s)) or inf, [ExprStmt(Assign("=", Id(name), e))])
            return "temp"
        return None

    def visible(self, v, inf):
        for a in inf.anc:
            if isinstance(a, Compound):
                for it in a.items:
                    if isinstance(it, Decl) and any(d.name == v for d in it.decls):
                        return True
        return v in dict(self.f.params)

    def m_reuse(self):
        return self.m_temp(reuse=True)

    def m_inline(self):
        cands = []
        for c in all_compounds(self.f.body):
            for i in range(len(c.items) - 1):
                a = c.items[i]
                if isinstance(a, ExprStmt) and isinstance(a.e, Assign) and a.e.op == "=" \
                        and isinstance(strip_paren(a.e.l), Id):
                    cands.append((c, i))
                if isinstance(a, Decl) and len(a.decls) == 1 and a.decls[0].init is not None \
                        and not isinstance(a.decls[0].init, Opaque) and not a.decls[0].suffix:
                    cands.append((c, i))
        self.rng.shuffle(cands)
        for c, i in cands:
            a, b = c.items[i], c.items[i + 1]
            if isinstance(a, Decl):
                v = a.decls[0].name
                val = a.decls[0].init
                if isinstance(b, Decl) is False and i + 1 != self.decl_region_end(c):
                    continue
            else:
                v = strip_paren(a.e.l).name
                val = a.e.r
            if not self.ctx.scalar_local(v) or has_side_effects(val) or v in ids_in(val):
                continue
            if isinstance(b, Decl):
                if len(b.decls) != 1 or b.decls[0].init is None:
                    continue
            elif not isinstance(b, (ExprStmt, If, Jump, Switch)) or (isinstance(b, Jump) and b.e is None):
                continue
            # exactly one occurrence of v in b's own top-level expression
            occ = []
            for top, once in stmt_expr_slots(b):
                if not once:
                    continue
                lst = []
                walk_expr_slots(top, frozenset(), lst)
                for slot, fl in lst:
                    if isinstance(sget(slot), Id) and sget(slot).name == v:
                        occ.append((slot, fl, top))
            if len(occ) != 1 or v in stmt_ids(b) - own_ids(b) or sum(1 for x in [b] if v in stmt_ids(x)) != 1:
                continue
            if v in stmt_ids(b) and any(v in stmt_ids(x) for x in child_bodies(b) if x is not b):
                continue
            slot, fl, top = occ[0]
            if fl & {"lval", "addr", "sizeof", "callee"}:
                continue
            if "sc" in fl and has_deref(val):
                continue
            veff = Eff()
            self.ctx.eff_expr(val, veff)
            if veff.barrier:
                continue
            inner_w = set()
            self._inner_writes(sget(top), inner_w, top_level=True)
            if inner_w & veff.r:
                continue
            beff = Eff()
            self.ctx.eff_expr(sget(top), beff)
            if veff.mr and (beff.call or beff.mw):
                continue
            inf_b = self.by_id[id(b)]
            if not dead_from(self.ctx, self.infos, v, inf_b, exclude=(a, b)) or \
                    not self.no_other_reads_after(v, a, b):
                continue
            sset(slot, Paren(val) if eprec(val) < 15 else val)
            if isinstance(a, Decl):
                a.decls[0].init = None
                if self.rng.random() < 0.7 and not any(v in stmt_ids(x) for x in self.all_items() if x is not a):
                    c.items.remove(a)
            else:
                c.items.remove(a)
                if self.rng.random() < 0.7:
                    self.drop_unused_decl(v)
            return "inline"
        return None

    def no_other_reads_after(self, v, a, b):
        # dead_from handles textual after b; also make sure nothing between (adjacent)
        return True

    def all_items(self):
        out = []
        for c in all_compounds(self.f.body):
            out.extend(c.items)
        return out

    def drop_unused_decl(self, v):
        for c in all_compounds(self.f.body):
            for it in list(c.items):
                if isinstance(it, Decl) and len(it.decls) == 1 and it.decls[0].name == v \
                        and it.decls[0].init is None:
                    if not any(v in stmt_ids(x) for x in self.all_items() if x is not it):
                        c.items.remove(it)
                        return

    # ---- expression rewrites
    def m_commute(self):
        sites = self.all_expr_sites()
        self.rng.shuffle(sites)
        for inf, slot, fl, top in sites:
            e = sget(slot)
            if isinstance(e, Binary):
                if e.op in ("+", "*", "&", "|", "^", "==", "!="):
                    if not self.commutable(e.l, e.r):
                        continue
                    # don't break pointer arithmetic / left-assoc chains: printer handles parens
                    e.l, e.r = e.r, e.l
                    return "commute"
                mirror = {"<": ">", ">": "<", "<=": ">=", ">=": "<="}
                if e.op in mirror:
                    if not self.commutable(e.l, e.r):
                        continue
                    sset(slot, Binary(mirror[e.op], e.r, e.l))
                    return "commute"
        return None

    def commutable(self, l, r):
        """Evaluation-order safety for swapping two operands."""
        sl, sr = has_side_effects(l), has_side_effects(r)
        if not sl and not sr:
            return True
        if sl and sr:
            return False
        other = r if sl else l
        oe = Eff()
        self.ctx.eff_expr(other, oe)
        se = Eff()
        self.ctx.eff_expr(l if sl else r, se)
        return not oe.mr and not (se.w & oe.r) and not oe.barrier

    def m_compound(self):
        sites = self.all_expr_sites()
        self.rng.shuffle(sites)
        for inf, slot, fl, top in sites:
            e = sget(slot)
            if isinstance(e, Assign) and e.op != "=" and not has_side_effects(e.l) \
                    and e.op[:-1] in BINPREC:
                sset(slot, Assign("=", e.l, Binary(e.op[:-1], copy.deepcopy(e.l), e.r)))
                return "compound"
            if isinstance(e, Assign) and e.op == "=" and isinstance(e.r, Binary) \
                    and e.r.op in ("+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>") \
                    and pe(e.r.l) == pe(e.l) and not has_side_effects(e.l):
                sset(slot, Assign(e.r.op + "=", e.l, e.r.r))
                return "compound"
        return None

    def m_cast(self):
        sites = self.all_expr_sites()
        self.rng.shuffle(sites)
        for inf, slot, fl, top in sites:
            e = sget(slot)
            if fl & {"lval", "addr", "callee", "sizeof"}:
                continue
            if isinstance(e, Cast):
                inner = strip_paren(e.e)
                if isinstance(inner, Id) and self.ctx.decl_type(inner.name) and \
                        self.ctx.decl_type(inner.name).replace(" ", "") == e.ty.replace(" ", ""):
                    sset(slot, e.e)
                    return "cast-"
                continue
            if isinstance(e, Id) and self.ctx.decl_type(e.name) and self.rng.random() < 0.5:
                sset(slot, Cast(self.ctx.decl_type(e.name), e))
                return "cast+"
        return None

    def m_wrapreal(self):
        sites = self.all_expr_sites()
        self.rng.shuffle(sites)
        for inf, slot, fl, top in sites:
            e = sget(slot)
            if fl & {"lval", "addr", "callee", "sizeof"}:
                continue
            if isinstance(e, Cast) and e.ty == "RwReal" and isinstance(strip_paren(e.e), Binary) \
                    and strip_paren(e.e).op in ("*", "/", "+", "-") and self.ctx.is_float(e.e):
                sset(slot, strip_paren(e.e))
                return "unwrapreal"
            if isinstance(e, Binary) and e.op in ("*",) and self.ctx.is_float(e):
                sset(slot, Cast("RwReal", Paren(e)))
                return "wrapreal"
        return None

    def m_ptrform(self):
        sites = self.all_expr_sites()
        self.rng.shuffle(sites)
        for inf, slot, fl, top in sites:
            e = sget(slot)
            if isinstance(e, Unary) and e.op == "*" and "addr" not in fl:
                inner = strip_paren(e.e)
                if isinstance(inner, Binary) and inner.op == "+" and self.is_ptr(inner.l):
                    sset(slot, Index(inner.l, inner.r))
                    return "ptrform"
                if self.is_ptr(e.e):
                    sset(slot, Index(e.e, Lit("0", "num")))
                    return "ptrform"
            if isinstance(e, Index) and self.is_ptr(e.a):
                if isinstance(e.i, Lit) and e.i.text == "0":
                    sset(slot, Unary("*", e.a))
                else:
                    sset(slot, Unary("*", Paren(Binary("+", e.a, e.i))))
                return "ptrform"
        # statement-level increments
        for inf in self.infos:
            s = inf.s
            if not isinstance(s, ExprStmt):
                continue
            e = s.e
            tgt = None
            if isinstance(e, (Postfix, Unary)) and e.op in ("++", "--") and isinstance(strip_paren(e.e), Id):
                tgt = (e.e, "+" if e.op == "++" else "-")
            elif isinstance(e, Assign) and e.op in ("+=", "-=") and isinstance(e.r, Lit) and e.r.text == "1":
                tgt = (e.l, e.op[0])
            elif isinstance(e, Assign) and e.op == "=" and isinstance(e.r, Binary) and e.r.op in "+-" \
                    and isinstance(e.r.r, Lit) and e.r.r.text == "1" and pe(e.r.l) == pe(e.l):
                tgt = (e.l, e.r.op)
            if tgt is None or self.rng.random() < 0.6:
                continue
            lv, op = tgt
            forms = [Postfix(op * 2, lv), Unary(op * 2, lv), Assign(op + "=", lv, Lit("1", "num")),
                     Assign("=", lv, Binary(op, copy.deepcopy(lv), Lit("1", "num")))]
            cur = pe(e)
            forms = [f for f in forms if pe(f) != cur]
            s.e = self.rng.choice(forms)
            return "incdec"
        return None

    def is_ptr(self, e):
        e = strip_paren(e)
        if isinstance(e, Id):
            t = self.ctx.vtypes.get(e.name)
            return bool(t) and bool(t[1]) and not t[2]
        return False

    def m_not(self):
        sites = self.all_expr_sites()
        self.rng.shuffle(sites)
        for inf, slot, fl, top in sites:
            e = sget(slot)
            is_cond_ctx = self.is_cond_slot(slot, top, inf)
            if isinstance(e, Unary) and e.op == "!":
                zero = "NULL" if self.is_ptr(e.e) else "0"
                sset(slot, Binary("==", e.e, Id(zero) if zero == "NULL" else Lit("0", "num")))
                return "not"
            if isinstance(e, Binary) and e.op in ("==", "!=") and isinstance(e.r, (Lit, Id)) \
                    and pe(e.r) in ("0", "NULL") and not self.ctx.is_float(e.l):
                if e.op == "==":
                    sset(slot, Unary("!", e.l))
                    return "not"
                if is_cond_ctx:
                    sset(slot, e.l)
                    return "not"
            if is_cond_ctx and not isinstance(e, (Binary, Unary)) and (self.is_ptr(e) or self.ctx.is_int(e)):
                zero = Id("NULL") if self.is_ptr(e) else Lit("0", "num")
                sset(slot, Binary("!=", e, zero))
                return "not"
        return None

    def is_cond_slot(self, slot, top, inf):
        if slot is top and slot[1] == "c" and isinstance(inf.s, (If, While, DoWhile, For)):
            return True
        o, k = slot
        if isinstance(o, Binary) and o.op in ("&&", "||"):
            return True
        if isinstance(o, Ternary) and k == "c":
            return True
        if isinstance(o, Unary) and o.op == "!":
            return True
        return False

    def negate(self, c):
        c0 = strip_paren(c)
        if isinstance(c0, Unary) and c0.op == "!":
            return c0.e
        inv = {"==": "!=", "!=": "==", "<": ">=", ">=": "<", ">": "<=", "<=": ">"}
        if isinstance(c0, Binary) and c0.op in inv and (c0.op in ("==", "!=") or
                                                       not (self.ctx.is_float(c0.l) or self.ctx.is_float(c0.r))):
            return Binary(inv[c0.op], c0.l, c0.r)
        return Unary("!", c if eprec(c) >= 14 else Paren(c))

    def m_ifswap(self):
        cands = [inf for inf in self.infos if isinstance(inf.s, If) and inf.s.els is not None]
        x = self.pick(cands)
        if not x:
            return None
        s = x.s
        s.c = self.negate(s.c)
        s.then, s.els = s.els, s.then
        return "ifswap"

    def single_assign(self, comp):
        if comp is None or len(comp.items) != 1:
            return None
        s = comp.items[0]
        if isinstance(s, ExprStmt) and isinstance(s.e, Assign) and s.e.op == "=" and not has_side_effects(s.e.l):
            return s.e
        return None

    def m_ternary(self):
        forms = []
        for c in all_compounds(self.f.body):
            for i, it in enumerate(c.items):
                if isinstance(it, If) and it.els is not None:
                    a = self.single_assign(it.then)
                    b = self.single_assign(it.els)
                    if a and b and pe(a.l) == pe(b.l):
                        forms.append(("if2tern", c, i))
                        forms.append(("if2pre", c, i))
                if isinstance(it, ExprStmt) and isinstance(it.e, Assign) and it.e.op == "=" \
                        and isinstance(strip_paren(it.e.r), Ternary) and not has_side_effects(it.e.l):
                    forms.append(("tern2if", c, i))
                if isinstance(it, ExprStmt) and isinstance(it.e, Assign) and it.e.op == "=" \
                        and i + 1 < len(c.items) and isinstance(c.items[i + 1], If) \
                        and c.items[i + 1].els is None:
                    a = self.single_assign(c.items[i + 1].then)
                    if a and pe(a.l) == pe(it.e.l):
                        forms.append(("pre2if", c, i))
        self.rng.shuffle(forms)
        for kind, c, i in forms:
            it = c.items[i]
            if kind == "if2tern":
                a = self.single_assign(it.then)
                b = self.single_assign(it.els)
                c.items[i] = ExprStmt(Assign("=", a.l, Ternary(it.c, a.r, b.r)))
                return kind
            if kind == "if2pre":
                a = self.single_assign(it.then)
                b = self.single_assign(it.els)
                lv = base_id(a.l)
                if lv is None or lv in ids_in(it.c) or lv in ids_in(a.r) or has_side_effects(b.r) \
                        or has_side_effects(it.c) or has_deref(b.r):
                    continue
                if not isinstance(strip_paren(a.l), Id):
                    continue
                c.items[i:i + 1] = [ExprStmt(Assign("=", b.l, b.r)),
                                    If(it.c, Compound([ExprStmt(Assign("=", a.l, a.r))], True), None)]
                return kind
            if kind == "tern2if":
                t = strip_paren(it.e.r)
                c.items[i] = If(strip_paren(t.c), Compound([ExprStmt(Assign("=", it.e.l, t.a))], True),
                                Compound([ExprStmt(Assign("=", copy.deepcopy(it.e.l), t.b))], True))
                return kind
            if kind == "pre2if":
                nx = c.items[i + 1]
                a = self.single_assign(nx.then)
                lv = base_id(a.l)
                if lv is None or not isinstance(strip_paren(a.l), Id) or lv in ids_in(nx.c) \
                        or lv in ids_in(a.r) or has_side_effects(it.e.r) or lv in ids_in(it.e.r) \
                        or has_side_effects(nx.c):
                    continue
                if self.rng.random() < 0.5:
                    c.items[i:i + 2] = [If(nx.c, nx.then, Compound([it], True))]
                else:
                    c.items[i:i + 2] = [ExprStmt(Assign("=", a.l, Ternary(nx.c, a.r, it.e.r)))]
                return kind
        return None

    def m_splitdecl(self):
        cands = []
        for c in all_compounds(self.f.body):
            end = self.decl_region_end(c)
            for i in range(end):
                it = c.items[i]
                if isinstance(it, Decl):
                    for k, d in enumerate(it.decls):
                        if d.init is not None and not isinstance(d.init, Opaque) and not d.suffix:
                            cands.append(("split", c, i, k))
            if end < len(c.items):
                st = c.items[end]
                if isinstance(st, ExprStmt) and isinstance(st.e, Assign) and st.e.op == "=" \
                        and isinstance(strip_paren(st.e.l), Id):
                    cands.append(("merge", c, end, 0))
        self.rng.shuffle(cands)
        for kind, c, i, k in cands:
            end = self.decl_region_end(c)
            if kind == "split":
                it = c.items[i]
                d = it.decls[k]
                if not self.ctx.scalar_local(d.name) and d.name in self.ctx.addr:
                    pass
                # later decl inits must not use d.name and must be independent of init
                ie = Eff()
                self.ctx.eff_expr(d.init, ie)
                if ie.barrier:
                    continue
                ok = True
                later = [dd.init for dd in it.decls[k + 1:]]
                for x in c.items[i + 1:end]:
                    if isinstance(x, Decl):
                        later += [dd.init for dd in x.decls]
                for li in later:
                    if li is None:
                        continue
                    if d.name in ids_in(li):
                        ok = False
                        break
                    le = Eff()
                    self.ctx.eff_expr(li, le)
                    if not independent(ie, le):
                        ok = False
                        break
                if not ok:
                    continue
                init = d.init
                d.init = None
                c.items.insert(end, ExprStmt(Assign("=", Id(d.name), init)))
                return "splitdecl"
            else:
                st = c.items[i]
                v = strip_paren(st.e.l).name
                pos = None
                for j in range(end):
                    x = c.items[j]
                    if isinstance(x, Decl) and len(x.decls) == 1 and x.decls[0].name == v \
                            and x.decls[0].init is None and not x.decls[0].suffix:
                        pos = j
                if pos is None:
                    continue
                val = st.e.r
                ve = Eff()
                self.ctx.eff_expr(val, ve)
                if ve.barrier or v in ids_in(val):
                    continue
                # names declared after pos must not be used in val; later inits independent
                ok = True
                for x in c.items[pos + 1:end]:
                    if isinstance(x, Decl):
                        for dd in x.decls:
                            if dd.name in ids_in(val):
                                ok = False
                            if dd.init is not None:
                                le = Eff()
                                self.ctx.eff_expr(dd.init, le)
                                if not independent(ve, le) or v in ids_in(dd.init):
                                    ok = False
                if not ok:
                    continue
                c.items[pos].decls[0].init = val
                c.items.pop(i)
                return "mergedecl"
        return None

    def m_splitmulti(self):
        cands = []
        for c in all_compounds(self.f.body):
            for i, it in enumerate(c.items):
                if isinstance(it, Decl) and len(it.decls) > 1:
                    cands.append(("split", c, i))
                if isinstance(it, Decl) and i + 1 < len(c.items) and isinstance(c.items[i + 1], Decl) \
                        and c.items[i + 1].ty == it.ty:
                    cands.append(("merge", c, i))
        x = self.pick(cands)
        if not x:
            return None
        kind, c, i = x
        it = c.items[i]
        if kind == "split":
            c.items[i:i + 1] = [Decl(it.ty, [d]) for d in it.decls]
            return "splitmulti"
        nx = c.items[i + 1]
        if any(d.init is not None for d in nx.decls) and any(
                dd.name in ids_in(d.init) for d in nx.decls if d.init is not None for dd in it.decls):
            pass
        c.items[i:i + 2] = [Decl(it.ty, it.decls + nx.decls)]
        return "mergemulti"

    def has_continue(self, body):
        def rec(s):
            if isinstance(s, Jump) and s.kind == "continue":
                return True
            if isinstance(s, LOOPS):
                return False
            if isinstance(s, Compound):
                return any(rec(x) for x in s.items)
            return any(rec(b) for b in child_bodies(s) if b is not s)
        return any(rec(x) for x in body.items)

    def m_forwhile(self):
        cands = []
        for c in all_compounds(self.f.body):
            for i, it in enumerate(c.items):
                if isinstance(it, For) and it.c is not None and not self.has_continue(it.body):
                    cands.append(("f2w", c, i))
                if isinstance(it, While) and i > 0 and isinstance(c.items[i - 1], ExprStmt) \
                        and it.body.items and isinstance(it.body.items[-1], ExprStmt) \
                        and not self.has_continue(it.body) and not isinstance(it.body.items[-1], Decl):
                    cands.append(("w2f", c, i))
        x = self.pick(cands)
        if not x:
            return None
        kind, c, i = x
        it = c.items[i]
        if kind == "f2w":
            body = it.body
            local_names = {d.name for x in body.items if isinstance(x, Decl) for d in x.decls}
            if it.step is not None and local_names & ids_in(it.step):
                return None
            items = list(body.items)
            if it.step is not None:
                items.append(ExprStmt(it.step))
            w = While(it.c, Compound(items, synthetic=False))
            rep = [w]
            if it.init is not None:
                rep.insert(0, ExprStmt(it.init))
            c.items[i:i + 1] = rep
            return "for2while"
        prev = c.items[i - 1]
        if isinstance(prev, Decl):
            return None
        step = it.body.items[-1]
        local_names = {d.name for x in it.body.items if isinstance(x, Decl) for d in x.decls}
        if local_names & (ids_in(step.e) | ids_in(it.c)):
            return None
        # init statement must be pure-local-ish; step must not be a decl
        f = For(prev.e, it.c, step.e, Compound(it.body.items[:-1], synthetic=False))
        c.items[i - 1:i + 1] = [f]
        return "while2for"

    def m_walkidx(self):
        """index -> pointer walk, or pointer walk -> index, inside a simple counted For."""
        loops = [inf for inf in self.infos if isinstance(inf.s, For)]
        self.rng.shuffle(loops)
        for inf in loops:
            f = inf.s
            # IV: init i = 0, step i++ / ++i / i += 1
            if not (isinstance(f.init, Assign) and f.init.op == "=" and isinstance(strip_paren(f.init.l), Id)):
                continue
            iv = strip_paren(f.init.l).name
            if not (isinstance(f.init.r, Lit) and f.init.r.text == "0"):
                continue
            st = f.step
            if not ((isinstance(st, (Postfix, Unary)) and st.op == "++" and pe(st.e) == iv) or
                    (isinstance(st, Assign) and st.op == "+=" and pe(st.l) == iv and pe(st.r) == "1")):
                continue
            if self.has_continue(f.body):
                continue
            body_ids = set()
            for x in f.body.items:
                body_ids |= stmt_ids(x)
            # iv must not be written in body
            beff = Eff()
            for x in f.body.items:
                beff.merge(self.ctx.eff_stmt(x))
            if iv in beff.w:
                continue
            # candidate pointers p: local pointer, used only as p[iv] in body, not written in body,
            # and dead after the loop
            if self.rng.random() < 0.5:
                for p in sorted(body_ids):
                    if not self.is_ptr(Id(p)) or not self.ctx.scalar_local(p) or p in beff.w:
                        continue
                    if p in own_ids(f):
                        continue
                    uses = self.collect_uses(f.body, p)
                    if not uses or not all(isinstance(sget(o), Index) and pe(sget(o).i) == iv for o in uses):
                        continue
                    if not dead_from(self.ctx, self.infos, p, self.next_info(inf), exclude=()) \
                            if self.next_info(inf) else False:
                        continue
                    for o in uses:
                        sset(o, Index(Id(p), Lit("0", "num")) if self.rng.random() < 0.5 else Unary("*", Id(p)))
                    f.body.items.append(ExprStmt(Postfix("++", Id(p))))
                    f.body.synthetic = False
                    return "idx2walk"
            else:
                # walk -> index: body ends with p++ and p only dereferenced as *p / p[0] / p->
                last = f.body.items[-1] if f.body.items else None
                if not (isinstance(last, ExprStmt) and isinstance(last.e, (Postfix, Unary))
                        and last.e.op == "++" and isinstance(strip_paren(last.e.e), Id)):
                    continue
                p = strip_paren(last.e.e).name
                if not self.is_ptr(Id(p)) or not self.ctx.scalar_local(p):
                    continue
                others = f.body.items[:-1]
                oe = Eff()
                for x in others:
                    oe.merge(self.ctx.eff_stmt(x))
                if p in oe.w or p in own_ids(f):
                    continue
                uses = []
                for x in others:
                    uses += self.collect_uses(Compound([x]), p, parent=True)
                ok = True
                reps = []
                for slot, par in uses:
                    if isinstance(par, Unary) and par.op == "*":
                        reps.append((par, "deref"))
                    elif isinstance(par, Index) and pe(par.i) == "0" and par.a is sget(slot):
                        reps.append((par, "idx0"))
                    elif isinstance(par, Member) and par.op == "->":
                        reps.append((par, "arrow"))
                    else:
                        ok = False
                        break
                if not ok or not uses:
                    continue
                nxt = self.next_info(inf)
                if nxt is None or not dead_from(self.ctx, self.infos, p, nxt):
                    continue
                for par, kind in reps:
                    if kind == "deref":
                        self.replace_node(f.body, par, Index(Id(p), Id(iv)))
                    elif kind == "idx0":
                        par.i = Id(iv)
                    else:
                        par.e = Index(Id(p), Id(iv))
                        par.op = "."
                f.body.items.pop()
                return "walk2idx"
        return None

    def next_info(self, inf):
        c = inf.comp
        idx = c.items.index(inf.s)
        if idx + 1 < len(c.items):
            return self.by_id.get(id(c.items[idx + 1]))
        return None

    def collect_uses(self, body, name, parent=False):
        out = []

        def rec_e(slot, par):
            e = sget(slot)
            if isinstance(e, Id) and e.name == name:
                out.append((slot, par) if parent else par_slot(slot, par))
                return
            for ch in expr_children(e):
                rec_e(ch, e)

        def par_slot(slot, par):
            return self._parent_slot.get(id(par), slot) if False else self._pslot(slot, par)

        self._pmap = {}
        for c in all_compounds(body):
            for it in c.items:
                for top, _ in stmt_expr_slots(it):
                    self._index_parents(top, None)
                    rec_e(top, None)
        return out

    def _index_parents(self, slot, pslot):
        e = sget(slot)
        self._pmap[id(e)] = pslot
        for ch in expr_children(e):
            self._index_parents(ch, slot)

    def _pslot(self, slot, par):
        # returns the slot holding the parent node `par` (for Index replacement)
        return self._pmap.get(id(sget(slot))) or slot

    def replace_node(self, body, old, new):
        for c in all_compounds(body):
            for it in c.items:
                for top, _ in stmt_expr_slots(it):
                    if self._replace_in(top, old, new):
                        return True
        return False

    def _replace_in(self, slot, old, new):
        e = sget(slot)
        if e is old:
            sset(slot, new)
            return True
        return any(self._replace_in(ch, old, new) for ch in expr_children(e))


MUTATIONS = {
    "swap": (Mut.m_swap, 5), "move": (Mut.m_move, 2), "declin": (Mut.m_declin, 1),
    "declout": (Mut.m_declout, 1), "temp": (Mut.m_temp, 3), "reuse": (Mut.m_reuse, 2),
    "inline": (Mut.m_inline, 2), "commute": (Mut.m_commute, 3), "ternary": (Mut.m_ternary, 1),
    "compound": (Mut.m_compound, 1), "cast": (Mut.m_cast, 1), "wrapreal": (Mut.m_wrapreal, 1),
    "splitdecl": (Mut.m_splitdecl, 2), "splitmulti": (Mut.m_splitmulti, 1),
    "forwhile": (Mut.m_forwhile, 1), "ptrform": (Mut.m_ptrform, 1), "walkidx": (Mut.m_walkidx, 1),
    "not": (Mut.m_not, 1), "ifswap": (Mut.m_ifswap, 1),
}


# --------------------------------------------------------------------------
# Build + score
# --------------------------------------------------------------------------

def compile_cmd(obj):
    out = subprocess.run(["ninja", "-t", "commands", obj], cwd=ROOT, capture_output=True, text=True, check=True)
    line = out.stdout.strip().splitlines()[-1]
    return line


class Scorer:
    def __init__(self, unit, src_rel, workdir):
        cfg = json.loads((ROOT / "objdiff.json").read_text())
        u = next(x for x in cfg["units"] if x["name"] == unit)
        self.target = str(ROOT / u["target_path"])
        base_obj = u["base_path"]
        cmd = compile_cmd(base_obj)
        src_win = src_rel.replace("/", "\\")
        self.src_rel = src_rel
        self.workdir = Path(workdir)
        self.workdir.mkdir(parents=True, exist_ok=True)
        self.copy = self.workdir / Path(src_rel).name
        if f"-c {src_win}" not in cmd and f"-c {src_rel}" not in cmd:
            raise SystemExit("could not find -c <src> in: " + cmd)
        cmd = cmd.replace(f"-c {src_win}", f"-c {self.copy}").replace(f"-c {src_rel}", f"-c {self.copy}")
        cmd = re.sub(r"-o \S+", lambda m: f"-o {self.workdir}", cmd)
        cmd = cmd.replace(" -MMD", "")
        origdir = str((ROOT / src_rel).parent)
        cmd = cmd.replace(" -i ", f" -i {origdir} -i ", 1)
        self.cmd = cmd
        self.obj = self.workdir / (Path(src_rel).stem + ".o")
        self.cache = {}
        self.evals = self.fails = self.cache_hits = 0

    def score(self, text):
        self.copy.write_bytes(text.encode("utf-8", "surrogateescape"))
        if self.obj.exists():
            self.obj.unlink()
        r = subprocess.run(self.cmd, cwd=ROOT, shell=True, capture_output=True, text=True, timeout=120)
        if r.returncode != 0 or not self.obj.exists():
            self.fails += 1
            return None
        self.evals += 1
        ob = self.obj.read_bytes()
        hk = hash(ob)
        if hk in self.cache:
            self.cache_hits += 1
            return dict(self.cache[hk])
        r = subprocess.run([str(OBJDIFF), "diff", "-1", self.target, "-2", str(self.obj), "-o", "-",
                            "--format", "json", "-c", "functionRelocDiffs=none"],
                           cwd=ROOT, capture_output=True, text=True, timeout=120)
        if r.returncode != 0:
            return None
        d = json.loads(r.stdout)
        res = {}
        for s in d["left"]["symbols"]:
            if s.get("kind") == "SYMBOL_FUNCTION":
                res[s["name"]] = float(s.get("match_percent", 0.0))
        if len(self.cache) < 20000:
            self.cache[hk] = dict(res)
        return res


# --------------------------------------------------------------------------
# Driver
# --------------------------------------------------------------------------

class State:
    """Original file text + parsed funcs; renders a candidate file."""

    def __init__(self, text, funcs, types):
        self.text = text
        self.nl = "\r\n" if "\r\n" in text else "\n"
        self.types = types
        scan_macros(text)
        self.funcs = {}
        self.spans = {}
        for name in funcs:
            f, span = parse_function(text, name, types)
            self.funcs[name] = f
            self.spans[name] = span

    def render(self, bodies):
        out = self.text
        for name in sorted(self.spans, key=lambda n: -self.spans[n][0]):
            ob, cb = self.spans[name]
            out = out[:ob] + print_body(bodies[name], self.nl) + out[cb + 1:]
        return out

    def body_texts(self, bodies):
        return {n: print_body(b, "\n") for n, b in bodies.items()}


def reparse_body(text, func, types):
    toks = tokenize(text)
    p = Parser(toks, types, {n for n, _ in func.params})
    return p.parse_compound()


def objective(res, syms):
    return sum(res.get(s, 0.0) for s in syms)


def others_ok(res, base, syms):
    for k, v in base.items():
        if k in syms:
            continue
        if res.get(k, 0.0) + 1e-6 < v:
            return False
    return True


def mutate(bodies, state, rng, weights, nmut):
    bodies = copy.deepcopy(bodies)
    names = list(bodies)
    done = []
    for _ in range(nmut):
        for _try in range(25):
            fname = rng.choice(names)
            f = state.funcs[fname]
            func = Func(f.name, f.params, bodies[fname], f.ptypes)
            m = Mut(func, state.types, rng)
            kinds = [k for k in weights if weights[k] > 0]
            k = rng.choices(kinds, [weights[x] for x in kinds])[0]
            try:
                r = MUTATIONS[k][0](m)
            except (RecursionError, ValueError, KeyError, AttributeError, IndexError, TypeError):
                r = None
            if r:
                bodies[fname] = func.body
                done.append(f"{fname}:{r}")
                break
    return bodies, done


def worker(args_dict, wid):
    a = argparse.Namespace(**args_dict)
    global PLAIN_TEMPS
    PLAIN_TEMPS = a.plain_temps
    rng = random.Random(a.seed_rng * 1000 + wid)
    text = Path(a.start_file).read_bytes().decode("utf-8", "surrogateescape")
    types = scan_typenames(text)
    state = State(text, a.func, types)
    scorer = Scorer(a.unit, a.src, Path(a.out) / f"w{wid}")
    syms = a.score
    base_bodies = {n: f.body for n, f in state.funcs.items()}
    base_res = json.loads(Path(a.out, "baseline.json").read_text())
    cur = base_bodies
    cur_s = objective(base_res, syms)
    best_s = cur_s
    seen = set()
    weights = a.weights
    deadline = time.time() + a.time
    it = 0
    last_imp = 0
    hits = 0
    gbest_path = Path(a.out, "gbest.json")
    log = open(Path(a.out, f"w{wid}.log"), "a")
    while time.time() < deadline:
        it += 1
        nm = rng.choices([1, 2, 3], [0.6, 0.3, 0.1])[0]
        cand, desc = mutate(cur, state, rng, weights, nm)
        if not desc:
            continue
        rendered = state.render(cand)
        key = hash(rendered)
        if key in seen:
            continue
        seen.add(key)
        try:
            res = scorer.score(rendered)
        except subprocess.TimeoutExpired:
            res = None
        if res is None:
            continue
        s = objective(res, syms)
        ok = others_ok(res, base_res, syms)
        if not ok:
            continue
        accept = s > cur_s + 1e-9 or (abs(s - cur_s) < 1e-9 and rng.random() < a.p_equal) \
            or (s < cur_s and rng.random() < a.p_worse)
        if s > best_s + 1e-9:
            best_s = s
            last_imp = it
            fn = Path(a.out, f"best_{s:.4f}_w{wid}.c")
            fn.write_bytes(rendered.encode("utf-8", "surrogateescape"))
            log.write(f"[{time.strftime('%H:%M:%S')}] it={it} new best {s:.4f} {desc} {res and {k: res.get(k) for k in syms}}\n")
            log.flush()
            # share
            try:
                g = json.loads(gbest_path.read_text()) if gbest_path.exists() else {"score": -1}
            except (OSError, ValueError):
                g = {"score": -1}
            if s > g["score"]:
                tmp = gbest_path.with_suffix(f".{wid}.tmp")
                tmp.write_text(json.dumps({"score": s, "bodies": state.body_texts(cand), "w": wid}))
                try:
                    os.replace(tmp, gbest_path)
                except OSError:
                    pass
        if all(res.get(x, 0) >= 100.0 for x in syms):
            hits += 1
            fn = Path(a.out, f"hit_w{wid}_{hits}.c")
            fn.write_bytes(rendered.encode("utf-8", "surrogateescape"))
            log.write(f"HIT {fn}\n")
            log.flush()
            if hits >= 3:
                break
        if accept:
            cur, cur_s = cand, s
        if it % 100 == 0:
            log.write(f"[{time.strftime('%H:%M:%S')}] it={it} evals={scorer.evals} fails={scorer.fails} "
                      f"samecode={scorer.cache_hits} cur={cur_s:.4f} best={best_s:.4f}\n")
            log.flush()
        # periodically adopt global best / restart
        if it % 40 == 0:
            try:
                g = json.loads(gbest_path.read_text())
                if g["score"] > cur_s + 1e-9 and rng.random() < 0.5:
                    cur = {n: reparse_body(t, state.funcs[n], types) for n, t in g["bodies"].items()}
                    cur_s = g["score"]
            except (OSError, ValueError, KeyError, SyntaxError, ParseError):
                pass
        if it - last_imp > a.restart and rng.random() < 0.02:
            cur, cur_s = base_bodies, objective(base_res, syms)
            last_imp = it
    log.write(f"done it={it} best={best_s:.4f}\n")
    log.close()
    return best_s, it


def minimize(a, hitfile):
    """Revert diff hunks of a hit while it stays at 100% and other functions hold."""
    text = Path(a.start_file).read_bytes().decode("utf-8", "surrogateescape")
    types = scan_typenames(text)
    base = State(text, a.func, types)
    htext = Path(hitfile).read_bytes().decode("utf-8", "surrogateescape")
    hit = State(htext, a.func, types)
    scorer = Scorer(a.unit, a.src, Path(a.out) / "min")
    base_res = json.loads(Path(a.out, "baseline.json").read_text())
    syms = a.score
    bl = {n: print_body(base.funcs[n].body, "\n").split("\n") for n in a.func}
    hl = {n: print_body(hit.funcs[n].body, "\n").split("\n") for n in a.func}
    # edit atoms: ("d", n, i) restore base line i; ("i", n, j) keep hit line j
    ops = {n: difflib.SequenceMatcher(None, bl[n], hl[n], autojunk=False).get_opcodes() for n in a.func}
    restored = set()   # ("d", n, i) atoms reverted (base line kept)
    dropped = set()    # ("i", n, j) atoms reverted (hit line dropped)
    groups = []
    for n in a.func:
        for tag, i1, i2, j1, j2 in ops[n]:
            if tag == "equal":
                continue
            ds = [("d", n, i) for i in range(i1, i2)]
            ins = [("i", n, j) for j in range(j1, j2)]
            groups.append(ds + ins)
            for k in range(max(len(ds), len(ins))):
                pair = ([ds[k]] if k < len(ds) else []) + ([ins[k]] if k < len(ins) else [])
                if len(pair) == 2:
                    groups.append(pair)
            for x in ds + ins:
                groups.append([x])

    def build(restored, dropped):
        texts = {}
        for n in a.func:
            out = []
            for tag, i1, i2, j1, j2 in ops[n]:
                if tag == "equal":
                    out += bl[n][i1:i2]
                    continue
                for k in range(max(i2 - i1, j2 - j1)):
                    if i1 + k < i2 and ("d", n, i1 + k) in restored:
                        out.append(bl[n][i1 + k])
                    if j1 + k < j2 and ("i", n, j1 + k) not in dropped:
                        out.append(hl[n][j1 + k])
            texts[n] = "\n".join(out)
        out = base.text
        for n in sorted(base.spans, key=lambda x: -base.spans[x][0]):
            ob, cb = base.spans[n]
            out = out[:ob] + texts[n].replace("\n", base.nl) + out[cb + 1:]
        return out, texts

    def good(restored, dropped):
        t, _ = build(restored, dropped)
        r = scorer.score(t)
        return r is not None and all(r.get(x, 0) >= 100.0 for x in syms) and others_ok(r, base_res, syms)

    if not good(restored, dropped):
        print("hit does not reproduce at 100%")
        return None
    changed = True
    while changed:
        changed = False
        for g in groups:
            todo = [x for x in g if x not in restored and x not in dropped]
            if not todo:
                continue
            r2 = restored | {x for x in todo if x[0] == "d"}
            d2 = dropped | {x for x in todo if x[0] == "i"}
            if good(r2, d2):
                restored, dropped = r2, d2
                changed = True
    keep = None
    t, texts = build(restored, dropped)
    diff = []
    for n in a.func:
        diff += list(difflib.unified_diff(bl[n], texts[n].split("\n"), f"{n} (normalised original)",
                                          f"{n} (minimised hit)", lineterm="", n=2))
    out = Path(hitfile).with_suffix(".min.diff")
    out.write_text("\n".join(diff) + "\n")
    Path(hitfile).with_suffix(".min.c").write_bytes(t.encode("utf-8", "surrogateescape"))
    print("\n".join(diff))
    return out


def parse_weights(s):
    w = {k: v[1] for k, v in MUTATIONS.items()}
    if s:
        for part in s.split(","):
            k, v = part.split("=")
            w[k.strip()] = float(v)
    return w


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--src", required=True, help="unit source path relative to repo root")
    ap.add_argument("--func", action="append", required=True, help="function(s) to mutate")
    ap.add_argument("--score", action="append", help="symbol(s) to score (default: --func)")
    ap.add_argument("--unit", required=True, help="objdiff unit name, e.g. main/rwsdk/src/baresamp")
    ap.add_argument("--seed", help="start from this source copy instead of --src")
    ap.add_argument("--out", help="output dir (default %%TEMP%%/rw_permute/<func>)")
    ap.add_argument("--time", type=float, default=600, help="time budget in seconds")
    ap.add_argument("--workers", type=int, default=5)
    ap.add_argument("--p-equal", type=float, default=0.25)
    ap.add_argument("--p-worse", type=float, default=0.01)
    ap.add_argument("--restart", type=int, default=400, help="iterations w/o improvement before possible restart")
    ap.add_argument("--weights", default="", help="e.g. 'temp=3,reuse=0'")
    ap.add_argument("--seed-rng", type=int, default=int(time.time()) % 100000)
    ap.add_argument("--selftest", action="store_true", help="parse/print round-trip + baseline score only")
    ap.add_argument("--minimize", help="minimise a hit file")
    ap.add_argument("--plain-temps", action="store_true", help="only introduce temps whose type is known (no __typeof__)")
    ap.add_argument("--no-reuse", action="store_true", help="disable dead-local reuse mutation")
    a = ap.parse_args()
    a.src = a.src.replace("\\", "/")
    a.score = a.score or list(a.func)
    a.weights = parse_weights(a.weights)
    if a.no_reuse:
        a.weights["reuse"] = 0
    a.start_file = str(Path(a.seed).resolve()) if a.seed else str(ROOT / a.src)
    if not a.out:
        a.out = str(Path(tempfile.gettempdir()) / "rw_permute" / a.func[0])
    Path(a.out).mkdir(parents=True, exist_ok=True)

    if a.minimize:
        minimize(a, a.minimize)
        return 0

    text = Path(a.start_file).read_bytes().decode("utf-8", "surrogateescape")
    types = scan_typenames(text)
    state = State(text, a.func, types)
    scorer = Scorer(a.unit, a.src, Path(a.out) / "base")
    real = scorer.score(Path(ROOT / a.src).read_bytes().decode("utf-8", "surrogateescape"))
    start = scorer.score(text)
    norm_text = state.render({n: f.body for n, f in state.funcs.items()})
    norm = scorer.score(norm_text)
    print("real source :", {k: real.get(k) for k in a.score} if real else "COMPILE FAILED")
    print("start file  :", {k: start.get(k) for k in a.score} if start else "COMPILE FAILED")
    print("normalised  :", {k: norm.get(k) for k in a.score} if norm else "COMPILE FAILED")
    if norm is None:
        Path(a.out, "norm_fail.c").write_bytes(norm_text.encode("utf-8", "surrogateescape"))
        print("normalised source does not compile; see", Path(a.out, "norm_fail.c"))
        return 1
    if start and norm != start:
        diffs = {k: (start.get(k), norm.get(k)) for k in set(start) | set(norm) if start.get(k) != norm.get(k)}
        print("WARNING: normalisation changed scores:", diffs)
    # round-trip check
    for n, f in state.funcs.items():
        t1 = print_body(f.body, "\n")
        t2 = print_body(reparse_body(t1, f, types), "\n")
        if t1 != t2:
            print(f"WARNING: {n} print/parse round-trip unstable")
    Path(a.out, "baseline.json").write_text(json.dumps(norm))
    Path(a.out, "normalised.c").write_bytes(norm_text.encode("utf-8", "surrogateescape"))
    gb = Path(a.out, "gbest.json")
    if gb.exists():
        gb.unlink()
    if a.selftest:
        rng = random.Random(1)
        bodies = {n: f.body for n, f in state.funcs.items()}
        stats = {}
        for k in MUTATIONS:
            okc = 0
            for t in range(20):
                b2, d = mutate(bodies, state, rng, {k: 1}, 1)
                if d:
                    okc += 1
            stats[k] = okc
        print("mutation applicability (of 20):", stats)
        for k in list(MUTATIONS)[:]:
            b2, d = mutate(bodies, state, rng, {k: 1}, 1)
            if d:
                r = scorer.score(state.render(b2))
                print(f"  {k:10s} -> {'COMPILE FAIL' if r is None else {s: round(r.get(s, 0), 3) for s in a.score}}")
        return 0
    print(f"baseline objective {objective(norm, a.score):.4f}; running {a.workers} workers for {a.time}s -> {a.out}")
    sys.stdout.flush()
    from concurrent.futures import ProcessPoolExecutor
    args_dict = vars(a)
    with ProcessPoolExecutor(max_workers=a.workers) as ex:
        futs = [ex.submit(worker, args_dict, w) for w in range(a.workers)]
        res = [f.result() for f in futs]
    print("workers:", res)
    hits = sorted(Path(a.out).glob("hit_*.c"))
    hits = [h for h in hits if not h.name.endswith(".min.c")]
    bests = sorted(Path(a.out).glob("best_*.c"))
    if bests:
        print("best file:", max(bests, key=lambda p: float(p.name.split("_")[1])))
    for h in hits[:3]:
        print("minimising", h)
        minimize(a, str(h))
    return 0


if __name__ == "__main__":
    sys.exit(main())
