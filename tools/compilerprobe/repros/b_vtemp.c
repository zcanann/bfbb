/* (b) clause V vs. stores to compiler temporaries.
 * A float->int (fctiwz + stfd/lwz) or int->float (stw/lfd) conversion goes
 * through a stack slot the COMPILER invents. 2.0p1a's clause V treats the
 * stfd/stw to that slot as a store that kills every cached .sdata2 literal,
 * so the next use of a literal reloads it.
 * Expected: 2.0p1 == 2.5 == 2.6 == 2.7 (literal loaded once);
 *           2.0p1a reloads; 2.0p1b/c/d == the stock code.             */
typedef unsigned char u8;
typedef struct { u8 r, g, b, a; } Col;

int two_conv(float w, float x)
{
    return (int)(w * 255.0f + 0.5f) + (int)(x * 255.0f + 0.5f);
}

extern void SetColour(Col c);
void colour(float r, float g, float b)
{
    Col c;
    c.r = (u8)(255.0f * r);
    c.g = (u8)(255.0f * g);
    c.b = (u8)(255.0f * b);
    c.a = 255;
    SetColour(c);
}

float int_to_float(int a, int b, float s)
{
    return (float)a * s * 0.25f + (float)b * 0.25f;
}

int loop_conv(const float *v, int n)
{
    int i, s = 0;
    for (i = 0; i < n; i++)
        s += (int)(v[i] * 1000.0f + 0.5f);
    return s;
}

unsigned int to_unsigned(float a, float b)
{
    unsigned int x = (unsigned int)(a * 65536.0f);
    unsigned int y = (unsigned int)(b * 65536.0f);
    return x ^ y;
}
