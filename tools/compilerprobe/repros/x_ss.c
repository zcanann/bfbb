/* (B) two stores to DIFFERENT statics with DIFFERENT opcodes: may the scheduler swap them?
 * Witness: zMusicNotify / zMusicNotifyEvent (zMusic.cpp, timer-first source): retail emits
 * `stwx s -> sMusicQueueData[t]` above the earlier `stfsx -> sMusicTimer[t]`, because the
 * queue store feeds the following load of sMusicQueueData[t] (longer critical path).
 * Stock 2.0p1 / 2.5 / 2.6 / 2.7 answer "no alias" for the store pair; 2.0p1a..f's clause C+
 * (AliasPatch.c, differing opcodes, both static) answers "may alias" and pins source order.
 * Expected (RW and game flags alike; ss0/ss = tools/compilerprobe/patch_licm_x.py builds):
 *   q_noglob, q_ptr, q_rev   [2.0p1 2.5 2.6 2.7 ss0 ss] [3.0a3 3.0a5.2] [2.0p1a 2.0p1f]
 *       -- ss0 is byte-identical to the stock compilers: the pointer store moves up.
 *   q_idx, q_const           2.0p1f != ss0, but the float store still precedes the
 *       gMode load (C+'s store/LOAD answer is untouched; retail keeps that edge too).
 *   sameop_idx, lit_idx      ss0 == 2.0p1f (controls: clause B, and C+ for a literal). */
typedef struct S { int game_state; int music_enum; } S;
extern float gTimer[2];
extern S *gQueue[2];
extern int gMode;
extern float gF[2];
extern int gI[2];

void q_idx(S *s, int t, float d)          /* the witness shape */
{
    gTimer[t] = d;
    gQueue[t] = s;
    gQueue[t]->game_state = (gMode == 12);
}

void q_const(S *s, float d)               /* constant index: direct sym stores */
{
    gTimer[1] = d;
    gQueue[1] = s;
    gQueue[1]->game_state = (gMode == 12);
}

void q_rev(S *s, int t, float d)          /* int store first, float second */
{
    gQueue[t] = s;
    gTimer[t] = d;
    gTimer[t] += gF[0];
}

void sameop_idx(int a, int b, int t)      /* two stw to different statics: B keeps order */
{
    gI[t] = a;
    gMode = b;
    gI[t] += gMode;
}

float lit_idx(int t, float d)             /* a float literal load after an int store */
{
    gI[t] = 3;
    return d * 1.5f;
}

void q_noglob(S *s, int t, float d)       /* no other static read after the stores */
{
    gTimer[t] = d;
    gQueue[t] = s;
    gQueue[t]->game_state = 1;
}

void q_ptr(S *s, int t, float *d)         /* the float comes from memory */
{
    gTimer[t] = *d;
    gQueue[t] = s;
    gQueue[t]->music_enum = t;
}

/* controls for ssi (both indirect AND different objects): */
extern int gCount;
extern S *gList[256];
void c_direct(S *s)                       /* direct-symbol stw + indexed stwx (FindAndInstanceAtomicCallback) */
{
    gCount = gCount + 1;
    gList[gCount] = s;
    gList[gCount]->game_state = 1;
}

typedef struct G { float f; int n; } G;
extern G gG[64];
void c_same(int t, float f)               /* two stores into ONE array, different opcodes (zLasso_AddGuide) */
{
    gG[t].f = f;
    gG[t].n = t;
    gG[t + 1].n = gG[t].n + 1;
}
