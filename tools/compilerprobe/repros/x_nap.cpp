/* nap: copy of a register value into an ADDRESS-TAKEN local, then read. Expected: [2.0p1 2.0p1e 2.7] (IRO propagates the copy, no frsp) vs [nap 3.0a3 3.0a5.2] (read stays a load; backend forwards it with frsp). sb2 matches sb2_arr (the zNPCTypeBossSB2 hack form) under nap. */
// address-taken float local copied from a register value, then read as an argument
float at2(float, float);
void accel(float& x, float& v, float a, float dt, float endx, float maxv);
void setyaw(float);
struct T { float vel, accel, maxv; float dx, dy; };

// SB2 shape: y = start; d = y + d; accel(y, ..., d)
void sb2(T* t, float dt, float start)
{
    float end = at2(t->dx, t->dy);
    float diff = end - start;
    if (diff > 3.14f) diff -= 6.28f;
    float yaw = start;
    diff = yaw + diff;
    accel(yaw, t->vel, t->accel, dt, diff, t->maxv);
    setyaw(yaw);
}
// array witness form (matches retail on 2.0p1e)
void sb2_arr(T* t, float dt, float start)
{
    float end = at2(t->dx, t->dy);
    float diff = end - start;
    if (diff > 3.14f) diff -= 6.28f;
    float yaw[1];
    yaw[0] = start;
    diff = yaw[0] + diff;
    accel(yaw[0], t->vel, t->accel, dt, diff, t->maxv);
    setyaw(yaw[0]);
}
// Plankton honest-direct: cur address-taken from the start
void pk(T* t, float dt, float ax, float ay)
{
    float cur = at2(ax, ay);
    float tgt = at2(t->dx, t->dy);
    float diff = tgt - cur;
    if (diff > 3.14f) diff -= 6.28f;
    accel(cur, t->vel, t->accel, dt, cur + diff, t->maxv);
    setyaw(cur);
}
// Plankton honest-copy
void pk_copy(T* t, float dt, float ax, float ay)
{
    float cur = at2(ax, ay);
    float tgt = at2(t->dx, t->dy);
    float diff = tgt - cur;
    if (diff > 3.14f) diff -= 6.28f;
    float y = cur;
    accel(y, t->vel, t->accel, dt, y + diff, t->maxv);
    setyaw(y);
}
// int control: S32 address-taken copy
void take(int&);
int ictl(int a, int b)
{
    int y = a;
    int z = y + b;
    take(y);
    return z + y;
}
