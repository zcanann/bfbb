#include "xCounter.h"

#if defined(PS2)
#include "xDebugTweak.h"
#else
#include "xDebug.h"
#endif
#include "xEvent.h"

namespace
{
#if defined(PS2)
    inline
#endif
    void add_tweaks(_xCounter&)
    {
        return;
    }
} // namespace

void xCounterInit()
{
    xDebugRemoveTweak("Widgets|Counters");
}

#if defined(PS2)
// The PS2 retail entry owns the initialization body directly.
void xCounterInit(void* b, void* counterAsset)
{
    xCounterAsset* asset = (xCounterAsset*)counterAsset;
#else
void xCounterInit(void* b, void* asset)
{
    xCounterInit((xBase*)b, (xCounterAsset*)asset);
}

void xCounterInit(xBase* b, xCounterAsset* asset)
{
#endif
    _xCounter* t = (_xCounter*)b;

    xBaseInit(t, asset);

    t->eventFunc = xCounterEventCB;
    t->asset = asset;

    if (t->linkCount)
    {
        t->link = (xLinkAsset*)(t->asset + 1);
    }
    else
    {
        t->link = 0;
    }

    t->state = 0;
    t->count = asset->count;
    t->counterFlags = 0;

    add_tweaks(*t);
}

void xCounterReset(xBase* b)
{
    _xCounter* t = (_xCounter*)b;

    xBaseInit(t, t->asset);

    if (t->linkCount)
    {
        t->link = (xLinkAsset*)(t->asset + 1);
    }
    else
    {
        t->link = 0;
    }

    t->count = t->asset->count;
    t->state = 0;
}

void xCounterSave(_xCounter* ent, xSerial* s)
{
    xBaseSave(ent, s);

    s->Write(ent->state);
    s->Write(ent->count);
}

void xCounterLoad(_xCounter* ent, xSerial* s)
{
    xBaseLoad(ent, s);

    s->Read(&ent->state);
    s->Read(&ent->count);
}

int xCounterEventCB(xBase*, xBase* to, U32 toEvent, const F32*, xBase*)
{
    _xCounter* t = (_xCounter*)to;

    switch (toEvent)
    {
    case eEventDecrement:
    {
        if (t->state != 1 && (!(t->counterFlags & 0x1) || t->count != 2))
        {
            t->count--;

            if (t->count == 0)
            {
                zEntEvent(t, t, eEventExpired);
            }
            else if (t->count > 0 && t->count <= 20)
            {
                zEntEvent(t, t, (U32)(t->count - 1) + eEventCount1);
            }
        }
        break;
    }
    case eEventIncrement:
    {
        if (t->state != 1)
        {
            t->count++;

            if (t->count == 0)
            {
                zEntEvent(t, t, eEventExpired);
            }
            else if (t->count > 0 && t->count <= 20)
            {
                zEntEvent(t, t, (U32)(t->count - 1) + eEventCount1);
            }
        }
        break;
    }
    case eEventReset:
    {
        t->state = 0;
        t->count = t->asset->count;
        break;
    }
    case eEventExpired:
    {
        if (!(t->counterFlags & 0x1) || t->count != 2)
        {
            t->count = 0;
            t->state = 1;
        }
        break;
    }
    default:
    {
        if (t->state != 1 && toEvent >= eEventCount1 && toEvent <= eEventCount20)
        {
            S16 newCount = toEvent - (eEventCount1 - 1);

            if (!(t->counterFlags & 0x1) || t->count != 2)
            {
                t->count = newCount;
            }
        }
    }
    }

    return 1;
}
