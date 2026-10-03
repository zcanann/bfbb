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

template <class T> void xListItem<T>::Insert(T* list)
{
    prev = list;
    next = list->next;

    if (list->next != NULL)
    {
        list->next->prev = (T*)this;
    }

    list->next = (T*)this;
}

template <class T> T* xListItem<T>::RemHead(T** listhead)
{
    if (*listhead == NULL)
    {
        return NULL;
    }

    T* head = (*listhead)->Head();

    if (head == NULL)
    {
        *listhead = NULL;
    }
    else
    {
        *listhead = head->Next();
        head->Remove();
    }

    return head;
}

template <class T> T* xListItem<T>::Head()
{
    T* item = (T*)this;

    if (item == NULL)
    {
        return item;
    }

    while (item->prev != NULL)
    {
        item = item->prev;
    }

    return item;
}

#endif
