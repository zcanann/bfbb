/* DS repros: a direct store to a named static, then later accesses */
typedef struct C { float kt; } C;
typedef struct S { char *name; int h; C *carry; } S;
typedef struct T { float kt; S *s; int pad[20]; } T;
extern S models[10];
extern int count;
extern T list[32];
extern C c_fruit;

/* (b) RAW: later load through a pointer into another static (zThrown_LaunchVel) */
void ds_ptrload(int id)
{
    S *s = models + 1;
    T *t;
    while (s->name) { if (s->h == id) break; s++; }
    if (!s->name) s = models;
    t = &list[count];
    count++;
    t->kt = s->carry->kt;
    t->s = s;
}

/* (b) + VN: the pointer load was already made before the store (zThrown_AddFruit) */
void ds_vn(int id)
{
    S *s = models + 1;
    T *t;
    while (s->name) { if (s->h == id) break; s++; }
    if (s->carry != &c_fruit) return;
    t = &list[count];
    count++;
    t->kt = s->carry->kt;
}

extern int gMode;
extern void (*tbl[13])(int);
/* (b) RAW: indexed table load (zGameModeSwitch) */
void ds_table(int m)
{
    gMode = m;
    tbl[m](30);
}

extern int cnt;
extern void *arr[27];
/* (b) WAW: pointer-op stores of an unrolled loop (zNPCHazard_ScenePrepare) */
void ds_unroll(void)
{
    int i;
    cnt = 0;
    for (i = 0; i < 27; i++)
        arr[i] = 0;
}

typedef struct G { char pad[1745]; unsigned char flag; char pad2[100]; } G;
extern G globals;
extern long long tlast;
extern long long tnow(void);
/* (b) RAW: base-register load of a field of a big global (zSaveLoad_Tick) */
int ds_baseload(void)
{
    tlast = tnow();
    return globals.flag;
}

typedef struct L { int a, b; } L;
extern L lim;
extern void err(L *);
/* (a) WAW: store to an escaping frame struct (StalacTiteAlloc) */
int ds_frame(int x, int y)
{
    L e;
    lim.a = x + y;
    e.a = 1;
    e.b = 0x80000013;
    err(&e);
    return 0;
}

typedef struct I { int a, b, c, d, e, f; } I;
extern void getinfo(I *);
extern int fly, flypaused, flydata;
/* (a) RAW: load of an escaping frame struct (zCameraFlyStart) */
int ds_frameload(int id)
{
    I info;
    getinfo(&info);
    fly = 1;
    flypaused = 0;
    flydata = info.e;
    return info.f;
}

/* controls: later plain base-register STORE to another static (zSurfaceInitDefaultSurface): no edge */
typedef struct A { int x; unsigned char b[8]; } A;
static A sasset;
static int sprops;
extern void use(A *, int *);
void ds_ctl_plainstore(void)
{
    sprops = 3;
    sasset.b[0] = 1;
    sasset.b[1] = 2;
    use(&sasset, &sprops);
}

/* control: later DIRECT load of another static */
extern int ga, gb;
int ds_ctl_direct(int v)
{
    ga = v;
    return gb + 1;
}
