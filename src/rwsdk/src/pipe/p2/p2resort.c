#include <rwsdk/rwcore.h>

typedef struct tagStackEntry tagStackEntry;
struct tagStackEntry
{
    RwUInt8* l;
    RwUInt8* r;
    RwUInt32 bit;
};

#define ELEMENTKEY(_element, _keyOffset) (*(RwUInt32*)((_element) + (_keyOffset)))

#define SWAPELEMENTS(_a, _b, _size)                                                                \
    MACRO_START                                                                                    \
    {                                                                                              \
        _l = (_a);                                                                                 \
        _r = (_b);                                                                                 \
        _elementSize = (_size);                                                                    \
        while (_elementSize >= sizeof(RwUInt32))                                                   \
        {                                                                                          \
            t0 = *(RwUInt32*)_l;                                                                   \
            t1 = *(RwUInt32*)_r;                                                                   \
            *(RwUInt32*)_r = t0;                                                                   \
            *(RwUInt32*)_l = t1;                                                                   \
            _l += sizeof(RwUInt32);                                                                \
            _r += sizeof(RwUInt32);                                                                \
            _elementSize -= sizeof(RwUInt32);                                                      \
        }                                                                                          \
    }                                                                                              \
    MACRO_STOP

static RwUInt32 _msbitpos(RwUInt32 un)
{
    RwUInt32 pos;

    if (un != 0)
    {
        pos = 0;
        while (un >>= 1)
        {
            pos++;
        }
    }
    else
    {
        pos = (RwUInt32)-1;
    }

    return pos;
}

static void _repartition(RwUInt8* l, RwUInt8* r, RwUInt32 elementSize, RwUInt32 elementKeyOffset,
                         RwUInt32 partitioningBit)
{
    tagStackEntry stack[32];
    tagStackEntry* stackptr;
    RwUInt8* savel;
    RwUInt8* saver;
    RwUInt8* _l;
    RwUInt8* _r;
    RwUInt32 _elementSize;
    RwUInt32 t0;
    RwUInt32 t1;
    RwUInt8* defer_l;
    RwUInt8* defer_r;

    stackptr = stack;
    stackptr->l = l;
    stackptr->r = r;
    stackptr->bit = partitioningBit;
    stackptr++;

    do
    {
        stackptr--;
        l = stackptr->l;
        r = stackptr->r;
        partitioningBit = stackptr->bit;

        do
        {
            savel = l;
            saver = r;

            do
            {
                while (!(ELEMENTKEY(l, elementKeyOffset) & partitioningBit))
                {
                    l += elementSize;
                    if (l > r)
                    {
                        goto partitioned;
                    }
                }

                while (ELEMENTKEY(r, elementKeyOffset) & partitioningBit)
                {
                    r -= elementSize;
                    if (l > r)
                    {
                        goto partitioned;
                    }
                }

                SWAPELEMENTS(l, r, elementSize);

                l += elementSize;
                r -= elementSize;
            } while (l <= r);

        partitioned:
            partitioningBit >>= 1;
            if (partitioningBit == 0)
            {
                break;
            }

            defer_l = r + elementSize;
            if (saver >= defer_l + elementSize * 5)
            {
                stackptr->l = defer_l;
                stackptr->r = saver;
                stackptr->bit = partitioningBit;
                stackptr++;
            }

            r = l - elementSize;
            l = savel;
        } while (r >= l + elementSize * 5);
    } while (stackptr != stack);
}

static void _insertionsort(RwUInt8* elements, RwUInt32 numElements, RwUInt32 elementSize,
                           RwUInt32 elementKeyOffset)
{
    RwUInt32 keyToPlace;
    RwUInt8* p;
    RwUInt8* _l;
    RwUInt8* _r;
    RwUInt32 _elementSize;
    RwUInt32 t0;
    RwUInt32 t1;

    while (elements += elementSize, --numElements)
    {
        keyToPlace = ELEMENTKEY(elements, elementKeyOffset);
        p = elements;

        while (p -= elementSize, ELEMENTKEY(p, elementKeyOffset) > keyToPlace)
        {
            SWAPELEMENTS(p, p + elementSize, elementSize);
        }
    }
}

void _rx_rxRadixExchangeSort(void* elements, RwUInt32 numElements, RwUInt32 elementSize,
                             RwUInt32 elementKeyOffset, RwUInt32 keyLo, RwUInt32 keyHi)
{
    RwUInt32 i;
    RwUInt32 minKey;
    RwUInt32 minKeyIndex;
    RwUInt32 key;
    RwUInt8* _l;
    RwUInt8* _r;
    RwUInt32 _elementSize;
    RwUInt32 t0;
    RwUInt32 t1;

    if (elements != NULL && elementKeyOffset + sizeof(RwUInt32) <= elementSize && keyLo < keyHi)
    {
        if (numElements > 5)
        {
            _repartition((RwUInt8*)elements, (RwUInt8*)elements + (numElements - 1) * elementSize,
                         elementSize, elementKeyOffset, 1 << _msbitpos(keyHi));
        }

        if (numElements > 1)
        {
            i = 4;
            if (numElements - 1 < i)
            {
                i = numElements - 1;
            }
            minKey = ELEMENTKEY((RwUInt8*)elements + i * elementSize, elementKeyOffset);
            minKeyIndex = i;

            i--;
            do
            {
                key = ELEMENTKEY((RwUInt8*)elements + i * elementSize, elementKeyOffset);
                if (key < minKey)
                {
                    minKey = key;
                    minKeyIndex = i;
                }
            } while (i--);

            if (minKeyIndex != 0)
            {
                SWAPELEMENTS((RwUInt8*)elements, (RwUInt8*)elements + minKeyIndex * elementSize,
                             elementSize);
            }

            _insertionsort((RwUInt8*)elements, numElements, elementSize, elementKeyOffset);
        }
    }
}
