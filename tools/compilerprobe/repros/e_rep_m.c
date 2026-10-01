/* GC/2.0p1e part rep: 2.5 alias for a const/restrict pointee (unrolled
 * const-pointer loops keep one addi per copy; addi off a twice-defined const
 * pointer). Expected: 2.5 == 2.0p1e, and 2.0p1/2.0p1d differ on some functions. */
extern int q(int);
int m1(const int *s)
{
    int t = 0;
    t += s[0] * s[1]; s += 3;
    t += s[0] * s[1]; s += 3;
    t += s[0] * s[1]; s += 3;
    return t + s[0];
}
int m2(const int *s, int k)
{
    if (s[0] == k) return 1; s += 3;
    if (s[0] == k) return 2; s += 3;
    if (s[0] == k) return 3; s += 3;
    return s[0];
}
int m3(const int *s, int n)
{
    int t = 0;
    while (n--) {
        t += s[0];
        if (t > 100) return t;
        s += 3;
        t += s[1];
        if (t > 200) return 0;
        s += 3;
    }
    return t;
}
