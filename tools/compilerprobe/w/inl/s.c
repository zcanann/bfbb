typedef struct S { int a, b, c, d; int *p; } S;
extern int ext(int);
extern int ext2(int, int);
static void f(S* p, S* q)
{
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 0;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 1;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 2;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 3;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 4;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 5;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 6;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 7;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 8;
    p->a = q->b + q->b + q->b + q->b + q->b + q->b + q->b + q->b + 9;
}
void g(S* p, S* q) { f(p, q); }
