// audit.js: per-clause firing log and per-class ablation for GC/2.0p1f(+ssi)'s
// AliasPatch.c clauses. Injected by aaudit.py into every compile.
//
// Every clause entry point of the 2.0p1a blob (as patched by 2.0p1b..f and ssi)
// is hooked; a clause "fires" when it returns 1 (or, for the VN walk, kills).
// Each firing is described by a class key, counted per function, and -- if the
// key matches ABL (a regex) -- its answer is replaced by 0 so that the rest of
// the clause chain and then the stock compiler decide instead.
//
// Blob map (2.0p1f, verified by disassembly):
//   0x60e2c4 sb_sched_clause(a, b, entry)  e0: E3n W C+ (A|B)  e1: W C B  e3: E3n C B  e4: S (B|A)
//   0x60e13c E3n(a,b,ma,mb)   reached through the at-sched gate 0x60e740 (escaping frame obj only)
//   0x60e1c8 W(a,b,ma,mb)     reached through the at-w gate 0x60e7b0
//   0x60e06c C+(a,b,ma,mb)    reached through the ssi stub (0x60efc0) from e0 (ret 0x60e31f) and C (ret 0x60e132)
//   0x60e0f0 C(a,b,ma,mb)     e1/e3; no fIsPtrOp, both <= 4 bytes, then C+
//   0x60e2a0 A(a,b)           e0 via at-sched gate 0x60e760 (ret 0x60e376); e4 direct (ret 0x60e48d)
//   0x60e254 B(a,b,ma,mb)     e0 0x60e385, e1 0x60e3d6, e3 0x60e41c, e4 0x60e47f
//   e4 same small static ("S="): sched_clause returns 1 with no sub-clause called
//   0x60e508 vn_store_kill(alias, head)       -> V walk (0x60e4d0) for static/frame stores; returns 1 = F
//   0x60e548 vn_subrange_store(alias, st, head) -> V walk if direct store; returns 1 = SV (fresh VN)
//   0x60e49c sb_licm_invariant(pcode)         returns 1 = "whole static read is never invariant"
var base = Process.getModuleByName("mwcceppc.exe").base;
function va(x) { return base.add(x - 0x400000); }
function rva(p) { return (p.sub(base).toInt32() + 0x400000) >>> 0; }
var WCP = va(0x5e9cb4);
var PSEUDO = va(0x5bd008);
var ABL = %ABL%;            // null or RegExp source
var ABLRE = ABL ? new RegExp(ABL) : null;
var LOG = %LOG%;
var FNF = %FN%;            // null, or ablate only inside this function
function abl(k) { return ABLRE && (!FNF || curfn == FNF) && ABLRE.test(k); }
var getLink = new NativeFunction(va(0x4FE710), "pointer", ["pointer"], "mscdecl");
var curfn = "?";
var C = {};                  // key -> {fn: count}
var NABL = 0;
function bump(k) {
  if (!LOG) return;
  var e = C[k]; if (!e) { e = C[k] = {}; }
  e[curfn] = (e[curfn] || 0) + 1;
}
Interceptor.attach(va(0x4333C0), { onEnter: function () {
  try { var o = this.context.esp.add(8).readPointer(); curfn = getLink(o).add(0xa).readCString(); } catch (e) { curfn = "??"; } } });

function opcls(op) {
  if (op >= 21 && op <= 24) return "lb"; if (op >= 25 && op <= 33) return "lh"; if (op >= 34 && op <= 39) return "lw";
  if (op >= 40 && op <= 43) return "sb"; if (op >= 44 && op <= 48) return "sh"; if (op >= 49 && op <= 54) return "sw";
  if (op >= 142 && op <= 145) return "lfs"; if (op >= 146 && op <= 149) return "lfd";
  if (op >= 150 && op <= 153) return "stfs"; if (op >= 154 && op <= 157) return "stfd";
  return "o" + op;
}
function inWC(obj) {
  var w = WCP.readPointer();
  if (w.isNull() || w.add(0x2c).readU8() != 2) return false;
  var n = w.add(0xc).readPointer();
  while (!n.isNull()) { if (n.add(0xc).readPointer().add(0x10).readPointer().equals(obj)) return true; n = n.readPointer(); }
  return false;
}
function szb(s) { return s <= 1 ? "1" : s <= 2 ? "2" : s <= 4 ? "4" : s <= 8 ? "8" : s <= 16 ? "16" : "L"; }
// storage of an alias: lit / st / fr+ (declared, escaping) / fr- / tmp / pso / h.. ; kind w/s/S/WC
function adesc(A) {
  if (A.isNull()) return {s: "nil", k: "", obj: null, sz: 0};
  var k = A.add(0x2c).readU8();
  if (k == 2) return {s: A.equals(WCP.readPointer()) ? "WC" : "S", k: "", obj: null, sz: 0};
  var o = A.add(0x10).readPointer(); var sz = A.add(0x18).readU32();
  var r = {k: k == 1 ? "s" : "w", obj: o, sz: sz, s: "none"};
  if (o.isNull()) return r;
  var h = o.readU32(); var nm = "";
  try { nm = o.add(0xa).readPointer().add(0xa).readCString(); } catch (e) {}
  r.nm = nm;
  if (o.add(0xe).readPointer().equals(PSEUDO)) r.s = "pso";
  else if (h == 5) r.s = (nm.charAt(0) == "@" ? "lit" : "st");
  else if (h == 0x10005) r.s = (o.add(0x18).readU32() == 0 || nm.charAt(0) == "@") ? "tmp" : (inWC(o) ? "fr+" : "fr-");
  else r.s = "h" + h.toString(16);
  return r;
}
// pcode descriptor: opclass, flags (*=fIsPtrOp 0x20, c=0x40, v=0x80, D=direct operand), storage, kind+size
function pdesc(p) {
  var f = p.add(0x14).readU32(); var op = p.add(0x20).readU16();
  var A = p.add(0x18).readPointer();
  var d = adesc(A);
  var fl = ((f & 0x20) ? "*" : "") + ((f & 0x40) ? "c" : "") + ((f & 0x80) ? "v" : "") + (p.add(0x3c).readU8() == 3 ? "D" : "");
  d.str = opcls(op) + fl + ":" + d.s + ":" + d.k + szb(d.sz);
  d.op = op;
  return d;
}

var Q = null;   // current may_alias query {a, b, e, site, sub}
Interceptor.attach(va(0x511fc0), {
  onEnter: function () { Q = {site: rva(this.context.esp.readPointer()), e: -1, sub: false}; },
  onLeave: function () { Q = null; }
});
Interceptor.attach(va(0x60e2c4), {
  onEnter: function (args) { this.a = args[0]; this.b = args[1]; this.e = args[2].toInt32();
    if (Q) { Q.e = this.e; Q.sub = false; Q.a = this.a; Q.b = this.b; } },
  onLeave: function (rv) {
    if (!Q || Q.sub || this.e != 4 || (rv.toInt32() & 0xff) == 0) return;
    // entry 4 hit with no sub-clause: the same small static (clause S)
    var k = "S=@4:" + pdesc(this.a).str + "|" + pdesc(this.b).str;
    bump(k);
    if (abl(k)) { rv.replace(0); NABL++; }
  }
});
function qkey(name, a, b, extra) {
  var da = pdesc(a), db = pdesc(b);
  var same = (da.obj && db.obj && !da.obj.isNull() && da.obj.equals(db.obj)) ? "=same" : "";
  var site = Q ? (Q.site == 0 ? "" : "") : "";
  return name + "@" + (Q ? Q.e : "?") + ":" + da.str + "|" + db.str + same + (extra || "");
}
function clause(addr, name, nargs, retmap) {
  Interceptor.attach(va(addr), {
    onEnter: function (args) { this.a = args[0]; this.b = args[1]; this.ret = rva(this.returnAddress); if (Q) Q.sub = true; },
    onLeave: function (rv) {
      if ((rv.toInt32() & 0xff) == 0) return;
      var nm = name;
      if (retmap) nm = retmap[this.ret] || (name + "?" + this.ret.toString(16));
      var k = qkey(nm, this.a, this.b);
      bump(k);
      if (abl(k)) { rv.replace(0); NABL++; }
    }
  });
}
clause(0x60e13c, "E3n");
clause(0x60e1c8, "W");
clause(0x60e06c, "C+", 4, {0x60e31f: "C+", 0x60e132: "C"});
clause(0x60e2a0, "A");
clause(0x60e254, "B");
// C itself only forwards to C+ (logged there as "C"); hook it only to mark sub
Interceptor.attach(va(0x60e0f0), { onEnter: function () { if (Q) Q.sub = true; } });

// ---- VN ------------------------------------------------------------------
var vkill = new NativeFunction(va(0x50a2c0), "void", ["pointer", "pointer"], "mscdecl");
var VST = null;   // current store {str, e}
Interceptor.attach(va(0x60e508), {
  onEnter: function (args) { VST = {A: args[0], e: 0, str: null}; },
  onLeave: function (rv) {
    var r = rv.toInt32();
    if (r == 1) {
      var d = adesc(VST.A);
      var k = "F@0:" + d.s + ":" + d.k + szb(d.sz);
      bump(k);
      if (abl(k)) { rv.replace(0); NABL++; }
    }
    VST = null;
  }
});
Interceptor.attach(va(0x60e548), {
  onEnter: function (args) { this.st = args[1]; VST = {A: args[0], e: 1, P: args[1], str: null}; },
  onLeave: function (rv) {
    if (rv.toInt32() == 1) {
      var d = adesc(VST.A);
      var dir = (!this.st.isNull() && this.st.add(0x3c).readU8() == 3) ? "D" : "";
      var k = "SV@1:" + d.s + ":" + d.k + szb(d.sz) + dir;
      bump(k);
      if (abl(k)) { rv.replace(0); NABL++; }
    }
    VST = null;
  }
});
// V walk: reimplemented so each kill can be classified (and ablated).
// stock blob: for x in list: if x.size <= 4 && isStatic(x.obj) (non-pseudo) -> kill(x, 0)
var walk0 = va(0x60e4d0);
if (%VW%) Interceptor.replace(walk0, new NativeCallback(function (head) {
  var sd = VST ? adesc(VST.A) : {s: "?", k: "", sz: 0};
  var sk = "V@" + (VST ? VST.e : "?") + ":" + sd.s + ":" + sd.k + szb(sd.sz);
  var x = head, n = 0, kinds = {};
  while (!x.isNull() && n < 1000000) {
    n++;
    if (x.add(0x18).readU32() <= 4) {
      var o = x.add(0x10).readPointer();
      if (!o.isNull() && o.readU32() == 5 && !o.add(0xe).readPointer().equals(PSEUDO)) {
        var nm = ""; try { nm = o.add(0xa).readPointer().add(0xa).readCString(); } catch (e) {}
        var kk = sk + "|kill:" + (nm.charAt(0) == "@" ? "lit" : "st") + ":" + (x.add(0x2c).readU8() == 1 ? "s" : "w");
        var ab = abl(kk);
        if (!ab) vkill(x, ptr(0)); else NABL++;
        if (!kinds[kk]) { kinds[kk] = 1; bump(kk); }
      }
    }
    x = x.readPointer();
  }
}, "void", ["pointer"], "mscdecl"));

// ---- LICM ----------------------------------------------------------------
Interceptor.attach(va(0x60e49c), {
  onEnter: function (args) { this.p = args[0]; },
  onLeave: function (rv) {
    if (rv.toInt32() != 1) return;
    var k = "LICM:" + pdesc(this.p).str;
    bump(k);
    if (abl(k)) { rv.replace(0); NABL++; }
  }
});

var SENT = false;
function final() { if (SENT) return; SENT = true; send({nabl: NABL, log: LOG ? C : null}); recv("ack", function () {}).wait(); }
Interceptor.attach(Process.getModuleByName('kernel32.dll').getExportByName('ExitProcess'), { onEnter: final });
Interceptor.attach(Process.getModuleByName('ntdll.dll').getExportByName('RtlExitUserProcess'), { onEnter: final });
