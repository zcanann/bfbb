/* (c-R3) scope check: 2.5's kind-1 pseudo-object alias is visible to every
 * pass, not just LICM. 2.0p1c grafts only the LICM consequence. These cases
 * ask whether 2.5 also uses it OUTSIDE loops (CSE across a call, scheduling a
 * load above a store through another pointer). If 2.5 differs from 2.0p1 here
 * and 2.0p1c/d do not follow, the graft is partial (documented, not hidden). */
typedef struct { int n; int *m; float f; } L;
extern int g(int);

/* the same load before and after a call */
int cse_call(const L *l) { int a = l->n; g(0); return a + l->n; }
int cse_call_nc(L *l)    { int a = l->n; g(0); return a + l->n; }

/* the same load before and after a store through another pointer */
int cse_store(int *o, const L *l) { int a = l->n; *o = 5; return a + l->n; }
int cse_store_nc(int *o, L *l)    { int a = l->n; *o = 5; return a + l->n; }

/* a const load that the scheduler could lift above an unrelated store */
float sched_store(float *o, const L *l, float x) { *o = x * x; return l->f * x; }
float sched_store_nc(float *o, L *l, float x)    { *o = x * x; return l->f * x; }
