/* GC/2.0p1e part rep: 2.5 alias for a const/restrict pointee (unrolled
 * const-pointer loops keep one addi per copy; addi off a twice-defined const
 * pointer). Expected: 2.5 == 2.0p1e, and 2.0p1/2.0p1d differ on some functions. */
typedef struct { float x, y, z, d; int pad; } P;
extern void use(const void *);
float b1(const P *p, int n)
{
    float t = 0;
    while (n--) { const P *q = p + 1; t += q->x * q->y + q->z; use(q); p += 2; }
    return t;
}
float b2(const P *p, int k)
{
    const P *q;
    if (k) p = p + 3;
    q = p + 1;
    t2: return q->x + q->y + q->z;
}
float b3(const P *p, int k)
{
    float t = 0;
    if (k) { p = p + 3; t = p->x + p->y; use(p); }
    return t + p->z;
}
float b4(const P *p, int n)
{
    float t = 0;
    int i;
    for (i = 0; i < n; i++) { t += p[i].x * p[i].y; }
    return t;
}
float b5(const P *p, const P *e)
{
    float t = 0;
    for (; p != e; p++) { t += p->x * p->d; }
    return t;
}
