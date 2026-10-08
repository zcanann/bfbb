typedef struct S { int a, b, c, d; } S;
static void f(S* p, S* q)
{
    p->a = q->a;
}
void g(S* p, S* q) { f(p, q); }
