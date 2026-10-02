#ifndef XLISTITEM_INL
#define XLISTITEM_INL

template <class T> void xListItem<T>::Remove()
{
    if (next != NULL)
    {
        next->prev = prev;
    }

    if (prev != NULL)
    {
        prev->next = next;
    }

    next = NULL;
    prev = NULL;
}

#endif
