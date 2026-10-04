typedef struct C { float kt; } C;
typedef struct S { char *name; int h; C *carry; } S;
typedef struct T { float kt; S *s; int pad[20]; } T;
extern S models[10];
extern int count;
extern T list[32];

/* zThrown_LaunchVel shape: pointer into a static array (via loop), store to another static, load through it */
void thr_loop(int id)
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

/* direct pointer to static element (no loop) */
void thr_fixed(void)
{
    S *s = &models[3];
    T *t = &list[count];
    count++;
    t->kt = s->carry->kt;
}

/* pointer parameter (unknown) control */
void thr_param(S *s)
{
    T *t = &list[count];
    count++;
    t->kt = s->carry->kt;
}

/* reuse (VN) shape: AddFruit */
extern C c_fruit;
void thr_vn(int id)
{
    S *s = models + 1;
    T *t;
    while (s->name) { if (s->h == id) break; s++; }
    if (s->carry != &c_fruit) return;
    t = &list[count];
    count++;
    t->kt = s->carry->kt;
}

/* direct field of static, not through pointer: control */
void thr_direct(void)
{
    T *t = &list[count];
    count++;
    t->kt = models[3].carry->kt;
}
