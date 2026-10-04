/* (A) provenance probes: a stack aggregate that later compilers do not
 * scalarise, read field by field (offset 0 included) inside a loop. */
typedef unsigned char U8;
struct V { float x, y, z; U8 r, g, b, a; float u, v; };
extern float sn(float);
struct BRGBA { U8 r, g, b, a; int pad[6]; };
struct BObj { BRGBA bot; };

void big(BObj* o, V* vtx)
{
    BRGBA bot = o->bot;
    for (int i = 0; i < 8; i++) {
        vtx->x = sn(i * 0.5f);
        vtx->r = bot.r; vtx->g = bot.g; vtx->b = bot.b; vtx->a = bot.a;
        vtx++;
    }
}

struct IV { int a, b, c, d; int pad[4]; };
void bigint(const IV* s, int* out)
{
    IV l = *s;
    for (int i = 0; i < 8; i++) {
        out[0] = (int)sn(i);
        out[1] = l.a; out[2] = l.b; out[3] = l.c; out[4] = l.d;
        out += 5;
    }
}

/* array local, element 0 included */
void arr(const int* s, int* out)
{
    int l[4];
    l[0] = s[0]; l[1] = s[1]; l[2] = s[2]; l[3] = s[3];
    for (int i = 0; i < 8; i++) {
        out[0] = (int)sn(i);
        out[1] = l[0]; out[2] = l[1]; out[3] = l[2]; out[4] = l[3];
        out += 5;
    }
}

/* struct local filled field by field from a call result, then read in a loop */
extern int gi(int);
struct S3 { int a, b, c; };
void filled(int* out)
{
    S3 l;
    l.a = gi(1); l.b = gi(2); l.c = gi(3);
    for (int i = 0; i < 8; i++) {
        out[0] = gi(i);
        out[1] = l.a; out[2] = l.b; out[3] = l.c;
        out += 4;
    }
}

/* offset-0 field CSE without a loop: read twice around a store elsewhere */
struct P2 { short a, b; };
int cse0(const P2* s, short* out)
{
    P2 l = *s;
    out[0] = l.a + 1;
    out[1] = l.b + 1;
    gi(0);
    return l.a + l.b;
}
