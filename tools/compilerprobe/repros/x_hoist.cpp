/* (A) order of loop-invariant loads hoisted out of a loop.
 * Witnesses: NPCBlinker::Render, NPCCone::RenderCone (zNPCSupport.cpp).
 * Four byte loads of a stack RwRGBA are loop-invariant; retail hoists
 * them red, green, blue, alpha; 2.0p1f emits red last.
 * Cause: IRO_IsExpressionCandidate (2.0p1 0x457540) treats indirect(OBJREF local) with
 * VarInfo noregister == 0 as a register-variable read, whatever the local's type, so the
 * offset-0 member of a struct local is never an IRO (CSE/LICM) expression; g/b/a are
 * indirect(add(&bot,k)) and are. IroLoop's mover (0x4a6794) hoists g,b,a as '@' temps;
 * the backend's moveinvariantsfromloop hoists r afterwards.
 * Expected: cone/direct/rotated/nocall/stkflt: every compiler 1.3.2 .. 2.7 loads offset 0
 * last; 3.0a3/3.0a5.2 scalarise the 4-byte copy but on x_hoist2.cpp `big` (a 28-byte
 * struct they keep in memory) also load offset 0 last. patch_licm_x.py --parts agg
 * (aggregate member reads are candidates) gives r,g,b,a order but measures -10 / +0. */
typedef unsigned char U8;
struct RGBA { U8 r, g, b, a; };
struct V { float x, y, z; U8 r, g, b, a; float u, v; };
extern float sn(float);
extern RGBA gcol;
struct Obj { RGBA top, bot; float rad; };

/* the witness shape: local struct copy, macro-style inner temp, call in loop */
void cone(Obj* o, V* vtx)
{
    RGBA bot = o->bot;
    for (int i = 0; i < 8; i++) {
        vtx->x = sn(i * 0.5f);
        {
            RGBA col;
            col.r = bot.r; col.g = bot.g; col.b = bot.b; col.a = bot.a;
            vtx->r = col.r; vtx->g = col.g; vtx->b = col.b; vtx->a = col.a;
        }
        vtx++;
    }
}

/* without the inner temp */
void direct(Obj* o, V* vtx)
{
    RGBA bot = o->bot;
    for (int i = 0; i < 8; i++) {
        vtx->x = sn(i * 0.5f);
        vtx->r = bot.r; vtx->g = bot.g; vtx->b = bot.b; vtx->a = bot.a;
        vtx++;
    }
}

/* rotated source order a,r,g,b */
void rotated(Obj* o, V* vtx)
{
    RGBA bot = o->bot;
    for (int i = 0; i < 8; i++) {
        vtx->x = sn(i * 0.5f);
        vtx->a = bot.a; vtx->r = bot.r; vtx->g = bot.g; vtx->b = bot.b;
        vtx++;
    }
}

/* four invariant computations, not loads */
extern int gi(int);
void ints(int* out, int p, int q)
{
    int k0 = p + 1, k1 = p + 2, k2 = p + 3, k3 = p + 4;
    for (int i = 0; i < 8; i++) {
        out[0] = gi(i);
        out[1] = k0 * q; out[2] = k1 * q; out[3] = k2 * q; out[4] = k3 * q;
        out += 5;
    }
}

/* invariant loads through a parameter pointer (not a stack object) */
struct S4 { int a, b, c, d; };
void ptrld(const S4* s, int* out)
{
    for (int i = 0; i < 8; i++) {
        out[0] = gi(i);
        out[1] = s->a; out[2] = s->b; out[3] = s->c; out[4] = s->d;
        out += 5;
    }
}

/* invariant loads of a stack struct of ints */
void stkint(const S4* s, int* out)
{
    S4 l = *s;
    for (int i = 0; i < 8; i++) {
        out[0] = gi(i);
        out[1] = l.a; out[2] = l.b; out[3] = l.c; out[4] = l.d;
        out += 5;
    }
}

/* stack struct of floats */
struct F4 { float a, b, c, d; };
void stkflt(const F4* s, float* out)
{
    F4 l = *s;
    for (int i = 0; i < 8; i++) {
        out[0] = sn(i);
        out[1] = l.a; out[2] = l.b; out[3] = l.c; out[4] = l.d;
        out += 5;
    }
}

/* byte loads through a const parameter pointer */
void ptrb(const RGBA* c, V* vtx)
{
    for (int i = 0; i < 8; i++) {
        vtx->x = sn(i * 0.5f);
        vtx->r = c->r; vtx->g = c->g; vtx->b = c->b; vtx->a = c->a;
        vtx++;
    }
}

/* stack struct of ints, first field not read */
void stkint3(const S4* s, int* out)
{
    S4 l = *s;
    for (int i = 0; i < 8; i++) {
        out[0] = gi(i);
        out[1] = l.b; out[2] = l.c; out[3] = l.d;
        out += 5;
    }
}

/* stack byte struct with pad first: r is not at offset 0 */
struct PRGBA { U8 pad, r, g, b, a; };
struct PObj { PRGBA bot; };
void padded(PObj* o, V* vtx)
{
    PRGBA bot = o->bot;
    for (int i = 0; i < 8; i++) {
        vtx->x = sn(i * 0.5f);
        vtx->r = bot.r; vtx->g = bot.g; vtx->b = bot.b; vtx->a = bot.a;
        vtx++;
    }
}

/* stack struct, written field by field (not a whole copy) */
void fields(Obj* o, V* vtx)
{
    RGBA bot;
    bot.r = o->bot.r; bot.g = o->bot.g; bot.b = o->bot.b; bot.a = o->bot.a;
    for (int i = 0; i < 8; i++) {
        vtx->x = sn(i * 0.5f);
        vtx->r = bot.r; vtx->g = bot.g; vtx->b = bot.b; vtx->a = bot.a;
        vtx++;
    }
}

/* no call in the loop */
void nocall(Obj* o, V* vtx, int n)
{
    RGBA bot = o->bot;
    for (int i = 0; i < n; i++) {
        vtx->r = bot.r; vtx->g = bot.g; vtx->b = bot.b; vtx->a = bot.a;
        vtx++;
    }
}
