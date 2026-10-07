// TU-private definitions preserve the retail xCamera inline section.

#if !defined(PS2)
inline U32 xEntIsVisible(const xEnt* ent)
{
    return (ent->flags & 0x81) == 0x1;
}
#endif
