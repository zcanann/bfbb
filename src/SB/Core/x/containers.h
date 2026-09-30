#ifndef CONTAINERS_H
#define CONTAINERS_H

#include <types.h>
#include "xMemMgr.h"

struct tier_queue_allocator
{
    struct block_data
    {
        U8 prev;
        U8 next;
        U16 flags;
        void* data;
    };

    block_data* blocks;
    U32 _unit_size;
    U32 _block_size;
    U32 _block_size_shift;
    U32 _max_blocks;
    U32 _max_blocks_shift;
    U8 head;

    void init(u32 unit_size, u32 block_size, u32 max_blocks);
    void clear();

    U32 block_size() const
    {
        return _block_size;
    }

    U32 block_size_shift() const
    {
        return _block_size_shift;
    }

    u32 mod_block_size(u32 value) const
    {
        return value & (_block_size - 1);
    }

    void* get_block(u32 index) const
    {
        return blocks[index].data;
    }

    u32 log2_ceil(u32 value) const
    {
        U32 shift = 0;

        while (value > 1)
        {
            value >>= 1;
            shift++;
        }

        return shift;
    }

    bool full() const
    {
        return blocks[head].next == head;
    }

    void* alloc_block_data() const
    {
        return xMemAlloc(gActiveHeap, _block_size * _unit_size, 0);
    }

    U8 alloc_block();
    void free_block(U8 block);
};

inline void tier_queue_allocator::init(u32 unit_size, u32 block_size, u32 max_blocks)
{
    _unit_size = (unit_size + 3) & ~3;
    _block_size_shift = log2_ceil(block_size);
    _block_size = 1 << _block_size_shift;
    _max_blocks_shift = log2_ceil(max_blocks);
    _max_blocks = 1 << _max_blocks_shift;
    blocks = (block_data*)xMemAlloc(gActiveHeap, sizeof(block_data) * _max_blocks, 0);

    u32 i;
    u32 count = _max_blocks;

    for (i = 0; i < count; i++)
    {
        blocks[i].data = NULL;
    }

    clear();
}

inline void tier_queue_allocator::clear()
{
    head = 0;

    u32 mask = _max_blocks - 1;
    u32 count = _max_blocks;
    u32 i;

    for (i = 0; i < count; i++)
    {
        blocks[i].prev = (i - 1) & mask;
        blocks[i].next = (i + 1) & mask;
    }
}

template <class T> struct tier_queue
{
    U32 first;
    U32 _size;
    U32 wrap_mask;
    tier_queue_allocator* alloc;
    U8 blocks[256];

    struct iterator
    {
        U32 _it;
        tier_queue<T>* _owner;

        T& operator*() const
        {
            return _owner->get_at(_it);
        }

        T* operator->() const
        {
            return &operator*();
        }

        iterator* operator--()
        {
            *this -= 1;
            return this;
        }

        bool operator==(const iterator& other) const
        {
            return _it == other._it;
        }

        bool operator!=(const iterator& other) const
        {
            return _it != other._it;
        }

        iterator* operator+=(S32 value)
        {
            _it = _owner->wrap_index(_it + value);
            return this;
        }

        iterator operator-=(S32 value)
        {
            return *operator+=(-value);
        }

        iterator operator+(S32 value) const
        {
            iterator tmp = *this;
            tmp += value;
            return tmp;
        }

        iterator operator-(S32 value) const
        {
            iterator tmp = *this;
            tmp -= value;
            return tmp;
        }

        U32 global_index() const
        {
            return _it;
        }
    };

    U32 size() const
    {
        return _size;
    }

    bool empty() const
    {
        return _size == 0;
    }

    // Returns u32 with a (U8) truncation in the body, not a U8 return: a
    // U8-returning member forces clrlwi r3,r3,24 at every call site, and
    // the target's own wrap_block is `clrlwi r3,r4,24; blr`.
    u32 wrap_block(u32 block) const
    {
        return (U8)block;
    }

    u32 wrap_index(u32 index) const
    {
        return index & wrap_mask;
    }

    u32 get_block(u32 index) const
    {
        return index >> alloc->block_size_shift();
    }

    T& get_at(u32 index) const
    {
        T* data = (T*)alloc->get_block(blocks[get_block(index)]);

        return data[alloc->mod_block_size(index)];
    }

    T& operator[](S32 index)
    {
        return get_at(wrap_index(first + index));
    }

    iterator create_iterator(u32 index) const
    {
        iterator it;
        it._it = index;
        it._owner = const_cast<tier_queue<T>*>(this);
        return it;
    }

    iterator begin() const
    {
        return create_iterator(first);
    }

    iterator end() const
    {
        return create_iterator(wrap_index(first + _size));
    }

    T& front()
    {
        iterator it = begin();
        return *it;
    }

    T& back()
    {
        iterator it = end() - 1;
        return *it;
    }

    bool front_full() const
    {
        if (!alloc->full())
        {
            return false;
        }

        return alloc->mod_block_size(first) == 0;
    }

    void init(tier_queue_allocator& allocator)
    {
        first = 0;
        _size = 0;
        wrap_mask = allocator.block_size() * 256 - 1;
        alloc = &allocator;
    }

    void push_front()
    {
        _size++;

        u32 old_first = first;

        first = wrap_index(first - 1);

        if (alloc->mod_block_size(old_first) == 0)
        {
            u32 block = get_block(first);

            blocks[block] = alloc->alloc_block();
        }
    }

    void pop_back()
    {
        if (_size <= 1)
        {
            clear();
            return;
        }

        _size--;

        u32 index = wrap_index(first + _size);

        if (alloc->mod_block_size(index) == 0)
        {
            u32 block = get_block(index);

            alloc->free_block(blocks[block]);
        }
    }

    void clear();
};

template <class T> struct static_queue
{
    U32 _first;
    U32 _size;
    U32 _max_size;
    U32 _max_size_mask;
    T* _buffer;

    struct iterator
    {
        U32 _it;
        static_queue<T>* _owner;

        T& operator*() const
        {
            return _owner->_buffer[_it];
        }

        bool operator!=(const iterator& other) const
        {
            return _it != other._it;
        }

        iterator* operator+=(S32 value)
        {
            _it = _owner->mod_max_size(_it + value);
            return this;
        }

        iterator operator-=(S32 value)
        {
            return *operator+=(-value);
        }

        iterator operator-(S32 value) const
        {
            iterator tmp;
            tmp._it = _it;
            tmp._owner = _owner;
            tmp -= value;
            return tmp;
        }

        iterator* operator++()
        {
            *this += 1;
            return this;
        }
    };

    bool empty() const
    {
        return size() == 0;
    }

    U32 size() const
    {
        return _size;
    }

    void init(u32 size)
    {
        U32 shift = 0;

        size++;

        while (size > 1)
        {
            size /= 2;
            shift++;
        }

        _max_size = 1 << shift;
        _max_size_mask = _max_size - 1;
        _buffer = (T*)xMemAlloc(gActiveHeap, sizeof(T) * _max_size, 0);
        clear();
    }

    void clear()
    {
        _size = 0;
        _first = 0;
    }

    T& front()
    {
        iterator it = begin();
        return *it;
    }

    T& back()
    {
        iterator it = end() - 1;
        return *it;
    }

    iterator begin() const
    {
        return create_iterator(_first);
    }

    iterator create_iterator(u32 initial_size) const
    {
        iterator it;
        it._it = initial_size;
        it._owner = const_cast<static_queue<T>*>(this);
        return it;
    }

    T& push_front()
    {
        _size += 1;
        _first = mod_max_size(_first - 1);
        return front();
    }

    U32 mod_max_size(u32 new_max_size) const
    {
        return new_max_size & _max_size_mask;
    }

    void pop_back()
    {
        _size -= 1;
    }

    bool full() const
    {
        return size() == max_size();
    }

    U32 max_size() const
    {
        return _max_size - 1;
    }

    void erase(const iterator& it, const iterator& other)
    {
        if (it._it == _first)
        {
            // One temp, not two: the target keeps _first in the register the
            // cmplw loaded it into and adds _size straight onto it. Caching
            // _first and _size separately reads 98.103%.
            U32 last = _first + _size;
            _first = other._it;
            _size = mod_max_size(last - _first);
        }
        else
        {
            _size -= mod_max_size(other._it - it._it);
        }
    }

    iterator end() const
    {
        iterator it;
        return create_iterator(mod_max_size(_first + _size));
    }
};

template <class T, U32 N> struct fixed_queue
{
    U32 _first;
    U32 _last;
    T _buffer[N + 1];

    void reset()
    {
        clear();
    }
    void clear()
    {
        _last = 0;
        _first = 0;
    }
    T& front()
    {
        fixed_queue<T, N>::iterator it = begin();
        return *it;
    }
    void pop_front()
    {
        _first = (_first + 1) % (N + 1);
    }
    void push_front()
    {
        _first = (_first + N) & N;
    }
    void push_front(const T& data)
    {
        push_front();
        T& new_front = front();
        new_front = data;
    }
    void push_back()
    {
        _last = (_last + 1) % (N + 1);
    }
    U32 max_size() const
    {
        return N;
    }
    bool full() const
    {
        return size() == max_size();
    }
    T& back()
    {
        fixed_queue<T, N>::iterator it = end() - 1;
        return *it;
    }
    void pop_back()
    {
        _last = (_last + N) & N;
    }
    bool empty() const
    {
        return _last == _first;
    }
    U32 size() const
    {
        return (_last + (N + 1) - _first) & N;
    }
    T& get_at(S32 index) const
    {
        // Retail computes (_first + index) + 0x80. Spelling it
        // `_first + index + (N + 1)` lets CW reassociate to
        // `_first + (index + 0x80)` and the function stalls at 48.750%.
        return const_cast<fixed_queue<T, N>*>(this)->_buffer[(N + 1 + _first + index) & N];
    }
    T& operator[](S32 index)
    {
        return get_at(index);
    }

    struct iterator
    {
        U32 _it;
        fixed_queue<T, N>* _owner;

        T& operator*() const
        {
            return _owner->_buffer[_it];
        }

        T* operator->() const
        {
            // Retail is literally `bl __ml__` -- it calls operator*, it does
            // not repeat the body. Inlining the subscript here reads 15.375%.
            return &operator*();
        }

        bool operator!=(const iterator& other) const
        {
            return _it != other._it;
        }

        iterator* operator+=(S32 value)
        {
            value = _it + value;
            _it = (value + N + 1) % (N + 1);
            return this;
        }

        iterator operator-=(S32 value)
        {
            return *operator+=(-value);
        }

        iterator* operator--()
        {
            *this -= 1;
            return this;
        }

        iterator operator-(S32 value) const
        {
            iterator tmp;
            tmp._it = _it;
            tmp._owner = _owner;
            tmp -= value;
            return tmp;
        }

        iterator* operator++()
        {
            *this += 1;
            return this;
        }
    };

    iterator create_iterator(u32 initial_location) const
    {
        iterator it;
        it._it = initial_location;
        it._owner = const_cast<fixed_queue<T, N>*>(this);
        return it;
    }

    iterator begin() const
    {
        return create_iterator(_first);
    }

    iterator end() const
    {
        return create_iterator(_last);
    }
};
#endif
