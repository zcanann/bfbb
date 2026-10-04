/* nap: see x_nap.cpp. p1 p2 p5 q4 r6 group nap with 3.0a3/3.0a5.2 (frsp on the forwarded F32 copy); p3/q1-q3/r1/r5 unchanged everywhere. */
float at2(float, float);
void accelp(float* x, float endx);
void sety(float);
void useptr(float*);
/* pointer arg */
void p1(float a, float b, float d) { float s = at2(a, b); float y = s; accelp(&y, y + d); sety(y); }
/* copy consumed before the address is taken */
void p2(float a, float b, float d) { float s = at2(a, b); float y = s; float z = y + d; accelp(&y, z); sety(y); }
/* never address-taken: register copy */
void p3(float a, float b, float d) { float s = at2(a, b); float y = s; sety(y + d); sety(y); }
/* address taken, value from a constant */
void p4(float d) { float y = 1.5f; accelp(&y, y + d); sety(y); }
/* address taken, value from a param */
void p5(float s, float d) { float y = s; accelp(&y, y + d); sety(y); }
/* double local */
void accd(double* x, double e);
void p6(double s, double d) { double y = s; accd(&y, y + d); }
/* int */
void takei(int* x, int e);
void p7(int s, int d) { int y = s; takei(&y, y + d); }
/* short (conversion visible) */
void takes(short* x, int e);
void p8(int s, int d) { short y = s; takes(&y, y + d); }
float g_;
void q1(int d) { int y = 5; takei(&y, y + d); }
void q2(float d) { float y = g_; accelp(&y, y + d); }
void q3(float* p, float d) { float y = *p; accelp(&y, y + d); }
void q4(float s, float d) { float y; y = s; accelp(&y, y * d); y = s + 1.0f; accelp(&y, y * d); }

typedef struct { float x, y; } V2; void takev(V2* v, float e);
/* addr-taken <- addr-taken */
void r1(float s, float d) { float a = s; float b; accelp(&a, d); b = a; accelp(&b, b + d); }
/* regable <- addr-taken (stock already refuses) */
void r2(float s, float d) { float a = s; float b; accelp(&a, d); b = a; sety(b + d); sety(b); }
/* addr-taken int */
void r3(int s, int d) { int y = s; takei(&y, y + d); takei(&y, y); }
/* addr-taken double */
void r4(double s, double d) { double y = s; accd(&y, y + d); }
/* struct member */
void r5(float s, float d) { V2 v; v.x = s; v.y = d; takev(&v, v.x + v.y); }
/* copy, then reassigned before address-take */
void r6(float s, float d) { float y = s; sety(y * d); y = d; accelp(&y, y + s); }
/* loop */
void r7(float s, float d, int n) { int i; float y = s; for (i = 0; i < n; i++) { accelp(&y, y + d); } }
/* short */
void r8(short s, int d) { short y = s; takes(&y, y + d); }
