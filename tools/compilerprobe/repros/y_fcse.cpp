// y_fcse.cpp: CSE of a frame-object address (&local.member) between an early direct
// member store and a later pass-by-address. Self-written, shaped after iRayHitsModel.
// Retail computes `addi rN, r1, off` at the later use; 2.0p1f hoists it to the early store.
typedef float F32;
struct V3 { F32 x, y, z; V3& operator=(const V3&); };
struct Line { V3 start, end; };
struct Isx { int type; Line line; };
extern void each(Isx*, void*);
extern void each2(Isx*, void*);
extern void assign(V3*, const V3*);

void f1(const V3* a, const V3* b, void* c)
{
    Isx isx;
    isx.type = 1;
    isx.line.start.x = a->x;
    isx.line.start.y = a->y;
    isx.line.start.z = a->z;
    isx.line.end.x = isx.line.start.x + b->x;
    isx.line.end.y = isx.line.start.y + b->y;
    isx.line.end.z = isx.line.start.z + b->z;
    each(&isx, c);
    V3 temp = isx.line.start;
    isx.line.start = isx.line.end;
    isx.line.end = temp;
    each2(&isx, c);
}

void f2(const V3* a, const V3* b, void* c)
{
    Isx isx;
    isx.type = 1;
    isx.line.end.x = a->x + b->x;
    isx.line.end.y = a->y + b->y;
    isx.line.end.z = a->z + b->z;
    each(&isx, c);
    assign(&isx.line.end, a);
    assign(&isx.line.end, b);
    each2(&isx, c);
}

void f3(const V3* a, void* c)
{
    Isx isx;
    isx.line.end.x = a->x;
    isx.line.end.y = a->y;
    isx.line.end.z = a->z;
    each(&isx, c);
    assign(&isx.line.end, a);
}

void f4(const V3* a, void* c)
{
    Isx isx;
    each(&isx, c);
    assign(&isx.line.end, a);
    each(&isx, c);
    assign(&isx.line.end, a);
}
