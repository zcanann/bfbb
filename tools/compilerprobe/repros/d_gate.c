/* (d-gate) clauses E3n / A / W / V fire on a frame object only when its
 * address escapes (it is a member of the worst_case alias set).
 * These clauses are 2.0p1a EMULATION: no stock compiler has them, so stock
 * GC/2.0p1 and GC/2.5 schedule all of these freely. Retail (see
 * docs/COMPILER_VARIANTS.md) keeps a literal/static load below a store to an
 * escaping local and lets it pass a store to a local nothing can point at.
 * Expected:  *_local (address never taken): 2.0p1d == 2.0p1c with the clauses
 *            off, i.e. the stock ordering; 2.0p1a/b/c pin.
 *            *_esc   (address passed to a call): 2.0p1d == 2.0p1c (pinned).  */
typedef union { float f; unsigned int i; } FU;
typedef struct { unsigned char r, g, b, a; } Col;
extern void take(void *);
extern void takec(Col *);
extern float gF;
extern int gI;

/* E3n: store to a declared frame object, then a literal load */
float pun_local(float x)
{
    FU u;
    u.f = x;
    u.i &= 0x7fffffff;
    return u.f * 0.5f + 1.5f;
}
float pun_esc(float x)
{
    FU u;
    u.f = x;
    take(&u);
    u.i &= 0x7fffffff;
    return u.f * 0.5f + 1.5f;
}

/* A: whole-scalar frame store vs a later static load (both <= 4 bytes) */
int scalar_local(int a, int b)
{
    int t;
    t = a * b;
    return t + gI * 3;
}
int scalar_esc(int a, int b)
{
    int t;
    take(&t);
    t = a * b;
    return t + gI * 3;
}

/* W: a literal load, then a plain store to a declared local */
float w_local(float x, float y)
{
    float v[2];
    v[0] = x * 2.0f;
    v[1] = y;
    return v[0] + v[1] * 4.0f;
}
float w_esc(float x, float y)
{
    float v[2];
    take(v);
    v[0] = x * 2.0f;
    v[1] = y;
    return v[0] + v[1] * 4.0f;
}

/* V: a store to a declared frame object between two uses of a literal */
float v_local(float x, float y)
{
    FU u;
    float a = x * 3.0f;
    u.f = y;
    u.i ^= 0x80000000;
    return a + u.f * 3.0f;
}
float v_esc(float x, float y)
{
    FU u;
    float a;
    take(&u);
    a = x * 3.0f;
    u.f = y;
    u.i ^= 0x80000000;
    return a + u.f * 3.0f;
}

/* a struct argument temporary (GXColor by pointer), then literal loads */
void col_esc(float r, float g)
{
    Col c;
    c.r = 1; c.g = 2; c.b = 3; c.a = 4;
    takec(&c);
    gF = r * 0.25f + g * 0.75f;
}
void col_local(float r, float g, int *out)
{
    Col c;
    c.r = 1; c.g = 2; c.b = 3; c.a = (unsigned char)*out;
    *out = c.r + c.a;
    gF = r * 0.25f + g * 0.75f;
}
