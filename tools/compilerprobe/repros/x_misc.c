/* L8: int->float conversion constant (lfd 0x4330000080000000) after stores to a static */
extern float ctab[256];
extern int ctabinit;
void l8_loop(void)
{
    int i;
    if (!ctabinit) {
        for (i = 0; i < 256; i++)
            ctab[i] = i / 255.0f;
        ctabinit = 1;
    }
}
extern int vals[5];
extern float outv[5];
void l8_ptr(int i, int v)
{
    int *p = &vals[i];
    *p = v;
    outv[i] = (float)v;
}

/* WE: a whole load of a small named static, then a store to an escaping frame struct (_rwDlNativeTextureWrite) */
typedef struct N { int a; short b, c; unsigned char d; } N;
extern int extoff;
extern int wr(void *, N *, int);
int we_f(void *s, char *raster)
{
    N n;
    char *ext;
    n.a = 5;
    ext = raster + extoff;
    n.b = *(short *)ext;
    n.c = 7;
    n.d = 1;
    return wr(s, &n, 9);
}

/* E3nc: a store to an escaping frame struct, then a load of a const-qualified global (zEntPlayer_Render) */
typedef struct V { float x, y, z; } V;
extern const int disable;
extern void drawat(V *);
void e3nc_f(float a, float b, float c)
{
    V v;
    v.x = a * 0.5f;
    v.y = b * 0.5f;
    v.z = c * 0.5f;
    if (!disable)
        drawat(&v);
}
