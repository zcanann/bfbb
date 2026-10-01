/* GC/2.0p1e part rep: 2.5 alias for a const/restrict pointee (unrolled
 * const-pointer loops keep one addi per copy; addi off a twice-defined const
 * pointer). Expected: 2.5 == 2.0p1e, and 2.0p1/2.0p1d differ on some functions. */
typedef struct { float x, y, z, d; int pad; } P;
int u1(const P *p, float r)   /* for with index */
{
    int i, res = 0;
    for (i = 0; i < 6; i++) {
        float t = p[i].x - p[i].d;
        if (t > r) return 2;
        if (t > -r) res = 1;
    }
    return res;
}
int u2(const P *p, float r)   /* for with pointer inc */
{
    int i, res = 0;
    for (i = 0; i < 6; i++) {
        float t = p->x - p->d;
        if (t > r) return 2;
        if (t > -r) res = 1;
        p++;
    }
    return res;
}
int u3(const P *p, float r, int n)   /* variable trip, pointer inc */
{
    int res = 0;
    while (n--) {
        float t = p->x - p->d;
        if (t > r) return 2;
        p++;
    }
    return res;
}
int u4(const int *s, int n)   /* no exits */
{
    int i, t = 0;
    for (i = 0; i < 12; i++) { t += *s; s += 3; }
    return t;
}
int u5(int *d, const int *s)
{
    int i;
    for (i = 0; i < 12; i++) { *d = *s; d += 2; s += 3; }
    return 0;
}
