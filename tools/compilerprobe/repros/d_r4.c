/* (d-R4) the alias of a load with TWO address registers (lwzx/lbzx ...).
 * GC/2.0p1's pointer-load alias pass answers worst_case when neither address
 * register's defs are unknown, so the load aliases every callee-save spill of
 * the prologue and is scheduled after all of them. GC/2.5's rewritten pass
 * lets a register with no alias contribute nothing, so the access keeps its
 * own alias (here R3's pointer-to-const pseudo-object, or a static table) and
 * is hoisted up to the spill of its own destination register.
 * Expected: xform_c, array_c: [2.0p1 2.0p1a 2.0p1b 2.0p1c] != [2.5 2.6 2.7 2.0p1d]
 *           *_nc controls: non-const pointer, no alias, pinned everywhere.
 *           table*_c: static-table indexed loads -- every compiler agrees in
 *           these shapes (no divergence to model, and none introduced).   */
typedef struct { int a, b; } T;
extern int use(T *, int, int, int);

int xform_c(const void *object, int offset, int x, int y)
{
    T *t = *(T **)((const char *)object + offset);
    int r = use(t, x, y, 0);
    r += use(t, y, x, r);
    return use(t, r, x, y) + x + y;
}

int xform_nc(void *object, int offset, int x, int y)
{
    T *t = *(T **)((char *)object + offset);
    int r = use(t, x, y, 0);
    r += use(t, y, x, r);
    return use(t, r, x, y) + x + y;
}

/* indexed load from a static table */
static unsigned char tbl[64];
int table_c(int i, int x, int y, T *t)
{
    int v = tbl[i];
    int r = use(t, x, y, v);
    r += use(t, y, x, r);
    return use(t, r, v, y) + x + y;
}

/* an indexed load through a pointer-to-const parameter */
int array_c(const unsigned char *a, int i, int x, T *t)
{
    int v = a[i];
    int r = use(t, x, v, 0);
    r += use(t, v, x, r);
    return use(t, r, v, x) + x;
}

/* the same through a plain pointer: no alias, worst_case on every compiler */
int array_nc(unsigned char *a, int i, int x, T *t)
{
    int v = a[i];
    int r = use(t, x, v, 0);
    r += use(t, v, x, r);
    return use(t, r, v, x) + x;
}

/* a static table indexed after the spills have something to protect */
static unsigned char tbl2[64];
int table2_c(T *t, int x, int i)
{
    int v = tbl2[i];
    int r = use(t, x, v, 0);
    r += use(t, v, x, r);
    return use(t, r, v, x) + x + i;
}

/* a small (.sbss/.sdata) static table indexed by a parameter, after spills */
static unsigned char stbl[8];
int table3_c(T *t, int x, int y, int i)
{
    int v;
    int r = use(t, x, y, 0);
    v = stbl[i];
    r += use(t, v, x, r);
    return use(t, r, v, x) + y;
}
extern unsigned char etbl[8];
int table4_c(int i, int x, int y, T *t)
{
    int v = etbl[i];
    int r = use(t, x, v, y);
    r += use(t, v, x, r);
    return use(t, r, v, x) + y;
}

/* an X-form store, not a load */
int store_x(int *a, int i, int x, T *t)
{
    a[i] = x;
    {
        int r = use(t, x, i, 0);
        r += use(t, i, x, r);
        return use(t, r, i, x) + x;
    }
}

/* two callee-saved registers, so the prologue spills with stw, not _savegpr */
extern unsigned char etbl2[8];
int table5_c(int i, int x)
{
    int v = etbl2[i];
    int r = use(0, x, v, 0);
    return use(0, r, v, x);
}
static unsigned char stbl2[64];
int table6_c(int i, int x)
{
    int v = stbl2[i];
    int r = use(0, x, v, 0);
    return use(0, r, v, x);
}
