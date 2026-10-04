// y_sfwd.cpp: store-to-static then read-back shapes (the volatile-device witnesses).
// Self-written, not game source. Question: which compilers re-load the static
// (retail does, per zMovePoint_GetMemPool / zGustInit / zGame soak / zGameScreenTransitionBegin)
// and which forward the stored register.
typedef int S32;
typedef unsigned int U32;
typedef unsigned short U16;
typedef float F32;

extern void* alloc(S32);
extern S32 count(U32);
extern void* make(S32, S32, S32);
extern void use(void*);
extern void usei(S32);

void* g_list;
S32 g_cnt;
U16 g_n16;
S32 g_idx, g_max;
void* g_ptr;
F32 g_tmr;
S32 g_out;
static S32 s_idx;
static S32 s_max;

// zMovePoint_GetMemPool: ternary of call result into static, return it
void* mp(S32 cnt)
{
    g_list = cnt ? alloc(cnt * 12) : 0;
    g_cnt = cnt;
    return g_list;
}

// return plain call result stored to static
void* mp2(S32 cnt)
{
    g_list = alloc(cnt);
    g_cnt = cnt;
    return g_list;
}

// zGustInit: U16 static from call, read back
void gust()
{
    g_n16 = count(0x47555354);
    U32 n = g_n16;
    if (n)
        g_list = alloc(n * 4);
}

// zGame soak: increment then compare
void soak()
{
    g_idx++;
    if (g_idx < g_max)
        g_out = 1;
}

void ssoak()
{
    s_idx++;
    if (s_idx < s_max)
        g_out = 1;
}

// zGameScreenTransitionBegin: call result into static, test, pass
void trans()
{
    g_ptr = make(640, 480, 0);
    if (g_ptr != 0)
        use(g_ptr);
}

// float timer
void tmr(F32 dt)
{
    g_tmr -= dt;
    if (g_tmr < 0.0f)
        g_out = 2;
}
