/* (c-R3) loop-invariant code motion of loads through a pointer-to-const.
 * GC/2.5+ give the value of a register variable typed pointer-to-const a
 * plain (kind 1) alias on a pseudo-object that nothing in the function can
 * define, so `p->field` is loop-invariant even across calls and stores.
 * GC/2.0p1 makes it a one-member alias SET and never hoists it.
 * Expected: *_c functions: [2.0p1 2.0p1a 2.0p1b] != [2.5 2.6 2.7 2.0p1c 2.0p1d]
 *           (2.5 group has fewer lwz inside the loop);
 *           *_nc controls (no const): no compiler hoists (2.0p1c/d == 2.0p1b).
 * Controls where 2.5 does NOT hoist and neither may the variant: local_c (a
 * const pointer returned by a call), derived_c (pointer made by add, not mr),
 * global_c (a global pointer-to-const), and the soundness cases walk_c /
 * bump_c / cond_c (the pointer changes inside the loop): one group each.  */
typedef struct { int n; int *m; float *f; } L;
extern int g(int);
extern void h(void);

/* a call in the loop */
int call_c(const L *l)  { int i, s = 0; for (i = 0; i < l->n; i++) s += g(l->m[i]); return s; }
int call_nc(L *l)       { int i, s = 0; for (i = 0; i < l->n; i++) s += g(l->m[i]); return s; }

/* a store through another pointer in the loop (no call) */
void store_c(int *out, const L *l)  { int i; for (i = 0; i < l->n; i++) out[i] = l->m[i] + 1; }
void store_nc(int *out, L *l)       { int i; for (i = 0; i < l->n; i++) out[i] = l->m[i] + 1; }

/* an early return from the loop */
int find_c(const L *l, int v)  { int i; for (i = 0; i < l->n; i++) if (l->m[i] == v) return i; return -1; }
int find_nc(L *l, int v)       { int i; for (i = 0; i < l->n; i++) if (l->m[i] == v) return i; return -1; }

/* a store through a NON-const pointer to the same type: 2.5 still hoists the
 * const side (it trusts the qualifier), and so must the variant */
void both_c(L *w, const L *r)  { int i; for (i = 0; i < r->n; i++) { w->m[i] = r->m[i]; h(); } }

/* a const pointer that is a local, assigned from a call */
extern const L *get(void);
int local_c(void) { const L *l = get(); int i, s = 0; for (i = 0; i < l->n; i++) s += g(l->m[i]); return s; }

/* float data through the const pointer, with a call */
float fsum_c(const L *l) { int i; float s = 0.0f; for (i = 0; i < l->n; i++) { s += l->f[i]; h(); } return s; }

/* the pointer is derived by arithmetic (add), not a plain copy: 2.5 hands the
 * pseudo-object alias on only through mr, so this must behave like 2.5 too */
int derived_c(const L *l, int k) { const L *q = l + k; int i, s = 0; for (i = 0; i < q->n; i++) s += g(q->m[i]); return s; }

/* double indirection: the loaded pointer is itself the base of a load */
typedef struct { const L *inner; int pad; } O;
int nested_c(const O *o) { int i, s = 0; for (i = 0; i < o->inner->n; i++) s += g(o->inner->m[i]); return s; }

/* soundness: the const pointer itself changes in the loop, so p->field is
 * NOT invariant; nobody may hoist it */
typedef struct N { const struct N *next; int v; } N;
int walk_c(const N *p) { int s = 0; while (p) { s += g(p->v); p = p->next; } return s; }
int bump_c(const int *p, int n) { int s = 0; while (n--) { s += g(*p); p++; } return s; }
/* the const pointer is reassigned only on some iterations */
int cond_c(const L *l, const L *alt) { int i, s = 0; for (i = 0; i < 8; i++) { if (g(i)) l = alt; s += l->n; } return s; }
/* invariant pointer, but the loop index selects the field */
int index_c(const L *l, int n) { int i, s = 0; for (i = 0; i < n; i++) s += g(l->m[i] + l->n); return s; }
/* a global pointer-to-const (not a register variable on entry) */
extern const L *gL;
int global_c(void) { int i, s = 0; for (i = 0; i < gL->n; i++) s += g(gL->m[i]); return s; }
