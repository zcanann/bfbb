struct V3 { float x, y, z; }; struct V2 { float x, y; float len2() const { return x*x + y*y; } };
V3 ret3(const V3&, const V3&);
void sink(V2*); void sink3(V3*);
struct O { V3 c; float r; V3 vel; };
void imp(O* o, const V3& v, const V3& loc)
{
    V3 add = ret3(v, loc);
    add.y = 0.0f;
    const V2 diff = { loc.x - o->c.x, loc.z - o->c.z };
    if (diff.len2() > o->r * o->r) add.z = 0.0f;
    o->vel.x += add.x; o->vel.y += add.y; o->vel.z += add.z;
}
