// Private trailing helper definitions owned by xScene.
#ifndef XSCENE_HELPERS_H
#define XSCENE_HELPERS_H

template <> U16 range_limit<U16>(U16 v, U16 minv, U16 maxv)
{
    if (v <= minv)
    {
        return minv;
    }

    if (v >= maxv)
    {
        return maxv;
    }

    return v;
}

#if defined(PS2)
inline
#endif
void xBoxFromRay(xBox& box, const xRay3& ray)
{
    xLine3 line;

    if (ray.flags & 0x400)
    {
        F32 x = ray.dir.x * ray.min_t;
        F32 y = ray.dir.y * ray.min_t;
        F32 z = ray.dir.z * ray.min_t;

        line.p1.x = ray.origin.x + x;
        line.p1.y = ray.origin.y + y;
        line.p1.z = ray.origin.z + z;
    }
    else
    {
        line.p1.x = ray.origin.x;
        line.p1.y = ray.origin.y;
        line.p1.z = ray.origin.z;
    }

    if (ray.flags & 0x800)
    {
        F32 dist = (ray.flags & 0x400) ? ray.max_t - ray.min_t : ray.max_t;

        line.p2.x = ray.dir.x * dist;
        line.p2.y = ray.dir.y * dist;
        line.p2.z = ray.dir.z * dist;
    }
    else
    {
        line.p2.x = ray.dir.x;
        line.p2.y = ray.dir.y;
        line.p2.z = ray.dir.z;
    }

    line.p2.x = line.p1.x + line.p2.x;
    line.p2.y = line.p1.y + line.p2.y;
    line.p2.z = line.p1.z + line.p2.z;

    xBoxFromLine(box, line);
}

void xBoxFromLine(xBox& box, const xLine3& line)
{
    box.upper.x = MAX(line.p1.x, line.p2.x);
    box.upper.y = MAX(line.p1.y, line.p2.y);
    box.upper.z = MAX(line.p1.z, line.p2.z);
    box.lower.x = MIN(line.p1.x, line.p2.x);
    box.lower.y = MIN(line.p1.y, line.p2.y);
    box.lower.z = MIN(line.p1.z, line.p2.z);
}

#endif
