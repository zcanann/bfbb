/* GC/2.0p1e part rep: 2.5 alias for a const/restrict pointee (unrolled
 * const-pointer loops keep one addi per copy; addi off a twice-defined const
 * pointer). Expected: 2.5 == 2.0p1e, and 2.0p1/2.0p1d differ on some functions. */
typedef struct { float x, y, z, d; int pad; } P;
int f(const P *p, float cx, float cy, float cz, float r)
{
    int n = 6, res = 0;
    while (n--) {
        float t = cx * p->x + cy * p->y + cz * p->z - p->d;
        if (t > r) return 2;
        if (t > -r) res = 1;
        p++;
    }
    return res;
}
/* integer variant: pointer walk with loads + stores */
void g(int *d, const int *s, int k)
{
    int n = 6;
    while (n--) {
        if (*s == k) return;
        *d = *s + k;
        d += 3; s += 2;
    }
}
/* plain counted loop with no exits */
int h(const int *s)
{
    int i, t = 0;
    for (i = 0; i < 8; i++) { t += s[0] * s[1]; s += 3; }
    return t;
}
