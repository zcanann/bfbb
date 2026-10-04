/* WE: whole load of a small static, then stores to an escaping frame struct; the loaded value is used after them */
typedef struct N { int a; short b, c; unsigned char d; } N;
extern int extoff;
extern float extf;
extern int wr(void *, N *, int);
extern int wrf(void *, N *, float);
int we_int(void *s, int *raster)
{
    N n;
    int off = extoff;
    n.a = raster[0];
    n.b = 3;
    n.c = 7;
    n.d = 1;
    return wr(s, &n, off);
}
int we_flt(void *s, int *raster)
{
    N n;
    float f = extf;
    n.a = raster[0];
    n.b = 3;
    n.c = 7;
    n.d = 1;
    return wrf(s, &n, f);
}
/* control: the frame struct does not escape */
int we_ctl(int *raster)
{
    N n;
    int off = extoff;
    n.a = raster[0];
    n.b = 3;
    return n.a + n.b + off;
}
/* E3nc control: non-const global (plain E3n already orders it) */
typedef struct V { float x, y, z; } V;
extern int disable_nc;
extern const int disable_c;
extern void drawat(V *);
void e3n_nc(float a, float b, float c)
{
    V v;
    v.x = a * 0.5f;
    v.y = b * 0.5f;
    v.z = c * 0.5f;
    if (!disable_nc)
        drawat(&v);
}
void e3n_c(float a, float b, float c)
{
    V v;
    v.x = a * 0.5f;
    v.y = b * 0.5f;
    v.z = c * 0.5f;
    if (!disable_c)
        drawat(&v);
}
/* _rwDlNativeTextureWrite shape: frame fields filled from a pointer, the static offset used after */
typedef struct R { int fmt; unsigned int flags; } R;
int we_tex(void *s, R *raster, int size)
{
    N n;
    char *ext;
    n.a = raster->fmt;
    n.b = (short)(raster->flags & 0xff);
    n.c = (short)size;
    ext = (char *)raster + extoff;
    n.d = (unsigned char)*ext;
    return wr(s, &n, *(int *)(ext + 4));
}
int we_tex2(void *s, R *raster, int size)
{
    N n;
    int off;
    n.a = raster->fmt;
    off = extoff;
    n.b = (short)(raster->flags & 0xff);
    n.c = (short)size;
    n.d = 1;
    return wr(s, &n, *(int *)((char *)raster + off));
}
