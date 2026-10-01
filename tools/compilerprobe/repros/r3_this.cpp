/* (c-R3) in C++: `this` in a const member function is a pointer-to-const, so
 * 2.5 hoists this->field out of loops with calls; a non-const member does not.
 * Compiled with the game's C++ flags (run.py does that for .cpp files).
 * Expected: sum_c: [2.0p1 2.0p1a 2.0p1b] != [2.5 2.6 2.7 2.0p1c 2.0p1d];
 *           sum_nc: one group.                                              */
extern int g(int);
struct S {
    int n;
    int *m;
    int sum_c() const;
    int sum_nc();
};
int S::sum_c() const { int s = 0; for (int i = 0; i < n; i++) s += g(m[i]); return s; }
int S::sum_nc()      { int s = 0; for (int i = 0; i < n; i++) s += g(m[i]); return s; }
