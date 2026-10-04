// y_kcse.cpp: a variable initialised to a constant, then the same constant stored
// elsewhere before the variable is reassigned. Self-written, shaped after zEntPlayer_StreakFX
// (retail: `li r31,0` is cp's register AND the source of the eleven `stw r31` zero stores).
typedef int S32;
struct Info { S32 activated; S32 pad[13]; };
extern Info sInfo[3][4];
extern Info* g_ip;
extern S32 g_a, g_b;
extern void use(S32);
extern void use2(S32, S32);

void k1(void* m, void* a, void* b)
{
    S32 i, p;
    S32 cp = 0;
    for (i = 0; i < 3; i++)
        for (p = 0; p < 4; p++)
            sInfo[i][p].activated = 0;
    if (m == a)
        cp = 1;
    else if (m == b)
        cp = 2;
    use(cp);
    use(cp);
}

void k2(S32* d, S32 x)
{
    S32 cp = 0;
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    if (x)
        cp = 1;
    use(cp);
}

void k3(S32* d, S32 x)
{
    S32 cp = 0;
    d[0] = 0;
    if (x)
        cp = 1;
    use(cp);
}

void k4(S32* d, S32 x)
{
    S32 cp = 5;
    d[0] = 5;
    d[1] = 5;
    if (x)
        cp = 1;
    use(cp);
    use(cp);
}

// constant first, variable later
void k5(S32* d, S32 x)
{
    d[0] = 0;
    d[1] = 0;
    S32 cp = 0;
    if (x)
        cp = 1;
    use(cp);
    use(cp);
}

// variable never reassigned (pure constant)
void k6(S32* d, S32 x)
{
    S32 cp = 0;
    d[0] = 0;
    d[1] = 0;
    use2(cp, x);
}
